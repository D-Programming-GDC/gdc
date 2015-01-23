// d-port.cc -- D frontend for GCC.
// Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

#include "d-system.h"

#include "port.h"

longdouble Port::ldbl_nan;
longdouble Port::snan;
longdouble Port::ldbl_infinity;
longdouble Port::ldbl_max;

void
Port::init()
{
  char buf[128];
  machine_mode mode = TYPE_MODE (long_double_type_node);

  real_nan (&ldbl_nan.rv(), "", 1, mode);
  real_nan (&snan.rv(), "", 0, mode);
  real_inf (&ldbl_infinity.rv());

  get_max_float (REAL_MODE_FORMAT (mode), buf, sizeof (buf));
  real_from_string (&ldbl_max.rv(), buf);
}

// Returns TRUE if longdouble value R is NaN.

int
Port::isNan (longdouble r)
{
  return REAL_VALUE_ISNAN (r.rv());
}

// Same as isNan, but also check if is signalling.

int
Port::isSignallingNan (longdouble r)
{
  return REAL_VALUE_ISNAN (r.rv())
    && r.rv().signalling;
}

// Returns TRUE if longdouble value is +Inf.

int
Port::isInfinity (longdouble r)
{
  return REAL_VALUE_ISINF (r.rv());
}

longdouble
Port::fmodl (longdouble x, longdouble y)
{
  return x % y;
}

// Returns TRUE if longdouble value X is identical to Y.

int
Port::fequal (longdouble x, longdouble y)
{
  return (Port::isNan(x) && Port::isNan(y))
    || REAL_VALUES_IDENTICAL (x.rv(), y.rv());
}

char *
Port::strupr (char *s)
{
  char *t = s;

  while (*s)
    {
      *s = TOUPPER (*s);
      s++;
    }

  return t;
}

int
Port::memicmp (const char *s1, const char *s2, int n)
{
  int result = 0;

  for (int i = 0; i < n; i++)
    {
      char c1 = s1[i];
      char c2 = s2[i];

      result = c1 - c2;
      if (result)
	{
	  result = TOUPPER (c1) - TOUPPER (c2);
	  if (result)
	    break;
	}
    }

  return result;
}

int
Port::stricmp (const char *s1, const char *s2)
{
  int result = 0;

  for (;;)
    {
      char c1 = *s1;
      char c2 = *s2;

      result = c1 - c2;
      if (result)
	{
	  result = TOUPPER (c1) - TOUPPER (c2);
	  if (result)
	    break;
	}
      if (!c1)
	break;
      s1++;
      s2++;
    }

  return result;
}

// Return a longdouble value from string BUFFER rounded to float mode.

longdouble
Port::strtof (const char *buffer, char **)
{
  longdouble r;
  real_from_string3 (&r.rv(), buffer, TYPE_MODE (float_type_node));

  // Front-end checks errno to see if the value is representable.
  if (r == ldbl_infinity)
    errno = ERANGE;

  return r;
}

// Return a longdouble value from string BUFFER rounded to double mode.

longdouble
Port::strtod (const char *buffer, char **)
{
  longdouble r;
  real_from_string3 (&r.rv(), buffer, TYPE_MODE (double_type_node));

  // Front-end checks errno to see if the value is representable.
  if (r == ldbl_infinity)
    errno = ERANGE;

  return r;
}

// Return a longdouble value from string BUFFER rounded to long double mode.

longdouble
Port::strtold (const char *buffer, char **)
{
  longdouble r;
  real_from_string3 (&r.rv(), buffer, TYPE_MODE (long_double_type_node));
  return r;
}

