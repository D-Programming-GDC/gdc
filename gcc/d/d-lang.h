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

/* Nothing is added to tree_identifier; */
struct GTY(()) lang_identifier
{
  struct tree_identifier common;
};

/* This is required to be defined, but we do not use it. */
struct GTY(()) language_function
{
  int unused;
};

/* The D front end types have not been integrated into the GCC garbage
   collection system.  Handle this by using the "skip" attribute. */
class Declaration;
typedef Declaration *DeclarationGTYP;
struct GTY(()) lang_decl
{
  DeclarationGTYP GTY((skip)) d_decl;
};

/* The lang_type field is not set for every GCC type. */
class Type;
typedef Type *TypeGTYP;
struct GTY(()) lang_type
{
  TypeGTYP GTY((skip)) d_type;
};

/* Another required, but unused declaration.  This could be simplified, since
   there is no special lang_identifier */
union GTY((desc ("TREE_CODE (&%h.generic) == IDENTIFIER_NODE"),
	   chain_next ("CODE_CONTAINS_STRUCT (TREE_CODE (&%h.generic), TS_COMMON)"
		       " ? ((union lang_tree_node *) TREE_CHAIN (&%h.generic)) : NULL")))
lang_tree_node
{
  union tree_node GTY((tag ("0"),
		       desc ("tree_node_structure (&%h)"))) generic;
  lang_identifier GTY((tag ("1"))) identifier;
};

extern GTY(()) tree d_eh_personality_decl;

/* True if the Tdelegate typed expression is not really a variable,
   but a literal function / method reference.  */
#define D_METHOD_CALL_EXPR(NODE) (TREE_LANG_FLAG_0 (NODE))

/* True if the type is an imaginary float type.  */
#define D_TYPE_IMAGINARY_FLOAT(NODE) (TYPE_LANG_FLAG_1 (TREE_CHECK ((NODE), REAL_TYPE)))

/* True if the symbol should be made "link one only".  This is used to
   defer calling make_decl_one_only() before the decl has been prepared. */
#define D_DECL_ONE_ONLY(NODE) (DECL_LANG_FLAG_0 (NODE))

/* True if the symbol is a template member.  Need to distinguish
   between templates and other shared static data so that the latter
   is not affected by -femit-templates. */
#define D_DECL_IS_TEMPLATE(NODE) (DECL_LANG_FLAG_1 (NODE))

/* The D front-end does not use the 'binding level' system for a symbol table,
   It is only needed to get debugging information for local variables and
   otherwise support the backend. */
struct GTY(()) binding_level
{
  /* A chain of declarations. These are in the reverse of the order supplied. */
  tree names;

  /* A pointer to the end of the names chain. Only needed to facilitate
     a quick test if a decl is in the list by checking if its TREE_CHAIN
     is not NULL or it is names_end (in pushdecl_nocheck()). */
  tree names_end;

  /* For each level (except the global one), a chain of BLOCK nodes for
     all the levels that were entered and exited one level down. */
  tree blocks;

  /* The BLOCK node for this level, if one has been preallocated.
     If NULL_TREE, the BLOCK is allocated (if needed) when the level is popped. */
  tree this_block;

  /* The binding level this one is contained in. */
  binding_level *level_chain;
};

extern GTY(()) binding_level *current_binding_level;
extern GTY(()) binding_level *global_binding_level;

enum d_tree_index
{
  DTI_VTBL_PTR_TYPE,

  DTI_BOOL_TYPE,
  DTI_CHAR_TYPE,
  DTI_WCHAR_TYPE,
  DTI_DCHAR_TYPE,

  DTI_BYTE_TYPE,
  DTI_UBYTE_TYPE,
  DTI_SHORT_TYPE,
  DTI_USHORT_TYPE,
  DTI_INT_TYPE,
  DTI_UINT_TYPE,
  DTI_LONG_TYPE,
  DTI_ULONG_TYPE,
  DTI_CENT_TYPE,
  DTI_UCENT_TYPE,

  DTI_IFLOAT_TYPE,
  DTI_IDOUBLE_TYPE,
  DTI_IREAL_TYPE,

  DTI_UNKNOWN_TYPE,

  DTI_MAX
};

extern GTY(()) tree d_global_trees[DTI_MAX];

#define vtbl_ptr_type_node		d_global_trees[DTI_VTBL_PTR_TYPE]
#define bool_type_node			d_global_trees[DTI_BOOL_TYPE]
#define char8_type_node			d_global_trees[DTI_CHAR_TYPE]
#define char16_type_node		d_global_trees[DTI_DCHAR_TYPE]
#define char32_type_node		d_global_trees[DTI_WCHAR_TYPE]
#define byte_type_node			d_global_trees[DTI_BYTE_TYPE]
#define ubyte_type_node			d_global_trees[DTI_UBYTE_TYPE]
#define short_type_node			d_global_trees[DTI_SHORT_TYPE]
#define ushort_type_node		d_global_trees[DTI_USHORT_TYPE]
#define int_type_node			d_global_trees[DTI_INT_TYPE]
#define uint_type_node			d_global_trees[DTI_UINT_TYPE]
#define long_type_node			d_global_trees[DTI_LONG_TYPE]
#define ulong_type_node			d_global_trees[DTI_ULONG_TYPE]
#define cent_type_node			d_global_trees[DTI_CENT_TYPE]
#define ucent_type_node			d_global_trees[DTI_UCENT_TYPE]
#define ifloat_type_node		d_global_trees[DTI_IFLOAT_TYPE]
#define idouble_type_node		d_global_trees[DTI_IDOUBLE_TYPE]
#define ireal_type_node			d_global_trees[DTI_IREAL_TYPE]
#define d_unknown_type_node		d_global_trees[DTI_UNKNOWN_TYPE]


/* In d-lang.cc.  */
tree d_truthvalue_conversion (tree);
void d_add_global_declaration (tree);

class Module;
Module *d_gcc_get_output_module();

struct lang_type *build_d_type_lang_specific (Type *t);
struct lang_decl *build_d_decl_lang_specific (Declaration *d);

/* In asmstmt.cc */
tree d_build_asm_stmt (tree insn_tmpl, tree outputs, tree inputs, tree clobbers);

/* In d-incpath.cc */
extern const char *iprefix;
extern const char *multilib_dir;
extern void add_import_paths (bool stdinc);
extern void add_phobos_versyms();

/* In d-lang.cc */
extern tree d_pushdecl (tree);
extern void push_binding_level();
extern tree pop_binding_level (int, int);

extern void init_global_binding_level();
extern void set_decl_binding_chain (tree decl_chain);

extern tree d_unsigned_type (tree);
extern tree d_signed_type (tree);

extern void d_init_exceptions();

extern void d_keep (tree t);
extern void d_free (tree t);

extern void set_block (tree);


/* In d-builtins.cc */
extern const attribute_spec d_builtins_attribute_table[];
extern const attribute_spec d_format_attribute_table[];
tree d_builtin_function (tree);
void d_init_builtins();
void d_register_builtin_type (tree, const char *);
void d_backend_init();
void d_backend_term();

class Expression;
extern Expression *build_expression (tree cst);
extern Type *build_dtype(tree type);

#include "d-dmd-gcc.h"

#endif
