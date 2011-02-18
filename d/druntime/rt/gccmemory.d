/**
 * This module exposes functionality for inspecting and manipulating memory.
 *
 * Scan data areas in /proc/maps. This cannot be done in rt.memory because
 * requires GDC compiler-specific implementation of D module references.
 *
 * Written by Iain Buclaw, February 2011.
 */

module rt.gccmemory;

private
{
    import core.stdc.stdlib;        // alloca, strtoul
    import core.stdc.string;        // memmove, strchr
    import core.sys.posix.fcntl;    // open
    import core.sys.posix.unistd;   // close, read

    extern (C) void gc_addRange( void* p, size_t sz );

    // Marked areas in the maps file.
    static if ( size_t.sizeof == 4 )
    {
        enum Ofs
        {
            Write_Prot = 19,
            Start_Addr = 0,
            End_Addr = 9,
            Addr_Len = 8,
            Inode_Num = 38,
        }
    }
    else static if ( size_t.sizeof == 8 )
    {
        enum Ofs
        {
            Write_Prot = 35,
            Start_Addr = 0,
            End_Addr = 9,
            Addr_Len = 17,
            Inode_Num = 70,
        }
    }
    else
        static assert( false, "Architecture not supported." );

    // This linked list is created by a compiler generated function inserted
    // into the .ctor list by the compiler.
    struct ModRef 
    {
        ModRef* next;
        void*   mod;
    }

    extern (C) extern __gshared ModRef* _Dmodule_ref;
}

void scanDataProcMaps(void* Data_Start, void* Data_End)
{
    int fd;         // file descriptor
    char * buffer;  // file buffer allocated on stack
    uint len = 256; // buffer length
    int count;      // file read count
    char * ptr;     // current location on buffer
    char * eptr;    // end location on buffer

    fd = open( "/proc/self/maps", O_RDONLY );
    if ( fd == -1 )
        return;
    scope( exit ) close( fd );

    buffer = cast(char*) alloca( len );
    if ( buffer == null )
        return;

    ptr = buffer;
    while (( count = read( fd, ptr, len - ( ptr - buffer ))) > 0 )
    {
        eptr = ptr + count;
        ptr = buffer;

        while( true )
        {
            auto nptr = strchr( ptr, '\n' );
            if ( nptr && nptr < eptr )
            {
                // parse the entry in ptr[]
                if ( ptr[Ofs.Write_Prot] == 'w' )
                {
                    ptr[Ofs.Start_Addr + Ofs.Addr_Len] = '\0';
                    ptr[Ofs.End_Addr + Ofs.Addr_Len] = '\0';
                    auto start = cast(void*) strtoul( ptr + Ofs.Start_Addr, null, 16 );
                    auto end   = cast(void*) strtoul( ptr + Ofs.End_Addr, null, 16 );

                    // 1. Exclude anything overlapping [Data_Start,Data_End]
                    // 2. Exclude stack
                    if (( ! Data_End ||
                                ! ( Data_Start >= start && Data_End <= end )) &&
                            ! ( buffer >= start && buffer < end ))
                    {
                        int addrange = 0;

                        // 3. Include heap
                        // 4. Include D modules
                        if ( ptr[Ofs.Inode_Num] == '0' )
                            addrange = 1;
                        else
                        {
                            ModRef* mr;
                            for ( mr = _Dmodule_ref; mr; mr = mr.next )
                            {
                                if ( mr >= start && mr <= end )
                                {
                                    addrange = 1;
                                    break;
                                }
                            }
                        }

                        if ( addrange )
                        {
                            debug ( ProcMaps )
                                printf( "Adding map range %p 0%p\n", start, end );
                            gc_addRange( start, cast(size_t) end - cast(size_t) start );
                        }
                    }
                }
                ptr = nptr + 1;
            }
            else
            {
                count = eptr - ptr;
                memmove( buffer, ptr, count );
                ptr = buffer + count;
                break;
            }
        }
    }
}
