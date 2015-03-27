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

#ifndef __PROCESSOR_ID_HH_DEFINED__
#define __PROCESSOR_ID_HH_DEFINED__

using namespace std;

#ifdef AP_NUM

/// a simple ProcessorID to be used by APs
class ProcessorID
{
public:
   /// return the current ID (proc, parent, and grandparent)
   static const AP_num3 & get_id()        { return id; }

   static void set_id(AP_num3 ap3)       { id = ap3; }

   static void clear_id()
      { id.proc = id.parent = id.grand = AP_NULL; }

   /// set the current ID
   static void set_own_ID(AP_num ap)      { id.proc = ap; }

   /// return the parent's ID
   static AP_num get_parent_ID()          { return id.parent; }

   /// set the parent's ID
   static void set_parent_ID(AP_num ap)   { id.parent = ap; }

   /// return the grandparent's ID
   static AP_num get_grand_ID()           { return id.grand; }

   /// set the grandparent's ID
   static void set_grand_ID(AP_num ap)    { id.grand = ap; }

protected:
   /// the current ID (proc, parent, and grandparent)
   static    AP_num3 id;
};

#else

// the normal ProcessorID to be used by the APL interpreter

/// a mapping between the left argument of ⎕SVO or ⎕SVQ and AP numbers
struct SvoPid
{
   SvoPid() : id(NO_AP, AP_NULL, AP_NULL) {}

   int svopid;           ///< left argument of ⎕SVO and ⎕SVQ
   int ip_addr;          ///< remote IP address
   char user[32];        ///< user account (login name)
   AP_num3 id;           ///< processor, parent and grandparent
};

/// A processor authentication
struct ProcAuth
{
   /// constructor: no ID
   ProcAuth() : id(NO_AP, AP_NULL, AP_NULL) {}

   /// the ID
   AP_num3 id;

   /// the allowed remote processors
   vector<int> rsvopid;        ///< left argument(s) of remote ⎕SVO and ⎕SVQ
};

/// A network profile
struct Network_Profile
{
   vector<SvoPid>     svo_pids;     ///< ///< left argument of ⎕SVO and ⎕SVQ
   vector<ProcAuth>   proc_auths;   ///< processor authentications

   /// clear everything
   void clear()
      {
        svo_pids.clear();
        proc_auths.clear();
      }
};

/// One processor. APL interpreters have numbers > 1000 while auxiliary
/// processors have numbers < 1000
class ProcessorID
{
public:
   /// initialize our own process ID, return non-0 on error.
   /// proc_id == 0 uses the next free ID > 1000; otherwise proc_id is used.
   /// \b do_sv defines if an APnnn process for incoming ⎕SVO offers
   /// shall be forked.
   static bool init(bool log_startup);

   /// return the own id, parent, and grand-parent
   static const AP_num3 & get_id()        { return id; }

   /// return the processor ID of this apl interpreter
   static AP_num get_own_ID()   { return id.proc; }

   /// return the processor ID of the parent of this apl interpreter
   static AP_num get_parent_ID()   { return id.parent; }

   /// read the network profile file from its default location
   static int read_network_profile();

   /// disconnect from APnnn process
   static void disconnect();

protected:
   /// read the network profile file from file \b file
   static int read_network_profile(const char * filename);

   /// read one SvoPid entry from \b file
   static const char * read_svopid(FILE * file, SvoPid & svopid, int & line);

   /// read one ProcAuth entry from \b file
   static const char * read_procauth(FILE * file, ProcAuth & procauth,
                                     int & line);

   /// the processor, parent, and grandparent of this apl interpreter
   static AP_num3 id;

   /// the network profile currently used
   static Network_Profile network_profile;
};

#endif

#endif // __PROCESSOR_ID_HH_DEFINED__
