/**
 * D header file for C99.
 *
 * Copyright: Public Domain
 * License:   Public Domain
 * Authors:   Hauke Duden, Walter Bright
 * Standards: ISO/IEC 9899:1999 (E)
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.
 */
module core.stdc.stdarg;

version (GNU)
{
    private import gcc.builtins;
    alias __builtin_va_list va_list;
    alias __builtin_va_end va_end;
    alias __builtin_va_copy va_copy;
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
    void va_start(out va_list ap, inout T parmn)
    {
    }
}
    
template va_arg(T)
{
    T va_arg(inout va_list ap)
    {
	T t;
	return t;
    }
}
