// D import file generated from 'src\core\memory.d'
module core.memory;
private 
{
    extern (C) 
{
    void gc_init();
}
    extern (C) 
{
    void gc_term();
}
    extern (C) 
{
    void gc_enable();
}
    extern (C) 
{
    void gc_disable();
}
    extern (C) 
{
    void gc_collect();
}
    extern (C) 
{
    void gc_minimize();
}
    extern (C) 
{
    uint gc_getAttr(in void* p);
}
    extern (C) 
{
    uint gc_setAttr(in void* p, uint a);
}
    extern (C) 
{
    uint gc_clrAttr(in void* p, uint a);
}
    extern (C) 
{
    void* gc_malloc(size_t sz, uint ba = 0);
}
    extern (C) 
{
    void* gc_calloc(size_t sz, uint ba = 0);
}
    extern (C) 
{
    BlkInfo_ gc_qalloc(size_t sz, uint ba = 0);
}
    extern (C) 
{
    void* gc_realloc(void* p, size_t sz, uint ba = 0);
}
    extern (C) 
{
    size_t gc_extend(void* p, size_t mx, size_t sz);
}
    extern (C) 
{
    size_t gc_reserve(size_t sz);
}
    extern (C) 
{
    void gc_free(void* p);
}
    extern (C) 
{
    void* gc_addrOf(in void* p);
}
    extern (C) 
{
    size_t gc_sizeOf(in void* p);
}
    struct BlkInfo_
{
    void* base;
    size_t size;
    uint attr;
}
    extern (C) 
{
    BlkInfo_ gc_query(in void* p);
}
    extern (C) 
{
    void gc_addRoot(in void* p);
}
    extern (C) 
{
    void gc_addRange(in void* p, size_t sz);
}
    extern (C) 
{
    void gc_removeRoot(in void* p);
}
    extern (C) 
{
    void gc_removeRange(in void* p);
}
}
struct GC
{
    static 
{
    void enable()
{
gc_enable();
}
}
    static 
{
    void disable()
{
gc_disable();
}
}
    static 
{
    void collect()
{
gc_collect();
}
}
    static 
{
    void minimize()
{
gc_minimize();
}
}
    enum BlkAttr : uint
{
FINALIZE = 1,
NO_SCAN = 2,
NO_MOVE = 4,
}
    alias BlkInfo_ BlkInfo;
    static 
{
    uint getAttr(in void* p)
{
return gc_getAttr(p);
}
}
    static 
{
    uint setAttr(in void* p, uint a)
{
return gc_setAttr(p,a);
}
}
    static 
{
    uint clrAttr(in void* p, uint a)
{
return gc_clrAttr(p,a);
}
}
    static 
{
    void* malloc(size_t sz, uint ba = 0)
{
return gc_malloc(sz,ba);
}
}
    static 
{
    BlkInfo qalloc(size_t sz, uint ba = 0)
{
return gc_qalloc(sz,ba);
}
}
    static 
{
    void* calloc(size_t sz, uint ba = 0)
{
return gc_calloc(sz,ba);
}
}
    static 
{
    void* realloc(void* p, size_t sz, uint ba = 0)
{
return gc_realloc(p,sz,ba);
}
}
    static 
{
    size_t extend(void* p, size_t mx, size_t sz)
{
return gc_extend(p,mx,sz);
}
}
    static 
{
    size_t reserve(size_t sz)
{
return gc_reserve(sz);
}
}
    static 
{
    void free(void* p)
{
gc_free(p);
}
}
    static 
{
    void* addrOf(in void* p)
{
return gc_addrOf(p);
}
}
    static 
{
    size_t sizeOf(in void* p)
{
return gc_sizeOf(p);
}
}
    static 
{
    BlkInfo query(in void* p)
{
return gc_query(p);
}
}
    static 
{
    void addRoot(in void* p)
{
gc_addRoot(p);
}
}
    static 
{
    void addRange(in void* p, size_t sz)
{
gc_addRange(p,sz);
}
}
    static 
{
    void removeRoot(in void* p)
{
gc_removeRoot(p);
}
}
    static 
{
    void removeRange(in void* p)
{
gc_removeRange(p);
}
}
}
