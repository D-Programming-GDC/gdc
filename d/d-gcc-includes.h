/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Iain Buclaw, (C) 2010

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

#ifndef GCC_DCMPLR_DC_GCC_INCLUDES_H
#define GCC_DCMPLR_DC_GCC_INCLUDES_H

// Better to define this here
#define __STDC_FORMAT_MACROS 1

#if D_GCC_VER >= 43
// GMP is C++-aware, so we cannot included it in an extern "C" block.
#include "gmp.h"
#endif

extern "C" {
// Conflicting definitions between stdio.h and libiberty.h over the throw()
#define HAVE_DECL_ASPRINTF 1

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
#if D_GCC_VER >= 44
#include "gimple.h"
#else
#include "tree-gimple.h"
#endif
#include "tree-dump.h"
#include "tree-inline.h"
#include "vec.h"

#if D_GCC_VER == 43
// Defined in tree-flow.h but gives us problems when trying to include it
extern bool useless_type_conversion_p (tree, tree);
#endif

}

// Define our own macro for handling mapped locations as
// future versions of GCC (> 4.3) will poison it's use.
#ifndef D_USE_MAPPED_LOCATION
#if D_GCC_VER <= 43
#  ifdef USE_MAPPED_LOCATION
#    define D_USE_MAPPED_LOCATION 1
#  endif
#else
#define D_USE_MAPPED_LOCATION 1
#endif
#endif

#if D_GCC_VER < 44
/* This macro allows casting away const-ness to pass -Wcast-qual
   warnings.  DO NOT USE THIS UNLESS YOU REALLY HAVE TO!  It should
   only be used in certain specific cases.  One valid case is where
   the C standard definitions or prototypes force you to.  E.g. if you
   need to free a const object, or if you pass a const string to
   execv, et al. */
#undef CONST_CAST
#ifdef __cplusplus
#define CONST_CAST(TYPE,X) (const_cast<TYPE> (X))
#else
#define CONST_CAST(TYPE,X) ((__extension__(union {const TYPE _q; TYPE _nq;})(X))._nq)
#endif

#endif

// Undefine things that give us problems
#undef RET

// Apple makes 'optimize' a macro
static inline int gcc_optimize() { return optimize; }
#ifdef optimize
#undef optimize
#endif

#endif
