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

#include "Quad_FFT.hh"
#include "Workspace.hh"

Quad_FFT  Quad_FFT::_fun;
Quad_FFT * Quad_FFT::fun = &Quad_FFT::_fun;

#if defined(HAVE_LIBFFTW3) && defined(HAVE_FFTW3_H)

#include <fftw3.h>
#include "ComplexCell.hh"

//-----------------------------------------------------------------------------
Token
Quad_FFT::eval_B(Value_P B)
{
   return do_fft(FFTW_FORWARD, B);
}
//-----------------------------------------------------------------------------
Token
Quad_FFT::do_fft(int dir, Value_P B)
{
const APL_Integer N = B->element_count();
   if (N == 0)   LENGTH_ERROR;
const double norm = sqrt(N);

const ShapeItem io_size = N * sizeof(fftw_complex);

fftw_complex * in  =  reinterpret_cast<fftw_complex *>(fftw_malloc(io_size));
   if (in == 0)    WS_FULL;
fftw_complex * out =  reinterpret_cast<fftw_complex *>(fftw_malloc(io_size));
   if (out == 0)    { fftw_free(in);   WS_FULL; }

   enum { flags = FFTW_ESTIMATE | FFTW_DESTROY_INPUT };

   // fill in[] with B
   loop(n, N)
      {
        in[n][0] = B->get_ravel(n).get_real_value();
        in[n][1] = B->get_ravel(n).get_imag_value();
      }

   if (B->get_rank() <= 1)   // one-dimensional FFT
      {
        fftw_plan plan = fftw_plan_dft_1d(N, in, out, dir, flags);
        if (plan == 0)
           {
             fftw_free(in);
             fftw_free(out);
             WS_FULL;
           }

        fftw_execute(plan);
        fftw_destroy_plan(plan);
      }
   else if (B->get_rank() == 2)   // two-dimensional FFT
      {
        fftw_plan plan = fftw_plan_dft_2d(B->get_shape_item(0),
                                          B->get_shape_item(1),
                                          in, out, dir, flags);
        if (plan == 0)
           {
             fftw_free(in);
             fftw_free(out);
             WS_FULL;
           }

        fftw_execute(plan);
        fftw_destroy_plan(plan);
      }
   else if (B->get_rank() == 3)   // two-dimensional FFT
      {
        fftw_plan plan = fftw_plan_dft_3d(B->get_shape_item(0),
                                          B->get_shape_item(1),
                                          B->get_shape_item(2),
                                          in, out, dir, flags);
        if (plan == 0)
           {
             fftw_free(in);
             fftw_free(out);
             WS_FULL;
           }

        fftw_execute(plan);
        fftw_destroy_plan(plan);
      }
   else
      {
        int ish[MAX_RANK];
        loop(r, B->get_rank())   ish[r] = B->get_shape_item(r);

        fftw_plan plan = fftw_plan_dft(B->get_rank(), ish, in, out, dir, flags);
        if (plan == 0)
           {
             fftw_free(in);
             fftw_free(out);
             WS_FULL;
           }

        fftw_execute(plan);
        fftw_destroy_plan(plan);
      }

Value_P Z(B->get_shape(), LOC);
   loop(n, N)   new (Z->next_ravel())
                    ComplexCell(out[n][0]/norm, out[n][1]/norm);

   fftw_free(in);
   fftw_free(out);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_FFT::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)         RANK_ERROR;
   if (A->element_count() != 1)   LENGTH_ERROR;

const APL_Integer what = A->get_ravel(0).get_int_value();
   switch(what)
      {
        case  0: return do_fft(FFTW_FORWARD,  B);
        case -1: return do_fft(FFTW_BACKWARD, B);
      }

   MORE_ERROR() << "Invalid left argument (mode) " << what
        << " of ⎕FFT. Valid left arguments are:\n"
"     0: normal FFT\n"
"    ¯1: inverse FFT\n";

   DOMAIN_ERROR;
}

#else // no libfftw3...

//-----------------------------------------------------------------------------
Token
Quad_FFT::eval_B(Value_P B)
{
    MORE_ERROR() <<
"⎕FFT is not available because either no libpcre2 libfftw3 was found on this\n"
"system when GNU APL was compiled, or because it was disabled in ./configure.";

   SYNTAX_ERROR;
   return Token();
}
//-----------------------------------------------------------------------------
Token
Quad_FFT::eval_AB(Value_P A, Value_P B)
{
    MORE_ERROR() <<
"⎕FFT is not available because either no libfftw3 library was found on this\n"
"system when GNU APL was compiled, or because it was disabled in ./configure.";

   SYNTAX_ERROR;
   return Token();
}
//-----------------------------------------------------------------------------

#endif // HAVE_FFTW3_H

