
// Copyright (c) 1999-2012 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com

#ifndef PORT_H
#define PORT_H

// Portable wrapper around compiler/system specific things.
// The idea is to minimize #ifdef's in the app code.

#include <stdint.h>

#include "longdouble.h"

#if _MSC_VER
typedef __int64 longlong;
typedef unsigned __int64 ulonglong;
#else
typedef long long longlong;
typedef unsigned long long ulonglong;
#endif

struct Port
{
    static longdouble nan;
    static longdouble snan;
    static longdouble infinity;

    static void init();

    static int isNan(longdouble);
    static int isSignallingNan(longdouble);
    static int isInfinity(longdouble);

    static longdouble fmodl(longdouble x, longdouble y);

    static char *strupr(char *);

    static int memicmp(const char *s1, const char *s2, int n);
    static int stricmp(const char *s1, const char *s2);
};

#endif
