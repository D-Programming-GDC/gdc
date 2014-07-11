module core.sys.solaris.sys.auxv;

import core.stdc.config;

version(Solaris):
extern (C):
nothrow:

struct auxv_t {
    int     a_type;
    union __a_un {
        c_long          a_val;
        void*           a_ptr;
        void function() a_fcn;
    }
    __a_un  a_un;
}

version (X86) {
    version = V386;
}
version (X86_64) {
    version = V386;
}
enum {
    AT_NULL             = 0,
    AT_IGNORE           = 1,
    AT_EXECFD           = 2,
    AT_PHDR             = 3,
    AT_PHENT            = 4,
    AT_PHNUM            = 5,
    AT_PAGESZ           = 6,
    AT_BASE             = 7,
    AT_FLAGS            = 8,
    AT_ENTRY            = 9,

    // Sun extensions
    AT_SUN_UID          = 2000,
    AT_SUN_RUID         = 2001,
    AT_SUN_GID          = 2002,
    AT_SUN_RGID         = 2003,
    AT_SUN_LDELF        = 2004,
    AT_SUN_LDSHDR       = 2005,
    AT_SUN_LDNAME       = 2006,
    AT_SUN_LPAGESZ      = 2007,
    AT_SUN_PLATFORM     = 2008,
    AT_SUN_HWCAP        = 2009,
    AT_SUN_HWCAP2       = 2023,
    AT_SUN_IFLUSH       = 2010,
    AT_SUN_CPU          = 2011,
    AT_SUN_EXECNAME     = 2014,
    AT_SUN_MMU          = 2015,
    AT_SUN_LDDATA       = 2016,
    AT_SUN_AUXFLAGS     = 2017,
    AT_SUN_EMULATOR     = 2018,
    AT_SUN_BRANDNAME    = 2019,
    AT_SUN_BRAND_AUX1   = 2020,
    AT_SUN_BRAND_AUX2   = 2021,
    AT_SUN_BRAND_AUX3   = 2022,

    AT_SUN_SETUGID      = 0x00000001,
    AT_SUN_HWCAPVERIFY  = 0x00000002,
    AT_SUN_SUN_NOPLM    = 0x00000004,
}

version (V386) {
    enum {
        AV_386_FPU          = 0x00001,
        AV_386_TSC          = 0x00002,
        AV_386_CX8          = 0x00004,
        AV_386_SEP          = 0x00008,
        AV_386_AMD_SYSC     = 0x00010,
        AV_386_CMOV         = 0x00020,
        AV_386_MMX          = 0x00040,
        AV_386_AMD_MMX      = 0x00080,
        AV_386_AMD_3DNow    = 0x00100,
        AV_386_AMD_3DNowx   = 0x00200,
        AV_386_FXSR         = 0x00400,
        AV_386_SSE          = 0x00800,
        AV_386_SSE2         = 0x01000,
                           // 0x02000 withdrawn
        AV_386_SSE3         = 0x04000,
                           // 0x08000 withdrawn
        AV_386_CX16         = 0x10000,
        AV_386_AHF          = 0x20000,
        AV_386_TSCP         = 0x40000,
        AV_386_AMD_SSE4A    = 0x80000,
        AV_386_POPCNT       = 0x100000,
        AV_386_AMD_LZCNT    = 0x200000,
        AV_386_SSSE3        = 0x400000,
        AV_386_SSE4_1       = 0x800000,
        AV_386_SSE4_2       = 0x1000000,
        AV_386_MOVBE        = 0x2000000,
        AV_386_AES          = 0x40000000,
        AV_386_PCLMULQDQ    = 0x80000000,
        AV_386_XSAVE        = 0x100000000,
        AV_386_AVX          = 0x200000000,
        AV_386_VMX          = 0x400000000,
        AV_386_AMD_SVM      = 0x800000000,
    }
}

// XXX: add sparc?
