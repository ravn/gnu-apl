/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2018-2020  Dr. Jürgen Sauermann

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

#include "Plot_data.hh"
#include "Plot_line_properties.hh"

using namespace std;

//=============================================================================
int
Plot_line_properties::print(std::ostream & out) const
{
char cc[40];
# define ldef(ty,  na,  _val, _descr)                   \
   snprintf(cc, sizeof(cc), #na "-%d:  ",              \
            int(line_number + Workspace::get_IO()));   \
   CERR << setw(20) << cc << Plot_data::ty ## _to_str(na) << endl;
# include "Quad_PLOT.def"

   return 0;
}
//=============================================================================

