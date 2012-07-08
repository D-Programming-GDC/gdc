// d-irstate.cc -- D frontend for GCC.
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
#include "d-irstate.h"

#include "init.h"
#include "symbol.h"

#include "d-codegen.h"

IRBase::IRBase (void)
{
  this->parent = 0;
  this->func = 0;
  this->volatileDepth = 0;
}

IRState *
IRBase::startFunction (FuncDeclaration *decl)
{
  IRState *new_irs = new IRState();
  new_irs->parent = g.irs;
  new_irs->func = decl;

  for (Dsymbol *p = decl->parent; p; p = p->parent)
    {
      if (p->isModule())
	{
	  new_irs->mod = p->isModule();
	  break;
	}
    }

  g.irs = (IRState *) new_irs;
  ModuleInfo & mi = *g.ofile->moduleInfo;

  if (decl->isSharedStaticCtorDeclaration())
    mi.sharedctors.push (decl);
  else if (decl->isStaticCtorDeclaration())
    mi.ctors.push (decl);
  else if (decl->isSharedStaticDtorDeclaration())
    {
      VarDeclaration *vgate;
      if ((vgate = decl->isSharedStaticDtorDeclaration()->vgate))
	mi.sharedctorgates.push (vgate);
      mi.shareddtors.push (decl);
    }
  else if (decl->isStaticDtorDeclaration())
    {
      VarDeclaration *vgate;
      if ((vgate = decl->isStaticDtorDeclaration()->vgate))
	mi.ctorgates.push (vgate);
      mi.dtors.push (decl);
    }
  else if (decl->isUnitTestDeclaration())
    mi.unitTests.push (decl);

  return new_irs;
}

void
IRBase::endFunction (void)
{
  gcc_assert (this->scopes.dim == 0);

  g.irs = (IRState *) this->parent;
}


void
IRBase::addExp (tree e)
{
  /* Need to check that this is actually an expression; it
     could be an integer constant (statement with no effect.)
     Maybe should filter those out anyway... */
  if (TREE_TYPE (e) && !VOID_TYPE_P (TREE_TYPE (e)))
    {
      if (warn_unused_value
	  && !TREE_NO_WARNING (e)
	  && !TREE_SIDE_EFFECTS (e))
	{
	  warning (OPT_Wunused_value, "statement has no effect");
	}
      e = build1 (CONVERT_EXPR, void_type_node, e);
    }

  // C doesn't do this for label_exprs %% why?
  if (EXPR_P (e) && ! EXPR_HAS_LOCATION (e))
    SET_EXPR_LOCATION (e, input_location);

  tree stmt_list = (tree) this->statementList.pop();
  append_to_statement_list_force (e, & stmt_list);
  this->statementList.push (stmt_list);
}


void
IRBase::pushStatementList (void)
{
  tree t = alloc_stmt_list();
  this->statementList.push (t);
  d_keep (t);
}

tree
IRBase::popStatementList (void)
{
  tree t = (tree) this->statementList.pop();

  // %% should gdc bother doing this?

  /* If the statement list is completely empty, just return it.  This is
     just as good small as build_empty_stmt, with the advantage that
     statement lists are merged when they appended to one another.  So
     using the STATEMENT_LIST avoids pathological buildup of EMPTY_STMT_P
     statements.  */
  if (TREE_SIDE_EFFECTS (t))
    {
      tree_stmt_iterator i = tsi_start (t);

      /* If the statement list contained exactly one statement, then
	 extract it immediately.  */
      if (tsi_one_before_end_p (i))
	{
	  tree u = tsi_stmt (i);
	  tsi_delink (&i);
	  free_stmt_list (t);
	  t = u;
	}
    }
  return t;
}


tree
IRBase::getLabelTree (LabelDsymbol *label)
{
  if (! label->statement)
    return NULL_TREE;

  if (! label->statement->lblock)
    {
      tree label_decl = build_decl (UNKNOWN_LOCATION, LABEL_DECL,
 				    get_identifier (label->ident->string), void_type_node);
      gcc_assert (this->func != 0);
      DECL_CONTEXT (label_decl) = getLocalContext();
      DECL_MODE (label_decl) = VOIDmode; // Not sure why or if this is needed
      TREE_USED (label_decl) = 1;
      // Not setting this doesn't seem to cause problems (unlike VAR_DECLs)
      if (label->statement->loc.filename)
	g.ofile->setDeclLoc (label_decl, label->statement->loc); // %% label->loc okay?
      label->statement->lblock = label_decl;
    }
  return label->statement->lblock;
}

Label *
IRBase::getLabelBlock (LabelDsymbol *label, Statement *from)
{
  Label *l = new Label();

  for (int i = this->loops.dim - 1; i >= 0; i--)
    {
      Flow *flow = this->loops[i];

      if (flow->kind != level_block &&
	  flow->kind != level_switch)
	{
	  l->block = flow->statement;
	  l->kind = flow->kind;
	  l->level = i + 1;
	  break;
	}
    }
  if (from)
    l->from = from;

  l->label = label;
  return l;
}


Flow *
IRBase::getLoopForLabel (Identifier *ident, bool want_continue)
{
  if (ident)
    {
      LabelStatement *lbl_stmt = this->func->searchLabel (ident)->statement;
      gcc_assert (lbl_stmt != 0);
      Statement *stmt = lbl_stmt->statement;
      ScopeStatement *scope_stmt = stmt->isScopeStatement();

      if (scope_stmt)
	stmt = scope_stmt->statement;

      for (int i = this->loops.dim - 1; i >= 0; i--)
	{
	  Flow *flow = this->loops[i];

	  if (flow->statement == stmt)
	    {
	      if (want_continue)
		gcc_assert (stmt->hasContinue());
	      return flow;
	    }
	}
      gcc_unreachable();
    }
  else
    {
      for (int i = this->loops.dim - 1; i >= 0; i--)
	{
	  Flow *flow = this->loops[i];

	  if ((! want_continue && flow->statement->hasBreak()) ||
	      flow->statement->hasContinue())
	    return flow;
	}
      gcc_unreachable();
    }
}


Flow *
IRBase::beginFlow (Statement *stmt)
{
  Flow *flow = new Flow (stmt);
  this->loops.push (flow);
  pushStatementList();
  return flow;
}

void
IRBase::endFlow (void)
{
  Flow *flow;

  gcc_assert (this->loops.dim);

  flow = (Flow *) this->loops.pop();
  if (flow->exitLabel)
    doLabel (flow->exitLabel);
  //%% delete flow;
}

void
IRBase::doLabel (tree t_label)
{
  /* Don't write out label unless it is marked as used by the frontend.
     This makes auto-vectorization possible in conditional loops.
     The only excemption to this is in LabelStatement::toIR, in which
     all computed labels are marked regardless.  */
  if (TREE_USED (t_label))
    addExp (build1 (LABEL_EXPR, void_type_node, t_label));
}


void
IRBase::startScope (void)
{
  unsigned *p_count = new unsigned;
  *p_count = 0;
  this->scopes.push (p_count);

  //printf ("%*sstartScope\n", this->scopes.dim, "");
  startBindings();
}

void
IRBase::endScope (void)
{
  unsigned *p_count;

  p_count = currentScope();

  //printf ("%*s  ending scope with %d left \n", this->scopes.dim, "", *p_count);
  while (*p_count)
    endBindings();
  this->scopes.pop();
  //printf ("%*sendScope\n", this->scopes.dim, "");
}


void
IRBase::startBindings (void)
{
  pushlevel (0);
  tree block = make_node (BLOCK);
  set_block (block);

  pushStatementList();

  ++(*currentScope());
  //printf ("%*s  start -> %d\n", this->scopes.dim, "", *currentScope());

}

void
IRBase::endBindings (void)
{
  // %%TODO: reversing list causes problems with inf loops in unshare_all_decls
  tree block = poplevel (1,0,0);

  tree t_body = popStatementList();
  addExp (build3 (BIND_EXPR, void_type_node,
		  BLOCK_VARS (block), t_body, block));

  // Because we used set_block, the popped level/block is not automatically recorded
  insert_block (block);

  --(*currentScope());
  gcc_assert (*(int *) currentScope() >= 0);
  //printf ("%*s  end -> %d\n", this->scopes.dim, "", *currentScope());

}

