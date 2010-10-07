// D import file generated from 'core\runtime.d'
module core.runtime;
private
{
    extern (C) 
{
    bool rt_isHalting();
}
    alias bool function() ModuleUnitTester;
    alias bool function(Object) CollectHandler;
    alias Exception.TraceInfo function(void* ptr = null) TraceHandler;
    extern (C) 
{
    void rt_setCollectHandler(CollectHandler h);
}
    extern (C) 
{
    void rt_setTraceHandler(TraceHandler h);
}
    alias void delegate(Exception) ExceptionHandler;
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
}
struct Runtime
{
    static 
{
    bool initialize(void delegate(Exception) dg = null)
{
return rt_init(dg);
}
}
    static 
{
    bool terminate(void delegate(Exception) dg = null)
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
    static 
{
    ModuleUnitTester sm_moduleUnitTester = null;
}
}
}
extern (C) 
{
    bool runModuleUnitTests();
}
