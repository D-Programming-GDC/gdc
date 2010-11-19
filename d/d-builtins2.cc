/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, Iain Buclaw, (C) 2010

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
#include "total.h"
#include "attrib.h"
#include "template.h"
#include "symbol.h"
#include "d-codegen.h"

static ListMaker bi_fn_list;

// Necessary for built-in struct types
static Array builtin_converted_types;
static Array builtin_converted_decls;

static Type * gcc_type_to_d_type(tree t);

Type * d_gcc_builtin_va_list_d_type;

void
d_bi_init(int nt, int nb)
{
    // assumes va_list_type_node already built
    d_gcc_builtin_va_list_d_type = gcc_type_to_d_type(va_list_type_node);
    if (! d_gcc_builtin_va_list_d_type)
    {   // fallback to array of byte of the same size?
        error("cannot represent built in va_list type in D");
        abort();
    }
    // generate ctype if it doesn't already exist
    d_gcc_builtin_va_list_d_type->toCtype();
}

/*
  Set ctype directly for complex types to save toCtype() the work.
  For others, it is not useful or, in the cast of (C char) -> (D char),
  will cause errors.  This also means char* ...

  NOTE: We cannot always use type->pointerTo (in V2, at least) because
  (explanation TBD -- see TypeFunction::semantic and
  TypePointer::semantic: {deco = NULL;} .)
*/

static Type *
gcc_type_to_d_type(tree t)
{
    Type *d;
    switch (TREE_CODE(t))
    {
        case POINTER_TYPE:
        {   // Check for strings first.  There are currently no 'char' arguments
            // for built-in functions, so this is all that needs to be done for
            // chars/string.
            if (TYPE_MAIN_VARIANT(TREE_TYPE(t)) == char_type_node)
            {
                d = Type::tchar;
                d->ctype = TREE_TYPE(t);
                return d->pointerTo();
            }
            d = gcc_type_to_d_type(TREE_TYPE(t));
            if (d)
            {
                if (d->ty == Tfunction)
                    return new TypePointer(d);
                else
                    return d->pointerTo();
            }
            break;
        }
        case REFERENCE_TYPE:
        {
            d = gcc_type_to_d_type(TREE_TYPE(t));
            if (d)
            {   // Want to assign ctype directly so that the REFERENCE_TYPE
                // code can be turned into an InOut argument below.  Can't use
                // pointerTo(), because that Type is shared.
                d = new TypePointer(d);
                d->ctype = t;
                return d;
            }
            break;
        }
        case BOOLEAN_TYPE:
        {   // Should be no need for size checking.
            return Type::tbool;
            break;
        }
        case INTEGER_TYPE:
        {
            unsigned sz = tree_low_cst(TYPE_SIZE_UNIT(t), 1);
            bool unsgn = TYPE_UNSIGNED(t);
            // This search assumes that integer types come before char and bit...
            for (int i = 0; i < (int) TMAX; i++)
            {
                d = Type::basic[i];
                if (d && d->isintegral() && d->size() == sz &&
                        (d->isunsigned()?true:false) ==  unsgn)
                {
                    return d;
                }
            }
            break;
        }
        case REAL_TYPE:
        {   // Double and long double may be the same size
            if (t == double_type_node)
                return Type::tfloat64;
            else if (t == long_double_type_node)
                return Type::tfloat80;

            unsigned sz = tree_low_cst(TYPE_SIZE_UNIT(t), 1);
            for (int i = 0; i < (int) TMAX; i++)
            {
                d = Type::basic[i];
                if (d && d->isfloating() && ! d->iscomplex() && ! d->isimaginary() &&
                    d->size() == sz)
                {
                    return d;
                }
            }
            break;
        }
        case COMPLEX_TYPE:
        {
            unsigned sz = tree_low_cst(TYPE_SIZE_UNIT(t), 1);
            for (int i = 0; i < (int) TMAX; i++)
            {
                d = Type::basic[i];
                if (d && d->iscomplex() && d->size() == sz)
                {
                    return d;
                }
            }
            break;
        }
        case VOID_TYPE:
        {
            return Type::tvoid;
        }
        case ARRAY_TYPE:
        {
            d = gcc_type_to_d_type(TREE_TYPE(t));
            if (d)
            {
                tree index = TYPE_DOMAIN (t);
                tree ub = TYPE_MAX_VALUE (index);
                tree lb = TYPE_MIN_VALUE (index);
                tree length
                    = size_binop (PLUS_EXPR, size_one_node,
                            convert (sizetype,
                                fold (build2 (MINUS_EXPR,
                                        TREE_TYPE (lb),
                                        ub, lb))));
                d = new TypeSArray(d,
                        new IntegerExp(0, gen.getTargetSizeConst(length),
                            Type::tindex));
                d->ctype = t;
                return d;
            }
            break;
        }
        case RECORD_TYPE:
        {
            for (unsigned i = 0; i < builtin_converted_types.dim; i += 2)
            {
                tree ti = (tree) builtin_converted_types.data[i];
                if (ti == t)
                    return (Type *) builtin_converted_types.data[i + 1];
            }

            const char * name;
            char name_buf[64];
            static int serial;

            if (TYPE_NAME(t))
                name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(t)));
            else
            {
                snprintf(name_buf, sizeof(name_buf), "__bi_type_%d", ++serial);
                name = name_buf;
            }

            StructDeclaration * sd = new StructDeclaration(0, Lexer::idPool(name));
            /* The gcc.builtins module may not exist yet, so cannot set
               sd->parent here. If it is va_list, the parent needs to
               be set to the object module which will not exist when
               this is called.  */
            sd->structsize = int_size_in_bytes(t);
            sd->alignsize = TYPE_ALIGN_UNIT(t);
            sd->sizeok = 1;

            d = new TypeStruct(sd);
            sd->type = d;
#if STRUCTTHISREF
            sd->handle = d;
#else
            sd->handle = new TypePointer(d);
#endif

            /* Does not seem necessary to convert fields, but the
               members field must be non-null for the above size
               setting to stick. */
#if V2
            sd->members = new Dsymbols;
#else
            sd->members = new Array;
#endif

            d->ctype = t;

            builtin_converted_types.push(t);
            builtin_converted_types.push(d);
            builtin_converted_decls.push(sd);
            return d;
        }
        case FUNCTION_TYPE:
        {
            Type * ret = gcc_type_to_d_type(TREE_TYPE(t));
            if (! ret)
                return NULL;

            tree t_arg_types = TYPE_ARG_TYPES(t);
            int varargs = t_arg_types != NULL_TREE;
            Parameters * args = new Parameters;

            args->reserve(list_length(t_arg_types));
            for (tree tl = t_arg_types; tl != NULL_TREE; tl = TREE_CHAIN(tl))
            {
                tree ta = TREE_VALUE(tl);
                if (ta != void_type_node)
                {
                    unsigned io = STCin;

                    if (TREE_CODE(ta) == REFERENCE_TYPE)
                    {
                        ta = TREE_TYPE(ta);
                        io = STCref;
                    }

                    Type * d_arg_type = gcc_type_to_d_type(ta);
                    if (! d_arg_type)
                        goto Lfail;

                    args->push(new Parameter(io, d_arg_type, NULL, NULL));
                }
                else
                    varargs = 0;
            }
            d = new TypeFunction(args, ret, varargs, LINKc);
            return d;
        Lfail:
            delete args;
            break;
        }
        default:
        {
            break;
        }
    }
    return NULL;
}

void
d_bi_builtin_func(tree decl)
{
    bi_fn_list.cons(NULL_TREE, decl);
}


// helper function for d_gcc_magic_stdarg_module
/*
  In D2, the members of std.stdarg are hidden via @system attributes.
  This function should be sufficient in looking through all members.
*/
static void
d_gcc_magic_stdarg_check(Dsymbol *m, bool is_c_std_arg)
{
    Identifier * id_arg = Lexer::idPool("va_arg");
    Identifier * id_start = Lexer::idPool("va_start");

    AttribDeclaration * ad = NULL;
    TemplateDeclaration * td = NULL;

    if ((ad = m->isAttribDeclaration()))
    {
        // Recursively search through attribute decls
        Array * decl = ad->include(NULL, NULL);
        if (decl && decl->dim)
        {
            for (size_t i = 0; i < decl->dim; i++)
            {
                Dsymbol * sym = (Dsymbol *)decl->data[i];
                d_gcc_magic_stdarg_check(sym, is_c_std_arg);
            }
        }
    }
    else if ((td = m->isTemplateDeclaration()))
    {
        if (td->ident == id_arg)
        {
            if (is_c_std_arg)
                IRState::setCStdArgArg(td);
            else
                IRState::setStdArg(td);
        }
        else if (td->ident == id_start && is_c_std_arg)
            IRState::setCStdArgStart(td);
        else
            td = NULL;
    }

    if (td == NULL)     // Not handled.
        return;

    if (TREE_CODE(va_list_type_node) == ARRAY_TYPE)
    {   /* For GCC, a va_list can be an array.  D static arrays are
           automatically passed by reference, but the 'inout'
           modifier is not allowed. */
        assert(td->members);
        for (unsigned j = 0; j < td->members->dim; j++)
        {
            FuncDeclaration * fd =
                ((Dsymbol *) td->members->data[j])->isFuncDeclaration();
            if (fd && (fd->ident == id_arg || fd->ident == id_start))
            {
                TypeFunction * tf;
                // Should have nice error message instead of ICE in case some tries
                // to tweak the file.
                assert(! fd->parameters);
                tf = (TypeFunction *) fd->type;
                assert(tf->ty == Tfunction &&
                        tf->parameters && tf->parameters->dim >= 1);
                ((Parameter*) tf->parameters->data[0])->storageClass &= ~(STCin|STCout|STCref);
                ((Parameter*) tf->parameters->data[0])->storageClass |= STCin;
            }
        }
    }
}

// std.stdarg is different: it expects pointer types (i.e. _argptr)
/*
  We can make it work fine as long as the argument to va_varg is _argptr,
  we just call va_arg on the hidden va_list.  As long _argptr is not
  otherwise modified, it will work. */

static void
d_gcc_magic_stdarg_module(Module *m, bool is_c_std_arg)
{
    Array * members = m->members;
    for (unsigned i = 0; i < members->dim; i++)
    {
        Dsymbol * sym = (Dsymbol *) members->data[i];
        d_gcc_magic_stdarg_check(sym, is_c_std_arg);
    }
}

static void
d_gcc_magic_builtins_module(Module *m)
{
#if V2
    Dsymbols * funcs = new Dsymbols;
#else
    Array * funcs = new Array;
#endif

    for (tree n = bi_fn_list.head; n; n = TREE_CHAIN(n))
    {
        tree decl = TREE_VALUE(n);
        const char * name = IDENTIFIER_POINTER(DECL_NAME(decl));
        TypeFunction * dtf = (TypeFunction *) gcc_type_to_d_type(TREE_TYPE(decl));
        if (! dtf)
        {   //warning("cannot create built in function type for %s", name);
            continue;
        }
        if (dtf->parameters && dtf->parameters->dim == 0 && dtf->varargs)
        {   //warning("one-arg va problem: %s", name);
            continue;
        }
#if V2
        // %% D2 - all builtins are pure, safe and optionally nothrow
        dtf->purity = PUREstrong;
        dtf->trust = TRUSTsafe;
        dtf->isnothrow = TREE_NOTHROW(decl);
#endif
        FuncDeclaration * func = new FuncDeclaration(0, 0,
            Lexer::idPool(name), STCextern, dtf);
        func->isym = new Symbol;
        func->isym->Stree = decl;

        funcs->push(func);
    }

    for (unsigned i = 0; i < builtin_converted_decls.dim ; ++i)
    {
        Dsymbol * sym = (Dsymbol *) builtin_converted_decls.data[i];
        /* va_list is a pain.  It can be referenced without importing
           gcc.builtins so it really needs to go in the object module. */
        if (! sym->parent)
        {
            Declaration * decl = sym->isDeclaration();
            if (! decl || decl->type != d_gcc_builtin_va_list_d_type)
            {
                sym->parent = m;
                /* Currently, there is no need to run semantic, but we do
                   want to output inits, etc. */
                funcs->push(sym);
            }
        }
    }

    Type * d = gcc_type_to_d_type(va_list_type_node);
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                           Lexer::idPool("__builtin_va_list"), d));
    }

    /* Provide access to target-specific integer types. */
    d = gcc_type_to_d_type(long_integer_type_node);
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                         Lexer::idPool("__builtin_Clong"), d));
    }
    d = gcc_type_to_d_type(long_unsigned_type_node);
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                         Lexer::idPool("__builtin_Culong"), d));
    }
    d = gcc_type_to_d_type(d_type_for_mode(word_mode, 0));
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                         Lexer::idPool("__builtin_machine_int"), d));
    }
    d = gcc_type_to_d_type(d_type_for_mode(word_mode, 1));
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                         Lexer::idPool("__builtin_machine_uint"), d));
    }
    d = gcc_type_to_d_type(d_type_for_mode(ptr_mode, 0));
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                         Lexer::idPool("__builtin_pointer_int"), d));
    }
    d = gcc_type_to_d_type(d_type_for_mode(ptr_mode, 1));
    if (d)
    {
        funcs->push(new AliasDeclaration(0,
                         Lexer::idPool("__builtin_pointer_uint"), d));
    }
    m->members->push(new LinkDeclaration(LINKc, funcs));
}


void d_gcc_magic_module(Module *m)
{
    ModuleDeclaration * md = m->md;
    if (!md || !md->packages || !md->id)
        return;

    if (md->packages->dim == 1)
    {
        if (! strcmp(((Identifier *) md->packages->data[0])->string, "gcc"))
        {
            if (! strcmp(md->id->string, "builtins"))
                d_gcc_magic_builtins_module(m);
        }
        else if (! strcmp(((Identifier *) md->packages->data[0])->string, "std"))
        {
            if (! strcmp(md->id->string, "stdarg"))
                d_gcc_magic_stdarg_module(m, false);
            else if (! strcmp(md->id->string, "intrinsic"))
                IRState::setIntrinsicModule(m);
        }
    }
    else if (md->packages->dim == 2)
    {
        if (! strcmp(((Identifier *) md->packages->data[0])->string, "std") &&
            ! strcmp(((Identifier *) md->packages->data[1])->string, "c"))
        {
            if (! strcmp(md->id->string, "stdarg"))
                d_gcc_magic_stdarg_module(m, true);
        }
#if V2
        else if (! strcmp(((Identifier *) md->packages->data[0])->string, "core") &&
                 ! strcmp(((Identifier *) md->packages->data[1])->string, "stdc"))
        {
            if (! strcmp(md->id->string, "stdarg"))
                d_gcc_magic_stdarg_module(m, true);
        }
#endif
    }
}
