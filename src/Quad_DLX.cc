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

#include "Logging.hh"
#include "PointerCell.hh"
#include "Quad_DLX.hh"
#include "PrintOperator.hh"
#include "Token.hh"
#include "Workspace.hh"

Quad_DLX  Quad_DLX::_fun;
Quad_DLX * Quad_DLX::fun = &Quad_DLX::_fun;

//=============================================================================
class DLX_Node
{
public:
   /// constructor for empty node array
   DLX_Node()
   : row(-1),
     col(-1),
     up(this),
     down(this),
     left(this),
     right(this)
   {}

   DLX_Node(ShapeItem R, ShapeItem C,
            DLX_Node * u, DLX_Node * d, DLX_Node * l, DLX_Node * r)
   : row(R),
     col(C),
     up(u),
     down(d),
     left(l),
      right(r)
   { insert_lr();   insert_ud(); }

   void insert_lr()   { left->right = this;  right->left = this; }
   void insert_ud()   { up->down    = this;  down->up    = this; }

   void remove_lr()   { left->right = right; right->left = left; }
   void remove_ud()   { up->down    = down;  down->up    = up; }

   void check() const
      {
        Assert(up);    Assert(up->col    == col);
        Assert(down);  Assert(down->col  == col);
        Assert(left);  Assert(left->row  == row);
        Assert(right); Assert(right->row == row);
      }

   ostream & print_RC(ostream & out) const
     {
       return out << std::right << setw(2) << row << ":"
                  << std::left  << setw(2) << col << std::right;
     }

   void print(ostream & out) const
      {
        print_RC(out);
        up->print_RC(out << " ↑");
        down->print_RC(out << " ↓");
        left->print_RC(out << " ←");
        right->print_RC(out << " →");
        out << endl;
        check();
      }

   /// the type of column
   enum Col_Type
      {
        Col_INVALID   = -1,
        Col_NULL      = 0,
        Col_PRIMARY   = 1,
        Col_SECONDARY = 2,
        Col_UNKNOWN = Col_NULL
      };

   static Col_Type get_col_type(const Cell & cell);

   const ShapeItem row;
   const ShapeItem col;
   DLX_Node * up;
   DLX_Node * down;
   DLX_Node * left;
   DLX_Node * right;
};
//=============================================================================
class DLX_Header_Node : public DLX_Node
{
public:
   /// constructor for empty header array
   DLX_Header_Node()
   {}

   /// constructor for memory allocation
   DLX_Header_Node(ShapeItem C, DLX_Node * l, DLX_Node * r)
   : DLX_Node(-1, C, this, this, l, r),
     count(0),
     col_type(Col_UNKNOWN),
     current_solution(0)
   {}

   /// the number of '1's remaining in this column
   ShapeItem count;

   /// the type of column
   Col_Type col_type;

   /// the current_solution has nothing to do with this header node, but is
   /// held here to avoid an extra memory allocation. Every move removes
   /// one column, therefore the max. number of moves is at most the number
   /// of columns
   DLX_Node * current_solution;
};
//-----------------------------------------------------------------------------
DLX_Node::Col_Type
DLX_Node::get_col_type(const Cell & cell)
{
   if (cell.is_character_cell())
      {
        // blank and '0' shall mean 0, '1' means 1, and '2' means 2
        //
        switch(cell.get_char_value())
           {
             case ' ':
             case '0':   return Col_NULL;
             case '1':   return Col_PRIMARY;
             case '2':   return Col_SECONDARY;
             default:    return Col_INVALID;
           }
      }
   if (cell.is_numeric())
      {
        switch(cell.get_int_value())
           {
             case 0:   return Col_NULL;
             case 1:   return Col_PRIMARY;
             case 2:   return Col_SECONDARY;
             default:  return Col_INVALID;
           }
      }

   return Col_INVALID;
}
//=============================================================================
class DLX_Root_Node : public DLX_Node
{
public:
   DLX_Root_Node(ShapeItem rows, ShapeItem cols, ShapeItem max_sol,
                 const Value & B);

   ostream & indent(ostream & out)
      {
        loop(s, solution_length)   out << "  ";
        return out;
      }

  void cover(ShapeItem j)
     {
       Log(LOG_Quad_DLX)   indent(CERR) << "Covering column " << j << endl;
       ++cover_count;

       DLX_Header_Node & c = headers[j];
       Assert(c.row == -1);
       Assert(c.col == j);
       c.remove_lr();
       if (c.col_type == DLX_Header_Node::Col_PRIMARY)   --primary_count;

       for (DLX_Node * i = c.down; i != &c; i = i->down)
           {
             Assert(i->col == j);
             for (DLX_Node * j = i->right; j != i; j = j->right)
                 {
                   j->remove_ud();
                   --headers[j->col].count;
                 }
           }
     }

   void uncover(ShapeItem j)
     {
       Log(LOG_Quad_DLX)   indent(CERR) << "  Uncovering column " << j << endl;

       DLX_Header_Node & c = headers[j];
       Assert(c.row == -1);
       Assert(c.col == j);
       for (DLX_Node * i = c.up; i != &c; i = i->up)
           {
             Assert(i->col == j);
             for (DLX_Node * j = i->left; j != i; j = j->left)
                 {
                   j->insert_ud();
                   ++headers[j->col].count;
                 }
           }
       c.insert_lr();
       if (c.col_type == DLX_Header_Node::Col_PRIMARY)   ++primary_count;
     }

   void display(ostream & out) const;

   void solve();

   ShapeItem get_solution_count() const   { return solution_count; }
   ShapeItem get_pick_count() const       { return pick_count; }
   ShapeItem get_cover_count() const      { return cover_count; }

   /// all solutions as len rows... len rows ...
   Simple_string<ShapeItem> all_solutions;

protected:
   /// the max. number of solutions to produce, 0 = all
   const ShapeItem max_solution_count;

   /// the number of rows
   const ShapeItem rows;

   /// the number of columns
   const ShapeItem cols;

   /// the column headers
   __DynArray<DLX_Header_Node> headers;

   /// the '1's and '2's in the (sparse) matrix
   __DynArray<DLX_Node> nodes;

   /// the number of primary columns
   ShapeItem primary_count;

   /// the number of primary columns
   ShapeItem secondary_count;

   /// the number of solutions found so far
   ShapeItem solution_count;

   /// the number of rows in the current partial solution (aka recursion depth)
   ShapeItem solution_length;

   /// the total number of pick'ed elements
   ShapeItem pick_count;

   /// the total number of covr operations
   ShapeItem cover_count;
};
//-----------------------------------------------------------------------------
DLX_Root_Node::DLX_Root_Node(ShapeItem rs, ShapeItem cs, ShapeItem max_sol,
                             const Value & B)
   : DLX_Node(-1, -1, this, this, this, this),
     max_solution_count(max_sol),
     rows(rs),
     cols(cs),
     headers(0),
     nodes(0),
     primary_count(0),
     secondary_count(0),
     solution_count(0),
     solution_length(0),
     pick_count(0),
     cover_count(0)
{
   // count the number of non-zero elemnts in the matrix
   //
const ShapeItem ec_B = B.element_count();
const int qio = Workspace::get_IO();
ShapeItem ones = 0;
const Cell * b = &B.get_ravel(0);
   loop(e, ec_B)
      {
        const Cell & cell = *b++;
        const Col_Type ct = get_col_type(cell);
        if (ct == Col_INVALID)
           {
             const ShapeItem row = qio + e/B.get_cols();
             const ShapeItem col = qio + e%B.get_cols();
             UCS_string & more = MORE_ERROR() << "Bad B[" << row << ";"
                                              << col << "]: ";
             if (cell.is_integer_cell())
                more << cell.get_int_value();
             else if (cell.is_character_cell())
                more << "'" << cell.get_char_value() << "'";
             else
                more << "neither integer nor character";
             DOMAIN_ERROR;
           }
        if (ct != Col_UNKNOWN)   ++ones;
      }

   Log(LOG_Quad_DLX)   CERR << "Matrix has " << ones << " ones" << endl;

   // set up column headers
   //
   new (&headers) __DynArray<DLX_Header_Node>(cols);
   loop (c, cols)
      {
        if (c) new (&headers[c]) DLX_Header_Node(c, &headers[c - 1], this);
        else   new (&headers[c]) DLX_Header_Node(c, this,            this);
      }

   // set up non-header nodes
   //
   b = &B.get_ravel(0);
   new (&nodes) __DynArray<DLX_Node>(ones);
   DLX_Node * n = &nodes[0];
   loop (r, rows)
      {
        DLX_Node * const lm = n;   // leftmost item in this row
        DLX_Node * rm = n;         // rightmost item in this row
        loop (c, cols)
           {
             DLX_Header_Node & hdr = headers[c];
             const Col_Type ct = get_col_type(*b++);
             if (ct < Col_PRIMARY)   continue;   // most likely: not '1' or '2'

             if (hdr.col_type == Col_UNKNOWN)   // first '1' or '2' in this col
                {
                  hdr.col_type = ct;
                  if (ct == Col_PRIMARY)   ++primary_count;
                  else                     ++secondary_count;
                }
             else if (ct != hdr.col_type)       // col type mismatch
                {
                  MORE_ERROR() <<
                  "the columns of B in ⎕DLX B must either be primary ("
                  "containing 1s) or secondary (containing  2s), but not both";
                  DOMAIN_ERROR;
                }


             if (rm == n)
                rm = new (n++)   DLX_Node(r, c, hdr.up, &hdr, rm, lm);
             else
                rm = new (n++)   DLX_Node(r, c, hdr.up, &hdr, rm, lm);
           }

        if (lm == rm) // there was no 1 in this row
           {
             MORE_ERROR() << "⎕DLX B with empty row B["
                          << (r + qio) << ";]";
             DOMAIN_ERROR;
           }
      }

   Log(LOG_Quad_DLX)   { loop(n, ones)   nodes[n].print(CERR); }
}
//-----------------------------------------------------------------------------
void
DLX_Root_Node::solve()
{
new_level:
DLX_Node * item = right;

   if (LOG_Quad_DLX || attention_is_raised())
      {
         CERR << "⎕DLX[" << solution_length << "]";
         loop(s, solution_length)
             CERR << " "
                  << (headers[s].current_solution->row + Workspace::get_IO());
         CERR << endl;
         clear_attention_raised(LOC);
      }

   if (interrupt_is_raised())
      {
        clear_interrupt_raised(LOC);
        INTERRUPT;
      }

   if (item != this)   // select shortest column
      {
        ShapeItem item_size = headers[item->col].count;
        for (DLX_Node * i = item->right; i != this; i = i->right)
            {
              const ShapeItem i_size = headers[i->col].count;
              if (i_size < item_size)
                 {
                   item = i;
                   item_size = i_size;
                 }
            }
      }

   Assert(item->row == -1);

   // the problem is solved if there are no primary columns left
   //
   if (primary_count == 0)
      {
        all_solutions.append(solution_length);
        loop(s, solution_length)
            all_solutions.append(headers[s].current_solution->row);

        Log(LOG_Quad_DLX)
           {
             CERR << "!!!!! solution " << solution_count
                  << ": rows are";
             loop(s, solution_length)
                  CERR << " " << setw(2) << headers[s].current_solution->row;
             CERR << endl;
           }

        ++solution_count;
        if (max_solution_count &&
            solution_count >= max_solution_count)   return;
      }
// else display(CERR);

next_in_column:
   item = item->down;
   if (item->row == -1)   // no '1' in column: no solution
      {
        if (solution_length == 0)   // no more backtracks
           {
             Log(LOG_Quad_DLX)
                {
                  if (solution_count == 0)   CERR << "no solution" << endl;
                }
             return;
           }

        item = headers[--solution_length].current_solution;   // restore item
        Log(LOG_Quad_DLX)
           {
             item->print_RC(indent(CERR) << "Backtrack item ") << endl;
           }

        for (DLX_Node * col_j = item->left; ; col_j = col_j->left)
            {
              uncover(col_j->col);
              if (col_j == item)   break;
            }

        goto next_in_column;
      }

   Log(LOG_Quad_DLX)
      {
        item->print_RC(indent(CERR) << "Picking item   ") << endl;
      }

   ++pick_count;
   headers[solution_length++].current_solution = item;

   // For each j such that A[r, j] = 1, i.e. for row r
   //
   for (DLX_Node * col_j = item->right; ; col_j = col_j->right)
       {
         // delete column j and its rows from matrix A;
         //
         cover(col_j->col);
         if (col_j == item)   break;
       }

   goto new_level;
}
//=============================================================================
Token
Quad_DLX::eval_AB(Value_P A, Value_P B)
{
   if (A->element_count() != 1)   LENGTH_ERROR;
const APL_Integer a0 = A->get_ravel(0).get_int_value();

   if (a0 < -3)   DOMAIN_ERROR;

   return do_DLX(a0, B.getref());
}
//-----------------------------------------------------------------------------
Token
Quad_DLX::eval_B(Value_P B)
{
   return do_DLX(0, B.getref());
};
//-----------------------------------------------------------------------------
Token
Quad_DLX::do_DLX(ShapeItem how, const Value & B)
{
   if (B.get_rank() != 2)   RANK_ERROR;

const ShapeItem rows = B.get_rows();
const ShapeItem cols = B.get_cols();

ShapeItem result_count = how;
   if      (how ==  0)  result_count = 1;   // first solution
   else if (how == -1)  result_count = 0;   // all solution
   else if (how == -2)  result_count = 1;   // statistics: first solution
   else if (how == -3)  result_count = 0;   // statistics: all solutions

DLX_Root_Node root(rows, cols, result_count, B);
   root.solve();

   if (how <= -2)   // return statistics
      {
       Value_P Z(3, LOC);
       new (Z->next_ravel())   IntCell(root.get_solution_count());
       new (Z->next_ravel())   IntCell(root.get_pick_count());
       new (Z->next_ravel())   IntCell(root.get_cover_count());
       Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const APL_Integer qio = Workspace::get_IO();
   if (how == 0)   // return first (flat) solution
      {
       if (root.get_solution_count() == 0)   // but no solution found
          {
            return Token(TOK_APL_VALUE1, Idx0(LOC));
          }

       const ShapeItem len = root.all_solutions[0];
       Value_P Z(len, LOC);
       loop(z, len)
         {
           new (Z->next_ravel()) IntCell(qio + root.all_solutions[z + 1]);
         }
       Z->check_value(LOC);
       return Token(TOK_APL_VALUE1, Z);
      }

   // return all solutions
   //
Value_P Z(root.get_solution_count(), LOC);
   if (root.get_solution_count() == 0)   // empty result
      {
        Value_P Z1 = Idx0(LOC);   // Z1 is ⍬
        new (&Z->get_ravel(0))   PointerCell(Z1, Z.getref());
      }
   else
      {
        ShapeItem a = 0;
        loop(s, root.get_solution_count())
           {
             const ShapeItem len = root.all_solutions[a++];
             Value_P Zs(len, LOC);
             loop(z, len)
                {
                  new (Zs->next_ravel()) IntCell(qio + root.all_solutions[a++]);
                }
             Zs->check_value(LOC);
             new (Z->next_ravel())   PointerCell(Zs, Z.getref());
           }
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

