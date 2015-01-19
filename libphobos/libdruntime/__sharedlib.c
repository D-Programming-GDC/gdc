#include <stddef.h>

typedef struct
{
    size_t _version;                                  // currently 1
    void** _slot;                                     // can be used to store runtime data
    void** _minfo_beg, **_minfo_end;       // array of modules in this object file
    void* _deh_beg, *_deh_end; // array of exception handling data
} CompilerDSOData;

void _d_dso_registry(CompilerDSOData* data);
extern void* __start_dminfo;
extern void* __stop_dminfo;

static void* slot;

__attribute__ ((constructor, destructor, used)) static void _dso_start()
{
    CompilerDSOData data;
    data._version = 1;
    data._slot = &slot;
    data._minfo_beg = &__start_dminfo;
    data._minfo_end = &__stop_dminfo;
    data._deh_beg = NULL;
    data._deh_end = NULL;
    _d_dso_registry(&data);
}
