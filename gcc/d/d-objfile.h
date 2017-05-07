// d-objfile.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_OBFILE_H
#define GCC_DCMPLR_OBFILE_H

extern location_t get_linemap (const Loc& loc);

extern void d_comdat_linkage (tree decl);

extern void d_finish_symbol (tree sym);
extern void d_finish_module();
extern void d_finish_compilation (tree *vec, int len);

extern tree build_artificial_decl(tree type, tree init, const char *prefix = NULL);
extern void build_type_decl (tree t, Dsymbol *dsym);

#endif

