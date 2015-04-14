// d-decls.cc -- D frontend for GCC.
// Copyright (C) 2011-2013 Free Software Foundation, Inc.

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


#include "d-system.h"
#include "d-lang.h"
#include "d-codegen.h"

#include "mars.h"
#include "aggregate.h"
#include "attrib.h"
#include "enum.h"
#include "id.h"
#include "init.h"
#include "module.h"
#include "statement.h"
#include "ctfe.h"

#include "dfrontend/target.h"

// Create the symbol with tree for struct initialisers.

Symbol *
SymbolDeclaration::toSymbol()
{
  return dsym->toInitializer();
}


// Helper for toSymbol.  Generate a mangled identifier for Symbol.
// We don't bother using sclass and t.

Symbol *
Dsymbol::toSymbolX (const char *prefix, int, type *, const char *suffix)
{
  const char *symbol = mangle(this);
  unsigned prefixlen = strlen (prefix);
  size_t sz = (2 + strlen (symbol) + sizeof (size_t) * 3 + prefixlen + strlen (suffix) + 1);
  Symbol *s = new Symbol();

  s->Sident = XNEWVEC (const char, sz);
  snprintf (CONST_CAST (char *, s->Sident), sz, "_D%s%u%s%s",
	    symbol, prefixlen, prefix, suffix);

  s->prettyIdent = XNEWVEC (const char, sz);
  snprintf (CONST_CAST (char *, s->prettyIdent), sz, "%s.%s",
	    toPrettyChars(), prefix);

  return s;
}


Symbol *
Dsymbol::toSymbol()
{
  fprintf (global.stdmsg, "Dsymbol::toSymbol() '%s', kind = '%s'\n", toChars(), kind());
  gcc_unreachable();          // BUG: implement
  return NULL;
}

// Generate an import symbol for debug.  If this is a module or package symbol,
// then build the chain of NAMESPACE_DECLs.

Symbol *
Dsymbol::toImport()
{
  if (!isym)
    {
      Module *m = this->isModule();
      if (m != NULL)
	{
	  isym = new Symbol();
	  isym->prettyIdent = this->toPrettyChars();

	  // Build the module namespace, this is considered toplevel,
	  // regardless if there are parent packages.
	  tree decl = build_decl (UNKNOWN_LOCATION, NAMESPACE_DECL,
				  get_identifier (isym->prettyIdent),
				  void_type_node);
	  isym->Stree = decl;
	  d_keep (decl);

	  Loc loc = (m->md != NULL) ? m->md->loc : Loc(m, 1, 0);
	  set_decl_location (decl, loc);

	  if (output_module_p (m))
	    DECL_EXTERNAL (decl) = 1;

	  TREE_PUBLIC (decl) = 1;
	  DECL_CONTEXT (decl) = NULL_TREE;
	}
      else
	{
	  // Any other kind of symbol should have their csym set.
	  // If this is an unexpected import, the compiler will throw an error.
	  if (!csym)
	    csym = toSymbol();

	  isym = toImport (csym);
	}
    }

  return isym;
}

// Generate an IMPORTED_DECL from symbol SYM.

Symbol *
Dsymbol::toImport (Symbol *sym)
{
  tree decl = make_node (IMPORTED_DECL);
  TREE_TYPE (decl) = void_type_node;
  IMPORTED_DECL_ASSOCIATED_DECL (decl) = sym->Stree;
  d_keep (decl);

  Symbol *s = new Symbol();
  s->Stree = decl;
  return s;
}


// Create the symbol with VAR_DECL tree for static variables.

Symbol *
VarDeclaration::toSymbol()
{
  if (!csym)
    {
      // For field declaration, it is possible for toSymbol to be called
      // before the parent's toCtype()
      if (isField())
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
	  csym->Sident = mangle(this);
	  csym->prettyIdent = toPrettyChars();
	}
      else
	csym->Sident = ident->string;

      tree_code code = isParameter() ? PARM_DECL
	: !canTakeAddressOf() ? CONST_DECL
	: VAR_DECL;

      tree decl = build_decl (UNKNOWN_LOCATION, code,
			      get_identifier (ident->string),
			      declaration_type (this));
      DECL_CONTEXT (decl) = d_decl_context (this);
      set_decl_location (decl, this);

      if (isParameter())
	{
	  DECL_ARG_TYPE (decl) = TREE_TYPE (decl);
	  gcc_assert (TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL);
	}
      else if (!canTakeAddressOf())
	{
	  // Manifest constants have no address in memory.
	  TREE_CONSTANT (decl) = 1;
	  TREE_READONLY (decl) = 1;
	  TREE_STATIC (decl) = 0;
	}
      else if (isDataseg())
	{
	  if (this->mangleOverride)
	    set_user_assembler_name (decl, this->mangleOverride);
	  else
	    {
	      tree mangle = get_identifier (csym->Sident);

	      if (protection == PROTpublic || storage_class & (STCstatic | STCextern))
		mangle = targetm.mangle_decl_assembler_name (decl, mangle);

	      SET_DECL_ASSEMBLER_NAME (decl, mangle);
	    }

	  setup_symbol_storage (this, decl, false);
	}

      DECL_LANG_SPECIFIC (decl) = build_d_decl_lang_specific (this);
      d_keep (decl);
      csym->Stree = decl;

      // Can't set TREE_STATIC, etc. until we get to toObjFile as this could be
      // called from a variable in an imported module.
      if ((isConst() || isImmutable()) && (storage_class & STCinit)
	  && !decl_reference_p (this))
	{
	  if (!TREE_STATIC (decl))
	    TREE_READONLY (decl) = 1;
	  else
	    csym->Sreadonly = true;
	}

      // Propagate volatile.
      if (TYPE_VOLATILE (TREE_TYPE (decl)))
	TREE_THIS_VOLATILE (decl) = 1;

      // Mark compiler generated temporaries as artificial.
      if (storage_class & STCtemp)
	DECL_ARTIFICIAL (decl) = 1;

#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
      // Have to test for import first
      if (isImportedSymbol())
	{
	  insert_decl_attribute (decl, "dllimport");
	  DECL_DLLIMPORT_P (decl) = 1;
	}
      else if (isExport())
	insert_decl_attribute (decl, "dllexport");
#endif

      if (global.params.vtls && isDataseg() && isThreadlocal())
	{
	  char *p = loc.toChars();
	  fprintf (global.stdmsg, "%s: %s is thread local\n", p ? p : "", toChars());
	  if (p)
	    free (p);
	}
    }

  return csym;
}

// Create the symbol with tree for classinfo decls.

Symbol *
ClassInfoDeclaration::toSymbol()
{
  return cd->toSymbol();
}

// Create the symbol with tree for typeinfo decls.

Symbol *
TypeInfoDeclaration::toSymbol()
{
  if (!csym)
    {
      gcc_assert(tinfo->ty != Terror);

      VarDeclaration::toSymbol();

      // This variable is the static initialization for the
      // given TypeInfo.  It is the actual data, not a reference
      gcc_assert (TREE_CODE (TREE_TYPE (csym->Stree)) == REFERENCE_TYPE);
      TREE_TYPE (csym->Stree) = TREE_TYPE (TREE_TYPE (csym->Stree));
      relayout_decl (csym->Stree);
      TREE_USED (csym->Stree) = 1;

      // Built-in typeinfo will be referenced as one-only.
      D_DECL_ONE_ONLY (csym->Stree) = 1;
      d_comdat_linkage (csym->Stree);
    }

  return csym;
}

// Create the symbol with tree for typeinfoclass decls.

Symbol *
TypeInfoClassDeclaration::toSymbol()
{
  gcc_assert (tinfo->ty == Tclass);
  TypeClass *tc = (TypeClass *) tinfo;
  return tc->sym->toSymbol();
}


// Create the symbol with tree for function aliases.

Symbol *
FuncAliasDeclaration::toSymbol()
{
  return funcalias->toSymbol();
}

// Create the symbol with FUNCTION_DECL tree for functions.

Symbol *
FuncDeclaration::toSymbol()
{
  if (!csym)
    {
      csym = new Symbol();

      TypeFunction *ftype = (TypeFunction *) (tintro ? tintro : type);
      tree fntype = NULL_TREE;
      tree vindex = NULL_TREE;

      // Run full semantic on symbols we need to know about during compilation.
      if (inferRetType && type && !type->nextOf() && !functionSemantic())
	{
	  csym->Stree = error_mark_node;
	  return csym;
	}

      // Save mangle/debug names for making thunks.
      csym->Sident = mangleExact(this);
      csym->prettyIdent = toPrettyChars();

      tree id = get_identifier (this->isMain()
				? csym->prettyIdent : ident->string);
      tree fndecl = build_decl (UNKNOWN_LOCATION, FUNCTION_DECL, id, NULL_TREE);
      DECL_CONTEXT (fndecl) = d_decl_context (this);

      csym->Stree = fndecl;

      TREE_TYPE (fndecl) = ftype->toCtype();
      DECL_LANG_SPECIFIC (fndecl) = build_d_decl_lang_specific (this);
      d_keep (fndecl);

      if (isNested())
	{
	  // Even if D-style nested functions are not implemented, add an
	  // extra argument to be compatible with delegates.
	  fntype = build_method_type (void_type_node, TREE_TYPE (fndecl));
	}
      else if (isThis())
	{
	  // Do this even if there is no debug info.  It is needed to make
	  // sure member functions are not called statically
	  AggregateDeclaration *agg_decl = isMember2();
	  tree handle = agg_decl->handleType()->toCtype();

	  // If handle is a pointer type, get record type.
	  if (!agg_decl->isStructDeclaration())
	    handle = TREE_TYPE (handle);

	  fntype = build_method_type (handle, TREE_TYPE (fndecl));

	  if (isVirtual() && vtblIndex != -1)
	    vindex = size_int (vtblIndex);
	}
      else if (isMain() && ftype->nextOf()->toBasetype()->ty == Tvoid)
	{
	  // void main() implicitly converted to int main().
	  fntype = build_function_type (int_type_node, TYPE_ARG_TYPES (TREE_TYPE (fndecl)));
	}

      if (fntype != NULL_TREE)
	{
	  TYPE_ATTRIBUTES (fntype) = TYPE_ATTRIBUTES (TREE_TYPE (fndecl));
	  TYPE_LANG_SPECIFIC (fntype) = TYPE_LANG_SPECIFIC (TREE_TYPE (fndecl));
	  TREE_ADDRESSABLE (fntype) = TREE_ADDRESSABLE (TREE_TYPE (fndecl));
	  TREE_TYPE (fndecl) = fntype;
	  d_keep (fntype);
	}

      if (this->mangleOverride)
	set_user_assembler_name (fndecl, this->mangleOverride);
      else
	{
	  tree mangle = get_identifier (csym->Sident);
	  mangle = targetm.mangle_decl_assembler_name (fndecl, mangle);
	  SET_DECL_ASSEMBLER_NAME (fndecl, mangle);
	}

      if (vindex)
	{
	  DECL_VINDEX (fndecl) = vindex;
	  DECL_VIRTUAL_P (fndecl) = 1;
	}

      if (isMember2() || isFuncLiteralDeclaration())
	{
	  // See grokmethod in cp/decl.c
	  DECL_DECLARED_INLINE_P (fndecl) = 1;
	  DECL_NO_INLINE_WARNING_P (fndecl) = 1;
	}

      if (naked)
	{
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
	  DECL_ARTIFICIAL (fndecl) = 1;
	  TREE_PUBLIC (fndecl) = 1;
	}

      // Storage class attributes
      if (storage_class & STCstatic)
	TREE_STATIC (fndecl) = 1;

#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
      // Have to test for import first
      if (isImportedSymbol())
	{
	  insert_decl_attribute (fndecl, "dllimport");
	  DECL_DLLIMPORT_P (fndecl) = 1;
	}
      else if (isExport())
	insert_decl_attribute (fndecl, "dllexport");
#endif
      set_decl_location (fndecl, this);
      setup_symbol_storage (this, fndecl, false);

      if (!ident)
	TREE_PUBLIC (fndecl) = 0;

      // %% Probably should be a little more intelligent about this
      TREE_USED (fndecl) = 1;

      maybe_set_intrinsic (this);
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
  toObjFile(true);

  /* If the thunk is to be static (that is, it is being emitted in this
     module, there can only be one FUNCTION_DECL for it.   Thus, there
     is a list of all thunks for a given function. */
  bool found = false;

  for (size_t i = 0; i < csym->thunks.length(); i++)
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
      thunk = new Thunk();
      thunk->offset = offset;
      csym->thunks.safe_push (thunk);
    }

  if (!thunk->symbol)
    {
      unsigned sz = strlen (csym->Sident) + 14;
      sthunk = new Symbol();
      sthunk->Sident = XNEWVEC (const char, sz);
      snprintf (CONST_CAST (char *, sthunk->Sident), sz, "_DT%u%s",
		offset, csym->Sident);

      tree target_func_decl = csym->Stree;
      tree thunk_decl = build_decl (DECL_SOURCE_LOCATION (target_func_decl),
				    FUNCTION_DECL, NULL_TREE, TREE_TYPE (target_func_decl));
      DECL_LANG_SPECIFIC (thunk_decl) = DECL_LANG_SPECIFIC (target_func_decl);
      TREE_READONLY (thunk_decl) = TREE_READONLY (target_func_decl);
      TREE_THIS_VOLATILE (thunk_decl) = TREE_THIS_VOLATILE (target_func_decl);
      TREE_NOTHROW (thunk_decl) = TREE_NOTHROW (target_func_decl);

      DECL_CONTEXT (thunk_decl) = d_decl_context (this);

      /* Thunks inherit the public access of the function they are targetting.  */
      TREE_PUBLIC (thunk_decl) = TREE_PUBLIC (target_func_decl);
      DECL_EXTERNAL (thunk_decl) = 0;

      /* Thunks are always addressable.  */
      TREE_ADDRESSABLE (thunk_decl) = 1;
      TREE_USED (thunk_decl) = 1;
      DECL_ARTIFICIAL (thunk_decl) = 1;
      DECL_DECLARED_INLINE_P (thunk_decl) = 0;

      DECL_VISIBILITY (thunk_decl) = DECL_VISIBILITY (target_func_decl);
      /* Needed on some targets to avoid "causes a section type conflict".  */
      D_DECL_ONE_ONLY (thunk_decl) = D_DECL_ONE_ONLY (target_func_decl);
      DECL_COMDAT (thunk_decl) = DECL_COMDAT (target_func_decl);
      DECL_WEAK (thunk_decl) = DECL_WEAK (target_func_decl);

      DECL_NAME (thunk_decl) = get_identifier (sthunk->Sident);
      SET_DECL_ASSEMBLER_NAME (thunk_decl, DECL_NAME (thunk_decl));

      d_keep (thunk_decl);
      sthunk->Stree = thunk_decl;

      use_thunk (thunk_decl, target_func_decl, offset);

      thunk->symbol = sthunk;
    }

  return thunk->symbol;
}


// Create the "ClassInfo" symbol for classes.

Symbol *
ClassDeclaration::toSymbol()
{
  if (!csym)
    {
      csym = toSymbolX ("__Class", 0, 0, "Z");

      tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      get_identifier (csym->prettyIdent), d_unknown_type_node);
      SET_DECL_ASSEMBLER_NAME (decl, get_identifier (csym->Sident));
      csym->Stree = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      DECL_ARTIFICIAL (decl) = 1;
      // ClassInfo cannot be const data, because we use the monitor on it.
      TREE_READONLY (decl) = 0;
    }

  return csym;
}

// Create the "InterfaceInfo" symbol for interfaces.

Symbol *
InterfaceDeclaration::toSymbol()
{
  if (!csym)
    {
      csym = toSymbolX ("__Interface", 0, 0, "Z");

      tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      get_identifier (csym->prettyIdent), d_unknown_type_node);
      SET_DECL_ASSEMBLER_NAME (decl, get_identifier (csym->Sident));
      csym->Stree = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      DECL_ARTIFICIAL (decl) = 1;
      TREE_READONLY (decl) = 1;
    }

  return csym;
}

// Create the "ModuleInfo" symbol for a given module.

Symbol *
Module::toSymbol()
{
  if (!csym)
    {
      csym = toSymbolX ("__ModuleInfo", 0, 0, "Z");

      tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      get_identifier (csym->prettyIdent), d_unknown_type_node);
      SET_DECL_ASSEMBLER_NAME (decl, get_identifier (csym->Sident));
      csym->Stree = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      DECL_ARTIFICIAL (decl) = 1;
      // Not readonly, moduleinit depends on this.
      TREE_READONLY (decl) = 0;
    }

  return csym;
}

Symbol *
StructLiteralExp::toSymbol()
{
  if (!sym)
    {
      sym = new Symbol();

      // Build reference symbol.
      tree ctype = type->toCtype();
      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE, ctype);
      get_unique_name (decl, "*");
      set_decl_location (decl, loc);

      TREE_PUBLIC (decl) = 0;
      TREE_STATIC (decl) = 1;
      TREE_READONLY (decl) = 1;
      TREE_USED (decl) = 1;
      DECL_ARTIFICIAL (decl) = 1;

      sym->Stree = decl;
      this->sinit = sym;

      toDt (&sym->Sdt);
      d_finish_symbol (sym);
    }

  return sym;
}

Symbol *
ClassReferenceExp::toSymbol()
{
  if (!value->sym)
    {
      value->sym = new Symbol();

      // Build reference symbol.
      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE, d_unknown_type_node);
      char *ident;

      ASM_FORMAT_PRIVATE_NAME (ident, "*", DECL_UID (decl));
      DECL_NAME (decl) = get_identifier (ident);
      set_decl_location (decl, loc);

      TREE_PUBLIC (decl) = 0;
      TREE_STATIC (decl) = 1;
      TREE_READONLY (decl) = 1;
      TREE_USED (decl) = 1;
      DECL_ARTIFICIAL (decl) = 1;

      value->sym->Stree = decl;
      value->sym->Sident = ident;

      toInstanceDt (&value->sym->Sdt);
      d_finish_symbol (value->sym);

      value->sinit = value->sym;
    }

  return value->sym;
}

// Create the "vtbl" symbol for ClassDeclaration.
// This is accessible via the ClassData, but since it is frequently
// needed directly (like for rtti comparisons), make it directly accessible.

Symbol *
ClassDeclaration::toVtblSymbol()
{
  if (!vtblsym)
    {
      vtblsym = toSymbolX ("__vtbl", 0, 0, "Z");

      /* The DECL_INITIAL value will have a different type object from the
	 VAR_DECL.  The back end seems to accept this. */
      Type *vtbltype = Type::tvoidptr->sarrayOf (vtbl.dim);
      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			      get_identifier (vtblsym->prettyIdent), vtbltype->toCtype());
      SET_DECL_ASSEMBLER_NAME (decl, get_identifier (vtblsym->Sident));
      vtblsym->Stree = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      TREE_READONLY(decl) = 1;
      TREE_ADDRESSABLE(decl) = 1;
      DECL_ARTIFICIAL(decl) = 1;

      DECL_CONTEXT (decl) = d_decl_context (this);
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
AggregateDeclaration::toInitializer()
{
  if (!sinit)
    {
      StructDeclaration *sd = isStructDeclaration();
      sinit = toSymbolX ("__init", 0, 0, "Z");

      if (sd)
	sinit->Salignment = sd->alignment;
    }

  if (!sinit->Stree && current_module_decl)
    {
      tree stype;
      if (isStructDeclaration())
	stype = type->toCtype();
      else
	stype = TREE_TYPE (type->toCtype());

      sinit->Stree = build_decl (UNKNOWN_LOCATION, VAR_DECL,
				 get_identifier (sinit->prettyIdent), stype);
      SET_DECL_ASSEMBLER_NAME (sinit->Stree, get_identifier (sinit->Sident));
      d_keep (sinit->Stree);

      setup_symbol_storage (this, sinit->Stree, true);
      set_decl_location (sinit->Stree, this);

      TREE_ADDRESSABLE (sinit->Stree) = 1;
      TREE_READONLY (sinit->Stree) = 1;
      DECL_ARTIFICIAL (sinit->Stree) = 1;
      // These initialisers are always global.
      DECL_CONTEXT (sinit->Stree) = NULL_TREE;
    }

  return sinit;
}

// Create the static initializer for the enum.

Symbol *
EnumDeclaration::toInitializer()
{
  if (!sinit)
    {
      Identifier *ident_save = ident;
      if (!ident)
	ident = Lexer::uniqueId("__enum");
      sinit = toSymbolX ("__init", 0, 0, "Z");
      ident = ident_save;
    }

  if (!sinit->Stree && current_module_decl)
    {
      sinit->Stree = build_decl (UNKNOWN_LOCATION, VAR_DECL,
				 get_identifier (sinit->prettyIdent), type->toCtype());
      SET_DECL_ASSEMBLER_NAME (sinit->Stree, get_identifier (sinit->Sident));
      d_keep (sinit->Stree);

      setup_symbol_storage (this, sinit->Stree, true);
      set_decl_location (sinit->Stree, this);

      TREE_READONLY (sinit->Stree) = 1;
      DECL_ARTIFICIAL (sinit->Stree) = 1;
      DECL_CONTEXT (sinit->Stree) = NULL_TREE;
    }

  return sinit;
}


//

void
ClassDeclaration::toDebug()
{
  tree rec_type = TREE_TYPE (type->toCtype());
  build_type_decl (rec_type, this);
  rest_of_type_compilation (rec_type, 1);
}

void
EnumDeclaration::toDebug()
{
  TypeEnum *tc = (TypeEnum *) type;
  if (!tc->sym->defaultval || type->isZeroInit())
    return;

  tree ctype = type->toCtype();

  // The ctype is not necessarily enum, which doesn't sit well with
  // rest_of_type_compilation.
  if (TREE_CODE (ctype) == ENUMERAL_TYPE)
    {
      build_type_decl (ctype, this);
      rest_of_type_compilation (ctype, 1);
    }
}

void
StructDeclaration::toDebug()
{
  tree ctype = type->toCtype();
  build_type_decl (ctype, this);
  rest_of_type_compilation (ctype, 1);
}


// Stubs unused in GDC, but required for D front-end.

Symbol *
Module::toModuleAssert()
{
  return NULL;
}

Symbol *
Module::toModuleUnittest()
{
  return NULL;
}

Symbol *
Module::toModuleArray()
{
  return NULL;
}

Symbol *
TypeAArray::aaGetSymbol (const char *, int)
{
  return 0;
}

