#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "flags.h"
#include "convert.h"
#include "cpplib.h"

add_cpp_dir_path (cpp_dir *p, int chain)
{
    /* nothing */
}

void
add_path (char *path, int chain, int cxx_aware, bool user_supplied_p)
{
    /* nothing */
}

void
builtin_define_with_value (const char *macro, const char *expansion, int is_str)
{
    /* nothing */
}

enum { unused } c_language;

enum cpp_ttype
c_lex (tree *value)
{
#if D_GCC_VER >= 40
    gcc_assert(0);
#else
    return 0;
#endif
}
