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
#include "print-tree.h"

#include "real.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "toplev.h"
#include "attribs.h"
#include "target.h"
#include "libfuncs.h"
#include "convert.h"
#include "opts.h"

#include "cgraph.h"
#include "stor-layout.h"
#include "stringpool.h"
#include "tree-iterator.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "gimple-expr.h"
#include "is-a.h"
#include "gimple.h"
#include "gimplify.h"
#include "varasm.h"
#include "vec.h"

#include "common/common-target.h"
#include "stmt.h"


// GCC backported macros

#define FUNC_OR_METHOD_TYPE_P(NODE) \
  (TREE_CODE (NODE) == FUNCTION_TYPE || TREE_CODE (NODE) == METHOD_TYPE)

/* The IDENTIFIER_NODE associated with the TYPE_NAME field.  */
#define TYPE_IDENTIFIER(NODE) \
  (TYPE_NAME (NODE) && DECL_P (TYPE_NAME (NODE)) \
   ? DECL_NAME (TYPE_NAME (NODE)) : TYPE_NAME (NODE))

#endif
