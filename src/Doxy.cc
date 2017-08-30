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
#include <errno.h>
#include <sys/stat.h>

#include "Doxy.hh"
#include "Heapsort.hh"
#include "Logging.hh"
#include "NativeFunction.hh"
#include "PrintOperator.hh"
#include "UserFunction.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

using namespace std;

#define BRLF "<BR>\r\n"
#define CRLF "\r\n"

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
     root_dir(dest_dir),
     errors(0)
{
   ws_name = Workspace::get_WS_name();
   if (ws_name.compare(UCS_string("CLEAR WS")) == 0)
      ws_name = UCS_string("CLEAR-WS");

   root_dir.append_str("/");
   root_dir.append(UTF8_string(ws_name));

   Log(LOG_command_DOXY)
      out << "Creating output directory " << root_dir << endl;

   mkdir(root_dir.c_str(), 0777);
   write_css();
}
//-----------------------------------------------------------------------------
void
Doxy::write_css()
{
UTF8_string css_filename(root_dir);
   css_filename.append_str("/apl_doxy.css");

   Log(LOG_command_DOXY)
      out << "Writing style sheet file " << css_filename << endl;

ofstream css(css_filename.c_str());
   css <<
"/* GNU APL Doxy css file */"                                              CRLF
"BODY     { background-color: #C0C0FF }"                                   CRLF
"TABLE    { background-color: #FFFFFF }"                                   CRLF
"H1       { text-align: center }"                                          CRLF
"H2       { text-align: center }"                                          CRLF
"TH, TD   { padding-left: 0.5em; padding-right: 0.5em }"                   CRLF
".code    { font-family: fixed }"                                          CRLF
"table, th, td { border: 1px solid black }"                                CRLF
".funtab,"                                                                 CRLF
".vartab  { margin-left:auto; margin-right:auto;"                          CRLF
"           border-collapse: collapse; }"                                  CRLF
".cg1,"                                                                    CRLF
".cg2     { margin:auto; display: block; }"                                CRLF
".doxy_comment { width: 40% }"                                             CRLF;

   css.close();
}
//-----------------------------------------------------------------------------
void
Doxy::gen()
{
const SymbolTable & symtab = Workspace::get_symbol_table();

Simple_string<const Symbol *, false> all_symbols = symtab.get_all_symbols();
Simple_string<const Symbol *, false> functions;
Simple_string<const Symbol *, false> variables;

   loop(a, all_symbols.size())
      {
        const Symbol * sym = all_symbols[a];
        if (sym->is_erased())                continue;
        if (sym->get_name()[0] == UNI_MUE)   continue;   // macro

        bool is_function = false;
        bool is_variable = false;
        loop(si, sym->value_stack_size())
            {
              switch((*sym)[si].name_class)
                 {
                   case NC_VARIABLE: is_variable = true;   break;
                   case NC_FUNCTION: is_function = true;   break;
                   case NC_OPERATOR: is_function = true;   break;
                   default: ;
                 }
            }
        if (is_function)   functions.append(sym);
        if (is_variable)   variables.append(sym);
      }

   if (functions.size() > 1)
      Heapsort<const Symbol *>::
              sort(&functions[0], functions.size(), 0, symcomp);

   if (variables.size() > 1)
      Heapsort<const Symbol *>::
              sort(&variables[0], variables.size(), 0, symcomp);

UTF8_string index_filename(root_dir);
   index_filename.append_str("/index.html");

   Log(LOG_command_DOXY)
      out << "Writing top-level HTML file " << index_filename << endl;

ofstream page(index_filename.c_str());
   page <<
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""                      CRLF
"                      \"http://www.w3.org/TR/html4/strict.dtd\">"         CRLF
"<html>"                                                                   CRLF
"  <head>"                                                                 CRLF
"    <title>Documentation of " << ws_name << "</title> "                   CRLF
"    <meta http-equiv=\"content-type\" "                                   CRLF
"          content=\"text/html; charset=UTF-8\">"                          CRLF
"   <link rel='stylesheet' type='text/css' href='apl_doxy.css'>"           CRLF
" </head>"                                                                 CRLF
" <body>"                                                                  CRLF
"  <H1>Workspace " << ws_name << "</H1>"                                   CRLF
"   <H2>Defined Functions</H2>"                                            CRLF
"    <TABLE class=funtab>"                                                 CRLF
"     <TR>"                                                                CRLF
"      <TH>Function"                                                       CRLF
"      <TH>SI"                                                             CRLF
"      <TH>⍬⍴⎕CR"                                                          CRLF
"      <TH>Header"                                                         CRLF
"      <TH class=doxy_comment>Doxy Documentation"                          CRLF;

   make_call_graph(functions);

   loop(f, functions.size())
      {
        const Symbol & fun_sym = *functions[f];
        loop(si, fun_sym.value_stack_size())
            {
              if (fun_sym[si].name_class != NC_FUNCTION &&
                  fun_sym[si].name_class != NC_OPERATOR)   continue;

              const Function * fp = fun_sym[si].sym_val.function;
              Assert(fp);
              const UserFunction * ufun = fp->get_ufun1();

              // colunm 1: function name/link
              //
              page << "  <tr>"                                             CRLF
                      "   <TD class=code>";

              int line_count = 0;
              if (fp->is_native())
                 {
                   page << fun_sym.get_name() << " (native)"               CRLF;
                 }
              else if (ufun && !fp->is_native())
                 {
                   line_count = ufun->get_text_size();
                   function_page(ufun);
                   page << "<A href=f_" << fun_sym.get_name() <<".html>"
                        << fun_sym.get_name() << "</A>"                    CRLF;
                 }
              else
                 {
                   page << fun_sym.get_name() <<                           CRLF;
                 }

              // column 2: SI level
              //
              page << "   <TD class=code>" << si <<                        CRLF;


              // column 3: number of lines
              //
              page <<  "    <TD class=code>" << line_count <<              CRLF;

              // column 4: function header (if any)
              //
              page <<  "   <TD class=code>";
              if (fp->is_native())
                 page << reinterpret_cast<const NativeFunction *>
                                               (fp)->get_so_path();
              else if (ufun)
                 bold_name(page, ufun);
              else
                 page << "-";
              page <<                                                      CRLF;

              // column 5: Doxygen comments
              //
              page <<  "   <TD class=doxy_comment>";
              if (ufun && !fp->is_native())
                 {
                   loop(l, ufun->get_text_size())
                      {
                        const UCS_string & line = ufun->get_text(l);
                        if (line.size() >= 2 &&
                            line[0] == UNI_COMMENT &&
                            line[1] == UNI_COMMENT)
                           page << line.to_HTML(2)  <<                     CRLF;
                      }
                 }
              else
                 page << "-"                                               CRLF;
            }
      }
   page <<
"    </TABLE>"                                                             CRLF

"   <H2>Variables</H2>"                                                    CRLF
"    <TABLE class=vartab>"                                                 CRLF
"     <TR>"                                                                CRLF
"      <TH>Variable"                                                       CRLF
"      <TH>SI"                                                             CRLF
"      <TH>⍴⍴"                                                             CRLF
"      <TH>⍴"                                                              CRLF
"      <TH>≡"                                                              CRLF
"      <TH>Type"                                                           CRLF;
   loop(v, variables.size())
      {
        const Symbol & var_sym = *variables[v];
        loop(si, var_sym.value_stack_size())
            {
              if (var_sym[si].name_class == NC_VARIABLE)
                 {
                   Assert(!!var_sym[si].apl_val);
                   const Value * value = var_sym[si].apl_val.get();
                   page << "  <tr>"                                       CRLF
                            "   <td class=code>" << var_sym.get_name() <<  CRLF
                            "   <td class=code>" << si <<                  CRLF
                            "   <td class=code>" << value->get_rank() <<   CRLF
                            "   <td class=code>";
                   loop(r, value->get_rank())
                       page << " " << value->get_shape_item(r);
                   page << CRLF << "   <td class=code>"
                         << value->compute_depth()                      << CRLF;

                   const CellType ct = value->deep_cell_types();
                   if ((ct & CT_NUMERIC) && (ct & CT_CHAR))
                      page <<   "   <td class=code>Mixed"                 CRLF;
                   else if (ct & CT_NUMERIC)
                      page <<   "   <td class=code>Numeric"               CRLF;
                   else if (ct & CT_CHAR)
                      page <<   "   <td class=code>Character"             CRLF;
                   else
                      page <<   "   <td class=code>? ? ?"                 CRLF;

                 }
            }
      }
   page <<
"    </TABLE>"                                                             CRLF
" </body>"                                                                 CRLF;

   page.close();
}
//-----------------------------------------------------------------------------
void
Doxy::function_page(const UserFunction * ufun)
{
UTF8_string fun_filename(root_dir);
   fun_filename.append_str("/f_");
   fun_filename.append(UTF8_string(ufun->get_name()));
   fun_filename.append_str(".html");

   Log(LOG_command_DOXY)
      out << "Writing function HTML file " << fun_filename << endl;

ofstream page(fun_filename.c_str());
   page <<
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""                      CRLF
"                      \"http://www.w3.org/TR/html4/strict.dtd\">"         CRLF
"<HTML>"                                                                   CRLF
"  <HEAD>"                                                                 CRLF
"    <TITLE>Documentation of function" << ufun->get_name() << "</TITLE> "  CRLF
"    <META http-equiv=\"content-type\" "                                   CRLF
"          content=\"text/html; charset=UTF-8\">"                          CRLF
"   <LINK rel='stylesheet' type='text/css' href='apl_doxy.css'>"           CRLF
" </HEAD>"                                                                 CRLF
" <BODY>"                                                                  CRLF
"  <H1>Function " << ufun->get_name() << "</H1>"                           CRLF
"  <H2><A href=index.html>→HOME</A></H2>"                                  CRLF;

   page << "   <pre>" CRLF "    ∇ ";
   loop(l, ufun->get_text_size())
       {
         const UCS_string & line(ufun->get_text(l));
         if (l > 99)       page << "[" << l << "]";
         else if (l > 9)   page << "[" << l << "] ";
         else if (l > 0)   page << "[" << l << "]  ";
         page << line.to_HTML(0) <<                                        CRLF;
       }
   page << "    ∇" CRLF "   </pre>"                                        CRLF;

   set_call_graph_root(ufun);
   if (write_call_graph(ufun, false) == 0)
      {
        page << "   <H2>Call Graph from function "
             << ufun->get_name() << "</H2>"                                CRLF
                "    <IMG class=cg1 src=cg_" << ufun->get_name() << ".png>"          CRLF;
      }
   swap_caller_calee();

   set_call_graph_root(ufun);
   if (write_call_graph(ufun, true) == 0)
      {
        page << "   <H2>Caller Graph to function "
             << ufun->get_name() << "</H2>"                                CRLF
             << "<IMG class=cg2 src=gc_" << ufun->get_name() << ".png>"               CRLF;
      }

   swap_caller_calee();   // restore

   page <<
" </BODY>"                                                                 CRLF
" </HTML>"                                                                 CRLF;

   page.close();
}
//-----------------------------------------------------------------------------

void
Doxy::bold_name(ostream & of, const UserFunction * ufun)
{
const UserFunction_header & header = ufun->get_header();
const char * bold = "<span style='font-weight: bold'>";

   if (header.Z())   of << header.Z()->get_name() << "←";
   if (header.A())   of << header.A()->get_name() << " ";
   if (header.LO())   // operator
      {
        of << "(" << header.LO()->get_name() << " ";
        of << bold << header.get_name() << "</span>";
        if (header.RO())   of << " " << header.RO()->get_name();
      }
   else               // function
      {
        of << bold << header.get_name() << "</span>";
      }

   if (header.B())   of << " " << header.B()->get_name();
}
//-----------------------------------------------------------------------------
void
Doxy::make_call_graph(const Simple_string<const Symbol *, false> & all_fns)
{
   loop(f, all_fns.size())
      {
        const Symbol & fun_sym = *(all_fns[f]);
        loop(si, fun_sym.value_stack_size())
            {
              if (fun_sym[si].name_class == NC_FUNCTION ||
                  fun_sym[si].name_class == NC_OPERATOR)
                 {
                   const Function * fp = fun_sym[si].sym_val.function;
                   Assert(fp);
                   const UserFunction * ufun = fp->get_ufun1();
                   if (ufun)   add_fun_to_call_graph(&fun_sym, ufun);
                 }
            }
      }
}
//-----------------------------------------------------------------------------
void
Doxy::add_fun_to_call_graph(const Symbol * sym, const UserFunction * ufun)
{
   Assert(sym->get_name() == ufun->get_name());

   Log(LOG_command_DOXY)
      out << "add Symbol " << ufun->get_name() << endl;

const Token_string & body = ufun->get_body();
   loop(b, body.size())
      {
        const Token & tok = body[b];
        if (tok.get_Class() != TC_SYMBOL)   continue;
        const Symbol & callee_sym = *tok.get_sym_ptr();
        Assert(&callee_sym);
        loop(si, callee_sym.value_stack_size())
            {
              if (callee_sym[si].name_class == NC_FUNCTION ||
                  callee_sym[si].name_class == NC_OPERATOR)
                 {
                   const Function * fp = callee_sym[si].sym_val.function;
                   Assert(fp);
                   const UserFunction * callee = fp->get_ufun1();
                   if (!callee)   continue;

                    // ignore multiple calls of the same callee from the
                    // same caller...
                    //
                    bool have_call = false;
                    loop(cg, call_graph.size())
                        {
                          if (call_graph[cg].caller == ufun &&
                              call_graph[cg].callee == callee)
                             {
                               have_call = true;
                               break;   // loop(cg, ...
                             }
                        }

                    if (!have_call)
                       {
                         const fcall_edge edge = { ufun, callee, 0 };
                         call_graph.append(edge);
                         Log(LOG_command_DOXY)
                            out << "    " << ufun->get_name()
                                << " calls " << callee->get_name() << endl;
                       }
                 }
            }
      }
}
//-----------------------------------------------------------------------------
void
Doxy::set_call_graph_root(const UserFunction * ufun)
{
const int MAX = 2 * call_graph.size();   // MAX means unreachable

   // 1. set all edges staring at ufun to 0 and all others unreachable
   //
   loop(cg, call_graph.size())
       {
        fcall_edge & edge = call_graph[cg];
        if (edge.caller == ufun)   edge.value = 0;     // root
        else                       edge.value = MAX;   // unreachable
       }

bool progress = true;
   for (int depth = 0; progress; ++depth)
       {
         progress = false;
         loop(f, call_graph.size())
             {
               const fcall_edge & from = call_graph[f];
               if (from.value != depth)   continue;

               // 'from' starts at level depth.
               // Mark all edges 'to'at level depth + 1
               //
               loop(t, call_graph.size())
                  {
                    fcall_edge & to = call_graph[t];
                    if (from.callee != to.caller)   continue;
                    if (to.value < (depth + 1))     continue;
                    to.value = depth + 1;
                    progress = true;
                  }
             }
       }

   loop(e, call_graph.size())
       {
         const fcall_edge & edge = call_graph[e];
         if (edge.value == MAX)   continue;   // not reachable
         Log(LOG_command_DOXY)
            out << "edge " << edge.caller->get_name() << " → "
                << edge.callee->get_name()
                << " has distance " << edge.value
                << " from " << ufun->get_name() << endl;
       }

   // create a list of nodes that are reachable
   //
   nodes.shrink(0);
   loop(e, call_graph.size())
       {
         const fcall_edge & edge = call_graph[e];
         if (edge.value == MAX)   continue;   // not reachable
         bool caller_is_new = true;
         bool callee_is_new = true;
         loop(n, nodes.size())
             {
               if (nodes[n] == edge.caller)   caller_is_new = false;
               if (nodes[n] == edge.callee)   callee_is_new = false;
             }

          if (caller_is_new)   nodes.append(edge.caller);
          if (callee_is_new)   nodes.append(edge.callee);
       }

   Log(LOG_command_DOXY)
      {
        loop(n, nodes.size())
        out << "Node: " << nodes[n]->get_name() << endl;
      }
}
//-----------------------------------------------------------------------------
int
Doxy::write_call_graph(const UserFunction * ufun, bool caller)
{
UTF8_string cg_filename(root_dir);
   if (caller)   cg_filename.append_str("/gc_");
   else          cg_filename.append_str("/cg_");
   cg_filename.append(UTF8_string(ufun->get_name()));
UTF8_string png_filename(cg_filename);
   cg_filename.append_str(".gv");
   png_filename.append_str(".png");

   Log(LOG_command_DOXY)
      out << "Writing function call graph .gv file " << cg_filename << endl;

ofstream gv(cg_filename.c_str());

const char * node_attributes = " shape=rect"
                               " fillcolor=\"#C0C0C0\"";
const char * node0_attributes = " shape=rect"
                               " style=filled"
                               " fillcolor=\"#C0C0C0\"";

   gv << "digraph" << endl
      << "{"       << endl
      << "  graph [rankdir=\"LR\"];" << endl;

   if (nodes.size() == 0)   // no edges, single node
      {
        gv << "n0 [label=" << ufun->get_name()
           << node0_attributes << "];" << endl;
      }
   else   // normal graph
      {
        // create nodes
        //
        loop(n, nodes.size())
            {
              const UserFunction * un = nodes[n];
              if (un == ufun)
                 gv << "n" << n << " [label= " << un->get_name()
                    << node0_attributes << "];" << endl;
              else
                 gv << "n" << n << " [label= " << un->get_name()
                    << node_attributes << "];" << endl;
            }

        // create edges
        //
        loop(e, call_graph.size())
           {
             const fcall_edge & edge = call_graph[e];
             if (edge.value >= call_graph.size())   continue;   // not reachable
             const int n0 = node_ID(edge.caller);
             const int n1 = node_ID(edge.callee);
             Assert(n0 != -1);
             Assert(n1 != -1);
             if (caller)   gv << "n" << n1 << " -> n" << n0 << endl;
             else          gv << "n" << n0 << " -> n" << n1 << endl;
           }
      }

   gv << "}" << endl;

   gv.close();

   return gv_to_png(cg_filename.c_str(), png_filename.c_str());
}
//-----------------------------------------------------------------------------
int Doxy::node_ID(const UserFunction * ufun)
{
   loop(n, nodes.size())
      if (nodes[n] == ufun)   return n;

   return -1;
}
//-----------------------------------------------------------------------------
void
Doxy::swap_caller_calee()
{
   loop(cg, call_graph.size())
       {
         const fcall_edge edge = call_graph[cg];
         call_graph[cg].caller = edge.callee;
         call_graph[cg].callee = edge.caller;
       }
}
//-----------------------------------------------------------------------------
int
Doxy::gv_to_png(const char * gv_filename, const char * png_filename)
{
   // convert .gv file gv_filename to .png file png_filename using the 'dot'
   // program

char cmd[200];
   snprintf(cmd, sizeof(cmd), "dot -Tpng %s -o%s", gv_filename, png_filename);
   errno = 0;
FILE * dot = popen(cmd, "r");
   if (dot == 0)
      {
        if (errno)
           CERR << "Could not run 'dot': " << strerror(errno) << endl;
        else
           CERR << "Could not run 'dot'. Maybe install package 'graphviz'?"
                << endl;
        ++errors;
        return 1;
      }

char buffer[1000];
   for (;;)
       {
         const int len = fread(buffer, 1, sizeof(buffer), dot);
         if (len == 0)   break;
         buffer[len] = 0;
         CERR << "dot says: " << buffer;
       }

   pclose(dot);
   Log(LOG_command_DOXY)
      out << "converted " << gv_filename << " to " << png_filename << endl;

   unlink(gv_filename);
   return 0;
}
//-----------------------------------------------------------------------------
