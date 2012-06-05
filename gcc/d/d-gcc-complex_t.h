
// Compiler implementation of the D programming language
// Copyright (c) 1999-2006 by Digital Mars
// All Rights Reserved
// written by Walter Bright and Burton Radons
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by David Friedman, September 2004

   Same as DMD complex_t.h, but use GCC's REAL_VALUE_TYPE-based real_t
   instead of long double.
*/

#ifndef DMD_COMPLEX_T_H
#define DMD_COMPLEX_T_H

/* Roll our own complex type for compilers that don't support complex
 */


struct complex_t
{
  real_t re;
  real_t im;

  complex_t () { this->re = 0; this->im = 0; }
  complex_t (real_t re) { this->re = re; this->im = 0; }
  complex_t (real_t re, real_t im) { this->re = re; this->im = im; }

  complex_t operator + (complex_t y) { complex_t r; r.re = re + y.re; r.im = im + y.im; return r; }
  complex_t operator - (complex_t y) { complex_t r; r.re = re - y.re; r.im = im - y.im; return r; }
  complex_t operator - () { complex_t r; r.re = -re; r.im = -im; return r; }
  complex_t operator * (complex_t y) { return complex_t (re * y.re - im * y.im, im * y.re + re * y.im); }

  complex_t operator / (complex_t y) {
      real_t abs_y_re = y.re.isNegative () ? -y.re : y.re;
      real_t abs_y_im = y.im.isNegative () ? -y.im : y.im;
      real_t r, den;

      if (abs_y_re < abs_y_im)
	{
	  r = y.re / y.im;
	  den = y.im + r * y.re;
	  return complex_t ((re * r + im) / den,
			   (im * r - re) / den);
	}
      else
	{
	  r = y.im / y.re;
	  den = y.re + r * y.im;
	  return complex_t ((re + r * im) / den,
			   (im - r * re) / den);
	}
  }

  operator bool () { return !re.isZero () || !im.isZero (); }

  int operator == (complex_t y) { return re == y.re && im == y.im; }
  int operator != (complex_t y) { return re != y.re || im != y.im; }
};

inline complex_t operator * (real_t x, complex_t y) { return complex_t (x) * y; }
inline complex_t operator * (complex_t x, real_t y) { return x * complex_t (y); }
inline complex_t operator / (complex_t x, real_t y) { return x / complex_t (y); }


inline real_t creall (complex_t x)
{
  return x.re;
}

inline real_t cimagl (complex_t x)
{
  return x.im;
}

#endif
