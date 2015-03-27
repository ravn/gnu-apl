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

#include "Bif_OPER2_RANK.hh"
#include "IntCell.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_RANK   Bif_OPER2_RANK::_fun;
Bif_OPER2_RANK * Bif_OPER2_RANK::fun = &Bif_OPER2_RANK::_fun;

/* general comment: we use the term 'chunk' instead of 'p-rank' to avoid
 * confusion with the rank of a value
 */

//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LRB(Token & LO, Token & y, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rk_chunk_B = B->get_rank();
   y123_to_B(y.get_apl_val(), rk_chunk_B);

   return do_LyXB(LO.get_function(), 0, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LRXB(Token & LO, Token & y, Value_P X, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rk_chunk_B = B->get_rank();

   y123_to_B(y.get_apl_val(), rk_chunk_B);

Shape sh_X(X, Workspace::get_CT(), Workspace::get_IO());
   return do_LyXB(LO.get_function(), &sh_X, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::do_LyXB(Function * LO, const Shape * axes,
                        Value_P B, Rank rk_chunk_B)
{
   if (!LO->has_result())   DOMAIN_ERROR;

   // split shape of B into high and low shapes.
   //
const Shape shape_Z = B->get_shape().high_shape(B->get_rank() - rk_chunk_B);
Value_P Z(shape_Z, LOC);
EOC_arg arg(Z, B);
RANK & _arg = arg.u.u_RANK;

   _arg.get_sh_chunk_B() = B->get_shape().low_shape(rk_chunk_B);
   _arg.get_sh_frame() = shape_Z;
   _arg.ec_frame   = shape_Z.get_volume();
   if (!_arg.ec_frame)   _arg.ec_frame = 1;
   _arg.ec_chunk_B = _arg.get_sh_chunk_B().get_volume();
   _arg.cB = &B->get_ravel(0);
   _arg.LO = LO;
   _arg.axes_valid = (axes != 0);
   if (axes)   _arg.get_axes() = *axes;
   _arg.h = -1;

   return finish_LyXB(arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::finish_LyXB(EOC_arg & arg, bool first)
{
RANK & _arg = arg.u.u_RANK;

   while (++_arg.h < _arg.ec_frame)
      {
        Value_P BB(_arg.get_sh_chunk_B(), LOC);
        Assert1(_arg.ec_chunk_B == _arg.get_sh_chunk_B().get_volume());
        loop(l, _arg.ec_chunk_B)
            BB->next_ravel()->init(*_arg.cB++, BB.getref());
        BB->check_value(LOC);

        Token result = _arg.LO->eval_B(BB);
        if (result.get_tag() == TOK_SI_PUSHED)
           {
             // LO was a user defined function or ⍎
             //
             if (first)   // first call
                Workspace::SI_top()->add_eoc_handler(eoc_RANK, arg, LOC);
             else           // subsequent call
                Workspace::SI_top()->move_eoc_handler(eoc_RANK, &arg, LOC);

             return result;   // continue in user defined function...
           }

        if (result.get_tag() == TOK_ERROR)   return result;

        if (result.get_Class() == TC_VALUE)
           {
             Value_P ZZ = result.get_apl_val();
             Cell * cZ = arg.Z->next_ravel();
             if (cZ == 0)           cZ = &arg.Z->get_ravel(0);   // empty Z
             if (ZZ->is_scalar())   cZ->init(ZZ->get_ravel(0), arg.Z.getref());
             else                   new (cZ)   PointerCell(ZZ, arg.Z.getref());
             continue;
           }

        Q1(result);   FIXME;
      }

   arg.Z->check_value(LOC);
   if (!_arg.axes_valid)   return Bif_F12_PICK::disclose(arg.Z, true);

   return Bif_F12_PICK::disclose_with_axis(_arg.get_axes(), arg.Z, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALRB(Value_P A, Token & LO, Token & y, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rk_chunk_A = A->get_rank();
Rank rk_chunk_B = B->get_rank();
   y123_to_AB(y.get_apl_val(), rk_chunk_A, rk_chunk_B);

   return do_ALyXB(A, rk_chunk_A, LO.get_function(), 0, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALRXB(Value_P A, Token & LO, Token & y,
                           Value_P X, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rk_chunk_A = A->get_rank();
Rank rk_chunk_B = B->get_rank();

   y123_to_AB(y.get_apl_val(), rk_chunk_A, rk_chunk_B);

Shape sh_X(X, Workspace::get_CT(), Workspace::get_IO());
   return do_ALyXB(A, rk_chunk_A, LO.get_function(), &sh_X, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::do_ALyXB(Value_P A, Rank rk_chunk_A, Function *  LO,
                         const Shape * axes, Value_P B, Rank rk_chunk_B)
{
   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_frame = A->get_rank() - rk_chunk_A;   // rk_A_frame is y8
Rank rk_B_frame = B->get_rank() - rk_chunk_B;   // rk_B_frame is y9

   // if both high-ranks are 0, then return A LO B.
   //
   if (rk_A_frame == 0 && rk_B_frame == 0)   return LO->eval_AB(A, B);

   // split shapes of A1 and B1 into high (frame) and low (chunk) shapes.
   // Even though A and B have the same shape, rk_A_frame and rk_B_frame
   // could be different, leading to different split shapes for A and B
   //
bool repeat_A = false;
bool repeat_B = false;
Shape sh_A_frame = A->get_shape().high_shape(A->get_rank() - rk_chunk_A);
const Shape sh_B_frame = B->get_shape().high_shape(B->get_rank() - rk_chunk_B);
Shape shape_Z;
   if (rk_A_frame == 0)   // "conform" A to B
      {
        shape_Z = sh_B_frame;
        repeat_A = true;
      }
   else if (rk_B_frame == 0)   // "conform" B to A
      {
        shape_Z = sh_A_frame;
        repeat_B = true;
      }
   else
      {
        if (rk_A_frame != rk_B_frame)   RANK_ERROR;
        if (sh_A_frame != sh_B_frame)   LENGTH_ERROR;
        shape_Z = sh_B_frame;
      }

Value_P Z(shape_Z, LOC);
EOC_arg arg(Z, B, A);
RANK & _arg = arg.u.u_RANK;

   _arg.get_sh_chunk_A() = A->get_shape().low_shape(rk_chunk_A);
   _arg.get_sh_chunk_B() = B->get_shape().low_shape(rk_chunk_B);
   _arg.get_sh_frame() = B->get_shape().high_shape(B->get_rank() - rk_chunk_B);

   // a trick to avoid conforming A to B or B to A. If A or B
   // needs to be conformed, then we set the corresponding repeat_A or 
   // repeat_B to true and copy the same A or B again and again
   //
   _arg.repeat_A = repeat_A;
   _arg.repeat_B = repeat_B;

   if (_arg.repeat_A)   // "conform" A to B
      {
         rk_A_frame = rk_B_frame;
         sh_A_frame = _arg.get_sh_frame();
      }
   else if (_arg.repeat_B)   // "conform" B to A
      {
         rk_B_frame = rk_A_frame;
         _arg.get_sh_frame() = sh_A_frame;
      }
 
   // at this point sh_A_frame == sh_B_frame, so we can use either of them
   // to compute ec_chunk_A
   //
   _arg.ec_frame = _arg.get_sh_frame().get_volume();
   if (!_arg.ec_frame)   _arg.ec_frame = 1;
   _arg.ec_chunk_A = _arg.get_sh_chunk_A().get_volume();
   _arg.ec_chunk_B = _arg.get_sh_chunk_B().get_volume();

   _arg.cA = &A->get_ravel(0);
   _arg.LO = LO;
   _arg.axes_valid = (axes != 0);
   if (axes)   _arg.get_axes() = *axes;
   _arg.cB = &B->get_ravel(0);
   _arg.h = -1;

   return finish_ALyXB(arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::finish_ALyXB(EOC_arg & arg, bool first)
{
RANK & _arg = arg.u.u_RANK;

   while (++_arg.h < _arg.ec_frame)
      {
        Value_P AA(_arg.get_sh_chunk_A(), LOC);
        Value_P BB(_arg.get_sh_chunk_B(), LOC);

        loop(l, _arg.ec_chunk_A)  AA->get_ravel(l).init(*_arg.cA++,AA.getref());
        loop(l, _arg.ec_chunk_B)  BB->get_ravel(l).init(*_arg.cB++,BB.getref());

        if (_arg.repeat_A)   _arg.cA = &arg.A->get_ravel(0);
        if (_arg.repeat_B)   _arg.cB = &arg.B->get_ravel(0);

        AA->check_value(LOC);
        BB->check_value(LOC);

        Token result = _arg.LO->eval_AB(AA, BB);
        if (result.get_tag() == TOK_SI_PUSHED)
           {
             // LO was a user defined function or ⍎
             //
             if (first)   // first call
                Workspace::SI_top()->add_eoc_handler(eoc_RANK, arg, LOC);
             else           // subsequent call
                Workspace::SI_top()->move_eoc_handler(eoc_RANK, &arg, LOC);

             return result;   // continue in user defined function...
           }

        if (result.get_tag() == TOK_ERROR)   return result;

        if (result.get_Class() == TC_VALUE)
           {
             Value_P ZZ = result.get_apl_val();
             Cell * cZ = arg.Z->is_empty() ? &arg.Z->get_ravel(0)
                                           : arg.Z->next_ravel();
             if (ZZ->is_scalar())   cZ->init(ZZ->get_ravel(0), arg.Z.getref());
             else                   new (cZ) PointerCell(ZZ, arg.Z.getref());

             continue;
          }

        Q1(result);   FIXME;
      }

   arg.Z->check_value(LOC);
   if (!_arg.axes_valid)   return Bif_F12_PICK::disclose(arg.Z, true);

   return Bif_F12_PICK::disclose_with_axis(_arg.get_axes(), arg.Z, true);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_RANK::eoc_RANK(Token & token)
{
EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
RANK & _arg = arg->u.u_RANK;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
Value_P ZZ = token.get_apl_val();
   if (ZZ->is_scalar())
      arg->Z->next_ravel()->init(ZZ->get_ravel(0), arg->Z.getref());
   else
      new (arg->Z->next_ravel())   PointerCell(ZZ, arg->Z.getref());

   if (_arg.h < (_arg.ec_frame - 1))   Workspace::pop_SI(LOC);

   if (!arg->A)   copy_1(token, finish_LyXB (*arg, false), LOC);
   else           copy_1(token, finish_ALyXB(*arg, false), LOC);

   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(token);

   return false;   // stop it
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::y123_to_B(Value_P y123, Rank & rank_B)
{
   // y123_to_AB() splits the ranks of A and B into a (higher-dimensions)
   // "frame" and a (lower-dimensions) "chunk" as specified by y123.

   // 1. on entry rank_B is the rank of B.
   //
   //    Remember the rank of B to limit rank_B
   //    if values in y123 should exceed them.
   //
const Rank rk_B = rank_B;

   if (!y123)                   VALUE_ERROR;
   if ( y123->get_rank() > 1)   DOMAIN_ERROR;

   // 2. the number of elements in y determine how rank_B shall be computed:
   //
   //                    -- monadic f⍤ --       -- dyadic f⍤ --
   //          	        rank_A     rank_B       rank_A   rank_B
   // ---------------------------------------------------------
   // y        :        N/A        y            y        y
   // yA yB    :        N/A        yB           yA       yB
   // yM yA yB :        N/A        yM           yA       yB
   // ---------------------------------------------------------

   switch(y123->element_count())
      {
        case 1: rank_B = y123->get_ravel(0).get_near_int();   break;

        case 2:          y123->get_ravel(0).get_near_int();
                rank_B = y123->get_ravel(1).get_near_int();   break;

        case 3: rank_B = y123->get_ravel(0).get_near_int();
                         y123->get_ravel(1).get_near_int();
                         y123->get_ravel(2).get_near_int();   break;

             default: LENGTH_ERROR;
      }

   // 3. adjust rank_B if they exceed its initial value or
   // if it is negative
   //
   if (rank_B > rk_B)   rank_B = rk_B;
   if (rank_B < 0)      rank_B += rk_B;
   if (rank_B < 0)      rank_B = 0;
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::y123_to_AB(Value_P y123, Rank & rank_A, Rank & rank_B)
{
   // y123_to_AB() splits the ranks of A and B into a (higher-dimensions)
   // "frame" and a (lower-dimensions) "chunk" as specified by y123.

   // 1. on entry rank_A and rank_B are the ranks of A and B.
   //
   //    Remember the ranks of A and B to limit rank_A and rank_B
   //    if values in y123 should exceed them.
   //
const Rank rk_A = rank_A;
const Rank rk_B = rank_B;

   if (!y123)                   VALUE_ERROR;
   if ( y123->get_rank() > 1)   DOMAIN_ERROR;

   // 2. the number of elements in y determine how rank_A and rank_B
   // shall be computed:
   //
   //                    -- monadic f⍤ --       -- dyadic f⍤ --
   //          	        rank_A     rank_B       rank_A   rank_B
   // ---------------------------------------------------------
   // y        :        N/A        y            y        y
   // yA yB    :        N/A        yB           yA       yB
   // yM yA yB :        N/A        yM           yA       yB
   // ---------------------------------------------------------

   switch(y123->element_count())
      {
        case 1:  rank_A = y123->get_ravel(0).get_near_int();
                 rank_B = rank_A;                            break;

        case 2:  rank_A = y123->get_ravel(0).get_near_int();
                 rank_B = y123->get_ravel(1).get_near_int();  break;

        case 3:           y123->get_ravel(0).get_near_int();
                 rank_A = y123->get_ravel(1).get_near_int();
                 rank_B = y123->get_ravel(2).get_near_int();  break;

        default: LENGTH_ERROR;
      }

   // 3. adjust rank_A and rank_B if they exceed their initial value or
   // if they are negative
   //
   if (rank_A > rk_A)   rank_A = rk_A;
   if (rank_A < 0)      rank_A += rk_A;
   if (rank_A < 0)      rank_A = 0;

   if (rank_B > rk_B)   rank_B = rk_B;
   if (rank_B < 0)      rank_B += rk_B;
   if (rank_B < 0)      rank_B = 0;
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::split_y123_B(Value_P y123_B, Value_P & y123, Value_P & B)
{
   // The ISO standard and NARS define the reduction pattern for the RANK
   // operator ⍤ as:
   //
   // Z ← A f ⍤ y B		(ISO)
   // Z ←   f ⍤ y B		(ISO)
   // Z ← A f ⍤ [X] y B		(NARS)
   // Z ←   f ⍤ [X] y B		(NARS)
   //
   // GNU APL may bind y to B at tokenization time if y and B are constants
   // This function tries to "unbind" its argument y123_B into the original
   // components y123 (= y in the standard) and B. The tokenization time
   // binding is shown as y123:B
   //
   //    Usage               y123   : B        Result:   j123       B
   //-------------------------------------------------------------------------
   // 1.   f ⍤ (y123):B...   nested   any                y123       B
   // 2.  (f ⍤ y123:⍬)       simple   empty              y123       -
   // 3.   f ⍤ y123:(B)      simple   nested skalar      y123       B
   // 4a.  f ⍤ y123:B...     simple   any                y123       B...
   //

   // y123_B shall be a skalar or vector
   //
   if (y123_B->get_rank() > 1)   RANK_ERROR;

const ShapeItem length = y123_B->element_count();
   if (length == 0)   LENGTH_ERROR;

   // check for case 1 (the only one with nested first element)
   //
   if (y123_B->get_ravel(0).is_pointer_cell())   // (y123)
      {
         y123 = y123_B->get_ravel(0).get_pointer_value();
         if (length == 1)        // empty B
            {
            }
         else if (length == 2)   // skalar B
            {
              const Cell & B0 = y123_B->get_ravel(1);
              if (B0.is_pointer_cell())   // (B)
                 {
                   B = B0.get_pointer_value();
                 }
              else
                 {
                   B = Value_P(LOC);
                   B->next_ravel()->init(B0, B.getref());
                 }
            }
         else                    // vector B
            {
              B = Value_P(length - 1, LOC);
              loop(l, length - 1)
                  B->next_ravel()->init(y123_B->get_ravel(l + 1), B.getref());
            }
         return;
      }

   // case 1. ruled out, so the first 1, 2, or 3 cells are j123.
   // see how many (at most)
   //
int y123_len = 0;
   loop(yy, 3)
      {
        if (yy >= length)   break;
        if (y123_B->get_ravel(yy).is_near_int())   ++y123_len;
        else                                          break;
      }
   if (y123_len == 0)   LENGTH_ERROR;   // at least y1 is needed

   // cases 2.-4. start with integers of length 1, 2, or 3
   //
   if (length == y123_len)   // case 2: y123:⍬
      {
        y123 = y123_B;
        return;
      }

   if (length == (y123_len + 1) &&
       y123_B->get_ravel(y123_len).is_pointer_cell())   // case 3. y123:⊂B
      {
        y123 = Value_P(y123_len, LOC);
        loop(yy, y123_len)
            y123->next_ravel()->init(y123_B->get_ravel(yy), y123.getref());
        B = y123_B->get_ravel(y123_len).get_pointer_value();
        return;
      }

   // case 4: y123:B...
   //
   y123 = Value_P(y123_len, LOC);
   loop(yy, y123_len)
       y123->next_ravel()->init(y123_B->get_ravel(yy), y123.getref());

   B = Value_P(length - y123_len, LOC);
   loop(bb, (length - y123_len))
       B->next_ravel()->init(y123_B->get_ravel(bb + y123_len), B.getref());
}
//-----------------------------------------------------------------------------
