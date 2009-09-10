#!/bin/sh

# -1. Make sure we are in the top-level GCC soure directory
if test -d gcc && test -d gcc/d && test -f gcc/d/setup-gcc.sh; then
    :
else
    echo "This script must be run from the top-level GCC source directory."
    exit 1
fi
top=`pwd`

# 0. Find out what GCC version this is
if grep version_string gcc/version.c | grep -q '"3.4'; then
    gcc_ver=3.4
elif grep version_string gcc/version.c | grep -q '"4.0'; then
    gcc_ver=4.0
elif grep -q '^4\.1\.' gcc/BASE-VER; then
    gcc_ver=4.1
fi

# 0.1. Find out if this is Apple's GCC
if grep version_string gcc/version.c | grep -qF '(Apple Computer, Inc.'; then
    gcc_apple=apple-
fi

# 0.2. Determine if this version of GCC is supported
gcc_patch_fn=d/patch-${gcc_apple}gcc-${gcc_ver}.x
if test ! -f gcc/"$gcc_patch_fn"; then
    echo "This version of GCC is not supported."
    exit 1
fi

# 0.5. Find out what GDC version this is
gdc_ver=`cat gcc/d/gdc-version`

# 1. Create a directory of links to the Phobos sources in the top-level
# directory.
mkdir libphobos && \
    cd libphobos && \
    ../symlink-tree ../gcc/d/phobos > /dev/null && \
    cd "$top"

# 2. Patch the top-level directory
#
# If the patch for the top-level Makefile.in doesn't take, you can regenerate
# it with:
#   autogen -T Makefile.tpl Makefile.def
#
# You will need the autogen package to do this. (http://autogen.sf.net/)
patch -p1 < gcc/d/patch-toplev-${gcc_ver}.x || exit 1

if test -n "$gcc_apple"; then
    patch -l -p1 < "gcc/d/patch-build_gcc-$gcc_ver" || exit 1
fi

# 3. Patch the gcc subdirectory
cd gcc || exit 1
patch -p1 < "$gcc_patch_fn" || exit 1

if test "$gcc_ver" = 4.1; then
    echo `cat DEV-PHASE` ' ' "($gdc_ver)" > DEV-PHASE
else
    sed -e 's/ *(gdc.*using dmd [0-9\.]*)//' \
	-e 's/\(, *\)gdc.*using dmd [0-9\.]*/\1/' \
	-e 's/\(version_string[^"]*"[^"]*\)"/\1 ('"$gdc_ver"')"/' \
	version.c > version.c.tmp && mv -f version.c.tmp version.c
fi

# 4. Maybe apply Darwin patches
if test -z "$gcc_apple" && test "`uname`" = Darwin; then
    if test -f d/patch-gcc-darwin-eh-${gcc_ver}.x; then
	patch -p1 < d/patch-gcc-darwin-eh-${gcc_ver}.x || exit 1
    fi
fi

echo
echo "GDC setup complete."
exit 0
