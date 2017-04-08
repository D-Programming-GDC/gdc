// d-objfile.cc -- D frontend for GCC.
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

#include "dfrontend/attrib.h"
#include "dfrontend/enum.h"
#include "dfrontend/import.h"
#include "dfrontend/init.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"
#include "dfrontend/module.h"
#include "dfrontend/statement.h"
#include "dfrontend/staticassert.h"
#include "dfrontend/template.h"
#include "dfrontend/nspace.h"
#include "dfrontend/target.h"
#include "dfrontend/hdrgen.h"

#include "tree.h"
#include "tree-iterator.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "tm.h"
#include "function.h"
#include "dumpfile.h"
#include "cgraph.h"
#include "toplev.h"
#include "langhooks.h"
#include "target.h"
#include "common/common-target.h"
#include "stringpool.h"
#include "varasm.h"
#include "stor-layout.h"
#include "debug.h"
#include "tree-pretty-print.h"

#include "d-tree.h"
#include "d-objfile.h"
#include "d-codegen.h"
#include "d-dmd-gcc.h"
#include "id.h"

static FuncDeclaration *build_call_function (const char *, vec<FuncDeclaration *>, bool);
static tree build_emutls_function (vec<VarDeclaration *> tlsVars);
static tree build_ctor_function (const char *, vec<FuncDeclaration *>, vec<VarDeclaration *>);
static tree build_dtor_function (const char *, vec<FuncDeclaration *>);
static tree build_unittest_function (const char *, vec<FuncDeclaration *>);

// Module info.  Assuming only one module per run of the compiler.
ModuleInfo *current_module_info;

// static constructors (not D static constructors)
static vec<FuncDeclaration *> static_ctor_list;
static vec<FuncDeclaration *> static_dtor_list;

// Returns true if DSYM is from the gcc.attribute module.

static bool
gcc_attribute_p (Dsymbol *dsym)
{
  ModuleDeclaration *md = dsym->getModule ()->md;

  if (md && md->packages && md->packages->dim == 1)
    {
      if (!strcmp ((*md->packages)[0]->toChars (), "gcc")
	  && !strcmp (md->id->toChars (), "attribute"))
	return true;
    }

  return false;
}

/* Create the FUNCTION_DECL for a function definition.
   This function creates a binding context for the function body
   as well as setting up the FUNCTION_DECL in current_function_decl.  */

static void
start_function (FuncDeclaration *decl)
{
  cfun->language = ggc_cleared_alloc<language_function>();
  cfun->language->function = decl;

  // Default chain value is 'null' unless parent found.
  cfun->language->static_chain = null_pointer_node;

  // Find module for this function
  for (Dsymbol *p = decl->parent; p != NULL; p = p->parent)
    {
      cfun->language->module = p->isModule ();
      if (cfun->language->module)
	break;
    }
  gcc_assert (cfun->language->module != NULL);

  // Check if we have a static this or unitest function.
  ModuleInfo *mi = current_module_info;

  if (decl->isSharedStaticCtorDeclaration ())
    mi->sharedctors.safe_push (decl);
  else if (decl->isStaticCtorDeclaration ())
    mi->ctors.safe_push (decl);
  else if (decl->isSharedStaticDtorDeclaration ())
    {
      VarDeclaration *vgate = ((SharedStaticDtorDeclaration *) decl)->vgate;
      if (vgate != NULL)
	mi->sharedctorgates.safe_push (vgate);
      mi->shareddtors.safe_push (decl);
    }
  else if (decl->isStaticDtorDeclaration ())
    {
      VarDeclaration *vgate = ((StaticDtorDeclaration *) decl)->vgate;
      if (vgate != NULL)
	mi->ctorgates.safe_push (vgate);
      mi->dtors.safe_push (decl);
    }
  else if (decl->isUnitTestDeclaration ())
    mi->unitTests.safe_push (decl);
}

/* Finish up a function declaration and compile that function
   all the way to assembler language output.  The free the storage
   for the function definition.  */

static void
finish_function (void)
{
  gcc_assert (vec_safe_is_empty (cfun->language->stmt_list));

  ggc_free (cfun->language);
  cfun->language = NULL;
}

/* Implements the visitor interface to lower all Declaration AST classes
   emitted from the D Front-end to GCC trees.
   All visit methods accept one parameter D, which holds the frontend AST
   of the declaration to compile.  These also don't return any value, instead
   generated code are appened to global_declarations or added to the
   current_binding_level by d_pushdecl().  */

class DeclVisitor : public Visitor
{
public:
  DeclVisitor (void) { }

  /* This should be overridden by each declaration class.  */

  void visit (Dsymbol *)
  {
  }

  /* Compile a D module, and all members of it.  */

  void visit (Module *d)
  {
    if (d->semanticRun >= PASSobj)
      return;

    /* There may be more than one module per object file, but should only
       ever compile them one at a time.  */
    assert (!current_module_info && !current_module_decl);

    ModuleInfo mi = ModuleInfo ();

    current_module_info = &mi;
    current_module_decl = d;

    if (d->members)
      {
	for (size_t i = 0; i < d->members->dim; i++)
	  {
	    Dsymbol *s = (*d->members)[i];
	    s->accept (this);
	  }
      }

    /* Default behaviour is to always generate module info because of templates.
       Can be switched off for not compiling against runtime library.  */
    if (!global.params.betterC && d->ident != Id::entrypoint)
      {
	if (!mi.ctors.is_empty () || !mi.ctorgates.is_empty ())
	  d->sctor = build_ctor_function ("*__modctor", mi.ctors, mi.ctorgates);

	if (!mi.dtors.is_empty ())
	  d->sdtor = build_dtor_function ("*__moddtor", mi.dtors);

	if (!mi.sharedctors.is_empty () || !mi.sharedctorgates.is_empty ())
	  d->ssharedctor = build_ctor_function ("*__modsharedctor",
						mi.sharedctors,
						mi.sharedctorgates);

	if (!mi.shareddtors.is_empty ())
	  d->sshareddtor = build_dtor_function ("*__modshareddtor",
						mi.shareddtors);

	if (!mi.unitTests.is_empty ())
	  d->stest = build_unittest_function ("*__modtest", mi.unitTests);

	layout_moduleinfo (d);
      }

    current_module_info = NULL;
    current_module_decl = NULL;

    d->semanticRun = PASSobj;
  }

  /* Write imported symbol D to debug.  */

  void visit (Import *d)
  {
    /* Implements import declarations by telling the debug backend we are
       importing the NAMESPACE_DECL of the module or IMPORTED_DECL of the
       declaration into the current lexical scope CONTEXT.  NAME is set if
       this is a renamed import.  */
    if (d->isstatic)
      return;

    /* Get the context of this import, this should never be null.  */
    tree context;
    if (cfun != NULL)
      context = current_function_decl;
    else
      context = build_import_decl (current_module_decl);

    if (d->ident == NULL)
      {
	/* Importing declaration list.  */
	for (size_t i = 0; i < d->names.dim; i++)
	  {
	    AliasDeclaration *aliasdecl = d->aliasdecls[i];
	    tree decl = build_import_decl (aliasdecl);

	    /* Skip over unhandled imports.  */
	    if (decl == NULL_TREE)
	      continue;

	    set_decl_location (decl, d);

	    Identifier *alias = d->aliases[i];
	    tree name = (alias != NULL)
	      ? get_identifier (alias->toChars ()) : NULL_TREE;

	    (*debug_hooks->imported_module_or_decl)(decl, name, context, false);
	  }
      }
    else
      {
	/* Importing the entire module.  */
	tree decl = build_import_decl (d->mod);
	set_input_location (d);

	tree name = (d->aliasId != NULL)
	  ? get_identifier (d->aliasId->toChars ()) : NULL_TREE;

	(*debug_hooks->imported_module_or_decl)(decl, name, context, false);
      }
  }

  /* Expand any local variables found in tuples.  */

  void visit (TupleDeclaration *d)
  {
    for (size_t i = 0; i < d->objects->dim; i++)
      {
	RootObject *o = (*d->objects)[i];
	if ((o->dyncast () == DYNCAST_EXPRESSION)
	    && ((Expression *) o)->op == TOKdsymbol)
	  {
	    Declaration *d = ((DsymbolExp *) o)->s->isDeclaration ();
	    if (d)
	      d->accept (this);
	  }
      }
  }

  /* Walk over all declarations in the attribute scope.  */

  void visit (AttribDeclaration *d)
  {
    Dsymbols *ds = d->include (NULL, NULL);

    if (!ds)
      return;

    for (size_t i = 0; i < ds->dim; i++)
      {
	Dsymbol *s = (*ds)[i];
	s->accept (this);
      }
  }

  /* */

  void visit (PragmaDeclaration *d)
  {
    if (!global.params.ignoreUnsupportedPragmas)
      {
	if (d->ident == Id::lib || d->ident == Id::startaddress)
	  {
	    warning_at (get_linemap (d->loc), OPT_Wunknown_pragmas,
			"pragma(%s) not implemented", d->ident->toChars ());
	  }
      }

    visit ((AttribDeclaration *) d);
  }

  /* Walk over all members in the namespace scope.  */

  void visit (Nspace *d)
  {
    if (isError (d) || !d->members)
      return;

    for (size_t i = 0; i < d->members->dim; i++)
      {
	Dsymbol *s = (*d->members)[i];
	s->accept (this);
      }
  }

  /* Walk over all members in the instantiated template.  */

  void visit (TemplateInstance *d)
  {
    if (isError (d)|| !d->members)
      return;

    if (!d->needsCodegen ())
      return;

    for (size_t i = 0; i < d->members->dim; i++)
      {
	Dsymbol *s = (*d->members)[i];
	s->accept (this);
      }
  }

  /* Walk over all members in the mixin template scope.  */

  void visit (TemplateMixin *d)
  {
    if (isError (d)|| !d->members)
      return;

    for (size_t i = 0; i < d->members->dim; i++)
      {
	Dsymbol *s = (*d->members)[i];
	s->accept (this);
      }
  }

  /*  */

  void visit (StructDeclaration *d)
  {
    if (d->type->ty == Terror)
      {
	d->error ("had semantic errors when compiling");
	return;
      }

    /* Add this decl to the current binding level.  */
    tree ctype = build_ctype (d->type);
    if (TYPE_NAME (ctype))
      d_pushdecl (TYPE_NAME (ctype));

    /* Anonymous structs/unions only exist as part of others,
       do not output forward referenced structs's.  */
    if (d->isAnonymous () || !d->members)
      return;

    /* Don't emit any symbols from gcc.attribute module.  */
    if (gcc_attribute_p (d))
      return;

    /* Generate TypeInfo.  */
    genTypeInfo (d->type, NULL);

    /* Generate static initialiser.  */
    d->sinit = aggregate_initializer_decl (d);
    DECL_INITIAL (d->sinit) = layout_struct_initializer (d);

    d_finish_symbol (d->sinit);

    /* Put out the members.  */
    for (size_t i = 0; i < d->members->dim; i++)
      {
	Dsymbol *member = (*d->members)[i];
	/* There might be static ctors in the members, and they cannot
	   be put in separate object files.  */
	member->accept (this);
      }

    /* Put out xopEquals, xopCmp and xopHash.  */
    if (d->xeq && d->xeq != d->xerreq)
      d->xeq->accept (this);

    if (d->xcmp && d->xcmp != d->xerrcmp)
      d->xcmp->accept (this);

    if (d->xhash)
      d->xhash->accept (this);
  }

  /*  */
  void visit (ClassDeclaration *d)
  {
    if (d->type->ty == Terror)
      {
	d->error ("had semantic errors when compiling");
	return;
      }

    if (!d->members)
      return;

    /* Put out the members.  */
    for (size_t i = 0; i < d->members->dim; i++)
      {
	Dsymbol *member = (*d->members)[i];
	member->accept (this);
      }

    /* Generate C symbols.  */
    d->csym = get_classinfo_decl (d);
    d->vtblsym = get_vtable_decl (d);
    d->sinit = aggregate_initializer_decl (d);

    /* Generate static initialiser.  */
    DECL_INITIAL (d->sinit) = layout_class_initializer (d);
    d_finish_symbol (d->sinit);

    /* Put out the TypeInfo.  */
    genTypeInfo (d->type, NULL);
    DECL_INITIAL (d->csym) = layout_classinfo (d);
    d_finish_symbol (d->csym);

    /* Put out the vtbl[].  */
    vec<constructor_elt, va_gc> *elms = NULL;

    /* First entry is ClassInfo reference.  */
    if (d->vtblOffset ())
      CONSTRUCTOR_APPEND_ELT (elms, size_zero_node, build_address (d->csym));

    for (size_t i = d->vtblOffset (); i < d->vtbl.dim; i++)
      {
	FuncDeclaration *fd = d->vtbl[i]->isFuncDeclaration ();

	if (!fd || (!fd->fbody && d->isAbstract ()))
	  continue;

	fd->functionSemantic ();

	if (d->isFuncHidden (fd))
	  {
	    /* The function fd is hidden from the view of the class.
	       If it overlaps with any function in the vtbl[], then
	       issue an error.  */
	    for (size_t j = 1; j < d->vtbl.dim; j++)
	      {
		if (j == i)
		  continue;

		FuncDeclaration *fd2 = d->vtbl[j]->isFuncDeclaration ();
		if (!fd2->ident->equals (fd->ident))
		  continue;

		if (fd->leastAsSpecialized (fd2) || fd2->leastAsSpecialized (fd))
		  {
		    TypeFunction *tf = (TypeFunction *) fd->type;
		    if (tf->ty == Tfunction)
		      {
			d->error ("use of %s%s is hidden by %s; "
				  "use 'alias %s = %s.%s;' "
				  "to introduce base class overload set.",
				  fd->toPrettyChars (),
				  parametersTypeToChars (tf->parameters, tf->varargs),
				  d->toChars (), fd->toChars (),
				  fd->parent->toChars (), fd->toChars ());
		      }
		    else
		      {
			error ("use of %s is hidden by %s",
			       fd->toPrettyChars (), d->toChars ());
		      }

		    break;
		  }
	      }
	  }

	CONSTRUCTOR_APPEND_ELT (elms, size_int (i),
				build_address (get_symbol_decl (fd)));
      }

    DECL_INITIAL (d->vtblsym)
      = build_constructor (TREE_TYPE (d->vtblsym), elms);
    d_finish_symbol (d->vtblsym);

    /* Add this decl to the current binding level.  */
    tree ctype = TREE_TYPE (build_ctype (d->type));
    if (TYPE_NAME (ctype))
      d_pushdecl (TYPE_NAME (ctype));
  }

  /*  */

  void visit (InterfaceDeclaration *d)
  {
    if (d->type->ty == Terror)
      {
	d->error ("had semantic errors when compiling");
	return;
      }

    if (!d->members)
      return;

    /* Put out the members.  */
    for (size_t i = 0; i < d->members->dim; i++)
      {
	Dsymbol *member = (*d->members)[i];
	member->accept (this);
      }

    /* Generate C symbols.  */
    d->csym = get_classinfo_decl (d);

    /* Put out the TypeInfo.  */
    genTypeInfo (d->type, NULL);
    d->type->vtinfo->accept (this);
    DECL_INITIAL (d->csym) = layout_classinfo (d);
    d_finish_symbol (d->csym);

    /* Add this decl to the current binding level.  */
    tree ctype = TREE_TYPE (build_ctype (d->type));
    if (TYPE_NAME (ctype))
      d_pushdecl (TYPE_NAME (ctype));
  }

  /*  */

  void visit (EnumDeclaration *d)
  {
    if (d->semanticRun >= PASSobj)
      return;

    if (d->errors || d->type->ty == Terror)
      {
	d->error ("had semantic errors when compiling");
	return;
      }

    if (d->isAnonymous ())
      return;

    /* Generate TypeInfo.  */
    genTypeInfo (d->type, NULL);

    TypeEnum *tc = (TypeEnum *) d->type;
    if (tc->sym->members && !d->type->isZeroInit ())
      {
	/* Generate static initialiser.  */
	d->sinit = enum_initializer_decl (d);
	DECL_INITIAL (d->sinit) = build_expr (tc->sym->defaultval, true);
	d_finish_symbol (d->sinit);

	/* Add this decl to the current binding level.  */
	tree ctype = build_ctype (d->type);
	if (TREE_CODE (ctype) == ENUMERAL_TYPE && TYPE_NAME (ctype))
	  d_pushdecl (TYPE_NAME (ctype));
      }

    d->semanticRun = PASSobj;
  }

  /*  */

  void visit (VarDeclaration *d)
  {
    if (d->type->ty == Terror)
      {
	d->error ("had semantic errors when compiling");
	return;
      }

    if (d->aliassym)
      {
	d->toAlias ()->accept (this);
	return;
      }

    /* Do not store variables we cannot take the address of,
       but keep the values for purposes of debugging.  */
    if (!d->canTakeAddressOf ())
      {
	/* Don't know if there is a good way to handle instantiations.  */
	if (d->isInstantiated ())
	  return;

	tree decl = get_symbol_decl (d);
	gcc_assert (d->_init && !d->_init->isVoidInitializer ());
	Expression *ie = d->_init->toExpression ();

	/* CONST_DECL was initially intended for enumerals and may be used for
	   scalars in general, but not for aggregates.  Here a non-constant
	   value is generated anyway so as the CONST_DECL only serves as a
	   placeholder for the value, however the DECL itself should never be
	   referenced in any generated code, or passed to the backend.  */
	if (!d->type->isscalar ())
	  DECL_INITIAL (decl) = build_expr (ie, false);
	else
	  {
	    DECL_INITIAL (decl) = build_expr (ie, true);
	    d_pushdecl (decl);
	    rest_of_decl_compilation (decl, 1, 0);
	  }
      }
    else if (d->isDataseg () && !(d->storage_class & STCextern))
      {
	tree s = get_symbol_decl (d);

	/* Duplicated VarDeclarations map to the same symbol. Check if this
	   is the one declaration which will be emitted.  */
	tree ident = DECL_ASSEMBLER_NAME (s);
	if (IDENTIFIER_DSYMBOL (ident) && IDENTIFIER_DSYMBOL (ident) != d)
	  return;

	if (d->isThreadlocal ())
	  {
	    ModuleInfo *mi = current_module_info;
	    mi->tlsVars.safe_push (d);
	  }

	/* How big a symbol can be should depend on backend.  */
	tree size = build_integer_cst (d->type->size (d->loc),
				       build_ctype (Type::tsize_t));
	if (!valid_constant_size_p (size))
	  {
	    d->error ("size is too large");
	    return;
	  }

	if (d->_init && !d->_init->isVoidInitializer ())
	  {
	    Expression *e = d->_init->toExpression (d->type);
	    DECL_INITIAL (s) = build_expr (e, true);
	  }
	else
	  {
	    if (d->type->ty == Tstruct)
	      {
		StructDeclaration *sd = ((TypeStruct *) d->type)->sym;
		DECL_INITIAL (s) = layout_struct_initializer (sd);
	      }
	    else
	      {
		Expression *e = d->type->defaultInitLiteral (d->loc);
		DECL_INITIAL (s) = build_expr (e, true);
	      }
	  }

	/* Frontend should have already caught this.  */
	gcc_assert (!integer_zerop (size)
		    || d->type->toBasetype ()->ty == Tsarray);

	d_finish_symbol (s);
      }
    else if (!d->isDataseg () && !d->isMember ())
      {
	/* This is needed for VarDeclarations in mixins that are to be local
	   variables of a function.  Otherwise, it would be enough to make
	   a check for isVarDeclaration() in DeclarationExp codegen.  */
	build_local_var (d);

	if (d->_init)
	  {
	    if (!d->_init->isVoidInitializer ())
	      {
		ExpInitializer *vinit = d->_init->isExpInitializer ();
		Expression *ie = vinit->toExpression ();
		tree exp = build_expr (ie);
		add_stmt (exp);
	      }
	    else if (d->size (d->loc) != 0)
	      {
		/* Zero-length arrays do not have an initializer.  */
		warning (OPT_Wuninitialized, "uninitialized variable '%s'",
			 d->ident ? d->ident->toChars () : "(no name)");
	      }
	  }
      }
  }

  /*  */

  void visit (TypeInfoDeclaration *d)
  {
    if (isSpeculativeType (d->tinfo))
      return;

    tree s = get_typeinfo_decl (d);
    DECL_INITIAL (s) = layout_typeinfo (d);
    d_finish_symbol (s);
  }

  /* Finish up a function declaration and compile it all the way
     down to assembler language output.  */

  void visit (FuncDeclaration *d)
  {
    /* Already generated the function.  */
    if (d->semanticRun >= PASSobj)
      return;

    /* Don't emit any symbols from gcc.attribute module.  */
    if (gcc_attribute_p (d))
      return;

    /* Not emitting unittest functions.  */
    if (!global.params.useUnitTests && d->isUnitTestDeclaration ())
      return;

    /* Check if any errors occurred when running semantic.  */
    if (d->type->ty == Tfunction)
      {
	TypeFunction *tf = (TypeFunction *) d->type;
	if (tf->next == NULL || tf->next->ty == Terror)
	  return;
      }

    if (d->semantic3Errors)
      return;

    if (d->isNested ())
      {
	FuncDeclaration *fdp = d;
	while (fdp && fdp->isNested ())
	  {
	    fdp = fdp->toParent2 ()->isFuncDeclaration ();

	    if (fdp == NULL)
	      break;

	    /* Parent failed to compile, but errors were gagged.  */
	    if (fdp->semantic3Errors)
	      return;
	  }
      }

    /* Ensure all semantic passes have ran.  */
    if (d->semanticRun < PASSsemantic3)
      {
	d->functionSemantic3 ();
	Module::runDeferredSemantic3 ();
      }

    if (global.errors)
      return;

    /* Duplicated FuncDeclarations map to the same symbol. Check if this
       is the one declaration which will be emitted.  */
    tree fndecl = get_symbol_decl (d);
    tree ident = DECL_ASSEMBLER_NAME (fndecl);
    if (IDENTIFIER_DSYMBOL (ident) && IDENTIFIER_DSYMBOL (ident) != d)
      return;

    /* For nested functions in particular, unnest fndecl in the cgraph, as
       all static chain passing is handled by the front-end.  Do this even
       if we are not emitting the body.  */
    struct cgraph_node *node = cgraph_node::get_create (fndecl);
    if (node->origin)
      node->unnest ();

    if (!d->fbody)
      {
	rest_of_decl_compilation (fndecl, 1, 0);
	return;
      }

    /* Start generating code for this function.  */
    gcc_assert (d->semanticRun == PASSsemantic3done);
    d->semanticRun = PASSobj;

    if (global.params.verbose)
      fprintf (global.stdmsg, "function  %s\n", d->toPrettyChars ());

    /* This function exists in static storage.
       (This does not mean `static' in the C sense!)  */
    TREE_STATIC (fndecl) = 1;

    /* Add this decl to the current binding level.  */
    d_pushdecl (fndecl);

    bool nested = current_function_decl != NULL_TREE;
    if (nested)
      push_function_context ();
    current_function_decl = fndecl;

    tree return_type = TREE_TYPE (TREE_TYPE (fndecl));
    tree result_decl = build_decl (UNKNOWN_LOCATION, RESULT_DECL,
				   NULL_TREE, return_type);

    set_decl_location (result_decl, d);
    DECL_RESULT (fndecl) = result_decl;
    DECL_CONTEXT (result_decl) = fndecl;
    DECL_ARTIFICIAL (result_decl) = 1;
    DECL_IGNORED_P (result_decl) = 1;

    allocate_struct_function (fndecl, false);
    set_function_end_locus (d->endloc);

    start_function (d);

    tree parm_decl = NULL_TREE;
    tree param_list = NULL_TREE;

    /* Special arguments...  */

    /* 'this' parameter:
       For nested functions, D still generates a vthis, but it
       should not be referenced in any expression.  */
    if (d->vthis)
      {
	parm_decl = get_symbol_decl (d->vthis);
	DECL_ARTIFICIAL (parm_decl) = 1;
	TREE_READONLY (parm_decl) = 1;

	if (d->vthis->type == Type::tvoidptr)
	  {
	    /* Replace generic pointer with backend closure type
	       (this wins for gdb).  */
	    tree frame_type = FRAMEINFO_TYPE (get_frameinfo (d));
	    gcc_assert (frame_type != NULL_TREE);
	    TREE_TYPE (parm_decl) = build_pointer_type (frame_type);
	  }

	set_decl_location (parm_decl, d->vthis);
	param_list = chainon (param_list, parm_decl);
	cfun->language->static_chain = parm_decl;
      }

    /* _arguments parameter.  */
    if (d->v_arguments)
      {
	parm_decl = get_symbol_decl (d->v_arguments);
	set_decl_location (parm_decl, d->v_arguments);
	param_list = chainon (param_list, parm_decl);
      }

    /* formal function parameters.  */
    size_t n_parameters = d->parameters ? d->parameters->dim : 0;

    for (size_t i = 0; i < n_parameters; i++)
      {
	VarDeclaration *param = (*d->parameters)[i];
	parm_decl = get_symbol_decl (param);
	set_decl_location (parm_decl, (Dsymbol *) param);
	/* Chain them in the correct order.  */
	param_list = chainon (param_list, parm_decl);
      }

    DECL_ARGUMENTS (fndecl) = param_list;
    rest_of_decl_compilation (fndecl, 1, 0);
    DECL_INITIAL (fndecl) = error_mark_node;

    push_stmt_list ();
    push_binding_level (level_function);
    set_input_location (d->loc);

    /* If this is a member function that nested (possibly indirectly) in another
       function, construct an expession for this member function's static chain
       by going through parent link of nested classes.  */
    if (d->isThis ())
      {
	AggregateDeclaration *ad = d->isThis ();
	tree this_tree = get_symbol_decl (d->vthis);

	while (ad->isNested ())
	  {
	    Dsymbol *pd = ad->toParent2 ();
	    tree vthis_field = get_symbol_decl (ad->vthis);
	    this_tree = component_ref (build_deref (this_tree), vthis_field);

	    ad = pd->isAggregateDeclaration ();
	    if (ad == NULL)
	      {
		cfun->language->static_chain = this_tree;
		break;
	      }
	  }
      }

    /* May change cfun->static_chain.  */
    build_closure (d);

    if (d->vresult)
      build_local_var (d->vresult);

    if (d->v_argptr)
      push_stmt_list ();

    /* The fabled D named return value optimisation.
       Implemented by overriding all the RETURN_EXPRs and replacing all
       occurrences of VAR with the RESULT_DECL for the function.
       This is only worth doing for functions that can return in memory.  */
    if (d->nrvo_can)
      {
	if (!AGGREGATE_TYPE_P (return_type))
	  d->nrvo_can = 0;
	else
	  d->nrvo_can = aggregate_value_p (return_type, fndecl);
      }

    if (d->nrvo_can)
      {
	TREE_TYPE (result_decl)
	  = build_reference_type (TREE_TYPE (result_decl));
	DECL_BY_REFERENCE (result_decl) = 1;
	TREE_ADDRESSABLE (result_decl) = 0;
	relayout_decl (result_decl);

	if (d->nrvo_var)
	  {
	    tree var = get_symbol_decl (d->nrvo_var);

	    /* Copy name from VAR to RESULT.  */
	    DECL_NAME (result_decl) = DECL_NAME (var);
	    /* Don't forget that we take it's address.  */
	    TREE_ADDRESSABLE (var) = 1;

	    result_decl = build_deref (result_decl);

	    SET_DECL_VALUE_EXPR (var, result_decl);
	    DECL_HAS_VALUE_EXPR_P (var) = 1;

	    SET_DECL_LANG_NRVO (var, result_decl);
	  }
      }

    build_ir (d);

    if (d->v_argptr)
      {
	tree body = pop_stmt_list ();
	tree var = get_decl_tree (d->v_argptr);
	var = build_address (var);

	tree init_exp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_VA_START),
					   2, var, parm_decl);
	build_local_var (d->v_argptr);
	add_stmt (init_exp);

	tree cleanup = d_build_call_nary (builtin_decl_explicit (BUILT_IN_VA_END),
					  1, var);
	add_stmt (build2 (TRY_FINALLY_EXPR, void_type_node, body, cleanup));
      }

    /* Backend expects a statement list to come from somewhere, however
       popStatementList returns expressions when there is a single statement.
       So here we create a statement list unconditionally.  */
    tree block = pop_binding_level ();
    tree body = pop_stmt_list ();
    tree bind = build3 (BIND_EXPR, void_type_node,
			BLOCK_VARS (block), body, block);

    if (TREE_CODE (body) != STATEMENT_LIST)
      {
	tree stmtlist = alloc_stmt_list ();
	append_to_statement_list_force (body, &stmtlist);
	BIND_EXPR_BODY (bind) = stmtlist;
      }
    else if (!STATEMENT_LIST_HEAD (body))
      {
	/* For empty functions: Without this, there is a segfault when inlined.
	   Seen on build=ppc-linux but not others (why?).  */
	tree ret = return_expr (NULL_TREE);
	append_to_statement_list_force (ret, &body);
      }

    DECL_SAVED_TREE (fndecl) = bind;

    if (!errorcount && !global.errors)
      {
	/* Dump the D-specific tree IR.  */
	int local_dump_flags;
	FILE *dump_file = dump_begin (TDI_original, &local_dump_flags);
	if (dump_file)
	  {
	    fprintf (dump_file, "\n;; Function %s",
		     lang_hooks.decl_printable_name (fndecl, 2));
	    fprintf (dump_file, " (%s)\n",
		     (!DECL_ASSEMBLER_NAME_SET_P (fndecl) ? "null"
		      : IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl))));
	    fprintf (dump_file, ";; enabled by -%s\n",
		     dump_flag_name (TDI_original));
	    fprintf (dump_file, "\n");

	    if (local_dump_flags & TDF_RAW)
	      dump_node (DECL_SAVED_TREE (fndecl),
			 TDF_SLIM | local_dump_flags, dump_file);
	    else
	      print_generic_expr (dump_file, DECL_SAVED_TREE (fndecl),
				  local_dump_flags);
	    fprintf (dump_file, "\n");

	    dump_end (TDI_original, dump_file);
	  }
      }

    if (!errorcount && !global.errors)
      d_finish_function (d);

    finish_function ();

    if (nested)
      pop_function_context ();
    else
      {
	set_cfun (NULL);
	current_function_decl = NULL;
      }
  }
};

/* Main entry point for the DeclVisitor interface to send
   the Declaration AST class D to GCC backend.  */

void
build_decl_tree (Dsymbol *d)
{
  DeclVisitor v = DeclVisitor();
  d->accept (&v);
}

// Get offset of base class's vtbl[] initialiser from start of csym.

unsigned
base_vtable_offset (ClassDeclaration *cd, BaseClass *bc)
{
  unsigned csymoffset = Target::classinfosize;
  csymoffset += cd->vtblInterfaces->dim * (4 * Target::ptrsize);

  for (size_t i = 0; i < cd->vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*cd->vtblInterfaces)[i];
      if (b == bc)
	return csymoffset;
      csymoffset += b->sym->vtbl.dim * Target::ptrsize;
    }

  // Put out the overriding interface vtbl[]s.
  for (ClassDeclaration *cd2 = cd->baseClass; cd2; cd2 = cd2->baseClass)
    {
      for (size_t k = 0; k < cd2->vtblInterfaces->dim; k++)
	{
	  BaseClass *bs = (*cd2->vtblInterfaces)[k];
	  if (bs->fillVtbl(cd, NULL, 0))
	    {
	      if (bc == bs)
		return csymoffset;
	      csymoffset += bs->sym->vtbl.dim * Target::ptrsize;
	    }
	}
    }

  return ~0;
}

// Build the ModuleInfo symbol for Module m

static tree
build_moduleinfo_symbol (Module *m)
{
  ClassDeclarations aclasses;
  FuncDeclaration *sgetmembers;

  if (Module::moduleinfo == NULL)
    ObjectNotFound (Id::ModuleInfo);

  for (size_t i = 0; i < m->members->dim; i++)
    {
      Dsymbol *member = (*m->members)[i];
      member->addLocalClass (&aclasses);
    }

  size_t aimports_dim = m->aimports.dim;
  for (size_t i = 0; i < m->aimports.dim; i++)
    {
      Module *mi = m->aimports[i];
      if (!mi->needmoduleinfo)
	aimports_dim--;
    }

  sgetmembers = m->findGetMembers ();

  size_t flags = 0;
  if (m->sctor)
    flags |= MItlsctor;
  if (m->sdtor)
    flags |= MItlsdtor;
  if (m->ssharedctor)
    flags |= MIctor;
  if (m->sshareddtor)
    flags |= MIdtor;
  if (sgetmembers)
    flags |= MIxgetMembers;
  if (m->sictor)
    flags |= MIictor;
  if (m->stest)
    flags |= MIunitTest;
  if (aimports_dim)
    flags |= MIimportedModules;
  if (aclasses.dim)
    flags |= MIlocalClasses;
  if (!m->needmoduleinfo)
    flags |= MIstandalone;

  flags |= MIname;

  tree decl = get_moduleinfo_decl (m);
  tree type = layout_moduleinfo_fields (m, TREE_TYPE (decl));
  tree field = TYPE_FIELDS (type);

  /* Put out the two named fields in a ModuleInfo decl:
	uint flags;
	uint index;  */
  vec<constructor_elt, va_gc> *minit = NULL;

  CONSTRUCTOR_APPEND_ELT (minit, field,
			  build_integer_cst (flags, TREE_TYPE (field)));
  field = TREE_CHAIN (field);

  CONSTRUCTOR_APPEND_ELT (minit, field,
			  build_integer_cst (0, TREE_TYPE (field)));
  field = TREE_CHAIN (field);

  /* EmuTLS scan function is added for targets that don't have native TLS.  */
  if (!targetm.have_tls)
    {
      if (current_module_info->tlsVars.is_empty ())
	CONSTRUCTOR_APPEND_ELT (minit, field, null_pointer_node);
      else
	{
	  tree emutls = build_emutls_function (current_module_info->tlsVars);
	  CONSTRUCTOR_APPEND_ELT (minit, field, build_address (emutls));
	}

      field = TREE_CHAIN (field);
    }

  /* Order of appearance, depending on flags:
	void function() tlsctor;
	void function() tlsdtor;
	void* function() xgetMembers;
	void function() ctor;
	void function() dtor;
	void function() ictor;
	void function() unitTest;
	ModuleInfo*[] importedModules;
	TypeInfo_Class[] localClasses;
	char[N] name;
   */
  if (flags & MItlsctor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sctor));
      field = TREE_CHAIN (field);
    }

  if (flags & MItlsdtor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sdtor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIctor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->ssharedctor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIdtor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sshareddtor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIxgetMembers)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field,
			      build_address (get_symbol_decl (sgetmembers)));
      field = TREE_CHAIN (field);
    }

  if (flags & MIictor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sictor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIunitTest)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->stest));
      field = TREE_CHAIN (field);
    }

  if (flags & MIimportedModules)
    {
      vec<constructor_elt, va_gc> *elms = NULL;
      tree satype = d_array_type (Type::tvoidptr, aimports_dim);
      size_t idx = 0;

      for (size_t i = 0; i < m->aimports.dim; i++)
	{
	  Module *mi = m->aimports[i];
	  if (mi->needmoduleinfo)
	    {
	      CONSTRUCTOR_APPEND_ELT (elms, size_int (idx),
				      build_address (get_moduleinfo_decl (mi)));
	      idx++;
	    }
	}

      CONSTRUCTOR_APPEND_ELT (minit, field, size_int (aimports_dim));
      field = TREE_CHAIN (field);
      CONSTRUCTOR_APPEND_ELT (minit, field, build_constructor (satype, elms));
      field = TREE_CHAIN (field);
    }

  if (flags & MIlocalClasses)
    {
      vec<constructor_elt, va_gc> *elms = NULL;
      tree satype = d_array_type (Type::tvoidptr, aclasses.dim);

      for (size_t i = 0; i < aclasses.dim; i++)
	{
	  ClassDeclaration *cd = aclasses[i];
	  CONSTRUCTOR_APPEND_ELT (elms, size_int (i),
				  build_address (get_classinfo_decl (cd)));
	}

      CONSTRUCTOR_APPEND_ELT (minit, field, size_int (aclasses.dim));
      field = TREE_CHAIN (field);
      CONSTRUCTOR_APPEND_ELT (minit, field, build_constructor (satype, elms));
      field = TREE_CHAIN (field);
    }

  if (flags & MIname)
    {
      // Put out module name as a 0-terminated C-string, to save bytes
      const char *name = m->toPrettyChars ();
      size_t namelen = strlen (name) + 1;
      tree strtree = build_string (namelen, name);
      TREE_TYPE (strtree) = d_array_type (Type::tchar, namelen);
      CONSTRUCTOR_APPEND_ELT (minit, field, strtree);
      field = TREE_CHAIN (field);
    }

  gcc_assert (field == NULL_TREE);

  TREE_TYPE (decl) = type;
  DECL_INITIAL (decl) = build_struct_literal (type, minit);
  d_finish_symbol (decl);

  return decl;
}

void
d_finish_module()
{
  /* If the target does not directly support static constructors,
     static_ctor_list contains a list of all static constructors defined
     so far.  This routine will create a function to call all of those
     and is picked up by collect2. */
  const char *ident;

  if (!static_ctor_list.is_empty())
    {
      ident = IDENTIFIER_POINTER (get_file_function_name ("I"));
      build_call_function (ident, static_ctor_list, true);
    }

  if (!static_dtor_list.is_empty())
    {
      ident = IDENTIFIER_POINTER (get_file_function_name ("D"));
      build_call_function (ident, static_dtor_list, true);
    }
}

location_t
get_linemap (const Loc& loc)
{
  location_t gcc_location;

  linemap_add (line_table, LC_ENTER, 0, loc.filename, loc.linnum);
  linemap_line_start (line_table, loc.linnum, 0);
  gcc_location = linemap_position_for_column (line_table, loc.charnum);
  linemap_add (line_table, LC_LEAVE, 0, NULL, 0);

  return gcc_location;
}

// Update input_location to LOC.

void
set_input_location (const Loc& loc)
{
  if (loc.filename)
    input_location = get_linemap (loc);
}

// Some D Declarations don't have the loc set, this searches DECL's parents
// until a valid loc is found.

void
set_input_location (Dsymbol *decl)
{
  Dsymbol *dsym = decl;
  while (dsym)
    {
      if (dsym->loc.filename)
	{
	  set_input_location (dsym->loc);
	  return;
	}
      dsym = dsym->toParent();
    }

  // Fallback; backend sometimes crashes if not set
  Module *mod = decl->getModule();
  Loc loc;

  if (mod && mod->srcfile && mod->srcfile->name)
    loc.filename = mod->srcfile->name->str;
  else
    // Empty string can mess up debug info
    loc.filename = "<no_file>";

  loc.linnum = 1;
  set_input_location (loc);
}

// Like set_input_location, but sets the location on decl T.

void
set_decl_location (tree t, const Loc& loc)
{
  // DWARF2 will often crash if the DECL_SOURCE_FILE is not set.
  // It's easier the error here.
  gcc_assert (loc.filename);
  DECL_SOURCE_LOCATION (t) = get_linemap (loc);
}

void
set_decl_location (tree t, Dsymbol *decl)
{
  Dsymbol *dsym = decl;
  while (dsym)
    {
      if (dsym->loc.filename)
	{
	  set_decl_location (t, dsym->loc);
	  return;
	}
      dsym = dsym->toParent();
    }

  // Fallback; backend sometimes crashes if not set
  Module *mod = decl->getModule();
  Loc loc;

  if (mod && mod->srcfile && mod->srcfile->name)
    loc.filename = mod->srcfile->name->str;
  else
    // Empty string can mess up debug info
    loc.filename = "<no_file>";

  loc.linnum = 1;
  set_decl_location (t, loc);
}

void
set_function_end_locus (const Loc& loc)
{
  if (loc.filename)
    cfun->function_end_locus = get_linemap (loc);
  else
    cfun->function_end_locus = DECL_SOURCE_LOCATION (cfun->decl);
}

// Return the COMDAT group into which DECL should be placed.

static tree
d_comdat_group(tree decl)
{
  // If already part of a comdat group, use that.
  if (DECL_COMDAT_GROUP (decl))
    return DECL_COMDAT_GROUP (decl);

  return DECL_ASSEMBLER_NAME (decl);
}

// Set DECL up to have the closest approximation of "initialized common"
// linkage available.

void
d_comdat_linkage(tree decl)
{
  // Weak definitions have to be public.
  if (!TREE_PUBLIC (decl))
    return;

  // Necessary to allow DECL_ONE_ONLY or DECL_WEAK functions to be inlined
  if (TREE_CODE (decl) == FUNCTION_DECL)
    DECL_DECLARED_INLINE_P (decl) = 1;

  // The following makes assumptions about the behavior of make_decl_one_only.
  if (SUPPORTS_ONE_ONLY)
    make_decl_one_only(decl, d_comdat_group(decl));
  else if (SUPPORTS_WEAK)
    {
      tree decl_init = DECL_INITIAL (decl);
      DECL_INITIAL (decl) = integer_zero_node;
      make_decl_one_only(decl, d_comdat_group(decl));
      DECL_INITIAL (decl) = decl_init;
    }
  else if (TREE_CODE (decl) == FUNCTION_DECL
	   || (VAR_P (decl) && DECL_ARTIFICIAL (decl)))
    // We can just emit function and compiler-generated variables
    // statically; having multiple copies is (for the most part) only
    // a waste of space.
    TREE_PUBLIC (decl) = 0;
  else if (DECL_INITIAL (decl) == NULL_TREE
	   || DECL_INITIAL (decl) == error_mark_node)
    // Fallback, cannot have multiple copies.
    DECL_COMMON (decl) = 1;

  if (TREE_PUBLIC (decl))
    DECL_COMDAT (decl) = 1;
}


// Mark DECL, which is a VAR_DECL or FUNCTION_DECL as a symbol that
// must be emitted in this, output module.

static void
mark_needed (tree decl)
{
  TREE_USED (decl) = 1;

  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      struct cgraph_node *node = cgraph_node::get_create (decl);
      node->forced_by_abi = true;
    }
  else if (VAR_P (decl))
    {
      struct varpool_node *node = varpool_node::get_create (decl);
      node->forced_by_abi = true;
    }
}

// Finish up a symbol declaration and compile it all the way to
// the assembler language output.

void
d_finish_symbol (tree decl)
{
  gcc_assert (!error_operand_p (decl));

  // We are sending this symbol to object file, can't be extern.
  TREE_STATIC (decl) = 1;
  DECL_EXTERNAL (decl) = 0;
  relayout_decl (decl);

#ifdef ENABLE_TREE_CHECKING
  if (DECL_INITIAL (decl) != NULL_TREE)
    {
      // Initialiser must never be bigger than symbol size.
      dinteger_t tsize = int_size_in_bytes (TREE_TYPE (decl));
      dinteger_t dtsize = int_size_in_bytes (TREE_TYPE (DECL_INITIAL (decl)));

      if (tsize < dtsize)
	{
	  tree name = DECL_ASSEMBLER_NAME (decl);

	  internal_error ("Mismatch between declaration '%qE' size (%wd) and "
			  "it's initializer size (%wd).",
			  IDENTIFIER_PRETTY_NAME (name)
			  ? IDENTIFIER_PRETTY_NAME (name) : name,
			  tsize, dtsize);
	}
    }
#endif

  // %% FIXME: DECL_COMMON so the symbol goes in .tcommon
  if (DECL_THREAD_LOCAL_P (decl)
      && DECL_ASSEMBLER_NAME (decl) == get_identifier ("_tlsend"))
    {
      DECL_INITIAL (decl) = NULL_TREE;
      DECL_COMMON (decl) = 1;
    }

  /* Add this decl to the current binding level.  */
  d_pushdecl (decl);

  rest_of_decl_compilation (decl, 1, 0);
}

void
d_finish_function(FuncDeclaration *fd)
{
  tree decl = get_symbol_decl (fd);

  gcc_assert(TREE_CODE (decl) == FUNCTION_DECL);

  // If we generated the function, but it's really extern.
  // Such as external inlinable functions or thunk aliases.
  if (!fd->isInstantiated() && fd->getModule() && !fd->getModule()->isRoot())
    {
      TREE_STATIC (decl) = 0;
      DECL_EXTERNAL (decl) = 1;
    }
  else if (DECL_SAVED_TREE (decl) != NULL_TREE)
    {
      TREE_STATIC (decl) = 1;
      DECL_EXTERNAL (decl) = 0;
    }

  if (!targetm.have_ctors_dtors)
    {
      if (DECL_STATIC_CONSTRUCTOR (decl))
	static_ctor_list.safe_push(fd);
      if (DECL_STATIC_DESTRUCTOR (decl))
	static_dtor_list.safe_push(fd);
    }

  cgraph_node::finalize_function(decl, true);
}

// Wrapup all global declarations and start the final compilation.

void
d_finish_compilation(tree *vec, int len)
{
  // Complete all generated thunks.
  symtab->process_same_body_aliases();

  // Process all file scopes in this compilation, and the external_scope,
  // through wrapup_global_declarations.
  for (int i = 0; i < len; i++)
    {
      tree decl = vec[i];
      wrapup_global_declarations(&decl, 1);

      // We want the static symbol to be written.
      if ((VAR_P (decl) && TREE_STATIC (decl))
	  || TREE_CODE (decl) == FUNCTION_DECL)
	mark_needed(decl);
    }
}

// Return an anonymous static variable of type TYPE, initialized with
// INIT, and optionally prefixing the name with PREFIX.
// At some point the INIT constant should be hashed to remove duplicates.

tree
build_artificial_decl(tree type, tree init, const char *prefix)
{
  tree decl = build_decl(UNKNOWN_LOCATION, VAR_DECL, NULL_TREE, type);
  const char *name = prefix ? prefix : "___s";
  char *label;

  ASM_FORMAT_PRIVATE_NAME (label, name, DECL_UID (decl));
  SET_DECL_ASSEMBLER_NAME (decl, get_identifier(label));
  DECL_NAME (decl) = DECL_ASSEMBLER_NAME (decl);

  TREE_PUBLIC (decl) = 0;
  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;
  DECL_IGNORED_P (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;

  DECL_INITIAL (decl) = init;

  return decl;
}

// Build TYPE_DECL for the declaration DSYM.

void
build_type_decl (tree type, Dsymbol *dsym)
{
  if (TYPE_STUB_DECL (type))
    return;

  gcc_assert(!POINTER_TYPE_P (type));

  tree decl = build_decl(UNKNOWN_LOCATION, TYPE_DECL,
			 get_identifier(dsym->ident->toChars()), type);
  DECL_ARTIFICIAL (decl) = 1;

  DECL_CONTEXT (decl) = d_decl_context(dsym);
  set_decl_location(decl, dsym);

  TYPE_CONTEXT (type) = DECL_CONTEXT (decl);
  TYPE_NAME (type) = decl;

  /* Not sure if there is a need for separate TYPE_DECLs in
     TYPE_NAME and TYPE_STUB_DECL. */
  if (TREE_CODE (type) == ENUMERAL_TYPE || RECORD_OR_UNION_TYPE_P (type))
    TYPE_STUB_DECL (type) = decl;

  rest_of_decl_compilation(decl, SCOPE_FILE_SCOPE_P (decl), 0);
}

// Build but do not emit a function named NAME, whose function body is in EXPR.

static FuncDeclaration *
build_simple_function_decl (const char *name, tree expr)
{
  Module *mod = current_module_decl;

  if (!mod)
    mod = Module::rootModule;

  if (name[0] == '*')
    {
      tree s = make_internal_name (mod, name + 1, "FZv");
      name = IDENTIFIER_POINTER (s);
    }

  TypeFunction *func_type = TypeFunction::create (0, Type::tvoid, 0, LINKc);
  FuncDeclaration *func = new FuncDeclaration (mod->loc, mod->loc,
					       Identifier::idPool (name), STCstatic, func_type);
  func->loc = Loc(mod->srcfile->toChars(), 1, 0);
  func->linkage = func_type->linkage;
  func->parent = mod;
  func->protection = PROTprivate;
  func->semanticRun = PASSsemantic3done;

  // %% Maybe remove the identifier
  WrappedExp *body = new WrappedExp (mod->loc, expr, Type::tvoid);
  func->fbody = ExpStatement::create (mod->loc, body);

  return func;
}

// Build and emit a function named NAME, whose function body is in EXPR.

static FuncDeclaration *
build_simple_function (const char *name, tree expr, bool static_ctor)
{
  FuncDeclaration *func = build_simple_function_decl (name, expr);
  tree func_decl = get_symbol_decl (func);

  if (static_ctor)
    DECL_STATIC_CONSTRUCTOR (func_decl) = 1;

  // D static ctors, dtors, unittests, and the ModuleInfo chain function
  // are always private (see setup_symbol_storage, default case)
  TREE_PUBLIC (func_decl) = 0;
  TREE_USED (func_decl) = 1;

  build_decl_tree (func);

  return func;
}

// Build and emit a function identified by NAME that calls (in order)
// the list of functions in FUNCTIONS.  If FORCE_P, create a new function
// even if there is only one function to call in the list.

static FuncDeclaration *
build_call_function (const char *name, vec<FuncDeclaration *> functions, bool force_p)
{
  tree expr_list = NULL_TREE;

  // If there is only one function, just return that
  if (functions.length() == 1 && !force_p)
    return functions[0];

  Module *mod = current_module_decl;
  if (!mod)
    mod = Module::rootModule;
  set_input_location(Loc(mod->srcfile->toChars(), 1, 0));

  // Shouldn't front end build these?
  for (size_t i = 0; i < functions.length(); i++)
    {
      tree fndecl = get_symbol_decl (functions[i]);
      tree call_expr = d_build_call_list (void_type_node, build_address (fndecl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    return build_simple_function (name, expr_list, false);

  return NULL;
}

// Build and emit a function that takes a scope
// delegate parameter and calls it once for every TLS variable in the
// module.

static tree
build_emutls_function (vec<VarDeclaration *> tlsVars)
{
  Module *mod = current_module_decl;

  if (!mod)
    mod = Module::rootModule;

  const char *name = "__modtlsscan";

  // void __modtlsscan(scope void delegate(void*, void*) nothrow dg) nothrow
  // {
  //     dg(&$tlsVars[0]$, &$tlsVars[0]$ + $tlsVars[0]$.sizeof);
  //     dg(&$tlsVars[1]$, &$tlsVars[1]$ + $tlsVars[0]$.sizeof);
  //     ...
  // }

  Parameters *del_args = new Parameters();
  del_args->push (Parameter::create (0, Type::tvoidptr, NULL, NULL));
  del_args->push (Parameter::create (0, Type::tvoidptr, NULL, NULL));

  TypeFunction *del_func_type = TypeFunction::create (del_args, Type::tvoid, 0,
						      LINKd, STCnothrow);
  Parameters *args = new Parameters();
  Parameter *dg_arg = Parameter::create (STCscope, new TypeDelegate (del_func_type),
					 Identifier::idPool ("dg"), NULL);
  args->push (dg_arg);
  TypeFunction *func_type = TypeFunction::create (args, Type::tvoid, 0, LINKd,
						  STCnothrow);
  FuncDeclaration *func = new FuncDeclaration (mod->loc, mod->loc,
					       Identifier::idPool (name), STCstatic, func_type);
  func->loc = Loc(mod->srcfile->toChars(), 1, 0);
  func->linkage = func_type->linkage;
  func->parent = mod;
  func->protection = PROTprivate;

  func->semantic (mod->_scope);
  Statements *body = new Statements();
  for (size_t i = 0; i < tlsVars.length(); i++)
    {
      VarDeclaration *var = tlsVars[i];
      Expression *addr = (VarExp::create (mod->loc, var))->addressOf();
      Expression *addr2 = new SymOffExp (mod->loc, var, var->type->size());
      Expressions* addrs = new Expressions();
      addrs->push (addr);
      addrs->push (addr2);

      Expression *call = CallExp::create (mod->loc, IdentifierExp::create (Loc (), dg_arg->ident), addrs);
      body->push (ExpStatement::create (mod->loc, call));
    }
  func->fbody = new CompoundStatement (mod->loc, body);
  func->semantic3 (mod->_scope);
  build_decl_tree (func);

  return get_symbol_decl (func);
}

// Same as build_call_function, but includes a gate to
// protect static ctors in templates getting called multiple times.

static tree
build_ctor_function (const char *name, vec<FuncDeclaration *> functions, vec<VarDeclaration *> gates)
{
  tree expr_list = NULL_TREE;

  // If there is only one function, just return that
  if (functions.length() == 1 && gates.is_empty())
    return get_symbol_decl (functions[0]);

  // Increment gates first.
  for (size_t i = 0; i < gates.length(); i++)
    {
      tree var_decl = get_symbol_decl (gates[i]);
      tree value = build2 (PLUS_EXPR, TREE_TYPE (var_decl), var_decl, integer_one_node);
      tree var_expr = modify_expr (var_decl, value);
      expr_list = compound_expr (expr_list, var_expr);
    }

  // Call Ctor Functions
  for (size_t i = 0; i < functions.length(); i++)
    {
      tree fndecl = get_symbol_decl (functions[i]);
      tree call_expr = d_build_call_list (void_type_node, build_address (fndecl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    {
      FuncDeclaration *fd = build_simple_function (name, expr_list, false);
      return get_symbol_decl (fd);
    }

  return NULL;
}

// Same as build_call_function, but calls all functions in
// the reverse order that the constructors were called in.

static tree
build_dtor_function (const char *name, vec<FuncDeclaration *> functions)
{
  tree expr_list = NULL_TREE;

  // If there is only one function, just return that
  if (functions.length() == 1)
    return get_symbol_decl (functions[0]);

  for (int i = functions.length() - 1; i >= 0; i--)
    {
      tree fndecl = get_symbol_decl (functions[i]);
      tree call_expr = d_build_call_list (void_type_node, build_address (fndecl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    {
      FuncDeclaration *fd = build_simple_function (name, expr_list, false);
      return get_symbol_decl (fd);
    }

  return NULL;
}

// Same as build_call_function, but returns the Symbol to
// the function generated.

static tree
build_unittest_function (const char *name, vec<FuncDeclaration *> functions)
{
  FuncDeclaration *fd = build_call_function (name, functions, false);
  return get_symbol_decl (fd);
}

// Build a variable used in the dso_registry code. The variable is always
// visibility = hidden and TREE_PUBLIC. Optionally sets an initializer, makes
// the variable external and/or comdat.

static tree
build_dso_registry_var(const char * name, tree type, tree init, bool comdat_p,
  bool external_p)
{
  tree var = build_decl(BUILTINS_LOCATION, VAR_DECL, get_identifier(name), type);
  set_decl_location(var, current_module_decl);
  DECL_VISIBILITY (var) = VISIBILITY_HIDDEN;
  DECL_VISIBILITY_SPECIFIED (var) = 1;
  TREE_PUBLIC (var) = 1;

  if (external_p)
    DECL_EXTERNAL (var) = 1;
  else
    TREE_STATIC (var) = 1;

  if (init != NULL_TREE)
    DECL_INITIAL (var) = init;
  if (comdat_p)
    d_comdat_linkage (var);

  rest_of_decl_compilation(var, 1, 0);
  return var;
}

// Build and emit a constructor or destructor calling dso_registry_func if
// dso_initialized is (false in a constructor or true in a destructor).

static void
emit_dso_registry_cdtor(Dsymbol *compiler_dso_type, Dsymbol *dso_registry_func,
  tree dso_initialized, tree dso_slot, bool ctor_p)
{
  const char* func_name = "gdc_dso_dtor";
  tree init_condition = boolean_false_node;
  if (ctor_p)
    {
      func_name = "gdc_dso_ctor";
      init_condition = boolean_true_node;
    }

  // extern extern(C) void* __start_minfo @hidden;
  // extern extern(C) void* __stop_minfo @hidden;
  tree start_minfo = build_dso_registry_var("__start_minfo", ptr_type_node,
    NULL_TREE, false, true);
  tree stop_minfo = build_dso_registry_var("__stop_minfo", ptr_type_node,
    NULL_TREE, false, true);

  tree dso_type = build_ctype(compiler_dso_type->isStructDeclaration()->type);
  tree registry_func = get_symbol_decl (dso_registry_func->isFuncDeclaration());

  // dso = {1, &dsoSlot, &__start_minfo, &__stop_minfo};
  vec<constructor_elt, va_gc> *ve = NULL;
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field(dso_type,
						   get_identifier("_version")),
			  build_int_cst(size_type_node, 1));
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field(dso_type,
						   get_identifier("_slot")),
			  build_address(dso_slot));
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field(dso_type,
						   get_identifier("_minfo_beg")),
			  build_address(start_minfo));
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field(dso_type,
						   get_identifier("_minfo_end")),
			  build_address(stop_minfo));
  tree dso_data = build_decl(BUILTINS_LOCATION, VAR_DECL,
			     get_identifier("dso"), dso_type);
  set_decl_location(dso_data, current_module_decl);
  tree set_dso_expr = modify_expr (dso_data, build_struct_literal(dso_type, ve));

  rest_of_decl_compilation(dso_data, 1, 0);

  // if (!(gdc_dso_initialized == %init_condition))
  // {
  //     gdc_dso_initialized = %init_condition;
  //     CompilerDSOData dso = {1, &dsoSlot, &__start_minfo, &__stop_minfo};
  //     _d_dso_registry(&dso);
  // }
  tree condition = build_boolop(NE_EXPR, dso_initialized, init_condition);
  tree assign_expr = modify_expr(dso_initialized, init_condition);
  assign_expr = compound_expr (set_dso_expr, assign_expr);
  tree call_expr = d_build_call_nary(registry_func, 1,
    build_address(dso_data));
  tree func_body = build_vcondition(condition, compound_expr(assign_expr,
    call_expr), void_node);

  // extern(C) void gdc_dso_[c/d]tor() @hidden @weak @[con/de]structor
  FuncDeclaration *func_decl = build_simple_function_decl(func_name, func_body);
  tree func_tree = get_symbol_decl (func_decl);

  // Setup bindings for stack variable dso_data
  tree block = make_node(BLOCK);
  BLOCK_VARS (block) = dso_data;
  TREE_CHAIN (dso_data) = NULL_TREE;
  DECL_CONTEXT (dso_data) = func_tree;
  func_body = build3 (BIND_EXPR, void_type_node, BLOCK_VARS (block), func_body, block);
  ((WrappedExp *)((ExpStatement *)func_decl->fbody)->exp)->e1 = func_body;

  set_decl_location(func_tree, current_module_decl);
  TREE_PUBLIC (func_tree) = 1;
  DECL_VISIBILITY (func_tree) = VISIBILITY_HIDDEN;
  DECL_VISIBILITY_SPECIFIED (func_tree) = 1;

  if (ctor_p)
    DECL_STATIC_CONSTRUCTOR (func_tree) = 1;
  else
    DECL_STATIC_DESTRUCTOR (func_tree) = 1;

  d_comdat_linkage(func_tree);
  build_decl_tree (func_decl);
}

// Build and emit the helper functions for the DSO registry code, including
// the gdc_dso_slot and gdc_dso_initialized variable and the gdc_dso_ctor
// and gdc_dso_dtor functions.

static void
emit_dso_registry_helpers(Dsymbol *compiler_dso_type, Dsymbol *dso_registry_func)
{
  // extern(C) void* gdc_dso_slot @hidden @weak = null;
  // extern(C) bool gdc_dso_initialized @hidden @weak = false;
  tree dso_slot = build_dso_registry_var("gdc_dso_slot", ptr_type_node,
    null_pointer_node, true, false);
  tree dso_initialized = build_dso_registry_var("gdc_dso_initialized", boolean_type_node,
    boolean_false_node, true, false);

  // extern(C) void gdc_dso_ctor() @hidden @weak @ctor
  // extern(C) void gdc_dso_dtor() @hidden @weak @dtor
  emit_dso_registry_cdtor(compiler_dso_type, dso_registry_func, dso_initialized,
    dso_slot, true);
  emit_dso_registry_cdtor(compiler_dso_type, dso_registry_func, dso_initialized,
    dso_slot, false);
}

// Emit code to place sym into the minfo list. Also emit helpers to call
// _d_dso_registry if necessary.

static void
emit_dso_registry_hooks(tree sym, Dsymbol *compiler_dso_type, Dsymbol *dso_registry_func)
{
  gcc_assert(targetm_common.have_named_sections);

  // Step 1: Place the ModuleInfo into the minfo section.
  // Do this once for every emitted Module
  // @section("minfo") void* __mod_ref_%s = &ModuleInfo(module);
  const char *name = concat ("__mod_ref_", IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (sym)), NULL);
  tree decl = build_decl(BUILTINS_LOCATION, VAR_DECL, get_identifier (name), ptr_type_node);
  d_keep(decl);
  set_decl_location(decl, current_module_decl);
  TREE_PUBLIC (decl) = 1;
  TREE_STATIC (decl) = 1;
  DECL_PRESERVE_P (decl) = 1;
  DECL_INITIAL (decl) = build_address(sym);
  TREE_STATIC (DECL_INITIAL (decl)) = 1;

  // Do not start section with '.' to use the __start_ feature:
  // https://sourceware.org/binutils/docs-2.26/ld/Orphan-Sections.html
  set_decl_section_name(decl, "minfo");
  rest_of_decl_compilation(decl, 1, 0);

  // Step 2: Emit helper functions. These are only required once per
  // shared library, so it's safe to emit them only one per object file.
  static bool emitted = false;
  if (!emitted)
    {
      emitted = true;
      emit_dso_registry_helpers(compiler_dso_type, dso_registry_func);
    }
}

// Emit code to add sym to the _Dmodule_ref linked list.

static void
emit_modref_hooks(tree sym, Dsymbol *mref)
{
  // Generate:
  //  struct ModuleReference
  //  {
  //    void *next;
  //    void* mod;
  //  }

  // struct ModuleReference in moduleinit.d
  tree tmodref = build_two_field_type (ptr_type_node, ptr_type_node,
				       NULL, "next", "mod");
  tree nextfield = TYPE_FIELDS (tmodref);
  tree modfield = TREE_CHAIN (nextfield);

  // extern (C) ModuleReference *_Dmodule_ref;
  tree dmodule_ref = get_symbol_decl (mref->isDeclaration ());

  // private ModuleReference modref = { next: null, mod: _ModuleInfo_xxx };
  tree modref = build_artificial_decl (tmodref, NULL_TREE, "__mod_ref");
  set_decl_location (modref, current_module_decl);
  d_keep (modref);

  vec<constructor_elt, va_gc> *ce = NULL;
  CONSTRUCTOR_APPEND_ELT (ce, nextfield, null_pointer_node);
  CONSTRUCTOR_APPEND_ELT (ce, modfield, build_address (sym));

  DECL_INITIAL (modref) = build_constructor (tmodref, ce);
  TREE_STATIC (DECL_INITIAL (modref)) = 1;
  d_pushdecl (modref);
  rest_of_decl_compilation (modref, 1, 0);

  // Generate:
  //  void ___modinit()  // a static constructor
  //  {
  //    modref.next = _Dmodule_ref;
  //    _Dmodule_ref = &modref;
  //  }
  tree m1 = modify_expr (component_ref (modref, nextfield), dmodule_ref);
  tree m2 = modify_expr (dmodule_ref, build_address (modref));

  build_simple_function ("*__modinit", compound_expr (m1, m2), true);
}

/* Output the ModuleInfo for module M and register it with druntime.  */

void
layout_moduleinfo (Module *m)
{
  /* Load the rt.sections module and retrieve the internal DSO/ModuleInfo
     types, ignoring any errors as a result of missing files.  */
  unsigned errors = global.startGagging ();
  Module *sections = Module::load (Loc (), NULL, Id::sectionsModule);
  global.endGagging (errors);

  if (sections == NULL)
    return;

  sections->importedFrom = sections;
  sections->importAll (NULL);
  sections->semantic (NULL);
  sections->semantic2 (NULL);
  sections->semantic3 (NULL);

  /* These symbols are normally private to the module they're declared in,
     but for internal compiler lookups, their visibility is discarded.  */
  int sflags = IgnoreErrors | IgnoreSymbolVisibility;

  /* Try to find the required types and functions in druntime.  */
  Dsymbol *mref = sections->search (Loc (), Id::Dmodule_ref, sflags);
  Dsymbol *dso_type = sections->search (Loc (), Id::compiler_dso_type, sflags);
  Dsymbol *dso_func = sections->search (Loc (), Id::dso_registry_func, sflags);

  /* Prefer _d_dso_registry model if available.  */
  if (dso_type != NULL && dso_func != NULL)
    {
      Symbol* sym = build_moduleinfo_symbol (m);
      emit_dso_registry_hooks (sym, dso_type, dso_func);
    }
  else if (mref != NULL)
    {
      Symbol* sym = build_moduleinfo_symbol (m);
      emit_modref_hooks (sym, mref);
    }
}
