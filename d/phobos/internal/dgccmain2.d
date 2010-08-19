
/* NOTE: This file is based on dmain2.d from the original DMD distribution.

*/

import object;
import std.c.stdio;
import std.c.stdlib;
import std.c.string;
version (GNU)
    private import gc_guess_stack;

extern (C) void _STI_monitor_staticctor();
extern (C) void _STD_monitor_staticdtor();
extern (C) void _STI_critical_init();
extern (C) void _STD_critical_term();
extern (C) void gc_init();
extern (C) void gc_term();
extern (C) void _minit();
extern (C) void _moduleCtor();
extern (C) void _moduleDtor();
extern (C) void _moduleUnitTests();

extern (C) bool no_catch_exceptions;

/***********************************
 * The D main() function supplied by the user's program
 */
//int main(char[][] args);
extern (C) alias int function(char[][] args) main_type;

/***********************************
 * Substitutes for the C main() function.
 * It's purpose is to wrap the call to the D main()
 * function and catch any unhandled exceptions.
 */

/* Note that this is not the C main function, nor does it refer
   to the D main function as in the DMD version.  The actual C
   main is in cmain.d

   This serves two purposes:
   1) Special applications that have a C main declared elsewhere.

   2) It is possible to create D shared libraries that can be used
   by non-D executables. (TODO: Not complete, need a general library
   init routine.)
*/
   
extern (C) int _d_run_main(int argc, char **argv, main_type main_func)
{
    char[] *am;
    char[][] args;
    int i;
    int result;

    version (GC_Use_Stack_Guess)
        stackOriginGuess = &argv;
    version (GNU_CBridge_Stdio)
        _d_gnu_cbridge_init_stdio();
    // Win32: original didn't do this -- what about Gcc?
    _STI_monitor_staticctor();
    _STI_critical_init();
    gc_init();
    am = cast(char[] *) malloc(argc * (char[]).sizeof);

    void go()
    {
        _moduleCtor();
        _moduleUnitTests();

        for (i = 0; i < argc; i++)
        {
            int len = strlen(argv[i]);
            am[i] = argv[i][0 .. len];
        }

        args = am[0 .. argc];

        result = main_func(args);
        _moduleDtor();
        gc_term();
    }

    if (no_catch_exceptions)
        go();
    else
    {
        try
            go();
        catch (Object o)
        {
            char[] msg = o.toString();
            fprintf(stderr, "Error: ");
            fprintf(stderr, "%.*s\n", cast(int) msg.length, msg.ptr);
            exit(EXIT_FAILURE);
        }
    }
    
    free(am);
    _STD_critical_term();
    _STD_monitor_staticdtor();

    return result;
}
