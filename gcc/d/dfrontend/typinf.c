
// Compiler implementation of the D programming language
// Copyright (c) 1999-2013 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mars.h"
#include "module.h"
#include "mtype.h"
#include "scope.h"
#include "init.h"
#include "expression.h"
#include "attrib.h"
#include "declaration.h"
#include "template.h"
#include "id.h"
#include "enum.h"
#include "import.h"
#include "aggregate.h"

#ifdef IN_GCC
#include "d-dmd-gcc.h"
#endif

#include "dt.h"

/*
 * Used in TypeInfo*::toDt to verify the runtime TypeInfo sizes
 */
void verifyStructSize(ClassDeclaration *typeclass, size_t expected)
{
        if (typeclass->structsize != expected)
        {
#ifdef DEBUG
            printf("expected = x%x, %s.structsize = x%x\n", expected,
                typeclass->toChars(), typeclass->structsize);
#endif
            error(typeclass->loc, "mismatch between compiler and object.d or object.di found. Check installation and import paths with -v compiler switch.");
            fatal();
        }
}

/****************************************************
 */

void TypeInfoDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoDeclaration::toDt() %s\n", toChars());
    verifyStructSize(Type::typeinfo, 2 * PTRSIZE);

    dtxoff(pdt, Type::typeinfo->toVtblSymbol(), 0); // vtbl for TypeInfo
    dtsize_t(pdt, 0);                        // monitor
}

#if DMDV2
void TypeInfoConstDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoConstDeclaration::toDt() %s\n", toChars());
    verifyStructSize(Type::typeinfoconst, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfoconst->toVtblSymbol(), 0); // vtbl for TypeInfo_Const
    dtsize_t(pdt, 0);                        // monitor
    Type *tm = tinfo->mutableOf();
    tm = tm->merge();
    tm->getTypeInfo(NULL);
    dtxoff(pdt, tm->vtinfo->toSymbol(), 0);
}

void TypeInfoInvariantDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoInvariantDeclaration::toDt() %s\n", toChars());
    verifyStructSize(Type::typeinfoinvariant, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfoinvariant->toVtblSymbol(), 0); // vtbl for TypeInfo_Invariant
    dtsize_t(pdt, 0);                        // monitor
    Type *tm = tinfo->mutableOf();
    tm = tm->merge();
    tm->getTypeInfo(NULL);
    dtxoff(pdt, tm->vtinfo->toSymbol(), 0);
}

void TypeInfoSharedDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoSharedDeclaration::toDt() %s\n", toChars());
    verifyStructSize(Type::typeinfoshared, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfoshared->toVtblSymbol(), 0); // vtbl for TypeInfo_Shared
    dtsize_t(pdt, 0);                        // monitor
    Type *tm = tinfo->unSharedOf();
    tm = tm->merge();
    tm->getTypeInfo(NULL);
    dtxoff(pdt, tm->vtinfo->toSymbol(), 0);
}

void TypeInfoWildDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoWildDeclaration::toDt() %s\n", toChars());
    verifyStructSize(Type::typeinfowild, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfowild->toVtblSymbol(), 0); // vtbl for TypeInfo_Wild
    dtsize_t(pdt, 0);                        // monitor
    Type *tm = tinfo->mutableOf();
    tm = tm->merge();
    tm->getTypeInfo(NULL);
    dtxoff(pdt, tm->vtinfo->toSymbol(), 0);
}

#endif

void TypeInfoTypedefDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoTypedefDeclaration::toDt() %s\n", toChars());
    verifyStructSize(Type::typeinfotypedef, 7 * PTRSIZE);

    dtxoff(pdt, Type::typeinfotypedef->toVtblSymbol(), 0); // vtbl for TypeInfo_Typedef
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Ttypedef);

    TypeTypedef *tc = (TypeTypedef *)tinfo;
    TypedefDeclaration *sd = tc->sym;
    //printf("basetype = %s\n", sd->basetype->toChars());

    /* Put out:
     *  TypeInfo base;
     *  char[] name;
     *  void[] m_init;
     */

    sd->basetype = sd->basetype->merge();
    sd->basetype->getTypeInfo(NULL);            // generate vtinfo
    assert(sd->basetype->vtinfo);
    dtxoff(pdt, sd->basetype->vtinfo->toSymbol(), 0);   // TypeInfo for basetype

    const char *name = sd->toPrettyChars();
    size_t namelen = strlen(name);
    dtsize_t(pdt, namelen);
    dtabytes(pdt, 0, namelen + 1, name);

    // void[] init;
    if (tinfo->isZeroInit() || !sd->init)
    {   // 0 initializer, or the same as the base type
        dtsize_t(pdt, 0);        // init.length
        dtsize_t(pdt, 0);        // init.ptr
    }
    else
    {
        dtsize_t(pdt, sd->type->size()); // init.length
        dtxoff(pdt, sd->toInitializer(), 0);    // init.ptr
    }
}

void TypeInfoEnumDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoEnumDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfoenum, 7 * PTRSIZE);

    dtxoff(pdt, Type::typeinfoenum->toVtblSymbol(), 0); // vtbl for TypeInfo_Enum
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tenum);

    TypeEnum *tc = (TypeEnum *)tinfo;
    EnumDeclaration *sd = tc->sym;

    /* Put out:
     *  TypeInfo base;
     *  char[] name;
     *  void[] m_init;
     */

    if (sd->memtype)
    {   sd->memtype->getTypeInfo(NULL);
        dtxoff(pdt, sd->memtype->vtinfo->toSymbol(), 0);        // TypeInfo for enum members
    }
    else
        dtsize_t(pdt, 0);

    const char *name = sd->toPrettyChars();
    size_t namelen = strlen(name);
    dtsize_t(pdt, namelen);
    dtabytes(pdt, 0, namelen + 1, name);

    // void[] init;
    if (!sd->defaultval || tinfo->isZeroInit())
    {   // 0 initializer, or the same as the base type
        dtsize_t(pdt, 0);        // init.length
        dtsize_t(pdt, 0);        // init.ptr
    }
    else
    {
        dtsize_t(pdt, sd->type->size()); // init.length
        dtxoff(pdt, sd->toInitializer(), 0);    // init.ptr
    }
}

void TypeInfoPointerDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoPointerDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfopointer, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfopointer->toVtblSymbol(), 0); // vtbl for TypeInfo_Pointer
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tpointer);

    TypePointer *tc = (TypePointer *)tinfo;

    tc->next->getTypeInfo(NULL);
    dtxoff(pdt, tc->next->vtinfo->toSymbol(), 0); // TypeInfo for type being pointed to
}

void TypeInfoArrayDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoArrayDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfoarray, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfoarray->toVtblSymbol(), 0); // vtbl for TypeInfo_Array
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tarray);

    TypeDArray *tc = (TypeDArray *)tinfo;

    tc->next->getTypeInfo(NULL);
    dtxoff(pdt, tc->next->vtinfo->toSymbol(), 0); // TypeInfo for array of type
}

void TypeInfoStaticArrayDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoStaticArrayDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfostaticarray, 4 * PTRSIZE);

    dtxoff(pdt, Type::typeinfostaticarray->toVtblSymbol(), 0); // vtbl for TypeInfo_StaticArray
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tsarray);

    TypeSArray *tc = (TypeSArray *)tinfo;

    tc->next->getTypeInfo(NULL);
    dtxoff(pdt, tc->next->vtinfo->toSymbol(), 0); // TypeInfo for array of type

    dtsize_t(pdt, tc->dim->toInteger());         // length
}

void TypeInfoVectorDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoVectorDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfovector, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfovector->toVtblSymbol(), 0); // vtbl for TypeInfo_Vector
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tvector);

    TypeVector *tc = (TypeVector *)tinfo;

    tc->basetype->getTypeInfo(NULL);
    dtxoff(pdt, tc->basetype->vtinfo->toSymbol(), 0); // TypeInfo for equivalent static array
}

void TypeInfoAssociativeArrayDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoAssociativeArrayDeclaration::toDt()\n");
#if DMDV2
    verifyStructSize(Type::typeinfoassociativearray, 5 * PTRSIZE);
#else
    verifyStructSize(Type::typeinfoassociativearray, 4 * PTRSIZE);
#endif

    dtxoff(pdt, Type::typeinfoassociativearray->toVtblSymbol(), 0); // vtbl for TypeInfo_AssociativeArray
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Taarray);

    TypeAArray *tc = (TypeAArray *)tinfo;

    tc->next->getTypeInfo(NULL);
    dtxoff(pdt, tc->next->vtinfo->toSymbol(), 0); // TypeInfo for array of type

    tc->index->getTypeInfo(NULL);
    dtxoff(pdt, tc->index->vtinfo->toSymbol(), 0); // TypeInfo for array of type

#if DMDV2
    tc->getImpl()->type->getTypeInfo(NULL);
    dtxoff(pdt, tc->getImpl()->type->vtinfo->toSymbol(), 0);    // impl
#endif
}

void TypeInfoFunctionDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoFunctionDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfofunction, 5 * PTRSIZE);

    dtxoff(pdt, Type::typeinfofunction->toVtblSymbol(), 0); // vtbl for TypeInfo_Function
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tfunction);

    TypeFunction *tc = (TypeFunction *)tinfo;

    tc->next->getTypeInfo(NULL);
    dtxoff(pdt, tc->next->vtinfo->toSymbol(), 0); // TypeInfo for function return value

    const char *name = tinfo->deco;
    assert(name);
    size_t namelen = strlen(name);
    dtsize_t(pdt, namelen);
    dtabytes(pdt, 0, namelen + 1, name);
}

void TypeInfoDelegateDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoDelegateDeclaration::toDt()\n");
    verifyStructSize(Type::typeinfodelegate, 5 * PTRSIZE);

    dtxoff(pdt, Type::typeinfodelegate->toVtblSymbol(), 0); // vtbl for TypeInfo_Delegate
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tdelegate);

    TypeDelegate *tc = (TypeDelegate *)tinfo;

    tc->next->nextOf()->getTypeInfo(NULL);
    dtxoff(pdt, tc->next->nextOf()->vtinfo->toSymbol(), 0); // TypeInfo for delegate return value

    const char *name = tinfo->deco;
    assert(name);
    size_t namelen = strlen(name);
    dtsize_t(pdt, namelen);
    dtabytes(pdt, 0, namelen + 1, name);
}

void TypeInfoStructDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoStructDeclaration::toDt() '%s'\n", toChars());
    if (global.params.is64bit)
        verifyStructSize(Type::typeinfostruct, 17 * PTRSIZE);
    else
        verifyStructSize(Type::typeinfostruct, 15 * PTRSIZE);

    dtxoff(pdt, Type::typeinfostruct->toVtblSymbol(), 0); // vtbl for TypeInfo_Struct
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tstruct);

    TypeStruct *tc = (TypeStruct *)tinfo;
    StructDeclaration *sd = tc->sym;

    /* Put out:
     *  char[] name;
     *  void[] init;
     *  hash_t function(in void*) xtoHash;
     *  bool function(in void*, in void*) xopEquals;
     *  int function(in void*, in void*) xopCmp;
     *  string function(const(void)*) xtoString;
     *  uint m_flags;
     *  //xgetMembers;
     *  xdtor;
     *  xpostblit;
     *  uint m_align;
     *  version (X86_64)
     *      TypeInfo m_arg1;
     *      TypeInfo m_arg2;
     *  xgetRTInfo
     */

    const char *name = sd->toPrettyChars();
    size_t namelen = strlen(name);
    dtsize_t(pdt, namelen);
    dtabytes(pdt, 0, namelen + 1, name);

    // void[] init;
    dtsize_t(pdt, sd->structsize);       // init.length
    if (sd->zeroInit)
        dtsize_t(pdt, 0);                // NULL for 0 initialization
    else
        dtxoff(pdt, sd->toInitializer(), 0);    // init.ptr

    FuncDeclaration *fd;
    FuncDeclaration *fdx;
    Dsymbol *s;

    static TypeFunction *tftohash;
    static TypeFunction *tftostring;

    if (!tftohash)
    {
        Scope sc;

        /* const hash_t toHash();
         */
        tftohash = new TypeFunction(NULL, Type::thash_t, 0, LINKd);
        tftohash->mod = MODconst;
        tftohash = (TypeFunction *)tftohash->semantic(0, &sc);

        tftostring = new TypeFunction(NULL, Type::tchar->invariantOf()->arrayOf(), 0, LINKd);
        tftostring = (TypeFunction *)tftostring->semantic(0, &sc);
    }

    TypeFunction *tfcmpptr;
    {
        Scope sc;

        /* const int opCmp(ref const KeyType s);
         */
        Parameters *arguments = new Parameters;

        // arg type is ref const T
        Parameter *arg = new Parameter(STCref, tc->constOf(), NULL, NULL);

        arguments->push(arg);
        tfcmpptr = new TypeFunction(arguments, Type::tint32, 0, LINKd);
        tfcmpptr->mod = MODconst;
        tfcmpptr = (TypeFunction *)tfcmpptr->semantic(0, &sc);
    }

    s = search_function(sd, Id::tohash);
    fdx = s ? s->isFuncDeclaration() : NULL;
    if (fdx)
    {   fd = fdx->overloadExactMatch(tftohash);
        if (fd)
        {
            dtxoff(pdt, fd->toSymbol(), 0);
            TypeFunction *tf = (TypeFunction *)fd->type;
            assert(tf->ty == Tfunction);
            /* I'm a little unsure this is the right way to do it. Perhaps a better
             * way would to automatically add these attributes to any struct member
             * function with the name "toHash".
             * So I'm leaving this here as an experiment for the moment.
             */
            if (!tf->isnothrow || tf->trust == TRUSTsystem /*|| tf->purity == PUREimpure*/)
                warning(fd->loc, "toHash() must be declared as extern (D) size_t toHash() const nothrow @safe, not %s", tf->toChars());
        }
        else
        {
            //fdx->error("must be declared as extern (D) uint toHash()");
            dtsize_t(pdt, 0);
        }
    }
    else
        dtsize_t(pdt, 0);

    if (sd->xeq)
        dtxoff(pdt, sd->xeq->toSymbol(), 0);
    else
        dtsize_t(pdt, 0);

    s = search_function(sd, Id::cmp);
    fdx = s ? s->isFuncDeclaration() : NULL;
    if (fdx)
    {
        //printf("test1 %s, %s, %s\n", fdx->toChars(), fdx->type->toChars(), tfeqptr->toChars());
        fd = fdx->overloadExactMatch(tfcmpptr);
        if (fd)
        {   dtxoff(pdt, fd->toSymbol(), 0);
            //printf("test2\n");
        }
        else
            //fdx->error("must be declared as extern (D) int %s(%s*)", fdx->toChars(), sd->toChars());
            dtsize_t(pdt, 0);
    }
    else
        dtsize_t(pdt, 0);

    s = search_function(sd, Id::tostring);
    fdx = s ? s->isFuncDeclaration() : NULL;
    if (fdx)
    {   fd = fdx->overloadExactMatch(tftostring);
        if (fd)
            dtxoff(pdt, fd->toSymbol(), 0);
        else
            //fdx->error("must be declared as extern (D) char[] toString()");
            dtsize_t(pdt, 0);
    }
    else
        dtsize_t(pdt, 0);

    // uint m_flags;
    size_t m_flags = tc->hasPointers();
    dtsize_t(pdt, m_flags);

#if DMDV2
#if 0
    // xgetMembers
    FuncDeclaration *sgetmembers = sd->findGetMembers();
    if (sgetmembers)
        dtxoff(pdt, sgetmembers->toSymbol(), 0);
    else
        dtsize_t(pdt, 0);                        // xgetMembers
#endif

    // xdtor
    FuncDeclaration *sdtor = sd->dtor;
    if (sdtor)
        dtxoff(pdt, sdtor->toSymbol(), 0);
    else
        dtsize_t(pdt, 0);                        // xdtor

    // xpostblit
    FuncDeclaration *spostblit = sd->postblit;
    if (spostblit && !(spostblit->storage_class & STCdisable))
        dtxoff(pdt, spostblit->toSymbol(), 0);
    else
        dtsize_t(pdt, 0);                        // xpostblit
#endif

    // uint m_align;
    dtsize_t(pdt, tc->alignsize());

    if (global.params.is64bit)
    {
        Type *t = sd->arg1type;
        for (int i = 0; i < 2; i++)
        {
            // m_argi
            if (t)
            {
                t->getTypeInfo(NULL);
                dtxoff(pdt, t->vtinfo->toSymbol(), 0);
            }
            else
                dtsize_t(pdt, 0);

            t = sd->arg2type;
        }
    }

    // xgetRTInfo
    if (sd->getRTInfo)
        sd->getRTInfo->toDt(pdt);
    else if (m_flags)
        dtsize_t(pdt, 1);       // has pointers
    else
        dtsize_t(pdt, 0);       // no pointers
}

void TypeInfoClassDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoClassDeclaration::toDt() %s\n", tinfo->toChars());
#if DMDV1
    verifyStructSize(Type::typeinfoclass, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfoclass->toVtblSymbol(), 0); // vtbl for TypeInfoClass
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tclass);

    TypeClass *tc = (TypeClass *)tinfo;
    Symbol *s;

    if (!tc->sym->vclassinfo)
        tc->sym->vclassinfo = new ClassInfoDeclaration(tc->sym);
    s = tc->sym->vclassinfo->toSymbol();
    dtxoff(pdt, s, 0);          // ClassInfo for tinfo
#else
    assert(0);
#endif
}

void TypeInfoInterfaceDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoInterfaceDeclaration::toDt() %s\n", tinfo->toChars());
    verifyStructSize(Type::typeinfointerface, 3 * PTRSIZE);

    dtxoff(pdt, Type::typeinfointerface->toVtblSymbol(), 0); // vtbl for TypeInfoInterface
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Tclass);

    TypeClass *tc = (TypeClass *)tinfo;
    Symbol *s;

    if (!tc->sym->vclassinfo)
#if DMDV1
        tc->sym->vclassinfo = new ClassInfoDeclaration(tc->sym);
#else
        tc->sym->vclassinfo = new TypeInfoClassDeclaration(tc);
#endif
    s = tc->sym->vclassinfo->toSymbol();
    dtxoff(pdt, s, 0);          // ClassInfo for tinfo
}

void TypeInfoTupleDeclaration::toDt(dt_t **pdt)
{
    //printf("TypeInfoTupleDeclaration::toDt() %s\n", tinfo->toChars());
    verifyStructSize(Type::typeinfotypelist, 4 * PTRSIZE);

    dtxoff(pdt, Type::typeinfotypelist->toVtblSymbol(), 0); // vtbl for TypeInfoInterface
    dtsize_t(pdt, 0);                        // monitor

    assert(tinfo->ty == Ttuple);

    TypeTuple *tu = (TypeTuple *)tinfo;

    size_t dim = tu->arguments->dim;
    dtsize_t(pdt, dim);                      // elements.length

    dt_t *d = NULL;
    for (size_t i = 0; i < dim; i++)
    {   Parameter *arg = (*tu->arguments)[i];
        Expression *e = arg->type->getTypeInfo(NULL);
        e = e->optimize(WANTvalue);
        e->toDt(&d);
    }

    Symbol *s;
    s = static_sym();
    s->Sdt = d;
    outdata(s);

    dtxoff(pdt, s, 0);              // elements.ptr
}

