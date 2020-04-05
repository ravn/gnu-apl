/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2019  Dr. Jürgen Sauermann

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

#ifndef __BIF_F12_DOMINO_HH_DEFINED__
#define __BIF_F12_DOMINO_HH_DEFINED__

#include "Common.hh"
#include "PrimitiveFunction.hh"

//-----------------------------------------------------------------------------
/** primitive functions matrix divide and matrix invert */
/// The class implementing ⌹
class Bif_F12_DOMINO : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_DOMINO()
   : NonscalarFunction(TOK_F12_DOMINO)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_DOMINO * fun;   ///< Built-in function
   static Bif_F12_DOMINO  _fun;   ///< Built-in function

   /// overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

protected:
   static void divide_matrix2(Cell * cZ, bool need_complex,
                              ShapeItem rows, ShapeItem cols_A, const Cell * cA,
                              ShapeItem cols_B, const Cell * cB, double EPS);

};
//-----------------------------------------------------------------------------
#endif // __BIF_F12_DOMINO_HH_DEFINED__

