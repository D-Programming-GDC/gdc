// GDC -- D front-end for GCC
// Copyright (C) 2004 David Friedman

// Modified by Iain Buclaw, (C) 2010, 2011, 2012

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#if D_GCC_VER >= 46
#include "tree-pretty-print.h"
#endif

#if D_GCC_VER >= 47
#include "common/common-target.h"
#endif
}

// Undefine things that give us problems
#undef RET

// Apple makes 'optimize' a macro
static inline int gcc_optimize() { return optimize; }
#ifdef optimize
#undef optimize
#endif

#endif
