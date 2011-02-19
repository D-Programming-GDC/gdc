/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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

void add_cpp_dir_path (cpp_dir *p, int chain);
void add_path (char *path, int chain, int cxx_aware, bool user_supplied_p);
void builtin_define_with_value (const char *macro, const char *expansion, int is_str);
tree default_conversion (tree exp);
tree build_binary_op (location_t location, enum tree_code code, tree orig_op0, tree orig_op1, int convert_p);
tree build_unary_op (location_t location, enum tree_code code, tree xarg, int flag);
tree build_indirect_ref (location_t loc, tree ptr, const char *errorstring);
enum cpp_ttype c_lex (tree *value);
enum cpp_ttype pragma_lex (tree *value);
tree lookup_name (tree name);


void
add_cpp_dir_path (cpp_dir *p ATTRIBUTE_UNUSED, int chain ATTRIBUTE_UNUSED)
{
    /* nothing */
}

void
add_path (char *path ATTRIBUTE_UNUSED, int chain ATTRIBUTE_UNUSED,
          int cxx_aware ATTRIBUTE_UNUSED, bool user_supplied_p ATTRIBUTE_UNUSED)
{
    /* nothing */
}

void
builtin_define_with_value (const char *macro ATTRIBUTE_UNUSED, const char *expansion ATTRIBUTE_UNUSED,
                           int is_str ATTRIBUTE_UNUSED)
{
    /* nothing */
}

tree
default_conversion (tree exp)
{
    return exp;
}

tree
build_binary_op (location_t location ATTRIBUTE_UNUSED, enum tree_code code ATTRIBUTE_UNUSED,
                 tree orig_op0 ATTRIBUTE_UNUSED, tree orig_op1 ATTRIBUTE_UNUSED, int convert_p ATTRIBUTE_UNUSED)
{
    return NULL_TREE;
}

tree
build_unary_op (location_t location ATTRIBUTE_UNUSED, enum tree_code code ATTRIBUTE_UNUSED,
                tree xarg ATTRIBUTE_UNUSED, int flag ATTRIBUTE_UNUSED)
{
    return NULL_TREE;
}

tree
build_indirect_ref (location_t loc ATTRIBUTE_UNUSED, tree ptr ATTRIBUTE_UNUSED,
                    const char *errorstring ATTRIBUTE_UNUSED)
{
    return NULL_TREE;
}

enum c_language_kind { unused } c_language;

enum cpp_ttype
c_lex (tree *value ATTRIBUTE_UNUSED)
{
    return (enum cpp_ttype)0;
}

enum cpp_ttype
pragma_lex (tree *value ATTRIBUTE_UNUSED)
{
    return (enum cpp_ttype)0;
}

tree
lookup_name (tree name ATTRIBUTE_UNUSED)
{
    return NULL_TREE;
}

