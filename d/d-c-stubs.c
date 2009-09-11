#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "flags.h"
#include "convert.h"
#include "cpplib.h"

/* Various functions referenced by $(C_TARGET_OBJS), $(CXX_TARGET_OBJS).
   We need to include these objects to get target-specific version
   symbols.  The objects also contain functions not needed by GDC and
   link against function in the C front end.  These definitions
   satisfy the link requirements, but should never be executed. */

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

enum cpp_ttype
pragma_lex (tree *value)
{
    gcc_assert(0);
}

tree
lookup_name (tree name)
{
    gcc_assert(0);
    return NULL_TREE;
}

