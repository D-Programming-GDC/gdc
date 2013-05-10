
// Compiler implementation of the D programming language
// Copyright (c) 1999-2012 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

/* A dt_t is a simple structure representing data to be added
 * to the data segment of the output object file. As such,
 * it is a list of initialized bytes, 0 data, and offsets from
 * other symbols.
 * Each D symbol and type can be converted into a dt_t so it can
 * be written to the data segment.
 */

#include        <stdio.h>
#include        <string.h>
#include        <time.h>
#include        <assert.h>

#include        "lexer.h"
#include        "mtype.h"
#include        "expression.h"
#include        "init.h"
#include        "enum.h"
#include        "aggregate.h"
#include        "declaration.h"


// Back end
#ifndef IN_GCC
#include        "cc.h"
#include        "el.h"
#include        "oper.h"
#include        "global.h"
#include        "code.h"
#include        "type.h"
#endif
#include        "dt.h"

extern Symbol *static_sym();

typedef ArrayBase<dt_t> Dts;

/* ================================================================= */

// Generate the data for the static initializer.

void ClassDeclaration::toDt(dt_t **pdt)
{
    //printf("ClassDeclaration::toDt(this = '%s')\n", toChars());

    // Put in first two members, the vtbl[] and the monitor
    dtxoff(pdt, toVtblSymbol(), 0);
    dtsize_t(pdt, 0);                    // monitor

    // Put in the rest
    toDt2(pdt, this);

    //printf("-ClassDeclaration::toDt(this = '%s')\n", toChars());
}

void ClassDeclaration::toDt2(dt_t **pdt, ClassDeclaration *cd)
{
    unsigned offset;
    dt_t *dt;
    unsigned csymoffset;

#define LOG 0

#if LOG
    printf("ClassDeclaration::toDt2(this = '%s', cd = '%s')\n", toChars(), cd->toChars());
#endif
    if (baseClass)
    {
        baseClass->toDt2(pdt, cd);
        offset = baseClass->structsize;
    }
    else
    {
        offset = PTRSIZE * 2;
    }

    // Note equivalence of this loop to struct's
    for (size_t i = 0; i < fields.dim; i++)
    {
        VarDeclaration *v = fields[i];
        Initializer *init;

        //printf("\t\tv = '%s' v->offset = %2d, offset = %2d\n", v->toChars(), v->offset, offset);
        dt = NULL;
        init = v->init;
        if (init)
        {   //printf("\t\t%s has initializer %s\n", v->toChars(), init->toChars());
            ExpInitializer *ei = init->isExpInitializer();
            Type *tb = v->type->toBasetype();
            if (init->isVoidInitializer())
                ;
            else if (ei && tb->ty == Tsarray)
                ((TypeSArray *)tb)->toDtElem(&dt, ei->exp);
            else
                dt = init->toDt();
        }
        else if (v->offset >= offset)
        {   //printf("\t\tdefault initializer\n");
            v->type->toDt(&dt);
        }
        if (dt)
        {
            if (v->offset < offset)
                error("duplicated union initialization for %s", v->toChars());
            else
            {
                if (offset < v->offset)
                    dtnzeros(pdt, v->offset - offset);
                dtcat(pdt, dt);
                offset = v->offset + v->type->size();
            }
        }
    }

    // Interface vptr initializations
    toSymbol();                                         // define csym

    for (size_t i = 0; i < vtblInterfaces->dim; i++)
    {   BaseClass *b = (*vtblInterfaces)[i];

#if 1 || INTERFACE_VIRTUAL
        for (ClassDeclaration *cd2 = cd; 1; cd2 = cd2->baseClass)
        {
            assert(cd2);
            csymoffset = cd2->baseVtblOffset(b);
            if (csymoffset != ~0)
            {
                if (offset < b->offset)
                    dtnzeros(pdt, b->offset - offset);
                dtxoff(pdt, cd2->toSymbol(), csymoffset);
                break;
            }
        }
#else
        csymoffset = baseVtblOffset(b);
        assert(csymoffset != ~0);
        dtxoff(pdt, csym, csymoffset);
#endif
        offset = b->offset + PTRSIZE;
    }

    if (offset < structsize)
        dtnzeros(pdt, structsize - offset);

#undef LOG
}

void StructDeclaration::toDt(dt_t **pdt)
{
    if (zeroInit)
    {
        dtnzeros(pdt, structsize);
        return;
    }

    unsigned offset;
    dt_t *dt;
    dt_t *sdt = NULL;

    //printf("StructDeclaration::toDt(), this='%s'\n", toChars());
    offset = 0;

    // Note equivalence of this loop to class's
    for (size_t i = 0; i < fields.dim; i++)
    {
        VarDeclaration *v = fields[i];
        //printf("\tfield '%s' voffset %d, offset = %d\n", v->toChars(), v->offset, offset);
        dt = NULL;
        int sz;

        if (v->storage_class & STCref)
        {
            sz = PTRSIZE;
            if (v->offset >= offset)
                dtnzeros(&dt, sz);
        }
        else
        {
            sz = v->type->size();
            Initializer *init = v->init;
            if (init)
            {   //printf("\t\thas initializer %s\n", init->toChars());
                ExpInitializer *ei = init->isExpInitializer();
                Type *tb = v->type->toBasetype();
                if (init->isVoidInitializer())
                    ;
                else if (ei && tb->ty == Tsarray)
                    ((TypeSArray *)tb)->toDtElem(&dt, ei->exp);
                else
                    dt = init->toDt();
            }
            else if (v->offset >= offset)
                v->type->toDt(&dt);
        }
        if (dt)
        {
            if (v->offset < offset)
                error("overlapping initialization for struct %s.%s", toChars(), v->toChars());
            else
            {
                if (offset < v->offset)
                    dtnzeros(&sdt, v->offset - offset);
                dtcat(&sdt, dt);
                offset = v->offset + sz;
            }
        }
    }

    if (offset < structsize)
        dtnzeros(&sdt, structsize - offset);
#ifdef IN_GCC
    dtcontainer(pdt, type, sdt);
#else
    dtcat(pdt, sdt);
#endif
    dt_optimize(*pdt);
}

