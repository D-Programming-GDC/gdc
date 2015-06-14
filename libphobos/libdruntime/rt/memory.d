/**
 * This module tells the garbage collector about the static data and bss segments,
 * so the GC can scan them for roots. It does not deal with thread local static data.
 *
 * Copyright: Copyright Digital Mars 2000 - 2012.
 * License: Distributed under the
 *      $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost Software License 1.0).
 *    (See accompanying file LICENSE)
 * Authors:   Walter Bright, Sean Kelly
 * Source: $(DRUNTIMESRC src/rt/_memory.d)
 */

/* NOTE: This file has been patched from the original DMD distribution to
 * work with the GDC compiler.
 */
module rt.memory;


import core.memory;
private
{
    version( MinGW )
    {
        extern (C)
        {
            extern __gshared
            {
                // We need to alias __data_*__ and __bss_*__ to correct
                // symbols on Win64. Binutils exports the same symbol for
                // both Win32 and Win64, where-as the ABI says no _ should be
                // prefixed.
                version (X86)
                {
                    int _data_start__;
                    int _data_end__;
                    int _bss_start__;
                    int _bss_end__;
                }
                else version (X86_64)
                {
                    pragma(mangle, "__data_start__") int _data_start__;
                    pragma(mangle, "__data_end__") int _data_end__;
                    pragma(mangle, "__bss_start__") int _bss_start__;
                    pragma(mangle, "__bss_end__") int _bss_end__;
                }
            }
        }
    }
    else version( Win32 )
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
    else version( Win64 )
    {
        extern (C)
        {
            extern __gshared
            {
                int __xc_a;      // &__xc_a just happens to be start of data segment
                //int _edata;    // &_edata is start of BSS segment
                void* _deh_beg;  // &_deh_beg is past end of BSS
            }
        }
    }
    else version( linux )
    {
        extern (C)
        {
            extern __gshared
            {
                int __data_start;
                int end;
            }
        }
    }
    else version( OSX )
    {
        import core.sys.osx.mach.dyld;
        import core.sys.osx.mach.getsect;

        struct Section
        {
            immutable(char)* segment;
            immutable(char)* section;
        }

        immutable Section[3] dataSections = [
            Section(SEG_DATA, SECT_DATA),
            Section(SEG_DATA, SECT_BSS),
            Section(SEG_DATA, SECT_COMMON)
        ];
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
                int __dso_handle;
                int _end;
            }
        }
    }
}


void initStaticDataGC()
{
    version( MinGW )
    {
        GC.addRange( &_data_start__, cast(size_t) &_bss_end__ - cast(size_t) &_data_start__ );
    }
    else version( Win32 )
    {
        GC.addRange( &_xi_a, cast(size_t) &_end - cast(size_t) &_xi_a );
    }
    else version( Win64 )
    {
        GC.addRange( &__xc_a, cast(size_t) &_deh_beg - cast(size_t) &__xc_a );
    }
    else version( linux )
    {
        GC.addRange( &__data_start, cast(size_t) &end - cast(size_t) &__data_start );
    }
    else version( OSX )
    {
        static extern(C) void scanSections(in mach_header* hdr, intptr_t slide)
        {
            foreach (s; dataSections)
            {
                // Should probably be decided at runtime by actual image bitness
                // (mach_header.magic) rather than at build-time?
                version (D_LP64)
                    auto sec = getsectbynamefromheader_64(
                        cast(mach_header_64*)hdr, s.segment, s.section);
                else
                    auto sec = getsectbynamefromheader(hdr, s.segment, s.section);

                if (sec !is null && sec.size > 0)
                    GC.addRange(cast(void*)(sec.addr + slide),
                                cast(size_t)sec.size);
            }
        }

        _dyld_register_func_for_add_image(&scanSections);
    }
    else version( FreeBSD )
    {
        version (X86_64)
        {
            GC.addRange( &etext, cast(size_t) &_deh_end - cast(size_t) &etext );
            GC.addRange( &__progname, cast(size_t) &_end - cast(size_t) &__progname );
        }
        else
        {
            GC.addRange( &etext, cast(size_t) &_end - cast(size_t) &etext );
        }
    }
    else version( Solaris )
    {
        GC.addRange(&__dso_handle, cast(size_t)&_end - cast(size_t)&__dso_handle);
    }
    else
    {
        static assert( false, "Operating system not supported." );
    }
}
