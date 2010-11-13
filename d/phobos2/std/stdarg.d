// Written in the D programming language.

/**
 * This is for use with variable argument lists with extern(D) linkage.
 *
 * Copyright: Copyright Hauke Duden 2004 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Hauke Duden, $(WEB digitalmars.com, Walter Bright)
 *
 *          Copyright Hauke Duden 2004 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.
 */
module std.stdarg;

version (GNU)
{
    // va_list might be a pointer, but assuming so is not portable.
    private import gcc.builtins;
    alias __builtin_va_list va_list;
}
else
{
    alias void* va_list;
}
    
// va_arg is handled magically by the compiler
template va_arg(T)
{
    T va_arg(ref va_list _argptr)
    {
        T t;
        return t;
    }
}
