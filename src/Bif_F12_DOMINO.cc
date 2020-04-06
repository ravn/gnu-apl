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
   if (!X->is_scalar())   RANK_ERROR;
const double EPS = X->get_ravel(0).get_real_value();

   if (B->get_rank() != 2)        RANK_ERROR;

   // if rank of A or B is < 2 then treat it as a
   // 1 by n (or 1 by 1) matrix..
   //
const ShapeItem rows_B = B->get_rows();
const ShapeItem cols_B = B->get_cols();
   if (rows_B <  cols_B)   LENGTH_ERROR;
   if (rows_B*cols_B == 0)   LENGTH_ERROR;


const bool need_complex = B->is_complex(true);
Value_P Z(2, LOC);
   QR_factorization(Z, need_complex, rows_B, cols_B, &B->get_ravel(0), EPS);

   Z->set_default(*B.get(), LOC);   // not needed
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
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
   TODO;
}
//-----------------------------------------------------------------------------
void
Bif_F12_DOMINO::QR_factorization(Value_P Z, bool need_complex, ShapeItem rows,
                                 ShapeItem cols, const Cell * cB, double EPS)
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
const ShapeItem len_B   = rows * cols;
const ShapeItem len_Q   = rows * rows;   // quadratic: M×M counting down to 2×2
const ShapeItem len_Q1  = len_Q;         // dito.
const ShapeItem len_R   = len_Q;         // dito.
const ShapeItem len_S   = len_Q;         // dito.
const ShapeItem base_B  = 1;
const ShapeItem base_Q  = 2 + base_B  + len_B *CPLX;
const ShapeItem base_Q1 = 2 + base_Q  + len_Q *CPLX;
const ShapeItem base_R  = 2 + base_Q1 + len_Q1*CPLX;
const ShapeItem base_S  = 2 + base_R  + len_R *CPLX;
const ShapeItem end     = 1 + base_R  + len_S *CPLX;

double * data = new double[end];   if (data == 0)   WS_FULL;
   memset(data, 0, end*sizeof(double));
   data[base_B - 1]  = len_B;   data[base_B  + len_B]  = 1.0 / len_B;
   data[base_Q - 1]  = len_Q;   data[base_Q  + len_Q]  = 1.0 / len_Q;
   data[base_Q1 - 1] = len_Q1;  data[base_Q1 + len_Q1] = 1.0 / len_Q1;
   data[base_R - 1]  = len_R;   data[base_R  + len_R]  = 1.0 / len_R;
   data[base_S - 1]  = len_S;   data[base_S  + len_S]  = 1.0 / len_S;

   if (need_complex)   // complex B
      {
        const Cell * C = cB;
        double * d = data + base_B;
        loop(b, len_B)
            {
              const Cell & cell = *C++;
              if (cell.is_float_cell())
                 { *d++ = cell.get_real_value();   *d++ = 0.0; }
              else if (cell.is_integer_cell())
                 { *d++ = cell.get_real_value();   *d++ = 0.0; }
              else if (cell.is_complex_cell())
                 { *d++ = cell.get_real_value();
                   *d++ = cell.get_imag_value(); }
              else   DOMAIN_ERROR;
            }

        householder<true>(data + base_B, rows, cols, data + base_Q,
                          data + base_Q1, data + base_R, data + base_S, EPS);
      }
   else                // real B
      {
        const Cell * C = cB;
        double * d = data + base_B;
        loop(b, len_B)
            {
              const Cell & cell = *C++;
              if      (cell.is_float_cell())     *d++ = cell.get_real_value();
              else if (cell.is_integer_cell())   *d++ = cell.get_real_value();
              else if (cell.is_complex_cell())   FIXME    // need_complex ?
              else                               DOMAIN_ERROR;
            }

        householder<false>(data + base_B, rows, cols, data + base_Q,
                           data + base_Q1, data + base_R, data + base_S, EPS);
      }

   Assert(data[base_B - 1]  == len_B);
   Assert(data[base_B + len_B]  = 1.0 / len_B);
   Assert(data[base_Q - 1]  == len_Q);
   Assert(data[base_Q + len_Q]  = 1.0 / len_Q);
   Assert(data[base_Q1 - 1] == len_Q1);
   Assert(data[base_Q1 + len_Q1] = 1.0 / len_Q1);
   Assert(data[base_R - 1]  == len_R);
   Assert(data[base_R + len_R]  = 1.0 / len_R);
   Assert(data[base_S - 1]  == len_S);
   Assert(data[base_S + len_S]  = 1.0 / len_S);

const Shape shape_QR(rows, rows);

   {
     Value_P vQ(shape_QR, LOC);
     if (need_complex)
        {
          loop(q, len_Q)
              new (vQ->next_ravel()) ComplexCell(data[base_Q + 2*q],
                                                 data[base_Q + 2*q + 1]);
        }
     else
        {
          loop(q, len_Q)
              new (vQ->next_ravel()) FloatCell(data[base_Q + q]);
        }
     vQ->check_value(LOC);
     new (Z->next_ravel()) PointerCell(vQ.get(), Z.getref());
   }
   {
     Value_P vR(shape_QR, LOC);
     if (need_complex)
        {
          loop(r, len_Q)
              new (vR->next_ravel()) ComplexCell(data[base_R + 2*r],
                                                 data[base_R + 2*r + 1]);
        }
     else
        {
          loop(r, len_Q)
              new (vR->next_ravel()) FloatCell(data[base_R + r]);
        }
     vR->check_value(LOC);
     new (Z->next_ravel()) PointerCell(vR.get(), Z.getref());
   }

   delete data;
}
//-----------------------------------------------------------------------------
template<bool cplx>
void
Bif_F12_DOMINO::householder(double * pB, ShapeItem rows, ShapeItem cols,
                            double * pQ, double * pQ1, double * pR, double * pS,
                            double EPS)
{
   // pB is the matrix to be factorized, pQ, pQ1, amd pR were initialized to 0
   //
Matrix<cplx> mB (pB,  rows, cols);
Matrix<cplx> mQ (pQ,  rows, rows);
Matrix<cplx> mQ1(pQ1, rows, rows);
Matrix<cplx> mR (pR,  rows, rows);
Matrix<cplx> mS (pS,  rows, rows);

   // [0]   Q←HSHLDR2 B;N;BMAX;S;M;L;L2;QI;COL1
   // [1]    Q←ID N←↑⍴B

   loop(r, rows)   mQ.real(r, r) = 1.0;

   // [2]    →(0=(1↓⍴B),BMAX←⌈/∣,B)/0   ⍝ done if no or only near-0 columns

   if (cols == 0)   return;
double BMAX = 0.0;
   loop(y, rows)
   loop(x, cols)
       {
         const double abs2 = mB.abs2(y, x);
         if (BMAX < abs2)   BMAX = abs2;
       }
   BMAX = sqrt(BMAX);
const double qct = Workspace::get_CT();
   if (BMAX < qct)   return;   // all B[x;y] = 0
const double qct2 = qct*qct;

   // [3]  SPRFLCTR: S←ID ↑⍴B

   do {

   // [4]    L2←NORM2 COL1←B[;1]

        Matrix<cplx> mCOL1 (pB,  rows, 1);   // COL1←B[;1]
        const double L2 = mB.col1_norm2();
        const bool significant = mCOL1.significant(BMAX, EPS);

   // [5]    IMBED → L2=0
   // [6]    IMBED → ∼0∈0=(1↓COL1) CMP_TOL EPS BMAX

        if (significant || L2 > qct2)
           {
   // [7]    B[1;1]←(↑B) + (L2⋆÷2) × (0≤↑B)-0>↑B

             const double L = sqrt(L2);
             double & B11 = mB.real(0, 0);
             if (B11 < 0)   B11 -= L;
             else           B11 += L;

   // [8]    COL1←B[;1] ◊ S←(COL1∘.×COL1×2÷NORM2 COL1) ◊ (1 1⍉S)←1 1⍉S - 1.0

             // COL1←B[;1] changes nothing, so only S←... remains.
             // we have moved the initialization of S to the else
             // clause below. Therefore -S on the right does not work
             // but we can simply subtract 1.0 from the diagonal.
             const double scale = 2/mCOL1.col1_norm2();
             mS.init_outer_product(scale, mCOL1);
             loop(r, rows)   mS.real(r, r) -= 1.0;   // subtract 1.0
           }
        else   // →IMBED but do [3] here
           {
             mS.init_identity(rows);
           }

   // [9] IMBED: QI←ID N
   // [10]   ((-2/↑⍴S)↑QI)←S
   // [12]   Q←Q+.×QI
   // [13]   →(0≠1↓⍴B←1 1↓S+.×B)/SPRFLCTR

      } while (cols);
}
//-----------------------------------------------------------------------------

