
/* Compiler implementation of the D programming language
 * Copyright (c) 1999-2014 by Digital Mars
 * All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/staticassert.c
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dsymbol.h"
#include "staticassert.h"
#include "expression.h"
#include "id.h"
#include "scope.h"
#include "template.h"
#include "declaration.h"

/********************************* AttribDeclaration ****************************/

StaticAssert::StaticAssert(Loc loc, Expression *exp, Expression *msg)
        : Dsymbol(Id::empty)
{
    this->loc = loc;
    this->exp = exp;
    this->msg = msg;
}

Dsymbol *StaticAssert::syntaxCopy(Dsymbol *s)
{
    assert(!s);
    return new StaticAssert(loc, exp->syntaxCopy(), msg ? msg->syntaxCopy() : NULL);
}

void StaticAssert::addMember(Scope *sc, ScopeDsymbol *sds)
{
    // we didn't add anything
}

bool StaticAssert::oneMember(Dsymbol **ps, Identifier *ident)
{
    //printf("StaticAssert::oneMember())\n");
    *ps = NULL;
    return true;
}

const char *StaticAssert::kind()
{
    return "static assert";
}
