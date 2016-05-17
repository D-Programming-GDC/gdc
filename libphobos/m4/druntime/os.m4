#
# Contains macros to detect OS features.
#


# DRUNTIME_OS_THREAD_MODEL
# ------------------------
# Detect thread model and substitute DCFG_THREAD_MODEL
AC_DEFUN([DRUNTIME_OS_THREAD_MODEL],
[
  AC_REQUIRE([AC_PROG_GDC])
  AC_MSG_CHECKING([for thread model used by GDC])
  d_thread_model=`$GDC -v 2>&1 | sed -n 's/^Thread model: //p'`
  AC_MSG_RESULT([$d_thread_model])

  # Map from thread model to thread interface.
  DRUNTIME_CONFIGURE_THREADS([$d_thread_model])
])


# DRUNTIME_CONFIGURE_THREADS(thread_model)
# ----------------------------------------
# Map target os to D version identifier
AC_DEFUN([DRUNTIME_CONFIGURE_THREADS],
[
case $1 in
    aix)    DCFG_THREAD_MODEL="Posix" ;;
    lynx)   DCFG_THREAD_MODEL="Posix" ;;
    posix)  DCFG_THREAD_MODEL="Posix" ;;
    single) DCFG_THREAD_MODEL="Single" ;;
    win32)  DCFG_THREAD_MODEL="Win32" ;;
    # TODO: These targets need porting.
    dce|mipssde|rtems|tpf|vxworks)
	    DCFG_THREAD_MODEL="Single" ;;
    *)	    as_fn_error "Thread implementation '$1' not recognised" "$LINENO" 5 ;;
esac
AC_SUBST(DCFG_THREAD_MODEL)
])


# DRUNTIME_OS_DETECT
# ------------------
# Set the druntime_cv_target_os variable
AC_DEFUN([DRUNTIME_OS_DETECT],
[
  AC_CACHE_CHECK([[for target OS]],
    [[druntime_cv_target_os]],
    [[druntime_cv_target_os=`echo $target_os | sed 's/^\([A-Za-z_]+\)/\1/'`]])
    AS_IF([[test -z "$phobos_cv_abs_srcdir"]],
      [AC_MSG_ERROR([[can't detect target OS]])],
      dnl fix vi syntax highlight bug. '
      [])
])


# DRUNTIME_OS_UNIX
# ----------------
# Add --enable-unix option or autodetects if system is unix
# and create the DRUNTIME_OS_UNIX conditional.
AC_DEFUN([DRUNTIME_OS_UNIX],
[
  AC_REQUIRE([DRUNTIME_OS_DETECT])
  AC_ARG_ENABLE(unix,
    AC_HELP_STRING([--enable-unix],
                   [enables Unix runtime (default: yes, for Unix targets)]),
    :,[enable_unix=auto])

  case "$druntime_cv_target_os" in
    aix*|*bsd*|cygwin*|darwin*|gnu*|linux*|skyos*|*solaris*|sysv*) d_have_unix=1 ;;
  esac
  if test -n "$d_have_unix" && test "$enable_unix" = auto ; then
    enable_unix=yes
  fi
  AM_CONDITIONAL([DRUNTIME_OS_UNIX], [test "$enable_unix" = "yes"])
])


# DRUNTIME_OS_SOURCES
# -------------------
# Detect target OS and add DRUNTIME_OS_AIX DRUNTIME_OS_DARWIN
# DRUNTIME_OS_BSD DRUNTIME_OS_LINUX DRUNTIME_OS_MINGW DRUNTIME_OS_SOLARIS
# conditionals.
AC_DEFUN([DRUNTIME_OS_SOURCES],
[
  AC_REQUIRE([DRUNTIME_OS_DETECT])

  druntime_target_os_parsed=""
  case "$druntime_cv_target_os" in
      aix*)    druntime_target_os_parsed="aix"
               ;;
      darwin*) druntime_target_os_parsed="darwin"
               ;;
      freebsd*|k*bsd*-gnu)
               druntime_target_os_parsed="bsd"
               ;;
      linux*)  druntime_target_os_parsed="linux"
               ;;
      mingw*)  druntime_target_os_parsed="mingw"
             ;;
      *solaris*) druntime_target_os_parsed="solaris"
  esac
  AM_CONDITIONAL([DRUNTIME_OS_AIX], [test "$druntime_target_os_parsed" = "aix"])
  AM_CONDITIONAL([DRUNTIME_OS_DARWIN], [test "$druntime_target_os_parsed" = "darwin"])
  AM_CONDITIONAL([DRUNTIME_OS_BSD], [test "$druntime_target_os_parsed" = "bsd"])
  AM_CONDITIONAL([DRUNTIME_OS_LINUX], [test "$druntime_target_os_parsed" = "linux"])
  AM_CONDITIONAL([DRUNTIME_OS_MINGW], [test "$druntime_target_os_parsed" = "mingw"])
  AM_CONDITIONAL([DRUNTIME_OS_SOLARIS], [test "$druntime_target_os_parsed" = "solaris"])
])


# DRUNTIME_OS_ARM_EABI_UNWINDER
# ------------------------
# Check if using ARM unwinder and substitute DCFG_ARM_EABI_UNWINDER
# and set DRUNTIME_OS_ARM_EABI_UNWINDER conditional.
AC_DEFUN([DRUNTIME_OS_ARM_EABI_UNWINDER],
[
  AC_MSG_CHECKING([for ARM unwinder])
  AC_TRY_COMPILE([#include <unwind.h>],[
  #if __ARM_EABI_UNWINDER__
  #error Yes, it is.
  #endif
  ],
    [AC_MSG_RESULT([no])
     DCFG_ARM_EABI_UNWINDER=false],
    [AC_MSG_RESULT([yes])
     DCFG_ARM_EABI_UNWINDER=true])
  AC_SUBST(DCFG_ARM_EABI_UNWINDER)
  AM_CONDITIONAL([DRUNTIME_OS_ARM_EABI_UNWINDER], [test "$DCFG_ARM_EABI_UNWINDER" = "true"])
])
