
/* Compiler implementation of the D programming language
 * Copyright (c) 1999-2017 by Digital Mars
 * All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 */

#include "mtype.h"
#include "scope.h"
#include "template.h"
#include "visitor.h"

Type *reliesOnTident(Type *t, TemplateParameters *tparams = NULL, size_t iStart = 0);
Expression *semantic(Expression *e, Scope *sc);
Type *semantic(Type *t, Loc loc, Scope *sc);
RootObject *aliasParameterSemantic(Loc loc, Scope *sc, RootObject *o, TemplateParameters *parameters);


class TemplateParameterSemanticVisitor : public Visitor
{
    Scope *sc;
    TemplateParameters *parameters;

public:
    bool result;

    TemplateParameterSemanticVisitor(Scope *sc, TemplateParameters *parameters)
    {
        this->sc = sc;
        this->parameters = parameters;
        this->result = false;
    }

    void visit(TemplateTypeParameter *ttp)
    {
        //printf("TemplateTypeParameter::semantic('%s')\n", ident->toChars());
        if (ttp->specType && !reliesOnTident(ttp->specType, parameters))
        {
            ttp->specType = semantic(ttp->specType, ttp->loc, sc);
        }
#if 0 // Don't do semantic() until instantiation
        if (defaultType)
        {
            defaultType = defaultType->semantic(ttp->loc, sc);
        }
#endif
        result = !(ttp->specType && isError(ttp->specType));
    }

    void visit(TemplateAliasParameter *tap)
    {
        if (tap->specType && !reliesOnTident(tap->specType, parameters))
        {
            tap->specType = semantic(tap->specType, tap->loc, sc);
        }
        tap->specAlias = aliasParameterSemantic(tap->loc, sc, tap->specAlias, parameters);
#if 0 // Don't do semantic() until instantiation
        if (defaultAlias)
            defaultAlias = defaultAlias->semantic(tap->loc, sc);
#endif
        result = !(tap->specType  && isError(tap->specType)) && !(tap->specAlias && isError(tap->specAlias));
    }

    void visit(TemplateValueParameter *tvp)
    {
        tvp->valType = semantic(tvp->valType, tvp->loc, sc);

#if 0   // defer semantic analysis to arg match
        if (specValue)
        {
            Expression *e = specValue;
            sc = sc->startCTFE();
            e = e->semantic(sc);
            sc = sc->endCTFE();
            e = e->implicitCastTo(sc, tvp->valType);
            e = e->ctfeInterpret();
            if (e->op == TOKint64 || e->op == TOKfloat64 ||
                e->op == TOKcomplex80 || e->op == TOKnull || e->op == TOKstring)
                specValue = e;
        }

        if (defaultValue)
        {
            Expression *e = defaultValue;
            sc = sc->startCTFE();
            e = e->semantic(sc);
            sc = sc->endCTFE();
            e = e->implicitCastTo(sc, tvp->valType);
            e = e->ctfeInterpret();
            if (e->op == TOKint64)
                defaultValue = e;
        }
#endif
        result = !isError(tvp->valType);
    }

    void visit(TemplateTupleParameter *ttp)
    {
        result = true;
    }

};

// Performs semantic on TemplateParamter AST nodes
bool semantic(TemplateParameter *tp, Scope *sc, TemplateParameters *parameters)
{
    TemplateParameterSemanticVisitor v = TemplateParameterSemanticVisitor(sc, parameters);
    tp->accept(&v);
    return v.result;
}

RootObject *aliasParameterSemantic(Loc loc, Scope *sc, RootObject *o, TemplateParameters *parameters)
{
    if (o)
    {
        Expression *ea = isExpression(o);
        Type *ta = isType(o);
        if (ta && (!parameters || !reliesOnTident(ta, parameters)))
        {
            Dsymbol *s = ta->toDsymbol(sc);
            if (s)
                o = s;
            else
                o = semantic(ta, loc, sc);
        }
        else if (ea)
        {
            sc = sc->startCTFE();
            ea = ::semantic(ea, sc);
            sc = sc->endCTFE();
            o = ea->ctfeInterpret();
        }
    }
    return o;
}

