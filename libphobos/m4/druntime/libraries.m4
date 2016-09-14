#
# Contains macros to handle library dependencies.
#


# DRUNTIME_LIBRARIES_DLOPEN
# -----------------------
# Check whether -ldl is required for dlopen.
AC_DEFUN([DRUNTIME_LIBRARIES_DLOPEN],
[
AC_SEARCH_LIBS([dlopen], [dl])
])


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
# DRUNTIME_ZLIB_SYSTEM conditional
AC_DEFUN([DRUNTIME_LIBRARIES_ZLIB],
[
  dnl switch between system zlib and gcc zlib
  AC_ARG_WITH(system-zlib,
    AS_HELP_STRING([--with-system-zlib],
                   [use installed libz (default: no)]),
    [system_zlib=yes],[system_zlib=no])
  AM_CONDITIONAL([DRUNTIME_ZLIB_SYSTEM], [test "$system_zlib" = yes])
])
