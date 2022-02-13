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

//============================================================================
Token
Bif_F12_PARTITION::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return partition(A, B, axis);
}
//----------------------------------------------------------------------------
Value_P
Bif_F12_PARTITION::do_eval_B(Value_P B)
{
   if (B->is_simple_scalar())   return B;

Value_P Z(LOC);
   Z->next_ravel_Pointer(B->clone(LOC).get());
   Z->check_value(LOC);
   return Z;
}
//----------------------------------------------------------------------------
Value_P
Bif_F12_PARTITION::enclose_with_axes(const Shape & axes_X, Value_P B)
{
Shape item_shape;
Shape it_weights;
Shape shape_Z;
Shape weights_Z;

const Shape weights_B = B->get_shape().get_weights();

int bmX = 0;   // axes in axes_X with ⎕IO←0
   loop(r, axes_X.get_rank())   // the axes in axes_X
       {
         const ShapeItem ax = axes_X.get_shape_item(r);

         // check that Xϵ⍳⍴⍴B
         //
         if (ax < 0)
            {
              const APL_Integer qio = Workspace::get_IO();
              MORE_ERROR() << "In A of A⊂[X]B: axis " << (ax + qio)
                           << " < ⎕IO";
              AXIS_ERROR;
            }

         if (ax >= B->get_rank())
            {
              const APL_Integer qio = Workspace::get_IO();
              MORE_ERROR() << "In A of A⊂[X]B: axis " << (ax + qio)
                           << " > ⎕IO+⍴B";
              AXIS_ERROR;
            }

         if (bmX & 1 << ax)
            {
              const APL_Integer qio = Workspace::get_IO();
              MORE_ERROR() << "In A of A⊂[X]B: duplicate axis " << (ax + qio);
              AXIS_ERROR;
            }

         // ax is OK.
         bmX |= 1 << ax;

         item_shape.add_shape_item(B->get_shape_item(ax));
         it_weights.add_shape_item(weights_B.get_shape_item(ax));
       }

   // put the dimensions mentioned in X into item_shape and the
   // others into shape_Z
   //
   loop(ax, B->get_rank())        // the axes not in axes_X
       {
         if (bmX & 1 << ax)   continue;   // ax is in X

         shape_Z.add_shape_item(B->get_shape_item(ax));
         weights_Z.add_shape_item(weights_B.get_shape_item(ax));
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
        const Shape it_Z_sh = it_Z.get_shape_offsets();
        ShapeItem off_Z = 0;
        loop(z, it_Z_sh.get_rank())
            off_Z += it_Z_sh.get_shape_item(z)
                   * weights_Z.get_shape_item(z);

        Value_P vZ(item_shape, LOC);
        Z->next_ravel_Pointer(vZ.get());

        if (item_shape.is_empty())
           {
             vZ->set_default(B.getref(), LOC);
           }
        else
           {
             for (ArrayIterator it_it(item_shape); it_it.more(); ++it_it)
                 {
                    const Shape it_sh = it_it.get_shape_offsets();
                    ShapeItem off_B = 0;
                    loop(i, item_shape.get_rank())
                        off_B += it_sh.get_shape_item(i)
                               * it_weights.get_shape_item(i);
                   vZ->next_ravel_Cell(B->get_cravel(off_Z + off_B));
                 }
           }
        vZ->check_value(LOC);
      }

   Z->check_value(LOC);
   return Z;
}
//----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::partition(Value_P A, Value_P B, Axis axis) const
{
   // A must be a scalar or vector (of non-negative integers)
   //
   if (A->get_rank() > 1)    RANK_ERROR;
   if (B->get_rank() == 0)   RANK_ERROR;

   if (A->element_count() == 1)
      {
        /* lrm p. 188:

           The length of the left argument and the size of the last axis of
           the right argument must match, unless the left argument is a
           scalar or one-item vector, in which case it is extended.

           We generalize "one-item vector" to mean "one-item array".
         */

         // a non-zero scalar A extends to A A A ... A, i.e. a single partition
         //
         if (const APL_Integer a = A->get_cscalar().get_near_int())
            {
              return Token(TOK_APL_VALUE1, do_eval_B(B));
            }

         // A is 0 which extends to 0 0 0 ... 0, i.e. an empty A ⊂ B.
         if (B->get_cfirst().is_numeric())
            {
              return Token(TOK_APL_VALUE1, Idx0(LOC));
            }
         if (B->get_cfirst().is_character_cell())
            {
              return Token(TOK_APL_VALUE1, Str0(LOC));
            }
         DOMAIN_ERROR;
      }

const ShapeItem len_A = A->get_shape_item(0);
   // construct a vector of breakpoints from A
   //
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
             Value_P Z(LOC);
             Z->next_ravel_Pointer(B->clone(LOC).get());
             Z->check_value(LOC);
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
//============================================================================
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
//----------------------------------------------------------------------------
Value_P
Bif_F12_PICK::disclose(Value_P B, bool rank_tolerant)
{
   // for simple scalars B: B ≡ ⊂ B and therefore B ≡ ⊃ B
   //
   if (B->is_simple_scalar())   return B;

   // compute item_shape, which is the smallest shape into which each
   // item of B fits, and then ⍴Z ←→ (⍴B), item_shape

const Shape item_shape = compute_item_shape(B, rank_tolerant);
const Shape shape_Z = B->get_shape() + item_shape;

const ShapeItem len_B = B->element_count();
   if (len_B == 0)
      {
         Value_P first = Bif_F12_TAKE::first(B);
         Value_P result = disclose(first, rank_tolerant);
         result->set_shape(shape_Z);
         return result;
      }

Value_P Z(shape_Z, LOC);

const ShapeItem item_len = item_shape.get_volume();

   if (item_len == 0)   // empty enclosed value
      {
        const Cell & B0 = B->get_cproto();
        if (B0.is_pointer_cell())
           {
             Value_P vB = B0.get_pointer_value();    // some nested value
             Value_P B_proto = vB->prototype(LOC);   // its prototype
             Z->get_wproto().init(B_proto->get_cproto(), Z.getref(), LOC);
           }
        else
           {
             Z->set_default(B0, LOC);
           }

        Z->check_value(LOC);
        return Z;
      }

   loop(b, len_B)   // for all items in B...
       {
         const Cell & B_item = B->get_cravel(b);
         disclose_item(Z.getref(), b, item_shape, item_len, B_item);
       }

   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Z;
}
//----------------------------------------------------------------------------
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
            Z.next_ravel_0();
      }
}
//----------------------------------------------------------------------------
Value_P
Bif_F12_PICK::disclose_with_axis(const Shape & axes_X, Value_P B)
{
   // disclose with axis: Z←⊃[Z] B
   // implemented as: cB ← ⊃ B ◊ cX ← ((⍳⍴⍴cB)∼X),X ◊ Z←cX ⍉ B

   // Note: axes_X is already normalized to ⎕IO←0

Value_P cB = disclose(B, true);   // cB ← ⊃ B

ShapeItem bmX = 0;   // axes in axes_X with ⎕IO←0

   loop(x, axes_X.get_rank())
       {
          const ShapeItem ax = axes_X.get_shape_item(x);
          if (bmX & 1 << ax)
             {
               MORE_ERROR() << "In ⊃[X]B: duplicated axis "
                            << (ax + Workspace::get_IO())
                            << " in X";
                AXIS_ERROR;
             }
          bmX |= 1 << ax;
       }

   /* ⍴Z is the axes of B that are not in X, followed by those which are.
      That is, shape_Z is a permutation of ⍳⍴⍴B defined by X.

      We prepend the permutation axes_X (of each disclosed item) with the
      axes not in X to obtain the entire permutation perm_cB of cB.
  */
Shape perm_cB;   // perm_cB is the permutation of cB, constructed from X
   loop(x, cB->get_rank())      // axes not in X
       {
         if (bmX & 1 << x)   continue;   // axis x is in X
         perm_cB.add_shape_item(x);       //  axis x is not in X
       }

   loop(x, axes_X.get_rank())   // axes in X
       {
         perm_cB.add_shape_item(axes_X.get_shape_item(x));
       }

   Assert(perm_cB.get_rank() == cB->get_rank());

   return Bif_F12_TRANSPOSE::transpose(perm_cB, cB);
}
//----------------------------------------------------------------------------
Shape
Bif_F12_PICK::compute_item_shape(Value_P B, bool rank_tolerant)
{
   /* The ravel cells of B are either simple or else PointerCells of nested
      values with possibly different shapes.

      Return the smallest shape that is large enough to contain each value.

      If rank_tolerant is false, then all nested values have the same rank
      (which is also the rank of the result). Otherwise: RANK_ERROR.

      If rank_tolerant is true, then the smaller ranks are extended to the
      largest rank of all shapes by prepending 1s.
    */

   // we use ret_rank and ret[MAX_RANK] instead of Shape to avoid unnecessary
   // volume recomputations in class Shape.
   //
ShapeItem ret_rank = 0;
ShapeItem ret[MAX_RANK];
   loop(r, MAX_RANK)   ret[r] = 0;

   loop(b, B->nz_element_count())
       {
         const Value * val;
         {
           const Cell & cB = B->get_cravel(b);
           if (cB.is_pointer_cell())
              {
                val = cB.get_pointer_value().get();
              }
           else if (cB.is_lval_cell())
              {
                const Cell * target = cB.get_lval_value();

                /* pointee was created by a selective assignment and
                   there are 3 cases:

                   1. pointee == 0 (e.g from over-take): do nothing
                   2. pointee is simple.                 do nothing
                   3. pointee is sub-value:              use its value
                 */
                if (!target)   continue;                      // case 1.
                if (!target->is_pointer_cell())   continue;   // case 2.
                val = target->get_pointer_value().get();      // case 3.
              }
           else
              {
                continue;   // cB is simple, so it fits anywhere.
              }
         }

         // at this point val is a non-scalar value.
         //
         const Rank val_rank = val->get_rank();
         if (ret_rank == 0)   // val is the first non-scalar
            {
              ret_rank = val_rank;
              loop(r, ret_rank)   ret[r] = val->get_shape_item(r);
              continue;
            }

         // the items of B must have the same rank, unless we are rank_tolerant
         //
         if (ret_rank != val_rank)
            {
              if (!rank_tolerant)   RANK_ERROR;

              if (ret_rank < val_rank)   ret_rank = val_rank;

              // if ret_rank() > v->get_rank() then we are OK because
              // only the dimensions present in v are expanded below.
            }

         loop(r, val_rank)
             {
               if (ret[r] < val->get_shape_item(r))
                  {
                    ret[r] = val->get_shape_item(r);
                  }
             }
       }

   return Shape(ret_rank, ret);
}
//----------------------------------------------------------------------------
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

             const Shape weights_B = B->get_shape().get_weights();
             const Shape A_as_shape(A, qio);
             ShapeItem offset = 0;

             loop(r, A->element_count())
                 {
                   const ShapeItem ar = A_as_shape.get_shape_item(r);
                   if (ar < 0)                       INDEX_ERROR;
                   if (ar >= B->get_shape_item(r))   INDEX_ERROR;
                   offset += weights_B.get_shape_item(r) * ar;
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
//----------------------------------------------------------------------------
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
//============================================================================




