// d-decls.cc -- D frontend for GCC.
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


//This file is based on dmd/tocsym.c.  Original copyright:

// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.


#include "d-gcc-includes.h"

#include "mars.h"
#include "statement.h"
#include "aggregate.h"
#include "init.h"
#include "attrib.h"
#include "enum.h"
#include "module.h"
#include "id.h"

#include "symbol.h"
#include "d-lang.h"
#include "d-codegen.h"

// Construct a SymbolDeclaration, whose components are a symbol S
// and a struct declaration DSYM.

SymbolDeclaration::SymbolDeclaration (Loc loc, Symbol *s, StructDeclaration *dsym)
    : Declaration (new Identifier (s->Sident, TOKidentifier))
{
  this->loc = loc;
  sym = s;
  this->dsym = dsym;
  storage_class |= STCconst;
}

// Create the symbol with tree for struct initialisers.

Symbol *
SymbolDeclaration::toSymbol (void)
{
  // Create the actual back-end value if not yet done
  if (!sym->Stree)
    {
      if (dsym)
	dsym->toInitializer();
      gcc_assert (sym->Stree);
    }
  return sym;
}


// Helper for toSymbol.  Generate a mangled identifier for Symbol.
// We don't bother using sclass and t.

Symbol *
Dsymbol::toSymbolX (const char *prefix, int, type *, const char *suffix)
{
  char *n = mangle();
  unsigned nlen = strlen (n);

  size_t sz = (2 + nlen + sizeof (size_t) * 3 + strlen (prefix) + strlen (suffix) + 1);
  char *id = (char *) alloca (sz);

  snprintf (id, sz, "_D%s%u%s%s", n, strlen (prefix), prefix, suffix);
  return symbol_calloc (id);
}


Symbol *
Dsymbol::toSymbol (void)
{
  fprintf (stderr, "Dsymbol::toSymbol() '%s', kind = '%s'\n", toChars(), kind());
  gcc_unreachable();          // BUG: implement
  return NULL;
}

// Generate an import symbol from symbol.

Symbol *
Dsymbol::toImport (void)
{
  if (!isym)
    {
      if (!csym)
	csym = toSymbol();
      isym = toImport (csym);
    }
  return isym;
}

Symbol *
Dsymbol::toImport (Symbol *)
{
  // This is not used in GDC (yet?)
  return NULL;
}


// When compiling multiple sources at once, there may be name collisions
// on compiler-generated or extern (C) symbols. This only should only
// apply to private symbols.  Otherwise, duplicate names are an error.

static StringTable *uniqueNames = NULL;

static void
uniqueName (Declaration *d, tree t, const char *asm_name)
{
  Dsymbol *p = d->toParent2();
  const char *out_name = asm_name;
  char *alloc_name;

  FuncDeclaration *f = d->isFuncDeclaration();
  VarDeclaration *v = d->isVarDeclaration();

  // Check cases for which it is okay to have a duplicate symbol name.
  // Otherwise, duplicate names are an error and the condition will
  // be caught by the assembler.
  bool duplicate_ok = false;

  if (f && !f->fbody)
    duplicate_ok = false;
  else if (v && (v->storage_class & STCextern))
    duplicate_ok = false;
  // Static declarations in different scope statements.
  else if (p && p->isFuncDeclaration())
    duplicate_ok = true;
  //  Top-level duplicate names are okay if private.
  else if ((!p || p->isModule()) && d->protection == PROTprivate)
    duplicate_ok = true;

  if (duplicate_ok)
    {
      StringValue *sv;

      // Assumes one assembler output file per compiler run.  Otherwise, need
      // to reset this for each file.
      if (!uniqueNames)
	{
	  uniqueNames = new StringTable;
	  uniqueNames->init();
	}
      sv = uniqueNames->update (asm_name, strlen (asm_name));

      if (sv->intvalue)
	{
	  ASM_FORMAT_PRIVATE_NAME (alloc_name, asm_name, sv->intvalue);
	  out_name = alloc_name;
	}
      sv->intvalue++;
    }

  /* It is now the job of the front-end to ensure decls get mangled for their target.
     We'll only allow FUNCTION_DECLs and VAR_DECLs for variables with static storage duration
     to get a mangled DECL_ASSEMBLER_NAME. And the backend should handle the rest. */
  tree id;
  if (f || (v && (v->protection == PROTpublic || v->storage_class & (STCstatic | STCextern))))
    id = targetm.mangle_decl_assembler_name (t, get_identifier (out_name));
  else
    id = get_identifier (out_name);

  SET_DECL_ASSEMBLER_NAME (t, id);
}


// Create the symbol with VAR_DECL tree for static variables.

Symbol *
VarDeclaration::toSymbol (void)
{
  if (!csym)
    {
      tree var_decl;
      enum tree_code decl_kind;

      // For field declaration, it is possible for toSymbol to be called
      // before the parent's toCtype()
      if (storage_class & STCfield)
	{
	  AggregateDeclaration *parent_decl = toParent()->isAggregateDeclaration();
	  gcc_assert (parent_decl);
	  parent_decl->type->toCtype();
	  gcc_assert (csym);
	  return csym;
	}

      csym = new Symbol();
      csym->Salignment = alignment;

      if (isDataseg())
	{
	  csym->Sident = mangle();
	  csym->prettyIdent = toPrettyChars();
	}
      else
	csym->Sident = ident->string;

      if (storage_class & STCparameter)
	decl_kind = PARM_DECL;
      else if (storage_class & STCmanifest)
	decl_kind = CONST_DECL;
      else
	decl_kind = VAR_DECL;

      var_decl = build_decl (UNKNOWN_LOCATION, decl_kind, get_identifier (csym->Sident),
			     gen.trueDeclarationType (this));

      csym->Stree = var_decl;

      if (decl_kind != CONST_DECL)
	{
	  if (isDataseg())
	    uniqueName (this, var_decl, csym->Sident);
	  if (c_ident)
	    SET_DECL_ASSEMBLER_NAME (var_decl, get_identifier (c_ident->string));
	}
      d_keep (var_decl);
      g.ofile->setDeclLoc (var_decl, this);
      if (decl_kind == VAR_DECL)
	g.ofile->setupSymbolStorage (this, var_decl);
      else if (decl_kind == PARM_DECL)
	{
	  /* from gcc code: Some languages have different nominal and real types.  */
	  // %% What about DECL_ORIGINAL_TYPE, DECL_ARG_TYPE_AS_WRITTEN, DECL_ARG_TYPE ?
	  DECL_ARG_TYPE (var_decl) = TREE_TYPE (var_decl);
	  DECL_CONTEXT (var_decl) = gen.declContext (this);
	  gcc_assert (TREE_CODE (DECL_CONTEXT (var_decl)) == FUNCTION_DECL);
	}
      else if (decl_kind == CONST_DECL)
	{
	  /* Not sure how much of an optimization this is... It is needed
	     for foreach loops on tuples which 'declare' the index variable
	     as a constant for each iteration. */
	  Expression *e = NULL;

	  if (init)
	    {
	      if (!init->isVoidInitializer())
		{
		  e = init->toExpression();
		  gcc_assert (e != NULL);
		}
	    }
	  else
	    e = type->defaultInit();

	  if (e)
	    DECL_INITIAL (var_decl) = e->toElem (g.irs);
	}

      // Can't set TREE_STATIC, etc. until we get to toObjFile as this could be
      // called from a varaible in an imported module
      // %% (out const X x) doesn't mean the reference is const...
      if ((isConst() || isImmutable()) && (storage_class & STCinit)
	  && !gen.isDeclarationReferenceType (this))
	{
	  // %% CONST_DECLS don't have storage, so we can't use those,
	  // but it would be nice to get the benefit of them (could handle in
	  // VarExp -- makeAddressOf could switch back to the VAR_DECL

	  if (!TREE_STATIC (var_decl))
	    TREE_READONLY (var_decl) = 1;
	  else
	    {
	      // Can't set "readonly" unless DECL_INITIAL is set, which
	      // doesn't happen until outdata is called for the symbol.
	      D_DECL_READONLY_STATIC (var_decl) = 1;
	    }

	  // can at least do this...
	  //  const doesn't seem to matter for aggregates, so prevent problems..
	  if (isConst() && isDataseg())
	    TREE_CONSTANT (var_decl) = 1;
	}

#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
      // Have to test for import first
      if (isImportedSymbol())
	{
	  gen.addDeclAttribute (var_decl, "dllimport");
	  DECL_DLLIMPORT_P (var_decl) = 1;
	}
      else if (isExport())
	gen.addDeclAttribute (var_decl, "dllexport");
#endif

      if (isDataseg() && isThreadlocal())
	{
	  if (TREE_CODE (var_decl) == VAR_DECL)
	    {
	      // %% If not marked, variable will be accessible
	      // from multiple threads, which is not what we want.
	      DECL_TLS_MODEL (var_decl) = decl_default_tls_model (var_decl);
	    }
	  if (global.params.vtls)
	    {
	      char *p = loc.toChars();
	      fprintf (stderr, "%s: %s is thread local\n", p ? p : "", toChars());
	      if (p)
		free (p);
	    }
	}
    }
  return csym;
}

// Create the symbol with tree for classinfo decls.

Symbol *
ClassInfoDeclaration::toSymbol (void)
{
  return cd->toSymbol();
}

// Create the symbol with tree for moduleinfo decls.

Symbol *
ModuleInfoDeclaration::toSymbol (void)
{
  return mod->toSymbol();
}

// Create the symbol with tree for typeinfo decls.

Symbol *
TypeInfoDeclaration::toSymbol (void)
{
  if (!csym)
    {
      VarDeclaration::toSymbol();

      // This variable is the static initialization for the
      // given TypeInfo.  It is the actual data, not a reference
      gcc_assert (TREE_CODE (TREE_TYPE (csym->Stree)) == REFERENCE_TYPE);
      TREE_TYPE (csym->Stree) = TREE_TYPE (TREE_TYPE (csym->Stree));

      /* DMD makes typeinfo decls one-only by doing:

	 s->Sclass = SCcomdat;

	 in TypeInfoDeclaration::toObjFile.  The difference is
	 that, in gdc, built-in typeinfo will be referenced as
	 one-only.
 	 */
      D_DECL_ONE_ONLY (csym->Stree) = 1;
      g.ofile->makeDeclOneOnly (csym->Stree);
    }
  return csym;
}

// Create the symbol with tree for typeinfoclass decls.

Symbol *
TypeInfoClassDeclaration::toSymbol (void)
{
  gcc_assert (tinfo->ty == Tclass);
  TypeClass *tc = (TypeClass *)tinfo;
  return tc->sym->toSymbol();
}


// Create the symbol with tree for function aliases.

Symbol *
FuncAliasDeclaration::toSymbol (void)
{
  return funcalias->toSymbol();
}

// Create the symbol with FUNCTION_DECL tree for functions.

Symbol *
FuncDeclaration::toSymbol (void)
{
  if (!csym)
    {
      csym = new Symbol();

      if (!isym)
	{
	  tree id;
	  TypeFunction *ftype = (TypeFunction *) (tintro ? tintro : type);
	  tree fndecl;

	  if (ident)
	    id = get_identifier (ident->string);
	  else
	    {
	      // This happens for assoc array foreach bodies

	      // Not sure if idents are strictly necc., but announce_function
	      //  dies without them.

	      // %% better: use parent name

	      static unsigned unamed_seq = 0;
	      char buf[64];
	      snprintf (buf, sizeof(buf), "___unamed_%u", ++unamed_seq);
	      id = get_identifier (buf);
	    }

	  tree fn_type = ftype->toCtype();
	  tree new_fn_type = NULL_TREE;

	  tree vindex = NULL_TREE;
	  if (isNested())
	    {
	      /* Even if DMD-style nested functions are not implemented, add an
		 extra argument to be compatible with delegates. */
	      new_fn_type = build_method_type (void_type_node, fn_type);
	    }
	  else if (isThis())
	    {
	      // Do this even if there is no debug info.  It is needed to make
	      // sure member functions are not called statically
	      AggregateDeclaration *agg_decl = isMember2();
	      gcc_assert (agg_decl != NULL);

	      tree handle = agg_decl->handle->toCtype();
#if STRUCTTHISREF
	      if (agg_decl->isStructDeclaration())
		{
		  // Handle not a pointer type
		  new_fn_type = build_method_type (handle, fn_type);
		}
	      else
#endif
		{
		  new_fn_type = build_method_type (TREE_TYPE (handle), fn_type);
		}

	      if (isVirtual())
		vindex = size_int (vtblIndex);
	    }
	  else if (isMain() && ftype->nextOf()->toBasetype()->ty == Tvoid)
	    {
	      new_fn_type = build_function_type (integer_type_node, TYPE_ARG_TYPES (fn_type));
	    }

	  if (new_fn_type != NULL_TREE)
	    {
	      TYPE_ATTRIBUTES (new_fn_type) = TYPE_ATTRIBUTES (fn_type);
	      TYPE_LANG_SPECIFIC (new_fn_type) = TYPE_LANG_SPECIFIC (fn_type);
	      d_keep (new_fn_type);
	    }

	  // %%CHECK: is it okay for static nested functions to have a FUNC_DECL context?
	  // seems okay so far...
	  fndecl = build_decl (UNKNOWN_LOCATION, FUNCTION_DECL,
			       id, new_fn_type ? new_fn_type : fn_type);
	  d_keep (fndecl);
	  if (ident)
	    {
	      csym->Sident = mangle(); // save for making thunks
	      csym->prettyIdent = toPrettyChars();
	      uniqueName (this, fndecl, csym->Sident);
	    }
	  if (c_ident)
	    SET_DECL_ASSEMBLER_NAME (fndecl, get_identifier (c_ident->string));
	  // %% What about DECL_SECTION_NAME ?
	  DECL_CONTEXT (fndecl) = gen.declContext (this); //context;
	  if (vindex)
	    {
	      DECL_VINDEX (fndecl) = vindex;
	      DECL_VIRTUAL_P (fndecl) = 1;
	    }

	  if (gen.functionNeedsChain (this))
	    {
	      D_DECL_STATIC_CHAIN (fndecl) = 1;

	      /* If a template instance has a nested function (because a template
		 argument is a local variable), the nested function may not have
		 its toObjFile called before the outer function is finished.
		 GCC requires that nested functions be finished first so we need
		 to arrange for toObjFile to be called earlier.  */
	      FuncDeclaration *outer_func = NULL;
	      bool is_template_member = false;
	      for (Dsymbol *p = parent; p; p = p->parent)
		{
		  if (p->isTemplateInstance() && !p->isTemplateMixin())
		    is_template_member = true;
		  else if (p->isFuncDeclaration())
		    {
		      outer_func = (FuncDeclaration *) p;
		      break;
		    }
		}
	      if (is_template_member && outer_func)
		{
		  Symbol *outer_sym = outer_func->toSymbol();

		  if (outer_sym->outputStage != Finished)
		    outer_sym->deferredNestedFuncs.push (this);
		}

	      // Save context and set decl_function_context for cgraph.
	      csym->ScontextDecl = DECL_CONTEXT (fndecl);
	      DECL_CONTEXT (fndecl) = decl_function_context (fndecl);
	    }

	  if (isMember2() || isFuncLiteralDeclaration())
	    {
	      // See grokmethod in cp/decl.c
	      DECL_DECLARED_INLINE_P (fndecl) = 1;
	      DECL_NO_INLINE_WARNING_P (fndecl) = 1;
	    }
	  // Don't know what to do with this.
	  else if (flag_inline_functions && canInline (0, 1, 0))
	    {
	      DECL_DECLARED_INLINE_P (fndecl) = 1;
	      DECL_NO_INLINE_WARNING_P (fndecl) = 1;
	    }

	  if (naked)
	    {
	      gen.addDeclAttribute (fndecl, "naked");
	      DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT (fndecl) = 1;
	      DECL_UNINLINABLE (fndecl) = 1;
	    }

	  // These are always compiler generated.
	  if (isArrayOp)
	    {
	      DECL_ARTIFICIAL (fndecl) = 1;
	      D_DECL_ONE_ONLY (fndecl) = 1;
	    }
	  // So are ensure and require contracts.
	  if (ident == Id::ensure || ident == Id::require)
	    {
	      D_DECL_IS_CONTRACT (fndecl) = 1;
	      DECL_ARTIFICIAL (fndecl) = 1;
	      TREE_PUBLIC (fndecl) = 1;
	    }

	  if (isStatic())
	    TREE_STATIC (fndecl) = 1;
	  // %% Pure functions don't imply nothrow
	  DECL_PURE_P (fndecl) = (isPure() == PUREstrong && ftype->isnothrow);
	  // %% Assert contracts in functions may throw.
	  TREE_NOTHROW (fndecl) = ftype->isnothrow && !global.params.useAssert;
	  // %% Make D const methods equivalent to GCC const
	  TREE_READONLY (fndecl) = (isPure() == PUREconst);

#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
	  // Have to test for import first
	  if (isImportedSymbol())
	    {
	      gen.addDeclAttribute (fndecl, "dllimport");
	      DECL_DLLIMPORT_P (fndecl) = 1;
	    }
	  else if (isExport())
	    {
	      gen.addDeclAttribute (fndecl, "dllexport");
	    }
#endif

	  g.ofile->setDeclLoc (fndecl, this);
	  g.ofile->setupSymbolStorage (this, fndecl);
	  if (!ident)
	    TREE_PUBLIC (fndecl) = 0;

	  TREE_USED (fndecl) = 1; // %% Probably should be a little more intelligent about this
	  csym->Stree = fndecl;

	  gen.maybeSetUpBuiltin (this);
	}
      else
	{
	  csym->Stree = isym->Stree;
	}
    }
  return csym;
}


// Create the thunk symbol functions.
// Thunk is added to class at OFFSET.

Symbol *
FuncDeclaration::toThunkSymbol (int offset)
{
  Symbol *sthunk;
  Thunk *thunk;

  toSymbol();

  /* If the thunk is to be static (that is, it is being emitted in this
     module, there can only be one FUNCTION_DECL for it.   Thus, there
     is a list of all thunks for a given function. */
  bool found = false;

  for (size_t i = 0; i < csym->thunks.dim; i++)
    {
      thunk = csym->thunks[i];
      if (thunk->offset == offset)
	{
	  found = true;
	  break;
	}
    }

  if (!found)
    {
      thunk = new Thunk;
      thunk->offset = offset;
      csym->thunks.push (thunk);
    }

  if (!thunk->symbol)
    {
      unsigned sz = strlen (csym->Sident) + 14;
      char *id = (char *) alloca (sz);
      snprintf (id, sz, "_DT%u%s", offset, csym->Sident);
      sthunk = symbol_calloc (id);

      tree target_func_decl = csym->Stree;
      tree thunk_decl = build_decl (DECL_SOURCE_LOCATION (target_func_decl),
				    FUNCTION_DECL, NULL_TREE, TREE_TYPE (target_func_decl));
      DECL_LANG_SPECIFIC (thunk_decl) = DECL_LANG_SPECIFIC (target_func_decl);
      DECL_CONTEXT (thunk_decl) = gen.declContext (this); // from c++...
      TREE_READONLY (thunk_decl) = TREE_READONLY (target_func_decl);
      TREE_THIS_VOLATILE (thunk_decl) = TREE_THIS_VOLATILE (target_func_decl);
      TREE_NOTHROW (thunk_decl) = TREE_NOTHROW (target_func_decl);

      /* Thunks inherit the public/private access of the function they are targetting.  */
      TREE_PUBLIC (thunk_decl) = TREE_PUBLIC (target_func_decl);
      TREE_PRIVATE (thunk_decl) = TREE_PRIVATE (target_func_decl);
      DECL_EXTERNAL (thunk_decl) = 0;

      /* Thunks are always addressable.  */
      TREE_ADDRESSABLE (thunk_decl) = 1;
      TREE_USED (thunk_decl) = 1;
      DECL_ARTIFICIAL (thunk_decl) = 1;
      DECL_DECLARED_INLINE_P (thunk_decl) = 0;

      DECL_VISIBILITY (thunk_decl) = DECL_VISIBILITY (target_func_decl);
      DECL_VISIBILITY_SPECIFIED (thunk_decl)
	= DECL_VISIBILITY_SPECIFIED (target_func_decl);
      /* Needed on some targets to avoid "causes a section type conflict".  */
      D_DECL_ONE_ONLY (thunk_decl) = D_DECL_ONE_ONLY (target_func_decl);
      DECL_COMDAT (thunk_decl) = DECL_COMDAT (target_func_decl);
      DECL_COMDAT_GROUP (thunk_decl) = DECL_COMDAT_GROUP (target_func_decl);
      DECL_WEAK (thunk_decl) = DECL_WEAK (target_func_decl);

      DECL_NAME (thunk_decl) = get_identifier (id);
      SET_DECL_ASSEMBLER_NAME (thunk_decl, DECL_NAME (thunk_decl));

      d_keep (thunk_decl);
      sthunk->Stree = thunk_decl;

      g.ofile->doThunk (thunk_decl, target_func_decl, offset);

      thunk->symbol = sthunk;
    }
  return thunk->symbol;
}


// Create the "ClassInfo" symbol for classes.

Symbol *
ClassDeclaration::toSymbol (void)
{
  if (!csym)
    {
      tree decl;
      csym = toSymbolX ("__Class", SCextern, 0, "Z");
      decl = build_decl (UNKNOWN_LOCATION, VAR_DECL, get_identifier (csym->Sident),
			 TREE_TYPE (ClassDeclaration::classinfo != NULL
      				    ? ClassDeclaration::classinfo->type->toCtype() // want the RECORD_TYPE, not the REFERENCE_TYPE
      				    : error_mark_node));
      csym->Stree = decl;
      d_keep (decl);

      g.ofile->setupStaticStorage (this, decl);
      g.ofile->setDeclLoc (decl, this);

      TREE_CONSTANT (decl) = 0; // DMD puts this into .data, not .rodata...
      TREE_READONLY (decl) = 0;
    }
  return csym;
}

// Create the "InterfaceInfo" symbol for interfaces.

Symbol *
InterfaceDeclaration::toSymbol (void)
{
  if (!csym)
    {
      csym = ClassDeclaration::toSymbol();
      tree decl = csym->Stree;

      Symbol *temp_sym = toSymbolX ("__Interface", SCextern, 0, "Z");
      DECL_NAME (decl) = get_identifier (temp_sym->Sident);
      delete temp_sym;

      TREE_CONSTANT (decl) = 1; // Interface ClassInfo images are in .rodata, but classes arent..?
    }
  return csym;
}

// Create the "ModuleInfo" symbol for given module.

Symbol *
Module::toSymbol (void)
{
  if (!csym)
    {
      Type *obj_type = gen.getObjectType();

      csym = toSymbolX ("__ModuleInfo", SCextern, 0, "Z");

      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL, get_identifier (csym->Sident),
			      TREE_TYPE (obj_type->toCtype())); // want the RECORD_TYPE, not the REFERENCE_TYPE
      g.ofile->setDeclLoc (decl, this);
      csym->Stree = decl;

      d_keep (decl);

      g.ofile->setupStaticStorage (this, decl);

      TREE_CONSTANT (decl) = 0; // *not* readonly, moduleinit depends on this
      TREE_READONLY (decl) = 0; // Not an lvalue, tho
    }
  return csym;
}

// Create the "vtbl" symbol for ClassDeclaration.
// This is accessible via the ClassData, but since it is frequently
// needed directly (like for rtti comparisons), make it directly accessible.

Symbol *
ClassDeclaration::toVtblSymbol (void)
{
  if (!vtblsym)
    {
      tree decl;

      vtblsym = toSymbolX ("__vtbl", SCextern, 0, "Z");

      /* The DECL_INITIAL value will have a different type object from the
	 VAR_DECL.  The back end seems to accept this. */
      TypeSArray *vtbl_type = new TypeSArray (Type::tvoidptr,
					       new IntegerExp (loc, vtbl.dim, Type::tindex));

      decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			 get_identifier (vtblsym->Sident), vtbl_type->toCtype());
      vtblsym->Stree = decl;
      d_keep (decl);

      g.ofile->setupStaticStorage (this, decl);
      g.ofile->setDeclLoc (decl, this);

      TREE_READONLY (decl) = 1;
      TREE_CONSTANT (decl) = 1;
      TREE_ADDRESSABLE (decl) = 1;
      // from cp/class.c
      DECL_CONTEXT (decl) = gen.declContext (this);
      DECL_ARTIFICIAL (decl) = 1;
      DECL_VIRTUAL_P (decl) = 1;
      DECL_ALIGN (decl) = TARGET_VTABLE_ENTRY_ALIGN;
    }
  return vtblsym;
}

// Create the static initializer for the struct/class.

// Because this is called from the front end (mtype.cc:TypeStruct::defaultInit()),
// we need to hold off using back-end stuff until the toobjfile phase.

// Specifically, it is not safe create a VAR_DECL with a type from toCtype()
// because there may be unresolved recursive references.
// StructDeclaration::toObjFile calls toInitializer without ever calling
// SymbolDeclaration::toSymbol, so we just need to keep checking if we
// are in the toObjFile phase.

Symbol *
AggregateDeclaration::toInitializer (void)
{
  if (!sinit)
    {
      sinit = toSymbolX ("__init", SCextern, 0, "Z");

      StructDeclaration *sd = isStructDeclaration();
      if (sd)
	sinit->Salignment = sd->alignment;
    }
  if (!sinit->Stree && g.ofile != NULL)
    {
      tree struct_type = type->toCtype();
      if (POINTER_TYPE_P (struct_type))
	struct_type = TREE_TYPE (struct_type); // for TypeClass, want the RECORD_TYPE, not the REFERENCE_TYPE
      tree t = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			   get_identifier (sinit->Sident), struct_type);
      sinit->Stree = t;
      d_keep (t);

      g.ofile->setupStaticStorage (this, t);
      g.ofile->setDeclLoc (t, this);

      // %% what's the diff between setting this stuff on the DECL and the
      // CONSTRUCTOR itself?

      TREE_ADDRESSABLE (t) = 1;
      TREE_READONLY (t) = 1;
      TREE_CONSTANT (t) = 1;
      DECL_CONTEXT (t) = 0; // These are always global
    }
  return sinit;
}

// Create the static initializer for the typedef variable.

Symbol *
TypedefDeclaration::toInitializer (void)
{
  Symbol *s;

  if (!sinit)
    {
      s = toSymbolX ("__init", SCextern, 0, "Z");
      s->Sfl = FLextern;
      s->Sflags |= SFLnodebug;
      sinit = s;
      sinit->Sdt = ((TypeTypedef *)type)->sym->init->toDt();
    }
  if (!sinit->Stree && g.ofile != NULL)
    {
      tree t = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			   get_identifier (sinit->Sident), type->toCtype());
      sinit->Stree = t;
      d_keep (t);

      g.ofile->setupStaticStorage (this, t);
      g.ofile->setDeclLoc (t, this);
      TREE_CONSTANT (t) = 1;
      TREE_READONLY (t) = 1;
      DECL_CONTEXT (t) = 0;
    }
  return sinit;
}

// Create the static initializer for the enum.

Symbol *
EnumDeclaration::toInitializer (void)
{
  Symbol *s;

  if (!sinit)
    {
      Identifier *ident_save = ident;
      if (!ident)
	{   static int num;
	  char name[6 + sizeof (num) * 3 + 1];
	  snprintf (name, sizeof (name), "__enum%d", ++num);
	  ident = Lexer::idPool (name);
	}
      s = toSymbolX ("__init", SCextern, 0, "Z");
      ident = ident_save;
      s->Sfl = FLextern;
      s->Sflags |= SFLnodebug;
      sinit = s;
    }
  if (!sinit->Stree && g.ofile != NULL)
    {
      tree t = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			   get_identifier (sinit->Sident), type->toCtype());
      sinit->Stree = t;
      d_keep (t);

      g.ofile->setupStaticStorage (this, t);
      g.ofile->setDeclLoc (t, this);
      TREE_CONSTANT (t) = 1;
      TREE_READONLY (t) = 1;
      DECL_CONTEXT (t) = 0;
    }
  return sinit;
}


Symbol *
Module::toModuleAssert (void)
{
  // Not used in GDC
  return NULL;
}

Symbol *
Module::toModuleUnittest (void)
{
  // Not used in GDC
  return NULL;
}

Symbol *
Module::toModuleArray (void)
{
  // Not used in GDC (all array bounds checks are inlined)
  return NULL;
}

Symbol *
TypeAArray::aaGetSymbol (const char *, int)
{
  // This is not used in GDC (yet?)
  return 0;
}

