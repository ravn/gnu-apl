/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2018-2019  Dr. Jürgen Sauermann

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

#include <iostream>
#include <iomanip>
#include <signal.h>

#include <vector>

#include "../config.h"
#if ! defined(HAVE_LIBX11)
# define WHY_NOT "libX11.so"
#elif ! defined(HAVE_LIBXCB)
# define WHY_NOT "libxcb.so"
#elif ! defined(HAVE_LIBX11_XCB)
# define WHY_NOT "libX11-xcb.so"
#elif ! defined(HAVE_XCB_XCB_H)
# define WHY_NOT "xcb/xcb.h"
#endif

#include "Avec.hh"
#include "Common.hh"
#include "Quad_PLOT.hh"

using namespace std;

#define I1616(x, y) int16_t(x), int16_t(y)
# define ITEMS(x) sizeof(x)/sizeof(*x), x

Quad_PLOT  Quad_PLOT::_fun;
Quad_PLOT * Quad_PLOT::fun = &Quad_PLOT::_fun;

sem_t __plot_threads_sema;
sem_t * Quad_PLOT::plot_threads_sema = &__plot_threads_sema;

/**
   \b Quad_PLOT::plot_threads contains one thread per (open) plot window.
   The thread handles the X main loop for the window. The thread also monitors
   its presence in plot_threads so that removing a thread from \b plot_threads
   causes the thread to close its plot window and then to exit.
 **/
vector<pthread_t> Quad_PLOT::plot_threads;

#if defined(WHY_NOT)
//-----------------------------------------------------------------------------
Quad_PLOT::Quad_PLOT()
  : QuadFunction(TOK_Quad_PLOT),
    verbosity(0)
{
}
//-----------------------------------------------------------------------------
Quad_PLOT::~Quad_PLOT()
{
}
//-----------------------------------------------------------------------------
Token
Quad_PLOT::eval_B(Value_P B)
{
    MORE_ERROR() <<
"⎕PLOT is not available because one or more of its build prerequisites (in\n"
"particular " WHY_NOT ") was missing, or because it was explicitly\n"
" disabled in ./configure.";

   SYNTAX_ERROR;
   return Token();
}
//-----------------------------------------------------------------------------
Token
Quad_PLOT::eval_AB(Value_P A, Value_P B)
{
   return eval_B(B);
}
//-----------------------------------------------------------------------------

#else   // not defined(WHY_NOT)

# include <xcb/xcb.h>
# include <xcb/xproto.h>

# include "ComplexCell.hh"
# include "FloatCell.hh"
# include "Workspace.hh"

// UTF8 support for XCB windows needs additional libraries that may
// not be present. You can disable that with: XCB_WINDOWS_WITH_UTF8_CAPTIONS 0
// or maybe CXXFLAGS=-D XCB_WINDOWS_WITH_UTF8_CAPTIONS=0 ./configure...
//
# ifndef XCB_WINDOWS_WITH_UTF8_CAPTIONS
#  define XCB_WINDOWS_WITH_UTF8_CAPTIONS 0
# endif

# if XCB_WINDOWS_WITH_UTF8_CAPTIONS
#  include <X11/Xutil.h>
#  include <X11/Xlib.h>
#  include <X11/Xlib-xcb.h>
# endif

# include <stdio.h>
# include <math.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <xcb/xcb.h>
# include <xcb/xproto.h>

# include <iostream>
# include <iomanip>

enum
{
   SHOW_EVENTS = 1,   ///< show X events
   SHOW_DATA   = 2,   ///< show APL data
   SHOW_DRAW   = 4,   ///< show draw details
};
using namespace std;

typedef uint16_t  Pixel_X;
typedef uint16_t  Pixel_Y;
typedef uint16_t  Pixel_Z;   // offset in the Z direction
struct Pixel_XY
{
   Pixel_XY(Pixel_X px, Pixel_Y py)
   : x(px),
     y(py)
   {}

   /// the square of the distance between \b this and other
   const double distance2(const Pixel_XY other) const
      { const double dx = x - other.x;
        const double dy = y - other.y;
        return dx*dx + dy*dy;
      }


   /// the distance between \b this and other
   const double distance(const Pixel_XY other) const
      { return sqrt(distance2(other)); }

   Pixel_X x; 
   Pixel_Y y;
};

typedef uint32_t Color;
typedef std::string String;

//=============================================================================
/// data for one plot line
class Plot_data_row
{
public:
   /// constructor
   Plot_data_row(const double * pX, const double * pY, const double * pZ,
                 uint32_t idx, uint32_t len)
   : X(pX), Y(pY), Z(pZ), row_num(idx), N(len)
   {
     min_X = max_X = get_X(0);
     min_Y = max_Y = get_Y(0);
     min_Z = max_Z = get_Z(0);

     for (unsigned int n = 1; n < N; ++n)
         {
            const double Xn = get_X(n);
            if (min_X > Xn)   min_X = Xn;
            if (max_X < Xn)   max_X = Xn;

            const double Yn = get_Y(n);
            if (min_Y > Yn)   min_Y = Yn;
            if (max_Y < Yn)   max_Y = Yn;

            const double Zn = get_Z(n);
            if (min_Z > Zn)   min_Z = Zn;
            if (max_Z < Zn)   max_Z = Zn;
         }
   }

   /// destructor
   ~Plot_data_row()
   {
     // all rows belong to the same double *, so we only delete row 0
     //
     if (row_num == 0)   delete[] X;
   }

   /// return the number of data points
   uint32_t get_N() const
      { return N; }

   /// return the n-th X coordinate
   double get_X(uint32_t n) const
      { return X ? X[n] : n + 1; }

   /// return the n-th Y coordinate
   double get_Y(uint32_t n) const
      { return Y[n]; }

   /// return the n-th Z coordinate
   double get_Z(uint32_t n) const
      { return Z ? Z[n] : row_num + 1; }

   /// return the smallest value in X
   double get_min_X() const
      { return min_X; }

   /// return the largest value in X
   double get_max_X() const
      { return max_X; }

   /// return the smallest value in Y
   double get_min_Y() const
      { return min_Y; }

   /// return the largest value in Y
   double get_max_Y() const
      { return max_Y; }

   /// return the smallest value in Z
   double get_min_Z() const
      { return min_Z; }

   /// return the largest value in Y
   double get_max_Z() const
      { return max_Z; }

protected:
   /// the X coordinates (optional, 0 if none)
   const double * X;

   /// the Y coordinates
   const double * Y;

   /// the Z coordinates (optional, 0 if none)
   const double * Z;

   /// the row number
   const uint32_t row_num;

   /// the row length
   const uint32_t N;

   /// the smallest value in X
   double min_X;

   /// the largest value in X
   double max_X;

   /// the smallest value in Y
   double min_Y;

   /// the largest value in Y
   double max_Y;

   /// the smallest value in Z
   double min_Z;

   /// the largest value in Z
   double max_Z;
};
//=============================================================================
/// data for all plot lines

struct level_color
{
   int      level;   ///< level 0-100
   uint32_t rgb;     ///< color as RGB integer
};

class Plot_data
{
public:
   /// constructor
   Plot_data(uint32_t rows)
   : surface(false),
     row_count(rows),
     idx(0)
      {
        data_rows = new const Plot_data_row *[row_count];
      }

   /// destructor
   ~Plot_data()
   {
     loop(r, idx)   delete data_rows[r];
     delete data_rows;
   }

   /// return the number of plot lines
   uint32_t get_row_count() const
      { return row_count; }

   /// return the X and Y values of the data matrix row/col
   void get_XY(double & X, double & Y, uint32_t row, uint32_t col) const
        {
          Assert(row < idx);
          X = data_rows[row]->get_X(col);
          Y = data_rows[row]->get_Y(col);
        }

   /// return the smallest X value in the data matrix
   double get_min_X() const
      { return min_X; }

   /// return the largest X value in the data matrix
   double get_max_X() const
      { return max_X; }

   /// return the smallest Y value in the data matrix
   double get_min_Y() const
      { return min_Y; }

   /// return the largest Y value in the data matrix
   double get_max_Y() const
      { return max_Y; }

   /// return the smallest Z value in the data matrix
   double get_min_Z() const
      { return min_Z; }

   /// return the largest Z value in the data matrix
   double get_max_Z() const
      { return max_Z; }

   /// return the X coordinate at row and col
   double get_X(uint32_t row, uint32_t col) const
      { return data_rows[row]->get_X(col); }

   /// return the Y coordinate at row and col
   double get_Y(uint32_t row, uint32_t col) const
      { return data_rows[row]->get_Y(col); }

   /// return the Z coordinate at row and col
   double get_Z(uint32_t row, uint32_t col) const
      { return data_rows[row]->get_Z(col); }

   /// return true for surface plots
   bool is_surface_plot() const       { return surface; }

   /// add a row to the data matrix
   void add_row(const Plot_data_row * row)
      {
        Assert(idx < row_count);
        data_rows[idx++] = row;
        if (idx == 1)   // first row
           {
             min_X = row->get_min_X();
             max_X = row->get_max_X();
             min_Y = row->get_min_Y();
             max_Y = row->get_max_Y();
             min_Z = row->get_min_Y();
             max_Z = row->get_max_Y();
           }
        else
           {
             if (min_X > row->get_min_X())   min_X = row->get_min_X();
             if (max_X < row->get_max_X())   max_X = row->get_max_X();
             if (min_Y > row->get_min_Y())   min_Y = row->get_min_Y();
             if (max_Y < row->get_max_Y())   max_Y = row->get_max_Y();
             if (min_Z > row->get_min_Z())   min_Z = row->get_min_Z();
             if (max_Z < row->get_max_Z())   max_Z = row->get_max_Z();
           }
      }

   /// convert integer to a string
   static const char * uint32_t_to_str(uint32_t val)
      {
        static char ret[40];
        snprintf(ret, sizeof(ret), "%u", val);
        ret[sizeof(ret) - 1] = 0;
        return ret;
      }

   static const char * Pixel_X_to_str(Pixel_X val)
      {
        static char ret[40];
        snprintf(ret, sizeof(ret), "%u pixel", val);
        ret[sizeof(ret) - 1] = 0;
        return ret;
      }

   static const char * Pixel_Y_to_str(Pixel_Y val)
      {
        static char ret[40];
        snprintf(ret, sizeof(ret), "%u pixel", val);
        ret[sizeof(ret) - 1] = 0;
        return ret;
      }

   /// convert a double to a string
   static const char * double_to_str(double val)
      {
        static char ret[40];
        snprintf(ret, sizeof(ret), "%.1lf", val);
        ret[sizeof(ret) - 1] = 0;
        return ret;
      }

   /// convert a Color to a string
   static const char * Color_to_str(Color val)
      {
        static char ret[40];
        snprintf(ret, sizeof(ret), "#%2.2X%2.2X%2.2X",
                 val >> 16 & 0xFF, val >>  8 & 0xFF, val       & 0xFF);
         ret[sizeof(ret) - 1] = 0;
         return ret;
      }

   /// convert a String to a string
   static String String_to_str(String val)
      {
        return val;
      }

   /// return the idx'th row
   const Plot_data_row & operator[](ShapeItem idx) const
      { Assert(idx < row_count);   return *data_rows[idx]; }

   /// convert a string to a Pixel
   static Pixel_X Pixel_X_from_str(const char * str, const char * & error)
      { error = 0;   return strtol(str, 0, 10); }

   /// convert a string to a Pixel
   static Pixel_Y Pixel_Y_from_str(const char * str, const char * & error)
      { error = 0;   return strtol(str, 0, 10); }

   /// convert a string to a double
   static double double_from_str(const char * str, const char * & error)
      { error = 0;   return strtod(str, 0); }

   /// convert a string like "#RGB" or "#RRGGB" to a color
   static Color Color_from_str(const char * str, const char * & error);

   /// convert a string to a uint32_t
   static uint32_t uint32_t_from_str(const char * str, const char * & error)
      { error = 0;   return strtol(str, 0, 10); }

   /// convert a string to a String
   static String String_from_str(const char * str, const char * & error)
      { error = 0;   return str; }

   /// true for surface plots
   bool surface;

protected:
   /// the rows of the data matrix (0-terminated)
   const Plot_data_row ** data_rows;

   /// the number of rows in the data matrix
   const uint32_t row_count;

   /// the current number of columns in the data matrix
   uint32_t idx;

   /// the smallest value in X
   double min_X;

   /// the largest value in X
   double max_X;

   /// the smallest value in Y
   double min_Y;

   /// the largest value in Y
   double max_Y;

   /// the smallest value in Z
   double min_Z;

   /// the largest value in Z
   double max_Z;
};
//-----------------------------------------------------------------------------
Color
Plot_data::Color_from_str(const char * str, const char * & error)
{
   error = 0;   // assume no error

uint32_t r, g, b;
   if (3 == sscanf(str, " %u %u %u", &r, &b, &g))
      return (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF);

   if (const char * h = strchr(str, '#'))
      {
        if (strlen(h) == 4)   // #RGB
           {
             const int v = strtol(str + 1, 0, 16);
             return (0x11*(v >> 8 & 0x0F)) << 16
                  | (0x11*(v >> 4 & 0x0F)) << 8
                  | (0x11*(v      & 0x0F)  << 0);
           }
        else if (strlen(h) == 7)   // #RRGGBB
           {
             return strtol(str + 1, 0, 16);
           }
      }

   error = "Bad color format";
   return 0;
}
//=============================================================================
/// properties of a single plot line
class Plot_line_properties
{
public:
   /** constructor. Set all property values to the defaults that are
       defined in Quad_PLOT.def, and then override those property that
       have different defaults for different lines.
   **/
   Plot_line_properties(int lnum) :
# define ldef(_ty,  na,  val, _descr) na(val),
# include "Quad_PLOT.def"
   line_number(lnum)
   {
     snprintf(legend_name_buffer, sizeof(legend_name_buffer),
              "Line-%d", int(lnum + Workspace::get_IO()));
     legend_name = legend_name_buffer;
     switch(lnum)
        {
           default: line_color = point_color = 0x000000;   break;
           case 0:  line_color = point_color = 0x00E000;   break;
           case 1:  line_color = point_color = 0xFF0000;   break;
           case 2:  line_color = point_color = 0x0000FF;   break;
           case 3:  line_color = point_color = 0xFF00FF;   break;
           case 4:  line_color = point_color = 0x00FFFF;   break;
           case 5:  line_color = point_color = 0xFFFF00;   break;
        }
   }

   // get_XXX() and set_XXX functions
# define ldef(ty,  na,  _val, _descr)      \
   /** return the value of na **/         \
   ty get_ ## na() const   { return na; } \
   /** set the  value of na **/           \
   void set_ ## na(ty val)   { na = val; }
# include "Quad_PLOT.def"

   /// print the line properties
   int print(ostream & out) const;

# define ldef(ty,  na,  _val, descr) /** descr **/ ty na;
# include "Quad_PLOT.def"

  /// plot line number
  const int line_number;   // starting a 0 regardless of ⎕IO

  /// a buffer for creating a legend name from a macro
  char legend_name_buffer[50];
};
//-----------------------------------------------------------------------------
int
Plot_line_properties::print(ostream & out) const
{
char cc[40];
# define ldef(ty,  na,  _val, _descr)                   \
   snprintf(cc, sizeof(cc), #na "-%d:  ",              \
            int(line_number + Workspace::get_IO()));   \
   CERR << setw(20) << cc << Plot_data::ty ## _to_str(na) << endl;
# include "Quad_PLOT.def"

   return 0;
}
//=============================================================================
/// properties of the entire plot window
class Plot_window_properties
{
public:
   /// the kind of range
   enum Plot_Range_type
      {
        NO_RANGE           = 0,   ///< no plot range provided
        PLOT_RANGE_MIN     = 1,   ///< min value provided
        PLOT_RANGE_MAX     = 2,   ///< max value provided
        PLOT_RANGE_MIN_MAX = PLOT_RANGE_MIN | PLOT_RANGE_MAX ///< min. and max.
      };

   /// constructor
   Plot_window_properties(const Plot_data * data, int verbosity);

   /// destructor
   ~Plot_window_properties()
      {
        delete &plot_data;
      }

   /// update derived properties after changing primary ones, return \b true
   /// on error
   bool update(int verbosity);

   /// handle window resize event
   void set_window_size(Pixel_X width, Pixel_Y height);

   // get_XXX() and set_XXX functions
# define gdef(ty,  na,  _val, _descr)                                     \
  /** return the value of na **/                                         \
  ty get_ ## na() const   { return na; }                                 \
  /** set the value of na **/                                            \
  void set_ ## na(ty val)                                                \
     { na = val;                                                         \
       if (!strcmp(#na, "rangeX_min"))                                   \
          rangeX_type = Plot_Range_type(rangeX_type | PLOT_RANGE_MIN);   \
       else if (!strcmp(#na, "rangeX_max"))                              \
          rangeX_type = Plot_Range_type(rangeX_type | PLOT_RANGE_MAX);   \
       else if (!strcmp(#na, "rangeY_min"))                              \
          rangeY_type = Plot_Range_type(rangeY_type | PLOT_RANGE_MIN);   \
       else if (!strcmp(#na, "rangeY_max"))                              \
          rangeY_type = Plot_Range_type(rangeY_type | PLOT_RANGE_MAX);   \
       else if (!strcmp(#na, "rangeZ_min"))                              \
          rangeZ_type = Plot_Range_type(rangeZ_type | PLOT_RANGE_MIN);   \
       else if (!strcmp(#na, "rangeY_max"))                              \
          rangeZ_type = Plot_Range_type(rangeZ_type | PLOT_RANGE_MAX);   \
     }
# include "Quad_PLOT.def"

   /// return true iff a rangeX_min property was specified
   bool rangeX_min_valid() const
      { return !!(rangeX_type & PLOT_RANGE_MIN); }

   /// return true iff a rangeX_max property was specified
   bool rangeX_max_valid() const
      { return !!(rangeX_type & PLOT_RANGE_MAX); }

   /// return true iff a rangeY_min property was specified
   bool rangeY_min_valid() const
      { return !!(rangeY_type & PLOT_RANGE_MIN); }

   /// return true iff a rangeY_max property was specified
   bool rangeY_max_valid() const
      { return !!(rangeY_type & PLOT_RANGE_MAX); }

   /// return true iff a rangeZ_min property was specified
   bool rangeZ_min_valid() const
      { return !!(rangeZ_type & PLOT_RANGE_MIN); }

   /// return true iff a rangeZ_max property was specified
   bool rangeZ_max_valid() const
      { return !!(rangeZ_type & PLOT_RANGE_MAX); }

   /// an array of plot line properties
   Plot_line_properties * const * const get_line_properties() const
      { return line_properties; }

   /// convert \n val_X to the X coordinate of the pixel in the plot area
   Pixel_X valX2pixel(double val_X) const
      { return pa_border_L + val_X * scale_X; }

   /// convert \n val_Y to the Y coordinate of the pixel in the plot area
   Pixel_Y valY2pixel(double val_Y) const
      { return pa_border_T + pa_height - val_Y * scale_Y; }

   /// convert \n val_Z to the relative Z vector (q. quadrant)
   Pixel_Z valZ2pixel(double val_Z) const

      { return  val_Z * scale_Z; }

   /// convert values \b val_X,  \b val_Y,  and \b val_Z, to the X and Y
   /// coordinate of the pixel below the Z=0 plane of the plot area
   Pixel_XY valXYZ2pixelXY(double val_X, double val_Y, double val_Z) const;

   /// print the properties (for debugging purposes)
   int print(ostream & out) const;

   /// for e.g. att_and_val = "pa_width: 600" set pa_width to 600.
   /// Return error string on error.
   const char * set_attribute(const char * att_and_val);

   /// return the width of the plot window
   Pixel_X  get_window_width() const    { return window_width; }

   /// return the height of the plot window
   Pixel_Y  get_window_height() const   { return window_height; }

   /// return the smallest X value
   double get_min_X() const           { return min_X; }

   /// return the largest X value
   double get_max_X() const           { return max_X; }

   /// return the smallest Y value
   double get_min_Y() const           { return min_Y; }

   /// return the largest Y value
   double get_max_Y() const           { return max_Y; }

   /// return the smallest Z value
   double get_min_Z() const           { return min_Z; }

   /// return the largest Z value
   double get_max_Z() const           { return max_Z; }

   /// return the X value → X pixels scaling factor
   double get_scale_X() const         { return scale_X; }

   /// return the Y value → Y pixels scaling factor
   double get_scale_Y() const         { return scale_Y; }

   /// return the Z value → Z pixels scaling factor
   double get_scale_Z() const        { return scale_Z; }

   /// return the number of pixels between X grid lines
   double get_tile_X() const          { return tile_X; }

   /// return the number of pixels between Y grid lines
   double get_tile_Y() const          { return tile_Y; }

   /// return the number of pixels between Z grid lines
   double get_tile_Z() const          { return tile_Z; }

   /// return the last index of the X grid
   Pixel_X get_gridX_last() const      { return gridX_last; }

   /// return the last index of the Y grid
   Pixel_Y get_gridY_last() const      { return gridY_last; }

   /// return the last index of the Y grid
   Pixel_Y get_gridZ_last() const      { return gridZ_last; }

   /// return the verbosity when plotting
   int get_verbosity() const       { return verbosity; }

   /// set the verbosity when plotting
   void set_verbosity(int verb)      { verbosity = verb; }

   /// return the date to be plotted
   const Plot_data & get_plot_data() const   { return plot_data; }

   /// return the 3D color gradient
   const vector<level_color> & get_gradient() const
      { return gradient; }

   /// for level 0.0 <= alpha <= 1.0: return the color for alpha according
   /// to \b gradient
   uint32_t get_color(double alpha) const;

protected:
   /// the number of plot lines
   const int line_count;

   /// the date to be plotted
   const Plot_data & plot_data;

# define gdef(ty, na, _val, descr) /** descr **/ ty na;
# include "Quad_PLOT.def"

   /// the width of the plot window
   Pixel_X window_width;

   /// the height of the plot window
   Pixel_Y window_height;

   /// the smallest X value
   double min_X;

   /// the largest X value
   double max_X;

   /// the smallest Y value
   double min_Y;

   /// the largest Y value
   double max_Y;

   /// the smallest Z value
   double min_Z;

   /// the largest Z value
   double max_Z;

   /// the X value → X pixels scaling factor
   double scale_X;

   /// the Y value → Y pixels scaling factor
   double scale_Y;

   /// the Z value → Z pixels scaling factor
   double scale_Z;

   /// the number of pixels between X grid lines
   double tile_X;

   /// the number of pixels between Y grid lines
   double tile_Y;

   /// the number of pixels between Z grid lines
   double tile_Z;

   /// the last index of the X grid
   int gridX_last;

   /// the last index of the Y grid
   int gridY_last;

   /// the last index of the Y grid
   int gridZ_last;

   /// the verbosity when plotting
   int verbosity;

   /// the kind of X range
   Plot_Range_type rangeX_type;

   /// the kind of Y range
   Plot_Range_type rangeY_type;

   /// the kind of Z range
   Plot_Range_type rangeZ_type;

   /// array containing the data for the plot lines
   Plot_line_properties * * line_properties;

   /// some levels with colors (for surface plots)
   vector<level_color> gradient;

   /// round val up to the next higher 1/2/5×10^N
   static double round_up_125(double val);
};
//-----------------------------------------------------------------------------
Plot_window_properties::Plot_window_properties(const Plot_data * data,
                                               int verbosity)
   : line_count(data->get_row_count()),
     plot_data(*data),
# define gdef(_ty,  na,  val, _descr) na(val),
# include "Quad_PLOT.def"
     window_width(pa_border_L  + origin_X + pa_width  + pa_border_R),
     window_height(pa_border_T + origin_Y + pa_height + pa_border_B),
     min_X(data->get_min_X()),
     max_X(data->get_max_X()),
     min_Y(data->get_min_Y()),
     max_Y(data->get_max_Y()),
     min_Z(data->get_min_Z()),
     max_Z(data->get_max_Z()),
     scale_X(0),
     scale_Y(0),
     scale_Z(0),
     tile_X(0),
     tile_Y(0),
     tile_Z(0),
     verbosity(0),
     rangeX_type(NO_RANGE),
     rangeY_type(NO_RANGE),
     rangeZ_type(NO_RANGE)
{
   if (verbosity & SHOW_DRAW)
      CERR << setw(20) << "min_X: " << min_X << endl
           << setw(20) << "max_X: " << max_X << endl
           << setw(20) << "min_Y: " << min_Y << endl
           << setw(20) << "max_Y: " << max_Y << endl
           << setw(20) << "min_Z: " << min_Z << endl
           << setw(20) << "max_Z: " << max_Z << endl;

   line_properties = new Plot_line_properties *[line_count + 1];
   loop (l, line_count)   line_properties[l] = new Plot_line_properties(l);
   line_properties[line_count] = 0;

   // set the origin_X and origin_Y properties back to 0 so that they
   // affect only surface plots.
   if (!data->surface)
      {
         set_origin_X(0);
         set_origin_Y(0);
      }

   update(verbosity);
}
//-----------------------------------------------------------------------------
void
Plot_window_properties::set_window_size(Pixel_X width, Pixel_Y height)
{
   // window_width and window_height are, despite of their names, the
   // size of the plot area (without borders).
   //
   pa_width  = width  - pa_border_L - origin_X - pa_border_R;
   pa_height = height - pa_border_T - origin_Y - pa_border_B;
   update(0);
}
//-----------------------------------------------------------------------------

bool
Plot_window_properties::update(int verbosity)
{
   // this function is called for 2 reasons:
   //
   // 1. when constructor Plot_window_properties::Plot_window_properties()
   //    is called in plot_main(), i.e. when the the plot window is created 
   //    for the first time, and
   //
   // 2. when the user resizes the plot window.
   //
   // In both cases the caller has changed (at least) pa_width and pa_height,
   // and this function recomputes all variables that depend on them.
   //

   window_width  = pa_border_L + origin_X + pa_width  + pa_border_R;
   window_height = pa_border_T + origin_Y + pa_height + pa_border_B;

   min_X = plot_data.get_min_X();
   max_X = plot_data.get_max_X();
   min_Y = plot_data.get_min_Y();
   max_Y = plot_data.get_max_Y();
   min_Z = plot_data.get_min_Z();
   max_Z = plot_data.get_max_Z();

   // the user may override the range derived from the data
   //
   if (rangeX_min_valid())   min_X = rangeX_min;
   if (rangeX_max_valid())   max_X = rangeX_max;
   if (rangeY_min_valid())   min_Y = rangeY_min;
   if (rangeY_max_valid())   max_Y = rangeY_max;
   if (rangeZ_min_valid())   min_Z = rangeZ_min;
   if (rangeZ_max_valid())   max_Z = rangeZ_max;

   if (min_X >= max_X)
      {
        MORE_ERROR() << "empty X range in A ⎕PLOT B";
        return true;
      }

   if (min_Y >= max_Y)
      {
        MORE_ERROR() << "empty Y range " << min_Y << ":" << max_Y
                     << " in A ⎕PLOT B";
        return true;
      }

   if (min_Z >= max_Z)
      {
        MORE_ERROR() << "empty Z range in A ⎕PLOT B";
        return true;
      }

double delta_X = max_X - min_X;
double delta_Y = max_Y - min_Y;
double delta_Z = max_Z - min_Z;
const double orig_len = sqrt(origin_X*origin_X + origin_Y*origin_Y);

   // first approximation for value → pixel factor
   //
   scale_X = pa_width  / delta_X;   // pixels per X-value
   scale_Y = pa_height / delta_Y;   // pixels per Y-value
   scale_Z = orig_len ? orig_len / delta_Z : 1.0;    // pixels per Z-value

   if (verbosity & SHOW_DRAW)
      CERR << setw(20) << "delta_X (1): " << delta_X << " (Xmax-Xmin)" << endl
           << setw(20) << "delta_Y (1): " << delta_Y << " (Ymax-Ymin)" << endl
           << setw(20) << "delta_Z (1): " << delta_Z << " (Zmax-Zmin)" << endl
           << setw(20) << "scale_X (1): " << scale_X << " pixels/X" << endl
           << setw(20) << "scale_Y (1): " << scale_Y << " pixels/Y" << endl
           << setw(20) << "scale_Z (1): " << scale_Z << " pixels/Z" << endl;

const double tile_X_raw = gridX_pixels / scale_X;
const double tile_Y_raw = gridY_pixels / scale_Y;
const double tile_Z_raw = gridZ_pixels / scale_Z;

   if (verbosity & SHOW_DRAW)
      CERR << setw(20) << "tile_X_raw: " << tile_X_raw
                                         << "(∆X between grid lines)" << endl
           << setw(20) << "tile_Y_raw: " << tile_Y_raw
                                         << "(∆Y between grid lines)" << endl
           << setw(20) << "tile_Z_raw: " << tile_Z_raw
                                         << "(∆Z between grid lines)" << endl;

   tile_X = round_up_125(tile_X_raw);
   tile_Y = round_up_125(tile_Y_raw);
   tile_Z = round_up_125(tile_Z_raw);

const int min_Xi = floor(min_X / tile_X);
const int min_Yi = floor(min_Y / tile_Y);
const int min_Zi = floor(min_Z / tile_Z);
const int max_Xi = ceil(max_X / tile_X);
const int max_Yi = ceil(max_Y / tile_Y);
const int max_Zi = ceil(max_Z / tile_Z);
   gridX_last = 0.5 + max_Xi - min_Xi;
   gridY_last = 0.5 + max_Yi - min_Yi;
   gridZ_last = 0.5 + max_Zi - min_Zi;

   min_X = tile_X * floor(min_X / tile_X);
   min_Y = tile_Y * floor(min_Y / tile_Y);
   min_Z = tile_Z * floor(min_Z / tile_Z);
   max_X = tile_X * ceil(max_X  / tile_X);
   max_Y = tile_Y * ceil(max_Y  / tile_Y);
   max_Z = tile_Z * ceil(max_Z  / tile_Z);

   delta_X = max_X - min_X;
   delta_Y = max_Y - min_Y;
   delta_Z = max_Z - min_Z;

   // final value → pixel factor
   //
   scale_X = pa_width  / delta_X;
   scale_Y = pa_height / delta_Y;
   scale_Z = orig_len ? orig_len  / delta_Z : 1.0;

   if (verbosity & SHOW_DRAW)
      CERR << setw(20) << "min_X (2): " << min_X << endl
           << setw(20) << "max_X (2): " << max_X << endl
           << setw(20) << "min_Y (2): " << min_Y << endl
           << setw(20) << "max_Y (2): " << max_Y << endl
           << setw(20) << "min_Z (2): " << min_Z << endl
           << setw(20) << "max_Z (2): " << max_Z << endl
           << setw(20) << "delta_X (2): " << delta_X << " (Xmax-Xmin)" << endl
           << setw(20) << "delta_Y (2): " << delta_Y << " (Ymax-Ymin)" << endl
           << setw(20) << "delta_Z (2): " << delta_Z << " (Zmax-Zmin)" << endl
           << setw(20) << "scale_X (2): " << scale_X << " pixels/X" << endl
           << setw(20) << "scale_Y (2): " << scale_Y << " pixels/Y" << endl
           << setw(20) << "scale_Z (2): " << scale_Z << " pixels/Z" << endl
           << setw(20) << "tile_X  (2): " << tile_X
                                          << "(∆X between grid lines)" << endl
           << setw(20) << "tile_Y  (2): " << tile_Y
                                          << "(∆Y between grid lines)" << endl
           << setw(20) << "tile_Z  (2): " << tile_Z
                                          << "(∆Z between grid lines)" << endl;

   return false;   // OK
}
//-----------------------------------------------------------------------------
int
Plot_window_properties::print(ostream & out) const
{
# define gdef(ty,  na,  _val, _descr) \
   out << setw(20) << #na ":  " << Plot_data::ty ## _to_str(na) << endl;
# include "Quad_PLOT.def"

   loop(g, gradient.size())
       {
         char cc[30];
         snprintf(cc, sizeof(cc), "color_level-%u: ", gradient[g].level);
         out << setw(20) << cc
             << Plot_data::Color_to_str(gradient[g].rgb) << endl;
       }

   for (Plot_line_properties ** lp = line_properties; *lp; ++lp)
       {
         out << endl;
         (*lp)->print(out);
       }

   return 0;
}
//-----------------------------------------------------------------------------
const char *
Plot_window_properties::set_attribute(const char * att_and_val)
{
   /*
       called with leading whitespaces in att_and_val removed.

       att_and_val is a string specifying one of:

       a window attribute, like:   origin_X: 100                 , or
       a line attribute, like:     point_color-1:  #000000       , or
       a color gradient, like:     color_level-50: #00FF00

       The value (right of ':') can be

       an integer value, like:     100
       a color, like:              #000000 or #000
       a string, like:             "Window Title"

       return 0 on success or else an appropriate error string.
    */

   // find demarcation between attribute name and attribute value
   ///
const char * colon = strchr(att_and_val, ':');
   if (colon == 0)   return "Expecting 'attribute: value' but no : found";

   // skip leading whitespace before the attribute value
   //
const char * value = colon + 1;
   while (*value && (*value <= ' '))   ++value;

   // check for line number resp. level
   // 
const char * minus = strchr(att_and_val, '-');
   if (minus && minus < colon)   // line attribute or color gradient
      {
        // it is not unlikely that the user wants to use the same attributes
        // for different plots that may differ in the number of lines. We
        // therefore silently ignore such over-specified attribute rather
        // than returning an error string (which then raises a DOMAIN error).
        // 
        if (!strncmp(att_and_val, "color_level-", 12))
           {
             const int level = strtol(minus + 1, 0, 10);
             if (level < 0)     return "negative color level";
             if (level > 100)   return "color level > 100%";

              const char * error = 0;
              const Color color = Plot_data::Color_from_str(value, error);
              if (error)   return error;

              level_color grad = { level, color };

              // insert grad into gradient, but keeping it sorted by level
              //
              loop(g, gradient.size())
                  {
                    if (gradient[g].level == grad.level)
                       return "duplicate gradient level";

                    if (gradient[g].level > grad.level)
                       {
                         gradient.insert(gradient.begin() + g, grad);
                         return 0;
                       }
                  }
              gradient.push_back(grad);
              return 0;   // OK
           }

        const int line = strtol(minus + 1, 0, 10) - Workspace::get_IO();
        if (line < 0)             return "line number ≤ ⎕IO";
        if (line >= line_count)   return "line number too large";

# define ldef(ty,  na,  val, _descr)                                       \
         if (!strncmp(#na "-", att_and_val, minus - att_and_val))         \
            { const char * error = 0;                                     \
              line_properties[line]->set_ ## na(Plot_data::ty ##          \
                                                _from_str(value, error)); \
              return error;                                               \
            }
# include "Quad_PLOT.def"

        // nothing found
        //
        return "Bad or unknown line attribute";
      }
   else                          // window attribute
      {
# define gdef(ty,  na,  _val, _descr) \
         if (!strncmp(#na, att_and_val, colon - att_and_val))         \
            { const char * error = 0;                                 \
              set_ ## na(Plot_data::ty ## _from_str(value, error));   \
              return error;                                           \
            }
# include "Quad_PLOT.def"
      }

   return "Bad or unknown window attribute";
}
//-----------------------------------------------------------------------------
double
Plot_window_properties::round_up_125(double val)
{
int expo = 0;
   while (val >= 10)    { val /= 10;   ++expo; }
   while (val <  1)     { val *= 10;   --expo; }

   // at this point 1 ≤ val < 10.
   //
   if (expo > -18 && expo < 18)   // use integer arithmetic for 10^expo
      {
        const int abs_expo = expo < 0 ? -expo : expo;
        uint64_t expo_val = 1;
        loop(a, abs_expo)   expo_val *= 10;

        if (val <= 2.0)   return  expo < 0 ? 2.0/expo_val : 2.0*expo_val;
        if (val <= 5.0)   return  expo < 0 ? 5.0/expo_val : 5.0*expo_val;
        return  expo < 0 ? 10.0/expo_val : 10.0*expo_val;
      }
   else                           // use float arithmetic for 10^expo
      {
        const double expo_val = pow(10, expo);
        if (val <= 2.0)   return  expo < 0 ? 2.0/expo_val : 2.0*expo_val;
        if (val <= 5.0)   return  expo < 0 ? 5.0/expo_val : 5.0*expo_val;
        return  expo < 0 ? 10.0/expo_val : 10.0*expo_val;
      }
}
//-----------------------------------------------------------------------------
uint32_t
Plot_window_properties::get_color(double alpha) const
{
   Assert(alpha >= 0.0);
   Assert(alpha <= 1.0);

   if (gradient.size() == 0)   // no gradient specified: use gray-scale
      return uint32_t(255*alpha) << 16
           | uint32_t(255*alpha) << 8
           | uint32_t(255*alpha);

   if (gradient.size() == 1)   // only one gradient specified: use it
      return gradient[0].rgb;

   loop(g, gradient.size())
       {
         const double level_g = 0.01*gradient[g].level;
         if (alpha == level_g)   return gradient[g].rgb;
         if (alpha < level_g)   // level_g is the first level above alpha
            {
               if (g == 0)   return gradient[0].rgb;   // use first gradient

               // interpolate
               //
               const double l0 = 0.01*gradient[g-1].level;   // previous level
               const double beta1 = (alpha - l0) / (level_g - l0);
               Assert(beta1 >= 0.0);
               Assert(beta1 <= 1.0);
               const double beta0 = 1.0 - beta1;
               const uint32_t rgb0 = gradient[g-1].rgb;
               const uint32_t rgb1 = gradient[g].rgb;
               const uint32_t red = beta1*(rgb1 >> 16 & 0xFF)
                                  + beta0*(rgb0 >> 16 & 0xFF);
               const uint32_t grn = beta1*(rgb1 >>  8 & 0xFF)
                                  + beta0*(rgb0 >>  8 & 0xFF);
               const uint32_t blu = beta1*(rgb1       & 0xFF)
                                  + beta0*(rgb0       & 0xFF);
               return red << 16 | grn << 8 | blu;
            }
       }

   // alpha is above last gradient: use last gradient
   //
   return gradient.back().rgb;
}
//=============================================================================
void
testCookie(xcb_void_cookie_t cookie, xcb_connection_t * conn,
           const char * errMessage)
{
xcb_generic_error_t * error = xcb_request_check(conn, cookie);
   if (error)
      {
        CERR <<"ERROR: " << errMessage << " : " << error->error_code << endl;
        xcb_disconnect(conn);
        exit (-1);
      }
}
//-----------------------------------------------------------------------------
/// Some xcb IDs (returned from the X server)
struct Plot_context
{
  Plot_context(const Plot_window_properties & pwp)
  : w_props(pwp)
  {}

  const Plot_window_properties & w_props;

  xcb_connection_t * conn;
  xcb_window_t       window;
  xcb_screen_t       * screen;
  xcb_gcontext_t     fill;
  xcb_gcontext_t     line;
  xcb_gcontext_t     point;
  xcb_gcontext_t     text;
  xcb_font_t         font;
};
//-----------------------------------------------------------------------------
/// the width and the height of a string (in pixels)
struct string_width_height
{
   /// constructor: ask X-server for the size (in pixels) of \b string
   string_width_height(const Plot_context & pctx, const char * string);

   /// the width of the string (pixels)
   Pixel_X width;

   /// the height of the string (pixels)
   Pixel_Y height;
};
//-----------------------------------------------------------------------------

string_width_height::string_width_height(const Plot_context & pctx,
                                         const char * string)
{
const size_t string_length = strlen(string);
xcb_char2b_t xcb_str[string_length + 10];

   loop(s, string_length)
       {
         xcb_str[s].byte1 = 0;
         xcb_str[s].byte2 = string[s];
       }

xcb_query_text_extents_cookie_t cookie =
   xcb_query_text_extents(pctx.conn, pctx.font, string_length, xcb_str);

xcb_query_text_extents_reply_t * reply =
   xcb_query_text_extents_reply(pctx.conn, cookie, 0);

   if (reply)
      {
        width  = reply->overall_width;
        height = reply->font_ascent + reply->font_descent;
        // origin = reply->font_ascent;
        free(reply);
      }
   else
      {
        puts("XCB ERROR: xcb_query_text_extents_reply() failed.");
        width  = 24;
        height = 13;
      }
}
//-----------------------------------------------------------------------------
xcb_gcontext_t
setup_font_gc(Plot_context & pctx, const char * font_name)
{
   /* get font */

   testCookie(xcb_open_font_checked(pctx.conn, pctx.font, strlen(font_name),
                                    font_name), pctx.conn, "can't open font");

   // create graphics context
xcb_gcontext_t gc = xcb_generate_id(pctx.conn);
uint32_t mask     = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
uint32_t values[3] = { pctx.screen->black_pixel,
                       pctx.screen->white_pixel,
                       pctx.font };

   testCookie(xcb_create_gc_checked(pctx.conn, gc, pctx.window, mask, values),
              pctx.conn, "can't create font gc");
   return gc;
}
//-----------------------------------------------------------------------------
void
draw_text(const Plot_context & pctx, const char * label, const Pixel_XY & xy)
{
/* draw the text */
xcb_void_cookie_t textCookie = xcb_image_text_8_checked(pctx.conn,
                                                        strlen(label),
                                                        pctx.window, pctx.text,
                                                        xy.x, xy.y, label);

   testCookie(textCookie, pctx.conn,
              "xcb_image_text_8_checked() failed.");
}
//-----------------------------------------------------------------------------
char *
format_tick(double val)
{
static char cc[40];
char * cp = cc;
   if (val == 0)
      {
        *cp++ = '0';
        *cp++ = 0;
        return cc;
      }

   if (val < 0)
      {
        *cp++ = '-';
        val = -val;
      }

const char * unit = 0;
   if (val >= 1e3)
      {
        if      (val >= 1e15)   ;
        else if (val >= 1e12)   { unit = "T";   val *= 1e-12; }
        else if (val >= 1e9)    { unit = "G";   val *= 1e-9;  }
        else if (val >= 1e6)    { unit = "M";   val *= 1e-6;  }
        else                    { unit = "k";   val *= 1e-3;  }
      }
   else
      {

        if (val >= 1e0)    { unit = "";                  }
        else if (val >= 1e-3)   { unit = "m";   val *= 1e3;   }
        else if (val >= 1e-6)   { unit = "μ";   val *= 1e6;   }
        else if (val >= 1e-9)   { unit = "n";   val *= 1e9;   }
        else if (val >= 1e-12)  { unit = "p";   val *= 1e12;  }
      }

   if (unit == 0)   // very large or very small
      {
        snprintf(cp, sizeof(cc) - 1, "%.2E", val);
        return cc;
      }

   // at this point: 1 ≤ val < 1000
   //
   if      (val < 10)    snprintf(cp, sizeof(cc) - 1, "%.2f%s", val, unit);
   else if (val < 100)   snprintf(cp, sizeof(cc) - 1, "%.1f%s", val, unit);
   else                  snprintf(cp, sizeof(cc) - 1, "%.0f%s", val, unit);
   return cc;
}
//-----------------------------------------------------------------------------
void
draw_line(const Plot_context & pctx, xcb_gcontext_t gc,
          const Pixel_XY & P0, const Pixel_XY & P1)
{
const xcb_segment_t segment = { I1616(P0.x, P0.y), I1616(P1.x, P1.y) };
   xcb_poly_segment(pctx.conn, pctx.window, gc, 1, &segment);
}
//-----------------------------------------------------------------------------
inline void
pv_swap(Pixel_XY & P0, double & Y0, Pixel_XY & P1, double & Y1)
{
const double   Y = Y0;   Y0 = Y1;   Y1 = Y;
const Pixel_XY P = P0;   P0 = P1;   P1 = P;
}
//-----------------------------------------------------------------------------
inline double
intersection(double Ax, double Ay, double Bx, double By,
             double Cx, double Cy, double Dx, double Dy)
{
   /** given 4 points A=(Ax, Ay), B=(Bx, By), C=(Cx, Cy), and D=(Dx, Dy)
       in the plane, return the factor beta so that the intersection point
       M=(Mx, My) of the lines A-C and B-D is given by

       M = (Mx, My) == A + alpha*(D-A) == B + beta*(C - B). Note that:

       A
        \   B
         \ /
          M
         / \
        /   \
       C     D
    Ax     + alpha*(Dx-Ax) == Bx + beta*(Cx-Bx)
    Ay     + alpha*(Dy-Ay) == By + beta*(Cy-By)

   **/

   /* first, translate A, B, and D by -C, so that C=(0, 0):

       A
        \   B
         \ /
          M
         / \
        /   \
       0     D
   */
   Ax -= Cx;   Bx -= Cx;   Dx -= Cx;
   Ay -= Cy;   By -= Cy;   Dy -= Cy;

   /* Thus:

   (Ax-Bx) + alpha*(Dx-Ax) == beta*(-Bx)
   (Ay-By) + alpha*(Dy-Ay) == beta*(-By)

   (Ax-Bx)*(Dy-Ay) + alpha*(Dx-Ax)*(Dy-Ay) == beta*(-Bx)*(Dy-Ay)
   (Ay-By)*(Dx-Ax) + alpha*(Dy-Ay)*(Dx-Ax) == beta*(-By)*(Dx-Ax)
   =======================================    ==================
   (Ax-Bx)*(Dy-Ay) - (Ay-By)*(Dx-Ax) ==  beta*((-Bx)*(Dy-Ay) - (-By)*(Dx-Ax))
   **/

const double numer = (Ax-Bx)*(Dy-Ay) - (Ay-By)*(Dx-Ax);   // left side
const double denom = ((-Bx)*(Dy-Ay) - (-By)*(Dx-Ax));     // right factor
   if (denom == 0.0)
      {
        Ax += Cx;   Bx += Cx;   Dx += Cx;
        Ay += Cy;   By += Cy;   Dy += Cy;
        MORE_ERROR() << "⎕PLOT: one of the X-Z squares has bad coordinates:"
                     << "P1=(" << Ax << ":" << Ay << "), "
                        "P2=(" << Bx << ":" << By << "), "
                        "P3=(" << Cx << ":" << Cy << ") "
                        "P4=(" << Dx << ":" << Dy << ")";
        DOMAIN_ERROR;
      }

   return /* beta == */ numer/denom;
}
//-----------------------------------------------------------------------------
void
draw_triangle(const Plot_context & pctx, int verbosity,
              Pixel_XY P0, double H0, Pixel_XY P1, Pixel_XY P2, double H12)
{
   // draw a triangle with P1 and P2 at the same level H12
   //
   Assert(H0 >= 0.0);    Assert(H0 <= 1.0);
   Assert(H12 >= 0.0);   Assert(H12 <= 1.0);

   if (verbosity & SHOW_DRAW)
      CERR <<   " ∆2: P0(" << P0.x << ":" << P0.y << ") @H0=" << H0
           << "     P1(" << P1.x << ":" << P1.y << ") @H12=" << H12
           << "     P2(" << P2.x << ":" << P2.y << ") @H12=" << H12 << endl;

const double d1 = P0.distance2(P1);
const double d2 = P0.distance2(P2);
const int steps = sqrt(d1 > d2 ? d1 : d2);
const double dH = (H12 - H0) / steps;
   loop(s, steps)
       {
         const double alpha = H0 + s*dH;
         const double beta  = 1.0 * s/ steps;
         const uint32_t color = pctx.w_props.get_color(alpha);
         xcb_change_gc(pctx.conn, pctx.fill, XCB_GC_FOREGROUND, &color);

         const Pixel_XY P0_P1(P0.x + beta * (P1.x - P0.x),
                              P0.y + beta * (P1.y - P0.y));
         const Pixel_XY P0_P2(P0.x + beta * (P2.x - P0.x),
                              P0.y + beta * (P2.y - P0.y));
         draw_line(pctx, pctx.fill, P0_P1, P0_P2);
       }
}
//-----------------------------------------------------------------------------
void
draw_triangle(const Plot_context & pctx, int verbosity, Pixel_XY P0, double H0,
              Pixel_XY P1, double H1, Pixel_XY P2, double H2)
{
   // draw a triangle with P0, P1 and P2 at levels H0, H1, and H2

   Assert(H0 >= 0);   Assert(H0 <= 1.0);
   Assert(H1 >= 0);   Assert(H1 <= 1.0);
   Assert(H2 >= 0);   Assert(H2 <= 1.0);

   if (verbosity & SHOW_DRAW)
      CERR << "\n∆1: P0(" << P0.x << ":" << P0.y << ")@H=" << H0
           << "      P1(" << P1.x << ":" << P1.y << ")@H=" << H1
           << "      P2(" << P2.x << ":" << P2.y << ")@H=" << H2 << endl;

   if (H0 < H1)   pv_swap(P0, H0, P1, H1);   // then H0 >= H1
   if (H0 < H2)   pv_swap(P0, H0, P2, H2);   // then H0 >= H2
   if (H1 < H2)   pv_swap(P1, H1, P2, H2);   // then H1 >= H2

   // here H0 >= H1 >= H2
   //
   Assert(H0 >= H1);
   Assert(H1 >= H2);

   if (H0 == H1)   // P0 and P1 have the same height
      {
        draw_triangle(pctx, verbosity, P2, H2, P0, P1, H0);
      }
   else if (H1 == H2)   // P1 and P2 have the same height
      {
        draw_triangle(pctx, verbosity, P0, H0, P1, P2, H1);
      }
   else            // P2 lies below P1
      {
        const double alpha = (H0 - H1) / (H0 - H2);   // alpha → 1 as P1 → P2
        Assert(alpha >= 0.0);
        Assert(alpha <= 1.0);

        // compute the point P on P0-P2 that has the heigth H1 (of P1)
        const Pixel_XY P(P0.x + alpha*(P2.x - P0.x),
                         P0.y + alpha*(P2.y - P0.y));
        draw_triangle(pctx, verbosity, P0, H0, P1, P, H1);
        draw_triangle(pctx, verbosity, P2, H2, P1, P, H1);
      }
}
//-----------------------------------------------------------------------------
void
draw_point(const Plot_context & pctx, const Pixel_XY & xy, Color canvas_color,
           const Plot_line_properties & l_props)
{
const Pixel_X point_size  = l_props.point_size;   // outer point diameter
const Pixel_X point_size2 = l_props.point_size2;  // inner point diameter
const int16_t half = point_size >> 1;             // outer point radius
const int point_style = l_props.get_point_style();

   if (point_style == 1)   // circle
      {
        xcb_arc_t arc = { I1616(xy.x - half, xy.y - half),   // positions
                          point_size, point_size,            // diameters
                          0, 360 << 6 };                     // angles
        xcb_poly_fill_arc(pctx.conn, pctx.window, pctx.point, 1, &arc);

        if (point_size2)   // hollow
           {
             const int16_t half2 = point_size2 >> 1;
             xcb_arc_t arc2 = { I1616(xy.x - half2, xy.y - half2),   // pos
                                point_size2, point_size2,            // dia
                                0, int16_t(360 << 6) };              // ang
             enum { mask = XCB_GC_FOREGROUND };
             xcb_change_gc(pctx.conn, pctx.point, mask, &canvas_color);
             xcb_poly_fill_arc(pctx.conn, pctx.window, pctx.point, 1, &arc2);
             xcb_change_gc(pctx.conn, pctx.point, mask, &l_props.point_color);
           }
      }
   else if (point_style == 2)   // triangle ∆
      {
        const int16_t center_y = xy.y - 2*point_size/3;
        const int16_t hypo_y = half*1.732;
        const xcb_point_t points[3]   = { { int16_t(xy.x), center_y  },
                                          { half, hypo_y },
                                          { int16_t(-2*half), 0   } };
        xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                      XCB_POLY_SHAPE_CONVEX,
                      XCB_COORD_MODE_PREVIOUS, ITEMS(points));
        if (point_size2)   // hollow
           {
             const int16_t half2 = point_size2 >> 1;
             const int16_t center2_y = xy.y - 2*point_size2/3;
             const int16_t hypo2_y = half2*1.732;
             const xcb_point_t points2[3]  = {
                 { int16_t(xy.x), center2_y   },
                 { int16_t(half2), hypo2_y },
                 { int16_t(-2*half2), 0    } };
             enum { mask = XCB_GC_FOREGROUND };
             xcb_change_gc(pctx.conn, pctx.point, mask, &canvas_color);
             xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                           XCB_POLY_SHAPE_CONVEX,
                           XCB_COORD_MODE_PREVIOUS, ITEMS(points2));
             xcb_change_gc(pctx.conn, pctx.point, mask, &l_props.point_color);
           }
      }
   else if (point_style == 3)   // triangle ∇
      {
        const int16_t center_y = xy.y + 2*point_size/3;
        const int16_t hypo_y = - half*1.732;
        const xcb_point_t points[3]   = {
              { int16_t(xy.x), center_y  },
              { int16_t(half), hypo_y },
              { int16_t(-2*half), 0   } };
        xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                      XCB_POLY_SHAPE_CONVEX,
                      XCB_COORD_MODE_PREVIOUS, ITEMS(points));
        if (point_size2)   // hollow
           {
             const int16_t half2 = point_size2 >> 1;
             const int16_t center2_y = xy.y + 2*point_size2/3;
             const int16_t hypo2_y = - half2*1.732;
             const xcb_point_t points2[3]  = {
                 { int16_t(xy.x), center2_y   },
                 { int16_t(half2), hypo2_y },
                 { int16_t(-2*half2), 0    } };
             enum { mask = XCB_GC_FOREGROUND };
             xcb_change_gc(pctx.conn, pctx.point, mask, &canvas_color);
             xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                           XCB_POLY_SHAPE_CONVEX,
                           XCB_COORD_MODE_PREVIOUS, ITEMS(points2));
             xcb_change_gc(pctx.conn, pctx.point, mask, &l_props.point_color);
           }
      }
   else if (point_style == 4)   // caro
      {
        const xcb_point_t points[4] =           {
              { int16_t(xy.x), int16_t(xy.y - half) },
              {  half,  half },
              { int16_t(-half),  half },
              { int16_t(-half), int16_t(-half) } };
        xcb_fill_poly(pctx.conn, pctx.window, pctx.point, XCB_POLY_SHAPE_CONVEX,
                      XCB_COORD_MODE_PREVIOUS, ITEMS(points));
        if (point_size2)   // hollow
           {
             const int16_t half2 = point_size2 >> 1;
             const xcb_point_t points2[4] =      {
                   { I1616(xy.x, xy.y - half2) },
                   { I1616( half2,  half2)     },
                   { I1616(-half2,  half2)     },
                   { I1616(-half2, -half2)     } };
             enum { mask = XCB_GC_FOREGROUND };
             xcb_change_gc(pctx.conn, pctx.point, mask, &canvas_color);
             xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                           XCB_POLY_SHAPE_CONVEX,
                           XCB_COORD_MODE_PREVIOUS,
                           ITEMS(points2));
             xcb_change_gc(pctx.conn, pctx.point, mask, &l_props.point_color);
           }
      }
   else if (point_style == 5)   // square
      {
        const xcb_point_t points[4] =   {
              { I1616(xy.x - half, xy.y - half) },
              { I1616(point_size, 0)  },
              { I1616(0, point_size)  },
              { I1616(-point_size, 0) } };
        xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                      XCB_POLY_SHAPE_CONVEX,
                      XCB_COORD_MODE_PREVIOUS, ITEMS(points));
        if (point_size2)   // hollow
           {
             const int16_t half2 = point_size2 >> 1;
             const xcb_point_t points2[4] =
                 { { int16_t(xy.x - half2), int16_t(xy.y - half2) },
                   { int16_t(point_size2), 0  },
                   { 0, int16_t(point_size2)  },
                   { int16_t(-point_size2), 0 } };
             enum { mask = XCB_GC_FOREGROUND };
             xcb_change_gc(pctx.conn, pctx.point, mask, &canvas_color);
             xcb_fill_poly(pctx.conn, pctx.window, pctx.point,
                           XCB_POLY_SHAPE_CONVEX,
                           XCB_COORD_MODE_PREVIOUS, ITEMS(points2));
             xcb_change_gc(pctx.conn, pctx.point, mask, &l_props.point_color);
           }
      }

   // otherwise: draw no point
}
//-----------------------------------------------------------------------------
/// draw the legend
void
draw_legend(const Plot_context & pctx, const Plot_window_properties & w_props)
{
Plot_line_properties const * const * l_props = w_props.get_line_properties();
const Color canvas_color = w_props.get_canvas_color();

   for (int l = 0; l_props[l]; ++l)
       {
         const Plot_line_properties & lp = *l_props[l];

         enum { mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH };
         const uint32_t gc_l[] = { lp.line_color, lp.line_width };
         const uint32_t gc_p[] = { lp.point_color, lp.point_size };

         const int x0 = w_props.get_legend_X();
         const int x1 = x0 + (w_props.get_legend_X() >> 1);
         const int x2 = x1 + (w_props.get_legend_X() >> 1);
         const int xt = x2 + 10;
         const int y  = w_props.get_legend_Y() + l*w_props.get_legend_dY();

         xcb_change_gc(pctx.conn, pctx.line, mask, gc_l);
         draw_line(pctx, pctx.line, Pixel_XY(x0, y), Pixel_XY(x2, y));
         draw_text(pctx, lp.legend_name.c_str(), Pixel_XY(xt, y + 5));
         xcb_change_gc(pctx.conn, pctx.point, mask, gc_p);
         draw_point(pctx, Pixel_XY(x1, y), canvas_color, *l_props[l]);
       }
}
//-----------------------------------------------------------------------------
/// draw the grid lines that start at the X axis
void
draw_X_grid(const Plot_context & pctx, const Plot_window_properties & w_props,
            bool surface)
{
enum { mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH };
const uint32_t values[] = { w_props.get_gridX_color(),
                            w_props.get_gridX_line_width() };
   xcb_change_gc(pctx.conn, pctx.line, mask, values);

const Pixel_Y py0 = w_props.valY2pixel(0);
const double dy = w_props.get_max_Y() - w_props.get_min_Y();
const Pixel_Y py1 = w_props.valY2pixel(dy);
   for (int ix = 0; ix <= w_props.get_gridX_last(); ++ix)
       {
         const double v = w_props.get_min_X() + ix*w_props.get_tile_X();
         const int px0 = w_props.valX2pixel(v - w_props.get_min_X())
                      + w_props.get_origin_X();
         if (ix == 0 || ix == w_props.get_gridX_last() ||
             w_props.get_gridX_style() == 1)
            {
              draw_line(pctx, pctx.line, Pixel_XY(px0, py0), Pixel_XY(px0, py1));
            }
         else if (w_props.get_gridX_style() == 2)
            {
              draw_line(pctx, pctx.line, Pixel_XY(px0, py0 - 5), Pixel_XY(px0, py0 + 5));
            }

         char * cc = format_tick(v);
         string_width_height wh(pctx, cc);
        if (surface)
           {
              const Pixel_X px2 = px0 - w_props.get_origin_X();
              const Pixel_Y py2 = py0 + w_props.get_origin_Y();
              draw_line(pctx, pctx.line, Pixel_XY(px0, py0), Pixel_XY(px2, py2));

              draw_text(pctx, cc,
                        Pixel_XY(px0 - wh.width/2 - w_props.get_origin_X(),
                                 py0 + wh.height + 3+ w_props.get_origin_Y()));
           }
        else
           {
             draw_text(pctx, cc,
                       Pixel_XY(px0 - wh.width/2, py0 + wh.height + 3));
           }
       }
}
//-----------------------------------------------------------------------------
/// draw the grid lines that start at the Y axis
void
draw_Y_grid(const Plot_context & pctx, const Plot_window_properties & w_props,
            bool surface)
{
enum { mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH};
const uint32_t values[] = { w_props.get_gridY_color(),
                            w_props.get_gridY_line_width() };
   xcb_change_gc(pctx.conn, pctx.line, mask, values);

const Pixel_X px0 = w_props.valX2pixel(0) + w_props.get_origin_X();
const double dx = w_props.get_max_X() - w_props.get_min_X();
const Pixel_X px1 = w_props.valX2pixel(dx) + w_props.get_origin_X();
   for (int iy = 0; iy <= w_props.get_gridY_last(); ++iy)
       {
         const double v = w_props.get_min_Y() + iy*w_props.get_tile_Y();
         const Pixel_Y py0 = w_props.valY2pixel(v - w_props.get_min_Y());
         if (iy == 0 || iy == w_props.get_gridY_last() ||
               w_props.get_gridY_style() == 1)
            {
              draw_line(pctx, pctx.line, Pixel_XY(px0, py0), Pixel_XY(px1, py0));
            }
         else if (w_props.get_gridY_style() == 2)
            {
              draw_line(pctx, pctx.line, Pixel_XY(px0 - 5, py0), Pixel_XY(px0 + 5, py0));
            }

        char * cc = format_tick(v);
        string_width_height wh(pctx, cc);
        if (surface)
           {
              const Pixel_X px2 = px0 - w_props.get_origin_X();
              const Pixel_Y py2 = py0 + w_props.get_origin_Y();
              draw_line(pctx, pctx.line, Pixel_XY(px0, py0), Pixel_XY(px2, py2));

             draw_text(pctx, cc,
                       Pixel_XY(px0 - wh.width - 5 - w_props.get_origin_X(),
                             py0 + wh.height/2 - 1 + w_props.get_origin_Y()));
           }
        else
           {
             draw_text(pctx, cc,
                       Pixel_XY(px0 - wh.width - 5,
                                py0 + wh.height/2 - 1));
           }
       }

   if (!surface)   return;
}
//-----------------------------------------------------------------------------
/// draw the grid lines that start at the Z axis
void
draw_Z_grid(const Plot_context & pctx, const Plot_window_properties & w_props)
{
enum { mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH };
const uint32_t values[] = { w_props.get_gridZ_color(),
                            w_props.get_gridZ_line_width() };
   xcb_change_gc(pctx.conn, pctx.line, mask, values);

const int iz_max = w_props.get_gridZ_last();
const Pixel_X len_Zx = w_props.get_origin_X();
const Pixel_Y len_Zy = w_props.get_origin_Y();
const Pixel_XY orig = w_props.valXYZ2pixelXY(w_props.get_min_X(),
                                             w_props.get_min_Y(),
                                             w_props.get_min_Z());
const Pixel_X len_X = w_props.valX2pixel(w_props.get_max_X())
                    - w_props.valX2pixel(w_props.get_min_X());
const Pixel_Y len_Y = w_props.valY2pixel(w_props.get_max_Y())
                    - w_props.valY2pixel(w_props.get_min_Y());
   for (int iz = 1; iz <= iz_max; ++iz)
       {
         const double v = w_props.get_min_Z() + iz*w_props.get_tile_Z();
         const Pixel_X px0 = orig.x - iz * len_Zx / iz_max;
         const Pixel_Y py0 = orig.y + iz * len_Zy / iz_max;
         const Pixel_X px1 = px0 + len_X;
         draw_line(pctx, pctx.line, Pixel_XY(px0, py0), Pixel_XY(px1,  py0));
         draw_line(pctx, pctx.line, Pixel_XY(px0, py0), Pixel_XY(px0,  py0 + len_Y));

        char * cc = format_tick(v);
        string_width_height wh(pctx, cc);

        draw_text(pctx, cc,
                  Pixel_XY(px1 + 10, py0 + wh.height/2 - 1));
       }
}
//-----------------------------------------------------------------------------
/// draw the plot lines
void
draw_plot_lines(const Plot_context & pctx,
                const Plot_window_properties & w_props, const Plot_data & data)
{
const Color canvas_color = w_props.get_canvas_color();
Plot_line_properties const * const * l_props = w_props.get_line_properties();

   loop(l, data.get_row_count())
       {
         const Plot_line_properties & lp = *l_props[l];

         enum { mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH };
         const uint32_t gc_l[] = { lp.line_color, lp.line_width };
         xcb_change_gc(pctx.conn, pctx.line, mask, gc_l);

         // draw lines between points
         //
         Pixel_XY last(0, 0);
         loop(n, data[l].get_N())
             {
               double vx, vy;   data.get_XY(vx, vy, l, n);
               const Pixel_XY P(w_props.valX2pixel(vx - w_props.get_min_X())
                                + w_props.get_origin_X(),
                                w_props.valY2pixel(vy - w_props.get_min_Y()));
               if (n)   draw_line(pctx, pctx.line, last, P);
               last = P;
             }

         // draw points...
         //
         const uint32_t gc_p[] = { lp.point_color, lp.point_size };
         xcb_change_gc(pctx.conn, pctx.point, mask, gc_p);
         loop(n, data[l].get_N())
             {
               double vx, vy;   data.get_XY(vx, vy, l, n);
               const Pixel_X px = w_props.valX2pixel(vx - w_props.get_min_X())
                                + w_props.get_origin_X();
               const Pixel_Y py = w_props.valY2pixel(vy - w_props.get_min_Y());
               draw_point(pctx, Pixel_XY(px, py), canvas_color, *l_props[l]);
             }
       }
}
//-----------------------------------------------------------------------------
Pixel_XY
Plot_window_properties::valXYZ2pixelXY(double X, double Y, double Z) const
{
const double phi = atan2(get_origin_Y(), get_origin_X());
const Pixel_Z pz = valZ2pixel(Z - get_min_Z());
const Pixel_X px = valX2pixel(X - get_min_X()) + get_origin_X() - pz * cos(phi);
const Pixel_X py = valY2pixel(Y - get_min_Y())                  + pz * sin(phi);

   return Pixel_XY(px, py);
}
//-----------------------------------------------------------------------------
/// draw the surface plot lines
void
draw_surface_lines(const Plot_context & pctx,
                   const Plot_window_properties & w_props,
                   const Plot_data & data)
{
const Color canvas_color = w_props.get_canvas_color();
Plot_line_properties const * const * l_props = w_props.get_line_properties();
const Plot_line_properties & lp0 = *l_props[0];

   // 1. setup graphic contexts
   //
enum { mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH };
const uint32_t gc_l[] = { lp0.line_color,        // XCB_GC_FOREGROUND
                          lp0.line_width };      // XCB_GC_LINE_WIDTH
   xcb_change_gc(pctx.conn, pctx.line, mask, gc_l);

const uint32_t gc_p[] = { lp0.point_color,       // XCB_GC_FOREGROUND
                          lp0.point_size };      // XCB_GC_LINE_WIDTH
   xcb_change_gc(pctx.conn, pctx.point, mask, gc_p);

   // 2. draw areas between plot lines...
   //
const double Hmin = w_props.get_min_Y();
const double dH = w_props.get_max_Y() - Hmin;   // 100%
const int verbosity = w_props.get_verbosity();

   loop(row, data.get_row_count() - 1)
   loop(col, data[row].get_N() - 1)
       {
         if (verbosity & SHOW_DATA)
            CERR << "B[" << row << ";" << col << "]"
                 << " X=" << data.get_X(row, col)
                 << " Y=" << data.get_Y(row, col)
                 << " Z=" << data.get_Z(row, col) << endl;

         if (!w_props.get_gradient().size())   continue; // no gradient
//       if (row != 3)   continue;   // show only given row
//       if (col != 0)   continue;   // show only given column

         const double   X0 = data.get_X(row, col);
         const double   Y0 = data.get_Y(row, col);
         const double   Z0 = data.get_Z(row, col);
         const double   H0 = (Y0 - Hmin)/dH;
         const Pixel_XY P0 = w_props.valXYZ2pixelXY(X0, Y0, Z0);

         const double   X1 = data.get_X(row, col + 1);
         const double   Y1 = data.get_Y(row, col + 1);
         const double   Z1 = data.get_Z(row, col + 1);
         const double   H1 = (Y1 - Hmin)/dH;
         const Pixel_XY P1 = w_props.valXYZ2pixelXY(X1, Y1, Z1);

         const double   X2 = data.get_X(row + 1, col);
         const double   Y2 = data.get_Y(row + 1, col);
         const double   Z2 = data.get_Z(row + 1, col);
         const double   H2 = (Y2 - Hmin)/dH;
         const Pixel_XY P2 = w_props.valXYZ2pixelXY(X2, Y2, Z2);

         const double   X3 = data.get_X(row + 1, col + 1);
         const double   Y3 = data.get_Y(row + 1, col + 1);
         const double   Z3 = data.get_Z(row + 1, col + 1);
         const double   H3 = (Y3 - Hmin)/dH;
         const Pixel_XY P3 = w_props.valXYZ2pixelXY(X3, Y3, Z3);


         /* the surface has 4 points P0, P1, P2, and P3, but they are not
            necessarily coplanar. We could draw the surface in 2 ways:

            A. find the intersection PM of P0-P3 and P1-P2 on the bottom
               plane Y=0, interpolate the respective heights, and draw
               4 triangles from PM to each of the four edges of the square

            B. fold the surface at its shorter edge into 2 triangles that
               have the edge in common. (triangles are always coplanar).

            Option B seems to look better.
         */

#if 0
         /* Option A...

            interpolate middle point PM with height HM and split the
            square P0-P1-P2-P3 into 4 triangles with common point PM
            and edges P0-P1, P1-P3, P3-P2, and P2-P0 respectively.
         */
         const double beta = intersection(X0,Z0, X1,Z1, X2,Z2, X3,Z3);
         const double   XM = X1 + beta*(X2 - X1);
         const double   YM = Y1 + beta*(Y2 - Y1);
         const double   ZM = Z1 + beta*(Z2 - Z1);
         const double   HM = (YM - Hmin)/dH;
         const Pixel_XY PM = w_props.valXYZ2pixelXY(XM, YM, ZM);

         draw_triangle(pctx, verbosity, PM, HM, P0, H0, P1, H1);
         draw_triangle(pctx, verbosity, PM, HM, P1, H1, P3, H3);
         draw_triangle(pctx, verbosity, PM, HM, P3, H3, P2, H2);
         draw_triangle(pctx, verbosity, PM, HM, P2, H2, P0, H0);
#else
         /* Option B...

            fold surface-square along its shorter edge
         */

         if (P0.distance2(P3) < P1.distance2(P2))   // P0-P3 is shorter
            {
              draw_triangle(pctx, verbosity, P1, H1, P0, H0, P3, H3);
              draw_triangle(pctx, verbosity, P2, H2, P0, H0, P3, H3);
            }
         else                                       // P1-P2 is shorter
            {
              draw_triangle(pctx, verbosity, P0, H0, P1, H1, P2, H2);
              draw_triangle(pctx, verbosity, P3, H3, P1, H1, P2, H2);
            }
#endif
       }

   // 3. draw lines between plot points
   //
   loop(row, data.get_row_count() - 1)
   loop(col, data[row].get_N() - 1)
       {
         if (verbosity & SHOW_DATA)
            CERR << "data[" << row << "," << col << "]"
                 << " X=" << data.get_X(row, col)
                 << " Y=" << data.get_Y(row, col)
                 << " Z=" << data.get_Z(row, col) << endl;

         const double X0 = data.get_X(row, col);
         const double Y0 = data.get_Y(row, col);
         const double Z0 = data.get_Z(row, col);
         const Pixel_XY P0 = w_props.valXYZ2pixelXY(X0, Y0, Z0);

         const double X1 = data.get_X(row, col + 1);
         const double Y1 = data.get_Y(row, col + 1);
         const double Z1 = data.get_Z(row, col + 1);
         const Pixel_XY P1 = w_props.valXYZ2pixelXY(X1, Y1, Z1);

         const double X2 = data.get_X(row + 1, col);
         const double Y2 = data.get_Y(row + 1, col);
         const double Z2 = data.get_Z(row + 1, col);
         const Pixel_XY P2 = w_props.valXYZ2pixelXY(X2, Y2, Z2);

         const double X3 = data.get_X(row + 1, col + 1);
         const double Y3 = data.get_Y(row + 1, col + 1);
         const double Z3 = data.get_Z(row + 1, col + 1);
         const Pixel_XY P3 = w_props.valXYZ2pixelXY(X3, Y3, Z3);

#if 0
         // debug: plot points as vertical lines
         //
         const Pixel_XY xy0 = w_props.valXYZ2pixelXY(data.get_X(row, col),
                                                     w_props.get_min_Y(),
                                                     data.get_Z(row, col));
         draw_line(pctx, pctx.line, xy0, P0);
         continue;
#endif
         draw_line(pctx, pctx.line, P0, P1);
         if (row == (data.get_row_count() - 2))   // last row
            draw_line(pctx, pctx.line, P2, P3);

         draw_line(pctx, pctx.line, P0, P2);
         if (col == (data[row].get_N() - 2))   // last column
            draw_line(pctx, pctx.line, P1, P3);
       }

   // 4. draw plot points
   //
   loop(row, data.get_row_count())
   loop(col, data[row].get_N())
       {
         const Pixel_XY P0 = w_props.valXYZ2pixelXY(data.get_X(row, col),
                                                    data.get_Y(row, col),
                                                    data.get_Z(row, col));
         draw_point(pctx, P0, canvas_color, *l_props[0]);
       }
}
//-----------------------------------------------------------------------------
void
do_plot(const Plot_context & pctx,
        const Plot_window_properties & w_props, const Plot_data & data)
{
   // draw the background
   //
const Color canvas_color = w_props.get_canvas_color();
   {
     enum { mask = XCB_GC_FOREGROUND };
     xcb_change_gc(pctx.conn, pctx.line, mask, &canvas_color);
     xcb_rectangle_t rect = { 0, 0, w_props.get_window_width(),
                                    w_props.get_window_height() };
     xcb_poly_fill_rectangle(pctx.conn, pctx.window, pctx.line, 1, &rect);
   }

const bool surface = data.is_surface_plot();
   draw_Y_grid(pctx, w_props, surface);
   draw_X_grid(pctx, w_props, surface);
   if (surface)   draw_Z_grid(pctx, w_props);
   draw_legend(pctx, w_props);
   if  (surface)   draw_surface_lines(pctx, w_props, data);
   else            draw_plot_lines(pctx, w_props, data);
}
//-----------------------------------------------------------------------------
xcb_atom_t
get_atom_ID(xcb_connection_t * conn, int only_existing, const char * name)
{
xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_existing,
                                                  strlen(name), name);
xcb_intern_atom_reply_t & reply = *xcb_intern_atom_reply(conn, cookie, 0);
   return reply.atom;
}
//-----------------------------------------------------------------------------
void *
plot_main(void * vp_props)
{
Plot_window_properties & w_props =
      *reinterpret_cast<Plot_window_properties *>(vp_props);

Plot_context pctx(w_props);

const Plot_data & data = w_props.get_plot_data();

   // open a connection to the X server
   //
# if XCB_WINDOWS_WITH_UTF8_CAPTIONS
Display * dpy = XOpenDisplay(0);
   pctx.conn = XGetXCBConnection(dpy);

# else   // not XCB_WINDOWS_WITH_UTF8_CAPTIONS
   pctx.conn = xcb_connect(0, 0);
# endif  // XCB_WINDOWS_WITH_UTF8_CAPTIONS

   if (pctx.conn == 0 || xcb_connection_has_error(pctx.conn))
      {
        xcb_disconnect(pctx.conn);
        MORE_ERROR() << "could not connect to the X-server ";
        DOMAIN_ERROR;
      }

   // get the first screen
   //
const xcb_setup_t * setup = xcb_get_setup(pctx.conn);
   pctx.screen = xcb_setup_roots_iterator(setup).data;

   // create a window
   //
   pctx.window = xcb_generate_id(pctx.conn);
   enum { mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK};
   const uint32_t values[] = { w_props.get_canvas_color(),
                               XCB_EVENT_MASK_EXPOSURE
                             | XCB_EVENT_MASK_PROPERTY_CHANGE
                             | XCB_EVENT_MASK_STRUCTURE_NOTIFY };
     xcb_create_window(pctx.conn,                       // connection
                       XCB_COPY_FROM_PARENT,            // depth
                       pctx.window,                     // window Id
                       pctx.screen->root,               // parent window
                       w_props.get_pw_pos_X(),          // X position on screen
                       w_props.get_pw_pos_Y(),          // Y position on screen
                       w_props.get_window_width(),
                       w_props.get_window_height(),
                       w_props.get_border_width(),
                       XCB_WINDOW_CLASS_INPUT_OUTPUT,   // class
                       pctx.screen->root_visual,        // visual
                       mask, values);                   // mask and values

   // create some graphic contexts
   //
   pctx.fill  = xcb_generate_id(pctx.conn);
   pctx.line  = xcb_generate_id(pctx.conn);
   pctx.point = xcb_generate_id(pctx.conn);
   pctx.font  = xcb_generate_id(pctx.conn);
   pctx.text = setup_font_gc(pctx, "fixed");

   {
     enum { mask = XCB_GC_GRAPHICS_EXPOSURES,
            mask_fill = mask | XCB_GC_LINE_WIDTH
          };
     const uint32_t values[] = { 2,   // XCB_GC_LINE_WIDTH
                                 0    // XCB_GC_GRAPHICS_EXPOSURES
                               };

     xcb_create_gc(pctx.conn, pctx.fill, pctx.screen->root, mask_fill, values);
     xcb_create_gc(pctx.conn, pctx.line,  pctx.screen->root, mask, values + 1);
     xcb_create_gc(pctx.conn, pctx.point, pctx.screen->root, mask, values + 1);
   }

   {

# if XCB_WINDOWS_WITH_UTF8_CAPTIONS

// size (and position) hints for the window manager
XSizeHints size_hints;
   memset(&size_hints, 0, sizeof(XSizeHints));
   size_hints.flags       = USPosition | PPosition;
   size_hints.x           = w_props.get_pw_pos_X();
   size_hints.base_width  = w_props.get_pw_pos_X();
   size_hints.y           = w_props.get_pw_pos_Y();
   size_hints.base_height = w_props.get_pw_pos_Y();

static char * caption = strdup(w_props.get_caption().c_str());
Xutf8SetWMProperties(dpy, pctx.window, caption,
                     "icon", 0, 0, &size_hints, 0, 0);
// free(caption);

# else   // not XCB_WINDOWS_WITH_UTF8_CAPTIONS

     xcb_change_property(pctx.conn, XCB_PROP_MODE_REPLACE, pctx.window,
                       XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                       w_props.get_caption().size(),
                       w_props.get_caption().c_str());

# endif  // XCB_WINDOWS_WITH_UTF8_CAPTIONS

   }

   // tell the window manager that we will handle WM_DELETE_WINDOW events...
   //
const xcb_atom_t window_deleted = get_atom_ID(pctx.conn, 0, "WM_DELETE_WINDOW");
const xcb_atom_t WM_protocols   = get_atom_ID(pctx.conn, 1, "WM_PROTOCOLS");
   xcb_change_property(pctx.conn, XCB_PROP_MODE_REPLACE, pctx.window,
                        WM_protocols, XCB_ATOM_ATOM,
                        /* bits */ 32, 1, &window_deleted);

   // remember who has the focus before mapping a new window
   //
const xcb_get_input_focus_reply_t * focusReply =
   xcb_get_input_focus_reply(pctx.conn, xcb_get_input_focus(pctx.conn), 0);

   // map the window on the screen and flush
   //
   xcb_map_window(pctx.conn, pctx.window);
   xcb_flush(pctx.conn);

   // X main loop
   //
   for (;;)
      {
        xcb_generic_event_t * event = xcb_poll_for_event(pctx.conn);
        if (event == 0)   // nothing happened
           {
             pthread_t thread = pthread_self();
             bool zombie = true;
             sem_wait(Quad_PLOT::plot_threads_sema);
                loop(pt, Quad_PLOT::plot_threads.size())
                    if (Quad_PLOT::plot_threads[pt] == thread)
                       {
                         zombie = false;
                         break;
                       }
             sem_post(Quad_PLOT::plot_threads_sema);

             if (zombie)
                {
                  free(event);
                  xcb_disconnect(pctx.conn);
                  delete &w_props;
                  return 0;
                }

             usleep(200000);
             continue;
           }

        switch(event->response_type & ~0x80)
           {
             case XCB_EXPOSE:             // 12
                  if (w_props.get_verbosity() & SHOW_EVENTS)
                     CERR << "\n*** XCB_EXPOSE " << endl;
                  do_plot(pctx, w_props, data);
                  if (focusReply)
                     {
                       /* this is the first XCB_EXPOSE event. X has moved
                          the focus way from our caller to the newly
                          created focus window (which is somehat annoying).

                          Return the focus to the window from which we have
                          stolen it.
                        */
                       xcb_set_input_focus(pctx.conn, XCB_INPUT_FOCUS_PARENT,
                                           focusReply->focus, XCB_CURRENT_TIME);
                       focusReply = 0;
                    }
                 xcb_flush(pctx.conn);
                 break;

             case XCB_UNMAP_NOTIFY:       // 18
                  if (w_props.get_verbosity() & SHOW_EVENTS)
                     CERR << "\n*** XCB_UNMAP_NOTIFY ***\n";
                  break;

             case XCB_MAP_NOTIFY:         // 19
                  if (w_props.get_verbosity() & SHOW_EVENTS)
                     CERR << "\n*** XCB_MAP_NOTIFY ***\n";
                  break;

             case XCB_REPARENT_NOTIFY:    // 21
                  break;

             case XCB_CONFIGURE_NOTIFY:   // 22
                  if (focusReply)   break;   // not yet exposed

                  {
                    const xcb_configure_notify_event_t * notify =
                          reinterpret_cast<const xcb_configure_notify_event_t *>
                             (event);
                    w_props.set_window_size(notify->width, notify->height);
                  }
             break;

             case XCB_PROPERTY_NOTIFY:     // 28
                  {
                    const xcb_property_notify_event_t * notify =
                          reinterpret_cast<const xcb_property_notify_event_t*>
                          (event);
                    if (w_props.get_verbosity() & SHOW_EVENTS)
                       CERR << "\n*** XCB_PROPERTY_NOTIFY"
                               " atom=" << int(notify->atom) <<
                               ", state=" << int(notify->state) << " ***\n";
                  }
             break;

             case XCB_CLIENT_MESSAGE:     // 33
                  if (reinterpret_cast<xcb_client_message_event_t *>(event)
                           ->data.data32[0] == window_deleted)
                     {
                       // CERR << "Killed!" << endl;
                       free(event);
                       xcb_disconnect(pctx.conn);
                       delete &w_props;
                       sem_wait(Quad_PLOT::plot_threads_sema);
                          const int count = Quad_PLOT::plot_threads.size();
                          const pthread_t thread = pthread_self();
                          loop(pt, count)
                              if (Quad_PLOT::plot_threads[pt] == thread)
                                 {
                                   Quad_PLOT::plot_threads[pt] =
                                      Quad_PLOT::plot_threads[count - 1];
                                   Quad_PLOT::plot_threads.pop_back();
                                   break;
                                 }
                       sem_post(Quad_PLOT::plot_threads_sema);
                       goto done;
                     }
                  break;

             default:
                if (w_props.get_verbosity() & SHOW_EVENTS)
                   CERR << "unexpected event type "
                        << int(event->response_type)
                         << " (ignored)" << endl;
           }

        free(event);
      }

done:
   // free the text_gc
   //
   testCookie(xcb_free_gc(pctx.conn, pctx.text), pctx.conn, "can't free gc");

   // close font
   {
     xcb_void_cookie_t cookie = xcb_close_font_checked(pctx.conn, pctx.font);
     testCookie(cookie, pctx.conn, "can't close font");
   }

   return 0;
}
//=============================================================================
Quad_PLOT::Quad_PLOT()
  : QuadFunction(TOK_Quad_PLOT),
    verbosity(0)
{
    __sem_init(plot_threads_sema, 0, 1);
}
//-----------------------------------------------------------------------------
Quad_PLOT::~Quad_PLOT()
{
   __sem_destroy(plot_threads_sema);
}
//-----------------------------------------------------------------------------
Token
Quad_PLOT::eval_AB(Value_P A, Value_P B)
{
   if (B->get_rank() > 3)        RANK_ERROR;
   if (B->element_count() < 2)   LENGTH_ERROR;

   // plot window with default attributes
   //
Plot_data * data = setup_data(B.get());
   if (data == 0)   DOMAIN_ERROR;

Plot_window_properties * w_props = new Plot_window_properties(data, verbosity);
   if (w_props == 0)
      {
        delete data;
         WS_FULL;
      }

   if (A->get_rank() > 1)   RANK_ERROR;

const ShapeItem len_A = A->element_count();
   if (len_A < 1)   LENGTH_ERROR;

const APL_Integer qio = Workspace::get_IO();

   loop(a, len_A)
       {
         const Cell & cell_A = A->get_ravel(a);
         if (!cell_A.is_pointer_cell())
            {
               MORE_ERROR() << "A[" << (a + qio)
                            << "] is not a string in A ⎕PLOT B";
               DOMAIN_ERROR;
            }

         const Value * attr = cell_A.get_pointer_value().get();
         if (!attr->is_char_string())
            {
               MORE_ERROR() << "A[" << (a + qio)
                            << "] is not a string in A ⎕PLOT B";
               DOMAIN_ERROR;
            }

         UCS_string ucs = attr->get_UCS_ravel();
         ucs.remove_leading_and_trailing_whitespaces();
         if (ucs.size() == 0)         continue;
         if (Avec::is_comment(ucs[0]))      continue;
         UTF8_string utf(ucs);
         const char * error = w_props->set_attribute(utf.c_str());
         if (error)
            {
              MORE_ERROR() << error << " in ⎕PLOT attribute '" << ucs << "'";
              DOMAIN_ERROR;
            }
       }

   if (w_props->update(verbosity))   DOMAIN_ERROR;
   return Token(TOK_APL_VALUE1, do_plot_data(w_props, data));
}
//-----------------------------------------------------------------------------
Token
Quad_PLOT::eval_B(Value_P B)
{
   if (B->get_rank() == 0 && !B->get_ravel(0).is_pointer_cell())
      {
        // scalar argument: plot window control
        union
           {
             int64_t B0;
             pthread_t thread;
           } u;

        u.B0 = B->get_ravel(0).get_int_value();
        if (u.B0 == 0)                 // reset plot verbosity
           {
             verbosity = 0;
             CERR << "⎕PLOT verbosity turned off" << endl;
             return Token(TOK_APL_VALUE1, Idx0(LOC));
           }

        if (u.B0 == -1)                // enable SHOW_EVENTS
           {
             verbosity |= SHOW_EVENTS;
             CERR << "⎕PLOT will show X events " << endl;
             return Token(TOK_APL_VALUE1, Idx0(LOC));
           }

        if (u.B0 == -2)                // enable SHOW_DATA
           {
             verbosity |= SHOW_DATA;
             CERR << "⎕PLOT will  show APL data " << endl;
             return Token(TOK_APL_VALUE1, Idx0(LOC));
           }

        if (u.B0 == -3)   // close all windows
           {
             while (plot_threads.size())
                {
                  plot_threads.pop_back();
                }
           }

        if (u.B0 == -4)                // enable SHOW_DRAW
           {
             verbosity |= SHOW_DRAW;
             CERR << "⎕PLOT will  show rendering details " << endl;
             return Token(TOK_APL_VALUE1, Idx0(LOC));
           }

        bool found = false;
        sem_wait(plot_threads_sema);
           loop(pt, Quad_PLOT::plot_threads.size())
               {
                 if (Quad_PLOT::plot_threads[pt] != u.thread)   continue;

                 plot_threads[pt] = plot_threads[plot_threads.size() - 1];
                 plot_threads.pop_back();
                 found = true;
                 break;
               }
        sem_post(plot_threads_sema);
        return Token(TOK_APL_VALUE1, IntScalar(found ? u.B0 : 0, LOC));
      }

   if (B->get_rank() == 1 && B->element_count() == 0)
      {
        help();
        return Token(TOK_APL_VALUE1, Idx0(LOC));
      }

   if (B->get_rank() > 3)   RANK_ERROR;

   // plot window with default attributes
   //
Plot_data * data = setup_data(B.get());
   if (data == 0)   DOMAIN_ERROR;

Plot_window_properties * w_props = new Plot_window_properties(data, verbosity);
   if (w_props == 0)
      {
        delete data;
         WS_FULL;
      }

   return Token(TOK_APL_VALUE1, do_plot_data(w_props, data));
}
//-----------------------------------------------------------------------------
Plot_data *
Quad_PLOT::setup_data(const Value * B)
{
   // check data. We expect B to be either:
   //  1a. a numeric vector (for a single plot line), or
   //  1b. a numeric matrix (for one plot line for each row of the matrix), or
   //  2a. a scalar of a nested numeric vector (for a single plot line), or
   //  2b. a vector of nested numeric vectors (for one plot line per item)
   //  3.  a 3-dimensional real vector for surface plots
   //
   if (B->is_scalar())   // 2a. → 1a.
      {
         if (!B->get_ravel(0).is_pointer_cell())   DOMAIN_ERROR;
         B = B->get_ravel(0).get_pointer_value().get();
      }

const APL_Integer qio = Workspace::get_IO();
Plot_data * data = 0;
   if (B->get_rank() == 3)   // case 3.
      {
        const ShapeItem planes = B->get_shape_item(0);   // (X), Y, and (Z)
        const ShapeItem rows = B->get_shape_item(1);
        const ShapeItem cols = B->get_shape_item(2);
        if (planes < 1 || planes > 3)    LENGTH_ERROR;
        if (rows < 1)                    LENGTH_ERROR;
        if (cols < 1)                    LENGTH_ERROR;
        const ShapeItem data_points = rows * cols;

        if (planes == 3)   // X, Y, and Z
           {
             double * X = new double[3*data_points];
             double * Y = X + data_points;
             double * Z = Y + data_points;
             if (!X)   WS_FULL;

             data = new Plot_data(rows);
             loop(r, rows)
                 {
                   loop(c, cols)
                       {
                         const ShapeItem p = c + r*cols;
                         const Cell & cX = B->get_ravel(p);
                         const Cell & cY = B->get_ravel(p + data_points);
                         const Cell & cZ = B->get_ravel(p + 2*data_points);

                         if (!(cX.is_integer_cell() ||
                               cX.is_real_cell()))   DOMAIN_ERROR;
                         if (!(cY.is_integer_cell() ||
                               cY.is_real_cell()))   DOMAIN_ERROR;
                         if (!(cZ.is_integer_cell() ||
                               cZ.is_real_cell()))   DOMAIN_ERROR;

                         X[p] = cX.get_real_value();
                         Y[p] = cY.get_real_value();
                         Z[p] = cZ.get_real_value();
                       }
                   const double * pX = X + r*cols;
                   const double * pY = Y + r*cols;
                   const double * pZ = Z + r*cols;
                   const Plot_data_row * pdr = new Plot_data_row(pX, pY, pZ,
                                                                 r, cols);
                   data->add_row(pdr);
                 }
           }
        else if (planes == 2)   // X and Y, but no Z
           {
             double * X = new double[2*data_points];
             double * Y = X + data_points;
             if (!X)   WS_FULL;

             data = new Plot_data(rows);
             loop(r, rows)
                 {
                   loop(c, cols)
                       {
                         const ShapeItem p = c + r*cols;
                         const Cell & cX = B->get_ravel(p);
                         const Cell & cY = B->get_ravel(p + data_points);

                         if (!(cX.is_integer_cell() ||
                               cX.is_real_cell()))   DOMAIN_ERROR;
                         if (!(cY.is_integer_cell() ||
                               cY.is_real_cell()))   DOMAIN_ERROR;

                         X[p] = cX.get_real_value();
                         Y[p] = cY.get_real_value();
                       }
                   const double * pX = X + r*cols;
                   const double * pY = Y + r*cols;
                   const Plot_data_row * pdr = new Plot_data_row(pX, pY, 0,
                                                                 r, cols);
                   data->add_row(pdr);
                 }
           }
        else                    // Y, but no X or Z
           {
Q(LOC)
             double * Y = new double[data_points];
             if (!Y)   WS_FULL;

             data = new Plot_data(rows);
             loop(r, rows)
                 {
                   loop(c, cols)
                       {
                         const ShapeItem p = c + r*cols;
                         const Cell & cY = B->get_ravel(p);

                         if (!(cY.is_integer_cell() ||
                               cY.is_real_cell()))   DOMAIN_ERROR;

                         Y[p] = cY.get_real_value();
                       }
                   const double * pY = Y + r*cols;
                   const Plot_data_row * pdr = new Plot_data_row(0, pY, 0,
                                                                 r, cols);
                   data->add_row(pdr);
                 }
           }
        data->surface = true;
      }
   else if (B->get_ravel(0).is_pointer_cell())   // 2b.
      {
        ShapeItem data_points = 0;
        if (B->get_rank() > 1)   RANK_ERROR;
        const ShapeItem rows = B->get_cols();
        loop(r, rows)
            {
              const Value * vrow = B->get_ravel(r).get_pointer_value().get();
              if (vrow->get_rank() > 1)   RANK_ERROR;
              const ShapeItem row_len = vrow->element_count();
              data_points += row_len;
              loop(rb, row_len)
                  {
                    if (!vrow->get_ravel(rb).is_numeric())   DOMAIN_ERROR;
                  }
            }
        double * X = new double[3*data_points];
        double * Y = X + data_points;
        double * Z = Y + data_points;
        if (!X)   WS_FULL;

        ShapeItem idx = 0;
        loop(r, rows)
            {
              const Value * vrow = B->get_ravel(r).get_pointer_value().get();
              loop(v, vrow->element_count())
                  {
                    const Cell & cB = vrow->get_ravel(v);
                    if (cB.is_complex_cell())
                       {
                         X[idx] = cB.get_real_value();
                         Z[idx] = Y[idx] = cB.get_imag_value();
                       }
                    else
                       {
                         X[idx] = qio + v;
                         Z[idx] = Y[idx] = cB.get_real_value();
                       }
                    ++idx;
                  }
            }
        data = new Plot_data(rows);
        idx = 0;
        loop(r, rows)
            {
              const double * pX = X + idx;
              const double * pY = Y + idx;
              const double * pZ = Z + idx;
              const Value * vrow = B->get_ravel(r).get_pointer_value().get();
              const ShapeItem row_len = vrow->element_count();
              const Plot_data_row * pdr = new Plot_data_row(pX, pY, pZ, r,
                                                            row_len);
              data->add_row(pdr);
              idx += row_len;
            }
      }
   else
      {
        const ShapeItem cols_B = B->get_cols();
        const ShapeItem rows_B = B->get_rows();
        const ShapeItem len_B = rows_B * cols_B;

        loop(b, len_B)
            {
              if (!B->get_ravel(b).is_numeric())   return 0;
            }

        // split B into X=real B, Y=imag Y

        double * X = new double[3*len_B];
        double * Y = X + len_B;
        double * Z = Y + len_B;
        if (!X)   WS_FULL;

        loop(b, len_B)
            {
              const Cell & cB = B->get_ravel(b);
              if (cB.is_complex_cell())
                 {
                   X[b] = cB.get_real_value();
                   Z[b] = Y[b] = cB.get_imag_value();
                 }
              else
                 {
                   X[b] = qio + b % cols_B;
                   Z[b] = Y[b] = cB.get_real_value();
                 }
            }

        data = new Plot_data(rows_B);
        loop(r, rows_B)
            {
              const double * pX = X + r*cols_B;
              const double * pY = Y + r*cols_B;
              const double * pZ = Z + r*cols_B;
              const Plot_data_row * pdr = new Plot_data_row(pX, pY, pZ,
                                                            r, cols_B);
              data->add_row(pdr);
            }

      }
   return data;
}
//-----------------------------------------------------------------------------
Value_P
Quad_PLOT::do_plot_data(Plot_window_properties * w_props,
                        const Plot_data * data)
{
   w_props->set_verbosity(verbosity);
   verbosity > 0 && w_props->print(CERR);

union
{
   pthread_t thread;
   int64_t   ret;
} u;
   u.ret = 0;
   pthread_create(&u.thread, 0, plot_main, w_props);
   sem_wait(Quad_PLOT::plot_threads_sema);
      plot_threads.push_back(u.thread);
   sem_post(Quad_PLOT::plot_threads_sema);
   return IntScalar(u.ret, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PLOT::help() const
{
   CERR <<
"\n"
"   ⎕PLOT Usage:\n"
"\n"
"   ⎕PLOT B     plot B with default attribute values\n"
"   A ⎕PLOT B   plot B with attributes specified by A\n"
"\n"
"   A is a nested vector of strings.\n"
"   Each string in A has the form \"Attribute: Value\"\n"
"   Colors are specified either as #RGB or as #RRGGBB or as RR GG BB)\n"
"\n"
"   The attributes understood by ⎕PLOT and their default values are:\n"
"\n"
"   1. Global (plot window) Attributes:\n"
"\n";

   CERR << left;

# define gdef(ty,  na,  val, descr)           \
   CERR << setw(20) << #na ":  " << setw(14) \
        << Plot_data::ty ## _to_str(val) << " (" << #descr << ")" << endl;
# include "Quad_PLOT.def"

   CERR <<
"\n"
"color_level-P:      (none)         "
                    "(color gradient at P% (surface plots only))\n"
"\n"
"   2. Local (plot line N) Attributes:\n"
"\n";

# define ldef(ty,  na,  val, descr)             \
   CERR << setw(20) << #na "-N:  " << setw(14) \
        << Plot_data::ty ## _to_str(val) << " (" << #descr << ")" << endl;
# include "Quad_PLOT.def"

   CERR << right;
}
//-----------------------------------------------------------------------------
#endif // defined(WHY_NOT)

