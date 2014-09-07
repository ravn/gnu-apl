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

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "main.hh"
#include "LvalCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Value.icc"
#include "SystemLimits.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
void *
Cell::operator new(std::size_t s, void * pos)
{
   return pos;
}
//-----------------------------------------------------------------------------
void
Cell::init(const Cell & other)
{
   Assert(&other);
   switch(other.get_cell_type())
      {
        default:
             Assert(0);

        case CT_CHAR:
             new (this) CharCell(other.get_char_value());
             return;

        case CT_INT:
             new (this) IntCell(other.get_int_value());
             return;

        case CT_FLOAT:
             new (this) FloatCell(other.get_real_value());
             return;

        case CT_COMPLEX:
             new (this) ComplexCell(other.get_complex_value());
             return;

        case CT_POINTER:
             {
               Value_P Z = other.get_pointer_value()->clone(LOC);

               new (this) PointerCell(Z);
             }
             return;

        case CT_CELLREF:
             new (this) LvalCell(other.get_lval_value());
             return;
      }

   Assert(0 && "Bad cell type");
}
//-----------------------------------------------------------------------------
void
Cell::init_from_value(Value_P value, const char * loc)
{
   if (value->is_scalar())
      {
        init(value->get_ravel(0));
      }
   else
      {
        new (this) PointerCell(value);
      }
}
//-----------------------------------------------------------------------------
Value_P
Cell::to_value(const char * loc) const
{
Value_P ret;
   if (is_pointer_cell())
      {
        ret = get_pointer_value(); // ->clone(LOC);
      }
   else
      {
        ret = Value_P(new Value(loc));
        ret->get_ravel(0).init(*this);
        ret->check_value(LOC);
      }

   return ret;
}
//-----------------------------------------------------------------------------
Cell *
Cell::init_type(const Cell & other)
{
   // Note: this function changes the type of this cell, but the
   // compiler may not notice that and use the declaration for this
   // Cell. Using the return value avoids that.
   //
   if (other.is_pointer_cell())
      {
        new (this) PointerCell(other.get_pointer_value()->clone(LOC));
        get_pointer_value()->to_proto();
      }
   else if (other.is_lval_cell())
      {
        FIXME;
      }
   else if (other.is_character_cell())
      {
        new (this) CharCell(UNI_ASCII_SPACE);
      }
   else // numeric
      {
        new (this) IntCell(0);
      }

   return this;
}
//-----------------------------------------------------------------------------
void
Cell::copy(Value & val, const Cell * & src, ShapeItem count)
{
   loop(c, count)
      {
        Assert1(val.more());
        val.next_ravel()->init(*src++);
      }
}
//-----------------------------------------------------------------------------
bool
Cell::greater(const Cell * other, bool ascending) const
{
   CERR << "greater() called on object of class" << get_classname() << endl;
   Assert(0);
}
//-----------------------------------------------------------------------------
bool
Cell::equal(const Cell & other, APL_Float qct) const
{
   CERR << "equal() called on object of class" << get_classname() << endl;
   Assert(0);
}
//-----------------------------------------------------------------------------
bool
Cell::tolerantly_equal(APL_Complex A, APL_Complex B, APL_Float C)
{
   if (A == B)                             return true;
   if (A.real() < 0.0 && B.real() > 0.0)   return false;
   if (A.real() > 0.0 && B.real() < 0.0)   return false;
   if (A.imag() < 0.0 && B.imag() > 0.0)   return false;
   if (A.imag() > 0.0 && B.imag() < 0.0)   return false;

const APL_Float mag2_A  = A.real() * A.real() + A.imag() * A.imag();
const APL_Float mag2_B  = B.real() * B.real() + B.imag() * B.imag();
const APL_Float max_mag = sqrt((mag2_A > mag2_B) ? mag2_A : mag2_B);

const APL_Complex A_B = A - B;
const APL_Float dist2_A_B = sqrt(A_B.real() * A_B.real()
                               + A_B.imag() * A_B.imag());

   return (dist2_A_B < C*max_mag);
}
//-----------------------------------------------------------------------------
bool
Cell::tolerantly_equal(APL_Float A, APL_Float B, APL_Float C)
{
   // if the signs of A and B differ then they are unequal (ISO standard
   // page 19). We treat exact 0.0 as having both signs
   //
   if (A == B)               return true;
   if (A < 0.0 && B > 0.0)   return false;
   if (A > 0.0 && B < 0.0)   return false;

APL_Float mag_A = A < 0 ? -A : A;
APL_Float mag_B = B < 0 ? -B : B;
APL_Float max_mag = (mag_A > mag_B) ? mag_A : mag_B;

const APL_Float dist_A_B = (A > B) ? (A - B) : (B - A);

   return (dist_A_B < C*max_mag);
}
//-----------------------------------------------------------------------------
bool
Cell::is_near_int(APL_Float value, APL_Float qct)
{
   if (value > LARGE_INT)   return false;
   if (value < SMALL_INT)   return false;

const double result = nearbyint(value);
const double diff = value - result;
   if (diff > qct)    return false;
   if (diff < -qct)   return false;

   return true;
}
//-----------------------------------------------------------------------------
APL_Integer
Cell::near_int(APL_Float value, APL_Float qct)
{
const double result = nearbyint(value);
const double diff = value - result;
   if (diff > qct)    DOMAIN_ERROR;
   if (diff < -qct)   DOMAIN_ERROR;

   if (result > 0)   return   APL_Integer(0.3 + result);
   else              return - APL_Integer(0.3 - result);
}
//-----------------------------------------------------------------------------
void 
Cell::make_heap(const Cell ** a, ShapeItem heapsize, int64_t i, bool ascending,
                const void * comp_arg, greater_fun greater)
{
const int64_t l = 2*i + 1;   // left  child of i.
const int64_t r = l + 1;     // right child of i.
int64_t max = i;   // assume parent is the max.

    if ((l < heapsize) && (*greater)(a[l], a[max], ascending, comp_arg))
       max = l;   // left child is larger
    if ((r < heapsize) && (*greater)(a[r], a[max], ascending, comp_arg))
       max = r;   // right child is larger
    if (max != i)   // parent was not the max: exchange it.
    {
        const Cell * t = a[max];   a[max] = a[i];   a[i] = t;
        make_heap(a, heapsize, max, ascending, comp_arg, greater);
    }
}
//-----------------------------------------------------------------------------
void 
Cell::init_heap(const Cell ** a, ShapeItem heapsize, bool ascending,
                const void * comp_arg, greater_fun greater)
{
   for (int64_t p = heapsize/2 - 1; p >= 0; --p)
       make_heap(a, heapsize, p, ascending, comp_arg, greater);

   // here a is a heap, i.e. a[i >= a[2i+1], a[2i+2]
}
//-----------------------------------------------------------------------------
void
Cell::heapsort(const Cell ** a, ShapeItem heapsize, bool ascending,
               const void * comp_arg, greater_fun greater)
{
   // Sort a[] into a heap.
   //
   init_heap(a, heapsize, ascending, comp_arg, greater);

   // here a[] is a heap, with a[0] being the largest node.

   for (int k = heapsize - 1; k > 0; k--)
       {
         // The root a[0] is the largest element (in a[0]..a[k]).
         // Store the root, replace it by another element.
         //
         const Cell * t = a[k];   a[k] = a[0];   a[0] = t;

         // Sort a[] into a heap again.
         //
         make_heap(a, k, 0, ascending, comp_arg, greater);
       }
}
//-----------------------------------------------------------------------------
bool
Cell::greater_vec(const Cell * ca, const Cell * cb, bool ascending, 
                  const void * comp_arg)
{
const ShapeItem comp_len = *(const ShapeItem *)comp_arg;
const APL_Float qct = Workspace::get_CT();

   // most frequently comp_len is 1, so we optimize for this case.
   //
   if (comp_len == 1)
      {
        const bool equal = ca[0].equal(cb[0], qct);
        if (equal)   return ca > cb;
        const bool result = ca[0].greater(&cb[0], ascending);
        return result;
      }

   loop(c, comp_len)
      {
        const bool equal = ca[c].equal(cb[c], qct);
        if (equal)   continue;
        const bool result = ca[c].greater(cb + c, ascending);
        return result;
      }

   return ca > cb;   // a and b are equal: sort by position
}
//-----------------------------------------------------------------------------
ostream & 
operator <<(ostream & out, const Cell & cell)
{
PrintBuffer pb = cell.character_representation(PR_BOXED_GRAPHIC);
UCS_string ucs(pb, 0, Workspace::get_PrintContext().get_PW());
   return out << ucs << " ";
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_equal(Cell * Z, const Cell * A) const
{
APL_Integer eq = 0;   // assume incompatible cell types ( == not equal)

   if (is_character_cell() == A->is_character_cell())   // compatible cell types
      {
       eq = equal(*A, Workspace::get_CT());
      }

   new (Z) IntCell(eq);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_not_equal(Cell * Z, const Cell * A) const
{
APL_Integer neq = 1;   // assume incompatible cell types ( == not equal)

   if (is_character_cell() == A->is_character_cell())   // compatible cell types
      {
       neq = !equal(*A, Workspace::get_CT());
      }

   new (Z) IntCell(neq);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_greater_than(Cell * Z, const Cell * A) const
{
   new (Z) IntCell((A->compare(*this) == COMP_GT) ? 1 : 0);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_less_eq(Cell * Z, const Cell * A) const
{
   new (Z) IntCell((A->compare(*this) != COMP_GT) ? 1 : 0);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_less_than(Cell * Z, const Cell * A) const
{
   new (Z) IntCell((A->compare(*this) == COMP_LT) ? 1 : 0);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_greater_eq(Cell * Z, const Cell * A) const
{
   new (Z) IntCell((A->compare(*this) != COMP_LT) ? 1 : 0);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
