module gcc.gccextern;

version(GC_Use_Stack_GLibC)
    extern (C) __gshared void * __libc_stack_end;

version(GC_Use_Data_Fixed)
{
    extern (C)
    {
        extern __gshared
        {
            int _data;
            int __data_start;
            int _end;
            int _data_start__;
            int _data_end__;
            int _bss_start__;
            int _bss_end__;
            int __fini_array_end;
        }
    }

    /* %% Move all this to configure script to test if it actually works?
       --enable-gc-data-fixed=Mode,s1,e1,s2,e2
       .. the Mode can be a version instead of enum trick
    */
    
    version (aix)
    {
        alias _data Data_Start;
        alias _end Data_End;
        enum FM { One = 1, MinMax = 0, Two = 0 }
    }
    else version (cygwin)
    {
        alias _data_start__ Data_Start;
        alias _data_end__ Data_End;
        alias _bss_start__ Data_Start_2;
        alias _bss_end__ Data_End_2;
        enum FM { MinMax = 1, One = 0, Two = 0 }
    }
    else version (FreeBSD)
    {
        // use '_etext' if '__fini_array_end' doesn't work
        /* There is a bunch of read-only data after .data and before .bss, but
           no linker symbols to find it.  Would have to set up a fault handler
           and scan... */
        alias __fini_array_end Data_Start;
        alias _end Data_End;
        enum FM { One = 1, MinMax = 0, Two = 0 }
    }
    else version (linux)
    {
        alias __data_start Data_Start;
        alias _end Data_End;
        /* possible better way:
           [__data_start,_DYNAMIC) and [_edata/edata or __bss_start,_end/end)
           This doesn't really save much.. a better linker script is needed.
        */
        enum FM { One = 1, MinMax = 0, Two = 0 }
    }
    else version (skyos)
    {
        alias _data_start__ Data_Start;
        alias _bss_end__ Data_End;
        enum FM { One = 1, MinMax = 0, Two = 0 }
    }
}

enum DataSegmentTracking {
    ExecutableOnly,
    LoadTimeLibrariesOnly,
    Dynamic
}
