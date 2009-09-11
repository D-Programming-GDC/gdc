#!/bin/sh

# Usage: simple.sh <pkg name>
#   Creates gdc-<pkg name>.tar.gz, etc.
#

# Special package types:
#  --skyos: different package prefix directories, metafile, filesystem workarounds
#  --macos: don't include shared libgcc

# set -x

make_skyos_package=
make_macos_package=
keep_gcov=
keep_gcc=
no_cleanup=
have_install_root=
MAKE=${MAKE:-make}

while test "$#" -gt 0; do
    case "$1" in
        --with-install-root=*) have_install_root=${1##--with-install-root=} ;;
	--macos) make_macos_package=yes ;;
	--skyos) make_skyos_package=yes ;;
	--keepgcc) keep_gcc=yes ;;
	--keepgcov) keep_gcov=yes ;;
	--nocleanup) no_cleanup=yes ;;
	--results-file=*) results_file=${1##--results-file=} ;;
	-*) echo "error: invalid option $1"
	    exit 1
	    ;;
	*)
	    if test -z "$pkg_name"; then
		pkg_name=gdc-$1
	    else
		echo "error: extra argument $1"
		exit 1
	    fi
	    ;;
    esac
    shift
done

if test ! -r gcc/d/pkgvars; then
    echo "This script most be run from the top-level GCC build directory."
    exit 1
fi

build_dir=`pwd`

# Grab some configuration info
. gcc/d/pkgvars

# Find the correct 'strip' program for the host    
host_strip=strip
target_strip=strip

if test "$target" != "$build"; then
    if $target_alias-strip -h; then
	target_strip=$target_alias-strip
    else
	echo "warning: probably have the wrong strip for $target"
    fi
fi
if test "$host" != "$build"; then
    if test "$host" = "$target"; then
	host_strip=$target_strip
    else
	echo "warning: probably have the wrong strip for $host"
    fi
fi

if test -n "$have_install_root"; then
    inst_root=$have_install_root
else
    inst_root=/tmp/gdcinstroot$$
    mkdir -p "$inst_root" || exit 1
    $MAKE install "DESTDIR=$inst_root" > /dev/null || exit 1
fi

# $srcdir can be relative to <build>/gcc
cd gcc

gdc_version=`sed -e 's/gdc \(.*\), using.*/\1/' "$srcdir/d/gdc-version"`

if test -z "$pkg_name"; then
    pkg_name="gdc-$gdc_version-$target_alias"
fi

# If this is a native compiler, try to build rdmd
# (This should be moved into the GCC build)
if test "$build" = "$host" && test "$host" = "$target"; then
    if test ! -f "$inst_root/$prefix/bin/rdmd$exeext"; then
	"$inst_root/$prefix/bin/gdc" -o "$inst_root/$prefix/bin/rdmd$exeext" \
	    $srcdir/d/rdmd.d || exit 1
    fi
    if test -d "$inst_root/$prefix/share/man/man1"; then
	# Note: No ".1" extension -- assumes MacOS Universal build
	cp $srcdir/d/rdmd.1 "$inst_root/$prefix/share/man/man1/rdmd" || exit 1
    else
	cp $srcdir/d/rdmd.1 "$inst_root/$prefix/man/man1" || exit 1
    fi
fi

#pkg_root=/tmp/$pkg_name
pkg_root=/tmp/gdc

file_list=/tmp/gdcpkglist$$
rm -f "$file_list" && touch "$file_list" || exit 1

cd "$inst_root/$prefix" || exit 1

# Use lib*/... to pick up multilib.  Assume that picking up libexec
# does not cause problems because of the rest of the search pattern.

if test -n "$keep_gcc"; then
    find . > "$file_list"
else
    ls    bin/*gdc*  >> "$file_list"
    ls    bin/*gdmd* >> "$file_list"
    ls    bin/*rdmd* >> "$file_list"
    ls -d include/d  >> "$file_list"
    ls -d include/d2 >> "$file_list"
    find  lib*/gcc -name '*.o' -o -name 'libgcc*.a' >> "$file_list"
    if test -z "$make_macos_package"; then
	ls    lib*/libgcc* >> "$file_list"
    fi
    ls    lib*/libgphobos* >> "$file_list"
    find  libexec -name '*cc1d*' >> "$file_list"
    find  libexec -name '*collect2*' >> "$file_list"
    ls    man/man1/*gdc* >> "$file_list"
    ls    man/man1/*gdmd* >> "$file_list"
    ls    man/man1/*rdmd* >> "$file_list"
    ls    share/man/man1/*gdc* >> "$file_list"
    ls    share/man/man1/*gdmd* >> "$file_list"
    ls    share/man/man1/*rdmd* >> "$file_list"

    if test -n "$keep_gcov"; then
	ls bin/*gcov* >> "$file_list"
	# Don't need lib* here -- it's all buried under lib/gcc/<target alias>
	find lib -name 'libgcov*' >> "$file_list"
	ls man/man1/*gcov* >> "$file_list"
	ls share/man/man1/*gcov* >> "$file_list"
    fi
fi

if test -n "$make_skyos_package"; then
    tgt=$pkg_root/$prefix
else
    tgt=$pkg_root
fi

mkdir -p "$tgt"

# simulate: rsync -a --files-from="$file_list" -rH . "$tgt"
while true; do
    read src || break
    if test -z "$src"; then continue; fi
    tar cf - "$src" | tar xf - -C "$tgt"
done < "$file_list"


# Apple build installs gdc-4.0, but not 'gdc'.  For a stand-alone package,
# there should be a unprefixed 'gdc' and 'gdmd'.  Cross compilers should
# *not* have unprefixed in <prefix>/bin.
if test "$host" = "$target"; then
    cd "$tgt/bin" || exit 1
    for prg in gdc gdmd; do
	if test "$prg" = gdmd; then
	    ext=''
	else
	    ext=$exeext
	fi
	if ! test -f $prg$ext; then
	    # ln -s $prg-* $prg is successful even if there is no match..
	    if ls $prg-*; then
		ln -s $prg-* $prg$ext || exit 1
	    elif ls *-$prg$ext; then
		ln -s *-$prg$ext $prg$ext || exit 1
	    else
		echo "error: no $prg-like executable in bin"
		exit 1
	    fi
	fi
    done
fi

cd "$tgt" || exit 1

# This is missing some files for MacOS universal, but the Apple build
# script has already stripped them.
$host_strip bin/* > /dev/null
$host_strip libexec/gcc/$target_alias/$gcc_version/* > /dev/null
$host_strip $target_alias/bin/* > /dev/null
# TODO: these options are specific to binutils?
$target_strip -S -X lib*/lib*.*
$target_strip -S -X lib/gcc/$target_alias/$gcc_version/* > /dev/null

if test "$build" != "$host"; then
    echo "note: removing specs as it is mostly like wrong"
    #rm -f find . -name specs
    rm lib/gcc/$target_alias/$gcc_version/specs
fi

cd "$build_dir/gcc"

doc_dir="$tgt/share/doc/gdc"
mkdir -p "$doc_dir" || exit 1
cp -p "$srcdir/d/README" "$doc_dir/README.GDC" || exit 1
cp -p "$srcdir/d/GDC.html" "$doc_dir" || exit 1

if test -z "$results_file"; then
    results_file=/tmp/gdc-packages-list
fi
rm -f "$results_file" && \
    touch "$results_file" \
    || exit 1

file_base="/tmp/$pkg_name"
tar_file="$file_base.tar"

if test -n "$make_skyos_package"; then
    # kill hard link variants
    rm -f "$pkg_root/$prefix"/bin/*-*
    
    cd "$build_dir/gcc"
    
    skyos_date=`date "+%B %e, %Y"`
    sed -e "s/@gdc_version@/$gdc_version/" -e "s/@skyos_date@/$skyos_date/" \
	"$srcdir/d/package/install.sif" > "$inst_root/boot/install.sif"

    cd "$pkg_root/boot" || exit 1
    tar cf "$tar_file" * || exit 1
else
    cd `dirname "$pkg_root"` || exit 1
    tar cf "$tar_file" `basename "$pkg_root"` || exit 1
fi
echo "$tar_file" >> "$results_file"

gzip -c9 "$tar_file" > "$tar_file.gz" || exit 1
echo "$tar_file.gz" >> "$results_file" || exit 1

bzip2 -c9 "$tar_file" > "$tar_file.bz2" || exit 1
echo "$tar_file.bz2" >> "$results_file" || exit 1

if test -n "$make_skyos_package"; then
    cp "$tar_file.gz" "$file_base.pkg" || exit 1
    echo "$file_base.pkg" >> "$results_file" || exit 1
fi

if test -z "$no_cleanup"; then
    rm -f "$file_list"
    rm -rf "$pkg_root"
    if test -z "$have_install_root"; then
	rm -rf "$inst_root"
    fi
fi
