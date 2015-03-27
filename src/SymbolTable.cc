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

#include <string.h>
#include <strings.h>

#include "CDR.hh"
#include "Command.hh"
#include "Function.hh"
#include "IndexExpr.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "SymbolTable.hh"
#include "SystemVariable.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
SymbolTable::SymbolTable()
{
   memset(symbol_table, 0, sizeof(symbol_table));
}
//-----------------------------------------------------------------------------
NamedObject *
SymbolTable::lookup_existing_name(const UCS_string & name)
{
   Assert(!Avec::is_quad(name[0]));   // should not be called for ⎕xx

Symbol * sym = lookup_existing_symbol(name);
   if (sym == 0)   return 0;

   switch(sym->get_nc())
      {
        case NC_VARIABLE: return sym;

        case NC_FUNCTION:
        case NC_OPERATOR: return sym->get_function();
        default:          return 0;
      }
}
//-----------------------------------------------------------------------------
Symbol *
SymbolTable::lookup_existing_symbol(const UCS_string & sym_name)
{
   Assert(!Avec::is_quad(sym_name[0]));   // should not be called for ⎕xx

uint32_t hash = FNV_Offset_32;
   loop(s, sym_name.size())   hash = hash*FNV_Prime_32 ^ sym_name[s];
   hash = ((hash >> 16) ^ hash) & 0x0000FFFF;

   for (Symbol * sp = symbol_table[hash]; sp; sp = sp->next)
       {
         if (!sp->is_erased() && sp->equal(sym_name))   return sp;
       }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
SymbolTable::lookup_symbol(const UCS_string & sym_name)
{
   Assert(!Avec::is_quad(sym_name[0]));   // should not be called for ⎕xx

   // compute hash for sym_name
   //
uint32_t hash = FNV_Offset_32;
   loop(s, sym_name.size())   hash = hash*FNV_Prime_32 ^ sym_name[s];
   hash = ((hash >> 16) ^ hash) & 0x0000FFFF;

Symbol * sp = symbol_table[hash];

   if (symbol_table[hash] == 0)   // unused hash value
      {
        // sym_name is the first symbol with this hash.
        // create a new symbol, insert it into symbol_table, and return it.
        //
        Log(LOG_SYMBOL_lookup_symbol)
           {
             CERR << "Symbol " << sym_name << " has hash " << HEX(hash) << endl;
           }

        Symbol * new_symbol = new Symbol(sym_name, ID::USER_SYMBOL);
        symbol_table[hash] =  new_symbol;
        return new_symbol;
      }

   // One or more symbols with this hash already exist. The number of symbols
   // with the same name is usually very short, so we walk the list twice.
   // The first walk checks for a (possibly erased) symbol with the
   // same name and return it if found.
   //
   for (Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
       {
         if (sym->equal(sym_name))   // found
            {
              sym->set_erased(false);
              return sym;
            }
       }

   // no symbol with name sym_name exists. The second walk:
   //
   // 1. search for an erased symbol and override it, or
   // 2. append a new symbol at the end if no erased symbol was seen.
   //
int pos = 0;
   for (Symbol * sym = symbol_table[hash]; ; sym = sym->next, ++pos)
       {
         if (sym->is_erased())   // override an erased symbol.
            {
              sym->set_erased(false);
              sym->name = sym_name;

              Log(LOG_SYMBOL_lookup_symbol)
                 {
                   CERR << "Symbol " << sym_name << " has hash "
                        << HEX(hash) << endl;
                 }
              return sp;
            }

         if (sym->next == 0)   // append a new symbol at the end
            {
              if (pos >= 255)
                 throw_apl_error(E_SYSTEM_LIMIT_SYMTAB, LOC);

              sym->next = new Symbol(sym_name, ID::USER_SYMBOL);

              Log(LOG_SYMBOL_lookup_symbol)
                 {
                   CERR << "Symbol " << sym_name << " has hash "
                        << HEX(hash) << endl;
                 }

              return sym->next;
            }
       }

   Assert(0 && "Not reached");
}
//-----------------------------------------------------------------------------
ostream &
SymbolTable::list_symbol(ostream & out, const UCS_string & buf) const
{
Token_string tos;
      {
        Tokenizer tokenizer(PM_STATEMENT_LIST, LOC);
        if (tokenizer.tokenize(buf, tos) != E_NO_ERROR)
           {
             CERR << "invalid token" << endl;
             return out;
           }
      }

   if (tos.size() == 0)   // empty argument
      {
        CERR << "no symbol" << endl;
        return out;
      }

   if (tos[0].get_ValueType() != TV_SYM)
      {
        CERR << "not a symbol" << endl;
        return out;
      }

Symbol * symbol = tos[0].get_sym_ptr();
   return symbol->print_verbose(out);
}
//-----------------------------------------------------------------------------
void
SymbolTable::list(ostream & out, ListCategory which, UCS_string from_to) const
{
UCS_string from;
UCS_string to;
   {
     const bool bad_from_to = Command::parse_from_to(from, to, from_to);
     if (bad_from_to)
        {
          CERR << "bad range argument " << from_to
               << ", expecting from - to" << endl;
          return;
        }
   }

   // put those symbols into 'list' that satisfy 'which'
   //
vector<Symbol *> list;
int symbol_count = 0;
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               ++symbol_count;

               // check range
               //
               if (from.size() && sym->get_name().lexical_before(from))
                  {
                    // CERR << "'" << sym->get_name() << "' comes before '"
                    //       << from << "'" << endl;
                    continue;
                  }
               if (to.size() && to.lexical_before(sym->get_name()))
                  {
                    // CERR << "'" << to << "' comes before '"
                    //      << sym->get_name() << "'" << endl;
                    continue;
                  }

               if (sym->value_stack.size() == 0)
                  {
                    if (which == LIST_ALL)
                       {
                         list.push_back(sym);
                       }
                    continue;
                  }

               if (sym->is_erased() && !(which & LIST_ERASED))   continue;

               const NameClass nc = sym->value_stack.back().name_class;
               if (((nc == NC_VARIABLE)         && (which & LIST_VARS))    ||
                   ((nc == NC_FUNCTION)         && (which & LIST_FUNS))    ||
                   ((nc == NC_OPERATOR)         && (which & LIST_OPERS))   ||
                   ((nc == NC_LABEL)            && (which & LIST_LABELS))  ||
                   ((nc == NC_LABEL)            && (which & LIST_VARS))    ||
                   ((nc == NC_INVALID)          && (which & LIST_INVALID)) ||
                   ((nc == NC_UNUSED_USER_NAME) && (which & LIST_UNUSED)))
                   {
                     list.push_back(sym);
                   }
             }
       }

   if (which == LIST_NONE)   // )SYMBOLS: display total symbol count
      {
        // this could be:
        //
        // 1. )SYMBOLS or   (show symbol count)
        // 2. )SYMBOLS N    (set symbol count, ignored by GNU APL)
        //
        if (from_to.size())   return;   // case 2
        out << "IS " << symbol_count << endl;
        return;
      }

const int count = list.size();
vector<UCS_string> names;
   loop(l, count)
      {
        UCS_string name = list[l]->get_name();
        if (which == LIST_NAMES)   // append .NC
           {
             name.append(UNI_ASCII_FULLSTOP);
             name.append_number(list[l]->value_stack.back().name_class);
           }
        names.push_back(name);
      }

DynArray(const UCS_string *, sorted_names, count);
   loop(n, count)   sorted_names[n] = &names[n];
   UCS_string::sort_names(sorted_names, count);

   // figure column widths
   //
const int qpw = Workspace::get_PrintContext().get_PW();
vector<int> col_width;
   enum { tabsize = 4 };
   UCS_string::compute_column_width(col_width, sorted_names, count,
                                    tabsize, qpw);

   loop(c, count)
      {
        const int col = c % col_width.size();
        out << *sorted_names[c];
        if (col == (col_width.size() - 1) || c == (count - 1))
           {
             // last column or last item: print newline
             //
             out << endl;
           }
        else
           {
             // intermediate column: print spaces
             //
             const int len = tabsize*col_width[col] - sorted_names[c]->size();
             Assert(len > 0);
             loop(l, len)   out << " ";
           }
      }
}
//-----------------------------------------------------------------------------
void
SymbolTable::unmark_all_values() const
{
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               sym->unmark_all_values();
             }
       }
}
//-----------------------------------------------------------------------------
int
SymbolTable::show_owners(ostream & out, const Value & value) const
{
int count = 0;
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               count += sym->show_owners(out, value);
             }
       }
   return count;
}
//-----------------------------------------------------------------------------
void
SymbolTable::write_all_symbols(FILE * out, uint64_t & seq) const
{
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               sym->write_OUT(out, seq);
             }
       }
}
//-----------------------------------------------------------------------------
void
SymbolTable::erase_symbols(ostream & out, const vector<UCS_string> & symbols)
{
int error_count = 0;
   loop(s, symbols.size())
       {
         const bool error = erase_one_symbol(symbols[s]);
         if (error)
            {
              ++error_count;
              if (error_count == 1)   // first error
                 out << "NOT ERASED:";
              out << " " << symbols[s];
            }
       }

   if (error_count)   out << endl;
}
//-----------------------------------------------------------------------------
void
SymbolTable::clear(ostream & out)
{
   // SymbolTable::clear() should only be called after Workspace::clear_SI()
   //
   Assert(Workspace::SI_entry_count() == 0);

   loop(hash, MAX_SYMBOL_COUNT)   clear_slot(out, hash);
}
//-----------------------------------------------------------------------------
void
SymbolTable::clear_slot(ostream & out, int hash)
{
Symbol * sym = symbol_table[hash];
   if (sym == 0)   return;   // no symbol with this hash

   symbol_table[hash] = 0;

Symbol * next;
   for (; sym; sym = next)
       {
         next = sym->next;

         // keep not user-defined symbols ??? ARE THERE ANY ???
         //
         if (!sym->is_user_defined())
            {
              sym->next = symbol_table[hash];
              symbol_table[hash] = sym;
              continue;
            }

         delete sym;
       }

}
//-----------------------------------------------------------------------------
bool
SymbolTable::erase_one_symbol(const UCS_string & sym)
{
Symbol * symbol = lookup_existing_symbol(sym);

   if (symbol == 0)
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': unknown symbol ");
        return true;
      }

   if (symbol->is_erased())
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': already erased");
        return true;
      }

   if (symbol->value_stack.size() != 1)
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': symbol is pushed (localized)");
        return true;
      }

   if (Workspace::is_called(sym))
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': symbol is called (is on SI)");
        return true;
      }

ValueStackItem & tos = symbol->value_stack[0];

   switch(tos.name_class)
      {
        case NC_LABEL:
             Assert(0 && "should not happen since stack height == 1");
             return true;

        case NC_VARIABLE:
             symbol->expunge();
             symbol->set_erased(true);
             return false;

        case NC_UNUSED_USER_NAME:
             symbol->set_erased(true);
             return true;

        case NC_FUNCTION:
        case NC_OPERATOR:
             Assert(tos.sym_val.function);
             if (tos.sym_val.function->is_native())
                {
                  symbol->expunge();
                  return false;
                }

             {
               const UserFunction * ufun = tos.sym_val.function->get_ufun1();
               Assert(ufun);
               if (Workspace::oldest_exec(ufun))
                  {
                    UCS_string & t4 = Workspace::more_error();
                    t4.clear();
                    t4.append_utf8("Can't )ERASE symbol '");
                    t4.append(sym);
                    t4.append_utf8("':  pushed on SI-stack");
                    return true;
                  }
             }

             delete tos.sym_val.function;
             tos.sym_val.function = 0;
             tos.name_class = NC_UNUSED_USER_NAME;
             symbol->set_erased(true);
             return false;

        default: break;
      }

    Assert(0 && "Bad name_class in SymbolTable::erase_one_symbol()");
   return true;
}
//-----------------------------------------------------------------------------
int
SymbolTable::symbols_allocated() const
{
int count = 0;

   loop(hash, MAX_SYMBOL_COUNT)
      {
        for (const Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
            ++count;
      }

   return count;
}
//-----------------------------------------------------------------------------
void
SymbolTable::get_all_symbols(Symbol ** table, int table_size) const
{
int idx = 0;

   loop(hash, MAX_SYMBOL_COUNT)
      {
        for (Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
            {
              Assert(idx < table_size);
              table[idx++] = sym;
            }
      }
}
//-----------------------------------------------------------------------------
void
SymbolTable::dump(ostream & out, int & fcount, int & vcount) const
{
vector<const Symbol *> symbols;
   loop(hash, MAX_SYMBOL_COUNT)
      {
        for (const Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
            {
              if (sym->is_erased())              continue;
              if (sym->value_stack_size() < 1)   continue;
              symbols.push_back(sym);
            }
      }

   // sort symbols by name
   //
   loop(d, symbols.size())
      {
        for (ShapeItem j = d + 1; j < symbols.size(); ++j)
            {
              if (symbols[d]->get_name().compare(symbols[j]->get_name()) > 0)
                 {
                   const Symbol * ss = symbols[d];
                   symbols[d] = symbols[j];
                   symbols[j] = ss;
                 }
            }
      }

   // pass 1: functions
   //
   loop(s, symbols.size())
      {
        const Symbol & sym = *symbols[s];
        const ValueStackItem & vs = sym[0];
         if      (vs.name_class == NC_FUNCTION)   { ++fcount;   sym.dump(out); }
         else if (vs.name_class == NC_OPERATOR)   { ++fcount;   sym.dump(out); }
      }

   // pass 2: variables
   //
   loop(s, symbols.size())
      {
        const Symbol & sym = *symbols[s];
        const ValueStackItem & vs = sym[0];
        if (vs.name_class == NC_VARIABLE)   { ++vcount;   sym.dump(out); }
      }
}
//-----------------------------------------------------------------------------
