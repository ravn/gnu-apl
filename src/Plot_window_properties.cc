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

#include "Quad_PLOT.hh"
#include "Plot_data.hh"
#include "Plot_line_properties.hh"
#include "Plot_window_properties.hh"

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
Plot_window_properties::~Plot_window_properties()

{
   Log(LOG_Quad_PLOT)
      CERR << "~Plot_window_properties(): deleting plot_data" << endl;

   delete &plot_data;
}
//-----------------------------------------------------------------------------
void
Plot_window_properties::set_window_size(Pixel_X width, Pixel_Y height)
{
   // pa_width and pa_height are the size of the plot area (without borders).
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
   //    is called in plot_main(), i.e. when the the plot window is created·
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
   //·
const char * minus = strchr(att_and_val, '-');
   if (minus && minus < colon)   // line attribute or color gradient
      {
        // it is not unlikely that the user wants to use the same attributes
        // for different plots that may differ in the number of lines. We
        // therefore silently ignore such over-specified attribute rather
        // than returning an error string (which then raises a DOMAIN error).
        //·
        if (!strncmp(att_and_val, "color_level-", 12))
           {
             const int level = strtoll(minus + 1, 0, 10);
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

        const int line = strtoll(minus + 1, 0, 10) - Workspace::get_IO();
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
//-----------------------------------------------------------------------------
Pixel_XY
Plot_window_properties::valXYZ2pixelXY(double X, double Y, double Z) const
{
const double phi = atan2(get_origin_Y(), get_origin_X());
const Pixel_Z pz = valZ2pixel(Z - get_min_Z());
const Pixel_X px = valX2pixel(X - get_min_X()) + get_origin_X() - pz*cos(phi);
const Pixel_X py = valY2pixel(Y - get_min_Y())                  + pz*sin(phi);

   return Pixel_XY(px, py);
}
//-----------------------------------------------------------------------------




