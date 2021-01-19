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

std::ostream & operator << (std::ostream &, const AP_num3 &);
std::ostream & operator << (std::ostream &, const Format_sub &);
std::ostream & operator << (std::ostream &, const Function &);
std::ostream & operator << (std::ostream &, const Function_PC2 &);
std::ostream & operator << (std::ostream &, const Cell &);
std::ostream & operator << (std::ostream &, const DynamicObject &);
std::ostream & operator << (std::ostream &,       Id id);
std::ostream & operator << (std::ostream &, const IndexExpr &);
std::ostream & operator << (std::ostream &, const LineLabel &);
std::ostream & operator << (std::ostream &, const PrintBuffer &);
std::ostream & operator << (std::ostream &, const Shape &);
std::ostream & operator << (std::ostream &, const Symbol &);
std::ostream & operator << (std::ostream &, const Token &);
std::ostream & operator << (std::ostream &, const Token_string &);
std::ostream & operator << (std::ostream &,       TokenTag);
std::ostream & operator << (std::ostream &,       TokenClass);
std::ostream & operator << (std::ostream &, const UCS_string &);
std::ostream & operator << (std::ostream &,       Unicode);
std::ostream & operator << (std::ostream &, const UTF8_string &);
std::ostream & operator << (std::ostream &, const Value &);

inline std::ostream & operator << (std::ostream & out,       ValueFlags flags)
   { return print_flags(out, flags); }

inline std::ostream & operator << (std::ostream & out, const labVal & lv)
   { return out << "Line-" << lv.line << ":"; }

#endif // __PRINTOPERATOR_HH_DEFINED
