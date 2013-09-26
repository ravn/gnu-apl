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

#ifndef __OUTPUT_HH_DEFINED__
#define __OUTPUT_HH_DEFINED__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "Assert.hh"
#include "UCS_string.hh"

using namespace std;

/*
 The classes below are used to combine normal user I/O and automatic
 testcase execution. It works like this:

       (cin)
         |
         |
         V
     +---+---+             +-------------------+
     | Input | <--------   | testcase files(s) |
     +---+---+             +---------+---------+
         |                           |
         |                           |
         V                           |
      +--+--+                        |
      | APL |                        |
      +--+--+                        |
         |                           |
         |                           |
         +-----------------------+   |
         |                       |   |
         |                       V   V
        |                    +--+---+--+
         |                    | compare |
         |                    +----+----+
         |                         |
         |                         |
         V                         V
       (cout)                (test results)
 
 */

/** the output of the APL interpreter to cout and maybe to a test result file
 **/
class Output
{
public:
   /// the color to be usedd for output
   enum ColorMode
      {
         COLM_UNDEF,    ///< color undefined
         COLM_INPUT,    ///< color for (echo of) input characters (CIN)
         COLM_OUTPUT,   ///< color for normal APL output (COUT)
         COLM_ERROR,    ///< color for debug output (CERR)
      };

   /// set the color mode (if colors_enabled). Outputs the escape sequence
   /// for \b mode when the color mode changes
   static void set_color_mode(ColorMode mode);

   /// reset() dout_filebuf
   static void reset_dout();

   /// reset colors to black and white
   static void reset_colors();

   /// toggle color mode (implementation of command ]XTERM)
   static void toggle_color(ostream & out, const UCS_string & arg);

   /// true if the print semaphore was acquired
   static bool print_sema_held;

protected:
   /// the current color mode
   static ColorMode color_mode;

   /// true if colors are currently enabled (by XTERM command)
   static bool colors_enabled;
};

/// normal APL output
extern ostream COUT;

/// echo of APL input
extern ostream CIN;

#endif // __OUTPUT_HH_DEFINED__
