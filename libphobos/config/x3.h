#ifndef __D_PHOBOS_X3_H__
#define __D_PHOBOS_X3_H__

#include <stdio.h>

#include <sys/types.h>
typedef size_t X3_Size;
typedef unsigned long long X3_TgtSize;
typedef long long X3_Int;
typedef unsigned long long X3_UInt;
typedef unsigned char X3u8;
typedef unsigned int X3sig;

char * x3f(const char * fmt, ...);

typedef struct {
    X3_Size length;
    void ** data;
    X3_Size  alloc;
} X3_Array;

void  x3_array_init(X3_Array *ma);
void  x3_array_fini(X3_Array *ma);
void  x3_array_push(X3_Array *ma, void *data);
void  x3_array_pop(X3_Array *ma, X3_Size count);
void  x3_array_copy(X3_Array *ma, X3_Array *from);
void* x3_array_first(X3_Array *ma);
void* x3_array_list(X3_Array *ma);
#define x3ae(ma,idx,type) ((type)(ma)->data[(idx)])
#define x3al(ma) ((ma)->length)

typedef struct {
    unsigned sizeof_char;
    unsigned sizeof_short;
    unsigned sizeof_int;
    unsigned sizeof_long_long;
    unsigned sizeof_long;
    int      is_big_endian;
    const char * s8_type;
    const char * u8_type;
    const char * s32_type;
    const char * u32_type;
    const char * size_type;
    unsigned sizeof_size_type;
} X3_TargetInfo;

typedef struct {
    X3_TargetInfo * target_info;
    X3u8*  data;
    X3_Size length;
    X3_Size pos;
} X3_Result;

X3_Result *x3_result_new();
void x3_result_init(X3_Result * r);
void x3_result_delete(X3_Result * r);
int x3_result_read_from_file(X3_Result * r, const char *filename);
void x3_result_rewind(X3_Result * r);
void x3_result_skip(X3_Result * r, unsigned count);
int x3_result_find_next(X3_Result * r, X3sig sig);
int x3_result_find_sig(X3_Result * r, X3sig sig);
int x3_result_find_item(X3_Result * r, unsigned uid);
int x3_result_read_u8(X3_Result * r);
int x3_result_read_int(X3_Result * r); // read target int, but returns host int
unsigned x3_result_read_uint(X3_Result * r); // read target unsisgned int, but reads host unsigned int
X3_TgtSize x3_result_read_size(X3_Result * r);
X3_Int x3_result_read_integer(X3_Result * r, int size, int is_signed);

#define X3_SIG(a,b,c,d) (((X3sig)(a))|((X3sig)(b)<<8)|((X3sig)(c)<<16)|((X3sig)(d)<<24))
#define X3_SIG_HEADER X3_SIG('z','O','m','G')
#define X3_SIG_ITEM   X3_SIG('i','T','e','M')

typedef struct _X3_InputItem {
    const char * input_text;
    unsigned     uid;
    void (*write_func)(struct _X3_InputItem *, FILE *);
    void (*post_func) (struct _X3_InputItem *, X3_Result *);
} X3_InputItem;

typedef struct {
    X3_InputItem ii;
    const char   *type_name;
    X3_TgtSize   int_type_size;
    int          int_type_is_signed;
} X3_InputIntType;

typedef struct {
    X3_InputIntType iit;
    X3_Int          int_value;
} X3_InputIntValue;

X3_InputItem* x3_input_item_new();
X3_InputItem* x3_input_item_new_text(const char * text);

typedef X3_Array X3_InputGroup;
X3_InputGroup * x3_input_group_new();

typedef struct {
    int error_count;

    X3_Array gcc_command;
    X3_Array input_groups;
    X3_InputGroup * current_group;
    X3_InputGroup * header_group;

    X3_Array optional_stack;

    FILE * output_stream;
    FILE * log_stream;
    const char * log_path;

    X3_TargetInfo target_info;
    X3_Result *result;

    int print_gcc_command;

    const char *temp_base;
} X3_Global;

X3_Global* x3_global_create();
void x3_global_init(X3_Global * global);
void x3_global_set_temp_fn_base(const char * temp_base);
char*x3_global_get_temp_fn(const char * suffix);
int  x3_global_keep_temps();

void x3_push_optional(int optional);
void x3_pop_optional();
int  x3_is_optional();

void x3_log(const char *fmt, ...);
void x3_error(const char *fmt, ...);
int x3_unsupported(const char * detail);
int x3_interp_error(const char * detail);

int x3_gcc_execute(X3_Array extra_args);
int x3_gcc_execute_l(const char * extra, ...);
int x3_compile_test(void);
int x3_compile(void);

typedef struct {
    const char * name;
    const char * args;
    const char * def;
} X3_Macro;

int x3_compile_get_macros(X3_Array * out_macros);

void x3_gi_push_standard();

void x3_gi_write_to_FILE(FILE *f);

void x3_gi_push_header(const char *hdr);
void x3_gi_push_headers_l(const char * hdr, ...);
void x3_gi_pop_header();
void x3_gi_pop_headers(int count);

void x3_gi_push_group();
void x3_gi_pop_group();

void x3_gi_push_input_item(X3_InputItem *ii);
void x3_gi_pop_items(int count);
X3_InputItem* x3_gi_push_text(const char *text);
X3_InputItem* x3_gi_push_subst_text(const char *text);
X3_InputIntValue* x3_gi_push_int_value(const char *expr);

void x3_gi_push_default_enum(const char * default_val, ...);

void x3_gi_write_start_item(FILE *f, unsigned uid);
void x3_gi_write_start_item_data(FILE *f, unsigned uid);
void x3_gi_write_finish_item(FILE *f);


int x3_query_target_info(X3_TargetInfo *ti);

typedef enum {
    X3_TYPE_KIND_ERROR,
    X3_TYPE_KIND_VOID,
    X3_TYPE_KIND_INT,
    X3_TYPE_KIND_POINTER,
    X3_TYPE_KIND_ARRAY,
    X3_TYPE_KIND_FUNC,
    X3_TYPE_KIND_NAME,
    X3_TYPE_KIND_RECORD
} X3_TypeKind;

typedef struct _X3_Type {
    int         ok;
    X3_TypeKind kind;
    const char *c_name;
    const char *out_name;
    X3_TgtSize  type_size;
    unsigned    type_alignment;
    int         type_is_signed;
    struct _X3_Type *next;
    //X3_TgtSize  int_param;
    union {
        struct _X3_Struct * struct_def;
        X3_TgtSize array_length;
    } u;
    int         flags;
} X3_Type;

X3_Type * x3_type_new(void);
void x3_type_init(X3_Type * type);
void x3_type_out(X3_Type * type);

int x3_query_int_type(const char *name, X3_Type * type);
int x3_query_type(const char *name, X3_Type * type);
int x3_query_int_value(const char *expr, const char *type_name,
    X3_Int * out_value, X3_Type * out_type);
int x3_query_struct_field(const char * struct_name, const char * field_name);
int x3_query_cpp_if(const char * expr);

extern const char * X3_STRUCT_FALLBACK;

void x3_out_int_type(const char* name);
void x3_out_char_type(const char* name);
void x3_out_type(const char* name);
void x3_out_type_ex(const char* c_name, const char * out_name, const char * fallback);
void x3_out_int_value(const char* expr);
void x3_out_int_value_ex(const char *name, const char* expr, const char *in_type,
    const char *out_type, int explicit_cast, const char * fallback);
void x3_out_enum(const char *name, const char *type, ...);
void x3_out_text(const char *text);

enum {
    X3_FLAG_OPTIONAL = 0x01,
    X3_FLAG_INT_TYPE = 0x02,
    X3_FLAG_USE_CHAR = 0x04,
    X3_FLAG_USE_BYTE = 0x08
};

enum {
    X3_RECORD_UNKNOWN,
    X3_RECORD_STRUCT,
    X3_RECORD_UNION,
};

typedef struct {
    int ok;
    int flags;
    const char * c_name;
    const char * out_name;
    X3_TgtSize field_offset;
    X3_Type    field_type;
    const char * init_expr;
} X3_Field;

typedef struct _X3_Struct {
    int       finished;
    int       record_kind;
    int       fallback;
    X3_Type   struct_type;
    X3_Array fields; // of X3_Field *
    X3_Array extra; // of char *
} X3_Struct;

X3_Struct * x3_struct_new();
X3_Struct * x3_struct_new_c_name(const char * c_name);
void x3_struct_delete(X3_Struct * s);
X3_Field * x3_struct_new_field(X3_Struct * s);
X3_Field * x3_struct_add_field(X3_Struct * s, X3_Field * field);
void x3_struct_add_fields(X3_Struct * s, ...);
void x3_struct_add_extra(X3_Struct * s, const char * text);
void x3_struct_remove_bad(X3_Struct * s);
void x3_struct_finish(X3_Struct * s);
void x3_struct_out(X3_Struct * s);

void x3_out_struct(const char * c_name, const char *out_name, ...);
void x3_out_struct_ex(const char * c_name, const char *out_name, int fallback, ...);

#endif
