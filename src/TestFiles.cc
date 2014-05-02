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

#include <stdlib.h>

#include "Common.hh"
#include "Command.hh"
#include "Input.hh"
#include "IndexExpr.hh"
#include "main.hh"
#include "Output.hh"
#include "TestFiles.hh"
#include "UserPreferences.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

int TestFiles::testcase_count = 0;
int TestFiles::testcases_done = 0;
int TestFiles::total_errors = 0;
int TestFiles::apl_errors = 0;
const char * TestFiles::last_apl_error_loc = "";
int TestFiles::last_apl_error_line = -1;
int TestFiles::assert_errors = 0;
int TestFiles::diff_errors = 0;
int TestFiles::parse_errors = 0;
TestFiles::TestMode TestFiles::test_mode = TM_EXIT_AFTER_LAST;
bool TestFiles::need_total = false;
ofstream TestFiles::current_testreport;

//-----------------------------------------------------------------------------
const UTF8 *
TestFiles::get_testcase_line()
{
   while (uprefs.current_file())   // as long as we have input files
      {
        if (uprefs.current_file()->file == 0)
           {
             open_next_testfile();
             if (uprefs.current_file()->file == 0)   break;   // no more files
           }

        // read a line with CR and LF removed.
        //
        const UTF8 * s = Input::read_file_line();

        if (s == 0)   // end of file reached: do some global checks
           {
             if (end_of_file_processing())   continue;   // try again.
              else                           break;      // done
           }

        current_testreport << "----> " << s << endl;

        return s;
      }

   // arrive here when all testfiles have been read.
   // Maybe print a testcase total summary
   //
   if (need_total)   // we had a -T option
      {
        print_summary();
        need_total = false;   // forget the -T option

        if ((test_mode == TM_EXIT_AFTER_LAST) ||
            (test_mode == TM_EXIT_AFTER_LAST_IF_OK && !error_count()))
          {
            CERR << "Exiting (test_mode " << test_mode << ")" << endl;
            cleanup();
            if (total_errors)   Command::cmd_OFF(1);
            else                Command::cmd_OFF(0);
          }
      }

   return 0;
}
//-----------------------------------------------------------------------------
bool
TestFiles::end_of_file_processing()
{
   // we expect )SI to be clear after a testcase has finished.
   // Complain if it is not.
   //
   if (Workspace::SI_entry_count() > 1)
      {
        CERR << endl << ")SI not cleared at the end of "
             << uprefs.current_filename() << ":" << endl;
        Workspace::list_SI(CERR, SIM_SIS);
        CERR << endl;

        if (current_testreport.is_open())
           {
             current_testreport << endl
                                << ")SI not cleared at the end of "
                                << uprefs.current_filename()<< ":"
                                << endl;
             Workspace::list_SI(current_testreport, SIM_SIS);
             current_testreport << endl;
           }
        apl_error(LOC);
      }

   // check for stale values and indices
   //
   if (current_testreport.is_open())
      {
        if (Value::print_incomplete(current_testreport))
           {
             current_testreport
                << " (automatic check for incomplete values failed)"
                << endl;
             apl_error(LOC);
           }

        if (Value::print_stale(current_testreport))
           {
             current_testreport
                << " (automatic check for stale values failed,"
                   " offending Value erased)." << endl;

             apl_error(LOC);
             Value::erase_stale(LOC);
           }

        if (IndexExpr::print_stale(current_testreport))
           {
             current_testreport
                << " (automatic check for stale indices failed,"
                   " offending IndexExpr erased)." << endl;
             apl_error(LOC);
             IndexExpr::erase_stale(LOC);
           }
      }
             
   uprefs.close_current_file();
   ++testcases_done;

   Log(LOG_test_execution)
      CERR << "closed testcase file " << uprefs.current_filename()
           << endl;

   ofstream summary("testcases/summary.log", ios_base::app);
   summary << error_count() << " ";

   if (error_count())
      {
        total_errors += error_count();
        summary << "(" << apl_errors    << " APL, ";
        if (apl_errors)
           summary << "    loc=" << last_apl_error_loc 
                   << " .tc line="  << last_apl_error_line << endl
                   << "    ";

        summary << assert_errors << " assert, "
                << diff_errors   << " diff, "
                << parse_errors  << " parse) ";
      }

   summary << uprefs.current_filename() << endl;

   if ((test_mode == TM_STOP_AFTER_ERROR ||
        test_mode == TM_EXIT_AFTER_ERROR) && error_count())
      {
        CERR << endl
             << "Stopping test execution since an error has occurred"  << endl
             << "The error count is " << error_count() << endl
             << "Failed testcase is " << uprefs.current_filename()
             << endl 
             << endl;
                  
        uprefs.files_todo.resize(0);
        return false;
      }

   uprefs.files_todo.erase(uprefs.files_todo.begin());

   Output::reset_dout();
   reset_errors();
   return true;   // continue processing
}
//-----------------------------------------------------------------------------
void
TestFiles::print_summary()
{
ofstream summary("testcases/summary.log", ios_base::app);

   summary << "======================================="
              "=======================================" << endl
           << total_errors << " errors in " << testcases_done
           << "(" << testcase_count << ")"
           << " testcase files" << endl;

   CERR    << endl
           << "======================================="
           "=======================================" << endl
           << total_errors << " errors in " << testcases_done
           << "(" << testcase_count << ")"
           << " testcase files" << endl;
}
//-----------------------------------------------------------------------------
void
TestFiles::open_next_testfile()
{
   if (uprefs.current_file() == 0)
      {
        CERR << "Workspace::open_next_testfile(): no more files" << endl;
        return;
      }

   if (uprefs.current_file()->file)
      {
        CERR << "Workspace::open_next_testfile(): already open" << endl;
        return;
      }

     for (;;)
         {
           if (uprefs.current_file()->test)
              {
                CERR << " #######################################"
                        "#######################################\n"
                     << "########################################"
                        "########################################\n"
                     << " #######################################"
                        "#######################################\n"
                     << " Testfile: " << uprefs.current_filename() << endl
                     << "########################################"
                        "########################################" << endl;
              }

           uprefs.open_current_file();
           if (uprefs.current_file()->file == 0)
              {
                CERR << "could not open " << uprefs.current_filename() << endl;
                uprefs.files_todo.erase(uprefs.files_todo.begin());
                continue;
              }

           Log(LOG_test_execution)   CERR <<
               "openened testcase file " << uprefs.current_filename() << endl;

           Output::reset_dout();
           reset_errors();

           char log_name[FILENAME_MAX];
           snprintf(log_name, sizeof(log_name) - 1,  "%s.log",
                    uprefs.current_filename());
        
           current_testreport.close();
           current_testreport.open(log_name, ofstream::out | ofstream::trunc);
           Assert(current_testreport.is_open());

           return;
         }
}
//-----------------------------------------------------------------------------
void
TestFiles::expect_apl_errors(const UCS_string & arg)
{
const int cnt = arg.atoi();
   if (apl_errors == cnt)
      {
        Log(LOG_test_execution)
           CERR << "APL errors reset from (expected) " << apl_errors
                << " to 0" << endl;
        apl_errors = 0;
        last_apl_error_line = -1;
        last_apl_error_loc = "";
      }
   else
      {
        Log(LOG_test_execution)
           CERR << "*** Not reseting APL errors (got " << apl_errors
                << " expecing " << cnt << endl;
      }
}
//-----------------------------------------------------------------------------
void
TestFiles::syntax_error()
{
   if (!uprefs.current_file()->file)   return;

   ++parse_errors;

   Log(LOG_test_execution)
      CERR << "parse errors incremented to " << parse_errors << endl;

   current_testreport << "**\n** Parse Error ********\n**" << endl;
}
//-----------------------------------------------------------------------------
void
TestFiles::apl_error(const char * loc)
{
   if (!uprefs.current_file()->file)   return;

   ++apl_errors;
   last_apl_error_loc = loc;
   last_apl_error_line = uprefs.current_line_no();

   Log(LOG_test_execution)
      CERR << "APL errors incremented to " << apl_errors << endl;
}
//-----------------------------------------------------------------------------
void
TestFiles::assert_error()
{
   if (!uprefs.current_file()->file)   return;

   ++assert_errors;
   Log(LOG_test_execution)
      CERR << "Assert errors incremented to " << assert_errors << endl;
}
//-----------------------------------------------------------------------------
void
TestFiles::diff_error()
{
   if (!uprefs.current_file()->file)   return;

   ++diff_errors;
   Log(LOG_test_execution)
      CERR << "Diff errors incremented to " << diff_errors << endl;
}
//-----------------------------------------------------------------------------
