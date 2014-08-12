/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

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

#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ComplexCell.hh"
#include "ErrorCode.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
ComplexCell::ComplexCell(APL_Complex c)
{
   value.cpxp = new APL_Complex(c);
   Log(LOG_delete)
      CERR << "new    " << (const void *)value.cpxp << " at " LOC << endl;
}
//-----------------------------------------------------------------------------
ComplexCell::ComplexCell(APL_Float r, APL_Float i)
{
   value.cpxp = new APL_Complex(r, i);
   Log(LOG_delete)
      CERR << "new    " << (const void *)value.cpxp << " at " LOC << endl;
}
//-----------------------------------------------------------------------------
ComplexCell::~ComplexCell()
{
   Log(LOG_delete)   CERR << "delete " HEX(value.cpxp) << " at " LOC << endl;
   delete value.cpxp;
}
//-----------------------------------------------------------------------------
APL_Float
ComplexCell::get_real_value() const
{
   return value.cpxp->real();
}
//-----------------------------------------------------------------------------
APL_Float
ComplexCell::get_imag_value() const
{
   return value.cpxp->imag();
}
//-----------------------------------------------------------------------------
bool
ComplexCell::is_near_int(APL_Float qct) const
{
   return Cell::is_near_int(value.cpxp->real(), qct) &&
          Cell::is_near_int(value.cpxp->imag(), qct);
}
//-----------------------------------------------------------------------------
void
ComplexCell::demote_complex_to_real(APL_Float qct)
{
const bool small_real = value.cpxp->real() <  qct &&
                        value.cpxp->real() >  -qct;

const bool small_imag = value.cpxp->imag() <  qct &&
                        value.cpxp->imag() >  -qct;

const APL_Float qct1 = 100000*qct;
const bool large_real = value.cpxp->real() >  qct1 ||
                        value.cpxp->real() <  -qct1;

const bool large_imag = value.cpxp->imag() >  qct1 ||
                        value.cpxp->imag() <  -qct1;

   if (small_imag && large_real)
      {
        // imag is close to zero, so we demote this cell either
        // to a FloatCell, or to an IntCell
        //
        const double val = value.cpxp->real();
        delete value.cpxp;

        if (Cell::is_near_int(val, qct))
           {
             if (val < 0)   new (this)   IntCell(APL_Integer(val - 0.3));
             else           new (this)   IntCell(APL_Integer(val + 0.3));
           }
        else
           {
             new (this) FloatCell(val);
           }
      }
   else if (small_real && large_imag)
      {
        // real is close to zero, so we round it down.
        //
        *value.cpxp = APL_Complex(0.0, value.cpxp->imag());
      }
}
//-----------------------------------------------------------------------------
bool
ComplexCell::is_near_zero(APL_Float qct) const
{
   if (value.cpxp->real() >  qct)   return false;
   if (value.cpxp->real() < -qct)   return false;
   if (value.cpxp->imag() >  qct)   return false;
   if (value.cpxp->imag() < -qct)   return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
ComplexCell::is_near_one(APL_Float qct) const
{
   if (value.cpxp->real() >  (1.0 + qct))   return false;
   if (value.cpxp->real() <  (1.0 - qct))   return false;
   if (value.cpxp->imag() >  qct)           return false;
   if (value.cpxp->imag() < -qct)           return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
ComplexCell::equal(const Cell & A, APL_Float qct) const
{
   if (!A.is_numeric())   return false;
   return tolerantly_equal(A.get_complex_value(), get_complex_value(), qct);
}
//-----------------------------------------------------------------------------
void
ComplexCell::release(const char * loc)
{
   delete value.cpxp;
   new (this) Cell;
}
//-----------------------------------------------------------------------------
// monadic build-in functions...
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_factorial(Cell * Z) const
{
   if (is_near_real(Workspace::get_CT()))
      {
        const FloatCell fc(get_real_value());
        fc.bif_factorial(Z);
        return E_NO_ERROR;
      }

const APL_Float zr = get_real_value() + 1.0;
const APL_Float zi = get_imag_value();
const APL_Complex z(zr, zi);

   new (Z) ComplexCell(gamma(get_real_value(), get_imag_value()));
   if (errno)   return E_DOMAIN_ERROR;
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_conjugate(Cell * Z) const
{
   new (Z) ComplexCell(conj(*value.cpxp));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_negative(Cell * Z) const
{
   new (Z) ComplexCell(- *value.cpxp);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_direction(Cell * Z) const
{
const APL_Complex val = *value.cpxp; 
const APL_Float mag = abs(val);
   if (mag == 0.0)   new (Z) IntCell(0);
   else              new (Z) ComplexCell(val/mag);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_magnitude(Cell * Z) const
{
   new (Z) FloatCell(abs(*value.cpxp));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_reciprocal(Cell * Z) const
{
const APL_Float qct = Workspace::get_CT();
   if (is_near_zero(qct))  return E_DOMAIN_ERROR;

   if (is_near_real(qct))   new (Z) FloatCell(1.0/value.cpxp->real());
   else                     new (Z) ComplexCell(APL_Complex(1.0)/ *value.cpxp);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_roll(Cell * Z) const
{
const APL_Integer qio = Workspace::get_IO();
   if (!is_near_int(Workspace::get_CT()))   return E_DOMAIN_ERROR;

const APL_Integer set_size = get_checked_near_int();
   if (set_size <= 0)   return E_DOMAIN_ERROR;

const uint64_t rnd = Workspace::get_RL();
   new (Z) IntCell(qio + (rnd % set_size));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_pi_times(Cell * Z) const
{
   new (Z) ComplexCell(M_PI * *value.cpxp);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_pi_times_inverse(Cell * Z) const
{
   new (Z) ComplexCell(*value.cpxp / M_PI);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_ceiling(Cell * Z) const
{
const double r = value.cpxp->real();
const double i = value.cpxp->imag();
const double dr = r - floor(r);
const double di = i - floor(i);

   if (dr + di > 1.0)   // round real and imag up.
      new (Z) ComplexCell(ceil(r), ceil(i));
   else if (di >= dr)   // round real down and imag up
      new (Z) ComplexCell(floor(r), ceil(i));
   else                 // round real up and imag down
      new (Z) ComplexCell(ceil(r), floor(i));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_floor(Cell * Z) const
{
const double r = value.cpxp->real();
const double i = value.cpxp->imag();
const double dr = r - floor(r);
const double di = i - floor(i);

   if (dr + di < 1.0)   // round real and imag down.
      new (Z) ComplexCell(floor(r), floor(i));
   else if (di > dr)   // round real down and imag up
      new (Z) ComplexCell(floor(r), ceil (i));
   else                // round real up and imag down
      new (Z) ComplexCell(ceil (r), floor(i));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_exponential(Cell * Z) const
{
   new (Z) ComplexCell(exp(*value.cpxp));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_nat_log(Cell * Z) const
{
   new (Z) ComplexCell(log(*value.cpxp));
   Z->demote_complex_to_real(Workspace::get_CT());
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
// dyadic build-in functions...
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_add(Cell * Z, const Cell * A) const
{
   new (Z) ComplexCell(A->get_complex_value() + get_complex_value());
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_subtract(Cell * Z, const Cell * A) const
{
   new (Z) ComplexCell(A->get_complex_value() - get_complex_value());
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_multiply(Cell * Z, const Cell * A) const
{
   new (Z) ComplexCell(A->get_complex_value() * get_complex_value());
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_divide(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (is_near_zero(qct))
      {
         if (A->is_near_zero(qct))   new (Z) IntCell(1);
         else                        return E_DOMAIN_ERROR;
      }

   new (Z) ComplexCell(A->get_complex_value() / get_complex_value());
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_residue(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_near_zero(qct))
      {
        Z->init(*this);
      }
   else if (is_near_zero(qct))
      {
        new (Z) IntCell(0);
      }
   else if (A->is_numeric())
      {
        const APL_Complex a = A->get_complex_value();
        const APL_Complex b = *value.cpxp;

        // We divide A by B, round down the result, and subtract.
        //
        A->bif_divide(Z, this);     // Z = B/A
        Z->bif_floor(Z);            // Z = A/B rounded down.
        new (Z) ComplexCell(b - a*Z->get_complex_value());
      }
   else
      {
        return E_DOMAIN_ERROR;
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_maximum(Cell * Z, const Cell * A) const
{
   // maximum of complex numbers gives DOMAN ERROR if one of the cells
   // is not near-real.
   //
const APL_Float qct = Workspace::get_CT();

   if (get_imag_value() < -qct)   return E_DOMAIN_ERROR;   // not near-real
   if (get_imag_value() >  qct)   return E_DOMAIN_ERROR;   // not near-real

const APL_Float rval = get_real_value();

   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (APL_Float(a) >= rval)   new (Z) IntCell(a);
         else                        new (Z) FloatCell(rval);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a >= rval)   new (Z) FloatCell(a);
         else             new (Z) FloatCell(rval);
      }
   else
      {
        if (A->get_imag_value() < -qct)   return E_DOMAIN_ERROR;   // not near-real
        if (A->get_imag_value() >  qct)   return E_DOMAIN_ERROR;   // not near-real

         const APL_Float a = A->get_real_value();
         if (a >= rval)   new (Z) FloatCell(a);
         else             new (Z) FloatCell(rval);
      }

   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_minimum(Cell * Z, const Cell * A) const
{
   // minimum of complex numbers gives DOMAN ERROR if one of the cells
   // is not near real.
   //
const APL_Float qct = Workspace::get_CT();

   if (get_imag_value() < -qct)   return E_DOMAIN_ERROR;   // not near-real
   if (get_imag_value() >  qct)   return E_DOMAIN_ERROR;   // not near-real

const APL_Float rval = get_real_value();

   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (APL_Float(a) <= rval)   new (Z) IntCell(a);
         else                        new (Z) FloatCell(rval);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a <= rval)   new (Z) FloatCell(a);
         else             new (Z) FloatCell(rval);
      }
   else
      {
        if (A->get_imag_value() < -qct)   return E_DOMAIN_ERROR;   // not near-real
        if (A->get_imag_value() >  qct)   return E_DOMAIN_ERROR;   // not near-real

         const APL_Float a = A->get_real_value();
         if (a <= rval)   new (Z) FloatCell(a);
         else             new (Z) FloatCell(rval);
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_equal(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();
   if (A->is_complex_cell())
      {
        const APL_Complex diff = A->get_complex_value() - get_complex_value();
        if      (diff.real()      >  qct)   new (Z) IntCell(0);
        else if (diff.real()      < -qct)   new (Z) IntCell(0);
        else if (diff.imag()      >  qct)   new (Z) IntCell(0);
        else if (diff.imag()      < -qct)   new (Z) IntCell(0);
        else                                new (Z) IntCell(1);
      }
   else if (A->is_numeric())
      {
        const APL_Float diff = A->get_real_value() - get_real_value();
        if      (diff             >  qct)   new (Z) IntCell(0);
        else if (diff             < -qct)   new (Z) IntCell(0);
        else if (get_imag_value() >  qct)   new (Z) IntCell(0);
        else if (get_imag_value() < -qct)   new (Z) IntCell(0);
        else                                new (Z) IntCell(1);
      }
   else
      {
        new (Z) IntCell(0);
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_power(Cell * Z, const Cell * A) const
{
   // some A to the complex B-th power
   //
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

const APL_Float qct = Workspace::get_CT();
   if (Cell::is_near_int(value.cpxp->real(), qct) &&
       Cell::is_near_zero(value.cpxp->imag(), qct))
      {
        APL_Integer b = get_near_int(qct);
        const bool invert_Z = b < 0;
        if (invert_Z)   b = - b;

        if (b <= 1)   // special cases A⋆1, A⋆0, and A⋆¯1
           {
             if (b == 0)   return IntCell::z1(Z);  // A⋆0 is 1

             if (invert_Z)               return A->bif_reciprocal(Z);
             if (A->is_real_cell())      return A->bif_conjugate(Z);
             new (Z) ComplexCell(A->get_complex_value());
             return E_NO_ERROR;
           }
      }

   if (A->is_near_zero(qct))
      {
        if (get_real_value() < 0)   return E_DOMAIN_ERROR;
        return IntCell::z0(Z);
      }

   if (A->is_real_cell())
      {
        APL_Complex complex_A(A->get_real_value(), 0);
        APL_Complex z = pow(complex_A, get_complex_value());
        new (Z) ComplexCell(z);
        Z->demote_complex_to_real(Workspace::get_CT());
        return E_NO_ERROR;
      }

   // complex A to the complex B-th power
   //
   {
     APL_Complex z = pow(A->get_complex_value(), get_complex_value());
     new (Z) ComplexCell(z);
     Z->demote_complex_to_real(Workspace::get_CT());

     return E_NO_ERROR;
   }
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_logarithm(Cell * Z, const Cell * A) const
{
   if (A->is_real_cell())
      {
        new (Z) ComplexCell(log(*value.cpxp) / log(A->get_real_value()));
      }
   else if (A->is_complex_cell())
      {
        new (Z) ComplexCell(log(*value.cpxp) / log(A->get_complex_value()));
      }
   else
      {
        return E_DOMAIN_ERROR;
      }

   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_circle_fun(Cell * Z, const Cell * A) const
{
const APL_Integer fun = A->get_near_int(Workspace::get_CT());
   return do_bif_circle_fun(Z, fun);
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::bif_circle_fun_inverse(Cell * Z, const Cell * A) const
{
const APL_Integer fun = A->get_near_int(Workspace::get_CT());

   switch(fun)
      {
        case 1: case -1:
        case 2: case -2:
        case 3: case -3:
        case 5: case -5:
        case 6: case -6:
        case 7: case -7:
                return do_bif_circle_fun(Z, -fun);

        case -10:  // +A is self-inverse
                return do_bif_circle_fun(Z, fun);

        default: return E_DOMAIN_ERROR;
      }

   // not reached
   return E_DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
ComplexCell::do_bif_circle_fun(Cell * Z, int fun) const
{
const APL_Complex b = *value.cpxp;

   switch(fun)
      {
        case -12: ComplexCell(-b.imag(), b.real()).bif_exponential(Z);   break;
        case -11: new (Z) ComplexCell(-b.imag(), b.real());              break;
        case -10: new (Z) ComplexCell(b.real(), -b.imag());              break;
        case  -9: new (Z) ComplexCell(            b   );                 break;
        case  -7: // arctanh(z) = 0.5 (ln(1 + z) - ln(1 - z))
                  {
                    const APL_Complex b1      = ONE() + b;
                    const APL_Complex b_1     = ONE() - b;
                    const APL_Complex log_b1  = log(b1);
                    const APL_Complex log_b_1 = log(b_1);
                    const APL_Complex diff    = log_b1 - log_b_1;
                    const APL_Complex half    = 0.5 * diff;
                    new (Z) ComplexCell(half);                           break;
                  }
        case  -6: // arccosh(z) = ln(z + sqrt(z + 1) sqrt(z - 1))
                  {
                    const APL_Complex b1     = b + ONE();
                    const APL_Complex b_1    = b - ONE();
                    const APL_Complex root1  = sqrt(b1);
                    const APL_Complex root_1 = sqrt(b_1);
                    const APL_Complex prod   = root1 * root_1;
                    const APL_Complex sum    = b + prod;
                    const APL_Complex loga   = log(sum);
                    new (Z) ComplexCell(loga);                           break;
                  }
        case  -5: // arcsinh(z) = ln(z + sqrt(z^2 + 1))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex b2_1 = b2 + ONE();
                    const APL_Complex root = sqrt(b2_1);
                    const APL_Complex sum =  b + root;
                    const APL_Complex loga = log(sum);
                    new (Z) ComplexCell(loga);                           break;
                  }
        case  -4: new (Z) ComplexCell(sqrt(b*b - 1.0));                  break;
        case  -3: // arctan(z) = i/2 (ln(1 - iz) - ln(1 + iz))
                  {
                    const APL_Complex iz = APL_Complex(- b.imag(), b.real());
                    const APL_Complex piz = ONE() + iz;
                    const APL_Complex niz = ONE() - iz;
                    const APL_Complex log_piz = log(piz);
                    const APL_Complex log_niz = log(niz);
                    const APL_Complex diff = log_niz - log_piz;
                    const APL_Complex prod = APL_Complex(0, 0.5) * diff;
                    new (Z) ComplexCell(prod);                           break;
                  }
        case  -2: // arccos(z) = -i (ln( z + sqrt(z^2 - 1)))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex diff = b2 - ONE();
                    const APL_Complex root = sqrt(diff);
                    const APL_Complex sum = b + root;
                    const APL_Complex loga = log(sum);
                    const APL_Complex prod = MINUS_i() * loga;
                    new (Z) ComplexCell(prod);                           break;
                  }
        case  -1: // arcsin(z) = -i (ln(iz + sqrt(1 - z^2)))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex diff = ONE() - b2;
                    const APL_Complex root = sqrt(diff);
                    const APL_Complex sum  = APL_Complex(-b.imag(), b.real())
                                           + root;
                    const APL_Complex loga = log(sum);
                    const APL_Complex prod = MINUS_i() * loga;
                    new (Z) ComplexCell(prod);                           break;
                  }
        case   0: new (Z) ComplexCell(sqrt (1.0 - b*b));                 break;
        case   1: new (Z) ComplexCell( sin       (b));                   break;
        case   2: new (Z) ComplexCell( cos       (b));                   break;
        case   3: new (Z) ComplexCell( tan       (b));                   break;
        case   4: new (Z) ComplexCell(sqrt (1.0 + b*b));                 break;
        case   5: new (Z) ComplexCell( sinh      (b));                   break;
        case   6: new (Z) ComplexCell( cosh      (b));                   break;
        case   7: new (Z) ComplexCell( tanh      (b));                   break;
        case  -8:
        case   8: {
                    bool pos_8 = false;
                    if (b.real() >  0)      { if (b.imag() > 0)  pos_8 = true; }
                    else if (b.real() == 0) { if (b.imag() > 1)  pos_8 = true; }
                    else                    { if (b.imag() >= 0) pos_8 = true; }

                    if (fun == -8)   pos_8 = ! pos_8;

                    const APL_Complex sq = sqrt(-(1.0 + get_complex_value()
                                                      * get_complex_value()));

                    if (pos_8)   new (Z) ComplexCell(sq);
                    else         new (Z) ComplexCell(-sq);
                  }                                                      break;

        case   9: new (Z) FloatCell(     b.real());                      break;
        case  10: new (Z) FloatCell(sqrt(b.real()*b.real()
                                       + b.imag()* b.imag()));           break;
        case  11: new (Z) FloatCell(b.imag());                           break;
        case  12: new (Z) FloatCell(phase());                            break;

        return E_DOMAIN_ERROR;
      }

   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
APL_Float
ComplexCell::phase() const
{
   if (is_near_zero(Workspace::get_CT()))   return 0.0;

APL_Float real = value.cpxp->real();
APL_Float imag = value.cpxp->imag();

   return atan2(imag, real);
}
//-----------------------------------------------------------------------------
APL_Complex
ComplexCell::gamma(APL_Float x, const APL_Float y)
{
   if (x < 0.5)
      return M_PI / sin(M_PI * APL_Complex(x, y)) * gamma(1 - x, -y);

   // coefficients for lanczos approximation of the gamma function.
   //
#define p0 APL_Complex(  1.000000000190015   )
#define p1 APL_Complex( 76.18009172947146    )
#define p2 APL_Complex(-86.50532032941677    )
#define p3 APL_Complex( 24.01409824083091    )
#define p4 APL_Complex( -1.231739572450155   )
#define p5 APL_Complex(  1.208650973866179E-3)
#define p6 APL_Complex( -5.395239384953E-6   )

   errno = 0;
   feclearexcept(FE_ALL_EXCEPT);

   x += 1.0;

const APL_Complex z(x, y);

const APL_Complex ret( (sqrt(2*M_PI) / z)
                      * (p0                            +
                          p1 / APL_Complex(x + 1.0, y) +
                          p2 / APL_Complex(x + 2.0, y) +
                          p3 / APL_Complex(x + 3.0, y) +
                          p4 / APL_Complex(x + 4.0, y) +
                          p5 / APL_Complex(x + 5.0, y) +
                          p6 / APL_Complex(x + 6.0, y))
                       * pow(z + 5.5, z + 0.5)
                       * exp(-(z + 5.5))
                     );

   // errno may be set and is checked by the caller

   return ret;

#undef p0
#undef p1
#undef p2
#undef p3
#undef p4
#undef p5
#undef p6
}
//=============================================================================
// throw/nothrow boundary. Functions above MUST NOT (directly or indirectly)
// throw while funcions below MAY throw.
//=============================================================================

#include "Error.hh"   // throws

//-----------------------------------------------------------------------------
bool
ComplexCell::get_near_bool(APL_Float qct)  const   
{
   if (value.cpxp->imag() >  qct)   DOMAIN_ERROR;
   if (value.cpxp->imag() < -qct)   DOMAIN_ERROR;

   if (value.cpxp->real() > qct)   // 1 or invalid
      {
        if (value.cpxp->real() < (1.0 - qct))   DOMAIN_ERROR;
        if (value.cpxp->real() > (1.0 + qct))   DOMAIN_ERROR;
        return true;
      }
   else
      {
        if (value.cpxp->real() < -qct)   DOMAIN_ERROR;
        return false;
      }
}
//-----------------------------------------------------------------------------
APL_Integer
ComplexCell::get_near_int(APL_Float qct) const
{
// if (value.cpxp->imag() >  qct)   DOMAIN_ERROR;
// if (value.cpxp->imag() < -qct)   DOMAIN_ERROR;

const double val = value.cpxp->real();
const double result = round(val);
const double diff = val - result;
   if (diff > qct)    DOMAIN_ERROR;
   if (diff < -qct)   DOMAIN_ERROR;

   return APL_Integer(result + 0.3);
}
//-----------------------------------------------------------------------------
bool
ComplexCell::greater(const Cell * other, bool ascending) const
{
   switch(other->get_cell_type())
      {
        case CT_CHAR:    return ascending;
        case CT_INT:
        case CT_FLOAT:
        case CT_COMPLEX: break;
        case CT_POINTER: return !ascending;
        case CT_CELLREF: DOMAIN_ERROR;
        default:         Assert(0 && "Bad celltype");
      }

const Comp_result comp = compare(*other);
   if (comp == COMP_EQ)   return this > other;
   if (comp == COMP_GT)   return ascending;
   else                   return !ascending;
}
//-----------------------------------------------------------------------------
Comp_result
ComplexCell::compare(const Cell & other) const
{
   // comparison of complex numbers gives DOMAN ERROR if one of the cells
   // is not near-real.
   //
const APL_Float qct = Workspace::get_CT();

   if (get_imag_value() < -qct)   DOMAIN_ERROR;   // not near-real
   if (get_imag_value() >  qct)   DOMAIN_ERROR;   // not near-real

const APL_Float rval = get_real_value();
   if (other.is_integer_cell())   // integer
      {
        if (equal(other, qct))   return COMP_EQ;
        return (rval < other.get_int_value()) ? COMP_LT : COMP_GT;
      }

   if (other.is_float_cell())
      {
        if (equal(other, qct))   return COMP_EQ;
        return (rval < other.get_real_value()) ? COMP_LT : COMP_GT;
      }

   if (other.is_complex_cell())   // complex
      {
        if (other.get_imag_value() < -qct)   DOMAIN_ERROR;   // not near-real
        if (other.get_imag_value() >  qct)   DOMAIN_ERROR;   // not near-real

        if (equal(other, qct))   return COMP_EQ;
        return (rval < other.get_real_value()) ? COMP_LT : COMP_GT;
        return COMP_EQ;
      }

   if (other.is_character_cell())
      {
        return COMP_GT;
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
PrintBuffer
ComplexCell::character_representation(const PrintContext & pctx) const
{
   if (pctx.get_PP() < MAX_Quad_PP)
      {
         // 10⋆get_PP()
         //
         APL_Float ten_to_PP = 1.0;
         loop(p, pctx.get_PP())   ten_to_PP *= 10.0;
      
         // lrm p. 13: In J notation, the real or imaginary part is not
         // displayed if it is less than the other by more than ⎕PP orders
         // of magnitude (unless ⎕PP is at its maximum).
         //
         const APL_Float pos_real = value.cpxp->real() < 0
                                  ? -value.cpxp->real() : value.cpxp->real();
         const APL_Float pos_imag = value.cpxp->imag() < 0
                                  ? -value.cpxp->imag() : value.cpxp->imag();

         if (pos_real >= pos_imag)   // pos_real dominates pos_imag
            {
              if (pos_real > pos_imag*ten_to_PP)
                 {
                   const FloatCell real_cell(value.cpxp->real());
                   return real_cell.character_representation(pctx);
                 }
            }
         else                        // pos_imag dominates pos_real
            {
              if (pos_imag > pos_real*ten_to_PP)
                 {
                   const FloatCell imag_cell(value.cpxp->imag());
                   PrintBuffer ret = imag_cell.character_representation(pctx);
                   ret.pad_l(UNI_ASCII_J, 1);
                   ret.pad_l(UNI_ASCII_0, 1);
                   
                   ret.get_info().flags |= CT_COMPLEX;
                   ret.get_info().imag_len = 1 + ret.get_info().real_len;
                   ret.get_info().int_len = 1;
                   ret.get_info().fract_len = 0;
                   ret.get_info().real_len = 1;
                   return ret;
                 }
            }
      }

bool scaled_real = pctx.get_scaled();   // may be changed by print function
UCS_string ucs(value.cpxp->real(), scaled_real, pctx);

ColInfo info;
   info.flags |= CT_COMPLEX;
   if (scaled_real)   info.flags |= real_has_E;
int int_fract = ucs.size();
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   loop(u, ucs.size())
      {
       if (ucs[u] == UNI_ASCII_FULLSTOP)
           {
             info.int_len = u;
             if (!scaled_real)   break;
             continue;
           }

        if (ucs[u] == UNI_ASCII_E)
           {
             if (info.int_len > u)   info.int_len = u;
             int_fract = u;
             break;
           }
      }
   info.fract_len = int_fract - info.int_len;

   if (!is_near_real(Workspace::get_CT()))
      {
        ucs.append(UNI_ASCII_J);
        bool scaled_imag = pctx.get_scaled();  // may be changed by UCS_string()
        const UCS_string ucs_i(value.cpxp->imag(), scaled_imag, pctx);

        ucs.append(ucs_i);

        info.imag_len = ucs.size() - info.real_len;
        if (scaled_imag)   info.flags |= imag_has_E;
      }

   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
bool
ComplexCell::need_scaling(const PrintContext &pctx) const
{
   // a complex number needs scaling if the real part needs it, ot
   // the complex part is significant and needs it.
   return FloatCell::need_scaling(value.cpxp->real(), pctx.get_PP()) ||
          (!is_near_real(Workspace::get_CT()) && 
          FloatCell::need_scaling(value.cpxp->imag(), pctx.get_PP()));
}
//-----------------------------------------------------------------------------
