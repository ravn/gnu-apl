/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2022  Dr. Jürgen Sauermann

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

#ifndef __Bif_OPER1_COMMUTE_HH_DEFINED__
#define __Bif_OPER1_COMMUTE_HH_DEFINED__

#include "PrimitiveOperator.hh"

//----------------------------------------------------------------------------
/** Primitive operator ⍨ (commute/duplicate)
 */
/// The class implementing ⍨
class Bif_OPER1_COMMUTE : public PrimitiveOperator
{
public:
   /// Constructor.
   Bif_OPER1_COMMUTE() : PrimitiveOperator(TOK_OPER1_COMMUTE) {}

   /// Overloaded Function::eval_LB().
   virtual Token eval_LB(Token & LO, Value_P B) const;

   /// Overloaded Function::eval_LXB().
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B) const;

   /// Overloaded Function::eval_ALB().
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B) const;

   /// Overloaded Function::eval_ALXB().
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B) const;

   static Bif_OPER1_COMMUTE * fun;      ///< Built-in function.
   static Bif_OPER1_COMMUTE  _fun;      ///< Built-in function.

protected:
};
//----------------------------------------------------------------------------

#endif // __Bif_OPER1_COMMUTE_HH_DEFINED__
