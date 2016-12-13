#
# Contains macros to handle library dependencies.
#


# DRUNTIME_LIBRARIES_THREAD
# -------------------------
# Allow specifying the thread library to link with or autodetect
# FIXME: This isn't actually used anywhere
AC_DEFUN([DRUNTIME_LIBRARIES_THREAD],
[
AC_ARG_ENABLE(thread-lib,
  AC_HELP_STRING([--enable-thread-lib=<arg>],
                 [specify linker option for the system thread library (default: autodetect)]),
  [d_thread_lib=$enableval],[d_thread_lib=""])
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
