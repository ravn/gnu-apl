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

#include <stdlib.h>
#include "../config.h"   // for HAVE_LIBC
#include "ComplexCell.hh"
#include "Quad_RVAL.hh"

Quad_RVAL  Quad_RVAL::_fun;
Quad_RVAL * Quad_RVAL::fun = &Quad_RVAL::_fun;

// ⎕RVAL depends on glibc, so we use it only in development mode

#if HAVE_LIBC

//-----------------------------------------------------------------------------
Quad_RVAL::Quad_RVAL()
   : QuadFunction(TOK_Quad_RVAL),
     N(8)
{
   memset(state, 0, sizeof(state));
   initstate_r(1, state, N, buf);

   while (desired_shape.get_rank() < MAX_RANK)
         desired_shape.add_shape_item(1);

   desired_ranks.push_back(0);
   desired_types.push_back(1);
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::do_eval_B(const Value & B)
{
   if (B.get_rank() != 1)   RANK_ERROR;
const ShapeItem ec_B = B.element_count();
   if (ec_B > 3)   LENGTH_ERROR;

   // save properties values so that we can restore them
   //
vector<int> old_desired_ranks = desired_ranks;
Shape old_desired_shape = desired_shape;
vector<int> old_desired_types = desired_types;
bool need_restore = false;

   try {
         need_restore = true;   // something was cahnged
         if (ec_B >= 1)
            do_eval_AB(1, B.get_ravel(1).get_pointer_value().getref());
         if (ec_B >= 2)
            do_eval_AB(2, B.get_ravel(2).get_pointer_value().getref());
         if (ec_B >= 3)
            do_eval_AB(3, B.get_ravel(3).get_pointer_value().getref());
       }
    catch (...)
      {
        desired_ranks = old_desired_ranks;
        desired_shape = old_desired_shape;
        desired_types = old_desired_types;
        throw;
      }

const Rank rank = choose_integer(desired_ranks);
Shape shape;


   for (Rank r = MAX_RANK - rank; r < MAX_RANK; ++r)
       {
         vector<int> vsh_r;   vsh_r.push_back(desired_shape.get_shape_item(r));
         const int sh_r = choose_integer(vsh_r);
         shape.add_shape_item(sh_r);
       }

Value_P Z(shape, LOC);

const ShapeItem ec = Z->element_count();

   loop(z, ec)
      {
         const int type_z = choose_integer(desired_types);
         Cell * cZ = Z->next_ravel();
         switch(type_z)
            {
               case 0:   random_character(cZ);               continue;
               case 1:   random_integer(cZ);                 continue;
               case 2:   random_float(cZ);                   continue;
               case 3:   random_complex(cZ);                 continue;
               case 4:   random_nested(cZ, Z.getref(), B);   continue;
               default:  FIXME;
            }
      }

   if (need_restore)
      {
        desired_ranks = old_desired_ranks;
        desired_shape = old_desired_shape;
        desired_types = old_desired_types;
      }

   if (ec == 0)   new (&Z->get_ravel(0))   IntCell(0);

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Token
Quad_RVAL::eval_AB(Value_P A, Value_P B)
{
   if (!A->is_scalar())       RANK_ERROR;
   if (!A->is_int_scalar())   DOMAIN_ERROR;

Value_P Z = do_eval_AB(A->get_ravel(0).get_int_value(), B.getref());
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::do_eval_AB(int A, const Value & B)
{
const int function = A;

   switch(function)
      {
        case 0: return generator_state(B);
        case 1: return result_rank(B);
        case 2: return result_shape(B);
        case 3: return result_type(B);
      }

   MORE_ERROR() << "Bad function number A in A ⎕RVAL B";
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::generator_state(const Value & B)
{
   // expect an empty, 8, 16, 32, 64, 128, or 256 byte integer vector
   if (B.get_rank() != 1)   RANK_ERROR;

const int new_N = B.element_count();
   if (new_N !=   0 && new_N !=   8 && new_N !=  32 &&
       new_N !=  64 && new_N != 128 && new_N != 256)   DOMAIN_ERROR;

   if (new_N)   // set generator state
      {
        // make sure that all values are bytes
        //
        loop(b, N)
            {
              const APL_Integer byte = B.get_ravel(b).get_int_value();
              if ((byte < -256) || (byte >  255))
                 {
                   MORE_ERROR() << "Bad right argument B in 0 ⎕RVAL B,"
                                   "expecting bytes (integers -256...255)";
                   DOMAIN_ERROR;
                 }
            }
        N = new_N;
        loop(b, N)
            {
               state[b] = B.get_ravel(b).get_int_value();
            }
         setstate_r(state, buf);
      }

   // always return the current state
   //
Value_P Z(N, LOC);
   loop(n, N)   new (Z->next_ravel())   IntCell(state[n] & 0xFF);
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::result_rank(const Value & B)
{
   // B is a single rank or a distribution of ranks 0, 1, ...
   if (B.get_rank() > 1)   RANK_ERROR;

   if (B.is_scalar())   // single rank (fixed or equal distribution)
      {
        ShapeItem rk = B.get_ravel(0).get_int_value();

        if ((rk < -MAX_RANK) || (rk > MAX_RANK))
           {
             MORE_ERROR() << "a scalar right argument B of 1 ⎕RVAL B should "
                             "be an integer from ¯" << MAX_RANK << " to "
                          << MAX_RANK;
             DOMAIN_ERROR;
           }

         desired_ranks.clear();
         desired_ranks.push_back(rk);
      }
   else if (B.element_count())   // distribution of ranks
      {
        vector<int>new_ranks;
        loop(b, B.element_count())
            {
              const int rank_b = B.get_ravel(b).get_int_value();
              if (rank_b < 0)
                 {
                   MORE_ERROR() << "a vector right argument B of 1 ⎕RVAL B "
                              "should be a distribution (integers ≥ 0)";
                   DOMAIN_ERROR;
                 }
              new_ranks.push_back(rank_b);
            }

        desired_ranks = new_ranks;
      }

   // always return the current rank
   //
Value_P Z(desired_ranks.size(), LOC);
   loop(z, desired_ranks.size())
       new (Z->next_ravel())   IntCell(desired_ranks[z]);
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::result_shape(const Value & B)
{
   if (!B.is_vector())   RANK_ERROR;

const ShapeItem len_B = B.element_count();
   if (len_B > MAX_RANK)   LENGTH_ERROR;   // to many shape items

   if (len_B)   // set the current shape
      {
        Shape new_shape;

        // fill leading dimensions with 1
        while (new_shape.get_rank() < MAX_RANK - len_B)
              new_shape.add_shape_item(1);

        // fill lower dimensions with B
         loop(b, len_B)
             new_shape.add_shape_item(B.get_ravel(b).get_int_value());

        desired_shape = new_shape;
      }

   // always return the current shape
   //
Value_P Z(MAX_RANK, LOC);
   loop(r, MAX_RANK)
      new (Z->next_ravel())   IntCell(desired_shape.get_shape_item(r));
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::result_type(const Value & B)
{
   // B is a distribution of cell types char (0), int (1), real (2),
   // complex (3), // or nested (4).

   if (!B.is_vector())          RANK_ERROR;
   if (B.element_count() > 5)   LENGTH_ERROR;

   if (B.element_count())   // distribution of depths
      {
        vector<int>new_types;
        loop(b, B.element_count())
            {
              const int type_b = B.get_ravel(b).get_int_value();
              if (type_b < 0)
                 {
                   MORE_ERROR() << "the right argument B of 3 ⎕RVAL B "
                   "should be a distribution (vector of integers ≥ 0)";
                   DOMAIN_ERROR;
                 }
              new_types.push_back(type_b);
            }

        while (new_types.size() < 5)    new_types.push_back(0);   // make 5=⍴Z
        desired_types = new_types;
      }

   // always return the current rank
   //
Value_P Z(desired_types.size(), LOC);
   loop(z, desired_types.size())
       new (Z->next_ravel())   IntCell(desired_types[z]);
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
int
Quad_RVAL::choose_integer(const vector<int> & dist)
{
const int n = dist.size();
   Assert(n > 0);

   if (n == 1)   // fixed value or equal distribution 0, 1, ... n
      {
        const int desired = dist[0];
        if (desired >= 0)   return desired;   // fixed value
        int rand = 0;
        const int err = random_r(buf, &rand);
        Assert(err == 0);
        return rand % (1 - desired);   // = 0... desired
      }

   // dist is a distribution
   //
int sum = 0;
   for (size_t d = 0; d < dist.size(); ++d)   sum += dist[d];

        int rand = 0;
        const int err = random_r(buf, &rand);
        Assert(err == 0);
        rand %= sum;

   sum = 0;
   for (size_t d = 0; d < dist.size(); ++d)
       {
          sum += dist[d];
          if (rand < sum)   return d;
       }

   FIXME;   // not reached
}
//-----------------------------------------------------------------------------
void
Quad_RVAL::random_character(Cell * cell)
{
int32_t rnd = 0;
const int err = random_r(buf, &rnd);
   Assert(err == 0);
   new (cell) CharCell(Unicode(rnd & 0x3FFF));
}
//-----------------------------------------------------------------------------
void
Quad_RVAL::random_integer(Cell * cell)
{
int32_t rnd1= 0;   if (random_r(buf, &rnd1))   FIXME;
int32_t rnd2= 0;   if (random_r(buf, &rnd2))   FIXME;
const int64_t rnd = uint32_t(rnd1) | uint64_t(uint32_t(rnd2)) << 32;
   new (cell) IntCell(rnd);
}
//-----------------------------------------------------------------------------
void
Quad_RVAL::random_float(Cell * cell)
{
int32_t rnd1= 0;   if (random_r(buf, &rnd1))   FIXME;
int32_t rnd2= 0;   if (random_r(buf, &rnd2))   FIXME;

union { int64_t i; double f; } u;
   u.i = uint32_t(rnd1) | uint64_t(uint32_t(rnd2)) << 32;
   new (cell) FloatCell(u.f);
}
//-----------------------------------------------------------------------------
void
Quad_RVAL::random_complex(Cell * cell)
{
int32_t rnd1= 0;   if (random_r(buf, &rnd1))   FIXME;
int32_t rnd2= 0;   if (random_r(buf, &rnd2))   FIXME;
int32_t rnd3= 0;   if (random_r(buf, &rnd3))   FIXME;
int32_t rnd4= 0;   if (random_r(buf, &rnd4))   FIXME;

union { int64_t i; double f; } u1, u2;
   u1.i = uint32_t(rnd1) | uint64_t(uint32_t(rnd2)) << 32;
   u2.i = uint32_t(rnd3) | uint64_t(uint32_t(rnd4)) << 32;
   new (cell) ComplexCell(u1.f, u2.f);
}
//-----------------------------------------------------------------------------
void
Quad_RVAL::random_nested(Cell * cell, Value & cell_owner, const Value & B)
{
Value_P Zsub = do_eval_B(B);
   new (cell) PointerCell(Zsub.get(), cell_owner);
}
//-----------------------------------------------------------------------------

#else    // not HAVE_LIBC

//-----------------------------------------------------------------------------
Quad_RVAL::Quad_RVAL()
   : QuadFunction(TOK_Quad_RVAL),
     N(8)
{
}
//-----------------------------------------------------------------------------
Value_P
Quad_RVAL::do_eval_B(const Value & B)
{
    MORE_ERROR() <<
"⎕RVAL is only available on platforms that have glibc.\n"
"Your platform is lacking initstate_r() (and probably others).";

   SYNTAX_ERROR;
   return Value_P();
}
//-----------------------------------------------------------------------------
Token
Quad_RVAL::eval_AB(Value_P A, Value_P B)
{
    MORE_ERROR() <<
"⎕RVAL is only available on platforms that have glibc.\n"
"Your platform is lacking initstate_r() (and probably others).";

   SYNTAX_ERROR;
   return Token();
}
//-----------------------------------------------------------------------------
#endif   // HAVE_LIBC
