/* d-builtins.c -- D frontend for GCC.
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

/* This file is mostly a copy of gcc/c-common.c */
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "intl.h"
#include "tree.h"
#include "flags.h"
#include "ggc.h"
#include "tm_p.h"
#include "target.h"
#include "common/common-target.h"
#include "langhooks.h"
#include "tree-inline.h"
#include "toplev.h"
#include "diagnostic.h"
#include "opts.h"
#include "cgraph.h"

#include "d-lang.h"
/* Implements GCC attributes in D. */
#include "d-bi-attrs.h"

/* Array of d type/decl nodes. */
tree d_global_trees[DTI_MAX];

/* Used to help initialize the builtin-types.def table.  When a type of
   the correct size doesn't exist, use error_mark_node instead of NULL.
   The later results in segfaults even when a decl using the type doesn't
   get invoked.  */

static tree
builtin_type_for_size (int size, bool unsignedp)
{
  tree type = lang_hooks.types.type_for_size (size, unsignedp);
  return type ? type : error_mark_node;
}

/* Handle default attributes.  */

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

static GTY(()) tree built_in_attributes[(int) ATTR_LAST];

static void
d_init_attributes (void)
{
  /* Fill in the built_in_attributes array.  */
#define DEF_ATTR_NULL_TREE(ENUM)                \
  built_in_attributes[(int) ENUM] = NULL_TREE;
# define DEF_ATTR_INT(ENUM, VALUE)                                           \
  built_in_attributes[(int) ENUM] = build_int_cst (NULL_TREE, VALUE);
#define DEF_ATTR_STRING(ENUM, VALUE)                                         \
  built_in_attributes[(int) ENUM] = build_string (strlen (VALUE), VALUE);
#define DEF_ATTR_IDENT(ENUM, STRING)                            \
  built_in_attributes[(int) ENUM] = get_identifier (STRING);
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN) \
  built_in_attributes[(int) ENUM]                       \
  = tree_cons (built_in_attributes[(int) PURPOSE],    \
   	       built_in_attributes[(int) VALUE],      \
   	       built_in_attributes[(int) CHAIN]);
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_STRING
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
}


#ifndef WINT_TYPE
#define WINT_TYPE "unsigned int"
#endif
#ifndef PID_TYPE
#define PID_TYPE "int"
#endif

static tree
lookup_ctype_name (const char *p)
{
  // These are the names used in c_common_nodes_and_builtins
  if (strcmp (p, "char"))
    return char_type_node;
  else if (strcmp (p, "signed char"))
    return signed_char_type_node;
  else if (strcmp (p, "unsigned char"))
    return unsigned_char_type_node;
  else if (strcmp (p, "short int"))
    return short_integer_type_node;
  else if (strcmp (p, "short unsigned int "))
    return short_unsigned_type_node; //cxx! -- affects ming/c++?
  else if (strcmp (p, "int"))
    return integer_type_node;
  else if (strcmp (p, "unsigned int"))
    return unsigned_type_node;
  else if (strcmp (p, "long int"))
    return long_integer_type_node;
  else if (strcmp (p, "long unsigned int"))
    return long_unsigned_type_node; // cxx!
  else if (strcmp (p, "long long int"))
    return long_integer_type_node;
  else if (strcmp (p, "long long unsigned int"))
    return long_unsigned_type_node; // cxx!

  internal_error ("unsigned C type '%s'", p);
}

static void
do_build_builtin_fn (enum built_in_function fncode,
 		     const char *name,
 		     enum built_in_class fnclass,
 		     tree fntype, bool both_p, bool fallback_p,
 		     tree fnattrs, bool implicit_p)
{
  tree decl;
  const char *libname;

  if (fntype == error_mark_node)
    return;

  gcc_assert ((!both_p && !fallback_p)
	      || !strncmp (name, "__builtin_",
			   strlen ("__builtin_")));

  libname = name + strlen ("__builtin_");

  decl = add_builtin_function (name, fntype, fncode, fnclass,
 			       fallback_p ? libname : NULL, fnattrs);

  set_builtin_decl (fncode, decl, implicit_p);
}

enum d_builtin_type
{
#define DEF_PRIMITIVE_TYPE(NAME, VALUE) NAME,
#define DEF_FUNCTION_TYPE_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) NAME,
#define DEF_FUNCTION_TYPE_6(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) NAME,
#define DEF_FUNCTION_TYPE_7(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) NAME,
#define DEF_FUNCTION_TYPE_VAR_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_VAR_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_VAR_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_VAR_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_VAR_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_VAR_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG6) NAME,
#define DEF_POINTER_TYPE(NAME, TYPE) NAME,
#include "builtin-types.def"
#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_0
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_7
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_POINTER_TYPE
  BT_LAST
};
typedef enum d_builtin_type builtin_type;


/* A temporary array for d_init_builtins.  Used in communication with def_fn_type.  */

static tree builtin_types[(int) BT_LAST + 1];


/* A helper function for d_init_builtins.  Build function type for DEF with
   return type RET and N arguments.  If VAR is true, then the function should
   be variadic after those N arguments.

   Takes special care not to ICE if any of the types involved are
   error_mark_node, which indicates that said type is not in fact available
   (see builtin_type_for_size).  In which case the function type as a whole
   should be error_mark_node.  */

static void
def_fn_type (builtin_type def, builtin_type ret, bool var, int n, ...)
{
  tree args = NULL, t;
  va_list list;
  int i;

  va_start (list, n);
  for (i = 0; i < n; ++i)
    {
      builtin_type a = (builtin_type) va_arg (list, int);
      t = builtin_types[a];
      if (t == error_mark_node)
	goto egress;
      args = tree_cons (NULL_TREE, t, args);
    }
  va_end (list);

  args = nreverse (args);
  if (!var)
    args = chainon (args, void_list_node);

  t = builtin_types[ret];
  if (t == error_mark_node)
    goto egress;
  t = build_function_type (t, args);

 egress:
  builtin_types[def] = t;
}


/* Build builtin functions and types for the D language frontend.  */

void
d_init_builtins (void)
{
  tree va_list_ref_type_node;
  tree va_list_arg_type_node;

  d_bi_init ();

  if (TREE_CODE (va_list_type_node) == ARRAY_TYPE)
    {
      /* It might seem natural to make the reference type a pointer,
	 but this will not work in D: There is no implicit casting from
	 an array to a pointer. */
      va_list_arg_type_node = build_reference_type (va_list_type_node);
      va_list_ref_type_node = va_list_arg_type_node;
    }
  else
    {
      va_list_arg_type_node = va_list_type_node;
      va_list_ref_type_node = build_reference_type (va_list_type_node);
    }

  intmax_type_node = intDI_type_node;
  uintmax_type_node = unsigned_intDI_type_node;
  signed_size_type_node = d_signed_type (size_type_node);
  string_type_node = build_pointer_type (char_type_node);
  const_string_type_node = build_pointer_type (build_qualified_type
 					       (char_type_node, TYPE_QUAL_CONST));

  void_list_node = build_tree_list (NULL_TREE, void_type_node);

  /* WINT_TYPE is a C type name, not an itk_ constant or something useful
     like that... */
  tree wint_type_node = lookup_ctype_name (WINT_TYPE);
  pid_type_node = lookup_ctype_name (PID_TYPE);

  /* Create the built-in __null node.  It is important that this is
     not shared.  */
  null_node = make_node (INTEGER_CST);
  TREE_TYPE (null_node) = d_type_for_size (POINTER_SIZE, 0);

  TYPE_NAME (integer_type_node) = build_decl (UNKNOWN_LOCATION, TYPE_DECL,
					      get_identifier ("int"), integer_type_node);
  TYPE_NAME (char_type_node) = build_decl (UNKNOWN_LOCATION, TYPE_DECL,
					   get_identifier ("char"), char_type_node);

  /* Types specific to D (but so far all taken from C).  */
  d_void_zero_node = make_node (INTEGER_CST);
  TREE_TYPE (d_void_zero_node) = void_type_node;

  d_null_pointer = convert (ptr_type_node, integer_zero_node);

  /* D variant types of C types.  */
  d_boolean_type_node = make_unsigned_type (1);
  TREE_SET_CODE (d_boolean_type_node, BOOLEAN_TYPE);

  d_char_type_node = build_variant_type_copy (unsigned_intQI_type_node);
  d_wchar_type_node = build_variant_type_copy (unsigned_intHI_type_node);
  d_dchar_type_node = build_variant_type_copy (unsigned_intSI_type_node);
  d_ifloat_type_node = build_variant_type_copy (float_type_node);
  d_idouble_type_node = build_variant_type_copy (double_type_node);
  d_ireal_type_node = build_variant_type_copy (long_double_type_node);

  {
    /* Make sure we get a unique function type, so we can give
       its pointer type a name.  (This wins for gdb.) */
    tree vtable_entry_type;
    tree vfunc_type = make_node (FUNCTION_TYPE);
    TREE_TYPE (vfunc_type) = integer_type_node;
    TYPE_ARG_TYPES (vfunc_type) = NULL_TREE;
    layout_type (vfunc_type);

    vtable_entry_type = build_pointer_type (vfunc_type);

    d_vtbl_ptr_type_node = build_pointer_type (vtable_entry_type);
    layout_type (d_vtbl_ptr_type_node);
  }

  /* Since builtin_types isn't gc'ed, don't export these nodes.  */
  memset (builtin_types, 0, sizeof (builtin_types));

#define DEF_PRIMITIVE_TYPE(ENUM, VALUE) \
  builtin_types[(int) ENUM] = VALUE;
#define DEF_FUNCTION_TYPE_0(ENUM, RETURN) \
  def_fn_type (ENUM, RETURN, 0, 0);
#define DEF_FUNCTION_TYPE_1(ENUM, RETURN, ARG1) \
  def_fn_type (ENUM, RETURN, 0, 1, ARG1);
#define DEF_FUNCTION_TYPE_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type (ENUM, RETURN, 0, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type (ENUM, RETURN, 0, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type (ENUM, RETURN, 0, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
  def_fn_type (ENUM, RETURN, 0, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_FUNCTION_TYPE_6(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6)                                       \
  def_fn_type (ENUM, RETURN, 0, 6, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
#define DEF_FUNCTION_TYPE_7(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7)                                 \
  def_fn_type (ENUM, RETURN, 0, 7, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
#define DEF_FUNCTION_TYPE_VAR_0(ENUM, RETURN) \
  def_fn_type (ENUM, RETURN, 1, 0);
#define DEF_FUNCTION_TYPE_VAR_1(ENUM, RETURN, ARG1) \
  def_fn_type (ENUM, RETURN, 1, 1, ARG1);
#define DEF_FUNCTION_TYPE_VAR_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type (ENUM, RETURN, 1, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_VAR_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type (ENUM, RETURN, 1, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_VAR_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type (ENUM, RETURN, 1, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_VAR_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
  def_fn_type (ENUM, RETURN, 1, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_POINTER_TYPE(ENUM, TYPE) \
  builtin_types[(int) ENUM] = build_pointer_type (builtin_types[(int) TYPE]);

#include "builtin-types.def"

#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_POINTER_TYPE
  builtin_types[(int) BT_LAST] = NULL_TREE;

  d_init_attributes ();

#define DEF_BUILTIN(ENUM, NAME, CLASS, TYPE, LIBTYPE, BOTH_P, FALLBACK_P, \
		    NONANSI_P, ATTRS, IMPLICIT, COND)                   \
  if (NAME && COND)                                                     \
  do_build_builtin_fn (ENUM, NAME, CLASS,                            \
 		       builtin_types[(int) TYPE],                    \
 		       BOTH_P, FALLBACK_P,                           \
 		       built_in_attributes[(int) ATTRS], IMPLICIT);
#include "builtins.def"
#undef DEF_BUILTIN

  targetm.init_builtins ();

  build_common_builtin_nodes ();
}

/* Registration of machine- or os-specific builtin types.  */
void
d_register_builtin_type (tree type, const char *name)
{
  tree ident = get_identifier (name);
  tree decl = build_decl (UNKNOWN_LOCATION, TYPE_DECL, ident, type);
  DECL_ARTIFICIAL (decl) = 1;

  if (! TYPE_NAME (type))
    TYPE_NAME (type) = decl;

  d_bi_builtin_type (decl);
}

/* Return a definition for a builtin function named NAME and whose data type
   is TYPE.  TYPE should be a function type with argument types.
   FUNCTION_CODE tells later passes how to compile calls to this function.
   See tree.h for its possible values.

   If LIBRARY_NAME is nonzero, use that for DECL_ASSEMBLER_NAME,
   the name to be called if we can't opencode the function.  If
   ATTRS is nonzero, use that for the function's attribute list.  */

tree
d_builtin_function (tree decl)
{
  d_bi_builtin_func (decl);
  return decl;
}


/* Backend init.  */

void
gcc_d_backend_init (void)
{
  init_global_binding_level ();

  /* This allows the code in d-builtins2 to not have to worry about
     converting (C signed char *) to (D char *) for string arguments of
     built-in functions.
     Parameters are (signed_char = false, short_double = false).  */
  build_common_tree_nodes (false, false);

  d_init_builtins ();

  if (flag_exceptions)
    d_init_exceptions ();

  /* This is the C main, not the D main.  */
  main_identifier_node = get_identifier ("main");
}


/* Backend term.  */

void
gcc_d_backend_term (void)
{
}

#include "gt-d-d-builtins.h"
