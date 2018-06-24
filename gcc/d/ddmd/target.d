/**
 * Compiler implementation of the
 * $(LINK2 http://www.dlang.org, D programming language).
 *
 * Copyright:   Copyright (c) 1999-2017 by Digital Mars, All Rights Reserved
 * Authors:     $(LINK2 http://www.digitalmars.com, Walter Bright)
 * License:     $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:      $(LINK2 https://github.com/dlang/dmd/blob/master/src/ddmd/target.d, _target.d)
 */

module ddmd.target;

// Online documentation: https://dlang.org/phobos/ddmd_target.html

import ddmd.dclass;
import ddmd.dmodule;
import ddmd.dsymbol;
import ddmd.expression;
import ddmd.globals;
import ddmd.mtype;
import ddmd.tokens : TOK;
import ddmd.root.ctfloat;
import ddmd.root.outbuffer;

/***********************************************************
 */
struct Target
{
    extern (C++) static __gshared int ptrsize;
    extern (C++) static __gshared int realsize;             // size a real consumes in memory
    extern (C++) static __gshared int realpad;              // 'padding' added to the CPU real size to bring it up to realsize
    extern (C++) static __gshared int realalignsize;        // alignment for reals
    extern (C++) static __gshared bool reverseCppOverloads; // with dmc and cl, overloaded functions are grouped and in reverse order
    extern (C++) static __gshared bool cppExceptions;       // set if catching C++ exceptions is supported
    extern (C++) static __gshared int c_longsize;           // size of a C 'long' or 'unsigned long' type
    extern (C++) static __gshared int c_long_doublesize;    // size of a C 'long double'
    extern (C++) static __gshared int classinfosize;        // size of 'ClassInfo'
    extern (C++) static __gshared ulong maxStaticDataSize;  // maximum size of static data

    extern (C++) struct FPTypeProperties(T)
    {
        static __gshared
        {
            real_t max;
            real_t min_normal;
            real_t nan;
            real_t snan;
            real_t infinity;
            real_t epsilon;

            d_int64 dig;
            d_int64 mant_dig;
            d_int64 max_exp;
            d_int64 min_exp;
            d_int64 max_10_exp;
            d_int64 min_10_exp;
        }
    }

    alias FloatProperties = FPTypeProperties!float;
    alias DoubleProperties = FPTypeProperties!double;
    alias RealProperties = FPTypeProperties!real_t;

    extern (C++) static void _init();
    extern (C++) static uint alignsize(Type type);
    extern (C++) static uint fieldalign(Type type);
    extern (C++) static uint critsecsize();
    extern (C++) static Type va_listType();
    extern (C++) static int isVectorTypeSupported(int sz, Type type);
    extern (C++) static bool isVectorOpSupported(Type type, TOK op, Type t2 = null);
    extern (C++) static Expression paintAsType(Expression e, Type type);
    extern (C++) static void loadModule(Module m);
    extern (C++) static void prefixName(OutBuffer* buf, LINK linkage);
    extern (C++) static const(char)* toCppMangle(Dsymbol s);
    extern (C++) static const(char)* cppTypeInfoMangle(ClassDeclaration cd);
    extern (C++) static const(char)* cppTypeMangle(Type t);
    extern (C++) static LINK systemLinkage();
}
