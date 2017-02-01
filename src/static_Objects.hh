/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2017  Dr. Jürgen Sauermann

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

#ifndef STATIC_OBJECTS_HH_DEFINED
#define STATIC_OBJECTS_HH_DEFINED

/// a class that triggers logging when static objects are abeing initialized
class static_Objects
{
public:
   /// constructor
   static_Objects(const char * l, const char * w);

   /// destructor
   ~static_Objects();

   const char * what;
   const char * loc;

   static bool show_constructors;
   static bool show_destructors;
};


#endif // STATIC_OBJECTS_HH_DEFINED

