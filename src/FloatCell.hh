/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#ifndef __FLOATCELL_HH_DEFINED__
#define __FLOATCELL_HH_DEFINED__

#include "RealCell.hh"

//-----------------------------------------------------------------------------
/*!
 A cell containing a single APL floating point value.  This class essentially
 overloads certain functions in class Cell with floating point number specific
 implementations.

 */
class FloatCell : public RealCell
{
public:
   /// Construct an floating point cell containing \b r.
   FloatCell(APL_Float r)
   { value.fval = r;}

   /// Overloaded Cell::is_float_cell().
   virtual bool is_float_cell()     const   { return true; }

   /// Overloaded Cell::greater().
   virtual bool greater(const Cell & other) const;

   /// Overloaded Cell::equal().
   virtual bool equal(const Cell & other, APL_Float qct) const;

   /// Overloaded Cell::bif_add().
   virtual ErrorCode bif_add(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_ceiling(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_conjugate(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_direction(Cell * Z) const;

   /// Overloaded Cell::bif_divide().
   virtual ErrorCode bif_divide(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_exponential(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_factorial(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_floor(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_magnitude(Cell * Z) const;

   /// Overloaded Cell::bif_multiply().
   virtual ErrorCode bif_multiply(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_power().
   virtual ErrorCode bif_power(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_nat_log(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_negative(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_pi_times(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_pi_times_inverse(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_reciprocal(Cell * Z) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_roll(Cell * Z) const;

   /// compare this with other, throw DOMAIN ERROR on illegal comparisons
   virtual Comp_result compare(const Cell & other) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_maximum(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_minimum(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_residue(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_subtract().
   virtual ErrorCode bif_subtract(Cell * Z, const Cell * A) const;

   /// the Quad_CR representation of this cell.
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

   /// return true iff this cell needs scaling (exponential format) in pctx.
   virtual bool need_scaling(const PrintContext &pctx) const
      { return need_scaling(value.fval, pctx.get_PP()); }

   /// return true if the integer part of val is longer than ⎕PP
   static bool is_big(APL_Float val, int quad_pp);

   /// Return true iff the (fixed point) floating point number num shall
   /// be printed in scaled form (like 1.2E6).
   static bool need_scaling(APL_Float val, int quad_pp);

   /// replace normal chars by special chars specified in ⎕FC
   static void map_FC(UCS_string & ucs);

   /// initialize Z to APL_Float v
   static ErrorCode zv(Cell * Z, APL_Float v)
      { new (Z) FloatCell(v);   return E_NO_ERROR; }

protected:
   ///  Overloaded Cell::get_cell_type().
   virtual CellType get_cell_type() const
      { return CT_FLOAT; }

   /// Overloaded Cell::get_real_value().
   virtual APL_Float    get_real_value() const   { return value.fval;  }

   /// Overloaded Cell::get_imag_value().
   virtual APL_Float get_imag_value() const   { return 0.0;  }

   /// Overloaded Cell::get_complex_value()
   virtual APL_Complex get_complex_value() const
      { return APL_Complex(value.fval, 0.0); }

   /// Overloaded Cell::get_near_bool().
   virtual bool get_near_bool()  const;

   /// Overloaded Cell::get_near_int().
   virtual APL_Integer get_near_int()  const
      { return near_int(value.fval); }

   /// Overloaded Cell::get_checked_near_int().
   virtual APL_Integer get_checked_near_int()  const
      { return APL_Integer(value.fval + 0.3); }

   /// Overloaded Cell::is_near_int().
   virtual bool is_near_int() const
      { return Cell::is_near_int(value.fval); }

   /// Overloaded Cell::is_near_zero().
   virtual bool is_near_zero() const
      { return value.fval >= -INTEGER_TOLERANCE
            && value.fval <   INTEGER_TOLERANCE; }

   /// Overloaded Cell::is_near_one().
   virtual bool is_near_one() const
      { return value.fval >= (1.0 - INTEGER_TOLERANCE)
            && value.fval <  (1.0 + INTEGER_TOLERANCE); }

   /// Overloaded Cell::is_near_real().
   virtual bool is_near_real() const
      { return true; }

   /// Overloaded Cell::get_classname().
   virtual const char * get_classname()   const   { return "FloatCell"; }

   /// Overloaded Cell::CDR_size()
   virtual int CDR_size() const { return 8; }
};
//=============================================================================

#endif // __FLOATCELL_HH_DEFINED__
