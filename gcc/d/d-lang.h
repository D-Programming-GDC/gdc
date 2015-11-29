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


#ifndef GCC_DCMPLR_DC_LANG_H
#define GCC_DCMPLR_DC_LANG_H

// Forward type declarations to avoid including unnecessary headers.
class Declaration;
class Expression;
class Module;
class Type;

// In d-convert.cc.
tree d_truthvalue_conversion (tree);

// In d-incpath.cc.
extern void add_import_paths(const char *iprefix, const char *imultilib, bool stdinc);

// In d-lang.cc.
void d_add_global_declaration (tree);

Module *d_gcc_get_output_module();

struct lang_type *build_d_type_lang_specific (Type *t);
struct lang_decl *build_d_decl_lang_specific (Declaration *d);
extern tree d_pushdecl (tree);

extern tree d_unsigned_type (tree);
extern tree d_signed_type (tree);

extern void d_init_exceptions();
extern void d_keep (tree t);

extern Modules builtin_modules;

// In d-builtins.cc.
extern const attribute_spec d_langhook_attribute_table[];
extern const attribute_spec d_langhook_common_attribute_table[];
extern const attribute_spec d_langhook_format_attribute_table[];

tree d_builtin_function (tree);
void d_init_builtins();
void d_register_builtin_type (tree, const char *);
void d_build_builtins_module(Module *);
void d_maybe_set_builtin(Module *);
void d_backend_init();
void d_backend_term();

extern Expression *build_expression (tree cst);
extern Type *build_dtype(tree type);

#endif
