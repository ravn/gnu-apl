/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2022  Dr. Jürgen Sauermann

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

#ifndef __ARRAY_ITERATOR_HH_DEFINED__
#define __ARRAY_ITERATOR_HH_DEFINED__

#include "Common.hh"
#include "Shape.hh"
#include "SystemLimits.hh"

//-----------------------------------------------------------------------------
/// An iterator counting 0, 1, 2, ... ⍴[N-1] along one axis of length N
class AxisIterator
{
public:
   /// default constructor for arrays. Initializes only the counters.
   AxisIterator()
   {}

   /// constructor for placement new. A weight of 0 indicates a scalar
   AxisIterator(ShapeItem len, ShapeItem weight, bool _wrap)
   : axis_length(len),
     axis_weight(weight),
     shape_offset(0),
     ravel_offset(0),
     wrap(_wrap)
   {}

   /** increment the offsets and return:

       true  (= "carry")       if the end of the axis was reached, or else
       false (= no more items) if the end was NOT reached
    **/

   bool next()
      {
        ravel_offset += axis_weight;

        // NOTE: increment of scalar shape_offset needed for more() below.
        if (++shape_offset < axis_length || is_scalar_iterator())  return false;

        // carry has occurred.
        if (wrap)   shape_offset = ravel_offset = 0;
        return true;   // carry: increment next (if any) iterator
     }

   /// true if more items coming
   bool more() const
      { return is_scalar_iterator() ? shape_offset == 0   // before next()
                                    : shape_offset < axis_length; }

   /// return the current shape offset (not weighted)
   ShapeItem get_shape_offset() const
      { return shape_offset; }

   /// return the current ravel offset (weighted)
   ShapeItem get_ravel_offset() const
      { return ravel_offset; }

   /// true if \b this is an iterator for a scalar value.
   bool is_scalar_iterator() const
      { return axis_weight == 0; }

protected:
   /// the length of the axis; the interator counts 0, 1, ... axis_length-1.
   ShapeItem axis_length;

   /// the weight (ravel increment) of this itertor (product of lower iterator
   /// weights). IMPORTANT: Scalars are flagged with an axis_weight of 0 here.
   ShapeItem axis_weight;

   /// the current offset, 0, 1, ... axis_len (excluding)
   ShapeItem shape_offset;

   /// the current offset, 0, weight, ... weight*axis_len (excluding)
   ShapeItem ravel_offset;

   /// true iff \b this iterator shall wrap around at the end. In order for
   /// more() of the entire array to be fast, we wrap all iterators except
   /// the first at the end so that more() of the first iterator becomes false
   /// when iterating over entire array is done.
   bool wrap;
};
//-----------------------------------------------------------------------------
/// An iterator counting along all axes of \b shape
class ArrayIterator
{
public:
   /// constructor
   ArrayIterator(const Shape & shape)
   : rank(shape.get_rank())
      {
        ShapeItem weight = 1;
        for (Axis a = rank - 1; a > 0; --a)
            {
              // axis_iterators[pos == 0] is the one with the highest weight
              const ShapeItem len = shape.get_shape_item(a);
              new (axis_iterators + a) AxisIterator(len, weight, true);
              weight *= len;
            }

        if (rank == 0)   // scalar
           {
             new (axis_iterators) AxisIterator(1, 0, false);
           }
        else
           {
             // the final iterator (which shall not wrap.)
             const ShapeItem len = shape.get_shape_item(0);
             new (axis_iterators) AxisIterator(len, weight, false);
           }
      }

   /// return true iff this iterator has more items to come.
   bool more() const
      {
        return axis_iterators[0].more();
      }

   /// the work-horse of this iterator.
   void operator++()
      {
        if (rank == 0)   // scalar
           {
             axis_iterators->next();
             return;
           }

        for (Axis a = rank - 1; a >= 0; --a)
            {
              if (!axis_iterators[a].next())   break;   // axis_iterators[0]
            }
      }

   /// Get the current offset for axis r
   ShapeItem get_shape_offset(Axis a) const
      { return axis_iterators[a].get_shape_offset(); }

   /// Get the current total offset
   ShapeItem get_ravel_offset() const
      {
        ShapeItem ret = 0;
        loop(a, rank)   ret += axis_iterators[a].get_ravel_offset();
        return ret;
      }

   /// return the offsets of the axis_iterators as a shape
   Shape get_shape_offsets()
      {
        Shape ret;
        loop(r, rank)
            ret.add_shape_item(axis_iterators[r].get_shape_offset());
        return ret;
      }

   /// multiply and cumulate the current ravel offsets
   ShapeItem multiply(const Shape & weights) const
      {
        Assert1(rank == weights.get_rank());
        ShapeItem ret = 0;
        for (Axis a = rank - 1; a >= 0; --a)
            ret += axis_iterators[a].get_shape_offset()
                 * weights.get_shape_item(a);
        return ret;
      }

protected:
   /// the number of valid axes (= iterators).
   uRank rank;

   /** axis iterators.                   Shape:  ⊏sh0:sh1:...⊐
                                                    │   │
       axis_iterators[0] ←→ get_shape_item(0), ─────┘   │       largest weight
       axis_iterators[1] ←→ get_shape_item(1), ─────────┘ ...
    **/

   AxisIterator axis_iterators[MAX_RANK];
};
//=============================================================================

#endif // __ARRAY_ITERATOR_HH_DEFINED__
