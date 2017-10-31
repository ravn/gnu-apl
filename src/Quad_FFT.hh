/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2017  Dr. Jürgen Sauermann

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

#ifndef __Quad_FFT_DEFINED__
#define __Quad_FFT_DEFINED__

#include <math.h>

#include "QuadFunction.hh"
#include "Value.hh"
#include "Simple_string.hh"

class Quad_FFT : public QuadFunction
{
public:
   /// Constructor.
   Quad_FFT()
      : QuadFunction(TOK_Quad_FFT),
        system_wisdom_loaded(false)
   {}

   static Quad_FFT * fun;          ///< Built-in function.
   static Quad_FFT  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB()
   Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_B()
   Token eval_B(Value_P B);

   /// window function for sample n of N with parameters a = a0, a1, ...
   typedef double (*window_function)(ShapeItem n, ShapeItem N);

   /// compute forward or backward FFT
   Token do_fft(int dir, Value_P B, window_function win);

   /// return the values of the window function \b win for length \b N
   Token do_window(Value_P B, window_function win);

   /// initialize \b in from B
   static void init_in(void * in, Value_P B, window_function win);

   /// return the no window value for sample n of N
   static double no_window(ShapeItem n, ShapeItem N)
      { return 1.0; }

   /// return the Hann window value for sample n of N
   static double hann_window(ShapeItem n, ShapeItem N)
      { return 0.5 - 0.5*cos(2*n*M_PI / (N-1)); }

   /// return the Hamming window value for sample n of N
   static double hamming_window(ShapeItem n, ShapeItem N)
      { return 0.54 - 0.46*cos(2*n*M_PI / (N-1)); }

   /// return the Hann window value for sample n of N
   static double blackman_window(ShapeItem n, ShapeItem N)
      { return 0.42 - 0.5*cos(2*n*M_PI / (N-1)) + 0.08*cos(4*M_PI*n / (N-1)); }

   /// return the Blackman-Harris window value for sample n of N
   static double blackman_harris_window(ShapeItem n, ShapeItem N)
      { return 0.35875
             - 0.48829*cos(2*n*M_PI / (N-1))
             + 0.14128*cos(4*n*M_PI / (N-1))
             - 0.01168*cos(6*n*M_PI / (N-1)); }

   /// return the Blackman-Nuttallwindow value for sample n of N
   static double blackman_nuttall_window(ShapeItem n, ShapeItem N)
      { return 0.3635819
             - 0.4891775*cos(2*n*M_PI / (N-1))
             + 0.1365995*cos(4*n*M_PI / (N-1))
             - 0.0106411*cos(6*n*M_PI / (N-1)); }

   /// return the Flat-Top window value for sample n of N
   static double flat_top(ShapeItem n, ShapeItem N)
      { return 1.0
             - 1.93 *cos(2*n*M_PI / (N-1))
             + 1.29 *cos(4*n*M_PI / (N-1))
             - 0.388*cos(6*n*M_PI / (N-1))
             + 0.028*cos(8*n*M_PI / (N-1)); }

   /// set up a multi-dimensional window for shape sh, using ithe window
   // function \b win
   static void fill_window(double * result, const Shape & shape,
                           window_function win);

   bool system_wisdom_loaded;
};

#endif // __Quad_FFT_DEFINED__
