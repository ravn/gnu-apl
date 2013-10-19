/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

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

#ifndef __VALUE_HH_DEFINED__
#define __VALUE_HH_DEFINED__

#include <memory>   // for shared_ptr
#include <vector>

#include "Cell.hh"
#include "DynamicObject.hh"
#include "Shape.hh"

using namespace std;

struct IndexExpr;
class CDR_string;

//=============================================================================
/**
    An APL value. It consists of a fixed header (rank, shape) and
    and a ravel (a sequence of cells).
 */
class Value : public DynamicObject
{
public:
   /// construct a skalar value (i.e. a values with rank 0).
   Value(const char * loc);

   /// construct a skalar value (i.e. a value with rank 0) from a cell
   Value(const Cell & cell, const char * loc);

   /// construct a true vector (i.e. a value with rank 1) with shape \b sh
   Value(ShapeItem sh, const char * loc);

   /// construct a general array with shape \b sh
   Value(const Shape & sh, const char * loc);

   /// construct a simple character vector from a UCS string
   Value(const UCS_string & ucs, const char * loc);

   /// construct a simple character vector from a CDR string
   Value(const CDR_string & ui8, const char * loc);

   /// construct a simple character vector from a 0-terminated C string
   Value(const char * string, const char * loc);

   /// destructor
   virtual ~Value();

   /// return a value containing pointers to all ravel cells of this value.
   Value_P get_cellrefs(const char * loc);

   /// assign \b val to the cell references in this value.
   void assign_cellrefs(Value_P val);

   /// return the number of elements (the product of the shapes).
   ShapeItem element_count() const
      { return shape.element_count(); }

   /// return the number of elements, but at least 1 (for the prototype).
   ShapeItem nz_element_count() const
      { return shape.nz_element_count(); }

   /// return the rank of \b this value
   Rank get_rank() const
     { return shape.get_rank(); }

   /// return the shape of \b this value
   const Shape & get_shape() const
     { return shape; }

   /// return the r'th shape element of \b this value
   ShapeItem get_shape_item(Rank r) const
      { return shape.get_shape_item(r); }

   /// return the length of the last dimension of \b this value
   ShapeItem get_last_shape_item() const
      { return shape.get_last_shape_item(); }

   /// return the length of the last dimension, or 1 for skalars
   ShapeItem get_cols() const
      { return shape.get_cols(); }

   /// return the product of all but the the last dimension, or 1 for skalars
   ShapeItem get_rows() const
      { return shape.get_rows(); }

   /// set the length of dimension \b r to \b sh.
   void set_shape_item(Rank r, ShapeItem sh)
      { shape.set_shape_item(r, sh); }

   /// reshape this value in place. The element count must not increase
   void set_shape(const Shape & sh)
      { Assert(sh.element_count() <= shape.element_count());   shape = sh; }

   /// return the position of cell in the ravel of \b this value.
   ShapeItem get_offset(const Cell * cell) const
      { return cell - &get_ravel(0); }

   /// return the first integer of a value (the line number of →Value).
   Function_Line get_line_number(double ct) const
      { const APL_Integer line(ravel[0].get_near_int(ct));
        Log(LOG_execute_goto)   CERR << "goto line " << line << endl;
        return Function_Line(line); }

   /// return the idx'th element of the ravel.
   Cell & get_ravel(ShapeItem idx)
      { Assert1(idx < nz_element_count());   return ravel[idx]; }

   /// return the idx'th element of the ravel.
   const Cell & get_ravel(ShapeItem idx) const
      { Assert1(idx < nz_element_count());   return ravel[idx]; }

   /// return the next byte after the ravel
   const Cell * get_ravel_end() const
      { return &ravel[nz_element_count()]; }

   /// set the prototype (according to B) if this value is empty.
   void set_default(Value_P B)
      { if (is_empty())   ravel[0].init_type(B->get_ravel(0)); }

   /// Return the number of skalars in this value (enlist).
   ShapeItem get_enlist_count() const;

   /// compute the depth of this value.
   Depth compute_depth() const;

   /// compute the CellType contained in \b this value.
   CellType compute_cell_types() const;

   /// store the skalars in this value into dest...
   void enlist(Cell * & dest, bool left) const;

   /// print \b this value
   ostream & print(ostream & out) const;

   /// print the properties (shape, flags etc) of \b this value
   ostream & print_properties(ostream & out, int indent) const;

   /// debug-print \b this value
   void debug(const char * info);

   /// print this value in 4 ⎕CR style
   ostream & print_boxed(ostream & out, const char * info = 0);

   /// return \b this indexed by (multi-dimensional) \b IDX.
   Value_P index(const IndexExpr & IDX);

   /// return \b this indexed by (one-dimensional) \b IDX.
   Value_P index(Value_P IDX);

   /// return \b this indexed by (one- or multi-dimensional) \b IDX.
   Value_P index(Token & B);

   /// If this value is a single axis between ⎕IO and ⎕IO + max_axis then
   /// return that axis. Otherwise throw AXIS_ERROR.
   Rank get_single_axis(Rank max_axis) const;

   /// convert the ravel of this value to a shape
   Shape to_shape() const;

   /// glue two values.
   static Value_P glue(Token & token, Token & token_A, Token & token_B,
                       const char * loc);

   /// return \b true iff \b this value is a skalar.
   bool is_skalar() const
      { return shape.get_rank() == 0; }

   /// return \b true iff \b this value is a simple (i.e. depth 0) skalar.
   bool is_simple_skalar() const
      { return is_skalar() && !get_ravel(0).is_pointer_cell(); }

   /// return \b true iff this value is an lval (selective assignment)
   /// i.e. return true if at least one leaf value is an lval.
   bool is_lval() const;

   /// return \b true iff \b this value is empty (some dimension is 0).
   bool is_empty() const
      { return shape.is_empty(); }

   /// return \b true iff \b this value is not a skalar.
   bool is_non_skalar() const
      { return  shape.get_rank() != 0; }

   /// return \b true iff \b this value is a skalar or vector.
   bool is_skalar_or_vector() const
      { return  get_rank() <= 1; }

   /// return \b true iff \b this value is a vector.
   bool is_vector() const
      { return  get_rank() == 1; }

   /// return \b true iff \b this value is a skalar or vector of length 1.
   bool is_skalar_or_len1_vector() const
      { return is_skalar() || is_vector() && (get_shape_item(0) == 1); }

   /// return \b true iff \b this value is a simple character vector.
   bool is_char_vector() const;

   /// return \b true iff \b this value is a simple character vector
   /// containing only APL characters (from ⎕AV)
   bool is_apl_char_vector() const;

   /// return \b true iff \b this value is a simple character scalar or vector.
   bool is_char_string() const
      { return is_char_skalar() || is_char_vector(); }

   /// return \b true iff \b this value is a simple character skalar.
   bool is_char_skalar() const;

   /// return the NOTCHAR property of the value. NOTCHAR is false for simple
   /// char arrays and true if any element is numeric or nested. The NOTCHAR
   /// property of empty arrays is the NOTCHAR property of its prototype.
   /// see also lrm p. 138.
   bool NOTCHAR() const;

   /// convert chars to ints and ints to chars (recursively).
   /// return the number of cells that are neither char nor int.
   int toggle_UCS();

   /// return \b true iff \b this value is a simple integer vector.
   bool is_int_vector(APL_Float qct) const;

   /// return \b true iff \b this value is a simple integer skalar.
   bool is_int_skalar(APL_Float qct) const;

   /// return true, if this value has complex cells, false iff it has only
   /// real cells. Throw domain error for other cells (char, nested etc.)
   bool is_complex(APL_Float qct) const;

   /// return \b true iff \b this value has the same rank as \b other.
   bool same_rank(Value_P other) const
      { return get_rank() == other->get_rank(); }

   /// return \b true iff \b this value has the same shape as \b other.
   bool same_shape(Value_P other) const
      { if (get_rank() != other->get_rank())   return false;
        for (uint32_t r = 0; r < get_rank(); ++r)
            if (get_shape_item(r) != other->get_shape_item(r))   return false;
        return true;
      }

   /// possible properties of a Value.
   enum ValueFlags
      {
        VF_NONE     = 0x000,   ///< no flags.
        VF_shared   = 0x001,   ///< value is shared:         don't delete
        VF_assigned = 0x002,   ///< value is assigned:       don't delete
        VF_forever  = 0x004,   ///< value is fixed forever:  don't delete
        VF_nested   = 0x008,   ///< value is nested:         don't delete
        VF_index    = 0x010,   ///< value belongs to index:  don't delete
        VF_deleted  = 0x020,   ///< value is deleted:        don't delete again
        VF_arg      = 0x040,   ///< value is an argument:    don't delete
        VF_eoc      = 0x080,   ///< value is an eoc arg:     don't delete
        VF_left     = 0x100,   ///< left value (←):          OK to delete
        VF_marked   = 0x200,   ///< marked to detect stale:  OK to delete
        VF_DONT_DELETE = VF_shared     // a constant in a user defined function
                       | VF_assigned   // assigned to a variable
                       | VF_forever    // static value
                       | VF_nested     // sub-value of a nested value
                       | VF_index      // owned by an IndexExpr
                       | VF_deleted
                       | VF_arg        // in use by SI::eval_XXX()
                       | VF_eoc,       // in use by an EOC handler
        VF_need_clone  = VF_shared      // need cloning on assign and friends
                       | VF_assigned
                       | VF_forever
                       | VF_nested
                       | VF_index
                       | VF_arg
                       | VF_eoc,
      };

   /// print debug info about setting or clearing of flags to CERR
   void flag_info(const char * loc, ValueFlags flag, const char * flag_name,
                  bool set) const;

   /// initialize value related variables and print some statistics.
   static void init();

#ifdef VF_TRACING_WANTED   // deep flag checking → enable LOC for set/clear
# define _LOC LOC
#else                      // no deep flag checkin
# define _LOC
#endif

   /// a macro defining set_x(), clear_x(), and is_x() for flag x
#define VF_flag(flag)                          \
   void SET_ ## flag(const char * loc) const   \
      { flag_info(loc, VF_ ## flag, #flag, true);   flags |=  VF_ ## flag; }       \
   void CLEAR_ ## flag(const char * loc) const \
      { flag_info(loc, VF_ ## flag, #flag, false); flags &=  ~VF_ ## flag; }       \
   void SET_ ## flag() const     { flags |=  VF_ ## flag;             } \
   void CLEAR_ ## flag() const   { flags &=  ~VF_ ## flag;            } \
   bool is_ ## flag() const      { return (flags & VF_ ## flag) != 0; }

# define set_shared()   SET_shared(_LOC)
# define set_assigned() SET_assigned(_LOC)
# define set_nested()   SET_nested(_LOC)
# define set_index()    SET_index(_LOC)
# define set_forever()  SET_forever(_LOC)
# define set_deleted()  SET_deleted(_LOC)
# define set_arg()      SET_arg(_LOC)
# define set_eoc()      SET_eoc(_LOC)
# define set_left()     SET_left(_LOC)
# define set_marked()   SET_marked(_LOC)

# define clear_shared()   CLEAR_shared(_LOC)
# define clear_assigned() CLEAR_assigned(_LOC)
# define clear_nested()   CLEAR_nested(_LOC)
# define clear_index()    CLEAR_index(_LOC)
# define clear_forever()  CLEAR_forever(_LOC)
# define clear_deleted()  CLEAR_deleted(_LOC)
# define clear_arg()      CLEAR_arg(_LOC)
# define clear_eoc()      CLEAR_eoc(_LOC)
# define clear_left()     CLEAR_left(_LOC)
# define clear_marked()   CLEAR_marked(_LOC)

   VF_flag(shared)
   VF_flag(assigned)
   VF_flag(nested)
   VF_flag(index)
   VF_flag(forever)
   VF_flag(deleted)
   VF_flag(arg)
   VF_flag(eoc)
   VF_flag(left)
   VF_flag(marked)

   /// mark all values, except static values
   static void mark_all_dynamic_values();

   /// clear marked flag on this value and its nested sub-values
   void unmark() const;

   /// set shared flag and unlink (for local values)
   void set_on_stack();

   /// maybe delete this value.
   void erase(const char * loc) const;

   /// the prototype of this value
   Value_P prototype(const char * loc) const;

   /// return a deep copy of \b this value
   Value_P clone(const char * loc) const;

   /// clone this value if needed (according to its flags)
   Value_P clone_if_owned(const char * loc)
      { return (flags & VF_need_clone) ? clone(loc) : this; }

   /// get the min spacing for this column and set/clear if there
   /// is/isn't a numeric item in the column.
   /// are/ain't numeric items in col.
   int32_t get_col_spacing(ShapeItem col, bool & numeric) const;

   /// list a value
   ostream & list_one(ostream & out, bool show_owners);

   /// check \b this value and return a token for it
   Token check_value(const char * loc);

   /// return the total CDR size (header + data + padding) for \b this value.
   int total_size_brutto(int CDR_type) const
      { return (total_size_netto(CDR_type) + 15) & ~15; }

   /// return the total CDR size in bytes (header + data),
   ///  not including any except padding for \b this value.
   int total_size_netto(int CDR_type) const;

   /// return the CDR size in bytes for the data of \b value,
   /// not including the CDR header and padding
   int data_size(int CDR_type) const;

   /// return the CDR type for \b this value
   int get_CDR_type() const;

   /// erase stale values
   static int erase_stale(const char * loc);

   /// erase all values (clean-up after )CLEAR)
   static void erase_all(ostream & out);

   /// list all values
   static ostream & list_all(ostream & out, bool show_owners);

   /// return the ravel of this values as UCS string, or throw DOMAIN error
   /// if the ravel contains non-char or nested cells.
   UCS_string get_UCS_ravel() const;

   /// convert this character value (of rank <= 2) to an array of UCS_string,
   /// one for each line
   void to_varnames(vector<UCS_string> & result, bool last) const;

   /// print address, shape, and flags of this value
   void print_structure(ostream & out, int indent, ShapeItem idx) const;

   /// return the current flags
   ValueFlags get_flags() const   { return ValueFlags(flags); }

   /// print info related to a stale value
   void print_stale_info(ostream & out, const DynamicObject * dob);

   /// print stale Values, and return the number of stale Values.
   static int print_stale(ostream & out);

#define stv_def(x) /** x **/ static Value x;
#include "StaticValues.def"

protected:
   /// init the ravel of an APL value, return the ravel length
   ShapeItem init_ravel()
      {
        const ShapeItem length = shape.element_count();
        if (length > SHORT_VALUE_LENGTH_WANTED)   ravel = new Cell[length];
        else                                      ravel = short_value;
        return length;
      }

   /// the shape of \b this value (only the first \b rank values are valid.
   Shape shape;

   /// a method number for creating a static value
   enum Value_how
      {
#define stv_def(x) Value_how_ ## x,
#include "StaticValues.def"
        Value_how_MAX   ///< all methof numbers are below this value
      };

   /// special constructor for static values (Zero, One etc.)
   Value(const char * loc, Value_how how);

   /// valueFlags for this value.
   mutable uint16_t flags;

   /// The ravel of \b this value.
   Cell * ravel;

   /// the cells of a short (i.e. ⍴,value ≤ SHORT_VALUE_LENGTH_WANTED) value
   Cell short_value[SHORT_VALUE_LENGTH_WANTED];
};
// ----------------------------------------------------------------------------

/// print flags in hex
inline ostream &
operator << (ostream & out, Value::ValueFlags flg)
{
  return out << HEX(flg);
}
// ----------------------------------------------------------------------------
/**
class Value_P : public shared_ptr<Value>
{
};
**/
// ----------------------------------------------------------------------------

#endif // __VALUE_HH_DEFINED__
