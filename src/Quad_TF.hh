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

#ifndef __QUAD_TF_HH_DEFINED__
#define __QUAD_TF_HH_DEFINED__

#include "QuadFunction.hh"

//-----------------------------------------------------------------------------
/**
   The system function Quad-TF (Transfer Form).
 */
class Quad_TF : public QuadFunction
{
public:
   /// Constructor.
   Quad_TF() : QuadFunction(TOK_Quad_TF) {}

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   static Quad_TF * fun;          ///< Built-in function.
   static Quad_TF  _fun;          ///< Built-in function.

   /// return true if val contains an 1⎕TF or 2⎕TF record
   static bool is_inverse(const UCS_string & maybe_name);

   /// return B in transfer format 1 (old APL format)
   static Value_P tf1(const UCS_string & symbol_name);

   /// return B in transfer format 2 (new APL2 format)
   static Token tf2(const UCS_string & symbol_name);

   /// return B in transfer format 3 (APL2 CDR format)
   static Value_P tf3(const UCS_string & symbol_name);

   /// append \b shape in tf2_format to \b ucs. Return true if (A⍴ was appended
   static bool tf2_shape(UCS_string & ucs, const Shape & shape);

   /// append ravel of \b value in tf2_format to \b ucs. Return true on error
   static bool tf2_value(int level, UCS_string & ucs, Value_P value);

   /// try inverse ⎕TF2 of ucs, set \b new_var_or_fun if successful
   static UCS_string tf2_inv(const UCS_string & ravel);

   /// store B in transfer format 2 (new APL format) into \b ucs
   static void tf2_fun_ucs(UCS_string & ucs, const UCS_string & fun_name,
                           const Function & fun);

   /// store simple character vector \b vec in \b ucs, either as
   /// 'xxx' or as (UCS nn nnn ...)
   static void tf2_char_vec(UCS_string & ucs, const UCS_string & vec);

   /// undo ⎕UCS() created by tf2_char_vec
   static UCS_string no_UCS(const UCS_string & ucs);

protected:
   /// return B in transfer format 1 (old APL format) for a variable
   static Value_P tf1(const UCS_string & var_name, Value_P val);

   /// return B in transfer format 1 (old APL format) for a function
   static Value_P tf1(const UCS_string & fun_name, const Function & fun);

   /// return inverse  transfer format 1 (old APL format) for a variable
   static Value_P tf1_inv(const UCS_string &  ravel);

   /// return B in transfer format 2 (new APL format) for a variable
   static Token tf2_var(const UCS_string & var_name, Value_P val);

   /// simplify tos by removing UCS nnn etc. return the number of errors.
   static int tf2_simplify(Token_string & tos);

   /// replace ⎕UCS n... by the corresponding Unicodes,
   /// return the number of errors
   static int tf2_remove_UCS(Token_string & tos);

   /// replace shape ⍴ value by a reshaped value
   static int tf2_remove_RHO(Token_string & tos, bool & progress);

   /// replace ( value ) by value
   static int tf2_remove_parentheses(Token_string & tos, bool & progress);

   /// replace value1 value2 by value
   static int tf2_glue(Token_string & tos, bool & progress);

   /// return B in transfer format 2 (new APL format) for a function
   static UCS_string tf2_fun(const UCS_string & fun_name, const Function & fun);
};
//-----------------------------------------------------------------------------
#endif // __QUAD_TF_HH_DEFINED__
