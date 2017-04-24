/* decl.cc -- Lower D frontend declarations to GCC trees.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

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
#include "dfrontend/template.h"
#include "dfrontend/ctfe.h"
#include "dfrontend/target.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "langhooks.h"
#include "target.h"
#include "common/common-target.h"
#include "cgraph.h"
#include "toplev.h"
#include "stringpool.h"
#include "varasm.h"
#include "stor-layout.h"
#include "attribs.h"
#include "function.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "id.h"


/* Return identifier for the external mangled name of DECL.  */

static const char *
mangle_decl (Dsymbol *decl)
{
  if (decl->isFuncDeclaration ())
    return mangleExact ((FuncDeclaration *)decl);
  else
    {
      OutBuffer buf;
      mangleToBuffer (decl, &buf);
      return buf.extractString ();
    }
}

/* Generate a mangled identifier using NAME and SUFFIX, prefixed by the
   assembler name for DSYM.  */

tree
make_internal_name (Dsymbol *dsym, const char *name, const char *suffix)
{
  const char *prefix = mangle_decl (dsym);
  unsigned namelen = strlen (name);
  unsigned buflen = (2 + strlen (prefix) + namelen + strlen (suffix)) * 2;
  char *buf = (char *) alloca (buflen);

  snprintf (buf, buflen, "_D%s%u%s%s", prefix, namelen, name, suffix);
  tree ident = get_identifier (buf);

  /* Symbol is not found in user code, but generate a readable name for it
     anyway for debug and diagnostic reporting.  */
  snprintf (buf, buflen, "%s.%s", dsym->toPrettyChars (), name);
  IDENTIFIER_PRETTY_NAME (ident) = get_identifier (buf);

  return ident;
}

/* Return the decl for the symbol, create it if it doesn't already exist.  */

tree
get_symbol_decl (Declaration *decl)
{
  if (decl->csym)
    return decl->csym;

  /* Deal with placeholder symbols immediately:
     SymbolDeclaration is used as a shell around an initializer symbol.  */
  SymbolDeclaration *sd = decl->isSymbolDeclaration ();
  if (sd)
    {
      decl->csym = aggregate_initializer_decl (sd->dsym);
      return decl->csym;
    }

  /* Global static TypeInfo declaration.  */
  if (decl->isTypeInfoDeclaration ())
    return get_typeinfo_decl ((TypeInfoDeclaration *) decl);

  /* FuncAliasDeclaration is used to import functions from another scope.  */
  FuncAliasDeclaration *fad = decl->isFuncAliasDeclaration ();
  if (fad)
    {
      decl->csym = get_symbol_decl (fad->funcalias);
      return decl->csym;
    }

  /* It is possible for a field declaration symbol to be requested
     before the parent type has been built.  */
  if (decl->isField ())
    {
      AggregateDeclaration *ad = decl->toParent ()->isAggregateDeclaration ();
      gcc_assert (ad != NULL);

      /* Finishing off the type should create the associated FIELD_DECL.  */
      build_ctype (ad->type);
      gcc_assert (decl->csym != NULL);

      return decl->csym;
    }

  /* Build the tree for the symbol.  */
  FuncDeclaration *fd = decl->isFuncDeclaration ();
  if (fd)
    {
      /* Run full semantic on functions we need to know about.  */
      if (!fd->functionSemantic ())
	{
	  decl->csym = error_mark_node;
	  return decl->csym;
	}

      decl->csym = build_decl (UNKNOWN_LOCATION, FUNCTION_DECL,
			      get_identifier (decl->ident->toChars ()),
			      NULL_TREE);

      /* Set function type afterwards as there could be self references.  */
      TREE_TYPE (decl->csym) = build_ctype (fd->type);

      if (!fd->fbody)
	DECL_EXTERNAL (decl->csym) = 1;
    }
  else
    {
      /* Build the variable declaration.  */
      VarDeclaration *vd = decl->isVarDeclaration ();
      gcc_assert (vd != NULL);

      tree_code code = vd->isParameter () ? PARM_DECL
	: !vd->canTakeAddressOf () ? CONST_DECL
	: VAR_DECL;
      decl->csym = build_decl (UNKNOWN_LOCATION, code,
			      get_identifier (decl->ident->toChars ()),
			      declaration_type (vd));

      /* If any alignment was set on the declaration.  */
      if (vd->alignment != STRUCTALIGN_DEFAULT)
	{
	  SET_DECL_ALIGN (decl->csym, vd->alignment * BITS_PER_UNIT);
	  DECL_USER_ALIGN (decl->csym) = 1;
	}

      if (vd->storage_class & STCextern)
	DECL_EXTERNAL (decl->csym) = 1;
    }

  /* Set the declaration mangled identifier if static.  */
  if (decl->isCodeseg () || decl->isDataseg ())
    {
      tree mangled_name;

      if (decl->mangleOverride)
	mangled_name = get_identifier (decl->mangleOverride);
      else
	mangled_name = get_identifier (mangle_decl (decl));

      mangled_name = targetm.mangle_decl_assembler_name (decl->csym,
							 mangled_name);
      /* The frontend doesn't handle duplicate definitions of unused symbols
	 with the same mangle.  So a check is done here instead.  */
      if (!DECL_EXTERNAL (decl->csym))
	{
	  if (IDENTIFIER_DSYMBOL (mangled_name))
	    {
	      Declaration *other = IDENTIFIER_DSYMBOL (mangled_name);

	      /* Non-templated variables shouldn't be defined twice.  */
	      if (!decl->isInstantiated ())
		ScopeDsymbol::multiplyDefined (decl->loc, decl, other);

	      decl->csym = get_symbol_decl (other);
	      return decl->csym;
	    }

	  IDENTIFIER_PRETTY_NAME (mangled_name) =
	    get_identifier (decl->toPrettyChars (true));
	  IDENTIFIER_DSYMBOL (mangled_name) = decl;
	}

      SET_DECL_ASSEMBLER_NAME (decl->csym, mangled_name);
    }

  DECL_LANG_SPECIFIC (decl->csym) = build_lang_decl (decl);
  DECL_CONTEXT (decl->csym) = d_decl_context (decl);
  set_decl_location (decl->csym, decl);

  if (TREE_CODE (decl->csym) == PARM_DECL)
    {
      /* Pass non-trivial structs by invisible reference.  */
      if (TREE_ADDRESSABLE (TREE_TYPE (decl->csym)))
	{
	  tree argtype = build_reference_type (TREE_TYPE (decl->csym));
	  argtype = build_qualified_type (argtype, TYPE_QUAL_RESTRICT);
	  gcc_assert (!DECL_BY_REFERENCE (decl->csym));
	  TREE_TYPE (decl->csym) = argtype;
	  DECL_BY_REFERENCE (decl->csym) = 1;
	  TREE_ADDRESSABLE (decl->csym) = 0;
	  relayout_decl (decl->csym);
	  decl->storage_class |= STCref;
	}

      DECL_ARG_TYPE (decl->csym) = TREE_TYPE (decl->csym);
      gcc_assert (TREE_CODE (DECL_CONTEXT (decl->csym)) == FUNCTION_DECL);
    }
  else if (TREE_CODE (decl->csym) == CONST_DECL)
    {
      /* Manifest constants have no address in memory.  */
      TREE_CONSTANT (decl->csym) = 1;
      TREE_READONLY (decl->csym) = 1;
    }
  else if (TREE_CODE (decl->csym) == FUNCTION_DECL)
    {
      /* The real function type may differ from it's declaration.  */
      tree fntype = TREE_TYPE (decl->csym);
      tree newfntype = NULL_TREE;

      if (fd->isNested ())
	{
	  /* Add an extra argument for the frame/closure pointer, this is also
	     required to be compatible with D delegates.  */
	  newfntype = build_vthis_type (void_type_node, fntype);
	}
      else if (fd->isThis ())
	{
	  /* Add an extra argument for the 'this' parameter.  The handle type is
	     used even if there is no debug info.  It is needed to make sure
	     virtual member functions are not called statically.  */
	  AggregateDeclaration *ad = fd->isMember2 ();
	  tree handle = build_ctype (ad->handleType ());

	  /* If handle is a pointer type, get record type.  */
	  if (!ad->isStructDeclaration ())
	    handle = TREE_TYPE (handle);

	  newfntype = build_vthis_type (handle, fntype);

	  /* Set the vindex on virtual functions.  */
	  if (fd->isVirtual () && fd->vtblIndex != -1)
	    {
	      DECL_VINDEX (decl->csym) = size_int (fd->vtblIndex);
	      DECL_VIRTUAL_P (decl->csym) = 1;
	    }
	}
      else if (fd->isMain ())
	{
	  /* The main function is named 'D main' to distinguish from C main.  */
	  DECL_NAME (decl->csym) = get_identifier (fd->toPrettyChars (true));

	  /* 'void main' is implicitly converted to returning an int.  */
	  newfntype = build_function_type (int_type_node,
					   TYPE_ARG_TYPES (fntype));
	}

      if (newfntype != NULL_TREE)
	{
	  /* Copy the old attributes from the original type.  */
	  TYPE_ATTRIBUTES (newfntype) = TYPE_ATTRIBUTES (fntype);
	  TYPE_LANG_SPECIFIC (newfntype) = TYPE_LANG_SPECIFIC (fntype);
	  TREE_ADDRESSABLE (newfntype) = TREE_ADDRESSABLE (fntype);
	  TREE_TYPE (decl->csym) = newfntype;
	  d_keep (newfntype);
	}

      /* Miscellaneous function flags.  */
      if (fd->isMember2 () || fd->isFuncLiteralDeclaration ())
	{
	  /* See grokmethod in cp/decl.c.  Maybe we shouldn't be setting inline
	     flags without reason or proper handling.  */
	  DECL_DECLARED_INLINE_P (decl->csym) = 1;
	  DECL_NO_INLINE_WARNING_P (decl->csym) = 1;
	}

      /* Function was declared 'naked'.  */
      if (fd->naked)
	{
	  DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT (decl->csym) = 1;
	  DECL_UNINLINABLE (decl->csym) = 1;
	}

      /* Vector array operations are always compiler generated.  */
      if (fd->isArrayOp)
	{
	  DECL_ARTIFICIAL (decl->csym) = 1;
	  D_DECL_ONE_ONLY (decl->csym) = 1;
	}

      /* And so are ensure and require contracts.  */
      if (fd->ident == Id::ensure || fd->ident == Id::require)
	{
	  DECL_ARTIFICIAL (decl->csym) = 1;
	  TREE_PUBLIC (decl->csym) = 1;
	}

      if (decl->storage_class & STCfinal)
	DECL_FINAL_P (decl->csym) = 1;

      /* Check whether this function is expanded by the frontend.  */
      maybe_set_intrinsic (fd);
    }

  /* Mark compiler generated temporaries as artificial.  */
  if (decl->storage_class & STCtemp)
    DECL_ARTIFICIAL (decl->csym) = 1;

  /* Propagate shared on the decl.  */
  if (TYPE_SHARED (TREE_TYPE (decl->csym)))
    TREE_ADDRESSABLE (decl->csym) = 1;

  /* Symbol was marked volatile.  */
  if (decl->storage_class & STCvolatile)
    TREE_THIS_VOLATILE (decl->csym) = 1;

  /* Protection attributes are used by the debugger.  */
  if (decl->protection == PROTprivate)
    TREE_PRIVATE (decl->csym) = 1;
  else if (decl->protection == PROTprotected)
    TREE_PROTECTED (decl->csym) = 1;

  /* Likewise, so could the deprecated attribute.  */
  if (decl->storage_class & STCdeprecated)
    TREE_DEPRECATED (decl->csym) = 1;

#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
  /* Have to test for import first.  */
  if (decl->isImportedSymbol ())
    {
      insert_decl_attribute (decl->csym, "dllimport");
      DECL_DLLIMPORT_P (decl->csym) = 1;
    }
  else if (decl->isExport ())
    insert_decl_attribute (decl->csym, "dllexport");
#endif

  if (decl->isDataseg () || decl->isCodeseg () || decl->isThreadlocal ())
    {
      /* Check if the declaration is a template, and whether it will be emitted
	 in the current compilation or not.  */
      TemplateInstance *ti = decl->isInstantiated ();
      if (ti)
	{
	  D_DECL_ONE_ONLY (decl->csym) = 1;

	  if (!DECL_EXTERNAL (decl->csym) && ti->needsCodegen ())
	    {
	      /* Warn about templates instantiated in this compilation.  */
	      if (ti == decl->parent)
		{
		  warning (OPT_Wtemplates, "%s %s instantiated",
			   ti->kind (), ti->toPrettyChars (false));
		}

	      TREE_STATIC (decl->csym) = 1;
	    }
	  else
	    DECL_EXTERNAL (decl->csym) = 1;
	}
      else
	{
	  if (!DECL_EXTERNAL (decl->csym)
	      && decl->getModule () && decl->getModule ()->isRoot ())
	    TREE_STATIC (decl->csym) = 1;
	  else
	    DECL_EXTERNAL (decl->csym) = 1;
	}

      /* Set TREE_PUBLIC by default, but allow private template to override.  */
      if (!fd || !fd->isNested ())
	TREE_PUBLIC (decl->csym) = 1;

      if (D_DECL_ONE_ONLY (decl->csym))
	d_comdat_linkage (decl->csym);
    }

  /* Symbol is going in thread local storage.  */
  if (decl->isThreadlocal () && !DECL_ARTIFICIAL (decl->csym))
    {
      if (global.params.vtls)
	{
	  const char *p = decl->loc.toChars ();
	  fprintf (global.stdmsg, "%s: %s is thread local\n",
		   p ? p : "", decl->toChars ());
	  if (p)
	    free (CONST_CAST (char *, p));
	}

      set_decl_tls_model (decl->csym, decl_default_tls_model (decl->csym));
    }

  /* Apply any user attributes that may affect semantic meaning.  */
  if (decl->userAttribDecl)
    {
      Expressions *attrs = decl->userAttribDecl->getAttributes ();
      decl_attributes (&decl->csym, build_attributes (attrs), 0);
    }
  else if (DECL_ATTRIBUTES (decl->csym) != NULL)
    decl_attributes (&decl->csym, DECL_ATTRIBUTES (decl->csym), 0);

  /* %% Probably should be a little more intelligent about setting this.  */
  TREE_USED (decl->csym) = 1;
  d_keep (decl->csym);

  return decl->csym;
}

/* Returns a declaration for a VAR_DECL.  Used to create compiler-generated
   global variables.  */

tree
declare_extern_var (tree ident, tree type)
{
  tree name = IDENTIFIER_PRETTY_NAME (ident)
    ? IDENTIFIER_PRETTY_NAME (ident) : ident;
  tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL, name, type);

  d_keep (decl);

  SET_DECL_ASSEMBLER_NAME (decl, ident);
  DECL_ARTIFICIAL (decl) = 1;
  TREE_STATIC (decl) = 1;
  TREE_PUBLIC (decl) = 1;

  /* The decl has not been defined -- yet.  */
  DECL_EXTERNAL (decl) = 1;

  return decl;
}

/* Add local variable VAR into the current function body.  */

void
declare_local_var (VarDeclaration *var)
{
  gcc_assert (!var->isDataseg () && !var->isMember ());
  gcc_assert (current_function_decl != NULL_TREE);

  FuncDeclaration *fd = cfun->language->function;
  tree decl = get_symbol_decl (var);

  gcc_assert (!TREE_STATIC (decl));

  set_input_location (var->loc);
  d_pushdecl (decl);
  DECL_CONTEXT (decl) = current_function_decl;

  /* Compiler generated symbols.  */
  if (var == fd->vresult || var == fd->v_argptr)
    DECL_ARTIFICIAL (decl) = 1;

  if (DECL_LANG_FRAME_FIELD (decl))
    {
      /* Fixes debugging local variables.  */
      SET_DECL_VALUE_EXPR (decl, get_decl_tree (var));
      DECL_HAS_VALUE_EXPR_P (decl) = 1;
    }
}

/* Return an unnamed local temporary of type TYPE.  */

tree
build_local_temp (tree type)
{
  tree decl = build_decl (input_location, VAR_DECL, NULL_TREE, type);

  DECL_CONTEXT (decl) = current_function_decl;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_IGNORED_P (decl) = 1;
  d_pushdecl (decl);

  return decl;
}

/* Return the correct decl to be used for DECL.  For VAR_DECLs, this could
   instead be a FIELD_DECL from a closure, or a RESULT_DECL from a named return
   value.  For PARM_DECLs, this could be a FIELD_DECL for a non-local `this'.
   For all other kinds of decls, this just returns the result of
   get_symbol_decl().  */

tree
get_decl_tree (Declaration *decl)
{
  tree t = get_symbol_decl (decl);
  FuncDeclaration *fd = cfun ? cfun->language->function : NULL;
  VarDeclaration *vd = decl->isVarDeclaration ();

  /* If cfun is NULL, then this is a global static.  */
  if (vd == NULL || fd == NULL)
    return t;

  /* Get the named return value.  */
  if (DECL_LANG_NRVO (t))
    return DECL_LANG_NRVO (t);

  /* Get the closure holding the var decl.  */
  if (DECL_LANG_FRAME_FIELD (t))
    {
      FuncDeclaration *parent = vd->toParent2 ()->isFuncDeclaration ();
      tree frame_ref = get_framedecl (fd, parent);

      return component_ref (build_deref (frame_ref),
			    DECL_LANG_FRAME_FIELD (t));
    }

  /* Get the non-local 'this' value by going through parent link
     of nested classes, this routine pretty much undoes what
     getRightThis in the frontend removes from codegen.  */
  if (vd->parent != fd && vd->isThisDeclaration ())
    {
      AggregateDeclaration *ad = fd->isThis ();
      gcc_assert (ad != NULL);

      t = get_symbol_decl (fd->vthis);
      Dsymbol *outer = fd;

      while (outer != vd->parent)
	{
	  gcc_assert (ad != NULL);
	  outer = ad->toParent2 ();

	  /* Get the this->this parent link.  */
	  tree vfield = get_symbol_decl (ad->vthis);
	  t = component_ref (build_deref (t), vfield);

	  ad = outer->isAggregateDeclaration ();
	  if (ad != NULL)
	    continue;

	  fd = outer->isFuncDeclaration ();
	  while (fd != NULL)
	    {
	      /* If outer function creates a closure, then the 'this'
		 value would be the closure pointer, and the real
		 'this' the first field of that closure.  */
	      tree ff = get_frameinfo (fd);
	      if (FRAMEINFO_CREATES_FRAME (ff))
		{
		  t = build_nop (build_pointer_type (FRAMEINFO_TYPE (ff)), t);
		  t = indirect_ref (build_ctype (fd->vthis->type), t);
		}

	      if (fd == vd->parent)
		break;

	      /* Continue looking for the right `this'.  */
	      outer = outer->toParent2 ();
	      fd = outer->isFuncDeclaration ();
	    }

	  ad = outer->isAggregateDeclaration ();
	}

      return t;
    }

  /* Auto variable that the back end will handle for us.  */
  return t;
}

/* Thunk code is based on g++ */

static int thunk_labelno;

/* Create a static alias to function.  */

static tree
make_alias_for_thunk (tree function)
{
  tree alias;
  char buf[256];

  /* Thunks may reference extern functions which cannot be aliased.  */
  if (DECL_EXTERNAL (function))
    return function;

  targetm.asm_out.generate_internal_label (buf, "LTHUNK", thunk_labelno);
  thunk_labelno++;

  alias = build_decl (DECL_SOURCE_LOCATION (function), FUNCTION_DECL,
		      get_identifier (buf), TREE_TYPE (function));
  DECL_LANG_SPECIFIC (alias) = DECL_LANG_SPECIFIC (function);
  lang_hooks.dup_lang_specific_decl (alias);
  DECL_CONTEXT (alias) = NULL_TREE;
  TREE_READONLY (alias) = TREE_READONLY (function);
  TREE_THIS_VOLATILE (alias) = TREE_THIS_VOLATILE (function);
  TREE_PUBLIC (alias) = 0;

  DECL_EXTERNAL (alias) = 0;
  DECL_ARTIFICIAL (alias) = 1;

  DECL_DECLARED_INLINE_P (alias) = 0;
  DECL_INITIAL (alias) = error_mark_node;
  DECL_ARGUMENTS (alias) = copy_list (DECL_ARGUMENTS (function));

  TREE_ADDRESSABLE (alias) = 1;
  TREE_USED (alias) = 1;
  SET_DECL_ASSEMBLER_NAME (alias, DECL_NAME (alias));

  if (!flag_syntax_only)
    {
      cgraph_node *aliasn;
      aliasn = cgraph_node::create_same_body_alias (alias, function);
      gcc_assert (aliasn != NULL);
    }
  return alias;
}

/* Emit the definition of a D vtable thunk.  */

static void
finish_thunk (tree thunk, tree function)
{
  /* Setup how D thunks are outputted.  */
  int fixed_offset = -THUNK_LANG_OFFSET (thunk);
  bool this_adjusting = true;
  tree alias;

  if (TARGET_USE_LOCAL_THUNK_ALIAS_P (function))
    alias = make_alias_for_thunk (function);
  else
    alias = function;

  TREE_ADDRESSABLE (function) = 1;
  TREE_USED (function) = 1;

  if (flag_syntax_only)
    {
      TREE_ASM_WRITTEN (thunk) = 1;
      return;
    }

  if (TARGET_USE_LOCAL_THUNK_ALIAS_P (function)
      && targetm_common.have_named_sections)
    {
      tree fn = function;
      symtab_node *symbol = symtab_node::get (function);

      if (symbol != NULL && symbol->alias)
	{
	  if (symbol->analyzed)
	    fn = symtab_node::get (function)->ultimate_alias_target ()->decl;
	  else
	    fn = symtab_node::get (function)->alias_target;
	}
      resolve_unique_section (fn, 0, flag_function_sections);

      if (DECL_SECTION_NAME (fn) != NULL && DECL_ONE_ONLY (fn))
	{
	  resolve_unique_section (thunk, 0, flag_function_sections);

	  /* Output the thunk into the same section as function.  */
	  set_decl_section_name (thunk, DECL_SECTION_NAME (fn));
	  symtab_node::get (thunk)->implicit_section
	    = symtab_node::get (fn)->implicit_section;
	}
    }

  /* Set up cloned argument trees for the thunk.  */
  tree t = NULL_TREE;
  for (tree a = DECL_ARGUMENTS (function); a; a = DECL_CHAIN (a))
    {
      tree x = copy_node (a);
      DECL_CHAIN (x) = t;
      DECL_CONTEXT (x) = thunk;
      SET_DECL_RTL (x, NULL);
      DECL_HAS_VALUE_EXPR_P (x) = 0;
      TREE_ADDRESSABLE (x) = 0;
      t = x;
    }
  DECL_ARGUMENTS (thunk) = nreverse (t);
  TREE_ASM_WRITTEN (thunk) = 1;

  cgraph_node *funcn, *thunk_node;

  funcn = cgraph_node::get_create (function);
  gcc_assert (funcn);
  thunk_node = funcn->create_thunk (thunk, thunk, this_adjusting,
				    fixed_offset, 0, 0, alias);

  if (DECL_ONE_ONLY (function))
    thunk_node->add_to_same_comdat_group (funcn);

  /* Target assemble_mi_thunk doesn't work across section boundaries
     on many targets, instead force thunk to be expanded in gimple.  */
  if (DECL_EXTERNAL (function))
    {
      /* cgraph::expand_thunk writes over current_function_decl, so if this
	 could ever be in use by the codegen pass, we want to know about it.  */
      gcc_assert (current_function_decl == NULL_TREE);

      if (!stdarg_p (TREE_TYPE (thunk)))
	{
	  /* Put generic thunk into COMDAT.  */
	  d_comdat_linkage (thunk);
	  thunk_node->create_edge (funcn, NULL, 0, CGRAPH_FREQ_BASE);
	  thunk_node->expand_thunk (false, true);
	}

      /* Tell the backend to not bother inlining the function, this is
	 assumed not to work as it could be referencing symbols outside
	 of the current compilation unit.  */
      DECL_UNINLINABLE (function) = 1;
    }
}

/* Return a thunk to DECL.  Thunks adjust the incoming `this' pointer by OFFSET.
   Adjustor thunks are created and pointers to them stored in the method entries
   in the vtable in order to set the this pointer to the start of the object
   instance corresponding to the implementing method.  */

tree
make_thunk (FuncDeclaration *decl, int offset)
{
  tree function = get_symbol_decl (decl);

  if (!DECL_ARGUMENTS (function) || !DECL_RESULT (function))
    {
      /* Compile the function body before generating the thunk, this is done
	 even if the decl is external to the current module.  */
      if (decl->fbody)
	build_decl_tree (decl);
      else
	{
	  /* Build parameters for functions that are not being compiled,
	     so that they can be correctly cloned in finish_thunk.  */
	  tree fntype = TREE_TYPE (function);
	  tree params = NULL_TREE;

	  for (tree t = TYPE_ARG_TYPES (fntype); t; t = TREE_CHAIN (t))
	    {
	      if (t == void_list_node)
		break;

	      tree param = build_decl (DECL_SOURCE_LOCATION (function),
				       PARM_DECL, NULL_TREE, TREE_VALUE (t));
	      DECL_ARG_TYPE (param) = TREE_TYPE (param);
	      DECL_ARTIFICIAL (param) = 1;
	      DECL_IGNORED_P (param) = 1;
	      DECL_CONTEXT (param) = function;
	      params = chainon (params, param);
	    }

	  DECL_ARGUMENTS (function) = params;

	  /* Also build the result decl, which is needed when force creating
	     the thunk in gimple inside cgraph_node::expand_thunk.  */
	  tree resdecl = build_decl (DECL_SOURCE_LOCATION (function),
				     RESULT_DECL, NULL_TREE,
				     TREE_TYPE (fntype));
	  DECL_ARTIFICIAL (resdecl) = 1;
	  DECL_IGNORED_P (resdecl) = 1;
	  DECL_CONTEXT (resdecl) = function;
	  DECL_RESULT (function) = resdecl;
	}
    }

  /* Don't build the thunk if the compilation step failed.  */
  if (global.errors)
    return error_mark_node;

  /* See if we already have the thunk in question.  */
  for (tree t = DECL_LANG_THUNKS (function); t; t = DECL_CHAIN (t))
    {
      if (THUNK_LANG_OFFSET (t) == offset)
	return t;
    }

  tree thunk = build_decl (DECL_SOURCE_LOCATION (function),
			   FUNCTION_DECL, NULL_TREE, TREE_TYPE (function));
  DECL_LANG_SPECIFIC (thunk) = DECL_LANG_SPECIFIC (function);
  lang_hooks.dup_lang_specific_decl (thunk);
  THUNK_LANG_OFFSET (thunk) = offset;

  TREE_READONLY (thunk) = TREE_READONLY (function);
  TREE_THIS_VOLATILE (thunk) = TREE_THIS_VOLATILE (function);
  TREE_NOTHROW (thunk) = TREE_NOTHROW (function);

  DECL_CONTEXT (thunk) = d_decl_context (decl);

  /* Thunks inherit the public access of the function they are targetting.  */
  TREE_PUBLIC (thunk) = TREE_PUBLIC (function);
  DECL_EXTERNAL (thunk) = 0;

  /* Thunks are always addressable.  */
  TREE_ADDRESSABLE (thunk) = 1;
  TREE_USED (thunk) = 1;
  DECL_ARTIFICIAL (thunk) = 1;
  DECL_DECLARED_INLINE_P (thunk) = 0;

  DECL_VISIBILITY (thunk) = DECL_VISIBILITY (function);
  /* Needed on some targets to avoid "causes a section type conflict".  */
  D_DECL_ONE_ONLY (thunk) = D_DECL_ONE_ONLY (function);
  DECL_COMDAT (thunk) = DECL_COMDAT (function);
  DECL_WEAK (thunk) = DECL_WEAK (function);

  tree target_name = DECL_ASSEMBLER_NAME (function);
  unsigned identlen = IDENTIFIER_LENGTH (target_name) + 14;
  const char *ident = XNEWVEC (const char, identlen);
  snprintf (CONST_CAST (char *, ident), identlen,
	    "_DT%u%s", offset, IDENTIFIER_POINTER (target_name));

  DECL_NAME (thunk) = get_identifier (ident);
  SET_DECL_ASSEMBLER_NAME (thunk, DECL_NAME (thunk));

  d_keep (thunk);

  finish_thunk (thunk, function);

  /* Add it to the list of thunks associated with the function.  */
  DECL_LANG_THUNKS (thunk) = NULL_TREE;
  DECL_CHAIN (thunk) = DECL_LANG_THUNKS (function);
  DECL_LANG_THUNKS (function) = thunk;

  return thunk;
}

/* Convenience function for layout_moduleinfo_fields.  Adds a field of TYPE to
   REC_TYPE at OFFSET, incrementing the offset to the next field position.  */

static void
add_moduleinfo_field (tree type, tree rec_type, size_t& offset)
{
  tree field = create_field_decl (type, NULL, 1, 1);
  insert_aggregate_field (Module::moduleinfo->loc, rec_type, field, offset);
  offset += int_size_in_bytes (type);
}

/* Layout fields that immediately come after the moduleinfo TYPE for DECL.
   Data relating to the module is packed into the type on an as-needed
   basis, this is done to keep its size to a minimum.  */

tree
layout_moduleinfo_fields (Module *decl, tree type)
{
  size_t offset = Module::moduleinfo->structsize;

  type = copy_aggregate_type (type);

  /* First fields added are all the function pointers.  */
  if (decl->sctor)
    add_moduleinfo_field (ptr_type_node, type, offset);
  if (decl->sdtor)
    add_moduleinfo_field (ptr_type_node, type, offset);
  if (decl->ssharedctor)
    add_moduleinfo_field (ptr_type_node, type, offset);
  if (decl->sshareddtor)
    add_moduleinfo_field (ptr_type_node, type, offset);
  if (decl->findGetMembers ())
    add_moduleinfo_field (ptr_type_node, type, offset);
  if (decl->sictor)
    add_moduleinfo_field (ptr_type_node, type, offset);
  if (decl->stest)
    add_moduleinfo_field (ptr_type_node, type, offset);

  /* Array of module imports is layed out as a length field, followed by
     a static array of ModuleInfo pointers.  */
  size_t aimports_dim = decl->aimports.dim;
  for (size_t i = 0; i < decl->aimports.dim; i++)
    {
      Module *mi = decl->aimports[i];
      if (!mi->needmoduleinfo)
	aimports_dim--;
    }

  if (aimports_dim)
    {
      Type *tn = Module::moduleinfo->type->pointerTo ();
      add_moduleinfo_field (size_type_node, type, offset);
      add_moduleinfo_field (make_array_type (tn, aimports_dim), type, offset);
    }

  /* Array of local ClassInfo decls are layed out in the same way.  */
  ClassDeclarations aclasses;
  for (size_t i = 0; i < decl->members->dim; i++)
    {
      Dsymbol *member = (*decl->members)[i];
      member->addLocalClass (&aclasses);
    }

  if (aclasses.dim)
    {
      Type *tn = Type::typeinfoclass->type;
      add_moduleinfo_field (size_type_node, type, offset);
      add_moduleinfo_field (make_array_type (tn, aclasses.dim), type, offset);
    }

  /* Lastly, the name of the module is a static char array.  */
  size_t namelen = strlen (decl->toPrettyChars ()) + 1;
  add_moduleinfo_field (make_array_type (Type::tchar, namelen), type, offset);

  finish_aggregate_type (offset, Module::moduleinfo->alignsize, type, NULL);

  return type;
}

/* Get the VAR_DECL of the ModuleInfo for DECL.  If this does not yet exist,
   create it.  The ModuleInfo decl is used to keep track of constructors,
   destructors, unittests, members, classes, and imports for the given module.
   This is used by the D runtime for module initialization and termination.  */

tree
get_moduleinfo_decl (Module *decl)
{
  if (decl->csym)
    return decl->csym;

  tree ident = make_internal_name (decl, "__ModuleInfo", "Z");
  tree type = build_ctype (Module::moduleinfo->type);

  decl->csym = declare_extern_var (ident, type);
  set_decl_location (decl->csym, decl);
  DECL_LANG_SPECIFIC (decl->csym) = build_lang_decl (NULL);

  DECL_CONTEXT (decl->csym) = build_import_decl (decl);
  /* Not readonly, moduleinit depends on this.  */
  TREE_READONLY (decl->csym) = 0;

  return decl->csym;
}

/* Get the VAR_DECL of the vtable symbol for DECL.  If this does not yet exist,
   create it.  The vtable is accessible via ClassInfo, but since it is needed
   frequently (like for rtti comparisons), make it directly accessible.  */

tree
get_vtable_decl (ClassDeclaration *decl)
{
  if (decl->vtblsym)
    return decl->vtblsym;

  tree ident = make_internal_name (decl, "__vtbl", "Z");
  /* Note: Using a static array type for the VAR_DECL, the DECL_INITIAL value
     will have a different type.  However the backend seems to accept this.  */
  tree type = build_ctype (Type::tvoidptr->sarrayOf (decl->vtbl.dim));

  decl->vtblsym = declare_extern_var (ident, type);
  set_decl_location (decl->vtblsym, decl);
  DECL_LANG_SPECIFIC (decl->vtblsym) = build_lang_decl (NULL);

  /* Class is a reference, want the record type.  */
  DECL_CONTEXT (decl->vtblsym) = TREE_TYPE (build_ctype (decl->type));
  TREE_READONLY (decl->vtblsym) = 1;
  DECL_VIRTUAL_P (decl->vtblsym) = 1;

  SET_DECL_ALIGN (decl->vtblsym, TARGET_VTABLE_ENTRY_ALIGN);
  DECL_USER_ALIGN (decl->vtblsym) = true;

  /* Could move setting comdat linkage to the caller, who knows whether
     this vtable is being emitted in this compilation.  */
  if (decl->isInstantiated ())
    d_comdat_linkage (decl->vtblsym);

  return decl->vtblsym;
}

/* Get the VAR_DECL of a class instance representing EXPR as static data.
   If this does not yet exist, create it.  This is used to support initializing
   a static variable that is of a class type using values known during CTFE.
   In user code, it is analogous to the following code snippet.

    enum E = new C(1, 2, 3);

   That we write the contents of `C(1, 2, 3)' to static data is only a compiler
   implementation detail.  The initialization of these symbols could be done at
   runtime using during as part of the module initialization or shared static
   constructors phase of runtime start-up - whichever comes after `gc_init()'.
   And infact that would be the better thing to do here eventually.  */

tree
build_new_class_expr (ClassReferenceExp *expr)
{
  if (expr->value->sym)
    return expr->value->sym;

  /* Build the reference symbol.  */
  tree type = build_ctype (expr->value->stype);
  expr->value->sym = build_artificial_decl (TREE_TYPE (type), NULL_TREE, "C");
  set_decl_location (expr->value->sym, expr->loc);

  DECL_INITIAL (expr->value->sym) = build_class_instance (expr);
  d_pushdecl (expr->value->sym);
  rest_of_decl_compilation (expr->value->sym, 1, 0);

  return expr->value->sym;
}

/* Get the VAR_DECL of the static initializer symbol for the struct/class DECL.
   If this does not yet exist, create it.  The static initializer data is
   accessible via TypeInfo, and is also used in 'new class' and default
   initializing struct literals.  */

tree
aggregate_initializer_decl (AggregateDeclaration *decl)
{
  if (decl->sinit)
    return decl->sinit;

  /* Class is a reference, want the record type.  */
  tree type = build_ctype (decl->type);
  StructDeclaration *sd = decl->isStructDeclaration ();
  if (!sd)
    type = TREE_TYPE (type);

  tree ident = make_internal_name (decl, "__init", "Z");

  decl->sinit = declare_extern_var (ident, type);
  set_decl_location (decl->sinit, decl);
  DECL_LANG_SPECIFIC (decl->sinit) = build_lang_decl (NULL);

  DECL_CONTEXT (decl->sinit) = type;
  TREE_READONLY (decl->sinit) = 1;

  /* Honor struct alignment set by user.  */
  if (sd && sd->alignment != STRUCTALIGN_DEFAULT)
    {
      SET_DECL_ALIGN (decl->sinit, sd->alignment * BITS_PER_UNIT);
      DECL_USER_ALIGN (decl->sinit) = true;
    }

  /* Could move setting comdat linkage to the caller, who knows whether
     this initializer is being emitted in this compilation.  */
  if (decl->isInstantiated ())
    d_comdat_linkage (decl->sinit);

  return decl->sinit;
}

/* Generate the data for the static initialiser.  */

tree
layout_class_initializer (ClassDeclaration *cd)
{
  NewExp *ne = new NewExp (cd->loc, NULL, NULL, cd->type, NULL);
  ne->type = cd->type;

  Expression *e = ne->ctfeInterpret ();
  gcc_assert (e->op == TOKclassreference);

  return build_class_instance ((ClassReferenceExp *) e);
}

tree
layout_struct_initializer (StructDeclaration *sd)
{
  StructLiteralExp *sle = StructLiteralExp::create (sd->loc, sd, NULL);

  if (!sd->fill (sd->loc, sle->elements, true))
    gcc_unreachable ();

  sle->type = sd->type;
  return build_expr (sle, true);
}

/* Get the VAR_DECL of the static initializer symbol for the enum DECL.
   If this does not yet exist, create it.  The static initializer data is
   accessible via TypeInfo_Enum, but the field member type is a byte[] that
   requires a pointer to a symbol reference.  */

tree
enum_initializer_decl (EnumDeclaration *decl)
{
  if (decl->sinit)
    return decl->sinit;

  tree type = build_ctype (decl->type);

  Identifier *ident_save = decl->ident;
  if (!decl->ident)
    decl->ident = Identifier::generateId ("__enum");
  tree ident = make_internal_name (decl, "__init", "Z");
  decl->ident = ident_save;

  decl->sinit = declare_extern_var (ident, type);
  set_decl_location (decl->sinit, decl);
  DECL_LANG_SPECIFIC (decl->sinit) = build_lang_decl (NULL);

  DECL_CONTEXT (decl->sinit) = d_decl_context (decl);
  TREE_READONLY (decl->sinit) = 1;

  /* Could move setting comdat linkage to the caller, who knows whether
     this initializer is being emitted in this compilation.  */
  if (decl->isInstantiated ())
    d_comdat_linkage (decl->sinit);

  return decl->sinit;
}

