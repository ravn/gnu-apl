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

#include <errno.h>
#include <string.h>

#include <vector>

#include "Avec.hh"
#include "Quad_FIO.hh"
#include "Quad_JSON.hh"
#include "Value.hh"
#include "Workspace.hh"

Quad_JSON  Quad_JSON::_fun;
Quad_JSON * Quad_JSON::fun = &Quad_JSON::_fun;

//=============================================================================
Token
Quad_JSON::eval_B(Value_P B) const
{
   if (B->is_structured())   // associative B array to JSON string
      {
        Value_P Z = APL_to_JSON(B.getref());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() != 1)   RANK_ERROR;

Value_P Z = JSON_to_APL(B.getref());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_JSON::APL_to_JSON(const Value & B)
{
   TODO;
}
//-----------------------------------------------------------------------------
Value_P
Quad_JSON::JSON_to_APL(const Value & B)
{
const UCS_string ucs_B(B);

std::vector<ShapeItem> parser_stack;
   loop(b, ucs_B.size())
       {
         switch(ucs_B[b])
            {
              case '"': parser_stack.push_back(b);   break;
              case '[':
              case ']':
              case ',': break;

              default: break;
            }


       }

   TODO;
}
//=============================================================================
