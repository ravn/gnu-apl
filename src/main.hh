/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2014  Dr. Jürgen Sauermann

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

/// clean up
extern void cleanup(bool soft);

/// true if Control-C was hit
extern bool attention_raised;
extern uint64_t attention_count;

/// true if Control-C was hit twice within 500 ms
extern bool interrupt_raised;
extern uint64_t interrupt_count;

/// time when ^C was hit last
extern APL_time_us interrupt_when;

extern void control_C(int);

extern const char * prog_name();

//-----------------------------------------------------------------------------

