/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2015  Dr. Dirk Laurie

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

#include <cstring>
#include <sstream>
#include <ostream>

#include <Command.hh>
#include <ComplexCell.hh>
#include <DiffOut.hh>
#include <Error.hh>
#include <FloatCell.hh>
#include <LineInput.hh>
#include <PointerCell.hh>
#include <Tokenizer.hh>
#include <UserPreferences.hh>
#include <Workspace.hh>

#include "libapl.h"

using namespace std;

/******************************************************************************
   1. APL value constructor functions. The APL_value returned must be released
      with release_value() below at some point in time (unless it is 0)
 */

/// A new integer scalar.
APL_value
int_scalar(int64_t val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) IntCell(val);
   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new floating point scalar.
APL_value
double_scalar(APL_Float val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) FloatCell(val);
   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new complex scalar.
APL_value
complex_scalar(APL_Float real, APL_Float imag, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) ComplexCell(real, imag);
   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new character scalar.
APL_value
char_scalar(int uni, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) CharCell(Unicode(uni));
   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------

/// A new APL value with given rank and shape. All ravel elements are
/// initialized to integer 0.
APL_value
apl_value(int rank, const int64_t * shape, const char * loc)
{
const Shape sh(rank, shape);
Value_P Z(sh, loc);

   while (Cell * cell = Z->next_ravel())   new (cell)   IntCell(0);

   Z->check_value(LOC);
   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new character vector.
APL_value
char_vector(const char * str, const char * loc)
{
UTF8_string utf8(str);
UCS_string ucs(utf8);

Value_P Z(ucs, loc);
   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}

/******************************************************************************
   2. APL value destructor function. All non-0 APL_values must be released
      at some point in time (even const ones). release_value(0) is not needed
      but accepted.
 */
void
release_value(const APL_value val, const char * loc)
{
Value * v = const_cast<Value *>(val);
   if (val)   v->decrement_owner_count(loc);
}


/******************************************************************************
   3. read access to APL values. All ravel indices count from ⎕IO←0.
 */

//-----------------------------------------------------------------------------
/// return ⍴⍴val
int
get_rank(const APL_value val)
{
   return val->get_rank();
}
//-----------------------------------------------------------------------------

/// return (⍴val)[axis]
int64_t
get_axis(const APL_value val, unsigned int axis)
{
   return uRank(axis) < val->get_rank() ? val->get_shape_item(axis) : -1;
}
//-----------------------------------------------------------------------------

/// return ×/⍴val
uint64_t
get_element_count(const APL_value val)
{
   return val->element_count();
}
//-----------------------------------------------------------------------------
/// return the type of (,val)[idx]
int
get_type(const APL_value val, uint64_t idx)
{
   if (idx >= uint64_t(val->nz_element_count()))   return 0;
   return val->get_ravel(idx).get_cell_type();
}
//-----------------------------------------------------------------------------
/// return non-0 if val is a simple character vector.
int
is_string(const APL_value val)
{
   return val->is_char_vector();
}
//-----------------------------------------------------------------------------

/// return the character val[idx] (after having checked is_char())
int
get_char(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_char_value();
}
//-----------------------------------------------------------------------------

/// return the integer val[idx] (after having checked is_int())
int64_t
get_int(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_int_value();
}
//-----------------------------------------------------------------------------

/// return the real part of val[idx] (after having checked is_numeric())
APL_Float
get_real(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_real_value();
}
//-----------------------------------------------------------------------------

/// return the imag part of val[idx] (after having checked is_numeric())
APL_Float
get_imag(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_imag_value();
}
//-----------------------------------------------------------------------------

/// return the (nested) value val[idx] (after having checked is_value()).
/// The APL_value returned must be released with release_value() later on.
///
APL_value
get_value(const APL_value val, uint64_t idx)
{
Value_P sub = val->get_ravel(idx).get_pointer_value();
   sub.get()->increment_owner_count(LOC);   // keep value
   return sub.get();
}
/******************************************************************************
   4. write access to APL values. All ravel indices count from ⎕IO←0.
 */

//-----------------------------------------------------------------------------
APL_value
assign_var(const unsigned int * var_name_ucs, int rank, uint64_t * shape)
{
Shape sh;
   loop(r, rank)   sh.add_shape_item(*shape++);  

Value_P Z(sh, LOC);
   loop(z, Z->nz_element_count())   new (Z->next_ravel())   IntCell(0);
   Z->check_value(LOC);

   if (var_name_ucs == 0)
      {
        // the caller wants only q value initialized to 0 without
        // assigning it to a value. We have to increment the owner count
        // and the caller is responsiblr fopr decrementing it when the
        // value is no longer needed.
        //
        Z.get()->increment_owner_count(LOC);   // keep value
        return Z.get();
      }

   if (!Avec::is_first_symbol_char(Unicode(*var_name_ucs)))
      return 0;

UCS_string var_name;
   var_name.reserve(40);
   while (*var_name_ucs)
      {
        const Unicode uni = Unicode(*var_name_ucs++);
        if (!Avec::is_symbol_char(uni))   return 0;
        var_name.append(uni);
      }

Symbol * symbol = Workspace::lookup_symbol(var_name);
   if (!symbol)   return 0;
   if (!symbol->can_be_assigned())   return 0;

   symbol->assign(Z, false, LOC);

   // at this point owner_count shall be at least 2 so we can return a pointer
   // to Z without Z gettting deleted when we return from this function;
   //
   if (Z->get_owner_count() < 2)   return 0;
   return Z.get();
}
//-----------------------------------------------------------------------------
/// val[idx]←unicode
void
set_char(int new_char, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())
      {
        Value * v = cell->get_pointer_value().get();
        v->decrement_owner_count(LOC);
      }

   new (cell)   CharCell(Unicode(new_char));
}
//-----------------------------------------------------------------------------

/// val[idx]←new_int
void
set_int(int64_t new_int, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())
      {
        Value * v = cell->get_pointer_value().get();
        v->decrement_owner_count(LOC);
      }

   new (cell)   IntCell(new_int);
}
//-----------------------------------------------------------------------------

/// val[idx]←new_double
void
set_double(APL_Float new_double, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())
      {
        Value * v = cell->get_pointer_value().get();
        v->decrement_owner_count(LOC);
      }

   new (cell)   FloatCell(new_double);
}
//-----------------------------------------------------------------------------

/// val[idx]←new_complex
void
set_complex(APL_Float new_real, APL_Float new_imag, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())
      {
        Value * v = cell->get_pointer_value().get();
        v->decrement_owner_count(LOC);
      }

   new (cell)   ComplexCell(new_real, new_imag);
}
//-----------------------------------------------------------------------------
void
set_value(APL_value new_value, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())
      {
        Value * v = cell->get_pointer_value().get();
        v->decrement_owner_count(LOC);
      }

   if (new_value->is_simple_scalar())   // e.g. ⊂5 is 5
      {
        cell->init(new_value->get_ravel(0), *val, LOC);
      }
   else if (new_value->is_scalar())     // e.g. ⊂⊂5 is ⊂5
      {
        const Cell & src = new_value->get_ravel(0);
        if (!src.is_pointer_cell())   DOMAIN_ERROR;
        Value_P sub = src.get_pointer_value()->clone(LOC);
        new (cell)   PointerCell(sub.get(), *val);
      }
   else
      {
        Value_P sub = new_value->clone(LOC);
        new (cell)   PointerCell(sub.get(), *val);
      }
}

/******************************************************************************
   5. other
 */

LIBAPL_error
apl_exec(const char* line)
{ 
UTF8_string line_utf8(line);
UCS_string line_ucs(line_utf8);
const StateIndicator * si = Workspace::SI_top();
  Command::process_line(line_ucs);
   if (si == Workspace::SI_top())   return LAE_NO_ERROR;

   si = Workspace::SI_top_error();
   if (si == 0)   return LAE_UNKNOWN_ERROR;
   return LIBAPL_error(StateIndicator::get_error(si).get_error_code());
} 
//-----------------------------------------------------------------------------
LIBAPL_error
apl_exec_ucs(const unsigned int * line_ucs)
{ 
UCS_string line;
   line.reserve(200);
   while (*line_ucs)   line.append(Unicode(*line_ucs++));

const StateIndicator * si = Workspace::SI_top();
  Command::process_line(line);
   if (si == Workspace::SI_top())   return LAE_NO_ERROR;

   si = Workspace::SI_top_error();
   if (si == 0)   return LAE_UNKNOWN_ERROR;
   return LIBAPL_error(StateIndicator::get_error(si).get_error_code());
}
//-----------------------------------------------------------------------------
const char *
apl_command(const char * command)
{
UTF8_string command_utf8(command);
UCS_string command_ucs(command_utf8);

ostringstream out;
  Command::do_APL_command(out, command_ucs);

  return strndup(out.str().data(), out.str().size());
}
//-----------------------------------------------------------------------------
const unsigned int *
apl_command_ucs(const unsigned int * command)
{
UCS_string command_ucs;
   command_ucs.reserve(200);
   while (*command)   command_ucs.append(Unicode(*command++));

ostringstream out;
  Command::do_APL_command(out, command_ucs);

UTF8_string result_utf8(out.str().c_str());
   if (result_utf8.size() == 0)   return 0;   // no output

UCS_string result_ucs(result_utf8);

unsigned int * ret = reinterpret_cast<unsigned int *>
                     (malloc((result_ucs.size() + 1) * sizeof(int)));
   loop(l, result_ucs.size())   ret[l] = result_ucs[l];
   ret[result_ucs.size()] = 0;
   return ret;
}
//-----------------------------------------------------------------------------
int
get_owner_count(APL_value val)
{
   return val->get_owner_count();
}
//-----------------------------------------------------------------------------
APL_function
get_function_ucs(const unsigned int * name, APL_function * L, APL_function * R)
{
UCS_string function_ucs;
   function_ucs.reserve(40);
   while (*name)   function_ucs.append(Unicode(*name++));

   if (function_ucs.size() == 0)   return 0;   // empty name

Tokenizer  tokenizer(PM_EXECUTE, LOC, false);
Token_string tos;
   if (tokenizer.tokenize(function_ucs, tos) != E_NO_ERROR)   return 0;

   // resolve user defined names to user defined functions
   //
   for (int j = 0; j < int(tos.size()); ++j)
       {
        if (tos[j].get_ValueType() == TV_SYM)   // user defined function
           {
             Symbol * sym = tos[j].get_sym_ptr();
             if (sym == 0)   return 0;
             sym->resolve(tos[j], false);
           }
       }

   if (tos.size() == 1)   // single function
      {
        if (L)   return 0;   // but fun and operator was requested

        switch(tos[0].get_Class())
           {
             case TC_FUN0:
             case TC_FUN2:
             case TC_OPER1:
             case TC_OPER2: return tos[0].get_function();
             default:       return 0;   // error
           }
      }
   else if (tos.size() == 2)   // got left function and operator
      {
        if (!L)   return 0;   // but only left fun was requested
        if (R)    return 0;   // but right fun is missing

        switch(tos[1].get_Class())
           {
             case TC_OPER1: *L = tos[0].get_function();   break;
             default:       return 0;   // error
           }

        switch(tos[0].get_Class())
           {
             case TC_FUN12:
             case TC_OPER1: return tos[1].get_function();
             default:       return 0;
           }
      }
   else if (tos.size() == 2)   // got left and right functions and operator
      {
        if (!L)   return 0;   // but only fun was requested
        if (!R)   return 0;   // but only fun was requested

        switch(tos[0].get_Class())
           {
             case TC_FUN12:
             case TC_OPER1: *R = tos[0].get_function();   break;
             default:       return 0;   // error
           }

        switch(tos[2].get_Class())
           {
             case TC_FUN12:
             case TC_OPER1: *L = tos[0].get_function();   break;
             default:       return 0;   // error
           }

        switch(tos[1].get_Class())
           {
             case TC_OPER2: return tos[1].get_function();
             default:       return 0;
           }
      }

   return 0;
}
//-----------------------------------------------------------------------------
void
print_ucs(FILE * out, const unsigned int * string_ucs)
{
UCS_string ucs;
   ucs.reserve(200);
   while (*string_ucs)   ucs.append(Unicode(*string_ucs++));

UTF8_string utf8(ucs);
   fprintf(out, "%s", utf8.c_str());
}
//-----------------------------------------------------------------------------
APL_value
get_var_value(const char * var_name, const char * loc)
{
UTF8_string var_name_utf8(var_name);
UCS_string var_name_ucs(var_name_utf8);
Symbol * symbol = Workspace::lookup_existing_symbol(var_name_ucs);
   if (symbol == 0)                       return 0;   // unknown name
   if (symbol->get_nc() != NC_VARIABLE)   return 0;   // name is not a variable

Value_P Z = symbol->get_value();
   if (!Z)                              return 0;

   Z.get()->increment_owner_count(loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
int
set_var_value(const char * var_name, const APL_value new_value,
              const char * loc)
{
UTF8_string var_name_utf8(var_name);
UCS_string var_name_ucs(var_name_utf8);

   // check name...
   //
   if (!Avec::is_quad(var_name_ucs[0]) &&
       !Avec::is_first_symbol_char(var_name_ucs[0]))   return 1;

   loop(s, var_name_ucs.size())
      {
        if (!Avec::is_symbol_char(var_name_ucs[s]))   return 2;
      }

Symbol * symbol = Workspace::lookup_symbol(var_name_ucs);
   if (symbol == 0)   return 3;   // unknown name

   if (new_value == 0)   return 0;   // only test var_name

  if (symbol->get_nc() != NC_VARIABLE &&
      symbol->get_nc() != NC_UNUSED_USER_NAME)   return 4;

Value_P B(new_value, loc);
   symbol->assign(B, true, loc); 
   return 0;   // ok
}
//-----------------------------------------------------------------------------
void
print_value(const APL_value value, FILE * file)
{
stringstream out;
   value->print(out);

const string st = out.str();
   fwrite(st.data(), 1, st.size(), file);
}
//-----------------------------------------------------------------------------
char *
print_value_to_string(const APL_value value)
{
stringstream out;
   value->print(out);

const string st = out.str();
   return strndup(st.data(), st.size());
}
//-----------------------------------------------------------------------------
ostream &
print_value(const APL_value value, ostream & out)
{
   value->print(out);
   return out;
}
//-----------------------------------------------------------------------------
int
UTF8_to_Unicode(const char * utf, int * length)
{
int len = 0;
const Unicode uni = UTF8_string::toUni(utf8P(utf), len, false);
   if (length)   *length = len;
   return uni;
}
//-----------------------------------------------------------------------------
void
Unicode_to_UTF8(int uni, char * dest, int * length)
{
UCS_string ucs(1, Unicode(uni));
UTF8_string utf8(ucs);
   for (int d = 0; d < int(utf8.size()); ++d)   dest[d] = utf8[d];
   dest[utf8.size()] = 0;
   if (length)   *length = utf8.size();
}
//-----------------------------------------------------------------------------
extern void init_1(const char * argv0, bool log_startup);
extern void init_2(bool log_startup);

void
init_libapl(const char * progname, int log_startup)
{
   uprefs.safe_mode = true;
   uprefs.user_do_svars = false;
   uprefs.system_do_svars = false;
   uprefs.requested_id = 2000;

   init_1(progname, log_startup);

   uprefs.read_config_file(true,  log_startup);   // in /etc/gnu-apl.d/
   uprefs.read_config_file(false, log_startup);   // in $HOME/.config/gnu_apl/

   init_2(log_startup);
}
//-----------------------------------------------------------------------------
extern DiffOut DOUT_filebuf;
extern DiffOut UERR_filebuf;
extern ErrOut  CERR_filebuf;

int
expand_LF_to_CRLF(int on)
{
const int ret = DOUT_filebuf.LF_to_CRLF(on != 0);
                UERR_filebuf.LF_to_CRLF(on != 0);
                CERR_filebuf.LF_to_CRLF(on != 0);

   return ret;
}
//-----------------------------------------------------------------------------
get_line_from_user_cb * glfu = 0;

void
libapl_glfu(LineInputMode mode, const UCS_string & prompt,
                  UCS_string & line, bool & eof, LineHistory & hist)
{
UTF8_string prompt_utf8(prompt);
const char * user_input = glfu(mode, prompt_utf8.c_str());
   if (user_input)
      {
         UTF8_string user_input_utf8(user_input);
        line = UCS_string(user_input_utf8);
      }
   else
      {
        eof = true;
      }
}

get_line_from_user_cb * 
install_get_line_from_user_cb(get_line_from_user_cb * new_callback)
{
get_line_from_user_cb * ret = glfu;
   glfu = new_callback;
   if (new_callback)    InputMux::install_get_line_callback(&libapl_glfu);
   else                 InputMux::install_get_line_callback(0);
   return ret;
}
//-----------------------------------------------------------------------------
APL_value
eval__fun(APL_function fun)
{
   try { Token tZ = fun->eval_();
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__A_fun_B(APL_value vA, APL_function fun, APL_value vB)
{
   try { Value_P A(vA, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_AB(A, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__A_L_oper_B(APL_value vA, APL_function fL, APL_function fun, APL_value vB)
{
   try { Value_P A(vA, LOC);
         Token L(fL->get_token());
         Value_P B(vB, LOC);
         Token tZ = fun->eval_ALB(A, L, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__A_fun_X_B(APL_value vA, APL_function fun, APL_value vX, APL_value vB)
{
   try { Value_P A(vA, LOC);
         Value_P X(vX, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_AXB(A, X, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__A_L_oper_R_B(APL_value vA, APL_function fL, APL_function fun,
                   APL_function fR, APL_value vB)
{
   try { Value_P A(vA, LOC);
         Token L(fL->get_token());
         Token R(fR->get_token());
         Value_P B(vB, LOC);
         Token tZ = fun->eval_ALRB(A, L, R, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__A_L_oper_X_B(APL_value vA, APL_function fL, APL_function fun,
                   APL_value vX, APL_value vB)
{
   try { Value_P A(vA, LOC);
         Token L(fL->get_token());
         Value_P X(vX, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_ALXB(A, L, X, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__A_L_oper_R_X_B(APL_value vA, APL_function fL, APL_function fun,
                     APL_function fR, APL_value vX, APL_value vB)
{
   try { Value_P A(vA, LOC);
         Token L(fL->get_token());
         Token R(fR->get_token());
         Value_P X(vX, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_ALRXB(A, L, R, X, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__fun_B(APL_function fun, APL_value vB)
{
   try { Value_P B(vB, LOC);
         Token tZ = fun->eval_B(B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__L_oper_B(APL_function L, APL_function oper, APL_value vB)
{
   try { Value_P B(vB, LOC);
         Token tFUN(TOK_FUN2, L);
         Token tZ = oper->eval_LB(tFUN, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__fun_X_B(APL_function fun, APL_value vX, APL_value vB)
{
   try { Value_P X(vX, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_XB(X, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__L_oper_R_B(APL_function fL, APL_function fun, APL_function fR,
                 APL_value vB)
{
   try { Token L(fL->get_token());
         Token R(fR->get_token());
         Value_P B(vB, LOC);
         Token tZ = fun->eval_LRB(L, R, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__L_oper_X_B(APL_function fL, APL_function fun, APL_value vX, APL_value vB)
{
   try { Token L(fL->get_token());
         Value_P X(vX, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_LXB(L, X, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------
APL_value
eval__L_oper_R_X_B(APL_function fL, APL_function fun, APL_function fR,
                   APL_value vX, APL_value vB)
{
   try { Token L(fL->get_token());
         Token R(fR->get_token());
         Value_P X(vX, LOC);
         Value_P B(vB, LOC);
         Token tZ = fun->eval_LRXB(L, R, X, B);
         if (tZ.get_tag() != TOK_SI_PUSHED)   return tZ.extract_and_keep(LOC);

         Token result = Workspace::SI_top()->run();
         return result.extract_and_keep(LOC);
       } catch (...)   { return 0; }
}
//-----------------------------------------------------------------------------

