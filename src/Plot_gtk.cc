
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

using namespace std;

/// the pthread that handles one plot window.
extern void * plot_main(void * vp_props);

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

// ===========================================================================
const char * gui =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!-- Generated with glade 3.22.1 -->\n"
"<interface>\n"
"  <requires lib=\"gtk+\" version=\"3.20\"/>\n"
"  <object class=\"GtkWindow\" id=\"top-level-window\">\n"
"    <property name=\"can_focus\">False</property>\n"
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
     canvas(0),
     window(0),
     drawing_area(0),
     surface(0),
     cr(0)
   {}

   /// the window properties (as choosen by the user)
   const Plot_window_properties & w_props;

   GtkBuilder * builder;
   GtkCssProvider * css_provider;
   pthread_t thread;
   GObject * canvas;
   GObject * window;
   GtkWidget * drawing_area;
   cairo_surface_t * surface;
   cairo_t * cr;
};

static vector<Plot_context *> all_plot_contexts;

//-----------------------------------------------------------------------------
inline GdkRGBA
RGBA(unsigned int color)
{
GdkRGBA rgba;
   rgba.red   = (color >> 16 & 0xFF) / 255.0;
   rgba.green = (color >>  8 & 0xFF) / 255.0;
   rgba.blue  = (color       & 0xFF) / 255.0;
   rgba.alpha = 1.0;   // opaque
   return rgba;
}
//-----------------------------------------------------------------------------
void
draw_line(const Plot_context & pctx, const GdkRGBA & color,
          const Pixel_XY & P0, const Pixel_XY & P1)
{
   Assert(pctx.surface);

cairo_t * cr = pctx.cr;

const int x0 = P0.x;
const int y0 = P0.y;
const int x1 = P1.x;
const int y1 = P1.y;

   cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);

const int sdelta_x = x1 - x0;
const int sdelta_y = y1 - y0;
const int adelta_x = (sdelta_x >= 0) ? sdelta_x : -sdelta_x;
const int adelta_y = (sdelta_y >= 0) ? sdelta_y : -sdelta_y;
const double rad = 1.0;

  if (adelta_x >= adelta_y)   // iterste over xx
      {
        // only one of the for() below will do anything
        //
        const double dy_dx = double(sdelta_y) / sdelta_x;
        for (int xx = x0; xx < x1; ++xx)
            {
              cairo_rectangle(cr, xx, y0 + (xx - x0)*dy_dx, rad, rad);
            }
        for (int xx = x1; xx < x0; ++xx)
            {
              cairo_rectangle(cr, xx, y0 + (xx - x1)*dy_dx, rad, rad);
            }
      }
   else                       // iterste over yy
      {
        // only one of the for() below will do anything
        //
        const double dx_dy = double(sdelta_x) / sdelta_y;
        for (int yy = y0; yy < y1; ++yy)
            {
              cairo_rectangle(cr, x0 + (yy - y0)*dx_dy, yy, rad, rad);
            }
        for (int yy = y1; yy < y0; ++yy)
            {
              cairo_rectangle(cr, x1 + (yy - y1)*dx_dy, yy, rad, rad);
            }
      }

   cairo_fill(cr);
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
draw_text(const Plot_context & pctx, const char * label, const Pixel_XY & xy)
{
cairo_t * cr = pctx.cr;
  cairo_move_to(cr, xy.x, xy.y);
  cairo_select_font_face(cr, "sans-serif",
                             CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 10);
  cairo_show_text(cr, label);
}
//-------------------------------------------------------------------------------
void
draw_X_grid(Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const int color = w_props.get_gridX_color();
GdkRGBA line_color;
   line_color.red   = color >> 16 & 0xFF;
   line_color.green = color >>  8 & 0xFF;
   line_color.blue  = color       & 0xFF;
   line_color.alpha = 255;   // opaque

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
              draw_line(pctx, line_color, Pixel_XY(px0, py0),
                                          Pixel_XY(px0, py1));
            }

         const char * cc = format_tick(v);
         cairo_text_extents_t extents;
         cairo_text_extents(pctx.cr, cc, &extents);
         const Pixel_XY cc_pos(px0 - 0.5*extents.width,
                             py0 + extents.height + 3);
         draw_text(pctx, cc, cc_pos);
       }
}
//-------------------------------------------------------------------------------
void
draw_Y_grid(Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const int color = w_props.get_gridY_color();
GdkRGBA line_color;
   line_color.red   = color >> 16 & 0xFF;
   line_color.green = color >>  8 & 0xFF;
   line_color.blue  = color       & 0xFF;
   line_color.alpha = 255;   // opaque

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
              draw_line(pctx, line_color, Pixel_XY(px0, py0),
                                          Pixel_XY(px1, py0));
            }

         const char * cc = format_tick(v);
         cairo_text_extents_t extents;
         cairo_text_extents(pctx.cr, cc, &extents);
         const Pixel_XY cc_pos(px0 - extents.width - 4,
                               py0 + 0.5 * extents.height - 1);
         draw_text(pctx, cc, cc_pos);
       }
}
//-------------------------------------------------------------------------------
void
draw_Z_grid(Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const int color = w_props.get_gridZ_color();
GdkRGBA line_color;
   line_color.red   = color >> 16 & 0xFF;
   line_color.green = color >>  8 & 0xFF;
   line_color.blue  = color       & 0xFF;
   line_color.alpha = 255;   // opaque

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
         draw_line(pctx, line_color, Pixel_XY(px0, py0),
                                     Pixel_XY(px1,  py0));
         draw_line(pctx, line_color, Pixel_XY(px0, py0),
                                     Pixel_XY(px0,  py0 + len_Y));

         const char * cc = format_tick(v);
         cairo_text_extents_t extents;
         cairo_text_extents(pctx.cr, cc, &extents);
         const Pixel_XY cc_pos(px1 + 10,
                               py0 + 0.5*extents.height - 1);
         draw_text(pctx, cc, cc_pos);
       }
}
//-----------------------------------------------------------------------------
/// draw the plot lines
void
draw_plot_lines(const Plot_context & pctx)
{
const Plot_window_properties & w_props = pctx.w_props;
const Plot_data & data = w_props.get_plot_data();
Plot_line_properties const * const * l_props = w_props.get_line_properties();

   loop(l, data.get_row_count())
       {
         const Plot_line_properties & lp = *l_props[l];
         const GdkRGBA line_color = RGBA(lp.line_color);

         // draw lines between points
         //
         Pixel_XY last(0, 0);
         loop(n, data[l].get_N())
             {
               double vx, vy;   data.get_XY(vx, vy, l, n);

               const Pixel_XY P(w_props.valX2pixel(vx - w_props.get_min_X())
                                + w_props.get_origin_X(),
                                w_props.valY2pixel(vy - w_props.get_min_Y()));
               if (n)   draw_line(pctx, line_color, last, P);
               last = P;
             }

         // draw points...
         //
#if 0
         const uint32_t gc_p[] = { lp.point_color, lp.point_size };
         xcb_change_gc(pctx.conn, pctx.point, mask, gc_p);
         const Color canvas_color = w_props.get_canvas_color();
         loop(n, data[l].get_N())
             {
               double vx, vy;   data.get_XY(vx, vy, l, n);
               const Pixel_X px = w_props.valX2pixel(vx - w_props.get_min_X())
                                + w_props.get_origin_X();
               const Pixel_Y py = w_props.valY2pixel(vy - w_props.get_min_Y());
               draw_point(pctx, Pixel_XY(px, py), canvas_color, *l_props[l]);
             }
#endif
       }
}
//-----------------------------------------------------------------------------
/// draw the surface plot lines
void
draw_surface_lines(const Plot_context & pctx)
{
//const Plot_window_properties & w_props = pctx.w_props;
}
//-------------------------------------------------------------------------------
static void
do_plot(Plot_context & pctx)
{
   // resize the top-level window and its drawing area to the desired size
   //
const int width  = pctx.w_props.get_pa_width()
                 + pctx.w_props.get_pa_border_L()
                 + pctx.w_props.get_origin_X()
                 + pctx.w_props.get_pa_border_R();

const int height = pctx.w_props.get_pa_height()
                 + pctx.w_props.get_pa_border_T()
                 + pctx.w_props.get_origin_Y()
                 + pctx.w_props.get_pa_border_B();

   // the pctx.window updates itself when pctx.canvas is resizd. Thus no:
   // gtk_widget_set_size_request(GTK_WIDGET(pctx.window), width, height);
   //
   gtk_widget_set_size_request(GTK_WIDGET(pctx.canvas), width, height);

   pctx.cr =  cairo_create(pctx.surface);
   cairo_set_source_rgb(pctx.cr, 1.0,  1.0,  1.0);
   cairo_rectangle(pctx.cr, 0, 0, width, height);
   cairo_fill(pctx.cr);

   // draw grid lines...
   //
const bool surface_plot = pctx.w_props.get_plot_data().is_surface_plot();

   draw_X_grid(pctx);
   draw_Y_grid(pctx);

   if  (surface_plot)   draw_surface_lines(pctx);
   else                 draw_plot_lines(pctx);
}
//-------------------------------------------------------------------------------
extern "C" gboolean
draw_callback(GtkWidget * drawing_area, cairo_t * cr, gpointer user_data);

gboolean
draw_callback(GtkWidget * drawing_area, cairo_t * cr, gpointer user_data)
{
   // callback from GTK.

   // find the Plot_context for this thread...
   //
CERR << "draw_callback()" << endl;

Plot_context * pctx = 0;
   for (size_t th = 0; th < all_plot_contexts.size(); ++th)
       {
           if (all_plot_contexts[th]->thread == pthread_self())
              {
                pctx = all_plot_contexts[th];
                break;
              }
       }

   if (pctx == 0)
      {
        CERR << "*** Could not find thread()" << endl;
        return false;
      }


   pctx->drawing_area = drawing_area;

GtkWidget * widget = GTK_WIDGET(drawing_area);
   pctx->surface = gdk_window_create_similar_surface(
                      gtk_widget_get_window(drawing_area),
                      CAIRO_CONTENT_COLOR,
                      gtk_widget_get_allocated_width(widget),
                      gtk_widget_get_allocated_height(widget));

   do_plot(*pctx);   // plot the data

   // copy pctx->surface to the cr of the caller
   //
   cairo_set_source_surface(cr, pctx->surface, 0, 0);
   cairo_paint(cr);

   cairo_surface_destroy(pctx->surface);
   pctx->surface = 0;
   pctx->drawing_area = 0;

CERR << "draw_callback() done." << endl;
   return TRUE;   // event handled by this handler
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

// XInitThreads();
int argc = sizeof(argv2) / sizeof(*argv2) - 1;
  gtk_init(&argc, &argv);

Plot_window_properties & w_props =
      *reinterpret_cast<Plot_window_properties *>(vp_props);

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


   gtk_widget_show_all(GTK_WIDGET(pctx->window));

   gtk_builder_connect_signals(pctx->builder, 0);

   pthread_create(&pctx->thread, 0,
                  reinterpret_cast<void *(*)(void *)>(gtk_main), 0);

   sem_post(Quad_PLOT::plot_window_sema);   // unleash the inetrpreter
   return 0;
}
//-----------------------------------------------------------------------------
#endif // HAVE_GTK3
