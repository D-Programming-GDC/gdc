// d-builtins2.cc -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

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

#include "d-gcc-includes.h"
#include "d-lang.h"
#include "attrib.h"
#include "module.h"
#include "template.h"
#include "symbol.h"
#include "d-codegen.h"

static ListMaker bi_fn_list;
static ListMaker bi_type_list;

// Necessary for built-in struct types
static Array builtin_converted_types;
static Dsymbols builtin_converted_decls;

Type *d_gcc_builtin_va_list_d_type;


// Build D frontend type from tree T type given.

// This will set ctype directly for complex types to save toCtype() the work.
// For others, it is not useful or, in the cast of (C char) -> (D char), will
// cause errors.  This also means char * ...

// NOTE: We cannot always use type->pointerTo (in V2, at least) because
// (explanation TBD -- see TypeFunction::semantic and
// TypePointer::semantic: {deco = NULL;} .)

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
      break;

    case INTEGER_TYPE:
      type_size = tree_low_cst (TYPE_SIZE_UNIT (t), 1);
      is_unsigned = TYPE_UNSIGNED (t);

      // This search assumes that integer types come before char and bit...
      for (size_t i = 0; i < TMAX; i++)
	{
	  d = Type::basic[i];
	  if (d && d->isintegral() && d->size() == type_size &&
	      (d->isunsigned() ? true : false) == is_unsigned)
	    return d;
	}
      break;

    case REAL_TYPE:
      // Double and long double may be the same size
      if (TYPE_MAIN_VARIANT (t) == double_type_node)
	return Type::tfloat64;
      else if (TYPE_MAIN_VARIANT (t) == long_double_type_node)
	return Type::tfloat80;

      type_size = tree_low_cst (TYPE_SIZE_UNIT (t), 1);
      for (size_t i = 0; i < TMAX; i++)
	{
	  d = Type::basic[i];
	  if (d && d->isreal() && d->size() == type_size)
	    {
	      return d;
	    }
	}
      break;

    case COMPLEX_TYPE:
      type_size = tree_low_cst (TYPE_SIZE_UNIT (t), 1);
      for (size_t i = 0; i < TMAX; i++)
	{
	  d = Type::basic[i];
	  if (d && d->iscomplex() && d->size() == type_size)
	    {
	      return d;
	    }
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
			       d_convert_basic (sizetype, length));

	  e = new IntegerExp (0, gen.getTargetSizeConst (length),
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
#if STRUCTTHISREF
      sdecl->handle = d;
#else
      sdecl->handle = new TypePointer (d);
#endif
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

// Map D frontend type and sizes to GCC backend types.

void
d_bi_init (void)
{
  // The "standard" abi va_list is va_list_type_node.
  // assumes va_list_type_node already built
  d_gcc_builtin_va_list_d_type = gcc_type_to_d_type (va_list_type_node);
  if (!d_gcc_builtin_va_list_d_type)
    {
      // fallback to array of byte of the same size?
      error ("cannot represent built in va_list type in D");
      gcc_unreachable();
    }

  // Need to avoid errors in gimplification, else, need to not ICE
  // in targetm.canonical_va_list_type
  d_gcc_builtin_va_list_d_type->ctype = va_list_type_node;
  TYPE_LANG_SPECIFIC (va_list_type_node) =
    build_d_type_lang_specific (d_gcc_builtin_va_list_d_type);

  REALSIZE = int_size_in_bytes (long_double_type_node);
  REALPAD = 0;
  REALALIGNSIZE = TYPE_ALIGN_UNIT (long_double_type_node);

  if (POINTER_SIZE == 32)
    Tptrdiff_t = Tint32;
  else if (POINTER_SIZE == 64)
    Tptrdiff_t = Tint64;
  else
    gcc_unreachable();

  PTRSIZE = (POINTER_SIZE / BITS_PER_UNIT);

  CLASSINFO_SIZE = 19 * PTRSIZE;
  CLASSINFO_SIZE_64 = 19 * PTRSIZE;
}

// Hook from d_builtin_function.
// Add DECL to builtin functions list for maybe processing later
// if gcc.builtins was imported into the current module.

void
d_bi_builtin_func (tree decl)
{
  if (gen.useBuiltins)
    bi_fn_list.cons (NULL_TREE, decl);
}

// Hook from d_register_builtin_type.
// Add DECL to builtin types list for maybe processing later
// if gcc.builtins was imported into the current module.

void
d_bi_builtin_type (tree decl)
{
  bi_type_list.cons (NULL_TREE, decl);
}

// Helper function for d_gcc_magic_stdarg_module
// In D2, the members of std.stdarg are hidden via @system attributes.
// This function should be sufficient in looking through all members.

static void
d_gcc_magic_stdarg_check (Dsymbol *m)
{
  Identifier *id_arg = Lexer::idPool ("va_arg");
  Identifier *id_start = Lexer::idPool ("va_start");

  AttribDeclaration *ad = NULL;
  TemplateDeclaration *td = NULL;

  if ((ad = m->isAttribDeclaration()))
    {
      // Recursively search through attribute decls
      Dsymbols *decl = ad->include (NULL, NULL);
      if (decl && decl->dim)
	{
	  for (size_t i = 0; i < decl->dim; i++)
	    {
	      Dsymbol *sym = decl->tdata()[i];
	      d_gcc_magic_stdarg_check (sym);
	    }
	}
    }
  else if ((td = m->isTemplateDeclaration()))
    {
      if (td->ident == id_arg)
	IRState::setCStdArg (td);
      else if (td->ident == id_start)
	IRState::setCStdArgStart (td);
      else
	td = NULL;
    }

  if (td == NULL)     // Not handled.
    return;

  if (TREE_CODE (va_list_type_node) == ARRAY_TYPE)
    {
      // For GCC, a va_list can be an array.  D static arrays are automatically
      // passed by reference, but the 'inout' modifier is not allowed.
      gcc_assert (td->members);
      for (size_t j = 0; j < td->members->dim; j++)
	{
	  FuncDeclaration *fd = (td->members->tdata()[j])->isFuncDeclaration();
	  if (fd && (fd->ident == id_arg || fd->ident == id_start))
	    {
	      TypeFunction *tf;
	      // Should have nice error message instead of ICE in case someone
	      // tries to tweak the file.
	      gcc_assert (!fd->parameters);
	      tf = (TypeFunction *) fd->type;
	      gcc_assert (tf->ty == Tfunction &&
  			  tf->parameters && tf->parameters->dim >= 1);
	      (tf->parameters->tdata()[0])->storageClass &= ~(STCin|STCout|STCref);
	      (tf->parameters->tdata()[0])->storageClass |= STCin;
	    }
	}
    }
}

// Helper function for d_gcc_magic_module.
// Checks all members of the stdarg module M for any special processing.
// std.stdarg is different: it expects pointer types (i.e. _argptr)

// We can make it work fine as long as the argument to va_varg is _argptr,
// we just call va_arg on the hidden va_list.  As long _argptr is not
// otherwise modified, it will work.

static void
d_gcc_magic_stdarg_module (Module *m)
{
  Dsymbols *members = m->members;
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *sym = members->tdata()[i];
      d_gcc_magic_stdarg_check (sym);
    }
}

// Helper function for d_gcc_magic_module.
// Generates all code for gcc.builtins module M.

static void
d_gcc_magic_builtins_module (Module *m)
{
  Dsymbols *funcs = new Dsymbols;

  for (tree n = bi_fn_list.head; n; n = TREE_CHAIN (n))
    {
      tree decl = TREE_VALUE (n);
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      TypeFunction *dtf
	= (TypeFunction *) gcc_type_to_d_type (TREE_TYPE (decl));

      if (!dtf)
	{
	  //warning (0, "cannot create built in function type for %s", name);
	  continue;
	}
      if (dtf->parameters && dtf->parameters->dim == 0 && dtf->varargs)
	{
	  //warning (0, "one-arg va problem: %s", name);
	  continue;
	}

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

  for (tree n = bi_type_list.head; n; n = TREE_CHAIN (n))
    {
      tree decl = TREE_VALUE (n);
      tree type = TREE_TYPE (decl);
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      Type *dt = gcc_type_to_d_type (type);
      if (!dt)
	{
	  //warning (0, "cannot create built in type for %s", name);
	  continue;
	}
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
	  if (!dt)
	    {
	      //warning (0, "cannot create built in type for "%s", name);
	      continue;
	    }
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
	  if (!decl || decl->type != d_gcc_builtin_va_list_d_type)
	    {
	      sym->parent = m;
	      // Currently, there is no need to run semantic, but we do
	      // want to output inits, etc.
	      funcs->push (sym);
	    }
	}
    }

  Type *t = NULL;
  Declaration *d = NULL;

  // va_list should already be built, so no need to convert to D type again.
  t = d_gcc_builtin_va_list_d_type;
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_va_list"), t);
      funcs->push (d);
    }

  // Provide access to target-specific integer types.
  t = gcc_type_to_d_type (long_integer_type_node);
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_Clong"), t);
      funcs->push (d);
    }

  t = gcc_type_to_d_type (long_unsigned_type_node);
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_Culong"), t);
      funcs->push (d);
    }

  t = gcc_type_to_d_type (d_type_for_mode (word_mode, 0));
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_machine_int"), t);
      funcs->push (d);
    }

  t = gcc_type_to_d_type (d_type_for_mode (word_mode, 1));
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_machine_uint"), t);
      funcs->push (d);
    }

  t = gcc_type_to_d_type (d_type_for_mode (ptr_mode, 0));
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_pointer_int"), t);
      funcs->push (d);
    }

  t = gcc_type_to_d_type (d_type_for_mode (ptr_mode, 1));
  if (t)
    {
      d = new AliasDeclaration (0, Lexer::idPool ("__builtin_pointer_uint"), t);
      funcs->push (d);
    }

  m->members->push (new LinkDeclaration (LINKc, funcs));
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
	    IRState::setIntrinsicModule (m, true);
	  else if (!strcmp (md->id->string, "math"))
	    IRState::setMathModule (m, true);
	}
      else if (!strcmp ((md->packages->tdata()[0])->string, "std"))
	{
	  if (!strcmp (md->id->string, "math"))
	    IRState::setMathModule (m, false);
	}
    }
  else if (md->packages->dim == 2)
    {
      if (!strcmp ((md->packages->tdata()[0])->string, "core") &&
	  !strcmp ((md->packages->tdata()[1])->string, "stdc"))
	{
	  if (!strcmp (md->id->string, "stdarg"))
	    d_gcc_magic_stdarg_module (m);
	}
    }
}

// Return GCC align size for type T.

int
d_gcc_type_align (Type *t)
{
  gcc_assert (t->isTypeBasic());
  return TYPE_ALIGN_UNIT (t->toCtype());
}

// Return GCC align size for field VAR.

int
d_gcc_field_align (VarDeclaration *var)
{
  tree field;

  // %% stor-layout.c:
  // Some targets (i.e. i386, VMS) limit struct field alignment
  // to a lower boundary than alignment of variables unless
  // it was overridden by attribute aligned.
  if (var->alignment != (unsigned) STRUCTALIGN_DEFAULT)
    return var->alignment;

  // Work out the correct alignment for the field decl.
  field = make_node (FIELD_DECL);
  DECL_ALIGN (field) = var->type->alignsize() * BITS_PER_UNIT;

#ifdef BIGGEST_FIELD_ALIGNMENT
  DECL_ALIGN (field)
    = MIN (DECL_ALIGN (field), (unsigned) BIGGEST_FIELD_ALIGNMENT);
#endif
#ifdef ADJUST_FIELD_ALIGN
  if (var->type->isTypeBasic())
    {
      TREE_TYPE (field) = var->type->toCtype();
      DECL_ALIGN (field)
	= ADJUST_FIELD_ALIGN (field, DECL_ALIGN (field));
    }
#endif

  // Also controlled by -fpack-struct=
  if (maximum_field_alignment)
    DECL_ALIGN (field) = MIN (DECL_ALIGN (field), maximum_field_alignment);

  return DECL_ALIGN_UNIT (field);
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
	  dinteger_t value = IRState::hwi2toli (TREE_INT_CST (cst));
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
      // TODO: VECTOR... ?
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
  Expression *arg0 = arguments->tdata()[0];
  Type *t0 = arg0->type;

  static IRState irs;
  tree callee = NULL_TREE;
  tree result;
  irs.doLineNote (loc);

  switch (builtin)
    {
    case BUILTINsin:
      callee = d_built_in_decls (BUILT_IN_SINL);
      break;

    case BUILTINcos:
      callee = d_built_in_decls (BUILT_IN_COSL);
      break;

    case BUILTINtan:
      callee = d_built_in_decls (BUILT_IN_TANL);
      break;

    case BUILTINsqrt:
      if (t0->ty == Tfloat32)
	callee = d_built_in_decls (BUILT_IN_SQRTF);
      else if (t0->ty == Tfloat64)
	callee = d_built_in_decls (BUILT_IN_SQRT);
      else if (t0->ty == Tfloat80)
	callee = d_built_in_decls (BUILT_IN_SQRTL);
      gcc_assert (callee);
      break;

    case BUILTINfabs:
      callee = d_built_in_decls (BUILT_IN_FABSL);
      break;

    case BUILTINbsf:
      callee = d_built_in_decls (BUILT_IN_CTZL);
      break;

    case BUILTINbsr:
      callee = d_built_in_decls (BUILT_IN_CLZL);
      break;

    case BUILTINbswap:
      if (t0->ty == Tint64 || t0->ty == Tuns64)
	callee = d_built_in_decls (BUILT_IN_BSWAP64);
      else if (t0->ty == Tint32 || t0->ty == Tuns32)
	callee = d_built_in_decls (BUILT_IN_BSWAP32);
      gcc_assert (callee);
      break;

    case BUILTINatan2:
      callee = d_built_in_decls (BUILT_IN_ATAN2L);
      break;

    case BUILTINrndtol:
      callee = d_built_in_decls (BUILT_IN_LLROUNDL);
      break;

    case BUILTINexpm1:
      callee = d_built_in_decls (BUILT_IN_EXPM1L);
      break;

    case BUILTINexp2:
      callee = d_built_in_decls (BUILT_IN_EXP2L);
      break;

    case BUILTINyl2x:
    case BUILTINyl2xp1:
      return NULL;


    default:
      gcc_unreachable();
    }

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

      // g.irs is not available.
      static IRState irs;
      irs.doLineNote (loc);
      tree result = irs.call (tf, callee, NULL, arguments);
      result = fold (result);

      if (TREE_CONSTANT (result) && TREE_CODE (result) != CALL_EXPR)
	{
	  // Builtin should be successfully evaluated.
	  // Will only return NULL if we can't convert it.
	  e = gcc_cst_to_d_expr (result);
	}

      return e;
    }
}

