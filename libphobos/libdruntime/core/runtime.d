/**
 * The runtime module exposes information specific to the D runtime code.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License:   $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Authors:   Sean Kelly
 * Source:    $(DRUNTIMESRC core/_runtime.d)
 */

/*          Copyright Sean Kelly 2005 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
module core.runtime;

/*
 * Configuration stuff for backtraces
 * Versions:
 *     HaveDLADDR = the extern(C) dladdr function is available
 *     GenericBacktrace = Use GCC unwinding for backtraces
 * 
 * TODO: HaveDLADDR should be set by the configure script
 */

version(Android)
{
    version = HaveDLADDR;
    version = GenericBacktrace;
}
else version(Windows)
{
    version = WindowsBacktrace;
}
else version(linux)
{
    //assume GLIBC backtrace function exists, not always correct!
    version = GlibcBacktrace;
}
else version(OSX)
{
    version = OSXBacktrace;
}
else version(GNU)
{
    version = GenericBacktrace;
}

private
{
    alias bool function() ModuleUnitTester;
    alias bool function(Object) CollectHandler;
    alias Throwable.TraceInfo function( void* ptr ) TraceHandler;

    extern (C) void rt_setCollectHandler( CollectHandler h );
    extern (C) CollectHandler rt_getCollectHandler();

    extern (C) void rt_setTraceHandler( TraceHandler h );
    extern (C) TraceHandler rt_getTraceHandler();

    alias void delegate( Throwable ) ExceptionHandler;
    extern (C) bool rt_init( ExceptionHandler dg = null );
    extern (C) bool rt_term( ExceptionHandler dg = null );

    extern (C) void* rt_loadLibrary( in char[] name );
    extern (C) bool  rt_unloadLibrary( void* ptr );

    extern (C) void* thread_stackBottom();

    extern (C) string[] rt_args();

    version(HaveDLADDR)
    {
        extern(C)
        {
            int dladdr(void *addr, Dl_info *info);
            struct Dl_info
            {
                const (char*) dli_fname;  /* Pathname of shared object that
                                           contains address */
                void*         dli_fbase;  /* Address at which shared object
                                           is loaded */
                const (char*) dli_sname;  /* Name of nearest symbol with address
                                           lower than addr */
                void*         dli_saddr;  /* Exact address of symbol named
                                           in dli_sname */
            }
        }
    }

    version(GenericBacktrace)
    {
        import gcc.unwind;
        import core.demangle;
        import core.stdc.stdio : snprintf, printf;
        import core.stdc.string : strlen;

        version(Posix)
            import core.sys.posix.signal; // segv handler
    }
    else version(GlibcBacktrace)
    {
        import core.demangle;
        import core.stdc.stdlib : free;
        import core.stdc.string : strlen, memchr;
        extern (C) int    backtrace(void**, int);
        extern (C) char** backtrace_symbols(void**, int);
        extern (C) void   backtrace_symbols_fd(void**, int, int);

        version(Posix)
            import core.sys.posix.signal; // segv handler
    }
    else version(OSXBacktrace)
    {
        import core.demangle;
        import core.stdc.stdlib : free;
        import core.stdc.string : strlen;
        extern (C) int    backtrace(void**, int);
        extern (C) char** backtrace_symbols(void**, int);
        extern (C) void   backtrace_symbols_fd(void**, int, int);
        import core.sys.posix.signal; // segv handler
    }
    else version(WindowsBacktrace)
    {
        import core.sys.windows.stacktrace;
    }

    // For runModuleUnitTests error reporting.
    version( Windows )
    {
        import core.sys.windows.windows;
    }
    else version( Posix )
    {
        import core.sys.posix.unistd;
    }
}


static this()
{
    // NOTE: Some module ctors will run before this handler is set, so it's
    //       still possible the app could exit without a stack trace.  If
    //       this becomes an issue, the handler could be set in C main
    //       before the module ctors are run.
    Runtime.traceHandler = &defaultTraceHandler;
}


///////////////////////////////////////////////////////////////////////////////
// Runtime
///////////////////////////////////////////////////////////////////////////////


/**
 * This struct encapsulates all functionality related to the underlying runtime
 * module for the calling context.
 */
struct Runtime
{
    /**
     * Initializes the runtime.  This call is to be used in instances where the
     * standard program initialization process is not executed.  This is most
     * often in shared libraries or in libraries linked to a C program.
     *
     * Params:
     *  dg = A delegate which will receive any exception thrown during the
     *       initialization process or null if such exceptions should be
     *       discarded.
     *
     * Returns:
     *  true if initialization succeeds and false if initialization fails.
     */
    static bool initialize( ExceptionHandler dg = null )
    {
        return rt_init( dg );
    }


    /**
     * Terminates the runtime.  This call is to be used in instances where the
     * standard program termination process will not be not executed.  This is
     * most often in shared libraries or in libraries linked to a C program.
     *
     * Params:
     *  dg = A delegate which will receive any exception thrown during the
     *       termination process or null if such exceptions should be
     *       discarded.
     *
     * Returns:
     *  true if termination succeeds and false if termination fails.
     */
    static bool terminate( ExceptionHandler dg = null )
    {
        return rt_term( dg );
    }


    /**
     * Returns the arguments supplied when the process was started.
     *
     * Returns:
     *  The arguments supplied when this process was started.
     */
    static @property string[] args()
    {
        return rt_args();
    }


    /**
     * Locates a dynamic library with the supplied library name and dynamically
     * loads it into the caller's address space.  If the library contains a D
     * runtime it will be integrated with the current runtime.
     *
     * Params:
     *  name = The name of the dynamic library to load.
     *
     * Returns:
     *  A reference to the library or null on error.
     */
    static void* loadLibrary( in char[] name )
    {
        return rt_loadLibrary( name );
    }


    /**
     * Unloads the dynamic library referenced by p.  If this library contains a
     * D runtime then any necessary finalization or cleanup of that runtime
     * will be performed.
     *
     * Params:
     *  p = A reference to the library to unload.
     */
    static bool unloadLibrary( void* p )
    {
        return rt_unloadLibrary( p );
    }


    /**
     * Overrides the default trace mechanism with s user-supplied version.  A
     * trace represents the context from which an exception was thrown, and the
     * trace handler will be called when this occurs.  The pointer supplied to
     * this routine indicates the base address from which tracing should occur.
     * If the supplied pointer is null then the trace routine should determine
     * an appropriate calling context from which to begin the trace.
     *
     * Params:
     *  h = The new trace handler.  Set to null to use the default handler.
     */
    static @property void traceHandler( TraceHandler h )
    {
        rt_setTraceHandler( h );
    }

    /**
     * Gets the current trace handler.
     *
     * Returns:
     *  The current trace handler or null if no trace handler is set.
     */
    static @property TraceHandler traceHandler()
    {
        return rt_getTraceHandler();
    }

    /**
     * Overrides the default collect hander with a user-supplied version.  This
     * routine will be called for each resource object that is finalized in a
     * non-deterministic manner--typically during a garbage collection cycle.
     * If the supplied routine returns true then the object's dtor will called
     * as normal, but if the routine returns false than the dtor will not be
     * called.  The default behavior is for all object dtors to be called.
     *
     * Params:
     *  h = The new collect handler.  Set to null to use the default handler.
     */
    static @property void collectHandler( CollectHandler h )
    {
        rt_setCollectHandler( h );
    }


    /**
     * Gets the current collect handler.
     *
     * Returns:
     *  The current collect handler or null if no trace handler is set.
     */
    static @property CollectHandler collectHandler()
    {
        return rt_getCollectHandler();
    }


    /**
     * Overrides the default module unit tester with a user-supplied version.
     * This routine will be called once on program initialization.  The return
     * value of this routine indicates to the runtime whether the tests ran
     * without error.
     *
     * Params:
     *  h = The new unit tester.  Set to null to use the default unit tester.
     */
    static @property void moduleUnitTester( ModuleUnitTester h )
    {
        sm_moduleUnitTester = h;
    }


    /**
     * Gets the current module unit tester.
     *
     * Returns:
     *  The current module unit tester handler or null if no trace handler is
     *  set.
     */
    static @property ModuleUnitTester moduleUnitTester()
    {
        return sm_moduleUnitTester;
    }


private:
    // NOTE: This field will only ever be set in a static ctor and should
    //       never occur within any but the main thread, so it is safe to
    //       make it __gshared.
    __gshared ModuleUnitTester sm_moduleUnitTester = null;
}


///////////////////////////////////////////////////////////////////////////////
// Overridable Callbacks
///////////////////////////////////////////////////////////////////////////////


/**
 * This routine is called by the runtime to run module unit tests on startup.
 * The user-supplied unit tester will be called if one has been supplied,
 * otherwise all unit tests will be run in sequence.
 *
 * Returns:
 *  true if execution should continue after testing is complete and false if
 *  not.  Default behavior is to return true.
 */
extern (C) bool runModuleUnitTests()
{
    static if( __traits( compiles, backtrace ) )
    {
        static extern (C) void unittestSegvHandler( int signum, siginfo_t* info, void* ptr )
        {
            static enum MAXFRAMES = 128;
            void*[MAXFRAMES]  callstack;
            int               numframes;

            numframes = backtrace( callstack.ptr, MAXFRAMES );
            backtrace_symbols_fd( callstack.ptr, numframes, 2 );
        }

        sigaction_t action = void;
        sigaction_t oldseg = void;
        sigaction_t oldbus = void;

        (cast(byte*) &action)[0 .. action.sizeof] = 0;
        sigfillset( &action.sa_mask ); // block other signals
        action.sa_flags = SA_SIGINFO | SA_RESETHAND;
        action.sa_sigaction = &unittestSegvHandler;
        sigaction( SIGSEGV, &action, &oldseg );
        sigaction( SIGBUS, &action, &oldbus );
        scope( exit )
        {
            sigaction( SIGSEGV, &oldseg, null );
            sigaction( SIGBUS, &oldbus, null );
        }
    }

    static struct Console
    {
        Console opCall( in char[] val )
        {
            version( Windows )
            {
                DWORD count = void;
                assert(val.length <= uint.max, "val must be less than or equal to uint.max");
                WriteFile( GetStdHandle( 0xfffffff5 ), val.ptr, cast(uint)val.length, &count, null );
            }
            else version( Posix )
            {
                write( 2, val.ptr, val.length );
            }
            return this;
        }
    }

    static __gshared Console console;

    if( Runtime.sm_moduleUnitTester is null )
    {
        size_t failed = 0;
        foreach( m; ModuleInfo )
        {
            if( m )
            {
                auto fp = m.unitTest;

                if( fp )
                {
                    try
                    {
                        fp();
                    }
                    catch( Throwable e )
                    {
                        console( e.toString() )( "\n" );
                        failed++;
                    }
                }
            }
        }
        return failed == 0;
    }
    return Runtime.sm_moduleUnitTester();
}


///////////////////////////////////////////////////////////////////////////////
// Default Implementations
///////////////////////////////////////////////////////////////////////////////


/**
 *
 */
Throwable.TraceInfo defaultTraceHandler( void* ptr = null )
{
    static if( __traits( compiles, backtrace ) ) //GlibcBacktrace || OSXBacktrace
    {
        class DefaultTraceInfo : Throwable.TraceInfo
        {
            this()
            {
                static enum MAXFRAMES = 128;
                void*[MAXFRAMES]  callstack;
              version( GNU )
                numframes = backtrace( callstack.ptr, MAXFRAMES );
              else
                numframes = 0; //backtrace( callstack, MAXFRAMES );
                if (numframes < 2) // backtrace() failed, do it ourselves
                {
                    static void** getBasePtr()
                    {
                        version( D_InlineAsm_X86 )
                            asm { naked; mov EAX, EBP; ret; }
                        else
                        version( D_InlineAsm_X86_64 )
                            asm { naked; mov RAX, RBP; ret; }
                        else
                            return null;
                    }

                    auto  stackTop    = getBasePtr();
                    auto  stackBottom = cast(void**) thread_stackBottom();
                    void* dummy;

                    if( stackTop && &dummy < stackTop && stackTop < stackBottom )
                    {
                        auto stackPtr = stackTop;

                        for( numframes = 0; stackTop <= stackPtr &&
                                            stackPtr < stackBottom &&
                                            numframes < MAXFRAMES; )
                        {
                            callstack[numframes++] = *(stackPtr + 1);
                            stackPtr = cast(void**) *stackPtr;
                        }
                    }
                }
                framelist = backtrace_symbols( callstack.ptr, numframes );
            }

            ~this()
            {
                free( framelist );
            }

            override int opApply( scope int delegate(ref const(char[])) dg ) const
            {
                return opApply( (ref size_t, ref const(char[]) buf)
                                {
                                    return dg( buf );
                                } );
            }

            override int opApply( scope int delegate(ref size_t, ref const(char[])) dg ) const
            {
                version( Posix )
                {
                    // NOTE: The first 5 frames with the current implementation are
                    //       inside core.runtime and the object code, so eliminate
                    //       these for readability.  The alternative would be to
                    //       exclude the first N frames that are in a list of
                    //       mangled function names.
                    static enum FIRSTFRAME = 5;
                }
                else
                {
                    // NOTE: On Windows, the number of frames to exclude is based on
                    //       whether the exception is user or system-generated, so
                    //       it may be necessary to exclude a list of function names
                    //       instead.
                    static enum FIRSTFRAME = 0;
                }
                int ret = 0;

                for( int i = FIRSTFRAME; i < numframes; ++i )
                {
                    char[4096] fixbuf;
                    auto buf = framelist[i][0 .. strlen(framelist[i])];
                    auto pos = cast(size_t)(i - FIRSTFRAME);
                    buf = fixline( buf, fixbuf );
                    ret = dg( pos, buf );
                    if( ret )
                        break;
                }
                return ret;
            }

            override string toString() const
            {
                string buf;
                foreach( i, line; this )
                    buf ~= i ? "\n" ~ line : line;
                return buf;
            }

        private:
            int     numframes;
            char**  framelist;

        private:
            const(char)[] fixline( const(char)[] buf, ref char[4096] fixbuf ) const
            {
                version( OSX )
                {
                    // format is:
                    //  1  module    0x00000000 D6module4funcAFZv + 0
                    for( size_t i = 0, n = 0; i < buf.length; i++ )
                    {
                        if( ' ' == buf[i] )
                        {
                            n++;
                            while( i < buf.length && ' ' == buf[i] )
                                i++;
                            if( 3 > n )
                                continue;
                            auto bsym = i;
                            while( i < buf.length && ' ' != buf[i] )
                                i++;
                            auto esym = i;
                            auto tail = buf.length - esym;
                            fixbuf[0 .. bsym] = buf[0 .. bsym];
                            auto m = demangle( buf[bsym .. esym], fixbuf[bsym .. $] );
                            fixbuf[bsym + m.length .. bsym + m.length + tail] = buf[esym .. $];
                            return fixbuf[0 .. bsym + m.length + tail];
                        }
                    }
                    return buf;
                }
                else version( linux )
                {
                    // format is:  module(_D6module4funcAFZv) [0x00000000]
                    // or:         module(_D6module4funcAFZv+0x78) [0x00000000]
                    auto bptr = cast(char*) memchr( buf.ptr, '(', buf.length );
                    auto eptr = cast(char*) memchr( buf.ptr, ')', buf.length );
                    auto pptr = cast(char*) memchr( buf.ptr, '+', buf.length );

                    if (pptr && pptr < eptr)
                        eptr = pptr;

                    if( bptr++ && eptr )
                    {
                        size_t bsym = bptr - buf.ptr;
                        size_t esym = eptr - buf.ptr;
                        auto tail = buf.length - esym;
                        fixbuf[0 .. bsym] = buf[0 .. bsym];
                        auto m = demangle( buf[bsym .. esym], fixbuf[bsym .. $] );
                        fixbuf[bsym + m.length .. bsym + m.length + tail] = buf[esym .. $];
                        return fixbuf[0 .. bsym + m.length + tail];
                    }
                    return buf;
                }
                else
                {
                    return buf;
                }
            }
        }

        return new DefaultTraceInfo;
    }
    else static if( __traits( compiles, new StackTrace ) ) // WindowsBacktrace
    {
        return new StackTrace;
    }
    else version(GenericBacktrace)
    {
        class DefaultTraceInfo : Throwable.TraceInfo
        {
            this()
            {
                callstack = gdcBacktrace();
                framelist = gdcBacktraceSymbols(callstack);
            }

            override int opApply( scope int delegate(ref char[]) dg )
            {
                return opApply( (ref size_t, ref char[] buf)
                                {
                                    return dg( buf );
                                } );
            }

            override int opApply( scope int delegate(ref size_t, ref char[]) dg )
            {
                version( Posix )
                {
                    // NOTE: The first 5 frames with the current implementation are
                    //       inside core.runtime and the object code, so eliminate
                    //       these for readability.  The alternative would be to
                    //       exclude the first N frames that are in a list of
                    //       mangled function names.
                    static enum FIRSTFRAME = 5;
                }
                else
                {
                    // NOTE: On Windows, the number of frames to exclude is based on
                    //       whether the exception is user or system-generated, so
                    //       it may be necessary to exclude a list of function names
                    //       instead.
                    static enum FIRSTFRAME = 0;
                }
                int ret = 0;

                for( int i = FIRSTFRAME; i < framelist.entries; ++i )
                {
                    auto pos = cast(size_t)(i - FIRSTFRAME);
                    auto buf = formatLine(framelist.symbols[i]);
                    ret = dg( pos, buf );
                    if( ret )
                        break;
                }
                return ret;
            }

            override string toString()
            {
                string buf;
                foreach( i, line; this )
                    buf ~= i ? "\n" ~ line : line;
                return buf;
            }

        private:
            btSymbolData     framelist;
            gdcBacktraceData callstack;

        private:
            char[4096] fixbuf;

            /*Do not put \n at end of line!*/
            char[] formatLine(backtraceSymbol sym)
            {
                int ret;
                
                if(sym.fileName)
                {
                    if(sym.name)
                    {
                        ret = snprintf(fixbuf.ptr, fixbuf.sizeof,
                            "%s(", sym.fileName);
                        if(ret >= fixbuf.sizeof)
                            return fixbuf[];

                        auto demangled = demangle(sym.name[0 .. strlen(sym.name)],
                            fixbuf[ret .. $]);

                        ret += demangled.length;
                        if(ret >= fixbuf.sizeof)
                            return fixbuf[];

                        ret += snprintf(fixbuf.ptr + ret, fixbuf.sizeof - ret,
                            "+%#x) [%p]", sym.offset, sym.address);
                    }
                    else
                    {
                        ret = snprintf(fixbuf.ptr, fixbuf.sizeof,
                            "%s() [%p]", sym.fileName, sym.address);
                    }
                }
                else
                {
                    if(sym.name)
                    {
                        fixbuf[0] = '(';
                        ret = 1;

                        auto demangled = demangle(sym.name[0 .. strlen(sym.name)],
                            fixbuf[ret .. $]);

                        ret += demangled.length;
                        if(ret >= fixbuf.sizeof)
                            return fixbuf[];

                        ret += snprintf(fixbuf.ptr + ret, fixbuf.sizeof - ret,
                            "+%#x) [%p]", sym.offset, sym.address);
                    }
                    else
                    {
                        ret = snprintf(fixbuf.ptr, fixbuf.sizeof, "() [%p]",
                            sym.address);
                    }
                }

                if(ret >= fixbuf.sizeof)
                    return fixbuf[];
                else
                    return fixbuf[0 .. ret];
            }
        }

        return new DefaultTraceInfo;
    }
    else
    {
        return null;
    }
}

version(GenericBacktrace)
{
    static enum MAXFRAMES = 128;

    struct gdcBacktraceData
    {
        void*[MAXFRAMES] callstack;
        int numframes = 0;
    }

    struct backtraceSymbol
    {
        const(char)* name, fileName;
        size_t offset;
        void* address;
    }

    struct btSymbolData
    {
        size_t entries;
        backtraceSymbol[MAXFRAMES] symbols;
    }
    
    static extern (C) _Unwind_Reason_Code unwindCB(_Unwind_Context *ctx, void *d)
    {
        gdcBacktraceData* bt = cast(gdcBacktraceData*)d;
        if(bt.numframes >= MAXFRAMES)
            return _URC_NO_REASON;

        bt.callstack[bt.numframes] = cast(void*)_Unwind_GetIP(ctx);
        bt.numframes++;
        return _URC_NO_REASON;
    }

    gdcBacktraceData gdcBacktrace()
    {
        gdcBacktraceData stackframe;
        _Unwind_Backtrace(&unwindCB, &stackframe);
        return stackframe;
    }

    btSymbolData gdcBacktraceSymbols(gdcBacktraceData data)
    {
        btSymbolData symData;

        for(auto i = 0; i < data.numframes; i++)
        {
            version(HaveDLADDR)
            {
                Dl_info funcInfo;

                if(data.callstack[i] !is null && dladdr(data.callstack[i], &funcInfo) != 0)
                {
                    symData.symbols[symData.entries].name = funcInfo.dli_sname;
                    symData.symbols[symData.entries].fileName = funcInfo.dli_fname;

                    if(funcInfo.dli_saddr is null)
                        symData.symbols[symData.entries].offset = 0;
                    else
                        symData.symbols[symData.entries].offset = data.callstack[i] - funcInfo.dli_saddr;

                    symData.symbols[symData.entries].address = data.callstack[i];
                    symData.entries++;
                }
                else
                {
                    symData.symbols[symData.entries].address = data.callstack[i];
                    symData.entries++;
                }
            }
            else
            {
                symData.symbols[symData.entries].address = data.callstack[i];
                symData.entries++;
            }
        }

        return symData;
    }
}
