// d-ir.cc -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

#include "d-system.h"

#include "id.h"
#include "enum.h"
#include "module.h"
#include "init.h"
#include "d-lang.h"
#include "d-codegen.h"


void
Statement::toIR (IRState *)
{
  ::error ("Statement::toIR: don't know what to do (%s)", toChars());
  gcc_unreachable();
}

void
LabelStatement::toIR (IRState *irs)
{
  FuncDeclaration *func = irs->func;
  LabelDsymbol *label = irs->isReturnLabel (ident) ? func->returnLabel : func->searchLabel (ident);
  tree t_label = irs->getLabelTree (label);

  if (t_label != NULL_TREE)
    {
      irs->pushLabel (label);
      irs->doLabel (t_label);
      if (irs->isReturnLabel (ident) && func->fensure)
	func->fensure->toIR (irs);
      else if (statement)
	statement->toIR (irs);
      if (fwdrefs)
	{
	  irs->checkPreviousGoto (fwdrefs);
	  delete fwdrefs;
	  fwdrefs = NULL;
	}
    }
  // else, there was an error
}

void
GotoStatement::toIR (IRState *irs)
{
  tree t_label;

  /* This makes the 'undefined label' error show up on the correct line...
     The extra doLineNote in doJump shouldn't cause a problem.  */
  irs->doLineNote (loc);

  if (!label->statement)
    error ("label %s is undefined", label->toChars());
  else if (tf != label->statement->tf)
    error ("cannot goto forward out of or into finally block");
  else
    irs->checkGoto (this, label);

  t_label = irs->getLabelTree (label);

  if (t_label != NULL_TREE)
    irs->doJump (this, t_label);
  // else, there was an error
}

void
GotoCaseStatement::toIR (IRState *irs)
{
  // assumes cblocks have been set in SwitchStatement::toIR
  irs->doJump (this, cs->cblock);
}

void
GotoDefaultStatement::toIR (IRState *irs)
{
  // assumes cblocks have been set in SwitchStatement::toIR
  irs->doJump (this, sw->sdefault->cblock);
}

void
SwitchErrorStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->addExp (d_assert_call (loc, LIBCALL_SWITCH_ERROR));
}

void
ThrowStatement::toIR (IRState *irs)
{
  ClassDeclaration *class_decl = exp->type->toBasetype()->isClassHandle();
  // Front end already checks for isClassHandle
  InterfaceDeclaration *intfc_decl = class_decl->isInterfaceDeclaration();
  tree arg = exp->toElemDtor (irs);

  if (!flag_exceptions)
    {
      static int warned = 0;
      if (!warned)
	{
	  error ("exception handling disabled, use -fexceptions to enable");
	  warned = 1;
	}
    }

  if (intfc_decl)
    {
      if (!intfc_decl->isCOMclass())
	arg = convert_expr (arg, exp->type, build_object_type());
      else
	error ("cannot throw COM interfaces");
    }
  irs->doLineNote (loc);
  irs->addExp (build_libcall (LIBCALL_THROW, 1, &arg));
}

void
TryFinallyStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->startTry (this);
  if (body)
    body->toIR (irs);

  irs->startFinally();
  if (finalbody)
    finalbody->toIR (irs);

  irs->endFinally();
}

void
TryCatchStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->startTry (this);
  if (body)
    body->toIR (irs);

  irs->startCatches();
  if (catches)
    {
      for (size_t i = 0; i < catches->dim; i++)
	{
	  Catch *a_catch = (*catches)[i];

	  irs->startCatch (a_catch->type->toCtype());
	  irs->doLineNote (a_catch->loc);
	  irs->startScope();

	  if (a_catch->var)
	    {
	      tree exc_obj = convert_expr (build_exception_object(),
					   build_object_type(), a_catch->type);
	      tree catch_var = a_catch->var->toSymbol()->Stree;
	      // need to override initializer...
	      // set DECL_INITIAL now and emitLocalVar will know not to change it
	      DECL_INITIAL (catch_var) = exc_obj;
	      irs->emitLocalVar (a_catch->var, false);
	    }

	  if (a_catch->handler)
	    a_catch->handler->toIR (irs);
	  irs->endScope();
	  irs->endCatch();
	}
    }
  irs->endCatches();
}

void
OnScopeStatement::toIR (IRState *)
{
}

void
WithStatement::toIR (IRState *irs)
{
  irs->startScope();
  if (wthis)
    irs->emitLocalVar (wthis, false);

  if (body)
    body->toIR (irs);

  irs->endScope();
}

void
SynchronizedStatement::toIR (IRState *)
{
  ::error ("SynchronizedStatement::toIR: we shouldn't emit this (%s)", toChars());
  gcc_unreachable();
}

void
ContinueStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->continueLoop (ident);
}

void
BreakStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->exitLoop (ident);
}

void
ReturnStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);

  if (exp == NULL || exp->type->toBasetype()->ty == Tvoid)
    {
      // Return has no value.
      irs->doReturn (NULL_TREE);
      return;
    }

  FuncDeclaration *func = irs->func;
  TypeFunction *tf = (TypeFunction *) func->type;
  Type *ret_type = func->tintro ? func->tintro->nextOf() : tf->nextOf();

  if (func->isMain() && ret_type->toBasetype()->ty == Tvoid)
    ret_type = Type::tint32;

  tree result_decl = DECL_RESULT (irs->func->toSymbol()->Stree);

  if (func->nrvo_can && func->nrvo_var)
    {
      // Just refer to the RESULT_DECL; this is a nop, but differs from using
      // NULL_TREE in that it indicates that we care about the value of the RESULT_DECL.
      irs->doReturn (result_decl);
    }
  else
    {
      tree result_value = convert_expr (exp->toElemDtor (irs), exp->type, ret_type);
      // %% convert for init -- if we were returning a reference,
      // would want to take the address...
      if (tf->isref)
	result_value = build_address (result_value);

      tree result_assign = build2 (INIT_EXPR, TREE_TYPE (result_decl),
				   result_decl, result_value);

      irs->doReturn (result_assign);
    }
}

void
DefaultStatement::toIR (IRState *irs)
{
  irs->checkSwitchCase (this, 1);
  irs->doCase (NULL_TREE, cblock);
  if (statement)
    statement->toIR (irs);
}

void
CaseStatement::toIR (IRState *irs)
{
  tree case_value;

  if (exp->type->isscalar())
    case_value = exp->toElem (irs);
  else
    case_value = build_integer_cst (index, Type::tint32->toCtype());

  irs->checkSwitchCase (this);
  irs->doCase (case_value, cblock);
  if (statement)
    statement->toIR (irs);
}

void
SwitchStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);

  tree cond_tree = condition->toElemDtor (irs);
  Type *cond_type = condition->type->toBasetype();

  if (condition->type->isString())
    {
      Type *elem_type = cond_type->nextOf()->toBasetype();
      LibCall libcall;
      switch (elem_type->ty)
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
	  ::error ("switch statement value must be an array of some character type, not %s",
		   elem_type->toChars());
	  gcc_unreachable();
	}

      // Apparently the backend is supposed to sort and set the indexes
      // on the case array, have to change them to be useable.
      cases->sort();

      tree args[2];
      Symbol *s = new Symbol();
      dt_t **pdt = &s->Sdt;

      for (size_t i = 0; i < cases->dim; i++)
	{
	  CaseStatement *cs = (*cases)[i];
	  cs->index = i;

	  if (cs->exp->op != TOKstring)
	    error("case '%s' is not a string", cs->exp->toChars());
	  else
	    pdt = cs->exp->toDt (pdt);
	}

      s->Sreadonly = true;
      d_finish_symbol (s);

      args[0] = d_array_value (cond_type->arrayOf()->toCtype(),
			       size_int (cases->dim),
			       build_address (s->Stree));
      args[1] = cond_tree;

      cond_tree = build_libcall (libcall, 2, args);
    }
  else if (!cond_type->isscalar())
    {
      ::error ("cannot handle switch condition of type %s", cond_type->toChars());
      gcc_unreachable();
    }

  if (cases)
    {
      // Build LABEL_DECLs now so they can be refered to by goto case
      for (size_t i = 0; i < cases->dim; i++)
	{
	  CaseStatement *case_stmt = (*cases)[i];
	  case_stmt->cblock = d_build_label (case_stmt->loc, NULL);
	}
      if (sdefault)
	sdefault->cblock = d_build_label (sdefault->loc, NULL);
    }
  cond_tree = fold (cond_tree);

  if (hasVars)
    {
      // Write cases as a series of if-then-else blocks.
      for (size_t i = 0; i < cases->dim; i++)
	{
	  CaseStatement *case_stmt = (*cases)[i];
	  tree case_cond = build2 (EQ_EXPR, cond_type->toCtype(), cond_tree,
				   case_stmt->exp->toElemDtor (irs));
	  irs->startCond (this, case_cond);
	  irs->doJump (NULL, case_stmt->cblock);
	  irs->endCond();
	}
      if (sdefault)
	irs->doJump (NULL, sdefault->cblock);
    }

  // Emit body.
  irs->startCase (this, cond_tree, hasVars);
  if (body)
    body->toIR (irs);
  irs->endCase();
}


void
IfStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->startScope();
  irs->startCond (this, convert_for_condition (condition->toElemDtor (irs),
					       condition->type));
  if (ifbody)
    ifbody->toIR (irs);

  if (elsebody)
    {
      irs->startElse();
      elsebody->toIR (irs);
    }
  irs->endCond();
  irs->endScope();
}

void
ForeachStatement::toIR (IRState *)
{
  ::error ("ForeachStatement::toIR: we shouldn't emit this (%s)", toChars());
  gcc_unreachable();
}

void
ForeachRangeStatement::toIR (IRState *)
{
  ::error ("ForeachRangeStatement::toIR: we shouldn't emit this (%s)", toChars());
  gcc_unreachable();
}

void
ForStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  if (init)
    init->toIR (irs);
  irs->startLoop (this);
  if (condition)
    {
      irs->doLineNote (condition->loc);
      irs->exitIfFalse (convert_for_condition (condition->toElemDtor (irs),
					       condition->type));
    }
  if (body)
    body->toIR (irs);
  irs->continueHere();
  if (increment)
    {
      // force side effects?
      irs->doLineNote (increment->loc);
      irs->addExp (increment->toElemDtor (irs));
    }
  irs->endLoop();
}

void
DoStatement::toIR (IRState *irs)
{
  irs->doLineNote (loc);
  irs->startLoop (this);
  if (body)
    body->toIR (irs);
  irs->continueHere();
  irs->doLineNote (condition->loc);
  irs->exitIfFalse (convert_for_condition (condition->toElemDtor (irs),
					   condition->type));
  irs->endLoop();
}

void
WhileStatement::toIR (IRState *)
{
  ::error ("WhileStatement::toIR: we shouldn't emit this (%s)", toChars());
  gcc_unreachable();
}

void
ScopeStatement::toIR (IRState *irs)
{
  if (statement)
    {
      irs->startScope();
      statement->toIR (irs);
      irs->endScope();
    }
}

void
CompoundStatement::toIR (IRState *irs)
{
  if (!statements)
    return;

  for (size_t i = 0; i < statements->dim; i++)
    {
      Statement *statement = (*statements)[i];
      if (statement)
	statement->toIR (irs);
    }
}

void
UnrolledLoopStatement::toIR (IRState *irs)
{
  if (!statements)
    return;

  irs->startLoop (this);
  irs->continueHere();
  for (size_t i = 0; i < statements->dim; i++)
    {
      Statement *statement = (*statements)[i];
      if (statement)
	{
	  irs->setContinueLabel (d_build_label (loc, NULL));
	  statement->toIR (irs);
	  irs->continueHere();
	}
    }
  irs->exitLoop (NULL);
  irs->endLoop();
}

void
ExpStatement::toIR (IRState *irs)
{
  if (exp)
    {
      irs->doLineNote (loc);
      tree exp_tree = exp->toElemDtor (irs);
      irs->addExp (exp_tree);
    }
}

void
DtorExpStatement::toIR (IRState *irs)
{
  FuncDeclaration *fd = irs->func;

  /* Do not call destructor if var is returned as the
     nrvo variable.  */
  bool noDtor = (fd->nrvo_can && fd->nrvo_var == var);

  if (!noDtor)
    ExpStatement::toIR (irs);
}

void
PragmaStatement::toIR (IRState *)
{
}

void
ImportStatement::toIR (IRState *)
{
}

