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

#ifndef __INPUT_FILE_HH_DEFINED__
#define __INPUT_FILE_HH_DEFINED__

#include <vector>

#include "UTF8_string.hh"
#include "UCS_string_vector.hh"

/** an input file and its properties. The file can be an apl script(.apl) file
    or a testcase (.tc) file. The file names initially come from the command
    line, but can be extended by )COPY commands 
 */
/// A single input file to be executed by the interpreter
struct InputFile
{
   friend class IO_Files;

   /// empty constructor
   InputFile() {}

   /// Normal constructor
   InputFile(const UTF8_string & _filename, FILE * _file,
                     bool _test, bool _echo, bool _is_script, LX_mode LX)
   : file     (_file),
     filename (&_filename[0], _filename.size()),
     test     (_test),
     echo     (_echo),
     is_script(_is_script),
     with_LX  (LX),
     line_no  (0),
     in_html  (0),
     in_function(false),
     in_variable(false),
     in_matched(false),
     from_COPY(false)
   {}

   /// set the from_COPY flag
   void set_COPY()
      { from_COPY = true; }

   /// set the current line number
   void set_line_no(int num)
      { line_no = num; }

   /// set the current HTML mode
   void set_html(int html)
      { in_html = html; }

   /// return the current HTML mode
   int get_html() const
      { return in_html; }

   /// the current file
   static InputFile * current_file()
      { return files_todo.size() ? &files_todo[0] : 0; }

   /// the name of the currrent file
   static const char * current_filename()
      { return files_todo.size() ? files_todo[0].filename.c_str() : "stdin"; }

   /// the line number of the currrent file
   static int current_line_no()
      { return files_todo.size() ? files_todo[0].line_no : stdin_line_no; }

   /// increment the line number of the current file
   static void increment_current_line_no()
      { if (files_todo.size()) ++files_todo[0].line_no; else ++stdin_line_no; }

   /// return true iff input comes from a script (as opposed to running
   /// interactively)
   static bool running_script()
      { return files_todo.size() > 0 && files_todo[0].is_script; }

   /// return true iff the current input file exists and is a test file
   static bool is_validating()
      { return files_todo.size() > 0 && files_todo[0].test; }

   /// the number of testcase (.tc) files
   static int testcase_file_count()
      {
        int count = 0;
        loop(f, files_todo.size())
            {
              if (files_todo[f].test)   ++count;
            }
        return count;
      }

   /// add object to object_filter
   void add_filter_object(UCS_string & object)
      {
        object_filter.push_back(object);
      }

   /// return true if this file as an oject filter (from )COPY file names...)
   bool has_object_filter() const
      { return object_filter.size() > 0; }

   /// check the current line and return true if the line is permitted by the
   /// object_filter. This function also updates \b in_function and \b
   /// in_variable
   bool check_filter(const UTF8_string & line);

   /// true if echo (of the input) is on for the current file
   static bool echo_current_file();

   /// open current file unless already open
   static void open_current_file();

   /// close the current file and perform some cleanup
   static void close_current_file();

   /// randomize the order of test_file_names
   static void randomize_files();

   /// files that need to be processed
   static std::vector<InputFile> files_todo;

   /// the initial set of files provided on the command line
   static std::vector<InputFile> files_orig;

   /// FILE * from fopen (or 0 if file is closed)
   FILE       * file;

   /// the file name
   UTF8_string  filename;

protected:

   bool test;         ///< true for -T testfile, false for -f APL file
   bool echo;         ///< echo stdin
   bool is_script;    ///< script (override existing functions)
   LX_mode with_LX;   ///< execute ⎕LX at the end
   int  line_no;      ///< line number in file
   int  in_html;      ///< 0: no HTML, 1: in HTML file 2: in HTML header

   /// functions and vars that shoule be )COPIED
   UCS_string_vector object_filter;

   /// true if current line belongs to a function. Cleared at final ∇
   bool in_function;

   /// return true if current line belongs to a variable. Cleared by empty line
   bool in_variable;

   /// true if current function or variable was mentioned in object_filter
   bool in_matched;

   /// true if this file comes from a )COPY XXX.apl (but not XXX.xml
   bool from_COPY;

   /// line number in stdin
   static int stdin_line_no;
};

#endif // __INPUT_FILE_HH_DEFINED__
