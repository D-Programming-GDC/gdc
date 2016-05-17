#
# Contains macros to configure libbacktrace.
#


# DRUNTIME_LIBBACKTRACE_SETUP
# ---------------------------
# Add the --with-libbacktrace=PATH and --disable-libbacktrace option,
# add BACKTRACE_SUPPORTED conditional
# and subsitute HAVE_DLADDR LIBBACKTRACE_DIR BACKTRACE_SUPPORTED
# BACKTRACE_USES_MALLOC BACKTRACE_SUPPORTS_THREADS.
AC_DEFUN([DRUNTIME_LIBBACKTRACE_SETUP],
[
  AC_LANG_PUSH([C])
  # FIXME: We have to check in libdl or whatever is the linker library.
  HAVE_DLADDR=false
  AC_CHECK_FUNC(dladdr, HAVE_DLADDR=true)
  AC_SUBST(HAVE_DLADDR)

  BACKTRACE_SUPPORTED=false
  BACKTRACE_USES_MALLOC=false
  BACKTRACE_SUPPORTS_THREADS=false
  LIBBACKTRACE_DIR=""

  gdc_save_CPPFLAGS=$CPPFLAGS
  AC_ARG_WITH([libbacktrace],
  [  --with-libbacktrace=PATH specify directory containing libbacktrace object files],
  [], [with_libbacktrace=no])

  AC_MSG_CHECKING([where to find compiled libbacktrace])
  if test "x$with_libbacktrace" != xno; then
    CPPFLAGS+=" -I$with_libbacktrace "
    AC_MSG_RESULT([$with_libbacktrace])
  else
    CPPFLAGS+=" -I../libbacktrace "
    AC_MSG_RESULT([in GCC tree])
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
  CPPFLAGS=$gdc_save_CPPFLAGS

  AM_CONDITIONAL([BACKTRACE_SUPPORTED], [$BACKTRACE_SUPPORTED])

  if $BACKTRACE_SUPPORTED; then
    if test "x$with_libbacktrace" != xno; then
      LIBBACKTRACE_DIR="$with_libbacktrace"
    else
      LIBBACKTRACE_DIR="../../libbacktrace"
    fi
  else
    LIBBACKTRACE_DIR=""
  fi

  AC_SUBST(LIBBACKTRACE_DIR)
  AC_SUBST(BACKTRACE_SUPPORTED)
  AC_SUBST(BACKTRACE_USES_MALLOC)
  AC_SUBST(BACKTRACE_SUPPORTS_THREADS)
  AC_LANG_POP([C])
])
