// d-irstate.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_IRSTATE_H
#define GCC_DCMPLR_IRSTATE_H

// Due to the inlined functions, "dc-gcc-includes.h" needs to
// be included before this header is included.

#include "mars.h"
#include "root.h"
#include "lexer.h"
#include "mtype.h"
#include "declaration.h"
#include "statement.h"
#include "expression.h"
#include "aggregate.h"

#include "d-objfile.h"

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

  Label (void)
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


typedef ArrayBase<Label> Labels;
typedef ArrayBase<struct Flow> Flows;



// IRBase contains the core functionality of IRState.  The actual IRState class
// extends this with lots of code generation utilities.
//
// Currently, each function gets its own IRState when emitting code.  There is
// also a global IRState.
//
// Most toElem calls don't actually need the IRState because they create GCC
// expression trees rather than emit instructions.


struct IRBase : Object
{
 public:
  IRBase *parent;

  IRBase (void);

  // ** Functions
  FuncDeclaration *func;
  Module *mod;

  // Static chain of function, for D2, this is a closure.
  tree sthis;

  IRState *startFunction (FuncDeclaration *decl);
  void endFunction (void);

  // Variables that are in scope that will need destruction later.
  VarDeclarations *varsInScope;

  // ** Statement Lists
  void addExp (tree e);
  void pushStatementList (void);
  tree popStatementList (void);

  // ** Labels
  // It is only valid to call this while the function in which the label is defined
  // is being compiled.
  tree    getLabelTree (LabelDsymbol *label);
  Label *getLabelBlock (LabelDsymbol *label, Statement *from = NULL);

  bool isReturnLabel (Identifier *ident)
  { return this->func->returnLabel ? ident == this->func->returnLabel->ident : 0; }

  // ** Loops (and case statements)
  // These routines don't generate code.  They are for tracking labeled loops.
  Flow *getLoopForLabel (Identifier *ident, bool want_continue = false);
  Flow *beginFlow (Statement *stmt);

  void endFlow (void);

  Flow *currentFlow (void)
  {
    gcc_assert (this->loops_.dim);
    return (Flow *) this->loops_.tos();
  }

  void doLabel (tree t_label);

  // ** "Binding contours"

  /* Definitions for IRBase scope code:
     "Scope": A container for binding contours.  Each user-declared
     function has a toplevel scope.  Every ScopeStatement creates
     a new scope. (And for now, until the emitLocalVar crash is
     solved, this also creates a default binding contour.)

     "Binding contour": Same as GCC's definition, whatever that is.
     Each user-declared variable will have a binding contour that begins
     where the variable is declared and ends at it's containing scope.
   */

  void startScope (void);
  void endScope (void);

  unsigned *currentScope (void)
  {
    gcc_assert (this->scopes_.dim);
    return (unsigned *) this->scopes_.tos();
  }

  void startBindings (void);
  void endBindings (void);

  // Update current source file location to LOC.
  void doLineNote (const Loc& loc)
  { set_input_location (loc); }

  // ** Instruction stream manipulation

  // ** Conditional statements.
  void startCond (Statement *stmt, tree t_cond);
  void startElse (void);
  void endCond (void);

  // ** Loop statements.
  void startLoop (Statement *stmt);
  void continueHere (void);
  void setContinueLabel (tree lbl);
  void exitIfFalse (tree t_cond);
  void endLoop (void);
  void continueLoop (Identifier *ident);
  void exitLoop (Identifier *ident);

  // ** Goto/Label statement evaluation
  void doJump (Statement *stmt, tree t_label);
  void pushLabel (LabelDsymbol *l);
  void checkGoto (Statement *stmt, LabelDsymbol *label);
  void checkPreviousGoto (Array *refs);

  // ** Switch statements.
  void startCase (Statement *stmt, tree t_cond, int has_vars = 0);
  void checkSwitchCase (Statement *stmt, int default_flag = 0);
  void doCase (tree t_value, tree t_label);
  void endCase (void);

  // ** Exception handling.
  void startTry (Statement *stmt);
  void startCatches (void);
  void startCatch (tree t_type);
  void endCatch (void);
  void endCatches (void);
  void startFinally (void);
  void endFinally (void);

  // ** Return statement.
  void doReturn (tree t_value);

 protected:
  Array statementList_;	// of tree
  Array scopes_;	// of unsigned *
  Flows loops_;
  Labels labels_;
};


#endif
