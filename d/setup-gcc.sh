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
if test -d gcc/d/dmd2; then
    d_lang_version=2
elif test -d gcc/d/dmd; then
    d_lang_version=1
fi

# True if we just want to rebuild the phobos directory
d_update_phobos=0

# Read command line arguments
for arg in "$@"; do
    case "$arg" in
        --d-language-version=*) d_lang_version=${arg##--d-language-version=} ;;
        --update) d_update_phobos=1 ;;
        -v1) d_lang_version=1 ;;
        -v2) d_lang_version=2 ;;
        *)
            echo "Usage: $0 [OPTION]"
            echo "error: invalid option '$arg'"
            exit 1
            ;;
    esac
done

if test $d_lang_version -ne 1; then
    d_subdir_sfx=$d_lang_version
fi

if test ! -d gcc/d/dmd$d_subdir_sfx; then
    echo "error: This distribution does not support D version $d_lang_version"
    exit 1
fi


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

# 0.2. Determine if this version of GCC is supported
gcc_patch_fn=d/patches/patch-gcc-$gcc_patch_key
if test ! -f gcc/"$gcc_patch_fn"; then
    echo "This version of GCC ($gcc_ver) is not supported."
    exit 1
fi

# 0.5. Find out what GDC and DMD version this is
gdc_ver=`cat gcc/d/gdc-version`
if command -v hg > /dev/null; then
    hg_branch=`hg  --cwd gcc/d identify -b 2> /dev/null`
    hg_revision=`hg --cwd gcc/d identify -n 2> /dev/null | sed -e 's/^\(.*\)+$/\1/'`
    hg_id=`hg --cwd gcc/d identify -i 2> /dev/null`
    hg --cwd gcc/d identify 2> /dev/null
    if [ "$?" -eq "0" ]; then
        # is branch useful?
        #gdc_ver="$gdc_ver - r$hg_revision:$hg_id($hg_branch)"
        gdc_ver="$gdc_ver - r$hg_revision:$hg_id"
    fi
fi

dmd_ver=`grep 'version = "v' gcc/d/dmd$d_subdir_sfx/mars.c | sed -e 's/^.*"v\(.*\)".*$/\1/'` || exit 1
gdc_ver_msg="gdc $gdc_ver, using dmd $dmd_ver"

# 0.7 Set the D language version.  Note: This creates a file in the D
# source directory.  If the file is a link, remove it first.
rm -f gcc/d/d-make-include
echo "D_LANGUAGE_VERSION=$d_lang_version" > gcc/d/d-make-include

# 0.9 Remove the Phobos directory if it already exists
if test -d libphobos; then
    rm -rf libphobos
fi


# 1. Create a directory of links to the Phobos sources in the top-level
# directory.
mkdir libphobos && \
    cd libphobos && \
    ../symlink-tree ../gcc/d/phobos$d_subdir_sfx > /dev/null && \
    cd "$top" || exit 1

# 1.1 If they are building D2, then create links to the druntime sources in
# the top level directory
if test "$d_lang_version" = 2; then
    cd libphobos && \
    ../symlink-tree ../gcc/d/druntime > /dev/null && \
    cd "$top" || exit 1
fi

# 1.2 Create a directory of links to the Zlib sources in the libphobos
# directory.
mkdir libphobos/zlib && \
    cd libphobos/zlib && \
    ../../symlink-tree ../../gcc/d/zlib > /dev/null && \
    cd "$top" || exit 1

# 1.3 Patch the gcc version string
cd gcc || exit 1
d_gcc_ver=`echo $gcc_ver | sed -e 's/\.//g'`

if test "$d_gcc_ver" -ge 41; then
    sed -i 's/gdc [a-z0-9. :]*, using dmd [0-9. ]*//g' DEV-PHASE
    cur_DEV_PHASE=`cat DEV-PHASE`
    if test -z "$cur_DEV_PHASE"; then
        echo "$gdc_ver_msg" > DEV-PHASE
    else
        echo "$cur_DEV_PHASE $gdc_ver_msg" > DEV-PHASE
    fi
else
    sed -e 's/ *(gdc.*using dmd [0-9\.]*)//' \
        -e 's/\(, *\)gdc.*using dmd [0-9\.]*/\1/' \
        -e 's/\(version_string[^"]*"[^"]*\)"/\1 ('"$gdc_ver_msg"')"/' \
        version.c > version.c.tmp && mv -f version.c.tmp version.c
fi
cd "$top" || exit 1

# 1.4 Exit early if we are just updating d-make-include and the
# directory of links to the Phobos sources.
if test "$d_update_phobos" = 1; then
    echo
    echo "Updated Phobos and Makefile for D language version $d_lang_version."
    echo
    echo "GDC setup complete."
    exit 0
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
