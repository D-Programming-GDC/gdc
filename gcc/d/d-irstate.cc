// d-irstate.cc -- D frontend for GCC.
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
#include "d-irstate.h"
#include "d-codegen.h"

#include "init.h"

IRState::IRState (void)
{
  this->parent = NULL;
  this->func = NULL;
  this->varsInScope = NULL;
  this->mod = NULL;
  this->sthis = NULL_TREE;
}

IRState *
IRState::startFunction (FuncDeclaration *decl)
{
  IRState *new_irs = new IRState();
  new_irs->parent = current_irstate;
  new_irs->func = decl;

  for (Dsymbol *p = decl->parent; p; p = p->parent)
    {
      if (p->isModule())
	{
	  new_irs->mod = p->isModule();
	  break;
	}
    }

  current_irstate = (IRState *) new_irs;
  ModuleInfo *mi = current_module_info;

  if (decl->isSharedStaticCtorDeclaration())
    mi->sharedctors.push (decl);
  else if (decl->isStaticCtorDeclaration())
    mi->ctors.push (decl);
  else if (decl->isSharedStaticDtorDeclaration())
    {
      VarDeclaration *vgate;
      if ((vgate = decl->isSharedStaticDtorDeclaration()->vgate))
	mi->sharedctorgates.push (vgate);
      mi->shareddtors.push (decl);
    }
  else if (decl->isStaticDtorDeclaration())
    {
      VarDeclaration *vgate;
      if ((vgate = decl->isStaticDtorDeclaration()->vgate))
	mi->ctorgates.push (vgate);
      mi->dtors.push (decl);
    }
  else if (decl->isUnitTestDeclaration())
    mi->unitTests.push (decl);

  return new_irs;
}

void
IRState::endFunction (void)
{
  gcc_assert (this->scopes_.dim == 0);
  current_irstate = (IRState *) this->parent;
}


// Emit statement E into function body.

void
IRState::addExp (tree e)
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

  tree stmt_list = (tree) this->statementList_.pop();
  append_to_statement_list_force (e, &stmt_list);
  this->statementList_.push (stmt_list);
}


void
IRState::pushStatementList (void)
{
  tree t = alloc_stmt_list();
  this->statementList_.push (t);
  d_keep (t);
}

tree
IRState::popStatementList (void)
{
  tree t = (tree) this->statementList_.pop();

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
IRState::getLabelTree (LabelDsymbol *label)
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
IRState::getLabelBlock (LabelDsymbol *label, Statement *from)
{
  Label *l = new Label();

  for (int i = this->loops_.dim - 1; i >= 0; i--)
    {
      Flow *flow = this->loops_[i];

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
IRState::getLoopForLabel (Identifier *ident, bool want_continue)
{
  if (ident)
    {
      LabelStatement *lbl_stmt = this->func->searchLabel (ident)->statement;
      gcc_assert (lbl_stmt != 0);
      Statement *stmt = lbl_stmt->statement;
      ScopeStatement *scope_stmt = stmt->isScopeStatement();

      if (scope_stmt)
	stmt = scope_stmt->statement;

      for (int i = this->loops_.dim - 1; i >= 0; i--)
	{
	  Flow *flow = this->loops_[i];

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
      for (int i = this->loops_.dim - 1; i >= 0; i--)
	{
	  Flow *flow = this->loops_[i];

	  if ((!want_continue && flow->statement->hasBreak())
	      || flow->statement->hasContinue())
	    return flow;
	}
      gcc_unreachable();
    }
}


Flow *
IRState::beginFlow (Statement *stmt)
{
  Flow *flow = new Flow (stmt);
  this->loops_.push (flow);
  this->pushStatementList();
  return flow;
}

void
IRState::endFlow (void)
{
  gcc_assert (this->loops_.dim);

  Flow *flow = (Flow *) this->loops_.pop();

  if (flow->exitLabel)
    this->doLabel (flow->exitLabel);
}

void
IRState::doLabel (tree label)
{
  /* Don't write out label unless it is marked as used by the frontend.
     This makes auto-vectorization possible in conditional loops.
     The only excemption to this is in LabelStatement::toIR, in which
     all computed labels are marked regardless.  */
  if (TREE_USED (label))
    this->addExp (build1 (LABEL_EXPR, void_type_node, label));
}


void
IRState::startScope (void)
{
  unsigned *p_count = new unsigned;
  *p_count = 0;

  this->scopes_.push (p_count);
  this->startBindings();
}

void
IRState::endScope (void)
{
  unsigned *p_count = this->currentScope();
  while (*p_count)
    this->endBindings();

  this->scopes_.pop();
}


void
IRState::startBindings (void)
{
  tree block;

  push_binding_level();
  block = make_node (BLOCK);
  current_binding_level->this_block = block;

  this->pushStatementList();

  ++(*this->currentScope());
}

void
IRState::endBindings (void)
{
  tree block = pop_binding_level (1, 0);
  TREE_USED (block) = 1;

  tree body = this->popStatementList();
  this->addExp (build3 (BIND_EXPR, void_type_node,
			BLOCK_VARS (block), body, block));

  // The popped level/block is not automatically recorded
  current_binding_level->blocks = block_chainon (current_binding_level->blocks, block);

  --(*this->currentScope());
  gcc_assert (*(int *) this->currentScope() >= 0);
}


// Routines for building statement lists around if/else conditions.
// STMT contains the statement to be executed if COND is true.

void
IRState::startCond (Statement *stmt, tree cond)
{
  Flow *flow = this->beginFlow (stmt);
  flow->condition = cond;
}

// Start a new statement list for the false condition branch.

void
IRState::startElse (void)
{
  Flow *flow = this->currentFlow();
  flow->trueBranch = this->popStatementList();
  this->pushStatementList();
}

// Wrap up our constructed if condition into a COND_EXPR.

void
IRState::endCond (void)
{
  Flow *flow = this->currentFlow();
  tree branch = this->popStatementList();
  tree false_branch = NULL_TREE;

  if (flow->trueBranch == NULL_TREE)
    flow->trueBranch = branch;
  else
    false_branch = branch;

  this->doLineNote (flow->statement->loc);
  tree stmt = build3 (COND_EXPR, void_type_node,
		      flow->condition, flow->trueBranch, false_branch);
  this->endFlow();
  this->addExp (stmt);
}


// Routines for building statement lists around for/while loops.
// STMT is the body of the loop.

void
IRState::startLoop (Statement *stmt)
{
  Flow *flow = this->beginFlow (stmt);
  // should be end for 'do' loop
  flow->continueLabel = d_build_label (stmt ? stmt->loc : Loc(), NULL);
}

// Emit continue label for loop.

void
IRState::continueHere (void)
{
  Flow *flow = this->currentFlow();
  this->doLabel (flow->continueLabel);
}

// Set LBL as the continue label for the current loop.
// Used in unrolled loop statements.

void
IRState::setContinueLabel (tree label)
{
  Flow *flow = this->currentFlow();
  flow->continueLabel = label;
}

// Emit exit loop condition.

void
IRState::exitIfFalse (tree cond)
{
  this->addExp (build1 (EXIT_EXPR, void_type_node,
			build1 (TRUTH_NOT_EXPR, TREE_TYPE (cond), cond)));
}

// Emit a goto to the continue label IDENT of a loop.

void
IRState::continueLoop (Identifier *ident)
{
  Flow *flow = this->getLoopForLabel (ident, true);
  this->doJump (NULL, flow->continueLabel);
}

// Emit a goto to the exit label IDENT of a loop.

void
IRState::exitLoop (Identifier *ident)
{
  Flow *flow = this->getLoopForLabel (ident);
  if (!flow->exitLabel)
    flow->exitLabel = d_build_label (flow->statement->loc, NULL);
  this->doJump (NULL, flow->exitLabel);
}

// Wrap up constructed loop body in a LOOP_EXPR.

void
IRState::endLoop (void)
{
  // says must contain an EXIT_EXPR -- what about while (1)..goto;? something other thand LOOP_EXPR?
  tree body = this->popStatementList();
  tree loop = build1 (LOOP_EXPR, void_type_node, body);
  this->addExp (loop);
  this->endFlow();
}


// Create a tree node to set multiple elements to a single value

tree
IRState::doArraySet(tree ptr, tree value, tree count)
{
  tree t;

  pushStatementList();
  startBindings();

  // Build temporary locals for count and ptr, and maybe value.
  t = build_local_temp (size_type_node);
  DECL_INITIAL (t) = count;
  count = t;
  expand_decl (count);

  t = build_local_temp (TREE_TYPE (ptr));
  DECL_INITIAL (t) = ptr;
  ptr = t;
  expand_decl (ptr);

  if (d_has_side_effects (value))
    {
      t = build_local_temp (TREE_TYPE (value));
      DECL_INITIAL (t) = value;
      value = t;
      expand_decl (value);
    }

  // Build loop to initialise { .length=count, .ptr=ptr } with value.
  //
  //   while (count != 0)
  //   {
  //     *ptr = value;
  //     ptr += (*ptr).sizeof;
  //     count -= 1;
  //   }
  tree pesize = TYPE_SIZE_UNIT (TREE_TYPE (TREE_TYPE (ptr)));
  tree count_zero = d_convert (TREE_TYPE (count), integer_zero_node);
  tree count_one = d_convert (TREE_TYPE (count), integer_one_node);

  startLoop (NULL);
  continueHere();
  exitIfFalse (build_boolop (NE_EXPR, count, count_zero));

  addExp (vmodify_expr (build_deref (ptr), value));
  addExp (vmodify_expr (ptr, build_offset (ptr, pesize)));
  addExp (build2 (POSTDECREMENT_EXPR, TREE_TYPE (count), count, count_one));

  endLoop();
  endBindings();

  return popStatementList();
}

// Routines for building statement lists around switches.  STMT is the body
// of the switch statement, COND is the condition to the switch. If HAS_VARS
// is true, then the switch statement has been converted to an if-then-else.

void
IRState::startCase (Statement *stmt, tree cond, int has_vars)
{
  Flow *flow = this->beginFlow (stmt);
  flow->condition = cond;
  flow->kind = level_switch;
  if (has_vars)
    {
      // %% dummy value so the tree is not NULL
      flow->hasVars = integer_one_node;
    }
}

// Emit a case statement for VALUE.

void
IRState::doCase (tree value, tree label)
{
  // SwitchStatement has already taken care of label jumps.
  if (this->currentFlow()->hasVars)
    this->doLabel (label);
  else
    {
      tree case_label = build_case_label (value, NULL_TREE, label);
      this->addExp (case_label);
    }
}

// Wrap up constructed body into a SWITCH_EXPR.

void
IRState::endCase (void)
{
  Flow *flow = this->currentFlow();
  tree body = this->popStatementList();
  tree condtype = TREE_TYPE (flow->condition);

  // Switch was converted to if-then-else expression
  if (flow->hasVars)
    this->addExp (body);
  else
    {
      tree stmt = build3 (SWITCH_EXPR, condtype,
			  flow->condition, body, NULL_TREE);
      this->addExp (stmt);
    }

  this->endFlow();
}

// Routines for building statement lists around try/catch/finally.
// Start a try statement, STMT is the body of the try expression.

void
IRState::startTry (Statement *stmt)
{
  Flow *flow = this->beginFlow (stmt);
  flow->kind = level_try;
}

// Pops the try body and starts a new statement list for all catches.

void
IRState::startCatches (void)
{
  Flow *flow = this->currentFlow();
  flow->tryBody = this->popStatementList();
  flow->kind = level_catch;
  this->pushStatementList();
}

// Start a new catch expression for exception type TYPE.

void
IRState::startCatch (tree type)
{
  this->currentFlow()->catchType = type;
  this->pushStatementList();
}

// Wrap up catch expression into a CATCH_EXPR.

void
IRState::endCatch (void)
{
  tree body = this->popStatementList();
  // % Wrong loc... can set pass statement to startCatch, set
  // The loc on type and then use it here...
  this->addExp (build2 (CATCH_EXPR, void_type_node,
			this->currentFlow()->catchType, body));
}

// Wrap up try/catch into a TRY_CATCH_EXPR.

void
IRState::endCatches (void)
{
  Flow *flow = this->currentFlow();
  tree catches = this->popStatementList();

  this->doLineNote (flow->statement->loc);
  this->addExp (build2 (TRY_CATCH_EXPR, void_type_node,
      			flow->tryBody, catches));
  this->endFlow();
}

// Start a new finally expression.

void
IRState::startFinally (void)
{
  Flow *flow = this->currentFlow();
  flow->tryBody = this->popStatementList();
  flow->kind = level_finally;
  this->pushStatementList();
}

// Wrap-up try/finally into a TRY_FINALLY_EXPR.

void
IRState::endFinally (void)
{
  Flow *flow = this->currentFlow();
  tree finally = this->popStatementList();

  this->doLineNote (flow->statement->loc);
  this->addExp (build2 (TRY_FINALLY_EXPR, void_type_node,
			flow->tryBody, finally));
  this->endFlow();
}

// Emit a return expression of value VALUE.

void
IRState::doReturn (tree value)
{
  this->addExp (build1 (RETURN_EXPR, void_type_node, value));
}

// Emit goto expression to LABEL.

void
IRState::doJump (Statement *stmt, tree label)
{
  if (stmt)
    this->doLineNote (stmt->loc);

  this->addExp (build1 (GOTO_EXPR, void_type_node, label));
  TREE_USED (label) = 1;
}

// Routines for checking goto statements don't jump to invalid locations.
// In particular, it is illegal for a goto to be used to skip initializations.
// Saves the block LABEL is declared in for analysis later.

void
IRState::pushLabel (LabelDsymbol *label)
{
  Label *lblock = this->getLabelBlock (label);
  this->labels_.push (lblock);
}

// Error if STMT is in it's own try statement separate from other
// cases in the switch statement.

void
IRState::checkSwitchCase (Statement *stmt, int default_flag)
{
  Flow *flow = this->currentFlow();

  if (flow->kind != level_switch && flow->kind != level_block)
    {
      stmt->error ("%s cannot be in different try block level from switch",
		   default_flag ? "default" : "case");
    }
}

// Error if the goto referencing LABEL is jumping into a try or
// catch block.  STMT is required to error on the correct line.

void
IRState::checkGoto (Statement *stmt, LabelDsymbol *label)
{
  Statement *curBlock = NULL;
  unsigned curLevel = this->loops_.dim;
  int found = 0;

  if (curLevel)
    curBlock = this->currentFlow()->statement;

  for (size_t i = 0; i < this->labels_.dim; i++)
    {
      Label *linfo = this->labels_[i];
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
      Label *lblock = this->getLabelBlock (label, stmt);

      if (!label->statement->fwdrefs)
	label->statement->fwdrefs = new Blocks();

      label->statement->fwdrefs->push (lblock);
    }
}

// Check all forward references REFS for a label, and error
// if goto is jumping into a try or catch block.

void
IRState::checkPreviousGoto (Array *refs)
{
  Statement *stmt; // Our forward reference.

  for (size_t i = 0; i < refs->dim; i++)
    {
      Label *ref = (Label *) refs->data[i];
      int found = 0;

      gcc_assert (ref && ref->from);
      stmt = ref->from;

      for (size_t i = 0; i < this->labels_.dim; i++)
	{
	  Label *linfo = this->labels_[i];
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

