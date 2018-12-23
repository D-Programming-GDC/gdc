#!/bin/sh
d_gdcsrc=$(realpath $(dirname $0))/gdc
d_gccsrc=gcc
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
            echo "error: '$arg' is not a valid option"
            exit 1 ;;
    esac
done


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
test -d "$d_gccsrc/gcc/d" && rm -r "$d_gccsrc/gcc/d"
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
if test -e "$d_test/gdc.dg" -o -e "$d_test/gdc.test" -o -e "$d_test/lib/gdc.exp" -o -e "$d_test/lib/gdc-dg.exp"; then
    echo "error: cannot update gcc source, please remove D testsuite sources by hand."
    exit 1
fi

# 2. Copy sources
cp -avT "$d_gdcsrc/gcc/d" "$d_gccsrc/gcc/d" || exit 1
cp -avT "$d_gdcsrc/gcc/config" "$d_gccsrc/gcc/config" || exit 1
cp -avT "$d_gdcsrc/libphobos" "$d_gccsrc/libphobos" || exit 1
cp -avT "$d_gdcsrc/gcc/testsuite/gdc.dg" "$d_gccsrc/gcc/testsuite/gdc.dg" || exit 1
cp -avT "$d_gdcsrc/gcc/testsuite/gdc.test" "$d_gccsrc/gcc/testsuite/gdc.test" || exit 1
cp -avT "$d_gdcsrc/gcc/testsuite/lib/gdc.exp" "$d_gccsrc/gcc/testsuite/lib/gdc.exp" || exit 1
cp -avT "$d_gdcsrc/gcc/testsuite/lib/gdc-dg.exp" "$d_gccsrc/gcc/testsuite/lib/gdc-dg.exp" || exit 1


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
    echo "../patches" > .pc/.quilt_patches && \
    echo "series" > .pc/.quilt_series || exit 1

  if test -f "../patches/gcc.patch"; then
    echo gcc.patch >> ../patches/series
  fi
  if test -f "../patches/ddmd.patch"; then
    echo ddmd.patch >> ../patches/series
  fi

  quilt upgrade && \
    quilt push -a && \
    cd $top || exit 1
else
  cd $d_gccsrc
  if test -f "../patches/gcc.patch"; then
    patch -p1 -i ../patches/gcc.patch || exit 1
  fi
  if test -f "../patches/ddmd.patch"; then
    patch -p1 -i ../patches/ddmd.patch || exit 1
  fi
  cd $top
fi

echo "GDC setup complete."
exit 0
