/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

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

#include "PrintContext.hh"
#include "Workspace.hh"

//----------------------------------------------------------------------------
PrintContext::PrintContext(const Workspace & ws)
: style(ws.get_PS()),
  precision(ws.get_PP()),
  width(ws.get_PW())
{
}
//----------------------------------------------------------------------------
PrintContext::PrintContext(PrintStyle st, const Workspace & ws)
: style(st),
  precision(ws.get_PP()),
  width(ws.get_PW())
{
}
//----------------------------------------------------------------------------

