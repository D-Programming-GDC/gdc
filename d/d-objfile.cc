/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, (C) 2009
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
#include "d-codegen.h"

#include <math.h>
#include <limits.h>

#include "module.h"
#include "template.h"
#include "init.h"
#include "symbol.h"
#include "dt.h"


ModuleInfo * ObjectFile::moduleInfo;
Modules ObjectFile::modules;
unsigned  ObjectFile::moduleSearchIndex;
DeferredThunks ObjectFile::deferredThunks;
FuncDeclarations ObjectFile::staticCtorList;
FuncDeclarations ObjectFile::staticDtorList;

ObjectFile::ObjectFile()
{
}

void
ObjectFile::beginModule(Module * m)
{
    moduleInfo = new ModuleInfo;
    g.mod = m;
}

void
ObjectFile::endModule()
{
    for (size_t i = 0; i < deferredThunks.dim; i++)
    {
        DeferredThunk * t = deferredThunks.tdata()[i];
        outputThunk(t->decl, t->target, t->offset);
    }
    deferredThunks.setDim(0);
    moduleInfo = NULL;
    g.mod = NULL;
}

bool
ObjectFile::hasModule(Module *m)
{
    if (!m || ! modules.dim)
        return false;

    if (modules.tdata()[moduleSearchIndex] == m)
        return true;
    for (size_t i = 0; i < modules.dim; i++)
    {
        if (modules.tdata()[i] == m)
        {
            moduleSearchIndex = i;
            return true;
        }
    }
    return false;
}

void
ObjectFile::finish()
{
#if D_GCC_VER < 43 && !(defined(__APPLE__) && D_GCC_VER == 42)
#define D_FFN_I 'I'
#define D_FFN_D 'D'
#else
#define D_FFN_I "I"
#define D_FFN_D "D"
#endif
    /* If the target does not directly support static constructors,
       staticCtorList contains a list of all static constructors defined
       so far.  This routine will create a function to call all of those
       and is picked up by collect2. */
    if (staticCtorList.dim)
    {
        doFunctionToCallFunctions(IDENTIFIER_POINTER(get_file_function_name(D_FFN_I)),
                & staticCtorList, true);
    }
    if (staticDtorList.dim)
    {
        doFunctionToCallFunctions(IDENTIFIER_POINTER(get_file_function_name(D_FFN_D)),
                & staticDtorList, true);
    }
}

void
ObjectFile::doLineNote(const Loc & loc)
{
    if (loc.filename)
        setLoc(loc);
    // else do nothing
}

#ifdef D_USE_MAPPED_LOCATION
static location_t
cvtLocToloc_t(const Loc loc)
{
    //gcc_assert(sizeof(StringValue.intvalue) == sizeof(location_t));
    static StringTable lmtab;
    StringValue * sv = lmtab.update(loc.filename, strlen(loc.filename));
    const struct line_map * lm = 0;
    unsigned new_line_count = 0;

    if (! sv->intvalue)
    {
        new_line_count = 5000;
    }
    else
    {
        lm = linemap_lookup(line_table, sv->intvalue);
        unsigned last_line = LAST_SOURCE_LINE(lm);
        if (loc.linnum > last_line)
            new_line_count = last_line * 5;
    }
    if (new_line_count)
    {
        lm = linemap_add(line_table, LC_ENTER, 0/*...*/, loc.filename, 0);
        sv->intvalue = linemap_line_start(line_table, new_line_count, 0);
        linemap_add(line_table, LC_LEAVE, 0, NULL, 0);
        lm = linemap_lookup(line_table, sv->intvalue);
    }
    // cheat...
    return lm->start_location + ((loc.linnum - lm->to_line) << lm->column_bits);
}
#endif

void
ObjectFile::setLoc(const Loc & loc)
{
    if (loc.filename)
    {
#ifdef D_USE_MAPPED_LOCATION
        input_location = cvtLocToloc_t(loc);
#else
        location_t gcc_loc = { loc.filename, loc.linnum };
        input_location = gcc_loc;
#endif
    }
    // else do nothing
}

void
ObjectFile::setDeclLoc(tree t, const Loc & loc)
{
    // DWARF2 will often crash if the DECL_SOURCE_FILE is not set.  It's
    // easier the error here.
    gcc_assert(loc.filename);
#ifdef D_USE_MAPPED_LOCATION
    DECL_SOURCE_LOCATION (t) = cvtLocToloc_t(loc);
#else
    DECL_SOURCE_FILE (t) = loc.filename;
    DECL_SOURCE_LINE (t) = loc.linnum;
#endif
}

void
ObjectFile::setDeclLoc(tree t, Dsymbol * decl)
{
    Dsymbol * dsym = decl;
    while (dsym)
    {
        if (dsym->loc.filename)
        {
            setDeclLoc(t, dsym->loc);
            return;
        }
        dsym = dsym->toParent();
    }

    // fallback; backend sometimes crashes if not set

    Loc l;

    Module * m = decl->getModule();
    if (m && m->srcfile && m->srcfile->name)
        l.filename = m->srcfile->name->str;
    else
        l.filename = "<no_file>"; // Emptry string can mess up debug info

    l.linnum = 1;
    setDeclLoc(t, l);
}

void
ObjectFile::setCfunEndLoc(const Loc & loc)
{
    tree fn_decl = cfun->decl;
# ifdef D_USE_MAPPED_LOCATION
    if (loc.filename)
        cfun->function_end_locus = cvtLocToloc_t(loc);
    else
        cfun->function_end_locus = DECL_SOURCE_LOCATION(fn_decl);
# else
    if (loc.filename)
    {
        cfun->function_end_locus.file = loc.filename;
        cfun->function_end_locus.line = loc.linnum;
    }
    else
    {
        cfun->function_end_locus.file = DECL_SOURCE_FILE (fn_decl);
        cfun->function_end_locus.line = DECL_SOURCE_LINE (fn_decl);
    }
# endif
}

void
ObjectFile::giveDeclUniqueName(tree decl, const char * prefix)
{
    /* It would be nice to be able to use TRANSLATION_UNIT_DECL
       so lhd_set_decl_assembler_name would do this automatically.
       Unforntuately, the non-NULL decl context confuses dwarf2out.

       Maybe this is fixed in later versions of GCC.
    */
    const char *name;
    if (prefix)
        name = prefix;
    else if (DECL_NAME(decl))
        name = IDENTIFIER_POINTER(DECL_NAME(decl));
    else
        name = "___s";

    char *label;
    ASM_FORMAT_PRIVATE_NAME(label, name, DECL_UID(decl));
    SET_DECL_ASSEMBLER_NAME(decl, get_identifier(label));
}

#if D_GCC_VER >= 45

/* For 4.5.x, return the COMDAT group into which DECL should be placed. */

static tree
d_comdat_group(tree decl)
{
    // %% May need special case here.
    return DECL_ASSEMBLER_NAME(decl);
}

#endif


void
ObjectFile::makeDeclOneOnly(tree decl_tree)
{
    if (! D_DECL_IS_TEMPLATE(decl_tree) || gen.emitTemplates != TEprivate)
    {   /* Weak definitions have to be public.  Nested functions may or
           may not be emitted as public even if TREE_PUBLIC is set.
           There is no way to tell if the back end implements
           make_decl_one_only with DECL_WEAK, so this check is
           done first.  */
        if (! TREE_PUBLIC(decl_tree) ||
                (TREE_CODE(decl_tree) == FUNCTION_DECL &&
                 decl_function_context(decl_tree) != NULL_TREE &&
                 DECL_STATIC_CHAIN(decl_tree)))
            return;
    }

    /* First method: Use one-only.
       If user has specified -femit-templates=private, honor that
       even if the target supports one-only. */
    if (! D_DECL_IS_TEMPLATE(decl_tree) || gen.emitTemplates != TEprivate)
    {
        /* The following makes assumptions about the behavior
           of make_decl_one_only */
        if (SUPPORTS_ONE_ONLY)
        {
#if D_GCC_VER >= 45
            make_decl_one_only(decl_tree, d_comdat_group(decl_tree));
#else
            make_decl_one_only(decl_tree);
#endif
        }
        else if (SUPPORTS_WEAK)
        {
            tree decl_init = DECL_INITIAL(decl_tree);
            DECL_INITIAL(decl_tree) = integer_zero_node;
#if D_GCC_VER >= 45
            make_decl_one_only(decl_tree, d_comdat_group(decl_tree));
#else
            make_decl_one_only(decl_tree);
#endif
            DECL_INITIAL(decl_tree) = decl_init;
        }
    }
    /* Second method: Make a private copy.
       For RTTI, we can always make a private copy.  For templates, only do
       this if the user specified -femit-templates=private. */
    else if (! D_DECL_IS_TEMPLATE(decl_tree) || gen.emitTemplates == TEprivate)
    {
        TREE_PRIVATE(decl_tree) = 1;
        TREE_PUBLIC(decl_tree) = 0;
    }
    else
    {   /* Fallback for templates, cannot have multiple copies. */
        if (DECL_INITIAL(decl_tree) == NULL_TREE
                || DECL_INITIAL(decl_tree) == error_mark_node)
            DECL_COMMON(decl_tree) = 1;
    }
}

void
ObjectFile::setupSymbolStorage(Dsymbol * dsym, tree decl_tree, bool force_static_public)
{
    Declaration * real_decl = dsym->isDeclaration();
    FuncDeclaration * func_decl = real_decl ? real_decl->isFuncDeclaration() : 0;

    if (force_static_public ||
        (TREE_CODE(decl_tree) == VAR_DECL && (real_decl && real_decl->isDataseg())) ||
        (TREE_CODE(decl_tree) == FUNCTION_DECL))
    {
        bool is_template = false;
        Dsymbol * sym = dsym->toParent();
        Module * ti_obj_file_mod;

        while (sym)
        {
            TemplateInstance * ti = sym->isTemplateInstance();
            if (ti)
            {
                ti_obj_file_mod = ti->objFileModule;
                is_template = true;
                break;
            }
            sym = sym->toParent();
        }

        bool is_static;

        if (is_template)
        {
            D_DECL_ONE_ONLY(decl_tree) = 1;
            D_DECL_IS_TEMPLATE(decl_tree) = 1;
            is_static = hasModule(ti_obj_file_mod) && gen.emitTemplates != TEnone;
        }
        else
            is_static = hasModule(dsym->getModule());

        if (real_decl && TREE_CODE(decl_tree) == VAR_DECL)
        {
            if (real_decl->storage_class & STCextern)
                is_static = false;
        }

        if (is_static)
        {
            DECL_EXTERNAL(decl_tree) = 0;
            TREE_STATIC(decl_tree) = 1; // %% don't set until there is a body?
            if (real_decl && (real_decl->storage_class & STCcomdat))
                D_DECL_ONE_ONLY(decl_tree) = 1;
        }
        else
        {
            DECL_EXTERNAL(decl_tree) = 1;
            TREE_STATIC(decl_tree) = 0;
        }

        // Do this by default, but allow private templates to override
        if (! func_decl || ! func_decl->isNested())
            TREE_PUBLIC(decl_tree) = 1;

        if (D_DECL_ONE_ONLY(decl_tree))
            makeDeclOneOnly(decl_tree);

    }
    else
    {
        TREE_STATIC(decl_tree) = 0;
        DECL_EXTERNAL(decl_tree) = 0;
        TREE_PUBLIC(decl_tree) = 0;
    }

    if (real_decl && real_decl->attributes)
        decl_attributes(& decl_tree, gen.attributes(real_decl->attributes), 0);
}

void
ObjectFile::setupStaticStorage(Dsymbol * dsym, tree decl_tree)
{
    setupSymbolStorage(dsym, decl_tree, true);
}


void
ObjectFile::outputStaticSymbol(Symbol * s)
{
    tree t = s->Stree;
    gcc_assert(t);

    if (s->prettyIdent)
        DECL_NAME(t) = get_identifier(s->prettyIdent);

    d_add_global_declaration(t);

    // %% Hack
    // Defer output of tls symbols to ensure that
    // _tlsstart gets emitted first.
    if (! DECL_THREAD_LOCAL_P(t))
        rodc(t, 1);
    else
    {
        tree sinit = DECL_INITIAL(t);
        DECL_INITIAL(t) = NULL_TREE;

        DECL_DEFER_OUTPUT(t) = 1;
        rodc(t, 1);
        DECL_INITIAL(t) = sinit;
    }
}

void
ObjectFile::outputFunction(FuncDeclaration * f)
{
    Symbol * s = f->toSymbol();
    tree t = s->Stree;

    // Write out _tlsstart/_tlsend.
    if (f->isMain() || f->isWinMain() || f->isDllMain())
        obj_tlssections();

    if (s->prettyIdent)
        DECL_NAME(t) = get_identifier(s->prettyIdent);

    d_add_global_declaration(t);

    if (TREE_CODE(t) == FUNCTION_DECL)
    {
        if (DECL_STATIC_CONSTRUCTOR(t))
        {
            // %% check for nested function -- error
            // otherwise, shouldn't be in a function, so safe to do asm_out
            if (targetm.have_ctors_dtors)
            {
                // handled in d_expand_function
            }
            else
            {   // %% assert FuncDeclaration
                staticCtorList.push(f);
                // %%TODO: fallback if ! targetm.have_ctors_dtors
            }
        }
        if (DECL_STATIC_DESTRUCTOR(t))
        {
            if (targetm.have_ctors_dtors)
            {
                // handled in d_expand_function
            }
            else
            {   // %% assert FuncDeclaration
                staticDtorList.push(f);
                // %%TODO: fallback if ! targetm.have_ctors_dtors
            }
        }
    }

    if (! gen.functionNeedsChain(f))
    {
        bool context = decl_function_context(t) != NULL;
        cgraph_finalize_function(t, context);
    }
}

/* Multiple copies of the same template instantiations can
   be passed to the backend from the frontend leaving
   assembler errors left in their wrath.

   One such example:
     class c(int i = -1) {}
     c!() aa = new c!()();

   So put these symbols - as generated by toSymbol,
   toInitializer, toVtblSymbol - on COMDAT.
 */
static StringTable * symtab = NULL;

bool
ObjectFile::shouldEmit(Declaration * d_sym)
{
    // If errors occurred compiling it.
    if (d_sym->type->ty == Tfunction && ((TypeFunction *)d_sym->type)->next->ty == Terror)
        return false;

    Symbol * s = d_sym->toSymbol();
    gcc_assert(s);

    return shouldEmit(s);
}

bool
ObjectFile::shouldEmit(Symbol * sym)
{
    // If have already started emitting, continue doing so.
    if (sym->outputStage)
        return true;

    if (D_DECL_ONE_ONLY(sym->Stree))
    {
        tree id = DECL_ASSEMBLER_NAME(sym->Stree);
        const char * ident = IDENTIFIER_POINTER(id);
        size_t len = IDENTIFIER_LENGTH(id);

        gcc_assert(ident != NULL);

        if (! symtab)
            symtab = new StringTable;

        if (! symtab->insert(ident, len))
            /* Don't emit, assembler name already in symtab. */
            return false;
    }

    // Not emitting templates, so return true all others.
    if (gen.emitTemplates == TEnone)
        return ! D_DECL_IS_TEMPLATE(sym->Stree);

    return true;
}

void
ObjectFile::addAggMethods(tree rec_type, AggregateDeclaration * agg)
{
    if (write_symbols != NO_DEBUG)
    {
        ListMaker methods;
        for (size_t i = 0; i < agg->methods.dim; i++)
        {
            FuncDeclaration * fd = agg->methods.tdata()[i];
            methods.chain(fd->toSymbol()->Stree);
        }
        TYPE_METHODS(rec_type) = methods.head;
    }
}

void
ObjectFile::initTypeDecl(tree t, Dsymbol * d_sym)
{
    gcc_assert(! POINTER_TYPE_P(t));
    if (! TYPE_STUB_DECL(t))
    {
        const char * name = d_sym->ident ? d_sym->ident->string : "fix";
        tree decl = d_build_decl(TYPE_DECL, get_identifier(name), t);
        DECL_CONTEXT(decl) = gen.declContext(d_sym);
        setDeclLoc(decl, d_sym);
        initTypeDecl(t, decl);
    }
}


void
ObjectFile::declareType(tree t, Type * d_type)
{
    // Note: It is not safe to call d_type->toCtype().
    Loc l;
    tree decl = d_build_decl(TYPE_DECL, get_identifier(d_type->toChars()), t);
    l.filename = "<internal>";
    l.linnum = 1;
    setDeclLoc(decl, l);

    declareType(t, decl);
}

void
ObjectFile::declareType(tree t, Dsymbol * d_sym)
{
    initTypeDecl(t, d_sym);
    declareType(t, TYPE_NAME(t));
}


void
ObjectFile::initTypeDecl(tree t, tree decl)
{
    if (TYPE_STUB_DECL(t))
        return;

    gcc_assert(! POINTER_TYPE_P(t));

    TYPE_CONTEXT(t) = DECL_CONTEXT(decl);
    TYPE_NAME(t) = decl;
    switch (TREE_CODE(t))
    {
        case ENUMERAL_TYPE:
        case RECORD_TYPE:
        case UNION_TYPE:
            /* Not sure if there is a need for separate TYPE_DECLs in
               TYPE_NAME and TYPE_STUB_DECL. */
            TYPE_STUB_DECL(t) = decl;
            // g++ does this and the debugging code assumes it:
            DECL_ARTIFICIAL(decl) = 1;
#if D_GCC_VER >= 43
            // code now assumes...
            DECL_SOURCE_LOCATION(decl) = DECL_SOURCE_LOCATION(decl);
#endif
            break;

        default:
            // nothing
            break;
    }
}

void
ObjectFile::declareType(tree t, tree decl)
{
    bool top_level = /*DECL_CONTEXT(decl) == fileContext || */
        ! DECL_CONTEXT(decl);
    // okay to do this?
    rodc(decl, top_level);
}

tree
ObjectFile::stripVarDecl(tree value)
{
    if (TREE_CODE(value) != VAR_DECL)
    {
        return value;
    }
    else if (DECL_INITIAL(value))
    {
        return DECL_INITIAL(value);
    }
    else
    {
        Type * d_type = gen.getDType(TREE_TYPE(value));
        if (d_type)
        {
            d_type = d_type->toBasetype();
            switch (d_type->ty)
            {
                case Tstruct:
                {   //tree t = gen.aggregateInitializer(((TypeStruct *) d_type)->sym, NULL, NULL, NULL_TREE);
                    // %% maker sure this is doing the right thing...
                    // %% better do get rid of it..

                    // need to VIEW_CONVERT?
                    dt_t * dt = NULL;
                    d_type->toDt(& dt);
                    tree t = dt2tree(dt);
                    TREE_CONSTANT(t) = 1;
                    return t;
                }
                default:
                {   // error below
                    break;
                }
            }
        }
        gcc_unreachable();
    }
}

void
ObjectFile::doThunk(tree thunk_decl, tree target_decl, int offset)
{
    if (current_function_decl)
    {
        DeferredThunk * t = new DeferredThunk;
        t->decl = thunk_decl;
        t->target = target_decl;
        t->offset = offset;
        deferredThunks.push(t);
    }
    else
        outputThunk(thunk_decl, target_decl, offset);
}

/* Thunk code is based on g++ */

#ifdef ASM_OUTPUT_DEF
static int thunk_labelno;

/* Create a static alias to function.  */

static tree
make_alias_for_thunk (tree function)
{
    tree alias;
    char buf[256];

#if defined (TARGET_IS_PE_COFF)
    /* make_alias_for_thunk does not seem to be needed for TARGET_IS_PE_COFF
       at all, and apparently causes problems... */
    //if (DECL_ONE_ONLY (function))
    return function;
#endif
    // For gdc: Thunks may reference extern functions which cannot be aliased.
    if (DECL_EXTERNAL(function))
        return function;

#if D_GCC_VER >= 46
    targetm.asm_out.generate_internal_label (buf, "LTHUNK", thunk_labelno);
#else
    ASM_GENERATE_INTERNAL_LABEL (buf, "LTHUNK", thunk_labelno);
#endif
    thunk_labelno++;
    alias = build_fn_decl (buf, TREE_TYPE (function));

    DECL_CONTEXT (alias) = NULL;
    TREE_READONLY (alias) = TREE_READONLY (function);
    TREE_THIS_VOLATILE (alias) = TREE_THIS_VOLATILE (function);
    TREE_NOTHROW (alias) = TREE_NOTHROW (function);
    TREE_PUBLIC (alias) = 0;

    DECL_EXTERNAL (alias) = 0;
    DECL_ARTIFICIAL (alias) = 1;

#if D_GCC_VER < 45
    DECL_NO_STATIC_CHAIN (alias) = 1;
#endif
#if D_GCC_VER < 44
    DECL_INLINE (alias) = 0;
#endif

    DECL_DECLARED_INLINE_P (alias) = 0;
#if D_GCC_VER >= 45
    DECL_INITIAL (alias) = error_mark_node;
    DECL_ARGUMENTS (alias) = copy_list (DECL_ARGUMENTS (function));
#endif

    TREE_ADDRESSABLE (alias) = 1;
    TREE_USED (alias) = 1;
    SET_DECL_ASSEMBLER_NAME (alias, DECL_NAME (alias));
    TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (alias)) = 1;

    if (!flag_syntax_only)
    {
#if D_GCC_VER >= 46
        struct cgraph_node *aliasn = cgraph_same_body_alias (cgraph_node (function),
                                                             alias, function);
        DECL_ASSEMBLER_NAME (function);
        gcc_assert (aliasn != NULL);
#elif D_GCC_VER >= 45
        bool ok = cgraph_same_body_alias (alias, function);
        DECL_ASSEMBLER_NAME (function);
        gcc_assert (ok);
#else
        assemble_alias (alias, DECL_ASSEMBLER_NAME (function));
#endif
    }
    return alias;
}
#endif

void
ObjectFile::outputThunk(tree thunk_decl, tree target_decl, int offset)
{
    int delta = -offset;
    tree alias;

#ifdef ASM_OUTPUT_DEF
    alias = make_alias_for_thunk(target_decl);
#else
    alias = target_decl;
#endif

    TREE_ADDRESSABLE(target_decl) = 1;
    TREE_USED(target_decl) = 1;
    DECL_VISIBILITY (thunk_decl) = DECL_VISIBILITY (target_decl);
#if ASM_OUTPUT_DEF && !defined (TARGET_IS_PE_COFF)
    if (targetm.have_named_sections)
    {
        resolve_unique_section (target_decl, 0, flag_function_sections);

        if (DECL_SECTION_NAME (target_decl) != NULL && DECL_ONE_ONLY (target_decl))
        {
            resolve_unique_section (thunk_decl, 0, flag_function_sections);
            /* Output the thunk into the same section as function.  */
            DECL_SECTION_NAME (thunk_decl) = DECL_SECTION_NAME (target_decl);
        }
    }
#endif
    // cp/method.c:
    /* The back-end expects DECL_INITIAL to contain a BLOCK, so we
       create one.  */
    // ... actually doesn't seem to be the case for output_mi_thunk
    DECL_INITIAL (thunk_decl) = make_node (BLOCK);
    BLOCK_VARS (DECL_INITIAL (thunk_decl)) = DECL_ARGUMENTS (thunk_decl);

    if (targetm.asm_out.can_output_mi_thunk(thunk_decl, delta, 0, alias))
    {
        const char *fnname;

        current_function_decl = thunk_decl;
        DECL_RESULT(thunk_decl) = d_build_decl_loc(DECL_SOURCE_LOCATION(thunk_decl),
                                               RESULT_DECL, 0, integer_type_node);
        fnname = XSTR(XEXP(DECL_RTL(thunk_decl), 0), 0);
        gen.initFunctionStart(thunk_decl, 0);
        cfun->is_thunk = 1;
        assemble_start_function (thunk_decl, fnname);
        targetm.asm_out.output_mi_thunk (asm_out_file, thunk_decl,
            delta, 0, alias);
        assemble_end_function(thunk_decl, fnname);
        free_after_compilation (cfun);
        set_cfun(0);
        current_function_decl = 0;
        TREE_ASM_WRITTEN (thunk_decl) = 1;
    }
    else
    {
        sorry("backend for this target machine does not support thunks");
    }
}

FuncDeclaration *
ObjectFile::doSimpleFunction(const char * name, tree expr, bool static_ctor, bool public_fn)
{
    if (! g.mod)
        g.mod = d_gcc_get_output_module();

    if (name[0] == '*')
    {
        Symbol * s = g.mod->toSymbolX(name + 1, 0, 0, "FZv");
        name = s->Sident;
    }

    TypeFunction * func_type = new TypeFunction(0, Type::tvoid, 0, LINKc);
    FuncDeclaration * func = new FuncDeclaration(g.mod->loc, g.mod->loc, // %% locs may be wrong
        Lexer::idPool(name), STCstatic, func_type); // name is only to prevent crashes
    func->loc = Loc(g.mod, 1); // to prevent debug info crash // maybe not needed if DECL_ARTIFICIAL?
    func->linkage = func_type->linkage;
    func->parent = g.mod;
    func->protection = public_fn ? PROTpublic : PROTprivate;

    tree func_decl = func->toSymbol()->Stree;
    if (static_ctor)
        DECL_STATIC_CONSTRUCTOR(func_decl) = 1; // apparently, the back end doesn't do anything with this

    // D static ctors, dtors, unittests, and the ModuleInfo chain function
    // are always private (see ObjectFile::setupSymbolStorage, default case)
    TREE_PUBLIC(func_decl) = public_fn;

    // %% maybe remove the identifier

    func->fbody = new ExpStatement(g.mod->loc,
        new WrappedExp(g.mod->loc, TOKcomma, expr, Type::tvoid));

    func->toObjFile(false);

    return func;
}

/* force: If true, create a new function even there is only one function in the
          list.
*/
FuncDeclaration *
ObjectFile::doFunctionToCallFunctions(const char * name, FuncDeclarations * functions, bool force_and_public)
{
    tree expr_list = NULL_TREE;

    // If there is only one function, just return that
    if (functions->dim == 1 && ! force_and_public)
    {
        return functions->tdata()[0];
    }
    else
    {   // %% shouldn't front end build these?
        for (size_t i = 0; i < functions->dim; i++)
        {
            FuncDeclaration * fn_decl = functions->tdata()[i];
            tree call_expr = gen.buildCall(void_type_node, gen.addressOf(fn_decl), NULL_TREE);
            expr_list = g.irs->maybeVoidCompound(expr_list, call_expr);
        }
    }
    if (expr_list)
        return doSimpleFunction(name, expr_list, false, false);

    return NULL;
}


/* Same as doFunctionToCallFunctions, but includes a gate to
   protect static ctors in templates getting called multiple times.
*/
FuncDeclaration *
ObjectFile::doCtorFunction(const char * name, FuncDeclarations * functions, VarDeclarations * gates)
{
    tree expr_list = NULL_TREE;

    // If there is only one function, just return that
    if (functions->dim == 1 && ! gates->dim)
    {
        return functions->tdata()[0];
    }
    else
    {   // Increment gates first.
        for (size_t i = 0; i < gates->dim; i++)
        {
            VarDeclaration * var = gates->tdata()[i];
            tree var_decl = var->toSymbol()->Stree;
            tree var_expr = build2(MODIFY_EXPR, void_type_node, var_decl,
                                build2(PLUS_EXPR, TREE_TYPE(var_decl), var_decl, integer_one_node));
            expr_list = g.irs->maybeVoidCompound(expr_list, var_expr);
        }
        // Call Ctor Functions
        for (size_t i = 0; i < functions->dim; i++)
        {
            FuncDeclaration * fn_decl = functions->tdata()[i];
            tree call_expr = gen.buildCall(void_type_node, gen.addressOf(fn_decl), NULL_TREE);
            expr_list = g.irs->maybeVoidCompound(expr_list, call_expr);
        }
    }
    if (expr_list)
        return doSimpleFunction(name, expr_list, false, false);

    return NULL;
}

/* Same as doFunctionToCallFunctions, but calls all functions in
   the reverse order that the constructors were called in.
*/
FuncDeclaration *
ObjectFile::doDtorFunction(const char * name, FuncDeclarations * functions)
{
    tree expr_list = NULL_TREE;

    // If there is only one function, just return that
    if (functions->dim == 1)
    {
        return functions->tdata()[0];
    }
    else
    {
        for (int i = functions->dim - 1; i >= 0; i--)
        {
            FuncDeclaration * fn_decl = functions->tdata()[i];
            tree call_expr = gen.buildCall(void_type_node, gen.addressOf(fn_decl), NULL_TREE);
            expr_list = g.irs->maybeVoidCompound(expr_list, call_expr);
        }
    }
    if (expr_list)
        return doSimpleFunction(name, expr_list, false, false);

    return NULL;
}

/* Currently just calls doFunctionToCallFunctions
*/
FuncDeclaration *
ObjectFile::doUnittestFunction(const char * name, FuncDeclarations * functions)
{
    return doFunctionToCallFunctions(name, functions);
}


tree
check_static_sym(Symbol * sym)
{
    if (! sym->Stree)
    {   //gcc_assert(sym->Sdt);    // Unfortunately cannot check for this; it might be an empty dt_t list...
        gcc_assert(! sym->Sident); // Can enforce that sym is anonymous though.
        tree t_ini = dt2tree(sym->Sdt); // %% recursion problems?
        tree t_var = d_build_decl(VAR_DECL, NULL_TREE, TREE_TYPE(t_ini));
        g.ofile->giveDeclUniqueName(t_var);
        DECL_INITIAL(t_var) = t_ini;
        TREE_STATIC(t_var) = 1;
        if (sym->Sseg == CDATA)
        {
            TREE_CONSTANT(t_var) = TREE_CONSTANT(t_ini) = 1;
            TREE_READONLY(t_var) = TREE_READONLY(t_ini) = 1;
        }
        // %% need to check SCcomdat?
        TREE_USED(t_var) = 1;
        TREE_PRIVATE(t_var) = 1;
        DECL_IGNORED_P(t_var) = 1;
        DECL_ARTIFICIAL(t_var) = 1;
        sym->Stree = t_var;
    }
    return sym->Stree;
}

void
outdata(Symbol * sym)
{
    tree t = check_static_sym(sym);
    gcc_assert(t);

    if (sym->Sdt && DECL_INITIAL(t) == NULL_TREE)
        DECL_INITIAL(t) = dt2tree(sym->Sdt);

    if (DECL_INITIAL(t) != NULL_TREE)
    {
        TREE_STATIC(t) = 1;
        DECL_EXTERNAL(t) = 0;
    }

    /* If the symbol was marked as readonly in the frontend, set TREE_READONLY.  */
    if (D_DECL_READONLY_STATIC(t))
        TREE_READONLY(t) = 1;

    /* Special case, outputting symbol of a module, but object.d is missing
       or corrupt. Set type as typeof DECL_INITIAL to satisfy runtime.  */
    tree type = TREE_TYPE(t);
    if (g.irs->isErrorMark(type))
    {
        gcc_assert(DECL_INITIAL(t));
        TREE_TYPE(t) = TREE_TYPE(DECL_INITIAL(t));
    }

    if (! g.ofile->shouldEmit(sym))
        return;

    // This was for typeinfo decls ... shouldn't happen now.
    // %% Oops, this was supposed to be static.
    gcc_assert(! DECL_EXTERNAL(t));
    layout_decl(t, 0);

    g.ofile->outputStaticSymbol(sym);
}

void
obj_includelib(const char *name)
{
    d_warning(OPT_Wunknown_pragmas, "pragma(lib) not implemented");
}

void
obj_startaddress(Symbol *s)
{
    d_warning(OPT_Wunknown_pragmas, "pragma(startaddress) not implemented");
}

void
obj_append(Dsymbol *s)
{
    // GDC does not do multi-obj, so just write it out now.
    s->toObjFile(false);
}

void
obj_moduleinfo(Symbol *sym)
{
    /*
        Generate:
        struct _modref_t {
            _modref_t  * next;
            ModuleInfo m;
        }
        extern (C) _modref_t * _Dmodule_ref;
        private _modref_t our_mod_ref =
          { next: null, m: _ModuleInfo_xxx };
        void ___modinit() { // a static constructor
           our_mod_ref.next = _Dmodule_ref;
           _Dmodule_ref = & our_mod_ref;
        }
    */
    // struct ModuleReference in moduleinit.d
    tree mod_ref_type = gen.twoFieldType(Type::tvoidptr, gen.getObjectType(),
                                         NULL, "next", "mod");
    tree f0 = TYPE_FIELDS(mod_ref_type);
    tree f1 = TREE_CHAIN(f0);

    tree our_mod_ref = d_build_decl(VAR_DECL, NULL_TREE, mod_ref_type);
    dkeep(our_mod_ref);
    g.ofile->giveDeclUniqueName(our_mod_ref, "__mod_ref");
    g.ofile->setDeclLoc(our_mod_ref, g.mod);

    CtorEltMaker ce;
    ce.cons(f0, d_null_pointer);
    ce.cons(f1, gen.addressOf(sym->Stree));

    tree init = build_constructor(mod_ref_type, ce.head);

    TREE_STATIC(init) = 1;
    DECL_ARTIFICIAL(our_mod_ref) = 1;
    DECL_IGNORED_P(our_mod_ref) = 1;
    TREE_PRIVATE(our_mod_ref) = 1;
    TREE_STATIC(our_mod_ref) = 1;
    DECL_INITIAL(our_mod_ref) = init;
    g.ofile->rodc(our_mod_ref, 1);

    tree the_mod_ref = d_build_decl(VAR_DECL, get_identifier("_Dmodule_ref"),
        build_pointer_type(mod_ref_type));
    dkeep(the_mod_ref);
    DECL_EXTERNAL(the_mod_ref) = 1;
    TREE_PUBLIC(the_mod_ref) = 1;

    tree m1 = build2(MODIFY_EXPR, void_type_node,
        gen.component(our_mod_ref, f0),
        the_mod_ref);
    tree m2 = build2(MODIFY_EXPR, void_type_node,
        the_mod_ref, gen.addressOf(our_mod_ref));
    tree exp = build2(COMPOUND_EXPR, void_type_node, m1, m2);

    g.ofile->doSimpleFunction("*__modinit", exp, true);
}

void
obj_tlssections()
{
    /*
        Generate:
        __thread int _tlsstart = 3;
        __thread int _tlsend;
     */
    tree tlsstart, tlsend;

    tlsstart = d_build_decl(VAR_DECL, get_identifier("_tlsstart"),
                            integer_type_node);
    TREE_PUBLIC(tlsstart) = 1;
    TREE_STATIC(tlsstart) = 1;
    DECL_ARTIFICIAL(tlsstart) = 1;
    // DECL_INITIAL so the symbol goes in .tdata
    DECL_INITIAL(tlsstart) = build_int_cst(integer_type_node, 3);
    DECL_TLS_MODEL(tlsstart) = decl_default_tls_model(tlsstart);
    g.ofile->setDeclLoc(tlsstart, g.mod);
    g.ofile->rodc(tlsstart, 1);

    tlsend = d_build_decl(VAR_DECL, get_identifier("_tlsend"),
                          integer_type_node);
    TREE_PUBLIC(tlsend) = 1;
    TREE_STATIC(tlsend) = 1;
    DECL_ARTIFICIAL(tlsend) = 1;
    // DECL_COMMON so the symbol goes in .tcommon
    DECL_COMMON(tlsend) = 1;
    DECL_TLS_MODEL(tlsend) = decl_default_tls_model(tlsend);
    g.ofile->setDeclLoc(tlsend, g.mod);
    g.ofile->rodc(tlsend, 1);
}

