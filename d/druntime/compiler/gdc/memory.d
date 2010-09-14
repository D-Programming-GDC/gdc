/**
 * This module exposes functionality for inspecting and manipulating memory.
 *
 * Copyright: Copyright (C) 2005-2006 Digital Mars, www.digitalmars.com.
 *            All rights reserved.
 * License:
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, in both source and binary form, subject to the following
 *  restrictions:
 *
 *  o  The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  o  Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *  o  This notice may not be removed or altered from any source
 *     distribution.
 * Authors:   Walter Bright, Sean Kelly
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by Iain Buclaw, September 2010.
*/
module rt.memory;


private
{
    version( GNU )
    {
	import gcgccextern;

	version( GC_Use_Stack_Guess )
	    import gc_guess_stack;

	version( GC_Use_Stack_FreeBSD )
	    extern (C) int _d_gcc_gc_freebsd_stack(void **);
    }
}


/**
 *
 */
extern (C) void* rt_stackBottom()
{
    version( Windows ) // TODO: Does this work with MinGW?
    {
        asm
        {
            naked;
            mov EAX,FS:4;
            ret;
        }
    }
    else version( GC_Use_Stack_GLibC )
    {
	return __libc_stack_end;
    }
    else version( GC_Use_Stack_Guess )
    {
	return stackOriginGuess;
    }
    else version( GC_Use_Stack_FreeBSD )
    {
	void * stack_origin;
	if( _d_gcc_gc_freebsd_stack(&stack_origin) )
	    return stack_origin;
	else
	    // No way to signal an error
	    return null;
    }
    else version( GC_Use_Stack_Scan )
    {
        static assert( false, "Operating system not supported." );
    }
    else version( GC_Use_Stack_Fixed )
    {
	version( darwin )
	{
	    static if( size_t.sizeof == 4 )
		return cast(void*) 0xc0000000;
	    else static if( size_t.sizeof == 8 )
		return cast(void*) 0x7ffff_00000000UL;
	    else
		static assert( false, "Operating system not supported." );
	}
    }
    else
    {
        static assert( false, "Operating system not supported." );
    }
}


/**
 *
 */
extern (C) void* rt_stackTop()
{
    // OK to use asm? or use the fallback method...
    version( D_InlineAsm_X86 )
    {
        asm
        {
            naked;
            mov EAX, ESP;
            ret;
        }
    }
    else version( D_InlineAsm_X86_64 )
    {
	asm
	{
	    naked;
	    mov RAX, RSP;
	    ret;
	}
    }
    else
    {
	// TODO: add builtin for using stack pointer rtx
	int dummy;
	void * p = & dummy + 1; // +1 doesn't help much; also assume stack grows down
	p = cast(void*)( (cast(size_t) p) & ~(size_t.sizeof - 1));
	return p;
    }
}


private
{
    version( Windows )
    {
        extern (C)
        {
            extern int _xi_a;   // &_xi_a just happens to be start of data segment
            extern int _edata;  // &_edata is start of BSS segment
            extern int _end;    // &_end is past end of BSS
        }
    }
    else version( linux )
    {
        extern (C)
        {
            extern int _data;
            extern int __data_start;
            extern int _end;
            extern int _data_start__;
            extern int _data_end__;
            extern int _bss_start__;
            extern int _bss_end__;
            extern int __fini_array_end;
        }

            alias __data_start  Data_Start;
            alias _end          Data_End;
    }

    alias void delegate( void*, void* ) scanFn;
}


/**
 *
 */
extern (C) void rt_scanStaticData( scanFn scan )
{
    scan(rt_staticDataBottom(), rt_staticDataTop());
}

/**
 *
 */
extern (C) void* rt_staticDataBottom()
{
    version( Windows )
    {
        return &_xi_a;
    }
    else version( linux )
    {
        return &__data_start;
    }
    else
    {
        static assert( false, "Operating system not supported." );
    }
}

/**
 *
 */
extern (C) void* rt_staticDataTop()
{
    version( Windows )
    {
        return &_end;
    }
    else version( linux )
    {
        return &_end;
    }
    else
    {
        static assert( false, "Operating system not supported." );
    }
}


