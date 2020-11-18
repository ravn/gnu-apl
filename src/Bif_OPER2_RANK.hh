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

#ifndef __BIF_OPER2_RANK_HH_DEFINED__
#define __BIF_OPER2_RANK_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/** Primitive operator ⍤ (rank)
 */
/// The class implementing ⍤
class Bif_OPER2_RANK : public PrimitiveOperator
{
public:
   /// constructor
   Bif_OPER2_RANK() : PrimitiveOperator(TOK_OPER2_RANK) {}

   /// overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B) const;

   /// overloaded Function::eval_ALRXB()
   virtual Token eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X, Value_P B) const;

   /// the 'normalized' implementation of all eval_ALxxx*( functions
   static Token do_ALyXB(Value_P A, Rank rk_chunk_A, Token & LO,
                         Value_P X, Value_P B, Rank rk_chunk_B);

   /// overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO, Value_P B) const;

   /// overloaded Function::eval_LRXB()
   virtual Token eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B) const;

   /// overloaded Function::may_push_SI()
   virtual bool may_push_SI() const
      { return false; }

   /// the 'normalized' implementation of all eval_Lxxx*( functions
   static Token do_LyXB(Token & LO, Value_P X, Value_P B, Rank rk_chunkB);

   /// split j B into j and B
   static void split_y123_B(Value_P y123_B, Value_P & y123, Value_P & B);

   static Bif_OPER2_RANK * fun;      ///< Built-in function
   static Bif_OPER2_RANK  _fun;      ///< Built-in function

protected:
   /// convert 1- 2- or 3-element vector y123 to chunk-rank of B
   static void y123_to_B(Value_P y123, Rank & rk_B);

   /// convert 1- 2- or 3-element vector y123 to chunk-ranks of A and B
   static void y123_to_AB(Value_P y123, Rank & rk_A, Rank & rk_B);
};
//-----------------------------------------------------------------------------

#endif // __BIF_OPER2_RANK_HH_DEFINED__
