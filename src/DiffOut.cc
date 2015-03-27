/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#include "Avec.hh"
#include "Common.hh"
#include "DiffOut.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "Output.hh"
#include "Performance.hh"
#include "UTF8_string.hh"

//-----------------------------------------------------------------------------
int
DiffOut::overflow(int c)
{
PERFORMANCE_START(cout_perf)
   Output::set_color_mode(errout ? Output::COLM_UERROR : Output::COLM_OUTPUT);
   cout << char(c);

   if (c != '\n')   // not end of line
      {
        aplout.append(c);
        PERFORMANCE_END(fs_COUT_B, cout_perf, 1)
        return 0;
      }

   if (!InputFile::is_validating())
      {
        PERFORMANCE_END(fs_COUT_B, cout_perf, 1)
        return 0;
      }

ofstream & rep = IO_Files::get_current_testreport();
   Assert(rep.is_open());

const char * apl = aplout.c_str();
UTF8_string ref;
bool eof = false;
   IO_Files::read_file_line(ref, eof);
   if (eof)   // nothing in current_testfile
      {
        rep << "extra: " << apl << endl;
        PERFORMANCE_END(fs_COUT_B, cout_perf, 1)
        return 0;
      }

   // print common part.
   //
   if (different((const UTF8 *)apl, (const UTF8 *)ref.c_str()))   // different
      {
        IO_Files::diff_error();
        rep << "apl: ⋅⋅⋅" << apl << "⋅⋅⋅" << endl
            << "ref: ⋅⋅⋅" << ref.c_str() << "⋅⋅⋅" << endl;
      }
   else                    // same
      {
        rep << "== " << apl << endl;
      }

   aplout.clear();
   PERFORMANCE_END(fs_COUT_B, cout_perf, 1)
   return 0;
}
//-----------------------------------------------------------------------------
bool
DiffOut::different(const UTF8 * apl, const UTF8 * ref)
{
   for (;;)
       {
         int len_apl, len_ref;

         const Unicode a = UTF8_string::toUni(apl, len_apl);
         const Unicode r = UTF8_string::toUni(ref, len_ref);
         apl += len_apl;
         ref += len_ref;

         if (a == r)   // same char
            {
              if (a == 0)   return false;   // not different
              continue;
            }

         // different chars: try special matches (⁰, ¹, ², or ⁿ)...

         if (r == UNI_PAD_U0)   // ⁰: match one or more digits
            {
              if (!Avec::is_digit(a))   return true;   // different

              // skip trailing digits
              while (Avec::is_digit(Unicode(*apl)))   ++apl;
              continue;
            }

         if (r == UNI_PAD_U1)   // ¹: match zero or more spaces
            {
              if (a != UNI_ASCII_SPACE)   return true;   // different

              // skip trailing digits
              while (*apl == ' ')   ++apl;
              continue;
            }

         if (r == UNI_PAD_U2)   // ²: match floating point number
            {
              if (!Avec::is_digit(a) &&
                  a != UNI_OVERBAR   &&
                  a != UNI_ASCII_E   &&
                  a != UNI_ASCII_J   &&
                  a != UNI_ASCII_FULLSTOP)   return true;   // different

              // skip trailing digits
              //
              for (;;)
                  {
                    if (Avec::is_digit(Unicode(*apl)) ||
                        *apl == UNI_ASCII_E           ||
                        *apl == UNI_ASCII_J           ||
                        *apl == UNI_ASCII_FULLSTOP)   ++apl;
                   else
                      {
                         int len;
                         if (UTF8_string::toUni(apl, len) == UNI_OVERBAR)
                            {
                              apl += len;
                              continue;
                            }
                         else break;
                      }
                  }

              continue;
            }

         if (r == UNI_PAD_U3)   // ³: match anything
            return false;   // not different

         if (r == UNI_COMMENT)   // maybe ⍝³: optional line
            {
              const Unicode r1 = UTF8_string::toUni(ref + len_ref, len_ref);
              return r1 == UNI_PAD_U3;
            }

         if (r == UNI_PAD_U4)   // ⁴: match optional ¯
            {
              if (a != UNI_OVERBAR)   apl -= len_apl;   // restore apl
              continue;
            }

         if (r == UNI_PAD_U5)   // ⁵: match + or -
            {
              if (a != '+' && a != '-')   return true;   // different
              continue;
            }

         if (r == UNI_PAD_Un)   // ⁿ: optional unit multiplier
            {
              // ⁿ shall match an optional  unit multiplier, ie.
              // m, n, u, or μ
              //
              if (a == 'm')   continue;
              if (a == 'n')   continue;
              if (a == 'u')   continue;
              if (a == UNI_mue)   continue;

              // no unit matches as well
              apl -= len_apl;
              continue;
            }

         return true;   // different
       }

   return false;   // not different
}
//-----------------------------------------------------------------------------
