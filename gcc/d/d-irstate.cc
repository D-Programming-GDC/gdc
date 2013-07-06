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

#include "d-system.h"
#include "d-lang.h"
#include "d-irstate.h"
#include "d-codegen.h"

extern "C" {
#include "init.h"
}//extern "C"

IRBase::IRBase (void)
{
  this->parent = NULL;
  this->func = NULL;
  this->varsInScope = NULL;
  this->mod = NULL;
  this->sthis = NULL_TREE;
}

IRState *
IRBase::startFunction (FuncDeclaration *decl)
{
  IRState *new_irs = new IRState();
  new_irs->parent = cirstate;
  new_irs->func = decl;

  for (Dsymbol *p = decl->parent; p; p = p->parent)
    {
      if (p->isModule())
	{
	  new_irs->mod = p->isModule();
	  break;
	}
    }

  cirstate = (IRState *) new_irs;
  ModuleInfo & mi = *object_file->moduleInfo;

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
  cirstate = (IRState *) this->parent;
}


// Emit statement E into function body.

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
	warning (OPT_Wunused_value, "statement has no effect");

      e = build1 (CONVERT_EXPR, void_type_node, e);
    }

  // C doesn't do this for label_exprs %% why?
  if (EXPR_P (e) && !EXPR_HAS_LOCATION (e))
    SET_EXPR_LOCATION (e, input_location);

  tree stmt_list = (tree) this->statementList.pop();
  append_to_statement_list_force (e, &stmt_list);
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
  if (!label->statement)
    return NULL_TREE;

  if (!label->statement->lblock)
    {
      tree label_decl = d_build_label (label->statement->loc, label->ident);
      TREE_USED (label_decl) = 1;
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

      if (flow->kind != level_block
	  && flow->kind != level_switch)
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

	  if ((!want_continue && flow->statement->hasBreak())
	      || flow->statement->hasContinue())
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
  startBindings();
}

void
IRBase::endScope (void)
{
  unsigned *p_count = currentScope();
  while (*p_count)
    endBindings();

  this->scopes.pop();
}


void
IRBase::startBindings (void)
{
  pushlevel (0);
  tree block = make_node (BLOCK);
  set_block (block);

  pushStatementList();

  ++(*currentScope());
}

void
IRBase::endBindings (void)
{
  tree block = poplevel (1,0,0);

  tree t_body = popStatementList();
  addExp (build3 (BIND_EXPR, void_type_node,
		  BLOCK_VARS (block), t_body, block));

  // Because we used set_block, the popped level/block is not automatically recorded
  insert_block (block);

  --(*currentScope());
  gcc_assert (*(int *) currentScope() >= 0);
}


// Routines for building statement lists around if/else conditions.
// STMT contains the statement to be executed if T_COND is true.

void
IRBase::startCond (Statement *stmt, tree t_cond)
{
  Flow *f = beginFlow (stmt);
  f->condition = t_cond;
}

// Start a new statement list for the false condition branch.

void
IRBase::startElse (void)
{
  currentFlow()->trueBranch = popStatementList();
  pushStatementList();
}

// Wrap up our constructed if condition into a COND_EXPR.

void
IRBase::endCond (void)
{
  Flow *f = currentFlow();
  tree t_brnch = popStatementList();
  tree t_false_brnch = NULL_TREE;

  if (f->trueBranch == NULL_TREE)
    f->trueBranch = t_brnch;
  else
    t_false_brnch = t_brnch;

  object_file->doLineNote (f->statement->loc);
  tree t_stmt = build3 (COND_EXPR, void_type_node,
			f->condition, f->trueBranch, t_false_brnch);
  endFlow();
  addExp (t_stmt);
}


// Routines for building statement lists around for/while loops.
// STMT is the body of the loop.

void
IRBase::startLoop (Statement *stmt)
{
  Flow *f = beginFlow (stmt);
  // should be end for 'do' loop
  f->continueLabel = d_build_label (stmt ? stmt->loc : 0, NULL);
}

// Emit continue label for loop.

void
IRBase::continueHere (void)
{
  doLabel (currentFlow()->continueLabel);
}

// Set LBL as the continue label for the current loop.
// Used in unrolled loop statements.

void
IRBase::setContinueLabel (tree lbl)
{
  currentFlow()->continueLabel = lbl;
}

// Emit exit loop condition.

void
IRBase::exitIfFalse (tree t_cond)
{
  addExp (build1 (EXIT_EXPR, void_type_node,
		  build1 (TRUTH_NOT_EXPR, TREE_TYPE (t_cond), t_cond)));
}

// Emit a goto to the continue label IDENT of a loop.

void
IRBase::continueLoop (Identifier *ident)
{
  doJump (NULL, getLoopForLabel (ident, true)->continueLabel);
}

// Emit a goto to the exit label IDENT of a loop.

void
IRBase::exitLoop (Identifier *ident)
{
  Flow *flow = getLoopForLabel (ident);
  if (!flow->exitLabel)
    flow->exitLabel = d_build_label (flow->statement->loc, NULL);
  doJump (NULL, flow->exitLabel);
}

// Wrap up constructed loop body in a LOOP_EXPR.

void
IRBase::endLoop (void)
{
  // says must contain an EXIT_EXPR -- what about while (1)..goto;? something other thand LOOP_EXPR?
  tree t_body = popStatementList();
  tree t_loop = build1 (LOOP_EXPR, void_type_node, t_body);
  addExp (t_loop);
  endFlow();
}


// Routines for building statement lists around switches.  STMT is the body
// of the switch statement, T_COND is the condition to the switch. If HAS_VARS
// is true, then the switch statement has been converted to an if-then-else.

void
IRBase::startCase (Statement *stmt, tree t_cond, int has_vars)
{
  Flow *f = beginFlow (stmt);
  f->condition = t_cond;
  f->kind = level_switch;
  if (has_vars)
    {
      // %% dummy value so the tree is not NULL
      f->hasVars = integer_one_node;
    }
}

// Emit a case statement for T_VALUE.

void
IRBase::doCase (tree t_value, tree t_label)
{
  if (currentFlow()->hasVars)
    {
      // SwitchStatement has already taken care of label jumps.
      doLabel (t_label);
    }
  else
    {
      tree t_case = build_case_label (t_value, NULL_TREE, t_label);
      addExp (t_case);
    }
}

// Wrap up constructed body into a SWITCH_EXPR.

void
IRBase::endCase (void)
{
  Flow *f = currentFlow();
  tree t_body = popStatementList();
  tree t_condtype = TREE_TYPE (f->condition);
  if (f->hasVars)
    {
      // %% switch was converted to if-then-else expression
      addExp (t_body);
    }
  else
    {
      tree t_stmt = build3 (SWITCH_EXPR, t_condtype, f->condition,
			    t_body, NULL_TREE);
      addExp (t_stmt);
    }
  endFlow();
}

// Routines for building statement lists around try/catch/finally.
// Start a try statement, STMT is the body of the try expression.

void
IRBase::startTry (Statement *stmt)
{
  beginFlow (stmt);
  currentFlow()->kind = level_try;
}

// Pops the try body and starts a new statement list for all catches.

void
IRBase::startCatches (void)
{
  currentFlow()->tryBody = popStatementList();
  currentFlow()->kind = level_catch;
  pushStatementList();
}

// Start a new catch expression for exception type T_TYPE.

void
IRBase::startCatch (tree t_type)
{
  currentFlow()->catchType = t_type;
  pushStatementList();
}

// Wrap up catch expression into a CATCH_EXPR.

void
IRBase::endCatch (void)
{
  tree t_body = popStatementList();
  // % Wrong loc... can set pass statement to startCatch, set
  // The loc on t_type and then use it here...
  addExp (build2 (CATCH_EXPR, void_type_node,
		  currentFlow()->catchType, t_body));
}

// Wrap up try/catch into a TRY_CATCH_EXPR.

void
IRBase::endCatches (void)
{
  tree t_catches = popStatementList();
  object_file->doLineNote (currentFlow()->statement->loc);
  addExp (build2 (TRY_CATCH_EXPR, void_type_node,
		  currentFlow()->tryBody, t_catches));
  endFlow();
}

// Start a new finally expression.

void
IRBase::startFinally (void)
{
  currentFlow()->tryBody = popStatementList();
  currentFlow()->kind = level_finally;
  pushStatementList();
}

// Wrap-up try/finally into a TRY_FINALLY_EXPR.

void
IRBase::endFinally (void)
{
  tree t_finally = popStatementList();
  object_file->doLineNote (currentFlow()->statement->loc);
  addExp (build2 (TRY_FINALLY_EXPR, void_type_node,
		  currentFlow()->tryBody, t_finally));
  endFlow();
}

// Emit a return expression of value T_VALUE.

void
IRBase::doReturn (tree t_value)
{
  addExp (build1 (RETURN_EXPR, void_type_node, t_value));
}

// Emit goto expression to T_LABEL.

void
IRBase::doJump (Statement *stmt, tree t_label)
{
  if (stmt)
    object_file->doLineNote (stmt->loc);
  addExp (build1 (GOTO_EXPR, void_type_node, t_label));
  TREE_USED (t_label) = 1;
}

// Routines for checking goto statements don't jump to invalid locations.
// In particular, it is illegal for a goto to be used to skip initializations.
// Saves the block label L is declared in for analysis later.

void
IRBase::pushLabel (LabelDsymbol *l)
{
  this->labels.push (getLabelBlock (l));
}

// Error if STMT is in it's own try statement separate from other
// cases in the switch statement.

void
IRBase::checkSwitchCase (Statement *stmt, int default_flag)
{
  Flow *flow = currentFlow();

  gcc_assert (flow);
  if (flow->kind != level_switch && flow->kind != level_block)
    {
      stmt->error ("%s cannot be in different try block level from switch",
		   default_flag ? "default" : "case");
    }
}

// Error if the goto referencing LABEL is jumping into a try or
// catch block.  STMT is required to error on the correct line.

void
IRBase::checkGoto (Statement *stmt, LabelDsymbol *label)
{
  Statement *curBlock = NULL;
  unsigned curLevel = this->loops.dim;
  int found = 0;

  if (curLevel)
    curBlock = currentFlow()->statement;

  for (size_t i = 0; i < this->labels.dim; i++)
    {
      Label *linfo = this->labels[i];
      gcc_assert (linfo);

      if (label == linfo->label)
	{
	  // No need checking for finally, should have already been handled.
	  if (linfo->kind == level_try
	      && curLevel <= linfo->level && curBlock != linfo->block)
	    {
	      stmt->error ("cannot goto into try block");
	    }
	  // %% doc: It is illegal for goto to be used to skip initializations,
	  // %%      so this should include all gotos into catches...
	  if (linfo->kind == level_catch && curBlock != linfo->block)
	    stmt->error ("cannot goto into catch block");

	  found = 1;
	  break;
	}
    }
  // Push forward referenced gotos.
  if (!found)
    {
      if (!label->statement->fwdrefs)
	label->statement->fwdrefs = new Blocks();
      label->statement->fwdrefs->push (getLabelBlock (label, stmt));
    }
}

// Check all forward references REFS for a label, and error
// if goto is jumping into a try or catch block.

void
IRBase::checkPreviousGoto (Array *refs)
{
  Statement *stmt; // Our forward reference.

  for (size_t i = 0; i < refs->dim; i++)
    {
      Label *ref = (Label *) refs->data[i];
      int found = 0;

      gcc_assert (ref && ref->from);
      stmt = ref->from;

      for (size_t i = 0; i < this->labels.dim; i++)
	{
	  Label *linfo = this->labels[i];
	  gcc_assert (linfo);

	  if (ref->label == linfo->label)
	    {
	      // No need checking for finally, should have already been handled.
	      if (linfo->kind == level_try
		  && ref->level <= linfo->level && ref->block != linfo->block)
		{
		  stmt->error ("cannot goto into try block");
		}
	      // %% doc: It is illegal for goto to be used to skip initializations,
	      // %%      so this should include all gotos into catches...
	      if (linfo->kind == level_catch
		  && (ref->block != linfo->block || ref->kind != linfo->kind))
		stmt->error ("cannot goto into catch block");

	      found = 1;
	      break;
	    }
	}
      gcc_assert (found);
    }
}

