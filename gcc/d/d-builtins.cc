/* d-builtins.c -- D frontend for GCC.
   Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/attrib.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"
#include "dfrontend/module.h"
#include "dfrontend/mtype.h"
#include "dfrontend/template.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "langhooks.h"
#include "target.h"
#include "stringpool.h"
#include "stor-layout.h"

#include "d-tree.h"
#include "d-lang.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "id.h"

static GTY(()) vec<tree, va_gc> *gcc_builtins_functions = NULL;
static GTY(()) vec<tree, va_gc> *gcc_builtins_libfuncs = NULL;
static GTY(()) vec<tree, va_gc> *gcc_builtins_types = NULL;

// Necessary for built-in struct types
struct builtin_sym
{
  builtin_sym(StructDeclaration *d, Type *t, tree c)
    : decl(d), dtype(t), ctype(c)
    { }

  StructDeclaration *decl;
  Type *dtype;
  tree ctype;
};

static vec<builtin_sym *> builtin_converted_decls;

// Array of d type/decl nodes.
tree d_global_trees[DTI_MAX];

// Build D frontend type from tree TYPE type given.

// This will set ctype directly for complex types to save build_ctype() the work.
// For others, it is not useful or, in the cast of (C char) -> (D char), will
// cause errors.  This also means char * ...

Type *
build_dtype(tree type)
{
  Type *dtype;

  switch (TREE_CODE (type))
    {
    case POINTER_TYPE:
      // Check for char * first. Needs to be done for chars/string.
      if (TYPE_MAIN_VARIANT (TREE_TYPE (type)) == char_type_node)
	return Type::tchar->pointerTo();

      dtype = build_dtype(TREE_TYPE (type));
      if (dtype)
	{
	  if (dtype->ty == Tfunction)
	    return new TypePointer(dtype);
	  else
	    return dtype->pointerTo();
	}
      break;

    case REFERENCE_TYPE:
      dtype = build_dtype(TREE_TYPE (type));
      if (dtype)
	{
	  // Want to assign ctype directly so that the REFERENCE_TYPE
	  // code can be turned into an InOut argument below.  Can't use
	  // pointerTo(), because that Type is shared.
	  dtype = new TypePointer(dtype);
	  dtype->ctype = type;
	  return dtype;
	}
      break;

    case BOOLEAN_TYPE:
      // Should be no need for size checking.
      return Type::tbool;

    case INTEGER_TYPE:
    {
      unsigned tsize = TREE_INT_CST_LOW (TYPE_SIZE_UNIT (type));
      bool unsignedp = TYPE_UNSIGNED (type);

      // This search assumes that integer types come before char and bit...
      for (size_t i = 0; i < TMAX; i++)
	{
	  dtype = Type::basic[i];
	  if (dtype && dtype->isintegral() && dtype->size() == tsize
	      && (dtype->isunsigned() ? true : false) == unsignedp
	      && dtype->ty != Tint128 && dtype->ty != Tuns128)
	    return dtype;
	}
      break;
    }

    case REAL_TYPE:
    {
      unsigned tsize = TREE_INT_CST_LOW (TYPE_SIZE_UNIT (type));
      for (size_t i = 0; i < TMAX; i++)
	{
	  dtype = Type::basic[i];
	  if (dtype && dtype->isreal() && dtype->size() == tsize)
	    return dtype;
	}
      break;
    }

    case COMPLEX_TYPE:
    {
      unsigned tsize = TREE_INT_CST_LOW (TYPE_SIZE_UNIT (type));
      for (size_t i = 0; i < TMAX; i++)
	{
	  dtype = Type::basic[i];
	  if (dtype && dtype->iscomplex() && dtype->size() == tsize)
	    return dtype;
	}
      break;
    }

    case VOID_TYPE:
      return Type::tvoid;

    case ARRAY_TYPE:
      dtype = build_dtype(TREE_TYPE (type));
      if (dtype)
	{
	  tree index = TYPE_DOMAIN (type);
	  tree ub = TYPE_MAX_VALUE (index);
	  tree lb = TYPE_MIN_VALUE (index);

	  tree length = fold_build2(MINUS_EXPR, TREE_TYPE (lb), ub, lb);
	  length = size_binop(PLUS_EXPR, size_one_node,
			      convert(sizetype, length));

	  dtype = dtype->sarrayOf(TREE_INT_CST_LOW (length));
	  dtype->ctype = type;
	  return dtype;
	}
      break;

    case VECTOR_TYPE:
      dtype = build_dtype(TREE_TYPE (type));
      if (dtype)
	{
	  dtype = dtype->sarrayOf(TYPE_VECTOR_SUBPARTS (type));

	  if (dtype->nextOf()->isTypeBasic() == NULL)
	    break;

	  // Support only 64bit, 128bit, and 256bit vectors.
	  unsigned tsize = dtype->size();
	  if (tsize != 8 && tsize != 16 && tsize != 32)
	    break;

	  dtype = new TypeVector(Loc(), dtype);
	  return dtype;
	}
      break;

    case RECORD_TYPE:
      for (size_t i = 0; i < builtin_converted_decls.length(); ++i)
	{
	  tree ti = builtin_converted_decls[i]->ctype;
	  if (TYPE_MAIN_VARIANT (ti) == TYPE_MAIN_VARIANT (type))
	    return builtin_converted_decls[i]->dtype;
	}

      if (TYPE_NAME (type))
	{
	  tree structname = DECL_NAME (TYPE_NAME (type));
	  Identifier *ident = Identifier::idPool(IDENTIFIER_POINTER (structname));

	  // The object and gcc.builtins module will not exist when this is
	  // called.  Use a stub 'object' module parent in the meantime.
	  // If gcc.builtins is later imported, the parent will be overridden.
	  static Module *stubmod = new Module("object.d", Id::object, 0, 0);

	  StructDeclaration *sdecl = new StructDeclaration(Loc(), ident);
	  sdecl->parent = stubmod;
	  sdecl->structsize = int_size_in_bytes(type);
	  sdecl->alignsize = TYPE_ALIGN_UNIT (type);
	  sdecl->sizeok = SIZEOKdone;
	  sdecl->type = new TypeStruct(sdecl);
	  sdecl->type->ctype = type;
	  sdecl->type->merge();

	  // Does not seem necessary to convert fields, but the members field
	  // must be non-null for the above size setting to stick.
	  sdecl->members = new Dsymbols;
	  dtype = sdecl->type;
	  builtin_converted_decls.safe_push(new builtin_sym(sdecl, dtype, type));
	  return dtype;
	}
      break;

    case FUNCTION_TYPE:
      dtype = build_dtype(TREE_TYPE (type));
      if (dtype)
	{
	  tree argtypes = TYPE_ARG_TYPES (type);
	  int varargs_p = argtypes != NULL_TREE;

	  Parameters *args = new Parameters;
	  args->reserve(list_length (argtypes));

	  for (tree tl = argtypes; tl != NULL_TREE; tl = TREE_CHAIN (tl))
	    {
	      tree ta = TREE_VALUE (tl);
	      if (ta != void_type_node)
		{
		  Type *d_arg_type;
		  unsigned io = STCin;

		  if (TREE_CODE (ta) == REFERENCE_TYPE)
		    {
		      ta = TREE_TYPE (ta);
		      io = STCref;
		    }
		  d_arg_type = build_dtype(ta);

		  if (!d_arg_type)
		    {
		      delete args;
		      return NULL;
		    }
		  args->push(new Parameter(io, d_arg_type, NULL, NULL));
		}
	      else
		varargs_p = 0;
	    }

	  dtype = new TypeFunction(args, dtype, varargs_p, LINKc);
	  return dtype;
	}
      break;

    default:
      break;
    }

  return NULL;
}

// Convert backend evaluated CST to a D Frontend Expression.
// Typically used for CTFE, returns NULL if it cannot convert CST.

Expression *
build_expression(tree cst)
{
  STRIP_TYPE_NOPS (cst);
  Type *type = build_dtype(TREE_TYPE (cst));

  if (type)
    {
      // Convert our GCC CST tree into a D Expression. This is kinda exper,
      // as it will only be converted back to a tree again later, but this
      // satisifies a need to have gcc builtins CTFE'able.
      tree_code code = TREE_CODE (cst);
      if (code == COMPLEX_CST)
	{
	  complex_t value;
	  value.re = TREE_REAL_CST (TREE_REALPART (cst));
	  value.im = TREE_REAL_CST (TREE_IMAGPART (cst));
	  return new ComplexExp (Loc(), value, type);
	}
      else if (code == INTEGER_CST)
	{
	  dinteger_t value = TREE_INT_CST_LOW (cst);
	  return new IntegerExp(Loc(), value, type);
	}
      else if (code == REAL_CST)
	{
	  real_value value = TREE_REAL_CST (cst);
	  return new RealExp(Loc(), ldouble(value), type);
	}
      else if (code == STRING_CST)
	{
	  const void *string = TREE_STRING_POINTER (cst);
	  size_t len = TREE_STRING_LENGTH (cst);
	  return new StringExp(Loc(), CONST_CAST (void *, string), len);
	}
      else if (code == VECTOR_CST)
	{
	  dinteger_t nunits = VECTOR_CST_NELTS (cst);
	  Expressions *elements = new Expressions;
	  elements->setDim(nunits);

	  for (size_t i = 0; i < nunits; i++)
	    {
	      Expression *elem = build_expression(VECTOR_CST_ELT (cst, i));
	      if (elem == NULL)
		return NULL;

	      (*elements)[i] = elem;
	    }

	  Expression *e = new ArrayLiteralExp(Loc(), elements);
	  e->type = ((TypeVector *) type)->basetype;

	  return new VectorExp(Loc(), e, type);
	}
    }

  return NULL;
}

// Helper function for Target::loadModule.
// Generates all code for gcc.builtins module M.

void
d_build_builtins_module(Module *m)
{
  Dsymbols *funcs = new Dsymbols;
  tree decl;

  for (size_t i = 0; vec_safe_iterate(gcc_builtins_functions, i, &decl); ++i)
    {
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      TypeFunction *dtf = (TypeFunction *) build_dtype(TREE_TYPE (decl));

      // Cannot create built-in function type for DECL
      if (!dtf)
	continue;

      // Typically gcc macro functions that can't be represented in D.
      if (dtf->parameters && dtf->parameters->dim == 0 && dtf->varargs)
	continue;

      // D2 @safe/pure/nothrow functions.
      // It is assumed that builtins solely provided by the compiler are
      // considered @safe, pure and nothrow.  Builtins that correspond to
      // functions in the standard library that don't throw are considered
      // @trusted.  The purity of a builtin can vary depending on compiler
      // flags set upon initialisation, or by the -foptions passed, such as
      // flag_unsafe_math_optimizations.
      // Builtins never use the GC and are always marked as @nogc.
      dtf->isnothrow = TREE_NOTHROW (decl) || !DECL_ASSEMBLER_NAME_SET_P (decl);
      dtf->purity = DECL_PURE_P (decl) ?   PUREstrong :
	TREE_READONLY (decl) ? PUREconst :
	DECL_IS_NOVOPS (decl) ? PUREweak :
	!DECL_ASSEMBLER_NAME_SET_P (decl) ? PUREweak :
	PUREimpure;
      dtf->trust = !DECL_ASSEMBLER_NAME_SET_P (decl) ? TRUSTsafe :
	TREE_NOTHROW (decl) ? TRUSTtrusted :
	TRUSTsystem;
      dtf->isnogc = true;

      FuncDeclaration *func = new FuncDeclaration(Loc(), Loc(), Identifier::idPool(name),
						   STCextern, dtf);
      func->csym = new Symbol;
      func->csym->Sident = name;
      func->csym->Stree = decl;
      func->builtin = BUILTINyes;

      funcs->push(func);
    }

  for (size_t i = 0; vec_safe_iterate(gcc_builtins_types, i, &decl); ++i)
    {
      tree type = TREE_TYPE (decl);
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      Type *dt = build_dtype (type);
      // Cannot create built-in type for DECL.
      if (!dt)
	continue;

      funcs->push(new AliasDeclaration(Loc(), Identifier::idPool(name), dt));
    }

  // Iterate through the target-specific builtin types for va_list.
  if (targetm.enum_va_list_p)
    {
      const char *name;
      tree type;

      for (int l = 0; targetm.enum_va_list_p(l, &name, &type); ++l)
	{
	  Type *dt = build_dtype(type);
	  // Cannot create built-in type.
	  if (!dt)
	    continue;

	  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool(name), dt));
	}
    }

  for (size_t i = 0; i < builtin_converted_decls.length(); ++i)
    {
      // Currently, there is no need to run semantic, but we do
      // want to output inits, etc.
      StructDeclaration *sym = builtin_converted_decls[i]->decl;
      sym->parent = m;
      funcs->push(sym);
    }

  // va_list should already be built, so no need to convert to D type again.
  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_va_list"),
				   Type::tvalist));

  // Provide access to target-specific integer types.
  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_clong"),
				   build_dtype(long_integer_type_node)));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_culong"),
				   build_dtype(long_unsigned_type_node)));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_machine_byte"),
				   build_dtype(lang_hooks.types.type_for_mode(byte_mode, 0))));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_machine_ubyte"),
				   build_dtype(lang_hooks.types.type_for_mode(byte_mode, 1))));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_machine_int"),
				   build_dtype(lang_hooks.types.type_for_mode(word_mode, 0))));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_machine_uint"),
				   build_dtype(lang_hooks.types.type_for_mode(word_mode, 1))));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_pointer_int"),
				   build_dtype(lang_hooks.types.type_for_mode(ptr_mode, 0))));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_pointer_uint"),
				   build_dtype(lang_hooks.types.type_for_mode(ptr_mode, 1))));

  // _Unwind_Word has it's own target specific mode.
  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_unwind_int"),
				   build_dtype(lang_hooks.types.type_for_mode(targetm.unwind_word_mode(), 0))));

  funcs->push(new AliasDeclaration(Loc(), Identifier::idPool("__builtin_unwind_uint"),
				   build_dtype(lang_hooks.types.type_for_mode(targetm.unwind_word_mode(), 1))));

  m->members->push(new LinkDeclaration(LINKc, funcs));
}

// Find and map extern(C) declarations in member M to GCC library
// builtins by overriding the backend internal symbol.

static void
maybe_set_builtin_1(Dsymbol *m)
{
  AttribDeclaration *ad = m->isAttribDeclaration();
  FuncDeclaration *fd = m->isFuncDeclaration();

  if (ad != NULL)
    {
      // Recursively search through attribute decls
      Dsymbols *decl = ad->include(NULL, NULL);
      if (decl && decl->dim)
	{
	  for (size_t i = 0; i < decl->dim; i++)
	    {
	      Dsymbol *sym = (*decl)[i];
	      maybe_set_builtin_1(sym);
	    }
	}
    }
  else if (fd && !fd->fbody)
    {
      tree decl;

      for (size_t i = 0; vec_safe_iterate(gcc_builtins_libfuncs, i, &decl); ++i)
	{
	  gcc_assert(DECL_ASSEMBLER_NAME_SET_P (decl));

	  const char *name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
	  if (fd->ident != Identifier::idPool(name))
	    continue;

	  fd->csym = new Symbol;
	  fd->csym->Sident = name;
	  fd->csym->Stree = decl;
	  fd->builtin = BUILTINyes;
	  return;
	}
    }
}

// Helper function for Target::loadModule.
// Map extern(C) declarations in member M to GCC library
// builtins by overriding the backend internal symbol.

void
d_maybe_set_builtin(Module *m)
{
  Dsymbols *members = m->members;
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *sym = (*members)[i];
      maybe_set_builtin_1(sym);
    }
}

/* Used to help initialize the builtin-types.def table.  When a type of
   the correct size doesn't exist, use error_mark_node instead of NULL.
   The later results in segfaults even when a decl using the type doesn't
   get invoked.  */

static tree
builtin_type_for_size(int size, bool unsignedp)
{
  tree type = lang_hooks.types.type_for_size(size, unsignedp);
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
d_init_attributes()
{
  /* Fill in the built_in_attributes array.  */
#define DEF_ATTR_NULL_TREE(ENUM)                \
  built_in_attributes[(int) ENUM] = NULL_TREE;
# define DEF_ATTR_INT(ENUM, VALUE)                                           \
  built_in_attributes[(int) ENUM] = build_int_cst(NULL_TREE, VALUE);
#define DEF_ATTR_STRING(ENUM, VALUE)                                         \
  built_in_attributes[(int) ENUM] = build_string(strlen(VALUE), VALUE);
#define DEF_ATTR_IDENT(ENUM, STRING)                            \
  built_in_attributes[(int) ENUM] = get_identifier(STRING);
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN) \
  built_in_attributes[(int) ENUM]                       \
  = tree_cons(built_in_attributes[(int) PURPOSE],    \
	      built_in_attributes[(int) VALUE],      \
	      built_in_attributes[(int) CHAIN]);
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_STRING
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
}


static void
do_build_builtin_fn(built_in_function fncode,
		    const char *name,
		    built_in_class fnclass,
		    tree fntype, bool both_p, bool fallback_p,
		    tree fnattrs, bool implicit_p)
{
  tree decl;
  const char *libname;

  if (fntype == error_mark_node)
    return;

  gcc_assert((!both_p && !fallback_p)
	     || !strncmp(name, "__builtin_",
			   strlen("__builtin_")));

  libname = name + strlen("__builtin_");

  decl = add_builtin_function(name, fntype, fncode, fnclass,
			      fallback_p ? libname : NULL, fnattrs);

  set_builtin_decl(fncode, decl, implicit_p);
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
#define DEF_FUNCTION_TYPE_6(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6) NAME,
#define DEF_FUNCTION_TYPE_7(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7) NAME,
#define DEF_FUNCTION_TYPE_8(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8) NAME,
#define DEF_FUNCTION_TYPE_9(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8, ARG9) NAME,
#define DEF_FUNCTION_TYPE_10(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			     ARG6, ARG7, ARG8, ARG9, ARG10) NAME,
#define DEF_FUNCTION_TYPE_11(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			     ARG6, ARG7, ARG8, ARG9, ARG10, ARG11) NAME,
#define DEF_FUNCTION_TYPE_VAR_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_VAR_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_VAR_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_VAR_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_VAR_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_VAR_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
				NAME,
#define DEF_FUNCTION_TYPE_VAR_6(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6) NAME,
#define DEF_FUNCTION_TYPE_VAR_7(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6, ARG7) NAME,
#define DEF_FUNCTION_TYPE_VAR_11(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				 ARG6, ARG7, ARG8, ARG9, ARG10, ARG11) NAME,
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
#undef DEF_FUNCTION_TYPE_8
#undef DEF_FUNCTION_TYPE_9
#undef DEF_FUNCTION_TYPE_10
#undef DEF_FUNCTION_TYPE_11
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_FUNCTION_TYPE_VAR_6
#undef DEF_FUNCTION_TYPE_VAR_7
#undef DEF_FUNCTION_TYPE_VAR_11
#undef DEF_POINTER_TYPE
  BT_LAST
};

typedef enum d_builtin_type builtin_type;

// A temporary array for d_init_builtins.
// Used in communication with def_fn_type.
static tree builtin_types[(int) BT_LAST + 1];

static GTY(()) tree string_type_node;
static GTY(()) tree const_string_type_node;
static GTY(()) tree wint_type_node;
static GTY(()) tree intmax_type_node;
static GTY(()) tree uintmax_type_node;
static GTY(()) tree signed_size_type_node;

/* A helper function for d_init_builtins.  Build function type for DEF with
   return type RET and N arguments.  If VAR is true, then the function should
   be variadic after those N arguments.

   Takes special care not to ICE if any of the types involved are
   error_mark_node, which indicates that said type is not in fact available
   (see builtin_type_for_size).  In which case the function type as a whole
   should be error_mark_node.  */

static void
def_fn_type(builtin_type def, builtin_type ret, bool var, int n, ...)
{
  tree t;
  tree *args = XALLOCAVEC (tree, n);
  va_list list;
  int i;

  va_start(list, n);
  for (i = 0; i < n; ++i)
    {
      builtin_type a = (builtin_type) va_arg(list, int);
      t = builtin_types[a];
      if (t == error_mark_node)
	goto egress;
      args[i] = t;
    }

  t = builtin_types[ret];
  if (t == error_mark_node)
    goto egress;
  if (var)
    t = build_varargs_function_type_array(t, n, args);
  else
    t = build_function_type_array(t, n, args);

 egress:
  builtin_types[def] = t;
  va_end(list);
}


// Build builtin functions and types for the D language frontend.

void
d_init_builtins()
{
  tree va_list_ref_type_node;
  tree va_list_arg_type_node;

  // The "standard" abi va_list is va_list_type_node.
  // This assumes va_list_type_node already built
  Type::tvalist = build_dtype(va_list_type_node);
  if (!Type::tvalist)
    {
      error("cannot represent built in va_list type in D");
      gcc_unreachable();
    }

  // Need to avoid errors in gimplification, else, need to not ICE
  // in targetm.canonical_va_list_type
  Type::tvalist->ctype = va_list_type_node;
  TYPE_LANG_SPECIFIC (va_list_type_node) =
    build_d_type_lang_specific(Type::tvalist);


  if (TREE_CODE (va_list_type_node) == ARRAY_TYPE)
    {
      // It might seem natural to make the argument type a pointer, but there
      // is no implicit casting from arrays to pointers in D.
      va_list_arg_type_node = va_list_type_node;
      va_list_ref_type_node = va_list_arg_type_node;
    }
  else
    {
      va_list_arg_type_node = va_list_type_node;
      va_list_ref_type_node = build_reference_type(va_list_type_node);
    }

  // Maybe it's better not to support these builtin types, which
  // inhibits support for a small number of builtin functions.
  void_list_node = build_tree_list(NULL_TREE, void_type_node);
  string_type_node = build_pointer_type(char_type_node);
  const_string_type_node
    = build_pointer_type(build_qualified_type(char_type_node, TYPE_QUAL_CONST));

  if (strcmp(SIZE_TYPE, "unsigned int") == 0)
    {
      intmax_type_node = integer_type_node;
      uintmax_type_node = unsigned_type_node;
      signed_size_type_node = integer_type_node;
    }
  else if (strcmp(SIZE_TYPE, "long unsigned int") == 0)
    {
      intmax_type_node = long_integer_type_node;
      uintmax_type_node = long_unsigned_type_node;
      signed_size_type_node = long_integer_type_node;
    }
  else if (strcmp(SIZE_TYPE, "long long unsigned int") == 0)
    {
      intmax_type_node = long_long_integer_type_node;
      uintmax_type_node = long_long_unsigned_type_node;
      signed_size_type_node = long_long_integer_type_node;
    }
  else
    gcc_unreachable();

  wint_type_node = unsigned_type_node;
  pid_type_node = integer_type_node;

  // D types are distinct from C types.

  // Integral types.
  byte_type_node = make_signed_type(8);
  ubyte_type_node = make_unsigned_type(8);

  short_type_node = make_signed_type(16);
  ushort_type_node = make_unsigned_type(16);

  int_type_node = make_signed_type(32);
  uint_type_node = make_unsigned_type(32);

  long_type_node = make_signed_type(64);
  ulong_type_node = make_unsigned_type(64);

  cent_type_node = make_signed_type(128);
  ucent_type_node = make_unsigned_type(128);

  {
    // Re-define size_t as a D type.
    machine_mode type_mode = TYPE_MODE(size_type_node);
    size_type_node = lang_hooks.types.type_for_mode(type_mode, 1);
  }

  // Bool and Character types.
  bool_type_node = make_unsigned_type(1);
  TREE_SET_CODE (bool_type_node, BOOLEAN_TYPE);

  char8_type_node = make_unsigned_type(8);
  TYPE_STRING_FLAG (char8_type_node) = 1;

  char16_type_node = make_unsigned_type(16);
  TYPE_STRING_FLAG (char16_type_node) = 1;

  char32_type_node = make_unsigned_type(32);
  TYPE_STRING_FLAG (char32_type_node) = 1;

  // Imaginary types.
  ifloat_type_node = build_distinct_type_copy(float_type_node);
  D_TYPE_IMAGINARY_FLOAT (ifloat_type_node) = 1;

  idouble_type_node = build_distinct_type_copy(double_type_node);
  D_TYPE_IMAGINARY_FLOAT (idouble_type_node) = 1;

  ireal_type_node = build_distinct_type_copy(long_double_type_node);
  D_TYPE_IMAGINARY_FLOAT (ireal_type_node) = 1;

  /* Used for ModuleInfo, ClassInfo, and Interface decls.  */
  unknown_type_node = make_node(RECORD_TYPE);

  {
    /* Make sure we get a unique function type, so we can give
       its pointer type a name.  (This wins for gdb.) */
    tree vtable_entry_type;
    tree vfunc_type = make_node(FUNCTION_TYPE);
    TREE_TYPE (vfunc_type) = int_type_node;
    TYPE_ARG_TYPES (vfunc_type) = NULL_TREE;
    layout_type(vfunc_type);

    vtable_entry_type = build_pointer_type(vfunc_type);

    vtbl_ptr_type_node = build_pointer_type(vtable_entry_type);
    layout_type(vtbl_ptr_type_node);
  }

  /* Since builtin_types isn't gc'ed, don't export these nodes.  */
  memset(builtin_types, 0, sizeof(builtin_types));

#define DEF_PRIMITIVE_TYPE(ENUM, VALUE) \
  builtin_types[(int) ENUM] = VALUE;
#define DEF_FUNCTION_TYPE_0(ENUM, RETURN) \
  def_fn_type(ENUM, RETURN, 0, 0);
#define DEF_FUNCTION_TYPE_1(ENUM, RETURN, ARG1) \
  def_fn_type(ENUM, RETURN, 0, 1, ARG1);
#define DEF_FUNCTION_TYPE_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type(ENUM, RETURN, 0, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type(ENUM, RETURN, 0, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type(ENUM, RETURN, 0, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
  def_fn_type(ENUM, RETURN, 0, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_FUNCTION_TYPE_6(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6)                                       \
  def_fn_type(ENUM, RETURN, 0, 6, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
#define DEF_FUNCTION_TYPE_7(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7)                                 \
  def_fn_type(ENUM, RETURN, 0, 7, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
#define DEF_FUNCTION_TYPE_8(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8)                           \
  def_fn_type(ENUM, RETURN, 0, 8, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,  \
	      ARG7, ARG8);
#define DEF_FUNCTION_TYPE_9(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8, ARG9)                     \
  def_fn_type(ENUM, RETURN, 0, 9, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,  \
	      ARG7, ARG8, ARG9);
#define DEF_FUNCTION_TYPE_10(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8, ARG9, ARG10)              \
  def_fn_type(ENUM, RETURN, 0, 10, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,  \
	      ARG7, ARG8, ARG9, ARG10);
#define DEF_FUNCTION_TYPE_11(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7, ARG8, ARG9, ARG10, ARG11)       \
  def_fn_type(ENUM, RETURN, 0, 11, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,  \
	      ARG7, ARG8, ARG9, ARG10, ARG11);
#define DEF_FUNCTION_TYPE_VAR_0(ENUM, RETURN) \
  def_fn_type(ENUM, RETURN, 1, 0);
#define DEF_FUNCTION_TYPE_VAR_1(ENUM, RETURN, ARG1) \
  def_fn_type(ENUM, RETURN, 1, 1, ARG1);
#define DEF_FUNCTION_TYPE_VAR_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type(ENUM, RETURN, 1, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_VAR_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type(ENUM, RETURN, 1, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_VAR_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type(ENUM, RETURN, 1, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_VAR_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
  def_fn_type(ENUM, RETURN, 1, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_FUNCTION_TYPE_VAR_6(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6)                                 \
  def_fn_type(ENUM, RETURN, 1, 6, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
#define DEF_FUNCTION_TYPE_VAR_7(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				ARG6, ARG7)                                 \
  def_fn_type(ENUM, RETURN, 1, 7, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
#define DEF_FUNCTION_TYPE_VAR_11(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
				 ARG6, ARG7, ARG8, ARG9, ARG10, ARG11)       \
  def_fn_type(ENUM, RETURN, 1, 11, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6,      \
	      ARG7, ARG8, ARG9, ARG10, ARG11);
#define DEF_POINTER_TYPE(ENUM, TYPE) \
  builtin_types[(int) ENUM] = build_pointer_type(builtin_types[(int) TYPE]);

#include "builtin-types.def"

#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_7
#undef DEF_FUNCTION_TYPE_8
#undef DEF_FUNCTION_TYPE_9
#undef DEF_FUNCTION_TYPE_10
#undef DEF_FUNCTION_TYPE_11
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_FUNCTION_TYPE_VAR_6
#undef DEF_FUNCTION_TYPE_VAR_7
#undef DEF_FUNCTION_TYPE_VAR_11
#undef DEF_POINTER_TYPE
  builtin_types[(int) BT_LAST] = NULL_TREE;

  d_init_attributes();

#define DEF_BUILTIN(ENUM, NAME, CLASS, TYPE, LIBTYPE, BOTH_P, FALLBACK_P,   \
		    NONANSI_P, ATTRS, IMPLICIT, COND)			    \
  if (NAME && COND)							    \
    do_build_builtin_fn(ENUM, NAME, CLASS,				    \
			builtin_types[(int) TYPE],			    \
			BOTH_P, FALLBACK_P,				    \
			built_in_attributes[(int) ATTRS], IMPLICIT);
#include "builtins.def"
#undef DEF_BUILTIN

  targetm.init_builtins();

  build_common_builtin_nodes();
}

/* Registration of machine- or os-specific builtin types.
   Add to builtin types list for maybe processing later
   if gcc.builtins was imported into the current module.  */

void
d_register_builtin_type(tree type, const char *name)
{
  tree ident = get_identifier(name);
  tree decl = build_decl(UNKNOWN_LOCATION, TYPE_DECL, ident, type);
  DECL_ARTIFICIAL (decl) = 1;

  if (!TYPE_NAME (type))
    TYPE_NAME (type) = decl;

  vec_safe_push(gcc_builtins_types, decl);
}

/* Add DECL to builtin functions list for maybe processing later
   if gcc.builtins was imported into the current module.  */

tree
d_builtin_function(tree decl)
{
  if (!flag_no_builtin && DECL_ASSEMBLER_NAME_SET_P (decl))
    vec_safe_push(gcc_builtins_libfuncs, decl);

  vec_safe_push(gcc_builtins_functions, decl);
  return decl;
}


#include "gt-d-d-builtins.h"
