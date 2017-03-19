// d-port.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/port.h"
#include "dfrontend/target.h"

#include "tree.h"
#include "stor-layout.h"

int
Port::memicmp(const char *s1, const char *s2, int n)
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

char *
Port::strupr(char *s)
{
  char *t = s;

  while (*s)
    {
      *s = TOUPPER (*s);
      s++;
    }

  return t;
}

// Return true a if the real_t value from string BUFFER overflows
// as a result of rounding down to float mode.

bool
Port::isFloat32LiteralOutOfRange (const char *buffer)
{
  real_t r;

  real_from_string3(&r.rv (), buffer, TYPE_MODE (float_type_node));

  return r == Target::RealProperties::infinity;
}

// Return true a if the real_t value from string BUFFER overflows
// as a result of rounding down to double mode.

bool
Port::isFloat64LiteralOutOfRange (const char *buffer)
{
  real_t r;

  real_from_string3(&r.rv (), buffer, TYPE_MODE (double_type_node));

  return r == Target::RealProperties::infinity;
}

// Fetch a little-endian 16-bit value.

unsigned
Port::readwordLE(void *buffer)
{
  unsigned char *p = (unsigned char*) buffer;

  return ((unsigned) p[1] << 8) | (unsigned) p[0];
}

// Fetch a big-endian 16-bit value.

unsigned
Port::readwordBE(void *buffer)
{
  unsigned char *p = (unsigned char*) buffer;

  return ((unsigned) p[0] << 8) | (unsigned) p[1];
}

// Write a little-endian 32-bit value.

void
Port::writelongLE(unsigned value, void *buffer)
{
    unsigned char *p = (unsigned char*) buffer;

    p[0] = (unsigned) value;
    p[1] = (unsigned) value >> 8;
    p[2] = (unsigned) value >> 16;
    p[3] = (unsigned) value >> 24;
}

// Fetch a little-endian 32-bit value.

unsigned
Port::readlongLE(void *buffer)
{
  unsigned char *p = (unsigned char*) buffer;

  return (((unsigned) p[3] << 24)
          | ((unsigned) p[2] << 16)
          | ((unsigned) p[1] << 8)
          | (unsigned) p[0]);
}

// Write a big-endian 32-bit value.

void
Port::writelongBE(unsigned value, void *buffer)
{
    unsigned char *p = (unsigned char*) buffer;

    p[0] = (unsigned) value >> 24;
    p[1] = (unsigned) value >> 16;
    p[2] = (unsigned) value >> 8;
    p[3] = (unsigned) value;
}

// Fetch a big-endian 32-bit value.

unsigned
Port::readlongBE(void *buffer)
{
  unsigned char *p = (unsigned char*) buffer;

  return (((unsigned) p[0] << 24)
          | ((unsigned) p[1] << 16)
          | ((unsigned) p[2] << 8)
          | (unsigned) p[3]);
}

void
Port::valcpy(void *dst, uint64_t val, size_t size)
{
  switch (size)
    {
    case 1:
      *(uint8_t *) dst = (uint8_t) val;
      break;

    case 2:
      *(uint16_t *) dst = (uint16_t) val;
      break;

    case 4:
      *(uint32_t *) dst = (uint32_t) val;
      break;

    case 8:
      *(uint64_t *) dst = (uint64_t) val;
      break;

    default:
      gcc_unreachable ();
    }
}
