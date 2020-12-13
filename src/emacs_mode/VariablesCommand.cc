/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2014  Elias Mårtenson

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

#include "NetworkConnection.hh"
#include "VariablesCommand.hh"
#include "emacs.hh"

#include <sstream>

enum TypeSpec {
    ALL,
    VARIABLE,
    FUNCTION
};

void VariablesCommand::run_command( NetworkConnection &conn, const std::vector<std::string> &args )
{
    stringstream out;
    bool tagged = false;

    TypeSpec cls = ALL;
    if( args.size() >= 2 ) {
        string typespec = args[1];
        if( typespec == "variable" ) {
            cls = VARIABLE;
        }
        else if( typespec == "function" ) {
            cls = FUNCTION;
        }
        else if( typespec == "tagged" ) {
            cls = ALL;
            tagged = true;
        }
        else {
            CERR << "Illegal variable type: " << typespec << endl;
            throw DisconnectedError( "Illegal variable type" );
        }
    }

    std::vector<const Symbol *> symbols = Workspace::get_all_symbols();
    for (int i = 0 ; i < symbols.size() ; i++)
        {
          const Symbol * symbol = symbols[i];
          if (symbol->is_erased())   continue;  // hide erased symbols

          if (const ValueStackItem * tos = symbol->top_of_stack())
             {
               const NameClass symbol_nc = tos->get_nc();
               const bool symbol_is_var = symbol_nc == NC_VARIABLE;
               const bool symbol_is_fun = symbol_nc == NC_FUNCTION ||
                                          symbol_nc == NC_OPERATOR;
               if ((cls == ALL && (symbol_is_var || symbol_is_fun)) ||
                   (cls == VARIABLE && symbol_is_var)               ||
                   (cls == FUNCTION && symbol_is_fun))
                   {
                     out << symbol->get_name();
                     if(tagged)   out << " " << symbol_nc;
                     out << endl;
                   }
             }
        }

    out << END_TAG << "\n";
    conn.write_string_to_fd( out.str() );
}
