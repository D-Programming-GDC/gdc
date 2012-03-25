private import gcgccextern;
private import std.gc;
private import std.c.stdlib;
private import std.string; // for memmove

debug(ProcMaps)
    private import std.c.stdio;

/* ------- Memory allocation ------------- */

version (GC_Use_Alloc_MMap)
{    
    private import std.c.unix.unix;

    void *os_mem_map(size_t nbytes)
    {   void *p;
        p = mmap(null, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        return (p == MAP_FAILED) ? null : p;
    }
    int os_mem_commit(void *base, size_t offset, size_t nbytes)
    {
        return 0;
    }

    int os_mem_decommit(void *base, size_t offset, size_t nbytes)
    {
        return 0;
    }

    int os_mem_unmap(void *base, size_t nbytes)
    {
        return munmap(base, nbytes);
    }
}
else version (GC_Use_Alloc_Valloc)
{
    extern (C) void * valloc(size_t);
    void *os_mem_map(size_t nbytes) { return valloc(nbytes); }
    int os_mem_commit(void *base, size_t offset, size_t nbytes) { return 0; }
    int os_mem_decommit(void *base, size_t offset, size_t nbytes) { return 0; }
    int os_mem_unmap(void *base, size_t nbytes) { free(base); return 0; }
}
else version (GC_Use_Alloc_Malloc)
{
    /* Assumes malloc granularity is at least (void *).sizeof.  If
       (req_size + PAGESIZE) is allocated, and the pointer is rounded
       up to PAGESIZE alignment, there will be space for a void* at the
       end after PAGESIZE bytes used by the GC. */
    
    private import gcx; // for PAGESIZE
    
    const uint PAGE_MASK = PAGESIZE - 1;

    void *os_mem_map(size_t nbytes)
    {   byte * p, q;
        p = cast(byte *) std.c.stdlib.malloc(nbytes + PAGESIZE);
        q = p + ((PAGESIZE - ((cast(size_t) p & PAGE_MASK))) & PAGE_MASK);
        * cast(void**)(q + nbytes) = p;
        return q;
    }
    int os_mem_commit(void *base, size_t offset, size_t nbytes)
    {
        return 0;
    }

    int os_mem_decommit(void *base, size_t offset, size_t nbytes)
    {
        return 0;
    }

    int os_mem_unmap(void *base, size_t nbytes)
    {
        std.c.stdlib.free( * cast(void**)( cast(byte*) base + nbytes ) );
        return 0;
    }
}
else version (GC_Use_Alloc_Fixed_Heap)
{
    // TODO
    static assert(0);
}
else
{
    static assert(0);
}


/* ------- Stack origin ------------- */

version (GC_Use_Stack_Guess)
    private import gc_guess_stack;

version (GC_Use_Stack_FreeBSD)
    extern (C) int _d_gcc_gc_freebsd_stack(void **);

void *os_query_stackBottom()
{
    version (GC_Use_Stack_GLibC)
    {
        return __libc_stack_end;
    }
    else version (GC_Use_Stack_Guess)
    {
        // dmainwhatever should be private too
        // import main?
        return stackOriginGuess;
    }
    else version (GC_Use_Stack_FreeBSD)
    {
        void * stack_origin;
        if (_d_gcc_gc_freebsd_stack(& stack_origin))
            return stack_origin;
        else
            // No way to signal an error
            return null;
    }
    else version (GC_Use_Stack_Scan)
    {
        static assert(0);
    }
    else version (GC_Use_Stack_Fixed)
    {
        version (darwin)
        {
            static if (size_t.sizeof == 4)
                return cast(void*) 0xc0000000;
            else static if (size_t.sizeof == 8)
                return cast(void*) 0x7ffff_00000000UL;
            else
                static assert(0);
        }
        else
            static assert(0);
    }
    else
    {
        static assert(0);
    }
}

// std.thread needs to know the stack origin 
extern (C) void* _d_gcc_query_stack_origin()
{
    return os_query_stackBottom();
}
  

/* ------- Data segments ------------- */

version (GC_Use_Data_Dyld)
    extern (C) void _d_gcc_dyld_start(DataSegmentTracking mode);

version (GC_Use_Data_Proc_Maps)
{
    private import std.c.unix.unix;
    private import std.c.stdlib;
}


/*
  It is assumed that this is called during GC initialization and
  only once.
*/

void os_query_staticdataseg(void **base, size_t *nbytes)
{
    const size_t S = (void *).sizeof;

    // Can't assume the input addresses are word-aligned
    static void * adjust_up(void * p)
    {
        return p + ((S - (cast(size_t)p & (S-1))) & (S-1)); // cast ok even if 64-bit
    }
    static void * adjust_down(void * p)
    {
        return p - (cast(size_t) p & (S-1));
    }
    
    void * main_data_start;
    void * main_data_end;

    *base = null;
    *nbytes = 0;
    
    version (GC_Use_Data_Dyld)
    {
        _d_gcc_dyld_start(DataSegmentTracking.Dynamic);
        return; // no need for any other method
    }

    version (GC_Use_Data_Fixed)
    {
        static if (FM.One) {
            main_data_start = adjust_up  ( & Data_Start );
            main_data_end   = adjust_down( & Data_End );
            *base = main_data_start;
            *nbytes = main_data_end - main_data_start;
        } else static if (FM.Two) {
            main_data_start = adjust_up  ( & Data_Start );
            main_data_end   = adjust_down( & Data_End );
            *base = main_data_start;
            *nbytes = main_data_end - main_data_start;
            addRange(adjust_up( & Data_Start_2 ), adjust_down( & Data_End_2 ));
        } else static if (FM.MinMax) {
            static void * min(void *a, void *b) { return a < b ? a : b; }
            static void * max(void *a, void *b) { return a > b ? a : b; }
            main_data_start = adjust_up  ( & Data_Start < & Data_Start_2 ? & Data_Start : & Data_Start_2 );
            main_data_end   = adjust_down( & Data_End > & Data_End_2 ? & Data_End : & Data_End_2 );
            *base = main_data_start;
            *nbytes = main_data_end - main_data_start;
        }
        //goto have_main_data;
    }

    //have_main_data:

    version (GC_Use_Data_Proc_Maps)
    {
        // TODO: Exclude zero-mapped regions...

        int fd = open("/proc/self/maps", O_RDONLY);
        int count; // %% need to configure ret for read..
        char buf[2024];
        char * p;
        char * e;
        char * s;
        void * start;
        void * end;

        p = buf;
        if (fd != -1) {
            while ( (count = read(fd, p, buf.sizeof - (p - buf.ptr))) > 0 ) {
                e = p + count;
                p = buf;
                while (1) {
                    s = p;
                    while (p < e && *p != '\n')
                        p++;
                    if (p < e) {
                        // parse the entry in [s, p)
                        static if (S == 4) {
                            enum Ofs {
                                Write_Prot = 19,
                                Start_Addr = 0,
                                End_Addr = 9,
                                Addr_Len = 8,
                            }
                        } else static if (S == 8) {
                            enum Ofs {
                                Write_Prot = 35,
                                Start_Addr = 0,
                                End_Addr = 9,
                                Addr_Len = 17,
                            }
                        } else {
                            static assert(0);
                        }

                        // %% this is wrong for 64-bit:
                        // uint   strtoul(char *,char **,int);

                        if (s[Ofs.Write_Prot] == 'w') {
                            s[Ofs.Start_Addr + Ofs.Addr_Len] = '\0';
                            s[Ofs.End_Addr + Ofs.Addr_Len] = '\0';
                            start = cast(void *) strtoul(s + Ofs.Start_Addr, null, 16);
                            end   = cast(void *) strtoul(s + Ofs.End_Addr, null, 16);

                            // 1. Exclude anything overlapping [main_data_start,main_data_end)
                            // 2. Exclude stack
                            if ( (! main_data_end ||
                                  ! (main_data_start >= start && main_data_end <= end)) &&
                                 ! (& buf >= start && & buf < end)) {
                                // we already have static data from this region.  anything else
                                // is heap (%% check)
                                debug (ProcMaps)
                                    printf("Adding map range %p 0%p\n", start, end);
                                addRange(start, end);
                            }
                        }

                        p++;
                    } else {
                        count = p - s;
                        memmove(buf, s, count);
                        p = buf.ptr + count;
                        break;
                    }
                }
            }
            close(fd);
        }
    }
}
