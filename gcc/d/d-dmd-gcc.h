/* d-dmd-gcc.h -- D frontend for GCC.
   Copyright (C) 2011, 2012 Free Software Foundation, Inc.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.
*/

/* This file contains declarations used by the modified DMD front-end to
   interact with GCC-specific code. */

#ifndef GCC_DCMPLR_DMD_GCC_H
#define GCC_DCMPLR_DMD_GCC_H

#ifndef GCC_SAFE_DMD

#include "mars.h"
#include "arraytypes.h"

/* used in module.c */
extern void d_gcc_magic_module (Module *);

/* used in template.c */
extern bool d_gcc_force_templates (void);
extern Module *d_gcc_get_output_module (void);

/* used in interpret.c */
extern Expression *d_gcc_eval_builtin (Loc, FuncDeclaration *, Expressions *);

/* used in arrayop.c */
extern int binary(const char *p , const char **tab, int high);

/* used in ctfeexpr.c */
extern Expression *d_gcc_paint_type (Expression *, Type *);

#endif /* GCC_SAFE_DMD */

#endif

