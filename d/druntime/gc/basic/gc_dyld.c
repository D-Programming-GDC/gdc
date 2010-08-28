// Could config test HAVE_PRIVATE_EXTERN, but this should be okay
#ifndef __private_extern__
#define __private_extern__ extern
#include <mach-o/dyld.h>
#undef __private_extern__
#else
#include <mach-o/dyld.h>
#endif

#include <mach-o/getsect.h>
#include <stdlib.h>

#include "gc_c.h"

const static struct { 
        const char *seg;
        const char *sect;
} GC_dyld_sections[] = {
        { SEG_DATA, SECT_DATA },
        { SEG_DATA, SECT_BSS },
        { SEG_DATA, SECT_COMMON }
};

#ifdef __ppc__
#define THIS_IS_64_BIT 0
#elif defined(__i386__)
#define THIS_IS_64_BIT 0
#elif defined(__ppc64__)
#define THIS_IS_64_BIT 1
#elif defined(__x86_64__)
#define THIS_IS_64_BIT 1
#else
#error This architecture is not supported
#endif

/* This should never be called by a thread holding the lock */
static void
on_dyld_add_image(const struct mach_header* hdr, intptr_t slide) {
    unsigned i;
    uintptr_t start, end;
#if THIS_IS_64_BIT
    const struct section_64 *sec;
#else
    const struct section *sec;
#endif

    for (i = 0;
         i < sizeof(GC_dyld_sections) / sizeof(GC_dyld_sections[0]);
         i++) {
        
#if THIS_IS_64_BIT
        sec = getsectbynamefromheader_64((const struct mach_header_64*) hdr,
            GC_dyld_sections[i].seg, GC_dyld_sections[i].sect);
#else
        sec = getsectbynamefromheader(hdr, GC_dyld_sections[i].seg,
            GC_dyld_sections[i].sect);
#endif
        if (sec == NULL || sec->size == 0)
            continue;
        start = slide + sec->addr;
        end = start + sec->size;
            
        GC_add_range((void*) start, (void*) end);
    }
}
    
/* This should never be called by a thread holding the lock */
static void
on_dyld_remove_image(const struct mach_header* hdr, intptr_t slide) {
    unsigned i;
    uintptr_t start, end;
#if THIS_IS_64_BIT
    const struct section_64 *sec;
#else
    const struct section *sec;
#endif

    for(i = 0;
        i < sizeof(GC_dyld_sections) / sizeof(GC_dyld_sections[0]);
        i++) {

#if THIS_IS_64_BIT
        sec = getsectbynamefromheader_64((const struct mach_header_64*) hdr,
            GC_dyld_sections[i].seg, GC_dyld_sections[i].sect);
#else
        sec = getsectbynamefromheader(hdr, GC_dyld_sections[i].seg,
            GC_dyld_sections[i].sect);
#endif
        if (sec == NULL || sec->size == 0)
            continue;
        start = slide + sec->addr;
        end = start + sec->size;

        GC_remove_range((void*) start);//, (void*) end);
    }
}

void _d_gcc_dyld_start(enum DataSegmentTracking mode)
{
    static int started = 0;

    if (! started) {
        started = 1;
        _dyld_register_func_for_add_image(on_dyld_add_image);
        _dyld_register_func_for_remove_image(on_dyld_remove_image);
    }

    // (for LoadTimeLibrariesOnly:) Can't unregister callbacks 
}
