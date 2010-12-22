/**
 * D header file for C99.
 *
 * Copyright: Copyright Digital Mars 2000 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Walter Bright, Hauke Duden
 * Standards: ISO/IEC 9899:1999 (E)
 */

/*          Copyright Digital Mars 2000 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.
 */
module core.stdc.stdarg;

@system:

version (GNU)
{
    private import gcc.builtins;
    alias __builtin_va_list va_list;
    alias __builtin_va_end va_end;
    alias __builtin_va_copy va_copy;

    version( X86_64 )
    {   // TODO: implement this?
        alias __builtin_va_list __va_argsave_t;
    }
}
else
{
    alias void* va_list;
    
    void va_end( va_list ap )
    {

    }

    void va_copy( out va_list dest, va_list src )
    {
	dest = src;
    }
}

// The va_start and va_arg template functions are magically
// handled by the compiler.
template va_start(T)
{
    void va_start(out va_list ap, ref T parmn)
    {
    }
}
    
template va_arg(T)
{
    T va_arg(ref va_list ap)
    {
	T t;
	return t;
    }
}
