/**
 * Written in the D programming language.
 * This module provides Win32-specific support for sections.
 *
 * Copyright: Copyright Digital Mars 2008 - 2012.
 * License: Distributed under the
 *      $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost Software License 1.0).
 *    (See accompanying file LICENSE)
 * Authors:   Walter Bright, Sean Kelly, Martin Nowak
 * Source: $(DRUNTIMESRC src/rt/_sections_win32.d)
 */

module rt.sections_mingw;

version(MinGW):

// debug = PRINTF;
debug(PRINTF) import core.stdc.stdio;
import rt.minfo;

struct SectionGroup
{
    static int opApply(scope int delegate(ref SectionGroup) dg)
    {
        return dg(_sections);
    }

    static int opApplyReverse(scope int delegate(ref SectionGroup) dg)
    {
        return dg(_sections);
    }

    @property inout(ModuleInfo*)[] modules() inout
    {
        return _moduleGroup.modules;
    }

    @property ref inout(ModuleGroup) moduleGroup() inout
    {
        return _moduleGroup;
    }

    @property inout(void[])[] gcRanges() inout
    {
        return _gcRanges[];
    }

private:
    ModuleGroup _moduleGroup;
    void[][1] _gcRanges;
}

void initSections()
{
    _sections._moduleGroup = ModuleGroup(getModuleInfos());

    version (GNU)
    {
        _sections._gcRanges[0] = (&_data_start__)[0 .. &_data_start__ - &_bss_end__];
    }
    else
    {
        auto pbeg = cast(void*)&_xi_a;
        auto pend = cast(void*)&_end;
        _sections._gcRanges[0] = pbeg[0 .. pend - pbeg];
    }
}

void finiSections()
{
}

void[] initTLSRanges()
{
    auto pbeg = cast(void*)&_tlsstart;
    auto pend = cast(void*)&_tlsend;
    return pbeg[0 .. pend - pbeg];
}

void finiTLSRanges(void[] rng)
{
}

void scanTLSRanges(void[] rng, scope void delegate(void* pbeg, void* pend) dg)
{
    dg(rng.ptr, rng.ptr + rng.length);
}

private:

__gshared SectionGroup _sections;

// Windows: this gets initialized by minit.asm
extern(C) __gshared ModuleInfo*[] _moduleinfo_array;
extern(C) void _minit();

ModuleInfo*[] getModuleInfos()
out (result)
{
    foreach(m; result)
        assert(m !is null);
}
body
{
    version (GNU)
    {
        return (&__start_dminfo)[0 .. &__stop_dminfo - &__start_dminfo];
    }
    else
    {
        // _minit directly alters the global _moduleinfo_array
        _minit();
        return _moduleinfo_array;
    }
}

version (GNU)
{
    // NOTE: The memory between the addresses of _tls_start and _tls_end
    // is the storage for thread-local data in MinGW. Both of
    // these are defined in tlssup.c.
    extern (C)
    {
        extern int _tls_start;
        extern int _tls_end;
    }
    alias _tls_start _tlsstart;
    alias _tls_end _tlsend;

    extern (C)
    {
        extern __gshared
        {
            ModuleInfo* __start_dminfo;
            ModuleInfo* __stop_dminfo;
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
else
{
    extern(C)
    {
        extern __gshared
        {
            int _xi_a;      // &_xi_a just happens to be start of data segment
            //int _edata;   // &_edata is start of BSS segment
            int _end;       // &_end is past end of BSS
        }

        extern
        {
            int _tlsstart;
            int _tlsend;
        }
    }
}
