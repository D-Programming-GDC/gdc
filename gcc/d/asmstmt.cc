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


tree
d_build_asm_stmt (tree insn_tmpl, tree outputs, tree inputs, tree clobbers)
{
  tree t = build5 (ASM_EXPR, void_type_node, insn_tmpl,
		   outputs, inputs, clobbers, NULL_TREE);
  TREE_SIDE_EFFECTS (t) = 1;
  SET_EXPR_LOCATION (t, input_location);
  return t;
}

Statement *
AsmStatement::semantic (Scope *sc)
{
  sc->func->hasReturnExp |= 8;
  return Statement::semantic (sc);
}

void
AsmStatement::toIR (IRState *)
{
  sorry ("D inline assembler statements are not supported in GDC.");
}


ExtAsmStatement::ExtAsmStatement (Loc loc, Expression *insnTemplate, Expressions *args, Identifiers *argNames,
			      	  Expressions *argConstraints, int nOutputArgs, Expressions *clobbers)
    : Statement (loc)
{
  this->insnTemplate = insnTemplate;
  this->args = args;
  this->argNames = argNames;
  this->argConstraints = argConstraints;
  this->nOutputArgs = nOutputArgs;
  this->clobbers = clobbers;
}

Statement *
ExtAsmStatement::syntaxCopy ()
{
  /* insnTemplate, argConstraints, and clobbers would be
     semantically static in GNU C. */
  Expression *insnTemplate = this->insnTemplate->syntaxCopy ();
  Expressions * args = Expression::arraySyntaxCopy (this->args);
  // argNames is an array of identifiers
  Expressions * argConstraints = Expression::arraySyntaxCopy (this->argConstraints);
  Expressions * clobbers = Expression::arraySyntaxCopy (this->clobbers);
  return new ExtAsmStatement (loc, insnTemplate, args, argNames,
			      argConstraints, nOutputArgs, clobbers);
}

Statement *
ExtAsmStatement::semantic (Scope *sc)
{
  insnTemplate = insnTemplate->semantic (sc);
  insnTemplate = insnTemplate->optimize (WANTvalue);

  if (sc->func)
    {
      if (sc->func->setUnsafe ())
	error ("extended assembler not allowed in @safe function %s", sc->func->toChars ());
    }

  if (insnTemplate->op != TOKstring || ((StringExp *)insnTemplate)->sz != 1)
    error ("instruction template must be a constant char string");

  if (args)
    {
      for (size_t i = 0; i < args->dim; i++)
	{
	  Expression * e = args->tdata()[i];
	  e = e->semantic (sc);
	  if (i < nOutputArgs)
	    e = e->modifiableLvalue (sc, NULL);
	  else
	    e = e->optimize (WANTvalue);
	  args->tdata()[i] = e;

	  e = argConstraints->tdata()[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
	    error ("constraint must be a constant char string");
	  argConstraints->tdata()[i] = e;
	}
    }
  if (clobbers)
    {
      for (size_t i = 0; i < clobbers->dim; i++)
	{
	  Expression * e = clobbers->tdata()[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
	    error ("clobber specification must be a constant char string");
	  clobbers->tdata()[i] = e;
	}
    }
  return this;
}

void
ExtAsmStatement::toCBuffer (OutBuffer *buf)
{
  buf->writestring ("gcc asm { ");
  if (insnTemplate)
    buf->writestring (insnTemplate->toChars ());
  buf->writestring (" : ");
  if (args)
    {
      for (size_t i = 0; i < args->dim; i++)
	{
	  Identifier * name = argNames->tdata()[i];
	  Expression * constr = argConstraints->tdata()[i];
	  Expression * arg = args->tdata()[i];

	  if (name)
	    {
	      buf->writestring ("[");
	      buf->writestring (name->toChars ());
	      buf->writestring ("] ");
	    }
	  if (constr)
	    {
	      buf->writestring (constr->toChars ());
	      buf->writestring (" ");
	    }
	  if (arg)
	    {
	      buf->writestring (arg->toChars ());
	    }

	  if (i < nOutputArgs - 1)
	    buf->writestring (", ");
	  else if (i == nOutputArgs - 1)
	    buf->writestring (" : ");
	  else if (i < args->dim - 1)
	    buf->writestring (", ");
	}
    }
  if (clobbers)
    {
      buf->writestring (" : ");
      for (size_t i = 0; i < clobbers->dim; i++)
	{
	  Expression * clobber = clobbers->tdata()[i];
	  buf->writestring (clobber->toChars ());
	  if (i < clobbers->dim - 1)
	    buf->writestring (", ");
	}
    }
  buf->writestring ("; }");
  buf->writenl ();
}

int
ExtAsmStatement::blockExit (bool mustNotThrow)
{
  if (mustNotThrow)
    error ("asm statements are assumed to throw", toChars ());
  // TODO: Be smarter about this
  return BEany;
}


// StringExp::toIR usually adds a NULL.  We don't want that...

static tree
naturalString (Expression * e)
{
  // don't fail, just an error?
  gcc_assert (e->op == TOKstring);
  StringExp * s = (StringExp *) e;
  gcc_assert (s->sz == 1);
  return build_string (s->len, (char *) s->string);
}

void
ExtAsmStatement::toIR (IRState *irs)
{
  ListMaker outputs;
  ListMaker inputs;
  ListMaker tree_clobbers;

  gen.doLineNote (loc);

  if (args)
    {
      for (size_t i = 0; i < args->dim; i++)
	{
	  Identifier * name = argNames->tdata()[i];
	  Expression * constr = argConstraints->tdata()[i];
	  tree p = tree_cons (name ? build_string (name->len, name->string) : NULL_TREE,
	      		      naturalString (constr), NULL_TREE);
	  tree v = (args->tdata()[i])->toElem (irs);

	  if (i < nOutputArgs)
	    outputs.cons (p, v);
	  else
	    inputs.cons (p, v);
	}
    }
  if (clobbers)
    {
      for (size_t i = 0; i < clobbers->dim; i++)
	{
	  Expression * clobber = clobbers->tdata()[i];
	  tree_clobbers.cons (NULL_TREE, naturalString (clobber));
	}
    }
  irs->doAsm (naturalString (insnTemplate), outputs.head, inputs.head, tree_clobbers.head);
}

