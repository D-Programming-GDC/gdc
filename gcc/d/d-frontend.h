/* d-frontend.h -- D frontend interface to the gcc backend.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* This file contains declarations defined in the D frontend proper,
   but used in the GDC glue interface.  */

#ifndef GCC_D_FRONTEND_H
#define GCC_D_FRONTEND_H

/* Forward type declarations to avoid including unnecessary headers.  */

class Dsymbol;
class FuncDeclaration;
class StructDeclaration;

/* Used in typeinfo.cc.  */
FuncDeclaration *search_toString (StructDeclaration *);
const char *cppTypeInfoMangle (Dsymbol *);

/* Used in d-lang.cc.  */
void initTraitsStringTable (void);

#endif  /* ! GCC_D_FRONTEND_H */
