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

#include "config.h"

#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "inchash.h"
#include "tree.h"
#include "stringpool.h"
#include "stor-layout.h"
#include "varasm.h"
#include "attribs.h"
#include "tm_p.h"
#include "tree-pretty-print.h"
#include "tree-dump.h"
#include "tree-iterator.h"
#include "diagnostic.h"
#include "plugin.h"

#include "langhooks.h"
#include "langhooks-def.h"
#include "toplev.h"
#include "target.h"
#include "common/common-target.h"
#include "convert.h"
#include "opts.h"

#include "function.h"
#include "gimple-expr.h"
#include "is-a.h"
#include "gimplify.h"
#include "ipa-ref.h"
#include "cgraph.h"

#ifdef optimize
#undef optimize
#endif

#endif
