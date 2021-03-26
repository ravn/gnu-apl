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

#ifndef __APL_TYPES_HH_DEFINED__
#define __APL_TYPES_HH_DEFINED__

#ifndef __COMMON_HH_DEFINED__
# error This file shall not be #included directly, but by #including Common.hh
#endif

#include <complex>
#include <memory>

#include <math.h>
#include <stdint.h>

#include "Common.hh"
#include "Unicode.hh"

using namespace std;

#define APL_Float_is_class 0

//////////////////////////////////////////////////////////////
// A. typedefs                                              //
//////////////////////////////////////////////////////////////

/// The rank of an APL value.
typedef int32_t Rank;
typedef Rank Axis;

typedef uint32_t uRank;
typedef uRank uAxis;

/// The dimensions of an APL value.
typedef int64_t ShapeItem;

/// The SI level, 0 = global (oldest), caller at level N called function at N+1
typedef int SI_level;

/// One APL character value.
typedef Unicode APL_Char;

/// One APL integer value.
typedef int64_t APL_Integer;

/// long long for sprintf() and friends
typedef long long long_long;

/// unsigned long for sprintf() and friends
typedef unsigned long unsigned_long;

class Function;
typedef const Function * Function_P;

inline void
Hswap(APL_Integer & i1, APL_Integer & i2)
{ const APL_Integer tmp = i1;   i1 = i2;   i2 = tmp; }

/// One (real) APL floating point value.
#if APL_Float_is_class // APL_Float is a class

#include "APL_Float_as_class.hh"

inline void release_APL_Float(APL_Float * x)   { x->~APL_Float(); }

#else   // APL_Float is a POD (double)

typedef double APL_Float_Base;
typedef APL_Float_Base APL_Float;

#define complex_exponent(x) exp(x)
#define complex_power(x, y) pow((x), (y))
#define complex_sqrt(x)     sqrt(x)
#define release_APL_Float(x)

#endif // APL_Float is class vs. POD

//-----------------------------------------------------------------------------
/// One APL complex value.
typedef complex<APL_Float> APL_Complex;
//-----------------------------------------------------------------------------
/// APL time = microseconds since Jan. 1. 1970 00:00:00 UTC
typedef int64_t APL_time_us;

class Symbol;
class Value;
class Cell;

//////////////////////////////////////////////////////////////
// B. enums            i                                    //
//////////////////////////////////////////////////////////////

/// The possible cell types (in the ravel of an APL value)
enum CellType
{
   CT_NONE    = 0,
   CT_BASE    = 0x01,
   CT_CHAR    = 0x02,
   CT_POINTER = 0x04,
   CT_CELLREF = 0x08,
   CT_INT     = 0x10,
   CT_FLOAT   = 0x20,
   CT_COMPLEX = 0x40,
   CT_NUMERIC = CT_INT  | CT_FLOAT   | CT_COMPLEX,
   CT_SIMPLE  = CT_CHAR | CT_NUMERIC,
   CT_MASK    = CT_CHAR | CT_NUMERIC | CT_POINTER | CT_CELLREF,

   // sub-types for Int and Char cells of different sizes.
   //
   CTS_BIT    = 0x0001000,   ///<                    1-bit  integer
   CTS_X8     = 0x0002000,   ///< signed or unsigned 8-bit  integer or char
   CTS_S8     = 0x0004000,   ///< signed             8-bit  integer or char
   CTS_U8     = 0x0008000,   ///< unsigned           8-bit  integer or char
   CTS_X16    = 0x0010000,   ///< signed or unsigned 16-bit integer or char
   CTS_S16    = 0x0020000,   ///< signed             16-bit integer or char
   CTS_U16    = 0x0040000,   ///< unsigned           16-bit integer or char
   CTS_X32    = 0x0080000,   ///< signed or unsigned 32-bit integer or char
   CTS_S32    = 0x0100000,   ///< signed             32-bit integer or char
   CTS_U32    = 0x0200000,   ///< unsigned           32-bit integer or char
   CTS_X64    = 0x0400000,   ///< signed or unsigned 64-bit integer
   CTS_S64    = 0x0800000,   ///< signed             64-bit integer
   CTS_U64    = 0x1000000,   ///< unsigned           64-bit integer
   CTS_MASK   = 0x1FFF000,   ///< 
};

/// flags of a line to be printed. These flags extend CellType
enum Col_flags
{
   has_j         = 0x0100,   // CT_COMPLEX and imag != 0
   real_has_E    = 0x0200,   // real part scaled (exponential format)
   imag_has_E    = 0x0400,   // imag part scaled (exponential format)
};

///  What to list, used by )SYMBOLS, )VARS, and )FUNS.
enum ListCategory
{
  LIST_NONE    = 0,           ///< list nothing
  LIST_VARS    = 0x01,        ///< list variables for )VARS
  LIST_FUNS    = 0x02,        ///< list functions for )FNS
  LIST_OPERS   = 0x04,        ///< list operators for )OPS
  LIST_LABELS  = 0x08,        ///< list labels
  LIST_ERASED  = 0x10,        ///< list erased symbols
  LIST_INVALID = 0x20,        ///< list invalid symbols
  LIST_UNUSED  = 0x40,        ///< list unused symbols
  LIST_NAMES   = LIST_VARS    ///< list names for )NMS
               | LIST_FUNS
               | LIST_OPERS,
  LIST_ALL     = 0xFFFFFFFF   ///< list everything
};
//-----------------------------------------------------------------------------
/// possible properties of a Value.
enum ValueFlags
{
  VF_NONE     = 0x0000,   ///< no flags
  VF_complete = 0x0400,   ///< CHECK called
  VF_marked   = 0x0800,   ///< marked to detect stale
  VF_temp     = 0x1000,   ///< computed value
  VF_member   = 0x2000,   ///< used for member access
};

extern ostream & print_flags(ostream & out, ValueFlags flags);

//-----------------------------------------------------------------------------
/// events for APL values
enum VH_event
{
  VHE_None =  0,   ///< no event
  VHE_Create,      ///< value created (shape is complete, ravel is not)
  VHE_Unroll,      ///< rollback creation
  VHE_Check,       ///< check function called (if enabled)
  VHE_SetFlag,     ///< set a value flag
  VHE_ClearFlag,   ///< clear a value flag
  VHE_Erase,       ///< erase the value
  VHE_Destruct,    ///< destruct the value
  VHE_Error,       ///< some APL error has occurred
  VHE_PtrNew,      ///< new Value_P created
  VHE_PtrNew0,     ///< new Value_P with 0-pointer created
  VHE_PtrCopy1,    ///< Value_P copied with constructor(Value_P)
  VHE_PtrCopy2,    ///< Value_P copied with constructor(Value_P, loc)
  VHE_PtrCopy3,    ///< Value_P copied with operator =()
  VHE_PtrClr,      ///< Value_P cleared
  VHE_PtrDel,      ///< Value_P deleted
  VHE_PtrDel0,     ///< Value_P deleted (with 0 pointer)
  VHE_TokCopy1,    ///< token with Value_P copied
  VHE_TokMove1,    ///< token with Value_P moved
  VHE_TokMove2,    ///< token with Value_P moved
  VHE_Completed,   ///< incomplete ravel set to 42424242
  VHE_Stale,       ///< stale value erased
  VHE_Visit,       ///< test point
};
//-----------------------------------------------------------------------------
/// The bits in an int
enum Bitmask
{
   BIT_0  = 1 <<  0,   ///<< dito.
   BIT_1  = 1 <<  1,   ///<< dito.
   BIT_2  = 1 <<  2,   ///<< dito.
   BIT_3  = 1 <<  3,   ///<< dito.
   BIT_4  = 1 <<  4,   ///<< dito.
   BIT_5  = 1 <<  5,   ///<< dito.
   BIT_6  = 1 <<  6,   ///<< dito.
   BIT_7  = 1 <<  7,   ///<< dito.
   BIT_8  = 1 <<  8,   ///<< dito.
   BIT_9  = 1 <<  9,   ///<< dito.
   BIT_10 = 1 << 10,   ///<< dito.
   BIT_11 = 1 << 11,   ///<< dito.
   BIT_12 = 1 << 12,   ///<< dito.
   BIT_13 = 1 << 13,   ///<< dito.
   BIT_14 = 1 << 14,   ///<< dito.
   BIT_15 = 1 << 15,   ///<< dito.
   BIT_16 = 1 << 16,   ///<< dito.
   BIT_17 = 1 << 17,   ///<< dito.
   BIT_18 = 1 << 18,   ///<< dito.
   BIT_19 = 1 << 19,   ///<< dito.
   BIT_20 = 1 << 20,   ///<< dito.
   BIT_21 = 1 << 21,   ///<< dito.
   BIT_22 = 1 << 22,   ///<< dito.
   BIT_23 = 1 << 23,   ///<< dito.
   BIT_24 = 1 << 24,   ///<< dito.
   BIT_25 = 1 << 25,   ///<< dito.
   BIT_26 = 1 << 26,   ///<< dito.
   BIT_27 = 1 << 27,   ///<< dito.
   BIT_28 = 1 << 28,   ///<< dito.
   BIT_29 = 1 << 29,   ///<< dito.
   BIT_30 = 1 << 30,   ///<< dito.
   BIT_31 = 1 << 31    ///<< dito.
};
//-----------------------------------------------------------------------------
/// the line number of an APL function line (0 being the line to
/// return from the function).
enum Function_Line
{
   Function_Retry   = -2,   // →'' in immediate execution
   Function_Line_0  =  0,
   Function_Line_1  =  1,
   Function_Line_10 = 10,
};
//-----------------------------------------------------------------------------
///  What is being parsed (function, immediate execution statements, or ⍎expr)
enum ParseMode
{
   PM_FUNCTION       = 0,   ///< function execution
   PM_STATEMENT_LIST = 1,   ///< immediate execution
   PM_EXECUTE        = 2,   ///< execute (⍎)
};
//-----------------------------------------------------------------------------
/// Different ways of printing APL values.
enum PrintStyle
{
   PST_NONE          = 0,                ///< nothing

   // character sets for inner and outer frames
   //
   PST_CS_NONE       = 0x00000000,       ///< ASCII chars (- | and +)
   PST_CS_ASCII      = 0x00000001,       ///< ASCII chars (- | and +)
   PST_CS_THIN       = 0x00000002,       ///< thin lines
   PST_CS_THICK      = 0x00000003,       ///< thick lines
   PST_CS_DOUBLE     = 0x00000004,       ///< double lines
   PST_CS_MASK       = 0x0000000F,       ///< mask for line style

   PST_CS_INNER      = PST_CS_MASK,      ///< mask for inner line style
   PST_CS_OUTER      = PST_CS_MASK << 4, ///< mask for outer line style

   // flags for numeric formatting
   //
   PST_SCALED        = 0x00000100,       ///< use exponential form
   PST_NO_EXPO_0     = 0x00000200,       ///< remove E0
   PST_NO_FRACT_0    = 0x00000400,       ///< remove trailing fractional 0s
   PST_EMPTY_LAST    = 0x00000800,       ///< last axis is empty
   PST_EMPTY_NLAST   = 0x00001000,       ///< some non-last axis is empty

   PST_MATRIX        = 0x00010000,       ///< multi line output
   PST_INPUT         = 0x00020000,       ///< use '' for chars and () for nested
   PST_PACKED        = 0x00040000,       ///< 4 byte/char
   PST_NARS          = 0x00080000,       ///< NARS APL ⎕FMT style
   PST_QUOTE_CHARS   = 0x00100000,       ///< quote chars and strings
   PST_PRETTY        = 0x00200000,       ///< print control characters nicely

   PR_APL            = PST_MATRIX,       ///< normal APL output
   PR_APL_MIN        = PST_MATRIX        ///< normal APL output w/o E0 and ...0
                     | PST_NO_FRACT_0
                     | PST_NO_EXPO_0,
   PR_APL_FUN        = PST_INPUT,        ///< ⎕CR function
   PR_Quad           = PST_INPUT,        ///< ⎕ output
   PR_QUOTE_Quad     = PST_NONE,         ///< ⍞ output
   PR_BOXED_CHAR     = PST_MATRIX
                     | PST_CS_ASCII,     ///< 2 ⎕CR (ASCII chars)
   PR_BOXED_GRAPHIC  = PST_MATRIX
                     | PST_PRETTY
                     | PST_CS_THICK,     ///< 3/4 ⎕CR (graphic characters)
   PR_BOXED_GRAPHIC1 = PST_MATRIX
                     | PST_PRETTY
                     | PST_CS_THIN,      ///< 7/8 ⎕CR (graphic characters)
   PR_BOXED_GRAPHIC2 = PST_MATRIX
                     | PST_PRETTY
                     | PST_CS_THICK 
                     | PST_CS_DOUBLE << 4, ///< DISPLAY graphic in double box
   PR_BOXED_GRAPHIC3 = PST_MATRIX
                     | PST_PRETTY
                     | PST_CS_THICK
                     | PST_QUOTE_CHARS,    ///<  29 ⎕CR (quoted chars)
   PR_NARS           = PR_BOXED_GRAPHIC  | PST_NARS,  ///< NARS APL style
   PR_NARS1          = PR_BOXED_GRAPHIC1 | PST_NARS,  ///< NARS APL style,
   PR_NARS2          = PR_BOXED_GRAPHIC2 | PST_NARS,  ///< NARS APL style,
};
//-----------------------------------------------------------------------------
/// an offset into the body of a user-defined function. If we consider the APL
/// interpreter as a high-level machine that executes token in user defined
/// functions then this offset is the "program counter" of the high-level
/// machine.
enum Function_PC
{
   Function_PC_0       =  0,   ///< the first token in a function
   Function_PC_done    = -1,   ///< goto 0
   Function_PC_invalid = -1    ///< dito
};
//-----------------------------------------------------------------------------
/// signature of a user defined function
enum Fun_signature
{
   // atoms
   //
   SIG_NONE            = 0,      ///< 
   SIG_Z               = 0x01,   ///< function has a result
   SIG_A               = 0x02,   ///< function has a left argument
   SIG_LO              = 0x04,   ///< operator left operand
   SIG_FUN             = 0x08,   ///< function (always set)
   SIG_RO              = 0x10,   ///< operator right operand
   SIG_X               = 0x20,   ///< RO has an axis
   SIG_B               = 0x40,   ///< function has a right argument

   // operator variants
   //
   SIG_FUN_X           = SIG_FUN | SIG_X,     ///< function with axis
   SIG_OP1             = SIG_LO  | SIG_FUN,   ///< monadic operator
   SIG_OP1_X           = SIG_OP1 | SIG_X,     ///< monadic operator with axis
   SIG_OP2             = SIG_OP1 | SIG_RO,    ///< dyadic operator
   SIG_LORO            = SIG_LO  | SIG_RO,    ///< monadic or dyadic operator

   // argument variants
   //
   SIG_NIL             = SIG_NONE,            ///< niladic function
   SIG_MON             = SIG_B,               ///< monadic function or operator
   SIG_DYA             = SIG_A | SIG_B,       ///< dyadic function or operator

   // allowed combinations of operator variants and argument variants...

   // niladic
   //
   SIG_F0              = SIG_FUN | SIG_NIL,             ///< dito

   SIG_Z_F0            = SIG_Z   | SIG_F0,              ///< dito

   // monadic
   //
   SIG_F1_B            = SIG_MON | SIG_FUN,             ///< dito
   SIG_F1_X_B          = SIG_MON | SIG_FUN_X,           ///< dito
   SIG_LO_OP1_B        = SIG_MON | SIG_OP1,             ///< dito
   SIG_LO_OP1_X_B      = SIG_MON | SIG_OP1_X,           ///< dito
   SIG_LO_OP2_RO_B     = SIG_MON | SIG_OP2,             ///< dito

   SIG_Z_F1_B          = SIG_Z   | SIG_F1_B,            ///< dito
   SIG_Z_F1_X_B        = SIG_Z   | SIG_F1_X_B,          ///< dito
   SIG_Z_LO_OP1_B      = SIG_Z   | SIG_LO_OP1_B,        ///< dito
   SIG_Z_LO_OP1_X_B    = SIG_Z   | SIG_LO_OP1_X_B,      ///< dito
   SIG_Z_LO_OP2_RO_B   = SIG_Z   | SIG_LO_OP2_RO_B,     ///< dito

   // dyadic
   //
   SIG_A_F2_B          = SIG_DYA | SIG_FUN,             ///< dito
   SIG_A_F2_X_B        = SIG_DYA | SIG_FUN_X,           ///< dito
   SIG_A_LO_OP1_B      = SIG_DYA | SIG_OP1,             ///< dito
   SIG_A_LO_OP1_X_B    = SIG_DYA | SIG_OP1_X,           ///< dito
   SIG_A_LO_OP2_RO_B   = SIG_DYA | SIG_OP2,             ///< dito

   SIG_Z_A_F2_B        = SIG_Z   | SIG_A_F2_B,          ///< dito
   SIG_Z_A_F2_X_B      = SIG_Z   | SIG_A_F2_X_B,        ///< dito
   SIG_Z_A_LO_OP1_B    = SIG_Z   | SIG_A_LO_OP1_B,      ///< dito
   SIG_Z_A_LO_OP1_X_B  = SIG_Z   | SIG_A_LO_OP1_X_B,    ///< dito
   SIG_Z_A_LO_OP2_RO_B = SIG_Z   | SIG_A_LO_OP2_RO_B,   ///< dito
};
//-----------------------------------------------------------------------------
/// the mode that distinguishes different SI commands (SI, SIS, SINL, ]SI, ]SIS)
enum SI_mode
{
   SIM_none       = 0x00,   ///< )SI
   SIM_debug      = 0x01,   ///< ]SI  and ]SIS
   SIM_statements = 0x02,   ///< )SIS and ]SIS
   SIM_name_list  = 0x04,   ///< )SINL
   SIM_SI         = SIM_none,
   SIM_SIS        = SIM_statements,
   SIM_SINL       = SIM_name_list,
   SIM_SI_dbg     = SIM_debug,
   SIM_SIS_dbg    = SIM_debug | SIM_statements,
};
//-----------------------------------------------------------------------------
/// the number of cores/tasks to be used
enum CoreCount
{
  CCNT_UNKNOWN = -1,   ///< unknown core count
  CCNT_0       = 0,    ///<< no core
  CCNT_1       = 1,    ///< one core ...
};
//-----------------------------------------------------------------------------
/// the cores/tasks to be used
enum CoreNumber
{
  CNUM_INVALID = -1,  ///< invalid core
  CNUM_MASTER  = 0,   ///< the interpreter core
  CNUM_WORKER1 = 1,   ///< the first worker core ...
};
//-----------------------------------------------------------------------------
/// the CPUs reported by the OS
enum CPU_Number
{
   CPU_0 = 0   ///< the first (only) CPU
};
//-----------------------------------------------------------------------------
/// the number of CPUs available
enum CPU_count
{
   CPU_CNT_1 = 1   ///< one CPU
};
//-----------------------------------------------------------------------------
/// the state of an assignment
enum Assign_state
{
   ASS_none       = 0,   ///< no assignment (right of ←)
   ASS_arrow_seen = 1,   ///< ← seen but no variable yet
   ASS_var_seen   = 2,   ///< var and ← seen
};
//-----------------------------------------------------------------------------
/// the cause for something
enum Cause
{
   NO_CAUSE       = 0,
   CAUSE_SHUTDOWN = 1,
   CAUSE_ERASED   = 2,
};
//-----------------------------------------------------------------------------
/// the CDR data types
enum CDR_type
{
   CDR_BOOL1   = 0,   ///< 1 bit boolean
   CDR_INT32   = 1,   ///< 32 bit integer
   CDR_FLT64   = 2,   ///< 64 bit double
   CDR_CPLX128 = 3,   ///< 2*64 bit complex
   CDR_CHAR8   = 4,   ///< 8 bit char
   CDR_CHAR32  = 5,   ///< 32 bit char
   CDR_PROG64  = 6,   ///< 2*32 bit progression vector
   CDR_NEST32  = 7,   ///< 32 bit pointer to nested value
};
//-----------------------------------------------------------------------------
/// the result of a comparison cell_1.compare(cell_2)
enum Comp_result
{
  COMP_LT = -1,   ///< less than:     cell_1 < cell_2
  COMP_EQ =  0,   ///< equal:         cell_1 = cell_2
  COMP_GT =  1,   ///< greater than:  cell_1 > cell_2
};
//-----------------------------------------------------------------------------
/// the order of a comparison
enum Sort_order
{
   SORT_DESCENDING = 0,   ///< sort descending
   SORT_ASCENDING  = 1,   ///< sort asscending
};
//-----------------------------------------------------------------------------
/// events for a symbol
enum Symbol_Event
{
   SEV_CREATED  = 1,
   SEV_PUSHED   = 2,
   SEV_POPED    = 3,
   SEV_ASSIGNED = 4,
   SEV_ERASED   = 5,
};
//-----------------------------------------------------------------------------
/// Auxiliary processor numbers
enum AP_num
{
  NO_AP          = -1,     ///< invalid AP
  AP_NULL        = 0,      ///< invalid AP for structs using memset(0)
  AP_GENERAL     = 0,      ///< AP for generic offers
  AP_INTERPRETER = 1000,   ///< the AP for the APL interpreters
  AP_FIRST_USER  = 1001,   ///< the first AP for APL users
};
//-----------------------------------------------------------------------------
/// longest filename
enum {  APL_PATH_MAX = 4096 };
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////
// C. structs                                               //
//////////////////////////////////////////////////////////////

/// three AP numbers that uniquely identify a processor
struct AP_num3
{
   /// constructor: processor, parent, and grand-parent
   AP_num3(AP_num pro = NO_AP, AP_num par = AP_NULL, AP_num gra = AP_NULL)
   : proc(pro),
     parent(par),
     grand(gra)
   {}

   /// copy \b other to \b this
   void operator =(const AP_num3 & other)
      {
        proc   = other.proc;
        parent = other.parent;
        grand  = other.grand;
      }

   /// true if \b this AP_num3 is equal to \b other
   bool operator==(const AP_num3 & other) const
      { return proc   == other.proc   &&
               parent == other.parent &&
               grand  == other.grand; }

   AP_num proc;     ///< the processor
   AP_num parent;   ///< the parent of the processor
   AP_num grand;    ///< the parent of the parent
};
//-----------------------------------------------------------------------------
/// two Function_PCs indication a token range in a user defined function body
struct Function_PC2
{
   /// a PC range
   Function_PC2(Function_PC l, Function_PC h)
   : low(l),
     high(h)
   {}

   Function_PC low;    ///< low PC  (including)
   Function_PC high;   ///< high PC (including)
};
//-----------------------------------------------------------------------------
/** A single label. A label is a local variable (sym) with an integer function
    line (in which \b sym: was specified as the first 2 tokens in the line)
 */
///  A label in a defined function
struct labVal
{
   Symbol      * sym;    ///< The symbol for the label variable.
   Function_Line line;   ///< The line number of the label.
};
//-----------------------------------------------------------------------------
/// The end and the state of an abstract iterator along one axis
/// (to / weight / current)
struct _twc
{
   /// the end index (exclusive) for each axis. It can be > dimension
   /// length for over-Take from the start.
   ShapeItem to;

   /// weight of the dimension
   ShapeItem weight;

   /// the current index
   ShapeItem current;
};
//-----------------------------------------------------------------------------
/// The range and the state of an abstract iterator along one axis
/// (from / to / weight / current)
struct _ftwc : public _twc
{
   /// the start index (inclusive) for each axis. It can be < 0 for
   /// over-Take from the end.
   ShapeItem from;
};
//-----------------------------------------------------------------------------
/// the ravel (of an APL value) and a comparison lenght (= number of
/// consecutive cells to be compared)
struct ravel_comp_len
{
   /// the ravel (first Cell of some Value)
   const Cell * ravel;

   /// the number of consecutive Cells to be compared
   ShapeItem comp_len;
};
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////
// D. Namespace APL_types
//////////////////////////////////////////////////////////////

/*
  instead of putting everything into a namespace (which makes the code
  rather unreadable, we put only those types into a namesapce that were
  observed to conflict with other libraries.
 */

namespace APL_types
{
  typedef int32_t Depth;
}

#endif // __APL_TYPES_HH_DEFINED__
