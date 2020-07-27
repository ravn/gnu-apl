/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2018-2020  Dr. Jürgen Sauermann

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

#include <errno.h>
#include <signal.h>

#include <iostream>
#include <iomanip>

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

sem_t __plot_window_sema;
sem_t * Quad_PLOT::plot_window_sema = &__plot_window_sema;

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

# include <X11/Xlib.h>
# include <X11/Xutil.h>
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

using namespace std;

#include "Plot_data.hh"
#include "Plot_line_properties.hh"
#include "Plot_window_properties.hh"

// the pthread that handles one plot window.
extern void * plot_main(void * vp_props);

//=============================================================================
Quad_PLOT::Quad_PLOT()
  : QuadFunction(TOK_Quad_PLOT),
    verbosity(0)
{
    __sem_init(plot_threads_sema, 0, 1);
    __sem_init(plot_window_sema, 1, 0);
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
   sem_wait(plot_threads_sema);
      plot_threads.push_back(u.thread);
   sem_post(plot_threads_sema);

   sem_wait(plot_window_sema);   // blocks until XCB_EXPOSE
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

