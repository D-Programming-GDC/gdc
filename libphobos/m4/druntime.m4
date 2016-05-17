#
# Contains some unsorted druntime utility macros.
#


# DRUNTIME_WERROR
# ---------------
# Add the --enable-werror option and create the
# DRUNTIME_WERROR conditional
AC_DEFUN([DRUNTIME_WERROR],
[
  AC_ARG_ENABLE(werror, [AS_HELP_STRING([--enable-werror],
                                        [turns on -Werror @<:@default=yes@:>@])])
  WERROR_FLAG=""
  if test "x$enable_werror" != "xno"; then
      enable_werror="yes"
      WERROR_FLAG="-Werror"
  fi
  AM_CONDITIONAL([DRUNTIME_WERROR], [test "$enable_werror" = "yes"])
])


# DRUNTIME_MULTILIB
# -----------------
# Prepare the multilib_arg variable
AC_DEFUN([DRUNTIME_MULTILIB],
[
  if test ${multilib} = yes; then
    multilib_arg="--enable-multilib"
  else
    multilib_arg=
  fi
])


# DRUNTIME_INSTALL_DIRECTORIES
# ----------------------------
# Setup various install directories for headers.
# Add the cross-host option and substitute the toolexecdir
# toolexeclibdir and gdc_include_dir variables.
AC_DEFUN([DRUNTIME_INSTALL_DIRECTORIES],
[
  AC_REQUIRE([AC_PROG_GDC])

  AC_MSG_CHECKING([D GCC version])
  d_gcc_ver=`$GDC -dumpversion`
  AC_MSG_RESULT($d_gcc_ver)
  AC_ARG_WITH([cross-host],
    AC_HELP_STRING([--with-cross-host=HOST],
                   [configuring with a cross compiler]))

  toolexecdir=no
  toolexeclibdir=no

  version_specific_libs=no

  # Version-specific runtime libs processing.
  if test $version_specific_libs = yes; then
      toolexecdir='${libdir}/gcc/${host_alias}'
      toolexeclibdir='${toolexecdir}/${gcc_version}$(MULTISUBDIR)'
  else
      # Calculate toolexecdir, toolexeclibdir
      # Install a library built with a cross compiler in tooldir, not libdir.
      if test -n "$with_cross_host" && test x"$with_cross_host" != x"no"; then
          toolexecdir='${exec_prefix}/${host_alias}'
          toolexeclibdir='${toolexecdir}/lib'
      else
          toolexecdir='${libdir}/gcc/${host_alias}'
          toolexeclibdir='${libdir}'
      fi
      multi_os_directory=`$GDC -print-multi-os-directory`
      case $multi_os_directory in
          .) ;; # Avoid trailing /.
          *) toolexeclibdir=${toolexeclibdir}/${multi_os_directory} ;;
      esac
  fi
  AC_SUBST(toolexecdir)
  AC_SUBST(toolexeclibdir)

  # Default case for install directory for D sources files.
  gdc_include_dir='${libdir}/gcc/${target_alias}'/${d_gcc_ver}/include/d
  AC_SUBST(gdc_include_dir)
])


# DRUNTIME_GC
# -----------
# Add the --enable-druntime-gc option and create the
# DRUNTIME_GC_ENABLE conditional
AC_DEFUN([DRUNTIME_GC],
[
  dnl switch between gc and gcstub
  AC_ARG_ENABLE(druntime-gc,
    AC_HELP_STRING([--enable-druntime-gc],
                   [enable D runtime garbage collector (default: yes)]),
    [enable_druntime_gc=no],[enable_druntime_gc=yes])

  AM_CONDITIONAL([DRUNTIME_GC_ENABLE], [test "$enable_druntime_gc" = "yes"])
])


# DRUNTIME_VERSION
# -----------------
# Substitute the RELEASE_VER variable, read from VERSION
AC_DEFUN([DRUNTIME_VERSION], [
  AC_MSG_CHECKING([for phobos source version])
  RELEASE_VER=`cat $srcdir/VERSION`
  AC_MSG_RESULT([$RELEASE_VER])])
  AC_SUBST([RELEASE_VER])
])
