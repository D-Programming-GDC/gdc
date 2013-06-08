// d-target.cc -- D frontend for GCC.
// Copyright (C) 2013 Software Foundation, Inc.

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
#include "aggregate.h"
#include "mtype.h"
#include "dfrontend/target.h"

int Target::ptrsize;
int Target::realsize;
int Target::realpad;
int Target::realalignsize;


void
Target::init (void)
{
  // Map D frontend type and sizes to GCC backend types.
  realsize = int_size_in_bytes (long_double_type_node);
  realpad = TYPE_PRECISION (long_double_type_node) / BITS_PER_UNIT;
  realalignsize = TYPE_ALIGN_UNIT (long_double_type_node);

  // Define what type to use for size_t, ptrdiff_t.
  size_t wordsize = int_size_in_bytes (size_type_node);
  if (wordsize == 2)
    Tsize_t = Tuns16;
  else if (wordsize == 4)
    Tsize_t = Tuns32;
  else if (wordsize == 8)
    Tsize_t = Tuns64;
  else
    gcc_unreachable();

  if (POINTER_SIZE == 32)
    Tptrdiff_t = Tint32;
  else if (POINTER_SIZE == 64)
    Tptrdiff_t = Tint64;
  else
    gcc_unreachable();

  ptrsize = (POINTER_SIZE / BITS_PER_UNIT);

  CLASSINFO_SIZE = 19 * ptrsize;
}

// Return GCC memory alignment size for type TYPE.

unsigned
Target::alignsize (Type *type)
{
  gcc_assert (type->isTypeBasic());
  return TYPE_ALIGN_UNIT (type->toCtype());
}


// Return GCC field alignment size for type TYPE.

unsigned
Target::fieldalign (Type *type)
{
  // Work out the correct alignment for the field decl.
  tree field = make_node (FIELD_DECL);
  DECL_ALIGN (field) = type->alignsize() * BITS_PER_UNIT;

#ifdef BIGGEST_FIELD_ALIGNMENT
  DECL_ALIGN (field)
    = MIN (DECL_ALIGN (field), (unsigned) BIGGEST_FIELD_ALIGNMENT);
#endif
#ifdef ADJUST_FIELD_ALIGN
  if (type->isTypeBasic())
    {
      TREE_TYPE (field) = type->toCtype();
      DECL_ALIGN (field) = ADJUST_FIELD_ALIGN (field, DECL_ALIGN (field));
    }
#endif

  // Also controlled by -fpack-struct=
  if (maximum_field_alignment)
    DECL_ALIGN (field) = MIN (DECL_ALIGN (field), maximum_field_alignment);

  return DECL_ALIGN_UNIT (field);
}

// Return size of OS critical section.
// Can't use the sizeof() calls directly since cross compiling is supported
// and would end up using the host sizes rather than the target sizes.

unsigned
Target::critsecsize (void)
{
  if (global.params.isWindows)
    {
      // sizeof(CRITICAL_SECTION) for Windows.
      return global.params.isLP64 ? 40 : 24;
    }
  else if (global.params.isLinux)
    {
      // sizeof(pthread_mutex_t) for Linux.
      if (global.params.is64bit)
	return global.params.isLP64 ? 40 : 32;
      else
	return global.params.isLP64 ? 40 : 24;
    }
  else if (global.params.isFreeBSD)
    {
      // sizeof(pthread_mutex_t) for FreeBSD.
      return global.params.isLP64 ? 8 : 4;
    }
  else if (global.params.isOpenBSD)
    {
      // sizeof(pthread_mutex_t) for OpenBSD.
      return global.params.isLP64 ? 8 : 4;
    }
  else if (global.params.isOSX)
    {
      // sizeof(pthread_mutex_t) for OSX.
      return global.params.isLP64 ? 64 : 44;
    }
  else if (global.params.isSolaris)
    {
      // sizeof(pthread_mutex_t) for Solaris.
      return 24;
    }

  gcc_unreachable();
}
