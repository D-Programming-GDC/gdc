// toir.cc -- D frontend for GCC.
// Copyright (C) 2011-2014 Free Software Foundation, Inc.

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

#include "id.h"
#include "enum.h"
#include "module.h"
#include "init.h"
#include "visitor.h"

#include "d-lang.h"
#include "d-codegen.h"


// Implements the visitor interface to build the GCC trees of all Statement
// AST classes emitted from the D Front-end, where IRS_ holds the state of
// the current function being compiled.
// All visit methods accept one parameter S, which holds the frontend AST
// of the statement to compile.  They also don't return any value, instead
// generated code are pushed to IRState::addExp(), which appends them to
// the statement list in the current_binding_level.

class IRVisitor : public Visitor
{
  IRState *irs_;

public:
  IRVisitor(IRState *irs) : irs_(irs) {}

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
    this->irs_->doLineNote(s->loc);
    this->irs_->startScope();

    // Build the outer 'if' condition, which may produce temporaries
    // requiring scope destruction.
    tree ifcond = convert_for_condition(s->condition->toElemDtor(this->irs_),
					s->condition->type);
    this->irs_->startCond(s, ifcond);

    if (s->ifbody)
      build_ir(s->ifbody, this->irs_);

    // Now build the 'else' branch, which may have nested 'else if' parts.
    if (s->elsebody)
      {
	this->irs_->startElse();
	build_ir(s->elsebody, this->irs_);
      }

    this->irs_->endCond();
    this->irs_->endScope();
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
    this->irs_->doLineNote(s->loc);
    this->irs_->startLoop(s);

    if (s->body)
      build_ir(s->body, this->irs_);

    this->irs_->continueHere();

    // Build the outer 'while' condition, which may produce temporaries
    // requiring scope destruction.
    this->irs_->doLineNote(s->condition->loc);
    tree exitcond = convert_for_condition(s->condition->toElemDtor(this->irs_),
					  s->condition->type);
    this->irs_->exitIfFalse(exitcond);

    this->irs_->endLoop();
  }

  // for(...) { ... }
  void visit(ForStatement *s)
  {
    this->irs_->doLineNote(s->loc);

    if (s->init)
      build_ir(s->init, this->irs_);

    this->irs_->startLoop(s);

    if (s->condition)
      {
	this->irs_->doLineNote(s->condition->loc);
	tree exitcond = convert_for_condition(s->condition->toElemDtor(this->irs_),
					      s->condition->type);
	this->irs_->exitIfFalse(exitcond);
      }

    if (s->body)
      build_ir(s->body, this->irs_);

    this->irs_->continueHere();

    if (s->increment)
      {
	// Force side effects?
	this->irs_->doLineNote(s->increment->loc);
	this->irs_->addExp(s->increment->toElemDtor(this->irs_));
      }

    this->irs_->endLoop();
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
    this->irs_->doLineNote(s->loc);
    this->irs_->exitLoop(s->ident);
  }

  // Jump to the associated continue label for the current loop.
  // If IDENT for the Statement is not null, then the label is user defined.
  void visit(ContinueStatement *s)
  {
    this->irs_->doLineNote(s->loc);
    this->irs_->continueLoop(s->ident);
  }

  //
  void visit(GotoStatement *s)
  {
    gcc_assert(s->label->statement != NULL);
    gcc_assert(s->tf == s->label->statement->tf);

    // This makes the 'undefined label' error show up on the correct line.
    // The extra doLineNote in doJump shouldn't cause a problem.
    this->irs_->doLineNote(s->loc);
    this->irs_->checkGoto(s, s->label);

    // If no label found, there was an error.
    tree label = this->irs_->getLabelTree(s->label);

    if (label != NULL_TREE)
      this->irs_->doJump(s, label);
  }

  //
  void visit(LabelStatement *s)
  {
    FuncDeclaration *fd = this->irs_->func;
    LabelDsymbol *sym = this->irs_->isReturnLabel(s->ident)
      ? fd->returnLabel : fd->searchLabel(s->ident);

    // If no label found, there was an error
    tree label = this->irs_->getLabelTree(sym);

    if (label != NULL_TREE)
      {
	this->irs_->pushLabel(sym);
	this->irs_->doLabel(label);

	if (this->irs_->isReturnLabel(s->ident) && fd->fensure != NULL)
	  build_ir(fd->fensure, this->irs_);
	else if (s->statement)
	  build_ir(s->statement, this->irs_);

	if (s->fwdrefs)
	  {
	    this->irs_->checkPreviousGoto(s->fwdrefs);
	    s->fwdrefs = NULL;
	  }
      }
  }

  //
  void visit(SwitchStatement *s)
  {
    this->irs_->doLineNote(s->loc);

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

	args[0] = d_array_value(condtype->arrayOf()->toCtype(),
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
	    tree case_cond = build2(EQ_EXPR, condtype->toCtype(), condition,
				    cs->exp->toElemDtor(this->irs_));
	    this->irs_->startCond(s, case_cond);
	    this->irs_->doJump(NULL, cs->cblock);
	    this->irs_->endCond();
	  }

	if (s->sdefault)
	  this->irs_->doJump(NULL, s->sdefault->cblock);
      }

    // Emit body.
    this->irs_->startCase(s, condition, s->hasVars);

    if (s->body)
      build_ir(s->body, this->irs_);

    this->irs_->endCase();
  }

  //
  void visit(CaseStatement *s)
  {
    tree caseval;

    if (s->exp->type->isscalar())
      caseval = s->exp->toElem(this->irs_);
    else
      caseval = build_integer_cst(s->index, Type::tint32->toCtype());

    this->irs_->checkSwitchCase(s);
    this->irs_->doCase(caseval, s->cblock);

    if (s->statement)
      build_ir(s->statement, this->irs_);
  }

  //
  void visit(DefaultStatement *s)
  {
    this->irs_->checkSwitchCase(s, 1);
    this->irs_->doCase(NULL_TREE, s->cblock);

    if (s->statement)
      build_ir(s->statement, this->irs_);
  }

  // Implements 'goto default' by jumping to the label associated with
  // the DefaultStatement in a switch block.
  // Assumes CBLOCK has been set in SwitchStatement visitor.
  void visit(GotoDefaultStatement *s)
  {
    this->irs_->doJump(s, s->sw->sdefault->cblock);
  }

  // Implements 'goto case' by jumping to the label associated with the
  // CaseStatement in a switch block.
  // Assumes CBLOCK has been set in SwitchStatement visitor.
  void visit(GotoCaseStatement *s)
  {
    this->irs_->doJump(s, s->cs->cblock);
  }

  // Throw a SwitchError exception, called when a switch statement has
  // no DefaultStatement, yet none of the cases match.
  void visit(SwitchErrorStatement *s)
  {
    this->irs_->doLineNote(s->loc);
    this->irs_->addExp(d_assert_call(s->loc, LIBCALL_SWITCH_ERROR));
  }

  //
  void visit(ReturnStatement *s)
  {
    this->irs_->doLineNote(s->loc);

    if (s->exp == NULL || s->exp->type->toBasetype()->ty == Tvoid)
      {
	// Return has no value.
	this->irs_->doReturn(NULL_TREE);
	return;
      }

    FuncDeclaration *fd = this->irs_->func;
    TypeFunction *tf = (TypeFunction *)fd->type;
    Type *type = fd->tintro ? fd->tintro->nextOf() : tf->nextOf();

    if (fd->isMain() && type->toBasetype()->ty == Tvoid)
      type = Type::tint32;

    tree decl = DECL_RESULT(fd->toSymbol()->Stree);

    if (fd->nrvo_can && fd->nrvo_var)
      {
	// Just refer to the DECL_RESULT; this is a nop, but differs
	// from using NULL_TREE in that it indicates that we care about
	// the value of the DECL_RESULT.
	this->irs_->doReturn(decl);
      }
    else
      {
	// Convert for initialising the DECL_RESULT.
	tree value = convert_expr(s->exp->toElemDtor(this->irs_),
				  s->exp->type, type);

	// If we are returning a reference, take the address.
	if (tf->isref)
	  value = build_address(value);

	tree assign = build2(INIT_EXPR, TREE_TYPE(decl), decl, value);

	this->irs_->doReturn(assign);
      }
  }

  // 
  void visit(ExpStatement *s)
  {
    if (s->exp)
      {
	this->irs_->doLineNote(s->loc);
	// Expression may produce temporaries requiring scope destruction.
	tree exp = s->exp->toElemDtor(this->irs_);
	this->irs_->addExp(exp);
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
	  build_ir(statement, this->irs_);
      }
  }

  // The frontend lowers foreach(Tuple!(...)) statements as an unrolled loop.
  // These are compiled down as a do ... while(0), where each unrolled loop is
  // nested inside and given their own continue label to jump to.
  void visit(UnrolledLoopStatement *s)
  {
    if (s->statements == NULL)
      return;

    this->irs_->startLoop(s);
    this->irs_->continueHere();

    for (size_t i = 0; i < s->statements->dim; i++)
      {
	Statement *statement = (*s->statements)[i];

	if (statement != NULL)
	  {
	    this->irs_->setContinueLabel(d_build_label(s->loc, NULL));
	    build_ir(statement, this->irs_);
	    this->irs_->continueHere();
	  }
      }

    this->irs_->exitLoop(NULL);
    this->irs_->endLoop();
  }

  // Start a new scope and visit all nested statements, wrapping
  // them up into a BIND_EXPR at the end of the scope.
  void visit(ScopeStatement *s)
  {
    if (s->statement == NULL)
      return;

    this->irs_->startScope();
    build_ir(s->statement, this->irs_);
    this->irs_->endScope();
  }

  //
  void visit(WithStatement *s)
  {
    this->irs_->doLineNote(s->loc);
    this->irs_->startScope();

    if (s->wthis)
      {
	// Perform initialisation of the 'with' handle.
	ExpInitializer *ie = s->wthis->init->isExpInitializer();
	gcc_assert(ie != NULL);

	build_local_var(s->wthis, this->irs_->func);
	tree init = ie->exp->toElemDtor(this->irs_);
	this->irs_->addExp(init);
      }

    if (s->body)
      build_ir(s->body, this->irs_);

    this->irs_->endScope();
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

    this->irs_->doLineNote(s->loc);
    this->irs_->addExp(build_libcall(LIBCALL_THROW, 1, &arg));
  }

  //
  void visit(TryCatchStatement *s)
  {
    this->irs_->doLineNote(s->loc);
    this->irs_->startTry(s);

    if (s->body)
      build_ir(s->body, this->irs_);

    this->irs_->startCatches();

    if (s->catches)
      {
	for (size_t i = 0; i < s->catches->dim; i++)
	  {
	    Catch *vcatch = (*s->catches)[i];

	    this->irs_->startCatch(vcatch->type->toCtype());
	    this->irs_->doLineNote(vcatch->loc);
	    this->irs_->startScope();

	    if (vcatch->var)
	      {
		tree object = convert_expr(build_exception_object(),
					   build_object_type(), vcatch->type);
		tree var = vcatch->var->toSymbol()->Stree;
		tree init = build_vinit(var, object);

		build_local_var(vcatch->var, this->irs_->func);
		this->irs_->addExp(init);
	      }

	    if (vcatch->handler)
	      build_ir(vcatch->handler, this->irs_);

	    this->irs_->endScope();
	    this->irs_->endCatch();
	  }
      }

    this->irs_->endCatches();
  }

  //
  void visit(TryFinallyStatement *s)
  {
    this->irs_->doLineNote(s->loc);
    this->irs_->startTry(s);

    if (s->body)
      build_ir(s->body, this->irs_);

    this->irs_->startFinally();

    if (s->finalbody)
      build_ir(s->finalbody, this->irs_);

    this->irs_->endFinally();
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

    this->irs_->doLineNote(s->loc);

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
	    tree label = this->irs_->getLabelTree(gs->label);

	    labels = chainon(labels, build_tree_list(name, label));
	  }
      }

    tree exp = build5(ASM_EXPR, void_type_node,
		      build_string(insn->len, (char *)insn->string),
		      outputs, inputs, clobbers, labels);
    SET_EXPR_LOCATION(exp, input_location);

    // If the extended syntax was not used, mark the ASM_EXPR.
    if (s->args == NULL && s->clobbers == NULL)
      ASM_INPUT_P(exp) = 1;

    // Asm statements without outputs are treated as volatile.
    ASM_VOLATILE_P(exp) = (s->outputargs == 0);

    this->irs_->addExp(exp);
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
build_ir(Statement *s, IRState *irs)
{
  IRVisitor v = IRVisitor(irs);
  s->accept(&v);
}


