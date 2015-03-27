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

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "Error.hh"
#include "Function.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "Parser.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "Value.icc"

//-----------------------------------------------------------------------------
void
Function::get_attributes(int mode, Cell * dest) const
{
   switch(mode)
      {
        case 1: // valences
                new (dest + 0) IntCell(has_result() ? 1 : 0);
                new (dest + 1) IntCell(get_fun_valence());
                new (dest + 2) IntCell(get_oper_valence());
                return;

        case 2: // creation time (7⍴0 for system functions)
                {
                  const YMDhmsu created(get_creation_time());
                  new (dest + 0) IntCell(created.year);
                  new (dest + 1) IntCell(created.month);
                  new (dest + 2) IntCell(created.day);
                  new (dest + 3) IntCell(created.hour);
                  new (dest + 4) IntCell(created.minute);
                  new (dest + 5) IntCell(created.second);
                  new (dest + 6) IntCell(created.micro/1000);
                }
                return;

        case 3: // execution properties
                new (dest + 0) IntCell(get_exec_properties()[0]);
                new (dest + 1) IntCell(get_exec_properties()[1]);
                new (dest + 2) IntCell(get_exec_properties()[2]);
                new (dest + 3) IntCell(get_exec_properties()[3]);
                return;

        case 4: // 4 ⎕DR functions is always 0 0
                new (dest + 0) IntCell(0);
                new (dest + 1) IntCell(0);
                return;
      }

   Assert(0 && "Not reached");
}
//-----------------------------------------------------------------------------
Token
Function::eval_()
{
   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   FIXME;
}
//-----------------------------------------------------------------------------
Token
Function::eval_B(Value_P B)
{
   Log(LOG_verbose_error)   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   VALENCE_ERROR;
}
//-----------------------------------------------------------------------------
Token
Function::eval_AB(Value_P A, Value_P B)
{
   Log(LOG_verbose_error)   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   VALENCE_ERROR;
}
//-----------------------------------------------------------------------------
Token
Function::eval_LB(Token & LO, Value_P B)
{
   Log(LOG_verbose_error)   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   VALENCE_ERROR;
}
//-----------------------------------------------------------------------------
Token
Function::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   Log(LOG_verbose_error)   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   VALENCE_ERROR;
}
//-----------------------------------------------------------------------------
Token
Function::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   Log(LOG_verbose_error)   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   VALENCE_ERROR;
}
//-----------------------------------------------------------------------------
Token
Function::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   Log(LOG_verbose_error)   CERR << get_name() << "::" << __FUNCTION__
        << "() called (overloaded variant not yet implemented?)" << endl;
   VALENCE_ERROR;
}
//-----------------------------------------------------------------------------
Fun_signature
Function::get_signature() const
{
int sig = SIG_FUN;
   if (has_result())   sig |= SIG_Z;
   if (has_axis())     sig |= SIG_X;

   if (get_oper_valence() == 2)   sig |= SIG_RO;
   if (get_oper_valence() >= 1)   sig |= SIG_LO;

   if (get_fun_valence() == 2)    sig |= SIG_A;
   if (get_fun_valence() >= 1)    sig |= SIG_B;

   return (Fun_signature)sig;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const Function & fun)
{
   fun.print(out);
   return out;
}
//-----------------------------------------------------------------------------

