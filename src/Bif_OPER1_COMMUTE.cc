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

#include "Bif_OPER1_COMMUTE.hh"

Bif_OPER1_COMMUTE   Bif_OPER1_COMMUTE::_fun;
Bif_OPER1_COMMUTE * Bif_OPER1_COMMUTE::fun = &Bif_OPER1_COMMUTE::_fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_COMMUTE::eval_LB(Token & LO, Value_P B)
{
   return LO.get_function()->eval_AB(B, B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_COMMUTE::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   return LO.get_function()->eval_AXB(B, X, B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_COMMUTE::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   return LO.get_function()->eval_AB(B, A);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_COMMUTE::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   return LO.get_function()->eval_AXB(B, X, A);
}
//-----------------------------------------------------------------------------
