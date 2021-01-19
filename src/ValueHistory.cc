/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2020  Dr. Jürgen Sauermann

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

#include <vector>

#include <iomanip>

#include "Common.hh"
#include "Error.hh"
#include "InputFile.hh"
#include "PrintOperator.hh"
#include "UCS_string.hh"
#include "Value.hh"
#include "ValueHistory.hh"

VH_entry VH_entry::history[VALUEHISTORY_SIZE + 1];
int VH_entry::idx = 0;

//----------------------------------------------------------------------------
VH_entry::VH_entry(const Value * _val, VH_event _ev, int _iarg,
                   const char * _loc)
  : val(_val),
    event(_ev),
    iarg(_iarg),
    loc(_loc)
{
   testcase_file = InputFile::current_filename();
   testcase_line = InputFile::current_line_no();
}
//----------------------------------------------------------------------------
void
VH_entry::init()
{
void * h = history;
   memset(h, 0, sizeof(history));
}
//----------------------------------------------------------------------------
void
add_event(const Value * val, VH_event ev, int ia, const char * loc)
{
   if (loc == 0)
      {
        if (val)   loc = val->where_allocated();
        else       loc = LOC;
      }

VH_entry * entry = VH_entry::history + VH_entry::idx++;
   if (VH_entry::idx >= VALUEHISTORY_SIZE)   VH_entry::idx = 0;

   new(entry) VH_entry(val, ev, ia, loc);
}
//----------------------------------------------------------------------------
void
VH_entry::print_history(std::ostream & out, const Value * val, const char * loc)
{
   // search backwards for events of val.
   //
std::vector<const VH_entry *> var_events;
int cidx = VH_entry::idx;

   loop(e, VALUEHISTORY_SIZE)
       {
         --cidx;
         if (cidx < 0)   cidx = VALUEHISTORY_SIZE - 1;

         const VH_entry * entry = VH_entry::history + cidx;

         if (entry->event == VHE_None)     break;      // end of history

         if (entry->event == VHE_Error)
            { 
              // add error event to every value history
              //
              var_events.push_back(entry);
              continue;
            }

          if (entry->val != val)            continue;   // some other var

          var_events.push_back(entry);

          if (entry->event == VHE_Create)   break;   // create event found
       }

   if (VALUEHISTORY_SIZE == 0)
      {
        out << "value history disabled" << std::endl;
      }
    else
      {
        out << std::endl << "value " << voidP(val)
            << " has " << var_events.size()
            << " events in its history";
        if (loc)   out << " (at " << loc << ")";
        out << ":" << std::endl;
      }

int flags = 0;
const VH_entry * previous = 0;
   loop(e, var_events.size())
      {
        const VH_entry * vev = var_events[var_events.size() - e - 1];
        vev->print(flags, out, val, previous);
        previous = vev;
      }
   out << std::endl;
}
//----------------------------------------------------------------------------
static UCS_string
flags_name(ValueFlags flags)
{
UCS_string ret;

  if (flags & VF_marked)   ret.append(UNI_M);
  if (flags & VF_complete) ret.append(UNI_C);

   while (ret.size() < 4)   ret.append(UNI_SPACE);
   return ret;
}
//----------------------------------------------------------------------------
void
VH_entry::print(int & flags, std::ostream & out, const Value * val,
               const VH_entry * previous) const
{
const ValueFlags flags_before = ValueFlags(flags);

   if (previous == 0                            ||
       previous->testcase_file == 0             ||
       previous->testcase_line != testcase_line ||
       strcmp(previous->testcase_file, testcase_file))
      {
        if (testcase_file)   out << "  FILE:        " << testcase_file
                                 << ":" << testcase_line << std::endl;
      }

   switch(event)
      {
        case VHE_Create:
             out << "  VHE_Create   " << flags_before
                 << "              ";
             break;

        case VHE_Unroll:
             out << "  VHE_Unroll   " << flags_before
                 << "              ";
             break;

        case VHE_Check:
             flags |= VF_complete;
             out << "  VHE_Check   " << flags_before << " ";
             if (flags_before != flags)   out << ValueFlags(flags)
                                              << " ";
             else                         out << "            ";
             break;

        case VHE_SetFlag:
             flags |= iarg;
             out << "  Set " << flags_name(ValueFlags(iarg))
                 << "     " << flags_before << " ";
             if (flags_before != flags)   out << ValueFlags(flags)
                                              << " ";
             else                         out << "            ";
             break;

        case VHE_ClearFlag:
             flags &= ~iarg;
             out << "  Clear " << flags_name(ValueFlags(iarg))
                 << "   " << flags_before << " ";
             if (flags_before != flags)   out << ValueFlags(flags)
                                              << " ";
             else                         out << "            ";
             break;

        case VHE_Erase:
             out << "  VHE_Erase    " << flags_before << "             ";
             break;

        case VHE_Destruct:
             out << "  VHE_Destruct " << std::setw(26) << iarg;
             break;

        case VHE_Error:
             out << "  " << std::setw(38)
                 << Error::error_name(ErrorCode(iarg)) << " ";
             break;

        case VHE_PtrNew:
             out << "  VHE_PtrNew   " << std::setw(26) << iarg;
             break;

        case VHE_PtrNew0:
             out << "  VHE_PtrNew0  " << std::setw(26) << iarg;
             break;

        case VHE_PtrCopy1:
             out << "  VHE_PtrCopy1 " << std::setw(26) << iarg;
             break;

        case VHE_PtrCopy2:
             out << "  VHE_PtrCopy2 " << std::setw(26) << iarg;
             break;

        case VHE_PtrCopy3:
             out << "  VHE_PtrCopy3 " << std::setw(26) << iarg;
             break;

        case VHE_PtrClr:
             out << "  VHE_PtrClr   " << std::setw(26) << iarg;
             break;

        case VHE_PtrDel:
             out << "  VHE_PtrDel   " << std::setw(26) << iarg;
             break;

        case VHE_PtrDel0:
             out << "  VHE_PtrDel0  " << std::setw(26) << iarg;
             break;

        case VHE_TokCopy1:
             out << "  VHE_TokCopy1 " << std::setw(26) << iarg;
             break;

        case VHE_TokMove1:
             out << "  VHE_TokMove1 " << std::setw(26) << iarg;
             break;

        case VHE_TokMove2:
             out << "  VHE_TokMove2 " << std::setw(26) << iarg;
             break;

        case VHE_Completed:
             out << "  VHE_Completed" << std::setw(26) << iarg;
             break;

        case VHE_Stale:
             out << "  VHE_Stale    " << std::setw(26) << iarg;
             break;

        case VHE_Visit:
             out << "  VHE_Visit    " << iarg
                 << "            " << flags_before << " ";
             break;

        default:
             out << "Unknown event  " << HEX(event) << std::endl;
             return;
      }

   out << std::left << std::setw(30) << (loc ? loc : "<no-loc>") << std::endl;
}
//----------------------------------------------------------------------------

