#include "Quad_MAP.hh"
#include "Workspace.hh"

Quad_MAP  Quad_MAP::_fun;
Quad_MAP * Quad_MAP::fun = &Quad_MAP::_fun;

//-----------------------------------------------------------------------------
/*
    ⊢A←5 2⍴'eEwWaAzZ92' ◊ ⊢B←'Halloween' ◊ A ⎕MAP B
 */
Token
Quad_MAP::eval_AB(Value_P A, Value_P B)
{
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

Value_P Z = do_map(&A->get_ravel(0), map_len, indices, B.get());
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
                 const ShapeItem * sorted_indices_A, const Value * B)
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
                 new (&cell_Z0) CharCell(UNI_ASCII_SPACE);
        else
                 new (&cell_Z0) IntCell(0);
            }
         else   // not mapped
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
         else   // not mapped
            {
             Z->next_ravel()->init(cell_B, Z.getref(), LOC);
            }
       }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
