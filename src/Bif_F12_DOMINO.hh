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

#include "Assert.hh"
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
   /// compute the Q matrix of B = QR
   static void QR_factorization(Value_P Z, bool need_complex, ShapeItem rows,
                                ShapeItem cols, const Cell * cB, double EPS);

   /// compute the householder transformation Q of B = QR
   template<bool cplx>
   static void householder(double * B, ShapeItem rows, ShapeItem cols,
                           double * Q, double * Q1, double * R, double * S,
                           double EPS);

   // initialize complex D with Cells cB
   static void setup_complex_B(const Cell * cB, double * D, ShapeItem count);

   // initialize real D with Cells cB
   static void setup_real_B(const Cell * cB, double * D, ShapeItem count);

   /** a real or complex matrix. Unlike "normal" matrices where the matrixc rows
       are adjacent, this matrix class separates the number of columns and the
       distance between column elements in order to avoid unnecessary copies of
       sub matrices (which are frequent in householder transformations).

       Also, the matrix does no memory allocation but operates on a double *
       provided in the constructor.
    **/

    struct norm_result
       {
         double norm2_real;
         double norm2_imag;
         double norm_real;      // sqrt(norm)
         double norm_imag;      // sqrt(norm)
         double norm__2_real;   // 2÷norm
         double norm__2_imag;   // 2÷norm
       };

   template<bool cplx>
   class Matrix
     {
       public:
       enum { dpi = cplx ? 2 : 1 };   ///< doubles per item

       /// check that cond is true
//     void matrix_assert(bool cond) const {}
#define matrix_assert(x) Assert(x)


       // constructor: M×N matrix with the default vertical item spacings dY
       //
       Matrix(double * pdata, ShapeItem uM, ShapeItem uN)
       : data(pdata),
         M(uM),
         N(uN),
         dY(dpi*uN)
       {}

       // constructor: M×1 matrix from a matrix column
       //
       Matrix(double * pdata, ShapeItem uM, ShapeItem uN, ShapeItem udY)
       : data(pdata),
         M(uM),
         N(uN),
         dY(udY)
       {}

       inline void resize(ShapeItem M, ShapeItem N)
          { new (this)   Matrix<cplx>(data, M, N); }

       /// return the real part of A[x;y]
       double & real(ShapeItem y, ShapeItem x)
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  N);
             matrix_assert(y >= 0);
             matrix_assert(y <  M);
             return data[x*dpi + y*dY];
          }

       // initialize this matrix to the identity matrix
       inline void init_identity(ShapeItem rows);
       inline void imbed(const Matrix<cplx> & S);
       inline void init_outer_product(const norm_result & scale,
                                      const Matrix<cplx> & vect);
       inline void operator =(const Matrix<cplx> & other);
       inline void init_inner_product(const Matrix<cplx> & src_A,
                                      const Matrix<cplx> & src_B);
       inline void transpose(ShapeItem M);

       inline double * drop_1_1()
          { new (this)   Matrix<cplx>(data + dpi*(N + 1), M-1, N-1, dY);
            return data; }

       /// return the real part of A[x;y]
       const double & real(ShapeItem y, ShapeItem x) const
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  N);
             matrix_assert(y >= 0);
             matrix_assert(y <  M);
             return data[x*dpi + y*dY];
          }

       /// return the imaginary part of A[x;y]
       double & imag(ShapeItem y, ShapeItem x)
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  N);
             matrix_assert(y >= 0);
             matrix_assert(y <  M);
             return data[x*dpi + y*dY + 1];
          }

       /// return the imaginary part of A[x;y]
       const double & imag(ShapeItem y, ShapeItem x) const
          {
             matrix_assert(x >= 0);
             matrix_assert(x <  N);
             matrix_assert(y >= 0);
             matrix_assert(y <  M);
             return data[x*dpi + y*dY + 1];
          }

       inline double abs2(ShapeItem y, ShapeItem x) const;
       inline void col1_norm(norm_result & result) const;

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

       /// print this matrix boxed with name
       void debug(const char * name) const;

       protected:
       /// the matrix elements
       double * data;

       public:
       /// the number of matrix rows
       const ShapeItem M;

       /// the number of matrix columns
       const ShapeItem N;

       /// the distance (in doubles) between  A[i;j] and A[i;j+1]
       const ShapeItem dY;
     };
};

//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<false>::col1_norm(norm_result & result) const
{
double sum = 0;

   loop(y, M)
       {
         const double r = real(y, 0);
         sum += r*r;
       }
   result.norm2_real   = sum;
   result.norm2_imag   = 0;
   result.norm_real     = sqrt(sum);
   result.norm_imag    = 0;

   result.norm__2_real = 2.0 / result.norm2_real;
   result.norm__2_imag = 0.0;
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<true>::col1_norm(norm_result & result) const
{
double sum_real = 0;
double sum_imag = 0;

   loop(y, M)
       {
         const double r = real(y, 0);
         const double i = imag(y, 0);
         sum_real += r*r - i*i;
         sum_imag += 2*r*i;
       }

const complex<double> sum(sum_real, sum_imag);
   result.norm2_real = sum.real();
   result.norm2_imag = sum.imag();
const complex<double> root = sqrt(sum);
   result.norm_real  = root.real();
   result.norm_imag  = root.imag();

const complex<double> _2__sum = 2.0 / sum;
   result.norm__2_real = _2__sum.real();
   result.norm__2_imag = _2__sum.imag();
}
//-----------------------------------------------------------------------------
template<>
inline double
Bif_F12_DOMINO::Matrix<false>::abs2(ShapeItem y, ShapeItem x) const
{
const ShapeItem b = x*dpi + y*dY;
   return data[b]*data[b];
}
//-----------------------------------------------------------------------------
template<>
inline double
Bif_F12_DOMINO::Matrix<true>::abs2(ShapeItem y, ShapeItem x) const
{
const ShapeItem b = x*dpi + y*dY;
   return data[b]*data[b] + data[b+1]*data[b+1];
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
inline void Bif_F12_DOMINO::Matrix<false>::imbed(const Matrix<false>& S)
{
   matrix_assert(  M ==   N);   // Qi is quadratic (N×N)
   matrix_assert(S.M == S.N);   // S  is quadratic (M×M)
   matrix_assert(S.M <=   M);   // S is smaller than Qi

const ShapeItem IN = M - S.M;   // the number of rows and columns from ID N

   loop(y, M)
      {
         if (y < IN)   // upper row: init from identity matrix
            {
              loop(x, N)   real(y, x) = 0.0;
              real(y, y) = 1.0;   // diagonal
            }
         else          // lower row: maybe init from S
            {
              loop(x, N)
                  {
                    if (x < IN)   // left columns: init from identity matrix
                       real(y, x) = 0.0;
                    else          // right columns: init from S
                       {
                         real(y, x) = S.real(y - IN, x - IN);
                       }
                  }
            }
      }
}
//-----------------------------------------------------------------------------
template<>
inline void Bif_F12_DOMINO::Matrix<true>::imbed(const Matrix<true>& S)
{
   matrix_assert(  M ==   N);   // Qi is quadratic (N×N)
   matrix_assert(S.M == S.N);   // S  is quadratic (M×M)
   matrix_assert(S.M <=   M);   // S is smaller than Qi

const ShapeItem IN = M - S.M;   // the number of rows and columns from ID N

   loop(y, M)
      {
         if (y < IN)   // upper row: init from identity matrix
            {
              loop(x, N)   real(y, x) = imag(y, x) = 0.0;
              real(y, y) = 1.0;   // diagonal
            }
         else          // lower row: maybe init from S
            {
              loop(x, N)
                  {
                    if (x < IN)   // left columns: init from identity matrix
                       real(y, x) = imag(y, x) = 0.0;
                    else          // right columns: init from S
                       {
                         real(y, x) = S.real(y - IN, x - IN);
                         imag(y, x) = S.imag(y - IN, x - IN);
                       }
                  }
            }
      }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<false>::init_outer_product(const norm_result & scale,
                                                  const Matrix<false> & src)
{
   loop(y, M) loop(x, N)
       real(y, x) = scale.norm__2_real * src.real(y, 0) * src.real(x, 0);
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<true>::init_outer_product(const norm_result & scale,
                                                 const Matrix<true> & src)
{
const complex<double> sc(scale.norm__2_real, scale.norm__2_imag);
   loop(y, M) loop(x, N)
       {
         const complex<double> sx(src.real(x, 0), src.imag(x, 0));
         const complex<double> sy(src.real(y, 0), src.imag(y, 0));
         const complex<double> prod = sc*sx*sy;
         real(y, x) = prod.real();
         imag(y, x) = prod.imag();
       }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<false>::operator =(const Matrix<false> & src)
{
   matrix_assert(M == src.M);
   matrix_assert(N == src.N);
   loop(y, src.M) loop(x, src.N)
       {
         real(y, x) = src.real(y, x);
       }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<true>::operator =(const Matrix<true> & src)
{
   matrix_assert(M == src.M);
   matrix_assert(N == src.N);
   new(this)   Matrix<true>(data, src.M, src.N);
   loop(y, src.M) loop(x, src.N)
       {
         real(y, x) = src.real(y, x);
         imag(y, x) = src.imag(y, x);
       }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<false>::init_inner_product(const Matrix<false> & src_A,
                                                  const Matrix<false> & src_B)
{
const ShapeItem Z = src_A.N;
   matrix_assert(M == src_A.M);
   matrix_assert(N == src_B.N);
   matrix_assert(Z == src_A.N);
   matrix_assert(Z == src_B.M);

   loop(y, src_A.M)   // for every row of src_A
   loop(x, src_B.N)   // for every column of src_B
       {
         double sum = 0.0;
         loop(z, Z)   sum += src_A.real(y, z) * src_B.real(z, x);
         real(y, x) = sum;
       }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<true>::init_inner_product(const Matrix<true> & src_A,
                                                 const Matrix<true> & src_B)
{
const ShapeItem Z = src_A.N;
   matrix_assert(M == src_A.M);
   matrix_assert(N == src_B.N);
   matrix_assert(Z == src_A.N);
   matrix_assert(Z == src_B.M);
   new (this) Matrix<true>(data, src_A.M, src_B.N);

   loop(y, src_A.M)   // for every row y of src_A
   loop(x, src_B.N)   // for every column x of src_B
       {
         complex<double> sum(0.0, 0.0);
         loop(z, Z)
             {
               const complex<double> row_a(src_A.real(y, z), src_A.imag(y, z));
               const complex<double> col_b(src_B.real(z, x), src_B.imag(z, x));
                 sum += row_a * col_b;
             }
         real(y, x) = sum.real();
         imag(y, x) = sum.imag();
       }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<false>::transpose(ShapeItem M)
{
   for (ShapeItem y = 1; y < M; ++y)   // for every row below the first
   loop(x, y)                          // for every column left of the diagonal
      {
        double r = real(y, x); real(y, x) = real(x, y); real(x, y) = r;
      }
}
//-----------------------------------------------------------------------------
template<>
inline void
Bif_F12_DOMINO::Matrix<true>::transpose(ShapeItem M)
{
   for (ShapeItem y = 1; y < M; ++y)   // for every row below the first
   loop(x, y)                          // for every column left of the diagonal
      {
        double t = real(y, x);   real(y, x) = real(x, y);   real(x, y) = t;
               t = imag(y, x);   imag(y, x) = imag(x, y);   imag(x, y) = t;
      }
}
//-----------------------------------------------------------------------------

#endif // __BIF_F12_DOMINO_HH_DEFINED__

