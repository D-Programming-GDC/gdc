
// Compiler implementation of the D programming language
// Copyright (c) 1999-2006 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by David Friedman, December 2006
*/

#ifndef DMD_ENUM_H
#define DMD_ENUM_H

#ifdef __DMC__
#pragma once
#endif /* __DMC__ */

#include "root.h"
#include "dsymbol.h"

struct Identifier;
struct Type;
struct Expression;
#ifdef _DH
struct HdrGenState;
#endif


struct EnumDeclaration : ScopeDsymbol
{
    Type *type;			// the TypeEnum
    Type *memtype;		// type of the members
    integer_t maxval;
    integer_t minval;
    integer_t defaultval;	// default initializer
    Expressions * attributes; // GCC decl/type attributes

    EnumDeclaration(Loc loc, Identifier *id, Type *memtype);
    Dsymbol *syntaxCopy(Dsymbol *s);
    void semantic(Scope *sc);
    int oneMember(Dsymbol **ps);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *getType();
    char *kind();

    void emitComment(Scope *sc);
    void toDocBuffer(OutBuffer *buf);

    EnumDeclaration *isEnumDeclaration() { return this; }

    void toObjFile(int multiobj);			// compile to .obj file
    void toDebug();
    int cvMember(unsigned char *p);

    Symbol *sinit;
    Symbol *toInitializer();
};


struct EnumMember : Dsymbol
{
    Expression *value;

    EnumMember(Loc loc, Identifier *id, Expression *value);
    Dsymbol *syntaxCopy(Dsymbol *s);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    char *kind();

    void emitComment(Scope *sc);
    void toDocBuffer(OutBuffer *buf);

    EnumMember *isEnumMember() { return this; }
};

#endif /* DMD_ENUM_H */
