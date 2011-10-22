/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
#include "symbol.h"

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
    LabelDsymbol * label;
    Statement * block;
    Statement * from;
    LevelKind kind;
    unsigned level;

    Label() : label(NULL), block(NULL), from(NULL),
              kind(level_block), level(0) { }
};

struct Flow
{
    Statement * statement;
    LevelKind kind;
    tree exitLabel;
    union
    {
        struct
        {
            tree continueLabel;
            tree hasVars; // D2 specific, != NULL_TREE if switch uses Lvalues for cases.
        };
        struct
        {
            tree condition;  // Only need this if it is not okay to convert an IfStatement's
            tree trueBranch; // condition after converting it's branches...
        };
        struct
        {
            tree tryBody;
            tree catchType;
        };
    };

    Flow(Statement * stmt) :
            statement(stmt), kind(level_block), exitLabel(NULL_TREE),
            continueLabel(NULL_TREE), hasVars(NULL_TREE) { }
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
    IRBase * parent;

    IRBase();

    // ** Functions

    // This is used by LabelStatement to find the LabelDsymbol that
    // GotoStatements refer to.
    FuncDeclaration * func; // %% make this a stack

    static IRState * startFunction(FuncDeclaration * decl);
    void endFunction();
public:
    static Array deferredFuncDecls;
    bool shouldDeferFunction(FuncDeclaration * decl);

    static void initFunctionStart(tree fn_decl, const Loc & loc);

    // ** Statement Lists

    void addExp(tree e);
    Array statementList;    // of tree

    void pushStatementList();
    tree popStatementList();

    // ** Labels

    Labels labels;

    // It is only valid to call this while the function in which the label is defined
    // is being compiled.
    tree    getLabelTree(LabelDsymbol * label);
    Label * getLabelBlock(LabelDsymbol * label, Statement * from = NULL);

    bool isReturnLabel(Identifier * ident)
    {
        return func->returnLabel ? ident == func->returnLabel->ident : 0;
    }

    // ** Loops (and case statements)
    
    Flows loops;

    // These routines don't generate code.  They are for tracking labeled loops.
    Flow * getLoopForLabel(Identifier * ident, bool want_continue = false);
    Flow * beginFlow(Statement * stmt);

    void endFlow();

    Flow * currentFlow()
    {
        gcc_assert(loops.dim);
        return (Flow *) loops.tos();
    }

    void doLabel(tree t_label);

    // ** DECL_CONTEXT support

    tree getLocalContext()
    {
        return func ? func->toSymbol()->Stree : NULL_TREE;
    }

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
    Array scopes; // of unsigned*

    void startScope();
    void endScope();

    unsigned * currentScope()
    {
        gcc_assert(scopes.dim);
        return (unsigned *) scopes.tos();
    }

    void startBindings();
    void endBindings();


    // ** Volatile state

    unsigned volatileDepth;

    bool inVolatile()
    {
        return volatileDepth != 0;
    }

    void pushVolatile()
    {
        ++volatileDepth;
    }

    void popVolatile()
    {
        --volatileDepth;
    }
};


#endif
