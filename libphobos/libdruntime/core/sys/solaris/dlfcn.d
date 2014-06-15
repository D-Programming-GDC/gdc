module core.sys.solaris.dlfcn;

version(Solaris):
extern(C):
nothrow:

import core.sys.posix.dlfcn;
import core.stdc.config;
import core.sys.solaris.sys.auxv;
import core.sys.solaris.sys.mman;

// struct Dl_info in core.sys.posix.dlfcn
alias Dl_info Dl_info_t;

// Values for dlsym()
enum RTLD_NEXT    = cast(void*) -1;
enum RTLD_DEFAULT = cast(void*) -2;
enum RTLD_SELF    = cast(void*) -3;
enum RTLD_PROBE   = cast(void*) -4;

// Additional flags to dlopen
enum RTLD_PARENT   = 0x00200;
enum RTLD_GROUP    = 0x00400;
enum RTLD_WORLD    = 0x00800;
enum RTLD_NODELETE = 0x01000;
enum RTLD_FIRST    = 0x02000;
enum RTLD_CONFGEN  = 0x10000;

struct Dl_serpath_t
{
    char*   dls_name;
    uint    dls_flags;
}

struct Dl_serinfo_t
{
    size_t              dls_size;
    uint                dls_cnt;
    Dl_serpath_t[1]     dls_serpath;
}

struct Dl_argsinfo_t
{
    c_long  dla_argc;
    char**  dla_argv;
    char**  dla_envp;
    auxv_t* dla_auxv;
}

struct Dl_mapinfo_t
{
    mmapobj_result_t*   dlm_maps;
    uint                dlm_acnt;
    uint                dlm_rcnt;
}

// Flags for Dl_amd64_unwindinfo.dlfi_flags
enum DLUI_FLG_NOUNWIND = 0x0001;
enum DLUI_FLG_NOOBJ    = 0x0002;

struct Dl_amd64_unwindinfo_t
{
    uint        dlui_version;
    uint        dlui_flags;
    char*       dlui_objname;
    void*       dlui_unwindstart;
    void*       dlui_unwindend;
    void*       dlui_segstart;
    void*       dlui_segend;
}

struct Dl_definfo_t
{
    const(char)*    dld_refname;
    const(char)*    dld_depname;
}

alias c_ulong Lmid_t;
void*   dlmopen(Lmid_t, in char*, int);

// Values for dladdr1() flags
enum RTLD_DL_SYMENT  = 1;
enum RTLD_DL_LINKMAP = 2;
enum RTLD_DL_MASK    = 0xffff;
int     dladdr1(void* address, Dl_info_t* dlip, void** info, int flags);

// Values for dldump flags
enum RTLD_REL_RELATIVE = 0x00001;
enum RTLD_REL_EXEC     = 0x00002;
enum RTLD_REL_DEPENDS  = 0x00004;
enum RTLD_REL_PRELOAD  = 0x00008;
enum RTLD_REL_SELF     = 0x00010;
enum RTLD_REL_WEAK     = 0x00020;
enum RTLD_REL_ALL      = 0x00fff;
enum RTLD_REL_MEMORY   = 0x01000;
enum RTLD_STRIP        = 0x02000;
enum RTLD_NOHEAP       = 0x04000;
enum RTLD_CONFSET      = 0x10000;
int     dldump(in char*, in char*, int);

// Argumens for dlinfo()
enum {
    RTLD_DI_LMID = 1,
    RTLD_DI_LINKMAP,
    RTLD_DI_CONFIGADDR,
    RTLD_DI_SERINFO,
    RTLD_DI_SERINFOSIZE,
    RTLD_DI_ORIGIN,
    RTLD_DI_PROFILENAME,
    RTLD_DI_PROFILEOUT,
    RTLD_DI_GETSIGNAL,
    RTLD_DI_ARGSINFO,
    RTLD_DI_MMAPS,
    RTLD_DI_MMAPCNT,
    RTLD_DI_DEFERRED,
    RTLD_DI_DEFERRED_SYM
}
enum RTLD_DI_MAX = RTLD_DI_DEFERRED_SYM;

int     dlinfo(void*, int, void*);
