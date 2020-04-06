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
   static void QR_factorization(Value_P Z, bool need_complex, ShapeItem rows,
                                ShapeItem cols, const Cell * cB, double EPS);

   template<bool cplx>
   static void householder(double * B, ShapeItem rows, ShapeItem cols,
                           double * Q, double * Q1, double * R, double * S,
                           double EPS);

   /** a real or complex matrix. Unlike "normal" matrices where the matrixc rows
       are adjacent, this matrix class separates the number of columns and the
       distance between column elements in order to avoid unnecessary copies of
       sub matrices (which are frequent in householder transformations).

       Also, the matrix does no memory allocation but operates on a double *
       provided in the constructor.
    **/
   template<bool cplx>
   class Matrix
     {
       enum { dpi = cplx ? 2 : 1 };   ///< doubles per item

       public:

       // constructor: M×N matrix with the default item spacings dX and dY
       //
       Matrix(double * _data, ShapeItem _M, ShapeItem _N)
       : data(_data),
         M(_M),
         N(_N),
         dX(dpi),
         dY(dpi*N)
       {}

       /// check that cond is true
       void matrix_assert(bool cond) const {}

       /// return the real part of A[x;y]
       double & real(ShapeItem x, ShapeItem y)
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  M);
             matrix_assert(y >= 0);
             matrix_assert(y <  N);
             return data[x*dX + y*dY];
          }

       // initialize this matrix to the identity matrix
       inline void init_identity(ShapeItem rows);
       inline void init_outer_product(double scale, const Matrix<cplx> & vector);

       /// return the real part of A[x;y]
       const double & real(ShapeItem x, ShapeItem y) const
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  M);
             matrix_assert(y >= 0);
             matrix_assert(y <  N);
             return data[x*dX + y*dY];
          }

       /// return the imaginary part of A[x;y]
       double & imag(ShapeItem x, ShapeItem y)
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  M);
             matrix_assert(y >= 0);
             matrix_assert(y <  N);
             return data[x*dX + y*dY + 1];
          }

       /// return the imaginary part of A[x;y]
       const double & imag(ShapeItem x, ShapeItem y) const
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  M);
             matrix_assert(y >= 0);
             matrix_assert(y <  N);
             return data[x*dX + y*dY + 1];
          }

       inline double abs2(ShapeItem y, ShapeItem x) const;
       inline double col1_norm2() const
          {
            double ret = 0;
            loop(y, M)   ret += abs2(y, 0);
            return ret;
          }

       inline bool significant(double bmax, double eps) const
          {
            const double bmax_eps = bmax + eps;
            const double diff = bmax - bmax_eps;
            const double diff2 = diff * diff;   // square diff since abs2 is
            for (ShapeItem y = 1; y < M; ++y)
                {
                  const double abs = abs2(y, 1);
                  if (abs < diff2)   return true;   // non-zero
                  if (abs > diff2)   return true;   // non-zero
                }

            return false;   // all items below A[0;1] are (close to) 0
          }

       protected:
       /// the matrix elements
       double * data;

       /// the number of matrix rows
       const ShapeItem M;

       /// the number of matrix columns
       const ShapeItem N;

       /// the distance (in doubles) between  A[i;j] and A[i+1;j]
       const ShapeItem dX;

       /// the distance (in doubles) between  A[i;j] and A[i;j+1]
       const ShapeItem dY;
     };
};

template<>
inline double
Bif_F12_DOMINO::Matrix<true>::abs2(ShapeItem y, ShapeItem x) const
{
const ShapeItem b = x*dX + y*dY;
   return data[b]*data[b] + data[b+1]*data[b+1];
}

template<>
inline double
Bif_F12_DOMINO::Matrix<false>::abs2(ShapeItem y, ShapeItem x) const
{
const ShapeItem b = x*dX + y*dY;
   return data[b]*data[b];
}
//-----------------------------------------------------------------------------
template<>
inline void Bif_F12_DOMINO::Matrix<true>::init_identity(ShapeItem rows)
{
   matrix_assert(rows == M);
   matrix_assert(rows == N);
   loop(r, M) loop(c, N)
      { real(r, c) = (r == c) ? 1.0 : 0.0;   imag(r, c) = 0.0; }
}
//-----------------------------------------------------------------------------
template<>
inline void Bif_F12_DOMINO::Matrix<true>::init_outer_product(double scale,
                                                      const Matrix<true> & src)
{
   loop(r, M) loop(c, N)
       real(c, r) = scale * src.real(c, 1) * src.real(r, 1);
}
//-----------------------------------------------------------------------------
template<>
inline void Bif_F12_DOMINO::Matrix<false>::init_outer_product(double scale,
                                                     const Matrix<false> & src)
{
   loop(r, M) loop(c, N)
       {
         real(c, r) = scale * (src.real(c, 1) * src.real(r, 1)
                             - src.imag(c, 1) * src.imag(r, 1));
         imag(c, r) = scale * (src.real(c, 1) * src.imag(r, 1)
                             + src.imag(c, 1) * src.real(r, 1));
       }
}
//-----------------------------------------------------------------------------
template<>
inline void Bif_F12_DOMINO::Matrix<false>::init_identity(ShapeItem rows)
{
   matrix_assert(rows == M);
   matrix_assert(rows == N);
   loop(r, M) loop(c, N)   { real(r, c) = (r == c) ? 1.0 : 0.0; }
}
//-----------------------------------------------------------------------------

#endif // __BIF_F12_DOMINO_HH_DEFINED__

