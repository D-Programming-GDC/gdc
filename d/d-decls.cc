/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrot, (C) 2009, 2010
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
/*
  This file is based on dmd/tocsym.c.  Original copyright:

// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

*/

#include "d-gcc-includes.h"

#include "mars.h"
#include "statement.h"
#include "aggregate.h"
#include "init.h"
#include "attrib.h"
#include "enum.h"
#include "module.h"
#include "id.h"

#include "symbol.h"
#include "d-lang.h"
#include "d-codegen.h"

/********************************* SymbolDeclaration ****************************/

SymbolDeclaration::SymbolDeclaration(Loc loc, Symbol *s, StructDeclaration *dsym)
    : Declaration(new Identifier(s->Sident, TOKidentifier))
{
    this->loc = loc;
    sym = s;
    this->dsym = dsym;
    storage_class |= STCconst;
}

Symbol *SymbolDeclaration::toSymbol()
{
    // Create the actual back-end value if not yet done
    if (! sym->Stree)
    {
        if (dsym)
            dsym->toInitializer();
        gcc_assert(sym->Stree);
    }
    return sym;
}

/*************************************
 * Helper
 */

Symbol *Dsymbol::toSymbolX(const char *prefix, int sclass, type *t, const char *suffix)
{
    Symbol *s;
    char *id;
    char *n;

    n = mangle();
    id = (char *) alloca(2 + strlen(n) + sizeof(size_t) * 3 + strlen(prefix) + strlen(suffix) + 1);
    sprintf(id,"_D%s%"PRIuSIZE"%s%s", n, strlen(prefix), prefix, suffix);
    s = symbol_name(id, sclass, t);
    return s;
}

/*************************************
 */

Symbol *Dsymbol::toSymbol()
{
    fprintf(stderr, "Dsymbol::toSymbol() '%s', kind = '%s'\n", toChars(), kind());
    gcc_unreachable();          // BUG: implement
    return NULL;
}

/*********************************
 * Generate import symbol from symbol.
 */

Symbol *Dsymbol::toImport()
{
    if (!isym)
    {
        if (!csym)
            csym = toSymbol();
        isym = toImport(csym);
    }
    return isym;
}

/*************************************
 */

Symbol *Dsymbol::toImport(Symbol * /*sym*/)
{
    // not used in GCC (yet?)
    return 0;
}



/* When using -combine, there may be name collisions on compiler-generated
   or extern(C) symbols. This only should only apply to private symbols.
   Otherwise, duplicate names are an error. */

static StringTable * uniqueNames = 0;

static void
uniqueName(Declaration * d, tree t, const char * asm_name)
{
    Dsymbol * p = d->toParent2();
    const char * out_name = asm_name;
    char * alloc_name;

    FuncDeclaration * f = d->isFuncDeclaration();
    VarDeclaration * v = d->isVarDeclaration();

    /* Check cases for which it is okay to have a duplicate symbol name.
       Otherwise, duplicate names are an error and the condition will
       be caught by the assembler. */
    if (!(f && ! f->fbody) &&
        !(v && (v->storage_class & STCextern)) &&
        (
         // Static declarations in different scope statements
         (p && p->isFuncDeclaration()) ||
         // Top-level duplicate names are okay if private.
         ((!p || p->isModule()) && d->protection == PROTprivate)
        )
       )
    {
        StringValue * sv;

        // Assumes one assembler output file per compiler run.  Otherwise, need
        // to reset this for each file.
        if (! uniqueNames)
            uniqueNames = new StringTable;
        sv = uniqueNames->update(asm_name, strlen(asm_name));

        if (sv->intvalue)
        {
            ASM_FORMAT_PRIVATE_NAME(alloc_name, asm_name, sv->intvalue);
            out_name = alloc_name;
        }
        sv->intvalue++;
    }

    tree id;
#if D_GCC_VER >= 43
    /* In 4.3.x, it is now the job of the front-end to ensure decls get mangled for their target.
       We'll only allow FUNCTION_DECLs and VAR_DECLs for variables with static storage duration
       to get a mangled DECL_ASSEMBLER_NAME. And the backend should handle the rest. */
    if (f || (v && (v->protection == PROTpublic || v->storage_class & (STCstatic | STCextern))))
    {
        id = targetm.mangle_decl_assembler_name(t, get_identifier(out_name));
    }
    else
#endif
    {
        id = get_identifier(out_name);
    }

    SET_DECL_ASSEMBLER_NAME(t, id);
}


/*************************************
 */

Symbol *VarDeclaration::toSymbol()
{
    if (! csym)
    {
        tree var_decl;

        // For field declaration, it is possible for toSymbol to be called
        // before the parent's toCtype()
        if (storage_class & STCfield)
        {
            AggregateDeclaration * parent_decl = toParent()->isAggregateDeclaration();
            gcc_assert(parent_decl);
            parent_decl->type->toCtype();
            gcc_assert(csym);
            return csym;
        }

        csym = new Symbol();
        if (isDataseg())
        {
            csym->Sident = mangle();
            csym->prettyIdent = toPrettyChars();
        }
        else
            csym->Sident = ident->string;

        enum tree_code decl_kind;

#if ! V2
        bool is_template_const = false;
        // Logic copied from toobj.c VarDeclaration::toObjFile

        Dsymbol * parent = this->toParent();
#if 1   /* private statics should still get a global symbol, in case
         * another module inlines a function that references it.
         */
        if (/*protection == PROTprivate ||*/
            !parent || parent->ident == NULL || parent->isFuncDeclaration())
        {
            // nothing
        }
        else
#endif
        {
            do
            {
                /* Global template data members need to be in comdat's
                 * in case multiple .obj files instantiate the same
                 * template with the same types.
                 */
                if (parent->isTemplateInstance() && !parent->isTemplateMixin())
                {
                    /* These symbol constants have already been copied,
                     * so no reason to output them.
                     * Note that currently there is no way to take
                     * the address of such a const.
                     */
                    if (isConst() && type->toBasetype()->ty != Tsarray &&
                        init && init->isExpInitializer())
                    {
                        is_template_const = true;
                        break;
                    }

                    break;
                }
                parent = parent->parent;
            } while (parent);
        }
#endif

        if (storage_class & STCparameter)
            decl_kind = PARM_DECL;
        else if (
#if V2
                 (storage_class & STCmanifest)
#else
                 is_template_const ||
                 (
                   isConst()
                   && ! gen.isDeclarationReferenceType(this) &&
                   type->isscalar() && ! isDataseg()
                 )
#endif
                 )
        {
            decl_kind = CONST_DECL;
        }
        else
        {
            decl_kind = VAR_DECL;
        }

        var_decl = d_build_decl(decl_kind, get_identifier(csym->Sident),
            gen.trueDeclarationType(this));

        csym->Stree = var_decl;

        if (decl_kind != CONST_DECL)
        {
            if (isDataseg())
                uniqueName(this, var_decl, csym->Sident);
            if (c_ident)
                SET_DECL_ASSEMBLER_NAME(var_decl, get_identifier(c_ident->string));
        }
        dkeep(var_decl);
        g.ofile->setDeclLoc(var_decl, this);
        if (decl_kind == VAR_DECL)
        {
            g.ofile->setupSymbolStorage(this, var_decl);
            //DECL_CONTEXT(var_decl) = gen.declContext(this);//EXPERkinda
        }
        else if (decl_kind == PARM_DECL)
        {
            /* from gcc code: Some languages have different nominal and real types.  */
            // %% What about DECL_ORIGINAL_TYPE, DECL_ARG_TYPE_AS_WRITTEN, DECL_ARG_TYPE ?
            DECL_ARG_TYPE(var_decl) = TREE_TYPE (var_decl);

            DECL_CONTEXT(var_decl) = gen.declContext(this);
            gcc_assert(TREE_CODE(DECL_CONTEXT(var_decl)) == FUNCTION_DECL);
        }
        else if (decl_kind == CONST_DECL)
        {
            /* Not sure how much of an optimization this is... It is needed
               for foreach loops on tuples which 'declare' the index variable
               as a constant for each iteration. */
            Expression * e = NULL;

            if (init)
            {
                if (! init->isVoidInitializer())
                {
                    e = init->toExpression();
                    gcc_assert(e != NULL);
                }
            }
            else
                e = type->defaultInit();

            if (e)
            {
                DECL_INITIAL(var_decl) = g.irs->assignValue(e, this);
                if (! DECL_INITIAL(var_decl))
                    DECL_INITIAL(var_decl) = e->toElem(g.irs);
            }
        }

        // Can't set TREE_STATIC, etc. until we get to toObjFile as this could be
        // called from a varaible in an imported module
        // %% (out const X x) doesn't mean the reference is const...
        if (
#if V2
            (isConst() || isImmutable()) && (storage_class & STCinit)
#else
            isConst()
#endif
            && ! gen.isDeclarationReferenceType(this))
        {
            // %% CONST_DECLS don't have storage, so we can't use those,
            // but it would be nice to get the benefit of them (could handle in
            // VarExp -- makeAddressOf could switch back to the VAR_DECL

            if (! TREE_STATIC(var_decl))
                TREE_READONLY(var_decl) = 1;
            else
            {   // Can't set "readonly" unless DECL_INITIAL is set, which
                // doesn't happen until outdata is called for the symbol.
                D_DECL_READONLY_STATIC(var_decl) = 1;
            }

            // can at least do this...
            //  const doesn't seem to matter for aggregates, so prevent problems..
            if (type->isscalar() || type->isString())
                TREE_CONSTANT(var_decl) = 1;
        }

#ifdef TARGET_DLLIMPORT_DECL_ATTRIBUTES
        // Have to test for import first
        if (isImportedSymbol())
        {
            gen.addDeclAttribute(var_decl, "dllimport");
            DECL_DLLIMPORT_P(var_decl) = 1;
        }
        else if (isExport())
            gen.addDeclAttribute(var_decl, "dllexport");
#endif

#if V2
        if (isDataseg() && isThreadlocal())
        {
            if (TREE_CODE(var_decl) == VAR_DECL)
            {   // %% If not marked, variable will be accessible
                // from multiple threads, which is not what we want.
                DECL_TLS_MODEL(var_decl) = decl_default_tls_model(var_decl);
            }
            if (global.params.vtls)
            {
                char *p = loc.toChars();
                fprintf(stderr, "%s: %s is thread local\n", p ? p : "", toChars());
                if (p)
                    free(p);
            }
        }
#endif
    }
    return csym;
}

/*************************************
 */

Symbol *ClassInfoDeclaration::toSymbol()
{
    return cd->toSymbol();
}

/*************************************
 */

Symbol *ModuleInfoDeclaration::toSymbol()
{
    return mod->toSymbol();
}

/*************************************
 */

Symbol *TypeInfoDeclaration::toSymbol()
{
    if (! csym)
    {
        VarDeclaration::toSymbol();

        // This variable is the static initialization for the
        // given TypeInfo.  It is the actual data, not a reference
        gcc_assert(TREE_CODE(TREE_TYPE(csym->Stree)) == REFERENCE_TYPE);
        TREE_TYPE(csym->Stree) = TREE_TYPE(TREE_TYPE(csym->Stree));

        /* DMD makes typeinfo decls one-only by doing:

            s->Sclass = SCcomdat;

           in TypeInfoDeclaration::toObjFile.  The difference is
           that, in gdc, built-in typeinfo will be referenced as
           one-only.
        */
        D_DECL_ONE_ONLY(csym->Stree) = 1;
        g.ofile->makeDeclOneOnly(csym->Stree);
    }
    return csym;
}

/*************************************
 */
#if V2

Symbol *TypeInfoClassDeclaration::toSymbol()
{
    gcc_assert(tinfo->ty == Tclass);
    TypeClass *tc = (TypeClass *)tinfo;
    return tc->sym->toSymbol();
}

#endif

/*************************************
 */

Symbol *FuncAliasDeclaration::toSymbol()
{
    return funcalias->toSymbol();
}

/*************************************
 */

// returns a FUNCTION_DECL tree
Symbol *FuncDeclaration::toSymbol()
{
    if (! csym)
    {
        csym  = new Symbol();

        if (! isym)
        {
            tree id;
            TypeFunction * func_type = (TypeFunction *)(tintro ? tintro : type);
            tree fn_decl;

            if (ident)
            {
                id = get_identifier(ident->string);
            }
            else
            {   // This happens for assoc array foreach bodies

                // Not sure if idents are strictly necc., but announce_function
                //  dies without them.

                // %% better: use parent name

                static unsigned unamed_seq = 0;
                char buf[64];
                sprintf(buf, "___unamed_%u", ++unamed_seq);//%% sprintf
                id = get_identifier(buf);
            }

            tree fn_type = func_type->toCtype();
            tree new_fn_type = NULL_TREE;

            tree vindex = NULL_TREE;
            if (isNested())
            {   /* Even if DMD-style nested functions are not implemented, add an
                   extra argument to be compatible with delegates. */
                new_fn_type = build_method_type(void_type_node, fn_type);
            }
            else if (isThis())
            {   // Do this even if there is no debug info.  It is needed to make
                // sure member functions are not called statically
                AggregateDeclaration * agg_decl = isMember();
                // Could be a template instance, check parent.
                if (agg_decl == NULL && parent->isTemplateInstance())
                    agg_decl = isThis();

                gcc_assert(agg_decl != NULL);

                tree handle = agg_decl->handle->toCtype();
#if STRUCTTHISREF
                if (agg_decl->isStructDeclaration())
                {   // Handle not a pointer type
                    new_fn_type = build_method_type(handle, fn_type);
                }
                else
#endif
                {
                    new_fn_type = build_method_type(TREE_TYPE(handle), fn_type);
                }

                if (isVirtual())
                    vindex = size_int(vtblIndex);
            }
            else if (isMain() && func_type->nextOf()->toBasetype()->ty == Tvoid)
            {
                new_fn_type = build_function_type(integer_type_node, TYPE_ARG_TYPES(fn_type));
            }

            if (new_fn_type != NULL_TREE)
            {
                TYPE_ATTRIBUTES(new_fn_type) = TYPE_ATTRIBUTES(fn_type);
                TYPE_LANG_SPECIFIC(new_fn_type) = TYPE_LANG_SPECIFIC(fn_type);
                fn_type = new_fn_type;
            }

            // %%CHECK: is it okay for static nested functions to have a FUNC_DECL context?
            // seems okay so far...
            fn_decl = d_build_decl(FUNCTION_DECL, id, fn_type);
            dkeep(fn_decl);
            if (ident)
            {
                csym->Sident = mangle(); // save for making thunks
                csym->prettyIdent = toPrettyChars();
                uniqueName(this, fn_decl, csym->Sident);
            }
            if (c_ident)
                SET_DECL_ASSEMBLER_NAME(fn_decl, get_identifier(c_ident->string));
            // %% What about DECL_SECTION_NAME ?
            //DECL_ARGUMENTS(fn_decl) = NULL_TREE; // Probably don't need to do this until toObjFile
            DECL_CONTEXT(fn_decl) = gen.declContext(this); //context;
            if (vindex)
            {
                DECL_VINDEX(fn_decl) = vindex;
                DECL_VIRTUAL_P(fn_decl) = 1;
            }
            if (! gen.functionNeedsChain(this)
                // gcc 4.0: seems to be an error to set DECL_NO_STATIC_CHAIN on a toplevel function
                // (tree-nest.c:1282:convert_all_function_calls)
                && decl_function_context(fn_decl))
            {
#if D_GCC_VER < 45
                // Prevent backend from thinking this is a nested function.
                DECL_NO_STATIC_CHAIN(fn_decl) = 1;
#endif
            }
            else
            {
#if D_GCC_VER >= 45
                // %% GCC-4.5: we do the opposite.
                DECL_STATIC_CHAIN(fn_decl) = 1;
#endif
                /* If a template instance has a nested function (because
                   a template argument is a local variable), the nested
                   function may not have its toObjFile called before the
                   outer function is finished.  GCC requires that nested
                   functions be finished first so we need to arrange for
                   toObjFile to be called earlier.

                   It may be possible to defer calling the outer
                   function's cgraph_finalize_function until all nested
                   functions are finished, but this will only work for
                   GCC >= 3.4. */
                FuncDeclaration * outer_func = NULL;
                bool is_template_member = false;
                for (Dsymbol * p = parent; p; p = p->parent)
                {
                    if (p->isTemplateInstance() && ! p->isTemplateMixin())
                        is_template_member = true;
                    else if (p->isFuncDeclaration())
                    {
                        outer_func = (FuncDeclaration *) p;
                        break;
                    }
                }
                if (is_template_member && outer_func)
                {
                    Symbol * outer_sym = outer_func->toSymbol();
                    if (outer_sym->outputStage != Finished)
                    {
                        if (! outer_sym->otherNestedFuncs)
                            outer_sym->otherNestedFuncs = new FuncDeclarations;
                        outer_sym->otherNestedFuncs->push(this);
                    }
                    else
                    {   // Probably a frontend bug.
                    }
                }
            }

            /* For now, inline asm means we can't inline (stack wouldn't be
               what was expected and LDASM labels aren't unique.)
               TODO: If the asm consists entirely
               of extended asm, we can allow inlining. */
            if (hasReturnExp & 8 /*inlineAsm*/)
            {
                DECL_UNINLINABLE(fn_decl) = 1;
            }
#if D_GCC_VER >= 44
            else if (isMember() || isFuncLiteralDeclaration())
            {
                // See grokmethod in cp/decl.c
                DECL_DECLARED_INLINE_P(fn_decl) = 1;
                DECL_NO_INLINE_WARNING_P (fn_decl) = 1;
            }
#else
            else
            {
                // see grokdeclarator in c-decl.c
                if (flag_inline_trees == 2 && fbody /* && should_emit? */)
                    DECL_INLINE (fn_decl) = 1;
            }
#endif
            if (naked)
            {
                D_DECL_NO_FRAME_POINTER(fn_decl) = 1;
                DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT(fn_decl) = 1;
                /* Need to do this or GCC will set up a frame pointer with -finline-functions.
                   Must have something to do with defered processing -- after we turn
                   flag_omit_frame_pointer back on. */
                DECL_UNINLINABLE(fn_decl) = 1;
            }

            // These are always compiler generated.
            if (isArrayOp)
            {
                DECL_ARTIFICIAL(fn_decl) = 1;
                D_DECL_ONE_ONLY(fn_decl) = 1;
            }
#if V2
            // %% Pure functions don't imply nothrow
            DECL_PURE_P(fn_decl) = (isPure() == PUREstrong && func_type->isnothrow);
            // %% Assert contracts in functions may throw.
            TREE_NOTHROW(fn_decl) = func_type->isnothrow && !global.params.useAssert;
            // TODO: check 'immutable' means arguments are readonly...
            TREE_READONLY(fn_decl) = func_type->isImmutable();
            TREE_CONSTANT(fn_decl) = func_type->isConst();
#endif

#ifdef TARGET_DLLIMPORT_DECL_ATTRIBUTES
            // Have to test for import first
            if (isImportedSymbol())
            {
                gen.addDeclAttribute(fn_decl, "dllimport");
                DECL_DLLIMPORT_P(fn_decl) = 1;
            }
            else if (isExport())
                gen.addDeclAttribute(fn_decl, "dllexport");
#endif
            g.ofile->setDeclLoc(fn_decl, this);
            g.ofile->setupSymbolStorage(this, fn_decl);
            if (! ident)
                TREE_PUBLIC(fn_decl) = 0;

            TREE_USED (fn_decl) = 1; // %% Probably should be a little more intelligent about this

            // %% hack: on darwin (at least) using a DECL_EXTERNAL (IRState::getLibCallDecl)
            // and TREE_STATIC FUNCTION_DECLs causes the stub label to be output twice.  This
            // is a work around.  This doesn't handle the case in which the normal
            // getLibCallDecl has already been created and used.  Note that the problem only
            // occurs with function inlining is used.
            if (linkage == LINKc)
                gen.replaceLibCallDecl(this);

            csym->Stree = fn_decl;

            gen.maybeSetUpBuiltin(this);
        }
        else
        {
            csym->Stree = isym->Stree;
        }
    }
    return csym;
}

#define D_PRIVATE_THUNKS 1

/*************************************
 */
Symbol *FuncDeclaration::toThunkSymbol(int offset)
{
    Symbol *sthunk;
    Thunk * thunk;

    toSymbol();

    /* If the thunk is to be static (that is, it is being emitted in this
       module, there can only be one FUNCTION_DECL for it.   Thus, there
       is a list of all thunks for a given function. */
    if (! csym->thunks)
        csym->thunks = new Thunks;
    Thunks & thunks = * csym->thunks;
    bool found = false;

    for (size_t i = 0; i < thunks.dim; i++)
    {
        thunk = thunks.tdata()[i];
        if (thunk->offset == offset)
        {
            found = true;
            break;
        }
    }

    if (! found)
    {
        thunk = new Thunk;
        thunk->offset = offset;
        thunks.push(thunk);
    }

    if (! thunk->symbol)
    {
        char * id;
        static unsigned thunk_sym_label = 0;
        ASM_FORMAT_PRIVATE_NAME(id, "___t", thunk_sym_label);
        thunk_sym_label++;
        sthunk = symbol_calloc(id);
        slist_add(sthunk);

        tree target_func_decl = csym->Stree;
        tree thunk_decl = build_fn_decl(id, TREE_TYPE(target_func_decl));
        dkeep(thunk_decl);
        sthunk->Stree = thunk_decl;

        //SET_DECL_ASSEMBLER_NAME(thunk_decl, DECL_NAME(thunk_decl));//old
        DECL_CONTEXT(thunk_decl) = DECL_CONTEXT(target_func_decl); // from c++...
        TREE_READONLY(thunk_decl) = TREE_READONLY(target_func_decl);
        TREE_THIS_VOLATILE(thunk_decl) = TREE_THIS_VOLATILE(target_func_decl);
        TREE_NOTHROW(thunk_decl) = TREE_NOTHROW(target_func_decl);

#ifdef D_PRIVATE_THUNKS
        DECL_EXTERNAL(thunk_decl) = 0;
        TREE_STATIC(thunk_decl) = 0;
        TREE_PRIVATE(thunk_decl) = 1;
        TREE_PUBLIC(thunk_decl) = 0;
#else
        /* Due to changes in the assembler, it is not possible to emit
           a private thunk that refers to an external symbol.
           http://lists.gnu.org/archive/html/bug-binutils/2005-05/msg00002.html
        */
        DECL_EXTERNAL(thunk_decl) = DECL_EXTERNAL(target_func_decl);
        TREE_STATIC(thunk_decl) = TREE_STATIC(target_func_decl);
        TREE_PRIVATE(thunk_decl) = TREE_PRIVATE(target_func_decl);
        TREE_PUBLIC(thunk_decl) = TREE_PUBLIC(target_func_decl);
#endif

        DECL_ARTIFICIAL(thunk_decl) = 1;
        DECL_IGNORED_P(thunk_decl) = 1;
#if D_GCC_VER < 45
        DECL_NO_STATIC_CHAIN(thunk_decl) = 1;
#endif
#if D_GCC_VER < 44
        DECL_INLINE(thunk_decl) = 0;
#endif
        DECL_DECLARED_INLINE_P(thunk_decl) = 0;
        //needed on some targets to avoid "causes a section type conflict"
        D_DECL_ONE_ONLY(thunk_decl) = D_DECL_ONE_ONLY(target_func_decl);
        if (D_DECL_ONE_ONLY(thunk_decl))
            g.ofile->makeDeclOneOnly(thunk_decl);

        TREE_ADDRESSABLE(thunk_decl) = 1;
        TREE_USED (thunk_decl) = 1;

#ifdef D_PRIVATE_THUNKS
        //g.ofile->prepareSymbolOutput(sthunk);
        g.ofile->doThunk(thunk_decl, target_func_decl, offset);
#else
        if (TREE_STATIC(thunk_decl))
            g.ofile->doThunk(thunk_decl, target_func_decl, offset);
#endif

        thunk->symbol = sthunk;
    }
    return thunk->symbol;
}


/****************************************
 * Create a static symbol we can hang DT initializers onto.
 */

Symbol *static_sym()
{
    Symbol * s = symbol_tree(NULL_TREE);
    //OLD//s->Sfl = FLstatic_sym;
    /* Before GCC 4.0, it was possible to take the address of a CONSTRUCTOR
       marked TREE_STATIC and the backend would output the data as an
       anonymous symbol.  This doesn't work in 4.0.  To keep things, simple,
       the same method is used for <4.0 and >= 4.0. */
    // Can't build the VAR_DECL because the type is unknown
    slist_add(s);
    return s;
}


/*************************************
 * Create the "ClassInfo" symbol
 */

Symbol *ClassDeclaration::toSymbol()
{
    if (! csym)
    {
        tree decl;
        csym = toSymbolX("__Class", SCextern, 0, "Z");
        slist_add(csym);
        decl = d_build_decl(VAR_DECL, get_identifier(csym->Sident),
            TREE_TYPE(ClassDeclaration::classinfo != NULL
                ? ClassDeclaration::classinfo->type->toCtype() // want the RECORD_TYPE, not the REFERENCE_TYPE
                : error_mark_node));
        csym->Stree = decl;
        dkeep(decl);

        g.ofile->setupStaticStorage(this, decl);
        g.ofile->setDeclLoc(decl, this);

        TREE_CONSTANT(decl) = 0; // DMD puts this into .data, not .rodata...
        TREE_READONLY(decl) = 0;
    }
    return csym;
}

/*************************************
 * Create the "InterfaceInfo" symbol
 */

Symbol *InterfaceDeclaration::toSymbol()
{
    if (!csym)
    {
        csym = ClassDeclaration::toSymbol();
        tree decl = csym->Stree;

        Symbol * temp_sym = toSymbolX("__Interface", SCextern, 0, "Z");
        DECL_NAME(decl) = get_identifier(temp_sym->Sident);
        delete temp_sym;

        TREE_CONSTANT(decl) = 1; // Interface ClassInfo images are in .rodata, but classes arent..?
    }
    return csym;
}

/*************************************
 * Create the "ModuleInfo" symbol
 */

Symbol *Module::toSymbol()
{
    if (!csym)
    {
        Type * obj_type = gen.getObjectType();

        csym = toSymbolX("__ModuleInfo", SCextern, 0, "Z");
        slist_add(csym);

        tree decl = d_build_decl(VAR_DECL, get_identifier(csym->Sident),
                TREE_TYPE(obj_type->toCtype())); // want the RECORD_TYPE, not the REFERENCE_TYPE
        g.ofile->setDeclLoc(decl, this);
        csym->Stree = decl;

        dkeep(decl);

        g.ofile->setupStaticStorage(this, decl);

        TREE_CONSTANT(decl) = 0; // *not* readonly, moduleinit depends on this
        TREE_READONLY(decl) = 0; // Not an lvalue, tho
    }
    return csym;
}

/*************************************
 * This is accessible via the ClassData, but since it is frequently
 * needed directly (like for rtti comparisons), make it directly accessible.
 */

Symbol *ClassDeclaration::toVtblSymbol()
{
    if (!vtblsym)
    {
        tree decl;

        vtblsym = toSymbolX("__vtbl", SCextern, 0, "Z");
        slist_add(vtblsym);

        /* The DECL_INITIAL value will have a different type object from the
           VAR_DECL.  The back end seems to accept this. */
        TypeSArray * vtbl_type = new TypeSArray(Type::tvoidptr,
                                                new IntegerExp(loc, vtbl.dim, Type::tindex));

        decl = d_build_decl(VAR_DECL, get_identifier(vtblsym->Sident), vtbl_type->toCtype());
        vtblsym->Stree = decl;
        dkeep(decl);

        g.ofile->setupStaticStorage(this, decl);
        g.ofile->setDeclLoc(decl, this);

        TREE_READONLY(decl) = 1;
        TREE_CONSTANT(decl) = 1;
        TREE_ADDRESSABLE(decl) = 1;
        // from cp/class.c
        DECL_CONTEXT (decl) =  TREE_TYPE(type->toCtype());
        DECL_VIRTUAL_P (decl) = 1;
        DECL_ALIGN (decl) = TARGET_VTABLE_ENTRY_ALIGN;
    }
    return vtblsym;
}

/**********************************
 * Create the static initializer for the struct/class.
 */

/* Because this is called from the front end (mtype.cc:TypeStruct::defaultInit()),
   we need to hold off using back-end stuff until the toobjfile phase.

   Specifically, it is not safe create a VAR_DECL with a type from toCtype()
   because there may be unresolved recursive references.
   StructDeclaration::toObjFile calls toInitializer without ever calling
   SymbolDeclaration::toSymbol, so we just need to keep checking if we
   are in the toObjFile phase.
*/

Symbol *AggregateDeclaration::toInitializer()
{
    Symbol *s;

    if (!sinit)
    {
        s = toSymbolX("__init", SCextern, 0, "Z");
        slist_add(s);
        sinit = s;
    }
    if (! sinit->Stree && g.ofile != NULL)
    {
        tree struct_type = type->toCtype();
        if (POINTER_TYPE_P(struct_type))
            struct_type = TREE_TYPE(struct_type); // for TypeClass, want the RECORD_TYPE, not the REFERENCE_TYPE
        tree t = d_build_decl(VAR_DECL, get_identifier(sinit->Sident), struct_type);
        sinit->Stree = t;
        dkeep(t);

        g.ofile->setupStaticStorage(this, t);
        g.ofile->setDeclLoc(t, this);

        // %% what's the diff between setting this stuff on the DECL and the
        // CONSTRUCTOR itself?

        TREE_ADDRESSABLE(t) = 1;
        TREE_READONLY(t) = 1;
        TREE_CONSTANT(t) = 1;
        DECL_CONTEXT(t) = 0; // These are always global
    }
    return sinit;
}

Symbol *TypedefDeclaration::toInitializer()
{
    Symbol *s;

    if (!sinit)
    {
        s = toSymbolX("__init", SCextern, 0, "Z");
        s->Sfl = FLextern;
        s->Sflags |= SFLnodebug;
        slist_add(s);
        sinit = s;
        sinit->Sdt = ((TypeTypedef *)type)->sym->init->toDt();
    }
    if (! sinit->Stree && g.ofile != NULL)
    {
        tree t = d_build_decl(VAR_DECL, get_identifier(sinit->Sident), type->toCtype());
        sinit->Stree = t;
        dkeep(t);

        g.ofile->setupStaticStorage(this, t);
        g.ofile->setDeclLoc(t, this);
        TREE_CONSTANT(t) = 1;
        TREE_READONLY(t) = 1;
        DECL_CONTEXT(t) = 0;
    }
    return sinit;
}

Symbol *EnumDeclaration::toInitializer()
{
    Symbol *s;

    if (!sinit)
    {
        Identifier *ident_save = ident;
        if (!ident)
        {   static int num;
            char name[6 + sizeof(num) * 3 + 1];
            snprintf(name, sizeof(name), "__enum%d", ++num);
            ident = Lexer::idPool(name);
        }
        s = toSymbolX("__init", SCextern, 0, "Z");
        ident = ident_save;
        s->Sfl = FLextern;
        s->Sflags |= SFLnodebug;
        slist_add(s);
        sinit = s;
    }
    if (! sinit->Stree && g.ofile != NULL)
    {
        tree t = d_build_decl(VAR_DECL, get_identifier(sinit->Sident), type->toCtype());
        sinit->Stree = t;
        dkeep(t);

        g.ofile->setupStaticStorage(this, t);
        g.ofile->setDeclLoc(t, this);
        TREE_CONSTANT(t) = 1;
        TREE_READONLY(t) = 1;
        DECL_CONTEXT(t) = 0;
    }
    return sinit;
}


/******************************************
 */

Symbol *Module::toModuleAssert()
{
    // Not used in GCC
    return 0;
}

/******************************************
 */
#if V2
Symbol *Module::toModuleUnittest()
{
    // Not used in GCC
    return 0;
}
#endif

/******************************************
 */

Symbol *Module::toModuleArray()
{
    // Not used in GCC (all array bounds checks are inlined)
    return 0;
}

/********************************************
 * Determine the right symbol to look up
 * an associative array element.
 * Input:
 *      flags   0       don't add value signature
 *              1       add value signature
 */

Symbol *TypeAArray::aaGetSymbol(const char *func, int flags)
{
    // This is not used in GCC (yet?)
    return 0;
}

