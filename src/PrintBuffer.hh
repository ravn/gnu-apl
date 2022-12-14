/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2022  Dr. Jürgen Sauermann

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

#ifndef __PRINT_BUFFER_HH__DEFINED__
#define __PRINT_BUFFER_HH__DEFINED__

#include <vector>

#include "Avec.hh"
#include "Common.hh"
#include "PrintContext.hh"
#include "PrintOperator.hh"
#include "Shape.hh"
#include "UCS_string_vector.hh"

class Value;
class Cell;

//----------------------------------------------------------------------------

/// some information about a column to be printed.
class ColInfo
{
public:
   /// properties of an empty column.
   ColInfo()
   : flags(0),
     int_len(0),
     fract_len(0),
     real_len(0),
     imag_len(0),
     denom_len(0)
   {}

   /// update the properties of \b this ColInfo according to
   /// the properties of \b other
   void consider(const ColInfo & other);

   /// true if \b this ColInfo has an exponent field
   bool have_expo() const
      { return denom_len == 0 && real_len > (int_len + fract_len); }

   /// the total length
   int total_len() const
      { return real_len + imag_len; }

   /// some flags
   int flags;

   /// the length of the integral part (of numbers) or total length
   /// I.e. an optional leading sign and the digits before the '.'
   int int_len;

   /// the length of the fraction part (of numbers)
   /// I.e. the leading dot and the digits left of an 'E'
   int fract_len;

   /// the length of the real part (of complex numbers) or total length
   int real_len;

   /// the length of the imaginary part (of complex numbers)
   int imag_len;

   /// the length of the denominator (for rational numbers)
   int denom_len;
};
//----------------------------------------------------------------------------
/// A two-dimensional Unicode character buffer
class PrintBuffer
{
public:
   /// contructor: empty PrintBuffer
   PrintBuffer();

   /// contructor: a single row PrintBuffer
   PrintBuffer(const UCS_string & ucs, const ColInfo & ci);

   /// contructor: a PrintBuffer from an APL value
   PrintBuffer(const Value & value, const PrintContext & pctx, ostream * out);

   /// helper for non-trivial PrintBuffer(const Value & ...) constructor
   void do_PrintBuffer(const Value & value,const PrintContext & pctx,
                         ostream * out, PrintStyle outer_style,
                         std::vector<bool> & scaling,
                         std::vector<PrintBuffer> & pcols,
                         PrintBuffer * item_matrix);

   /// PrintBuffer from an APL value in function-style
   void pb_for_function(const Value & value, PrintContext pctx,
                        PrintStyle outer_style);

   /// PrintBuffer from an empty APL value
   void pb_empty(const Value & value, PrintContext pctx,
                        PrintStyle outer_style);

   /// return the number of rows
   int get_row_count() const
      { return buffer.size(); }

   /// return the first (and only) line
   UCS_string l1() const
      { Assert(get_row_count() == 1);   return buffer[0]; }

   /// return line y
   UCS_string get_line(int y) const
      { Assert(y < get_row_count());   return buffer[y]; }

   /// print this buffer, interruptible with ^C
   void print_interruptible(ostream & out, sRank rank, int quad_pw);

   /// return the number of columns
   int get_column_count() const
      { return get_column_count(0); }

   /// return the number of columns in row y
   int get_column_count(int y) const
      { if (get_row_count() == 0)   return 0;   // no rows
        Assert(y < get_row_count());   return buffer[y].size(); }

   /// Set the char in column x and row y to uc.
   void set_char(int x, int y, Unicode uc);

   /// Return the char in column x and row y.
   Unicode get_char(int x, int y) const;

   /// prepend buffer with \b count characters \b pad
   void pad_l(Unicode pad, ShapeItem count);

   /// append count charactera \b pad  to buffer
   void pad_r(Unicode pad, ShapeItem count);

   /// append lines to reach height
   void pad_height(Unicode pad, ShapeItem height);

   /// prepend lines to reach height
   void pad_height_above(Unicode pad, ShapeItem height);

   /// replace pad chars by spaces
   void pad_to_spaces();

   /// add a decorator frame around this buffer
   void add_frame(PrintStyle style, const Shape & shape, int depth);

   /// add an outer frame around this buffer
   void add_outer_frame(PrintStyle style);

   /// print properties of \b this PrintBuffer
   ostream & debug(ostream & out, const char * title = 0) const;

   /// append PrintBuffer \b pb1 to \b this PrintBuffer
   void append_col(const PrintBuffer & pb1);

   /// append UCS_string \b ucs to \b this PrintBuffer
   void append_ucs(const UCS_string & ucs);

   /// append ucs, aligned at char \b align.
   void append_aligned(const UCS_string & ucs, Unicode align);

   /// add padding and a column \b pb to \b this PrintBuffer
   void add_column(Unicode pad, int32_t pad_count, const PrintBuffer & pb);

   /// add PrintBuffer \b row to \b this PrintBuffer
   void add_row(const PrintBuffer & row);

   /// add empty rows to \b this PrintBuffer
   void add_empty_rows(ShapeItem count)
      { loop(c, count)   buffer.push_back(UCS_string()); }

   /// return the ColInfo of \b this PrintBuffer
   const ColInfo & get_info() const   { return col_info; }

   /// update col_info after a change of buffer
   void update_info()/// append UCS_string \b ucs to \b this PrintBuffer
      { if (buffer.size())   col_info.int_len =
                             col_info.real_len = buffer[0].size(); }

   /// return the ColInfo of \b this PrintBuffer
   ColInfo & get_info() { return col_info; }

   /// return true iff all strings have the same size.
   bool is_rectangular() const;

   /// return the characters for style \b pst
   static void get_frame_chars(PrintStyle pst, Unicode & HORI, Unicode & VERT,
                       Unicode & NW, Unicode & NE, Unicode & SE, Unicode & SW);

protected:
   /// align this PrintBuffer to col
   void align(ColInfo & col);

   /// align this char-only PrintBuffer to col
   void align_left(ColInfo & col);

   /// align this real-only PrintBuffer to col
   void align_dot(ColInfo & col);

   /// align this complex PrintBuffer to col
   void align_j(ColInfo & col);

   /// pad the fraction part to \b wanted_fract_len with '0'
   void pad_fraction(int wanted_fract_len, bool want_expo);

   /// return the number of separator rows before row \b y in a value with
   /// shape \b shape
   static ShapeItem separator_rows(ShapeItem y, const Value & value,
                                   bool nested, sRank rk1, sRank rk2);

   /// the character buffer.
   UCS_string_vector buffer;

   /// column properties
   ColInfo col_info;

   /// true if completely constructed (as opposed to interrupted by ^C)
   bool complete;
};

#endif // __PRINT_BUFFER_HH__DEFINED__
