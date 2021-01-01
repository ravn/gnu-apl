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

#include <string.h>
#include <stdio.h>

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "Bif_F12_COMMA.hh"
#include "Bif_F12_SORT.hh"
#include "Bif_F12_TAKE_DROP.hh"
#include "Bif_OPER1_EACH.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Command.hh"
#include "FloatCell.hh"
#include "Id.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "Macro.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "StateIndicator.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

// primitive function instances
//
Bif_F0_ZILDE      Bif_F0_ZILDE     ::_fun;    // ⍬
Bif_F1_EXECUTE    Bif_F1_EXECUTE   ::_fun;    // ⍎
Bif_F2_INDEX      Bif_F2_INDEX     ::_fun;    // ⌷
Bif_F12_ELEMENT   Bif_F12_ELEMENT  ::_fun;    // ∈
Bif_F12_EQUIV     Bif_F12_EQUIV    ::_fun;    // ≡
Bif_F12_NEQUIV    Bif_F12_NEQUIV   ::_fun;    // ≢
Bif_F12_ENCODE    Bif_F12_ENCODE   ::_fun;    // ⊤
Bif_F12_DECODE    Bif_F12_DECODE   ::_fun;    // ⊥
Bif_F12_ROTATE    Bif_F12_ROTATE   ::_fun;    // ⌽
Bif_F12_ROTATE1   Bif_F12_ROTATE1  ::_fun;    // ⊖
Bif_F12_TRANSPOSE Bif_F12_TRANSPOSE::_fun;    // ⍉
Bif_F12_RHO       Bif_F12_RHO      ::_fun;    // ⍴
Bif_F2_INTER      Bif_F2_INTER     ::_fun;    // ∩
Bif_F12_UNION     Bif_F12_UNION    ::_fun;    // ∪
Bif_F2_LEFT       Bif_F2_LEFT      ::_fun;    // ⊣
Bif_F2_RIGHT      Bif_F2_RIGHT     ::_fun;    // ⊢

// primitive function pointers
//
Bif_F0_ZILDE      * Bif_F0_ZILDE     ::fun = &Bif_F0_ZILDE     ::_fun;
Bif_F1_EXECUTE    * Bif_F1_EXECUTE   ::fun = &Bif_F1_EXECUTE   ::_fun;
Bif_F2_INDEX      * Bif_F2_INDEX     ::fun = &Bif_F2_INDEX     ::_fun;
Bif_F12_ELEMENT   * Bif_F12_ELEMENT  ::fun = &Bif_F12_ELEMENT  ::_fun;
Bif_F12_EQUIV     * Bif_F12_EQUIV    ::fun = &Bif_F12_EQUIV    ::_fun;
Bif_F12_NEQUIV    * Bif_F12_NEQUIV   ::fun = &Bif_F12_NEQUIV   ::_fun;
Bif_F12_ENCODE    * Bif_F12_ENCODE   ::fun = &Bif_F12_ENCODE   ::_fun;
Bif_F12_DECODE    * Bif_F12_DECODE   ::fun = &Bif_F12_DECODE   ::_fun;
Bif_F12_ROTATE    * Bif_F12_ROTATE   ::fun = &Bif_F12_ROTATE   ::_fun;
Bif_F12_ROTATE1   * Bif_F12_ROTATE1  ::fun = &Bif_F12_ROTATE1  ::_fun;
Bif_F12_TRANSPOSE * Bif_F12_TRANSPOSE::fun = &Bif_F12_TRANSPOSE::_fun;
Bif_F12_RHO       * Bif_F12_RHO      ::fun = &Bif_F12_RHO      ::_fun;
Bif_F2_INTER      * Bif_F2_INTER     ::fun = &Bif_F2_INTER     ::_fun;
Bif_F12_UNION     * Bif_F12_UNION    ::fun = &Bif_F12_UNION    ::_fun;
Bif_F2_LEFT       * Bif_F2_LEFT      ::fun = &Bif_F2_LEFT      ::_fun;
Bif_F2_RIGHT      * Bif_F2_RIGHT     ::fun = &Bif_F2_RIGHT     ::_fun;

int Bif_F1_EXECUTE::copy_pending = 0;

//-----------------------------------------------------------------------------
Token
PrimitiveFunction::eval_fill_AB(Value_P A, Value_P B) const
{
   return eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
PrimitiveFunction::eval_fill_B(Value_P B) const
{
   return eval_B(B);
}
//-----------------------------------------------------------------------------
ostream &
PrimitiveFunction::print(ostream & out) const
{
   return out << get_Id();
}
//-----------------------------------------------------------------------------
void
PrimitiveFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "System Function ";
   print(out);
   out << endl;
}
//=============================================================================
Token
Bif_F0_ZILDE::eval_() const
{
   return Token(TOK_APL_VALUE1, Idx0(LOC));
}
//=============================================================================
Token
Bif_F12_RHO::eval_B(Value_P B) const
{
Value_P Z(B->get_rank(), LOC);

   loop(r, B->get_rank())   new (Z->next_ravel()) IntCell(B->get_shape_item(r));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_RHO::eval_AB(Value_P A, Value_P B) const
{
#ifdef PERFORMANCE_COUNTERS_WANTED
const uint64_t start_1 = cycle_counter();
#endif

const Shape shape_Z(A.get(), 0);

   // check that shape_Z is positive
   //
   loop(r, shape_Z.get_rank())
      {
        if (shape_Z.get_shape_item(r) < 0)   DOMAIN_ERROR;
      }

const ShapeItem len_Z = shape_Z.get_volume();

   if (len_Z <= B->element_count()   &&
       B->get_owner_count() == 2 && 
       this == Workspace::SI_top()->get_prefix().get_dyadic_fun())
      {
        // at this point Z is not greater than B and B has only 2 owners:
        // the prefix (who will discard it after we return) and our Value_P B
        //
        // Since we will give up our ownership) and prefix will
        //  pop_args_push_result() in reduce_A_F_B_(), B is no longer used
        // and we can reshape it in place instead of copying B into a new Z.
        //
        Log(LOG_optimization) CERR << "optimizing A⍴B" << endl;

        // release unused cells
        //
        const ShapeItem len_B = B->element_count();
        ShapeItem rest = len_Z;
        if (rest == 0)
           {
             rest = 1;
             if (B->get_ravel(0).is_pointer_cell())
                {
                  B->get_ravel(0).get_pointer_value()->to_proto();
                }
             else
                {
                   B->get_ravel(0).init_type(B->get_ravel(0), B.getref(), LOC);
                }
           }

        while (rest < len_B)   B->get_ravel(rest++).release(LOC);

        B->set_shape(shape_Z);

#ifdef PERFORMANCE_COUNTERS_WANTED
const uint64_t end_1 = cycle_counter();
   Performance::fs_F12_RHO_AB.add_sample(end_1 - start_1,
                                         B->nz_element_count());
#endif

        return Token(TOK_APL_VALUE1, B);
      }

Token ret = do_reshape(shape_Z, *B);

#ifdef PERFORMANCE_COUNTERS_WANTED
const uint64_t end_1 = cycle_counter();
   Performance::fs_F12_RHO_AB.add_sample(end_1 - start_1,
                                         shape_Z.get_volume());
#endif

   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_RHO::do_reshape(const Shape & shape_Z, const Value & B)
{
const ShapeItem len_B = B.element_count();
const ShapeItem len_Z = shape_Z.get_volume();

Value_P Z(shape_Z, LOC);

   if (len_B == 0)   // empty B: use prototype
      {
        loop(z, len_Z)
            Z->next_ravel()->init_type(B.get_ravel(0), Z.getref(), LOC);
      }
   else
      {
        loop(z, len_Z)
            B.get_ravel(z % len_B).init_other(Z->next_ravel(), Z.getref(), LOC);
      }

   Z->set_default(B, LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Bif_ROTATE::reverse(Value_P B, Axis axis)
{
   if (B->is_scalar())
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

const Shape3 shape_B3(B->get_shape(), axis);

Value_P Z(B->get_shape(), LOC);

   loop(h, shape_B3.h())
   loop(l, shape_B3.l())
       {
         const ShapeItem hl = h * shape_B3.m() * shape_B3.l() + l;
         const Cell * cB = &B->get_ravel(hl);
         Cell * cZ = &Z->get_ravel(hl);
         loop(m, shape_B3.m())
            cZ[m*shape_B3.l()]
               .init(cB[(shape_B3.m() - m - 1)*shape_B3.l()], Z.getref(), LOC);
       }

   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_ROTATE::rotate(Value_P A, Value_P B, Axis axis)
{
int32_t gsh = 0;   // global shift (scalar A); 0 means local shift (A) used.

const Shape3 shape_B3(B->get_shape(), axis);
const Shape shape_A2(shape_B3.h(), shape_B3.l());

   if (A->is_scalar_or_len1_vector())
      {
        gsh = A->get_ravel(0).get_near_int();
        if (gsh == 0)   // nothing to do.
           {
             Token result(TOK_APL_VALUE1, B->clone(LOC));
             return result;
           }
      }
   else   // otherwise shape A must be shape B with axis removed.
      {
        A->get_shape().check_same(B->get_shape().without_axis(axis),
                                 E_RANK_ERROR, E_LENGTH_ERROR, LOC);
      }


Value_P Z(B->get_shape(), LOC);

   loop(h, shape_B3.h())
   loop(m, shape_B3.m())
   loop(l, shape_B3.l())
       {
         ShapeItem src = gsh;
         if (!src)   src = A->get_ravel(l + h*shape_B3.l()).get_near_int();
         src += shape_B3.m() + m;
         while (src < 0)               src += shape_B3.m();
         while (src >= shape_B3.m())   src -= shape_B3.m();
         Z->next_ravel()
          ->init(B->get_ravel(shape_B3.hml(h, src, l)), Z.getref(), LOC);
       }

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE::eval_XB(Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return reverse(B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return rotate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE1::eval_XB(Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return reverse(B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE1::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return rotate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TRANSPOSE::eval_B(Value_P B) const
{
Shape shape_A;

   // monadic transpose is A⍉B with A = ... 4 3 2 1 0
   //
   loop(r, B->get_rank())   shape_A.add_shape_item(B->get_rank() - r - 1);

Value_P Z = transpose(shape_A, B);
   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TRANSPOSE::eval_AB(Value_P A, Value_P B) const
{
   // A should be a scalar or vector.
   //
   if (A->get_rank() > 1)   RANK_ERROR;

   if (B->is_scalar())   // B is a scalar (so A should be empty)
      {
        if (A->element_count() != 0)   LENGTH_ERROR;
        Value_P Z = B->clone(LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape shape_A(A.get(), Workspace::get_IO());
   if (shape_A.get_rank() != B->get_rank())   LENGTH_ERROR;

   // the elements in A shall be valid axes of B->
   loop(r, A->get_rank())
      {
        if (shape_A.get_shape_item(r) < 0)               DOMAIN_ERROR;
        if (shape_A.get_shape_item(r) >= B->get_rank())   DOMAIN_ERROR;
      }

Value_P Z = (shape_A.get_rank() == B->get_rank() && is_permutation(shape_A))
          ? transpose(shape_A, B) : transpose_diag(shape_A, B);

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TRANSPOSE::transpose(const Shape & A, Value_P B)
{
const Shape shape_Z = permute(B->get_shape(), inverse_permutation(A));
Value_P Z(shape_Z, LOC);

   if (shape_Z.is_empty())
      {
         Z->set_default(*B.get(), LOC);
         return Z;
      }

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it_Z(shape_Z); it_Z.more(); ++it_Z)
       {
         const Shape idx_B = permute(it_Z.get_offsets(), A);
         const ShapeItem b = B->get_shape().ravel_pos(idx_B);
         Z->next_ravel()->init(cB[b], Z.getref(), LOC);
       }

   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TRANSPOSE::transpose_diag(const Shape & A, Value_P B)
{
   // check that: ∧/(⍳⌈/0,A)∈A
   //
   // I.e. 0, 1, ... max_A are in A
   // we search sequentially, since A is small.
   // we construct shape_Z as we go.
   //
Shape shape_Z;
   {
     // check that 0 ≤ A[a] ≤ ⍴⍴B, and compute max_A = ⌈/A
     // i.e. ∧/(ι/0,L)∈L   lrm p. 253
     ShapeItem max_A = 0;   // the largest index in A (- qio).
     loop(ra, A.get_rank())
         {
           if (A.get_shape_item(ra) < 0)                DOMAIN_ERROR;
           if (A.get_shape_item(ra) >= B->get_rank())   DOMAIN_ERROR;
           if (max_A < A.get_shape_item(ra))   max_A = A.get_shape_item(ra);
         }

     // check that every m < max_A is in A.
     // the smallest axis in B found is added to shape_Z.
     //
     loop(m, max_A + 1)
         {
           ShapeItem min_Bm = -1;
           loop(ra, A.get_rank())
               {
                if (m != A.get_shape_item(ra))   continue;
                const ShapeItem B_ra = B->get_shape_item(ra);
                if (min_Bm == -1)   min_Bm = B_ra;
                else if (min_Bm > B_ra)   min_Bm = B_ra;
               }

           if (min_Bm == -1)   DOMAIN_ERROR;   // m not in A

           shape_Z.add_shape_item(min_Bm);
         }
   }

Value_P Z(shape_Z, LOC);
   if (Z->is_empty())
      {
         Z->set_default(*B.get(), LOC);
        return Z;
      }

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it_Z(shape_Z); it_Z.more(); ++it_Z)
       {
         const Shape idx_B = permute(it_Z.get_offsets(), A);
         const ShapeItem b = B->get_shape().ravel_pos(idx_B);
         Z->next_ravel()->init(cB[b], Z.getref(), LOC);
       }

   return Z;
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_TRANSPOSE::inverse_permutation(const Shape & sh)
{
ShapeItem rho[MAX_RANK];

   // first, set all items to -1.
   //
   loop(r, sh.get_rank())   rho[r] = -1;

   // then, set all items to the shape items of sh
   //
   loop(r, sh.get_rank())
       {
         const ShapeItem rx = sh.get_shape_item(r);
         if (rx < 0)                 DOMAIN_ERROR;
         if (rx >= sh.get_rank())    DOMAIN_ERROR;
         if (rho[rx] != -1)          DOMAIN_ERROR;

         rho[rx] = r;
       }

   return Shape(sh.get_rank(), rho);
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_TRANSPOSE::permute(const Shape & sh, const Shape & perm)
{
Shape ret;

   loop(r, perm.get_rank())
      {
        ret.add_shape_item(sh.get_shape_item(perm.get_shape_item(r)));
      }

   return ret;
}
//-----------------------------------------------------------------------------
bool
Bif_F12_TRANSPOSE::is_permutation(const Shape & sh)
{
ShapeItem rho[MAX_RANK];

   // first, set all items to -1.
   //
   loop(r, sh.get_rank())   rho[r] = -1;

   // then, set all items to the shape items of sh
   //
   loop(r, sh.get_rank())
       {
         const ShapeItem rx = sh.get_shape_item(r);
         if (rx < 0)                return false;
         if (rx >= sh.get_rank())   return false;
         if (rho[rx] != -1)         return false;
          rho[rx] = r;
       }

   return true;
}
//=============================================================================
Token
Bif_F12_DECODE::eval_AB(Value_P A, Value_P B) const
{
   // ρZ  is: (¯1↓ρA),1↓ρB
   // ρρZ is: (0⌈¯1+ρρA) + (0⌈¯1+ρρB)
   //
Shape shape_A1;
   if (!A->is_scalar())
      shape_A1 = A->get_shape().without_axis(A->get_rank() - 1);

Shape shape_B1(B->get_shape());
   if (shape_B1.get_rank() > 0)   shape_B1 = B->get_shape().without_axis(0);

const ShapeItem l_len_A = A->get_rank() ? A->get_last_shape_item() : 1;
const ShapeItem h_len_B = B->get_rank() ? B->get_shape_item(0)     : 1;

const ShapeItem h_len_A = shape_A1.get_volume();
const ShapeItem l_len_B = shape_B1.get_volume();

   if (l_len_A == 0 || h_len_B == 0)   // empty result
      {
        const Shape shape_Z(shape_A1 + shape_B1);
        Value_P Z(shape_Z, LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if ((l_len_A != 1) && (h_len_B != 1) && (l_len_A != h_len_B))   LENGTH_ERROR;

const double qct = Workspace::get_CT();

const Shape shape_Z = shape_A1 + shape_B1;

Value_P Z(shape_Z, LOC);

const Cell * cA = &A->get_ravel(0);

   loop(h, h_len_A)
       {
         // cA ... cA + len_A are used. See if they are complex.
         //
         bool complex_A = false;
         bool integer_A = true;
         loop(aa, l_len_A)
             {
                if (!cA[aa].is_near_real())
                   {
                     complex_A = true;
                     integer_A = false;
                     break;
                   }

                if (!cA[aa].is_near_int())   integer_A = false;
             }

         loop(l, l_len_B)
             {
                // cB, cB + l_len_B, ... are used. See if they are complex
                //
                bool complex_B = false;
                bool integer_B = true;
                loop(bb, h_len_B)
                    {
                      if (!B->get_ravel(l + bb*l_len_B).is_near_real())
                         {
                           complex_B = true;
                           integer_B = false;
                           break;
                         }

                      if (!B->get_ravel(l + bb*l_len_B).is_near_int())
                         integer_B = false;
                    }

               const Cell * cB = &B->get_ravel(l);
               Cell * cZ = Z->next_ravel();
               if (integer_A && integer_B)
                  {
                    const bool overflow = decode_int(cZ, l_len_A, cA,
                                                     h_len_B, cB, l_len_B);
                     if (!overflow)   continue;

                     // otherwise: compute as float
                  }

               if (complex_A || complex_B)
                  decode_complex(cZ, l_len_A, cA, h_len_B, cB, l_len_B, qct);
               else
                  decode_real(cZ, l_len_A, cA, h_len_B, cB, l_len_B, qct);
             }
         cA += l_len_A;
       }

   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
bool
Bif_F12_DECODE::decode_int(Cell * cZ, ShapeItem len_A, const Cell * cA,
                       ShapeItem len_B, const Cell * cB, ShapeItem dB)
{
APL_Integer value = 0;
APL_Float value_f = 0.0;

APL_Integer weight = 1;
APL_Float weight_f = 1.0;
const ShapeItem len = (len_A == 1) ? len_B : len_A;

   cA += len_A - 1;         // cA points to the lowest item in A
   cB += dB*(len_B - 1);    // cB points to the lowest item in B
   loop(l, len)
      {
        if (weight_f > LARGE_INT)   return true;
        if (weight_f < SMALL_INT)   return true;

        const APL_Integer vB = cB[0].get_near_int();
        value   = value   + weight   * vB;
        value_f = value_f + weight_f * vB;
        if (value_f > LARGE_INT)   return true;
        if (value_f < SMALL_INT)   return true;

        weight   = weight   * cA[0].get_near_int();
        weight_f = weight_f * cA[0].get_near_int();
        if (len_A != 1)   --cA;
        if (len_B != 1)   cB -= dB;
      }

   new (cZ)   IntCell(value);

   return false;   // no overflow
}
//-----------------------------------------------------------------------------
void
Bif_F12_DECODE::decode_real(Cell * cZ, ShapeItem len_A, const Cell * cA,
                       ShapeItem len_B, const Cell * cB, ShapeItem dB,
                       double qct)
{
APL_Float value = 0.0;
APL_Float weight = 1.0;
const ShapeItem len = (len_A == 1) ? len_B : len_A;

   cA += len_A - 1;         // cA points to the lowest item in A
   cB += dB*(len_B - 1);    // cB points to the lowest item in B
   loop(l, len)
      {
        value += weight*cB[0].get_real_value();
        weight *= cA[0].get_real_value();
        if (len_A != 1)   --cA;
        if (len_B != 1)   cB -= dB;
      }

   if (value < LARGE_INT &&
       value > SMALL_INT &&
       Cell::is_near_int(value))
      new (cZ)   IntCell(Cell::near_int(value));
   else
      new (cZ)   FloatCell(value);
}
//-----------------------------------------------------------------------------
void
Bif_F12_DECODE::decode_complex(Cell * cZ, ShapeItem len_A, const Cell * cA,
                       ShapeItem len_B, const Cell * cB, ShapeItem dB,
                       double qct)
{
APL_Complex value(0.0, 0.0);
APL_Complex weight(1.0, 0.0);
const ShapeItem len = (len_A == 1) ? len_B : len_A;

   cA += len_A - 1;         // cA points to the lowest item in A
   cB += dB*(len_B - 1);    // cB points to the lowest item in B
   loop(l, len)
      {
        value += weight*cB[0].get_complex_value();
        weight *= cA[0].get_complex_value();
        if (len_A != 1)   --cA;
        if (len_B != 1)   cB -= dB;
      }

   if (value.imag() > qct)
      new (cZ)   ComplexCell(value);
   else if (value.imag() < -qct)
      new (cZ)   ComplexCell(value);
   else if (value.real() < LARGE_INT
         && value.real() > SMALL_INT
         && Cell::is_near_int(value.real()))
      new (cZ)   IntCell(value.real());
   else
      new (cZ)   FloatCell(value.real());
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ENCODE::eval_AB(Value_P A, Value_P B) const
{
   if (A->is_scalar())   return Bif_F12_STILE::fun->eval_AB(A, B);

const ShapeItem ec_A = A->element_count();
const ShapeItem ec_B = B->element_count();
   if (ec_A == 0 || ec_B == 0)   // empty A or B
      {
        const Shape shape_Z(A->get_shape() + B->get_shape());
        Value_P Z(shape_Z, LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const ShapeItem ah = A->get_shape_item(0);    // first dimension
const ShapeItem al = A->element_count()/ah;   // remaining dimensions

const double qct = Workspace::get_CT();

const Shape shape_Z = A->get_shape() + B->get_shape();
const ShapeItem dZ = shape_Z.get_volume()/shape_Z.get_shape_item(0);

Value_P Z(shape_Z, LOC);

   loop(a1, al)
      {
        const Cell * cB = &B->get_ravel(0);

        // find largest Celltype in a1...
        //
        CellType ct_a1 = CT_INT;
        loop(h, ah)
            {
              const CellType ct = A->get_ravel(a1 + h*al).get_cell_type();
              if (ct == CT_INT)            ;
              else if (ct == CT_FLOAT)     { if (ct_a1 == CT_INT)  ct_a1 = ct; }
              else if (ct == CT_COMPLEX)   ct_a1 = CT_COMPLEX;
              else                         DOMAIN_ERROR;
            }

        loop(b, ec_B)
            {
              CellType ct = ct_a1;
              const CellType ct_b = cB->get_cell_type();
              if (ct_b == CT_INT)            ;
              else if (ct_b == CT_FLOAT)     { if (ct == CT_INT)  ct = ct_b; }
              else if (ct_b == CT_COMPLEX)   ct = CT_COMPLEX;
              else                           DOMAIN_ERROR;

              if (ct == CT_INT)   // both cells are integer
                 encode(dZ, Z->next_ravel(), ah, al,
                        &A->get_ravel(a1), cB++->get_int_value());
              else if (ct == CT_FLOAT)
                 encode(dZ, Z->next_ravel(), ah, al,
                        &A->get_ravel(a1), cB++->get_real_value(), qct);
              else
                 encode(dZ, Z->next_ravel(), ah, al,
                        &A->get_ravel(a1), cB++->get_complex_value(), qct);
            }
       }

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, APL_Integer b)
{
   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;

        const IntCell cC(b);
        cC.bif_residue(cZ, cA);

        if (cA->get_int_value() == 0)
           {
             b = 0;
           }
         else
           {
             b -= cZ->get_int_value();
             b /= cA->get_int_value();
           }
       }
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, APL_Float b, double qct)
{
   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;

        const FloatCell cC(b);
        cC.bif_residue(cZ, cA);

        if (cA->is_near_zero())
           {
             b = 0.0;
           }
         else
           {
             b -= cZ->get_real_value();
             b /= cA->get_real_value();
           }
       }
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, APL_Complex b, double qct)
{
   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;

        const ComplexCell cC(b);
        cC.bif_residue(cZ, cA);

        if (cA->is_near_zero())
           {
             b = APL_Complex(0, 0);
           }
         else
           {
             b -= cZ->get_complex_value();
             b /= cA->get_complex_value();
           }
       }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ELEMENT::eval_B(Value_P B) const
{
   // enlist
   //
   if (B->element_count() == 0)   // empty argument
      {
        ShapeItem N = 0;
        Value_P Z(N, LOC);   // empty vector with prototype ' ' or '0'
        const Cell * C = &B->get_ravel(0);
        bool left = false;
        for (;;)
            {
              if (C->is_pointer_cell())
                 {
                   C = &C->get_pointer_value()->get_ravel(0);
                   continue;
                 }

              if (left && C->is_lval_cell())
                 {
                   C = C->get_lval_value();
                   if (C == 0)
                      {
                        CERR << "0-pointer at " LOC << endl;
                        FIXME;
                      }
                   else if (C->is_pointer_cell())
                      {
                        C = &C->get_pointer_value()->get_ravel(0);
                      }
                   else
                      {
                        Value * owner = C->cLvalCell().get_cell_owner();
                        new (&Z->get_ravel(0))
                            LvalCell(C->get_lval_value(), owner);
                        break;
                      }
                 }

              if (C->is_numeric())
                 {
                   new (&Z->get_ravel(0)) CharCell(UNI_ASCII_0);
                   break;
                 }

              if (C->is_character_cell())
                 {
                   new (&Z->get_ravel(0)) CharCell(UNI_ASCII_SPACE);
                   break;
                 }

              if (C->is_lval_cell())
                 {
                   left = C->cLvalCell().get_cell_owner();
                   C = C->get_lval_value();
                   continue;
                 }

               // not reached
               //
               FIXME;
            }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const ShapeItem len_Z = B->get_enlist_count();
Value_P Z(len_Z, LOC);

Cell * z = &Z->get_ravel(0);
   if (B->get_lval_cellowner())   B->enlist_left(z, Z.getref());
   else                           B->enlist_right(z, Z.getref());

   Z->set_default(*B.get(), LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ELEMENT::eval_AB(Value_P A, Value_P B) const
{
   // member
   //
const double qct = Workspace::get_CT();
const ShapeItem len_Z = A->element_count();
const ShapeItem len_B = B->element_count();
Value_P Z(A->get_shape(), LOC);

   loop(z, len_Z)
       {
         const Cell & cell_A = A->get_ravel(z);
         APL_Integer same = 0;
         loop(b, len_B)
             if (cell_A.equal(B->get_ravel(b), qct))
                {
                  same = 1;
                  break;
                }
         new (Z->next_ravel())   IntCell(same);
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INDEX::eval_AB(Value_P A, Value_P B) const
{
   if (A->get_rank() > 1)   RANK_ERROR;

const ShapeItem ec_A = A->element_count();
   if (ec_A != B->get_rank())   RANK_ERROR;

   // index_expr is in reverse order!
   //
IndexExpr index_expr(ASS_none, LOC);
   loop(a, ec_A)
      {
         const Cell & cell = A->get_ravel(ec_A - a - 1);
         Value_P val;
         if (cell.is_pointer_cell())
            {
              val = cell.get_pointer_value()->clone(LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
            }
        else
            {
              const APL_Integer i = cell.get_near_int();
              val = Value_P(LOC);
              if (i < 0)   DOMAIN_ERROR;

              new (&val->get_ravel(0))   IntCell(i);
            }

        index_expr.add(val);
      }

   // the index() do set_default() and check_value(), so we return immediately
   //
   index_expr.quad_io = Workspace::get_IO();

   if (index_expr.value_count() == 1)   // one-dimensional index
      {
        Value_P single_index = index_expr.extract_value(0);
        Value_P Z = B->index(single_index);
        return Token(TOK_APL_VALUE1, Z);
      }
   else
      {
        Value_P Z = B->index(index_expr);
        return Token(TOK_APL_VALUE1, Z);
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INDEX::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
   if (A->get_rank() > 1)   RANK_ERROR;

const Shape axes_present = Value::to_shape(X.get());

const ShapeItem ec_A = A->element_count();
   if (ec_A != axes_present.get_rank())   RANK_ERROR;
   if (ec_A > B->get_rank())              RANK_ERROR;

   // index_expr is in reverse order!
   //
const APL_Integer qio = Workspace::get_IO();
IndexExpr index_expr(ASS_none, LOC);
   loop(rb, B->get_rank())   index_expr.add(Value_P());
   index_expr.quad_io = qio;

   loop(a, ec_A)
      {
         const Rank axis = axes_present.get_shape_item(a);

         const Cell & cell = A->get_ravel(a);
         Value_P val;
         if (cell.is_pointer_cell())
            {
              val = cell.get_pointer_value()->clone(LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
            }
        else
            {
              const APL_Integer i = cell.get_near_int();
              val = Value_P(LOC);
              if (i < 0)   DOMAIN_ERROR;

              new (&val->get_ravel(0))   IntCell(i);
            }

        index_expr.set_value(axis, val);
      }

   if (index_expr.value_count() == 1)
      {
        Value_P single_index = index_expr.extract_value(0);
        Value_P Z = B->index(single_index);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }
   else
      {
        Value_P Z = B->index(index_expr);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }
}
//=============================================================================
Token
Bif_F12_EQUIV::eval_B(Value_P B) const
{
const APL_types::Depth depth = B->compute_depth();

Value_P Z(LOC);
   new (Z->next_ravel()) IntCell(depth);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_EQUIV::eval_AB(Value_P A, Value_P B) const
{
   // match
   //

const double qct = Workspace::get_CT();
const ShapeItem count = A->nz_element_count();  // compare at least prototype

   if (!A->same_shape(*B))   //shape mismatch
      return Token(TOK_APL_VALUE1, IntScalar(0, LOC));

   loop(c, count)
       {
         if (!A->get_ravel(c).equal(B->get_ravel(c), qct))
            {
              return Token(TOK_APL_VALUE1, IntScalar(0, LOC));   // no match
            }
       }

   return Token(TOK_APL_VALUE1, IntScalar(1, LOC));   // match
}
//=============================================================================
Token
Bif_F12_NEQUIV::eval_B(Value_P B) const
{
   // Tally
   //
const ShapeItem len = B->is_scalar() ? 1 : B->get_shape().get_shape_item(0);

   return Token(TOK_APL_VALUE1, IntScalar(len, LOC));   // match
}
//-----------------------------------------------------------------------------
Token
Bif_F12_NEQUIV::eval_AB(Value_P A, Value_P B) const
{
   // match
   //

const double qct = Workspace::get_CT();
const ShapeItem count = A->nz_element_count();  // compare at least prototype

   if (!A->same_shape(*B))   return Token(TOK_APL_VALUE1, IntScalar(1, LOC));   // no match

   loop(c, count)
       if (!A->get_ravel(c).equal(B->get_ravel(c), qct))
          {
            return Token(TOK_APL_VALUE1, IntScalar(1, LOC));   // no match
          }

   return Token(TOK_APL_VALUE1, IntScalar(0, LOC));   // match
}
//=============================================================================
Token
Bif_F1_EXECUTE::eval_B(Value_P B) const
{
   if (B->get_rank() > 1)   RANK_ERROR;

UCS_string statement(*B.get());

   if (statement.size() == 0)   return Token(TOK_NO_VALUE);

   return execute_statement(statement);
}
//-----------------------------------------------------------------------------
Token
Bif_F1_EXECUTE::execute_statement(UCS_string & statement)
{
   statement.remove_leading_and_trailing_whitespaces();

   // check for commands
   //
   if (statement.size() &&
       (statement[0] == UNI_ASCII_R_PARENT ||
        statement[0] == UNI_ASCII_R_BRACK))   return execute_command(statement);

ExecuteList * fun = ExecuteList::fix(statement.no_pad(), LOC);
   if (fun == 0)   SYNTAX_ERROR;

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   // important special case: ⍎ of an APL literal value
   //
   if (fun->get_body().size() == 2 &&
       fun->get_body()[0].get_Class() == TC_VALUE)
      {
        Value_P Z = fun->get_body()[0].get_apl_val();
        delete fun;
        return Token(TOK_APL_VALUE1, Z);
      }

   Workspace::push_SI(fun, LOC);

   Log(LOG_StateIndicator__push_pop)
      {
        Workspace::SI_top()->info(CERR, LOC);
      }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
Bif_F1_EXECUTE::execute_command(UCS_string & command)
{
   if (copy_pending &&
       (
//      command.starts_iwith(")COPY")    ||
        command.starts_iwith(")ERASE")   ||
        command.starts_iwith(")FNS")     ||
        command.starts_iwith(")NMS")     ||
        command.starts_iwith(")QLOAD")   ||
        command.starts_iwith(")SYMBOLS") ||
        command.starts_iwith(")VARS")))
      {
        throw_apl_error(E_COPY_PENDING, LOC);
      }

   if (command.starts_iwith(")LOAD")  ||
       command.starts_iwith(")QLOAD") ||
       command.starts_iwith(")CLEAR") ||
       command.starts_iwith(")RESET") ||
       command.starts_iwith(")SIC"))
      {
        // the command modifies the SI stack. We throw E_COMMAND_PUSHED
        // but without displaying it. That should bring us back to
        // Command::do_APL_expression() with token.get_tag() == TOK_ERROR
        //
        Workspace::push_Command(command);
        Error error(E_COMMAND_PUSHED, LOC);
        throw error;
      }

UTF8_ostream out;
const bool user_cmd = Command::do_APL_command(out, command);
   if (user_cmd)   return execute_statement(command);

UTF8_string result_utf8 = out.get_data();
   if (result_utf8.size() == 0 ||
       result_utf8.back() != UNI_ASCII_LF)
      result_utf8 += '\n';

std::vector<ShapeItem> line_starts;
   line_starts.push_back(0);
   loop(r, result_utf8.size())
      {
        if (result_utf8[r] == UNI_ASCII_LF)   line_starts.push_back(r + 1);
      }

Value_P Z(ShapeItem(line_starts.size() - 1), LOC);
   loop(l, line_starts.size() - 1)
      {
        ShapeItem len;
        if (l < ShapeItem(line_starts.size() - 1))
           len = line_starts[l + 1] - line_starts[l] - 1;
        else
           len = result_utf8.size() - line_starts[l];

        UTF8_string line_utf8(&result_utf8[line_starts[l]], len);
        UCS_string line_ucs(line_utf8);
        Value_P ZZ(line_ucs, LOC);
        new (Z->next_ravel())   PointerCell(ZZ.get(), Z.getref());
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
ShapeItem
Bif_F12_UNION::append_zone(const Cell ** cells_Z, const Cell ** cells_B,
               Zone_list & B_from_to, double qct)
{
const Cell ** Z0 = cells_Z;

   while (B_from_to.size())
      {
        const Zone zone = B_from_to.back();
        const ShapeItem zone_count = zone.count();
        Assert(zone_count > 0);
        B_from_to.pop_back();

        const ShapeItem B_from = zone.from;
        const ShapeItem B_to   = zone.to;

        // Find the smallest pointer (i.e. smallest Cell address, not the
        // smallest Cell value) in the zone. This pointer will go into Z and
        //  it will kill all its neighbours that are equal within qct.
        //

#if 0
        // this fails when used with char cells but is quite handy for
        // testing the algorithm!
        Q1(LOC)
        fprintf(stderr, "%ld-element zone:\n", B_to - B_from);
        for (ShapeItem j = B_from; j < B_to; ++j)
            fprintf(stderr, "[%ld] value: %.12f\n", j,
                    cells_B[j]->get_real_value());
#endif


        // the by far most likely cases are zones with 1 or,
        // already less likely, 2 cells. These cases can be handled
        // without searching the smallest element and are, for
        // performance reasons, handled beforehand.
        //
        if (zone_count == 1)
           {
             *cells_Z++ = cells_B[B_from];
             continue;   // zone done
           }

        if (zone_count == 2)
           {
             if (cells_B[B_from] < cells_B[B_from + 1])
                *cells_Z++ = cells_B[B_from];
             else
                *cells_Z++ = cells_B[B_from + 1];
             continue;   // zone done
           }

        ShapeItem smallest = B_from;
        for (ShapeItem bb = B_from + 1; bb < B_to; ++bb)
            {
              if (cells_B[smallest] > cells_B[bb])   smallest = bb;
            }

        //  See if the zone is transitive or not.
        //
        if (cells_B[smallest]->equal(*cells_B[B_to - 1], qct))
           {
             // the zone is transitive (i.e. all cells are equal within ⎕CT).
             // The unique of the zone is then the smallest pointer (i.e.first,
             // not the smallest Cell) in the zone.
             //
             *cells_Z++ = cells_B[smallest];
             continue;   // zone done
           }

        // the zone is not transitive, i.e. cells_B[B_from] is NOT equal to
        // cells_B[B_to-1]. Divide the zone into 3 (possibly empty) zones:
        //
        // a.   cells < smallest within ⎕CT,
        // b.   cells = smallest within ⎕CT, and
        // c.  cells > smallest within ⎕CT

        // Start with the middle zone b. This zone initially contains only
        // smallest and its then blown up with elements that are equal (within)
        // ⎕CT) to smallest.
        //
        const Cell * first_B = cells_B[smallest];
        *cells_Z++ = first_B;
        ShapeItem from1 = smallest;      // initial start of zone b.
        ShapeItem to1   = smallest;      // initial end of zone b.

        // decrement from1 as long as its cell is within ⎕CT of smallest.
        // That makes all
        while (from1 > B_from && first_B->equal(*cells_B[from1], qct))  --from1;
        if (from1 > B_from)   // non-empty zone before smallest
           {
             const Zone smaller(B_from, from1);
             B_from_to.push_back(smaller);
             continue;   // zone done
           }

        // increment to1 as long as its cell is within ⎕CT of smallest.
        //
        while (to1 < B_to && first_B->equal(*cells_B[to1], qct))   ++to1;
        if (to1 < B_to)   // non-empty zone after smallest
           {
             Zone larger(to1, B_to);
             B_from_to.push_back(larger);
             continue;   // zone done
           }
      }

   return cells_Z - Z0;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_UNION::eval_B(Value_P B) const
{
   if (B->get_rank() > 1)   RANK_ERROR;

const double qct = Workspace::get_CT();
const ShapeItem len_B = B->element_count();
   if (len_B <= 1)   return Token(TOK_APL_VALUE1, B->clone(LOC));

   // 1. construct a vector of Cell pointers and sort it so that the
   //    cells being pointed to are sorted ascendingly.
   //
   // For efficiency we allocate both the Cell pointers for argument B and
   // for the result Z. Sorting is first done with ⎕CT←0; ⎕CT will be
   // considered later on.
   //
const Cell ** cells_B = new const Cell *[2*len_B];
   if (cells_B == 0)   WS_FULL;

const Cell ** cells_Z = cells_B + len_B;
ShapeItem len_Z = 0;

   loop(b, len_B)   cells_B[b] = &B->get_ravel(b);
   Heapsort<const Cell*>::sort(cells_B, len_B, 0, Cell::A_greater_B);

   // 2. divide the cells into zones. A zone is a sequence of cells
   //    B[from] (including)  ... B[to] (excluding) so that the difference
   //    between B[i] and B[i+1] is < ⎕CT.
   //
   //    Then append the unique element(s) of each zone to cells_Z.
   //
ShapeItem from = 0;
Zone_list from_to;
   for (ShapeItem b = 1; b < len_B; ++b)
       {
           if (cells_B[b]->equal(*cells_B[b-1], qct))
              {
                // cells_B[b] belongs to the zone
                //
                continue;
              }

           const Zone ft0(from, b);
           from_to.push_back(ft0);
           len_Z += append_zone(cells_Z + len_Z, cells_B, from_to, qct);
           Assert(from_to.size() == 0);
           from = b;
           continue;   // next b (in the next zone)
       }
   if (from < len_B)   // the rest
      {
        const Zone ft0(from, len_B);
        from_to.push_back(ft0);
        len_Z += append_zone(cells_Z + len_Z, cells_B, from_to, qct);
        Assert(from_to.size() == 0);
      }

   // 3. cells_Z now contains only the unique cells in cells_B, but sorted by
   //    cell contnt. Sort cells_Z by address (= position in B)  so that the
   //    original order in B is reconstructed
   //
   Heapsort<const Cell *>::sort(cells_Z, len_Z, 0, Cell::compare_ptr);

   // 4. construct the result.
   //
Value_P Z(len_Z, LOC);
   loop(z, len_Z)   Z->next_ravel()->init(*cells_Z[z], Z.getref(), LOC);
   delete[] cells_B;   // also deletes cells_Z
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INTER::eval_AB(Value_P A, Value_P B) const
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (B->get_rank() > 1)   RANK_ERROR;

const ShapeItem len_A = A->element_count();
const ShapeItem len_B = B->element_count();

const double qct = Workspace::get_CT();

   if (len_A*len_B > 60*60)
      {
        // large A or B: sort A and B to speed up searches
        //
        const Cell ** cells_A = new const Cell *[2*len_A + len_B];
        const Cell ** cells_Z = cells_A + len_A;
        const Cell ** cells_B = cells_A + 2*len_A;

        loop(a, len_A)   cells_A[a] = &A->get_ravel(a);
        loop(b, len_B)   cells_B[b] = &B->get_ravel(b);

        Heapsort<const Cell *>::sort(cells_A, len_A, 0, Cell::compare_stable);
        Heapsort<const Cell *>::sort(cells_B, len_B, 0, Cell::compare_stable);

        ShapeItem len_Z = 0;
        ShapeItem idx_B = 0;
        loop(idx_A, len_A)
            {
              const Cell * ref = cells_A[idx_A];
              while (idx_B < len_B)
                  {
                    if (ref->equal(*cells_B[idx_B], qct))
                       {
                         cells_Z[len_Z++] = ref;   // A is in B
                         break;   // for idx_B → next idx_A
                       }

                    // B is much (by ⎕CT) smaller or greater than A
                    //
                    if (ref->greater(*cells_B[idx_B]))    ++idx_B;
                    else                                  break;
                 }
            }

        // sort cells_Z by position so that the original order in A is
        //  reconstructed
        //
        Heapsort<const Cell *>::sort(cells_Z, len_Z, 0, Cell::compare_ptr);
        Value_P Z(len_Z, LOC);
        loop(z, len_Z)   Z->next_ravel()->init(*cells_Z[z], Z.getref(), LOC);

        Z->set_default(*B, LOC);
        Z->check_value(LOC);
        delete[] cells_A;
        return Token(TOK_APL_VALUE1, Z);
      }
    else
      {
        // small A and B: use quadratic algorithm.
        //
        const Cell ** cells_Z = new const Cell *[len_A];
        ShapeItem len_Z = 0;

        loop(a, len_A)
        loop(b, len_B)
            {
              if (A->get_ravel(a).equal(B->get_ravel(b), qct))
                 {
                   cells_Z[len_Z++] = &A->get_ravel(a);
                   break;   // loop(b)
                 }
            }
        Value_P Z(len_Z, LOC);
        loop(z, len_Z)
            Z->next_ravel()->init(*cells_Z[z], Z.getref(), LOC);

        Z->set_default(*B, LOC);
        Z->check_value(LOC);
        delete[] cells_Z;
        return Token(TOK_APL_VALUE1, Z);
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_UNION::eval_AB(Value_P A, Value_P B) const
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (B->get_rank() > 1)   RANK_ERROR;

   // A ∪ B ←→ A,B∼A
   //
Token BwoA = Bif_F12_WITHOUT::fun->eval_AB(B, A);

const ShapeItem len_A = A->element_count();
const ShapeItem len_B = BwoA.get_apl_val()->element_count();
Value_P Z(len_A + len_B, LOC);

   loop(a, len_A)   Z->next_ravel()->init(A->get_ravel(a), Z.getref(), LOC);
   loop(b, len_B)   Z->next_ravel()->init(B->get_ravel(b), Z.getref(), LOC);
   Z->set_default(*B, LOC);
   Z->check_value(LOC);
   return Bif_F12_COMMA::fun->eval_AB(A, BwoA.get_apl_val());
}
//=============================================================================
Token
Bif_F2_RIGHT::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
   // select corresponding items of A or B according to X. A, B, and X
   // must have matching shapes
   //
const int inc_A = A->is_scalar_extensible() ? 0 : 1;
const int inc_B = B->is_scalar_extensible() ? 0 : 1;
const int inc_X = X->is_scalar_extensible() ? 0 : 1;

   if (inc_X == 0)   // single item X: pick entire A or B according to X
      {
        const APL_Integer x0 = X->get_ravel(0).get_int_value();
        if (x0 == 0)   return Token(TOK_APL_VALUE1, A->clone(LOC));
        if (x0 == 1)   return Token(TOK_APL_VALUE1, B->clone(LOC));
        DOMAIN_ERROR;
      }

   // X is non-scalar, so it must match any non-scalar A and B
   //
   if (inc_A && ! X->same_shape(*A))
      {
        if (A->get_rank() != X->get_rank())   RANK_ERROR;
        else                                  LENGTH_ERROR;
      }

   if (inc_B && ! X->same_shape(*B))
      {
        if (B->get_rank() != X->get_rank())   RANK_ERROR;
        else                                  LENGTH_ERROR;
      }

Value_P Z(X->get_shape(), LOC);
   loop(z, Z->nz_element_count())
       {
        const APL_Integer xz = X->get_ravel(z).get_int_value();
        Cell * cZ = Z->next_ravel();
        if      (xz == 0)   cZ->init(A->get_ravel(z*inc_A), Z.getref(), LOC);
        else if (xz == 1)   cZ->init(B->get_ravel(z*inc_B), Z.getref(), LOC);
        else                DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================

