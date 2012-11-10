// asmstmt.cc -- D frontend for GCC.
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
#include "statement.h"
#include "scope.h"
#include "id.h"

#include "d-lang.h"
#include "d-codegen.h"


// Build an asm-statement, whose components are a INSN_TMPL, some
// OUTPUTS, some INPUTS and some CLOBBERS.

tree
d_build_asm_stmt (tree insn_tmpl, tree outputs, tree inputs, tree clobbers)
{
  tree t = build5 (ASM_EXPR, void_type_node, insn_tmpl,
		   outputs, inputs, clobbers, NULL_TREE);
  TREE_SIDE_EFFECTS (t) = 1;
  SET_EXPR_LOCATION (t, input_location);
  return t;
}

// Semantically analyze AsmStatement where SC is the scope.

Statement *
AsmStatement::semantic (Scope *sc)
{
  sc->func->hasReturnExp |= 8;
  return Statement::semantic (sc);
}

// Build the intermediate representation of an AsmStatment.
// Not used in GDC.

void
AsmStatement::toIR (IRState *)
{
  sorry ("D inline assembler statements are not supported in GDC.");
}

// Construct an ExtAsmStatement, whose components are an INSNTEMPATE,
// some ARGS, a list of all ARGNAMES used, and their ARGCONSTRAINTS,
// the number of NOUTPUTARGS, and the list of CLOBBERS.

ExtAsmStatement::ExtAsmStatement (Loc loc, Expression *insnTemplate,
				  Expressions *args, Identifiers *argNames,
				  Expressions *argConstraints, int nOutputArgs,
				  Expressions *clobbers)
    : Statement (loc)
{
  this->insnTemplate = insnTemplate;
  this->args = args;
  this->argNames = argNames;
  this->argConstraints = argConstraints;
  this->nOutputArgs = nOutputArgs;
  this->clobbers = clobbers;
}

// Create a copy of ExtAsmStatement.

Statement *
ExtAsmStatement::syntaxCopy (void)
{
  Expression *insnTemplate = this->insnTemplate->syntaxCopy();
  Expressions *args = Expression::arraySyntaxCopy (this->args);
  Expressions *argConstraints
    = Expression::arraySyntaxCopy (this->argConstraints);
  Expressions *clobbers = Expression::arraySyntaxCopy (this->clobbers);

  return new ExtAsmStatement (this->loc, insnTemplate, args, this->argNames,
			      argConstraints, this->nOutputArgs, clobbers);
}

// Semantically analyze ExtAsmStatement where SC is the scope of the statment.
// Determines types, fold constants, etc.

Statement *
ExtAsmStatement::semantic (Scope *sc)
{
  this->insnTemplate->semantic (sc);
  this->insnTemplate->optimize (WANTvalue);

  if (sc->func)
    {
      if (sc->func->setUnsafe())
	error ("extended assembler not allowed in @safe function %s",
	       sc->func->toChars());
    }

  if (this->insnTemplate->op != TOKstring
      || ((StringExp *)this->insnTemplate)->sz != 1)
    error ("instruction template must be a constant char string");

  if (this->args)
    {
      for (size_t i = 0; i < this->args->dim; i++)
	{
	  Expression *e = this->args->tdata()[i];
	  e = e->semantic (sc);
	  if (i < this->nOutputArgs)
	    e = e->modifiableLvalue (sc, NULL);
	  else
	    e = e->optimize (WANTvalue);
	  this->args->tdata()[i] = e;

	  e = this->argConstraints->tdata()[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
	    error ("constraint must be a constant char string");
	  this->argConstraints->tdata()[i] = e;
	}
    }
  if (this->clobbers)
    {
      for (size_t i = 0; i < this->clobbers->dim; i++)
	{
	  Expression *e = this->clobbers->tdata()[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
	    error ("clobber specification must be a constant char string");
	  this->clobbers->tdata()[i] = e;
	}
    }
  return this;
}

// Write C-style representation of ExtAsmStatement to BUF.

void
ExtAsmStatement::toCBuffer (OutBuffer *buf, HdrGenState *hgs ATTRIBUTE_UNUSED)
{
  buf->writestring ("gcc asm { ");
  if (this->insnTemplate)
    buf->writestring (this->insnTemplate->toChars());
  buf->writestring (" : ");
  if (this->args)
    {
      for (size_t i = 0; i < this->args->dim; i++)
	{
	  Identifier *name = this->argNames->tdata()[i];
	  Expression *constr = this->argConstraints->tdata()[i];
	  Expression *arg = this->args->tdata()[i];

	  if (name)
	    {
	      buf->writestring ("[");
	      buf->writestring (name->toChars());
	      buf->writestring ("] ");
	    }
	  if (constr)
	    {
	      buf->writestring (constr->toChars());
	      buf->writestring (" ");
	    }
	  if (arg)
	    {
	      buf->writestring (arg->toChars());
	    }

	  if (i < this->nOutputArgs - 1)
	    buf->writestring (", ");
	  else if (i == this->nOutputArgs - 1)
	    buf->writestring (" : ");
	  else if (i < this->args->dim - 1)
	    buf->writestring (", ");
	}
    }
  if (this->clobbers)
    {
      buf->writestring (" : ");
      for (size_t i = 0; i < this->clobbers->dim; i++)
	{
	  Expression *clobber = this->clobbers->tdata()[i];
	  buf->writestring (clobber->toChars());
	  if (i < this->clobbers->dim - 1)
	    buf->writestring (", ");
	}
    }
  buf->writestring ("; }");
  buf->writenl();
}

// Return how an ExtAsmStatement exits.
// If MUSTNOTTHROW is true, generate an error if it throws.

int
ExtAsmStatement::blockExit (bool mustNotThrow)
{
  if (mustNotThrow)
    error ("asm statements are assumed to throw", toChars());
  // Assume the worst
  return BEany;
}

// Return a STRING_CST node whose value is E.

static tree
naturalString (Expression *e)
{
  // StringExp::toIR usually adds a NULL.  We don't want that...
  // don't fail, just an error?
  gcc_assert (e->op == TOKstring);
  StringExp *s = (StringExp *) e;
  gcc_assert (s->sz == 1);
  return build_string (s->len, (char *) s->string);
}

// Build the intermediate representation of an ExtAsmStatment where IRS
// holds the state of the current function.

void
ExtAsmStatement::toIR (IRState *irs)
{
  ListMaker outputs;
  ListMaker inputs;
  ListMaker tree_clobbers;

  irs->doLineNote (this->loc);

  if (this->args)
    {
      for (size_t i = 0; i < this->args->dim; i++)
	{
	  Identifier *name = this->argNames->tdata()[i];
	  Expression *constr = this->argConstraints->tdata()[i];
	  tree n = name ? build_string (name->len, name->string) : NULL_TREE;
	  tree p = tree_cons (n, naturalString (constr), NULL_TREE);
	  tree v = (this->args->tdata()[i])->toElem (irs);

	  if (i < this->nOutputArgs)
	    outputs.cons (p, v);
	  else
	    inputs.cons (p, v);
	}
    }
  if (this->clobbers)
    {
      for (size_t i = 0; i < this->clobbers->dim; i++)
	{
	  Expression *clobber = this->clobbers->tdata()[i];
	  tree_clobbers.cons (NULL_TREE, naturalString (clobber));
	}
    }
  irs->doAsm (naturalString (this->insnTemplate), outputs.head,
	      inputs.head, tree_clobbers.head);
}

