// d-port.cc -- D frontend for GCC.
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

#include "d-system.h"

#include "port.h"

longdouble Port::nan;
longdouble Port::snan;
longdouble Port::infinity;

void
Port::init (void)
{
  real_nan (&nan.rv(), "", 1, TYPE_MODE (long_double_type_node));
  real_nan (&snan.rv(), "", 0, TYPE_MODE (long_double_type_node));
  real_inf (&infinity.rv());
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

