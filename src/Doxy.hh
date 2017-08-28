/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2017  Dr. Jürgen Sauermann

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
#ifndef __DOXY_HH_DEFINED__
#define __DOXY_HH_DEFINED__

#include "UCS_string.hh"
#include "UTF8_string.hh"

#include <ostream>

using namespace std;

class UserFunction;

//-----------------------------------------------------------------------------
class Doxy
{
public:
   Doxy(ostream & out, const UCS_string & root_dir);

   void gen();

protected:
   /// write a fixed CSS file
   void write_css();

   /// (HTML-)print header with name in bold to file of
   void bold_name(ostream & of, const UserFunction * ufun);

   ostream & out;

   /// the name of the workspace (for HTML output)
   UCS_string ws_name;

   /// the directory that shall contain all documentation files
   UTF8_string root_dir;
};
//-----------------------------------------------------------------------------

#endif // __DOXY_HH_DEFINED__
