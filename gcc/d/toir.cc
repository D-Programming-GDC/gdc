// toir.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "dfrontend/enum.h"
#include "dfrontend/module.h"
#include "dfrontend/init.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/expression.h"
#include "dfrontend/statement.h"
#include "dfrontend/visitor.h"

#include "tree.h"
#include "tree-iterator.h"
#include "options.h"
#include "stmt.h"
#include "fold-const.h"
#include "diagnostic.h"

#include "d-tree.h"
#include "d-lang.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "d-irstate.h"
#include "id.h"


// Implements the visitor interface to build the GCC trees of all Statement
// AST classes emitted from the D Front-end, where IRS_ holds the state of
// the current function being compiled.
// All visit methods accept one parameter S, which holds the frontend AST
// of the statement to compile.  They also don't return any value, instead
// generated code are pushed to add_stmt(), which appends them to the
// statement list in the current_binding_level.

class IRVisitor : public Visitor
{
  FuncDeclaration *func_;
  IRState *irs_;

public:
  IRVisitor(FuncDeclaration *fd, IRState *irs) : func_(fd), irs_(irs) {}

  // Definitions for scope code:
  // "Scope": A container for binding contours.  Each user-declared function
  // has a toplevel scope.  Every ScopeStatement creates a new scope.
  //
  // "Binding contour": Same as GCC's definition, whatever that is.
  // Each user-declared variable will have a binding contour that begins
  // where the variable is declared and ends at it's containing scope.
  void start_scope()
  {
    push_binding_level();
    push_stmt_list();
  }

  // Leave scope pushed by start_scope.
  void end_scope()
  {
    tree block = pop_binding_level(false);
    tree body = pop_stmt_list();

    add_stmt(build3(BIND_EXPR, void_type_node,
		    BLOCK_VARS (block), body, block));
  }

  // Get or build the LABEL_DECL for the given LABEL symbol.
  tree get_label_tree(LabelDsymbol *label)
  {
    static hash_map<Statement *, tree> labels;

    if (!label->statement)
      return NULL_TREE;

    tree &label_decl = labels.get_or_insert(label->statement);
    if (label_decl == NULL_TREE)
      {
        label_decl = d_build_label(label->statement->loc, label->ident);
        TREE_USED (label_decl) = 1;
      }

    return label_decl;
  }

  // Build a new label structure to hold intermediate information on where
  // LABEL is declared so we can check whether all jumps at future points
  // in code satisfy EH boundaries.
  // If FROM has a value, then the label was forward referenced, and we
  // need to record the location of the originating jump.
  Label *get_label_block(LabelDsymbol *label, Statement *from)
  {
    Label *l = new Label();

    for (int i = this->irs_->loops_.length() - 1; i >= 0; i--)
      {
	Flow *flow = this->irs_->loops_[i];

	// Anything that isn't an EH block is considered transparent.
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

  // Find and return the Flow scope relating to the break or continue
  // label of the current loop.  If IDENT has a value, then we are
  // looking for a named break or continue label in code.
  // Unless WANT_CONTINUE, search for a break label.
  Flow *get_loop_for_label(Identifier *ident, bool want_continue)
  {
    if (ident)
      {
	// The break label for a loop may actually be some levels up;
	// eg: on a try/finally wrapping a loop.
	LabelStatement *lstmt = this->func_->searchLabel(ident)->statement;
	gcc_assert(lstmt != NULL);
	Statement *stmt = lstmt->statement->getRelatedLabeled();

	for (int i = this->irs_->loops_.length() - 1; i >= 0; i--)
	  {
	    Flow *flow = this->irs_->loops_[i];

	    if (flow->statement->getRelatedLabeled() == stmt)
	      {
		if (want_continue)
		  gcc_assert(flow->statement->hasContinue());
		return flow;
	      }
	  }

	return NULL;
      }

    for (int i = this->irs_->loops_.length() - 1; i >= 0; i--)
      {
	Flow *flow = this->irs_->loops_[i];

	if ((!want_continue && flow->statement->hasBreak())
	    || flow->statement->hasContinue())
	  return flow;
      }

    return NULL;
  }

  // Return TRUE if IDENT is the current function return label.
  bool is_return_label(Identifier *ident)
  {
    if (this->irs_->func->returnLabel)
      return this->irs_->func->returnLabel->ident == ident;

    return false;
  }

  // Emit a LABEL expression.
  void do_label(tree label)
  {
    // Don't write out label unless it is marked as used by the frontend.
    // This makes auto-vectorization possible in conditional loops.
    // The only excemption to this is in the LabelStatement visitor,
    // in which all computed labels are marked regardless.
    if (TREE_USED (label))
      add_stmt(build1(LABEL_EXPR, void_type_node, label));
  }

  // Emit a goto expression to LABEL.
  void do_jump(Statement *stmt, tree label)
  {
    if (stmt)
      set_input_location(stmt->loc);

    add_stmt(fold_build1(GOTO_EXPR, void_type_node, label));
    TREE_USED (label) = 1;
  }

  // Emit a goto to the continue label IDENT of a loop.
  void continue_loop(Identifier *ident)
  {
    Flow *flow = this->get_loop_for_label(ident, true);
    gcc_assert(flow);
    this->do_jump(NULL, flow->continueLabel);
  }

  // Emit a goto to the exit label IDENT of a loop.
  void exit_loop(Identifier *ident)
  {
    Flow *flow = this->get_loop_for_label(ident, false);
    gcc_assert(flow);

    if (!flow->exitLabel)
      flow->exitLabel = d_build_label(flow->statement->loc, NULL);

    this->do_jump(NULL, flow->exitLabel);
  }

  // Routines for building statement lists around if/else conditions.
  // STMT contains the statement to be executed.
  void start_condition(Statement *stmt)
  {
    this->irs_->beginFlow(stmt);
    push_stmt_list();
  }

  // Pops and returns the 'then' body, starting a new statement list
  // for the 'else' condition branch.
  tree start_else()
  {
    tree ifbody = pop_stmt_list();
    push_stmt_list();
    return ifbody;
  }

  // Wrap up our constructed if condition into a COND_EXPR.
  void end_condition(tree ifcond, tree ifbody)
  {
    Flow *flow = this->irs_->currentFlow();
    tree elsebody;

    if (ifbody == NULL_TREE)
      {
	ifbody = pop_stmt_list();
	elsebody = void_node;
      }
    else
      elsebody = pop_stmt_list();

    set_input_location(flow->statement->loc);
    tree stmt = build3(COND_EXPR, void_type_node,
		       ifcond, ifbody, elsebody);
    this->irs_->endFlow();
    add_stmt(stmt);
  }

  // Routines for building statement lists around try/catch/finally.
  // Start a try statement, STMT is the body of the try expression.
  void start_try(Statement *stmt)
  {
    Flow *flow = this->irs_->beginFlow(stmt);
    flow->kind = level_try;
    push_stmt_list();
  }

  // Pops and returns the try body and starts a new statement list for all catches.
  tree start_catches()
  {
    tree try_body = pop_stmt_list();

    Flow *flow = this->irs_->currentFlow();
    flow->kind = level_catch;
    push_stmt_list();

    return try_body;
  }

  // Start a new catch expression for exception type TYPE.
  tree start_catch(Type *type)
  {
    push_stmt_list();
    return build_ctype(type);
  }

  // Wrap up catch expression for type CATCH_TYPE into a CATCH_EXPR.
  void end_catch(tree catch_type)
  {
    tree body = pop_stmt_list();
    add_stmt(build2(CATCH_EXPR, void_type_node, catch_type, body));
  }

  // Wrap up try/catch into a TRY_CATCH_EXPR.
  void end_catches(tree try_body)
  {
    Flow *flow = this->irs_->currentFlow();
    tree catches = pop_stmt_list();

    // Backend expects all catches in a TRY_CATCH_EXPR to be enclosed in a
    // statement list, however pop_stmt_list may optimise away the list
    // if there is only a single catch to push.
    if (TREE_CODE (catches) != STATEMENT_LIST)
      {
        tree stmt_list = alloc_stmt_list();
        append_to_statement_list_force(catches, &stmt_list);
        d_keep(stmt_list);
        catches = stmt_list;
      }

    set_input_location(flow->statement->loc);
    add_stmt(build2(TRY_CATCH_EXPR, void_type_node,
		    try_body, catches));
    this->irs_->endFlow();
  }

  // Pops and returns the try body and starts a new finally expression.
  tree start_finally()
  {
    tree try_body = pop_stmt_list();

    Flow *flow = this->irs_->currentFlow();
    flow->kind = level_finally;
    push_stmt_list();

    return try_body;
  }

  // Wrap-up try/finally into a TRY_FINALLY_EXPR.
  void end_finally(tree try_body)
  {
    Flow *flow = this->irs_->currentFlow();
    tree finally = pop_stmt_list();

    set_input_location(flow->statement->loc);
    add_stmt(build2(TRY_FINALLY_EXPR, void_type_node,
		    try_body, finally));
    this->irs_->endFlow();
  }

  // Routines for building statement lists around for/while loops.
  // STMT is the body of the loop.
  void start_loop(Statement *stmt)
  {
    Flow *flow = this->irs_->beginFlow(stmt);
    // should be end for 'do' loop
    flow->continueLabel = d_build_label(stmt->loc, NULL);
    push_stmt_list();
  }

  // Emit continue label for loop.
  void continue_label()
  {
    Flow *flow = this->irs_->currentFlow();
    this->do_label(flow->continueLabel);
  }

  // Set LBL as the continue label for the current loop.
  // Used in unrolled loop statements.
  void set_continue_label(tree label)
  {
    Flow *flow = this->irs_->currentFlow();
    flow->continueLabel = label;
  }

  // Emit exit loop condition.
  void exit_if_false(tree cond)
  {
    add_stmt(build1(EXIT_EXPR, void_type_node,
		    build1(TRUTH_NOT_EXPR, TREE_TYPE (cond), cond)));
  }

  // Wrap up constructed loop body in a LOOP_EXPR.
  void end_loop()
  {
    tree body = pop_stmt_list();
    tree loop = build1(LOOP_EXPR, void_type_node, body);
    add_stmt(loop);
    this->irs_->endFlow();
  }

  // Routines for building statement lists around switches.  STMT is the body
  // of the switch statement.  If HAS_VARS is true, then the switch statement
  // has been converted to an if-then-else.
  void start_case(Statement *stmt, bool has_vars)
  {
    Flow *flow = this->irs_->beginFlow(stmt);
    flow->kind = level_switch;
    flow->hasVars = has_vars;
    push_stmt_list();
  }

  // Emit a case statement for VALUE.
  void add_case(tree value, tree label)
  {
    // SwitchStatement has already taken care of label jumps.
    if (this->irs_->currentFlow()->hasVars)
      this->do_label(label);
    else
      {
	tree case_label = build_case_label(value, NULL_TREE, label);
	add_stmt(case_label);
      }
  }

  // Wrap up constructed body into a SWITCH_EXPR.
  // CASE_COND is the condition to the switch.
  void end_case(tree case_cond)
  {
    Flow *flow = this->irs_->currentFlow();
    tree case_body = pop_stmt_list();

    // Switch was converted to if-then-else expression
    if (flow->hasVars)
      add_stmt(case_body);
    else
      {
	tree stmt = build3(SWITCH_EXPR, TREE_TYPE (case_cond),
			   case_cond, case_body, NULL_TREE);
	add_stmt(stmt);
      }

    this->irs_->endFlow();
  }


  // Visitor interfaces.

  // This should be overridden by each statement class.
  void visit(Statement *)
  {
    gcc_unreachable();
  }

  // The frontend lowers scope(exit/failure/success) statements as
  // try/catch/finally. At this point, this statement is just an empty
  // placeholder.  Maybe the frontend shouldn't leak these.
  void visit(OnScopeStatement *)
  {
  }

  // if(...) { ... } else { ... }
  void visit(IfStatement *s)
  {
    set_input_location(s->loc);
    this->start_scope();

    // Build the outer 'if' condition, which may produce temporaries
    // requiring scope destruction.
    tree ifcond = convert_for_condition(s->condition->toElemDtor(this->irs_),
					s->condition->type);
    this->start_condition(s);

    if (s->ifbody)
      s->ifbody->accept(this);

    // Now build the 'else' branch, which may have nested 'else if' parts.
    tree ifbody = NULL_TREE;
    if (s->elsebody)
      {
	ifbody = this->start_else();
	s->elsebody->accept(this);
      }

    this->end_condition(ifcond, ifbody);
    this->end_scope();
  }

  // Should there be any pragma(...) statements requiring code generation,
  // here would be the place to do it.  For now, all pragmas are handled
  // by the frontend.
  void visit(PragmaStatement *)
  {
  }

  // The frontend lowers while(...) statements as for(...) loops.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(WhileStatement *)
  {
    gcc_unreachable();
  }

  // 
  void visit(DoStatement *s)
  {
    set_input_location(s->loc);
    this->start_loop(s);

    if (s->body)
      s->body->accept(this);

    this->continue_label();

    // Build the outer 'while' condition, which may produce temporaries
    // requiring scope destruction.
    set_input_location(s->condition->loc);
    tree exitcond = convert_for_condition(s->condition->toElemDtor(this->irs_),
					  s->condition->type);
    this->exit_if_false(exitcond);
    this->end_loop();
  }

  // for(...) { ... }
  void visit(ForStatement *s)
  {
    set_input_location(s->loc);

    if (s->init)
      s->init->accept(this);

    this->start_loop(s);

    if (s->condition)
      {
	set_input_location(s->condition->loc);
	tree exitcond = convert_for_condition(s->condition->toElemDtor(this->irs_),
					      s->condition->type);
	this->exit_if_false(exitcond);
      }

    if (s->body)
      s->body->accept(this);

    this->continue_label();

    if (s->increment)
      {
	// Force side effects?
	set_input_location(s->increment->loc);
	add_stmt(s->increment->toElemDtor(this->irs_));
      }

    this->end_loop();
  }

  // The frontend lowers foreach(...) statements as for(...) loops.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(ForeachStatement *)
  {
    gcc_unreachable();
  }

  // The frontend lowers foreach(...[x..y]) statements as for(...) loops.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(ForeachRangeStatement *)
  {
    gcc_unreachable();
  }

  // Jump to the associated exit label for the current loop.
  // If IDENT for the Statement is not null, then the label is user defined.
  void visit(BreakStatement *s)
  {
    set_input_location(s->loc);
    this->exit_loop(s->ident);
  }

  // Jump to the associated continue label for the current loop.
  // If IDENT for the Statement is not null, then the label is user defined.
  void visit(ContinueStatement *s)
  {
    set_input_location(s->loc);
    this->continue_loop(s->ident);
  }

  //
  void visit(GotoStatement *s)
  {
    gcc_assert(s->label->statement != NULL);
    gcc_assert(s->tf == s->label->statement->tf);

    // This makes the 'undefined label' error show up on the correct line.
    // The extra set_input_location in do_jump shouldn't cause a problem.
    set_input_location(s->loc);

    // Need to error if the goto is jumping into a try or catch block.
    Statement *block = NULL;
    unsigned level = this->irs_->loops_.length();
    int found = 0;

    if (level)
      block = this->irs_->currentFlow()->statement;

    for (size_t i = 0; i < this->irs_->labels_.length(); i++)
      {
	Label *linfo = this->irs_->labels_[i];
	gcc_assert(linfo);

	if (s->label != linfo->label)
	  continue;

	// No need checking for finally, should have already been handled.
	if (linfo->kind == level_try
	    && level <= linfo->level && block != linfo->block)
	  s->error("cannot goto into try block");

	// It is illegal for goto to be used to skip initializations,
	// so this should include all gotos into catches...
	if (linfo->kind == level_catch && block != linfo->block)
	  s->error("cannot goto into catch block");

	found = 1;
	break;
      }

    // Push forward referenced gotos.
    if (!found)
      {
	Label *lblock = this->get_label_block(s->label, s);

	if (!s->label->statement->fwdrefs)
	  s->label->statement->fwdrefs = new Blocks();

	s->label->statement->fwdrefs->push(lblock);
      }

    // If no label found, there was an error.
    tree label = this->get_label_tree(s->label);

    if (label != NULL_TREE)
      this->do_jump(s, label);
  }

  //
  void visit(LabelStatement *s)
  {
    LabelDsymbol *sym;

    if (this->is_return_label(s->ident))
      sym = this->func_->returnLabel;
    else
      sym = this->func_->searchLabel(s->ident);

    // If no label found, there was an error
    tree label = this->get_label_tree(sym);
    if (label == NULL_TREE)
      return;

    // Save the block that the label is declared in for analysis later.
    Label *lblock = this->get_label_block(sym, NULL);
    this->irs_->labels_.safe_push (lblock);

    this->do_label(label);

    if (this->is_return_label(s->ident) && this->func_->fensure != NULL)
      this->func_->fensure->accept(this);
    else if (s->statement)
      s->statement->accept(this);

    // Check all forward references, and error if the goto
    // is jumping into a try or catch block.
    if (s->fwdrefs)
      {
	for (size_t i = 0; i < s->fwdrefs->dim; i++)
	  {
	    Label *ref = (*s->fwdrefs)[i];
	    int found = 0;

	    gcc_assert(ref && ref->from);
	    Statement *fwdref = ref->from;

	    for (size_t i = 0; i < this->irs_->labels_.length(); i++)
	      {
		Label *linfo = this->irs_->labels_[i];
		gcc_assert(linfo);

		if (ref->label != linfo->label)
		  continue;

		// No need checking for finally, should have already been handled.
		if (linfo->kind == level_try
		    && ref->level <= linfo->level && ref->block != linfo->block)
		  fwdref->error("cannot goto into try block");
		// It is illegal for goto to be used to skip initializations,
		// so this should include all gotos into catches...
		if (linfo->kind == level_catch
		    && (ref->block != linfo->block || ref->kind != linfo->kind))
		  fwdref->error("cannot goto into catch block");

		found = 1;
		break;
	      }

	    gcc_assert(found);
	  }

	s->fwdrefs = NULL;
      }
  }

  //
  void visit(SwitchStatement *s)
  {
    set_input_location(s->loc);

    tree condition = s->condition->toElemDtor(this->irs_);
    Type *condtype = s->condition->type->toBasetype();

    if (s->condition->type->isString())
      {
	Type *etype = condtype->nextOf()->toBasetype();
	LibCall libcall;

	switch (etype->ty)
	  {
	  case Tchar:
	    libcall = LIBCALL_SWITCH_STRING;
	    break;

	  case Twchar:
	    libcall = LIBCALL_SWITCH_USTRING;
	    break;

	  case Tdchar:
	    libcall = LIBCALL_SWITCH_DSTRING;
	    break;

	  default:
	    ::error("switch statement value must be an array of some character type, not %s",
		    etype->toChars());
	    gcc_unreachable();
	  }

	// Apparently the backend is supposed to sort and set the indexes
	// on the case array, have to change them to be useable.
	s->cases->sort();

	tree args[2];
	Symbol *sym = new Symbol();
	dt_t **pdt = &sym->Sdt;

	for (size_t i = 0; i < s->cases->dim; i++)
	  {
	    CaseStatement *cs = (*s->cases)[i];
	    cs->index = i;

	    if (cs->exp->op != TOKstring)
	      s->error("case '%s' is not a string", cs->exp->toChars());
	    else
	      pdt = cs->exp->toDt(pdt);
	  }

	sym->Sreadonly = true;
	d_finish_symbol(sym);

	args[0] = d_array_value(build_ctype(condtype->arrayOf()),
				size_int(s->cases->dim),
				build_address(sym->Stree));
	args[1] = condition;

	condition = build_libcall(libcall, 2, args);
      }
    else if (!condtype->isscalar())
      {
	::error("cannot handle switch condition of type %s", condtype->toChars());
	gcc_unreachable();
      }

    if (s->cases)
      {
	// Build LABEL_DECLs now so they can be refered to by goto case
	for (size_t i = 0; i < s->cases->dim; i++)
	  {
	    CaseStatement *cs = (*s->cases)[i];
	    cs->cblock = d_build_label(cs->loc, NULL);
	  }
	if (s->sdefault)
	  s->sdefault->cblock = d_build_label(s->sdefault->loc, NULL);
      }

    condition = fold(condition);

    if (s->hasVars)
      {
	// Write cases as a series of if-then-else blocks.
	for (size_t i = 0; i < s->cases->dim; i++)
	  {
	    CaseStatement *cs = (*s->cases)[i];
	    tree case_cond = build2(EQ_EXPR, build_ctype(condtype), condition,
				    cs->exp->toElemDtor(this->irs_));
	    this->start_condition(s);
	    this->do_jump(NULL, cs->cblock);
	    this->end_condition(case_cond, NULL_TREE);
	  }

	if (s->sdefault)
	  this->do_jump(NULL, s->sdefault->cblock);
      }

    // Emit body.
    this->start_case(s, s->hasVars);

    if (s->body)
      s->body->accept(this);

    this->end_case(condition);
  }

  //
  void visit(CaseStatement *s)
  {
    tree caseval;
    if (s->exp->type->isscalar())
      caseval = s->exp->toElem(this->irs_);
    else
      caseval = build_integer_cst(s->index, build_ctype(Type::tint32));

    Flow *flow = this->irs_->currentFlow();
    if (flow->kind != level_switch && flow->kind != level_block)
      s->error("case cannot be in different try block level from switch");

    this->add_case(caseval, s->cblock);

    if (s->statement)
      s->statement->accept(this);
  }

  //
  void visit(DefaultStatement *s)
  {
    Flow *flow = this->irs_->currentFlow();
    if (flow->kind != level_switch && flow->kind != level_block)
      s->error("default cannot be in different try block level from switch");

    this->add_case(NULL_TREE, s->cblock);

    if (s->statement)
      s->statement->accept(this);
  }

  // Implements 'goto default' by jumping to the label associated with
  // the DefaultStatement in a switch block.
  // Assumes CBLOCK has been set in SwitchStatement visitor.
  void visit(GotoDefaultStatement *s)
  {
    this->do_jump(s, s->sw->sdefault->cblock);
  }

  // Implements 'goto case' by jumping to the label associated with the
  // CaseStatement in a switch block.
  // Assumes CBLOCK has been set in SwitchStatement visitor.
  void visit(GotoCaseStatement *s)
  {
    this->do_jump(s, s->cs->cblock);
  }

  // Throw a SwitchError exception, called when a switch statement has
  // no DefaultStatement, yet none of the cases match.
  void visit(SwitchErrorStatement *s)
  {
    set_input_location(s->loc);
    add_stmt(d_assert_call(s->loc, LIBCALL_SWITCH_ERROR));
  }

  //
  void visit(ReturnStatement *s)
  {
    set_input_location(s->loc);

    if (s->exp == NULL || s->exp->type->toBasetype()->ty == Tvoid)
      {
	// Return has no value.
	add_stmt(return_expr(NULL_TREE));
	return;
      }

    TypeFunction *tf = (TypeFunction *)this->func_->type;
    Type *type = this->func_->tintro != NULL
      ? this->func_->tintro->nextOf() : tf->nextOf();

    if (this->func_->isMain() && type->toBasetype()->ty == Tvoid)
      type = Type::tint32;

    tree decl = DECL_RESULT (this->func_->toSymbol()->Stree);

    if (this->func_->nrvo_can && this->func_->nrvo_var)
      {
	// Just refer to the DECL_RESULT; this is a nop, but differs
	// from using NULL_TREE in that it indicates that we care about
	// the value of the DECL_RESULT.
	add_stmt(return_expr(decl));
      }
    else
      {
	// Convert for initialising the DECL_RESULT.
	tree value = convert_expr(s->exp->toElemDtor(this->irs_),
				  s->exp->type, type);

	// If we are returning a reference, take the address.
	if (tf->isref)
	  value = build_address(value);

	tree assign = build2(INIT_EXPR, TREE_TYPE (decl), decl, value);
	add_stmt(return_expr(assign));
      }
  }

  // 
  void visit(ExpStatement *s)
  {
    if (s->exp)
      {
	set_input_location(s->loc);
	// Expression may produce temporaries requiring scope destruction.
	tree exp = s->exp->toElemDtor(this->irs_);
	add_stmt(exp);
      }
  }

  //
  void visit(CompoundStatement *s)
  {
    if (s->statements == NULL)
      return;

    for (size_t i = 0; i < s->statements->dim; i++)
      {
	Statement *statement = (*s->statements)[i];

	if (statement != NULL)
	  statement->accept(this);
      }
  }

  // The frontend lowers foreach(Tuple!(...)) statements as an unrolled loop.
  // These are compiled down as a do ... while(0), where each unrolled loop is
  // nested inside and given their own continue label to jump to.
  void visit(UnrolledLoopStatement *s)
  {
    if (s->statements == NULL)
      return;

    this->start_loop(s);
    this->continue_label();

    for (size_t i = 0; i < s->statements->dim; i++)
      {
	Statement *statement = (*s->statements)[i];

	if (statement != NULL)
	  {
	    this->set_continue_label(d_build_label(s->loc, NULL));
	    statement->accept(this);
	    this->continue_label();
	  }
      }

    this->exit_loop(NULL);
    this->end_loop();
  }

  // Start a new scope and visit all nested statements, wrapping
  // them up into a BIND_EXPR at the end of the scope.
  void visit(ScopeStatement *s)
  {
    if (s->statement == NULL)
      return;

    this->start_scope();
    s->statement->accept(this);
    this->end_scope();
  }

  //
  void visit(WithStatement *s)
  {
    set_input_location(s->loc);
    this->start_scope();

    if (s->wthis)
      {
	// Perform initialisation of the 'with' handle.
	ExpInitializer *ie = s->wthis->init->isExpInitializer();
	gcc_assert(ie != NULL);

	build_local_var(s->wthis, this->func_);
	tree init = ie->exp->toElemDtor(this->irs_);
	add_stmt(init);
      }

    if (s->body)
      s->body->accept(this);

    this->end_scope();
  }

  // Implements 'throw Object'.  Frontend already checks that the object
  // thrown is a class type, but does not check if it is derived from
  // Object.  Foreign objects are not currently supported in runtime.
  void visit(ThrowStatement *s)
  {
    ClassDeclaration *cd = s->exp->type->toBasetype()->isClassHandle();
    InterfaceDeclaration *id = cd->isInterfaceDeclaration();
    tree arg = s->exp->toElemDtor(this->irs_);

    if (!flag_exceptions)
      {
	static int warned = 0;
	if (!warned)
	  {
	    s->error("exception handling disabled, use -fexceptions to enable");
	    warned = 1;
	  }
      }

    if (cd->cpp || (id != NULL && id->cpp))
      s->error("cannot throw C++ classes");
    else if (cd->com || (id != NULL && id->com))
      s->error("cannot throw COM objects");
    else
      arg = convert_expr(arg, s->exp->type, build_object_type());

    set_input_location(s->loc);
    add_stmt(build_libcall(LIBCALL_THROW, 1, &arg));
  }

  //
  void visit(TryCatchStatement *s)
  {
    set_input_location(s->loc);
    this->start_try(s);

    if (s->body)
      s->body->accept(this);

    tree try_body = this->start_catches();

    if (s->catches)
      {
	for (size_t i = 0; i < s->catches->dim; i++)
	  {
	    Catch *vcatch = (*s->catches)[i];
	    set_input_location(vcatch->loc);

	    tree catch_type = this->start_catch(vcatch->type);
	    this->start_scope();

	    if (vcatch->var)
	      {
		// Get D's internal exception Object, different
		// from the generic exception pointer.
		tree ehptr = d_build_call_nary(builtin_decl_explicit(BUILT_IN_EH_POINTER),
					       1, integer_zero_node);
		tree object = build_libcall(LIBCALL_BEGIN_CATCH, 1, &ehptr);
		object = build1(NOP_EXPR, build_ctype(build_object_type()), object);
		object = convert_expr(object, build_object_type(), vcatch->type);

		tree var = vcatch->var->toSymbol()->Stree;
		tree init = build_vinit(var, object);

		build_local_var(vcatch->var, this->func_);
		add_stmt(init);
	      }

	    if (vcatch->handler)
	      vcatch->handler->accept(this);

	    this->end_scope();
	    this->end_catch(catch_type);
	  }
      }

    this->end_catches(try_body);
  }

  //
  void visit(TryFinallyStatement *s)
  {
    set_input_location(s->loc);
    this->start_try(s);

    if (s->body)
      s->body->accept(this);

    tree try_body = this->start_finally();

    if (s->finalbody)
      s->finalbody->accept(this);

    this->end_finally(try_body);
  }

  // The frontend lowers synchronized(...) statements as a call to
  // monitor/critical enter and exit wrapped around try/finally.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(SynchronizedStatement *)
  {
    gcc_unreachable();
  }

  // D Inline Assembler is not implemented, as would require a writing
  // an assembly parser for each supported target.  Instead we leverage
  // GCC extended assembler using the ExtAsmStatement class.
  void visit(AsmStatement *)
  {
    sorry("D inline assembler statements are not supported in GDC.");
  }

  // Build a GCC extended assembler expression, whose components are
  // an INSN string, some OUTPUTS, some INPUTS, and some CLOBBERS.
  void visit(ExtAsmStatement *s)
  {
    StringExp *insn = (StringExp *)s->insn;
    tree outputs = NULL_TREE;
    tree inputs = NULL_TREE;
    tree clobbers = NULL_TREE;
    tree labels = NULL_TREE;

    set_input_location(s->loc);

    // Collect all arguments, which may be input or output operands.
    if (s->args)
      {
	for (size_t i = 0; i < s->args->dim; i++)
	  {
	    Identifier *name = (*s->names)[i];
	    StringExp *constr = (StringExp *)(*s->constraints)[i];
	    Expression *arg = (*s->args)[i];

	    tree id = name ? build_string(name->len, name->string) : NULL_TREE;
	    tree str = build_string(constr->len, (char *)constr->string);
	    tree val = arg->toElem(this->irs_);

	    if (i < s->outputargs)
	      {
		tree arg = build_tree_list(id, str);
		outputs = chainon(outputs, build_tree_list(arg, val));
	      }
	    else
	      {
		tree arg = build_tree_list(id, str);
		inputs = chainon(inputs, build_tree_list(arg, val));
	      }
	  }
      }

    // Collect all clobber arguments.
    if (s->clobbers)
      {
	for (size_t i = 0; i < s->clobbers->dim; i++)
	  {
	    StringExp *clobber = (StringExp *)(*s->clobbers)[i];
	    tree val = build_string(clobber->len, (char *)clobber->string);
	    clobbers = chainon(clobbers, build_tree_list(0, val));
	  }
      }

    // Collect all goto labels, these should have been already checked
    // by the front-end, so pass down the label symbol to the backend.
    if (s->labels)
      {
	for (size_t i = 0; i < s->labels->dim; i++)
	  {
	    Identifier *ident = (*s->labels)[i];
	    GotoStatement *gs = (*s->gotos)[i];

	    gcc_assert(gs->label->statement != NULL);
	    gcc_assert(gs->tf == gs->label->statement->tf);

	    tree name = build_string(ident->len, ident->string);
	    tree label = this->get_label_tree(gs->label);

	    labels = chainon(labels, build_tree_list(name, label));
	  }
      }

    // Should also do some extra validation on all input and output operands.
    tree string = build_string(insn->len, (char *)insn->string);
    string = resolve_asm_operand_names(string, outputs, inputs, labels);

    tree exp = build5(ASM_EXPR, void_type_node, string,
		      outputs, inputs, clobbers, labels);
    SET_EXPR_LOCATION (exp, input_location);

    // If the extended syntax was not used, mark the ASM_EXPR.
    if (s->args == NULL && s->clobbers == NULL)
      ASM_INPUT_P (exp) = 1;

    // Asm statements without outputs are treated as volatile.
    ASM_VOLATILE_P (exp) = (s->outputargs == 0);

    add_stmt(exp);
  }

  //
  void visit(ImportStatement *s)
  {
    if (s->imports == NULL)
      return;

    for (size_t i = 0; i < s->imports->dim; i++)
      {
	Dsymbol *dsym = (*s->imports)[i];

	if (dsym != NULL)
	  dsym->toObjFile(0);
      }
  }
};


// Main entry point for the IRVisitor interface to generate code for the
// statement AST class S.  IRS holds the state of the current function.

void
build_ir(FuncDeclaration *fd, IRState *irs)
{
  IRVisitor v = IRVisitor(fd, irs);
  fd->fbody->accept(&v);
}


