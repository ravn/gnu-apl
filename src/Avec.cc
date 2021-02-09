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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Avec.hh"
#include "Command.hh"
#include "Output.hh"
#include "Parser.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"
#include "Value.hh"

using namespace std;

// See RFC 3629 for UTF-8

/// Various character attributes.
struct Character_definition
{
   CHT_Index     av_val;      ///< Atomic vector enum of the char.
   Unicode       unicode;     ///< Unicode of the char.
   const char *  char_name;   ///< Name of the char.
   int           def_line;    ///< Line where the char is defined.
   TokenTag      token_tag;   ///< Token tag for the char.
   CharacterFlag flags;       ///< Character class.
   int           av_pos;      ///< position in ⎕AV (== ⎕AF unicode)
};

//-----------------------------------------------------------------------------
#define char_def(n, _u, t, f, p) \
   { AV_ ## n, UNI_ ## n, # n, __LINE__, TOK_ ## t, FLG_ ## f, 0x ## p },
#define char_df1(_n, _u, _t, _f, _p)
const Character_definition character_table[MAX_AV] =
{
#include "Avec.def"
};
//-----------------------------------------------------------------------------
void
Avec::show_error_pos(int i, int line, bool cond, int def_line)
{
   if (!cond)
      {
        CERR << "Error index (line " << line << ", Avec.def line "
             << def_line << ") = " << i << " = " << HEX(i) << endl;
        Assert(0);
      }
}
//-----------------------------------------------------------------------------
#if 0

void
Avec::check_file(const char * filename)
{
const int fd = open(filename, O_RDONLY);
   if (fd == -1)   return;

   Log(LOG_startup)   CERR << "Checking " << filename << endl;

uint32_t datalen;
   {
     struct stat s;
     const int res = fstat(fd, &s);
     Assert(res == 0);
     datalen = s.st_size;
   }

const void * data = mmap(NULL, datalen, PROT_READ, MAP_PRIVATE, fd, 0);
   Assert(data != (void *)-1);

const UTF8_string utf = UTF8_string((const UTF8 *)data, datalen);
UCS_string ucs(utf);

   loop(i, ucs.size())
       {
         if (!is_known_char(ucs[i]))
            CERR << "APL char " << UNI(ucs[i]) << " is missing in AV ("
                 << i << ")" << endl;
       }

   close(fd);
}
#endif
//-----------------------------------------------------------------------------
void
Avec::init()
{
   check_av_table();
}
//-----------------------------------------------------------------------------
void
Avec::check_av_table()
{
   if (MAX_AV != 256)
      {
        CERR << "AV has " << MAX_AV << " entries (should be 256)" << endl;
        return;
      }

   Assert(sizeof(character_table) / sizeof(*character_table) == MAX_AV);

   // check that character_table.unicode is sorted by increasing UNICODE
   //
   for (int i = 1; i < MAX_AV; ++i)
       show_error_pos(i, __LINE__, character_table[i].def_line,
          character_table[i].unicode > character_table[i - 1].unicode);

   // check that ASCII chars map to themselves.
   //
   loop(i, 0x80)   show_error_pos(i, __LINE__, character_table[i].def_line,
                                  character_table[i].unicode == i);

   // display holes and duplicate AV positions in character_table
   {
     int holes = 0;
     loop(pos, MAX_AV)
        {
          int count = 0;
          loop(i, MAX_AV)
              {
                if (character_table[i].av_pos == pos)   ++count;
              }

          if (count == 0)
             {
               ++holes;
               CERR << "AV position " << HEX(pos) << " unused" << endl;
             }

          if (count > 1)
             CERR << "duplicate AV position " << HEX(pos) << endl;
        }

     if (holes)   CERR << holes << " unused positions in ⎕AV" << endl;
   }

   // check that find_char() works
   //
   loop(i, MAX_AV)   show_error_pos(i, __LINE__, character_table[i].def_line,
                                    i == find_char(character_table[i].unicode));

   Log(LOG_SHOW_AV)
      {
        for (int i = 0x80; i < MAX_AV; ++i)
            {
              CERR << character_table[i].unicode << " is AV[" << i << "] AV_"
                   << character_table[i].char_name << endl;
            }
      }

// check_file("../keyboard");
// check_file("../keyboard1");
}
//-----------------------------------------------------------------------------
Unicode
Avec::unicode(CHT_Index av)
{
   if (av < 0)         return UNI_AV_MAX;
   if (av >= MAX_AV)   return UNI_AV_MAX;
   return character_table[av].unicode;
}
//-----------------------------------------------------------------------------
uint32_t
Avec::get_av_pos(CHT_Index av)
{
   if (av < 0)         return UNI_AV_MAX;
   if (av >= MAX_AV)   return UNI_AV_MAX;
   return character_table[av].av_pos;
}
//-----------------------------------------------------------------------------
Token
Avec::uni_to_token(Unicode & uni, const char * loc)
{
CHT_Index idx = find_char(uni);
   if (idx != Invalid_CHT)   return Token(character_table[idx].token_tag, uni);

   // not found: try alternative characters.
   //
   idx = map_alternative_char(uni);
   if (idx != Invalid_CHT)
      {
        uni = character_table[idx].unicode;
        return Token(character_table[idx].token_tag,
                     character_table[idx].unicode);
      }

   Log(LOG_verbose_error)
      {
        CERR << endl << "Avec::uni_to_token() : Char " << UNI(uni)
             << " (" << uni << ") not found in ⎕AV! (called from "
             << loc << ")" << endl;

         BACKTRACE
      }
   return Token();
}
//-----------------------------------------------------------------------------
uint32_t Avec::find_av_pos(Unicode av)
{
const CHT_Index pos = find_char(av);

   if (pos < 0)   return MAX_AV;   // not found

   return character_table[pos].av_pos;
}
//-----------------------------------------------------------------------------
CHT_Index
Avec::find_char(Unicode av)
{
int l = 0;
int h = sizeof(character_table)/sizeof(*character_table) - 1;

   for (;;)
       {
         if (l > h)   return Invalid_CHT;   // not in table.

         const int m((h + l)/2);
         const Unicode um = character_table[m].unicode;
         if      (av < um)   h = m - 1;
         else if (av > um)   l = m + 1;
         else                return CHT_Index(m);
       }
}
//-----------------------------------------------------------------------------
CHT_Index
Avec::map_alternative_char(Unicode alt_av)
{
   // map characters that look similar to characters used in GNU APL
   // to the GNU APL character.
   //
   switch(int(alt_av))
      {
        case 0x005E: return AV_AND;              //  map ^ to ∧
        case 0x007C: return AV_DIVIDES;          //  map | to ∣
        case 0x007E: return AV_TILDE_OPERATOR;   //  map ~ to ∼
        case 0x03B1: return AV_ALPHA;            //  map α to ⍺
        case 0x03B5: return AV_ELEMENT;          //  map ε to ∈
        case 0x03B9: return AV_IOTA;             //  map ι to ⍳
        case 0x03C1: return AV_RHO;              //  map ρ to ⍴
        case 0x03C9: return AV_OMEGA;            //  map ω to ⍵
        case 0x2018: return AV_SINGLE_QUOTE;     //  map ‘ to '
        case 0x2019: return AV_SINGLE_QUOTE;     //  map ’ to '
        case 0x220A: return AV_ELEMENT;          //  map ∊ to ∈
        case 0x2212: return AV_MINUS;            //  map − to -
        case 0x22BC: return AV_NAND;             //  map ⊼ to ⍲
        case 0x22BD: return AV_NOR;              //  map ⊽ to ⍱
        case 0x22C4: return AV_DIAMOND;          //  map ⋄ to ◊
        case 0x2377: return AV_EPSILON_UBAR;     //  map ⍷ to ⋸
        case 0x25AF: return AV_Quad_Quad;        //  map ▯ to ⎕
        case 0x25E6: return AV_RING_OPERATOR;    //  map ◦ to ∘
        case 0x2662: return AV_DIAMOND;          //  map ♢ to ◊
        case 0x26AA: return AV_CIRCLE;           //  map ⚪ to ○
        case 0x2A7D: return AV_LESS_OR_EQUAL;    //  map ⩽ to ≤
        case 0x2A7E: return AV_MORE_OR_EQUAL;    //  map ⩾ to ≥
        case 0x2B25: return AV_DIAMOND;          //  map ⬥ to ◊
        case 0x2B26: return AV_DIAMOND;          //  map ⬦ to ◊
        case 0x2B27: return AV_DIAMOND;          //  map ⬧ to ◊
        default:     break;
      }

   return Invalid_CHT;
}
//-----------------------------------------------------------------------------
bool
Avec::is_known_char(Unicode av)
{
   switch(av)
      {
#define char_def(_n, u, _t, _f, _p)  case u: 
#define char_df1(_n, u, _t, _f, _p)  case u: 
#include "Avec.def"
          return true;

        default: break;
      }

   return false;   // not found
}
//-----------------------------------------------------------------------------
int
Avec::digit_value(Unicode uni, bool hex)
{
   if (uni <  UNI_0)   return -1;
   if (uni <= UNI_9)   return uni - '0';
   if (hex)
      {
        if (uni <  UNI_A)   return -1;
        if (uni <= UNI_F)   return 10 + uni - 'A';
        if (uni <  UNI_a)   return -1;
        if (uni <= UNI_f)   return 10 + uni - 'a';
      }
   return -1;
}
//-----------------------------------------------------------------------------
bool
Avec::need_UCS(Unicode uni)
{
   if (is_control(uni))   return true;                      // ASCII control
   if (uni >= 0 && uni < UNI_DELETE)  return false;   // printable ASCII

const CHT_Index idx = find_char(uni);
   if (idx == Invalid_CHT)   return true;           // char not in GNU APL's ⎕AV
   if (unicode_to_cp(uni) == 0xB0)   return true;   // not in IBM's ⎕AV

   return false;
}
//-----------------------------------------------------------------------------
bool
Avec::is_symbol_char(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_SYMBOL) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::is_first_symbol_char(Unicode av)
{
   return is_symbol_char(av) && ! is_digit(av);
}
//-----------------------------------------------------------------------------
bool
Avec::no_space_after(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NO_SPACE_AFTER) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::no_space_before(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NO_SPACE_BEFORE) != 0;
}
//-----------------------------------------------------------------------------
Unicode
Avec::subscript(uint32_t i)
{
   switch(i)
      {
        case 0: return Unicode(0x2080);
        case 1: return Unicode(0x2081);
        case 2: return Unicode(0x2082);
        case 3: return Unicode(0x2083);
        case 4: return Unicode(0x2084);
        case 5: return Unicode(0x2085);
        case 6: return Unicode(0x2086);
        case 7: return Unicode(0x2087);
        case 8: return Unicode(0x2088);
        case 9: return Unicode(0x2089);
      }

   return Unicode(0x2093);
}
//-----------------------------------------------------------------------------
Unicode
Avec::superscript(uint32_t i)
{
   switch(i)
      {
        case 0: return Unicode(0x2070);
        case 1: return Unicode(0x00B9);
        case 2: return Unicode(0x00B2);
        case 3: return Unicode(0x00B3);
        case 4: return Unicode(0x2074);
        case 5: return Unicode(0x2075);
        case 6: return Unicode(0x2076);
        case 7: return Unicode(0x2077);
        case 8: return Unicode(0x2078);
        case 9: return Unicode(0x2079);
      }

   return Unicode(0x207A);
}
//-----------------------------------------------------------------------------
/* the IBM APL2 character set shown in lrm figure 68 on page 470

   The table is indexed with an 8-bit position in IBM's ⎕AV and returns
   the Unicode for that position. In addition CTRL-K is mapped to ⍬ for
   compatibility with Dyalog-APL
 */
static const int ibm_av[] =
{
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
  0x0008, 0x0009, 0x000A, 0x236C, 0x000C, 0x000D, 0x000E, 0x000F, 
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
  0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
  0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
  0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
  0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
  0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
  0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
  0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F, 
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x2395, 0x235E, 0x2339, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x22A4, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x22A5, 0x2376, 0x2336,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x2308, 0x00AC, 0x00BD, 0x222A, 0x00A1, 0x2355, 0x234E,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x235F, 0x2206, 0x2207,
  0x2192, 0x2563, 0x2551, 0x2557, 0x255D, 0x2190, 0x230A, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x2191, 0x2193,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2261,
  0x2378, 0x22F8, 0x2235, 0x2337, 0x2342, 0x233B, 0x22A2, 0x22A3,
  0x25CA, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
  0x237A, 0x2379, 0x2282, 0x2283, 0x235D, 0x2372, 0x2374, 0x2371,
  0x233D, 0x2296, 0x25CB, 0x2228, 0x2373, 0x2349, 0x2208, 0x2229,
  0x233F, 0x2340, 0x2265, 0x2264, 0x2260, 0x00D7, 0x00F7, 0x2359,
  0x2218, 0x2375, 0x236B, 0x234B, 0x2352, 0x00AF, 0x00A8, 0x00A0
};

const Unicode *
Avec::IBM_quad_AV()
{
   return reinterpret_cast<const Unicode *>(ibm_av);
}
//-----------------------------------------------------------------------------
Avec::Unicode_to_IBM_codepoint Avec::inverse_ibm_av[256] =
{
  { 0x0000,   0 }, { 0x0001,   1 }, { 0x0002,   2 }, { 0x0003,   3 },
  { 0x0004,   4 }, { 0x0005,   5 }, { 0x0006,   6 }, { 0x0007,   7 },
  { 0x0008,   8 }, { 0x0009,   9 }, { 0x000A,  10 }, { 0x000C,  12 },
  { 0x000D,  13 }, { 0x000E,  14 }, { 0x000F,  15 }, { 0x0010,  16 },
  { 0x0011,  17 }, { 0x0012,  18 }, { 0x0013,  19 }, { 0x0014,  20 },
  { 0x0015,  21 }, { 0x0016,  22 }, { 0x0017,  23 }, { 0x0018,  24 },
  { 0x0019,  25 }, { 0x001A,  26 }, { 0x001B,  27 }, { 0x001C,  28 },
  { 0x001D,  29 }, { 0x001E,  30 }, { 0x001F,  31 }, { 0x0020,  32 },
  { 0x0021,  33 }, { 0x0022,  34 }, { 0x0023,  35 }, { 0x0024,  36 },
  { 0x0025,  37 }, { 0x0026,  38 }, { 0x0027,  39 }, { 0x0028,  40 },
  { 0x0029,  41 }, { 0x002A,  42 }, { 0x002B,  43 }, { 0x002C,  44 },
  { 0x002D,  45 }, { 0x002E,  46 }, { 0x002F,  47 }, { 0x0030,  48 },
  { 0x0031,  49 }, { 0x0032,  50 }, { 0x0033,  51 }, { 0x0034,  52 },
  { 0x0035,  53 }, { 0x0036,  54 }, { 0x0037,  55 }, { 0x0038,  56 },
  { 0x0039,  57 }, { 0x003A,  58 }, { 0x003B,  59 }, { 0x003C,  60 },
  { 0x003D,  61 }, { 0x003E,  62 }, { 0x003F,  63 }, { 0x0040,  64 },
  { 0x0041,  65 }, { 0x0042,  66 }, { 0x0043,  67 }, { 0x0044,  68 },
  { 0x0045,  69 }, { 0x0046,  70 }, { 0x0047,  71 }, { 0x0048,  72 },
  { 0x0049,  73 }, { 0x004A,  74 }, { 0x004B,  75 }, { 0x004C,  76 },
  { 0x004D,  77 }, { 0x004E,  78 }, { 0x004F,  79 }, { 0x0050,  80 },
  { 0x0051,  81 }, { 0x0052,  82 }, { 0x0053,  83 }, { 0x0054,  84 },
  { 0x0055,  85 }, { 0x0056,  86 }, { 0x0057,  87 }, { 0x0058,  88 },
  { 0x0059,  89 }, { 0x005A,  90 }, { 0x005B,  91 }, { 0x005C,  92 },
  { 0x005D,  93 }, { 0x005E,  94 }, { 0x005F,  95 }, { 0x0060,  96 },
  { 0x0061,  97 }, { 0x0062,  98 }, { 0x0063,  99 }, { 0x0064, 100 },
  { 0x0065, 101 }, { 0x0066, 102 }, { 0x0067, 103 }, { 0x0068, 104 },
  { 0x0069, 105 }, { 0x006A, 106 }, { 0x006B, 107 }, { 0x006C, 108 },
  { 0x006D, 109 }, { 0x006E, 110 }, { 0x006F, 111 }, { 0x0070, 112 },
  { 0x0071, 113 }, { 0x0072, 114 }, { 0x0073, 115 }, { 0x0074, 116 },
  { 0x0075, 117 }, { 0x0076, 118 }, { 0x0077, 119 }, { 0x0078, 120 },
  { 0x0079, 121 }, { 0x007A, 122 }, { 0x007B, 123 }, { 0x007C, 124 },
  { 0x007D, 125 }, { 0x007E, 126 }, { 0x007F, 127 }, { 0x00A0, 255 },
  { 0x00A1, 173 }, { 0x00A3, 156 }, { 0x00A6, 221 }, { 0x00A8, 254 },
  { 0x00AA, 166 }, { 0x00AC, 170 }, { 0x00AF, 253 }, { 0x00BA, 167 },
  { 0x00BD, 171 }, { 0x00BF, 168 }, { 0x00C4, 142 }, { 0x00C5, 143 },
  { 0x00C7, 128 }, { 0x00CC, 222 }, { 0x00D1, 165 }, { 0x00D6, 153 },
  { 0x00D7, 245 }, { 0x00DC, 154 }, { 0x00E0, 133 }, { 0x00E1, 160 },
  { 0x00E2, 131 }, { 0x00E4, 132 }, { 0x00E5, 134 }, { 0x00E7, 135 },
  { 0x00E8, 138 }, { 0x00E9, 130 }, { 0x00EA, 136 }, { 0x00EB, 137 },
  { 0x00EC, 141 }, { 0x00ED, 161 }, { 0x00EE, 140 }, { 0x00EF, 139 },
  { 0x00F1, 164 }, { 0x00F2, 149 }, { 0x00F3, 162 }, { 0x00F4, 147 },
  { 0x00F6, 148 }, { 0x00F7, 246 }, { 0x00F8, 155 }, { 0x00F9, 151 },
  { 0x00FA, 163 }, { 0x00FB, 150 }, { 0x00FC, 129 }, { 0x2190, 189 },
  { 0x2191, 198 }, { 0x2192, 184 }, { 0x2193, 199 }, { 0x2206, 182 },
  { 0x2207, 183 }, { 0x2208, 238 }, { 0x2218, 248 }, { 0x2228, 235 },
  { 0x2229, 239 }, { 0x222A, 172 }, { 0x2235, 210 }, { 0x2260, 244 },
  { 0x2261, 207 }, { 0x2264, 243 }, { 0x2265, 242 }, { 0x2282, 226 },
  { 0x2283, 227 }, { 0x2296, 233 }, { 0x22A2, 214 }, { 0x22A3, 215 },
  { 0x22A4, 152 }, { 0x22A5, 157 }, { 0x22F8, 209 }, { 0x2308, 169 },
  { 0x230A, 190 }, { 0x2336, 159 }, { 0x2337, 211 }, { 0x2339, 146 },
  { 0x233B, 213 }, { 0x233D, 232 }, { 0x233F, 240 }, { 0x2340, 241 },
  { 0x2342, 212 }, { 0x2349, 237 }, { 0x234B, 251 }, { 0x234E, 175 },
  { 0x2352, 252 }, { 0x2355, 174 }, { 0x2359, 247 }, { 0x235D, 228 },
  { 0x235E, 145 }, { 0x235F, 181 }, { 0x236B, 250 }, { 0x236C,  11 },
  { 0x2371, 231 }, { 0x2372, 229 }, { 0x2373, 236 }, { 0x2374, 230 },
  { 0x2375, 249 }, { 0x2376, 158 }, { 0x2378, 208 }, { 0x2379, 225 },
  { 0x237A, 224 }, { 0x2395, 144 }, { 0x2500, 196 }, { 0x2502, 179 },
  { 0x250C, 218 }, { 0x2510, 191 }, { 0x2514, 192 }, { 0x2518, 217 },
  { 0x251C, 195 }, { 0x2524, 180 }, { 0x252C, 194 }, { 0x2534, 193 },
  { 0x253C, 197 }, { 0x2550, 205 }, { 0x2551, 186 }, { 0x2554, 201 },
  { 0x2557, 187 }, { 0x255A, 200 }, { 0x255D, 188 }, { 0x2560, 204 },
  { 0x2563, 185 }, { 0x2566, 203 }, { 0x2569, 202 }, { 0x256C, 206 },
  { 0x2580, 223 }, { 0x2584, 220 }, { 0x2588, 219 }, { 0x2591, 176 },
  { 0x2592, 177 }, { 0x2593, 178 }, { 0x25CA, 216 }, { 0x25CB, 234 }
};

void
Avec::print_inverse_IBM_quad_AV()
{
   // a helper function that sorts ibm_av by Unicode and prints it on CERR
   //
   // To use it change #if 0 to #if 1 in Command.cc:1707
   // recompile and start apl, and then )IN <file> or so.
   //
   loop(c, 256)
      {
        inverse_ibm_av[c].uni = -1;
        inverse_ibm_av[c].cp  = -1;
      }

int current_max = -1;
Unicode_to_IBM_codepoint * map = inverse_ibm_av;
   loop(c, 256)
      {
        // find next Unicode after current_max
        //
        int next_idx = -1;
        loop(n, 256)
           {
             const int uni = ibm_av[n];
             if (uni <= current_max)   continue;   // already done
             if (next_idx == -1)               next_idx = n;
             else if (uni < ibm_av[next_idx])   next_idx = n;
           }
        current_max = map->uni = ibm_av[next_idx];
        map->cp  = next_idx;
        ++map;
      }

   loop(row, 64)
      {
        CERR << " ";
        loop(col, 4)
           {
             const int pos = col + 4*row;
             CERR << " { " << HEX4(inverse_ibm_av[pos].uni) << ", "
                  << setw(3) << inverse_ibm_av[pos].cp << " }";
             if (pos < 255)   CERR << ",";
           }
        CERR << endl;
      }
}
//-----------------------------------------------------------------------------
/// compare the unicodes of two chars in the IBM ⎕AV
int
Avec::compare_uni(const void * key, const void * entry)
{
   return *reinterpret_cast<const Unicode *>(key) -
           reinterpret_cast<const Unicode_to_IBM_codepoint *>(entry)->uni;
}
//-----------------------------------------------------------------------------
unsigned char
Avec::unicode_to_cp(Unicode uni)
{
   if (uni <= 0x80)                return uni;
   if (uni == UNI_STAR_OPERATOR)   return '*';   // ⋆ → *
   if (uni == UNI_AND)             return '^';   // ∧ → ^
   if (uni == UNI_TILDE_OPERATOR)  return 126;   // ∼ → ~

   // search in uni_to_cp_map table
   //
const void * where = bsearch(&uni, inverse_ibm_av, 256,
                             sizeof(Unicode_to_IBM_codepoint), compare_uni);

   if (where == 0)
      {
        // the workspace being )OUT'ed can contain characters that are not
        // in IBM's APL character set. We replace such characters by 0xB0
        //
        return 0xB0;
      }

   Assert(where);
   return reinterpret_cast<const Unicode_to_IBM_codepoint *>(where)->cp;
}
//-----------------------------------------------------------------------------

