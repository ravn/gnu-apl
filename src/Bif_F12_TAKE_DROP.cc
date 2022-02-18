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

#include "Bif_F12_PARTITION_PICK.hh"
#include "Bif_OPER1_EACH.hh"
#include "Bif_F12_TAKE_DROP.hh"
#include "Workspace.hh"

// primitive function instances
//
Bif_F12_TAKE      Bif_F12_TAKE     ::_fun;    // ↑
Bif_F12_DROP      Bif_F12_DROP     ::_fun;    // ↓

// primitive function pointers
//
Bif_F12_TAKE      * Bif_F12_TAKE     ::fun = &Bif_F12_TAKE     ::_fun;
Bif_F12_DROP      * Bif_F12_DROP     ::fun = &Bif_F12_DROP     ::_fun;

//============================================================================
Value_P
Bif_F12_TAKE::first(Value_P B)
{
const Cell & first_B = B->get_cfirst();
   if (B->element_count() == 0)   // empty value: return prototype
      {
        if (first_B.is_lval_cell())   // (↑...)←V
            {
              Value_P Z(LOC);
              Z->next_ravel_Cell(first_B);
              return Z;
            }

        // normal (right-) value
        //
        Value_P Z = B->prototype(LOC);
        Z->check_value(LOC);
        return Z;
      }

   if (!first_B.is_pointer_cell())   // simple cell
      {
        Value_P Z(LOC);
        Z->get_wscalar().init(first_B, Z.getref(), LOC);
        Z->check_value(LOC);
        return Z;
      }

Value_P v1 = first_B.get_pointer_value();
Value * v1_owner = v1->get_lval_cellowner();
   if (v1_owner)   // B is a left value
      {
        Value_P B1(LOC);
        B1->next_ravel_Pointer(v1.get());
        B1->check_value(LOC);

        Value_P Z(LOC);
        Z->next_ravel_Lval(&B1->get_wscalar(), v1_owner);

        Z->check_value(LOC);
        return Z;
      }
   else
      {
        const ShapeItem ec = v1->element_count();
        Value_P Z(v1->get_shape(), LOC);
        if (ec == 0)   Z->set_default(v1.getref(), LOC);

        loop(e, ec)   Z->next_ravel_Cell(v1->get_cravel(e));

        Z->check_value(LOC);
        return Z;
      }
}
//----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (X->get_rank() > 1)   RANK_ERROR;

const ShapeItem len_A = A->element_count();
const ShapeItem len_X = X->element_count();
   if (len_A != len_X)   LENGTH_ERROR;

   if (len_X == 0)   // no axes
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   // construct the left argument of Bif_F12_TAKE::fill().
   //  Start with ⍴B and then replace its shape items with the shape items
   // A[X[i]]. Reject duplicate items in X.
   //
const APL_Integer qio = Workspace::get_IO();
ShapeItem axes_X = 0;
Shape sh_take = B->get_shape();
   loop(x, len_X)
      {
        const APL_Integer axis = X->get_cravel(x).get_near_int() - qio;
         if (axis < 0)
           {
             MORE_ERROR() << "axis " << axis << " too small in X of ↑[X]B";
             AXIS_ERROR;
           }

         if (axis >= B->get_rank())
           {
             MORE_ERROR() << "axis " << axis << " too large in X of A ↑[X]B";
             AXIS_ERROR;
           }

        if (axes_X & 1 << axis)   // duplicate axis
           {
             MORE_ERROR() << "duplicate axis " << axis << " in X of A ↑[X]B";
             AXIS_ERROR;
           }

        axes_X |= 1 << axis;   // remember axis to detect duplicates

        const APL_Integer alen = A->get_cravel(x).get_near_int();
        sh_take.set_shape_item(axis, alen);
      }

   return Token(TOK_APL_VALUE1, do_take(sh_take, B.getref(), true));
}
//----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_XB(Value_P X, Value_P B) const
{
   // ↑[X] B    ←→ ((⍴X)⍴1)↑[X] B   (GNU APL only)
   if (X->get_rank() > 1)   AXIS_ERROR;

const APL_Integer qio = Workspace::get_IO();
ShapeItem axes_X = 0;
Shape sh_take = B->get_shape();
   loop(x, X->element_count())
       {
        const APL_Integer axis = X->get_cravel(x).get_near_int() - qio;
         if (axis < 0)
           {
             MORE_ERROR() << "axis " << axis << " too small in X of ↑[X]B";
             AXIS_ERROR;
           }

         if (axis >= B->get_rank())
           {
             MORE_ERROR() << "axis " << axis << " too large in X of ↑[X]B";
             AXIS_ERROR;
           }

        if (axes_X & 1 << axis)   // duplicate axis
           {
             MORE_ERROR() << "duplicate axis " << axis << " in X of ↑[X]B";
             AXIS_ERROR;
           }

        axes_X |= 1 << axis;   // remember axis to detect duplicates

        sh_take.set_shape_item(axis, 1);
       }

   return Token(TOK_APL_VALUE1, do_take(sh_take, B.getref(), true));
}
//----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_AB(Value_P A, Value_P B) const
{
Shape ravel_A1(A.get(), /* ⎕IO */ 0);   // checks that 1 ≤ ⍴⍴A and ⍴A ≤ MAX_RANK

   if (B->is_scalar())
      {
        Shape shape_B1;
        loop(a, ravel_A1.get_rank())   shape_B1.add_shape_item(1);
        Value_P B1 = B->clone(LOC);   // so that we can set_shape()
        B1->set_shape(shape_B1);
        return Token(TOK_APL_VALUE1, do_take(ravel_A1, B1.getref(), false));
      }
   else
      {
        if (ravel_A1.get_rank() != B->get_rank())   LENGTH_ERROR;
        return Token(TOK_APL_VALUE1, do_take(ravel_A1, B.getref(), false));
      }
}
//----------------------------------------------------------------------------
Value_P
Bif_F12_TAKE::do_take(const Shape & ravel_A1, const Value &  B,  bool axes)
{
   // ravel_A1 can have negative items (for take from the end).
   //
Value_P Z(ravel_A1.abs(), LOC);

   if (ravel_A1.is_empty())   Z->set_default(B, LOC);
   else                       fill(ravel_A1, Z.getref(), B, axes);
   Z->check_value(LOC);
   return Z;
}
//----------------------------------------------------------------------------
void
Bif_F12_TAKE::fill(const Shape & shape_Zi, Value & Z,
                   const Value & B, bool axes)
{
   for (TakeDropIterator i(true, shape_Zi, B.get_shape()); i.more(); ++i)
       {
         const ShapeItem offset = i();
         if (offset != -1)                          // valid cell
            {
              Z.next_ravel_Cell(B.get_cravel(offset));
            }
         else if (axes)                             // overtake from axis
            {
              const ShapeItem offset = i.axis_proto();
              Z.next_ravel_Proto(B.get_cravel(offset));
            }
         else                                       // overtake from ↑B
            {
              Z.next_ravel_Proto(B.get_cproto());
            }
       }
}
//============================================================================
Token
Bif_F12_DROP::eval_AB(Value_P A, Value_P B) const
{
const Shape ravel_A(A.get(), /* ⎕IO */ 0);
   if (A->get_rank() > 1)   RANK_ERROR;

   if (B->is_scalar())
      {
        /* Z←A↓B with scalar B has an element count() of 1 (if all items of A
           are are 0 thus nothing is dropped) or else 1.

           ⍴⍴Z is ⍴⍴A and ⍴Z[j] ←→  A[j] = 0
         */
        Shape shape_Z;
        loop(r, ravel_A.get_rank())
            {
               // the r'th shape item is either 0 (if anything is dropped)
               // or 1 if not.
               if (ravel_A.get_shape_item(r))   shape_Z.add_shape_item(0);
               else                             shape_Z.add_shape_item(1);
            }

        Value_P Z(shape_Z, LOC);

        Z->set_ravel_Cell(0, B->get_cfirst());
        if (shape_Z.get_volume() == 0)   Z->to_proto();
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (ravel_A.get_rank() != B->get_rank())   LENGTH_ERROR;

Shape sh_Z;
   loop(r, ravel_A.get_rank())
       {
         const ShapeItem sA = ravel_A.get_shape_item(r);
         const ShapeItem sB = B->get_shape_item(r);
         const ShapeItem pA = sA < 0 ? -sA : sA;
         if (pA >= sB)   sh_Z.add_shape_item(0);   // over-drop
         else            sh_Z.add_shape_item(sB - pA); 
       }

Value_P Z(sh_Z, LOC);
   if (sh_Z.is_empty())   // empty Z, e.g. from overdrop
      {
        Value_P Z(sh_Z, LOC);
        Z->set_default(*B.get(), LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   for (TakeDropIterator i(false, ravel_A, B->get_shape()); i.more(); ++i)
      {
        const ShapeItem offset = i();
        Z->next_ravel_Cell(B->get_cravel(offset));
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//----------------------------------------------------------------------------
Token
Bif_F12_DROP::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
   if (X->element_count() == 0)   // no axes
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   if (X->get_rank() > 1)    INDEX_ERROR;

const uint64_t len_X = X->element_count();
   if (len_X > MAX_RANK)     INDEX_ERROR;
   if (len_X == 0)           INDEX_ERROR;

   if (A->get_rank() > 1)    RANK_ERROR;

uint64_t len_A = A->element_count();
   if (len_A != len_X)   LENGTH_ERROR;

const APL_Integer qio = Workspace::get_IO();

   // init ravel_A = shape_B and seen.
   //
Shape ravel_A(B->get_shape());
bool seen[MAX_RANK];
   loop(r, B->get_rank())   seen[r] = false;

   loop(r, len_X)
       {
         const APL_Integer a = A->get_cravel(r).get_near_int();
         const APL_Integer x = X->get_cravel(r).get_near_int() - qio;

         if (x >= B->get_rank())   INDEX_ERROR;
         if (seen[x])              INDEX_ERROR;
         seen[x] = true;

         const ShapeItem amax = B->get_shape_item(x);
         if      (a >= amax)   ravel_A.set_shape_item(x, 0);
         else if (a >= 0)      ravel_A.set_shape_item(x, a - amax);
         else if (a > -amax)   ravel_A.set_shape_item(x, amax + a);
         else                  ravel_A.set_shape_item(x, 0);
       }

   return Token(TOK_APL_VALUE1,
                Bif_F12_TAKE::do_take(ravel_A, B.getref(), false));
}
//============================================================================
ShapeItem
TakeDropIterator::axis_proto() const
{
   // compute the offset for the prototype of current_offset. The APL2
   // language reference is a little vague as to how to to it. We assume
   // that the axis prototype is current_offset but with all axis offsets
   // outsode B set to 0.
   //
ShapeItem ret = 0;
   loop(r, ref_B.get_rank())
      {
        const _ftwc & ftwc_r = ftwc[r];
        const ShapeItem current_r = ftwc_r.current;
        if (current_r < 0)                                     continue;
        if (current_r >= ref_B.get_transposed_shape_item(r))   continue;
        ret += current_r * ftwc_r.weight;
      }

   return ret;
}
//============================================================================

