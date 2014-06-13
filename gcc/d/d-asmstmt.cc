// d-asmstmt.cc -- D frontend for GCC.
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
#include "statement.h"
#include "scope.h"
#include "id.h"

#include "d-lang.h"
#include "d-codegen.h"


// Semantically analyze AsmStatement where SC is the scope.

Statement *
AsmStatement::semantic (Scope *sc)
{
  sc->func->hasReturnExp |= 8;
  return Statement::semantic (sc);
}

// Construct an ExtAsmStatement, whose components are an INSN,
// some ARGS, a list of all NAMES used, and their CONSTRAINTS,
// the number of OUTPUTARGS, and the list of CLOBBERS.

ExtAsmStatement::ExtAsmStatement (Loc loc, Expression *insn,
				  Expressions *args, Identifiers *names,
				  Expressions *constraints, int outputargs,
				  Expressions *clobbers)
    : Statement (loc)
{
  this->insn = insn;
  this->args = args;
  this->names = names;
  this->constraints = constraints;
  this->outputargs = outputargs;
  this->clobbers = clobbers;
}

// Create a copy of ExtAsmStatement.

Statement *
ExtAsmStatement::syntaxCopy (void)
{
  Expression *insn = this->insn->syntaxCopy();
  Expressions *args = NULL;
  Expressions *constraints = NULL;
  Expressions *clobbers = NULL;

  args = Expression::arraySyntaxCopy (this->args);
  constraints = Expression::arraySyntaxCopy (this->constraints);
  clobbers = Expression::arraySyntaxCopy (this->clobbers);

  return new ExtAsmStatement (this->loc, insn, args, this->names, constraints,
			      this->outputargs, clobbers);
}

// Semantically analyze ExtAsmStatement where SC is the scope of the statment.
// Determines types, fold constants, etc.

Statement *
ExtAsmStatement::semantic (Scope *sc)
{
  this->insn->semantic (sc);
  this->insn->optimize (WANTvalue);

  if (sc->func)
    {
      if (sc->func->setUnsafe())
	error ("extended assembler not allowed in @safe function %s",
	       sc->func->toChars());
    }

  if (this->insn->op != TOKstring || ((StringExp *) this->insn)->sz != 1)
    error ("instruction template must be a constant char string");

  if (this->args)
    {
      for (size_t i = 0; i < this->args->dim; i++)
	{
	  Expression *e = (*this->args)[i];
	  e = e->semantic (sc);
	  if (i < this->outputargs)
	    e = e->modifiableLvalue (sc, NULL);
	  else
	    e = e->optimize (WANTvalue);
	  (*this->args)[i] = e;

	  e = (*this->constraints)[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *) e)->sz != 1)
	    error ("constraint must be a constant char string");
	  (*this->constraints)[i] = e;
	}
    }

  if (this->clobbers)
    {
      for (size_t i = 0; i < this->clobbers->dim; i++)
	{
	  Expression *e = (*this->clobbers)[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *) e)->sz != 1)
	    error ("clobber specification must be a constant char string");
	  (*this->clobbers)[i] = e;
	}
    }

  return this;
}

// Write C-style representation of ExtAsmStatement to BUF.

void
ExtAsmStatement::toCBuffer (OutBuffer *buf, HdrGenState *hgs ATTRIBUTE_UNUSED)
{
  buf->writestring ("gcc asm { ");

  if (this->insn)
    buf->writestring (this->insn->toChars());

  buf->writestring (" : ");

  if (this->args)
    {
      for (size_t i = 0; i < this->args->dim; i++)
	{
	  Identifier *name = (*this->names)[i];
	  Expression *constr = (*this->constraints)[i];
	  Expression *arg = (*this->args)[i];

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
	    buf->writestring (arg->toChars());

	  if (i < this->outputargs - 1)
	    buf->writestring (", ");
	  else if (i == this->outputargs - 1)
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
	  Expression *clobber = (*this->clobbers)[i];
	  buf->writestring (clobber->toChars());
	  if (i < this->clobbers->dim - 1)
	    buf->writestring (", ");
	}
    }

  buf->writestring ("; }");
  buf->writenl();
}

// TRUE if statement 'comes from' somewhere else, like a goto.

bool
ExtAsmStatement::comeFromImpl()
{
  return true;
}

// Return how an ExtAsmStatement exits.

int
ExtAsmStatement::blockExit (bool)
{
  // Assume the worst
  return BEany;
}

