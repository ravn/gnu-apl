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

#include <dirent.h>

#include "Value.icc"
#include "Common.hh"
#include "UTF8_string.hh"

class Workspace;

//-----------------------------------------------------------------------------
/*!
    Some command related functions, including the main input loop
    of the APL interpreter.
 */
class Command
{
public:
   /// process a single line entered by the user (in immediate execution mode)
   static void process_line();

   /// process \b line which contains a command or statements
   static void process_line(UCS_string & line);

   /// process \b line which contains an APL command. Return true iff the
   /// command was found
   static bool do_APL_command(ostream & out, UCS_string & line);

   /// process \b line which contains APL statements
   static void do_APL_expression(UCS_string & line);

   /// parse user-suplied argument (of )VARS, )OPS, or )NMS commands)
   /// into strings from and to
   static bool parse_from_to(UCS_string & from, UCS_string & to,
                             const UCS_string & user_input);

   /// return true if \b lib looks like a library reference (a 1-digit number
   /// or a path containing . or / chars
   static bool is_lib_ref(const UCS_string & lib);

   /// clean-up and exit from APL interpreter
   static void cmd_OFF(int exit_val);

   /// return the current boxing format
   static int get_boxing_format()
      { return boxing_format; }

   /// one user defined command
   struct user_command
      {
        UCS_string prefix;         ///< the first characters of the command
        UCS_string apl_function;   ///< APL function implementing the command
        int        mode;           ///< how left arg of apl_function is computed
      };

   /// one user defined command
   static vector<user_command> user_commands;

protected:
   /// show list of commands
   static void cmd_BOXING(ostream & out, const UCS_string & arg);

   /// check workspace integrity (stale Value and IndexExpr objects, etc)
   static void cmd_CHECK(ostream & out);

   /// )SAVE active WS as CONTINUE and )OFF
   static void cmd_CONTINUE(ostream & out);

   /// )DROP: delete a workspace file
   static void cmd_DROP(ostream & out, const vector<UCS_string> & args);

   /// )ERASE: erase symbols
   static void cmd_ERASE(ostream & out, vector<UCS_string> & args);

   /// show list of commands
   static void cmd_HELP(ostream & out);

   /// )HOST: execute OS command
   static void cmd_HOST(ostream & out, const UCS_string & arg);

   /// )IN: import a workspace file
   static void cmd_IN(ostream & out, vector<UCS_string> & args, bool protect);

   /// show US keyboard layout
   static void cmd_KEYB(ostream & out, const UCS_string & arg);

   /// open directory arg and follow symlinks
   static DIR * open_LIB_dir(UTF8_string & path, ostream & out,
                            const UCS_string & arg);

   /// list library: common helper
   static void lib_common(ostream & out, const UCS_string & args, int variant);

   /// return true if entry is a directory
   static bool is_directory(dirent * entry, const UTF8_string & path);

   /// list content of workspace and wslib directories: )LIB [N]
   static void cmd_LIB1(ostream & out, const UCS_string & args);

   /// list content of workspace and wslib directories: ]LIB [N]
   static void cmd_LIB2(ostream & out, const UCS_string & args);

   /// show list of commands
   static void cmd_LOG(ostream & out, const UCS_string & arg);

   /// list paths of workspace and wslib directories
   static void cmd_LIBS(ostream & out, const vector<UCS_string> & lib_ref);

   /// print more error info
   static void cmd_MORE(ostream & out);

   /// )OUT: export a workspace file
   static void cmd_OUT(ostream & out, vector<UCS_string> & args);

   static void cmd_USERCMD(ostream & out, const UCS_string & arg,
                           vector<UCS_string> & args);

   static void do_USERCMD(ostream & out, UCS_string & line,
                          const UCS_string & line1, const UCS_string & cmd,
                          vector<UCS_string> & args, int uidx);

   static bool check_name_conflict(ostream & out, const UCS_string & cnew,
                                   const UCS_string cold);

   static bool check_redefinition(ostream & out, const UCS_string & cnew,
                                  const UCS_string fnew, const int mnew);

   /// a helper struct for the )IN command
   struct transfer_context
      {
        /// constructor
        transfer_context(bool prot)
        : new_record(true),
          recnum(0),
          timestamp(0),
          protection(prot)
        {}

        /// process one record of a workspace file
        void process_record(const UTF8 * record,
                            const vector<UCS_string> & objects);

        /// get the name, rank, and shape of a 1 ⎕TF record
        uint32_t get_nrs(UCS_string & name, Shape & shape) const;

        /// process a 'A' (array in 2 ⎕TF format) item.
        void array_2TF(const vector<UCS_string> & objects) const;

        /// process a 'C' (character, in 1 ⎕TF format) item.
        void chars_1TF(const vector<UCS_string> & objects) const;

        /// process an 'F' (function in 2 ⎕TF format) item.
        void function_2TF(const vector<UCS_string> & objects) const;

        /// process an 'N' (numeric in 1 ⎕TF format) item.
        void numeric_1TF(const vector<UCS_string> & objects) const;

        /// add \b len UTF8 bytes to \b this transfer_context
        void add(const UTF8 * str, int len);

        /// true if a new record has started
        bool new_record;

        /// true if record is EBCDIC (not yet supported)
        bool is_ebcdic;

        /// the record number
        int recnum;

        /// the record type ('A', 'C', 'N', or 'F')
        int item_type;

        /// the last timestamp (if any)
        APL_time_us timestamp;

        /// true if )IN shall not iverride existing objects
        bool protection;   // protect existing objects

        /// accumulator for data of different records
        UCS_string data;
      };

   /// parse the argument of the ]LOG command and set logging accordingly
   static void log_control(const UCS_string & args);

   /// format for ]BOXING
   static int boxing_format;
};
//-----------------------------------------------------------------------------
