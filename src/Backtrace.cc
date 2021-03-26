/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2020  Dr. Jürgen Sauermann

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

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Common.hh"   // for HAVE_EXECINFO_H
#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
# include <cxxabi.h>
#define EXEC(x) x
#else
#define EXEC(x)
#endif

#include "Common.hh"
#include "Backtrace.hh"

using namespace std;

Backtrace::_lines_status 
       Backtrace::lines_status = Backtrace::LINES_not_checked;

std::vector<Backtrace::PC_src> Backtrace::pc_2_src;

//-----------------------------------------------------------------------------
int
Backtrace::pc_cmp(const int64_t & key, const PC_src & pc_src, const void *)
{
   return key - pc_src.pc;
}
//-----------------------------------------------------------------------------
const char *
Backtrace::find_src(int64_t pc)
{
   if (pc == -1LL)   return 0;   // no pc

   if (pc_2_src.size())   // database was properly set up.
      {
        if (const PC_src * posp = Heapsort<Backtrace::PC_src>
                           ::search<const int64_t &>(pc, pc_2_src, &pc_cmp, 0))
        return posp->src_loc;   // found
      }

   return 0;   // not found
}
//-----------------------------------------------------------------------------
void
Backtrace::open_lines_file()
{
   if (lines_status != LINES_not_checked)   return;

   // we are here for the first time.
   // Assume that apl.lines is outdated until proven otherwise.
   //
   lines_status = LINES_outdated;

struct stat st;
   if (stat("apl", &st))   return;   // stat apl failed

const time_t apl_time = st.st_mtime;

const int fd = open("apl.lines", O_RDONLY);
   if (fd == -1)   return;

   if (fstat(fd, &st))   // stat apl.lines failed
      {
        close(fd);
        return;
      }

const time_t apl_lines_time = st.st_mtime;

   if (apl_lines_time < apl_time)
      {
#ifndef Makefile__srcdir
# define Makefile__srcdir "src"
#endif
        close(fd);
        get_CERR() << endl
                   << "Cannot show line numbers, since apl.lines is older "
                      "than apl." << endl
                   << "Consider running 'make apl.lines' in directory " << endl
                   << Makefile__srcdir " to fix this" << endl;
        return;
      }

   pc_2_src.reserve(100000);
char buffer[1000];
FILE * file = fdopen(fd, "r");
   assert(file);

const char * src_line = 0;
bool new_line = false;
int64_t prev_pc = -1LL;

   for (;;)
       {
         const char * s = fgets(buffer, sizeof(buffer) - 1, file);
         if (s == 0)   break;   // end of file.

         buffer[sizeof(buffer) - 1] = 0;
         int slen = strlen(buffer);
         if (slen && buffer[slen - 1] == '\n')   buffer[--slen] = 0;
         if (slen && buffer[slen - 1] == '\r')   buffer[--slen] = 0;

         // we look for 2 types of line: abolute source file paths
         // and code lines.
         //
         if (s[0] == '/')   // source file path
            {
              src_line = strdup(strrchr(s, '/') + 1);
              new_line = true;
              continue;
            }

         if (!new_line)   continue;

         if (s[8] == ':')   // opcode address
            {
              long long pc = -1LL;
              if (1 == sscanf(s, " %llx:", &pc))
                 {
                   if (pc < prev_pc)
                      {
                         assert(0 && "apl.lines not ordered by PC");
                         break;
                      }

                   PC_src pcs = { pc, src_line };
                   pc_2_src.push_back(pcs);
                   prev_pc = pc;
                   new_line = false;
                 }
              continue;
            }
       }

   fclose(file);   // also closes fd

   cerr << "read " << pc_2_src.size() << " line numbers" << endl;
   lines_status = LINES_valid;
}
//-----------------------------------------------------------------------------
void
Backtrace::show_item(int idx, char * s)
{
#ifdef HAVE_EXECINFO_H
  //
  // s looks like this:
  //
  // ./apl(_ZN10APL_parser9nextTokenEv+0x1dc) [0x80778dc]
  //

   // split off address from s.
   //
const char * addr = "????????";
int64_t pc = -1LL;
   {
     char * space = strchr(s, ' ');
     if (space)
        {
          *space = 0;
          addr = space + 2;
          char * e = strchr(space + 2, ']');
          if (e)   *e = 0;
          pc = strtoll(addr, 0, 16);
        }
   }

const char * src_loc = find_src(pc);

   // split off function from s.
   //
char * fun = 0;
   {
    char * opar = strchr(s, '(');
    if (opar)
       {
         *opar = 0;
         fun = opar + 1;
          char * e = strchr(opar + 1, ')');
          if (e)   *e = 0;
       }
   }

   // split off offset from fun.
   //
int offs = 0;
   if (fun)
      {
       char * plus = strchr(fun, '+');
       if (plus)
          {
            *plus++ = 0;
            sscanf(plus, "%X", &offs);
          }
      }

char obuf[200] = "@@@@";
   if (fun)
      {
        strncpy(obuf, fun, sizeof(obuf) - 1);
        obuf[sizeof(obuf) - 1] = 0;

       size_t obuflen = sizeof(obuf) - 1;
       int status = 0;
//     cerr << "mangled fun is: " << fun << endl;
       __cxxabiv1::__cxa_demangle(fun, obuf, &obuflen, &status);
       switch(status)
          {
            case 0: // demangling succeeded
                 break;

            case -2: // not a valid name under the C++ ABI mangling rules.
                 break;

            default:
                 cerr << "__cxa_demangle() returned " << status << endl;
                 break;
          }
      }

// cerr << setw(2) << idx << ": ";

   cerr << HEX(pc);

// cerr << left << setw(20) << s << right << " ";

   // indent.
   for (int i = -1; i < idx; ++i)   cerr << " ";

   cerr << obuf;

// if (offs)   cerr << " +" << offs;

   if (src_loc)   cerr << " at " << src_loc;
   cerr << endl;
#endif
}
//-----------------------------------------------------------------------------
void
Backtrace::show(const char * file, int line)
{
   // CYGWIN, for example, has no execinfo.h and the functions declared there
   //
#ifndef HAVE_EXECINFO_H
   cerr << "Cannot show function call stack since execinfo.h seems not"
           " to exist on this OS (WINDOWs ?)." << endl;
   return;

#else

   open_lines_file();

void * buffer[200];
const int size = backtrace(buffer, sizeof(buffer)/sizeof(*buffer));

char ** strings = backtrace_symbols(buffer, size);

   cerr << endl
        << "----------------------------------------"  << endl
        << "-- Stack trace at " << file << ":" << line << endl
        << "----------------------------------------"  << endl;

   if (strings == 0)
      {
        cerr << "backtrace_symbols() failed. Using backtrace_symbols_fd()"
                " instead..." << endl << endl;
        // backtrace_symbols_fd(buffer, size, STDERR_FILENO);
        for (int b = size - 1; b > 0; --b)
            {
              for (int s = b + 1; s < size; ++s)   cerr << " ";
                  backtrace_symbols_fd(buffer + b, 1, STDERR_FILENO);
            }
        cerr << "========================================" << endl;
        return;
      }

   for (int i = 1; i < size - 1; ++i)
       {
         // make a copy of strings[i] that can be
         // messed up in show_item().
         //
         const char * si = strings[size - i - 1];
         char cc[strlen(si) + 1];
         strcpy(cc, si);
         cc[sizeof(cc) - 1] = 0;
         show_item(i - 1, cc);
       }
   cerr << "========================================" << endl;

#if 0
   // crashes at times
   free(strings);   // but not strings[x] !
#endif

#endif
}
//-----------------------------------------------------------------------------
#ifdef HAVE_EXECINFO_H
int
Backtrace::demangle_line(char * result, size_t result_max, const char * buf)
{
std::string tmp;
   tmp.reserve(result_max + 1);
   for (const char * b = buf; *b &&  b < (buf + result_max); ++b)
       tmp += *b;
   tmp.append(0);

char * e = 0;
int status = 3;
char * p = strchr(&tmp[0], '(');
   if (p == 0)   goto error;
   else          ++p;

   e = strchr(p, '+');
   if (e == 0)   goto error;
   else *e = 0;

// cerr << "mangled fun is: " << p << endl;
   __cxxabiv1::__cxa_demangle(p, result, &result_max, &status);
   if (status)   goto error;
   return 0;

error:
   strncpy(result, buf, result_max);
   result[result_max - 1] = 0;
   return status;
}
//-----------------------------------------------------------------------------
const char *
Backtrace::caller(int offset)
{
void * buffer[200];
const int size = backtrace(buffer, sizeof(buffer)/sizeof(*buffer));
char ** strings = backtrace_symbols(buffer, size);

char * demangled = static_cast<char *>(malloc(200));
   if (demangled)
       {
         *demangled = 0;
         demangle_line(demangled, 200, strings[offset]);
         return demangled;
       }
   return strings[offset];
}
//-----------------------------------------------------------------------------
#endif // HAVE_EXECINFO_H
