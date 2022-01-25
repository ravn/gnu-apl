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
   Z->next_ravel_Pointer(B->clone(LOC).get());
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
        Z->next_ravel_Pointer(vZ.get());

        if (item_shape.is_empty())
           {
             vZ->get_wproto().init(B->get_cfirst(), vZ.getref(), LOC);
           }
        else
           {
             for (ArrayIterator it_it(item_shape); it_it.more(); ++it_it)
                 {
                   const ShapeItem off_B =  // offset in B
                         it_it.multiply(it_weight);
                   vZ->next_ravel_Cell(B->get_cravel(off_Z + off_B));
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
   // A must be a scalar or vector (of non-negative integers)
   //
   if (A->get_rank() > 1)    RANK_ERROR;
   if (B->get_rank() == 0)   RANK_ERROR;

   // construct a vector of breakpoints from A
   //
const ShapeItem len_A = A->get_shape_item(0);
   if (len_A != B->get_shape_item(axis))   LENGTH_ERROR;

vector<ShapeItem> breakpoints;   // start position of a partition (including)
vector<ShapeItem> breakends;     // end position of a partition (excluding)

   // A is scalar-extended as needed. The case where A is a scalar is, however
   // identical to the case where all items in A are equal, and in both cases
   // there is only one breakpoint (at B[0])
   //
ShapeItem len_Zm = 0;

   if (A->is_scalar())
      {
        breakpoints.push_back(0);
        breakends.push_back(len_A);
      }
   else
      {
        ShapeItem prev_A = 0;
        bool in_partition = false;
        loop(apos, len_A)
            {
              const APL_Integer aval = A->get_cravel(apos).get_near_int();
              if (aval < 0)             DOMAIN_ERROR;

              if (aval > prev_A)   // new partition starting at apos
                 {
                   breakpoints.push_back(apos);
                   if (in_partition)   breakends.push_back(apos);
                   in_partition = true;
                   ++len_Zm;
                 }
              else if (in_partition && aval == 0)
                 {
                   breakends.push_back(apos);
                   in_partition = false;
                 }
              prev_A = aval;
            }

        if (in_partition)   breakends.push_back(len_A);

        Assert(breakpoints.size() == breakends.size());
      }

   if (len_Zm == 1)   // single partition
      {
        if (A->get_cfirst().get_near_int())   // non-zero A0
           {
             Value_P ZZ = B->clone(LOC);
             Value_P Z(LOC);
             Z->next_ravel_Pointer(ZZ.get());
             return Token(TOK_APL_VALUE1, Z);
           }

        // 0 ⊂ B is empty
        return Token(TOK_APL_VALUE1, Idx0(LOC));
      }

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, len_Zm);

Value_P Z(shape_Z, LOC);

   if (Z->is_empty())
      {
        Z->set_default(*B.get(), LOC);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_B3(B->get_shape(), axis);

   loop(h, shape_B3.h())
   loop(m, len_Zm)
   loop(l, shape_B3.l())
       {
         const Cell * src_B = &B->get_cravel(l + shape_B3.l() *
                                    (breakpoints[m] + h * shape_B3.m()));

         const ShapeItem partition_len = breakends[m] - breakpoints[m];

         Value_P ZZ(partition_len, LOC);
         loop(p, partition_len)
             {
               ZZ->next_ravel_Cell(*src_B);
               src_B += shape_B3.l();
             }
         ZZ->check_value(LOC);

         Z->next_ravel_Pointer(ZZ.get());
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
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

Value_P Z = pick(&A->get_cfirst(), 0, ec_A, B, qio);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::disclose(Value_P B, bool rank_tolerant)
{
   // for simple scalars B: B ≡ ⊂ B and therefore B ≡ ⊃ B
   //
   if (B->is_simple_scalar())   return Token(TOK_APL_VALUE1, B);

   // compute item_shape, which is the smallest shape into which each
   // item of B fits, and then ⍴Z ←→ (⍴B), item_shape

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

const ShapeItem item_len = item_shape.get_volume();

   if (item_len == 0)   // empty enclosed value
      {
        const Cell & B0 = B->get_cproto();
        if (B0.is_pointer_cell())
           {
             Value_P vB = B0.get_pointer_value();
             Value_P B_proto = vB->prototype(LOC);
             Z->get_wproto().init(B_proto->get_cproto(), Z.getref(), LOC);
           }
        else
           {
             Z->get_wproto().init(B0, Z.getref(), LOC);
           }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   loop(b, len_B)   // for all items in B...
       {
         const Cell & B_item = B->get_cravel(b);
         disclose_item(Z.getref(), b, item_shape, item_len, B_item);
       }

   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_PICK::disclose_item(Value & Z, ShapeItem b,
                            const Shape & item_shape, ShapeItem item_len,
                             const Cell & B_item)
{
   if (B_item.is_pointer_cell())
      {
         // B_item points to a nested APL value. Take that value, expand
         // it to item_shape, and store it in Z.
         //
        const Value & vB = B_item.get_pointer_value().getref();
        Bif_F12_TAKE::fill(item_shape, Z, vB);
      }
   else if (B_item.is_lval_cell())
      {
        /* B_item is a left-value Cell (pointing to a Cell (target) that
           shall be assigned at a later point in time. This is a litte
           tricky since there are 3 cases:

           1. target == 0, which means that target was padded at an earlier
              point in time (and then nothing shall be done), or

           2. target is a right-value (even though it is left of →. The
              convention for cellrefs has it, that left hand PointerCells are
              not expanded when their parents are (because they are subject to
              be deleted by other primitives, and then the early conversion to
              left-values would not pay of). We expand the target now, expand
              it to item_shape, and store it in Z.

           3. target is a simple Cell, so B_item points a simple Cell. We
              expand B_item to item_shape using LvalCells that point nowhere.

           In all 3 cases, since we are left of ←, the expansion needs to be
           made with LvalCell(0, 0) pointing nowhere rather than with UNI_SPACE
           or with 0.
        */
        Cell * target = B_item.get_lval_value();
        if (target == 0)                           // case 1.
           {
             loop(c, item_len)   Z.next_ravel_Lval(0, 0);
           }
        else if (target->is_pointer_cell())        // case 2.
           {
             Value_P vB = target->get_pointer_value();
             const Value & cellref_B = vB->get_cellrefs(LOC).getref();
             Bif_F12_TAKE::fill(item_shape, Z, cellref_B);
           }
        else                                       // case 3.
           {
             Z.next_ravel_Cell(B_item);
             for (ShapeItem c = 1; c < item_len; ++c)
             Z.next_ravel_Lval(0, 0);
           }
      }
   else if (B_item.is_character_cell())   // simple char scalar
      {
        // B_item is a character, so expand it to item_shape with UNI_SPACE,
        //  and store it in Z
        //
        Z.next_ravel_Cell(B_item);
        for (ShapeItem c = 1; c < item_len; ++c)   // the remaining items.
            Z.next_ravel_Char(UNI_SPACE);
      }
   else                                   // simple numeric scalar
      {
        // B_item is a number, so expand it to item_shape with 0, and store
        //  it in Z
        //
        Z.next_ravel_Cell(B_item);
        for (ShapeItem c = 1; c < item_len; ++c)
            Z.next_ravel_Int(0);
      }
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
        const Cell & B_item = B->get_cravel(it_B());
        if (B_item.is_pointer_cell())
           {
             const Value & vB = B_item.get_pointer_value().getref();
             ArrayIterator vB_it(vB.get_shape());
             for (ArrayIterator item_it(item_shape); item_it.more(); ++item_it)
                 {
                   Cell * dest = &Z->get_wravel(it_Z());
                   if (vB.get_shape().contains(item_it.get_offsets()))
                      {
                        dest->init(vB.get_cravel(vB_it()), Z.getref(), LOC);
                        ++vB_it;
                      }
                   else if (vB.get_cproto().is_character_cell())  // char
                      Value::zU(dest, UNI_SPACE);
                   else                                   // simple numeric
                      Value::z0(dest);

                   ++it_Z;
                 }
           }
        else
           {
             for (ArrayIterator it_it(item_shape); it_it.more(); ++it_it)
                 {
                   if (const ShapeItem pos = it_it())   // subsequent cell
                      {
                        if (B_item.is_character_cell())   // fill with ' '
                           Z->set_ravel_Char(pos, UNI_SPACE);
                        else                              // fll with 0
                           Z->set_ravel_Int(pos, 0);
                      }
                   else                    // first Cell: use B_item
                      {
                        Z->set_ravel_Cell(0, B_item);
                      }
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
           const Cell & cB = B->get_cravel(b);
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
             if (!A->is_char_string())
                {
                  MORE_ERROR() << "member name expected for A⊃B (nested A["
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
             return Bsub - &B->get_cfirst();
           }
        else                  // case ii. (normal B)
           {
             if (A->get_rank() > 1)
                {
                  MORE_ERROR() << "rank A ≤ 1 expected for A⊃B (nested A["
                               << (idx_A + qio) << "])";
                  RANK_ERROR;
                }

             const ShapeItem len_A = A->element_count();
             if (B->get_rank() != len_A)
                {
                  MORE_ERROR() << "⍴⍴B (" << B->get_rank()
                               << ") = ⍴,A (" << len_A
                               << ") expected for A⊃B (nested A["
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
                          << ") = 1 expected for A⊃B (with scalar A)";
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
const Cell * cB = &B->get_cravel(offset);

   if (len_A > 1)   // more levels coming.
      {
        if (cB->is_pointer_cell())
           {
             return pick(A0, idx_A+1, len_A-1, cB->get_pointer_value(), qio);
           }

        if (cB->is_lval_cell())
           {
             // Note: this is a little tricky...

             // first of all, we need a pointer cell. Therefore the target of cB
             // should be a PointerCell.
             //
             Cell & target = *cB->get_lval_value();
             if (!target.is_pointer_cell())   DOMAIN_ERROR;

             // secondly, get_cellrefs() is not recursive, therefore target has
             // not (yet) been converted to a left-value. We do that now.
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
        Cell * target = cB->get_lval_value();
        Assert(target);

        if (target->is_pointer_cell())
           {
             // cB was created by get_cellrefs(), which is flat (non-recursive).
             // That means that PointerCell points to a right-hand value that
             // needs to be converted to a left-hand value here.
             //
             Value_P sub = target->get_pointer_value();
             Value_P Z = sub->get_cellrefs(LOC);
             return Z;
           }

        Value_P Z(LOC);
        Value * cell_owner = B->get_lval_cellowner();
        Z->next_ravel_Lval(target, cell_owner);
        return Z;
      }
   else   // simple cell
      {
        Value_P Z(LOC);
        Z->next_ravel_Cell(*cB);
        return Z;
      }
}
//=============================================================================




