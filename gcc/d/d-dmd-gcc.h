/* d-dmd-gcc.h -- D frontend for GCC.
   Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

// Functions defined in the frontend, but used in the glue interface.

// Used in init.cc
FuncDeclaration *search_toString(StructDeclaration *);

// Used in d-lang.cc
void initTraitsStringTable();

// Used in d-codegen.cc
Expression *getTypeInfo(Type *type, Scope *sc);

// Used in d-objfile.cc
void genTypeInfo(Type *type, Scope *sc);

#endif /* GCC_SAFE_DMD */

#endif

