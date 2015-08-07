// d-irstate.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_IRSTATE_H
#define GCC_DCMPLR_IRSTATE_H

// The kinds of levels we recognize.
enum LevelKind
{
  level_block = 0,    // An ordinary block scope.
  level_switch,       // A switch-block
  level_try,          // A try-block.
  level_catch,        // A catch-block.
  level_finally,      // A finally-block.
};


struct Label
{
  LabelDsymbol *label;
  Statement *block;
  Statement *from;
  LevelKind kind;
  unsigned level;

  Label()
    : label(NULL), block(NULL), from(NULL),
      kind(level_block), level(0)
  { }
};

struct Flow
{
  Statement *statement;
  LevelKind kind;
  tree exitLabel;
  union
  {
    struct
    {
      tree continueLabel;
      tree hasVars;         // D2 specific, != NULL_TREE if switch uses Lvalues for cases.
    };
    struct
    {
      tree condition;       // Only need this if it is not okay to convert an IfStatement's
      tree trueBranch;      // condition after converting it's branches...
    };
    struct
    {
      tree tryBody;
      tree catchType;
    };
  };

  Flow (Statement *stmt)
    : statement(stmt), kind(level_block), exitLabel(NULL_TREE),
      continueLabel(NULL_TREE), hasVars(NULL_TREE)
  { }
};


// IRState contains the core functionality of code generation utilities.
//
// Currently, each function gets its own IRState when emitting code.  There is
// also a global IRState current_irstate.
//
// Most toElem calls don't actually need the IRState because they create GCC
// expression trees rather than emit instructions.

// Functions without a verb create trees
// Functions with 'do' affect the current instruction stream (or output assembler code).
// functions with other names are for attribute manipulate, etc.

struct IRState
{
 public:
  // ** Functions
  FuncDeclaration *func;
  Module *mod;

  // Static chain of function, for D2, this is a closure.
  tree sthis;

  auto_vec<FuncDeclaration *> deferred;

  // Variables that are in scope that will need destruction later.
  auto_vec<VarDeclaration *> varsInScope;

  // ** Loops (and case statements)
  // These routines don't generate code.  They are for tracking labeled loops.
  Flow *beginFlow (Statement *stmt);
  void endFlow();

  Flow *currentFlow()
  {
    gcc_assert (!this->loops_.is_empty());
    return this->loops_.last();
  }

  void doLabel (tree t_label);

 //protected:
  auto_vec<tree> statementList_;
  auto_vec<Flow *> loops_;
  auto_vec<Label *> labels_;
};


#endif
