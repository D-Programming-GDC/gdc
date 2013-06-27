# GDC -- D front-end for GCC
# Copyright (C) 2004 David Friedman
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

dnl Parts taken from libobjc configure.in

AC_PREREQ(2.64)
AC_INIT(libphobos, version-unused)
AC_CONFIG_SRCDIR(std/algorithm.d)
AC_CONFIG_SRCDIR(libdruntime/gcc/builtins.d)
AC_CONFIG_SRCDIR(libdruntime/gcc/attribute.d)
AC_CONFIG_HEADERS(config.h)

AC_CANONICAL_SYSTEM
target_alias=${target_alias-$target}
AC_SUBST(target_alias)

AM_INIT_AUTOMAKE([1.9.3 foreign no-dependencies])
AH_TEMPLATE(PACKAGE, [Name of package])
AH_TEMPLATE(VERSION, [Version number of package])

AM_ENABLE_MULTILIB(, ..)

# libphobos is usually a symlink to gcc/d/phobos, so libphobos/..
# is not the toplevel GCC directory.  gcc/d may also be a symlink.
# Find the correct top-level directory by removing "/libphobos"
# from $srcdir.
dnl NOTE: This assumes that AM_ENABLE_MULTILIB uses $multi_basedir
if test ! -r "$multi_basedir/config-ml.in"; then
    better_dir=`echo "$srcdir" | sed -e 's|/libphobos||'`
    if test -r "$better_dir/config-ml.in"; then
	multi_basedir=$better_dir
    fi
fi

if test "$build" != "$host"; then
  # We are being configured with some form of cross compiler.
  #GLIBCXX_IS_NATIVE=false
  GCC_NO_EXECUTABLES
  d_cross_comp=yes
else
  d_cross_comp=
fi

dnl Copied from libstdc++-v3/acinclude.m4.  Indeed, multilib will not work
dnl correctly without this.
# We're almost certainly being configured before anything else which uses
# C++, so all of our AC_PROG_* discoveries will be cached.  It's vital that
# we not cache the value of CXX that we "discover" here, because it's set
# to something unique for us and libjava.  Other target libraries need to
# find CXX for themselves.  We yank the rug out from under the normal AC_*
# process by sneakily renaming the cache variable.  This also lets us debug
# the value of "our" CXX in postmortems.
#
# We must also force CXX to /not/ be a precious variable, otherwise the
# wrong (non-multilib-adjusted) value will be used in multilibs.  This
# little trick also affects CPPFLAGS, CXXFLAGS, and LDFLAGS.  And as a side
# effect, CXXFLAGS is no longer automagically subst'd, so we have to do
# that ourselves.  Un-preciousing AC_PROG_CC also affects CC and CFLAGS.
#
# -fno-builtin must be present here so that a non-conflicting form of
# std::exit can be guessed by AC_PROG_CXX, and used in later tests.

m4_define([ac_cv_prog_CXX],[glibcxx_cv_prog_CXX])
m4_rename([_AC_ARG_VAR_PRECIOUS],[glibcxx_PRECIOUS])
m4_define([_AC_ARG_VAR_PRECIOUS],[])
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -fno-builtin"
# --- Extra hack for Phobos ---
# AC_PROG_CC, if not "cross-compiling", tries to run a simple C program.
# However, a given multilib variant may not be executable on the current
# system.  Example:  Building for x86_64 on IA-32.  This is technically
# cross-compiling, but we don't want cross-compiling directory layouts
# and we still need link tests.  Solution is to make autoconf think it
# is cross compiling only when it tests the compilers.
d_save_cross_compiling=$cross_compiling
cross_compiling=yes
AC_PROG_CC
AC_PROG_CXX
cross_compiling=$d_save_cross_compiling
CXXFLAGS="$save_CXXFLAGS"
#m4_rename([glibcxx_PRECIOUS],[_AC_ARG_VAR_PRECIOUS])
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
dnl -- End of copy from libstdc++-v3/acinclude.m4

dnl These should be inherited in the recursive make, but ensure they are
dnl defined:
test "$AR" || AR=ar
AC_SUBST(AR)
if test "$RANLIB"; then :
  AC_SUBST(RANLIB)
else
  AC_PROG_RANLIB
fi
AC_PROG_INSTALL
AC_PROG_MAKE_SET
dnl AC_PROG_LIBTOOL

AC_ARG_ENABLE(thread-lib,
  AC_HELP_STRING([--enable-thread-lib=<arg>],
                 [specify linker option for the system thread library (default: autodetect)]),
  [d_thread_lib=$enableval],[d_thread_lib=""])

AC_ARG_ENABLE(unix,
  AC_HELP_STRING([--enable-unix],
                 [enables Unix runtime (default: yes, for Unix targets)]),
  :,[enable_unix=auto])

dnl switch between gc and gcstub
AC_ARG_ENABLE(druntime-gc,
  AC_HELP_STRING([--enable-druntime-gc],
                 [enable D runtime garbage collector (default: yes)]),
  :,[enable_druntime_gc=yes])

dnl switch between system zlib and gcc's zlib
AC_ARG_WITH(system-zlib,
  AS_HELP_STRING([--with-system-zlib],
                 [use installed libz (default: no)]),
  :,[system_zlib=no])

AC_ARG_ENABLE(phobos-config-dir,
  AC_HELP_STRING([--enable-phobos-config-dir=<dir>],
		 [use source file fragments in <dir>]),
[if test -z "${enableval}"; then
  AC_MSG_ERROR([must specify a value for --enable-phobos-config-dir])
fi],
:)

if test ${multilib} = yes; then
  multilib_arg="--enable-multilib"
else
  multilib_arg=
fi

d_target_os=`echo $target_os | sed 's/^\([A-Za-z_]+\)/\1/'`

# SkyOS uses i386-skyos-pe
case "$target" in
*-skyos*-pe*) d_target_os=skyos ;;
esac

GDC=$CC
GDC=`echo $CC | sed s/xgcc/gdc/`

AC_MSG_CHECKING([If $GDC can compile D sources])
echo "int function(int) test;" > actest.d
$GDC -c -x d -I "$srcdir/libdruntime" actest.d
r=$?
rm -f actest.[do]
if test $r -eq 0; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([can't compile D sources!])
  dnl fix vi syntax highlight bug. '
fi

AC_SUBST(GDC)

AC_MSG_CHECKING([D GCC version])
d_gcc_ver=`$GDC -dumpversion`
AC_MSG_RESULT($d_gcc_ver)

# Need to export this variables for multilib
export CC_FOR_BUILD
export CFLAGS_FOR_BUILD
AC_SUBST(CC_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)

# Find build libiberty which is needed by x3
# Should not have to go up too many directories

d_libiberty_dir=../..
while test "$d_count" != 'xxx'; do
    BUILD_LIBIBERTY=$d_libiberty_dir/build-${build_alias}/libiberty/libiberty.a
    if test -f $BUILD_LIBIBERTY; then
	break
    fi
    # GCC 3.x does not use the 'build-' dir, so we have so search
    # for plain 'libiberty' with some kind of check it is not a target dir
    BUILD_LIBIBERTY=$d_libiberty_dir/libiberty/libiberty.a
    if test -f $BUILD_LIBIBERTY && test -d $d_libiberty_dir/gcc; then
	break
    fi

    d_libiberty_dir=../$d_libiberty_dir
    d_count="x$d_count"
done

if test ! -f "$BUILD_LIBIBERTY"; then
    AC_MSG_ERROR([cannot find libiberty.a for build])
fi

#used in druntime, so srcdir is 'libphobos/libdruntime'
LIBIBERTY_H_PATH='$(srcdir)/../../include'
BUILD_LIBIBERTY="../$BUILD_LIBIBERTY"

AC_SUBST(BUILD_LIBIBERTY)
AC_SUBST(LIBIBERTY_H_PATH)


dnl Eventually need to include everything from libstdc++-v3/acinclude.m4
dnl (# Default case for install directory for include files.) and on

# include dir .. need to support --enable-version-specific.. but
# will have to modify gcc/configure.ac ..
# For now, basic workaround for cross compilers ..
if test "${host}" != "${build}"; then
    gdc_include_dir='${libdir}/gcc/${host_alias}'/${d_gcc_ver}/include/d
else
    gdc_include_dir='${prefix}'/include/d/${d_gcc_ver}
fi
AC_SUBST(gdc_include_dir)
AC_ARG_WITH([cross-host],
  AC_HELP_STRING([--with-cross-host=HOST],
	       [configuring with a cross compiler]))
if test -n "$with_cross_host" &&
   test x"$with_cross_host" != x"no"; then
  phobos_toolexecdir='${exec_prefix}/${host_alias}'
  phobos_toolexeclibdir='${toolexecdir}/lib'
else
  phobos_toolexecdir='${libdir}/gcc/${host_alias}'
  phobos_toolexeclibdir='${libdir}'
fi
# The norm would be to use $GDC -print-multi-os-directory, but
# that would require modifying config-ml.in
multi_os_directory=`$CC -print-multi-os-directory`
case $multi_os_directory in
  .) ;; # Avoid trailing /.
  *) phobos_toolexeclibdir=$phobos_toolexeclibdir/$multi_os_directory ;;
esac
AC_SUBST(phobos_toolexecdir)
AC_SUBST(phobos_toolexeclibdir)

dnl Checks for header files.
# Sanity check for the cross-compilation case:
AC_CHECK_HEADER(stdio.h,:,
  [AC_MSG_ERROR([cannot find stdio.h.])])

dnl AC_HEADER_STDC
# TODO...

D_EXTRA_OBJS=
AC_SUBST(D_EXTRA_OBJS)
DRUNTIME_OBJS=
AC_SUBST(DRUNTIME_OBJS)

if test "$with_newlib" = no; then

AC_CHECK_LIB(m,cos)

case "$d_target_os" in
  aix*) AC_CHECK_LIB(C,sqrtf) ;;
esac

case "$target_os" in
  linux*|k*bsd*-gnu)
    case "$target_cpu" in
	powerpc*)
	    # Libc without nldbl not supported...
	    AC_CHECK_FUNC(__nldbl_printf,d_have_nldbl_funcs=1,:)
	    if test "$d_have_nldbl_funcs" = 1; then
		:
	    fi
	    ;;
    esac
esac

save_CFLAGS=$CFLAGS
dnl Check for BSD(?) specific fields in struct tm
dnl Maybe test fields separately
AC_MSG_CHECKING([for tm_gmtoff])
AC_TRY_COMPILE([#include <time.h>],[
struct tm t;
t.tm_gmtoff = t.tm_gmtoff;
t.tm_zone = t.tm_zone;],
  [AC_MSG_RESULT([yes])
   AC_DEFINE(HAVE_TM_GMTOFF_AND_ZONE,1,[Extra fields in struct tm])],
  [AC_MSG_RESULT([no])])

dnl The '* 42' is to ensure a type error occurs if timezone is not a
dnl number.  Simple assignment will not do this.
AC_MSG_CHECKING([for timezone])
AC_TRY_COMPILE([#include <time.h>],[
time_t t = timezone * 42;],
  [AC_MSG_RESULT([yes])
   AC_DEFINE(HAVE_TIMEZONE,1,[Global timezone variable])],
  [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for _timezone])
AC_TRY_COMPILE([#include <time.h>],[
time_t t = _timezone * 42;],
  [AC_MSG_RESULT([yes])
   AC_DEFINE(HAVE__TIMEZONE,1,[Another global timezone variable])],
  [AC_MSG_RESULT([no])])

AC_CHECK_FUNCS(snprintf _snprintf,break,[AC_MSG_ERROR([No variant of snprintf.])])
AC_CHECK_FUNCS(vsnprintf _vsnprintf,break,[AC_MSG_ERROR([No variant of vsnprintf.])])

# end of 'if $with_newlib...'
fi

AC_MSG_CHECKING([for ARM unwinder])
AC_TRY_COMPILE([#include <unwind.h>],[
#if __ARM_EABI_UNWINDER__
#error Yes, it is.
#endif
],
  [AC_MSG_RESULT([no])
   DCFG_ARM_EABI_UNWINDER=""
   DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/unwind_generic.o"],
  [AC_MSG_RESULT([yes])
   DCFG_ARM_EABI_UNWINDER="GNU_ARM_EABI_Unwinder"
   DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/unwind_arm.o"])
AC_SUBST(DCFG_ARM_EABI_UNWINDER)

case "$d_target_os" in
  aix*|*bsd*|cygwin*|darwin*|gnu*|linux*|skyos*|*solaris*|sysv*) d_have_unix=1 ;;
esac

DCFG_CBRIDGE_STDIO=
AC_SUBST(DCFG_CBRIDGE_STDIO)

if test -n "$d_have_unix" && test "$enable_unix" = auto ; then
  enable_unix=yes
fi

case "$d_target_os" in
    aix*)    d_is_aix=1
	     ;;
    darwin*) d_module_mach=1
	     d_sem_impl="mach"
	     ;;
    freebsd*|k*bsd*-gnu)
	     DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/cbridge_stdio.o"
	     D_EXTRA_OBJS="$D_EXTRA_OBJS \$(FREEBSD_OBJS)"
	     DCFG_CBRIDGE_STDIO="GNU_CBridge_Stdio"
	     ;;
    linux*)  #D_EXTRA_OBJS="$D_EXTRA_OBJS std/c/linux/linux.o"
	     D_EXTRA_OBJS="$D_EXTRA_OBJS \$(LINUX_OBJS)"
	     d_sem_impl="posix"
             ;;
    cygwin*) DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/cbridge_stdio.o"
	     DCFG_CBRIDGE_STDIO="GNU_CBridge_Stdio"
	     ;;
    mingw*)  #DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/cbridge_stdio.o"
	     DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_STDC_OBJS)"
	     DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_WINDOWS_OBJS)"
	     D_EXTRA_OBJS="$D_EXTRA_OBJS \$(WINDOWS_OBJS)"
	     D_EXTRA_OBJS="$D_EXTRA_OBJS \$(OS_OBJS)"
	     #DCFG_CBRIDGE_STDIO="GNU_CBridge_Stdio"
	     DCFG_UNIX=Windows
	     ;;
    skyos*)  d_sem_impl="skyos"
	     DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/cbridge_stdio.o"
	     DCFG_CBRIDGE_STDIO="GNU_CBridge_Stdio"
	     ;;
    *)       if test "$enable_unix" != "yes"; then
	         DCFG_UNIX=NoSystem
	     fi
	     DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/cbridge_stdio.o"
	     DCFG_CBRIDGE_STDIO="GNU_CBridge_Stdio"
	     ;;
esac

if test -n "$d_module_mach"; then
  DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_OSX_OBJS)"
  D_EXTRA_OBJS="$D_EXTRA_OBJS \$(OSX_OBJS)"
fi

if test "$enable_unix" = "yes"; then
  DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_STDC_OBJS)"
  DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_POSIX_OBJS)"
  D_EXTRA_OBJS="$D_EXTRA_OBJS \$(OS_OBJS)"
  DPHOBOS_CONFIGURE_UNIX
fi
dnl can these be moved to acinclude.m4?
AC_SUBST(DCFG_SEMAPHORE_IMPL)
AC_SUBST(DCFG_MMAP)
AC_SUBST(DCFG_GETPWNAM_R)


AC_SUBST(DCFG_UNIX)
AC_SUBST(DCFG_POSIX)

if test -z "$DFLAGS"; then
    DFLAGS="-Wall -g -frelease -O2"
fi
AC_SUBST(DFLAGS)

if test -z "$DFLAGSX"; then
    DFLAGSX="-Wall -g -fno-release -funittest"
fi
AC_SUBST(DFLAGSX)

dnl TODO: change this to using pthreads? if so, define usepthreads
dnl and configure semaphore


# phobose_use_pthreads was here...

d_subdirs=`( cd $srcdir && find . -type d ) | sed -e 's/^.\///'`
d_subdirs="$d_subdirs gcc"
for i in $d_subdirs; do
	mkdir -p $i;
done

AC_SUBST(srcdir)


# Garbage collection configuration

D_GC_MODULES=

if test "$enable_druntime_gc" = "yes"; then
    D_GC_MODULES="gc/gc.o gc/gcalloc.o gc/gcbits.o gc/gcstats.o gc/gcx.o"
else
    D_GC_MODULES="gcstub/gc.o"
fi

AC_SUBST(D_GC_MODULES)


ZLIB_OBJS=
if test "$system_zlib" = yes; then
    AC_CHECK_LIB(z, deflate, ZLIB_OBJS=, ZLIB_OBJS="\$(Z_OBJS)")
else
    ZLIB_OBJS="\$(Z_OBJS)"
fi
AC_SUBST(ZLIB_OBJS)

# Copied from libstdc++-v3/configure.ac
# Multilibs need MULTISUBDIR defined correctly in certain makefiles so
# that multilib installs will end up installed in the correct place.
# The testsuite needs it for multilib-aware ABI baseline files.
# To work around this not being passed down from config-ml.in ->
# srcdir/Makefile.am -> srcdir/{src,libsupc++,...}/Makefile.am, manually
# append it here.  Only modify Makefiles that have just been created.
#
# Also, get rid of this simulated-VPATH thing that automake does.
AC_CONFIG_FILES(AC_FOREACH([DIR], src libdruntime, [DIR/Makefile ]),
  [cat > vpsed$$ << \_EOF
s!`test -f '$<' || echo '$(srcdir)/'`!!
_EOF
   sed -f vpsed$$ $ac_file > tmp$$
   mv tmp$$ $ac_file
   rm vpsed$$
   echo 'MULTISUBDIR =' >> $ac_file
   ml_norecursion=yes
   . ${multi_basedir}/config-ml.in
   AS_UNSET([ml_norecursion])
])

#TODO: Should be possible to get rid of libdruntime/phobos-ver-syms
AC_OUTPUT([Makefile src/phobos-ver-syms libdruntime/phobos-ver-syms])
