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

#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "Common.hh"
#include "LibPaths.hh"
#include "Output.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "Svar_signals.hh"
#include "UserPreferences.hh"

extern const char * prog_name();

AP_num3 ProcessorID::id(NO_AP, AP_NULL, AP_NULL);

Network_Profile ProcessorID::network_profile;

//-----------------------------------------------------------------------------
bool
ProcessorID::init(bool log_startup)
{
   if (log_startup)
      {
        CERR << "uprefs.user_do_svars:   " << uprefs.user_do_svars   << endl
             << "uprefs.system_do_svars: " << uprefs.system_do_svars << endl
             << "uprefs.requested_id:    " << uprefs.requested_id    << endl
             << "uprefs.requested_par:   " << uprefs.requested_par   << endl;
      }

   id.proc = (AP_num)uprefs.requested_id;
   id.parent = uprefs.requested_par ? (AP_num)uprefs.requested_par : AP_NULL;
   id.grand = AP_NULL;

   if (!uprefs.system_do_svars)
      {
        // shared variables are disabled, so Svar_DB is unavailable,
        // we use id.proc of 1000 if no ID is provided and otherwise
        // trust the provided ID.
        //
        if (id.proc == 0)   id.proc = (AP_num)1000;
        if (log_startup)   CERR << "id.proc: " << id.proc << " at " LOC << endl;
        return false;
      }

   if (id.proc == 0)
      {
        // no --id option in argv: use first free user ID. We poll IDs starting
        // at 1000 and stop at the first ID that is not registered.
        //
        for (AP_num proc = AP_FIRST_USER; ; proc = AP_num(proc + 1))
            {
              id.proc = proc;
              if (!Svar_DB::is_registered_id(AP_num(id.proc)))   break;
            }

        if (log_startup)   CERR << "id.proc: " << id.proc << " at " LOC << endl;
      }
   else
      {
        // --id option provided in argv: check that it is not in use
        //
        if (Svar_DB::is_registered_id(AP_num(id.proc)))
           {
             CERR << "*** Another APL interpreter with --id "
                  << id.proc <<  " is already running" << endl;
 
             return true;
           }
      }

   if (log_startup)
      {
        CERR << "Processor ID was completely initialized: "
             << id.proc << ":" << id.parent << ":" << id.grand << endl
             << "system_do_svars is: " << uprefs.system_do_svars << endl;

      }

   // if we have an APserver then let it know our IDs
   //
const TCP_socket sock = Svar_DB::get_DB_tcp();
   if (sock != NO_TCP_SOCKET)
      {
        string progname(prog_name());
        REGISTER_PROCESSOR_c request(sock, id.proc, id.parent, id.grand,
                             0, progname);
      }

   return false;   // no error
}
//-----------------------------------------------------------------------------
const char *
ProcessorID::read_svopid(FILE * file, SvoPid & svopid, int & line)
{
const char * loc = 0;

   for (line = 1;; ++line)
       {
         char buffer[200];
         const long filepos = ftell(file);   // remember file position
         char * s = fgets(buffer, sizeof(buffer) - 1, file);

         if (s == 0)   break;   // end of file


         // skip leading whitespaces
         //
         while (*s && *s < ' ')   ++s;

         if (*s == '*')   continue;   // comment line
         if (*s == 0)     continue;   // empty line

         if (!strcmp(s, ":processor,"))
            {
              int i = NO_AP;
              int j = AP_NULL;
              int k = AP_NULL;

              const int count = sscanf(s, ":processor,%u,%u,%u", &i, &j, &k);
              if (count < 1)   { loc = LOC;   break; }
              if (count > 3)   { loc = LOC;   break; }

              svopid.id = AP_num3(AP_num(i), AP_num(j), AP_num(k));
              continue;
            }

         if (!strcmp(s, ":address,"))
            {
              // skip tag and remove trailing whitespaces
              //
              s += 9;
              char * e = s + strlen(s);
              while (e > s && *e > 0 && *e < ' ')   *--e = 0;

              hostent * host = gethostbyname(s);
              if (host == 0)
                 {
                   CERR << "gethostbyname(" << s << ") failed" << endl;
                   loc = LOC;
                   break;
                 }

              unsigned int hh = host->h_addr_list[0][0];
              unsigned int hl = host->h_addr_list[0][1];
              unsigned int lh = host->h_addr_list[0][2];
              unsigned int ll = host->h_addr_list[0][3];

              svopid.ip_addr = hh << 24 | hl << 16 | lh << 8 | ll;
              continue;
            }

         if (!strcmp(s, ":userid,"))
            {
              s += 8;
              for (int u = 1; u < sizeof(svopid.user); ++u)
                  {
                    if (*s < ' ')   break;
                    svopid.user[u - 1] = *s++;
                    svopid.user[u] = 0;
                  }

              continue;
            }

         if (!strcmp(s, ":crypt,"))
            {
              // ignored
              continue;
            }

         // invalid tag (probably next entry). restore file position
         //
         fseek(file, filepos, SEEK_SET);
         break;
       }

   return loc;
}
//-----------------------------------------------------------------------------
const char *
ProcessorID::read_procauth(FILE * file, ProcAuth & procauth, int & line)
{
const char * loc = 0;

   for (line = 1;; ++line)
       {
         char buffer[200];
         const long filepos = ftell(file);   // remember file position
         char * s = fgets(buffer, sizeof(buffer) - 1, file);

         if (s == 0)   break;   // end of file


         // skip leading whitespaces
         //
         while (*s && *s < ' ')   ++s;

         if (*s == '*')   continue;   // comment line
         if (*s == 0)     continue;   // empty line

         if (!strcmp(s, ":rsvopid,"))
            {
              // skip tag and remove trailing whitespaces
              //
              s += 8;   // leave comma
              char * e = s + strlen(s);
              while (e > s && *e > 0 && *e < ' ')   *--e = 0;

              for (unsigned int id; s;)
                  {
                    ++s;   // skip comma
                    if (1 != sscanf(s, "%u", &id))   break;
                    
                    procauth.rsvopid.push_back(id);
                    s = strchr(s, ',');
                  }
              continue;
            }

         // invalid tag (probably next entry). restore file position
         //
         fseek(file, filepos, SEEK_SET);
         break;
       }

   return loc;
}
//-----------------------------------------------------------------------------
int
ProcessorID::read_network_profile()
{
const char * filename = getenv("APL2SVPPRF");

   if (filename)
      {
        return read_network_profile(filename);
      }
   else
      {
        string fname = LibPaths::get_APL_bin_path();
        fname += "/apl2svp.prf";
        return read_network_profile(fname.c_str());
      }
}
//-----------------------------------------------------------------------------
int
ProcessorID::read_network_profile(const char * filename)
{
FILE * file = fopen(filename, "r");
int line = 0;
const char * loc = 0;

   if (file == 0)
      {
        CERR << "Cannot open network profile '" << filename << "'" << endl;
        return 1;
      }

   network_profile.clear();
   for (line = 1;; ++line)
       {
         char buffer[200];
         const char * s = fgets(buffer, sizeof(buffer) - 1, file);

         if (s == 0)   // end of file
            {
              line = 0;
              break;
            }

         // skip leading whitespaces
         //
         while (*s && *s < ' ')   ++s;

        if (*s == '*')   continue;   // comment line
        if (*s == 0)     continue;   // empty line

        if (!strcmp(s, ":svopid,"))
           {
             SvoPid svopid;
              if (1 != sscanf(s, ":svopid,%u", &svopid.svopid))
                 { loc = LOC;   break; }

             loc = read_svopid(file, svopid, line);
             if (loc)   break;

             network_profile.svo_pids.push_back(svopid);
             continue;
           }

        if (!strcmp(s, ":procauth,"))
           {
             ProcAuth procauth;
             int i = NO_AP;
             int j = AP_NULL;
             int k = AP_NULL;
             const int count = sscanf(s, ":procauth,%u,%u,%u", &i, &j, &k);
             if (count < 1)   { loc = LOC;   break; }
             if (count > 3)   { loc = LOC;   break; }

             procauth.id = AP_num3(AP_num(i), AP_num(j), AP_num(k));

             loc = read_procauth(file, procauth, line);
             if (loc)   break;

             network_profile.proc_auths.push_back(procauth);
             continue;
           }

        loc = LOC;
        break;
       }

   fclose(file);

   if (loc)
      {
        CERR << "Syntax error in network profile '" << filename
             << "' line " << line << ", detected at " << loc << endl;
      }

   return line;
}
//-----------------------------------------------------------------------------
void
ProcessorID::disconnect()
{
   Svar_DB::DB_tcp_error(0, 0, 0);
}
//-----------------------------------------------------------------------------
