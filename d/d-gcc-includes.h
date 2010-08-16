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


#if D_GCC_VER >= 43
// GMP is C++-aware, so we cannot included it in an extern "C" block.
#include "gmp.h"
#endif

extern "C" {

// hack needed to prevent inclusion of the generated insn-flags.h
// which defines some inline functions that use C prototypes....
#define GCC_INSN_FLAGS_H

// Conflicting definitions between stdio.h and libiberty.h over the throw()
#define HAVE_DECL_ASPRINTF 1

#include "config.h"
#include "system.h"

/* Before gcc 4.0, <stdbool.h> was included before defining bool.  In 4.0,
   it is always defined as "unsigned char" unless __cplusplus.  Have to make
   sure the "bool" under c++ is the same so that structs are laid out
   correctly. */
#if D_GCC_VER >= 40
#define bool unsigned char
#endif

#include "coretypes.h"
#include "tm.h"
#include "cpplib.h"
#include "cppdefault.h"
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

#if D_GCC_VER >= 40
#include "cgraph.h"
#include "tree-iterator.h"
#include "tree-gimple.h"
#include "tree-dump.h"
#include "tree-inline.h"
#endif
    
#if D_GCC_VER >= 41
#include "vec.h"
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

// Undefine things that give us problems
#undef RET

// Apple makes 'optimize' a macro
static inline int gcc_optimize() { return optimize; }
#ifdef optimize
#undef optimize
#endif

#endif
