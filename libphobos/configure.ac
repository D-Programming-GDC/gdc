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
AM_PROG_AS
AC_PROG_CC
AC_PROG_GDC
WITH_LOCAL_DRUNTIME([GDC_CHECK_COMPILE], [])

m4_rename_force([glibd_PRECIOUS],[_AC_ARG_VAR_PRECIOUS])

AC_SUBST(CFLAGS)
AM_MAINTAINER_MODE
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# This should be inherited in the recursive make, but ensure it is defined.
test "$AR" || AR=ar
AC_SUBST(AR)

CC_FOR_BUILD=${CC_FOR_BUILD:-gcc}
AC_SUBST(CC_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)

# Enable libtool
LT_INIT
AM_PROG_LIBTOOL
WITH_LOCAL_DRUNTIME([LT_LANG([D])], [])
AM_CONDITIONAL([ENABLE_SHARED], [test "$enable_shared" = "yes"])
AM_CONDITIONAL([ENABLE_STATIC], [test "$enable_static" = "yes"])


# libphobos/libdruntime specific options and feature detection
DRUNTIME_MULTILIB
DRUNTIME_WERROR
DRUNTIME_GC
DRUNTIME_OS_UNIX
DRUNTIME_OS_SOURCES
DRUNTIME_OS_THREAD_MODEL
DRUNTIME_OS_ARM_EABI_UNWINDER
DRUNTIME_OS_MINFO_BRACKETING
DRUNTIME_LIBRARIES_DLOPEN
DRUNTIME_LIBRARIES_THREAD
DRUNTIME_LIBRARIES_ZLIB
DRUNTIME_LIBBACKTRACE_SETUP
DRUNTIME_INSTALL_DIRECTORIES
DRUNTIME_LIB_VERSIONS

# Set default flags (after DRUNTIME_WERROR!)
if test -z "$DFLAGS"; then
    DFLAGS="-Wall $WERROR_FLAG -g -frelease -O2"
fi
AC_SUBST(DFLAGS)

if test -z "$DFLAGSX"; then
    DFLAGSX="-Wall $WERROR_FLAG -g -fno-release -funittest"
fi
AC_SUBST(DFLAGSX)

# Sanity check for the cross-compilation case:
AC_CHECK_HEADER(stdio.h,:,
  [AC_MSG_ERROR([cannot find stdio.h.])])

# Create all directories ready to start building.
d_subdirs=`( cd $srcdir && find . -type d ) | sed -e 's/^.\///'`
d_subdirs="$d_subdirs"
for i in $d_subdirs; do
    mkdir -p $i;
done
AC_SUBST(srcdir)

# Multilibs need MULTISUBDIR defined correctly in certain makefiles so
# that multilib installs will end up installed in the correct place.
# The testsuite needs it for multilib-aware ABI baseline files.
# To work around this not being passed down from config-ml.in ->
# srcdir/Makefile.am -> srcdir/{src,libsupc++,...}/Makefile.am, manually
# append it here.  Only modify Makefiles that have just been created.
#
# Also, get rid of this simulated-VPATH thing that automake does.
AC_CONFIG_FILES(AC_FOREACH([DIR], src libdruntime testsuite, [DIR/Makefile ]),
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
