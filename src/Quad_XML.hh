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

#ifndef __Quad_XML_DEFINED__
#define __Quad_XML_DEFINED__

#include "QuadFunction.hh"

class XML_node;

class Quad_XML : public QuadFunction
{
public:
   /// Constructor.
   Quad_XML()
      : QuadFunction(TOK_Quad_XML)
   {}

   static Quad_XML * fun;          ///< Built-in function.
   static Quad_XML  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB()
   Token eval_AB(Value_P A, Value_P B) const;

   /// overloaded Function::eval_B()
   Token eval_B(Value_P B) const;
};

#endif // __Quad_XML_DEFINED__

