
/* Compiler implementation of the D programming language
 * Copyright (c) 1999-2014 by Digital Mars
 * All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/enum.c
 */

#include <stdio.h>
#include <assert.h>

#include "root.h"
#include "enum.h"
#include "scope.h"
#include "id.h"
#include "module.h"
#include "init.h"

void semantic(Dsymbol *dsym, Scope *sc);
Expression *semantic(Expression *e, Scope *sc);
Type *semantic(Type *t, Loc loc, Scope *sc);

/********************************* EnumDeclaration ****************************/

EnumDeclaration::EnumDeclaration(Loc loc, Identifier *id, Type *memtype)
    : ScopeDsymbol(id)
{
    //printf("EnumDeclaration() %s\n", toChars());
    this->loc = loc;
    type = new TypeEnum(this);
    this->memtype = memtype;
    maxval = NULL;
    minval = NULL;
    defaultval = NULL;
    sinit = NULL;
    isdeprecated = false;
    protection = Prot(PROTundefined);
    parent = NULL;
    added = false;
    inuse = 0;
}

Dsymbol *EnumDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(!s);
    EnumDeclaration *ed = new EnumDeclaration(loc, ident,
        memtype ? memtype->syntaxCopy() : NULL);
    return ScopeDsymbol::syntaxCopy(ed);
}

void EnumDeclaration::setScope(Scope *sc)
{
    if (semanticRun > PASSinit)
        return;
    ScopeDsymbol::setScope(sc);
}

void EnumDeclaration::addMember(Scope *sc, ScopeDsymbol *sds)
{
#if 0
    printf("EnumDeclaration::addMember() %s\n", toChars());
    for (size_t i = 0; i < members->dim; i++)
    {
        EnumMember *em = (*members)[i]->isEnumMember();
        printf("    member %s\n", em->toChars());
    }
#endif

    /* Anonymous enum members get added to enclosing scope.
     */
    ScopeDsymbol *scopesym = isAnonymous() ? sds : this;

    if (!isAnonymous())
    {
        ScopeDsymbol::addMember(sc, sds);

        if (!symtab)
            symtab = new DsymbolTable();
    }

    if (members)
    {
        for (size_t i = 0; i < members->dim; i++)
        {
            EnumMember *em = (*members)[i]->isEnumMember();
            em->ed = this;
            //printf("add %s to scope %s\n", em->toChars(), scopesym->toChars());
            em->addMember(sc, isAnonymous() ? scopesym : this);
        }
    }
    added = true;
}


/******************************
 * Get the value of the .max/.min property as an Expression
 * Input:
 *      id      Id::max or Id::min
 */

Expression *EnumDeclaration::getMaxMinValue(Loc loc, Identifier *id)
{
    //printf("EnumDeclaration::getMaxValue()\n");
    bool first = true;

    Expression **pval = (id == Id::max) ? &maxval : &minval;

    if (inuse)
    {
        error(loc, "recursive definition of .%s property", id->toChars());
        goto Lerrors;
    }
    if (*pval)
        goto Ldone;

    if (_scope)
        semantic(this, _scope);
    if (errors)
        goto Lerrors;
    if (semanticRun == PASSinit || !members)
    {
        error("is forward referenced looking for .%s", id->toChars());
        goto Lerrors;
    }
    if (!(memtype && memtype->isintegral()))
    {
        error(loc, "has no .%s property because base type %s is not an integral type",
                id->toChars(),
                memtype ? memtype->toChars() : "");
        goto Lerrors;
    }

    for (size_t i = 0; i < members->dim; i++)
    {
        EnumMember *em = (*members)[i]->isEnumMember();
        if (!em)
            continue;
        if (em->errors)
            goto Lerrors;

        Expression *e = em->value();
        if (first)
        {
            *pval = e;
            first = false;
        }
        else
        {
            /* In order to work successfully with UDTs,
             * build expressions to do the comparisons,
             * and let the semantic analyzer and constant
             * folder give us the result.
             */

            /* Compute:
             *   if (e > maxval)
             *      maxval = e;
             */
            Expression *ec = new CmpExp(id == Id::max ? TOKgt : TOKlt, em->loc, e, *pval);
            inuse++;
            ec = semantic(ec, em->_scope);
            inuse--;
            ec = ec->ctfeInterpret();
            if (ec->toInteger())
                *pval = e;
        }
    }
Ldone:
  {
    Expression *e = *pval;
    if (e->op != TOKerror)
    {
        e = e->copy();
        e->loc = loc;
    }
    return e;
  }

Lerrors:
    *pval = new ErrorExp();
    return *pval;
}

Expression *EnumDeclaration::getDefaultValue(Loc loc)
{
    //printf("EnumDeclaration::getDefaultValue() %p %s\n", this, toChars());
    if (defaultval)
        return defaultval;

    if (_scope)
        semantic(this, _scope);
    if (errors)
        goto Lerrors;
    if (semanticRun == PASSinit || !members)
    {
        error(loc, "forward reference of %s.init", toChars());
        goto Lerrors;
    }

    for (size_t i = 0; i < members->dim; i++)
    {
        EnumMember *em = (*members)[i]->isEnumMember();
        if (!em)
            continue;
        defaultval = em->value();
        return defaultval;
    }

Lerrors:
    defaultval = new ErrorExp();
    return defaultval;
}

Type *EnumDeclaration::getMemtype(Loc loc)
{
    if (loc.linnum == 0)
        loc = this->loc;
    if (_scope)
    {
        /* Enum is forward referenced. We don't need to resolve the whole thing,
         * just the base type
         */
        if (memtype)
            memtype = semantic(memtype, loc, _scope);
        else
        {
            if (!isAnonymous() && members)
                memtype = Type::tint32;
        }
    }
    if (!memtype)
    {
        if (!isAnonymous() && members)
            memtype = Type::tint32;
        else
        {
            error(loc, "is forward referenced looking for base type");
            return Type::terror;
        }
    }
    return memtype;
}

bool EnumDeclaration::oneMember(Dsymbol **ps, Identifier *ident)
{
    if (isAnonymous())
        return Dsymbol::oneMembers(members, ps, ident);
    return Dsymbol::oneMember(ps, ident);
}

Type *EnumDeclaration::getType()
{
    return type;
}

const char *EnumDeclaration::kind()
{
    return "enum";
}

bool EnumDeclaration::isDeprecated()
{
    return isdeprecated;
}

Prot EnumDeclaration::prot()
{
    return protection;
}

Dsymbol *EnumDeclaration::search(Loc loc, Identifier *ident, int flags)
{
    //printf("%s.EnumDeclaration::search('%s')\n", toChars(), ident->toChars());
    if (_scope)
    {
        // Try one last time to resolve this enum
        semantic(this, _scope);
    }

    if (!members || !symtab || _scope)
    {
        error("is forward referenced when looking for '%s'", ident->toChars());
        //*(char*)0=0;
        return NULL;
    }

    Dsymbol *s = ScopeDsymbol::search(loc, ident, flags);
    return s;
}

/********************************* EnumMember ****************************/

EnumMember::EnumMember(Loc loc, Identifier *id, Expression *value, Type *origType)
    : VarDeclaration(loc, NULL, id ? id : Id::empty, new ExpInitializer(loc, value))
{
    this->ed = NULL;
    this->origValue = value;
    this->origType = origType;
}

Expression *&EnumMember::value()
{
    return ((ExpInitializer*)_init)->exp;
}

Dsymbol *EnumMember::syntaxCopy(Dsymbol *s)
{
    assert(!s);
    return new EnumMember(loc, ident,
        value() ? value()->syntaxCopy() : NULL,
        origType ? origType->syntaxCopy() : NULL);
}

const char *EnumMember::kind()
{
    return "enum member";
}

Expression *EnumMember::getVarExp(Loc loc, Scope *sc)
{
    semantic(this, sc);
    if (errors)
        return new ErrorExp();
    Expression *e = new VarExp(loc, this);
    return semantic(e, sc);
}
