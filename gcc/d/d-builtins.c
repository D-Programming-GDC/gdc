/* d-builtins.c -- D frontend for GCC.
   Copyright (C) 2011, 2012, 2013 Free Software Foundation, Inc.

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

#include "d-system.h"

#include "d-lang.h"
#include "attrib.h"
#include "module.h"
#include "template.h"
#include "d-codegen.h"

static tree bi_fn_list;
static tree bi_lib_list;
static tree bi_type_list;

// Necessary for built-in struct types
static Array builtin_converted_types;
static Dsymbols builtin_converted_decls;

// Built-in symbols that require special handling.
static Module *std_intrinsic_module;
static Module *std_math_module;
static Module *core_math_module;

static Dsymbol *va_arg_template;
static Dsymbol *va_arg2_template;
static Dsymbol *va_start_template;

// Internal attribute handlers for built-in functions.
static tree handle_noreturn_attribute (tree *, tree, tree, int, bool *);
static tree handle_leaf_attribute (tree *, tree, tree, int, bool *);
static tree handle_const_attribute (tree *, tree, tree, int, bool *);
static tree handle_malloc_attribute (tree *, tree, tree, int, bool *);
static tree handle_pure_attribute (tree *, tree, tree, int, bool *);
static tree handle_novops_attribute (tree *, tree, tree, int, bool *);
static tree handle_nonnull_attribute (tree *, tree, tree, int, bool *);
static tree handle_nothrow_attribute (tree *, tree, tree, int, bool *);
static tree handle_sentinel_attribute (tree *, tree, tree, int, bool *);
static tree handle_type_generic_attribute (tree *, tree, tree, int, bool *);
static tree handle_transaction_pure_attribute (tree *, tree, tree, int, bool *);
static tree handle_returns_twice_attribute (tree *, tree, tree, int, bool *);
static tree handle_fnspec_attribute (tree *, tree, tree, int, bool *);
static tree handle_alias_attribute (tree *, tree, tree, int, bool *);
static tree handle_weakref_attribute (tree *, tree, tree, int, bool *);
static tree ignore_attribute (tree *, tree, tree, int, bool *);

// Array of d type/decl nodes.
tree d_global_trees[DTI_MAX];

// Build D frontend type from tree T type given.

// This will set ctype directly for complex types to save toCtype() the work.
// For others, it is not useful or, in the cast of (C char) -> (D char), will
// cause errors.  This also means char * ...

static Type *
gcc_type_to_d_type (tree t)
{
  Expression *e;
  Type *d;
  unsigned type_size;
  bool is_unsigned;

  const char *structname;
  char structname_buf[64];
  static int struct_serial;
  StructDeclaration *sdecl;

  Type *typefunc_ret;
  tree t_arg_types;
  int t_varargs;
  Parameters *t_args;

  switch (TREE_CODE (t))
    {
    case POINTER_TYPE:
      // Check for char * first. Needs to be done for chars/string.
      if (TYPE_MAIN_VARIANT (TREE_TYPE (t)) == char_type_node)
	return Type::tchar->pointerTo();

      d = gcc_type_to_d_type (TREE_TYPE (t));
      if (d)
	{
	  if (d->ty == Tfunction)
	    return new TypePointer (d);
	  else
	    return d->pointerTo();
	}
      break;

    case REFERENCE_TYPE:
      d = gcc_type_to_d_type (TREE_TYPE (t));
      if (d)
	{
	  // Want to assign ctype directly so that the REFERENCE_TYPE
	  // code can be turned into an InOut argument below.  Can't use
	  // pointerTo(), because that Type is shared.
	  d = new TypePointer (d);
	  d->ctype = t;
	  return d;
	}
      break;

    case BOOLEAN_TYPE:
      // Should be no need for size checking.
      return Type::tbool;

    case INTEGER_TYPE:
      type_size = tree_low_cst (TYPE_SIZE_UNIT (t), 1);
      is_unsigned = TYPE_UNSIGNED (t);

      // This search assumes that integer types come before char and bit...
      for (size_t i = 0; i < TMAX; i++)
	{
	  d = Type::basic[i];
	  if (d && d->isintegral() && d->size() == type_size
	      && (d->isunsigned() ? true : false) == is_unsigned
	      && d->ty != Tint128 && d->ty != Tuns128)
	    return d;
	}
      break;

    case REAL_TYPE:
      type_size = tree_low_cst (TYPE_SIZE_UNIT (t), 1);
      for (size_t i = 0; i < TMAX; i++)
	{
	  d = Type::basic[i];
	  if (d && d->isreal() && d->size() == type_size)
	    return d;
	}
      break;

    case COMPLEX_TYPE:
      type_size = tree_low_cst (TYPE_SIZE_UNIT (t), 1);
      for (size_t i = 0; i < TMAX; i++)
	{
	  d = Type::basic[i];
	  if (d && d->iscomplex() && d->size() == type_size)
	    return d;
	}
      break;

    case VOID_TYPE:
      return Type::tvoid;

    case ARRAY_TYPE:
      d = gcc_type_to_d_type (TREE_TYPE (t));
      if (d)
	{
	  tree index = TYPE_DOMAIN (t);
	  tree ub = TYPE_MAX_VALUE (index);
	  tree lb = TYPE_MIN_VALUE (index);

	  tree length = fold_build2 (MINUS_EXPR, TREE_TYPE (lb), ub, lb);
	  length = size_binop (PLUS_EXPR, size_one_node,
			       convert (sizetype, length));

	  e = new IntegerExp (0, tree_to_hwi (length),
			      Type::tindex);
	  d = new TypeSArray (d, e);
	  d = d->semantic (0, NULL);
	  d->ctype = t;
	  return d;
	}
      break;

    case VECTOR_TYPE:
      d = gcc_type_to_d_type (TREE_TYPE (t));
      if (d)
	{
	  e = new IntegerExp (0, TYPE_VECTOR_SUBPARTS (t), Type::tindex);
	  d = new TypeSArray (d, e);

	  if (d->nextOf()->isTypeBasic() == NULL)
	    break;

	  // Support only 64bit, 128bit, and 256bit vectors.
	  type_size = d->size();
	  if (type_size != 8 && type_size != 16 && type_size != 32)
	    break;

	  d = new TypeVector (0, d);
	  d = d->semantic (0, NULL);
	  return d;
	}
      break;

    case RECORD_TYPE:
      for (size_t i = 0; i < builtin_converted_types.dim; i += 2)
	{
	  tree ti = (tree) builtin_converted_types.data[i];
	  if (TYPE_MAIN_VARIANT (ti) == TYPE_MAIN_VARIANT (t))
	    return (Type *) builtin_converted_types.data[i + 1];
	}

      if (TYPE_NAME (t))
	structname = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (t)));
      else
	{
	  snprintf (structname_buf, sizeof (structname_buf),
		    "__bi_type_%d", ++struct_serial);
	  structname = structname_buf;
	}

      sdecl = new StructDeclaration (0, Lexer::idPool (structname));
      // The gcc.builtins module may not exist yet, so cannot set
      // sdecl->parent here. If it is va_list, the parent needs to
      // be set to the object module which will not exist when
      // this is called.
      sdecl->structsize = int_size_in_bytes (t);
      sdecl->alignsize = TYPE_ALIGN_UNIT (t);
      sdecl->sizeok = SIZEOKdone;

      d = new TypeStruct (sdecl);
      d->ctype = t;

      sdecl->type = d;
      sdecl->handle = d;

      // Does not seem necessary to convert fields, but the
      // members field must be non-null for the above size
      // setting to stick.
      sdecl->members = new Dsymbols;

      builtin_converted_types.push (t);
      builtin_converted_types.push (d);
      builtin_converted_decls.push (sdecl);
      return d;

    case FUNCTION_TYPE:
      typefunc_ret= gcc_type_to_d_type (TREE_TYPE (t));
      if (!typefunc_ret)
	break;

      t_arg_types = TYPE_ARG_TYPES (t);
      t_varargs = t_arg_types != NULL_TREE;
      t_args = new Parameters;

      t_args->reserve (list_length (t_arg_types));
      for (tree tl = t_arg_types; tl != NULL_TREE; tl = TREE_CHAIN (tl))
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
	      d_arg_type = gcc_type_to_d_type (ta);

	      if (!d_arg_type)
		goto Lfail;

	      t_args->push (new Parameter (io, d_arg_type, NULL, NULL));
	    }
	  else
	    t_varargs = 0;
	}
      d = new TypeFunction (t_args, typefunc_ret, t_varargs, LINKc);
      return d;

    Lfail:
      delete t_args;
      break;

    default:
      break;
    }

  return NULL;
}


// Hook from d_builtin_function.
// Add DECL to builtin functions list for maybe processing later
// if gcc.builtins was imported into the current module.

void
d_bi_builtin_func (tree decl)
{
  if (!flag_no_builtin && DECL_ASSEMBLER_NAME_SET_P (decl))
    bi_lib_list = chainon (bi_lib_list, build_tree_list (0, decl));

  bi_fn_list = chainon (bi_fn_list, build_tree_list (0, decl));
}

// Hook from d_register_builtin_type.
// Add DECL to builtin types list for maybe processing later
// if gcc.builtins was imported into the current module.

void
d_bi_builtin_type (tree decl)
{
  bi_type_list = chainon (bi_type_list, build_tree_list (0, decl));
}


// Returns TRUE if M is a module that contains specially handled intrinsics.
// If module was never imported into current compilation, return false.

bool
is_intrinsic_module_p (Module *m)
{
  if (!std_intrinsic_module)
    return false;

  return m == std_intrinsic_module;
}

bool
is_math_module_p (Module *m)
{
  if (std_math_module != NULL)
    {
      if (m == std_math_module)
	return true;
    }

  if (core_math_module != NULL)
    {
      if (m == core_math_module)
	return true;
    }

  return false;
}

// Returns TRUE if D is the built-in va_arg template.  If CSTYLE, test
// if D is the T va_arg() decl, otherwise the void va_arg() decl.

bool
is_builtin_va_arg_p (Dsymbol *d, bool cstyle)
{
  if (cstyle)
    return d == va_arg_template;

  return d == va_arg2_template;
}

// Returns TRUE if D is the built-in va_start template.

bool
is_builtin_va_start_p (Dsymbol *d)
{
  return d == va_start_template;
}

// Helper function for d_gcc_magic_stdarg_module
// In D2, the members of core.vararg are hidden via @system attributes.
// This function should be sufficient in looking through all members.

static void
d_gcc_magic_stdarg_check (Dsymbol *m)
{
  AttribDeclaration *ad = m->isAttribDeclaration();
  TemplateDeclaration *td = m->isTemplateDeclaration();

  if (ad != NULL)
    {
      // Recursively search through attribute decls
      Dsymbols *decl = ad->include (NULL, NULL);
      if (decl && decl->dim)
	{
	  for (size_t i = 0; i < decl->dim; i++)
	    {
	      Dsymbol *sym = (*decl)[i];
	      d_gcc_magic_stdarg_check (sym);
	    }
	}
    }
  else if (td != NULL)
    {
      if (td->ident == Lexer::idPool ("va_arg"))
	{
	  FuncDeclaration *fd;
	  TypeFunction *tf;

	  // Should have nice error message instead of ICE in case someone
	  // tries to tweak the file.
	  gcc_assert (td->members && td->members->dim == 1);
	  fd = ((*td->members)[0])->isFuncDeclaration();
	  gcc_assert (fd && !fd->parameters);
	  tf = (TypeFunction *) fd->type;

	  // Function signature is: T va_arg (va_list va);
	  if (tf->parameters->dim == 1 && tf->nextOf()->ty == Tident)
	    va_arg_template = td;

	  // Function signature is: void va_arg (va_list va, T parm);
	  if (tf->parameters->dim == 2 && tf->nextOf()->ty == Tvoid)
	    va_arg2_template = td;
	}
      else if (td->ident == Lexer::idPool ("va_start"))
	va_start_template = td;
      else
	td = NULL;
    }

  if (td == NULL)     // Not handled.
    return;
}

// Helper function for d_gcc_magic_module.
// Checks all members of the stdarg module M for any special processing.
// core.vararg is different: it expects pointer types (i.e. _argptr)

// We can make it work fine as long as the argument to va_varg is _argptr,
// we just call va_arg on the hidden va_list.  As long _argptr is not
// otherwise modified, it will work.

static void
d_gcc_magic_stdarg_module (Module *m)
{
  Dsymbols *members = m->members;
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *sym = (*members)[i];
      d_gcc_magic_stdarg_check (sym);
    }
}

// Helper function for d_gcc_magic_module.
// Generates all code for gcc.builtins module M.

static void
d_gcc_magic_builtins_module (Module *m)
{
  Dsymbols *funcs = new Dsymbols;

  for (tree n = bi_fn_list; n != NULL_TREE; n = TREE_CHAIN (n))
    {
      tree decl = TREE_VALUE (n);
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      TypeFunction *dtf = (TypeFunction *) gcc_type_to_d_type (TREE_TYPE (decl));

      // Cannot create built-in function type for DECL
      if (!dtf)
	continue;

      // Typically gcc macro functions that can't be represented in D.
      if (dtf->parameters && dtf->parameters->dim == 0 && dtf->varargs)
	continue;

      // %% D2 - builtins are trusted and optionally nothrow.
      // The purity of a builtins can vary depending on compiler
      // flags set at init, or by the -foptions passed, such as
      // flag_unsafe_math_optimizations.

      // Similiarly, if a builtin does not correspond to a function
      // in the standard library (is provided by the compiler), then
      // will also mark the function as weakly pure in the D frontend.
      dtf->trust = TREE_NOTHROW (decl) ? TRUSTtrusted : TRUSTsystem;
      dtf->isnothrow = TREE_NOTHROW (decl);
      dtf->purity = DECL_PURE_P (decl) ?   PUREstrong :
	TREE_READONLY (decl) ? PUREconst :
	DECL_IS_NOVOPS (decl) ? PUREweak :
	!DECL_ASSEMBLER_NAME_SET_P (decl) ? PUREweak :
	PUREimpure;

      FuncDeclaration *func = new FuncDeclaration (0, 0, Lexer::idPool (name),
						   STCextern, dtf);
      func->isym = new Symbol;
      func->isym->Stree = decl;

      funcs->push (func);
    }

  for (tree n = bi_type_list; n != NULL_TREE; n = TREE_CHAIN (n))
    {
      tree decl = TREE_VALUE (n);
      tree type = TREE_TYPE (decl);
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      Type *dt = gcc_type_to_d_type (type);
      // Cannot create built-in type for DECL.
      if (!dt)
	continue;

      funcs->push (new AliasDeclaration (0, Lexer::idPool (name), dt));
    }

  // Iterate through the target-specific builtin types for va_list.
  if (targetm.enum_va_list_p)
    {
      const char *name;
      tree type;

      for (int l = 0; targetm.enum_va_list_p (l, &name, &type); ++l)
	{
	  Type *dt = gcc_type_to_d_type (type);
	  // Cannot create built-in type.
	  if (!dt)
	    continue;

	  funcs->push (new AliasDeclaration (0, Lexer::idPool (name), dt));
	}
    }

  for (size_t i = 0; i < builtin_converted_decls.dim ; ++i)
    {
      Dsymbol *sym = builtin_converted_decls[i];
      // va_list is a pain.  It can be referenced without importing
      // gcc.builtins so it really needs to go in the object module.
      if (!sym->parent)
	{
	  Declaration *decl = sym->isDeclaration();
	  if (!decl || decl->type != Type::tvalist)
	    {
	      sym->parent = m;
	      // Currently, there is no need to run semantic, but we do
	      // want to output inits, etc.
	      funcs->push (sym);
	    }
	}
    }

  // va_list should already be built, so no need to convert to D type again.
  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_va_list"),
				     Type::tvalist));

  // Provide access to target-specific integer types.
  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_clong"),
				     gcc_type_to_d_type (long_integer_type_node)));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_culong"),
				     gcc_type_to_d_type (long_unsigned_type_node)));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_machine_byte"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (byte_mode, 0))));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_machine_ubyte"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (byte_mode, 1))));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_machine_int"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (word_mode, 0))));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_machine_uint"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (word_mode, 1))));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_pointer_int"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (ptr_mode, 0))));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_pointer_uint"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (ptr_mode, 1))));

  // _Unwind_Word has it's own target specific mode.
  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_unwind_int"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (targetm.unwind_word_mode(), 0))));

  funcs->push (new AliasDeclaration (0, Lexer::idPool ("__builtin_unwind_uint"),
				     gcc_type_to_d_type (lang_hooks.types.type_for_mode (targetm.unwind_word_mode(), 1))));

  m->members->push (new LinkDeclaration (LINKc, funcs));
}

// Map extern(C) declarations in member M to GCC library
// builtins by overriding the backend internal symbol.

static void
d_gcc_magic_libbuiltins_check (Dsymbol *m)
{
  AttribDeclaration *ad = m->isAttribDeclaration();
  FuncDeclaration *fd = m->isFuncDeclaration();

  if (ad != NULL)
    {
      // Recursively search through attribute decls
      Dsymbols *decl = ad->include (NULL, NULL);
      if (decl && decl->dim)
	{
	  for (size_t i = 0; i < decl->dim; i++)
	    {
	      Dsymbol *sym = (*decl)[i];
	      d_gcc_magic_libbuiltins_check (sym);
	    }
	}
    }
  else if (fd && !fd->fbody)
    {
      for (tree n = bi_lib_list; n != NULL_TREE; n = TREE_CHAIN (n))
	{
	  tree decl = TREE_VALUE (n);
	  gcc_assert (DECL_ASSEMBLER_NAME_SET_P (decl));

	  const char *name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
	  if (fd->ident == Lexer::idPool (name))
	    {
	      if (Type::tvalist->ty == Tsarray)
		{
		  // As per gcc.builtins module, va_list is passed by reference.
		  TypeFunction *dtf = (TypeFunction *) gcc_type_to_d_type (TREE_TYPE (decl));
		  TypeFunction *tf = (TypeFunction *) fd->type;
		  for (size_t i = 0; i < dtf->parameters->dim; i++)
		    {
		      if ((*dtf->parameters)[i]->type == Type::tvalist)
			(*tf->parameters)[i]->storageClass |= STCref;
		    }
		}
	      fd->isym = new Symbol;
	      fd->isym->Stree = decl;
	      return;
	    }
	}
    }
}

static void
d_gcc_magic_libbuiltins_module (Module *m)
{
  Dsymbols *members = m->members;
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *sym = (*members)[i];
      d_gcc_magic_libbuiltins_check (sym);
    }
}

// Check imported module M for any special processing.
// Modules we look out for are:
//  - gcc.builtins: For all gcc builtins.
//  - core.bitop: For D bitwise intrinsics.
//  - core.math, std.math: For D math intrinsics.
//  - core.stdc.stdarg: For D va_arg intrinsics.

void
d_gcc_magic_module (Module *m)
{
  ModuleDeclaration *md = m->md;
  if (!md || !md->packages || !md->id)
    return;

  if (md->packages->dim == 1)
    {
      if (!strcmp ((md->packages->tdata()[0])->string, "gcc"))
	{
	  if (!strcmp (md->id->string, "builtins"))
	    d_gcc_magic_builtins_module (m);
	}
      else if (!strcmp ((md->packages->tdata()[0])->string, "core"))
	{
	  if (!strcmp (md->id->string, "bitop"))
	    std_intrinsic_module = m;
	  else if (!strcmp (md->id->string, "math"))
	    core_math_module = m;
	}
      else if (!strcmp ((md->packages->tdata()[0])->string, "std"))
	{
	  if (!strcmp (md->id->string, "math"))
	    std_math_module = m;
	}
    }
  else if (md->packages->dim == 2)
    {
      if (!strcmp ((md->packages->tdata()[0])->string, "core")
	  && !strcmp ((md->packages->tdata()[1])->string, "stdc"))
	{
	  if (!strcmp (md->id->string, "stdarg"))
	    d_gcc_magic_stdarg_module (m);
	  else
	    d_gcc_magic_libbuiltins_module (m);
	}
    }
}

// Convert backend evaluated CST to D Frontend Expressions for CTFE

static Expression *
gcc_cst_to_d_expr (tree cst)
{
  STRIP_TYPE_NOPS (cst);
  Type *type = gcc_type_to_d_type (TREE_TYPE (cst));

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
	  return new ComplexExp (0, value, type);
	}
      else if (code == INTEGER_CST)
	{
	  dinteger_t value = cst_to_hwi (TREE_INT_CST (cst));
	  return new IntegerExp (0, value, type);
	}
      else if (code == REAL_CST)
	{
	  real_t value = TREE_REAL_CST (cst);
	  return new RealExp (0, value, type);
	}
      else if (code == STRING_CST)
	{
	  const void *string = TREE_STRING_POINTER (cst);
	  size_t len = TREE_STRING_LENGTH (cst);
	  return new StringExp (0, CONST_CAST (void *, string), len);
	}
    }
  return NULL;
}

// Helper for d_gcc_eval_builtin. Evaluate builtin D
// function BUILTIN whose argument list is ARGUMENTS.
// Return result; NULL if cannot evaluate it.

Expression *
eval_builtin (Loc loc, BUILTIN builtin, Expressions *arguments)
{
  Expression *e = NULL;
  Expression *arg0 = (*arguments)[0];
  Type *t0 = arg0->type;

  tree callee = NULL_TREE;
  tree result;

  set_input_location (loc);

  switch (builtin)
    {
    case BUILTINsin:
      callee = builtin_decl_explicit (BUILT_IN_SINL);
      break;

    case BUILTINcos:
      callee = builtin_decl_explicit (BUILT_IN_COSL);
      break;

    case BUILTINtan:
      callee = builtin_decl_explicit (BUILT_IN_TANL);
      break;

    case BUILTINsqrt:
      if (t0->ty == Tfloat32)
	callee = builtin_decl_explicit (BUILT_IN_SQRTF);
      else if (t0->ty == Tfloat64)
	callee = builtin_decl_explicit (BUILT_IN_SQRT);
      else if (t0->ty == Tfloat80)
	callee = builtin_decl_explicit (BUILT_IN_SQRTL);
      gcc_assert (callee);
      break;

    case BUILTINfabs:
      callee = builtin_decl_explicit (BUILT_IN_FABSL);
      break;

    case BUILTINbsf:
      callee = builtin_decl_explicit (BUILT_IN_CTZL);
      break;

    case BUILTINbsr:
      callee = builtin_decl_explicit (BUILT_IN_CLZL);
      break;

    case BUILTINbswap:
      if (t0->ty == Tint64 || t0->ty == Tuns64)
	callee = builtin_decl_explicit (BUILT_IN_BSWAP64);
      else if (t0->ty == Tint32 || t0->ty == Tuns32)
	callee = builtin_decl_explicit (BUILT_IN_BSWAP32);
      gcc_assert (callee);
      break;

    case BUILTINatan2:
      callee = builtin_decl_explicit (BUILT_IN_ATAN2L);
      break;

    case BUILTINrndtol:
      callee = builtin_decl_explicit (BUILT_IN_LLROUNDL);
      break;

    case BUILTINexpm1:
      callee = builtin_decl_explicit (BUILT_IN_EXPM1L);
      break;

    case BUILTINexp2:
      callee = builtin_decl_explicit (BUILT_IN_EXP2L);
      break;

    case BUILTINyl2x:
    case BUILTINyl2xp1:
      return NULL;

    default:
      gcc_unreachable();
    }

  static IRState irs;
  TypeFunction *tf = (TypeFunction *) gcc_type_to_d_type (TREE_TYPE (callee));
  result = irs.call (tf, callee, NULL, arguments);
  result = fold (result);

  // Special case bsr.
  if (builtin == BUILTINbsr)
    {
      tree type = t0->toCtype();
      tree lhs = size_int (tree_low_cst (TYPE_SIZE (type), 1) - 1);
      result = fold_build2(MINUS_EXPR, type,
			   fold_convert (type, lhs), result);
    }

  if (TREE_CONSTANT (result) && TREE_CODE (result) != CALL_EXPR)
    {
      // Builtin should be successfully evaluated.
      // Will only return NULL if we can't convert it.
      e = gcc_cst_to_d_expr (result);
    }

  return e;

}

// Evaluate builtin D function FD whose argument list is ARGUMENTS.
// Return result; NULL if cannot evaluate it.

Expression *
d_gcc_eval_builtin (Loc loc, FuncDeclaration *fd, Expressions *arguments)
{
  gcc_assert (arguments && arguments->dim);

  if (fd->builtin != BUILTINgcc)
    return eval_builtin (loc, fd->builtin, arguments);
  else
    {
      Expression *e = NULL;
      TypeFunction *tf = (TypeFunction *) fd->type;
      tree callee = NULL_TREE;

      // cirstate is not available.
      static IRState irs;
      set_input_location (loc);
      tree result = irs.call (tf, callee, NULL, arguments);
      result = fold (result);

      // Builtin should be successfully evaluated.
      // Will only return NULL if we can't convert it.
      if (TREE_CONSTANT (result) && TREE_CODE (result) != CALL_EXPR)
	e = gcc_cst_to_d_expr (result);

      return e;
    }
}

Expression *
d_gcc_paint_type (Expression *expr, Type *type)
{
  /* We support up to 512-bit values.  */
  unsigned char buffer[64];
  int len;
  Expression *e;
  tree cst;

  if (type->isintegral())
    cst = build_float_cst (expr->toReal(), expr->type);
  else
    cst = build_integer_cst (expr->toInteger(), expr->type->toCtype());

  len = native_encode_expr (cst, buffer, sizeof (buffer));
  cst = native_interpret_expr (type->toCtype(), buffer, len);

  e = gcc_cst_to_d_expr (cst);
  gcc_assert (e != NULL);

  return e;
}

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
  tree t;
  tree *args = XALLOCAVEC (tree, n);
  va_list list;
  int i;

  va_start (list, n);
  for (i = 0; i < n; ++i)
    {
      builtin_type a = (builtin_type) va_arg (list, int);
      t = builtin_types[a];
      if (t == error_mark_node)
	goto egress;
      args[i] = t;
    }

  t = builtin_types[ret];
  if (t == error_mark_node)
    goto egress;
  if (var)
    t = build_varargs_function_type_array (t, n, args);
  else
    t = build_function_type_array (t, n, args);

 egress:
  builtin_types[def] = t;
  va_end (list);
}


/* Build builtin functions and types for the D language frontend.  */

void
d_init_builtins (void)
{
  tree va_list_ref_type_node;
  tree va_list_arg_type_node;

  // The "standard" abi va_list is va_list_type_node.
  // assumes va_list_type_node already built
  Type::tvalist = gcc_type_to_d_type (va_list_type_node);
  if (!Type::tvalist)
    {
      // fallback to array of byte of the same size?
      error ("cannot represent built in va_list type in D");
      gcc_unreachable();
    }

  // Need to avoid errors in gimplification, else, need to not ICE
  // in targetm.canonical_va_list_type
  Type::tvalist->ctype = va_list_type_node;
  TYPE_LANG_SPECIFIC (va_list_type_node) =
    build_d_type_lang_specific (Type::tvalist);


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
  TREE_TYPE (null_node) = lang_hooks.types.type_for_size (POINTER_SIZE, 0);

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

  /* Imaginary types.  */
  d_ifloat_type_node = build_variant_type_copy (float_type_node);
  D_TYPE_IMAGINARY_FLOAT (d_ifloat_type_node) = 1;

  d_idouble_type_node = build_variant_type_copy (double_type_node);
  D_TYPE_IMAGINARY_FLOAT (d_idouble_type_node) = 1;

  d_ireal_type_node = build_variant_type_copy (long_double_type_node);
  D_TYPE_IMAGINARY_FLOAT (d_ireal_type_node) = 1;

  /* Used for ModuleInfo, ClassInfo, and Interface decls.  */
  d_unknown_type_node = make_node (RECORD_TYPE);

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

  if (!TYPE_NAME (type))
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


/* Table of machine-independent attributes supported in GIMPLE.  */
const struct attribute_spec d_builtins_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler,
       affects_type_identity } */
  { "noreturn",               0, 0, true,  false, false,
			      handle_noreturn_attribute, false },
  { "leaf",                   0, 0, true,  false, false,
			      handle_leaf_attribute, false },
  { "const",                  0, 0, true,  false, false,
			      handle_const_attribute, false },
  { "malloc",                 0, 0, true,  false, false,
			      handle_malloc_attribute, false },
  { "returns_twice",          0, 0, true,  false, false,
			      handle_returns_twice_attribute, false },
  { "pure",                   0, 0, true,  false, false,
			      handle_pure_attribute, false },
  { "nonnull",                0, -1, false, true, true,
			      handle_nonnull_attribute, false },
  { "nothrow",                0, 0, true,  false, false,
			      handle_nothrow_attribute, false },
  { "sentinel",               0, 1, false, true, true,
			      handle_sentinel_attribute, false },
  { "transaction_pure",	      0, 0, false, true, true,
			      handle_transaction_pure_attribute, false },
  /* For internal use (marking of builtins) only.  The name contains space
     to prevent its usage in source code.  */
  { "no vops",                0, 0, true,  false, false,
			      handle_novops_attribute, false },
  { "type generic",           0, 0, false, true, true,
			      handle_type_generic_attribute, false },
  { "fn spec",	 	      1, 1, false, true, true,
			      handle_fnspec_attribute, false },
  /* For internal use only.  The leading '*' both prevents its usage in
     source code and signals that it may be overridden by machine tables.  */
  { "*tm regparm",            0, 0, false, true, true,
			      ignore_attribute, false },
  { "alias",                  1, 1, true,  false, false,
			      handle_alias_attribute, false },
  { "weakref",                0, 1, true,  false, false,
			      handle_weakref_attribute, false },
  { NULL,                     0, 0, false, false, false, NULL, false }
};

/* Give the specifications for the format attributes, used by C and all
   descendants.  */

const struct attribute_spec d_format_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler,
       affects_type_identity } */
  { "format",                 3, 3, false, true,  true,
			      ignore_attribute, false },
  { "format_arg",             1, 1, false, true,  true,
			      ignore_attribute, false },
  { NULL,                     0, 0, false, false, false, NULL, false }
};


/* Built-in Attribute handlers.  */

/* Handle a "noreturn" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noreturn_attribute (tree *node, tree ARG_UNUSED (name),
			   tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			   bool * ARG_UNUSED (no_add_attrs))
{
  tree type = TREE_TYPE (*node);

  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_THIS_VOLATILE (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(build_type_variant (TREE_TYPE (type),
			     TYPE_READONLY (TREE_TYPE (type)), 1));
  else
    gcc_unreachable ();

  return NULL_TREE;
}

/* Handle a "leaf" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_leaf_attribute (tree *node, tree name,
		       tree ARG_UNUSED (args),
		       int ARG_UNUSED (flags), bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }
  if (!TREE_PUBLIC (*node))
    {
      warning (OPT_Wattributes, "%qE attribute has no effect on unit local functions", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "const" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_const_attribute (tree *node, tree ARG_UNUSED (name),
			tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			bool * ARG_UNUSED (no_add_attrs))
{
  tree type = TREE_TYPE (*node);

  /* See FIXME comment on noreturn in c_common_attribute_table.  */
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_READONLY (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(build_type_variant (TREE_TYPE (type), 1,
			     TREE_THIS_VOLATILE (TREE_TYPE (type))));
  else
    gcc_unreachable ();

  return NULL_TREE;
}

/* Handle a "malloc" attribute; arguments as in
   struct attribute_spec.handler.  */

tree
handle_malloc_attribute (tree *node, tree ARG_UNUSED (name),
			 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			 bool * ARG_UNUSED (no_add_attrs))
{
  if (TREE_CODE (*node) == FUNCTION_DECL
      && POINTER_TYPE_P (TREE_TYPE (TREE_TYPE (*node))))
    DECL_IS_MALLOC (*node) = 1;
  else
    gcc_unreachable ();

  return NULL_TREE;
}

/* Handle a "pure" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_pure_attribute (tree *node, tree ARG_UNUSED (name),
		       tree ARG_UNUSED (args), int ARG_UNUSED (flags),
		       bool * ARG_UNUSED (no_add_attrs))
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_PURE_P (*node) = 1;
  else
    gcc_unreachable ();

  return NULL_TREE;
}

/* Handle a "no vops" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_novops_attribute (tree *node, tree ARG_UNUSED (name),
			 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			 bool *ARG_UNUSED (no_add_attrs))
{
  gcc_assert (TREE_CODE (*node) == FUNCTION_DECL);
  DECL_IS_NOVOPS (*node) = 1;
  return NULL_TREE;
}

/* Helper for nonnull attribute handling; fetch the operand number
   from the attribute argument list.  */

static bool
get_nonnull_operand (tree arg_num_expr, unsigned HOST_WIDE_INT *valp)
{
  /* Verify the arg number is a constant.  */
  if (TREE_CODE (arg_num_expr) != INTEGER_CST
      || TREE_INT_CST_HIGH (arg_num_expr) != 0)
    return false;

  *valp = TREE_INT_CST_LOW (arg_num_expr);
  return true;
}

/* Handle the "nonnull" attribute.  */

static tree
handle_nonnull_attribute (tree *node, tree ARG_UNUSED (name),
			  tree args, int ARG_UNUSED (flags),
			  bool * ARG_UNUSED (no_add_attrs))
{
  tree type = *node;

  /* If no arguments are specified, all pointer arguments should be
     non-null.  Verify a full prototype is given so that the arguments
     will have the correct types when we actually check them later.  */
  if (!args)
    {
      gcc_assert (prototype_p (type));
      return NULL_TREE;
    }

  /* Argument list specified.  Verify that each argument number references
     a pointer argument.  */
  for (; args; args = TREE_CHAIN (args))
    {
      tree argument;
      unsigned HOST_WIDE_INT arg_num = 0, ck_num;

      if (!get_nonnull_operand (TREE_VALUE (args), &arg_num))
	gcc_unreachable ();

      argument = TYPE_ARG_TYPES (type);
      if (argument)
	{
	  for (ck_num = 1; ; ck_num++)
	    {
	      if (!argument || ck_num == arg_num)
		break;
	      argument = TREE_CHAIN (argument);
	    }

	  gcc_assert (argument
		      && TREE_CODE (TREE_VALUE (argument)) == POINTER_TYPE);
	}
    }

  return NULL_TREE;
}

/* Handle a "nothrow" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_nothrow_attribute (tree *node, tree ARG_UNUSED (name),
			  tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			  bool * ARG_UNUSED (no_add_attrs))
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_NOTHROW (*node) = 1;
  else
    gcc_unreachable ();

  return NULL_TREE;
}

/* Handle a "sentinel" attribute.  */

static tree
handle_sentinel_attribute (tree *node, tree ARG_UNUSED (name), tree args,
			   int ARG_UNUSED (flags),
			   bool * ARG_UNUSED (no_add_attrs))
{
  gcc_assert (stdarg_p (*node));

  if (args)
    {
      tree position = TREE_VALUE (args);
      gcc_assert (TREE_CODE (position) == INTEGER_CST);
      if (tree_int_cst_lt (position, integer_zero_node))
	gcc_unreachable ();
    }

  return NULL_TREE;
}

/* Handle a "type_generic" attribute.  */

static tree
handle_type_generic_attribute (tree *node, tree ARG_UNUSED (name),
			       tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			       bool * ARG_UNUSED (no_add_attrs))
{
  /* Ensure we have a function type.  */
  gcc_assert (TREE_CODE (*node) == FUNCTION_TYPE);

  /* Ensure we have a variadic function.  */
  gcc_assert (!prototype_p (*node) || stdarg_p (*node));

  return NULL_TREE;
}

/* Handle a "transaction_pure" attribute.  */

static tree
handle_transaction_pure_attribute (tree *node, tree ARG_UNUSED (name),
				   tree ARG_UNUSED (args),
				   int ARG_UNUSED (flags),
				   bool * ARG_UNUSED (no_add_attrs))
{
  /* Ensure we have a function type.  */
  gcc_assert (TREE_CODE (*node) == FUNCTION_TYPE);

  return NULL_TREE;
}

/* Handle a "returns_twice" attribute.  */

static tree
handle_returns_twice_attribute (tree *node, tree ARG_UNUSED (name),
				tree ARG_UNUSED (args),
				int ARG_UNUSED (flags),
				bool * ARG_UNUSED (no_add_attrs))
{
  gcc_assert (TREE_CODE (*node) == FUNCTION_DECL);

  DECL_IS_RETURNS_TWICE (*node) = 1;

  return NULL_TREE;
}

/* Handle a "fn spec" attribute; arguments as in
   struct attribute_spec.handler.  */

tree
handle_fnspec_attribute (tree *node ATTRIBUTE_UNUSED, tree ARG_UNUSED (name),
			 tree args, int ARG_UNUSED (flags),
			 bool *no_add_attrs ATTRIBUTE_UNUSED)
{
  gcc_assert (args
	      && TREE_CODE (TREE_VALUE (args)) == STRING_CST
	      && !TREE_CHAIN (args));
  return NULL_TREE;
}

/* Handle an "alias" or "ifunc" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_alias_attribute (tree *node, tree ARG_UNUSED (name),
			tree args, int ARG_UNUSED (flags),
			bool *no_add_attrs ATTRIBUTE_UNUSED)
{
  gcc_assert (TREE_CODE (*node) == FUNCTION_DECL);
  gcc_assert (!DECL_INITIAL (*node));
  gcc_assert (!decl_function_context (*node));

  tree id = TREE_VALUE (args);
  gcc_assert (TREE_CODE (id) == STRING_CST);

  id = get_identifier (TREE_STRING_POINTER (id));
  /* This counts as a use of the object pointed to.  */
  TREE_USED (id) = 1;
  DECL_INITIAL (*node) = error_mark_node;

  return NULL_TREE;
}

/* Handle a "weakref" attribute; arguments as in struct
   attribute_spec.handler.  */

static tree
handle_weakref_attribute (tree *node, tree ARG_UNUSED (name),
			  tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			  bool *no_add_attrs ATTRIBUTE_UNUSED)
{
  gcc_assert (!decl_function_context (*node));
  gcc_assert (!lookup_attribute ("alias", DECL_ATTRIBUTES (*node)));

  /* Can't call declare_weak because it wants this to be TREE_PUBLIC,
     and that isn't supported; and because it wants to add it to
     the list of weak decls, which isn't helpful.  */
  DECL_WEAK (*node) = 1;

  return NULL_TREE;
}

/* Ignore the given attribute.  Used when this attribute may be usefully
   overridden by the target, but is not used generically.  */

static tree
ignore_attribute (tree * ARG_UNUSED (node), tree ARG_UNUSED (name),
		  tree ARG_UNUSED (args), int ARG_UNUSED (flags),
		  bool *no_add_attrs)
{
  *no_add_attrs = true;
  return NULL_TREE;
}


/* Backend init.  */

void
d_backend_init (void)
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
d_backend_term (void)
{
}

#include "gt-d-d-builtins.h"
