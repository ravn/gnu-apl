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
extern void cleanup();

/// true if Control-C was hit
extern bool attention_raised;

/// true if Control-C was hit twice within 500 ms
extern bool interrupt_raised;

//-----------------------------------------------------------------------------

/// a structure that contains user preferences from different sources
/// (command line arguments, config files, environment variables ...)
struct user_preferences
{
   user_preferences()
   : silent(false),
     emacs_mode(false),
     do_not_echo(false),
     safe_mode(false),
     do_svars(true),
     do_CONT(true),
     do_Color(true),
     requested_id(0),
     requested_par(0),
     requested_cc(CCNT_UNKNOWN),
     daemon(false),
     append_summary(false),
     wait_ms(0),
     randomize_testfiles(false)
   {}

   /// parse command line parameters
   int parse_argv(int argc, const char * argv[]);

   /// true if no banner/Goodbye is wanted.
   bool silent;

   /// true if emacs mode is wanted
   bool emacs_mode;

   /// true if no banner/Goodbye is wanted.
   bool do_not_echo;

   /// true if --safe command line option was given
   bool safe_mode;

   /// true if shared variables are enabled
   bool do_svars;

   /// load workspace CONTINUE on start-up
   bool do_CONT;

  /// output coloring enabled
   bool do_Color;

   /// desired --id (⎕AI[1] and shared variable functions)
   int requested_id;

   /// desired --par (⎕AI[1] and shared variable functions)
   int requested_par;

   /// desired core count
   CoreCount requested_cc;

   /// run as deamon
   bool daemon;

   /// append test results to summary.log rather than overriding it
   bool append_summary;

   /// wait at start-up
   int wait_ms;

   /// randomize the order of testfiles
   bool randomize_testfiles;
};

extern user_preferences uprefs;

