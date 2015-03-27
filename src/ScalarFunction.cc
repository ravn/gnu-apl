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

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "Parallel.hh"
#include "PointerCell.hh"
#include "ScalarFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

// scalar function instances
//
Bif_F2_LESS     Bif_F2_LESS    ::_fun;                // <
Bif_F2_EQUAL    Bif_F2_EQUAL   ::_fun;                // =
Bif_F2_FIND     Bif_F2_FIND    ::_fun;                // ⋸ (almost scalar)
Bif_F2_GREATER  Bif_F2_GREATER ::_fun;                // >
Bif_F2_AND      Bif_F2_AND     ::_fun;                // ∧
Bif_F2_OR       Bif_F2_OR      ::_fun;                // ∨
Bif_F2_LEQ      Bif_F2_LEQ     ::_fun;                // ≤
Bif_F2_MEQ      Bif_F2_MEQ     ::_fun;                // ≥
Bif_F2_UNEQ     Bif_F2_UNEQ    ::_fun;                // ≠
Bif_F2_NOR      Bif_F2_NOR     ::_fun;                // ⍱
Bif_F2_NAND     Bif_F2_NAND    ::_fun;                // ⍲
Bif_F12_PLUS    Bif_F12_PLUS   ::_fun;                // +
Bif_F12_POWER   Bif_F12_POWER  ::_fun;                // ⋆
Bif_F12_BINOM   Bif_F12_BINOM  ::_fun;                // !
Bif_F12_MINUS   Bif_F12_MINUS  ::_fun;                // -
Bif_F12_ROLL    Bif_F12_ROLL   ::_fun;                // ? (monadic is scalar)
Bif_F12_TIMES   Bif_F12_TIMES  ::_fun;                // ×
Bif_F12_DIVIDE  Bif_F12_DIVIDE ::_fun;                // ÷
Bif_F12_CIRCLE  Bif_F12_CIRCLE ::_fun(false);         // ○
Bif_F12_CIRCLE  Bif_F12_CIRCLE ::_fun_inverse(true);  // A inverted
Bif_F12_RND_UP  Bif_F12_RND_UP ::_fun;                // ⌈
Bif_F12_RND_DN  Bif_F12_RND_DN ::_fun;                // ⌊
Bif_F12_STILE   Bif_F12_STILE  ::_fun;                // ∣
Bif_F12_LOGA    Bif_F12_LOGA   ::_fun;                // ⍟
Bif_F12_WITHOUT Bif_F12_WITHOUT::_fun;                // ∼ (monadic is scalar)

// scalar function pointers
//
Bif_F2_LESS     * Bif_F2_LESS    ::fun         = &Bif_F2_LESS    ::_fun;
Bif_F2_EQUAL    * Bif_F2_EQUAL   ::fun         = &Bif_F2_EQUAL   ::_fun;
Bif_F2_FIND     * Bif_F2_FIND    ::fun         = &Bif_F2_FIND    ::_fun;
Bif_F2_GREATER  * Bif_F2_GREATER ::fun         = &Bif_F2_GREATER ::_fun;
Bif_F2_AND      * Bif_F2_AND     ::fun         = &Bif_F2_AND     ::_fun;
Bif_F2_OR       * Bif_F2_OR      ::fun         = &Bif_F2_OR      ::_fun;
Bif_F2_LEQ      * Bif_F2_LEQ     ::fun         = &Bif_F2_LEQ     ::_fun;
Bif_F2_MEQ      * Bif_F2_MEQ     ::fun         = &Bif_F2_MEQ     ::_fun;
Bif_F2_UNEQ     * Bif_F2_UNEQ    ::fun         = &Bif_F2_UNEQ    ::_fun;
Bif_F2_NOR      * Bif_F2_NOR     ::fun         = &Bif_F2_NOR     ::_fun;
Bif_F2_NAND     * Bif_F2_NAND    ::fun         = &Bif_F2_NAND    ::_fun;
Bif_F12_PLUS    * Bif_F12_PLUS   ::fun         = &Bif_F12_PLUS   ::_fun;
Bif_F12_POWER   * Bif_F12_POWER  ::fun         = &Bif_F12_POWER  ::_fun;
Bif_F12_BINOM   * Bif_F12_BINOM  ::fun         = &Bif_F12_BINOM  ::_fun;
Bif_F12_MINUS   * Bif_F12_MINUS  ::fun         = &Bif_F12_MINUS  ::_fun;
Bif_F12_ROLL    * Bif_F12_ROLL   ::fun         = &Bif_F12_ROLL   ::_fun;
Bif_F12_TIMES   * Bif_F12_TIMES  ::fun         = &Bif_F12_TIMES  ::_fun;
Bif_F12_DIVIDE  * Bif_F12_DIVIDE ::fun         = &Bif_F12_DIVIDE ::_fun;
Bif_F12_CIRCLE  * Bif_F12_CIRCLE ::fun         = &Bif_F12_CIRCLE ::_fun;
Bif_F12_CIRCLE  * Bif_F12_CIRCLE ::fun_inverse = &Bif_F12_CIRCLE ::_fun_inverse;
Bif_F12_RND_UP  * Bif_F12_RND_UP ::fun         = &Bif_F12_RND_UP ::_fun;
Bif_F12_RND_DN  * Bif_F12_RND_DN ::fun         = &Bif_F12_RND_DN ::_fun;
Bif_F12_STILE   * Bif_F12_STILE  ::fun         = &Bif_F12_STILE  ::_fun;
Bif_F12_LOGA    * Bif_F12_LOGA   ::fun         = &Bif_F12_LOGA   ::_fun;
Bif_F12_WITHOUT * Bif_F12_WITHOUT::fun         = &Bif_F12_WITHOUT::_fun;


/// one monadic scalar job
struct PJob_scalar_B
{
   /// default constructor
   PJob_scalar_B()
   : value_Z(*(Value *)0)
   {}

   /// assign \b other to \b this
   void operator =(const PJob_scalar_B & other)
      { memcpy(this, &other, sizeof(*this)); }

   /// constructor
   PJob_scalar_B(Value & Z, const Value & B)
   : value_Z(Z),
     len_Z(Z.nz_element_count()),
     cZ(&Z.get_ravel(0)),
     cB(&B.get_ravel(0)),
     error(E_NO_ERROR)
   {}

   /// the value being computed
   Value & value_Z;

   /// the length of the result
   ShapeItem len_Z;

   /// ravel of the result
   Cell * cZ;

   /// ravel of the right argument
   const Cell * cB;

   /// an error detected during computation of, eg. fun1 or fun2
   ErrorCode error;

   /// the APL function being computed
   PrimitiveFunction * fun;   // not initialized by constructor!

   /// the monadic cell function to be computed
   prim_f1 fun1;   // not initialized by constructor!

   /// return B[z]
   const Cell & B_at(ShapeItem z) const
      { return cB[z]; }

   /// return Z[z]
   Cell & Z_at(ShapeItem z) const
      { return cZ[z]; }
};

/// all monadic scalar jobs
static Parallel_job_list<PJob_scalar_B> joblist_B;

/// one dyadic scalar job
struct PJob_scalar_AB
{
   /// default constructor
   PJob_scalar_AB()
   : value_Z(*(Value *)0)
   {}

   /// assign \b other to \b this
   void operator =(const PJob_scalar_AB & other)
      { memcpy(this, &other, sizeof(*this)); }

   /// constructor
   PJob_scalar_AB(Value & Z, const Cell * _cA, int iA, const Cell * _cB, int iB)
   : value_Z(Z),
     len_Z(Z.nz_element_count()),
     cZ(&Z.get_ravel(0)),
     cA(_cA),
     inc_A(iA),
     cB(_cB),
     inc_B(iB),
     error(E_NO_ERROR)
   {}

   /// A value (e.g parallel ~Value())
   Value & value_Z;

   /// the length of the result
   ShapeItem len_Z;

   /// ravel of the result
   Cell * cZ;

   /// ravel of the left argument
   const Cell * cA;

   /// 0 (for scalar A) or 1
   int inc_A;

   /// ravel of the right argument
   const Cell * cB;

   /// 0 (for scalar B) or 1
   int inc_B;

   /// an error detected during computation of, eg. fun1 or fun2
   ErrorCode error;

   /// the APL function being computed
   PrimitiveFunction * fun;   // not initialized by constructor!

   /// the dyadic cell function to be computed
   prim_f2 fun2;   // not initialized by constructor!

   /// return A[z]
   const Cell & A_at(ShapeItem z) const
      { return cA[z * inc_A]; }

   /// return B[z]
   const Cell & B_at(ShapeItem z) const
      { return cB[z * inc_B]; }

   /// return Z[z]
   Cell & Z_at(ShapeItem z) const
      { return cZ[z]; }
};

/// all dyadic scalar jobs
static Parallel_job_list<PJob_scalar_AB> joblist_AB;

//-----------------------------------------------------------------------------
Token
ScalarFunction::eval_scalar_B(Value_P B, prim_f1 fun)
{
PERFORMANCE_START(start_1)

const ShapeItem len_Z = B->element_count();
   if (len_Z == 0)   return eval_fill_B(B);

Value_P Z(B->get_shape(), LOC);

   // create a worklist with one item that computes Z. If nested values are
   // detected when computing Z then jobs for them are added to the worklist.
   // Finished jobs are NOT removed from the list to avoid unnecessary
   // copying of worklist items.
   //
   {
     PJob_scalar_B j(Z.getref(), B.getref());
     joblist_B.start(j, LOC);
   }

   for (PJob_scalar_B * job = joblist_B.next_job();
        job; job = joblist_B.next_job())
      {
        if (  Parallel::run_parallel
           && Thread_context::get_active_core_count() > 1
           && job->len_Z > get_monadic_threshold())
           {
            job->fun = this;
            job->error = E_NO_ERROR;
            job->fun1 = fun;
            Thread_context::do_work = PF_eval_scalar_B;
            Thread_context::M_fork("eval_scalar_B");   // start pool
            PF_eval_scalar_B(Thread_context::get_master());
            Thread_context::M_join();
            if (job->error != E_NO_ERROR)
               {
                 joblist_B.cancel_jobs();
                 throw_apl_error(job->error, LOC);
              }
           }
        else   // sequential
           {
             loop(z, job->len_Z)
                {
                  const Cell & cell_B = job->B_at(z);
                  Cell & cell_Z       = job->Z_at(z);

                  if (cell_B.is_pointer_cell())
                     {
                       POOL_LOCK(joblist_B.parallel_jobs_lock,
                          Value_P B1 = cell_B.get_pointer_value();
                          Value_P Z1(B1->get_shape(), LOC);
                          new (&cell_Z) PointerCell(Z1, job->value_Z);

                          PJob_scalar_B j1(Z1.getref(), B1.getref());
                          joblist_B.add_job(j1))
                     }
                  else
                     {
PERFORMANCE_START(start_2)

                       const ErrorCode ec = (cell_B.*fun)(&cell_Z);
                       if (ec != E_NO_ERROR)
                          {
                            joblist_B.cancel_jobs();
                            throw_apl_error(ec, LOC);
                          }

CELL_PERFORMANCE_END(get_statistics_B(), start_2, z)
                     }
                }
           }
      }

   Z->check_value(LOC);

PERFORMANCE_END(fs_SCALAR_B, start_1, Z->nz_element_count());

   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
ScalarFunction::PF_eval_scalar_B(Thread_context & tctx)
{
const int cores = Thread_context::get_active_core_count();
PJob_scalar_B & job = joblist_B.get_current_job();

const ShapeItem slice_len = (job.len_Z + cores - 1) / cores;
ShapeItem z = tctx.get_N() * slice_len;
ShapeItem end_z = z + slice_len;
   if (end_z > job.len_Z)   end_z = job.len_Z;

   for (; z < end_z; ++z)
       {
         const Cell & cell_B = job.B_at(z);
         Cell & cell_Z       = job.Z_at(z);

         if (cell_B.is_pointer_cell())
            {
              // B is nested
              //
              Value_P B1 = cell_B.get_pointer_value();

              const ShapeItem len_Z1 = B1->get_shape().get_volume();
              if (len_Z1 == 0)
                 {
                   POOL_LOCK(joblist_B.parallel_jobs_lock,
                      Value_P Z1= B1->clone(LOC);
                      Z1->to_proto();
                      Z1->check_value(LOC);
                      new (&cell_Z) PointerCell(Z1, job.value_Z))
                 }
              else
                 {
                   POOL_LOCK(joblist_B.parallel_jobs_lock,
                      Value_P B1 = cell_B.get_pointer_value();
                      Value_P Z1(B1->get_shape(), LOC);
                      new (&cell_Z) PointerCell(Z1, job.value_Z);

                      PJob_scalar_B j1(Z1.getref(), B1.getref());
                      joblist_B.add_job(j1))
                 }
            }
         else
            {
                  // B not nested: execute fun
                  //
PERFORMANCE_START(start_2)
                  const ErrorCode ec = (cell_B.*job.fun1)(&cell_Z);
                  if (ec != E_NO_ERROR)   return;

CELL_PERFORMANCE_END(job.fun->get_statistics_B(), start_2, z)
            }
       }
}
//-----------------------------------------------------------------------------
void
ScalarFunction::expand_pointers(Cell * cell_Z, Value & Z_owner,
                                const Cell * cell_A, const Cell * cell_B,
                                prim_f2 fun)
{
   if (cell_A->is_pointer_cell())
      {
        if (cell_B->is_pointer_cell())   // A and B are both pointers
           {
             POOL_LOCK(joblist_AB.parallel_jobs_lock,
                Value_P value_A = cell_A->get_pointer_value();
                Value_P value_B = cell_B->get_pointer_value();
                Token token = eval_scalar_AB(value_A, value_B, fun);
                new (cell_Z) PointerCell(token.get_apl_val(), Z_owner))
           }
        else                             // A is pointer, B is simple
           {
             POOL_LOCK(joblist_AB.parallel_jobs_lock,
                Value_P value_A = cell_A->get_pointer_value();
                Value_P scalar_B(*cell_B, LOC);
             Token token = eval_scalar_AB(value_A, scalar_B, fun);
             new (cell_Z) PointerCell(token.get_apl_val(), Z_owner))
           }
      }
   else                                  // A is simple
      {
        if (cell_B->is_pointer_cell())   // A is simple, B is pointer
           {
             POOL_LOCK(joblist_AB.parallel_jobs_lock,
                Value_P scalar_A(*cell_A, LOC);
                Value_P value_B = cell_B->get_pointer_value();
                Token token = eval_scalar_AB(scalar_A, value_B, fun);
                new (cell_Z) PointerCell(token.get_apl_val(), Z_owner))
           }
   else                                // A and B are both plain
         {
           (cell_B->*fun)(cell_Z, cell_A);
         }
      }
}
//-----------------------------------------------------------------------------
Token
ScalarFunction::eval_scalar_AB(Value_P A, Value_P B, prim_f2 fun)
{
PERFORMANCE_START(start_1)

const int inc_A = A->is_scalar_or_len1_vector() ? 0 : 1;
const int inc_B = B->is_scalar_or_len1_vector() ? 0 : 1;
const Shape * shape_Z = 0;
   if      (A->is_scalar())      shape_Z = &B->get_shape();
   else if (B->is_scalar())      shape_Z = &A->get_shape();
   else if (inc_A == 0)          shape_Z = &B->get_shape();
   else if (inc_B == 0)          shape_Z = &A->get_shape();
   else if (A->same_shape(*B))   shape_Z = &B->get_shape();
   else 
      {
        if (!A->same_rank(*B))   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const ShapeItem len_Z = shape_Z->get_volume();
   if (len_Z == 0)   return eval_fill_AB(A, B);

Value_P Z(*shape_Z, LOC);

   // create a worklist with one item that computes Z. If nested values are
   // detected when computing Z then jobs for them are added to the worklist.
   // Finished jobs are NOT removed from the list to avoid unnecessary
   // copying of worklist items.
   //
   {
     PJob_scalar_AB j(Z.getref(), &A->get_ravel(0), inc_A,
                                  &B->get_ravel(0), inc_B);
     joblist_AB.start(j, LOC);
   }

   for (PJob_scalar_AB * job = joblist_AB.next_job();
        job; job = joblist_AB.next_job())
      {
        if (  Parallel::run_parallel
           && Thread_context::get_active_core_count() > 1
           && job->len_Z > get_dyadic_threshold())
           {
            job->fun = this;
            job->error = E_NO_ERROR;
            job->fun2 = fun;
            Thread_context::do_work = PF_eval_scalar_AB;
            Thread_context::M_fork("eval_scalar_AB");   // start pool
            PF_eval_scalar_AB(Thread_context::get_master());
            Thread_context::M_join();
            if (job->error != E_NO_ERROR)
               {
                 joblist_AB.cancel_jobs();
                 throw_apl_error(job->error, LOC);
               }
           }
        else
           {
             loop(z, job->len_Z)
                {
                  const Cell & cell_A = job->A_at(z);
                  const Cell & cell_B = job->B_at(z);
                  Cell & cell_Z       = job->Z_at(z);

                  if (cell_A.is_pointer_cell())
                     if (cell_B.is_pointer_cell())
                        {
                          // both A and B are nested
                          //
                          Value_P A1 = cell_A.get_pointer_value();
                          Value_P B1 = cell_B.get_pointer_value();
                          const int inc_A1 =
                                    A1->is_scalar_or_len1_vector() ? 0 : 1;
                          const int inc_B1 =
                                    B1->is_scalar_or_len1_vector() ? 0 : 1;
                          const Shape * sh_Z1 = &B1->get_shape();
                          if      (A1->is_scalar())   sh_Z1 = &B1->get_shape();
                          else if (B1->is_scalar())   sh_Z1 = &A1->get_shape();
                          else if (inc_B1 == 0)       sh_Z1 = &A1->get_shape();

                          if (inc_A1 && inc_B1 && !A1->same_shape(*B1))
                             {
                               if (!A1->same_rank(*B1))   RANK_ERROR;
                               else                       LENGTH_ERROR;
                             }

                          const ShapeItem len_Z1 = sh_Z1->get_volume();
                          if (len_Z1 == 0)
                             {
                               joblist_AB.cancel_jobs();
                               return eval_fill_AB(A1, B1);
                             }

                          POOL_LOCK(joblist_AB.parallel_jobs_lock,
                             Value_P Z1(*sh_Z1, LOC);
                             new (&cell_Z) PointerCell(Z1, job->value_Z);

                             PJob_scalar_AB j1(Z1.getref(),
                                               &A1->get_ravel(0), inc_A1,
                                               &B1->get_ravel(0), inc_B1);
                             joblist_AB.add_job(j1))
                        }
                     else
                        {
                          // A is nested, B is not
                          //
                          Value_P A1 = cell_A.get_pointer_value();
                          const int inc_A1 =
                                    A1->is_scalar_or_len1_vector() ? 0 : 1;

                          const ShapeItem len_Z1 = A1->get_shape().get_volume();
                          if (len_Z1 == 0)
                             {
                               Value_P Z1 = A1->clone(LOC);
                               Z1->to_proto();
                               Z1->check_value(LOC);
                               new (&cell_Z) PointerCell(Z1, job->value_Z);
                             }
                          else
                             {
                                POOL_LOCK(joblist_AB.parallel_jobs_lock,
                                  Value_P Z1(A1->get_shape(), LOC);
                                  new (&cell_Z) PointerCell(Z1, job->value_Z);

                                  PJob_scalar_AB j1(Z1.getref(),
                                                    &A1->get_ravel(0), inc_A1,
                                                    &cell_B, 0);
                                  joblist_AB.add_job(j1))
                             }
                        }
                  else
                     if (cell_B.is_pointer_cell())
                        {
                          // A is not nested, B is nested
                          //
                          Value_P B1 = cell_B.get_pointer_value();
                          const int inc_B1 =
                                    B1->is_scalar_or_len1_vector() ? 0 : 1;

                          const ShapeItem len_Z1 = B1->get_shape().get_volume();
                          if (len_Z1 == 0)
                             {
                               Value_P Z1= B1->clone(LOC);
                               Z1->to_proto();
                               Z1->check_value(LOC);
                               new (&cell_Z) PointerCell(Z1, job->value_Z);
                             }
                          else
                             {
                               POOL_LOCK(joblist_AB.parallel_jobs_lock,
                                  Value_P Z1(B1->get_shape(), LOC);
                                  new (&cell_Z) PointerCell(Z1, job->value_Z);

                                  PJob_scalar_AB j1(Z1.getref(),
                                                    &cell_A, 0,
                                                    &B1->get_ravel(0), inc_B1);
                                  joblist_AB.add_job(j1))
                            }
                        }
                     else
                        {
                          // neither A nor B are nested: execute fun
                          //
PERFORMANCE_START(start_2)

                          const ErrorCode ec = (cell_B.*fun)(&cell_Z, &cell_A);
                          if (ec != E_NO_ERROR)
                             {
                               joblist_AB.cancel_jobs();
                               throw_apl_error(ec, LOC);
                             }

CELL_PERFORMANCE_END(get_statistics_AB(), start_2, z)
                        }
                }
           }
      }

   joblist_AB.cancel_jobs();
   Z->set_default(*B.get());
   Z->check_value(LOC);

PERFORMANCE_END(fs_SCALAR_AB, start_1, len_Z)

   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
ScalarFunction::PF_eval_scalar_AB(Thread_context & tctx)
{
const CoreCount cores = Thread_context::get_active_core_count();
PJob_scalar_AB & job = joblist_AB.get_current_job();

const ShapeItem slice_len = (job.len_Z + cores - 1) / cores;
ShapeItem z = tctx.get_N() * slice_len;
ShapeItem end_z = z + slice_len;
   if (end_z > job.len_Z)   end_z = job.len_Z;

   for (; z < end_z; ++z)
       {
             const Cell & cell_A = job.A_at(z);
             const Cell & cell_B = job.B_at(z);
             Cell & cell_Z       = job.Z_at(z);

             if (cell_A.is_pointer_cell())
                if (cell_B.is_pointer_cell())
                   {
                     // both A and B are nested
                     //
                     Value_P A1 = cell_A.get_pointer_value();
                     Value_P B1 = cell_B.get_pointer_value();
                     const int inc_A1 = A1->is_scalar_or_len1_vector() ? 0 : 1;
                     const int inc_B1 = B1->is_scalar_or_len1_vector() ? 0 : 1;
                     const Shape * sh_Z1 = &B1->get_shape();
                     if      (A1->is_scalar())   sh_Z1 = &B1->get_shape();
                     else if (B1->is_scalar())   sh_Z1 = &A1->get_shape();
                     else if (inc_B1 == 0)       sh_Z1 = &A1->get_shape();

                     if (inc_A1 && inc_B1 && !A1->same_shape(*B1))
                        {
                          job.error = A1->same_rank(*B1) ? E_LENGTH_ERROR
                                                         : E_RANK_ERROR;
                          return;
                        }

                     const ShapeItem len_Z1 = sh_Z1->get_volume();
                     if (len_Z1 == 0)
                        {
                          Token result =job.fun->eval_fill_AB(A1, B1);
                          if (result.get_tag() == TOK_ERROR)
                             {
                               job.error = (ErrorCode)(result.get_int_val());
                               return;
                             }
                        }

                     POOL_LOCK(joblist_AB.parallel_jobs_lock,
                        Value_P Z1(*sh_Z1, LOC);
                        new (&cell_Z) PointerCell(Z1, job.value_Z);

                        PJob_scalar_AB j1(Z1.getref(),
                                          &A1->get_ravel(0), inc_A1,
                                          &B1->get_ravel(0), inc_B1);
                        joblist_AB.add_job(j1))
                   }
                else
                   {
                     // A is nested, B is not
                     //
                     Value_P A1 = cell_A.get_pointer_value();
                     const int inc_A1 = A1->is_scalar_or_len1_vector() ? 0 : 1;

                     const ShapeItem len_Z1 = A1->get_shape().get_volume();
                     if (len_Z1 == 0)
                        {
                          Value_P Z1= A1->clone(LOC);
                          Z1->to_proto();
                          Z1->check_value(LOC);
                          new (&cell_Z) PointerCell(Z1, job.value_Z);
                        }
                     else
                        {
                          POOL_LOCK(joblist_AB.parallel_jobs_lock,
                             Value_P Z1(A1->get_shape(), LOC);
                             new (&cell_Z) PointerCell(Z1, job.value_Z);

                             PJob_scalar_AB j1(Z1.getref(),
                                               &A1->get_ravel(0), inc_A1,
                                               &cell_B, 0);
                             joblist_AB.add_job(j1))
                        }
                   }
             else
                if (cell_B.is_pointer_cell())
                   {
                     // A is not nested, B is nested
                     //
                     Value_P B1 = cell_B.get_pointer_value();
                     const int inc_B1 = B1->is_scalar_or_len1_vector() ? 0 : 1;

                     const ShapeItem len_Z1 = B1->get_shape().get_volume();
                     if (len_Z1 == 0)
                        {
                          Value_P Z1= B1->clone(LOC);
                          Z1->to_proto();
                          Z1->check_value(LOC);
                          new (&cell_Z) PointerCell(Z1, job.value_Z);
                        }
                     else
                        {
                          POOL_LOCK(joblist_AB.parallel_jobs_lock,
                             Value_P Z1(B1->get_shape(), LOC);
                             new (&cell_Z) PointerCell(Z1, job.value_Z);

                             PJob_scalar_AB j1(Z1.getref(),
                                               &cell_A, 0,
                                               &B1->get_ravel(0), inc_B1);
                             joblist_AB.add_job(j1))
                       }
                   }
                else
                   {
                     // neither A nor B are nested: execute fun
                     //
PERFORMANCE_START(start_2)

                     const ErrorCode ec = (cell_B.*job.fun2)(&cell_Z, &cell_A);
                     if (ec != E_NO_ERROR)   return;

CELL_PERFORMANCE_END(job.fun->get_statistics_AB(), start_2, z)
                   }
       }
}
//-----------------------------------------------------------------------------
Token
ScalarFunction::eval_fill_AB(Value_P A, Value_P B)
{
   // eval_fill_AB() is called when A or B or both are empty
   //
   if (A->is_scalar_or_len1_vector())   // then B is empty
      {
        Value_P Z = B->clone(LOC);
        Z->to_proto();
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->is_scalar_or_len1_vector())   // then A is empty
      {
        Value_P Z = A->clone(LOC);
        Z->to_proto();
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // both A and B are empty
   //
   Assert(A->same_shape(*B));   // has been checked already
Value_P Z = B->clone(LOC);
   Z->to_proto();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);

//   return Bif_F2_UNEQ::fun.eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
ScalarFunction::eval_fill_B(Value_P B)
{
   // eval_fill_B() is called when a scalar function with empty B is called
   //
Value_P Z = B->clone(LOC);
   Z->to_proto();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
ScalarFunction::eval_scalar_identity_fun(Value_P B, Axis axis, Value_P FI0)
{
   // for scalar functions the result of the identity function for scalar
   // function F is defined as follows (lrm p. 210)
   //
   // Z←SRρB+F/ι0    with SR ↔ ⍴Z
   //
   // The term F/ι0 is passed as argument FI0, so that the above becomes
   //
   // Z←SRρB+FI0
   //
   // Since F is scalar, the ravel elements of B (if any) are 0 and
   // therefore B+FI0 becomes (⍴B)⍴FI0.
   //
   if (!FI0->is_scalar())   Q1(FI0->get_shape())

const Shape shape_Z = B->get_shape().without_axis(axis);

Value_P Z(shape_Z, LOC);

const Cell & proto_B = B->get_ravel(0);
const Cell & cell_FI0 = FI0->get_ravel(0);

   if (proto_B.is_pointer_cell())
      {
        // create a value like ↑B but with all ravel elements like FI0...
        //
        POOL_LOCK(joblist_B.parallel_jobs_lock,
           Value_P sub(proto_B.get_pointer_value()->get_shape(),LOC);
        const ShapeItem len_sub = sub->nz_element_count();
        Cell * csub = &sub->get_ravel(0);
        loop(s, len_sub)   csub++->init(cell_FI0, sub.getref());

        while (Z->more())
           new (Z->next_ravel()) PointerCell(sub->clone(LOC), Z.getref()))
      }
   else
      {
        while (Z->more())   Z->next_ravel()->init(cell_FI0, Z.getref());
        if (Z->is_empty())  Z->get_ravel(0).init(cell_FI0, Z.getref());
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
ScalarFunction::eval_scalar_AXB(Value_P A, Value_P X, Value_P B, prim_f2 fun)
{
PERFORMANCE_START(start_1)

   if (A->is_scalar_or_len1_vector() || B->is_scalar_or_len1_vector() || !X)
      return eval_scalar_AB(A, B, fun);

   if (X->get_rank() > 1)   AXIS_ERROR;

const APL_Integer qio = Workspace::get_IO();
const Rank rank_A = A->get_rank();
const Rank rank_B = B->get_rank();
bool axis_in_X[MAX_RANK];
   loop(r, MAX_RANK)   axis_in_X[r] = false;

const ShapeItem len_X = X->element_count();
   loop(iX, len_X)
       {
         APL_Integer i = X->get_ravel(iX).get_near_int() - qio;
         if (i < 0)                        AXIS_ERROR;   // too small
         if (i >= rank_A && i >= rank_B)   AXIS_ERROR;   // too large
         if (axis_in_X[i])                 AXIS_ERROR;   // twice
         axis_in_X[i] = true;
       }

Value_P Z = (rank_A < rank_B) ? eval_scalar_AXB(A, axis_in_X, B, fun, false)
                              : eval_scalar_AXB(B, axis_in_X, A, fun, true);

PERFORMANCE_END(fs_SCALAR_AB, start_1, Z->nz_element_count())

   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
ScalarFunction::eval_scalar_AXB(Value_P A, bool * axis_in_X,
                                Value_P B, prim_f2 fun, bool reversed)
{
   // A is the value with the smaller rank.
   // B the value with the larger rank.
   //
   // If (reversed) then A and B have changed roles (in order to
   /// make A is the value with the smaller rank).

   // check that A and B agree on the axes in X
   //
   {
     Rank rA = 0;
     loop(rB, B->get_rank())
         {
            if (axis_in_X[rB])
               {
                 // if the axis is in X then the corresponding shape items in
                 // A and B must agree.
                 //
                 if (B->get_shape_item(rB) != A->get_shape_item(rA++))
                    LENGTH_ERROR;
               }
         }
   }

Shape weight_A = A->get_shape().reverse_scan();

Value_P Z(B->get_shape(), LOC);

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it(B->get_shape()); !it.done(); ++it)
       {
         ShapeItem a = 0;
         Rank rA = 0;
         loop(rB, B->get_rank())
             {
               if (axis_in_X[rB])
                  {
                    a += weight_A.get_shape_item(rA++)
                       * it.get_value(rB);
                  }
             }

         const Cell * cA = &A->get_ravel(a);
         if (reversed)
            expand_pointers(Z->next_ravel(), Z.getref(), cB++, cA, fun);
         else
            expand_pointers(Z->next_ravel(), Z.getref(), cA, cB++, fun);
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Token
Bif_F2_FIND::eval_AB(Value_P A, Value_P B)
{
PERFORMANCE_START(start_1)

const APL_Float qct = Workspace::get_CT();
Value_P Z(B->get_shape(), LOC);

const ShapeItem len_Z = Z->element_count();
Shape shape_A;

   if (A->get_rank() > B->get_rank())   // then Z is all zeros.
      {
        loop(z, len_Z)   new (&Z->get_ravel(z))   IntCell(0);
        goto done;
      }

   // Reshape A to match rank B if necessary...
   //
   {
     const Rank rank_diff = B->get_rank() - A->get_rank();
     loop(d, rank_diff)       shape_A.add_shape_item(1);
     loop(r, A->get_rank())   shape_A.add_shape_item(A->get_shape_item(r));
   }

   // if any dimension of A is longer than that of B, then A cannot be found.
   //
   loop(r, B->get_rank())
       {
         if (shape_A.get_shape_item(r) > B->get_shape_item(r))
            {
              loop(z, len_Z)   new (&Z->get_ravel(z))   IntCell(0);
              goto done;
            }
       }

   for (ArrayIterator zi(B->get_shape()); !zi.done(); ++zi)
       {
PERFORMANCE_START(start_2)
         if (contained(shape_A, &A->get_ravel(0), B, zi.get_values(), qct))
            new (&Z->get_ravel(zi.get_total()))   IntCell(1);
         else
            new (&Z->get_ravel(zi.get_total()))   IntCell(0);

CELL_PERFORMANCE_END(get_statistics_AB(), start_2, zi.get_total())
       }

done:
   Z->set_default_Zero();
   Z->check_value(LOC);

PERFORMANCE_END(fs_SCALAR_AB, start_1, Z->nz_element_count());

   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
bool
Bif_F2_FIND::contained(const Shape & shape_A, const Cell * cA,
                       Value_P B, const Shape & idx_B, double qct)
{
   loop(r, B->get_rank())
       {
         if ((idx_B.get_shape_item(r) + shape_A.get_shape_item(r))
             > B->get_shape_item(r))    return false;
       }

const Shape weight = B->get_shape().reverse_scan();

   for (ArrayIterator ai(shape_A); !ai.done(); ++ai)
       {
         const Shape & pos_A = ai.get_values();
         ShapeItem pos_B = 0;
         loop(r, B->get_rank())   pos_B += weight.get_shape_item(r)
                                         * (idx_B.get_shape_item(r)
                                         + pos_A.get_shape_item(r));

         if (!cA[ai.get_total()].equal(B->get_ravel(pos_B), qct))
            return false;
       }

   return true;
}
//=============================================================================
Token
Bif_F12_ROLL::eval_AB(Value_P A, Value_P B)
{
const APL_Integer qio = Workspace::get_IO();

   // draw A items  from the set [quad-IO ... B]
   //
   if (!A->is_scalar_or_len1_vector())   RANK_ERROR;
   if (!B->is_scalar_or_len1_vector())   RANK_ERROR;

const uint32_t aa = A->get_ravel(0).get_near_int();
APL_Integer set_size = B->get_ravel(0).get_near_int();
   if (aa > set_size)           DOMAIN_ERROR;
   if (set_size <= 0)           DOMAIN_ERROR;
   if (set_size > 0x7FFFFFFF)   DOMAIN_ERROR;


Value_P Z(aa, LOC);

   // set_size can be rather big, so we new/delete it
   //
uint32_t * idx_B = new uint32_t[set_size];
   if (idx_B == 0)   DOMAIN_ERROR;
   
   loop(c, set_size)   idx_B[c] = c + qio;

   loop(z, aa)
       {
         const uint64_t rnd = Workspace::get_RL(set_size) % set_size;
         new (&Z->get_ravel(z)) IntCell(idx_B[rnd]);
         idx_B[rnd] = idx_B[set_size - 1];   // move last item in.
         --set_size;
       }

   delete [] idx_B;

   Z->set_default_Zero();

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROLL::eval_B(Value_P B)
{
   // the standard wants ? to be atomic. We therefore check beforehand
   // that all elements of B are proper, and throw an error if not
   //
   if (check_B(*B, Workspace::get_CT()))   DOMAIN_ERROR;

   return eval_scalar_B(B, &Cell::bif_roll);
}
//-----------------------------------------------------------------------------
bool
Bif_F12_ROLL::check_B(const Value & B, const APL_Float qct)
{
const ShapeItem count = B.nz_element_count();
const Cell * C = &B.get_ravel(0);

   loop(b, count)
      {
       if (C->is_pointer_cell())
          {
            Value_P sub_val = C->get_pointer_value();
            if (check_B(*sub_val, qct))   return true;   // check sub_val failed
            continue;
          }

       if (!C->is_near_int())       return true;
       if (C->get_near_int() < 0)   return true;

        ++C;
      }

   return false;
}
//=============================================================================
Token
Bif_F12_WITHOUT::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;

const uint32_t len_A = A->element_count();
const uint32_t len_B = B->element_count();
const APL_Float qct = Workspace::get_CT();
Value_P Z(len_A, LOC);

uint32_t len_Z = 0;

   loop(a, len_A)
      {
        bool found = false;
        const Cell & cell_A = A->get_ravel(a);
        loop(b, len_B)
            {
              if (cell_A.equal(B->get_ravel(b), qct))
                 {
                   found = true;
                   break;
                 }
            }

        if (!found)
           {
             Z->get_ravel(len_Z++).init(cell_A, Z.getref());
           }
      }

   Z->set_shape_item(0, len_Z);

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
// Inverse functions...

//-----------------------------------------------------------------------------
Function *
Bif_F12_POWER::get_monadic_inverse() const
{
   return Bif_F12_LOGA::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_POWER::get_dyadic_inverse() const
{
   return Bif_F12_LOGA::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_LOGA::get_monadic_inverse() const
{
   return Bif_F12_POWER::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_LOGA::get_dyadic_inverse() const
{
   return Bif_F12_POWER::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_TIMES::get_monadic_inverse() const
{
   return Bif_F12_DIVIDE::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_TIMES::get_dyadic_inverse() const
{
   return Bif_F12_DIVIDE::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_DIVIDE::get_monadic_inverse() const
{
   return Bif_F12_TIMES::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_DIVIDE::get_dyadic_inverse() const
{
   return Bif_F12_TIMES::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_PLUS::get_monadic_inverse() const
{
   return Bif_F12_MINUS::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_PLUS::get_dyadic_inverse() const
{
   return Bif_F12_MINUS::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_MINUS::get_monadic_inverse() const
{
   return Bif_F12_PLUS::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_MINUS::get_dyadic_inverse() const
{
   return Bif_F12_PLUS::fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_CIRCLE::get_monadic_inverse() const
{
   if (this == fun)   return fun_inverse;
   else               return fun;
}
//-----------------------------------------------------------------------------
Function *
Bif_F12_CIRCLE::get_dyadic_inverse() const
{
   if (this == fun)   return fun_inverse;
   else               return fun;
}
//=============================================================================

