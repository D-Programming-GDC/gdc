/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// This is a C implementation of the D module gcc.fpcls;

#include <math.h>
#include "config.h"

/*
#if HAVE_DISTINCT_LONG_DOUBLE
typedef long double my_long_double;
#else
typedef double my_long_double;
#endif
*/
typedef long double my_long_double;

#define XFUNC(name,name2) \
int name##FeZi(my_long_double v) { return name2(v); } \
int name##FdZi(double v) { return name2(v); } \
int name##FfZi(float v) { return name2(v); }

XFUNC(_D3gcc5fpcls10fpclassify,fpclassify)
XFUNC(_D3gcc5fpcls7signbit,signbit)

/* can add the others with config tests, buf for now,
   just use fpclassify */
