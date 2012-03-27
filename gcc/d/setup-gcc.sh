#!/bin/sh
# -1. Make sure we are in the top-level GCC soure directory
if test -d gcc && test -d gcc/d && test -f gcc/d/setup-gcc.sh; then
    :
else
    echo "This script must be run from the top-level GCC source directory."
    exit 1
fi
top=`pwd`

# D 2.0 is the default
d_lang_version=2

# 0. Find out what GCC version this is
if grep version_string gcc/version.c | grep -q '"3.4'; then
    gcc_ver=3.4
elif grep version_string gcc/version.c | grep -q '"4.0'; then
    gcc_ver=4.0
elif grep -q '^4\.1\.' gcc/BASE-VER; then
    gcc_ver=4.1
elif grep -q '^4\.2\.' gcc/BASE-VER; then
    gcc_ver=4.2
elif grep -q '^4\.3\.' gcc/BASE-VER; then
    gcc_ver=4.3
elif grep -q '^4\.4\.' gcc/BASE-VER; then
    gcc_ver=4.4
elif grep -q '^4\.5\.' gcc/BASE-VER; then
    gcc_ver=4.5
elif grep -q '^4\.6\.' gcc/BASE-VER; then
    gcc_ver=4.6
elif grep -q '^4\.7\.' gcc/BASE-VER; then
    gcc_ver=4.7
fi

gcc_patch_key=${gcc_ver}.x

# 1. Determine if this version of GCC is supported
gcc_patch_fn=d/patches/patch-gcc-$gcc_patch_key
if test ! -f gcc/"$gcc_patch_fn"; then
    echo "This version of GCC ($gcc_ver) is not supported."
    exit 1
fi

# 2. Patch the top-level directory
#
# If the patch for the top-level Makefile.in doesn't take, you can regenerate
# it with:
#   autogen -T Makefile.tpl Makefile.def
#
# You will need the autogen package to do this. (http://autogen.sf.net/)
patch -p1 < gcc/d/patches/patch-toplev-$gcc_patch_key || exit 1


# 3. Patch the gcc subdirectory
cd gcc || exit 1
patch -p1 < "$gcc_patch_fn" || exit 1


echo
echo "Building D language version $d_lang_version."
echo
echo "GDC setup complete."
exit 0
