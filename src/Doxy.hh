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
/// one endge in a (directed) function call graph
struct fcall_edge
{
   /// the calling function
   const UserFunction * caller;

   /// the called function
   const UserFunction * callee;

   /// some arbitrary int used in graph algorithms
   int      value;
};

typedef Simple_string<fcall_edge, false> CallGraph;
//-----------------------------------------------------------------------------
/// implementation of the ]Doxy command.
class Doxy
{
public:
   /// Constructor: create (and remember) the root directory
   Doxy(ostream & out, const UCS_string & root_dir);

   /// generate the entire documentation
   void gen();

   /// return the number of errors that have occurred
   int get_errors() const
      { return errors; }

   const UTF8_string & get_root_dir() const
      { return root_dir; }

protected:
   /// write a fixed CSS file
   void write_css();

   /// (HTML-)print a function header with the name in bold to file of
   void bold_name(ostream & of, const UserFunction * ufun);

   /// write the page for one defined function
   void function_page(const UserFunction * ufun);

   /// create the call graph
   void make_call_graph(const Simple_string<const Symbol *, false> & all_funs);

   /// add one symbol to the call graph. Note that one symbol can have different
   /// UserFunctions at different SI levels.
   void add_fun_to_call_graph(const Symbol * sym, const UserFunction * ufun);

   /// make the call graph start from root ufun and set nodes to nodes that are
   /// reachable from the root
   void set_call_graph_root(const UserFunction * ufun);

   /// write the call graph (if caller == false), or else the caller graph
   int write_call_graph(const UserFunction * ufun, bool caller);

   /// swap callers and callees
   void swap_caller_calee();

   int node_ID(const UserFunction * ufun);

   /// convert a .gv file to a .png file using program 'dot'
   int gv_to_png(const char * gv_filename, const char * png_filename);

   /// the command output channel (COUR or CERR)
   ostream & out;

   /// the name of the workspace (for HTML output)
   UCS_string ws_name;

   /// the directory that shall contain all documentation files
   UTF8_string root_dir;

   /// the nodes for the current root.
   Simple_string<const UserFunction *, false> nodes;

   /// a directed graph telling which function calles which
   CallGraph call_graph;

   int errors;
};
//-----------------------------------------------------------------------------
#endif // __DOXY_HH_DEFINED__
