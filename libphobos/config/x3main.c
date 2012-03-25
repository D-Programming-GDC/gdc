#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "x3.h"

/* TODO: need to use autoconf to figure out which header to
   include for unlink -- but need to do it for the *build*
   system.  Also, before linking disabled due to cross
   compilation...  */

extern int unlink(const char *fn);

extern void x3_go(void);

static char * g_output_fn = NULL;

void cleanup(void)
{
    if (g_output_fn)
        unlink(g_output_fn);
}

int main(int argc, char **argv)
{
    X3_Global * global = x3_global_create();
    X3_Array gcc_cmd;
    int i = 1;
    const char * output_fn = "x3_out.d";

    atexit(& cleanup);

    while (i < argc && argv[i][0] == '-') {
        if (strcmp("-o", argv[i]) == 0 && (i + 1) < argc)
            output_fn = argv[++i];
        else if (strcmp("-t", argv[i]) == 0 && (i + 1) < argc)
            x3_global_set_temp_fn_base(argv[++i]);
        else if (strcmp("-v", argv[i]) == 0)
            global->print_gcc_command = 1;
        else {
            fprintf(stderr, "invalid option '%s'\n", argv[i]);
            return 1;
        }
        ++i;
    }

    unlink(output_fn);

    x3_array_init(& gcc_cmd);
    for (; i < argc; i++)
        x3_array_push(& gcc_cmd, argv[i]);
    global->gcc_command = gcc_cmd;

    if (! x3_compile_test()) {
        fprintf(stderr, "cannot run the compiler\n");
        return 1;
    }

    if (! x3_query_target_info(& global->target_info))
        return 1;
    x3_gi_push_standard();

    global->output_stream = fopen(output_fn, "w");
    if (! global->output_stream)
        x3_error("cannot open %s: %s", output_fn, strerror(errno));

    g_output_fn = output_fn;

    x3_go();

    fclose(global->output_stream);

    if (global->error_count == 0)
        g_output_fn = NULL;

    return global->error_count == 0 ? 0 : 1;
}
