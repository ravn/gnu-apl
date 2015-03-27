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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "buildtag.hh"
#include "Command.hh"
#include "Common.hh"
#include "IO_Files.hh"
#include "LibPaths.hh"
#include "LineInput.hh"
#include "Logging.hh"
#include "makefile.h"
#include "Output.hh"
#include "NativeFunction.hh"
#include "Parallel.hh"
#include "Prefix.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"
#include "UserPreferences.hh"
#include "Value.icc"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char * build_tag[] = { BUILDTAG, 0 };

//-----------------------------------------------------------------------------

static struct sigaction old_control_C_action;
static struct sigaction new_control_C_action;

//-----------------------------------------------------------------------------
static struct sigaction old_SEGV_action;
static struct sigaction new_SEGV_action;

static void
signal_SEGV_handler(int)
{
   CERR << "\n\n====================================================\n"
           "SEGMENTATION FAULT" << endl;

   CERR << "thread: " << (const void *)pthread_self() << endl;
   Thread_context::print_all(CERR);

   Backtrace::show(__FILE__, __LINE__);

   CERR << "====================================================\n";

   // count errors
   IO_Files::assert_error();

     Command::cmd_OFF(3);
}
//-----------------------------------------------------------------------------
static void
signal_USR1_handler(int)
{
   CERR << "Got signal USR1" << endl;
}

static struct sigaction old_USR1_action;
static struct sigaction new_USR1_action;

//-----------------------------------------------------------------------------
static struct sigaction old_TERM_action;
static struct sigaction new_TERM_action;

static void
signal_TERM_handler(int)
{
   cleanup(true);
   sigaction(SIGTERM, &old_TERM_action, 0);
   raise(SIGTERM);
}
//-----------------------------------------------------------------------------
static struct sigaction old_control_BSL_action;
static struct sigaction new_control_BSL_action;

static void
control_BSL(int sig)
{
   CERR << endl << "^\\" << endl;
   Thread_context::print_all(CERR);
}
//-----------------------------------------------------------------------------
static struct sigaction old_HUP_action;
static struct sigaction new_HUP_action;

static void
signal_HUP_handler(int)
{
   cleanup(true);
   sigaction(SIGHUP, &old_HUP_action, 0);
   raise(SIGHUP);
}
//-----------------------------------------------------------------------------
static void
show_argv(int argc, const char ** argv)
{
   CERR << "argc: " << argc << endl;
   loop(a, argc)   CERR << "  argv[" << a << "]: '" << argv[a] << "'" << endl;
}
//-----------------------------------------------------------------------------
/// print a welcome message (copyright notice)
static void
show_welcome(ostream & out, const char * argv0)
{
char c1[200];
char c2[200];
   snprintf(c1, sizeof(c1), "Welcome to GNU APL version %s",build_tag[1]);
   snprintf(c2, sizeof(c2), "for details run: %s --gpl.", argv0);

const char * lines[] =
{
  ""                                                                      ,
  "   ______ _   __ __  __    ___     ____   __ "                         ,
  "  / ____// | / // / / /   /   |   / __ \\ / / "                        ,
  " / / __ /  |/ // / / /   / /| |  / /_/ // /  "                         ,
  "/ /_/ // /|  // /_/ /   / ___ | / ____// /___"                         ,
  "\\____//_/ |_/ \\____/   /_/  |_|/_/    /_____/"                       ,
  ""                                                                      ,
  c1                                                                      ,
  ""                                                                      ,
  "Copyright (C) 2008-2015  Dr. Jürgen Sauermann"                         ,
  "Banner by FIGlet: www.figlet.org"                                      ,
  ""                                                                      ,
  "This program comes with ABSOLUTELY NO WARRANTY;"                       ,
  c2                                                                      ,
  ""                                                                      ,
  "This program is free software, and you are welcome to redistribute it" ,
  "according to the GNU Public License (GPL) version 3 or later."         ,
  ""                                                                      ,
  0
};

   // compute max. length
   //
int len = 0;
   for (const char ** l = lines; *l; ++l)
       {
         const char * cl = *l;
         const int clen = strlen(cl);
         if (len < clen)   len = clen;
       }
 
const int left_pad = (80 - len)/2;

   for (const char ** l = lines; *l; ++l)
       {
         const char * cl = *l;
         const int clen = strlen(cl);
         const int pad = left_pad + (len - clen)/2;
         loop(p, pad)   out << " ";
         out << cl << endl;
       }
}
//-----------------------------------------------------------------------------
int
init_apl(int argc, const char * argv[])
{
   {
     // make curses happy
     //
     const char * term = getenv("TERM");
     if (term == 0 || *term == 0)   setenv("TERM", "dumb", 1);
   }

   uprefs.expand_argv(argc, argv);
const bool log_startup = uprefs.parse_argv_1();

#ifdef DYNAMIC_LOG_WANTED
   if (log_startup)   Log_control(LID_startup, true);
#endif // DYNAMIC_LOG_WANTED

   init_1(argv[0], log_startup);

   uprefs.read_config_file(true,  log_startup);   // /etc/gnu-apl.d/preferences
   uprefs.read_config_file(false, log_startup);   // $HOME/.gnu_apl/preferences
   uprefs.read_threshold_file(true,  log_startup);  // dito parallel_thresholds
   uprefs.read_threshold_file(false, log_startup);  // dito parallel_thresholds

   // struct sigaction differs between GNU/Linux and other systems, which
   // causes direct bracket assignment to not compile on some machines.
   //
   // We therefore memset everything to 0 and then set the handler (which
   // should be compatible on GNU/Linux and other systems.
   //
   memset(&new_control_C_action,   0, sizeof(struct sigaction));
   memset(&new_USR1_action,        0, sizeof(struct sigaction));
   memset(&new_SEGV_action,        0, sizeof(struct sigaction));
   memset(&new_TERM_action,        0, sizeof(struct sigaction));
   memset(&new_control_BSL_action, 0, sizeof(struct sigaction));
   memset(&new_HUP_action,         0, sizeof(struct sigaction));

   new_control_C_action.sa_handler = &control_C;
   new_control_BSL_action.sa_handler = &control_BSL;
   new_USR1_action .sa_handler = &signal_USR1_handler;
   new_SEGV_action .sa_handler = &signal_SEGV_handler;
   new_TERM_action .sa_handler = &signal_TERM_handler;
   new_HUP_action  .sa_handler = &signal_HUP_handler;

   sigaction(SIGINT,  &new_control_C_action,   &old_control_C_action);
   sigaction(SIGQUIT, &new_control_BSL_action, &old_control_BSL_action);
   sigaction(SIGUSR1, &new_USR1_action,        &old_USR1_action);
   sigaction(SIGSEGV, &new_SEGV_action,        &old_SEGV_action);
   sigaction(SIGTERM, &new_TERM_action,        &old_TERM_action);
   sigaction(SIGHUP,  &new_HUP_action,       &old_HUP_action);

   uprefs.parse_argv_2(log_startup);

   if (uprefs.emacs_mode)
      {
        UCS_string info;
        if (uprefs.emacs_arg)
           {
             info = NativeFunction::load_emacs_library(uprefs.emacs_arg);
           }

        if (info.size())   // problems loading library
           {
             CERR << info << endl;
           }
        else
           {
             // CIN = U+F00C0 = UTF8 F3 B0 83 80 ...
             Output::color_CIN[0] = 0xF3;
             Output::color_CIN[1] = 0xB0;
             Output::color_CIN[2] = 0x83;
             Output::color_CIN[3] = 0x80;
             Output::color_CIN[4] = 0;

             // COUT = U+F00C1 = UTF8 F3 B0 83 81 ...
             Output::color_COUT[0] = 0xF3;
             Output::color_COUT[1] = 0xB0;
             Output::color_COUT[2] = 0x83;
             Output::color_COUT[3] = 0x81;
             Output::color_COUT[4] = 0;

             // CERR = U+F00C2 = UTF8 F3 B0 83 82 ...
             Output::color_CERR[0] = 0xF3;
             Output::color_CERR[1] = 0xB0;
             Output::color_CERR[2] = 0x83;
             Output::color_CERR[3] = 0x82;
             Output::color_CERR[4] = 0;

             // UERR = U+F00C3 = UTF8 F3 B0 83 83 ...
             Output::color_UERR[0] = 0xF3;
             Output::color_UERR[1] = 0xB0;
             Output::color_UERR[2] = 0x83;
             Output::color_UERR[3] = 0x83;
             Output::color_UERR[4] = 0;

             // no clear_EOL
             Output::clear_EOL[0] = 0;

             Output::use_curses = false;
           }
      }

   if (uprefs.daemon)
      {
        const pid_t pid = fork();
        if (pid)   // parent
           {
             Log(LOG_startup)
                CERR << "parent pid = " << getpid()
                     << " child pid = " << pid << endl;

             exit(0);   // parent returns
           }

        Log(LOG_startup)
           CERR << "child forked (pid" << getpid() << ")" << endl;
      }

   if (uprefs.wait_ms)   usleep(1000*uprefs.wait_ms);

   init_2(log_startup);

   if (!uprefs.silent)   show_welcome(cout, argv[0]);

   if (log_startup)   CERR << "PID is " << getpid() << endl;
   Log(LOG_argc_argv || log_startup)   show_argv(argc, argv);

   if (ProcessorID::init(log_startup))
      {
        // error message printed in ProcessorID::init()
        return 8;
      }

   if (uprefs.do_Color)   Output::toggle_color("ON");

   if (uprefs.latent_expression.size())
      {
        // there was a --LX expression on the command line
        //
        UCS_string lx(uprefs.latent_expression);

        if (log_startup)
           CERR << "executing --LX '" << lx << "'" << endl;

        Command::process_line(lx);
      }

   if (uprefs.do_CONT)
      {
         UCS_string cont("CONTINUE");
         UTF8_string filename =
            LibPaths::get_lib_filename(LIB0, cont, true, ".xml", ".apl");

         if (access(filename.c_str(), F_OK) == 0)
            {
              // CONTINUE workspace exists and was not inhibited by --noCONT
              //
              UCS_string load_cmd(")LOAD CONTINUE");
              Command::process_line(load_cmd);
              return 0;
            }
      }

   if (uprefs.initial_workspace.size())
      {
         // the user has provided a workspace name via -L
         //
         UCS_string init_ws(uprefs.initial_workspace);
          const char * cmd = uprefs.silent ? ")QLOAD " : ")LOAD ";
         UCS_string load_cmd(cmd);
         load_cmd.append(init_ws);
         Command::process_line(load_cmd);
      }

   return 0;
}
//-----------------------------------------------------------------------------
int
main(int argc, const char *argv[])
{
const int ret = init_apl(argc, argv);
   if (ret)   return ret;

   for (;;)
       {
         Token t = Workspace::immediate_execution(
                       IO_Files::test_mode == IO_Files::TM_EXIT_AFTER_ERROR);
         if (t.get_tag() == TOK_OFF)   Command::cmd_OFF(0);
       }

   return 0;
}
//-----------------------------------------------------------------------------
