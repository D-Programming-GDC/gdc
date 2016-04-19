// d-system.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_DC_SYSTEM_H
#define GCC_DCMPLR_DC_SYSTEM_H

#include "tm.h"
#include "tree.h"
#include "tm_p.h"
#include "tree-pretty-print.h"
#include "tree-dump.h"
#include "ggc.h"
#include "flags.h"
#include "diagnostic.h"
#include "tree-inline.h"

#include "real.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "toplev.h"
#include "target.h"
#include "libfuncs.h"
#include "convert.h"
#include "opts.h"

#include "cgraph.h"
#include "tree-iterator.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "is-a.h"
#include "gimple.h"
#include "vec.h"

#include "common/common-target.h"

/* True if NODE designates a variable declaration.  */
#define VAR_P(NODE) \
  (TREE_CODE (NODE) == VAR_DECL)


// GCC backported macros

#define FUNC_OR_METHOD_TYPE_P(NODE) \
  (TREE_CODE (NODE) == FUNCTION_TYPE || TREE_CODE (NODE) == METHOD_TYPE)

/* The IDENTIFIER_NODE associated with the TYPE_NAME field.  */
#define TYPE_IDENTIFIER(NODE) \
  (TYPE_NAME (NODE) && DECL_P (TYPE_NAME (NODE)) \
   ? DECL_NAME (TYPE_NAME (NODE)) : TYPE_NAME (NODE))

#define ANON_AGGRNAME_PREFIX "__anon_"
#define ANON_AGGRNAME_P(ID_NODE) \
  (!strncmp (IDENTIFIER_POINTER (ID_NODE), ANON_AGGRNAME_PREFIX, \
	     sizeof (ANON_AGGRNAME_PREFIX) - 1))
#define ANON_AGGRNAME_FORMAT "__anon_%d"

/* T is an INTEGER_CST whose numerical value (extended according to
   TYPE_UNSIGNED) fits in an unsigned HOST_WIDE_INT.  Return that
   HOST_WIDE_INT.  */

extern inline __attribute__ ((__gnu_inline__)) HOST_WIDE_INT
tree_to_uhwi (const_tree t)
{
  gcc_assert (cst_and_fits_in_hwi (t));
  return TREE_INT_CST_LOW (t);
}

#endif
