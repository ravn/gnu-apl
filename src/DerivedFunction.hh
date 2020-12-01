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

#ifndef __DERIVED_FUNCTION__DEFINED__
#define __DERIVED_FUNCTION__DEFINED__

#include "Error.hh"
#include "Function.hh"
#include "Output.hh"

//-----------------------------------------------------------------------------
/** an operator bound to one or more functions. The bound operator is then
    another operator (e.g. (+.) or a function (e.g. (+.×) or (+/).
 **/
/// An APL operator bound to its function argument(s)
class DerivedFunction : public Function
{
public:
   /// Constructor for DerivedFunctionCache
   DerivedFunction() : Function(TOK_FUN0)   {}

   /// Constructor (dyadic operator)
   DerivedFunction(Token & lf, Function_P dyop, Token & rf, const char * loc);

   /// Constructor (dyadic operator with axis: fun ⍤[] rval)
   DerivedFunction(Token & lf, Function_P dyop, Value_P X, Token & rval,
                   const char * loc);

   /// Constructor (monadic operator, no axis)
   DerivedFunction(Token & lfun, Function_P monop, const char * loc);

   /// Constructor (monadic operator, with axis)
   DerivedFunction(Token & lf, Function_P monop, Value_P X, const char * loc);

   /// Constructor (function with axis)
   DerivedFunction(Function_P func, Value_P X, const char * loc);

   /// overloaded Function::print();
   virtual ostream & print(ostream & out) const;

   /// overloaded Function::is_derived();
   virtual bool is_derived() const
      { return true; }

  Function_P get_LO() const
      { return left_arg.get_function(); }

   // return the value (if any) bound to an operator (that allows it)
   Value_P get_bound_LO_value() const
      {
        if (left_arg.is_apl_val())   return left_arg.get_apl_val();
        return Value_P();
      }

   Function_P get_OPER() const
      { return oper; }

   Function_P get_RO() const
      {   if (right_fun.get_tag() == TOK_VOID)   return 0;
          return right_fun.get_function();
      }

   const Value * get_AXIS() const
      { return axis.get(); }

   /// Overloaded Function::has_result();
   virtual bool has_result() const;

protected:
   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// overloaded Function::eval_B();
   virtual Token eval_B(Value_P B) const;

   /// overloaded Function::eval_XB();
   virtual Token eval_XB(Value_P X, Value_P B) const;

   /// overloaded Function::eval_AB();
   virtual Token eval_AB(Value_P A, Value_P B) const;

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B) const;

   /// overloaded Function::may_push_SI()
   virtual bool may_push_SI() const
      { return   oper->may_push_SI()
        || (left_arg .is_function() && left_arg .get_function()->may_push_SI())
        || (right_fun.is_function() && right_fun.get_function()->may_push_SI());
      }

   /// overloaded Function::locate_X()
   virtual const Value_P * locate_X() const
      { return (!axis) ? 0 : &axis; }

   /// the function (to the left of the operator)
   Token left_arg;

   /// the monadic operator (to the right of the function)
   Function_P oper;

   /// the function on the right of the (dyadic) operator (if any)
   Token right_fun;

   /// the axis for \b mon_oper, or 0 if no axis
   Value_P axis;
};
//=============================================================================
/// a small cache for storing a few DerivedFunction objects
class DerivedFunctionCache
{
public:
   /// constructor: create empty FunOper cache
   DerivedFunctionCache();

   /// destructor
   ~DerivedFunctionCache();

   const DerivedFunction & operator [](size_t i) const
      { Assert(i < idx);    return cache[i]; }

   /// return the number of items in the cache
   size_t size() const
      { return idx; }

   /// reset (clear) the cache
   void reset();

   /// return the last cache entry and increment \b idx. To be used with
   /// placement new.
   DerivedFunction * get(const char * loc);

protected:
   /// a cache for derived functions
   DerivedFunction cache[MAX_FUN_OPER];

   /// the number of elements in \b cache
   size_t idx;
};
//-----------------------------------------------------------------------------
#endif // __DERIVED_FUNCTION__DEFINED__
