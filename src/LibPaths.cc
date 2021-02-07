/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2013-2015  Dr. Jürgen Sauermann

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "Error.hh"
#include "LibPaths.hh"
#include "PrintOperator.hh"

bool LibPaths::root_from_env = false;
bool LibPaths::root_from_pwd = false;

char LibPaths::APL_bin_path[APL_PATH_MAX + 1] = "";
const char * LibPaths::APL_bin_name = LibPaths::APL_bin_path;
char LibPaths::APL_lib_root[APL_PATH_MAX + 10] = "";

LibPaths::LibDir LibPaths::lib_dirs[LIB_MAX];

const void * unused = 0;

//-----------------------------------------------------------------------------
void
LibPaths::init(const char * argv0, bool logit)
{
   logit && CERR << "\ninitializing paths from argv[0] = " << argv0 << endl;

   compute_bin_path(argv0, logit);
   search_APL_lib_root();

   loop(d, LIB_MAX)
      {
        if      (root_from_env)   lib_dirs[d].cfg_src = LibDir::CSRC_ENV;
        else if (root_from_pwd)   lib_dirs[d].cfg_src = LibDir::CSRC_PWD;
        else                      lib_dirs[d].cfg_src = LibDir::CSRC_NONE;
      }
}
//-----------------------------------------------------------------------------
void
LibPaths::compute_bin_path(const char * argv0, bool logit)
{
   // compute APL_bin_path from argv0
   //
   if (strchr(argv0, '/') == 0)
      {
         // if argv0 contains no / then realpath() seems to prepend the current
         // directory to argv0 (which is wrong since argv0 may be in $PATH).
         //
         // we fix this by searching argv0 in $PATH
         //
         const char * path = getenv("PATH");   // must NOT be modified

         if (path)
            {
              logit && CERR << "initializing paths from  $PATH = "
                            << path << endl;

              // we must not modify path, so we copy it to path1 and
              // replace the semicolons in path1 by 0. That converts
              // p1;p2; ... into a sequence of 0-terminated strings
              // p1 p2 ... The variable next points to the start of each
              // string.
              //
              const size_t plen = strlen(path);
              std::string   path1;
              path1.reserve(plen + 1);
              loop(p, (plen + 1))   path1 += path[p];
              char * next = &path1[0];
              for (;;)
                  {
                    char * semi = strchr(next, ':');
                    if (semi)   *semi = 0;
                    UTF8_string filename;
                    for (const char * n = next; *n; ++n)   filename += *n;
                    filename += '/';
                    for (const char * a = argv0; *a; ++a)   filename += *a;

                    if (access(filename.c_str(), X_OK) == 0)
                       {
                         strncpy(APL_bin_path, filename.c_str(),
                                 sizeof(APL_bin_path) - 1);
                         APL_bin_path[sizeof(APL_bin_path) - 1] = 0;

                         char * slash =   strrchr(APL_bin_path, '/');
                         Assert(slash);   // due to %s/%s above
                         *slash = 0;
                         APL_bin_name = slash + 1;
                         goto done;
                       }

                    if (semi == 0)   break;
                    next = semi + 1;
                  }
            }
           else
            {
              logit && CERR << "initializing paths from $PATH failed because "
                               "it was not set" << endl;
            }
      }

   unused = realpath(argv0, APL_bin_path);
   APL_bin_path[APL_PATH_MAX] = 0;
   {
     char * slash =   strrchr(APL_bin_path, '/');
     if (slash)   { *slash = 0;   APL_bin_name = slash + 1; }
     else         { APL_bin_name = APL_bin_path;            }
   }

   // if we have a PWD and it is a prefix of APL_bin_path then replace PWD
   // by './'
   //
   if (const char * PWD = getenv("PWD"))   // we have a pwd
      {
        logit && CERR << "initializing paths from  $PWD = " << PWD << endl;
        const int PWD_len = strlen(PWD);
        if (!strncmp(PWD, APL_bin_path, PWD_len) && PWD_len > 1)
           {
             strcpy(APL_bin_path + 1, APL_bin_path + PWD_len);
             APL_bin_path[0] = '.';
           }
      }
   else
      {
        logit && CERR << "initializing paths from $PWD failed because "
                         "it was not set" << endl;
      }

done:
   logit && CERR << "APL_bin_path is: " << APL_bin_path << endl
                 << "APL_bin_name is: " << APL_bin_name << endl;
}
//-----------------------------------------------------------------------------
bool
LibPaths::is_lib_root(const char * dir)
{
char filename[APL_PATH_MAX + 1];

   snprintf(filename, sizeof(filename), "%s/workspaces", dir);
   if (access(filename, F_OK))   return false;

   snprintf(filename, sizeof(filename), "%s/wslib1", dir);
   if (access(filename, F_OK))   return false;

   return true;
}
//-----------------------------------------------------------------------------
void
LibPaths::search_APL_lib_root()
{
   APL_lib_root[0] = 0;

const char * path = getenv("APL_LIB_ROOT");
   if (path)
      {
        unused = realpath(path, APL_lib_root);
        root_from_env = true;
        return;
      }

   root_from_pwd = true;

   // search from "." to "/" for  a valid lib-root
   //
   unused = realpath(".", APL_lib_root);
   while (strlen(APL_lib_root))
      {
        if (is_lib_root(APL_lib_root))   return;   // lib-root found
        if (char * s = strrchr(APL_lib_root, '/'))         *s = 0;
        else if (char * s = strrchr(APL_lib_root, '\\'))   *s = 0;
        else
           {
             CERR << "*** Cannot locate APL_lib_root: no / or \\ in "
                  << APL_lib_root << endl;
             break;
           }
      }

   unused = realpath(".", APL_lib_root);
}
//-----------------------------------------------------------------------------
void
LibPaths::set_APL_lib_root(const char * new_root)
{
   unused = realpath(new_root, APL_lib_root);
}
//-----------------------------------------------------------------------------
void
LibPaths::set_lib_dir(LibRef libref, const char * path, LibDir::CfgSrc src)
{
   lib_dirs[libref].dir_path = UTF8_string(path);
   lib_dirs[libref].cfg_src = src;
}
//-----------------------------------------------------------------------------
UTF8_string
LibPaths::get_lib_dir(LibRef libref)
{
   switch(lib_dirs[libref].cfg_src)
      {
        case LibDir::CSRC_NONE:      return UTF8_string();

        case LibDir::CSRC_ENV:
        case LibDir::CSRC_PWD:       break;   // continue below

        case LibDir::CSRC_PREF_SYS:
        case LibDir::CSRC_PREF_HOME:
        case LibDir::CSRC_CMD:       return lib_dirs[libref].dir_path;
      }

UTF8_string ret(APL_lib_root);
   if (libref == LIB0)   // workspaces
      {
        const UTF8_string subdir("/workspaces");
        ret.append_UTF8(subdir);
      }
   else                  // wslibN
      {
        const UTF8_string subdir("/wslib");
        ret.append_UTF8(subdir);
        ret += libref + '0';
      }

   return ret;
}
//-----------------------------------------------------------------------------
void
LibPaths::maybe_warn_ambiguous(int name_has_extension, const UTF8_string name,
                               const char * ext1, const char * ext2)
{
   if (name_has_extension)   return;   // extension was provided
   if (ext2 == 0)            return;   // no second extension

UTF8_string filename_ext2 = name;
   filename_ext2.append_ASCII(ext2);
   if (access(filename_ext2.c_str(), F_OK))   return;   // not existing

   CERR << endl 
        << "WARNING: filename " << name << endl
        << "    is ambiguous because another file" << endl << "    "
        << filename_ext2 << endl
        << "    exists as well. Using the first (.xml) file." << endl << endl;
}
//-----------------------------------------------------------------------------
UTF8_string
LibPaths::get_lib_filename(LibRef lib, const UTF8_string & name, 
                           bool existing, const char * ext1, const char * ext2)
{
   // check if name has one of the extensions ext1 or ext2 already.
   //
int name_has_extension = 0;   // assume name has neither extension ext1 nor ext2
   if      (name.ends_with(ext1))   name_has_extension = 1;
   else if (name.ends_with(ext2))   name_has_extension = 2;

   if (name.starts_with("/")   || 
       name.starts_with("./")  || 
       name.starts_with("../"))
      {
        // paths from / or ./ are fallbacks for the case where the library
        // path setup is wrong. So that the user can survive by using an
        // explicit path
        //
        if (name_has_extension)   return name;

        UTF8_string filename(name);
        if (access(filename.c_str(), F_OK) == 0)   return filename;

        if (ext1)
           {
             UTF8_string filename_ext1 = name;
             filename_ext1.append_ASCII(ext1);
             if (!access(filename_ext1.c_str(), F_OK))
                {
                   // filename_ext1 exists, but filename_ext2 may exist as well.
                   // warn user unless an explicit extension was given.
                   //
                   maybe_warn_ambiguous(name_has_extension, name, ext1, ext2);
                   return filename_ext1;
                }
           }

        if (ext2)
           {
             UTF8_string filename_ext2 = name;
             filename_ext2.append_ASCII(ext2);
             if (!access(filename_ext2.c_str(), F_OK))   return filename_ext2;
           }

         // neither ext1 nor ext2 worked: return original name
         //
         filename = name;
         return filename;
      }

UTF8_string filename = get_lib_dir(lib);
   filename += '/';
   filename.append_UTF8(name);

   if (name_has_extension)   return filename;

   if (existing)
      {
        // file ret is supposed to exist (and will be openend read-only).
        // If it does return filename otherwise filename.extension.
        //
        if (access(filename.c_str(), F_OK) == 0)   return filename;

        if (ext1)
           {
             UTF8_string filename_ext1 = filename;
             filename_ext1.append_ASCII(ext1);
             if (!access(filename_ext1.c_str(), F_OK))
                {
                  maybe_warn_ambiguous(name_has_extension,
                                       filename, ext1, ext2);
                   return filename_ext1;
                }
           }

        if (ext2)
           {
             UTF8_string filename_ext2 = filename;
             filename_ext2.append_ASCII(ext2);
             if (!access(filename_ext2.c_str(), F_OK))   return filename_ext2;
           }

        return filename;   // without ext
      }
   else
      {
        // file may or may not exist (and will be created if not).
        // therefore checking the existence does not work.
        // check that the file ends with ext1 or ext2 if provided
        //
        if (name_has_extension)   return filename;

        if      (ext1) filename.append_ASCII(ext1);
        else if (ext2) filename.append_ASCII(ext2);
        return filename;
      }
}
//-----------------------------------------------------------------------------
bool
LibPaths::is_present(LibRef lib)
{
   // try to open dir and return false if that fails
   //
UTF8_string path = LibPaths::get_lib_dir(lib);
DIR * dir = opendir(path.c_str());
   if (!dir)   return false;

   closedir(dir);
   return true;
}
//-----------------------------------------------------------------------------

