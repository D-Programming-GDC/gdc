/* d-lang.h -- D frontend for GCC.
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


#ifndef GCC_DCMPLR_DC_BUILTINS_H
#define GCC_DCMPLR_DC_BUILTINS_H

enum built_in_attribute
{
#define DEF_ATTR_NULL_TREE(ENUM) ENUM,
#define DEF_ATTR_INT(ENUM, VALUE) ENUM,
#define DEF_ATTR_STRING(ENUM, VALUE) ENUM,
#define DEF_ATTR_IDENT(ENUM, STRING) ENUM,
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN) ENUM,
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_STRING
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
  ATTR_LAST
};

extern GTY(()) vec<tree, va_gc> *gcc_builtins_functions;
extern GTY(()) vec<tree, va_gc> *gcc_builtins_libfuncs;
extern GTY(()) vec<tree, va_gc> *gcc_builtins_types;

extern GTY(()) tree built_in_attributes[(int) ATTR_LAST];

extern GTY(()) tree string_type_node;
extern GTY(()) tree const_string_type_node;
extern GTY(()) tree wint_type_node;
extern GTY(()) tree intmax_type_node;
extern GTY(()) tree uintmax_type_node;
extern GTY(()) tree signed_size_type_node;

#endif
