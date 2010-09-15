/**
 * This module exposes functionality for inspecting and manipulating memory.
 *
 * Based off the original rt.memory template, but written to work with the GDC compiler.
 *
 * Written by Iain Buclaw, September 2010.
 */

module rt.memory;

private
{
    import gcgccextern;

    version( GC_Use_Stack_Guess )
	import gc_guess_stack;

    version( GC_Use_Stack_FreeBSD )
	extern (C) int _d_gcc_gc_freebsd_stack(void **);
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
    // Safe to use asm? or use the fallback method...
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
    const size_t S = (void *).sizeof;

    // Can't assume the input addresses are word-aligned
    static void * adjust_up(void * p)
    {
	return p + ((S - (cast(size_t)p & (S-1))) & (S-1)); // cast ok even if 64-bit
    }
    static void * adjust_down(void * p)
    {
	return p - (cast(size_t) p & (S-1));
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
    version( GC_Use_Data_Fixed )
    {
	return adjust_down(&Data_End);
    }
    else // FIXME
    {
        static assert( false, "Operating system not supported." );
    }
}

/**
 *
 */
extern (C) void* rt_staticDataTop()
{
    version( GC_Use_Data_Fixed )
    {
	return adjust_up(&Data_Start);
    }
    else // FIXME
    {
        static assert( false, "Operating system not supported." );
    }
}


