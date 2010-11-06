// D import file generated from 'src\core\runtime.d'
module core.runtime;
private 
{
    extern (C) 
{
    bool rt_isHalting();
}
    alias bool function() ModuleUnitTester;
    alias bool function(Object) CollectHandler;
    alias Throwable.TraceInfo function(void* ptr = null) TraceHandler;
    extern (C) 
{
    void rt_setCollectHandler(CollectHandler h);
}
    extern (C) 
{
    void rt_setTraceHandler(TraceHandler h);
}
    alias void delegate(Throwable) ExceptionHandler;
    extern (C) 
{
    bool rt_init(ExceptionHandler dg = null);
}
    extern (C) 
{
    bool rt_term(ExceptionHandler dg = null);
}
    extern (C) 
{
    void* rt_loadLibrary(in char[] name);
}
    extern (C) 
{
    bool rt_unloadLibrary(void* ptr);
}
    version (linux)
{
    import core.stdc.stdlib;
    import core.stdc.string;
    extern (C) 
{
    int backtrace(void**, size_t);
}
    extern (C) 
{
    char** backtrace_symbols(void**, int);
}
    extern (C) 
{
    void backtrace_symbols_fd(void**, int, int);
}
    import core.sys.posix.signal;
}
else
{
    version (OSX)
{
    import core.stdc.stdlib;
    import core.stdc.string;
    extern (C) 
{
    int backtrace(void**, size_t);
}
    extern (C) 
{
    char** backtrace_symbols(void**, int);
}
    extern (C) 
{
    void backtrace_symbols_fd(void**, int, int);
}
    import core.sys.posix.signal;
}
}
}
struct Runtime
{
    static 
{
    bool initialize(ExceptionHandler dg = null)
{
return rt_init(dg);
}
}
    static 
{
    bool terminate(ExceptionHandler dg = null)
{
return rt_term(dg);
}
}
    static 
{
    bool isHalting()
{
return rt_isHalting();
}
}
    static 
{
    void* loadLibrary(in char[] name)
{
return rt_loadLibrary(name);
}
}
    static 
{
    bool unloadLibrary(void* p)
{
return rt_unloadLibrary(p);
}
}
    static 
{
    void traceHandler(TraceHandler h)
{
rt_setTraceHandler(h);
}
}
    static 
{
    void collectHandler(CollectHandler h)
{
rt_setCollectHandler(h);
}
}
    static 
{
    void moduleUnitTester(ModuleUnitTester h)
{
sm_moduleUnitTester = h;
}
}
    private 
{
    __gshared 
{
    ModuleUnitTester sm_moduleUnitTester = null;
}
}
}
extern (C) 
{
    bool runModuleUnitTests();
}
Throwable.TraceInfo defaultTraceHandler(void* ptr = null);
