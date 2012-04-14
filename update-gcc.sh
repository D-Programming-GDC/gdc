#!/bin/sh
# -1. Make sure we know where the top-level GCC source directory is
if test -d gcc && test -d gcc/d && test -f gcc/d/setup-gcc.sh; then
    if test $# -gt 0; then
        :
    else
        echo "Usage: $0 [OPTION] PATH"
        exit 1
    fi
else
    echo "This script must be run from the top-level D source directory."
    exit 1
fi

d_gccsrc=
d_setup_gcc=0
top=`pwd`

# Read command line arguments
for arg in "$@"; do
    case "$arg" in
        --setup) d_setup_gcc=1 ;;
        *)
            if test -z "$d_gccsrc" && test -d "$arg" && test -d "$arg/gcc"; then
                d_gccsrc="$arg";
            else
                echo "error: invalid option '$arg'"
                exit 1
            fi ;;
    esac
done


# 0. Remove d sources from d_gccsrc if already exist
test -h "$d_gccsrc/gcc/d" && rm "$d_gccsrc/gcc/d"
test -d "$d_gccsrc/libphobos" && rm -r "$d_gccsrc/libphobos"
if test -e "$d_gccsrc/gcc/d" -o -e "$d_gccsrc/libphobos"; then
    echo "error: cannot update gcc source, please remove D sources by hand."
    exit 1
fi

d_test=$d_gccsrc/gcc/testsuite
# remove testsuite sources
test -d "$d_test/gdc.test" && rm -r "$d_test/gdc.test"
test -e "$d_test/lib/gdc.exp" && rm "$d_test/lib/gdc.exp"
test -e "$d_test/lib/gdc-dg.exp" && rm "$d_test/lib/gdc-dg.exp"
if test -e "$d_test/gdc.test" -o -e "$d_test/lib/gdc.exp" -o -e "$d_test/lib/gdc-dg.exp"; then
    echo "error: cannot update gcc source, please remove D testsuite sources by hand."
    exit 1
fi


# 1. Copy sources
ln -s "$top/gcc/d" "$d_gccsrc/gcc/d"   && \
  mkdir "$d_gccsrc/libphobos"          && \
  cd "$d_gccsrc/libphobos"             && \
  ../symlink-tree "$top/libphobos"     && \
  cd "../gcc/testsuite"                && \
  ../../symlink-tree "$top/gcc/testsuite" && \
  cd $top


# 2. Maybe run setup-gcc.sh
if test $d_setup_gcc -eq 1; then
    cd $d_gccsrc             && \
        ./gcc/d/setup-gcc.sh && \
        cd $top
fi

echo "GDC update complete."
exit 0
