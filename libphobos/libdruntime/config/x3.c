#include <string.h>
#include <stdlib.h>
#include <libiberty.h>
#include <errno.h>

/* TODO: need to use autoconf to figure out which header to
   include for unlink -- but need to do it for the *build*
   system.  Also, before linking disabled due to cross
   compilation...  */

extern int unlink(const char *fn);

#include "x3.h"
#include "xregex.h"

#define x3_assert(cond) {if(cond){}else{x3_error("assert failed: %s:%d: %s", \
                                 __FILE__,__LINE__,#cond);exit(1);}}

#define x3_new(T) ((T*)malloc(sizeof(T)))
#define x3_new0(T) ((T*)calloc(1, sizeof(T)))

#define diag(x) fprintf(stderr, x);

static void _x3_write_d_int(FILE * f, X3_Int value, X3_Type *type);
static void _x3_write_d_int_2(FILE * f, X3_Int value, unsigned size, int is_signed);

char *
x3f(const char * fmt, ...)
{
    char * result = NULL;
    va_list va;

    va_start(va, fmt);
    vasprintf(& result, fmt, va);

    return result;
}

X3_Global * global;

void
x3_array_init(X3_Array *ma)
{
    memset(ma, 0, sizeof(*ma));
}

void
x3_array_fini(X3_Array *ma)
{
    free(ma->data);
    memset(ma, 0, sizeof(*ma));
}

void
x3_array_push(X3_Array *ma, void *data)
{
    if (ma->length >= ma->alloc) {
        if (ma->alloc)
            ma->alloc *= 2;
        else
            ma->alloc = 4;
        ma->data = realloc(ma->data, sizeof(void*) * ma->alloc);
    }
    ma->data[ma->length++] = data;
}

void
x3_array_pop(X3_Array *ma, X3_Size count)
{
    if (count <= ma->length)
        ma->length -= count;
    else
        ma->length = 0;
}

void*
x3_array_first(X3_Array *ma)
{
    if (ma->length)
        return ma->data[0];
    else
        return NULL;
}

void*
x3_array_last(X3_Array *ma)
{
    if (ma->length)
        return ma->data[ma->length - 1];
    else
        return NULL;
}

X3_Result *
x3_result_new()
{
    X3_Result * r = x3_new(X3_Result);
    x3_result_init(r);
    return r;
}

void
x3_result_init(X3_Result * r)
{
    memset(r, 0, sizeof(*r));
}

void
x3_result_delete(X3_Result * r)
{
    if (! r)
        return;
    free(r->data);
    r->data = NULL;
    free(r);
}

int
x3_result_read_from_file(X3_Result * r, const char *filename)
{
    FILE * f;
    X3_Size f_size;
    int result = 0;

    free(r->data);
    r->data = NULL;

    f = fopen(filename, "rb");
    if (! f) {
        fprintf(stderr, "cannot open object file: %s: %s\n", filename, strerror(errno));
        return 0;
    }
    fseek(f, 0, SEEK_END);
    f_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    r->data = (X3u8*) malloc(f_size);
    r->length = f_size;
    if (fread(r->data, 1, f_size, f) != f_size) {
        fprintf(stderr, "failed to read object file: %s: %s\n", filename, strerror(errno));
        goto cleanup;
    }

    result = 1;

 cleanup:
    fclose(f);
    return result;
}

void
x3_result_rewind(X3_Result * r)
{
    r->pos = 0;
}

void
x3_result_skip(X3_Result * r, unsigned count)
{
    /* !! */
    r->pos += count;
}

static inline X3sig
x3_result_read_sig(X3_Result * r)
{
    X3sig sig;
    sig = (r->data[r->pos])|(r->data[r->pos+1]<<8)|
        (r->data[r->pos+2]<<16)|(r->data[r->pos+3]<<24);
    r->pos += 4;
    return sig;
}
static inline unsigned/*!!*/
x3_result_read_u32(X3_Result * r)
{
    return x3_result_read_integer(r, 4, 0);
}

int
x3_result_find_next(X3_Result * r, X3sig sig)
{
    /* assumes data in object file is aligned.. */
    while (r->pos < r->length && (r->pos & 3))
        ++r->pos;

    while (r->pos < r->length) {
        if (x3_result_read_sig(r) == X3_SIG_HEADER)
            if (r->pos < r->length &&
                x3_result_read_sig(r) == sig)
                return 1;
    }
    return 0;
}

int
x3_result_find_sig(X3_Result * r, X3sig sig)
{
    x3_result_rewind(r);
    if (x3_result_find_next(r, sig))
        return 1;
    else {
        x3_error("cannot find expected data in output");
        return 0;
    }
}

int
x3_result_find_item(X3_Result * r, unsigned uid)
{
    x3_result_rewind(r);
    while (x3_result_find_next(r, X3_SIG_ITEM))
        if (x3_result_read_uint(r) == uid)
            return 1;
    return 0;
}

int
x3_result_read_u8(X3_Result * r)
{
    if (r->pos < r->length)
        return r->data[r->pos++];
    x3_error("read beyond end of input");
    return -1;
}

int
x3_result_read_int(X3_Result * r)
{
    return x3_result_read_integer(r, global->target_info.sizeof_int, 1);
}

unsigned
x3_result_read_uint(X3_Result * r)
{
    return x3_result_read_integer(r, global->target_info.sizeof_int, 0);
}

X3_TgtSize
x3_result_read_size(X3_Result * r)
{
    return x3_result_read_integer(r, global->target_info.sizeof_size_type, 0);
}

X3_Int
x3_result_read_integer(X3_Result * r, int size, int is_signed)
{
    X3_UInt result;
    X3u8 * p = r->data + r->pos ;

    if (! size)
        return 0;

    r->pos += size;
    if (! r->target_info->is_big_endian)
        p += size - 1;
    if (is_signed && (*p & 0x80))
        result = (X3_Int) -1;
    else
        result = 0;
    if (r->target_info->is_big_endian)
        while (size--)
            result = (result << 8) | *p++;
    else
        while (size--)
            result = (result << 8) | *p--;
    return result;
}

X3_InputItem*
x3_input_item_new()
{
    X3_InputItem * ii = x3_new0(X3_InputItem);
    return ii;
}

static void
_x3_input_item_text_write_func(X3_InputItem * ii , FILE * f)
{
    fputs(ii->input_text, f);
    fputc('\n', f);
}

static void
_x3_input_item_subst_text_write_func(X3_InputItem * ii , FILE * f)
{
    const char *p;
    const char *text = ii->input_text;
    while ( (p = strchr(text, '@')) ) {
        fwrite(text, 1, p - text, f);
        fprintf(f, "%d", ii->uid);
        text = p + 1;
    }
    fputs(text, f);
    fputc('\n', f);
}

X3_InputItem*
x3_input_item_new_text(const char * text)
{
    X3_InputItem * ii = x3_new0(X3_InputItem);
    ii->input_text = text;
    ii->write_func = & _x3_input_item_text_write_func;
    return ii;
}

X3_InputGroup *
x3_input_group_new()
{
    X3_InputGroup * ig = x3_new(X3_Array);
    x3_array_init(ig);
    return ig;
}

X3_Global*
x3_global_create()
{
    global = x3_new0(X3_Global);
    x3_global_init(global);
    return global;
}

void
x3_global_init(X3_Global * global)
{
    /* needed for integer writing to work */
    x3_assert(sizeof(X3_Int) >= 8); /* maybe not for 32-bit targets */
    x3_assert(sizeof(int) >= 4);
    x3_assert(sizeof(int) * 2 >= sizeof(X3_Int));

    memset(global, 0, sizeof(*global));
    x3_array_init(& global->gcc_command);

    x3_array_init(& global->input_groups);

    global->header_group = x3_input_group_new();
    x3_array_push(& global->input_groups, global->header_group);

    global->current_group = x3_input_group_new();
    x3_array_push(& global->input_groups, global->current_group);

    x3_array_init(& global->optional_stack);

    global->log_stream = stderr;

    x3_push_optional(0);

}

void
x3_global_set_temp_fn_base(const char * temp_base)
{
    global->temp_base = temp_base;

    char * log_fn = concat(global->temp_base, ".log", NULL);
    if ( (global->log_stream = fopen(log_fn, "a")) != NULL )
        global->log_path = log_fn;
    else
        free(log_fn);
}

char *
x3_global_get_temp_fn(const char * suffix)
{
    // TODO: use make_temp_file..
    if (global->temp_base) {
        char buf[96];
        snprintf(buf, sizeof(buf), "%s%s", global->temp_base, suffix);
        return strdup(buf);
    } else
        return make_temp_file(suffix);
}

int
x3_global_keep_temps()
{
    return global->temp_base != NULL;
}


void
x3_push_optional(int optional)
{
    x3_array_push(& global->optional_stack, (void *) optional);
}

void
x3_pop_optional()
{
    x3_array_pop(& global->optional_stack, 1);
}

int
x3_is_optional()
{
    return x3_array_last(& global->optional_stack) != (void*) 0 ;
}

static void x3_logv(const char *fmt, va_list va)
{
    FILE * stream = stderr;

    if (global)
        stream = global->log_stream;

    fputs("x3: ", stream);
    vfprintf(stream, fmt, va);
    fputc('\n', stream);
}


void x3_log(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    x3_logv(fmt, va);
    va_end(va);
}

void
x3_error(const char *fmt, ...)
{
    va_list va, va2;

    fputs("x3: ", stderr);
    va_start(va, fmt);
    va_copy(va2, va);
    x3_logv(fmt, va2);
    vfprintf(stderr, fmt, va);
    if (global)
        global->error_count++;
    fputc('\n', stderr);
    exit(1);
}

int
x3_unsupported(const char * detail)
{
    x3_error("target unsupported: %s", detail);
    return 0;
}

int
x3_interp_error(const char * detail)
{
    x3_error("cannot interpret output: %s", detail);
    return 0;
}

int
x3_gcc_execute(X3_Array extra_args)
{
    int status;
    const char * result;
#if _WIN32
    /* x3 programs get the c:\foo\bar form of path names.  Passing that
       as a program name to the MSYS shell's 'exec' causes the path
       to be mangled. */
    const char * exec_bit = "\"$@\"";
#else
    const char * exec_bit = "exec \"$@\"";
#endif
    char * cmd = concat(exec_bit,
        global->log_path ? " >>" : "",
        global->log_path ? global->log_path : "", // TODO: assumptions about charaters in the path
        global->log_path ? " 2>&1" : "",
        NULL);

    char**args = (char**)malloc(sizeof(char*) *
        (global->gcc_command.length + extra_args.length + 4 + 1));
    int argi = 0;
    char * shell = getenv("SHELL");

    if (! shell)
        shell = "sh";

    args[argi++] = shell;
    args[argi++] = "-c";
    args[argi++] = cmd;
    args[argi++] = shell;

    memcpy(args + argi, global->gcc_command.data,
        global->gcc_command.length * sizeof(char*));
    argi += global->gcc_command.length;

    memcpy(args + argi, extra_args.data,
        extra_args.length * sizeof(char*));
    argi += extra_args.length;
    args[argi++] = NULL;

    FILE * stream = global->log_stream;
    int i;
    if (global->print_gcc_command) {
        fprintf(stream, "x3: running gcc: ");
        for (i = 0; args[i]; ++i)
            fprintf(stream, "%s ", args[i]);
        fprintf(stream, "\n");
    }
    fflush(stream);

#ifdef PEX_RECORD_TIMES
    {
        int err;
        result = pex_one(PEX_SEARCH, args[0], args, "x3", NULL, NULL, & status, & err);
        if (! result) {
            /*
              if (status == 0)
              return 1;
              else { // TODO: signals fatal?
              fprintf(stderr, "error: %s exited with %s %d\n",
              args[0], status > 0 ? "code" : "signal",
              status > 0 ? status : -status);
              return 0;
              }
            */
            return status == 0;
        }
    }
#else
    {
        char * errmsg_arg;
        char * rr;

        int pid = pexecute(args[0], args, "x3", NULL,
            & rr, & errmsg_arg, PEXECUTE_SEARCH | PEXECUTE_ONE);
        result = rr;
        if (pid > 0) { /* !! */
            if (pwait(pid, & status, 0) != -1)
                return status == 0;
            else {
                fprintf(stderr, "pwait failed: %s\n", strerror(errno));
                return 0;
            }
        }
    }
#endif
    free(cmd);

    x3_error("failed to run compiler: %s", result);
    return 0;
}

int
x3_gcc_execute_l(const char * extra, ...)
{
    va_list va;
    X3_Array extra_args;

    va_start(va, extra);
    x3_array_init(& extra_args);
    x3_array_push(& extra_args, (/*!!*/ char *) extra);
    while (1) {
        char *p = va_arg(va, char *);
        if (p)
            x3_array_push(& extra_args, p);
        else
            break;
    }
    va_end(va);

    return x3_gcc_execute(extra_args);
}

/* Run compiler using the current input data. Not an error if it fails. */
int
x3_compile_test(void)
{
    const char * src_fn = x3_global_get_temp_fn(".c");
    const char * obj_fn = x3_global_get_temp_fn(".o");
    FILE * f = fopen(src_fn, "w");
    int result = 0;
    int gcc_result;

    x3_result_delete(global->result);
    global->result = NULL;

    /* TODO: errors */
    x3_gi_write_to_FILE(f);
    fclose(f);

    gcc_result = x3_gcc_execute_l("-c", src_fn, "-o", obj_fn, NULL);
    if (gcc_result) {
        int i1, i2;

        /* open blob... */
        global->result = x3_result_new();
        global->result->target_info = & global->target_info;
        if (! x3_result_read_from_file(global->result, obj_fn))
            /* TODO print error? */
            goto cleanup;

        for (i1 = 0; i1 < x3al(& global->input_groups); ++i1) {
            X3_InputGroup * ig = x3ae(& global->input_groups, i1, X3_InputGroup *);
            for (i2 = 0; i2 < x3al(ig); ++i2) {
                X3_InputItem * ii = x3ae(ig, i2, X3_InputItem *);
                if (ii->post_func)
                    ii->post_func(ii, global->result);
            }
        }

        result = 1;
    }

 cleanup:
    if (! x3_global_keep_temps()) {
        unlink(src_fn);
        unlink(obj_fn);
    }
    return result;
}

char *
_x3_read_line(FILE *f, char * buf, unsigned buf_size)
{
    char * p = fgets(buf, buf_size, f);
    char * q;
    if (! p)
        return NULL;
    while (! (q=strchr(p, '\n')) && ! feof(f)) {
        size_t len = strlen(p);
        if (p == buf) {
            p = (char*) malloc(len * 2);
            strncpy(p, buf, len);
            p[len] = 0;
        } else
            p = (char*) realloc(p, len * 2);
        fgets(p + len, len, f);
    }
    if (q)
        *q = 0;
    return p;
}

static regex_t *
_x3_get_macros_regex() {
    static int initialized = 0;
    static regex_t macros_regex;
    if (! initialized) {
        initialized = 1;
        if (regcomp(& macros_regex,
                "^#define ([^[:space:]\(]+)(\\([^[:space:]]+\\))?([[:space:]](.*))?$",
                REG_EXTENDED) != 0) {
            x3_assert("failed to compile regex"==0);
            return NULL;
        }
    }
    return & macros_regex;
}

char *
_x3_strdup_slice(const char * s, int start, int end)
{
    int len = end - start;
    char * result = (char*) malloc(len + 1);
    strncpy(result, s + start, len);
    result[len] = '\0';
    return result;
}

int
x3_compile_get_macros(X3_Array * out_macros)
{
    const char * src_fn = x3_global_get_temp_fn(".c");
    const char * mac_fn = x3_global_get_temp_fn(".macros");
    FILE * f = fopen(src_fn, "w");
    int result = 0;
    int gcc_result;
    regex_t * re = _x3_get_macros_regex();

    /* TODO: errors */
    x3_gi_write_to_FILE(f);
    fclose(f);

    if (! re)
        return 0;

    gcc_result = x3_gcc_execute_l("-E", "-dM", "-o", mac_fn, src_fn, NULL);
    if (gcc_result) {
        X3_Array macros;
        char buf[1024];
        char *line;

        result = 0;
        f = fopen(mac_fn, "r");
        if (!f) {
            x3_error("cannot open %s: %s", mac_fn, strerror(errno));
            goto cleanup;
        }

        x3_array_init(& macros);

        while ( (line = _x3_read_line(f, buf, sizeof(buf))) ) {
            regmatch_t matches[4];
            if (regexec(re, line, 4, matches, 0) == 0) {
                X3_Macro * macro = x3_new0(X3_Macro);
                macro->name = _x3_strdup_slice(line, matches[1].rm_so, matches[1].rm_eo);
                macro->args = _x3_strdup_slice(line, matches[2].rm_so, matches[2].rm_eo);
                macro->def = _x3_strdup_slice(line, matches[3].rm_so, matches[3].rm_eo);
                /*fprintf(stderr, "{%s}{%s}{%s}\n", macro->name, macro->args, macro->def);*/
                x3_array_push(& macros, macro);
            }
        }

        *out_macros = macros;
        result = 1;
    }

 cleanup:
    if (! x3_global_keep_temps()) {
        unlink(src_fn);
        unlink(mac_fn);
    }
    return result;
}

/* Run GCC.  If it fails, it is an error. */
int
x3_compile(void)
{
    int result = x3_compile_test();
    if (! result)
        x3_error("error: compiler exited with error");
    return result;
}

static const char * _x3_offsetof_function = "offsetof";

void
x3_gi_push_standard()
{
    x3_gi_push_text(x3f("typedef %s x3i_size_t;", global->target_info.size_type));

    /* GCC 3.x does not have __builtin_offsetof... */
    x3_gi_push_header("<stddef.h>");
}

void
x3_gi_write_to_FILE(FILE *f)
{
    int i1, i2;
    unsigned uid = 1;

    for (i1 = 0; i1 < x3al(& global->input_groups); ++i1) {
        X3_InputGroup * ig = x3ae(& global->input_groups, i1, X3_InputGroup *);
        for (i2 = 0; i2 < x3al(ig); ++i2) {
            X3_InputItem * ii = x3ae(ig, i2, X3_InputItem *);
            ii->uid = uid++;
            if (ii->write_func)
                ii->write_func(ii, f);
        }
    }
}


void
x3_gi_push_header(const char *hdr)
{
    X3_InputItem * ii = x3_input_item_new_text(x3f("#include %s\n", hdr));
    x3_array_push(global->header_group, ii);
}

void
x3_gi_push_headers_l(const char * hdr, ...)
{
    x3_gi_push_header(hdr);

    va_list va;
    va_start(va, hdr);
    while (1) {
        const char *p = va_arg(va, const char *);
        if (p)
            x3_gi_push_header(p);
        else
            break;
    }
    va_end(va);
}

void
x3_gi_pop_header()
{
    x3_gi_pop_headers(1);
}

void
x3_gi_pop_headers(int count)
{
    x3_array_pop(global->header_group, 1);
}

void
x3_gi_push_group()
{
    global->current_group = x3_input_group_new();
    x3_array_push(& global->input_groups, global->current_group);
}

void
x3_gi_pop_group()
{
    x3_array_pop(& global->input_groups, 1);
    global->current_group = x3_array_last(& global->input_groups);
}

void
x3_gi_push_input_item(X3_InputItem *ii)
{
    x3_array_push(global->current_group, ii);
}

void
x3_gi_pop_items(int count)
{
    x3_array_pop(global->current_group, count);
}

X3_InputItem*
x3_gi_push_text(const char *text)
{
    X3_InputItem* ii = x3_input_item_new_text(text);
    x3_gi_push_input_item(ii);
    return ii;
}

X3_InputItem*
x3_gi_push_subst_text(const char *text)
{
    X3_InputItem* ii = x3_input_item_new_text(text);
    ii->write_func = & _x3_input_item_subst_text_write_func;
    x3_gi_push_input_item(ii);
    return ii;
}


static void
_x3_input_int_value_write_func(X3_InputItem * xi , FILE * f)
{
    X3_InputIntValue * ii = (X3_InputIntValue *) xi;
    unsigned uid = ii->iit.ii.uid;
    const char *type_name = ii->iit.type_name;
    x3_gi_write_start_item(f, uid);
    fprintf(f, "x3i_size_t int_sz; int int_sgn; %s int_val;\n", type_name);
    x3_gi_write_start_item_data(f, uid);
    fprintf(f, "sizeof(%s), ((%s)-1)<0, %s", type_name, type_name,
        ii->iit.ii.input_text);
    x3_gi_write_finish_item(f);
}

static void
_x3_input_int_value_post_func(X3_InputItem * xi , X3_Result * result)
{
    X3_InputIntValue * ii = (X3_InputIntValue *) xi;
    if (x3_result_find_item(result, ii->iit.ii.uid)) {
        int sz = x3_result_read_size(result);
        int sgn = x3_result_read_int(result);
        ii->iit.int_type_size = sz;
        ii->iit.int_type_is_signed = sgn;
        ii->int_value = x3_result_read_integer(result, sz, sgn);
    }
    /* !! find_item doesn't record error ... do it here? */
}

X3_InputIntValue*
x3_gi_push_int_value(const char *expr)
{
    X3_InputIntValue * ii = x3_new0(X3_InputIntValue);
    ii->iit.ii.write_func = & _x3_input_int_value_write_func;
    ii->iit.ii.post_func = & _x3_input_int_value_post_func;
    ii->iit.type_name = "int";
    ii->iit.ii.input_text = expr;
    x3_gi_push_input_item(& ii->iit.ii);
    return ii;
}


void
x3_gi_push_default_enum(const char * default_val, ...)
{
    va_list va;
    char * result = "";

    va_start(va, default_val);
    while (1) {
        const char * p = va_arg(va, const char *);
        if (! p)
            break;
        result = x3f("%s"
            "#ifndef %s\n# define %s %s\n#endif\n",
            result, p, p, default_val);
    }
    x3_gi_push_text(result);
}

void
x3_gi_write_start_item(FILE *f, unsigned uid)
{
    fprintf(f, "struct { char _x3_sig[4]; char _x3_type[4]; unsigned _x3_uid;\n  ");
}

void
x3_gi_write_start_item_data(FILE *f, unsigned uid)
{
    fprintf(f, "} __attribute__((packed)) \n_x3_item_%d\n = { \"zOmG\", \"iTeM\", %d, ",
        uid, uid);
}

void
x3_gi_write_finish_item(FILE *f)
{
    fprintf(f, "};\n");
}

int
x3_query_target_info(X3_TargetInfo *ti)
{
    int v;
    int result = 0;

    x3_gi_push_group();

    x3_gi_push_text("struct { char sig[4]; char what[4];"
        "unsigned char sz_char; unsigned char sz_short; "
        "unsigned char sz_int; unsigned char sz_long_long; "
        "unsigned char sz_long; unsigned short endian_test; }\n "
        "__attribute__((packed)) _x3i_tgt_info = {\n"
        "\"zOmG\", \"tGtI\", sizeof(char), sizeof(short), "
        "sizeof(int), sizeof(long long), sizeof(long), "
        "0x2211 };\n");
    if (! x3_compile())
        goto cleanup;

    if (! x3_result_find_sig(global->result, X3_SIG('t','G','t','I')))
        goto cleanup;

    ti->sizeof_char = x3_result_read_u8(global->result);
    if (ti->sizeof_char != 1) {
        x3_unsupported("sizeof(char) is not 1");
        goto cleanup;
    }
    ti->sizeof_short = x3_result_read_u8(global->result);
    ti->sizeof_int = x3_result_read_u8(global->result);
    ti->sizeof_long_long = x3_result_read_u8(global->result);
    ti->sizeof_long = x3_result_read_u8(global->result);
    v = x3_result_read_u8(global->result);
    if (v == 0x11)
        ti->is_big_endian = 0;
    else {
        x3_result_skip(global->result, ti->sizeof_short - 2);
        if (x3_result_read_u8(global->result) != 0x11)
            x3_interp_error("cannot determine byte order");
        ti->is_big_endian = 1;
    }

    x3_gi_push_header("<sys/types.h>");
    x3_gi_pop_items(1);
    x3_gi_push_text("struct { char sig[4]; char what[4];"
        "unsigned char sz_szt; }\n "
        "__attribute__((packed)) _x3i_szt_info = {\n"
        "\"zOmG\", \"tGtI\", sizeof(size_t) };\n");
    x3_compile();
    x3_gi_pop_headers(1);
    ti->size_type = "unsigned int";
    ti->sizeof_size_type = ti->sizeof_int;

    result = 1;

 cleanup:
    x3_gi_pop_group();
    return result;
}

int
x3_query_int_type(const char *name, X3_Type * type)
{
    X3_Int dummy;
    return x3_query_int_value("0", name, & dummy, type);
}

int
x3_query_type(const char *name, X3_Type * type)
{
    static unsigned uid_counter = 1;
    X3_Int  int_val;
    X3_Type dummy;
    char * p;
    unsigned my_uid = ++uid_counter;

    type->ok = 0;

    x3_gi_push_group();

    /* test if type is even valid */
    const char * td_name = x3f("_x3i_qttd_%d", my_uid);
    x3_gi_push_text(p = x3f("typedef %s %s;\n", name, td_name));
    if (! x3_compile_test())
        goto cleanup;

    /* is it just void? */
    if (x3_query_int_value(x3f("__builtin_types_compatible_p(void,%s)", name),
            "int", & int_val, & dummy) && int_val) {
        type->ok = 1;
        type->kind = X3_TYPE_KIND_VOID;
    } else {
        if (! x3_query_int_value(x3f("sizeof(%s)", name), "x3i_size_t", & int_val, & dummy))
            goto cleanup;
        type->type_size = (X3_UInt) int_val;

        /* test if scalarish (or array...) */
        int result1;
        x3_gi_push_subst_text(x3f("\n"
                "const %s  _x3i_qt_@_1;\n"
                "typedef typeof(_x3i_qt_@_1 < 0) _x3i_qt_@_2;", name));
        result1 = x3_compile_test();
        x3_gi_pop_items(1);
        if (result1) {
            int result2;
            /* is it a pointer or array? */
            const char * sub_td_name = x3f("_x3i_qttd_%d_sub", my_uid);
            x3_gi_push_subst_text(p = x3f("\n"
                    "const %s  _x3i_qt_inst_@;\n"
                    "typedef typeof(* _x3i_qt_inst_@) %s;", name, sub_td_name));
            result2 = x3_compile_test();
            if (result2) {

                if (x3_query_int_value(x3f(""
                            "__builtin_types_compatible_p(%s[],%s)",
                            sub_td_name, td_name),
                        "int", & int_val, & dummy) && int_val) {
                    /* It is an array type. */
                    type->kind = X3_TYPE_KIND_ARRAY;
                    type->next = x3_type_new();
                    type->next->flags |= type->flags & (X3_FLAG_USE_CHAR|X3_FLAG_USE_BYTE);
                    if (! x3_query_type(sub_td_name, type->next))
                        goto cleanup;
                    if (type->type_size && type->next->type_size) {
                        x3_assert( (type->type_size % type->next->type_size) == 0);
                        type->u.array_length = type->type_size / type->next->type_size;
                    } else
                        type->u.array_length = 0;
                    type->next->out_name = NULL;
                    type->ok = 1;

                } else { /* It is pointer. */
                    int is_ptr_to_func = 0;

                    type->kind = X3_TYPE_KIND_POINTER;
                    type->next = x3_type_new();

                    /* Test for pointer to function which can do *type forever... */
                    x3_gi_push_subst_text(x3f("\n"
                            "const %s _x3i_qtptf_@;\n"
                            "typedef typeof(*_x3i_qtptf_@) _x3i_qtptf_f2_%d;\n",
                            sub_td_name, my_uid));
                    is_ptr_to_func =  x3_query_int_value(x3f("\n"
                            "__builtin_types_compatible_p(%s,_x3i_qtptf_f2_%d)",
                            sub_td_name, my_uid),
                        "int", & int_val, & dummy) && int_val;
                    x3_gi_pop_items(1);
                    if (! is_ptr_to_func)
                        x3_query_type(sub_td_name, type->next);
                    if (! type->next->ok || type->next->kind == X3_TYPE_KIND_RECORD) {
                        /* void* is good enough... */
                        type->next->ok = 1;
                        type->next->kind = X3_TYPE_KIND_VOID;
                        type->next->c_name = type->next->out_name = "void";
                    }
                    type->next->out_name = NULL;
                    type->ok = 1;
                }
                x3_gi_pop_items(1);
            } else {
                x3_gi_pop_items(1);
                /* Must be an int */
                x3_query_int_type(name, type);
                type->out_name = NULL;
            }
        } else {
            /* if we get a pointer to this, there's gonna be a problem...
               array and pointer above need to check this and either push
               a private type or change it to an array of bytes... !!

               also a problem if an instance...
            */

            X3_Struct * s = x3_struct_new();
            X3_Field * f;

            f = x3_struct_new_field(s);
            f->ok = 1;
            f->out_name = "__opaque";
            f->field_type.ok = 1;
            f->field_type.kind = X3_TYPE_KIND_NAME;
            f->field_type.type_size = type->type_size;
            f->field_type.out_name = x3f("byte[%u]", (unsigned) type->type_size);
            /* might be function (not ptr-to-functiono) or struct... keep it as a name.. */
            /* !! figure out alignment... */
            type->ok = 1;
            type->kind = X3_TYPE_KIND_RECORD;
            type->u.struct_def = s;

            s->finished = 1;
            s->struct_type = *type;
        }
    }

 cleanup:
    --uid_counter;
    x3_gi_pop_group();
    return type->ok;
}

int
x3_query_int_value(const char *expr, const char *type_name,
    X3_Int * out_value, X3_Type * out_type)
{
    x3_assert(type_name != NULL);
    X3_InputIntValue *c = x3_gi_push_int_value(expr);
    c->iit.type_name = type_name;
    if (x3_compile_test()) {
        out_type->ok = 1;
        out_type->kind = X3_TYPE_KIND_INT;
        out_type->c_name = type_name;
        if (! out_type->out_name)
            out_type->out_name = type_name;
        out_type->type_size = c->iit.int_type_size;
        out_type->type_is_signed = c->iit.int_type_is_signed;
        *out_value = c->int_value;
    } else
        out_type->ok = 0;
    x3_gi_pop_items(1);
    return out_type->ok;
}


static const char *
_x3_d_int_type(X3_Type * type)
{
    x3_assert(type->kind == X3_TYPE_KIND_INT);
    switch (type->type_size) {
    case 1:
        if (type->flags & X3_FLAG_USE_BYTE)
            return type->type_is_signed ? "byte" : "ubyte";
        else
            return "char";
    case 2: return type->type_is_signed ? "short" : "ushort";
    case 4: return type->type_is_signed ? "int" : "uint";
    case 8: return type->type_is_signed ? "long" : "ulong";
    default:
        x3_error("no D equivalent for integer type of %d bytes", (int) type->type_size);
        return "error";
    }
}

static const char *
_x3_d_char_type(X3_Type * type)
{
    x3_assert(type->kind == X3_TYPE_KIND_INT);
    switch (type->type_size) {
    case 1: return "char";
    case 2: return "wchar";
    case 4: return "dchar";
    default:
        x3_error("no D character type of %d bytes", (int) type->type_size);
        return "error";
    }
}

static void
_x3_write_d_int_2(FILE * f, X3_Int value, unsigned size, int is_signed)
{
    if (is_signed) {
        int hi = (value >> (int)32) & 0xffffffff;
        if (hi == 0 || hi == -1)
            fprintf(f, "%d", (int)value);
        else
            /* !! grr.... */
            fprintf(f, "cast(long)0x%x%08x", hi, (unsigned)(value & 0xffffffff));
    } else {
        unsigned hi = ((X3_UInt) value >> (unsigned)32) & 0xffffffff;
        if (hi == 0)
            fprintf(f, "%u", (unsigned)value);
        else
            fprintf(f, "0x%x%08xUL", hi, (unsigned)(value & 0xffffffff));
    }
}

static void
_x3_write_d_int(FILE * f, X3_Int value, X3_Type *type)
{
    x3_assert(type->kind == X3_TYPE_KIND_INT);
    _x3_write_d_int_2(f, value, type->type_size, type->type_is_signed);
}

static const char * _x3_format_d_type(X3_Type *type);

static const char *
_x3_format_d_type_ex(X3_Type *type, int use_name)
{
    const char * result;
    x3_assert(type->ok);
    if (! type->out_name || ! use_name) {
        switch (type->kind)
        {
        case X3_TYPE_KIND_VOID:
            result = "void";
            break;
        case X3_TYPE_KIND_INT:
            result = _x3_d_int_type(type);
            break;
        case X3_TYPE_KIND_ARRAY:
            result = x3f("%s[%u]", _x3_format_d_type(type->next),
                (unsigned) type->u.array_length);
            break;
        case X3_TYPE_KIND_POINTER:
            result = x3f("%s*", _x3_format_d_type(type->next));
            break;
        default:
            x3_assert(0);
        }
        /*
        if (! type->out_name && use_name)
        type->out_name = result !! // ...should be separate cached.
        */
    } else
        result = type->out_name;
    return result;;
}

static const char *
_x3_format_d_type(X3_Type *type)
{
    return _x3_format_d_type_ex(type, 1);
}


X3_Type *
x3_type_new(void)
{
    X3_Type * t = x3_new0(X3_Type);
    x3_type_init(t);
    return t;
}

void
x3_type_init(X3_Type * type)
{
    memset(type, 0, sizeof(*type));
    type->flags |= X3_FLAG_USE_CHAR;
}

void
x3_type_out(X3_Type * type)
{
    x3_assert(type->ok);

    const char * out_name = type->out_name ? type->out_name : type->c_name;
    if (! out_name)
        return; // !!?

    switch (type->kind)
    {
    case X3_TYPE_KIND_NAME:
        // can't do anything !!?
        break;
    case X3_TYPE_KIND_RECORD:
        x3_struct_out(type->u.struct_def);
        break;
    case X3_TYPE_KIND_ERROR:
        // !!
        break;
    default:
        fprintf(global->output_stream, "alias %s %s;\n",
            _x3_format_d_type_ex(type, 0), out_name);
        break;
    }
}

void
x3_out_int_type(const char* name)
{
    X3_Type it;
    x3_log("checking for int type '%s'", name);
    if (x3_query_int_type(name, & it)) {
        fprintf(global->output_stream, x3f("alias %s %s;\n",
                _x3_d_int_type(& it), name));
    } else if (! x3_is_optional())
        x3_error("cannot resolve integer type %s", name);
}

void x3_out_char_type(const char* name)
{
    X3_Type it;
    x3_log("checking for char type '%s'", name);
    if (x3_query_int_type(name, & it)) {
        fprintf(global->output_stream, x3f("alias %s %s;\n",
                _x3_d_char_type(& it), name));
    } else if (! x3_is_optional())
        x3_error("cannot resolve integer type %s", name);
}


void
x3_out_type(const char* name)
{
    x3_out_type_ex(name, NULL, NULL);
}

const char * X3_STRUCT_FALLBACK = "X3_STRUCT_FALLBACK";

void
x3_out_type_ex(const char* c_name, const char * out_name, const char * fallback)
{
    X3_Type type;

    x3_log("checking for type '%s'", c_name);
    out_name = type.out_name = out_name ? out_name : c_name;
    if (x3_query_type(c_name, & type))
        x3_type_out(& type);
    else if (fallback) {
        if (fallback != X3_STRUCT_FALLBACK)
            x3_out_text(x3f("alias %s %s;\n", fallback, out_name));
        else
            x3_out_text(x3f("struct %s;\n", out_name));
    } else if (! x3_is_optional())
        x3_error("cannot resolve type %s", c_name);
}

void
x3_out_int_value(const char* expr)
{
    x3_out_int_value_ex(expr, expr, NULL, NULL, 0, NULL);
}

void
x3_out_int_value_ex(const char *name, const char* expr, const char *in_type,
    const char *out_type, int explicit_cast, const char * fallback)
{
    X3_Int value;
    X3_Type type;

    if (! expr)
        expr = name;
    x3_log("checking for int value '%s'", expr);
    if (! in_type)
        in_type = "int";
    if (! out_type)
        out_type = in_type;

    if (x3_query_int_value(expr, in_type, & value, & type)) {
        fprintf(global->output_stream, "const %s %s = ",
            out_type, name);
        if (explicit_cast)
            fprintf(global->output_stream, "cast(%s) ", out_type);
        _x3_write_d_int(global->output_stream, value, & type);
        fprintf(global->output_stream, ";\n");
    } else if (fallback) {
        fprintf(global->output_stream, "const %s %s = ",
            out_type, name);
        if (explicit_cast)
            fprintf(global->output_stream, "cast(%s) ", out_type);
        fputs(fallback, global->output_stream);
        fprintf(global->output_stream, ";\n");
    } else if (! x3_is_optional())
        x3_error("cannot resolve integer expression '%s'", expr);
}

void
x3_out_enum(const char *name, const char *type, ...)
{
    X3_Array macros;
    //X3_MArray incl_symbols;
    X3_Array incl_patterns;
    X3_Array excl_patterns;

    X3_Array enum_names;
    X3_Array enum_exprs;

    x3_array_init(& incl_patterns);
    x3_array_init(& excl_patterns);
    x3_array_init(& enum_names);
    x3_array_init(& enum_exprs);

    fputs("x3: enums ", global->log_stream);
    {
        va_list va;
        const char * p;
        va_start(va, type);
        while ( (p = va_arg(va, const char *)) ) {
            /*
            if (strcpsn(p, "-+()[]{}.*?/\\$^") == strlen(p)) { //!! should be ident reg or ident chars..
                x3_marray_push(&
            } else {
            }
            */
            if (excl_patterns.length || incl_patterns.length)
                fputs(", ", global->log_stream);
            fputs(p, global->log_stream);

            int is_excl = *p == '-';
            regex_t * pre = x3_new0(regex_t);

            if (is_excl)
                ++p;

            if (regcomp(pre, p, REG_EXTENDED) != 0) {
                x3_error("invalid pattern '%s'", p);
                exit(1);
            }
            x3_array_push(is_excl ? & excl_patterns : & incl_patterns, pre);
        }
    }
    fputc('\n', global->log_stream);

    if (x3_compile_get_macros(& macros)) {
        int i;
        int mi;
        regmatch_t matches[1];

        x3_gi_push_group();

        for (i = 0; i < macros.length; i++) {
            X3_Macro * macro = (X3_Macro *) macros.data[i];
            int incl = 0, excl = 0;

            if (strlen(macro->args))
                continue;

            for (mi = 0; mi < incl_patterns.length; mi++) {
                if ( regexec((regex_t*) incl_patterns.data[mi],
                        macro->name, 1, matches, 0) == 0) {
                    incl = 1;
                    break;
                }
            }
            if (incl) {
                for (mi = 0; mi < excl_patterns.length; mi++) {
                    if ( regexec((regex_t*) excl_patterns.data[mi],
                            macro->name, 1, matches, 0) == 0) {
                        excl = 1;
                        break;
                    }
                }
            }
            if (incl && ! excl) {
                // !! TYPE
                x3_array_push(& enum_exprs,
                    x3_gi_push_int_value(macro->def));
                x3_array_push(& enum_names, (char *) macro->name);
            }
        }

        if (enum_names.length && x3_compile_test()) {
            FILE * f = global->output_stream;
            fprintf(f, "enum");
            if (name)
                fprintf(f, " %s", name);
            if (type)
                fprintf(f, " : %s", type);
            fprintf(f, "\n{\n");
            for (i = 0; i < enum_names.length; i++) {
                X3_InputIntValue * ii = (X3_InputIntValue *) enum_exprs.data[i];
                fprintf(f, "  %s = ", (char*) enum_names.data[i]);
                _x3_write_d_int_2(f, ii->int_value, ii->iit.int_type_size,
                    ii->iit.int_type_is_signed);
                fprintf(f, ",\n");
            }
            fprintf(f, "}\n\n");
        } else if (! x3_is_optional())
            x3_error("failed to resolve macro values.");

        x3_gi_pop_group();
    } else if (! x3_is_optional())
        x3_error("failed to get macros.");
}

void
x3_out_text(const char *text)
{
    fputs(text, global->output_stream);
}

int
x3_query_struct_field(const char * struct_name, const char * field_name)
{
    x3_gi_push_subst_text(x3f(""
            "const int _x3i_struct_field_test_@ = "
            "%s(%s, %s);", _x3_offsetof_function, struct_name, field_name));
    int result = x3_compile_test();
    x3_gi_pop_items(1);
    return result;
}

static void
_x3_query_cpp_if_write_func(X3_InputItem * ii , FILE * f)
{
    fprintf(f, "#if %s\n", ii->input_text);
    x3_gi_write_start_item(f, ii->uid);
    fputs("int unused;", f);
    x3_gi_write_start_item_data(f, ii->uid);
    fprintf(f, "0");
    x3_gi_write_finish_item(f);
    fputs("#endif\n", f);
}

int
 x3_query_cpp_if(const char * expr)
{
    X3_InputItem * ii = x3_input_item_new();
    int result = 0;

    ii->input_text = expr;
    ii->write_func = & _x3_query_cpp_if_write_func;
    x3_gi_push_input_item(ii);
    if (x3_compile_test())
        result = x3_result_find_item(global->result, ii->uid);
    else if (! x3_is_optional())
        x3_error("failed to test conditional...");

    x3_gi_pop_items(1);

    return result;
}

X3_Struct *
x3_struct_new()
{
    X3_Struct * s = x3_new0(X3_Struct);
    x3_array_init(& s->fields);
    x3_array_init(& s->extra);
    s->struct_type.kind = X3_TYPE_KIND_RECORD;
    s->struct_type.u.struct_def = s;
    return s;
}

X3_Struct *
x3_struct_new_c_name(const char * c_name)
{
    X3_Struct * s = x3_struct_new();
    s->struct_type.c_name = c_name;
    if (strncmp(c_name, "struct ", 7) == 0) {
        s->record_kind = X3_RECORD_STRUCT;
        s->struct_type.out_name = c_name + 7;
    } else if (strncmp(c_name, "union ", 6) == 0) {
        s->record_kind = X3_RECORD_UNION;
        s->struct_type.out_name = c_name + 6;
    } else
        s->struct_type.out_name = c_name;
    return s;
}

void
x3_struct_delete(X3_Struct * s)
{
    if (! s)
        return;
    /* blah... */
    free(s);
}

X3_Field *
x3_struct_new_field(X3_Struct * s)
{
    X3_Field * field = x3_new0(X3_Field);
    x3_type_init(& field->field_type);
    x3_array_push(& s->fields, field);
    return field;
}

X3_Field *
x3_struct_add_field(X3_Struct * s, X3_Field * field)
{
    x3_array_push(& s->fields, field);
    return field;
}


void
_x3_struct_add_fields_v(X3_Struct * s, va_list va)
{
    while (1) {
        const char * p = va_arg(va, const char *);
        const char * name;
        const char * flag_end;
        X3_Field * field;

        if (! p)
            break;
        field = x3_struct_new_field(s);
        if ( (flag_end = strchr(p, ':')) ) {
            name = flag_end + 1;

            while (p < flag_end) {
                switch (*p++) {
                case 'i': field->flags |= X3_FLAG_INT_TYPE; break;
                case 'o': field->flags |= X3_FLAG_OPTIONAL; break;
                case 'c': field->flags |= X3_FLAG_USE_CHAR; break;
                case 'b': field->flags |= X3_FLAG_USE_BYTE; break;
                case 't':
                    {
                        const char * e = strchr(p, ';');

                        char * tn = (char*) malloc(e-p);
                        strncpy(tn, p, e-p);
                        tn[e-p] = 0;

                        field->field_type.kind = X3_TYPE_KIND_NAME;
                        field->field_type.out_name = tn;
                        p = e + 1;
                    }
                    break;
                }
            }
        } else
            name = p;
        field->c_name = field->out_name = name;
    }
}

void
x3_struct_add_fields(X3_Struct * s, ...)
{
    va_list va;
    va_start(va, s);
    _x3_struct_add_fields_v(s, va);
}


void
x3_struct_add_extra(X3_Struct * s, const char * text)
{
    x3_array_push(& s->extra, (char*) text);
}

int
_x3_field_compare(const void * pa, const void * pb)
{
    X3_Field * fa = (X3_Field*) * (void**) pa;
    X3_Field * fb = (X3_Field*) * (void**) pb;
    return (int) fa->field_offset - (int) fb->field_offset;
}

void
x3_struct_finish(X3_Struct * s)
{
    int i;

    x3_log("translating struct '%s'", s->struct_type.c_name);
    x3_gi_push_group();

    X3_Array cur_fields = s->fields;
    X3_Array next_fields;

    x3_array_init(& next_fields);
    for (i = 0; i < cur_fields.length; ++i) {
        X3_Field * field = (X3_Field *) cur_fields.data[i];
        if (/*! field->ok && */(field->flags & X3_FLAG_OPTIONAL)) {
            x3_log("checking for %s.%s", s->struct_type.c_name, field->c_name);
            if (x3_query_struct_field(s->struct_type.c_name, field->c_name))
                x3_array_push(& next_fields, field);
        } else
            x3_array_push(& next_fields, field);
    }

    /* No uid for 'inst' -- assumes no recursive x3_struct_finish. */
    x3_gi_push_text(x3f("static %s inst;", s->struct_type.c_name));

    cur_fields = next_fields;
    for (i = 0; i < cur_fields.length; ++i) {
        X3_Field * field = (X3_Field *) cur_fields.data[i];
        if (! field->field_type.ok) {
            /* note: does not clear existing flags... */
            field->field_type.flags |= field->flags & (X3_FLAG_USE_CHAR | X3_FLAG_USE_BYTE);
            if (field->flags & X3_FLAG_INT_TYPE) {
                x3_query_int_type(x3f("typeof(inst.%s)", field->c_name),
                    & field->field_type);
                field->field_type.out_name = _x3_d_int_type(& field->field_type);
            } else {
                x3_query_type(x3f("typeof(inst.%s)", field->c_name),
                    & field->field_type);
            }
            if (! field->field_type.ok) {
                x3_error("cannot resolve type of %s.%s", s->struct_type.c_name, field->c_name);
                goto cleanup;
            }
        }
    }

    X3_InputIntValue **offsets = (X3_InputIntValue**) calloc(cur_fields.length, sizeof(void**));
    for (i = 0; i < cur_fields.length; ++i) {
        X3_Field * field = (X3_Field *) cur_fields.data[i];
        offsets[i] = x3_gi_push_int_value(x3f("%s(%s,%s)",
                _x3_offsetof_function, s->struct_type.c_name, field->c_name));
    }
    if (! x3_compile())
        goto cleanup;
    for (i = 0; i < cur_fields.length; ++i) {
        X3_Field * field = (X3_Field *) cur_fields.data[i];
        field->field_offset = offsets[i]->int_value;
    }

    qsort(cur_fields.data, cur_fields.length, sizeof(void*), & _x3_field_compare);

    s->fields = cur_fields;

    if (s->record_kind == X3_RECORD_UNKNOWN) {
        if (cur_fields.length > 1 &&
            ((X3_Field *) cur_fields.data[cur_fields.length - 1])->field_offset == 0)
            s->record_kind = X3_RECORD_UNION;
        else
            s->record_kind = X3_RECORD_STRUCT;
    }

    s->struct_type.kind = X3_TYPE_KIND_RECORD;
    {
        X3_Int sz;
        X3_Type dummy;
        if (! x3_query_int_value(x3f("sizeof(%s)", s->struct_type.c_name), "x3i_size_t",
                & sz, & dummy))
            goto cleanup;
        s->struct_type.type_size = sz;
    }

    s->struct_type.ok = 1;
 cleanup:
    x3_gi_pop_group();

    s->finished = 1;
}

static void
_x3_struct_write_to_FILE(X3_Struct * s, FILE * f)
{
    int i;
    unsigned pad_idx = 1;
    X3_TgtSize last_offset = (X3_TgtSize) -1;
    X3_TgtSize next_offset = 0;
    //FieldInfo *p_fi = fields;
    //FieldInfo *p_fi_end = p_fi + field_count;
    int in_union = 0;

    int n_fields = s->fields.length;
    X3_Field **all_fields = (X3_Field **) s->fields.data;

    fprintf(f, "%s %s\n{\n",
        s->record_kind == X3_RECORD_UNION ? "union" : "struct",
        s->struct_type.out_name);

    for (i = 0; i < n_fields; ++i) {
        X3_Field * field = (X3_Field *) all_fields[i];
        if (field->field_offset == last_offset) {
            // in a union
        } else if (in_union) {
            fprintf(f, "    }\n");
            in_union = 0;
        }

        if (field->field_offset > next_offset)
            fprintf(f, "    ubyte[%u] __pad%u;\n",
                (unsigned)(field->field_offset - next_offset), pad_idx++);

        if (! in_union &&
            i + 1 != n_fields &&
            field->field_offset == all_fields[i +1]->field_offset) {
            in_union = 1;
            fprintf(f, "    union {\n");
        }

        fprintf(f, "%s    %s %s%s%s;\n", in_union?"    ":"",
            _x3_format_d_type(& field->field_type), field->out_name,
            field->init_expr ? " = " : "",
            field->init_expr ? field->init_expr : "");
        last_offset = field->field_offset;
        next_offset = last_offset + field->field_type.type_size; /* doesn't handle alignment */
    }
    if (in_union)
        fprintf(f, "   }\n");
    X3_TgtSize size = s->struct_type.type_size;
    if (next_offset < size)
        fprintf(f, "   ubyte[%u] __pad%u;\n", (unsigned)(size - next_offset), pad_idx++);

    for (i = 0; i < s->extra.length; ++i) {
        fputs((const char *) s->extra.data[i], f);
        fputc('\n', f);
    }

    fprintf(f, "}\n\n");
}

void
x3_struct_out(X3_Struct * s)
{
    if (! s->finished)
        x3_struct_finish(s);
    if (s->struct_type.ok)
        _x3_struct_write_to_FILE(s, global->output_stream);
    else if (s->fallback)
        fprintf(global->output_stream, "struct %s;\n",
            s->struct_type.out_name ?
            s->struct_type.out_name : s->struct_type.c_name);
    else if (! x3_is_optional())
        x3_error("cannot resolve struct type %s", s->struct_type.c_name);
}

void
x3_out_struct(const char * c_name, const char *out_name, ...)
{
    X3_Struct * s = x3_struct_new_c_name(c_name);

    va_list va;

    if (out_name)
        s->struct_type.out_name = out_name;

    va_start(va, out_name);
    _x3_struct_add_fields_v(s, va);

    x3_struct_out(s);
}

void x3_out_struct_ex(const char * c_name, const char *out_name, int fallback, ...)
{
    X3_Struct * s = x3_struct_new_c_name(c_name);

    va_list va;

    if (out_name)
        s->struct_type.out_name = out_name;

    s->fallback = fallback;

    va_start(va, fallback);
    _x3_struct_add_fields_v(s, va);

    x3_struct_out(s);
}
