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

#ifndef __UCS_STRING_HH_DEFINED__
#define __UCS_STRING_HH_DEFINED__

#include <stdint.h>
#include <stdio.h>

#include "Common.hh"
#include "Simple_string.hh"
#include "Unicode.hh"
#include "UTF8_string.hh"

using namespace std;

class PrintBuffer;
class PrintContext;
class Value;

//=============================================================================
/// A string of Unicode characters (32-bit)
class UCS_string : public  Simple_string<Unicode>
{
public:
   /// constructor: empty string
   UCS_string()
   : Simple_string<Unicode>(0, 0)
   {}

   /// constructor: one-element string
   UCS_string(Unicode uni)
   : Simple_string<Unicode>(1, uni)
   {}

   /// constructor: \b len Unicode characters, starting at \b data
   UCS_string(const Unicode * data, size_t len)
   : Simple_string<Unicode>(data, len)
   {}

   /// constructor: \b len times \b uni
   UCS_string(size_t len, Unicode uni)
   : Simple_string<Unicode>(len, uni)
   {}

   /// constructor: copy of another UCS_string
   UCS_string(const UCS_string & ucs, size_t pos, size_t len)
   : Simple_string<Unicode>(ucs, pos, len)
   {}

   /// constructor: UCS_string from UTF8_string
   UCS_string(const UTF8_string & utf);

   /// constructor: UCS_string from 0-terminated C string
   UCS_string(const char * cstring);

   /// constructor: UCS_string from print buffer
   UCS_string(const PrintBuffer & pb, Rank rank, int quad_PW);

   /// constructor: UCS_string from a double with quad_pp valid digits.
   /// (eg. 3.33 has 3 digits), In standard APL format.
   UCS_string(APL_Float value, bool & scaled, const PrintContext & pctx);

   /// constructor: read one line from UTF8-encoded file.
   UCS_string(istream & in);

   /// constructor: UCS_string from simple character vector value.
   UCS_string(const Value & value);

   /// compute the length of an output row
   int compute_chunk_length(int quad_PW, int col) const;

   /// remove trailing pad characters
   void remove_trailing_padchars();

   /// remove trailing blanks, tabs, etc
   void remove_trailing_whitespaces();

   /// remove leading blanks, tabs, etc
   void remove_leading_whitespaces();

   /// remove leading and trailing whitespaces
   void remove_leading_and_trailing_whitespaces()
      {
        remove_trailing_whitespaces();
        remove_leading_whitespaces();
      }

   /// skip leading whitespaces starting at idx, append the following
   /// non-whitespaces (if any) to \b dest, and skip trailing whitespaces
   void copy_black(UCS_string & dest, int & idx) const;

   /// copy names etc. separated by whitespaces to \b dest
   void copy_black_list(vector<UCS_string> & dest) const;

   /// \b this is a command with optional args. Remove leading and trailing
   /// whitespaces, append args to rest, and remove args from this.
   void split_ws(UCS_string & rest);

   /// return \b true iff \b this string is contained in \b list
   bool contained_in(const vector<UCS_string> & list) const;

   /// return the start position of \b sub in \b this string or -1 if \b sub
   /// is not contained in \b this string
   int substr_pos(const UCS_string & sub) const;

   /// remove the first \b drop_count characters from this string
   UCS_string drop(int drop_count) const
      {
        if (drop_count <= 0)        return UCS_string(*this, 0, size());
        if (size() <= drop_count)   return UCS_string();
        return UCS_string(*this, drop_count, size() - drop_count);
      }

   /// return the last character in \b this string
   Unicode back() const
    { return size() ? (*this)[size() - 1] : Invalid_Unicode; }

   /// return true if this string contains non-whitespace characters
   bool has_black() const;

   /// return true if \b this starts with prefix (ASCII, case matters).
   bool starts_with(const char * prefix) const;

   /// return true if \b this starts with \b prefix (case sensitive).
   bool starts_with(const UCS_string & prefix) const;

   /// return true if \b this starts with \b prefix (ASCII, case insensitive).
   bool starts_iwith(const char * prefix) const;

   /// return true if \b this starts with \b prefix (case insensitive).
   bool starts_iwith(const UCS_string & prefix) const;

   /// return a string like this, but with pad chars mapped to spaces
   UCS_string no_pad() const;

   /// replace pad chars in \b this string by spaces
   void map_pad();

   /// return a string like this, but with pad chars removed
   UCS_string remove_pad() const;

   /// remove leading and trailing spaces from \b this string.
   void remove_lt_spaces();

   /// remove the last character in \b this string
   void pop_back()
   { Assert(size() > 0);   shrink(size() - 1); }

   /// return this string reversed (i.e. characters from back to front).
   UCS_string reverse() const;

   /// return true if \b yhis string starts with # or ⍝ or x:
   bool is_comment_or_label() const;

   /// return true if every char in \b this string is '0'
   bool all_zeroes()
      { loop(s, size())   if ((*this)[s] != UNI_ASCII_0)   return false;   
        return true;
      }

   /// return integer value for a string starting with optional whitespaces,
   /// followed by digits.
   int atoi() const;

   /// append 0-terminated ASCII string \b str to this string. str is NOT
   /// interpreted as UTF8 string (use append_utf8() if such interpretation        /// is desired)
   void append_ascii(const char * str);

   /// append 0-terminated UTF8 string str to \b this UCS_string.
   // This is different from append_ascii((const char *)str):
   ///
   /// append_ascii() appends one Unicode per byte (i.e. strlen(str) in total),
   /// without checking for UTF8 sequences.
   ///
   /// append_utf8() appends one Unicode per UTF8 sequence (which is the same
   /// if all characteras are ASCII, but less if not.
   void append_utf8(const UTF8 * str);

   /// same as app(const UTF8 * str)
   void append_utf8(const char * str)
      { append_utf8((const UTF8 *)str); }

   /// return \b this string and \b other concatenated
   UCS_string operator +(const UCS_string & other) const
      { UCS_string ret(*this);   ret.append(other);   return ret; }

   /// set this string to the 0-terminated ASCII c_string (don't use for UTF8
   /// encoded strings)
   UCS_string & operator =(const char * c_string)
      { *this = UCS_string(c_string);   return *this; }

   /// append \b other in quotes, doubling quoted in \b other
   void append_quoted(const UCS_string & other);

   /// append number (in ASCII encoding like %d) to this string
   void append_number(ShapeItem num);

   /// append number (in ASCII encoding like %X or %x) to this string
   void append_hex(ShapeItem num, bool uppercase);

   /// append number (in ASCII encoding like %lf) to this string
   void append_float(double num);

   /// split \b this multi-line string into individual lines,
   /// removing the CR and NL chars in \b this string.
   size_t to_vector(vector<UCS_string> & result) const;

   /// an iterator for UCS_strings
   class iterator
      {
        public:
           /// constructor: start at position p
           iterator(const UCS_string & ucs, int p)
           : s(ucs),
             pos(p)
           {}

           /// return char at offset off from current position
           Unicode get(int off = 0) const
              { return (pos + off) < s.size() ? s[pos+off] : Invalid_Unicode; }

           /// return next char
           Unicode next()
              { return pos < s.size() ? s[pos++] : Invalid_Unicode; }

           /// return truei iff there are more chars in the string
           bool more() const
              { return pos < s.size(); }

        protected:
           /// the string
           const UCS_string & s;

           /// the current position in the string
           int pos;
      };

   /// an iterator set to the start of this string
   UCS_string::iterator begin() const
      { return iterator(*this, 0); }

   /// round last digit and discard it.
   void round_last_digit();

   /// return true if \b this string contains \b uni
   bool contains(Unicode uni);

   /// case-sensitive comparison: return true iff \b this comes before \b other
   bool lexical_before(const UCS_string other) const;

   /// dump \b this string to out (like U+nnn U+mmm ... )
   ostream & dump(ostream & out) const;

   /// convert a signed integer value to an UCS_string (like sprintf())
   static UCS_string from_int(int64_t value);

   /// convert an unsigned integer value to an UCS_string (like sprintf())
   static UCS_string from_uint(uint64_t value);

   /// convert the integer part of value to an UCS_string and remove it
   /// from value
   static UCS_string from_big(double & value);

   /// convert double \b value to an UCS_string with \b fract_digits fractional
   /// digits in scaled (exponential) format
   static UCS_string from_double_expo_prec(double value,
                                            unsigned int fract_digits);

   /// convert double \b value to an UCS_string with \b fract_digits fractional
   /// digits in fixed point format
   static UCS_string from_double_fixed_prec(double value,
                                            unsigned int fract_digits);

   /// convert double \b value to an UCS_string with \b quad_pp significant
   /// digits in acaled (exponential) format
   static UCS_string from_double_expo_pp(double value, unsigned int quad_pp);

   /// convert double \b value to an UCS_string with \b quad_pp significant
   /// digits in fixed point format
   static UCS_string from_double_fixed_pp(double value, unsigned int quad_pp);

   /// sort a (small) number of UCS_strings (filenames, variables, or functions)
   /// using a simple but quadratic time algorithm
   static void sort_names(const UCS_string ** names, int count);

   /// sort a (small) number of UCS_strings (filenames, variables, or functions)
   /// using a simple but quadratic time algorithm
   static void sort_names(vector<UCS_string> names);

   /// compute column widths so that names align nicely
   static void compute_column_width(vector<int> & result,
                                    const UCS_string ** names, int name_count,
                                    int tab_size, int quad_PW);
};
//-----------------------------------------------------------------------------
/// a singly linked list of UCS_strings.
struct
UCS_string_list
{
   /// the string
   UCS_string string;

   /// the previous string
   UCS_string_list * prev;

   /// constructor
   UCS_string_list(const UCS_string & s, UCS_string_list * p)
   : string(s),
     prev(p)
   {}

   /// return the length of the list
   static int length(const UCS_string_list * list)
      {
        for (int ret = 0; ; list = list->prev, ++ret)
            { if (list == 0)   return ret; }
      }
};
//-----------------------------------------------------------------------------

#endif // __UCS_STRING_HH_DEFINED__

