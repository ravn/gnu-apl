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

#include "ArrayIterator.hh"
#include "Bif_F12_PARTITION_PICK.hh"
#include "Bif_F12_TAKE_DROP.hh"
#include "Bif_OPER1_EACH.hh"
#include "Workspace.hh"

Bif_F12_PARTITION Bif_F12_PARTITION::_fun;    // ⊂
Bif_F12_PICK      Bif_F12_PICK     ::_fun;    // ⊃

Bif_F12_PARTITION * Bif_F12_PARTITION::fun = &Bif_F12_PARTITION::_fun;
Bif_F12_PICK      * Bif_F12_PICK     ::fun = &Bif_F12_PICK     ::_fun;

//=============================================================================
Token
Bif_F12_PARTITION::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return partition(A, B, axis);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_PARTITION::do_eval_B(Value_P B)
{
   if (B->is_simple_scalar())   return B;

Value_P Z(LOC);
   new (Z->next_ravel()) PointerCell(B->clone(LOC).get(), Z.getref());
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_PARTITION::do_eval_XB(Value_P X, Value_P B)
{
const Shape shape_X = Value::to_shape(X.get());

   return enclose_with_axes(shape_X, B);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_PARTITION::enclose_with_axes(const Shape & shape_X, Value_P B)
{
Shape item_shape;
Shape it_weight;
Shape shape_Z;
Shape weight_Z;

const Shape weight_B = B->get_shape().reverse_scan();

   // put the dimensions mentioned in X into item_shape and the
   // others into shape_Z
   //

   loop(r, B->get_rank())        // the axes not in shape_X
       {
         if (!shape_X.contains_axis(r))
            {
              shape_Z.add_shape_item(B->get_shape_item(r));
              weight_Z.add_shape_item(weight_B.get_shape_item(r));
            }
       }

int X_axes_used = 0;
   loop(r, shape_X.get_rank())   // the axes in shape_X
       {
         const ShapeItem x_r = shape_X.get_shape_item(r);

         // check that X∈⍳⍴⍴B
         //
         if (x_r < 0)                  AXIS_ERROR;
         if (x_r >= B->get_rank())     AXIS_ERROR;
         if (X_axes_used & 1 << x_r)
            {
              MORE_ERROR() = "Duplicate axis";
              AXIS_ERROR;
            }
         X_axes_used |= 1 << x_r;

         item_shape.add_shape_item(B->get_shape_item(x_r));
         it_weight.add_shape_item(weight_B.get_shape_item(x_r));
       }

   if (item_shape.get_rank() == 0)   // empty axes
      {
        //  ⊂[⍳0]B   ←→   ⊂¨B
        Token part(TOK_FUN1, Bif_F12_PARTITION::fun);
        return Bif_OPER1_EACH::fun->eval_LB(part, B).get_apl_val();
      }

Value_P Z(shape_Z, LOC);
   if (Z->is_empty())
      {
         Z->set_default(*B.get(), LOC);
         Z->check_value(LOC);
         return Z;
      }

   for (ArrayIterator it_Z(shape_Z); it_Z.more(); ++it_Z)
      {
        const ShapeItem off_Z = it_Z.multiply(weight_Z);   // offset in Z

        Value_P vZ(item_shape, LOC);
        new (Z->next_ravel()) PointerCell(vZ.get(), Z.getref());

        if (item_shape.is_empty())
           {
             vZ->get_ravel(0).init(B->get_ravel(0), vZ.getref(), LOC);
           }
        else
           {
             for (ArrayIterator it_it(item_shape); it_it.more(); ++it_it)
                 {
                   const ShapeItem off_B =  // offset in B
                         it_it.multiply(it_weight);
                   vZ->next_ravel()->init(B->get_ravel(off_Z + off_B),
                                                       vZ.getref(), LOC);
                 }
           }
        vZ->check_value(LOC);
      }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::partition(Value_P A, Value_P B, Axis axis) const
{
   if (A->get_rank() > 1)    RANK_ERROR;
   if (B->get_rank() == 0)   RANK_ERROR;

   if (A->is_scalar())
      {
        APL_Integer val = A->get_ravel(0).get_near_int();
        if (val == 0)
           {
             return Token(TOK_APL_VALUE1, Idx0(LOC));
           }

        return Token(TOK_APL_VALUE1, do_eval_B(B));
      }

   if (A->get_shape_item(0) != B->get_shape_item(axis))   LENGTH_ERROR;

   // determine the length of the partitioned dimension...
   //
ShapeItem len_Z = 0;
   {
     ShapeItem prev = 0;
     loop(l, A->get_shape_item(0))
        {
           const APL_Integer am = A->get_ravel(l).get_near_int();
           if (am < 0)   DOMAIN_ERROR;
           if (am  > prev)   ++len_Z;
           prev = am;
        }
   }

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, len_Z);

Value_P Z(shape_Z, LOC);


   if (Z->is_empty())
      {
        Z->set_default(*B.get(), LOC);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_B3(B->get_shape(), axis);
Cell * cZ;

   loop(h, shape_B3.h())
   loop(l, shape_B3.l())
       {
         ShapeItem from = -1;
         ShapeItem prev_am = 0;
         ShapeItem zm = 0;
         cZ = &Z->get_ravel(h*len_Z*shape_B3.l() + l);

         loop(m, shape_B3.m())
             {
               const APL_Integer am = A->get_ravel(m).get_near_int();
               Assert(am >= 0);   // already verified above.

               if (am == 0)   // skip element m (and following)
                  {
                    if (from != -1)   // an old segment is pending
                       {
                         copy_segment(cZ + zm, Z.getref(), h,
                                      from, m, shape_B3.m(),
                                      l, shape_B3.l(), B);
                         zm += shape_B3.l();
                       }
                    from = -1;
                  }
               else if (am > prev_am)   // new segment
                  {
                    if (from != -1)   // an old segment is pending
                       {
                         copy_segment(cZ + zm, Z.getref(), h, from, m,
                                      shape_B3.m(), l, shape_B3.l(), B);
                         zm += shape_B3.l();
                       }
                    from = m;
                  }
               prev_am = am;
             }

         if (from != -1)   // an old segment is pending
            {
              copy_segment(cZ + zm, Z.getref(), h, from, shape_B3.m(),
                           shape_B3.m(), l, shape_B3.l(), B);

              // zm += shape_B3.l();   not needed since we are done.
            }
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_PARTITION::copy_segment(Cell * dest, Value & dest_owner,
                                ShapeItem h, ShapeItem m_from,
                                ShapeItem m_to, ShapeItem m_len,
                                ShapeItem l, ShapeItem l_len, Value_P B)
{
   Assert(m_from < m_to);
   Assert(m_to <= m_len);
   Assert(l < l_len);

Value_P V(m_to - m_from, LOC);

Cell * vv = &V->get_ravel(0);
   for (ShapeItem m = m_from; m < m_to; ++m)
       {
         const Cell & cb = B->get_ravel(l + (m + h*m_len)*l_len);
         vv++->init(cb, V.getref(), LOC);
       }

   V->check_value(LOC);
   new (dest) PointerCell(V.get(), dest_owner);
}
//=============================================================================
Token
Bif_F12_PICK::eval_AB(Value_P A, Value_P B) const
{
   if (A->get_rank() > 1)    RANK_ERROR;

const ShapeItem ec_A = A->element_count();

   // if A is empty, return B
   //
   if (ec_A == 0)   return Token(TOK_APL_VALUE1, B);

const APL_Integer qio = Workspace::get_IO();

Value_P Z = pick(&A->get_ravel(0), 0, ec_A, B, qio);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::disclose(Value_P B, bool rank_tolerant)
{
   if (B->is_simple_scalar())   return Token(TOK_APL_VALUE1, B);

const Shape item_shape = compute_item_shape(B, rank_tolerant);
const Shape shape_Z = B->get_shape() + item_shape;

const ShapeItem len_B = B->element_count();
   if (len_B == 0)
      {
         Value_P first = Bif_F12_TAKE::first(B);
         Token result = disclose(first, rank_tolerant);
         if (result.get_Class() == TC_VALUE)   // success
            result.get_apl_val()->set_shape(shape_Z);
         return result;
      }

Value_P Z(shape_Z, LOC);

const ShapeItem llen = item_shape.get_volume();

   if (llen == 0)   // empty enclosed value
      {
        const Cell & B0 = B->get_ravel(0);
        if (B0.is_pointer_cell())
           {
             Value_P vB = B0.get_pointer_value();
             Value_P B_proto = vB->prototype(LOC);
             Z->get_ravel(0).init(B_proto->get_ravel(0), Z.getref(), LOC);
           }
        else
           {
             Z->get_ravel(0).init(B0, Z.getref(), LOC);
           }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   loop(h, len_B)
       {
         const Cell & B_item = B->get_ravel(h);
         Cell * Z_from = &Z->get_ravel(h*llen);
         if (B_item.is_pointer_cell())
            {
              Value_P vB = B_item.get_pointer_value();
              Bif_F12_TAKE::fill(item_shape, Z_from, Z.getref(), vB);
            }
         else if (B_item.is_lval_cell())
            {
              const Cell * pointee = B_item.get_lval_value();
               if (pointee && pointee->is_pointer_cell())   // pointer to nested
                  {
                    Value_P vB = pointee->get_pointer_value();
                    Value_P ref_B = vB->get_cellrefs(LOC);
                    Bif_F12_TAKE::fill(item_shape, Z_from, Z.getref(), ref_B);
                  }
               else                             // pointer to simple scalar
                  {
                    Z_from->init(B_item, Z.getref(), LOC);
                    for (ShapeItem c = 1; c < llen; ++c)
                        new (Z_from + c) LvalCell(0, 0);
                  }
            }
         else if (B_item.is_character_cell())   // simple char scalar
            {
              Z_from->init(B_item, Z.getref(), LOC);
              for (ShapeItem c = 1; c < llen; ++c)
                  new (Z_from + c)   CharCell(UNI_ASCII_SPACE);
            }
         else                                   // simple numeric scalar
            {
              Z->get_ravel(h*llen).init(B_item, Z.getref(), LOC);
              for (ShapeItem c = 1; c < llen; ++c)
                  new (Z_from + c) IntCell(0);
            }
       }

   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::disclose_with_axis(const Shape & axes_X, Value_P B,
                                 bool rank_tolerant)
{
   // disclose with axis

   // all items of B must have the same rank. item_shape is the smallest
   // shape that can contain each item of B.
   //
const Shape item_shape = compute_item_shape(B, rank_tolerant);

   // the number of items in X must be the number of axes in item_shape
   if (item_shape.get_rank() != axes_X.get_rank())   AXIS_ERROR;

   // distribute shape_B and item_shape into 2 shapes perm and shape_Z.
   // if a dimension is mentioned in axes_X then it goes into shape_Z and
   // otherwise into perm.
   //
Shape perm;
Shape shape_Z;
   {
     ShapeItem B_idx = 0;
     //
     loop(z, B->get_rank() + item_shape.get_rank())
        {
          // check if z is in X, remembering its position if so.
          //
          bool z_in_X = false;
          ShapeItem x_pos = -1;
          loop(x, axes_X.get_rank())
              {
                if (axes_X.get_shape_item(x) == z)
                   {
                     z_in_X = true;
                     x_pos = x;
                     break;
                   }
              }

          if (z_in_X)   // z is an item dimension: put it in shape_Z
             {
               shape_Z.add_shape_item(item_shape.get_shape_item(x_pos));
             }
         else          // z is a B dimension: put it in perm
             {
               if (B_idx >= B->get_rank())   INDEX_ERROR;
               perm.add_shape_item(z);
               shape_Z.add_shape_item(B->get_shape_item(B_idx++));
             }
        }

     // append X to perm with each X item reduced by the B items before it.
     loop(x, axes_X.get_rank())
        {
          Rank before_x = 0;   // items before X that are not in X
          loop(x1, x - 1)
             if (!axes_X.contains_axis(x1))   ++before_x;

          perm.add_shape_item(axes_X.get_shape_item(x) + before_x);
        }
   }

Value_P Z(shape_Z, LOC);
   if (Z->is_empty())
      {
         Z->set_default(*B.get(), LOC);
         return Token(TOK_APL_VALUE1, Z);
      }

   // loop over sources and place them in the result.
   //
PermutedArrayIterator it_Z(shape_Z, perm);

   for (ArrayIterator it_B(B->get_shape()); it_B.more(); ++it_B)
      {
        const Cell & B_item = B->get_ravel(it_B());
        if (B_item.is_pointer_cell())
           {
             Value_P vB = B_item.get_pointer_value();
             ArrayIterator vB_it(vB->get_shape());
             for (ArrayIterator it_it(item_shape); it_it.more(); ++it_it)
                 {
                   Cell * dest = &Z->get_ravel(it_Z());
                   if (vB->get_shape().contains(it_it.get_offsets()))
                      {
                        dest->init(vB->get_ravel(vB_it()), Z.getref(), LOC);
                        ++vB_it;
                      }
                   else if (vB->get_ravel(0).is_character_cell())  // char
                      new (dest) CharCell(UNI_ASCII_SPACE);
                   else                                   // simple numeric
                      new (dest) IntCell(0);

                   ++it_Z;
                 }
           }
        else
           {
             for (ArrayIterator it_it(item_shape); it_it.more(); ++it_it)
                 {
                   Cell * dest = &Z->get_ravel(it_Z());
                   if (it_it() == 0)   // first element: use B_item
                      dest->init(B_item, Z.getref(), LOC);
                   else if (B_item.is_character_cell())   // simple char scalar
                      new (dest) CharCell(UNI_ASCII_SPACE);
                   else                                // simple numeric scalar
                      new (dest) IntCell(0);
                   ++it_Z;
                 }
            }
      }

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_PICK::compute_item_shape(Value_P B, bool rank_tolerant)
{
   // all items of B are scalars or (nested) arrays of the same rank R.
   // return the shape with rank R and the (per-dimension) max. of
   // each shape item
   //
const ShapeItem len_B = B->nz_element_count();

Shape ret;   // of the first non-scalar in B

   loop(b, len_B)
       {
         Value_P v;
         {
           const Cell & cB = B->get_ravel(b);
           if (cB.is_pointer_cell())
              {
                v = cB.get_pointer_value();
              }
           else if (cB.is_lval_cell())
              {
                const Cell * pointee = cB.get_lval_value();
                if (!pointee)   continue;   // ⊃ NOOP element
                if (!pointee->is_pointer_cell())   continue;  // ptr to scalar
                v = pointee->get_pointer_value();
              }
           else
              {
                continue;   // simple scalar
              }
         }

         if (ret.get_rank() == 0)   // first non-scalar
            {
              ret = v->get_shape();
              continue;
            }

         // the items of B must have the same rank, unless we are rank_tolerant
         //
         if (ret.get_rank() != v->get_rank())
            {
              if (!rank_tolerant)   RANK_ERROR;

              if (ret.get_rank() < v->get_rank())
                 ret.expand_rank(v->get_rank());

              // if ret.get_rank() > v->get_rank() then we are OK because
              // only the dimensions present in v are expanded below.
            }

         loop(r, v->get_rank())
             {
               if (ret.get_shape_item(r) < v->get_shape_item(r))
                  {
                    ret.set_shape_item(r, v->get_shape_item(r));
                  }
             }
       }

   return ret;
}
//-----------------------------------------------------------------------------
ShapeItem
Bif_F12_PICK::pick_offset(const Cell * const A0, ShapeItem idx_A,
                          ShapeItem len_A, Value_P B, APL_Integer qio)
{
const Cell & cA = A0[idx_A];

   if (cA.is_pointer_cell())   // then B shall be a 1-dimensional array
      {
        /*
           cA = A[idx_A] is nested, e.g.

           case i.    (⊂"b") ⊃ A.b.c ← 'leaf-A.b.c'   (structured variable A)

           case ii.   (⊂1 1) ⊃ A←3 3⍴⍳9               (normal A)
         */

        const Value * A = cA.get_pointer_value().get();

        if (B->is_member())   // case i. (structured B)
           {
             if (!A->is_char_vector())
                {
                  MORE_ERROR() << "member name expected for A⊂B (nested A["
                               << (idx_A + qio) << "])";
                  DOMAIN_ERROR;
                }

             const UCS_string top_level("B");
             const UCS_string member(*A);
             vector<const UCS_string *> members;
             members.push_back(&member);
             members.push_back(&top_level);   // dummy, must be last
             Value * val_B = B.get();
             Cell * Bsub = B->get_member(members, val_B, false, true);
             return Bsub - &B->get_ravel(0);
           }
        else                  // case ii. (normal B)
           {
             if (A->get_rank() > 1)
                {
                  MORE_ERROR() << "rank A ≤ 1 expected for A⊂B (nested A["
                               << (idx_A + qio) << "])";
                  RANK_ERROR;
                }

             const ShapeItem len_A = A->element_count();
             if (B->get_rank() != len_A)
                {
                  MORE_ERROR() << "⍴⍴B (" << B->get_rank()
                               << ") = ⍴,A (" << len_A
                               << ") expected for A⊂B (nested A["
                               << (idx_A + qio) << "])";
                  RANK_ERROR;
                }

             const Shape weight = B->get_shape().reverse_scan();
             const Shape A_as_shape(A, qio);
             ShapeItem offset = 0;

             loop(r, A->element_count())
                 {
                   const ShapeItem ar = A_as_shape.get_shape_item(r);
                   if (ar < 0)                       INDEX_ERROR;
                   if (ar >= B->get_shape_item(r))   INDEX_ERROR;
                   offset += weight.get_shape_item(r) * ar;
                 }
             return offset;
           }
      }
   else   // A is a scalar, so B must be a vector.
      {
        if (B->get_rank() != 1)
           {
             MORE_ERROR() << "⍴⍴B (" << B->get_rank()
                          << ") = 1 expected for A⊂B (with scalar A)";
             RANK_ERROR;
           }
        const APL_Integer a = cA.get_near_int() - qio;
        if (a < 0)                       INDEX_ERROR;
        if (a >= B->get_shape_item(0))   INDEX_ERROR;
        return a;
      }
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_PICK::pick(const Cell * const A0, ShapeItem idx_A, ShapeItem len_A,
                   Value_P B, APL_Integer qio)
{
   // A0 points to ↑A and we are at depth idx_A, which means that A0[idx_A] is
   // the current index of B.
   //
const ShapeItem offset = pick_offset(A0, idx_A, len_A, B, qio);
const Cell * cB = &B->get_ravel(offset);

   if (len_A > 1)   // more levels coming.
      {
        if (cB->is_pointer_cell())
           {
             return pick(A0, idx_A + 1, len_A - 1, cB->get_pointer_value(), qio);
           }

        if (cB->is_lval_cell())
           {
             // Note: this is a little tricky...

             // first of all, we need a pointer cell. Therefore the target of cB
             // should be a PointerCell.
             //
             Cell & target = *cB->get_lval_value();
             if (!target.is_pointer_cell())   DOMAIN_ERROR;

             // secondly, get_cellrefs() is not recursive, therefore target has not
             // (yet) been converted to a left-value. We do that now.
             //
             Value_P subval = target.get_pointer_value();   // right-value
             Value_P subrefs = subval->get_cellrefs(LOC);   // left-value
             return pick(A0, idx_A + 1, len_A - 1, subrefs, qio);
           }

        // simple cell. This means that the depth of B does not suffice to pick
        // the item selected by A or, in other words, A is too long).
        //
        RANK_ERROR;   // ISO p.166 wants rank-error. LENGTH ERROR would be correct.
      }

   // len_A == 1, i.e. the end of the recursion over A has been reached,
   // cB is the cell in B that was pick'ed by A->
   //
   if (cB->is_pointer_cell())
      {
        Value_P Z = cB->get_pointer_value()->clone(LOC);
        return Z;
      }

   if (cB->is_lval_cell())   // e.g. (A⊃B) ← C
      {
        Cell * cell = cB->get_lval_value();
        Assert(cell);

        Value_P Z(LOC);
        Value * cell_owner = B->get_lval_cellowner();
        new (Z->next_ravel())   LvalCell(cell, cell_owner);
        return Z;
      }
   else   // simple cell
      {
        Value_P Z(LOC);
        Z->next_ravel()->init(*cB, Z.getref(), LOC);
        return Z;
      }
}
//=============================================================================




