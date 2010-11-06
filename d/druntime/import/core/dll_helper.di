// D import file generated from 'src\core\dll_helper.d'
module core.dll_helper;
version (Windows)
{
    import std.c.windows.windows;
    import core.stdc.string;
    import core.runtime;
    public 
{
    import core.thread_helper;
}
    extern (C) 
{
    extern __gshared 
{
    void* _tlsstart;
}
    extern __gshared 
{
    void* _tlsend;
}
    extern __gshared 
{
    int _tls_index;
}
    extern __gshared 
{
    void* _tls_callbacks_a;
}
}
    private 
{
    struct dll_helper_aux
{
    struct LdrpTlsListEntry
{
    LdrpTlsListEntry* next;
    LdrpTlsListEntry* prev;
    void* tlsstart;
    void* tlsend;
    void* ptr_tlsindex;
    void* callbacks;
    void* zerofill;
    int tlsindex;
}
    extern (Windows) 
{
    alias void* fnRtlAllocateHeap(void* HeapHandle, uint Flags, uint Size);
}
    static 
{
    void* findCodeReference(void* adr, int len, ref ubyte[] pattern, bool relative);
}
    static __gshared 
{
    int* pNtdllBaseTag;
}
    static __gshared 
{
    ubyte[] jmp_LdrpInitialize = [51,237,233];
}
    static __gshared 
{
    ubyte[] jmp__LdrpInitialize = [93,233];
}
    static __gshared 
{
    ubyte[] call_LdrpInitializeThread = [255,117,8,232];
}
    static __gshared 
{
    ubyte[] call_LdrpAllocateTls = [0,0,232];
}
    static __gshared 
{
    ubyte[] jne_LdrpAllocateTls = [15,133];
}
    static __gshared 
{
    ubyte[] mov_LdrpNumberOfTlsEntries = [139,13];
}
    static __gshared 
{
    ubyte[] mov_NtdllBaseTag = [81,139,13];
}
    static __gshared 
{
    ubyte[] mov_LdrpTlsList = [139,61];
}
    static 
{
    LdrpTlsListEntry* addTlsListEntry(void** peb, void* tlsstart, void* tlsend, void* tls_callbacks_a, int* tlsindex);
}
    static 
{
    bool addTlsData(void** teb, void* tlsstart, void* tlsend, int tlsindex);
}
    alias bool BOOLEAN;
    struct UNICODE_STRING
{
    short Length;
    short MaximumLength;
    wchar* Buffer;
}
    struct LIST_ENTRY
{
    LIST_ENTRY* next;
    LIST_ENTRY* prev;
}
    struct LDR_MODULE
{
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID BaseAddress;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    SHORT LoadCount;
    SHORT TlsIndex;
    LIST_ENTRY HashTableEntry;
    ULONG TimeDateStamp;
}
    struct PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
}
    static 
{
    LDR_MODULE* findLdrModule(HINSTANCE hInstance, void** peb);
}
    static 
{
    bool setDllTlsUsage(HINSTANCE hInstance, void** peb);
}
}
    public 
{
    bool dll_fixTLS(HINSTANCE hInstance, void* tlsstart, void* tlsend, void* tls_callbacks_a, int* tlsindex);
    bool dll_process_attach(HINSTANCE hInstance, bool attach_threads, void* tlsstart, void* tlsend, void* tls_callbacks_a, int* tlsindex);
    bool dll_process_attach(HINSTANCE hInstance, bool attach_threads = true)
{
return dll_process_attach(hInstance,attach_threads,&_tlsstart,&_tlsend,&_tls_callbacks_a,&_tls_index);
}
    void dll_process_detach(HINSTANCE hInstance, bool detach_threads = true);
    void dll_thread_attach(bool attach_thread = true, bool initTls = true)
{
if (attach_thread && !Thread.findThread(GetCurrentThreadId()))
thread_attachThis();
if (initTls && !_moduleinfo_tlsdtors_i)
_moduleTlsCtor();
}
    void dll_thread_detach(bool detach_thread = true, bool exitTls = true)
{
if (exitTls)
_moduleTlsDtor();
if (detach_thread)
thread_detachThis();
}
}
}
}
