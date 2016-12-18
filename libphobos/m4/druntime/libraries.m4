#
# Contains macros to handle library dependencies.
#


# DRUNTIME_LIBRARIES_THREAD
# -------------------------
# Allow specifying the thread library to link with or autodetect
# Add thread library to LIBS if necessary.
AC_DEFUN([DRUNTIME_LIBRARIES_THREAD],
[
enable_thread_lib=yes
AC_ARG_ENABLE(thread-lib,
  AC_HELP_STRING([--enable-thread-lib=<arg>],
                 [specify linker option for the system thread library (default: autodetect)]))

  AS_IF([test "x$enable_thread_lib" = "xyes"], [
    AC_SEARCH_LIBS([pthread_create], [pthread])
  ], [
    AS_IF([test "x$enable_thread_lib" = "xno"], [
      AC_MSG_CHECKING([for thread library])
      AC_MSG_RESULT([disabled])
      ], [
      AC_CHECK_LIB([$enable_thread_lib], [pthread_create], [], [
        AC_MSG_ERROR([Thread library not found])])
        ])
      ])
  ])
])


# DRUNTIME_LIBRARIES_ZLIB
# -----------------------
# Allow specifying whether to use the system zlib or
# compiling the zlib included in GCC. Define
# DRUNTIME_ZLIB_SYSTEM conditional and add zlib to
# LIBS if necessary.
AC_DEFUN([DRUNTIME_LIBRARIES_ZLIB],
[
  AC_ARG_WITH(target-system-zlib,
    AS_HELP_STRING([--with-target-system-zlib],
                   [use installed libz (default: no)]))

  system_zlib=false
  AS_IF([test "x$with_target_system_zlib" = "xyes"], [
    AC_CHECK_LIB([z], [deflate], [
      system_zlib=yes
      ], [
      AC_MSG_ERROR([System zlib not found])])
  ], [
    AC_MSG_CHECKING([for zlib])
    AC_MSG_RESULT([just compiled])
  ])

  AM_CONDITIONAL([DRUNTIME_ZLIB_SYSTEM], [test "$with_target_system_zlib" = yes])
])

# DRUNTIME_LIBRARIES_ATOMIC
# -------------------------
# Allow specifying whether to use libatomic for
# 128 bit atomic support.
AC_DEFUN([DRUNTIME_LIBRARIES_ATOMIC],
[
  AC_ARG_WITH(libatomic,
    AS_HELP_STRING([--without-libatomic],
                   [Do not use libatomic in core.atomic (default: auto)]))

  DCFG_HAVE_LIBATOMIC=false
  AS_IF([test "x$with_libatomic" != "xno"], [
    AC_SEARCH_LIBS([__atomic_load], [atomic], [
        DCFG_HAVE_LIBATOMIC=true
      ])
  ], [
    AC_MSG_CHECKING([for libatomic])
    AC_MSG_RESULT([disabled])
  ])

  AC_SUBST(DCFG_HAVE_LIBATOMIC)
])
