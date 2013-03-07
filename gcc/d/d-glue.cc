// d-glue.cc -- D frontend for GCC.
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

// from DMD
#include "id.h"
#include "enum.h"
#include "module.h"
#include "init.h"
#include "symbol.h"
#include "d-lang.h"
#include "d-codegen.h"


static void
d_genericize (tree fndecl)
{
  FILE *dump_file;
  int local_dump_flags;

  // Build cgraph for function.
  cgraph_get_create_node (fndecl);

  // Set original decl context back to true context
  if (D_DECL_STATIC_CHAIN (fndecl))
    {
      struct lang_decl *d = DECL_LANG_SPECIFIC (fndecl);
      DECL_CONTEXT (fndecl) = d->d_decl->toSymbol()->ScontextDecl;
    }

  /* Dump the C-specific tree IR.  */
  dump_file = dump_begin (TDI_original, &local_dump_flags);
  if (dump_file)
    {
      fprintf (dump_file, "\n;; Function %s",
	       lang_hooks.decl_printable_name (fndecl, 2));
      fprintf (dump_file, " (%s)\n",
	       (!DECL_ASSEMBLER_NAME_SET_P (fndecl) ? "null"
		: IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl))));
      fprintf (dump_file, ";; enabled by -%s\n", dump_flag_name (TDI_original));
      fprintf (dump_file, "\n");

      if (local_dump_flags & TDF_RAW)
	dump_node (DECL_SAVED_TREE (fndecl),
		   TDF_SLIM | local_dump_flags, dump_file);
      else
	print_generic_expr (dump_file, DECL_SAVED_TREE (fndecl), local_dump_flags);
      fprintf (dump_file, "\n");

      dump_end (TDI_original, dump_file);
    }
}


void
FuncDeclaration::toObjFile (int)
{
  if (!global.params.useUnitTests && isUnitTestDeclaration())
    return;

  if (!g.ofile->shouldEmit (this))
    return;

  Symbol *this_sym = toSymbol();
  if (this_sym->outputStage)
    return;

  this_sym->outputStage = InProgress;

  tree fn_decl = this_sym->Stree;

  if (!fbody)
    {
      if (!isNested())
	{
	  // %% Should set this earlier...
	  DECL_EXTERNAL (fn_decl) = 1;
	  TREE_PUBLIC (fn_decl) = 1;
	}
      rest_of_decl_compilation (fn_decl, 1, 0);
      return;
    }

  if (global.params.verbose)
    fprintf (stdmsg, "function  %s\n", this->toPrettyChars());

  IRState *irs = g.irs->startFunction (this);

  irs->useChain (NULL, NULL_TREE);
  tree chain_expr = NULL;
  FuncDeclaration *chain_func = NULL;

  // in 4.0, doesn't use push_function_context
  tree old_current_function_decl = current_function_decl;
  function *old_cfun = cfun;
  current_function_decl = fn_decl;

  tree return_type = TREE_TYPE (TREE_TYPE (fn_decl));
  tree result_decl = build_decl (UNKNOWN_LOCATION, RESULT_DECL,
				 NULL_TREE, return_type);
  g.ofile->setDeclLoc (result_decl, this);
  DECL_RESULT (fn_decl) = result_decl;
  DECL_CONTEXT (result_decl) = fn_decl;
  DECL_ARTIFICIAL (result_decl) = 1;
  DECL_IGNORED_P (result_decl) = 1;

  allocate_struct_function (fn_decl, false);
  // assuming the above sets cfun
  g.ofile->setCfunEndLoc (endloc);

  // Add method to record for debug information.
  if (isThis())
    {
      AggregateDeclaration *ad = isThis();
      tree rec = ad->type->toCtype();

      if (ad->isClassDeclaration())
	rec = TREE_TYPE (rec);

      g.ofile->addAggMethod (rec, this);
    }

  tree parm_decl = NULL_TREE;
  tree param_list = NULL_TREE;

  // Special arguments...

  // 'this' parameter
  if (vthis)
    {
      parm_decl = vthis->toSymbol()->Stree;
      if (!isThis() && isNested())
	{
	  /* DMD still generates a vthis, but it should not be
	     referenced in any expression.
	   */
	  DECL_ARTIFICIAL (parm_decl) = 1;
	  chain_func = toParent2()->isFuncDeclaration();
	  chain_expr = parm_decl;
	}
      g.ofile->setDeclLoc (parm_decl, vthis);
      param_list = chainon (param_list, parm_decl);
    }

  // _arguments parameter.
  if (v_arguments)
    {
      parm_decl = v_arguments->toSymbol()->Stree;
      g.ofile->setDeclLoc (parm_decl, v_arguments);
      param_list = chainon (param_list, parm_decl);
    }

  // formal function parameters.
  size_t n_parameters = parameters ? parameters->dim : 0;

  for (size_t i = 0; i < n_parameters; i++)
    {
      VarDeclaration *param = (*parameters)[i];
      parm_decl = param->toSymbol()->Stree;
      g.ofile->setDeclLoc (parm_decl, (Dsymbol *) param);
      // chain them in the correct order
      param_list = chainon (param_list, parm_decl);
    }
  DECL_ARGUMENTS (fn_decl) = param_list;
  for (tree t = param_list; t; t = DECL_CHAIN (t))
    DECL_CONTEXT (t) = fn_decl;

  // http://www.tldp.org/HOWTO/GCC-Frontend-HOWTO-7.html
  rest_of_decl_compilation (fn_decl, /*toplevel*/1, /*atend*/0);

  DECL_INITIAL (fn_decl) = error_mark_node;

  pushlevel (0);
  irs->pushStatementList();

  irs->startScope();
  irs->doLineNote (loc);

  /* If this is a member function that nested (possibly indirectly) in another
     function, construct an expession for this member function's static chain
     by going through parent link of nested classes.
     */
  /* D 2.0 Closures: this->vthis is passed as a normal parameter and
     is valid to access as Stree before the closure frame is created. */
  if (isThis())
    {
      AggregateDeclaration *ad = isThis();
      tree this_tree = vthis->toSymbol()->Stree;
      while (ad->isNested())
	{
	  Dsymbol *d = ad->toParent2();
	  tree vthis_field = ad->vthis->toSymbol()->Stree;
	  this_tree = irs->component (irs->indirect (this_tree), vthis_field);

	  FuncDeclaration *fd = d->isFuncDeclaration();
	  ad = d->isAggregateDeclaration();
	  if (ad == NULL)
	    {
	      gcc_assert (fd != NULL);
	      chain_expr = this_tree;
	      chain_func = fd;
	      break;
	    }
	}
    }

  if (isNested() || chain_expr)
    irs->useParentChain();

  if (chain_expr)
    {
      if (!DECL_P (chain_expr))
	{
	  tree c = irs->localVar (ptr_type_node);
	  DECL_INITIAL (c) = chain_expr;
	  irs->expandDecl (c);
	  chain_expr = c;
	}
      irs->useChain (chain_func, chain_expr);
    }

  irs->buildChain (this); // may change irs->chainLink and irs->chainFunc
  DECL_LANG_SPECIFIC (fn_decl) = build_d_decl_lang_specific (this);

  if (vresult)
    irs->emitLocalVar (vresult, true);

  if (v_argptr)
    irs->pushStatementList();
  if (v_arguments_var)
    irs->emitLocalVar (v_arguments_var);

  fbody->toIR (irs);

  // Process all deferred nested functions.
  for (size_t i = 0; i < this_sym->deferredNestedFuncs.dim; ++i)
    {
      (this_sym->deferredNestedFuncs[i])->toObjFile (false);
    }

  if (v_argptr)
    {
      tree body = irs->popStatementList();
      tree var = irs->var (v_argptr);
      var = irs->addressOf (var);

      tree init_exp = irs->buildCall (builtin_decl_explicit (BUILT_IN_VA_START), 2, var, parm_decl);
      v_argptr->init = NULL; // VoidInitializer?
      irs->emitLocalVar (v_argptr, true);
      irs->addExp (init_exp);

      tree cleanup = irs->buildCall (builtin_decl_explicit (BUILT_IN_VA_END), 1, var);
      irs->addExp (build2 (TRY_FINALLY_EXPR, void_type_node, body, cleanup));
    }

  irs->endScope();

  DECL_SAVED_TREE (fn_decl) = irs->popStatementList();

  /* In tree-nested.c, init_tmp_var expects a statement list to come
     from somewhere.  popStatementList returns expressions when
     there is a single statement.  This code creates a statemnt list
     unconditionally because the DECL_SAVED_TREE will always be a
     BIND_EXPR. */
    {
      tree body = DECL_SAVED_TREE (fn_decl);
      tree t;

      gcc_assert (TREE_CODE (body) == BIND_EXPR);

      t = TREE_OPERAND (body, 1);
      if (TREE_CODE (t) != STATEMENT_LIST)
	{
	  tree sl = alloc_stmt_list();
	  append_to_statement_list_force (t, &sl);
	  TREE_OPERAND (body, 1) = sl;
	}
      else if (!STATEMENT_LIST_HEAD (t))
	{
	  /* For empty functions: Without this, there is a
	     segfault when inlined.  Seen on build=ppc-linux but
	     not others (why?). */
	  tree ret = build1 (RETURN_EXPR, void_type_node, NULL_TREE);
	  append_to_statement_list_force (ret, &t);
	}
    }
  //block = (*lang_hooks.decls.poplevel) (1, 0, 1);
  tree block = poplevel (1, 0, 1);

  DECL_INITIAL (fn_decl) = block; // %% redundant, see poplevel
  BLOCK_SUPERCONTEXT (DECL_INITIAL (fn_decl)) = fn_decl; // done in C, don't know effect

  if (!errorcount && !global.errors)
    d_genericize (fn_decl);

  this_sym->outputStage = Finished;
  if (!errorcount && !global.errors)
    g.ofile->outputFunction (this);

  current_function_decl = old_current_function_decl; // must come before endFunction
  set_cfun (old_cfun);

  irs->endFunction();
}

void
FuncDeclaration::buildClosure (IRState *irs)
{
  FuncFrameInfo *ffi = irs->getFrameInfo (this);
  gcc_assert (ffi->is_closure);

  if (!ffi->creates_frame)
    {
      if (ffi->static_chain)
	{
	  tree link = irs->chainLink();
	  irs->useChain (this, link);
	}
      return;
    }

  tree closure_rec_type = irs->buildFrameForFunction (this);
  gcc_assert(COMPLETE_TYPE_P (closure_rec_type));

  tree closure_ptr = irs->localVar (build_pointer_type (closure_rec_type));
  DECL_NAME (closure_ptr) = get_identifier ("__closptr");
  DECL_IGNORED_P (closure_ptr) = 0;

  tree arg = convert (Type::tsize_t->toCtype(),
		      TYPE_SIZE_UNIT (closure_rec_type));

  DECL_INITIAL (closure_ptr) =
    irs->nop (TREE_TYPE (closure_ptr),
	      irs->libCall (LIBCALL_ALLOCMEMORY, 1, &arg));
  irs->expandDecl (closure_ptr);

  // set the first entry to the parent closure, if any
  tree chain_link = irs->chainLink();

  if (chain_link != NULL_TREE)
    {
      tree chain_field = irs->component (irs->indirect (closure_ptr),
					 TYPE_FIELDS (closure_rec_type));
      tree chain_expr = irs->vmodify (chain_field, chain_link);
      irs->doExp (chain_expr);
    }

  // copy parameters that are referenced nonlocally
  for (size_t i = 0; i < closureVars.dim; i++)
    {
      VarDeclaration *v = closureVars[i];
      if (!v->isParameter())
	continue;

      Symbol *vsym = v->toSymbol();

      tree closure_field = irs->component (irs->indirect (closure_ptr), vsym->SframeField);
      tree closure_expr = irs->vmodify (closure_field, vsym->Stree);
      irs->doExp (closure_expr);
    }

  irs->useChain (this, closure_ptr);
}

void
Module::genobjfile (int multiobj)
{
  /* Normally would create an ObjFile here, but gcc is limited to one obj file
     per pass and there may be more than one module per obj file. */
  gcc_assert (g.ofile);

  g.ofile->beginModule (this);
  g.ofile->setupStaticStorage (this, toSymbol()->Stree);

  if (members)
    {
      for (size_t i = 0; i < members->dim; i++)
	{
	  Dsymbol *dsym = (*members)[i];
	  dsym->toObjFile (multiobj);
	}
    }

  // Always generate module info.
  if (1 || needModuleInfo())
    {
      ModuleInfo& mi = *g.ofile->moduleInfo;
      if (mi.ctors.dim || mi.ctorgates.dim)
	sctor = g.ofile->doCtorFunction ("*__modctor", &mi.ctors, &mi.ctorgates)->toSymbol();
      if (mi.dtors.dim)
	sdtor = g.ofile->doDtorFunction ("*__moddtor", &mi.dtors)->toSymbol();
      if (mi.sharedctors.dim || mi.sharedctorgates.dim)
	ssharedctor = g.ofile->doCtorFunction ("*__modsharedctor",
					       &mi.sharedctors, &mi.sharedctorgates)->toSymbol();
      if (mi.shareddtors.dim)
	sshareddtor = g.ofile->doDtorFunction ("*__modshareddtor", &mi.shareddtors)->toSymbol();
      if (mi.unitTests.dim)
	stest = g.ofile->doUnittestFunction ("*__modtest", &mi.unitTests)->toSymbol();

      genmoduleinfo();
    }

  g.ofile->endModule();
}

void
TypedefDeclaration::toDebug (void)
{
}

void
StructDeclaration::toDebug (void)
{
  tree ctype = type->toCtype();
  g.ofile->declareType (ctype, this);
  rest_of_type_compilation (ctype, /*toplevel*/1);
}

/* Create debug information for a ClassDeclaration's inheritance tree.
   Interfaces are not included. */
static tree
binfo_for (tree tgt_binfo, ClassDeclaration *cls)
{
  tree binfo = make_tree_binfo (1);
  TREE_TYPE (binfo) = TREE_TYPE (cls->type->toCtype()); // RECORD_TYPE, not REFERENCE_TYPE
  BINFO_INHERITANCE_CHAIN (binfo) = tgt_binfo;
  BINFO_OFFSET (binfo) = integer_zero_node;

  if (cls->baseClass)
    {
      BINFO_BASE_APPEND (binfo, binfo_for (binfo, cls->baseClass));
#ifdef BINFO_BASEACCESSES
#error update vector stuff
      tree prot_tree;

      BINFO_BASEACCESSES (binfo) = make_tree_vec (1);
      BaseClass *bc = (BaseClass *) cls->baseclasses->data[0];
      switch (bc->protection)
	{
	case PROTpublic:
	  prot_tree = access_public_node;
	  break;
	case PROTprotected:
	  prot_tree = access_protected_node;
	  break;
	case PROTprivate:
	  prot_tree = access_private_node;
	  break;
	default:
	  prot_tree = access_public_node;
	  break;
	}
      BINFO_BASEACCESS (binfo,0) = prot_tree;
#endif
    }

  return binfo;
}

/* Create debug information for an InterfaceDeclaration's inheritance
   tree.  In order to access all inherited methods in the debugger,
   the entire tree must be described.

   This function makes assumptions about inherface layout. */
static tree
intfc_binfo_for (tree tgt_binfo, ClassDeclaration *iface, unsigned& inout_offset)
{
  tree binfo = make_tree_binfo (iface->baseclasses->dim);

  TREE_TYPE (binfo) = TREE_TYPE (iface->type->toCtype()); // RECORD_TYPE, not REFERENCE_TYPE
  BINFO_INHERITANCE_CHAIN (binfo) = tgt_binfo;
  BINFO_OFFSET (binfo) = size_int (inout_offset * PTRSIZE);

  if (iface->baseclasses->dim)
    {
#ifdef BINFO_BASEACCESSES
      BINFO_BASEACCESSES (binfo) = make_tree_vec (iface->baseclasses->dim);
#endif
    }
  for (size_t i = 0; i < iface->baseclasses->dim; i++)
    {
      BaseClass *bc = iface->baseclasses->tdata()[i];

      if (i)
	inout_offset++;

      BINFO_BASE_APPEND (binfo, intfc_binfo_for (binfo, bc->base, inout_offset));
#ifdef BINFO_BASEACCESSES
      tree prot_tree;
      switch (bc->protection)
	{
	case PROTpublic:
	  prot_tree = access_public_node;
	  break;
	case PROTprotected:
	  prot_tree = access_protected_node;
	  break;
	case PROTprivate:
	  prot_tree = access_private_node;
	  break;
	default:
	  prot_tree = access_public_node;
	  break;
	}
      BINFO_BASEACCESS (binfo, i) = prot_tree;
#endif
    }

  return binfo;
}

void
ClassDeclaration::toDebug (void)
{
  /* Used to create BINFO even if debugging was off.  This was needed to keep
     references to inherited types. */
  tree rec_type = TREE_TYPE (type->toCtype());

  if (!isInterfaceDeclaration())
    TYPE_BINFO (rec_type) = binfo_for (NULL_TREE, this);
  else
    {
      unsigned offset = 0;
      TYPE_BINFO (rec_type) = intfc_binfo_for (NULL_TREE, this, offset);
    }

  g.ofile->declareType (rec_type, this);
}

void
EnumDeclaration::toDebug (void)
{
  TypeEnum *tc = (TypeEnum *)type;
  if (!tc->sym->defaultval || type->isZeroInit())
    return;

  tree ctype = type->toCtype();

  // %% For D2 - ctype is not necessarily enum, which doesn't
  // sit well with rotc.  Can call this on structs though.
  if (AGGREGATE_TYPE_P (ctype) || TREE_CODE (ctype) == ENUMERAL_TYPE)
    rest_of_type_compilation (ctype, /*toplevel*/1);
}


/* ===================================================================== */

/* Insert CV info into *p.
 * Returns:
 *      number of bytes written, or that would be written if p==NULL
 */

int
Dsymbol::cvMember (unsigned char *)
{
  return 0;
}

int
EnumDeclaration::cvMember (unsigned char *)
{
  return 0;
}

int
FuncDeclaration::cvMember (unsigned char *)
{
  return 0;
}

int
VarDeclaration::cvMember (unsigned char *)
{
  return 0;
}

int
TypedefDeclaration::cvMember (unsigned char *)
{
  return 0;
}

