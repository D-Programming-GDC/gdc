/* ctfloat.d -- Compile-time floating-pointer helper functions.
 * Copyright (C) 2018 Free Software Foundation, Inc.
 *
 * GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC; see the file COPYING3.  If not see
 * <http://www.gnu.org/licenses/>.
 */

module dmd.root.ctfloat;

nothrow:

// Type used by the front-end for compile-time reals
public import dmd.root.longdouble : real_t = longdouble;

// Compile-time floating-point helper
extern (C++) struct CTFloat
{
    static __gshared bool yl2x_supported = false;
    static __gshared bool yl2xp1_supported = false;

    static void yl2x(const real_t* x, const real_t* y, real_t* res);
    static void yl2xp1(const real_t* x, const real_t* y, real_t* res);
    static real_t sin(real_t x);
    static real_t cos(real_t x);
    static real_t tan(real_t x);
    static real_t sqrt(real_t x);
    static real_t fabs(real_t x);
    static real_t ldexp(real_t n, int exp);
    static bool isIdentical(real_t a, real_t b);
    static size_t hash(real_t a);
    static bool isNaN(real_t r);
    static bool isSNaN(real_t r);
    static bool isInfinity(real_t r);
    static real_t parse(const(char)* literal, bool* isOutOfRange = null);
    static int sprint(char* str, char fmt, real_t x);

    static __gshared real_t zero;
    static __gshared real_t one;
    static __gshared real_t minusone;
    static __gshared real_t half;
}
