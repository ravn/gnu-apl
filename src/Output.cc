/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../config.h"   // for HAVE_ macros from configure

#if HAVE_CURSES_H && HAVE_CURSES_H

# if defined(__sun) && defined(__SVR4)
#  define NOMACROS
# endif

# include <curses.h>
# include <term.h>

// curses #defines erase() and tab() on Solaris, conflicting with eg.
// vector::erase() and others
# ifdef erase
#  undef erase
# endif

# ifdef tab
#  undef tab
# endif

#else // not (HAVE_CURSES_H && HAVE_CURSES_H)

# define tputs(x, y, z)
# define setupterm(x, y, z) -1

#endif

#include "Command.hh"
#include "Common.hh"
#include "DiffOut.hh"
#include "InputFile.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Svar_DB.hh"

bool Output::colors_enabled = false;
bool Output::colors_changed = false;
bool Output::use_curses = true;

int Output::color_CIN_foreground = 0;
int Output::color_CIN_background = 7;
int Output::color_COUT_foreground = 0;
int Output::color_COUT_background = 8;
int Output::color_CERR_foreground = 5;
int Output::color_CERR_background = 8;
int Output::color_UERR_foreground = 5;
int Output::color_UERR_background = 8;

/// a filebuf for stdin echo
class CinOut : public filebuf
{
   /// overloaded filebuf::overflow
   virtual int overflow(int c);
} cin_filebuf;

/// a filebuf for stderr output
class ErrOut : public filebuf
{
protected:
   /// overloaded filebuf::overflow()
   virtual int overflow(int c);

public:
   /** a helper function telling whether the constructor for CERR was called
       if CERR is used before its constructor was called (which can happen in
       when constructors of static objects are called and use CERR) then a
       segmentation fault would occur.

       We avoid that by using get_CERR() instead of CERR in such constructors.
       get_CERR() checks \b used and returns cerr instead of CERR if it is
       false.
    **/
   filebuf * use()   { used = true;   return this; }

   /// true iff the constructor for CERR was called
   static bool used;   // set when CERR is constructed

} cerr_filebuf;   ///< a filebuf for CERR

bool ErrOut::used = false;

DiffOut dout_filebuf(false);
DiffOut uerr_filebuf(true);

// Android defines its own CIN, COUT, CERR, and UERR ostreams
#ifndef WANT_ANDROID

ostream CIN (&cin_filebuf);
ostream COUT(&dout_filebuf);
ostream CERR(cerr_filebuf.use());
ostream UERR(&uerr_filebuf);

#endif

extern ostream & get_CERR();
ostream & get_CERR()
{
   return ErrOut::used ? CERR : cerr;
};

Output::ColorMode Output::color_mode = COLM_UNDEF;

/// CSI sequence for ANSI/VT100 escape sequences (ESC [)
#define CSI "\x1b["

/// VT100 escape sequence to change to cin color
char Output::color_CIN[100] = CSI "0;30;47m";

/// VT100 escape sequence to change to cout color
char Output::color_COUT[100] = CSI "0;30;48;2;255;255;255m";

/// VT100 escape sequence to change to cerr color
char Output::color_CERR[100] = CSI "0;35;48;2;255;255;255m";

/// VT100 escape sequence to change to uerr color
char Output::color_UERR[100] = CSI "0;35;48;2;255;255;255m";

/// VT100 escape sequence to reset colors to their default
char Output::color_RESET[100] = CSI "0;30;48;2;255;255;255m";

/// VT100 escape sequence to clear to end of line
char Output::clear_EOL[100] = CSI "K";

//-----------------------------------------------------------------------------
int
CinOut::overflow(int c)
{
   if (!InputFile::echo_current_file())   return 0;

   Output::set_color_mode(Output::COLM_INPUT);
   cerr << (char)c;
   return 0;
}
//-----------------------------------------------------------------------------
int
ErrOut::overflow(int c)
{
   Output::set_color_mode(Output::COLM_ERROR);
   cerr << (char)c;
   return 0;
}
//-----------------------------------------------------------------------------
void
Output::init()
{
const int ret = setupterm(NULL, fileno(stdout), NULL);
   if (ret != 0)
      {
        use_curses = false;
      }
}
//-----------------------------------------------------------------------------
int
Output::putc_stderr(TPUTS_arg3 ch)
{
   cerr << (char)ch;
   return ch;
}
//-----------------------------------------------------------------------------
int
Output::putc_stdout(TPUTS_arg3 ch)
{
   cout << (char)ch;
   return ch;
}
//-----------------------------------------------------------------------------
void
Output::reset_dout()
{
   dout_filebuf.reset();
}
//-----------------------------------------------------------------------------
void
Output::reset_colors()
{
   if (!colors_changed)   return;

   if (use_curses)
      {
        tputs(exit_attribute_mode, 1, putc_stdout);
        tputs(clr_eol, 1, putc_stdout);

        tputs(exit_attribute_mode, 1, putc_stderr);
        tputs(clr_eol, 1, putc_stderr);
      }
   else
      {
        cout << color_RESET << clear_EOL;
        cerr << color_RESET << clear_EOL;
      }
}
//-----------------------------------------------------------------------------
/// make Solaris happy
#define tparm10(x, y) tparm(x, y, 0, 0, 0, 0, 0, 0, 0, 0)

void
Output::set_color_mode(Output::ColorMode mode)
{
   if (!colors_enabled)      return;   // colors disabled
   if (color_mode == mode)   return;   // no change in color mode

   // mode changed
   //
   color_mode = mode;
   colors_changed = true;   // to reset them in reset_colors()

   if (use_curses)
      {
        switch(color_mode)
           {
             case COLM_INPUT:
                  tputs(tparm10(set_foreground, color_CIN_foreground),
                              1, putc_stdout);
                  tputs(tparm10(set_background, color_CIN_background),
                              1, putc_stdout);
                  tputs(clr_eol, 1, putc_stdout);
                  break;

             case COLM_OUTPUT:
                  tputs(tparm10(set_foreground, color_COUT_foreground),
                              1, putc_stdout);
                  tputs(tparm10(set_background, color_COUT_background),
                              1, putc_stdout);
                  tputs(clr_eol, 1, putc_stdout);
                  break;

             case COLM_ERROR:
                  tputs(tparm10(set_foreground, color_CERR_foreground),
                              1, putc_stderr);
                  tputs(tparm10(set_background, color_CERR_background),
                              1, putc_stderr);
                  tputs(clr_eol, 1, putc_stderr);
                  break;

             case COLM_UERROR:
                  tputs(tparm10(set_foreground, color_UERR_foreground),
                              1, putc_stdout);
                  tputs(tparm10(set_background, color_UERR_background),
                              1, putc_stdout);
                  tputs(clr_eol, 1, putc_stdout);
                  break;

             default: break;
           }
      }
   else
      {
        switch(color_mode)
           {
             case COLM_INPUT:  cerr << color_CIN  << clear_EOL;   break;
             case COLM_OUTPUT: cout << color_COUT << clear_EOL;   break;
             case COLM_ERROR:  cerr << color_CERR << clear_EOL;   break;
             case COLM_UERROR: cout << color_UERR << clear_EOL;   break;
             default: break;
           }
      }
}
//-----------------------------------------------------------------------------
void 
Output::toggle_color(const UCS_string & arg)
{
int a = 0;
   while (a < arg.size() && arg[a] < UNI_ASCII_SPACE)   ++a;

   if (arg.starts_iwith("ON"))         colors_enabled = true;
   else if (arg.starts_iwith("OFF"))   colors_enabled = false;
   else                                colors_enabled = !colors_enabled;
}
//-----------------------------------------------------------------------------
