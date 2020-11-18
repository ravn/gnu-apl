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

#ifndef __COMPLEXCELL_HH_DEFINED__
#define __COMPLEXCELL_HH_DEFINED__

#include "NumericCell.hh"
#include "IntCell.hh"

//-----------------------------------------------------------------------------
/**
 A cell containing a single APL complex value. This class essentially
 overloads certain functions in class Cell with complex number specific
 implementations.
 */
/// A celkl containing one complex numbver
class ComplexCell : public NumericCell
{
public:
   /// Construct an complex number cell from a complex number
   ComplexCell(APL_Complex c);

   /// overloaded Cell::init_other
   virtual void init_other(void * other, Value & cell_owner, const char * loc)
      const { new (other)   ComplexCell(value.cval[0], value.cval[1]); }

   /// Construct an complex number cell from real part \b r and imag part \b i.
   ComplexCell(APL_Float r, APL_Float i);

   /// overloaded Cell::is_complex_cell().
   virtual bool is_complex_cell() const   { return true; }

   /// Overloaded Cell::is_finite().
   virtual bool is_finite() const
      { return isfinite(value.cval[0]) && isfinite(value.cval[1]); }

   /// overloaded Cell::greater().
   virtual bool greater(const Cell & other) const;

   /// overloaded Cell::equal().
   virtual bool equal(const Cell & other, double qct) const;

   /// overloaded Cell::compare()
   virtual Comp_result compare(const Cell & other) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_ceiling(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_conjugate(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_direction(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_exponential(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_factorial(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_floor(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_magnitude(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_nat_log(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_negative(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_pi_times(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_pi_times_inverse(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_reciprocal(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_roll(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_add(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_subtract(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_divide(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_equal(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_logarithm(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_multiply(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_power(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_maximum(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_minimum(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_residue(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_circle_fun(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_circle_fun_inverse(Cell * Z, const Cell * A) const;

   /// the Quad_CR representation of this cell.
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

   /// return true iff this cell needs scaling (exponential format) in pctx.
   virtual bool need_scaling(const PrintContext &pctx) const;

   /// overloaded Cell::bif_add_inverse()
   virtual ErrorCode bif_add_inverse(Cell * Z, const Cell * A) const;

   /// overloaded Cell::bif_multiply_inverse()
   virtual ErrorCode bif_multiply_inverse(Cell * Z, const Cell * A) const;

   /// the square of the magnitude of aJb (= a² + b²)
   APL_Float mag2() const
      { return value.cval[0] * value.cval[0] + value.cval[1] * value.cval[1]; }

   /// the square of the magnitude (a² + b² for aJb)
   static APL_Float mag2(APL_Complex A)
      { return A.real() * A.real() + A.imag() * A.imag(); }

   /// initialize Z to real r
   static ErrorCode zv(Cell * Z, APL_Float r)
      { new (Z) ComplexCell(r, 0.0);   return E_NO_ERROR; }

   /// initialize Z to complex r + ij
   static ErrorCode zv(Cell * Z, APL_Float r, APL_Float j)
      { new (Z) ComplexCell(r, j);   return E_NO_ERROR; }

   /// initialize Z to APL_Complex v
   static ErrorCode zv(Cell * Z, APL_Complex v)
      { new (Z) ComplexCell(v.real(), v.imag());   return E_NO_ERROR; }

   /// the lanczos approximation for gamma(x + iy)
   static APL_Complex gamma(APL_Float x, const APL_Float & y);

   /// compute circle function \b fun
   static ErrorCode do_bif_circle_fun(Cell * Z, int fun, APL_Complex b);

protected:
   /// return the complex value of \b this cell
   APL_Complex cval() const
      { return APL_Complex(value.cval[0], value.cval[1]); }

   /// 1.0
   static APL_Complex ONE()       { return APL_Complex(1, 0); }

   /// i
   static APL_Complex PLUS_i()   { return APL_Complex(0, 1); }

   /// -i
   static APL_Complex MINUS_i()   { return APL_Complex(0, -1); }

   /// overloaded Cell::get_cell_type().
   virtual CellType get_cell_type() const
      { return CT_COMPLEX; }

   /// overloaded Cell::get_real_value().
   virtual APL_Float get_real_value() const;

   /// overloaded Cell::get_imag_value().
   virtual APL_Float get_imag_value() const;

   /// overloaded Cell::get_complex_value()
   virtual APL_Complex get_complex_value() const   { return cval(); }

   /// overloaded Cell::get_near_bool()
   virtual bool get_near_bool()  const;

   /// overloaded Cell::get_near_int()
   virtual APL_Integer get_near_int()  const;

   /// overloaded Cell::get_checked_near_int()
   virtual APL_Integer get_checked_near_int()  const
      {
        if (value.cval[0] < 0.0)   return APL_Integer(value.cval[0] - 0.3);
        else                       return APL_Integer(value.cval[0] + 0.3);
      }

   /// overloaded Cell::is_near_int()
   virtual bool is_near_int() const;

   /// overloaded Cell::is_near_int64_t()
   virtual bool is_near_int64_t() const;

   /// overloaded Cell::bif_near_int64_t()
   virtual ErrorCode bif_near_int64_t(Cell * Z) const;

   /// overloaded Cell::bif_within_quad_CT()
   virtual ErrorCode bif_within_quad_CT(Cell * Z) const;

   /// overloaded Cell::is_near_zero()
   virtual bool is_near_zero() const;

   /// overloaded Cell::is_near_one()
   virtual bool is_near_one() const;

   /// overloaded Cell::is_near_real()
   virtual bool is_near_real() const;

   /// overloaded Cell::get_classname()
   virtual const char * get_classname() const   { return "ComplexCell"; }

   /// overloaded Cell::CDR_size()
   virtual int CDR_size() const { return 16; }

   /// overloaded Cell::to_type()
   virtual void to_type()
      { new(this)   IntCell(0); }
};
//=============================================================================

#endif // __COMPLEXCELL_HH_DEFINED__
