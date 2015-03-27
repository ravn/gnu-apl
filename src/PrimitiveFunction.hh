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

#ifndef __PRIMITIVE_FUNCTION_HH_DEFINED__
#define __PRIMITIVE_FUNCTION_HH_DEFINED__

#include "Common.hh"
#include "Function.hh"
#include "Performance.hh"
#include "Value.icc"
#include "Id.hh"

class ArrayIterator;
class CharCell;
class IntCell;
class CollatingCache;

//-----------------------------------------------------------------------------
/**
    Base class for the APL system functions (Quad functions and primitives
    like +, -, ...) and operators

    The individual system functions are derived from this class
 */
class PrimitiveFunction : public Function
{
public:
   /// Construct a PrimitiveFunction with \b TokenTag \b tag
   PrimitiveFunction(TokenTag tag,
                     CellFunctionStatistics * stat_AB = 0, 
                     CellFunctionStatistics * stat_B = 0)
   : Function(tag),
       statistics_AB(stat_AB),
       statistics_B(stat_B)
   {}

   /// overloaded Function::has_result()
   virtual bool has_result() const   { return true; }

   /// return the dyadic cell statistics of \b this (scalar) function
   CellFunctionStatistics *
   get_statistics_AB() const   { return statistics_AB; }

   /// return the monadic cell statistics of \b this (scalar) function
   CellFunctionStatistics *
   get_statistics_B() const   { return statistics_B; }

   /// overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

protected:
   /// overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Print the name of \b this PrimitiveFunction to \b out
   virtual ostream & print(ostream & out) const;

   /// performance statistics for eval_B()
   CellFunctionStatistics * statistics_AB;

   /// performance statistics for dyadic calls
   CellFunctionStatistics * statistics_B;

   /// a cell containing ' '
   static const CharCell c_filler;

   /// a cell containing 0
   static const IntCell n_filler;
};
//-----------------------------------------------------------------------------
/** The various non-scalar functions
 */
class NonscalarFunction : public PrimitiveFunction
{
public:
   /// Constructor
   NonscalarFunction(TokenTag tag)
   : PrimitiveFunction(tag)
   {}
};
//-----------------------------------------------------------------------------
/** System function zilde (⍬)
 */
class Bif_F0_ZILDE : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F0_ZILDE()
   : NonscalarFunction(TOK_F0_ZILDE)
   {}

   static Bif_F0_ZILDE * fun;   ///< Built-in function
   static Bif_F0_ZILDE  _fun;   ///< Built-in function

   /// overladed Function::eval_()
   virtual Token eval_();

protected:
   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return false; }
};
//-----------------------------------------------------------------------------
/** System function execute
 */
class Bif_F1_EXECUTE : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F1_EXECUTE()
   : NonscalarFunction(TOK_F1_EXECUTE)
   {}

   static Bif_F1_EXECUTE * fun;   ///< Built-in function
   static Bif_F1_EXECUTE  _fun;   ///< Built-in function

   /// execute string
   static Token execute_statement(UCS_string & statement);

   /// overladed Function::eval_B()
   virtual Token eval_B(Value_P B);

protected:
   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return true; }
};
//-----------------------------------------------------------------------------
/** System function index (⌷)
 */
class Bif_F2_INDEX : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F2_INDEX()
   : NonscalarFunction(TOK_F2_INDEX)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F2_INDEX * fun;   ///< Built-in function
   static Bif_F2_INDEX  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions partition and enclose
 */
class Bif_F12_PARTITION : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_PARTITION()
   : NonscalarFunction(TOK_F12_PARTITION)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return partition(A, B, B->get_rank() - 1); }

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_PARTITION * fun;   ///< Built-in function
   static Bif_F12_PARTITION  _fun;   ///< Built-in function

protected:
   /// enclose B
   Token enclose(Value_P B);

   /// enclose B
   Token enclose_with_axis(Value_P B, Value_P X);

   /// Partition B according to A
   Token partition(Value_P A, Value_P B, Axis axis);

   /// Copy one partition to dest
   static void copy_segment(Cell * dest, Value & dest_owner, ShapeItem h,
                            ShapeItem from, ShapeItem to,
                            ShapeItem m_len, ShapeItem l, ShapeItem len_l,
                            Value_P B);
};
//-----------------------------------------------------------------------------
/** primitive functions pick and disclose
 */
class Bif_F12_PICK : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_PICK()
   : NonscalarFunction(TOK_F12_PICK)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return disclose(B, false); }

   /// ⊃B
   static Token disclose(Value_P B, bool rank_tolerant);

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B)
      { const Shape axes_X = X->to_shape();
        return disclose_with_axis(axes_X, B, false); }

   /// ⊃[X]B
   static Token disclose_with_axis(const Shape & axes_X, Value_P B,
                                   bool rank_tolerant);

   static Bif_F12_PICK * fun;   ///< Built-in function
   static Bif_F12_PICK  _fun;   ///< Built-in function

protected:
   /// the shape of the items being disclosed
   static Shape item_shape(Value_P B, bool rank_tolerant);

   /// Pick from B according to cA and len_A. \b cell_owner is non-zero
   /// if a left-vlues is picked, (e.g (2 1⊃B)←'TR')
   static Value_P pick(const Cell * cA, ShapeItem len_A, Value_P B, 
                       APL_Float qct, APL_Integer qio, Value * cell_owner);
};
//-----------------------------------------------------------------------------
/** Comma related functions (catenate, laminate, and ravel.)
 */
class Bif_COMMA : public NonscalarFunction
{
public:
   /// Constructor
   Bif_COMMA(TokenTag tag)
   : NonscalarFunction(tag)
   {}

   /// ravel along axis, with axis being the first (⍪( or last (,) axis of B
   Token ravel_axis(Value_P X, Value_P B, Axis axis);

   /// Return the ravel of B as APL value
   static Token ravel(const Shape & new_shape, Value_P B);

   /// Catenate A and B
   static Token catenate(Value_P A, Axis axis, Value_P B);

   /// Laminate A and B
   static Token laminate(Value_P A, Axis axis, Value_P B);

   /// Prepend scalar cell_A to B along axis
   static Value_P prepend_scalar(const Cell & cell_A, Axis axis, Value_P B);

   /// Prepend scalar cell_B to A along axis
   static Value_P append_scalar(Value_P A, Axis axis, const Cell & cell_B);
};
//-----------------------------------------------------------------------------
/** primitive functions catenate, laminate, and ravel along last axis
 */
class Bif_F12_COMMA : public Bif_COMMA
{
public:
   /// Constructor
   Bif_F12_COMMA()
   : Bif_COMMA(TOK_F12_COMMA)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B)
      { return ravel_axis(X, B, B->get_rank()); }

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_COMMA * fun;   ///< Built-in function
   static Bif_F12_COMMA  _fun;   ///< Built-in function

protected:
};
//-----------------------------------------------------------------------------
/** primitive functions catenate and laminate along first axis, table
 */
class Bif_F12_COMMA1 : public Bif_COMMA
{
public:
   /// Constructor
   Bif_F12_COMMA1()
   : Bif_COMMA(TOK_F12_COMMA1)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B)
      { return ravel_axis(X, B, 0); }

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_COMMA1 * fun;   ///< Built-in function
   static Bif_F12_COMMA1  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions take and first
 */
class Bif_F12_TAKE : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_TAKE()
   : NonscalarFunction(TOK_F12_TAKE)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return Token(TOK_APL_VALUE1, first(B));}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Take from B according to ravel_A
   static Token do_take(const Shape shape_Zi, Value_P B);

   /// Fill Z with B, pad as necessary
   static void fill(const Shape & shape_Zi, Cell * cZ, Value & Z_owner,
                    Value_P B);

   static Bif_F12_TAKE * fun;   ///< Built-in function
   static Bif_F12_TAKE  _fun;   ///< Built-in function

   /// ↑B
   static Value_P first(Value_P B);

protected:
   /// Take A from B
   Token take(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** System function drop
 */
class Bif_F12_DROP : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_DROP()
   : NonscalarFunction(TOK_F12_DROP)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_DROP * fun;   ///< Built-in function
   static Bif_F12_DROP  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions member and enlist
 */
class Bif_F12_ELEMENT : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_ELEMENT()
   : NonscalarFunction(TOK_F12_ELEMENT)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_ELEMENT * fun;   ///< Built-in function
   static Bif_F12_ELEMENT  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions match and depth
 */
class Bif_F12_EQUIV : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_EQUIV()
   : NonscalarFunction(TOK_F12_EQUIV)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_EQUIV * fun;   ///< Built-in function
   static Bif_F12_EQUIV  _fun;   ///< Built-in function

protected:
   /// return the depth of B
   Token depth(Value_P B);
};
//-----------------------------------------------------------------------------
/** primitive function natch (≢)
 */
class Bif_F12_NEQUIV : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_NEQUIV()
   : NonscalarFunction(TOK_F12_NEQUIV)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_NEQUIV * fun;   ///< Built-in function
   static Bif_F12_NEQUIV  _fun;   ///< Built-in function
};
//-----------------------------------------------------------------------------
/** System function encode
 */
class Bif_F12_ENCODE : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_ENCODE()
   : NonscalarFunction(TOK_F12_ENCODE)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_ENCODE * fun;   ///< Built-in function
   static Bif_F12_ENCODE  _fun;   ///< Built-in function
protected:
   /// encode b according to A (integer A and b)
   void encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
               const Cell * cA, APL_Integer b);

   /// encode b according to A
   void encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
               const Cell * cA, APL_Float b, double qct);

   /// encode B according to A
   void encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
               const Cell * cA, APL_Complex b, double qct);
};
//-----------------------------------------------------------------------------
/** System function decode
 */
class Bif_F12_DECODE : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_DECODE()
   : NonscalarFunction(TOK_F12_DECODE)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_DECODE * fun;   ///< Built-in function
   static Bif_F12_DECODE  _fun;   ///< Built-in function
protected:
   /// decode B according to len_A and cA (integer A, B and Z)
   bool decode_int(Cell * cZ, ShapeItem len_A, const Cell * cA,
                   ShapeItem len_B, const Cell * cB, ShapeItem dB);

   /// decode B according to len_A and cA (real A and B)
   void decode_real(Cell * cZ, ShapeItem len_A, const Cell * cA,
                    ShapeItem len_B, const Cell * cB, ShapeItem dB,
                    double qct);

   /// decode B according to len_A and cA (complex A or B)
   void decode_complex(Cell * cZ, ShapeItem len_A, const Cell * cA,
                       ShapeItem len_B, const Cell * cB, ShapeItem dB,
                       double qct);
};
//-----------------------------------------------------------------------------
/** primitive functions matrix divide and matrix invert
 */
class Bif_F12_DOMINO : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_DOMINO()
   : NonscalarFunction(TOK_F12_DOMINO)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_DOMINO * fun;   ///< Built-in function
   static Bif_F12_DOMINO  _fun;   ///< Built-in function

   /// overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

protected:
   /// Invert matrix B
   Token matrix_inverse(Value_P B);

   /// Divide matrix A by matrix B
   Token matrix_divide(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** primitive functions rotate and reverse
 */
class Bif_ROTATE : public NonscalarFunction
{
public:
   /// Constructor.
   Bif_ROTATE(TokenTag tag)
   : NonscalarFunction(tag)
   {}

protected:
   /// Rotate B according to A along axis
   static Token rotate(Value_P A, Value_P B, Axis axis);

   /// Reverse B along axis
   static Token reverse(Value_P B, Axis axis);
};
//-----------------------------------------------------------------------------
/** primitive functions rotate and reverse along last axis
 */
class Bif_F12_ROTATE : public Bif_ROTATE
{
public:
   /// Constructor
   Bif_F12_ROTATE()
   : Bif_ROTATE(TOK_F12_ROTATE)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return reverse(B, B->get_rank() - 1); }

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return rotate(A, B, B->get_rank() - 1); }

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_ROTATE * fun;   ///< Built-in function
   static Bif_F12_ROTATE  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions rotate and reverse along first axis
 */
class Bif_F12_ROTATE1 : public Bif_ROTATE
{
public:
   /// Constructor
   Bif_F12_ROTATE1()
   : Bif_ROTATE(TOK_F12_ROTATE1)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return reverse(B, 0); }

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return rotate(A, B, 0); }

   /// overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_ROTATE1 * fun;   ///< Built-in function
   static Bif_F12_ROTATE1  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function transpose
 */
class Bif_F12_TRANSPOSE : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_TRANSPOSE()
   : NonscalarFunction(TOK_F12_TRANSPOSE)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_TRANSPOSE * fun;   ///< Built-in function
   static Bif_F12_TRANSPOSE  _fun;   ///< Built-in function

protected:
   /// Transpose B according to A (without diagonals)
   Value_P transpose(const Shape & A, Value_P B);

   /// Transpose B according to A (with diagonals)
   Value_P transpose_diag(const Shape & A, Value_P B);

   /// for \b sh being a permutation of 0, 1, ... rank - 1,
   /// return the inverse permutation sh⁻¹
   static Shape inverse_permutation(const Shape & sh);

   /// return sh permuted according to permutation perm
   static Shape permute(const Shape & sh, const Shape & perm);

   /// return true iff sh is a permutation
   static bool is_permutation(const Shape & sh);
};
//-----------------------------------------------------------------------------
/** System function index of (⍳)
 */
class Bif_F12_INDEX_OF : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_INDEX_OF()
   : NonscalarFunction(TOK_F12_INDEX_OF)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_INDEX_OF * fun;   ///< Built-in function
   static Bif_F12_INDEX_OF  _fun;   ///< Built-in function

protected:
};
//-----------------------------------------------------------------------------
/** primitive functions reshape and shape
 */
class Bif_F12_RHO : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_RHO()
   : NonscalarFunction(TOK_F12_RHO)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Reshape B according to rank and shape
   static Token do_reshape(const Shape & shape, const Value & B);

   static Bif_F12_RHO * fun;   ///< Built-in function
   static Bif_F12_RHO  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/// base class for Bif_F12_UNION and Bif_F2_INTER
class Bif_UNION_INTER : public NonscalarFunction
{
public:
   /// constructor
   Bif_UNION_INTER(TokenTag tag)
   : NonscalarFunction(tag)
   {}

protected:
   /// return unique elements in B (sorted or not)
   Value_P do_unique(Value_P B, bool sorted);
};
//-----------------------------------------------------------------------------
/** System function unique
 */
class Bif_F12_UNION : public Bif_UNION_INTER
{
public:
   /// Constructor
   Bif_F12_UNION()
   : Bif_UNION_INTER(TOK_F12_UNION)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P, Value_P B);

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return Token(TOK_APL_VALUE1, do_unique(B, false)); }

   /// Built-in function
   static Bif_F12_UNION * fun;
   /// Built-in function
   static Bif_F12_UNION  _fun;
protected:
};
//-----------------------------------------------------------------------------
/** System function intersection
 */
class Bif_F2_INTER : public Bif_UNION_INTER
{
public:
   /// Constructor
   Bif_F2_INTER()
   : Bif_UNION_INTER(TOK_F2_INTER)
   {}

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F2_INTER * fun;   ///< Built-in function
   static Bif_F2_INTER  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function left (⊣)
 */
class Bif_F2_LEFT : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F2_LEFT()
   : NonscalarFunction(TOK_F2_LEFT)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return Token(TOK_APL_VALUE2, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return Token(TOK_APL_VALUE1, A->clone(LOC)); }

   static Bif_F2_LEFT * fun;   ///< Built-in function
   static Bif_F2_LEFT  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function right (⊢)
 */
class Bif_F2_RIGHT : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F2_RIGHT()
   : NonscalarFunction(TOK_F2_RIGHT)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return Token(TOK_APL_VALUE1, B->clone(LOC)); }

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return Token(TOK_APL_VALUE1, B->clone(LOC)); }

   static Bif_F2_RIGHT * fun;   ///< Built-in function
   static Bif_F2_RIGHT  _fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------

#endif // __PRIMITIVE_FUNCTION_HH_DEFINED__
