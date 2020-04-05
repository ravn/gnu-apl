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

#include "Bif_F12_DOMINO.hh"
#include "ComplexCell.hh"
#include "Workspace.hh"
extern void divide_matrix(Cell * cZ, bool need_complex,
                          ShapeItem rows, ShapeItem cols_A, const Cell * cA,
                          ShapeItem cols_B, const Cell * cB);


Bif_F12_DOMINO   Bif_F12_DOMINO   ::_fun;    // ⌹
Bif_F12_DOMINO * Bif_F12_DOMINO::fun = &Bif_F12_DOMINO::_fun;

//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_B(Value_P B)
{
   if (B->is_scalar())
      {
        Value_P Z(LOC);

        B->get_ravel(0).bif_reciprocal(Z->next_ravel());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() == 1)   // inversion at the unit sphere
      {
        const double qct = Workspace::get_CT();
        const ShapeItem len = B->get_shape_item(0);
        APL_Complex r2(0.0);
        loop(l, len)
            {
              const APL_Complex b = B->get_ravel(l).get_complex_value();
              r2 += b*b;
            }

        if (r2.real() < qct && r2.real() > -qct &&
            r2.imag() < qct && r2.imag() > -qct)
            DOMAIN_ERROR;

        Value_P Z(len, LOC);

        if (r2.imag() < qct && r2.imag() > -qct)   // real result
           {
             loop(l, len)
                 {
                   const APL_Float b = B->get_ravel(l).get_real_value();
                   new (Z->next_ravel())   FloatCell(b / r2.real());
                 }
           }
        else                                       // complex result
           {
             loop(l, len)
                 {
                   const APL_Complex b = B->get_ravel(l).get_complex_value();
                   new (Z->next_ravel())   ComplexCell(b / r2);
                 }
           }

        Z->set_default(*B.get(), LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem rows = B->get_shape_item(0);
const ShapeItem cols = B->get_shape_item(1);
   if (cols > rows)   LENGTH_ERROR;

   // create an identity matrix I and call eval_AB(I, B).
   //
const Shape shape_I(rows, rows);
Value_P I(shape_I, LOC);

   loop(r, rows)
   loop(c, rows)
       new (I->next_ravel()) FloatCell((c == r) ? 1.0 : 0.0);

Token result = eval_AB(I, B);
   return result;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_XB(Value_P X, Value_P B)
{
   if (B->get_rank() != 2)   return eval_B(B);

const ShapeItem rows = B->get_shape_item(0);
const ShapeItem cols = B->get_shape_item(1);
   if (cols > rows)   LENGTH_ERROR;

   // create an identity matrix I and call eval_AXB(I, X, B).
   //
const Shape shape_I(rows, rows);
Value_P I(shape_I, LOC);

   loop(r, rows)
   loop(c, rows)
       new (I->next_ravel()) FloatCell((c == r) ? 1.0 : 0.0);

Token result = eval_AXB(I, X, B);
   return result;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_AB(Value_P A, Value_P B)
{
ShapeItem rows_A = 1;
ShapeItem cols_A = 1;

Shape shape_Z;

   // if rank of A or B is < 2 then treat it as a
   // 1 by n (or 1 by 1) matrix..
   //
ShapeItem rows_B = 1;
ShapeItem cols_B = 1;
   switch(B->get_rank())
      {
         case 0:  break;

         case 1:  rows_B = B->get_shape_item(0);
                  break;

         case 2:  cols_B = B->get_shape_item(1);
                  rows_B = B->get_shape_item(0);
                  shape_Z.add_shape_item(cols_B);
                  break;

         default: RANK_ERROR;
      }

   switch(A->get_rank())
      {
         case 0:  break;

         case 1:  rows_A = A->get_shape_item(0);
                  break;

         case 2:  cols_A = A->get_shape_item(1);
                  rows_A = A->get_shape_item(0);
                  shape_Z.add_shape_item(cols_A);
                  break;

         default: RANK_ERROR;
      }

   if (rows_B <  cols_B)   LENGTH_ERROR;
   if (rows_A != rows_B)   LENGTH_ERROR;

const bool need_complex = A->is_complex(true) || B->is_complex(true);
Value_P Z(shape_Z, LOC);
   divide_matrix(&Z->get_ravel(0), need_complex,
                 rows_A, cols_A, &A->get_ravel(0),
                 cols_B, &B->get_ravel(0));

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_fill_B(Value_P B)
{
   return Bif_F12_TRANSPOSE::fun->eval_B(B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_fill_AB(Value_P A, Value_P B)
{
Shape shape_Z;
   loop(r, A->get_rank() - 1)  shape_Z.add_shape_item(A->get_shape_item(r + 1));
   loop(r, B->get_rank() - 1)  shape_Z.add_shape_item(B->get_shape_item(r + 1));

Value_P Z(shape_Z, LOC);
   while (Z->more())   new (Z->next_ravel())   IntCell(0);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (!X->is_scalar())   RANK_ERROR;
const double EPS = X->get_ravel(0).get_real_value();

ShapeItem rows_A = 1;
ShapeItem cols_A = 1;

Shape shape_Z;

   // if rank of A or B is < 2 then treat it as a
   // 1 by n (or 1 by 1) matrix..
   //
ShapeItem rows_B = 1;
ShapeItem cols_B = 1;
   switch(B->get_rank())
      {
         case 0:  break;

         case 1:  rows_B = B->get_shape_item(0);
                  break;

         case 2:  cols_B = B->get_shape_item(1);
                  rows_B = B->get_shape_item(0);
                  shape_Z.add_shape_item(cols_B);
                  break;

         default: RANK_ERROR;
      }

   switch(A->get_rank())
      {
         case 0:  break;

         case 1:  rows_A = A->get_shape_item(0);
                  break;

         case 2:  cols_A = A->get_shape_item(1);
                  rows_A = A->get_shape_item(0);
                  shape_Z.add_shape_item(cols_A);
                  break;

         default: RANK_ERROR;
      }

   if (rows_B <  cols_B)   LENGTH_ERROR;
   if (rows_A != rows_B)   LENGTH_ERROR;

const bool need_complex = A->is_complex(true) || B->is_complex(true);
Value_P Z(shape_Z, LOC);
   divide_matrix2(&Z->get_ravel(0), need_complex,
                 rows_A, cols_A, &A->get_ravel(0),
                 cols_B, &B->get_ravel(0), EPS);

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_DOMINO::divide_matrix2(Cell * cZ, bool need_complex, ShapeItem rows,
                               ShapeItem cols_A, const Cell * cA,
                               ShapeItem cols_B, const Cell * cB, double EPS)
{
   /* We want to store all floating point variables (including complex ones)
      in a single double[]. Before and after each variable V leave one double
      for storing ⍴V (before) and ÷⍴V (after). These doubles are used to check
      for overrides of the allocated space.

      Complex numbers are stored as real followed by imag part.

      The variables are B, Q, and R with B = Q +.× R, with Q real orthogonal
      and R real or complex upper triangular.
   */

   // start with the base addresses of the variables...
   //
const int CPLX = need_complex ? 2 : 1;   // number of doubles per variable item
const ShapeItem len_B   = rows * cols_B;
const ShapeItem len_Q   = rows * rows;   // quadratic: M×M counting down to 2×2
const ShapeItem len_Q1  = len_Q;         // dito.
const ShapeItem len_R   = len_Q;         // dito.
const ShapeItem base_B  = 1;
const ShapeItem base_Q  = 2 + base_B  + len_B *CPLX;
const ShapeItem base_Q1 = 2 + base_Q  + len_Q *CPLX;
const ShapeItem base_R  = 2 + base_Q1 + len_Q1*CPLX;
const ShapeItem end     = 1 + base_R  + len_R *CPLX;

double * data = new double[end];   if (data == 0)   WS_FULL;
   memset(data, 0, end*sizeof(double));
   data[base_B - 1]  = len_B;   data[base_B + len_B]  = 1.0 / len_B;
   data[base_Q - 1]  = len_Q;   data[base_Q + len_B]  = 1.0 / len_Q;
   data[base_Q1 - 1] = len_Q1;  data[base_Q1 + len_B] = 1.0 / len_Q1;
   data[base_R - 1]  = len_R;   data[base_R + len_B]  = 1.0 / len_R;

   divide_matrix(cZ, need_complex, rows, cols_A, cA, cols_B, cB);

   Assert(data[base_B - 1]  == len_B);
   Assert(data[base_B + len_B]  = 1.0 / len_B);
   Assert(data[base_Q - 1]  == len_Q);
   Assert(data[base_Q + len_B]  = 1.0 / len_Q);
   Assert(data[base_Q1 - 1] == len_Q1);
   Assert(data[base_Q1 + len_B] = 1.0 / len_Q1);
   Assert(data[base_R - 1]  == len_R);
   Assert(data[base_R + len_B]  = 1.0 / len_R);
   delete data;
}
//-----------------------------------------------------------------------------

