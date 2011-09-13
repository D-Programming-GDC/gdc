/* GDC -- D front-end for GCC
   Copyright (C) 2007 David Friedman

   Modified by
    Michael Parrott, (C) 2010

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

// from DMD
#include "enum.h"
#include "init.h"
#include "symbol.h"
#include "d-lang.h"
#include "d-codegen.h"

// Declared in dmd/mangle.c
char *cpp_mangle(Dsymbol *s);

static void
to_base36(unsigned n, OutBuffer * buf)
{
    static const char base_36_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (! n)
    {
        buf->writeByte('0');
        return;
    }

    char cbuf[64];
    char *p = cbuf + sizeof(cbuf);

    while (n && p > cbuf)
    {
        unsigned d = n / 36;
        *--p = base_36_digits[n - d * 36];
        n = d;
    }
    buf->write(p, sizeof(cbuf) - (p - cbuf));
}

struct CppMangleState
{
    Dsymbol * topSymbol;
    Voids substitutions;

    bool hasSubstitute(void * p, OutBuffer * buf)
    {
        for (size_t i = 0; i < substitutions.dim; i++)
            if (substitutions.tdata()[i] == p)
            {
                if (buf)
                {
                    buf->writeByte('S');
                    if (i)
                        to_base36(i - 1, buf);
                    buf->writeByte('_');
                }
                return true;
            }
        return false;
    }

    void add(void * p)
    {
        substitutions.push(p);
    }
};

static void
cpp_mangle_arguments(TypeFunction * tf, OutBuffer * buf, CppMangleState *cms)
{
    bool have_some_args = false;

    if (tf->parameters)
    {
        size_t dim = Parameter::dim(tf->parameters);
        for (size_t i = 0; i < dim; i++)
        {   Parameter *arg = Parameter::getNth(tf->parameters, i);
            Type * type = arg->type;

            have_some_args = true;
            if (arg->storageClass & (STClazy))
            {   // DMD does not report an error...
                cms->topSymbol->error("cannot represent lazy parameter in C++");
            }
            else if (arg->storageClass & (STCout | STCref))
            {
                type->referenceTo()->toCppMangle(buf, cms);
                continue;
            }
            else if (type->ty == Tsarray)
            {   /* C++ would encode as pointer-to-elem-type, but DMD encodes
                   as pointer-to-array-type. */
                type->pointerTo()->toCppMangle(buf, cms);
                continue;
            }
#if V2
            // %% basic, enum and struct types not marked as const.
            if ((type->ty == Tenum || type->ty == Tstruct || type->isTypeBasic()) && type->isConst())
                type->mutableOf()->toCppMangle(buf, cms);
            else
#endif
            {
                arg->type->toCppMangle(buf, cms);
            }
        }
    }
    if (tf->varargs == 1)
        buf->writeByte('z');
    else if (! have_some_args)
        buf->writeByte('v');
}

static void
cpp_mangle1(Dsymbol *sthis, OutBuffer * buf, CppMangleState * cms)
{
    if (cms->hasSubstitute(sthis, buf))
        return;

    Dsymbol * s = sthis;
    bool is_nested_ident = false;
    FuncDeclaration * fd;
    Dsymbols pfxs;

    do
    {
        if ( s != sthis && s->isFuncDeclaration() )
        {
            buf->writeByte('Z');
            cpp_mangle1(s, buf, cms);
            buf->writeByte('E');
            break;
        }
        if (s != sthis)
            is_nested_ident = true;
        pfxs.push(s);

        s = s->parent; // %% ?
    } while (s && ! s->isModule());

    if (is_nested_ident)
        buf->writeByte('N');

    unsigned ii;
    for (ii = 0; ii < pfxs.dim; ++ii)
    {
        s = pfxs.tdata()[ii];
        if (cms->hasSubstitute(s, buf))
            break;
    }
    while (ii > 0)
    {
        s = pfxs.tdata()[--ii];
        if (s->isCtorDeclaration())
        {
            buf->writeByte('C');
            buf->writeByte('1');
        }
        else if (s->ident)
        {
            buf->printf("%d", (int) s->ident->len);
            buf->write(s->ident->string, s->ident->len);
        }
        else
            buf->writeByte('0');
        if (! s->isFuncDeclaration())
            cms->add(s);
    }

    if (is_nested_ident)
        buf->writeByte('E');

    if ( (fd = sthis->isFuncDeclaration()) )
    {
        cpp_mangle_arguments((TypeFunction *) fd->type, buf, cms);
    }
}

char *
cpp_mangle(Dsymbol *s)
{
    OutBuffer o;
    CppMangleState cms;
    memset(&cms, 0, sizeof(cms));

    cms.substitutions.setDim(0);
    cms.topSymbol = s;

    o.writestring("_Z");
    cpp_mangle1(s, & o, & cms);
    o.toChars();

    return (char*) o.extractData();
}

void
Type::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    //error("cannot represent type '%s' in C++", toChars());
    // DMD uses D type mangle.
    if (! cms->hasSubstitute(this, buf))
    {
        OutBuffer o;
        toDecoBuffer(& o, 0);
        buf->printf("%d", (int) o.offset);
        buf->write(& o);
        cms->add(this);
    }
}

static void
cpp_mangle_fp(Type * t, const char * mngl, OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(t, buf))
    {
        buf->writestring(mngl);
        cms->add(t);
    }
}

void
TypeBasic::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    char c;
    const char * s;

    switch (ty)
    {
        case Tvoid: c = 'v'; break;
        case Tint8: c = 'a'; break;
        case Tuns8: c = 'h'; break;
        case Tint16: c = 's'; break;
        case Tuns16: c = 't'; break;
        case Tint32: c = 'i'; break;
        case Tuns32: c = 'j'; break;
        case Tint64: c = 'x'; break;
        case Tuns64: c = 'y'; break;
        case Tfloat32: c = 'f'; break;
        case Tfloat64: c = 'd'; break;
        case Tfloat80: c = 'e'; break;  // %% could change in the future when
                                        // D real vs. C long double type is corrected.
        case Timaginary32: s = "Gf"; goto do_fp;
        case Timaginary64: s = "Gd"; goto do_fp;
        case Timaginary80: s = "Ge"; goto do_fp; // %% ditto
        case Tcomplex32: s = "Cf"; goto do_fp;
        case Tcomplex64: s = "Cd"; goto do_fp;
        case Tcomplex80: s = "Ce";  // %% ditto
    do_fp:
            cpp_mangle_fp(this, s, buf, cms);
            return;

        case Tbool: c = 'b'; break;

        case Tchar: c = 'c'; break;
#ifdef WCHAR_TYPE_SIZE
        case Twchar:
            if (WCHAR_TYPE_SIZE == 16)
                c = 'w';
            else
                c = 't';
            break;
        case Tdchar:
            if (WCHAR_TYPE_SIZE == 32)
                c = 'w';
            else
                c = 'j';
            break;
#else
        case Twchar: c = 't'; break;
        case Tdchar: c = 'j'; break;
#endif
        default:
            Type::toCppMangle(buf, cms);
            return;
    }

    if (isConst())
    {
        buf->writeByte('K');
        cms->add(this);
    }

    buf->writeByte(c);
}

void
TypeSArray::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        if (dim)
            buf->printf("A%"PRIuMAX, dim->toInteger());
        buf->writeByte('_');
        if (next)
            next->toCppMangle(buf, cms);
        gcc_assert(! cms->hasSubstitute(this, NULL));
        cms->add(this);
    }
}

void
TypeDArray::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    return Type::toCppMangle(buf, cms);
}

void
TypeAArray::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    return Type::toCppMangle(buf, cms);
}

void
TypePointer::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        buf->writeByte('P');
        if (next)
            next->toCppMangle(buf, cms);
        gcc_assert(! cms->hasSubstitute(this, NULL));
        cms->add(this);
    }
}

void
TypeReference::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        buf->writeByte('R');
        if (next)
            next->toCppMangle(buf, cms);
        gcc_assert(! cms->hasSubstitute(this, NULL));
        cms->add(this);
    }
}

void
TypeFunction::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        buf->writeByte('F');
        if (linkage == LINKc)
            buf->writeByte('Y');
        if (next)
            next->toCppMangle(buf, cms);
        cpp_mangle_arguments(this, buf, cms);
        buf->writeByte('E');
        gcc_assert(! cms->hasSubstitute(this, NULL));
        cms->add(this);
    }
}

void
TypeDelegate::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    Type::toCppMangle(buf, cms);
}

void
TypeStruct::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        if (isConst())
            buf->writeByte('K');

        cpp_mangle1(sym, buf, cms);

        if (isConst())
            cms->add(this);
    }
}

void
TypeEnum::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        if (isConst())
            buf->writeByte('K');

        cpp_mangle1(sym, buf, cms);

        if (isConst())
            cms->add(this);
    }
}

void
TypeTypedef::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    // DMD uses the D mangled typedef name.
    Type::toCppMangle(buf, cms);
}

void
TypeClass::toCppMangle(OutBuffer *buf, CppMangleState *cms)
{
    if (! cms->hasSubstitute(this, buf))
    {
        buf->writeByte('P');
        cpp_mangle1(sym, buf, cms);
        gcc_assert(! cms->hasSubstitute(this, NULL));
        cms->add(this);
    }
}
