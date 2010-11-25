/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Iain Buclaw, (C) 2010

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

#include "d-gcc-includes.h"
#include "d-lang.h"
#include "d-irstate.h"

#include <assert.h>

#include "total.h"
#include "init.h"
#include "symbol.h"

#include "d-codegen.h"


Array IRBase::deferredFuncDecls;

IRBase::IRBase()
{
    parent = 0;
    func = 0;
    volatileDepth = 0;

    // declContextStack is composed... choose one..
#if D_GCC_VER >= 40
    statementList = NULL_TREE;
#endif
}

IRState *
IRBase::startFunction(FuncDeclaration * decl)
{
    IRState * new_irs = new IRState();
    new_irs->parent = g.irs;
    new_irs->func = decl;

    tree decl_tree = decl->toSymbol()->Stree;

    // %%MOVE to ofile -- shuold be able to do after codegen
    // Need to do this for GCC 3.4 or functions will not be emitted
    d_add_global_function( decl_tree );

    g.irs = (IRState*) new_irs;
    ModuleInfo & mi = * g.mi();
#if V2
    if (decl->isSharedStaticCtorDeclaration())
        mi.sharedctors.push(decl);
    else if (decl->isStaticCtorDeclaration())
        mi.ctors.push(decl);
    else if (decl->isSharedStaticDtorDeclaration())
    {
        VarDeclaration * vgate;
        if ((vgate = decl->isSharedStaticDtorDeclaration()->vgate))
            mi.sharedctorgates.push(vgate);
        mi.shareddtors.push(decl);
    }
    else if (decl->isStaticDtorDeclaration())
    {
        VarDeclaration * vgate;
        if ((vgate = decl->isStaticDtorDeclaration()->vgate))
            mi.ctorgates.push(vgate);
        mi.dtors.push(decl);
    }
#else
    if (decl->isStaticConstructor())
        mi.ctors.push(decl);
    else if (decl->isStaticDestructor())
        mi.dtors.push(decl);
#endif
    else if (decl->isUnitTestDeclaration())
        mi.unitTests.push(decl);

    return new_irs;
}

void
IRBase::endFunction()
{
    assert(scopes.dim == 0);

    func = 0; // make shouldDeferFunction return false
    // Assumes deferredFuncDecls will not change after this point

    g.irs = (IRState *) parent;

    if (g.irs == & gen) {
        Array a;
        a.append(& deferredFuncDecls);
        deferredFuncDecls.setDim(0);
        for (unsigned i = 0; i < a.dim; i++) {
            FuncDeclaration * f = (FuncDeclaration *) a.data[i];
            f->toObjFile(false);
        }
    }

    // %%TODO
    /*
    if (can_delete)
        delete this;
    */
}

bool
IRBase::shouldDeferFunction(FuncDeclaration * decl)
{
#if D_GCC_VER < 40
    if (! func || gen.functionNeedsChain(decl)) {
        return false;
    } else {
        deferredFuncDecls.push(decl);
        return true;
    }
#else
    /* There is no need to defer functions for 4.0.  In fact, deferring
       causes problems because statically nested functions need to
       be prepared (maybe just have a struct function allocated) before
       the nesting function goes though lower_nested_functions.

       Start nesting function F.
       Defer nested function (a static class method)
       Output class's virtual table
       The reference to the function for the vtable creates a cgraph node
       ...
       lower_nested(F)
       the method is in the nested cgraph nodes, but the function hasing
       even been translated! */
    return false;
#endif
}

void
IRBase::initFunctionStart(tree fn_decl, const Loc & loc)
{
    init_function_start (fn_decl);
}

#if D_GCC_VER < 40
void
IRBase::addExp(tree e)
{
    expand_expr_stmt_value(e, 0, 1);
}
#else

void
IRBase::addExp(tree e)
{
    /* Need to check that this is actually an expression; it
       could be an integer constant (statement with no effect.)
       Maybe should filter those out anyway... */
    // C doesn't do this for label_exprs %% why?
    if (EXPR_P(e) && ! EXPR_HAS_LOCATION (e))
        SET_EXPR_LOCATION (e, input_location);

    append_to_statement_list_force (e, & statementList);
}

void
IRBase::pushStatementList()
{
    tree t;
    t = alloc_stmt_list ();
#if ENABLE_GC_CHECKING
    dkeep(t);   // %% Must be doing something wrong to need this.
#endif
    TREE_CHAIN (t) = statementList;
    statementList = t;
}

tree
IRBase::popStatementList()
{
    tree t = statementList;
    tree u;
    tree chain = TREE_CHAIN(t);
    TREE_CHAIN(t) = NULL_TREE;
    statementList = chain;

    // %% should gdc bother doing this?

    /* If the statement list is completely empty, just return it.  This is
       just as good small as build_empty_stmt, with the advantage that
       statement lists are merged when they appended to one another.  So
       using the STATEMENT_LIST avoids pathological buildup of EMPTY_STMT_P
       statements.  */
    if (TREE_SIDE_EFFECTS (t))
    {
        tree_stmt_iterator i = tsi_start (t);

        /* If the statement list contained exactly one statement, then
           extract it immediately.  */
        if (tsi_one_before_end_p (i))
        {
            u = tsi_stmt (i);
            tsi_delink (&i);
            free_stmt_list (t);
            t = u;
        }
    }
    return t;
}
#endif


tree
IRBase::getLabelTree(LabelDsymbol * label)
{
    if (! label->statement)
        return NULL_TREE;

    if (! label->statement->lblock)
    {
        tree label_decl = build_decl (LABEL_DECL, get_identifier(label->ident->string), void_type_node);

        assert(func != 0);
        DECL_CONTEXT( label_decl ) = getLocalContext();
        DECL_MODE( label_decl ) = VOIDmode; // Not sure why or if this is needed
        D_LABEL_IS_USED( label_decl ) = 1;
        // Not setting this doesn't seem to cause problems (unlike VAR_DECLs)
        if (label->statement->loc.filename)
            g.ofile->setDeclLoc( label_decl, label->statement->loc ); // %% label->loc okay?
        label->statement->lblock = label_decl;
    }
    return label->statement->lblock;
}

IRBase::Label *
IRBase::getLabelBlock(LabelDsymbol * label, Statement * from)
{
    Label * l = new Label;
    l->block = NULL;
    l->from = NULL;
    l->kind = level_block;
    l->level = 0;

    for (int i = loops.dim - 1; i >= 0; i--)
    {
        Flow * flow = (Flow *)loops.data[i];

        if (flow->kind != level_block &&
            flow->kind != level_switch)
        {
            l->block = flow->statement;
            l->kind  = flow->kind;
            l->level = i + 1;
            break;
        }
    }
    if (from)
        l->from = from;

    l->label = label;
    return l;
}


IRBase::Flow *
IRBase::getLoopForLabel(Identifier * ident, bool want_continue)
{
    if (ident) {
        LabelStatement * lbl_stmt = func->searchLabel(ident)->statement;
        assert(lbl_stmt != 0);
        Statement * stmt = lbl_stmt->statement;
        ScopeStatement * scope_stmt = stmt->isScopeStatement();

        if (scope_stmt)
            stmt = scope_stmt->statement;

        for (int i = loops.dim - 1; i >= 0; i--) {
            Flow * flow = (Flow *) loops.data[i];

            if (flow->statement == stmt) {
                if (want_continue)
                    assert(stmt->hasContinue());
                return flow;
            }
        }
        assert(0);
    } else {
        for (int i = loops.dim - 1; i >= 0; i--) {
            Flow * flow = (Flow *) loops.data[i];

            if (( ! want_continue && flow->statement->hasBreak() ) ||
                flow->statement->hasContinue())
                return flow;
        }
        assert(0);
    }
}

#if D_GCC_VER < 40

IRBase::Flow *
IRBase::beginFlow(Statement * stmt, nesting * loop)
{
    Flow * flow = new Flow;

    flow->statement = stmt;
    flow->loop = loop;
    flow->kind = level_block;
    flow->exitLabel = NULL;
    flow->overrideContinueLabel = NULL;

    loops.push(flow);

    return flow;
}

void
IRBase::endFlow()
{
    Flow * flow;

    assert(loops.dim);

    flow = (Flow *) loops.pop();
    if (flow->exitLabel)
        expand_label(flow->exitLabel); // %% need a statement after this?
    //%% delete flow;
}

void
IRBase::doLabel(tree t_label)
{
    expand_label(t_label);
}

#else

IRBase::Flow *
IRBase::beginFlow(Statement * stmt)
{
    Flow * flow = new Flow;

    flow->statement = stmt;
    flow->kind = level_block;
    flow->exitLabel = NULL_TREE;
    flow->condition = NULL_TREE;
    flow->trueBranch = NULL_TREE;

    loops.push(flow);

    pushStatementList();

    return flow;
}

void
IRBase::endFlow()
{
    Flow * flow;

    assert(loops.dim);

    flow = (Flow *) loops.pop();
    if (flow->exitLabel)
        doLabel(flow->exitLabel);
    //%% delete flow;
}

void
IRBase::doLabel(tree t_label)
{
    /* Don't write out label unless it is marked as used by the frontend.
       This makes auto-vectorization possible in conditional loops.
       The only excemption to this is in LabelStatement::toIR, in which
       all computed labels are marked regardless.  */
    if(D_LABEL_IS_USED(t_label))
        addExp(build1(LABEL_EXPR, void_type_node, t_label));
}

#endif

extern "C" void pushlevel PARAMS ((int));
extern "C" tree poplevel PARAMS ((int, int, int));
extern "C" void set_block PARAMS ((tree));
extern "C" void insert_block PARAMS ((tree));

void IRBase::startScope()
{
    unsigned * p_count = new unsigned;
    *p_count = 0;
    scopes.push(p_count);

    //printf("%*sstartScope\n", scopes.dim, "");
    startBindings();
}

void IRBase::endScope()
{
    unsigned * p_count;

    assert(scopes.dim);
    p_count = currentScope();

    //endBindings();

    //printf("%*s  ending scope with %d left \n", scopes.dim, "", *p_count);
    while ( *p_count )
        endBindings();
    scopes.pop();
    //printf("%*sendScope\n", scopes.dim, "");
}


void IRBase::startBindings()
{
    pushlevel(0);
    tree block = make_node(BLOCK);
    set_block(block);

#if D_GCC_VER < 40
    // This is the key to getting local variables recorded for debugging.
    // Simplying having the whole tree of BLOCKs doesn't matter as
    // reorder_blocks will discard the whole thing.
    expand_start_bindings_and_block(0, block);
#else
    pushStatementList();
#endif

    assert(scopes.dim);
    ++( * currentScope() );
    //printf("%*s  start -> %d\n", scopes.dim, "", * currentScope() );

}

void IRBase::endBindings()
{
    // %%TODO: reversing list causes problems with inf loops in unshare_all_decls
    tree block = poplevel(1,0,0);

#if D_GCC_VER < 40
    // %%TODO: DONT_JUMP_IN flag -- don't think DMD checks for
    // jumping past initializations yet..
    expand_end_bindings(NULL_TREE, 1, 0);
#else
    tree t_body = popStatementList();
    addExp(build3(BIND_EXPR, void_type_node,
               BLOCK_VARS( block ), t_body, block));
#endif

    // Because we used set_block, the popped level/block is not automatically recorded
    insert_block(block);

    assert(scopes.dim);
    --( * currentScope() );
    assert( * (int *) currentScope() >= 0 );
    //printf("%*s  end -> %d\n", scopes.dim, "", * currentScope() );

}
