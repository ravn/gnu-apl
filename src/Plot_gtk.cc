
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

/// the pthread that handles one plot window.
extern void * plot_main(void * vp_props);

static int verbosity;

#include "../config.h"
#if HAVE_GTK3

#include <X11/Xlib.h>
# include <gtk/gtk.h>

# include "Plot_data.hh"
# include "Plot_line_properties.hh"
# include "Plot_window_properties.hh"

# include "ComplexCell.hh"
# include "FloatCell.hh"
# include "Quad_PLOT.hh"
# include "Workspace.hh"

const char * FONT_NAME = "sans-serif";
enum {  FONT_SIZE = 10 };

// ===========================================================================
const char * gui =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!-- Generated with glade 3.22.1 -->\n"
"<interface>\n"
"  <requires lib=\"gtk+\" version=\"3.20\"/>\n"
"  <object class=\"GtkWindow\" id=\"top-level-window\">\n"
"    <property name=\"can_focus\">False</property>\n"
"    <signal name=\"destroy\" handler=\"plot_destroyed\" swapped=\"no\"/>\n"
"    <child>\n"
"      <placeholder/>\n"
"    </child>\n"
"    <child>\n"
"      <object class=\"GtkDrawingArea\" id=\"canvas\">\n"
"        <property name=\"visible\">True</property>\n"
"        <property name=\"can_focus\">False</property>\n"
"        <signal name=\"draw\" handler=\"draw_callback\" swapped=\"no\"/>\n"
"      </object>\n"
"    </child>\n"
"  </object>\n"
"</interface>\n"
;

// plot window CSS (none yet)

const char * css =
"\n"
;
//-------------------------------------------------------------------------------
struct Plot_context
{
   /// constructor
   Plot_context(const Plot_window_properties & pwp)
   : w_props(pwp),
     builder(0),
     css_provider(0),
     window(0),
     canvas(0),
     drawing_area(0)
   {}

   /// return the required width of the entire plot area
   int get_total_width() const
       {
         return w_props.get_pa_width()
              + w_props.get_pa_border_L()
              + w_props.get_origin_X()
              + w_props.get_pa_border_R();
       }

   /// return the required height of the entire plot area
   int get_total_height() const
       {
         return w_props.get_pa_height()
              + w_props.get_pa_border_T()
              + w_props.get_origin_Y()
              + w_props.get_pa_border_B();
       }

   /// the window properties (as choosen by the user)
   const Plot_window_properties & w_props;

   /// the GtkBuilder that contains the gui
   GtkBuilder * builder;

   /// the GtkCssProvider that contains the CSS for the gui
   GtkCssProvider * css_provider;

   /// the pthread_t that handles this Plot_context
   pthread_t thread;

   /// the window of this Plot_context
   GObject * window;

   /// the drawing area of this Plot_context (in GUI)
   GObject * canvas;

   /// the drawing area in the window of this Plot_context
   GtkWidget * drawing_area;
};

/// all Plot_contexts (= all open windows)
static vector<Plot_context *> all_plot_contexts;

//-----------------------------------------------------------------------------
inline void
cairo_set_RGB_source(cairo_t * cr, Color color)
{
   cairo_set_source_rgba(cr, (color >> 16 & 0xFF) / 255.0,
                             (color >>  8 & 0xFF) / 255.0,
                             (color       & 0xFF) / 255.0, 1.0);   // opaque
}
//-----------------------------------------------------------------------------
void
draw_circle(cairo_t * cr, Pixel_XY P0, Color color, int size)
{
   cairo_set_RGB_source(cr, color);
   cairo_arc(cr, P0.x, P0.y, 1.0*size, 0.0, 2*M_PI);
   cairo_fill(cr);
}
//-----------------------------------------------------------------------------
void
draw_delta(cairo_t * cr, Pixel_XY P0, bool up, Color color, int size)
{
const double l = 1.0*size;        // ∆ center to top vertex
const double m = 0.866025404*l;   // ∆ center to base
const double s = 0.5*l;           // ∆ base middle to left/right vertex

   cairo_set_RGB_source(cr, color);

   if (up)   // ∆
      {
        cairo_move_to(cr, P0.x,     P0.y + l);   // top vertex
        cairo_line_to(cr, P0.x + m, P0.y - s);   // right vertex
        cairo_line_to(cr, P0.x - m, P0.y - s);   // left vertex
        cairo_close_path(cr);                    // back to top
      }
   else      // ∇
      {
        cairo_move_to(cr, P0.x,     P0.y - l);   // bottom vertex
        cairo_line_to(cr, P0.x + m, P0.y + s);   // right vertex
        cairo_line_to(cr, P0.x - m, P0.y + s);   // left vertex
        cairo_close_path(cr);                    // back to top
      }

   cairo_fill(cr);
}
//-----------------------------------------------------------------------------
void
draw_quad(cairo_t * cr, Pixel_XY P0, bool caro, Color color, int size)
{
   cairo_set_RGB_source(cr, color);

   if (caro)   // ◊
      {
        const double dlta = 2.0 * size;
        cairo_move_to(cr, P0.x,        P0.y + dlta);   // top vertex
        cairo_line_to(cr, P0.x + dlta, P0.y);          // left vertex
        cairo_line_to(cr, P0.x,        P0.y - dlta);   // top vertex
        cairo_line_to(cr, P0.x - dlta, P0.y);          // left vertex
        cairo_close_path(cr);                          // back to top
      }
   else        // □
      {
        const double dlta = 1.414213562 * size;
        cairo_move_to(cr, P0.x - dlta,  P0.y + dlta);   // top left vertex
        cairo_line_to(cr, P0.x + dlta,  P0.y + dlta);   // top right vertex
        cairo_line_to(cr, P0.x + dlta,  P0.y - dlta);   // bottom right vertex
        cairo_line_to(cr, P0.x - dlta,  P0.y - dlta);   // bottom left vertex
        cairo_close_path(cr);                           // back to top
      }

   cairo_fill(cr);
}
//-----------------------------------------------------------------------------
void
draw_point(cairo_t * cr, const Plot_context & pctx, Pixel_XY P0, int point_style,
           const Color outer_color, int outer_dia,
           const Color inner_color, int inner_dia)
{
   if (point_style == 1)        // circle
      {
        draw_circle(cr, P0, outer_color, outer_dia);
        if (inner_dia)   draw_circle(cr, P0, inner_color, inner_dia);
      }
   else if (point_style == 2)   // triangle ∆
      {
        draw_delta(cr, P0, true, outer_color, outer_dia);
        if (inner_dia)   draw_delta(cr, P0, true, inner_color, inner_dia);
      }
   else if (point_style == 3)   // triangle ∇
      {
        draw_delta(cr, P0, false, outer_color, outer_dia);
        if (inner_dia)   draw_delta(cr, P0, false, inner_color, inner_dia);
      }
   else if (point_style == 4)   // caro ◊
      {
        draw_quad(cr, P0, true, outer_color, outer_dia);
        if (inner_dia)   draw_quad(cr, P0, true, inner_color, inner_dia);
      }
   else if (point_style == 5)   // square □
      {
        draw_quad(cr, P0, false, outer_color, outer_dia);
        if (inner_dia)   draw_quad(cr, P0, false, inner_color, inner_dia);
      }
   else
      {
        CERR << "Invalid point style " << point_style;
      }
}
//-----------------------------------------------------------------------------
void
draw_line(cairo_t * cr, const Plot_context & pctx, Color color,
          int line_width, const Pixel_XY & P0, const Pixel_XY & P1)
{
   cairo_set_RGB_source(cr, color);

   cairo_set_line_width(cr, 2.0*line_width);   // 1 pixel = 2 cairo
   cairo_move_to(cr, P0.x, P0.y);
   cairo_line_to(cr, P1.x, P1.y);
   cairo_stroke(cr);
}
//-----------------------------------------------------------------------------
const char *
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
cairo_string_size(double & lx, double & ly, cairo_t * cr, const char * label,
                  const char * font_name, double font_size)
{
  cairo_new_path(cr);
  cairo_select_font_face(cr, font_name, CAIRO_FONT_SLANT_NORMAL,
                                        CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(cr, font_size);

  cairo_move_to(cr, 0.0, 0.0);
  cairo_text_path(cr, label);

  cairo_get_current_point(cr, &lx, &ly);
  if (ly == 0.0)   ly = font_size;   // cairo bug
  cairo_new_path(cr);
}
//-----------------------------------------------------------------------------
void
draw_text(cairo_t * cr, const Plot_context & pctx,
          const char * label, const Pixel_XY & xy)
{
  cairo_move_to(cr, xy.x, xy.y);
  cairo_select_font_face(cr, FONT_NAME,
                             CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(cr, FONT_SIZE);

  ///  cairo_show_text(cr, label);
  cairo_text_path(cr, label);
  cairo_fill(cr);
}
//-------------------------------------------------------------------------------
void
draw_legend(cairo_t * cr, Plot_context & pctx, bool surface_plot)
{
const Plot_window_properties & w_props = pctx.w_props;
Plot_line_properties const * const * l_props = w_props.get_line_properties();

const int line_count = surface_plot ? 1 : w_props.get_line_count();

   // compute the length of the longest legend string
   //
  cairo_select_font_face(cr, FONT_NAME, CAIRO_FONT_SLANT_NORMAL,
                                        CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, FONT_SIZE);

double ly = FONT_SIZE;
double longest_len = 0.0;
   for (int l = 0; l < line_count; ++l)
       {
         double lx = 0.0;
         cairo_string_size(lx, ly, cr,
                           l_props[l]->get_legend_name().c_str(),
                           FONT_NAME, FONT_SIZE);
         if (longest_len < lx)   longest_len = lx;
       }

const Color canvas_color = w_props.get_canvas_color();
// const Color canvas_color = 0xB0B0B0;

  /*
                             ┌────────────────────────┐
   every legend looks like:  │  ---o---  legend_name  │  -- y0
                             └────────────────────────┘
                                |  |  |  |
                               x0 x1 x2 xt
  */

int x0 = w_props.get_legend_X();               // left of    --o--
   if (surface_plot)   x0 += w_props.get_origin_X();

const int x1 = x0 + 30;                              // point o in --o--
const int x2 = x1 + 30;                              // end of     --o--
const int xt = x2 + 10;                              // text after --o--
const int xe = xt + longest_len;                     // end of legend_name

const int y0 = w_props.get_legend_Y();               // y of first legends
const int dy = w_props.get_legend_dY();

   // draw legend background
   {
     // clear the background with a 10 px border around the items
     //
     const double ly2 = 0.5*ly;

     enum { BORDER = 10 };   // border around legend block
     const double X0 = x0                             - BORDER;
     const double X1 = xe                             + BORDER;
     const double Y0 = y0 - ly2                       - BORDER;
     const double Y1 = y0 + ly2 + dy*(line_count - 1) + BORDER;

     cairo_set_RGB_source(cr, canvas_color);
     cairo_move_to(cr, X0, Y0);
     cairo_line_to(cr, X1, Y0);
     cairo_line_to(cr, X1, Y1);
     cairo_line_to(cr, X0, Y1);
     cairo_close_path(cr);
     cairo_fill(cr);
   }

   for (int l = 0; l < line_count; ++l)
       {
         const Plot_line_properties & lp = *l_props[l];

         const Pixel_Y y1 = y0 + l*dy;

         draw_line(cr, pctx, lp.get_line_color(), lp.get_line_width(),
                   Pixel_XY(x0, y1), Pixel_XY(x2, y1));
         draw_point(cr, pctx, Pixel_XY(x1, y1), lp.get_point_style(),
                    lp.get_point_color(), lp.get_point_size(),
                    canvas_color, lp.get_point_size2());
         draw_text(cr, pctx, lp.get_legend_name().c_str(), Pixel_XY(xt, y1 + 5));
       }
}
//-------------------------------------------------------------------------------
static inline void
pv_swap(Pixel_XY & P0, double & Y0, Pixel_XY & P1, double & Y1)
{
const double   Y = Y0;   Y0 = Y1;   Y1 = Y;
const Pixel_XY P = P0;   P0 = P1;   P1 = P;
}
//-------------------------------------------------------------------------------
// draw a 3D triangle where one point P0 is at visual height H0 and the
// other two points P1 and P2 are the same at visual height H12.
void
draw_triangle(cairo_t * cr, const Plot_context & pctx, int verbosity,
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

   // every line is ~1 pixel, so the max y should suffice for steps
   //
int steps = P0.y;
   if (steps < P1.y)   steps = P1.y;
   if (steps < P2.y)   steps = P2.y;
const double dH = (H12 - H0) / steps;

   loop(s, steps + 1)
       {
         const double alpha = H0 + s*dH;
         const double beta  = (1.0 * s)/steps;   // line lenght
         const Color line_color = pctx.w_props.get_color(alpha);
         const Pixel_XY P0_P1(P0.x + beta * (P1.x - P0.x),
                              P0.y + beta * (P1.y - P0.y));
         const Pixel_XY P0_P2(P0.x + beta * (P2.x - P0.x),
                              P0.y + beta * (P2.y - P0.y));
         draw_line(cr, pctx, line_color, 1, P0_P1, P0_P2);
       }
}
//-------------------------------------------------------------------------------
// draw a 3D triangle where the points P0, P1, and P2 are at visual
// heights H0, H1, and H2 respectively. This is done by splitting the
// triangle into two triangles that each have 2 points at the same height.
void
draw_triangle(cairo_t * cr, const Plot_context & pctx, int verbosity,
              Pixel_XY P0, double H0, Pixel_XY P1, double H1,
              Pixel_XY P2, double H2)
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

const Plot_window_properties & w_props = pctx.w_props;
const vector<level_color> & color_steps = w_props.get_gradient();
   if (color_steps.size() == 0)
      {
        CERR << "*** no color_steps" << endl;
        return;
      }

   if (H0 == H1)   // P0 and P1 have the same height
      {
        draw_triangle(cr, pctx, verbosity, P2, H2, P0, P1, H0);
      }
   else if (H1 == H2)   // P1 and P2 have the same height
      {
        draw_triangle(cr, pctx, verbosity, P0, H0, P1, P2, H1);
      }
   else            // P2 lies below P1
      {
        const double alpha = (H0 - H1) / (H0 - H2);   // alpha → 1 as P1 → P2
        Assert(alpha >= 0.0);
        Assert(alpha <= 1.0);

        // compute the point P on P0-P2 that has the heigth H1 (of P1)
        const Pixel_XY P(P0.x + alpha*(P2.x - P0.x),
                         P0.y + alpha*(P2.y - P0.y));
        draw_triangle(cr, pctx, verbosity, P0, H0, P1, P, H1);
        draw_triangle(cr, pctx, verbosity, P2, H2, P1, P, H1);
      }
}
//-------------------------------------------------------------------------------
void
draw_X_grid(cairo_t * cr, Plot_context & pctx, bool surface_plot)
{
const Plot_window_properties & w_props = pctx.w_props;
const int line_width = w_props.get_gridX_line_width();
const Color line_color = w_props.get_gridX_color();

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
              draw_line(cr, pctx, line_color, line_width,
                        Pixel_XY(px0, py0), Pixel_XY(px0, py1));
            }
         else if (w_props.get_gridX_style() == 2)
            {
              draw_line(cr, pctx, line_color, line_width,
                        Pixel_XY(px0, py0 + 5), Pixel_XY(px0, py1 - 5));
            }


         const char * cc = format_tick(v);
         double cc_width, cc_height;
         cairo_string_size(cc_width, cc_height, cr, cc, FONT_NAME, FONT_SIZE);

         Pixel_XY cc_pos(px0 - 0.5*cc_width, py0 + cc_height + 3);
         if (surface_plot)
            {
              cc_pos.x -= w_props.get_origin_X();
              cc_pos.y += w_props.get_origin_Y();
            }
         draw_text(cr, pctx, cc, cc_pos);
       }
}
//-------------------------------------------------------------------------------
void
draw_Y_grid(cairo_t * cr, Plot_context & pctx, bool surface_plot)
{
const Plot_window_properties & w_props = pctx.w_props;
const int line_width = w_props.get_gridY_line_width();
const Color line_color = w_props.get_gridY_color();

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
              draw_line(cr, pctx, line_color, line_width,
                        Pixel_XY(px0, py0), Pixel_XY(px1, py0));
            }
         else if (w_props.get_gridY_style() == 2)
            {

              draw_line(cr, pctx, line_color, line_width,
                        Pixel_XY(px0 - 5, py0), Pixel_XY(px1 + 5, py0));
            }
         const char * cc = format_tick(v);
         double cc_width, cc_height;
         cairo_string_size(cc_width, cc_height, cr, cc, FONT_NAME, FONT_SIZE);

         Pixel_XY cc_pos(px0 - cc_width - 4, py0 + 0.5 * cc_height - 1);
         if (surface_plot)
            {
              cc_pos.x -= w_props.get_origin_X();
              cc_pos.y += w_props.get_origin_Y();
            }
           draw_text(cr, pctx, cc, cc_pos);
       }
}
//-------------------------------------------------------------------------------
void
draw_Z_grid(cairo_t * cr, Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const int line_width = w_props.get_gridZ_line_width();
const Color line_color = w_props.get_gridZ_color();

const int ix_max = w_props.get_gridX_last();
const int iy_max = w_props.get_gridY_last();
const int iz_max = w_props.get_gridZ_last();
const Pixel_X len_Zx = w_props.get_origin_X();
const Pixel_Y len_Zy = w_props.get_origin_Y();
const Pixel_XY orig = w_props.valXYZ2pixelXY(w_props.get_min_X(),
                                             w_props.get_min_Y(),
                                             w_props.get_min_Z());
const Pixel_X len_X = w_props.valX2pixel(w_props.get_max_X())
                    - w_props.valX2pixel(w_props.get_min_X());

   // NOTE: in cairo (other than in xcb) the Y-coordinates increase when moving
   // down the screen. Therefore larger Y values correspond to smsller Y
   // coordinates and we must subtract w_props.valY2pixel(w_props.get_max_Y())
   // from w_props.valY2pixel(w_props.get_min_Y()) and not the other way around.
   //
const Pixel_Y len_Y = w_props.valY2pixel(w_props.get_min_Y())
                    - w_props.valY2pixel(w_props.get_max_Y());

   // lines starting on the Z-axis...
   //
   for (int iz = 1; iz <= iz_max; ++iz)
       {
         const Pixel_X px0 = orig.x - iz * len_Zx / iz_max;
         const Pixel_Y py0 = orig.y + iz * len_Zy / iz_max;
         const Pixel_X px1 = px0 + len_X;
         const Pixel_Y py1 = py0 - len_Y;
         const  Pixel_XY PZ(px0, py0);   // point on the Z axis
         const  Pixel_XY PX(px1, py0);   // along the X axis
         const  Pixel_XY PY(px0, py1);   // along the Y axis
         draw_line(cr, pctx, line_color, line_width, PZ, PX);
         draw_line(cr, pctx, line_color, line_width, PZ, PY);

         const double v = w_props.get_min_Z() + iz*w_props.get_tile_Z();
         const char * cc = format_tick(v);
         double cc_width, cc_height;
         cairo_string_size(cc_width, cc_height, cr, cc, FONT_NAME, FONT_SIZE);
         const Pixel_XY cc_pos(px1 + 10, py0 + 0.5*cc_height - 1);
         draw_text(cr, pctx, cc, cc_pos);
       }

   // lines starting on the X-axis...
   //
   for (int ix = 0; ix <= ix_max; ++ix)
       {
         const Pixel_X px0 = orig.x + ix * len_X / ix_max;
         const Pixel_Y py0 = orig.y;
         const Pixel_X px1 = px0 - len_Zx;
         const Pixel_Y py1 = py0 + len_Zy;
         const  Pixel_XY P0(px0, py0);   // point on the X axis
         const  Pixel_XY P1(px1, py1);   // point along the Z axis

         draw_line(cr, pctx, line_color, line_width, P0, P1);
       }

   // lines starting on the Y-axis...
   //
   for (int iy = 1; iy <= iy_max; ++iy)
       {
         const Pixel_X px0 = orig.x;
         const Pixel_Y py0 = orig.y - iy * len_Y / iy_max;
         const Pixel_X px1 = px0 - len_Zx;
         const Pixel_Y py1 = py0 + len_Zy;
         const  Pixel_XY P0(px0, py0);   // point on the Y axis
         const  Pixel_XY P1(px1, py1);   // point along the Z axis

         draw_line(cr, pctx, line_color, line_width, P0, P1);
       }
}
//-----------------------------------------------------------------------------
/// draw normal (2D) data lines
void
draw_plot_lines(cairo_t * cr, const Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const Plot_data & data = w_props.get_plot_data();
Plot_line_properties const * const * l_props = w_props.get_line_properties();

   loop(l, data.get_row_count())
       {
         const Plot_line_properties & lp = *l_props[l];
         const Color line_color = lp.get_line_color();
         const int line_width = lp.get_line_width();

         // draw lines between points
         //
         Pixel_XY last(0, 0);
         loop(n, data[l].get_N())
             {
               double vx, vy;   data.get_XY(vx, vy, l, n);

               const Pixel_XY P(w_props.valX2pixel(vx - w_props.get_min_X())
                                + w_props.get_origin_X(),
                                w_props.valY2pixel(vy - w_props.get_min_Y()));
               if (n)   draw_line(cr, pctx, line_color, line_width, last, P);
               last = P;
             }

         // draw points...
         //
         const Color canvas_color = w_props.get_canvas_color();
         const int point_style  = lp.get_point_style();
         loop(n, data[l].get_N())
             {
               double vx, vy;   data.get_XY(vx, vy, l, n);
               const Pixel_X px = w_props.valX2pixel(vx - w_props.get_min_X())
                                + w_props.get_origin_X();
               const Pixel_Y py = w_props.valY2pixel(vy - w_props.get_min_Y());
               draw_point(cr, pctx, Pixel_XY(px, py), point_style,
                                             lp.get_point_color(),
                          lp.get_point_size(), canvas_color, lp.get_point_size2());
             }
       }
}
//-----------------------------------------------------------------------------
/// draw surface (3D) data lines
void
draw_surface_lines(cairo_t * cr, const Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const Plot_data & data = w_props.get_plot_data();
Plot_line_properties const * const * l_props = w_props.get_line_properties();
const Plot_line_properties & lp0 = *l_props[0];

const Color canvas_color = w_props.get_canvas_color();
const Color point_color  = lp0.get_point_color();
const Color line_color   = lp0.get_line_color();
const int line_width     = lp0.get_line_width();

   // 1. draw areas between plot lines...
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

    //   if (row != 2)   continue;   // show only given row
    //   if (col != 2)   continue;   // show only given column

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

#if 0    /* Option A...

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

#else    /* Option B...

            fold surface-square along its shorter edge
          */

         if (P0.distance2(P3) < P1.distance2(P2))   // P0-P3 is shorter
            {
              draw_triangle(cr, pctx, verbosity, P1, H1, P0, H0, P3, H3);
              draw_triangle(cr, pctx, verbosity, P2, H2, P0, H0, P3, H3);
            }
         else                                       // P1-P2 is shorter
            {
              draw_triangle(cr, pctx, verbosity, P0, H0, P1, H1, P2, H2);
              draw_triangle(cr, pctx, verbosity, P3, H3, P1, H1, P2, H2);
            }
#endif
       }

   // 2. draw lines between plot points
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
         draw_line(cr, pctx, line_color, line_width, P0, P1);
         if (row == (data.get_row_count() - 2))   // last row
            draw_line(cr, pctx, line_color, line_width, P2, P3);

         draw_line(cr, pctx, line_color, line_width, P0, P2);
         if (col == (data[row].get_N() - 2))   // last column
            draw_line(cr, pctx, line_color, line_width, P1, P3);
       }

  // 3. draw plot points
  //
const int point_style  = lp0.get_point_style();

  loop(row, data.get_row_count())
  loop(col, data[row].get_N())
      {
        const Pixel_XY P0 = w_props.valXYZ2pixelXY(data.get_X(row, col),
                                                   data.get_Y(row, col),
                                                   data.get_Z(row, col));
        draw_point(cr, pctx, P0, point_style, point_color, 2, canvas_color, 0);
      }
}
//-------------------------------------------------------------------------------
static void
do_plot(GtkWidget * drawing_area, Plot_context & pctx, cairo_t * cr)
{
  cairo_set_source_rgb(cr, 1.0,  1.0,  1.0);
  cairo_rectangle(cr, 0, 0, pctx.get_total_width(), pctx.get_total_height());
  cairo_fill(cr);

  cairo_select_font_face(cr, "sans-serif",
                             CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 12);

   // draw grid lines...
   //
const bool surface_plot = pctx.w_props.get_plot_data().is_surface_plot();

  draw_X_grid(cr, pctx, surface_plot);
  draw_Y_grid(cr, pctx, surface_plot);
  if (surface_plot)    draw_Z_grid(cr, pctx);

  if  (surface_plot)   draw_surface_lines(cr, pctx);
  else                 draw_plot_lines(cr, pctx);

  // draw legend...
  //
  draw_legend(cr, pctx, surface_plot);
}
//-------------------------------------------------------------------------------
extern "C" gboolean
plot_destroyed(GtkWidget * top_level);

gboolean
plot_destroyed(GtkWidget * top_level)
{
   if (verbosity & SHOW_EVENTS)   CERR << "PLOT DESTROYED" << endl;
  gtk_main_quit();
  return TRUE;   // event handled by this handler
}
//-------------------------------------------------------------------------------
static void
save_file(Plot_context & pctx, cairo_surface_t * surface)
{
const Plot_window_properties & w_props = pctx.w_props;

string fname = w_props.get_output_filename();
   if (fname.size() == 0)   return;

   if (fname.size() < 4 || strcmp(".png", fname.c_str() + fname.size() - 4))
      fname += ".png";

const cairo_status_t stat = cairo_surface_write_to_png(surface, fname.c_str());
   if (stat == CAIRO_STATUS_SUCCESS)
      {
        CERR << "wrote output file: " << fname << endl;
        if (w_props.get_auto_close() == 1)   gtk_main_quit();
      }

   else
      {
        CERR << "*** writing output fil: " << fname << " failed." << endl;
        if (w_props.get_auto_close() == 2)   gtk_main_quit();
      }
}
//-------------------------------------------------------------------------------
extern "C" gboolean
draw_callback(GtkWidget * drawing_area, cairo_t * cr, gpointer user_data);

gboolean
draw_callback(GtkWidget * drawing_area, cairo_t * cr, gpointer user_data)
{
   // callback from GTK.

   if (verbosity & SHOW_EVENTS)
      CERR << "draw_callback(drawing_area = " << drawing_area << ")" << endl;

   // find the Plot_context for this thread...
   //

Plot_context * pctx = 0;
   for (size_t th = 0; th < all_plot_contexts.size(); ++th)
       {
           if (all_plot_contexts[th]->drawing_area == drawing_area)
              {
                pctx = all_plot_contexts[th];
                break;
              }
       }

   if (pctx == 0)
      {
        CERR << "*** Could not find thread handling drawing_area "
             << reinterpret_cast<void *>(drawing_area) << endl;
        return false;
      }


GtkWidget * widget = GTK_WIDGET(drawing_area);
cairo_surface_t * surface = gdk_window_create_similar_surface(
                                gtk_widget_get_window(drawing_area),
                                CAIRO_CONTENT_COLOR,
                                gtk_widget_get_allocated_width(widget),
                                gtk_widget_get_allocated_height(widget));

   {
     cairo_t * cr1 = cairo_create(surface);
     do_plot(drawing_area, *pctx, cr1);   // plot the data
     cairo_destroy(cr1);
   }

   // copy pctx->surface to the cr of the caller
   //
   cairo_set_source_surface(cr, surface, 0, 0);
   cairo_paint(cr);


   save_file(*pctx, surface);

   cairo_surface_destroy(surface);

   if (verbosity & SHOW_EVENTS)   CERR << "draw_callback() done." << endl;
   return TRUE;   // event handled by this handler
}
//-------------------------------------------------------------------------------
static void *
gtk_main_wrapper(void * w_props)
{
   gtk_main();

   if (verbosity & SHOW_EVENTS)   CERR << "gtk_main() thread done" << endl;

   delete reinterpret_cast<char *>(w_props);

   if (verbosity & SHOW_EVENTS)
      CERR << "wprops " << w_props << " deleted." << endl;

   // remove this plot context
   for (size_t th = 0; th < all_plot_contexts.size(); ++th)
       {
         if (w_props == &all_plot_contexts[th]->w_props)
            {
              all_plot_contexts[th] = all_plot_contexts.back();
              usleep(1000);   // let others find back()
              all_plot_contexts.pop_back();
            }
       }

   return 0;
}
//-------------------------------------------------------------------------------
void *
plot_main(void * vp_props)
{
   if (getenv("DISPLAY") == 0)   // DISPLAY not set
      setenv("DISPLAY", ":0", true);

char * name = strdup("⎕PLOT");
char * argv2[2] = { name, 0 };
char ** argv = argv2;

   XInitThreads();

static bool gtk_init_done = false;
   if (!gtk_init_done)
      {
        int argc = sizeof(argv2) / sizeof(*argv2) - 1;
        gtk_init(&argc, &argv);
        gtk_init_done = true;
      }

Plot_window_properties & w_props =
      *reinterpret_cast<Plot_window_properties *>(vp_props);
   verbosity = w_props.get_verbosity();

Plot_context * pctx = new Plot_context(w_props);
   Assert(pctx);
   all_plot_contexts.push_back(pctx);

   pctx->builder = gtk_builder_new_from_string(gui, strlen(gui));
   Assert(pctx->builder);
   pctx->css_provider = gtk_css_provider_new();
   Assert(pctx->css_provider);
GError * error = 0;
   gtk_css_provider_load_from_data(pctx->css_provider, css, strlen(css), &error);
   Assert(error == 0);
   pctx->window = gtk_builder_get_object(pctx->builder, "top-level-window");
   Assert(pctx->window);
   pctx->canvas = gtk_builder_get_object(pctx->builder, "canvas");
   Assert(pctx->canvas);

   // drawing_area is the same GObject * as the first argument of draw_callback()
   //
   if (GObject * canvas = gtk_builder_get_object(pctx->builder, "canvas"))
      { pctx-> drawing_area = GTK_WIDGET(canvas); }

   // resize the drawing_area before showing it so that draw_callback() won't be
   // called twice.
   //
   gtk_widget_set_size_request(GTK_WIDGET(pctx->drawing_area),
                                          pctx->get_total_width(),
                                          pctx->get_total_height());

   gtk_widget_show_all(GTK_WIDGET(pctx->window));

   /*
      NOTE: supposedly we should connect our signals like this:

      gtk_builder_connect_signals(pctx->builder, 0);

      However, gtk_builder_connect_signals() then complains about -rdynamic not
      being used, for example

      (⎕PLOT:6029): Gtk-WARNING **: 14:35:25.772: Could not find signal handler 'draw_callback'.  Did you compile with -rdynamic?

      If we fix that by compiling with -rdynamic, then everything works fine
      with gcc, but clang complains about -rdynamic being used:

c++: warning: argument unused during compilation: '-rdynamic' [-Wunused-command-line-argument]

      We therefore connect our signals manually using g_signal_connect_object().
    */
   g_signal_connect_object(pctx->window, "destroy", G_CALLBACK(plot_destroyed),
                           0, G_CONNECT_AFTER);
   g_signal_connect_object(pctx->canvas, "draw", G_CALLBACK(draw_callback),
                           0, G_CONNECT_AFTER);

   // fork a handler for Gtk window events.
   //
   pthread_create(&pctx->thread, 0, gtk_main_wrapper, &w_props);

   sem_post(Quad_PLOT::plot_window_sema);   // unleash the APL intrpreter
   return 0;
}
//-----------------------------------------------------------------------------
#endif // HAVE_GTK3
