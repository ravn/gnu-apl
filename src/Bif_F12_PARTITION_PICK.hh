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

#ifndef __Bif_F12_PARTITION_PICK_HH_DEFINED__
#define __Bif_F12_PARTITION_PICK_HH_DEFINED__

#include "Common.hh"
#include "PrimitiveFunction.hh"

//=============================================================================
/** primitive functions partition and enclose */
/// The class implementing ⊂
class Bif_F12_PARTITION : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_PARTITION()
   : NonscalarFunction(TOK_F12_PARTITION)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B) const
      { return Token(TOK_APL_VALUE1, do_eval_B(B)); }

   /// implementation of eval_B()
   static Value_P do_eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B) const
      { return partition(A, B, B->get_rank() - 1); }

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B) const
      { return Token(TOK_APL_VALUE1, do_eval_XB(X, B)); }

   /// implementation of eval_XB()
   static Value_P do_eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B) const;

   static Bif_F12_PARTITION * fun;   ///< Built-in function
   static Bif_F12_PARTITION  _fun;   ///< Built-in function

   /// enclose_with_axes
   static Value_P enclose_with_axes(const Shape & shape_X, Value_P B);

protected:
   /// enclose B
   static Token enclose(Value_P B);

   /// enclose B
   static Token enclose_with_axis(Value_P B, Value_P X);

   /// Partition B according to A
   Token partition(Value_P A, Value_P B, Axis axis) const;

   /// Copy one partition to dest
   static void copy_segment(Cell * dest, Value & dest_owner, ShapeItem h,
                            ShapeItem from, ShapeItem to,
                            ShapeItem m_len, ShapeItem l, ShapeItem len_l,
                            Value_P B);
};
//=============================================================================
/** primitive functions pick and disclose */
/// The class implementing ⊃
class Bif_F12_PICK : public NonscalarFunction
{  
public:
   /// Constructor
   Bif_F12_PICK()
   : NonscalarFunction(TOK_F12_PICK) 
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B) const;

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B) const
      { return disclose(B, false); }

   /// ⊃B
   static Token disclose(Value_P B, bool rank_tolerant);

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B) const
      { const Shape axes_X = Value::to_shape(X.get());
        return disclose_with_axis(axes_X, B, false); }

   /// ⊃[X]B
   static Token disclose_with_axis(const Shape & axes_X, Value_P B,
                                   bool rank_tolerant);

   static Bif_F12_PICK * fun;   ///< Built-in function
   static Bif_F12_PICK  _fun;   ///< Built-in function

protected:
   /// the shape of the items being disclosed
   static Shape compute_item_shape(Value_P B, bool rank_tolerant);

   /// Pick from B according to cA and len_A. \b cell_owner is non-zero
   /// if a left-vlues is picked, (e.g (2 1⊃B)←'TR')
   static Value_P pick(const Cell * cA, ShapeItem len_A, Value_P B,
                       APL_Integer qio, Value * cell_owner);
};
//=============================================================================

#endif // __Bif_F12_PARTITION_PICK_HH_DEFINED__

