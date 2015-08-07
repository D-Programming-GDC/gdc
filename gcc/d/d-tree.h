// d-tree.h -- D frontend for GCC.
// Copyright (C) 2015 Free Software Foundation, Inc.

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


#ifndef GCC_D_TREE_H
#define GCC_D_TREE_H

// Forward type declarations to avoid including unnecessary headers.
class Declaration;
class Statement;
class Type;
struct IRState;

// The kinds of scopes we recognise.
enum level_kind
{
  level_block = 0,	// An ordinary block scope.
  level_try,		// A try-block.
  level_catch,		// A catch-block.
  level_finally,	// A finally-block.
  level_cond,		// An if-condition.
  level_switch,		// A switch-block.
  level_loop,		// A for, do-while, or unrolled-loop block.
  level_with,		// A with-block.
  level_function,	// The block representing an entire function.
};

// For use with break and continue statements.
enum bc_kind
{
  bc_break    = 0,
  bc_continue = 1,
};

// The datatype used to implement D scope.  It is needed primarily to support
// the backend, but also helps debugging information for local variables.
struct GTY((chain_next ("%h.level_chain"))) binding_level
{
  // A chain of declarations. These are in the reverse of the order supplied.
  tree names;

  // For each level (except the global one), a chain of BLOCK nodes for
  // all the levels that were entered and exited one level down.
  tree blocks;

  // The binding level this one is contained in.
  binding_level *level_chain;

  // The kind of scope this object represents.
  ENUM_BITFIELD (level_kind) kind : 4;
};

extern GTY(()) binding_level *current_binding_level;
extern GTY(()) binding_level *global_binding_level;


// Used only for jumps to as-yet undefined labels, since jumps to
// defined labels can have their validity checked immediately.
struct GTY((chain_next ("%h.next"))) d_label_use_entry
{
  d_label_use_entry *next;

  // The statement block associated with the jump.
  Statement * GTY((skip)) statement;

  // The binding level to which this entry is *currently* attached.
  // This is initially the binding level in which the goto appeared,
  // but is modified as scopes are closed.
  binding_level *level;
};

struct GTY(()) d_label_entry
{
  // The label decl itself.
  tree label;

  // The statement block associated with the label.
  Statement * GTY((skip)) statement;

  // The binding level to which this entry is *currently* attached.
  // This is initially the binding level in which the label is defined,
  // but is modified as scopes are closed.
  binding_level *level;

  // A list of forward references of the label.
  d_label_use_entry *fwdrefs;

  // The following bits are set after the label is defined, and are
  // updated as scopes are popped.  They indicate that a backward jump
  // to the label will illegally enter a scope of the given flavor.
  bool in_try_scope;
  bool in_catch_scope;

  // If set, the label we reference represents a break/continue pair.
  bool bc_label;
};


// Nothing is added to tree_identifier.
struct GTY(()) lang_identifier
{
  struct tree_identifier common;
};

// Global state pertinent to the current function.
struct GTY(()) language_function
{
  IRState * GTY((skip)) irs;

  // Table of all used or defined labels in the function.
  hash_map<Statement *, d_label_entry> *labels;
};

// The D front end types have not been integrated into the GCC garbage
// collection system.  Handle this by using the "skip" attribute. */
struct GTY(()) lang_decl
{
  Declaration * GTY((skip)) d_decl;
};

// The lang_type field is not set for every GCC type.
struct GTY(()) lang_type
{
  Type * GTY((skip)) d_type;
};

// Another required, but unused declaration.  This could be simplified, since
// there is no special lang_identifier.
union GTY((desc ("TREE_CODE (&%h.generic) == IDENTIFIER_NODE"),
	   chain_next ("CODE_CONTAINS_STRUCT (TREE_CODE (&%h.generic), TS_COMMON)"
		       " ? ((union lang_tree_node *) TREE_CHAIN (&%h.generic)) : NULL")))
lang_tree_node
{
  union tree_node GTY((tag ("0"),
		       desc ("tree_node_structure (&%h)"))) generic;
  lang_identifier GTY((tag ("1"))) identifier;
};

// True if the Tdelegate typed expression is not really a variable,
// but a literal function / method reference.
#define D_METHOD_CALL_EXPR(NODE) \
  (TREE_LANG_FLAG_0 (NODE))

// True if the type is an imaginary float type.
#define D_TYPE_IMAGINARY_FLOAT(NODE) \
  (TYPE_LANG_FLAG_1 (TREE_CHECK ((NODE), REAL_TYPE)))

// True if the symbol should be made "link one only".  This is used to
// defer calling make_decl_one_only() before the decl has been prepared.
#define D_DECL_ONE_ONLY(NODE) \
  (DECL_LANG_FLAG_0 (NODE))

// True if the symbol is a template member.  Need to distinguish
// between templates and other shared static data so that the latter
// is not affected by -femit-templates.
#define D_DECL_IS_TEMPLATE(NODE) \
  (DECL_LANG_FLAG_1 (NODE))

// True if the decl is a variable case label decl.
#define D_LABEL_VARIABLE_CASE(NODE) \
  (DECL_LANG_FLAG_2 (LABEL_DECL_CHECK (NODE)))

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

#endif
