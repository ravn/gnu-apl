/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2020  Dr. Jürgen Sauermann

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

#include "Quad_MAP.hh"
#include "Workspace.hh"

Quad_MAP  Quad_MAP::_fun;
Quad_MAP * Quad_MAP::fun = &Quad_MAP::_fun;

//-----------------------------------------------------------------------------
/*
    8 ⎕CR A←5 2⍴'eEwWaAzZ92' ◊ 8 ⎕CR B←'Halloween' ◊ A ⎕MAP B
 */
Token
Quad_MAP::eval_AB(Value_P A, Value_P B) const
{
bool recursive = false;

   // a nested scalar A indicates recursive ⎕MAP
   //
   if (A->get_rank() == 0 && A->get_ravel(0).is_pointer_cell())
      {
         recursive = true;
         A = A->get_ravel(0).get_pointer_value();
      }

   if (A->get_rank() != 2)   RANK_ERROR;
   if (A->get_cols() != 2)   LENGTH_ERROR;
const ShapeItem map_len = A->get_rows();
   if (map_len == 0)   LENGTH_ERROR;

ShapeItem * indices = new ShapeItem[map_len];
   if (indices == 0)   WS_FULL;

   loop(m, map_len)   indices[m] = m;

const ravel_comp_len ctx = { &A->get_ravel(0), 1};
   Heapsort<ShapeItem>::sort(indices, map_len, &ctx, &Quad_MAP::greater_map);

   // complain about duplicated keys
   //
const double qct = Workspace::get_CT();
   for (ShapeItem m = 1; m < map_len; ++m)
       {
          const Cell & cm1 = A->get_ravel(2*indices[m - 1]);
          const Cell & cm  = A->get_ravel(2*indices[m    ]);
          if (cm1.equal(cm, qct))
             {
               const int qio = Workspace::get_IO();
               MORE_ERROR() << "Duplicate keys (e.g. A["
                            << (qio + indices[m - 1]) << "] and A["
                            << (qio + indices[m]) << "]) in 'A ⎕MAP B'";
               delete[] indices;
               DOMAIN_ERROR;
             }
       }

Value_P Z = do_map(&A->get_ravel(0), map_len, indices, B.get(), recursive);
   delete[] indices;
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
bool
Quad_MAP::greater_map(const ShapeItem & a, const ShapeItem & b,
                      const void * ctx)
{
const ravel_comp_len * rcl = reinterpret_cast<const ravel_comp_len *>(ctx);
const Cell * cells = rcl->ravel;

   if (const Comp_result cr = cells[2*a].compare(cells[2*b]))   // A[1] ≠ B[1]
      return cr == COMP_GT;

   return a > b;
}
//-----------------------------------------------------------------------------
int
compare_MAP(const Cell & key, const ShapeItem & item, const void * ctx)
{
const ravel_comp_len * rcl = reinterpret_cast<const ravel_comp_len *>(ctx);
const Cell * cells_A = rcl->ravel;

   return key.compare(cells_A[2*item]);
}
//-----------------------------------------------------------------------------

Value_P
Quad_MAP::do_map(const Cell * ravel_A, ShapeItem len_A,
                 const ShapeItem * sorted_indices_A, const Value * B,
                 bool recursive)
{
Value_P Z(B->get_shape(), LOC);
const ravel_comp_len ctx = { ravel_A, 1};

const ShapeItem len_B = B->element_count();
   if (len_B == 0)   // empty value
      {
         const Cell & cell_B = B->get_ravel(0);
         if (const ShapeItem * map =
                   Heapsort<ShapeItem>::search<const Cell &>
                                              (cell_B,
                                               sorted_indices_A,
                                               len_A,
                                               compare_MAP,
                                               &ctx))
            {
              Cell & cell_Z0 = Z->get_ravel(0);
              cell_Z0.init(ravel_A[*map*2 + 1], Z.getref(), LOC);
              if (cell_Z0.is_pointer_cell())
                 cell_Z0.get_pointer_value()->to_proto();
        else if (cell_Z0.is_character_cell())
                 new (&cell_Z0) CharCell(UNI_SPACE);
        else
                 new (&cell_Z0) IntCell(0);
            }
         else   // not mapped, simple
            {
              Z->get_ravel(0).init(cell_B, Z.getref(), LOC);
            }
      }

   loop(b, len_B)
       {
         const Cell & cell_B = B->get_ravel(b);
         if (const ShapeItem * map =
                   Heapsort<ShapeItem>::search<const Cell &>
                                              (cell_B,
                                               sorted_indices_A,
                                               len_A,
                                               compare_MAP,
                                               &ctx))
            {
             Z->next_ravel()->init(ravel_A[*map*2 + 1], Z.getref(), LOC);
            }
         else   // cell_B shall not be mapped
            {
              if (recursive && cell_B.is_pointer_cell())   // nested: recursive
                 {
                   Value_P sub_B = cell_B.get_pointer_value();
                   Value_P sub_Z = do_map(ravel_A, len_A, sorted_indices_A,
                                          sub_B.get(), true);
                   new (Z->next_ravel())  PointerCell(sub_Z.get(), Z.getref());
                 }
              else   // not mapped, simple
                 {
                  Z->next_ravel()->init(cell_B, Z.getref(), LOC);
                 }
            }
       }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
