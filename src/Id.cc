/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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

#include "Avec.hh"
#include "Bif_F12_FORMAT.hh"
#include "Bif_F12_SORT.hh"
#include "Bif_OPER1_COMMUTE.hh"
#include "Bif_OPER1_EACH.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "Bif_OPER2_INNER.hh"
#include "Bif_OPER2_OUTER.hh"
#include "Bif_OPER2_POWER.hh"
#include "Bif_OPER2_RANK.hh"
#include "Common.hh"
#include "Id.hh"
#include "Output.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_DLX.hh"
#include "Quad_FX.hh"
#include "Quad_SQL.hh"
#include "Quad_SVx.hh"
#include "Quad_TF.hh"
#include "ScalarFunction.hh"
#include "UCS_string.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
/// an Id and how it looks like in APL
struct Id_name
{
  /// compare \b key with \b item (for bsearch())
  static int compare(const void * key, const void * item)
     {
       return *(const ID::Id *)key - ((const Id_name *)item)->id;
     }

  /// the ID
  ID::Id id;

   /// how \b id is being printed
  const UCS_string * ucs_name;
};

static Id_name id2ucs[] =
{
#define pp(i, _u, _v) { ID::i, 0}, 
#define qf(i,  _u, _v) {ID::Quad_ ## i, 0}, 
#define qv(i,  _u, _v) {ID::Quad_ ## i, 0}, 
#define sf(i,  _u, _v) {ID::i, 0}, 
#define st(i,  _u, _v) {ID::i, 0},

#include "Id.def"
};

const UCS_string &
ID::get_name(Id id)
{
const void * result =
    bsearch(&id, id2ucs, sizeof(id2ucs) / sizeof(Id_name),
            sizeof(Id_name), Id_name::compare);

   Assert(result);
Id_name * idn = (Id_name *) result;
   if (const UCS_string * ucs = idn->ucs_name)   return *ucs; 

   // the name was not yet constructed. Do it now
   //
const char * name = "unknown ID";
   switch(id)
       {
#define pp(i, _u, _v) case ID::i:          name = #i;   break;
#define qf(i,  u, _v) case ID::Quad_ ## i: name = u;   break;
#define qv(i,  u, _v) case ID::Quad_ ## i: name = u;   break;
#define sf(i,  u, _v) case ID::i:          name = u;   break;
#define st(i,  u, _v) case ID::i:          name = u;   break;

#include "Id.def"
       }

UTF8_string utf(name);
UCS_string * ucs = new UCS_string(utf);
   idn->ucs_name = ucs;
   return *ucs;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, ID::Id id)
{
   return out << ID::get_name(id);
}
//-----------------------------------------------------------------------------
Function *
ID::get_system_function(ID::Id id)
{
   switch(id)
      {
#define pp(i, _u, _v)
#define qf(i, _u, _v) case ID::Quad_ ## i: return Quad_ ## i::fun;
#define qv(i, _u, _v)
#define sf(i, _u, _v) case ID:: i:        return Bif_ ## i::fun;
#define st(i, _u, _v)

#include "Id.def"

        default: break;
      }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
ID::get_system_variable(ID::Id id)
{
   switch(id)
      {
#define pp(_i, _u, _v)
#define qf(_i, _u, _v)
#define qv( i, _u, _v)   case ID::Quad_ ## i: \
                              return &Workspace::get_v_Quad_ ## i();
#define sf(_i, _u, _v) 
#define st(_i, _u, _v) 

#include "Id.def"

        default: break;
      }

   return 0;
}
//-----------------------------------------------------------------------------
int
ID::get_token_tag(Id id)
{
   switch(id)
      {
#define pp(_i, _u, _v)
#define qf(_i, _u, _v)
#define qv( i, _u, _v)   case ID::Quad_ ## i: return TOK_Quad_## i;
#define sf(_i, _u, _v) 
#define st(_i, _u, _v) 

#include "Id.def"

        default: break;
      }

   return 0;
}
//-----------------------------------------------------------------------------
