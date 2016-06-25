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

#include <unistd.h>

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "Bif_F12_FORMAT.hh"
#include "CDR.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LineInput.hh"
#include "Macro.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_FX.hh"
#include "Quad_TF.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

extern char **environ;

// ⎕-function instances
//
Quad_AF    Quad_AF   ::_fun;
Quad_AT    Quad_AT   ::_fun;
Quad_DL    Quad_DL   ::_fun;
Quad_EA    Quad_EA   ::_fun;
Quad_EB    Quad_EB   ::_fun;
Quad_EC    Quad_EC   ::_fun;
Quad_ENV   Quad_ENV  ::_fun;
Quad_ES    Quad_ES   ::_fun;
Quad_EX    Quad_EX   ::_fun;
Quad_INP   Quad_INP  ::_fun;
Quad_NA    Quad_NA   ::_fun;
Quad_NC    Quad_NC   ::_fun;
Quad_NL    Quad_NL   ::_fun;
Quad_SI    Quad_SI   ::_fun;
Quad_UCS   Quad_UCS  ::_fun;
Quad_STOP  Quad_STOP ::_fun;            // S∆
Quad_TRACE Quad_TRACE::_fun;           // T∆

// ⎕-function pointers
//
Quad_AF    * Quad_AF   ::fun = &Quad_AF   ::_fun;
Quad_AT    * Quad_AT   ::fun = &Quad_AT   ::_fun;
Quad_DL    * Quad_DL   ::fun = &Quad_DL   ::_fun;
Quad_EA    * Quad_EA   ::fun = &Quad_EA   ::_fun;
Quad_EB    * Quad_EB   ::fun = &Quad_EB   ::_fun;
Quad_EC    * Quad_EC   ::fun = &Quad_EC   ::_fun;
Quad_ENV   * Quad_ENV  ::fun = &Quad_ENV  ::_fun;
Quad_ES    * Quad_ES   ::fun = &Quad_ES   ::_fun;
Quad_EX    * Quad_EX   ::fun = &Quad_EX   ::_fun;
Quad_INP   * Quad_INP  ::fun = &Quad_INP  ::_fun;
Quad_NA    * Quad_NA   ::fun = &Quad_NA   ::_fun;
Quad_NC    * Quad_NC   ::fun = &Quad_NC   ::_fun;
Quad_NL    * Quad_NL   ::fun = &Quad_NL   ::_fun;
Quad_SI    * Quad_SI   ::fun = &Quad_SI   ::_fun;
Quad_UCS   * Quad_UCS  ::fun = &Quad_UCS  ::_fun;
Quad_STOP  * Quad_STOP ::fun = &Quad_STOP ::_fun;
Quad_TRACE * Quad_TRACE::fun = &Quad_TRACE::_fun;

//=============================================================================
Token
Quad_AF::eval_B(Value_P B)
{
const ShapeItem ec = B->element_count();
Value_P Z(B->get_shape(), LOC);

   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // Unicode to AV index
            {
              const Unicode uni = cell_B.get_char_value();
              int32_t pos = Avec::find_av_pos(uni);
              if (pos < 0)   new (Z->next_ravel()) IntCell(MAX_AV);
              else           new (Z->next_ravel()) IntCell(pos);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer idx = cell_B.get_near_int();
              new (Z->next_ravel()) CharCell(Quad_AV::indexed_at(idx));
              continue;
            }

         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_AT::eval_AB(Value_P A, Value_P B)
{
   // A should be an integer scalar 1, 2, 3, or 4
   //
   if (A->get_rank() > 0)   RANK_ERROR;
const APL_Integer mode = A->get_ravel(0).get_near_int();
   if (mode < 1)   DOMAIN_ERROR;
   if (mode > 4)   DOMAIN_ERROR;

const ShapeItem cols = B->get_cols();
const ShapeItem rows = B->get_rows();
   if (rows == 0)   LENGTH_ERROR;

const int mode_vec[] = { 3, 7, 4, 2 };
const int mode_len = mode_vec[mode - 1];
Shape shape_Z(rows);
   shape_Z.add_shape_item(mode_len);

Value_P Z(shape_Z, LOC);

   loop(r, rows)
      {
        // get the symbol name by stripping trailing spaces
        //
        const ShapeItem b = r*cols;   // start of the symbol name
        UCS_string symbol_name;
        loop(c, cols)
           {
            const Unicode uni = B->get_ravel(b + c).get_char_value();
            if (uni == UNI_ASCII_SPACE)   break;
            symbol_name.append(uni);
           }

        NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
        if (obj == 0)   throw_symbol_error(symbol_name, LOC);

        const Function * function = obj->get_function();
        if (function)             // user defined or system function.
           {
             function->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        Symbol * symbol = obj->get_symbol();
        if (symbol)               // user defined or system var.
           {
             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        // neither function nor variable (e.g. unused name)
        VALUE_ERROR;

        if (Avec::is_quad(symbol_name[0]))   // system function or variable
           {
             int l;
             const Token t(Workspace::get_quad(symbol_name, l));
             if (t.get_Class() == TC_SYMBOL)   // system variable
                {
                  // Assert() if symbol_name is not a function
                  t.get_sym_ptr();
                }
             else
                {
                  // throws SYNTAX_ERROR if t is not a function
                  t.get_function();
                }
           }
        else                                   // user defined
           {
             Symbol * symbol = Workspace::lookup_existing_symbol(symbol_name);
             if (symbol == 0)   VALUE_ERROR;

             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
           }
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_DL::eval_B(Value_P B)
{
const APL_time_us start = now();

   // B should be an integer or real scalar
   //
   if (B->get_rank() > 0)                 RANK_ERROR;
   if (!B->get_ravel(0).is_real_cell())   DOMAIN_ERROR;

const APL_time_us end = start + 1000000 * B->get_ravel(0).get_real_value();
   if (end < start)                            DOMAIN_ERROR;
   if (end > start + 31*24*60*60*1000000LL)   DOMAIN_ERROR;   // > 1 month

   for (;;)
       {
         // compute time remaining.
         //
         const APL_time_us remaining_us =  end - now();
         if (remaining_us <= 0)   break;

         const int wait_sec  = remaining_us/1000000;
         const int wait_usec = remaining_us%1000000;
         timeval tv = { wait_sec, wait_usec };
         if (select(0, 0, 0, 0, &tv) == 0)   break;
       }

   // return time elapsed.
   //
Value_P Z(LOC);
   new (Z->next_ravel()) FloatCell(0.000001*(now() - start));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_EA::eval_AB(Value_P A, Value_P B)
{
   if (!A->is_char_string())
      {
        if (A->get_rank() > 1)   RANK_ERROR;
        else                     DOMAIN_ERROR;
      }

   if (!B->is_char_string())
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     DOMAIN_ERROR;
      }

   return Macro::Z__A_Quad_EA_B->eval_AB(A, B);
}
//=============================================================================
Token
Quad_EB::eval_AB(Value_P A, Value_P B)
{
   if (!A->is_char_string())
      {
        if (A->get_rank() > 1)   RANK_ERROR;
        else                     DOMAIN_ERROR;
      }

   if (!B->is_char_string())
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     DOMAIN_ERROR;
      }

   return Macro::Z__A_Quad_EB_B->eval_AB(A, B);
}
//=============================================================================
Token
Quad_EC::eval_B(Value_P B)
{
const UCS_string statement_B(*B.get());

ExecuteList * fun = 0;
   try { fun = ExecuteList::fix(statement_B, LOC); }    catch (...) {}

   if (fun == 0)
      {
        // syntax error in B
        //
        Value_P Z2(2, LOC);
            new (Z2->next_ravel())  IntCell(Error::error_major(E_SYNTAX_ERROR));
            new (Z2->next_ravel())  IntCell(Error::error_minor(E_SYNTAX_ERROR));

        Value_P Z3(Error::error_name(E_SYNTAX_ERROR), LOC);
        Value_P Z(3, LOC);
        new (Z->next_ravel()) IntCell(0);        // return code = error
        new (Z->next_ravel()) PointerCell(Z2, Z.getref());   // ⎕ET value
        new (Z->next_ravel()) PointerCell(Z3, Z.getref());   // ⎕EM

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
Quad_EC::eval_fill_B(Value_P B)
{
Value_P Z2(2, LOC);
   new(Z2->next_ravel())   IntCell(0);
   new(Z2->next_ravel())   IntCell(0);
   Z2->check_value(LOC);

Value_P Zsub(3, LOC);
   new (Zsub->next_ravel())   IntCell(3);   // statement without result
   new (Zsub->next_ravel())   PointerCell(Z2, Zsub.getref());
   new (Zsub->next_ravel())   PointerCell(Idx0(LOC), Zsub.getref());
   Zsub->check_value(LOC);

Value_P Z((ShapeItem)0, LOC);
  new (&Z->get_ravel(0))   PointerCell(Zsub, Z.getref());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Quad_EC::eoc(Token & result)
{
   // set result to an APL value Z = (Z1 Z2 Z3) where:
   //
   // Z1 is an integer scalar 0-5 (return code),
   // Z2 is a two-element integer vector with the value that ⎕ET would have,
   // Z3 is some value
   //
Value_P Z(3, LOC);
   if (result.get_tag() == TOK_ERROR)
      {
        StateIndicator * si = Workspace::SI_top();
        si->clear_safe_execution();

        const Error & err = si->get_error();
        const ErrorCode ec = ErrorCode(result.get_int_val());

        PrintBuffer pb;
        pb.append_ucs(Error::error_name(ec));
        pb.append_ucs(err.get_error_line_2());
        pb.append_ucs(err.get_error_line_3());

        Value_P Z2(2, LOC);
            new (Z2->next_ravel()) IntCell(Error::error_major(ec));
            new (Z2->next_ravel()) IntCell(Error::error_minor(ec));
        Z2->check_value(LOC);

        Value_P Z3(pb, LOC);   // 3 line message like ⎕EM

        new (Z->next_ravel()) IntCell(0);
        new (Z->next_ravel()) PointerCell(Z2, Z.getref());
        new (Z->next_ravel()) PointerCell(Z3, Z.getref());

        Z->check_value(LOC);
        move_2(result, Token(TOK_APL_VALUE1, Z), LOC);
        return;
      }

   // all other cases have Z2 = 0 0
   //
Value_P Z2(2, LOC);
   new (Z2->next_ravel()) IntCell(0);
   new (Z2->next_ravel()) IntCell(0);
        Z2->check_value(LOC);

   switch(result.get_tag())
      {
        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             new (Z->next_ravel()) IntCell(1);
             new (Z->next_ravel()) PointerCell(Z2, Z.getref());
             new (Z->next_ravel()) PointerCell(result.get_apl_val(),Z.getref());
             break;

        case TOK_APL_VALUE2:
             new (Z->next_ravel()) IntCell(2);
             new (Z->next_ravel()) PointerCell(Z2, Z.getref());
             new (Z->next_ravel()) PointerCell(result.get_apl_val(),Z.getref());
             break;

        case TOK_NO_VALUE:
        case TOK_VOID:
             new (Z->next_ravel()) IntCell(3);
             new (Z->next_ravel()) PointerCell(Z2, Z.getref());
             new (Z->next_ravel()) PointerCell(Idx0_0(LOC), Z.getref());
             break;

        case TOK_BRANCH:
             new (Z->next_ravel()) IntCell(4);
             new (Z->next_ravel()) PointerCell(Z2, Z.getref());
             new (Z->next_ravel()) IntCell(result.get_int_val());
             break;

        case TOK_ESCAPE:
             new (Z->next_ravel()) IntCell(5);
             new (Z->next_ravel()) PointerCell(Z2, Z.getref());
             new (Z->next_ravel()) PointerCell(Idx0_0(LOC), Z.getref());
             break;

        default: CERR << "unexpected result tag " << result.get_tag()
                      << " in Quad_EC::eoc()" << endl;
                 Assert(0);
      }

   Z->check_value(LOC);
   move_2(result, Token(TOK_APL_VALUE1, Z), LOC);
}
//=============================================================================
Token
Quad_ENV::eval_B(Value_P B)
{
   if (!B->is_char_string())   DOMAIN_ERROR;

const ShapeItem ec_B = B->element_count();

vector<const char *> evars;

   for (char **e = environ; *e; ++e)
       {
         const char * env = *e;

         // check if env starts with B.
         //
         bool match = true;
         loop(b, ec_B)
            {
              if (B->get_ravel(b).get_char_value() != Unicode(env[b]))
                 {
                   match = false;
                   break;
                 }
            }

         if (match)   evars.push_back(env);
       }

const Shape sh_Z(evars.size(), 2);
Value_P Z(sh_Z, LOC);

   loop(e, evars.size())
      {
        const char * env = evars[e];
        UCS_string ucs;
        while (*env)
           {
             if (*env != '=')   ucs.append(Unicode(*env++));
             else               break;
           }
        ++env;   // skip '='

        Value_P varname(ucs, LOC);

        ucs.clear();
        while (*env)   ucs.append(Unicode(*env++));

        Value_P varval(ucs, LOC);

        new (Z->next_ravel()) PointerCell(varname, Z.getref());
        new (Z->next_ravel()) PointerCell(varval, Z.getref());
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_ES::eval_AB(Value_P A, Value_P B)
{
const UCS_string ucs(*A.get());
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(&ucs, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::eval_B(Value_P B)
{
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(0, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;
   if (Workspace::SI_top()->get_safe_execution())   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::event_simulate(const UCS_string * A, Value_P B, Error & error)
{
   // B is empty: no action
   //
   if (B->element_count() == 0)   return Token();

const ErrorCode ec = get_error_code(B);
   if (ec == E_QUAD_EA_EXEC)   return Token(TOK_EA_EXEC, B->clone(LOC));

   error.init(ec, error.throw_loc);

   if (error.error_code == E_NO_ERROR)   // B = 0 0: reset ⎕ET and ⎕EM.
      {
        Workspace::clear_error(LOC);
        return Token();
      }

   if (error.error_code == E_ASSERTION_FAILED)   // B = 0 ASSERTION_FAILED
      {
        Assert(0 && "simulated ASSERTION_FAILED in ⎕ES");
      }

   // at this point we shall throw the error. Add some error details.
   //
   // set up error message 1
   //
   if (A)                                 // A ⎕ES B
      {
        error.error_message_1 = *A;
      }
   else if (error.error_code == E_USER_DEFINED_ERROR)   // ⎕ES with character B
      {
        error.error_message_1 = UCS_string(*B.get());
      }
   else if (error.is_known())             //  ⎕ES B with known major/minor B
      {
        /* error_message_1 already OK */ ;
      }
   else                                   //  ⎕ES B with unknown major/minor B
      {
        error.error_message_1.clear();
      }

   error.show_locked = true;

   Assert(Workspace::SI_top());
   if (StateIndicator * si = Workspace::SI_top()->get_parent())
      {
        const UserFunction * ufun = si->get_executable()->get_ufun();
        if (ufun)
           {
             // lrm p 282: When ⎕ES is executed from within a defined function
             // and B is not empty, the event action is generated as though
             // the function were primitive.
             //
             error.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
             error.error_message_2.append(ufun->get_name());
             error.left_caret = 6;
             error.right_caret = -1;
             Workspace::pop_SI(LOC);
             Workspace::SI_top()->get_error() = error;
             error.print_em(UERR, LOC);
             return Token();
           }
      }

   Workspace::SI_top()->update_error_info(error);
   return Token();
}
//-----------------------------------------------------------------------------
ErrorCode
Quad_ES::get_error_code(Value_P B)
{
   // B shall be one of:                  → Error code
   //
   // 1. empty, or                        → No error
   // 2. a 2 element integer vector, or   → B[0]/N[1]
   // 3. a simple character vector.       → user defined
   //
   // Otherwise, throw an error

   if (B->get_rank() > 1)   RANK_ERROR;

   if (B->element_count() == 0)   return E_NO_ERROR;
   if (B->is_char_string())       return E_USER_DEFINED_ERROR;

const APL_Integer major = B->get_ravel(0).get_near_int();
   if (major == (E_QUAD_EA_EXEC >> 16) && B->element_count() == 4)
      return E_QUAD_EA_EXEC;

   if (B->element_count() != 2)   LENGTH_ERROR;
const APL_Integer minor = B->get_ravel(1).get_near_int();

   return ErrorCode(major << 16 | (minor & 0xFFFF));
}
//=============================================================================
Token
Quad_EX::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)   new (Z->next_ravel()) IntCell(expunge(vars[z]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
int
Quad_EX::expunge(UCS_string name)
{
   if (name.size() == 0)   return 0;

Symbol * symbol = Workspace::lookup_existing_symbol(name);
   if (symbol == 0)   return 0;
   return symbol->expunge();
}
//=============================================================================
Token
Quad_INP::eval_AB(Value_P A, Value_P B)
{
   if (Quad_INP_running)
      {
        Workspace::more_error() = UCS_string("⎕INP called recursively");
        SYNTAX_ERROR;
      }

   // make sure that B is a non-empty string
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

   // temporary strings if get_esc() should fail
   //
UCS_string e1;
UCS_string e2;
   get_esc(A, e1, e2);

   Quad_INP_running = true;

   end_marker = B->get_UCS_ravel();
   esc1 = e1;
   esc2 = e2;

   prefixes.clear();
   escapes.clear();
   suffixes.clear();

   read_strings();    // read lines from file or stdin
   split_strings();   // split lines into prefixes, escapes, and suffixes

const ShapeItem line_count = raw_lines.size();
Value_P BB(line_count, LOC);
   loop(l, line_count)
       {
         Cell * cBB = BB->next_ravel();
         if (escapes[l].size() == 0)   // prefix only
            {
              Value_P c1(prefixes[l], LOC);
              Value_P row(1, LOC);
              new (row->next_ravel())   PointerCell(c1, row.getref());
              row->check_value(LOC);
              new (cBB)   PointerCell(row, BB.getref());
            }
         else if (suffixes[l].size() == 0)   // prefix and escape
            {
              Value_P c1(prefixes[l], LOC);
              Value_P c2(escapes [l], LOC);
              Value_P row(2, LOC);
              new (row->next_ravel())   PointerCell(c1, row.getref());
              new (row->next_ravel())   PointerCell(c2, row.getref());
              row->check_value(LOC);
              new (cBB)   PointerCell(row, BB.getref());
            }
         else                                // prefix, escape, and suffix
            {
              Value_P c1(prefixes[l], LOC);
              Value_P c2(escapes [l], LOC);
              Value_P c3(suffixes[l], LOC);
              Value_P row(3, LOC);
              new (row->next_ravel())   PointerCell(c1, row.getref());
              new (row->next_ravel())   PointerCell(c2, row.getref());
              new (row->next_ravel())   PointerCell(c3, row.getref());
              row->check_value(LOC);
              new (cBB)   PointerCell(row, BB.getref());
            }
       }

Token ret = Macro::Z__Quad_INP_B->eval_B(BB);
   Assert1(ret.get_tag() == TOK_SI_PUSHED);

   loop(l, line_count)   BB->get_ravel(l).release(LOC);

   Quad_INP_running = false;
   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_B(Value_P B)
{
   if (Quad_INP_running)
      {
        Workspace::more_error() = UCS_string("⎕INP called recursively");
        SYNTAX_ERROR;
      }

   // make sure that B is a non-empty string
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

   Quad_INP_running = true;

   end_marker = B->get_UCS_ravel();
   read_strings();    // read lines from file or stdin

Value_P Z(raw_lines.size(), LOC);
   loop(l, raw_lines.size())
       {
         Value_P ZZ(raw_lines[l], LOC);
         new (Z->next_ravel())   PointerCell(ZZ, Z.getref());
       }

   Quad_INP_running = false;
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_XB(Value_P X, Value_P B)
{
   if (X->element_count() != 1)
      {
        if (X->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

APL_Integer x = X->get_ravel(0).get_near_int();
   if (x == 0)   return eval_B(B);
   if (x > 1)    DOMAIN_ERROR;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

UCS_string end_marker(B->get_UCS_ravel());

vector<UCS_string> lines;
Parser parser(PM_EXECUTE, LOC, false);

   for (;;)
      {
         bool eof = false;
         UCS_string line;
         UCS_string prompt;
         InputMux::get_line(LIM_Quad_INP, prompt, line, eof,
                            LineHistory::quad_INP_history);
         if (eof)   break;
         const int end_pos = line.substr_pos(end_marker);
         if (end_pos != -1)  break;

         Token_string tos;
         ErrorCode ec = parser.parse(line, tos);
         if (ec != E_NO_ERROR)
            {
              throw_apl_error(ec, LOC);
            }

         if ((tos.size() & 1) == 0)   LENGTH_ERROR;

         // expect APL values at even positions and , at odd positions
         //
         loop(t, tos.size())
            {
              Token & tok = tos[t];
              if (t & 1)   // , or ⍪
                 {
                   if (tok.get_tag() != TOK_F12_COMMA &&
                       tok.get_tag() != TOK_F12_COMMA1)   DOMAIN_ERROR;
                 }
              else         // value
                 {
                   if (tok.get_Class() != TC_VALUE)   DOMAIN_ERROR;
                 }

            }
         lines.push_back(line);
      }

Value_P Z(lines.size(), LOC);
   loop(z, lines.size())
      {
         Token_string tos;
         parser.parse(lines[z], tos);
         const ShapeItem val_count = (tos.size() + 1)/2;
         Value_P ZZ(val_count, LOC);
         loop(v, val_count)
            {
              new (ZZ->next_ravel()) PointerCell(tos[2*v].get_apl_val(),
                                                 ZZ.getref());
            }

         ZZ->check_value(LOC);
         new (Z->next_ravel()) PointerCell(ZZ, Z.getref());
      }

   if (lines.size() == 0)   // empty result
      {
        Value_P ZZ(UCS_string(), LOC);
        new(&Z->get_ravel(0)) PointerCell(ZZ, Z.getref());
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Quad_INP::get_esc(Value_P A, UCS_string & esc1, UCS_string & esc2)
{

   // A is either one string (esc1 == esc2) or two (nested) strings
   // for esc1 and esc2 respectively.
   //
   if (A->compute_depth() == 2)   // two (nested) strings
      {
        if (A->get_rank() != 1)        RANK_ERROR;
        if (A->element_count() != 2)   LENGTH_ERROR;

        loop(e, 2)
           {
             const Cell & cell = A->get_ravel(e);
             if (cell.is_pointer_cell())   // char vector
                {
                  if (e)   esc2 = cell.get_pointer_value()->get_UCS_ravel();
                  else     esc1 = cell.get_pointer_value()->get_UCS_ravel();
                }
             else                          // char scalar
                {
                  if (e)   esc2 = cell.get_pointer_value()->get_UCS_ravel();
                  else     esc1 = cell.get_pointer_value()->get_UCS_ravel();
                }
           }

        if (esc1.size() == 0)   LENGTH_ERROR;
      }
   else                       // one string 
      {
        esc1 = A->get_UCS_ravel();
        if (esc1.size() == 0)   LENGTH_ERROR;
        esc2 = esc1;
      }
}
//-----------------------------------------------------------------------------
void
Quad_INP::read_strings()
{
   // read lines until an end-maker is detected
   //
   raw_lines.clear();
   for (;;)
      {
        bool eof = false;
        UCS_string prompt;
        UCS_string line;
        InputMux::get_line(LIM_Quad_INP, prompt, line, eof,
                                 LineHistory::quad_INP_history);

        const int end = line.substr_pos(end_marker);
        if (end != -1)   // end marker found
           {
             line.shrink(end);
             if (line.size())   raw_lines.push_back(line);
             break;
           }

        if (eof && !line.size())   break;
        raw_lines.push_back(line);
        if (eof)   break;
      }
}
//-----------------------------------------------------------------------------
void
Quad_INP::split_strings()
{
UCS_string empty;
   loop(r, raw_lines.size())
      {
        UCS_string & line = raw_lines[r];
        const ShapeItem epos = line.substr_pos(esc1);
        if (esc1.size() == 0 || epos == -1)   // no escape in this line
           {
             prefixes.push_back(line);
             escapes.push_back(empty);
             suffixes.push_back(empty);
             continue;
           }

        // at this point line did contain an exec string
        //
        UCS_string pref = line;
        pref.shrink(epos);
        prefixes.push_back(pref);

        line = line.drop(epos + esc1.size());   // skip prefix and esc1
        if (esc2.size())   // end defined
           {
             const ShapeItem eend = line.substr_pos(esc2);
             if (eend == -1)   // no exec end in this line
                {
                  escapes.push_back(line);
                  suffixes.push_back(empty);
                  continue;
                }
             else              // found an exec end in this line
                {
                  UCS_string exec = line;
                  exec.shrink(eend);
                  escapes.push_back(exec);
                  line = line.drop(eend + esc2.size());   // skip exec and esc2
                  suffixes.push_back(line);
                  continue;
                }
           }
        else               // no end defined
           {
             escapes.push_back(line);
             suffixes.push_back(empty);
           }
      }

   Assert(prefixes.size() >= raw_lines.size());
   Assert(prefixes.size() == escapes.size());
   Assert(prefixes.size() == suffixes.size());
}
//=============================================================================
Token
Quad_NC::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(v, var_count)   new (Z->next_ravel())   IntCell(get_NC(vars[v]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
APL_Integer
Quad_NC::get_NC(const UCS_string ucs)
{
   if (ucs.size() == 0)   return -1;   // invalid name

const Unicode uni = ucs[0];
Symbol * symbol = 0;
   if      (uni == UNI_ALPHA)            symbol = &Workspace::get_v_ALPHA();
   else if (uni == UNI_LAMBDA)           symbol = &Workspace::get_v_LAMBDA();
   else if (uni == UNI_CHI)              symbol = &Workspace::get_v_CHI();
   else if (uni == UNI_OMEGA)            symbol = &Workspace::get_v_OMEGA();
   else if (uni == UNI_ALPHA_UNDERBAR)   symbol = &Workspace::get_v_OMEGA_U();
   else if (uni == UNI_OMEGA_UNDERBAR)   symbol = &Workspace::get_v_OMEGA_U();
   if (ucs.size() != 1)   symbol = 0;    // unless more than one char

   if (Avec::is_quad(uni))   // distinguished name
      {
        int len = 0;
        const Token t = Workspace::get_quad(ucs, len);
        if (len < 2)                      return 5;    // ⎕ : quad variable
        if (t.get_Class() == TC_VOID)     return -1;   // invalid
        if (t.get_Class() == TC_SYMBOL)   symbol = t.get_sym_ptr();
        else                              return  6;   // quad function
      }

   if (symbol)   // system variable (⎕xx, ⍺, ⍶, ⍵, ⍹, λ, or χ
      {
        const NameClass nc = symbol->get_nc();
        if (nc == 0)   return nc;
        return nc + 3;
      }

   // user-defined name
   //
   symbol = Workspace::lookup_existing_symbol(ucs);

   if (!symbol)
      {
        // symbol not found. Distinguish between invalid and unused names
        //
        if (!Avec::is_first_symbol_char(ucs[0]))   return -1;   // invalid
        loop (u, ucs.size())
           {
             if (!Avec::is_symbol_char(ucs[u]))   return -1;   // invalid
           }
        return 0;   // unused
      }

const NameClass nc = symbol->get_nc();

   // return nc, but map shared variables to regular variables
   //
   return nc == NC_SHARED_VAR ? NC_VARIABLE : nc;
}
//=============================================================================
Token
Quad_NL::do_quad_NL(Value_P A, Value_P B)
{
   if (!!A && !A->is_char_string())   DOMAIN_ERROR;
   if (B->get_rank() > 1)             RANK_ERROR;
   if (B->element_count() == 0)   // nothing requested
      {
        return Token(TOK_APL_VALUE1, Str0_0(LOC));
      }

UCS_string first_chars;
   if (!!A)   first_chars = UCS_string(*A);

   // 1. create a bitmap of ⎕NC values requested in B
   //
int requested_NCs = 0;
   {
     loop(b, B->element_count())
        {
          const APL_Integer bb = B->get_ravel(b).get_near_int();
          if (bb < 1)   DOMAIN_ERROR;
          if (bb > 6)   DOMAIN_ERROR;
          requested_NCs |= 1 << bb;
        }
   }

   // 2, build a name table, starting with user defined names
   //
vector<UCS_string> names;
   {
     const ShapeItem symbol_count = Workspace::symbols_allocated();
     DynArray(Symbol *, table, symbol_count);
     Workspace::get_all_symbols(&table[0], symbol_count);

     loop(s, symbol_count)
        {
          Symbol * symbol = table[s];
          if (symbol->is_erased())   continue;   // erased symbol

          NameClass nc = symbol->get_nc();
          if (nc == NC_SHARED_VAR)   nc = NC_VARIABLE;

          if (!(requested_NCs & 1 << nc))   continue;   // name class not in B

          if (first_chars.size())
             {
               const Unicode first_char = symbol->get_name()[0];
               if (!first_chars.contains(first_char))   continue;
             }

          names.push_back(symbol->get_name());
        }
   }

   // 3, append ⎕-vars and ⎕-functions to name table (unless prevented by A)
   //
   if (first_chars.size() == 0 || first_chars.contains(UNI_Quad_Quad))
      {
#define ro_sv_def(x, _str, _txt)                                   \
   { Symbol * symbol = &Workspace::get_v_ ## x();           \
     if ((requested_NCs & 1 << 5) && symbol->get_nc() != 0) \
        names.push_back(symbol->get_name()); }

#define rw_sv_def(x, _str, _txt)                                   \
   { Symbol * symbol = &Workspace::get_v_ ## x();           \
     if ((requested_NCs & 1 << 5) && symbol->get_nc() != 0) \
        names.push_back(symbol->get_name()); }

#define sf_def(x, _str, _txt)                                      \
   { if (requested_NCs & 1 << 6)   names.push_back((x::fun->get_name())); }
#include "SystemVariable.def"
      }


   // 4. compute length of longest name
   //
ShapeItem longest = 0;
   loop(n, names.size())
      {
        if (longest < names[n].size())
           longest = names[n].size();
      }

const Shape shZ(names.size(), longest);
Value_P Z(shZ, LOC);

   // 5. construct result. The number of symbols is small and ⎕NL is
   // (or should) not be a perfomance // critical function, so we can
   // use a simple (find smallest and print) // approach.
   //
   for (int count = names.size(); count; --count)
      {
        // find smallest.
        //
        uint32_t smallest = count - 1;
        loop(t, count - 1)
           {
             if (names[smallest].compare(names[t]) > 0)   smallest = t;
           }
        // copy name to result, padded with spaces.
        //
        loop(l, longest)
           {
             const UCS_string & ucs = names[smallest];
             new (Z->next_ravel())
                 CharCell(l < ucs.size() ? ucs[l] : UNI_ASCII_SPACE);
           }

        // remove smalles from table
        //
        names[smallest] = names[count - 1];
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SI::eval_AB(Value_P A, Value_P B)
{
   if (A->element_count() != 1)   // not scalar-like
      {
        if (A->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }
APL_Integer a = A->get_ravel(0).get_near_int();
const ShapeItem len = Workspace::SI_entry_count();
   if (a >= len)   DOMAIN_ERROR;
   if (a < -len)   DOMAIN_ERROR;
   if (a < 0)   a += len;   // negative a counts backwards from end

const StateIndicator * si = 0;
   for (si = Workspace::SI_top(); si; si = si->get_parent())
       {
         if (si->get_level() == a)   break;   // found
       }

   Assert(si);

   if (B->element_count() != 1)   // not scalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const Function_PC PC = Function_PC(si->get_PC() - 1);
const Executable * exec = si->get_executable();
const ParseMode pm = exec->get_parse_mode();
const UCS_string & fun_name = exec->get_name();
const Function_Line fun_line = exec->get_line(PC);

Value_P Z;

const APL_Integer b = B->get_ravel(0).get_near_int();
   switch(b)
      {
        case 1:  Z = Value_P(fun_name, LOC);
                 break;

        case 2:  Z = Value_P(LOC);
                new (&Z->get_ravel(0)) IntCell(fun_line);
                break;

        case 3:  {
                   UCS_string fun_and_line(fun_name);
                   fun_and_line.append(UNI_ASCII_L_BRACK);
                   fun_and_line.append_number(fun_line);
                   fun_and_line.append(UNI_ASCII_R_BRACK);
                   Z = Value_P(fun_and_line, LOC); 
                 }
                 break;

        case 4:  if (si->get_error().error_code)
                    {
                      const UCS_string & text =
                                        si->get_error().get_error_line_2();
                      Z = Value_P(text, LOC);
                    }
                 else
                    {
                      const UCS_string text = exec->statement_text(PC);
                      Z = Value_P(text, LOC);
                    }
                 break;

        case 5: Z = Value_P(LOC);
                new (&Z->get_ravel(0)) IntCell(PC);                      break;

        case 6: Z = Value_P(LOC);
                new (&Z->get_ravel(0)) IntCell(pm);                      break;

        default: DOMAIN_ERROR;
      }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_SI::eval_B(Value_P B)
{
   if (B->element_count() != 1)   // not scalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const APL_Integer b = B->get_ravel(0).get_near_int();
const ShapeItem len = Workspace::SI_entry_count();

   if (b < 1)   DOMAIN_ERROR;
   if (b > 4)   DOMAIN_ERROR;

   // at this point we should not fail...
   //
Value_P Z(len, LOC);

ShapeItem z = 0;
   for (const StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         Cell * cZ = &Z->get_ravel(len - ++z);

         const Function_PC PC = Function_PC(si->get_PC() - 1);
         const Executable * exec = si->get_executable();
         const ParseMode pm = exec->get_parse_mode();
         const UCS_string & fun_name = exec->get_name();
         const Function_Line fun_line = exec->get_line(PC);

         switch (b)
           {
             case 1:  new (cZ) PointerCell(
                                Value_P(fun_name, LOC), Z.getref());
                      break;

             case 2:  new (cZ) IntCell(fun_line);
                      break;

             case 3:  {
                        UCS_string fun_and_line(fun_name);
                        fun_and_line.append(UNI_ASCII_L_BRACK);
                        fun_and_line.append_number(fun_line);
                        fun_and_line.append(UNI_ASCII_R_BRACK);
                        new (cZ) PointerCell(Value_P(
                                    fun_and_line, LOC), Z.getref()); 
                      }
                      break;

             case 4:  if (si->get_error().error_code)
                         {
                           const UCS_string & text =
                                        si->get_error().get_error_line_2();
                           new (cZ) PointerCell(Value_P(text, LOC), Z.getref()); 
                         }
                      else
                         {
                           const UCS_string text = exec->statement_text(PC);
                           new (cZ) PointerCell(Value_P( text, LOC),
                                                Z.getref()); 
                         }
                      break;

             case 5:  new (cZ) IntCell(PC);                             break;
             case 6:  new (cZ) IntCell(pm);                             break;
           }

       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_UCS::eval_B(Value_P B)
{
Value_P Z(B->get_shape(), LOC);

const ShapeItem ec = B->nz_element_count();
   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // char to Unicode
            {
              const Unicode uni = cell_B.get_char_value();
              new (&Z->get_ravel(v)) IntCell(uni);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer bint = cell_B.get_near_int();
              new (&Z->get_ravel(v))   CharCell(Unicode(bint));
              continue;
            }

         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
UserFunction *
Stop_Trace::locate_fun(const Value & fun_name)
{
   if (!fun_name.is_char_string())   return 0;

UCS_string fun_name_ucs(fun_name);
   if (fun_name_ucs.size() == 0)   return 0;

Symbol * fun_symbol = Workspace::lookup_existing_symbol(fun_name_ucs);
   if (fun_symbol == 0)
      {
        CERR << "symbol " << fun_name_ucs << " not found" << endl;
        return 0;
      }

Function * fun = fun_symbol->get_function();
   if (fun == 0)
      {
        CERR << "symbol " << fun_name_ucs << " is not a function" << endl;
        return 0;
      }

UserFunction * ufun = fun->get_ufun1();
   if (ufun == 0)
      {
        CERR << "symbol " << fun_name_ucs
             << " is not a defined function" << endl;
        return 0;
      }

   return ufun;
}
//-----------------------------------------------------------------------------
Token
Stop_Trace::reference(const vector<Function_Line> & lines, bool assigned)
{
Value_P Z(lines.size(), LOC);

   loop(z, lines.size())   new (Z->next_ravel()) IntCell(lines[z]);

   Z->set_default_Zero();
   if (assigned)   return Token(TOK_APL_VALUE2, Z);
   else            return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Stop_Trace::assign(UserFunction * ufun, const Value & new_value, bool stop)
{
DynArray(Function_Line, lines, new_value.element_count());
int line_count = 0;

   loop(l, new_value.element_count())
      {
         APL_Integer line = new_value.get_ravel(l).get_near_int();
         if (line < 1)   continue;
         lines[line_count++] = (Function_Line)line;
      }

   ufun->set_trace_stop(lines.get_data(), line_count, stop);
}
//=============================================================================
Token
Quad_STOP::eval_AB(Value_P A, Value_P B)
{
   // Note: Quad_STOP::eval_AB can be called directly or via S∆. If
   //
   // 1. called via S∆   then A is the function and B are the lines.
   // 2. called directly then B is the function and A are the lines.
   //
UserFunction * ufun = locate_fun(*A);
   if (ufun)   // case 1.
      {
        assign(ufun, *B, true);
        return reference(ufun->get_stop_lines(), true);
      }

   // case 2.
   //
   ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   assign(ufun, *A, true);
   return reference(ufun->get_stop_lines(), true);
}
//-----------------------------------------------------------------------------
Token
Quad_STOP::eval_B(Value_P B)
{
UserFunction * ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   return reference(ufun->get_stop_lines(), false);
}
//=============================================================================
Token
Quad_TRACE::eval_AB(Value_P A, Value_P B)
{
   // Note: Quad_TRACE::eval_AB can be called directly or via S∆. If
   //
   // 1. called via S∆   then A is the function and B are the lines.
   // 2. called directly then B is the function and A are the lines.
   //
UserFunction * ufun = locate_fun(*A);
   if (ufun)   // case 1.
      {
        assign(ufun, *B, false);
        return reference(ufun->get_trace_lines(), true);
      }

   // case 2.
   //
   ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   assign(ufun, *A, false);
   return reference(ufun->get_trace_lines(), true);
}
//-----------------------------------------------------------------------------
Token
Quad_TRACE::eval_B(Value_P B)
{
UserFunction * ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   return reference(ufun->get_trace_lines(), false);
}
//=============================================================================

