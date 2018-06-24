#!/bin/sh
d_gdcsrc=$(realpath $(dirname $0))
d_gccsrc=
d_update_gcc=0
d_quilt_patch=0
top=$(pwd)

# -1. Make sure we know where the top-level GCC source directory is
if test -d "$d_gdcsrc/gcc" && test -d "$d_gdcsrc/gcc/d" && test -d "$d_gdcsrc/gcc/testsuite/gdc.test"; then
    if test $# -eq 0; then
        echo "Usage: $0 [OPTION] PATH"
        exit 1
    fi
else
    echo "This script must be run from the top-level D source directory."
    exit 1
fi

# Read command line arguments
for arg in "$@"; do
    case "$arg" in
        --quilt) d_quilt_patch=1 ;;
        --update) d_update_gcc=1 ;;
        *)
            if test -z "$d_gccsrc" && test -d "$arg" && test -d "$arg/gcc"; then
                d_gccsrc="$arg";
            else
                echo "error: '$arg' is not a valid option or path"
                exit 1
            fi ;;
    esac
done


# 0. Find out what GCC version this is
if grep version_string $d_gccsrc/gcc/version.c | grep -q '"3.4'; then
    gcc_ver=3.4
elif grep version_string $d_gccsrc/gcc/version.c | grep -q '"4.0'; then
    gcc_ver=4.0
elif grep -q -E '^4\.[1-9]+([^0-9]|$)' $d_gccsrc/gcc/BASE-VER; then
    gcc_ver=$(grep -oh -E '^4\.[0-9]+|$' $d_gccsrc/gcc/BASE-VER)
elif grep -q -E '^5\.[0-9]+([^0-9]|$)' $d_gccsrc/gcc/BASE-VER; then
    gcc_ver=5
elif grep -q -E '^6\.[0-9]+([^0-9]|$)' $d_gccsrc/gcc/BASE-VER; then
    gcc_ver=6
elif grep -q -E '^7\.[0-9]+([^0-9]|$)' $d_gccsrc/gcc/BASE-VER; then
    gcc_ver=7
elif grep -q -E '^8\.[0-9]+([^0-9]|$)' $d_gccsrc/gcc/BASE-VER; then
    gcc_ver=8
elif grep -q -E '^9\.[0-9]+([^0-9]|$)' $d_gccsrc/gcc/BASE-VER; then
    gcc_ver=9
else
    echo "cannot get gcc version"
    exit 1
fi
echo "found gcc version $gcc_ver"
gcc_patch_key=${gcc_ver}.patch

# Determine if this version of GCC is supported
if test ! -f "$d_gdcsrc/gcc/d/patches/patch-gcc-$gcc_patch_key"; then
    echo "This version of GCC ($gcc_ver) is not supported."
    exit 1
fi

# 1. Clean the GCC tree if any changes were already made to it.

# Remove any applied patches, if managed by quilt.
if test $d_quilt_patch -eq 1 && test -d "$d_gccsrc/.pc"; then
  cd $d_gccsrc && \
    quilt pop -a && \
    rm -r ".pc"
  cd $top

  if test -d "$d_gccsrc/.pc"; then
    echo "error: cannot update gcc source, please remove applied quilt patches by hand."
    exit 1
  fi
fi

# Remove D sources from d_gccsrc if already exist
test -h "$d_gccsrc/gcc/d" && rm "$d_gccsrc/gcc/d"
test -d "$d_gccsrc/libphobos" && rm -r "$d_gccsrc/libphobos"
if test -e "$d_gccsrc/gcc/d" -o -e "$d_gccsrc/libphobos"; then
    echo "error: cannot update gcc source, please remove D sources by hand."
    exit 1
fi

# Remove testsuite sources
d_test=$d_gccsrc/gcc/testsuite
test -d "$d_test/gdc.dg" && rm -r "$d_test/gdc.dg"
test -d "$d_test/gdc.test" && rm -r "$d_test/gdc.test"
test -e "$d_test/lib/gdc.exp" && rm "$d_test/lib/gdc.exp"
test -e "$d_test/lib/gdc-dg.exp" && rm "$d_test/lib/gdc-dg.exp"
if test -e "$d_test/gdc.test" -o -e "$d_test/lib/gdc.exp" -o -e "$d_test/lib/gdc-dg.exp"; then
    echo "error: cannot update gcc source, please remove D testsuite sources by hand."
    exit 1
fi

# 2. Copy sources
ln -s "$d_gdcsrc/gcc/d" "$d_gccsrc/gcc/d"   && \
  mkdir "$d_gccsrc/libphobos"               && \
  cd "$d_gccsrc/libphobos"                  && \
  ../symlink-tree "$d_gdcsrc/libphobos"     && \
  cd "../gcc/testsuite"                     && \
  ../../symlink-tree "$d_gdcsrc/gcc/testsuite" && \
  cd $top


if test $d_update_gcc -eq 1; then
  echo "GDC update complete"
  exit 0
fi


# 3. Patch the GCC tree, both toplevel and subdirectories.
#
# If the patch for the top-level Makefile.in doesn't take, you can regenerate
# it with:
#   autogen -T Makefile.tpl Makefile.def
#
# You will need the autogen package to do this. (http://autogen.sf.net/)
#
# If managing the patch series with quilt, then will also initialize metadata.
# Existing patches can be refreshed using:
#   quilt refresh -p ab --no-index --sort
if test $d_quilt_patch -eq 1; then
  cd $d_gccsrc && \
    mkdir -p .pc && \
    echo "gcc/d/patches" > .pc/.quilt_patches && \
    echo "series" > .pc/.quilt_series || exit 1

  cat > gcc/d/patches/series << EOF
patch-toplev-${gcc_patch_key}
patch-toplev-ddmd-${gcc_patch_key}
patch-gcc-${gcc_patch_key}
patch-gcc-ddmd-${gcc_patch_key}
patch-targetdm-${gcc_patch_key}
EOF

  quilt upgrade && \
    quilt push -a && \
    cd $top || exit 1
else
  cd $d_gccsrc && \
    patch -p1 -i gcc/d/patches/patch-toplev-${gcc_patch_key} && \
    patch -p1 -i gcc/d/patches/patch-toplev-ddmd-${gcc_patch_key} && \
    patch -p1 -i gcc/d/patches/patch-gcc-${gcc_patch_key} && \
    patch -p1 -i gcc/d/patches/patch-gcc-ddmd-${gcc_patch_key} && \
    patch -p1 -i gcc/d/patches/patch-targetdm-${gcc_patch_key} && \
    cd $top || exit 1
fi

echo "GDC setup complete."
exit 0
