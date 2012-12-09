/**
 * This module exposes functionality for inspecting and manipulating memory.
 *
 * Copyright: Copyright Digital Mars 2000 - 2010.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Walter Bright, Sean Kelly
 */

/*          Copyright Digital Mars 2000 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.
*/
module rt.memory;


private
{
    version( GNU )
    {
        import gcc.builtins;

        version( GC_Use_Data_Proc_Maps )
        {
            import rt.gccmemory;
        }
    }
    extern (C) void gc_addRange( void* p, size_t sz );
    extern (C) void gc_removeRange( void* p );


    version( MinGW )
    {
        extern (C)
        {
            extern __gshared
            {
                int _data_start__;
                int _data_end__;
                int _bss_start__;
                int _bss_end__;
            }
        }
    }
    else version( Windows )
    {
        extern (C)
        {
            extern __gshared
            {
                int _xi_a;   // &_xi_a just happens to be start of data segment
                int _edata;  // &_edata is start of BSS segment
                int _end;    // &_end is past end of BSS
            }
        }
    }
    else version( linux )
    {
        extern (C)
        {
            extern __gshared
            {
                int _data;
                int __data_start;
                int _end;
                int _data_start__;
                int _data_end__;
                int _bss_start__;
                int _bss_end__;
                int __fini_array_end;
            }
        }

            alias __data_start  Data_Start;
            alias _end          Data_End;
    }
    else version( OSX )
    {
        extern (C) void _d_osx_image_init();
    }
    else version( FreeBSD )
    {
        extern (C)
        {
            extern __gshared
            {
                size_t etext;
                size_t _end;
            }
        }
        version (X86_64)
        {
            extern (C)
            {
                extern __gshared
                {
                    size_t _deh_end;
                    size_t __progname;
                }
            }
        }
    }
    else version( Solaris )
    {
        extern (C)
        {
            extern __gshared
            {
                int etext;
                int _end;
            }
        }
    }
}


void initStaticDataGC()
{
    version( MinGW )
    {
        gc_addRange( &_data_start__, cast(size_t) &_bss_end__ - cast(size_t) &_data_start__ );
    }
    else version( Windows )
    {
        gc_addRange( &_xi_a, cast(size_t) &_end - cast(size_t) &_xi_a );
    }
    else version( linux )
    {
        gc_addRange( &__data_start, cast(size_t) &_end - cast(size_t) &__data_start );
    }
    else version( OSX )
    {
        _d_osx_image_init();
    }
    else version( FreeBSD )
    {
        version (X86_64)
        {
            gc_addRange( &etext, cast(size_t) &_deh_end - cast(size_t) &etext );
            gc_addRange( &__progname, cast(size_t) &_end - cast(size_t) &__progname );
        }
        else
        {
            gc_addRange( &etext, cast(size_t) &_end - cast(size_t) &etext );
        }
    }
    else version( Solaris )
    {
        gc_addRange( &etext, cast(size_t) &_end - cast(size_t) &etext );
    }
    else
    {
        static assert( false, "Operating system not supported." );
    }

    version( GC_Use_Data_Proc_Maps )
    {
        version( linux )
        {
            scanDataProcMaps( &__data_start, &_end );
        }
        else version( FreeBSD )
        {
            version (X86_64)
            {
                scanDataProcMaps( &etext, &_deh_end );
                scanDataProcMaps( &__progname, &_end );
            }
            else
            {
                scanDataProcMaps( &etext, &_end );
            }
        }
        else version( Solaris )
        {
            scanDataProcMaps( &etext, &_end );
        }
        else
        {
            static assert( false, "Operating system not supported." );
        }
    }
}
