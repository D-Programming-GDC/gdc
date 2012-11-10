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

// Construct a new Symbol.

Symbol::Symbol (void)
{
  this->Sident = 0;
  this->prettyIdent = 0;
  this->Sclass = SC_INVALID;
  this->Sfl = FL_INVALID;
  this->Sseg = INVALID;
  this->Sflags = 0;

  this->Sdt = 0;
  this->Salignment = 0;

  this->Stree = NULL_TREE;
  this->ScontextDecl = NULL_TREE;
  this->SframeField = NULL_TREE;

  this->outputStage = NotStarted;
  this->frameInfo = NULL;
}

// Create a static symbol given the name STRING.

Symbol *
symbol_calloc (const char *string)
{
  // Need to dup the string because sometimes the string is alloca()'d
  Symbol *s = new Symbol;
  s->Sident = xstrdup (string);
  return s;
}

// Create a static symbol we can hang DT initializers onto.

Symbol *
static_sym (void)
{
  Symbol *s = new Symbol;
  return s;
}

