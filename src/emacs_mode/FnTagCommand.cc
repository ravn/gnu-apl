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
#include "FnTagCommand.hh"
#include "emacs.hh"

#include "../UserFunction.hh"

#include <sstream>

void FnTagCommand::run_command( NetworkConnection &conn, const std::vector<std::string> &args )
{
    std::string name = args[1];

    std::stringstream out;

    UCS_string ucs_name = ucs_string_from_string(name);
    const NamedObject * obj = Workspace::lookup_existing_name(ucs_name);
    if( obj == NULL ) {
        out << "undefined\n";
    }
    else if( !obj->is_user_defined() ) {
        out << "system function\n";
    }
    else {
        const Function *function = obj->get_function();
        if( function == NULL ) {
            out << "symbol is not a function\n";
        }
        else if( function->get_exec_properties()[0] != 0 ) {
            out << "function is not executable\n";
        }
        else {
            const UserFunction *ufun = function->get_func_ufun();
            if( ufun == NULL ) {
                out << "not a user function";
            }
            else {
                UTF8_string creator = ufun->get_creator();
                out << "tag\n" << creator.c_str() << "\n";
            }
        }
    }
    out << END_TAG << "\n";

    conn.write_string_to_fd( out.str() );
}
