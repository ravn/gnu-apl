/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2021  Dr. Jürgen Sauermann

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

#ifndef __Quad_JSON_DEFINED__
#define __Quad_JSON_DEFINED__

#include "QuadFunction.hh"

//------------------------------------------------------------------------------
/**
   The system function ⎕JSON
 */
/// The class implementing ⎕JSON
class Quad_JSON : public QuadFunction
{
public:
   /// Constructor.
   Quad_JSON()
      : QuadFunction(TOK_Quad_JSON)
   {}

   static Quad_JSON * fun;          ///< Built-in function.
   static Quad_JSON  _fun;          ///< Built-in function.

   static UCS_string skip_pos_prefix(const UCS_string & ucs);

   /// split src, e.g. "_2_name" into integer 2, Unicode '_', and
   /// UCS_string 'name'. Null pointers if not relevant.
   static int split_name(Unicode * category, ShapeItem * position,
                         UCS_string * name, const Value & src);
protected:
   /// convert APL associative array to JSON string
   static Value_P APL_to_JSON(const Value & B);

   /// convert JSON string to APL associative array
   static Value_P JSON_to_APL(const Value & B);

   /// overloaded Function::eval_B()
   Token eval_B(Value_P B) const;
};

#endif // __Quad_JSON_DEFINED__

