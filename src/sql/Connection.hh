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

#ifndef CONNECTION_HH
#define CONNECTION_HH

#include "apl-sqlite.hh"
#include "ArgListBuilder.hh"

#include <stdlib.h>

#include <vector>

class ColumnDescriptor {
public:
    ColumnDescriptor( const std::string &name_in, const std::string &type_in )
    : name(name_in),
      type(type_in)
    {}

    ColumnDescriptor operator=(ColumnDescriptor &orig)
       { return ColumnDescriptor( orig.name, orig.type ); }
    const std::string &get_name( void ) { return name; }
    const std::string &get_type( void ) { return type; }

private:
    const std::string name;
    const std::string type;
};

class Connection
{
public:
    virtual ~Connection() {}
    virtual ArgListBuilder *make_prepared_query( const std::string &sql ) = 0;
    virtual ArgListBuilder *make_prepared_update( const std::string &sql ) = 0;
    virtual void transaction_begin( void ) = 0;
    virtual void transaction_commit( void ) = 0;
    virtual void transaction_rollback( void ) = 0;
    virtual void fill_tables( std::vector<std::string> &tables ) = 0;
    virtual void fill_cols( const std::string &table, std::vector<ColumnDescriptor> &cols ) = 0;
    virtual const std::string make_positional_param( int pos ) = 0;

    virtual const std::string replace_bind_args( const std::string &sql );
};

#endif
