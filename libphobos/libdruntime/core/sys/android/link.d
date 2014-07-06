/**
 * D header file for Android
 *
 * $(LINK2 https://android.googlesource.com/platform/bionic.git/+/master/libc/include/link.h, bionic link.h)
 */
module core.sys.android.link;

version (Android):
extern (C):
nothrow:

import core.stdc.stdint : uintptr_t, uint32_t;
import core.sys.posix.config : __WORDSIZE;
import core.sys.android.elf;

alias __WORDSIZE __ELF_NATIVE_CLASS;

template ElfW(string type)
{
    mixin("alias Elf"~__ELF_NATIVE_CLASS.stringof~"_"~type~" ElfW;");
}

struct dl_phdr_info
{
    ElfW!"Addr" dlpi_addr;
    const(char)* dlpi_name;
    const(ElfW!"Phdr")* dlpi_phdr;
    ElfW!"Half" dlpi_phnum;
}

private alias extern(C) int function(dl_phdr_info*, size_t, void *) __Callback;
extern int dl_iterate_phdr(__Callback __callback, void*__data);

version (ARM)
{
    alias c_ulong* _Unwind_Ptr;
    extern(C) _Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr, int*);
}

struct link_map
{
    ElfW!"Addr" l_addr;
    char* l_name;
    ElfW!"Dyn"* l_ld;
    link_map* l_next, l_prev;
}

enum
{
    RT_CONSISTENT,
    RT_ADD,
    RT_DELETE,
}

struct r_debug
{
    int r_version;
    link_map* r_map;
    ElfW!"Addr" r_brk;
    typeof(RT_CONSISTENT) r_state;
    ElfW!"Addr" r_ldbase;
}
