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

#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <iomanip>

#include "Backtrace.hh"
#include "Logging.hh"
#include "main.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"
#include "UdpSocket.hh"

extern ostream CERR;
extern ostream & get_CERR();

#ifndef USE_APserver

Svar_DB shared_memory_Svar_DB;

void Svar_DB::init(const char * progname, bool logit, bool do_svars)
   { shared_memory_Svar_DB.open_shared_memory(progname, logit, do_svars); }

const char * SHM_NAME = "/apl-svars";

#ifndef HAVE_shm_open
# define shm_open(name, oflag, mode) (-1)
# define shm_unlink(name) (-1)
#endif

#endif // (dont) USE_APserver

uint16_t Svar_DB_memory_P::APserver_port = Default_APserver_tcp_port;

TCP_socket Svar_DB_memory_P::DB_tcp = NO_TCP_SOCKET;

Svar_DB_memory Svar_DB_memory_P::cache;
offered_SVAR   offered_SVAR_P::cache;

//=============================================================================

Svar_DB_memory * Svar_DB_memory_P::memory_p = 0;

Svar_DB_memory_P::Svar_DB_memory_P(bool ronly)
   : read_only(ronly)
{
   if (!APserver_available())   return;

const char command = ronly ? 'a' : 'A';
ssize_t len = ::send(DB_tcp, &command, 1, 0);
   if (len != 1)   DB_tcp_error("send() command", len, 1);

   len = ::recv(DB_tcp, &cache, sizeof(cache), MSG_WAITALL);
   if (len != sizeof(cache))   DB_tcp_error("recv() db", len, sizeof(cache));
}
//-----------------------------------------------------------------------------
Svar_DB_memory_P::~Svar_DB_memory_P()
{
   if (read_only)               return;
   if (!APserver_available())   return;

const ssize_t len = ::send(DB_tcp, &cache, sizeof(cache), 0);
   if (len != sizeof(cache))   DB_tcp_error("send() db", len, sizeof(cache));
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory_P::connect_to_APserver(const char * bin_path, bool logit)
{
   DB_tcp = (TCP_socket)(socket(AF_INET, SOCK_STREAM, 0));
   if (DB_tcp == NO_TCP_SOCKET)
      {
        get_CERR() << "*** socket(AF_INET, SOCK_STREAM, 0) failed at "
                   << LOC << endl;
        return;
      }

   // We try to connect to the TCP port number APnnn_port (of the APserver)
   // on localhost. If that fails then no APserver is running; we fork one
   // and try again.
   //
   for (bool retry = false; ; retry = true)
       {
         sockaddr_in addr;
         memset(&addr, 0, sizeof(sockaddr_in));
         addr.sin_family = AF_INET;
         addr.sin_port = htons(APserver_port);
         addr.sin_addr.s_addr = htonl(UdpSocket::IP_LOCALHOST);

         if (::connect(DB_tcp, (sockaddr *)&addr,
                       sizeof(addr)) == 0)   break;   // success

         if (retry)
            {
              get_CERR() << "::connect() to existing APserver failed: "
                   << strerror(errno) << endl;

              ::close(DB_tcp);
              DB_tcp = NO_TCP_SOCKET;

              return;
           }

         // fork an APserver
         //
         logit && get_CERR() << "forking new APserver listening on TCP port "
                             << APserver_port << endl;

         const pid_t pid = fork();
         if (pid)
            {
              // give child a little time to start up...
              //
              usleep(20000);
            }
         else   // child: run as APserver
            {
              ::close(DB_tcp);
              DB_tcp = NO_TCP_SOCKET;

              char arg0[FILENAME_MAX + 20];
              snprintf(arg0, sizeof(arg0), "%s/APserver", bin_path);
              char arg1[] = { "--port" };
              char arg2[20];
              snprintf(arg2, sizeof(arg2), "%d", APserver_port);
              char * argv[] = { arg0, arg1, arg2, 0 };
              char * envp[] = { 0 };
              execve(arg0, argv, envp);

              // execve() failed, try APs subdir...
              //
              snprintf(arg0, sizeof(arg0), "%s/APs/APserver", bin_path);
              execve(arg0, argv, envp);

              get_CERR() << "execve() failed" << endl;
              exit(99);
            }
       }

   // at this point DB_tcp is != NO_TCP_SOCKET and connected.
   //
   usleep(20000);
   logit && get_CERR() << "connected to APserver, DB_tcp is "
                       << DB_tcp << endl;
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory_P::DB_tcp_error(const char * op, int got, int expected)
{
   CERR << "⋆⋆⋆ " << op << " failed: got " << got << " when expecting "
        << expected << " (" << strerror(errno) << ")" << endl;

   ::close(DB_tcp);
   DB_tcp = NO_TCP_SOCKET;
}
//=============================================================================

offered_SVAR * offered_SVAR_P::offered_svar_p = 0;

offered_SVAR_P::offered_SVAR_P(bool ronly, SV_key key)
   : read_only(ronly)
{
   if (!Svar_DB_memory_P::APserver_available())   return;

   offered_svar_p = 0;

const int sock = Svar_DB_memory_P::get_DB_tcp();
const char command = ronly ? 'r' : 'R';
ssize_t len = ::send(sock, &command, 1, 0);
   if (len != 1)   Svar_DB_memory_P::DB_tcp_error("send() command", len, 1);

   len = ::send(sock, &key, sizeof(key), 0);
   if (len != sizeof(key))
      Svar_DB_memory_P::DB_tcp_error("send() command", len, sizeof(key));

   len = ::recv(sock, &cache, sizeof(cache), MSG_WAITALL);
   if (len != sizeof(cache))
      Svar_DB_memory_P::DB_tcp_error("recv() db", len, sizeof(cache));

   offered_svar_p = &cache;
}
//-----------------------------------------------------------------------------
offered_SVAR_P::~offered_SVAR_P()
{
   if (read_only)                                 return;
   if (!Svar_DB_memory_P::APserver_available())   return;

const ssize_t len = ::send(Svar_DB_memory_P::get_DB_tcp(),
                           &cache, sizeof(cache), 0);

   if (len != sizeof(cache))
      {
        CERR << "send() failed in offered_SVAR_P destructor: sent "
             << len << " expecting " << sizeof(cache) << endl;
        Svar_DB_memory_P::DB_tcp_error("send()", len, sizeof(cache));
      }
}
//=============================================================================

#ifdef USE_APserver

void
Svar_DB::init(const char * progname, bool logit, bool do_svars)
{
   if (!do_svars)   // shared variables disable
      {
        // shared variables disable by the user. We init memory_p to be
        // on the safe side, but complain only if logit is true
        //
        memset(&Svar_DB_memory_P::cache, 0, sizeof(Svar_DB_memory_P::cache));
        Svar_DB_memory_P::memory_p = &Svar_DB_memory_P::cache;

        if (logit)
           get_CERR() << "Not opening shared memory because command "
                         "line option --noSV (or similar) was given." << endl;
        return;
      }

char * path = strdup(progname);
   if (char * slash = strrchr(path, '/'))   *slash = 0;
   Svar_DB_memory_P::memory_p = &Svar_DB_memory_P::cache;
   Svar_DB_memory_P::connect_to_APserver(path, logit);
   if (Svar_DB_memory_P::APserver_available())
      {
        if (logit)   CERR << "using Svar_DB on APserver!" << endl;
      }
   else
      {
        CERR << "*** using local Svar_DB cache" << endl;
        memset(&Svar_DB_memory_P::cache, 0, sizeof(Svar_DB_memory_P::cache));
        Svar_DB_memory_P::memory_p = &Svar_DB_memory_P::cache;
      }
}

#else // dont USE_APserver

void
Svar_DB::open_shared_memory(const char * progname, bool logit, bool do_svars)
{
   if (do_svars)
      {
        if (logit)
           get_CERR() << "Opening shared memory (pid "
                      << getpid() << ", " << progname << ")... " << endl;
      }
   else
      {
        if (logit)
           get_CERR() << "Not opening shared memory because command "
                         "line option --noSV (or similar) was given." << endl;
        return;
      }

   // in order for CGI scripts to work (which often uses a user with
   // low permissions), we allow RW (i.e. mask only X) for everybody.
   //
   umask(S_IXUSR | S_IXGRP | S_IXOTH);

const int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
   if (fd == -1)
      {
         get_CERR() << "shm_open(" << SHM_NAME << ") failed: "
              << strerror(errno) << endl;
        return;   // error
      }

   // check if initialized. If not, doit
   //
struct stat st;
   if (fstat(fd, &st))
      {
        get_CERR() << "fstat() failed: " << strerror(errno) << endl;
        close(fd);
        shm_unlink(SHM_NAME);
        return;   // error
      }

   // if shared memory is already initialized then it should be
   // sizeof(Svar_DB_memory), possibly rounded up to page_size
   //
bool existing = false;
   if (st.st_size)   // already initialized
      {
        const int page_size = ::getpagesize();
        const int min_size = sizeof(Svar_DB_memory);
        const int max_size = ((min_size + page_size - 1)/page_size)*page_size;

        if (min_size <= st.st_size && st.st_size <= max_size)
           {
             existing = true;
             goto mapit;
           }

        get_CERR() << "bad size of shared memory: is " << int(st.st_size)
             << ", expected " << sizeof(Svar_DB_memory) << endl;
        close(fd);
        shm_unlink(SHM_NAME);
        return;   // error
      }

   // initialize shared memory
   //
   if (ftruncate(fd, sizeof(Svar_DB_memory)))
      {
        get_CERR() << "ftruncate() failed: " << strerror(errno) << endl;
        close(fd);
        shm_unlink(SHM_NAME);
      }

mapit:

const size_t length = sizeof(Svar_DB_memory);
void * vp = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (vp == MAP_FAILED)
      {
        get_CERR() << "mmap() failed: " << strerror(errno) << endl;
        close(fd);
        shm_unlink(SHM_NAME);
      }

   Svar_DB_memory_P::memory_p = (Svar_DB_memory *)vp;

   // close() does not unmap() so we can free the file descriptor here
   close(fd);

Svar_DB_memory_P db(false);
   //
   if (existing)   // existing (initialized) shared memory
      {
        db->remove_stale();
      }
}
#endif // (dont) USE_APserver
//-----------------------------------------------------------------------------
Svar_DB::~Svar_DB()
{
   if (!Svar_DB_memory_P::has_memory())   return;

const pid_t our_pid = getpid();

   // remove all variables offered by this process.
   //
Svar_DB_memory_P db(false);
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_SVAR & svar = db->offered_vars[o];

         if (svar.accepting.pid == our_pid)   svar.remove_accepting();
         if (svar.offering.pid == our_pid)    svar.remove_offering();
         if (svar.get_coupling() == NO_COUPLING)   svar.clear();
       }

   Svar_DB_memory_P::disconnect();

}
//-----------------------------------------------------------------------------
void
Svar_DB::print(ostream & out)
{
   if (!Svar_DB_memory_P::has_memory())
      {
        out << "*** Svar_DB is not open!" << endl;
        return;
      }

const Svar_DB_memory_P db(true);

   // print active processors
   //
   out << "┌───────────┬─────┬─────┬──┐" << endl
       << "│ Proc, par │ PID │ Port│Fl│" << endl
       << "╞═══════════╪═════╪═════╪══╡" << endl;
   for (int p = 0; p < MAX_ACTIVE_PROCS; ++p)
       {
         const Svar_partner_events & sp = db->active_processors[p];
           if (sp.partner.id.proc)
              {
                out << "│";   sp.partner.print(CERR) << "│" << endl;
              }
       }
   out << "╘═══════════╧═════╧═════╧══╛" << endl;

   // print shared variables
   out <<
"╔═════╤═╦═══════════╤═════╤═════╤══╦═══════════╤═════╤═════╤══╦════╤══════════╗\n"
"║     │ ║ Offering  │     │     │  ║ Accepting │     │     │  ║OAOA│          ║\n"
"║ Seq │C║ Proc,par  │ PID │ Port│Fl║ Proc,par  │ PID │ Port│Fl║SSUU│ Varname  ║\n"
"╠═════╪═╬═══════════╪═════╪═════╪══╬═══════════╪═════╪═════╪══╬════╪══════════╣\n";
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const offered_SVAR & svar = db->offered_vars[o];
         if (svar.valid())   svar.print(out);
       }

   out <<
"╚═════╧═╩═══════════╧═════╧═════╧══╩═══════════╧═════╧═════╧══╩════╧══════════╝\n"
       << endl;
}
//-----------------------------------------------------------------------------

