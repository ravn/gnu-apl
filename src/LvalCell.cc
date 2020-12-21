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

#include "Backtrace.hh"
#include "LvalCell.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
LvalCell::LvalCell(Cell * cell, Value * cell_owner)
{
   value.lval = cell;
   value.pval.owner = cell_owner;
   check_consistency();
}
//-----------------------------------------------------------------------------
void
LvalCell::init_other(void * other, Value &, const char * loc) const
{
   new (other)  LvalCell(get_lval_value(), get_cell_owner());
}
//-----------------------------------------------------------------------------
Cell *
LvalCell::get_lval_value()  const
{
  return value.lval;
}
//-----------------------------------------------------------------------------
PrintBuffer
LvalCell::character_representation(const PrintContext & pctx) const
{
   if (!value.lval)
      {
        UTF8_string utf("(Cell * 0)");
        UCS_string ucs(utf);
        ColInfo ci;
        PrintBuffer pb(ucs, ci);
        return pb;
      }

PrintBuffer pb = value.lval->character_representation(pctx);
   pb.pad_l(Unicode('='), 1);
   pb.pad_r(Unicode('='), 1);
   return pb;
}
//-----------------------------------------------------------------------------
void
LvalCell::check_consistency() const
{
  if (value.lval)                      // valid owner
     {
        const Cell * C0 = &value.pval.owner->get_ravel(0);
        const Cell * CN = C0 + value.pval.owner->nz_element_count();
       if (value.lval < C0 || value.lval >= CN)   // wrong owner
          {
            Q1(C0)
            Q1(value.lval)
            Q1(CN)
            Assert(0 && "LvalCell::check_consistency() failed");
            BACKTRACE;
          }
     }
  else Assert(value.pval.owner == 0);   // no owner
}

//-----------------------------------------------------------------------------

