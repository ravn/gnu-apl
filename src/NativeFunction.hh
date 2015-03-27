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

#ifndef __NATIVE_FUNCTION_HH_DEFINED__
#define __NATIVE_FUNCTION_HH_DEFINED__

#include <sys/types.h>

#include "Executable.hh"
#include "Function.hh"
#include "Error.hh"

//-----------------------------------------------------------------------------
/**
    A user-defined function.
 */
class NativeFunction : public Function
{
public:
   /// ⎕FX a native funtion from a shared library (dlopen) handle
   static NativeFunction * fix(const UCS_string & so_name,
                               const UCS_string & apl_name);

   /// close all shared libs
   static void cleanup();

   /// kind of native function
   enum NativeCategory
      {
        NCAT_F0,   ///< niladic function
        NCAT_F12,   ///< nomadic (monadic or dyadic) function
        NCAT_OP1,   ///< monadic operator
        NCAT_OP2,   ///< dyadic operator
      };

   /// load shared lib for emacs mode, return error info on failure
   static UCS_string load_emacs_library(const char * emacs_arg);

protected:
   /// constructor (only fix() may call it)
   NativeFunction(const UCS_string & so_name, const UCS_string & apl_name);

   /// destructor: close so_handle
   ~NativeFunction();

   /// open .so file in one of several directories. On success set handle 
   /// and update so_path.
   static void * open_so_file(UCS_string & t4, UCS_string & so_path);

   /// try to open one .so file
   static void * try_one_file(const char * filename, UCS_string & t4);

   /// overloaded Function::is_native()
   virtual bool is_native() const   { return true; }

   /// overloaded Function::has_result()
   virtual bool has_result() const;

   /// overloaded Function::is_operator()
   virtual bool is_operator() const;

   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// Overloaded Function::print()
   virtual ostream & print(std::ostream&) const;

   /// Overloaded Function::canonical()
   virtual UCS_string canonical(bool with_lines) const;

   /// Overloaded Function::eval_()
   virtual Token eval_();

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_LB()
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_ALB()
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   /// Overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Overloaded Function::eval_LXB()
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALXB()
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_LRXB()
   virtual Token eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALRXB()
   virtual Token eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X,
                            Value_P B);

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_identity_fun()
   virtual Token eval_identity_fun(Value_P B, Axis axis);

   /// Overloaded Function::destroy()
   virtual void destroy();

   UCS_string get_name() const
      { return name; }

   /// dl_open() handle of shared library
   void * handle;

   /// APL name of the function
   UCS_string name;

   /// the right argument B of of A ⎕FX B before expanding it to a path
   UCS_string original_so_path;

   /// library file path of the function
   UCS_string so_path;

   /// true if the shared lib was successfully opened
   bool valid;

   /// type of function (niladic / nomadic / moadic operator / dyadic operator)
   Fun_signature signature;

   /// all native functions currently active
   static vector<NativeFunction *>valid_functions;

   typedef Value_P Vr;   ///< shortcut for Value_P

   typedef Function & Fr;   ///< shortcut for Function &

#define Th const NativeFunction *

   /// pointer to function eval_() in shared library
   Token (*f_eval_)        (                                Th);

   /// pointer to function eval_B() in shared library
   Token (*f_eval_B)       (                          Vr B, Th);

   /// pointer to function eval_AB() in shared library
   Token (*f_eval_AB)      (Vr A,                     Vr B, Th);

   /// pointer to function eval_LB() in shared library
   Token (*f_eval_LB)      (      Fr LO,              Vr B, Th);

   /// pointer to function eval_ALB() in shared library
   Token (*f_eval_ALB)     (Vr A, Fr LO,              Vr B, Th);

   /// pointer to function eval_LRB() in shared library
   Token (*f_eval_LRB)     (      Fr LO, Fr RO,       Vr B, Th);

   /// pointer to function eval_ALRB() in shared library
   Token (*f_eval_ALRB)    (Vr A, Fr LO, Fr RO,       Vr B, Th);

   /// pointer to function eval_XB() in shared library
   Token (*f_eval_XB)      (                    Vr X, Vr B, Th);

   /// pointer to function eval_AXB() in shared library
   Token (*f_eval_AXB)     (Vr A,               Vr X, Vr B, Th);

   /// pointer to function eval_LXB() in shared library
   Token (*f_eval_LXB)     (      Fr LO,        Vr X, Vr B, Th);

   /// pointer to function eval_() in shared library
   Token (*f_eval_ALXB)    (Vr A, Fr LO,        Vr X, Vr B, Th);

   /// pointer to function eval_LRXB() in shared library
   Token (*f_eval_LRXB)    (      Fr LO, Fr RO, Vr X, Vr B, Th);

   /// pointer to function eval_ALRXB() in shared library
   Token (*f_eval_ALRXB)   (Vr A, Fr LO, Fr RO, Vr X, Vr B, Th);


   /// pointer to function eval_fill_B() in shared library
   Token (*f_eval_fill_B)  (                          Vr B, Th);

   /// pointer to function eval_fill_AB() in shared library
   Token (*f_eval_fill_AB) (Vr A,                     Vr B, Th);

   /// pointer to function eval_identity_fun() in shared library
   Token (*f_eval_ident_Bx)(Vr B,                   Axis x, Th);
#undef Th

   /// callback before library is closed
   bool (*close_fun)(Cause cause, const NativeFunction * caller);
};
//-----------------------------------------------------------------------------

#endif // __NATIVE_FUNCTION_HH_DEFINED__
