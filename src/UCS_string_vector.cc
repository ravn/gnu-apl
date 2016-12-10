/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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
#include "UCS_string_vector.hh"
#include "Workspace.hh"

#include "Value.icc"

//----------------------------------------------------------------------------
UCS_string_vector::UCS_string_vector(const Value & val, bool surrogate)
{
const ShapeItem var_count = val.get_rows();
const ShapeItem name_len = val.get_cols();

   loop(v, var_count)
      {
        ShapeItem nidx = v*name_len;
        const ShapeItem end = nidx + name_len;
        UCS_string name;
        loop(n, name_len)
           {
             const Unicode uni = val.get_ravel(nidx++).get_char_value();

             if (n == 0 && Avec::is_quad(uni))   // leading ⎕
                {
                  name.append(uni);
                  continue;
                }

             if (Avec::is_symbol_char(uni))   // valid symbol char
                {
                  name.append(uni);
                  continue;
                }
             // end of (first) name reached. At this point we expect either
             // spaces until 'end' or some spaces and another name.
             //
             if (uni != UNI_ASCII_SPACE)
                {
                  name.shrink(0);
                  break;
                }

             // we have reached the end of the first name. At this point
             // there could be:
             //
             // 1. spaces until 'end' (= one name), or
             // 2. a second name (alias)

             // skip spaces from nidx and subsequent spaces
             //
             while (nidx < end &&
                    val.get_ravel(nidx).get_char_value() == UNI_ASCII_SPACE)
                   ++nidx;


             if (nidx == end)   break;   // only spaces (no second name)

             // at this point we maybe have the start of a second name, which
             // is an error (if last is false) or not. In both cases the first
             // name can be ignored.
             //
             name.shrink(0);
             if (!surrogate)   break;   // error

             // 'last' is true thus to_varnames() was called from ⎕SVO and
             // the line may contains two variable names.
             // Return the second (i.e. surrogate name)
             //
             surrogate = false;
             while (nidx < end)
                {
                  const Unicode uni = val.get_ravel(nidx++).get_char_value();
                  if (Avec::is_symbol_char(uni))   // valid symbol char
                     {
                       name.append(uni);
                     }
                  else if (uni == UNI_ASCII_SPACE)
                     {
                       break;
                     }
                  else
                     {
                       name.shrink(0);   // error
                       break;
                     }
                }
             break;
           }
        append(name);
      }
}
//----------------------------------------------------------------------------
Simple_string<int, false>
UCS_string_vector::compute_column_width(int tab_size)
{
const int quad_PW = Workspace::get_PW();

Simple_string<int, false> result;

   if (size() < 2)
      {
        if (size())   result.append(strings[0]->size());
        else          result.append(quad_PW);
        return result;
      }

   // compute block counts (one block having tab_size characters)
   //
const int max_blocks = (quad_PW + 1) / tab_size;
Simple_string<int, false> name_blocks;
   loop(n, size())
       {
         name_blocks.append(1 + (1 + strings[n]->size()) / tab_size);
       }

   // compute max number of column blocks based on first line blocks
   //
int max_col = -1;
   {
     int blocks = 0;
     loop(n, size())
         {
           if ((blocks + name_blocks[n]) < max_blocks)   // name_blocks[n] fits
              blocks += name_blocks[n];
           else                                          // max_blocks exceeded
             {
               max_col = n - 1;
               break;
             }
         }

     if (max_col == -1)   // all blocks fit
        {
          result.shrink(0);
          loop(n, size())
              {
                result.append(name_blocks[n]);
              }
          return result;
        }
   }

   // decrease max_col until all names fit...
   //
   for (;max_col > 1; --max_col)
      {
        result.shrink(0);
        int free_blocks = max_blocks;
        loop(n, size())   // try to fit blocks into result
            {
              const int col_n = n % max_col;
              const int bn = name_blocks[n];
              if (n < max_col)   // first row: append column
                 {
                   free_blocks -= bn;
                   if (free_blocks < 0)   break;
                   result.append(bn);
                 }
              else if (bn > result[col_n])
                 {
                   free_blocks -= bn - result[col_n];
                   if (free_blocks < 0)   break;
                   result[col_n] = bn;
                 }
            }

        if (free_blocks >= 0)   return result;   // success
      }

   // single colums
   //
   result.shrink(0);
int max_nb = 0;
   loop(n, size())
      {
        if (max_nb < name_blocks[n])   max_nb = name_blocks[n];
      }
   result.append(max_nb);
   return result;
}
//----------------------------------------------------------------------------
