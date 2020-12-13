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

#include <stdlib.h>

#include "Quad_RL.hh"
#include "Value.hh"
#include "IntCell.hh"

uint64_t Quad_RL::state = 0;

//=============================================================================
Quad_RL::Quad_RL()
   : SystemVariable(ID_Quad_RL)
{
Value_P value(LOC);

const unsigned int seed = reset_seed();

   new (value->next_ravel()) IntCell(seed);
   value->check_value(LOC);

   Symbol::assign(value, false, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_RL::assign(Value_P value, bool clone, const char * loc)
{
   if (!value->is_scalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_near_int();

   state = val;
   Symbol::assign(IntScalar(val, LOC), false, LOC);
}
//-----------------------------------------------------------------------------
uint64_t
Quad_RL::get_random()
{
   state *= Knuth_a;
   state += Knuth_c;

   Assert(value_stack.size());
   if (value_stack.back().get_nc() != NC_VARIABLE)   VALUE_ERROR;

   new (&value_stack.back().apl_val->get_ravel(0))   IntCell(state);
   return state;
}
//-----------------------------------------------------------------------------
void
Quad_RL::push()
{
   // clone the current state
   //
   Symbol::push_value(IntScalar(state, LOC));
}
//-----------------------------------------------------------------------------
void
Quad_RL::pop()
{
   Symbol::pop();
   state = value_stack.back().apl_val->get_ravel(0).get_near_int();
}
//=============================================================================
