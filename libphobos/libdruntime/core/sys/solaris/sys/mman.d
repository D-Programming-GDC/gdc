module core.sys.solaris.sys.mman;

import core.sys.posix.sys.types : caddr_t;

version(Solaris):
extern (C):
nothrow:

struct mmapobj_result_t {
    caddr_t     mr_addr;
    size_t      mr_msize;
    size_t      mr_fsize;
    size_t      mr_offset;
    size_t      mr_prot;
    size_t      mr_flags;
}
