# GDC -- D front-end for GCC
# Copyright (C) 2011-2015 Free Software Foundation, Inc.
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

# Process this file with autoreconf to produce a configure script.
# This requires that you have your environment set-up to use explicit
# versions of automake and autoconf.
#
#    export ACLOCAL=/usr/bin/aclocal-1.11
#    export AUTOMAKE=/usr/bin/automake-1.11
#    export AUTOM4TE=/usr/bin/autom4te2.64
#    export AUTOCONF=/usr/bin/autoconf2.64
#
#    autoreconf2.64
#

AC_PREREQ(2.64)
AC_INIT(package-unused, version-unused,, libphobos)
AC_CONFIG_SRCDIR(std/algorithm.d)
AC_CONFIG_SRCDIR(libdruntime/gcc/builtins.d)
AC_CONFIG_SRCDIR(libdruntime/gcc/attribute.d)
AC_CONFIG_HEADERS(config.h)

AM_ENABLE_MULTILIB(, ..)

AC_CANONICAL_SYSTEM
target_alias=${target_alias-$target}
AC_SUBST(target_alias)

# 1.11.1: Require that version of automake.
# foreign: Don't require README, INSTALL, NEWS, etc.
# no-define: Don't define PACKAGE and VERSION.
# no-dependencies: Don't generate automatic dependencies.
#    (because it breaks when using bootstrap-lean, since some of the
#    headers are gone at "make install" time).
# -Wall: Issue all automake warnings.
# -Wno-portability: Don't warn about constructs supported by GNU make.
#    (because GCC requires GNU make anyhow).
AM_INIT_AUTOMAKE([1.11.1 foreign no-dist no-define no-dependencies -Wall -Wno-portability])

m4_rename([_AC_ARG_VAR_PRECIOUS],[glibd_PRECIOUS])
m4_define([_AC_ARG_VAR_PRECIOUS],[])
AC_PROG_CC
m4_rename_force([glibd_PRECIOUS],[_AC_ARG_VAR_PRECIOUS])

AC_SUBST(CFLAGS)

AM_MAINTAINER_MODE

AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl This should be inherited in the recursive make, but ensure it is defined.
test "$AR" || AR=ar
AC_SUBST(AR)

AC_SUBST(enable_shared)
AC_SUBST(enable_static)

CC_FOR_BUILD=${CC_FOR_BUILD:-gcc}
AC_SUBST(CC_FOR_BUILD)

AC_SUBST(CFLAGS_FOR_BUILD)

dnl libphobos/libdruntime configure switches start here.
AC_ARG_WITH([cross-host],
  AC_HELP_STRING([--with-cross-host=HOST],
                 [configuring with a cross compiler]))

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
  [enable_druntime_gc=no],[enable_druntime_gc=yes])

dnl switch between system zlib and gcc's zlib
AC_ARG_WITH(system-zlib,
  AS_HELP_STRING([--with-system-zlib],
                 [use installed libz (default: no)]),
  [system_zlib=yes],[system_zlib=no])

AC_ARG_ENABLE(werror, [AS_HELP_STRING([--enable-werror],
                                      [turns on -Werror @<:@default=yes@:>@])])
if test "x$enable_werror" != "xno"; then
    WERROR="-Werror"
fi
AC_SUBST(WERROR)

if test ${multilib} = yes; then
  multilib_arg="--enable-multilib"
else
  multilib_arg=
fi

d_target_os=`echo $target_os | sed 's/^\([A-Za-z_]+\)/\1/'`

if test -z "$GDC"; then
  AC_CHECK_PROG(GDC, gdc, gdc, , , gdc)
fi

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

phobos_toolexecdir=no
phobos_toolexeclibdir=no

AC_MSG_CHECKING([for --enable-version-specific-runtime-libs])
AC_ARG_ENABLE([version-specific-runtime-libs],
  AC_HELP_STRING([--enable-version-specific-runtime-libs],
                 [Specify that runtime libraries should be installed in a compiler-specific directory]),
  [case "$enableval" in
    yes) version_specific_libs=yes ;;
    no)  version_specific_libs=no ;;
    *)   AC_MSG_ERROR([Unknown argument to enable/disable version-specific libs]);;
   esac],
  [version_specific_libs=no])
AC_MSG_RESULT($version_specific_libs)

# Version-specific runtime libs processing.
if test $version_specific_libs = yes; then
    phobos_toolexecdir='${libdir}/gcc/${host_alias}'
    phobos_toolexeclibdir='${toolexecdir}/${gcc_version}$(MULTISUBDIR)'
else
    # Calculate phobos_toolexecdir, phobos_toolexeclibdir
    # Install a library built with a cross compiler in tooldir, not libdir.
    if test -n "$with_cross_host" && test x"$with_cross_host" != x"no"; then
        phobos_toolexecdir='${exec_prefix}/${host_alias}'
        phobos_toolexeclibdir='${toolexecdir}/lib'
    else
        phobos_toolexecdir='${libdir}/gcc/${host_alias}'
        phobos_toolexeclibdir='${libdir}'
    fi
    multi_os_directory=`$GDC -print-multi-os-directory`
    case $multi_os_directory in
        .) ;; # Avoid trailing /.
        *) phobos_toolexeclibdir=${phobos_toolexeclibdir}/${multi_os_directory} ;;
    esac
fi
AC_SUBST(phobos_toolexecdir)
AC_SUBST(phobos_toolexeclibdir)

# Default case for install directory for D sources files.
gdc_include_dir='${libdir}/gcc/${target_alias}'/${d_gcc_ver}/include/d
AC_SUBST(gdc_include_dir)

dnl Checks for header files.
# Sanity check for the cross-compilation case:
AC_CHECK_HEADER(stdio.h,:,
  [AC_MSG_ERROR([cannot find stdio.h.])])

dnl libbacktrace support
HAVE_DLADDR=false
AC_CHECK_FUNC(dladdr, HAVE_DLADDR=true)
AC_SUBST(HAVE_DLADDR)

BACKTRACE_SUPPORTED=false
BACKTRACE_USES_MALLOC=false
BACKTRACE_SUPPORTS_THREADS=false
LIBBACKTRACE_LIB=""

with_libbacktrace=no
AC_ARG_WITH(libbacktrace,
[  --with-libbacktrace=PATH specify directory containing libbacktrace object files],
[with_libbacktrace=$withval], [with_libbacktrace=no])

if test "x$with_libbacktrace" != xno; then
  CPPFLAGS+=" -I$with_libbacktrace "
else
  CPPFLAGS+=" -I../libbacktrace "
fi

AC_ARG_ENABLE(libbacktrace,
 [  --disable-libbacktrace  Do not use libbacktrace for backtraces],
 check_libbacktrace_h="$enableval", check_libbacktrace_h="yes")

if test $check_libbacktrace_h = yes ; then
  AC_CHECK_HEADER(backtrace-supported.h, have_libbacktrace_h=true,
    have_libbacktrace_h=false)
else
  have_libbacktrace_h=false
fi

if $have_libbacktrace_h; then
  AC_MSG_CHECKING([libbacktrace: BACKTRACE_SUPPORTED])
  AC_EGREP_CPP(FOUND_LIBBACKTRACE_RESULT_GDC,
  [
  #include <backtrace-supported.h>
  #if BACKTRACE_SUPPORTED
    FOUND_LIBBACKTRACE_RESULT_GDC
  #endif
  ], BACKTRACE_SUPPORTED=true, BACKTRACE_SUPPORTED=false)
  AC_MSG_RESULT($BACKTRACE_SUPPORTED)

  AC_MSG_CHECKING([libbacktrace: BACKTRACE_USES_MALLOC])
  AC_EGREP_CPP(FOUND_LIBBACKTRACE_RESULT_GDC,
  [
  #include <backtrace-supported.h>
  #if BACKTRACE_USES_MALLOC
    FOUND_LIBBACKTRACE_RESULT_GDC
  #endif
  ], BACKTRACE_USES_MALLOC=true, BACKTRACE_USES_MALLOC=false)
  AC_MSG_RESULT($BACKTRACE_USES_MALLOC)

  AC_MSG_CHECKING([libbacktrace: BACKTRACE_SUPPORTS_THREADS])
  AC_EGREP_CPP(FOUND_LIBBACKTRACE_RESULT_GDC,
  [
  #include <backtrace-supported.h>
  #if BACKTRACE_SUPPORTS_THREADS
    FOUND_LIBBACKTRACE_RESULT_GDC
  #endif
  ], BACKTRACE_SUPPORTS_THREADS=true, BACKTRACE_SUPPORTS_THREADS=false)
  AC_MSG_RESULT($BACKTRACE_SUPPORTS_THREADS)
fi

AM_CONDITIONAL([BACKTRACE_SUPPORTED], [$BACKTRACE_SUPPORTED])

if $BACKTRACE_SUPPORTED; then
  if test "x$with_libbacktrace" != xno; then
    LIBBACKTRACE_LIB="$with_libbacktrace/.libs/libbacktrace.a"
  else
    LIBBACKTRACE_LIB="../../libbacktrace/.libs/libbacktrace.a"
  fi
else
  LIBBACKTRACE_LIB=""
fi

AC_SUBST(LIBBACKTRACE_LIB)
AC_SUBST(BACKTRACE_SUPPORTED)
AC_SUBST(BACKTRACE_USES_MALLOC)
AC_SUBST(BACKTRACE_SUPPORTS_THREADS)

dnl Thread model.
AC_MSG_CHECKING([for thread model used by GDC])
d_thread_model=`$GDC -v 2>&1 | sed -n 's/^Thread model: //p'`
AC_MSG_RESULT([$d_thread_model])

# Map from thread model to thread interface.
DRUNTIME_CONFIGURE_THREADS([$d_thread_model])

D_EXTRA_OBJS=
AC_SUBST(D_EXTRA_OBJS)
DRUNTIME_OBJS=
AC_SUBST(DRUNTIME_OBJS)

AC_MSG_CHECKING([for ARM unwinder])
AC_TRY_COMPILE([#include <unwind.h>],[
#if __ARM_EABI_UNWINDER__
#error Yes, it is.
#endif
],
  [AC_MSG_RESULT([no])
   DCFG_ARM_EABI_UNWINDER=false
   DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/unwind/generic.o"],
  [AC_MSG_RESULT([yes])
   DCFG_ARM_EABI_UNWINDER=true
   DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/unwind/arm.o"])
AC_SUBST(DCFG_ARM_EABI_UNWINDER)

case "$d_thread_model" in
  posix) DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/gthreads/posix.o"
         ;;
  win32) DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/gthreads/win32.o"
         ;;
  *)     DRUNTIME_OBJS="$DRUNTIME_OBJS gcc/gthreads/single.o"
         ;;
esac

case "$d_target_os" in
  aix*|*bsd*|cygwin*|darwin*|gnu*|linux*|skyos*|*solaris*|sysv*) d_have_unix=1 ;;
esac

if test -n "$d_have_unix" && test "$enable_unix" = auto ; then
  enable_unix=yes
fi

case "$d_target_os" in
    aix*)    d_is_aix=1
             ;;
    darwin*) DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_OSX_OBJS)"
             D_EXTRA_OBJS="$D_EXTRA_OBJS \$(OSX_OBJS)"
             ;;
    freebsd*|k*bsd*-gnu)
             D_EXTRA_OBJS="$D_EXTRA_OBJS \$(FREEBSD_OBJS)"
             ;;
    linux*)  D_EXTRA_OBJS="$D_EXTRA_OBJS \$(LINUX_OBJS)"
             ;;
    mingw*)  DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_STDC_OBJS)"
             DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_WINDOWS_OBJS)"
             D_EXTRA_OBJS="$D_EXTRA_OBJS \$(WINDOWS_OBJS)"
             D_EXTRA_OBJS="$D_EXTRA_OBJS \$(OS_OBJS)"
             ;;
    *solaris*)
	    D_EXTRA_OBJS="$D_EXTRA_OBJS \$(RT_SOLARIS_OBJS)"
esac

if test "$enable_unix" = "yes"; then
  DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_STDC_OBJS)"
  DRUNTIME_OBJS="$DRUNTIME_OBJS \$(RT_POSIX_OBJS)"
  D_EXTRA_OBJS="$D_EXTRA_OBJS \$(OS_OBJS)"
fi

if test -z "$DFLAGS"; then
    DFLAGS="-Wall \$(WERROR) -g -frelease -O2"
fi
AC_SUBST(DFLAGS)

if test -z "$DFLAGSX"; then
    DFLAGSX="-Wall \$(WERROR) -g -fno-release -funittest"
fi
AC_SUBST(DFLAGSX)

# Create all directories ready to start building.
d_subdirs=`( cd $srcdir && find . -type d ) | sed -e 's/^.\///'`
d_subdirs="$d_subdirs"
for i in $d_subdirs; do
    mkdir -p $i;
done
AC_SUBST(srcdir)


# Garbage collection configuration
D_GC_MODULES=

if test "$enable_druntime_gc" = "yes"; then
    D_GC_MODULES="gc/bits.o gc/config.o gc/gc.o gc/os.o \
		  gc/pooltable.o gc/proxy.o gc/stats.o"
else
    D_GC_MODULES="gcstub/gc.o"
fi

AC_SUBST(D_GC_MODULES)

# Use gcc or system zlib
ZLIB_OBJS=
if test "$system_zlib" = yes; then
    AC_CHECK_LIB(z, deflate, ZLIB_OBJS=, ZLIB_OBJS="\$(Z_OBJS)")
else
    ZLIB_OBJS="\$(Z_OBJS)"
fi
AC_SUBST(ZLIB_OBJS)

dnl Test for the -ldl library.
DL_LIBS=
AC_CHECK_LIB([dl], [dlopen], DL_LIBS=-ldl)
AC_SUBST(DL_LIBS)

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

AC_CONFIG_FILES([Makefile libdruntime/gcc/config.d libdruntime/gcc/libbacktrace.d])
AC_OUTPUT
