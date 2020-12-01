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

#include <vector>

#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "LvalCell.hh"
#include "Macro.hh"
#include "Workspace.hh"

Bif_OPER1_SCAN    Bif_OPER1_SCAN ::_fun;
Bif_OPER1_SCAN1   Bif_OPER1_SCAN1::_fun;

Bif_OPER1_SCAN  * Bif_OPER1_SCAN ::fun = &Bif_OPER1_SCAN ::_fun;
Bif_OPER1_SCAN1 * Bif_OPER1_SCAN1::fun = &Bif_OPER1_SCAN1::_fun;

//-----------------------------------------------------------------------------
Token
Bif_SCAN::expand(Value_P A, Value_P B, uAxis axis)
{
   // turn scalar B into ,B
   //
Shape shape_B = B->get_shape();
   if (shape_B.get_rank() == 0)
      {
         shape_B.add_shape_item(1);
         axis = 0;
      }
   if (axis >= shape_B.get_rank())   INDEX_ERROR;

   if (A->get_rank() > 1)            RANK_ERROR;
   if (shape_B.get_rank() <= axis)   RANK_ERROR;

const ShapeItem ec_A = A->element_count();
ShapeItem ones_A = 0;
std::vector<ShapeItem> rep_counts;
   rep_counts.reserve(ec_A);
   loop(a, ec_A)
      {
        APL_Integer rep_A = A->get_ravel(a).get_near_int();
        rep_counts.push_back(rep_A);
        if      (rep_A == 0)        ;
        else if (rep_A == 1)        ++ones_A;
        else                        DOMAIN_ERROR;
      }

Shape shape_Z(shape_B);
   shape_Z.set_shape_item(axis, ec_A);
Value_P Z(shape_Z, LOC);

   if (ec_A == 0)   // (⍳0)/B : 
      {
        if (shape_B.get_shape_item(axis) > 1)   LENGTH_ERROR;

        Z->set_default(*B.get(), LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_Z3(shape_Z, axis);

const Cell * cB = &B->get_ravel(0);
const bool lval = cB->is_lval_cell();

ShapeItem inc_1 = shape_Z3.l();   // increment after result l items
ShapeItem inc_2 = 0;              // increment after result m*l items

   if (B->is_scalar() || (shape_B.get_shape_item(axis) == 1
                      && (shape_Z3.l() != 1)))
      {
         inc_1 = 0;
         inc_2 = shape_Z3.l();
      }
   else if (ones_A != shape_B.get_shape_item(axis))   LENGTH_ERROR;

   loop(h, shape_Z3.h())
      {
        const Cell * fill = cB;
        loop(m, rep_counts.size())
           {
             if (rep_counts[m] == 1)   // copy items from B
                {
                  loop(l, shape_Z3.l())
                      Z->next_ravel()->init(cB[l], Z.getref(), LOC);
                  cB += inc_1;
                }
             else                      // init items
                {
                  if (lval)
                     {
                       loop(l, shape_Z3.l())
                           new (Z->next_ravel()) LvalCell(0, 0);
                     }
                  else
                     {
                       loop(l, shape_Z3.l())
                           Z->next_ravel()->init_type(fill[l], Z.getref(), LOC);
                     }
                }
           }

        cB += inc_2;
      }

   Z->set_default(*B.get(), LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_SCAN::scan(Token & tok_LO, Value_P B, uAxis axis) const
{
   // if B is a scalar, then Z is B.
   //
   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (!tok_LO.is_function())
      {
        MORE_ERROR() << "The left argument of operator A /"
                     << get_name() << " is not a function";
        DOMAIN_ERROR;
      }

Function_P LO = tok_LO.get_function();

   if (!LO->has_result())
      {
        MORE_ERROR() << "The left argument of operator "
                     << get_name() << " is a function that returns no result";
        DOMAIN_ERROR;
      }

   if (LO->get_fun_valence() != 2)
      {
        MORE_ERROR() << "The left argument of operator "
                     << get_name() << " is a function that is not dyadic";
        SYNTAX_ERROR;
      }

   if (axis >= B->get_rank())   AXIS_ERROR;

const ShapeItem m_len = B->get_shape_item(axis);

   if (m_len == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (m_len == 1)
      {
        const Shape shape_Z = B->get_shape().without_axis(axis);
        return Bif_F12_RHO::do_reshape(shape_Z, *B);
      }

const Shape3 shape_Z3(B->get_shape(), axis);

Value_P Z(B->get_shape(), LOC);
ErrorCode (Cell::*assoc_f2)(Cell *, const Cell *) const = LO->get_assoc();

   if (assoc_f2)
      {
        // LO is an associative primitive scalar function.
        //
        const Cell * cB = &B->get_ravel(0);
        loop(h, shape_Z3.h())
        loop(m, shape_Z3.m())
        loop(l, shape_Z3.l())
            {
              Cell * cZ = Z->next_ravel();
              if (m == 0)
                 {
                   cZ->init(*cB++, Z.getref(), LOC);
                 }
              else
                 {

                   Value_P AA(LOC);
                   AA->next_ravel()->init(cZ[-shape_Z3.l()], AA.getref(), LOC);
                   AA->set_complete();

                   Value_P BB(LOC);
                   BB->next_ravel()->init(*cB++, BB.getref(), LOC);
                   BB->set_complete();

                   Token tok = LO->eval_AB(AA, BB);
                   if (!tok.is_apl_val())   return tok;

                   Value_P ZZ = tok.get_apl_val();
                   cZ->init(ZZ->get_ravel(0), ZZ.getref(), LOC);
                   ZZ->set_complete();
                 }
            }
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // non-trivial reduce (len > 1)
   //
const Shape3 Z3(B->get_shape(), axis);
   if (LO->may_push_SI())   // user defined LO
      {
        Value_P X4(4, LOC);
        new (X4->next_ravel())   IntCell(axis + Workspace::get_IO());
        new (X4->next_ravel())   IntCell(Z3.h());
        new (X4->next_ravel())   IntCell(Z3.m());
        new (X4->next_ravel())   IntCell(Z3.l());
        X4->check_value(LOC);
        return Macro::get_macro(Macro::MAC_Z__LO_SCAN_X4_B)
                    ->eval_LXB(tok_LO, X4, B);
      }

   if (B->get_shape().is_empty())   return LO->eval_identity_fun(B, axis);

   return Bif_REDUCE::do_reduce(B->get_shape(), Z3, -1, LO, B, m_len);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return expand(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN::eval_LXB(Token & LO, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return scan(LO, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN1::eval_AXB(Value_P A, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return expand(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN1::eval_LXB(Token & LO, Value_P X, Value_P B) const
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return scan(LO, B, axis);
}
//-----------------------------------------------------------------------------
