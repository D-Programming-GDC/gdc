/* d-frontend.h -- D frontend interface to the gcc backend.
   Copyright (C) 2006-2018 Free Software Foundation, Inc.

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
class Type;
class Module;
class Initializer;
struct OutBuffer;

/* Used in typeinfo.cc.  */
FuncDeclaration *search_toString (StructDeclaration *);

/* Used in d-lang.cc.  */
void gendocfile(Module *);

/* Used in intrinsics.cc.  */
void mangleToBuffer (Type *, OutBuffer *);

/* Used in d-target.cc.  */
const char *toCppMangleItanium (Dsymbol *);
const char *cppTypeInfoMangleItanium (Dsymbol *);

/* Used in decl.cc.  */
Expression *initializerToExpression (Initializer *, Type * = NULL);

#endif  /* ! GCC_D_FRONTEND_H */
