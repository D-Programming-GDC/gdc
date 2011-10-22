/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Iain Buclaw, (C) 2010, 2011

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
    statementList.zero();
}

IRState *
IRBase::startFunction(FuncDeclaration * decl)
{
    IRState * new_irs = new IRState();
    new_irs->parent = g.irs;
    new_irs->func = decl;

    tree decl_tree = decl->toSymbol()->Stree;

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
    gcc_assert(scopes.dim == 0);

    g.irs = (IRState *) parent;
}

bool
IRBase::shouldDeferFunction(FuncDeclaration * decl)
{
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
}

void
IRBase::initFunctionStart(tree fn_decl, const Loc & loc)
{
    init_function_start (fn_decl);
}


void
IRBase::addExp(tree e)
{
    /* Need to check that this is actually an expression; it
       could be an integer constant (statement with no effect.)
       Maybe should filter those out anyway... */
    // C doesn't do this for label_exprs %% why?
    if (EXPR_P(e) && ! EXPR_HAS_LOCATION (e))
        SET_EXPR_LOCATION (e, input_location);

    tree stmt_list = (tree) statementList.pop();
    append_to_statement_list_force (e, & stmt_list);
    statementList.push(stmt_list);
}


void
IRBase::pushStatementList()
{
    //tree t = alloc_stmt_list ();
    tree t = make_node (STATEMENT_LIST);
    TREE_TYPE (t) = void_type_node;
    statementList.push(t);
}

tree
IRBase::popStatementList()
{
    tree t = (tree) statementList.pop();

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
            tree u = tsi_stmt (i);
            tsi_delink (&i);
            free_stmt_list (t);
            t = u;
        }
    }
    return t;
}


tree
IRBase::getLabelTree(LabelDsymbol * label)
{
    if (! label->statement)
        return NULL_TREE;

    if (! label->statement->lblock)
    {
        tree label_decl = d_build_decl (LABEL_DECL, get_identifier(label->ident->string), void_type_node);

        gcc_assert(func != 0);
        DECL_CONTEXT(label_decl) = getLocalContext();
        DECL_MODE(label_decl) = VOIDmode; // Not sure why or if this is needed
        TREE_USED(label_decl) = 1;
        // Not setting this doesn't seem to cause problems (unlike VAR_DECLs)
        if (label->statement->loc.filename)
            g.ofile->setDeclLoc(label_decl, label->statement->loc); // %% label->loc okay?
        label->statement->lblock = label_decl;
    }
    return label->statement->lblock;
}

Label *
IRBase::getLabelBlock(LabelDsymbol * label, Statement * from)
{
    Label * l = new Label();

    for (int i = loops.dim - 1; i >= 0; i--)
    {
        Flow * flow = loops.tdata()[i];

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


Flow *
IRBase::getLoopForLabel(Identifier * ident, bool want_continue)
{
    if (ident)
    {
        LabelStatement * lbl_stmt = func->searchLabel(ident)->statement;
        gcc_assert(lbl_stmt != 0);
        Statement * stmt = lbl_stmt->statement;
        ScopeStatement * scope_stmt = stmt->isScopeStatement();

        if (scope_stmt)
            stmt = scope_stmt->statement;

        for (int i = loops.dim - 1; i >= 0; i--)
        {
            Flow * flow = loops.tdata()[i];

            if (flow->statement == stmt)
            {
                if (want_continue)
                    gcc_assert(stmt->hasContinue());
                return flow;
            }
        }
        gcc_unreachable();
    }
    else
    {
        for (int i = loops.dim - 1; i >= 0; i--)
        {
            Flow * flow = loops.tdata()[i];

            if (( ! want_continue && flow->statement->hasBreak() ) ||
                flow->statement->hasContinue())
                return flow;
        }
        gcc_unreachable();
    }
}


Flow *
IRBase::beginFlow(Statement * stmt)
{
    Flow * flow = new Flow(stmt);
    loops.push(flow);
    pushStatementList();
    return flow;
}

void
IRBase::endFlow()
{
    Flow * flow;

    gcc_assert(loops.dim);

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
    if(TREE_USED(t_label))
        addExp(build1(LABEL_EXPR, void_type_node, t_label));
}


void
IRBase::startScope()
{
    unsigned * p_count = new unsigned;
    *p_count = 0;
    scopes.push(p_count);

    //printf("%*sstartScope\n", scopes.dim, "");
    startBindings();
}

void
IRBase::endScope()
{
    unsigned * p_count;

    p_count = currentScope();

    //endBindings();

    //printf("%*s  ending scope with %d left \n", scopes.dim, "", *p_count);
    while ( *p_count )
        endBindings();
    scopes.pop();
    //printf("%*sendScope\n", scopes.dim, "");
}


void
IRBase::startBindings()
{
    pushlevel(0);
    tree block = make_node(BLOCK);
    set_block(block);

    pushStatementList();

    ++( * currentScope() );
    //printf("%*s  start -> %d\n", scopes.dim, "", * currentScope() );

}

void
IRBase::endBindings()
{
    // %%TODO: reversing list causes problems with inf loops in unshare_all_decls
    tree block = poplevel(1,0,0);

    tree t_body = popStatementList();
    addExp(build3(BIND_EXPR, void_type_node,
           BLOCK_VARS(block), t_body, block));

    // Because we used set_block, the popped level/block is not automatically recorded
    insert_block(block);

    --( * currentScope() );
    gcc_assert( * (int *) currentScope() >= 0 );
    //printf("%*s  end -> %d\n", scopes.dim, "", * currentScope() );

}
