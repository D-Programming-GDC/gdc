
// Compiler implementation of the D programming language
// Copyright (c) 1999-2012 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by David Friedman, July 2007
*/

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

/* ================================================================ */

#ifdef IN_GCC
static dt_t *createTsarrayDt(dt_t * dt, Type *t)
{
    assert(dt != NULL);
    size_t eoa_size = dt_size(dt);
    if (eoa_size == t->size())
        return dt;
    else
    {
        TypeSArray * tsa = (TypeSArray *) t->toBasetype();
        assert(tsa->ty == Tsarray);

        size_t dim = tsa->dim->toInteger();
        dt_t * adt = NULL;
        dt_t ** padt = & adt;

        if (eoa_size * dim == eoa_size)
        {
            for (size_t i = 0; i < dim; i++)
                padt = dtcontainer(padt, NULL, dt);
        }
        else
        {
            assert(tsa->size(0) % eoa_size == 0);
            for (size_t i = 0; i < dim; i++)
                padt = dtcontainer(padt, NULL,
                    createTsarrayDt(dt, tsa->next));
        }
        dt_t * fdt = NULL;
        dtcontainer(& fdt, t, adt);
        return fdt;
    }
}
#endif


dt_t *Initializer::toDt()
{
    assert(0);
    return NULL;
}


dt_t *VoidInitializer::toDt()
{   /* Void initializers are set to 0, just because we need something
     * to set them to in the static data segment.
     */
    dt_t *dt = NULL;

    dtnzeros(&dt, type->size());
    return dt;
}


dt_t *StructInitializer::toDt()
{
    Dts dts;
    dt_t *dt;
    dt_t *d;
    dt_t **pdtend;
    unsigned offset;

    //printf("StructInitializer::toDt('%s')\n", toChars());
    dts.setDim(ad->fields.dim);
    dts.zero();

    for (size_t i = 0; i < vars.dim; i++)
    {
        VarDeclaration *v = vars[i];
        Initializer *val = value[i];

        //printf("vars[%d] = %s\n", i, v->toChars());

        for (size_t j = 0; 1; j++)
        {
            assert(j < dts.dim);
            //printf(" adfield[%d] = %s\n", j, (ad->fields[j])->toChars());
            if (ad->fields[j] == v)
            {
                if (dts[j])
                    error(loc, "field %s of %s already initialized", v->toChars(), ad->toChars());
                dts[j] = val->toDt();
                break;
            }
        }
    }

    dt = NULL;
    pdtend = &dt;
    offset = 0;
    for (size_t j = 0; j < dts.dim; j++)
    {
        VarDeclaration *v = ad->fields[j];

        d = dts[j];
        if (!d)
        {   // An instance specific initializer was not provided.
            // Look to see if there's a default initializer from the
            // struct definition
            if (v->init && v->init->isVoidInitializer())
                ;
            else if (v->init)
            {
                d = v->init->toDt();
            }
            else if (v->offset >= offset)
            {
                unsigned k;
                unsigned offset2 = v->offset + v->type->size();
                // Make sure this field does not overlap any explicitly
                // initialized field.
                for (k = j + 1; 1; k++)
                {
                    if (k == dts.dim)           // didn't find any overlap
                    {
                        v->type->toDt(&d);
                        break;
                    }
                    VarDeclaration *v2 = ad->fields[k];

                    if (v2->offset < offset2 && dts[k])
                        break;                  // overlap
                }
            }
        }
        if (d)
        {
            if (v->offset < offset)
                error(loc, "duplicate union initialization for %s", v->toChars());
            else
            {   size_t sz = dt_size(d);
                size_t vsz = v->type->size();
                size_t voffset = v->offset;

                if (sz > vsz)
                {   assert(v->type->ty == Tsarray && vsz == 0);
                    error(loc, "zero length array %s has non-zero length initializer", v->toChars());
                }

                unsigned dim = 1;
                for (Type *vt = v->type->toBasetype();
                     vt->ty == Tsarray;
                     vt = vt->nextOf()->toBasetype())
                {   TypeSArray *tsa = (TypeSArray *)vt;
                    dim *= tsa->dim->toInteger();
                }
                //printf("sz = %d, dim = %d, vsz = %d\n", sz, dim, vsz);
                assert(sz == vsz || sz * dim <= vsz);

                for (size_t i = 0; i < dim; i++)
                {
                    if (offset < voffset)
                        pdtend = dtnzeros(pdtend, voffset - offset);
                    if (!d)
                    {
                        if (v->init)
                            d = v->init->toDt();
                        else
                            v->type->toDt(&d);
                    }
                    pdtend = dtcat(pdtend, d);
                    d = NULL;
                    offset = voffset + sz;
                    voffset += vsz / dim;
                    if (sz == vsz)
                        break;
                }
            }
        }
    }
    if (offset < ad->structsize)
        dtnzeros(pdtend, ad->structsize - offset);

#ifdef IN_GCC
    dt_t * cdt = NULL;
    dtcontainer(&cdt, ad->type, dt);
    dt = cdt;
#endif
    return dt;
}


dt_t *ArrayInitializer::toDt()
{
    //printf("ArrayInitializer::toDt('%s')\n", toChars());
    Type *tb = type->toBasetype();
    Type *tn = tb->nextOf()->toBasetype();

    Dts dts;
    unsigned size;
    unsigned length;
    dt_t *dt;
    dt_t *d;
    dt_t **pdtend;

    //printf("\tdim = %d\n", dim);
    dts.setDim(dim);
    dts.zero();

    size = tn->size();

    length = 0;
    for (size_t i = 0; i < index.dim; i++)
    {   Expression *idx;
        Initializer *val;

        idx = index[i];
        if (idx)
            length = idx->toInteger();
        //printf("\tindex[%d] = %p, length = %u, dim = %u\n", i, idx, length, dim);

        assert(length < dim);
        val = value[i];
        dt = val->toDt();
        if (dts[length])
            error(loc, "duplicate initializations for index %d", length);
        dts[length] = dt;
        length++;
    }

    Expression *edefault = tb->nextOf()->defaultInit();
#ifdef IN_GCC
    dt_t * sadefault = NULL;

    if (tn->ty == Tsarray)
        tn->toDt(& sadefault);
    else
        edefault->toDt(& sadefault);
#else
    unsigned n = 1;
    for (Type *tbn = tn; tbn->ty == Tsarray; tbn = tbn->nextOf()->toBasetype())
    {   TypeSArray *tsa = (TypeSArray *)tbn;

        n *= tsa->dim->toInteger();
    }
#endif

    d = NULL;
    pdtend = &d;
    for (size_t i = 0; i < dim; i++)
    {
        dt = dts[i];
#ifdef IN_GCC
        pdtend = dtcontainer(pdtend, NULL, dt ? dt : sadefault);
#else
        if (dt)
            pdtend = dtcat(pdtend, dt);
        else
        {
            for (size_t j = 0; j < n; j++)
                pdtend = edefault->toDt(pdtend);
        }
#endif
    }
    switch (tb->ty)
    {
        case Tsarray:
        {   unsigned tadim;
            TypeSArray *ta = (TypeSArray *)tb;

            tadim = ta->dim->toInteger();
            if (dim < tadim)
            {
                if (edefault->isBool(FALSE))
                    // pad out end of array
                    // (ok for GDC as well)
                    pdtend = dtnzeros(pdtend, size * (tadim - dim));
                else
                {
                    for (size_t i = dim; i < tadim; i++)
#ifdef IN_GCC
                        pdtend = dtcontainer(pdtend, NULL, sadefault);
#else
                    {   for (size_t j = 0; j < n; j++)
                            pdtend = edefault->toDt(pdtend);
                    }
#endif
                }
            }
            else if (dim > tadim)
            {
                error(loc, "too many initializers, %d, for array[%d]", dim, tadim);
            }
#ifdef IN_GCC
            dt_t * cdt = NULL;
            dtcontainer(& cdt, type, d);
            d = cdt;
#endif
            break;
        }

        case Tpointer:
        case Tarray:
        {   // Create symbol, and then refer to it
            Symbol *s = static_sym();
            s->Sdt = d;
            outdata(s);

            d = NULL;
            if (tb->ty == Tarray)
                dtsize_t(&d, dim);
            dtxoff(&d, s, 0, TYnptr);
#ifdef IN_GCC
            dt_t * cdt;
            cdt = NULL;
            if (tb->ty == Tarray)
            {
                dtcontainer(& cdt, type, d);
                d = cdt;
            }
#endif
            break;
        }

        default:
            assert(0);
    }
    return d;
}


dt_t *ArrayInitializer::toDtBit()
{
#if DMDV1
    unsigned size;
    unsigned length;
    unsigned tadim;
    dt_t *d;
    dt_t **pdtend;
    Type *tb = type->toBasetype();

    //printf("ArrayInitializer::toDtBit('%s')\n", toChars());

    Bits databits;
    Bits initbits;

    if (tb->ty == Tsarray)
    {
        /* The 'dim' for ArrayInitializer is only the maximum dimension
         * seen in the initializer, not the type. So, for static arrays,
         * use instead the dimension of the type in order
         * to get the whole thing.
         */
        dinteger_t value = ((TypeSArray*)tb)->dim->toInteger();
        tadim = value;
        assert(tadim == value);  // truncation overflow should already be checked
        databits.resize(tadim);
        initbits.resize(tadim);
    }
    else
    {
        databits.resize(dim);
        initbits.resize(dim);
    }

    /* The default initializer may be something other than zero.
     */
    if (tb->nextOf()->defaultInit()->toInteger())
       databits.set();

    size = sizeof(databits[0]);

    length = 0;
    for (size_t i = 0; i < index.dim; i++)
    {   Expression *idx;
        Initializer *val;
        Expression *eval;

        idx = index[i];
        if (idx)
        {   dinteger_t value;
            value = idx->toInteger();
            length = value;
            if (length != value)
            {   error(loc, "index overflow %llu", value);
                length = 0;
            }
        }
        assert(length < dim);

        val = value[i];
        eval = val->toExpression();
        if (initbits.test(length))
            error(loc, "duplicate initializations for index %d", length);
        initbits.set(length);
        if (eval->toInteger())          // any non-zero value is boolean 'true'
            databits.set(length);
        else
            databits.clear(length);     // boolean 'false'
        length++;
    }

    d = NULL;
#ifdef IN_GCC
    pdtend = dtnbits(&d, databits.allocdim * size, (char *)databits.data, sizeof(databits.tdata()[0]));
#else
    pdtend = dtnbytes(&d, databits.allocdim * size, (char *)databits.data);
#endif
    switch (tb->ty)
    {
        case Tsarray:
        {
            if (dim > tadim)
            {
                error(loc, "too many initializers, %d, for array[%d]", dim, tadim);
            }
            else
            {
                tadim = (tadim + 31) / 32;
                if (databits.allocdim < tadim)
                    pdtend = dtnzeros(pdtend, size * (tadim - databits.allocdim));      // pad out end of array
            }
            break;
        }

        case Tpointer:
        case Tarray:
            // Create symbol, and then refer to it
            Symbol *s;
            s = static_sym();
            s->Sdt = d;
            outdata(s);

            d = NULL;
            if (tb->ty == Tarray)
                dtsize_t(&d, dim);
            dtxoff(&d, s, 0, TYnptr);
            break;

        default:
            assert(0);
    }
    return d;
#else
    return NULL;
#endif
}


dt_t *ExpInitializer::toDt()
{
    //printf("ExpInitializer::toDt() %s\n", exp->toChars());
    dt_t *dt = NULL;

    exp = exp->optimize(WANTvalue);
    exp->toDt(&dt);
    return dt;
}

/* ================================================================ */

dt_t **Expression::toDt(dt_t **pdt)
{
#if 0
    printf("Expression::toDt() %d\n", op);
    dump(0);
#endif
    error("non-constant expression %s", toChars());
    pdt = dtnzeros(pdt, 1);
    return pdt;
}

#ifndef IN_GCC

dt_t **IntegerExp::toDt(dt_t **pdt)
{   unsigned sz;

    //printf("IntegerExp::toDt() %d\n", op);
    sz = type->size();
    if (value == 0)
        pdt = dtnzeros(pdt, sz);
    else
        pdt = dtnbytes(pdt, sz, (char *)&value);
    return pdt;
}

static char zeropad[6];

dt_t **RealExp::toDt(dt_t **pdt)
{
    d_float32 fvalue;
    d_float64 dvalue;
    d_float80 evalue;

    //printf("RealExp::toDt(%Lg)\n", value);
    switch (type->toBasetype()->ty)
    {
        case Tfloat32:
        case Timaginary32:
            fvalue = value;
            pdt = dtnbytes(pdt,4,(char *)&fvalue);
            break;

        case Tfloat64:
        case Timaginary64:
            dvalue = value;
            pdt = dtnbytes(pdt,8,(char *)&dvalue);
            break;

        case Tfloat80:
        case Timaginary80:
            evalue = value;
            pdt = dtnbytes(pdt,REALSIZE - REALPAD,(char *)&evalue);
            pdt = dtnbytes(pdt,REALPAD,zeropad);
            assert(REALPAD <= sizeof(zeropad));
            break;

        default:
            fprintf(stderr, "%s\n", toChars());
            type->print();
            assert(0);
            break;
    }
    return pdt;
}

dt_t **ComplexExp::toDt(dt_t **pdt)
{
    //printf("ComplexExp::toDt() '%s'\n", toChars());
    d_float32 fvalue;
    d_float64 dvalue;
    d_float80 evalue;

    switch (type->toBasetype()->ty)
    {
        case Tcomplex32:
            fvalue = creall(value);
            pdt = dtnbytes(pdt,4,(char *)&fvalue);
            fvalue = cimagl(value);
            pdt = dtnbytes(pdt,4,(char *)&fvalue);
            break;

        case Tcomplex64:
            dvalue = creall(value);
            pdt = dtnbytes(pdt,8,(char *)&dvalue);
            dvalue = cimagl(value);
            pdt = dtnbytes(pdt,8,(char *)&dvalue);
            break;

        case Tcomplex80:
            evalue = creall(value);
            pdt = dtnbytes(pdt,REALSIZE - REALPAD,(char *)&evalue);
            pdt = dtnbytes(pdt,REALPAD,zeropad);
            evalue = cimagl(value);
            pdt = dtnbytes(pdt,REALSIZE - REALPAD,(char *)&evalue);
            pdt = dtnbytes(pdt,REALPAD,zeropad);
            break;

        default:
            assert(0);
            break;
    }
    return pdt;
}


#endif

dt_t **NullExp::toDt(dt_t **pdt)
{
    assert(type);
    return dtnzeros(pdt, type->size());
}

dt_t **StringExp::toDt(dt_t **pdt)
{
    //printf("StringExp::toDt() '%s', type = %s\n", toChars(), type->toChars());
    Type *t = type->toBasetype();

    // BUG: should implement some form of static string pooling
    switch (t->ty)
    {
        case Tarray:
            dt_t *adt; adt = NULL;
            dtsize_t(&adt, len);
#ifdef IN_GCC
            dtawords(&adt, len + 1, string, sz);
            dtcontainer(pdt, type, adt);
#else
            dtabytes(&adt, TYnptr, 0, (len + 1) * sz, (char *)string);
            dtcat(pdt, adt);
#endif
            
            break;

        case Tsarray:
        {   TypeSArray *tsa = (TypeSArray *)type;
            dinteger_t dim;

#ifdef IN_GCC
            pdt = dtnwords(pdt, len, string, sz);
#else
            pdt = dtnbytes(pdt, len * sz, (const char *)string);
#endif
            if (tsa->dim)
            {
                dim = tsa->dim->toInteger();
                if (len < dim)
                {
                    // Pad remainder with 0
                    pdt = dtnzeros(pdt, (dim - len) * tsa->next->size());
                }
            }
            break;
        }
        case Tpointer:
#ifdef IN_GCC
            pdt = dtawords(pdt, len + 1, string, sz);
#else
            pdt = dtabytes(pdt, TYnptr, 0, (len + 1) * sz, (char *)string);
#endif
            break;

        default:
            fprintf(stderr, "StringExp::toDt(type = %s)\n", type->toChars());
            assert(0);
    }
    return pdt;
}

dt_t **ArrayLiteralExp::toDt(dt_t **pdt)
{
    //printf("ArrayLiteralExp::toDt() '%s', type = %s\n", toChars(), type->toChars());

    dt_t *d;
    dt_t **pdtend;

    d = NULL;
    pdtend = &d;
    for (size_t i = 0; i < elements->dim; i++)
    {   Expression *e = (*elements)[i];

        pdtend = e->toDt(pdtend);
    }
#ifdef IN_GCC
    dt_t *cdt = NULL;
    dtcontainer(&cdt, type, d);
    d = cdt;
#endif
    Type *t = type->toBasetype();

    switch (t->ty)
    {
        case Tsarray:
            pdt = dtcat(pdt, d);
            break;

        case Tpointer:
        case Tarray:
            dt_t *adt; adt = NULL;
            if (t->ty == Tarray)
                dtsize_t(&adt, elements->dim);
            if (d)
            {
                // Create symbol, and then refer to it
                Symbol *s;
                s = static_sym();
                s->Sdt = d;
                outdata(s);

                dtxoff(&adt, s, 0, TYnptr);
            }
            else
                dtsize_t(&adt, 0);
#ifdef IN_GCC
            if (t->ty == Tarray)
                dtcontainer(pdt, type, adt);
            else
#endif
                dtcat(pdt, adt);

            break;

        default:
            assert(0);
    }
    return pdt;
}

dt_t **StructLiteralExp::toDt(dt_t **pdt)
{
    //printf("StructLiteralExp::toDt() %s, ctfe = %d\n", toChars(), ownedByCtfe);

    /* For elements[], construct a corresponding array dts[] the elements
     * of which are the initializers.
     * Nulls in elements[] become nulls in dts[].
     */
    Dts dts;
    dt_t *sdt = NULL;
    dts.setDim(sd->fields.dim);
    dts.zero();
    assert(elements->dim <= sd->fields.dim);
    for (size_t i = 0; i < elements->dim; i++)
    {
        Expression *e = (*elements)[i];
        if (!e)
            continue;
        dt_t *dt = NULL;
        e->toDt(&dt);           // convert e to an initializer dt
        dts[i] = dt;
    }

    unsigned offset = 0;
    for (size_t j = 0; j < dts.dim; j++)
    {
        VarDeclaration *v = sd->fields[j];

        dt_t *d = dts[j];
        if (!d)
        {   /* An instance specific initializer was not provided.
             * If there is no overlap with any explicit initializer in dts[],
             * supply a default initializer.
             */
#if 0
           // An instance specific initializer was not provided.
            // Look to see if there's a default initializer from the
            // struct definition
            if (v->init && v->init->isVoidInitializer())
                ;
            else if (v->init)
            {
                d = v->init->toDt();
            } else
#endif
            if (v->offset >= offset)
            {
                unsigned offset2 = v->offset + v->type->size();
                for (size_t k = j + 1; 1; k++)
                {
                    if (k == dts.dim)           // didn't find any overlap
                    {
                        // Set d to be the default initializer
                        if (v->init)
                            d = v->init->toDt();
                        else
                            v->type->toDt(&d);
                        break;
                    }
                    VarDeclaration *v2 = sd->fields[k];

                    if (v2->offset < offset2 && dts[k])
                        break;                  // overlap
                }
            }
        }
        if (d)
        {
            if (v->offset < offset)
                error("duplicate union initialization for %s", v->toChars());
            else
            {   size_t sz = dt_size(d);
                size_t vsz = v->type->size();
                size_t voffset = v->offset;

                if (sz > vsz)
                {   assert(v->type->ty == Tsarray && vsz == 0);
                    error("zero length array %s has non-zero length initializer", v->toChars());
                }

                unsigned dim = 1;
                Type *vt;
                for (vt = v->type->toBasetype();
                     vt->ty == Tsarray;
                     vt = vt->nextOf()->toBasetype())
                {   TypeSArray *tsa = (TypeSArray *)vt;
                    dim *= tsa->dim->toInteger();
                }
                //fprintf("sz = %d, dim = %d, vsz = %d\n", sz, dim, vsz);
                assert(sz == vsz || sz * dim <= vsz);

                for (size_t i = 0; i < dim; i++)
                {
                    if (offset < voffset)
                        dtnzeros(&sdt, voffset - offset);
                    if (!d)
                    {
                        if (v->init)
                            d = v->init->toDt();
                        else
                            vt->toDt(&d);
                    }
                    dtcat(&sdt, d);
                    d = NULL;
                    offset = voffset + sz;
                    voffset += vsz / dim;
                    if (sz == vsz)
                        break;
                }
            }
        }
    }
    if (offset < sd->structsize)
        dtnzeros(&sdt, sd->structsize - offset);
#ifdef IN_GCC
    dtcontainer(pdt, type, sdt);
#else
    dtcat(pdt, sdt);
#endif
    return pdt;
}


dt_t **SymOffExp::toDt(dt_t **pdt)
{
    Symbol *s;

    //printf("SymOffExp::toDt('%s')\n", var->toChars());
    assert(var);
    if (!(var->isDataseg() || var->isCodeseg()) ||
        var->needThis() ||
        var->isThreadlocal())
    {
#if 0
        printf("SymOffExp::toDt()\n");
#endif
        error("non-constant expression %s", toChars());
        return pdt;
    }
    s = var->toSymbol();
    return dtxoff(pdt, s, offset, TYnptr);
}

dt_t **VarExp::toDt(dt_t **pdt)
{
    //printf("VarExp::toDt() %d\n", op);
    for (; *pdt; pdt = &((*pdt)->DTnext))
        ;

    VarDeclaration *v = var->isVarDeclaration();
    if (v && (v->isConst() || v->isImmutable()) &&
        type->toBasetype()->ty != Tsarray && v->init)
    {
        if (v->inuse)
        {
            error("recursive reference %s", toChars());
            return pdt;
        }
        v->inuse++;
        *pdt = v->init->toDt();
        v->inuse--;
        return pdt;
    }
    SymbolDeclaration *sd = var->isSymbolDeclaration();
    if (sd && sd->dsym)
    {
        sd->dsym->toDt(pdt);
        return pdt;
    }
#if 0
    printf("VarExp::toDt(), kind = %s\n", var->kind());
#endif
    error("non-constant expression %s", toChars());
    pdt = dtnzeros(pdt, 1);
    return pdt;
}

dt_t **FuncExp::toDt(dt_t **pdt)
{
    //printf("FuncExp::toDt() %d\n", op);
    Symbol *s = fd->toSymbol();
    if (fd->isNested())
    {   error("non-constant nested delegate literal expression %s", toChars());
        return NULL;
    }
    fd->toObjFile(0);
    return dtxoff(pdt, s, 0, TYnptr);
}

/* ================================================================= */

// Generate the data for the static initializer.

void ClassDeclaration::toDt(dt_t **pdt)
{
    //printf("ClassDeclaration::toDt(this = '%s')\n", toChars());

    // Put in first two members, the vtbl[] and the monitor
    dtxoff(pdt, toVtblSymbol(), 0, TYnptr);
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
                dtxoff(pdt, cd2->toSymbol(), csymoffset, TYnptr);
                break;
            }
        }
#else
        csymoffset = baseVtblOffset(b);
        assert(csymoffset != ~0);
        dtxoff(pdt, csym, csymoffset, TYnptr);
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

/* ================================================================= */

dt_t **Type::toDt(dt_t **pdt)
{
    //printf("Type::toDt()\n");
    Expression *e = defaultInit();
    return e->toDt(pdt);
}

dt_t **TypeSArray::toDt(dt_t **pdt)
{
    return toDtElem(pdt, NULL);
}

dt_t **TypeSArray::toDtElem(dt_t **pdt, Expression *e)
{
    unsigned len;

    //printf("TypeSArray::toDtElem()\n");
    len = dim->toInteger();
    if (len)
    {
        while (*pdt)
            pdt = &((*pdt)->DTnext);
        Type *tnext = next;
        Type *tbn = tnext->toBasetype();
        while (tbn->ty == Tsarray && (!e || tbn != e->type->nextOf()))
        {   TypeSArray *tsa = (TypeSArray *)tbn;

            len *= tsa->dim->toInteger();
            tnext = tbn->nextOf();
            tbn = tnext->toBasetype();
        }
        if (!e)                         // if not already supplied
            e = tnext->defaultInit();   // use default initializer
        dt_t *adt = NULL;
        dt_t **padt = & adt;
        e->toDt(padt);
        dt_optimize(*padt);
        // These first four cases are OK for GDC
        if (e->op == TOKstring)
            len /= ((StringExp *)e)->len;
        if (e->op == TOKarrayliteral)
            len /= ((ArrayLiteralExp *)e)->elements->dim;
        if ((*padt)->dt == DT_azeros && !(*padt)->DTnext)
        {
            (*padt)->DTazeros *= len;
            pdt = dtcat(pdt, adt);
        }
        else if ((*padt)->dt == DT_1byte && (*padt)->DTonebyte == 0 && !(*padt)->DTnext)
        {
            (*padt)->dt = DT_azeros;
            (*padt)->DTazeros = len;
            pdt = dtcat(pdt, adt);
        }
        else
        {
#ifdef IN_GCC
            pdt = dtcat(pdt, createTsarrayDt(adt, this));
#else
            for (size_t i = 1; i < len; i++)
            {
                if (tbn->ty == Tstruct)
                {   padt = tnext->toDt(padt);
                    while (*padt)
                        padt = &((*padt)->DTnext);
                }
                else
                    padt = e->toDt(padt);
            }
            pdt = dtcat(pdt, adt);
#endif
        }
    }
    return pdt;
}

dt_t **TypeStruct::toDt(dt_t **pdt)
{
    sym->toDt(pdt);
    return pdt;
}

dt_t **TypeTypedef::toDt(dt_t **pdt)
{
    if (sym->init)
    {
        dt_t *dt = sym->init->toDt();

        while (*pdt)
            pdt = &((*pdt)->DTnext);
        *pdt = dt;
        return pdt;
    }
    sym->basetype->toDt(pdt);
    return pdt;
}



