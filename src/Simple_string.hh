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
   : items_allocated(MIN_ALLOC),
     items_valid(0)
      {
        items = new T[items_allocated];
      }

   /// constructor: the first \b len items of \b data
   Simple_string(const T * data, ShapeItem len)
   : items_allocated(len + ADD_ALLOC),
     items_valid(len)
      {
        Assert1(items_valid >= 0);
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
        _copy(items, data, len);
      }

   /// constructor: \b len times \b data
   Simple_string(ShapeItem len, T data)
   : items_allocated(len + ADD_ALLOC),
     items_valid(len)
      {
        Assert(items_valid >= 0);
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
        for (ShapeItem l = 0; l < len; ++l)   items[l] = data;
      }

   /// constructor: copy other string
   Simple_string(const Simple_string & other)
   : items_allocated(other.items_valid + ADD_ALLOC),
     items_valid(other.items_valid)
      {
        Assert(items_valid >= 0);
        items = new T[items_allocated];
        _copy(items, other.items, items_valid);
      }

   /// constructor: copy other string, starting at pos, at most len
   Simple_string(const Simple_string & other, ShapeItem pos, ShapeItem len)
      {
        if (len > other.items_valid - pos)   len = other.items_valid - pos;
        if (len < 0)   len = 0;

        items_valid = len;
        items_allocated = items_valid + ADD_ALLOC;
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
        _copy(items, other.items + pos, len);
      }

   /// destructor
   ~Simple_string()
      { destruct(); }

   /// destructor
   void destruct()
      { 
        delete [] items;
      }

   /// copy \b other
   Simple_string & operator =(const Simple_string & other)
      {
        delete [] items;
        new (this) Simple_string(other);
        return *this;
      }

   /// return the items of the string (not 0-terminated)
   const T * get_items() const
      {
        return items;
      }

   /// return true iff \b this is equal to \b other
   bool operator ==(const Simple_string<T> & other) const
      {
        if (items_valid != other.items_valid)   return false;
        for (ShapeItem c = 0; c < items_valid; ++c)
            if (items[c] != other.items[c])   return false;
        return true;
      }

   /// return true iff \b this is different from \b other
   bool operator !=(const Simple_string<T> & other) const
      { return !(*this == other); }

   /// return the number of characters in \b this string
   ShapeItem size() const
      { return items_valid; }

   /// return the idx'th character
   const T & operator[](ShapeItem idx) const
      {
        Assert(idx < items_valid);
        return items[idx];
      }

   /// return the idx'th character
   T & operator[](ShapeItem idx)
      {
        if ((items_valid - idx) <= 0)
           {
             debug(cerr) << "idx = " << idx << endl;
             Assert(0 && "Bad index");
           }
        return items[idx];
      }

   /// append character \b t to \b this string
   void append(const T & t)
      {
        if (items_valid - items_allocated >= 0)   extend();
        copy_1(items[items_valid++], t, LOC);
      }

   /// append character \b t to \b this string
   void append(const T & t, const char * loc)
      {
        if (items_valid - items_allocated >= 0)   extend();
        copy_1(items[items_valid++], t, loc);
      }

   /// append string \b other to \b this string
   void append(const Simple_string & other)
      {
        if (other.items_valid)
           {
             extend(items_valid + other.items_valid);
             _copy(items + items_valid, other.items, other.items_valid);
             items_valid += other.items_valid;
           }
      }

   /// insert character \b t after position \b pos
   void insert_after(ShapeItem pos, const T & t)
      {
        Assert(pos < items_valid);
        if (items_valid - items_allocated >= 0)   extend();
        for (ShapeItem s = items_valid - 1; s > pos; --s)  items[s + 1] = items[s];
        items[pos + 1] = t;
        ++items_valid;
      }

   /// insert character \b t before position \b pos
   void insert_before(ShapeItem pos, const T & t)
      {
        Assert(pos < items_valid);
        if (items_valid - items_allocated >= 0)   extend();
        for (ShapeItem s = items_valid - 1; s >= pos; --s)   items[s + 1] = items[s];
        items[pos] = t;
        ++items_valid;
      }

   /// decrease size to \b new_size
   void shrink(ShapeItem new_size)
      {
        Assert((items_valid - new_size) >= 0);
        items_valid = new_size;
      }

   /// insert \b count characters \b t after position \b pos
   void insert(ShapeItem pos, ShapeItem count, T t)
      {
        Assert(count >= 0);
        Assert(pos <= items_valid);
        extend(items_valid + count);
        if (pos < items_valid)
           copy_downwards(items + pos + count, items + pos, items_valid - pos);
        for (ShapeItem c = 0; c < count; ++c)  items[pos + c] = t;
        items_valid += count;
      }

    /// forget last element
    void pop()
      { if (items_valid)   --items_valid; }

   /// remove the first element(s)
   void drop_leading(ShapeItem count)
      {
        if (count <= 0)   return;
        if (count >= items_valid)   items_valid = 0;
        else
           {
             items_valid -= count;
             _copy(items, items + count, items_valid);
           }
      }

   /// return a reference to the last item (size() MUST be checked beforehand)
   const T & last() const
      { return items[items_valid - 1]; }

   /// return a reference to the last item (size() MUST be checked beforehand)
   T & last()
      { return items[items_valid - 1]; }

   /// shrink to size 0
   void clear()
      { items_valid = 0; }

   /// erase all items above pos
   void erase(ShapeItem pos)
      {
        if (items_valid > pos)   items_valid = pos;
      }

   /// erase \b count items above pos
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

         _copy(items + pos, items + pos + count, rest);
         items_valid -= count;
      }

   /// erase first character
   void remove_front()   { erase(0, 1); }

   /// extend allocated size
   void reserve(ShapeItem new_alloc_size)
      {
        extend(new_alloc_size);
      }

   /// compare strings
   Comp_result compare(const Simple_string & other) const
      {
        const ShapeItem common_len = items_valid < other.items_valid
                             ? items_valid : other.items_valid;
        for (ShapeItem c = 0; c < common_len; ++c)
            {
              if (items[c] < other.items[c])   return COMP_LT;
              if (items[c] > other.items[c])   return COMP_GT;
            }

        if (items_valid < other.items_valid)   return COMP_LT;
        if (items_valid > other.items_valid)   return COMP_GT;
        return COMP_EQ;
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
   /// print properties
   ostream & debug(ostream & out)
     {
       out << "items_allocated = " << items_allocated << endl
            << "items[" << items_valid << "] = ";
       for (ShapeItem i = 0; i < items_valid; ++i)   out << items[i];
       return out << endl;
     }

   enum
      {
        ADD_ALLOC = 4,   ///< extra chars added when extending the string
        MIN_ALLOC = 16,   ///< min. size allocated
      };

   /// double the allocated size
   void extend()
      {
        items_allocated += items_allocated;
        T * new_items = new T[items_allocated];
        _copy(new_items, items, items_valid);
        delete [] items;
        items = new_items;
      }

   /// increase the allocated size to at least new_size
   void extend(ShapeItem new_size)
      {
        if ((items_allocated - new_size) >= 0)   return;

        items_allocated = new_size + ADD_ALLOC;
        T * new_items = new T[items_allocated];
        _copy(new_items, items, items_valid);
        delete [] items;
        items = new_items;
      }

   /// copy \b count characters
   static void _copy(T * dst, const T * src, ShapeItem count)
      {
        Assert1(count >= 0);
        for (ShapeItem c = 0; c < count; ++c)  copy_1(dst[c], src[c], LOC);
      }

   /// copy \b count characters downwards (for overlapping src/dst
   static void copy_downwards(T * dst, const T * src, ShapeItem count)
      {
        Assert(count >= 0);
        for (ShapeItem c = count - 1; c >= 0; --c)  copy_1(dst[c], src[c], LOC);
      }

   /// the number of characters allocated
   ShapeItem items_allocated;

   /// the number of characters valid (always <= items_allocated)
   ShapeItem items_valid;

   /// the items
   T * items;
};
//-----------------------------------------------------------------------------

#endif // __SIMPLE_STRING_HH_DEFINED__

