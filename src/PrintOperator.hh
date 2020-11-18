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

#ifndef __PRINTOPERATOR_HH_DEFINED
#define __PRINTOPERATOR_HH_DEFINED

#include <iostream>

#include "Common.hh"
#include "Id.hh"
#include "TokenEnums.hh"
#include "Unicode.hh"

struct AP_num3;
class  Cell;
class  DynamicObject;
struct Format_sub;
class  Function;
struct Function_PC2;
class  IndexExpr;
struct LineLabel;
class  PrintBuffer;
class  Shape;
class  Symbol;
class  Token;
class  Token_string;
class  UCS_string;
class  UTF8_string;
class  Value;

ostream & operator << (ostream &, const AP_num3 &);
ostream & operator << (ostream &, const Format_sub &);
ostream & operator << (ostream &, const Function &);
ostream & operator << (ostream &, const Function_PC2 &);
ostream & operator << (ostream &, const Cell &);
ostream & operator << (ostream &, const DynamicObject &);
ostream & operator << (ostream &,       Id id);
ostream & operator << (ostream &, const IndexExpr &);
ostream & operator << (ostream &, const LineLabel &);
ostream & operator << (ostream &, const PrintBuffer &);
ostream & operator << (ostream &, const Shape &);
ostream & operator << (ostream &, const Symbol &);
ostream & operator << (ostream &, const Token &);
ostream & operator << (ostream &, const Token_string &);
ostream & operator << (ostream &,       TokenTag);
ostream & operator << (ostream &,       TokenClass);
ostream & operator << (ostream &, const UCS_string &);
ostream & operator << (ostream &,       Unicode);
ostream & operator << (ostream &, const UTF8_string &);
ostream & operator << (ostream &, const Value &);

inline ostream & operator << (ostream & out,       ValueFlags flags)
   { return print_flags(out, flags); }

inline ostream & operator << (ostream & out, const labVal & lv)
   { return out << "Line-" << lv.line << ":"; }

#endif // __PRINTOPERATOR_HH_DEFINED
