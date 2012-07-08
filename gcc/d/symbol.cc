// symbol.cc -- D frontend for GCC.
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
#include "symbol.h"

Symbol::Symbol ()
{
  this->Sident = 0;
  this->prettyIdent = 0;
  this->Sclass = SC_INVALID;
  this->Sfl = FL_INVALID;
  this->Sseg = INVALID;
  this->Sflags = 0;

  this->Sdt = 0;

  this->Stree = NULL_TREE;
  this->ScontextDecl = 0;
  this->SframeField = 0;

  this->outputStage = NotStarted;
  this->frameInfo = NULL;
}

Symbol *
symbol_calloc (const char *string)
{
  // Need to dup the string because sometimes the string is alloca()'d
  Symbol *s = new Symbol;
  s->Sident = xstrdup (string);
  return s;
}

Symbol *
symbol_name (const char *id, int , TYPE *)
{
  // %% Nothing special, just do the same as symbol_calloc
  // we don't even bother using sclass and t
  return symbol_calloc (id);
}

Symbol *
struct_calloc ()
{
  return new Symbol;
}

Symbol *
symbol_generate (SymbolStorageClass, TYPE *)
{
  return 0;
}

Thunk::Thunk ()
{
  offset = 0;
  symbol = 0;
}

void
symbol_func (Symbol *)
{
}

Symbol *
symbol_tree (tree t)
{
  Symbol *s = new Symbol;
  s->Stree = t;
  return s;
}

