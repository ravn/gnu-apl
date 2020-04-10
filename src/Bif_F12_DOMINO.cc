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

/// a macro to enable debug printouts
#define DOMINO_DEBUG

#include "Bif_F12_DOMINO.hh"
#include "Bif_F12_FORMAT.hh"
#include "ComplexCell.hh"
#include "Value.hh"
#include "Workspace.hh"
extern void divide_matrix(Cell * cZ, bool need_complex,
                          ShapeItem rows, ShapeItem cols_A, const Cell * cA,
                          ShapeItem cols_B, const Cell * cB);


Bif_F12_DOMINO   Bif_F12_DOMINO   ::_fun;    // ⌹
Bif_F12_DOMINO * Bif_F12_DOMINO::fun = &Bif_F12_DOMINO::_fun;

#ifndef DOMINO_DEBUG
# undef Q1
# define Q1(x)
#endif

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

   loop(y, rows)
   loop(x, rows)
       new (I->next_ravel()) FloatCell((y == x) ? 1.0 : 0.0);

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
template<>
void Bif_F12_DOMINO::Matrix<false>::debug(const char * name) const
{
#ifdef DOMINO_DEBUG
const Shape shape_B(M, N);
Value_P B(shape_B, LOC);

   loop(y, M)
   loop(x, N)   new (B->next_ravel())   FloatCell(real(y, x));
   B->check_value(LOC);

Value_P A(2, LOC);
   new (A->next_ravel())   IntCell(0);
   new (A->next_ravel())   IntCell(4);   // number of fractional digits
   A->check_value(LOC);

Value_P Z = Bif_F12_FORMAT::fun->format_by_specification(A, B);
   Z->print_boxed(CERR, name);
#endif // DOMINO_DEBUG
}
//-----------------------------------------------------------------------------
template<>
void Bif_F12_DOMINO::Matrix<true>::debug(const char * name) const
{
#ifdef DOMINO_DEBUG
const Shape shape_B(M, N);
Value_P B(shape_B, LOC);

   loop(y, M)
   loop(x, N)   new (B->next_ravel())   ComplexCell(real(y, x), imag(y, x));
   B->check_value(LOC);

Value_P A(2, LOC);
   new (A->next_ravel())   IntCell(0);
   new (A->next_ravel())   IntCell(4);   // number of fractional digits
   A->check_value(LOC);

Value_P Z = Bif_F12_FORMAT::fun->format_by_specification(A, B);
   Z->print_boxed(CERR, name);
#endif // DOMINO_DEBUG
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
const ShapeItem len_Qi  = len_Q;         // dito.
const ShapeItem len_R   = len_Q;         // dito.
const ShapeItem len_S   = len_Q;         // dito.
const ShapeItem base_B  = 1;
const ShapeItem base_Q  = base_B  + CPLX*len_B  + 2;
const ShapeItem base_Qi = base_Q  + CPLX*len_Q  + 2;
const ShapeItem base_R  = base_Qi + CPLX*len_Qi + 2;
const ShapeItem base_S  = base_R  + CPLX*len_R  + 2;
const ShapeItem end     = base_S  + CPLX*len_S  + 2;

double * data = new double[end*CPLX];   if (data == 0)   WS_FULL;
   memset(data, 0, end*sizeof(double));
   data[base_B - 1]  = len_B;   data[base_B  + CPLX*len_B]  = 42.0 / len_B;
   data[base_Q - 1]  = len_Q;   data[base_Q  + CPLX*len_Q]  = 42.0 / len_Q;
   data[base_Qi - 1] = len_Qi;  data[base_Qi + CPLX*len_Qi] = 42.0 / len_Qi;
   data[base_R - 1]  = len_R;   data[base_R  + CPLX*len_R]  = 42.0 / len_R;
   data[base_S - 1]  = len_S;   data[base_S  + CPLX*len_S]  = 42.0 / len_S;

   if (need_complex)   // complex B
      {
        setup_complex_B(cB, data + base_B, len_B);
        householder<true>(data + base_B, rows, cols, data + base_Q,
                          data + base_Qi, data + base_R, data + base_S, EPS);

        setup_complex_B(cB, data + base_B, len_B);   // restore B
        const Matrix<true> mB(data + base_B, rows, cols);
        Matrix<true> mQ(data + base_Q, rows, rows);
        mQ.transpose(rows);
        Matrix<true> mR(data + base_R, rows, cols);
        mR.init_inner_product(mQ, mB);
        mQ.debug("final Q");
        mR.debug("final R");
      }
   else                // real B
      {
        setup_real_B(cB, data + base_B, len_B);
        householder<false>(data + base_B, rows, cols, data + base_Q,
                           data + base_Qi, data + base_R, data + base_S, EPS);

        setup_complex_B(cB, data + base_B, len_B);   // restore B
        const Matrix<false> mB(data + base_B, rows, cols);
        Matrix<false> mQ(data + base_Q, rows, rows);
        mQ.transpose(rows);
        Matrix<false> mR(data + base_R, rows, cols);
        mR.init_inner_product(mQ, mB);
      }

   // check that the memory areas were not overridden
   Assert(data[base_B - 1]  == len_B);
   Assert(data[base_B + CPLX*len_B]  == 42.0 / len_B);
   Assert(data[base_Q - 1]  == len_Q);
   Assert(data[base_Q + CPLX*len_Q]  == 42.0 / len_Q);
   Assert(data[base_Qi - 1] == len_Qi);
   Assert(data[base_Qi + CPLX*len_Qi] == 42.0 / len_Qi);
   Assert(data[base_R - 1]  == len_R);
   Assert(data[base_R + CPLX*len_R]  == 42.0 / len_R);
   Assert(data[base_S - 1]  == len_S);
   Assert(data[base_S + CPLX*len_S]  == 42.0 / len_S);

   {
     const Shape shape_Q(rows, rows);
     Value_P vQ(shape_Q, LOC);
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
     const Shape shape_R(rows, cols);
     Value_P vR(shape_R, LOC);
     if (need_complex)
        {
          loop(r, rows*cols)
              new (vR->next_ravel()) ComplexCell(data[base_R + 2*r],
                                                 data[base_R + 2*r + 1]);
        }
     else
        {
          loop(r, rows*cols)
              new (vR->next_ravel()) FloatCell(data[base_R + r]);
        }
     vR->check_value(LOC);
     new (Z->next_ravel()) PointerCell(vR.get(), Z.getref());
   }

   delete[] data;
}
//-----------------------------------------------------------------------------
void
Bif_F12_DOMINO::setup_complex_B(const Cell * cB, double * D, ShapeItem count)
{
   loop(b, count)
      {
        const Cell & cell = *cB++;
        if (cell.is_float_cell())
           { *D++ = cell.get_real_value();   *D++ = 0.0; }
        else if (cell.is_integer_cell())
           { *D++ = cell.get_real_value();   *D++ = 0.0; }
        else if (cell.is_complex_cell())
           { *D++ = cell.get_real_value(); *D++ = cell.get_imag_value(); }
        else   DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
Bif_F12_DOMINO::setup_real_B(const Cell * cB, double * D, ShapeItem count)
{
   loop(b, count)
      {
        const Cell & cell = *cB++;
        if (cell.is_float_cell())
           { *D++ = cell.get_real_value();   *D++ = 0.0; }
        else if (cell.is_integer_cell())
           { *D++ = cell.get_real_value();   *D++ = 0.0; }
        else   DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
template<bool cplx>
void
Bif_F12_DOMINO::householder(double * pB, ShapeItem rows, ShapeItem cols,
                            double * pQ, double * pQi, double * pR, double * pS,
                            double EPS)
{
   // pB is the matrix to be factorized, pQ, pQi, amd pR were initialized to 0
   //
Matrix<cplx> mB (pB,  rows, cols);   // shrinks
Matrix<cplx> mQ (pQ,  rows, rows);   // keeps size
Matrix<cplx> mQi(pQi, rows, rows);   // keeps size
Matrix<cplx> mR (pR,  rows, rows);   // temporary storage

   // [0]  Q←HSHLDR2 B;N;BMAX;S;L2;QI;COL1
   // [1]  Q←ID N←↑⍴B

   // mQ was cleared, so setting the diagonal suffices
   loop(x, rows)   mQ.real(x, x) = 1.0;

   // [2]  →(0=(1↓⍴B),BMAX←⌈/∣,B)/0   ⍝ done if no or only near-0 columns

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

   for (;;)
       {

   // [3]  SPRFLCTR: S←ID ↑⍴B ◊ Debug 'B'

        Matrix<cplx> mS (pS,  rows, rows);
mB.debug("[3] B");

   // [4]  L2←NORM2 COL1←B[;1] ◊ Debug 'COL1' ◊ Debug 'L2'

        Matrix<cplx> mCOL1 (pB,  rows, 1, mB.dY);   // COL1←B[;1]
mCOL1.debug("[4] COL1");
        norm_result L;   mB.col1_norm(L);
Q(L.norm2_real)
Q(L.norm2_imag)
        const bool significant = mCOL1.significant(BMAX, EPS);

   // [5]  IMBED → L2=0
   // [6]  IMBED → ∼0∈0=(1↓COL1) CMP_TOL EPS BMAX

        if (significant || (L.norm2_real + L.norm2_imag) > qct2)
           {
Q1("SIGNIFICANT")
   // [7]  B[1;1]←(↑B) + (L2⋆÷2)×(0≤↑B)-0>↑B

             double * B11_real = &mB.real(0, 0);
             double * B11_imag = B11_real + 1;
             if (*B11_real < 0)
                {
Q1("SIGN ¯1")
Q1(*B11_real)
Q1(*B11_imag)
                  *B11_real -= L.norm_real;
                  *B11_imag -= L.norm_imag;
                }
             else
                {
Q1("SIGN 1")
Q1(*B11_real)
Q1(*B11_imag)
                  *B11_real += L.norm_real;
                  *B11_imag += L.norm_imag;
                }

   // [8]   COL1←B[;1] ◊ Debug 'COL1'
   // [9]   SCALE←2÷NORM2 COL1 ◊ Debug 'NORM2 COL1' ◊ Debug 'SCALE'·
   // [10]  S←COL1∘.×COL1×SCALE ◊ (1 1⍉S)←1 1⍉S - 1.0


             // COL1←B[;1] changes nothing, so only S←... remains.
             // We have moved the initialization of S to the else
             // clause below. Therefore -S on the right does not work
             // here, but we can simply subtract 1.0 from the diagonal.
             //
mCOL1.debug("[8] COL1");
             norm_result scale;   mCOL1.col1_norm(scale);
             mS.init_outer_product(scale, mCOL1);
Q1(scale.norm__2_real)
Q1(scale.norm__2_imag)
             loop(y, rows)   mS.real(y, y) -= 1.0;   // subtract 1.0
           }
        else   // →IMBED but do [3] here
           {
             mS.init_identity(rows);
           }

   // [9] QI←ID N ◊ ((-2/↑⍴S)↑QI)←S ◊ Debug 'QI'

   mQi.imbed(mS);
mQi.debug("[11] QI");
mS .debug("[11] S");

   // [12]   Q←Q+.×QI ◊ Debug 'Q'

   mR.resize(mQ.M, mQ.N);   mR = mQ;
   mQ.init_inner_product(mR, mQi);
mR.debug("[12] R");
mQi.debug("[12] Qi");
mQ.debug("[12] Q");

   // [13]   Debug 'B' ◊ Debug 'S' ◊ B←1 1↓S+.×B ◊ Debug 'B'

   mR.resize(mB.M, mB.N);   mR = mB;
mR.debug("[13] B");
mS.debug("[13] S");
   mB.resize(mS.M, mR.N);
   mB.init_inner_product(mS, mR);

   pB = mB.drop_1_1();   // 1 1↓B
mB.debug("[13] B");

   // [14]   →(0≠1↓⍴B)/SPRFLCTR

         if (0 == --cols)   break;
         --rows;
       }

mQ.debug("[end] Q");
}
//-----------------------------------------------------------------------------

