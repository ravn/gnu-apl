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

#ifndef __BIF_COMMA_HH_DEFINED__
#define __BIF_COMMA_HH_DEFINED__

#include "PrimitiveFunction.hh"

//-----------------------------------------------------------------------------
/** Comma related functions (catenate, laminate, and ravel.) */
/// Base class for , and ⍪
class Bif_COMMA : public NonscalarFunction
{
public:
   /// Constructor
   Bif_COMMA(TokenTag tag)
   : NonscalarFunction(tag)
   {}

   /// ravel along axis, with axis being the first (⍪( or last (,) axis of B
   static Token ravel_axis(Value_P X, Value_P B, uAxis axis);

   /// Return the ravel of B as APL value
   static Token ravel(const Shape & new_shape, Value_P B);

   /// Catenate A and B
   static Token catenate(Value_P A, Axis axis, Value_P B);
   /// Laminate A and B
   static Token laminate(Value_P A, Axis axis, Value_P B);

   /// Prepend scalar cell_A to B along axis
   static Value_P prepend_scalar(const Cell & cell_A, uAxis axis, Value_P B);

   /// Prepend scalar cell_B to A along axis
   static Value_P append_scalar(Value_P A, uAxis axis, const Cell & cell_B);
};
//-----------------------------------------------------------------------------
/** primitive functions catenate, laminate, and ravel along last axis */
/// The class implementing ,
class Bif_F12_COMMA : public Bif_COMMA
{
public:
   /// Constructor
   Bif_F12_COMMA()
   : Bif_COMMA(TOK_F12_COMMA)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B) const;

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B) const;

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B) const
      { return ravel_axis(X, B, B->get_rank()); }

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B) const;

   static Bif_F12_COMMA * fun;   ///< Built-in function
   static Bif_F12_COMMA  _fun;   ///< Built-in function

protected:
};
//-----------------------------------------------------------------------------
/** primitive functions catenate and laminate along first axis, table */
/// The class implementing ⍪
class Bif_F12_COMMA1 : public Bif_COMMA
{
public:
   /// Constructor
   Bif_F12_COMMA1()
   : Bif_COMMA(TOK_F12_COMMA1)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B) const;

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B) const;

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B) const
      { return ravel_axis(X, B, 0); }

  /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B) const;

   static Bif_F12_COMMA1 * fun;   ///< Built-in function
   static Bif_F12_COMMA1  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------

#endif // __BIF_COMMA_HH_DEFINED__

