/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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

#ifndef __SIMPLE_STRING_HH_DEFINED__
#define __SIMPLE_STRING_HH_DEFINED__

#include <iostream>

#ifdef AUXILIARY_PROCESSOR
# define __ASSERT_HH_DEFINED__
# include <assert.h>
# define Assert(x) assert(x)
#else
# include "Assert.hh"
# include "Common.hh"
#endif

using namespace std;

//-----------------------------------------------------------------------------
/// a simple string
template <typename T>
class Simple_string
{
public:
   /// constructor: empty string
   Simple_string()
      {
        allocate(0);
      }

   /// constructor: the first \b len items of \b data
   Simple_string(const T * data, ShapeItem len)
      {
        allocate(len);
        loop(l, items_valid)   new (items + l) T(data[l]);
      }

   /// constructor: \b len times \b data
   Simple_string(ShapeItem len, const T & data)
      {
        allocate(len);
        loop(l, items_valid)   new (items + l) T(data);
      }

   /// constructor: copy other string
   Simple_string(const Simple_string & other)
      {
        allocate(other.items_valid);
        loop(l, items_valid)   new (items + l) T(other.items[l]);
      }

   Simple_string(const Simple_string & other, ShapeItem pos, ShapeItem len)
      {
        Assert((pos + len) <= other.items_valid);
        allocate(len);
        loop(l, items_valid)   new (items + l) T(other.items[l + pos]);
      }

   /// destructor
   ~Simple_string()
      { destruct(); }

   /// explicit destructor
   void destruct()
      { 
        delete [] items;
        items = 0;
      }

   /// copy \b other
   void operator =(const Simple_string & other)
      {
        destruct();
        new (this) Simple_string(other);
      }

   /// return the items of the string (not 0-terminated)
   const T * get_items() const
      {
        return items;
      }

   /// return the number of characters in \b this string
   ShapeItem size() const
      { return items_valid; }

   /// return the idx'th character
   const T & operator[](ShapeItem idx) const
      { return at(idx); }

   /// return the idx'th character
   T & operator[](ShapeItem idx)
      { return at(idx); }

   /// append character \b t to \b this string
   void append(const T & t)
      {
        if (items_valid - items_allocated >= 0)   extend(2*items_allocated);
        new (items + items_valid++) T(t);
      }

   /// append character \b t to \b this string
   void append(const T & t, const char * loc)
      {
        if (items_valid - items_allocated >= 0)   extend(2*items_allocated);
        new (items + items_valid++) T(t);
      }

   /// append string \b other to \b this string
   void append(const Simple_string & other)
      {
        extend(items_valid + other.items_valid);
        loop(o, other.items_valid)   append(other[o]);
      }

   /// insert character \b t before position \b pos
   void insert_before(ShapeItem pos, const T & t)
      {
        Assert(pos <= items_valid);
        if (items_valid - items_allocated >= 0)   extend(2*items_allocated);
        for (ShapeItem s = items_valid - 1; s >= pos; --s)
            items[s + 1] = items[s];
        items[pos] = t;
        ++items_valid;
      }

   /// decrease size to \b new_size
   void shrink(ShapeItem new_size)
      {
        Assert((items_valid - new_size) >= 0);
        items_valid = new_size;
      }

    /// forget last element
    void pop()
      { if (items_valid)   --items_valid; }

   /// return a reference to the last item (size() MUST be checked beforehand)
   const T & last() const
      { return items[items_valid - 1]; }

   /// return a reference to the last item (size() MUST be checked beforehand)
   T & last()
      { return items[items_valid - 1]; }

   /// shrink to size 0
   void clear()
      { items_valid = 0; }

   /// erase \b count items, starting at \b pos
   void erase(ShapeItem pos, ShapeItem count)
      {
        if (pos >= items_valid)   return;   // nothing to erase

         // rest is the number of items right of pos.
         //
         // before:     front     erased        rest
         //             0                              
         //             <- pos -> <-count->
         //             <-  pos + count  ->     
         //
         const ShapeItem rest = items_valid - (pos + count);

         if (rest < 0)   // erase more than we have, i.e. no rest
           {
             items_valid = pos;
             return;
           }

         loop(r, rest)   new(items + pos + r) T(items[pos + count + r]);
         items_valid -= count;
      }

   /// extend allocated size
   void reserve(ShapeItem new_alloc_size)
      {
        extend(new_alloc_size);
      }

   /// exchange this and other (without copying the data)
   void swap(Simple_string & other)
      {
        const ShapeItem ia = items_allocated;
        items_allocated = other.items_allocated;
        other.items_allocated = ia;

        const ShapeItem iv = items_valid;
        items_valid = other.items_valid;
        other.items_valid = iv;

        T * const it = items;
        items = other.items;
        other.items = it;
      }

protected:
   enum
      {
        ADD_ALLOC = 4,    ///< extra chars added when extending the string
        MIN_ALLOC = 16,   ///< min. size allocated
      };

   void allocate(ShapeItem min_size)
      {
        Assert1(min_size >= 0);
        items_valid = min_size;
        items_allocated = items_valid + ADD_ALLOC;   // and a few more
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
      }

   /// increase the allocated size to at least new_size
   void extend(ShapeItem new_size)
      {
        if ((items_allocated - new_size) < 0)   // need more space
           {
             T * old_items = items;
             items_allocated = new_size + ADD_ALLOC;
             items = new T[items_allocated];
             loop(c, items_valid)   new (items + c) T(old_items[c]);
             delete [] old_items;
           }
      }

   /// the number of characters allocated
   ShapeItem items_allocated;

   /// the number of characters valid (always <= items_allocated)
   ShapeItem items_valid;

   /// the items
   T * items;

   /// return the idx'th character
   const T & at(ShapeItem idx) const
      {
        if (idx < 0)                    Assert(0 && "Bad index");
        if ((items_valid - idx) <= 0)   Assert(0 && "Bad index");
        return items[idx];
      }

   /// return the idx'th character
   T & at(ShapeItem idx)
      {
        if (idx < 0)                    Assert(0 && "Bad index");
        if ((items_valid - idx) <= 0)   Assert(0 && "Bad index");
        return items[idx];
      }

};
//-----------------------------------------------------------------------------

#endif // __SIMPLE_STRING_HH_DEFINED__

