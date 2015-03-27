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

#ifndef __UTF8_STRING_HH_DEFINED__
#define __UTF8_STRING_HH_DEFINED__

#include <iostream>
#include <stdint.h>
#include <string.h>

#include "Simple_string.hh"

using namespace std;

class UCS_string;

//-----------------------------------------------------------------------------
/// one byte of a UTF8 encoded Unicode (RFC 3629) string
typedef uint8_t UTF8;

//-----------------------------------------------------------------------------
/// an UTF8 encoded Unicode (RFC 3629) string
class UTF8_string :  public Simple_string<UTF8>
{
public:
   /// constructor: empty UTF8_string
   UTF8_string()
   : Simple_string<UTF8>((const UTF8 *)0, 0)
   {}

   /// constructor: UTF8_string from 0-terminated C string.
   UTF8_string(const char * str)
   : Simple_string<UTF8>((const UTF8 *)str, str ? strlen(str) : 0)
   {}

   /// constructor: copy of string, but at most len bytes
   UTF8_string(const UTF8 * string, size_t len)
   : Simple_string<UTF8>(string, len)
   {}

   /// constructor: copy of UCS string. The UCS characters will be UTF8-encoded
   UTF8_string(const UCS_string & ucs);

   /// constructor: UCS_string from (simple character vector) APL value.
   /// Non-ASCII The UCS characters will be UTF8 encoded.
   UTF8_string(const Value & value);

   /// display bytes in this UTF string
   ostream & dump_hex(ostream & out, int max_bytes) const;

   /// return this string as a C-string (appending a 0-byte)
   const char * c_str()
      {
        extend(items_valid + 1);
        items[items_valid] = 0;   // the terminating 0
        return (const char *)items;
      }

   /// return true iff string ends with ext (usually a file name extennsion)
   bool ends_with(const char * ext) const;

   /// return true iff string ends with ext (usually a file path)
   bool starts_with(const char * path) const;

   /// append a 0-terminated C string
   void append_str(const char * str)
      { const UTF8_string str_utf(str);  append(str_utf); }

   /// convert the first char in UTF8-encoded string to Unicode, set
   /// setting len to the number of bytes in the UTF8 encoding of the char
   static Unicode toUni(const UTF8 * string, int & len);

   /// return the next UTF8 encoded char from an input file
   static Unicode getc(istream & in);
};
//=============================================================================
/// A UTF8 string to be used as filebuf in UTF8_ostream
class UTF8_filebuf : public filebuf
{
public:
   /// return the data in this filebuf
   const UTF8_string & get_data()
      { return data; }

protected:
   /// insert \b c into this filebuf
   virtual int overflow(int c);

   /// the data in this filebuf
   UTF8_string data;
};
//=============================================================================
/// a UTF8 string that can be used as ostream
class UTF8_ostream : public ostream
{
public:
   /// An UTF8_string that can be used like an ostream to format data
   UTF8_ostream()
   : ostream(&utf8_filebuf)
   {}

   /// return the data in this UTF8_string
   const UTF8_string & get_data()
      { return utf8_filebuf.get_data(); }

protected:
   /// the filebuf of this ostream
   UTF8_filebuf utf8_filebuf;
};
//=============================================================================

#endif // __UTF8_STRING_HH_DEFINED__
