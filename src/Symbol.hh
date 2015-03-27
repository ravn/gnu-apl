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

#ifndef __SYMBOL_HH_DEFINED__
#define __SYMBOL_HH_DEFINED__

#include <stdint.h>

#include "ErrorCode.hh"
#include "NamedObject.hh"
#include "Parser.hh"
#include "SystemLimits.hh"
#include "Svar_DB.hh"

class IndexExpr;
class UserFunction;

//-----------------------------------------------------------------------------
/// An entry of the value stack for \b this symbol. The value stack
/// is pushed/poped when the symbol is localized on entry/return of
/// a user defined function.
struct ValueStackItem
{
   /// constructor: ValueStackItem for an unused symbol
   ValueStackItem() : name_class(NC_UNUSED_USER_NAME)
      { memset(&sym_val, 0, sizeof(sym_val)); }

   /// constructor: ValueStackItem for a label (function line)
   ValueStackItem(Function_Line lab) : name_class(NC_LABEL)
      { sym_val.label = lab; }

   /// constructor: ValueStackItem for a variable
   ValueStackItem(Value_P val)
   : apl_val(val),
     name_class(NC_VARIABLE)
   {}

   /// constructor: ValueStackItem for a shared variable
   ValueStackItem(SV_key key) : name_class(NC_SHARED_VAR)
      { sym_val.sv_key = key; }

   /// reset \b this ValueStackItem to being unused
   void clear()
      {
        memset(&sym_val, 0, sizeof(sym_val));
        name_class = NC_UNUSED_USER_NAME;
      }

   /// the possible values of a symbol
   union _sym_val
      {
        Function *    function;   ///< if \b Symbol is a function
        Function_Line label;      ///< if \b Symbol is a label
        SV_key        sv_key;     ///< if \b Symbol is a shared variable
      };

   /// the (current) value of this symbol (unless variable)
   _sym_val sym_val;

   /// the (current) value of this symbol (if variable)
   Value_P apl_val;

   /// the (current) name class (like ⎕NC, unless shared variable)
   NameClass name_class;
};
//-----------------------------------------------------------------------------
/**
    Base class for variables and user defined functions.
 */
class Symbol : public NamedObject
{
   friend class SymbolTable;

public:
   /// create a system symbol with Id \b id
   Symbol(ID::Id id);

   /// create a symbol with name \b ucs
   Symbol(const UCS_string & ucs, ID::Id id);

   /// explicit destructor
   void destruct()
      {
        clear_vs();
        vector<ValueStackItem> x;
        value_stack.swap(x);
        name.destruct();
      }

   /// destructor
   virtual ~Symbol()
      { clear_vs(); }

   /// List \b this \b Symbol ( for )VARS, )FNS )
   ostream & list(ostream & out);

   /// write this symbol in )OUT format to file \b out
   void write_OUT(FILE * out, uint64_t & seq) const;

   /// set \b token according to the current NC/sym_val of \b this \b Symbol
   virtual void resolve(Token & token, bool left);

   /// set \b token according to the current NC/sym_val of \b this shared var
   void resolve_shared_variable(Token & token);

   /// resolve a variable name for an assignment
   Token resolve_lv(const char * loc);

   /// return the token class of \b this \b Symbol WITHOUT calling resolve()
   TokenClass resolve_class(bool left);

   /// set current NameClass of this Symbol to NC_UNUSED_USER_NAME and remove
   /// any values associated with this symbol
   virtual int expunge();

   /// Set current NameClass of this Symbol to \b nc
   void set_nc(NameClass nc);

   /// share variable with \b proc
   void share_var(SV_key key);

   /// unshare variable
   SV_Coupling unshare_var();

   /// clear the value stack of \b this symbol
   void clear_vs();

   /// Set current NameClass of this Symbol to \b nc and function fun
   void set_nc(NameClass nc, Function * fun);

   /// Compare name of \b this value with \b other
   int compare(const Symbol & other) const
       { return name.compare(other.name); }

   /// return true iff this variable is read-only
   /// (overloaded by RO_SystemVariable)
   virtual bool is_readonly() const   { return false; }

   /// Assign \b value to \b this \b Symbol
   virtual void assign(Value_P value, const char * loc);

   /// Assign \b value to \b this \b Symbol (which is a shared variable)
   void assign_shared_variable(Value_P value, const char * loc);

   /// Indexed (multi-dimensional) assign \b value to \b this \b Symbol
   virtual void assign_indexed(IndexExpr & index, Value_P value);

   /// Indexed (one-dimensional) assign \b value to \b this \b Symbol
   virtual void assign_indexed(Value_P index, Value_P value);

   /// assign lambda, eg. V←{ ... }
   virtual void assign_named_lambda(Function * lambda, const char * loc);

   /// Print \b this \b Symbol to \b out
   virtual ostream & print(ostream & out) const;

   /// Print \b this \b Symbol and its stack to \b out
   ostream & print_verbose(ostream & out) const;

   /// Pop latest entry from the stack of \b this \b Symbol
   virtual void pop();

   /// Push an undefined entry onto the stack of \b this \b Symbol
   virtual void push();

   /// Push a label onto the stack of \b this \b Symbol
   virtual void push_label(Function_Line label);

   /// Push a function onto the stack of \b this \b Symbol
   virtual void push_function(Function * function);

   /// Push an APL value onto the stack of \b this \b Symbol
   virtual void push_value(Value_P value);

   /// return the depth (global == 0) of \b ufun on the stack. Use the largest
   /// depth if ufun is pushed multiple times
   int get_ufun_depth(const UserFunction * ufun);

   /// return the current APL value (or throw a VALUE_ERROR)
   virtual Value_P get_apl_value() const;

   /// return the first Cell of this value without creating a value
   const Cell * get_first_cell() const;

   /// return true, iff this Symbol can be assigned
   bool can_be_assigned() const;

   /// return the current SV_key (or throw a VALUE_ERROR)
   SV_key get_SV_key() const;

   /// set the current SV_key
   void set_SV_key(SV_key key);

   /// Return true, iff this Symbol is erased
   bool is_erased() const
   { return erased; }

   /// set \b erased to \b on_off
   void set_erased(bool on_off)
      { erased = on_off; }

   /// Return the current function (or throw a VALUE_ERROR)
   virtual const Function * get_function() const;

   /// The name of \b this \b Symbol
   virtual UCS_string get_name() const   { return name; }

   /// overloaded NamedObject::get_function()
   virtual Function * get_function();

   /// overloaded NamedObject::get_value()
   virtual Value_P get_value();

   /// return a reason why this symbol cant become a defined function
   const char * cant_be_defined() const;

   /// overloaded NamedObject::get_symbol()
   virtual Symbol * get_symbol() 
      { return this; }

   /// overloaded NamedObject::get_symbol()
   virtual const Symbol * get_symbol() const
      { return this; }

   /// store the attributes (as per ⎕AT) of symbol at dest...
   virtual void get_attributes(int mode, Cell * dest) const;

   /// return the size of the value stack
   const int value_stack_size() const
      { return value_stack.size(); }

   /// return the top-most item on the value stack
   const ValueStackItem * top_of_stack() const
      { return value_stack.size() ? &value_stack.back() : 0; }

   /// return the top-most item on the value stack
   ValueStackItem * top_of_stack()
      { return value_stack.size() ? &value_stack.back() : 0; }

   /// return the idx'th item on stack (higher index = newer item)
   const ValueStackItem & operator [](int idx) const
      { return value_stack[idx]; }

   /// return the idx'th item on stack (higher index = newer item)
   ValueStackItem & operator [](int idx)
      { return value_stack[idx]; }

   /// set a callback function for symbol events
   void set_monitor_callback(void (* callback)(const Symbol &, Symbol_Event ev))
      { monitor_callback = callback; }

   /// clear the marked flag of all entries
   void unmark_all_values() const;

   /// print variables owning value
   int show_owners(ostream & out, const Value & value) const;

   /// perform a vector assignment (like (A B C)←1 2 3) for variables in
   /// \b symbols with values \b values
   static void vector_assignment(Symbol * * symbols, int count, Value_P values);

   /// dump this symbol to out
   void dump(ostream & out) const;

protected:
   /// Compare the name of \b this \b Symbol with \b ucs
   bool equal(const UCS_string & ucs) const
      { return (name.compare(ucs) == 0); }

   /// The next Symbol with the same hash value as \b this \b Symbol
   Symbol * next;

   /// The name of \b this \b Symbol
   UCS_string name;

   /// \b True if \b this \b Symbol is/was erased
   bool erased;

   /// called on symbol events (if non-0)
   void (*monitor_callback)(const Symbol &, Symbol_Event sev);

   /// the value stack of \b this \b Symbol
   vector<ValueStackItem> value_stack;
};
//-----------------------------------------------------------------------------
/// lambda result λ
class LAMBDA : public Symbol
{
public:
   /// constructor
   LAMBDA()
   : Symbol(ID::LAMBDA)
   {}

   /// destroy variable
   void destroy_var() {}
};
//-----------------------------------------------------------------------------
/// lambda variable ⍺
class ALPHA : public Symbol
{
public:
   /// constructor
   ALPHA()
   : Symbol(ID::ALPHA)
   {}

   /// destroy variable
   void destroy_var() {}
};
//-----------------------------------------------------------------------------
/// lambda variable ⍶
class ALPHA_U : public Symbol
{
public:
   /// constructor
   ALPHA_U()
   : Symbol(ID::ALPHA_U)
   {}

   /// destroy variable
   void destroy_var() {}
};
//-----------------------------------------------------------------------------
/// lambda variable χ
class CHI : public Symbol
{
public:
   /// constructor
   CHI()
   : Symbol(ID::CHI)
   {}

   /// destroy variable
   void destroy_var() {}
};
//-----------------------------------------------------------------------------
/// lambda variable ⍵
class OMEGA : public Symbol
{
public:
   /// constructor
   OMEGA()
   : Symbol(ID::OMEGA)
   {}

   /// destroy variable
   void destroy_var() {}
};
//-----------------------------------------------------------------------------
/// lambda variable ⍹
class OMEGA_U : public Symbol
{
public:
   /// constructor
   OMEGA_U()
   : Symbol(ID::OMEGA_U)
   {}

   /// destroy variable
   void destroy_var() {}
};
//-----------------------------------------------------------------------------

#endif // __SYMBOL_HH_DEFINED__
