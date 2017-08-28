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
#include <sys/stat.h>

#include "Doxy.hh"
#include "Heapsort.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

using namespace std;

//-----------------------------------------------------------------------------
/// a helper for sorting Symbol * by symbol name
static bool
symcomp(const Symbol * const & s1, const Symbol * const & s2, const void *)
{
   return s2->get_name().compare(s1->get_name()) == COMP_LT;
}
//-----------------------------------------------------------------------------
Doxy::Doxy(ostream & cout, const UCS_string & dest_dir)
   : out(cout),
     root_dir(dest_dir)
{
   ws_name = Workspace::get_WS_name();
   if (ws_name.compare(UCS_string("CLEAR WS")) == 0)
      ws_name = UCS_string("CLEAR-WS");

   root_dir.append_str("/");
   root_dir.append(UTF8_string(ws_name));

   write_css();
}
//-----------------------------------------------------------------------------
void
Doxy::write_css()
{
UTF8_string css_filename(root_dir);
   css_filename.append_str("/apl_doxy.css");
ofstream css(css_filename.c_str());
   css <<
"/* GNU APL Doxy css file */"                                        << endl <<
"BODY  { background-color: #B0FFB0 }"                                 << endl <<
"TABLE { background-color: #FFFF80 }"                                 << endl <<
"H1    { text-align: center }"                                        << endl <<
"H2    { text-align: center }"                                        << endl <<
".code    { font-family: fixed }"                                     << endl <<
   endl;

   css.close();
}
//-----------------------------------------------------------------------------
void
Doxy::gen()
{
   out << "Creating output directory " << root_dir << endl;
   mkdir(root_dir.c_str(), 0777);

const SymbolTable & symtab = Workspace::get_symbol_table();

Simple_string<const Symbol *, false> all_symbols = symtab.get_all_symbols();

UTF8_string index_filename(root_dir);
   index_filename.append_str("/index.html");
ofstream index(index_filename.c_str());
   index <<
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""               << endl <<
"                      \"http://www.w3.org/TR/html4/strict.dtd\">"  << endl <<
"<html>"                                                            << endl <<
"  <head>"                                                          << endl <<
"    <title>Documentation of " << ws_name << "</title> "            << endl <<
"    <meta http-equiv=\"content-type\" "                            << endl <<
"          content=\"text/html; charset=UTF-8\">"                   << endl <<
"   <link rel='stylesheet' type='text/css' href='apl_doxy.css'>"    << endl <<
" </head>"                                                          << endl <<
" <body>"                                                           << endl <<
"  <H1>Workspace " << ws_name << "</H1>"                            << endl;

Simple_string<const Symbol *, false> functions;
Simple_string<const Symbol *, false> variables;

   loop(a, all_symbols.size())
      {
        const Symbol * sym = all_symbols[a];
        if (sym->is_erased())                continue;
        if (sym->get_name()[0] == UNI_MUE)   continue;   // macro

        const NameClass nc = sym->get_nc() ;
        switch(nc)
           {
             case NC_VARIABLE: variables.append(sym);   break;
             case NC_FUNCTION: functions.append(sym);   break;
             case NC_OPERATOR: functions.append(sym);   break;
             default: ;
           }
      }

   Heapsort<const Symbol *>::sort(&functions[0], functions.size(), 0, symcomp);
   index << "<H2>Defined Functions</H2>"                                << endl
         << " <table>"                                                  << endl
         << "  <tr>"                                                    << endl
         << "   <th>Function"                                           << endl
         << "   <th>Header"                                             << endl;
   loop(f, functions.size())
      {
        index << "  <tr>"                                               << endl;
        const Function * fp = functions[f]->get_function();
        Assert(fp);
        if (fp->is_native())
           {
             index << "   <td class=code>" << functions[f]->get_name()
                   << " (native)" << endl;
           }
        else
           {
             index << "   <td class=code>" << functions[f]->get_name()
                   << endl;
           }
      }
   index << " </table>" << endl;

   Heapsort<const Symbol *>::sort(&variables[0], variables.size(), 0, symcomp);
   index << "<H2>Variables</H2>"                                        << endl
         << " <table>"                                                  << endl
         << "  <tr>"                                                    << endl
         << "   <th>Variable"                                           << endl
         << "   <th>⍴⍴"                                                 << endl
         << "   <th>⍴"                                                  << endl
         << "   <th>≡"                                                  << endl;
   loop(v, variables.size())
      {
        index << "  <tr>"                                               << endl
              << "   <td class=code>" << variables[v]->get_name()       << endl;
      }
   index << " </table>" << endl;

   index <<
" </body>"                                                              << endl;

   index.close();
}
//-----------------------------------------------------------------------------
