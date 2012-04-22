// d-gcc-includes.h -- D frontend for GCC.
// Originally contributed by David Friedman
// Maintained by Iain Buclaw

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

#ifndef GCC_DCMPLR_DC_GCC_INCLUDES_H
#define GCC_DCMPLR_DC_GCC_INCLUDES_H

// Better to define this here
#define __STDC_FORMAT_MACROS 1

// GMP is C++-aware, so we cannot included it in an extern "C" block.
#include "gmp.h"

// Conflicting definitions between stdio.h and libiberty.h over the throw()
#define HAVE_DECL_ASPRINTF 1

extern "C" {
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"

#include "tree.h"
#include "real.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "debug.h"
#include "flags.h"
#include "toplev.h"
#include "target.h"
#include "function.h"
#include "rtl.h"
#include "diagnostic.h"
#include "output.h"
#include "except.h"
#include "libfuncs.h"
#include "expr.h"
#include "convert.h"
#include "ggc.h"
#include "opts.h"
#include "tm_p.h"

#include "cgraph.h"
#include "tree-iterator.h"
#include "gimple.h"
#include "tree-dump.h"
#include "tree-inline.h"
#include "vec.h"

#include "tree-pretty-print.h"
#include "common/common-target.h"
}

// Undefine things that give us problems
#undef RET

// Apple makes 'optimize' a macro
static inline int gcc_optimize() { return optimize; }
#ifdef optimize
#undef optimize
#endif

#endif
