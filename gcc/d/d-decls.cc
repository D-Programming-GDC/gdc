// d-decls.cc -- D frontend for GCC.
// Copyright (C) 2011-2016 Free Software Foundation, Inc.

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


#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/mars.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/attrib.h"
#include "dfrontend/enum.h"
#include "dfrontend/globals.h"
#include "dfrontend/init.h"
#include "dfrontend/module.h"
#include "dfrontend/statement.h"
#include "dfrontend/ctfe.h"
#include "dfrontend/target.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "target.h"
#include "stringpool.h"
#include "stor-layout.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "id.h"

// Create the symbol with tree for struct initialisers.

Symbol *
SymbolDeclaration::toSymbol()
{
  return dsym->toInitializer();
}


/* Generate a mangled identifier using NAME and SUFFIX, prefixed by the
   assembler name for DSYM.  */

Symbol *
make_internal_name (Dsymbol *dsym, const char *name, const char *suffix)
{
  const char *prefix = mangle(dsym);
  unsigned namelen = strlen (name);
  unsigned buflen = (2 + strlen (prefix) + namelen + strlen (suffix)) * 2;
  char *buf = (char *) alloca (buflen);

  snprintf (buf, buflen, "_D%s%u%s%s", prefix, namelen, name, suffix);
  tree ident = get_identifier (buf);

  /* Symbol is not found in user code, but generate a readable name for it
     anyway for debug and diagnostic reporting.  */
  snprintf (buf, buflen, "%s.%s", dsym->toPrettyChars(), name);
  IDENTIFIER_PRETTY_NAME (ident) = get_identifier (buf);

  Symbol *s = new Symbol();
  DECL_LANG_IDENTIFIER (s) = ident;
  return s;
}

Symbol *
Dsymbol::toSymbol()
{
  gcc_unreachable();          // BUG: implement
  return NULL;
}

// Create the symbol with VAR_DECL tree for static variables.

Symbol *
VarDeclaration::toSymbol()
{
  if (!csym)
    {
      // For field declaration, it is possible for toSymbol to be called
      // before the parent's build_ctype()
      if (isField())
	{
	  AggregateDeclaration *parent_decl = toParent()->isAggregateDeclaration();
	  gcc_assert (parent_decl);
	  build_ctype(parent_decl->type);
	  gcc_assert (csym);
	  return csym;
	}

      // Use same symbol for VarDeclaration templates with same mangle
      bool local_p, template_p;
      get_template_storage_info(this, &local_p, &template_p);
      if (!this->mangleOverride && isDataseg () && !(storage_class & STCextern)
	  && local_p && template_p)
	{
	  tree ident = get_identifier(mangle(this));
	  if (!IDENTIFIER_DSYMBOL (ident))
	    {
	      csym = new Symbol();
	      IDENTIFIER_DSYMBOL (ident) = this;
	    }
	  else
	    {
	      csym = IDENTIFIER_DSYMBOL (ident)->toSymbol();
	      return csym;
	    }
	}
      else
	{
	  csym = new Symbol();
	}


      tree_code code = isParameter() ? PARM_DECL
	: !canTakeAddressOf() ? CONST_DECL
	: VAR_DECL;

      tree decl = build_decl (UNKNOWN_LOCATION, code,
			      get_identifier (ident->string),
			      declaration_type (this));
      DECL_CONTEXT (decl) = d_decl_context (this);
      set_decl_location (decl, this);

      DECL_LANG_SPECIFIC (decl) = build_lang_decl (this);
      DECL_LANG_TREE (csym) = decl;

      if (isDataseg())
	{
	  DECL_LANG_IDENTIFIER (csym) = get_identifier (mangle(this));
	  DECL_LANG_PRETTY_NAME (csym) = get_identifier (toPrettyChars(true));
	}
      else
	DECL_LANG_IDENTIFIER (csym) = get_identifier (ident->string);

      if (this->alignment > 0)
	{
	  SET_DECL_ALIGN (decl, this->alignment * BITS_PER_UNIT);
	  DECL_USER_ALIGN (decl) = 1;
	}

      if (isParameter())
	{
	  // Pass non-trivial structs by invisible reference.
	  if (TREE_ADDRESSABLE (TREE_TYPE (decl)))
	    {
	      tree argtype = build_reference_type(TREE_TYPE (decl));
	      argtype = build_qualified_type(argtype, TYPE_QUAL_RESTRICT);
	      gcc_assert (!DECL_BY_REFERENCE (decl));
	      TREE_TYPE (decl) = argtype;
	      DECL_BY_REFERENCE (decl) = 1;
	      TREE_ADDRESSABLE (decl) = 0;
	      relayout_decl (decl);
	      this->storage_class |= STCref;
	    }

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
            {
              tree mangle = get_identifier (this->mangleOverride);
              SET_DECL_ASSEMBLER_NAME (decl, mangle);
            }
	  else
	    {
	      tree mangle = DECL_LANG_IDENTIFIER (csym);

	      if (protection == PROTpublic || storage_class & (STCstatic | STCextern))
		mangle = targetm.mangle_decl_assembler_name (decl, mangle);

	      SET_DECL_ASSEMBLER_NAME (decl, mangle);
	    }

	  setup_symbol_storage (this, decl, false);
	}

      d_keep (decl);

      // Can't set TREE_STATIC, etc. until we get to toObjFile as this could be
      // called from a variable in an imported module.
      if ((isConst() || isImmutable()) && (storage_class & STCinit)
	  && declaration_reference_p(this))
	{
	  if (!TREE_STATIC (decl))
	    TREE_READONLY (decl) = 1;
	  else
	    DECL_LANG_READONLY (csym) = true;
	}

      // Propagate shared.
      if (TYPE_SHARED (TREE_TYPE (decl)))
	TREE_ADDRESSABLE (decl) = 1;

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
      gcc_assert (TREE_CODE (TREE_TYPE (DECL_LANG_TREE (csym))) == POINTER_TYPE);
      TREE_TYPE (DECL_LANG_TREE (csym)) = TREE_TYPE (TREE_TYPE (DECL_LANG_TREE (csym)));
      relayout_decl (DECL_LANG_TREE (csym));
      TREE_USED (DECL_LANG_TREE (csym)) = 1;

      // Built-in typeinfo will be referenced as one-only.
      D_DECL_ONE_ONLY (DECL_LANG_TREE (csym)) = 1;
      d_comdat_linkage (DECL_LANG_TREE (csym));
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
      TypeFunction *ftype = (TypeFunction *) (tintro ? tintro : type);
      tree fntype = NULL_TREE;
      tree vindex = NULL_TREE;

      // Run full semantic on symbols we need to know about during compilation.
      if (inferRetType && type && !type->nextOf())
	{
	  Module *old_current_module_decl = current_module_decl;
	  current_module_decl = NULL;
	  if (!functionSemantic())
	    {
	      csym = new Symbol();
	      DECL_LANG_TREE (csym) = error_mark_node;
	      return csym;
	    }
	  current_module_decl = old_current_module_decl;
	}

      // Use same symbol for FuncDeclaration templates with same mangle
      if (!this->mangleOverride && this->fbody)
	{
	  tree ident = get_identifier(mangleExact(this));
	  if (!IDENTIFIER_DSYMBOL (ident))
	    {
	      csym = new Symbol();
	      IDENTIFIER_DSYMBOL (ident) = this;
	    }
	  else
	    {
	      bool local_p, template_p;
	      get_template_storage_info(this, &local_p, &template_p);
	      // Non-templated functions shouldn't be defined twice
	      if (!template_p)
		ScopeDsymbol::multiplyDefined(loc, this, IDENTIFIER_DSYMBOL (ident));

	      csym = IDENTIFIER_DSYMBOL (ident)->toSymbol();
	      return csym;
	    }
	}
      else
	{
	  csym = new Symbol();
	}

      tree fndecl = build_decl (UNKNOWN_LOCATION, FUNCTION_DECL, NULL_TREE, NULL_TREE);
      DECL_CONTEXT (fndecl) = d_decl_context (this);

      DECL_LANG_SPECIFIC (fndecl) = build_lang_decl (this);
      DECL_LANG_TREE (csym) = fndecl;

      // Save mangle/debug names for making thunks.
      DECL_LANG_IDENTIFIER (csym) = get_identifier (mangleExact(this));
      DECL_LANG_PRETTY_NAME (csym) = get_identifier (toPrettyChars(true));

      tree id = this->isMain()
	? DECL_LANG_PRETTY_NAME (csym) : get_identifier (ident->string);
      DECL_NAME (fndecl) = id;
      TREE_TYPE (fndecl) = build_ctype(ftype);
      d_keep (fndecl);

      if (isNested())
	{
	  // Add an extra argument for the frame/closure pointer,
	  // also needed to be compatible with delegates.
	  fntype = build_vthis_type(void_type_node, TREE_TYPE (fndecl));
	}
      else if (isThis())
	{
	  // Do this even if there is no debug info.  It is needed to make
	  // sure member functions are not called statically
	  AggregateDeclaration *agg_decl = isMember2();
	  tree handle = build_ctype(agg_decl->handleType());

	  // If handle is a pointer type, get record type.
	  if (!agg_decl->isStructDeclaration())
	    handle = TREE_TYPE (handle);

	  fntype = build_vthis_type(handle, TREE_TYPE (fndecl));

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
        {
          tree mangle = get_identifier (this->mangleOverride);
          SET_DECL_ASSEMBLER_NAME (fndecl, mangle);
        }
      else
	{
	  tree mangle = DECL_LANG_IDENTIFIER (csym);
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
      if (storage_class & STCfinal)
	DECL_FINAL_P (fndecl) = 1;

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
  toObjFile();

  /* If the thunk is to be static (that is, it is being emitted in this
     module, there can only be one FUNCTION_DECL for it.   Thus, there
     is a list of all thunks for a given function. */
  bool found = false;

  for (size_t i = 0; i < DECL_LANG_THUNKS (csym).length(); i++)
    {
      thunk = DECL_LANG_THUNKS (csym)[i];
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
      DECL_LANG_THUNKS (csym).safe_push (thunk);
    }

  if (!thunk->symbol)
    {
      unsigned sz = strlen (IDENTIFIER_POINTER (DECL_LANG_IDENTIFIER (csym))) + 14;
      sthunk = new Symbol();

      tree target_func_decl = DECL_LANG_TREE (csym);
      tree thunk_decl = build_decl (DECL_SOURCE_LOCATION (target_func_decl),
				    FUNCTION_DECL, NULL_TREE, TREE_TYPE (target_func_decl));
      DECL_LANG_SPECIFIC (thunk_decl) = DECL_LANG_SPECIFIC (target_func_decl);
      DECL_LANG_TREE (sthunk) = thunk_decl;

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

      const char *ident = XNEWVEC (const char, sz);
      snprintf (CONST_CAST (char *, ident), sz,
		"_DT%u%s", offset, IDENTIFIER_POINTER (DECL_LANG_IDENTIFIER (csym)));

      DECL_NAME (thunk_decl) = get_identifier (ident);
      SET_DECL_ASSEMBLER_NAME (thunk_decl, DECL_NAME (thunk_decl));

      d_keep (thunk_decl);

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
      csym = make_internal_name (this, "__Class", "Z");

      tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      DECL_LANG_PRETTY_NAME (csym), unknown_type_node);
      SET_DECL_ASSEMBLER_NAME (decl, DECL_LANG_IDENTIFIER (csym));
      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (csym) = decl;
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
      csym = make_internal_name (this, "__Interface", "Z");

      tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      DECL_LANG_PRETTY_NAME (csym), unknown_type_node);
      SET_DECL_ASSEMBLER_NAME (decl, DECL_LANG_IDENTIFIER (csym));
      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (csym) = decl;
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
      csym = make_internal_name (this, "__ModuleInfo", "Z");

      tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      DECL_LANG_PRETTY_NAME (csym), unknown_type_node);
      SET_DECL_ASSEMBLER_NAME (decl, DECL_LANG_IDENTIFIER (csym));
      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (csym) = decl;
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
      tree ctype = build_ctype(type);
      tree decl = build_artificial_decl(ctype, NULL_TREE, "S");
      set_decl_location(decl, loc);

      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (sym) = decl;
      this->sinit = sym;

      DECL_INITIAL (DECL_LANG_TREE (sym)) = build_expr(this, true);
      d_finish_symbol(sym);
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
      tree ctype = build_ctype(value->stype);
      tree decl = build_artificial_decl(TREE_TYPE (ctype), NULL_TREE, "C");
      set_decl_location(decl, loc);

      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (value->sym) = decl;
      DECL_LANG_IDENTIFIER (value->sym) = DECL_NAME (decl);

      toInstanceDt(&DECL_LANG_INITIAL (value->sym));
      d_finish_symbol(value->sym);

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
      vtblsym = make_internal_name (this, "__vtbl", "Z");

      /* The DECL_INITIAL value will have a different type object from the
	 VAR_DECL.  The back end seems to accept this. */
      Type *vtbltype = Type::tvoidptr->sarrayOf (vtbl.dim);
      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			      DECL_LANG_PRETTY_NAME (vtblsym),
			      build_ctype (vtbltype));
      SET_DECL_ASSEMBLER_NAME (decl, DECL_LANG_IDENTIFIER (vtblsym));
      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (vtblsym) = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      TREE_READONLY(decl) = 1;
      TREE_ADDRESSABLE(decl) = 1;
      DECL_ARTIFICIAL(decl) = 1;

      DECL_CONTEXT (decl) = d_decl_context (this);
      SET_DECL_ALIGN (decl, TARGET_VTABLE_ENTRY_ALIGN);
    }
  return vtblsym;
}

// Create the static initializer for the struct/class.

// Because this is called from the front end (mtype.cc:TypeStruct::defaultInit()),
// we need to hold off using back-end stuff until the toobjfile phase.

// Specifically, it is not safe create a VAR_DECL with a type from build_ctype()
// because there may be unresolved recursive references.
// StructDeclaration::toObjFile calls toInitializer without ever calling
// SymbolDeclaration::toSymbol, so we just need to keep checking if we
// are in the toObjFile phase.

Symbol *
AggregateDeclaration::toInitializer()
{
  if (!sinit)
    sinit = make_internal_name (this, "__init", "Z");

  if (!DECL_LANG_TREE (sinit) && current_module_decl)
    {
      tree stype = build_ctype (type);
      if (!this->isStructDeclaration())
	stype = TREE_TYPE (stype);

      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			      DECL_LANG_PRETTY_NAME (sinit), stype);
      SET_DECL_ASSEMBLER_NAME (decl, DECL_LANG_IDENTIFIER (sinit));
      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (sinit) = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      TREE_ADDRESSABLE (decl) = 1;
      TREE_READONLY (decl) = 1;
      DECL_ARTIFICIAL (decl) = 1;
      // These initialisers are always global.
      DECL_CONTEXT (decl) = NULL_TREE;
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
	ident = Identifier::generateId("__enum");
      sinit = make_internal_name (this, "__init", "Z");
      ident = ident_save;
    }

  if (!DECL_LANG_TREE (sinit) && current_module_decl)
    {
      tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			      DECL_LANG_PRETTY_NAME (sinit),
			      build_ctype (type));
      SET_DECL_ASSEMBLER_NAME (decl, DECL_LANG_IDENTIFIER (sinit));
      DECL_LANG_SPECIFIC (decl) = build_lang_decl (NULL);
      DECL_LANG_TREE (sinit) = decl;
      d_keep (decl);

      setup_symbol_storage (this, decl, true);
      set_decl_location (decl, this);

      TREE_READONLY (decl) = 1;
      DECL_ARTIFICIAL (decl) = 1;
      DECL_CONTEXT (decl) = NULL_TREE;
    }

  return sinit;
}

