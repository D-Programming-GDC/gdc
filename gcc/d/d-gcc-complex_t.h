// d-gcc-complex_t.h -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#ifndef DMD_COMPLEX_T_H
#define DMD_COMPLEX_T_H

/* Roll our own complex type for compilers that don't support complex
 */


struct complex_t
{
  real_t re;
  real_t im;

  complex_t (void)
  {
    this->re = 0;
    this->im = 0;
  }

  complex_t (real_t re)
  {
    this->re = re;
    this->im = 0;
  }

  complex_t (real_t re, real_t im)
  {
    this->re = re;
    this->im = im;
  }

  complex_t operator+ (complex_t y)
  {
    complex_t r;
    r.re = this->re + y.re;
    r.im = this->im + y.im;
    return r;
  }

  complex_t operator- (complex_t y)
  {
    complex_t r;
    r.re = this->re - y.re;
    r.im = this->im - y.im;
    return r;
  }

  complex_t operator- (void)
  {
    complex_t r;
    r.re = -this->re;
    r.im = -this->im;
    return r;
  }

  complex_t operator* (complex_t y)
  { return complex_t (this->re * y.re - this->im * y.im, this->im * y.re + this->re * y.im); }

  complex_t operator/ (complex_t y)
  {
    real_t abs_y_re = y.re.isNegative() ? -y.re : y.re;
    real_t abs_y_im = y.im.isNegative() ? -y.im : y.im;
    real_t r, den;

    if (abs_y_re < abs_y_im)
      {
	r = y.re / y.im;
	den = y.im + r * y.re;
	return complex_t ((this->re * r + this->im) / den,
			  (this->im * r - this->re) / den);
      }
    else
      {
	r = y.im / y.re;
	den = y.re + r * y.im;
	return complex_t ((this->re + r * this->im) / den,
			  (this->im - r * this->re) / den);
      }
  }

  operator bool (void)
  { return !re.isZero() || !im.isZero(); }

  int operator== (complex_t y)
  { return this->re == y.re && this->im == y.im; }

  int operator!= (complex_t y)
  { return this->re != y.re || this->im != y.im; }
};

inline complex_t operator* (real_t x, complex_t y)
{
  return complex_t (x) * y;
}

inline complex_t operator* (complex_t x, real_t y)
{
  return x * complex_t (y);
}

inline complex_t operator/ (complex_t x, real_t y)
{
  return x / complex_t (y);
}

inline real_t creall (complex_t x)
{
  return x.re;
}

inline real_t cimagl (complex_t x)
{
  return x.im;
}

#endif
