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

#ifndef __HEAPSORT_HH_DEFINED__
#define __HEAPSORT_HH_DEFINED__

#include <stdint.h>

/// heapsort an array of items of type \b T
template<typename T>
class Heapsort
{
public:
   /// a function to compare two items. The function returns true if item_a
   /// is larger than item_b. comp_arg is some additional argument guiding
   /// the comparison
   typedef bool (*greater_fun)(const T & item_a, const T & item_b,
                 const void * comp_arg);

   /// sort \b a according to \b gf
   static void sort(T * a, int64_t heapsize, const void * comp_arg,
                    greater_fun gf)
      {
        // turn a[] into a heap, i.e. a[i] > a[2i] and a[i] > a[2i+1]
        // for all i. At heapsize/2 + 1 ... the heap property is already
        // fulfilled, because these items have no children. So we move
        // downwards from heapsize/2 to 0.
        //
        for (int64_t p = heapsize/2 - 1; p >= 0; --p)
            make_heap(a, heapsize, p, comp_arg, gf);

        // here a[] is a heap (and therefore a[0] is the largest element)

        for (--heapsize; heapsize > 0; heapsize--)
            {
              // The root a[0] is the largest element in a[0] ... a[k].
              // Exchange a[0] and a[k], decrease the heap size,
              // and re-establish the heap property of the new a[0].
              //
              Hswap(a[heapsize], a[0]);

              // re-establish the heap property of the new a[0]
              //
              make_heap(a, heapsize, 0, comp_arg, gf);
            }
      }

   /// binary search of key in array)
   template<typename K>
   static const T * search(const K & key, const T * array,
                           int64_t /* array size */ u,
                           int (*compare)(const K & key, const T & item))
      {
        for (int64_t l = 0; l < u;)
           {
             const int64_t half = (l + u) / 2;
             const T * middle = &array[half];
             const int comp = (*compare)(key, *middle);
             if     (comp < 0)   u = half;
            else if (comp > 0)   l = half + 1;
             else                return middle;
          }

        return 0;
      }

protected:
   /// establish the heap property of the subtree with root a[i]
   static void make_heap(T * a, int64_t heapsize, int64_t parent,
                         const void * comp_arg, greater_fun gf)
      {
        for (;;)
           {
             const int64_t left = 2*parent + 1;   // left  child of parent.
             const int64_t right = left + 1;      // right child of parent.
             int64_t max = parent;                // assume parent is the max.

             // set max to the position of the largest of a[i], a[l], and a[r]
             //
             if ((left < heapsize) && (*gf)(a[left], a[max], comp_arg))
                max = left;   // left child is larger

             if ((right < heapsize) && (*gf)(a[right], a[max], comp_arg))
                max = right;   // right child is larger

             if (max == parent)   return; // parent was the max: done

             // left or right was the max. exchange and continue
             Hswap(a[max], a[parent]);
             parent = max;
           }
      }
};

#endif // __HEAPSORT_HH_DEFINED__
