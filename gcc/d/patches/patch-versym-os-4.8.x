This patch implements the following official versions:
* Windows
** Win32
** Win64
** Cygwin
** MinGW
* linux
* OSX
* FreeBSD
* OpenBSD
* NetBSD
* Solaris
* Posix
* AIX
* Hurd
* Android

These gdc specific versions are also implemented:
* GNU_MinGW64 (for mingw-w64)
* GNU_OpenSolaris (for opensolaris)
* GNU_GLibc (implemented for linux & bsd & opensolaris)
* GNU_UCLibc (implemented for linux) (not on rs6000, alpha)
* GNU_Bionic (implemented for linux) (not on rs6000, alpha)

These official OS versions are not implemented:
* DragonFlyBSD
* BSD (other BSDs)
* Haiku
* SkyOS
* SysV3
* SysV4
---

--- gcc/config/alpha/linux.h	2013-07-04 20:28:59.559658493 +0200
+++ gcc/config/alpha/linux.h	2013-07-04 20:04:56.000000000 +0200
@@ -33,6 +33,16 @@ along with GCC; see the file COPYING3.
 	  builtin_define ("_GNU_SOURCE");			\
     } while (0)
 
+#undef TARGET_OS_D_BUILTINS 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	if (OPTION_GLIBC)					\
+	  builtin_define ("GNU_GLibc");				\
+								\
+	builtin_define ("linux");				\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #undef LIB_SPEC
 #define LIB_SPEC \
   "%{pthread:-lpthread} \
--- gcc/config/arm/linux-eabi.h	2013-07-04 20:28:59.559658493 +0200
+++ gcc/config/arm/linux-eabi.h	2013-07-04 18:30:31.000000000 +0200
@@ -30,6 +30,15 @@
     }						\
   while (false)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS() 			\
+  do 						\
+    {						\
+      TARGET_GENERIC_LINUX_OS_D_BUILTINS();	\
+      ANDROID_TARGET_OS_D_BUILTINS();		\
+    }						\
+  while (false)
+
 /* We default to a soft-float ABI so that binaries can run on all
    target hardware.  If you override this to use the hard-float ABI then
    change the setting of GLIBC_DYNAMIC_LINKER_DEFAULT as well.  */
--- gcc/config/darwin.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/darwin.h	2013-07-04 18:30:31.000000000 +0200
@@ -921,4 +921,10 @@ extern void darwin_driver_init (unsigned
    providing an osx-version-min of this unless overridden by the User.  */
 #define DEF_MIN_OSX_VERSION "10.4"
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("OSX");					\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #endif /* CONFIG_DARWIN_H */
--- gcc/config/freebsd.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/freebsd.h	2013-07-04 18:30:31.000000000 +0200
@@ -32,6 +32,13 @@ along with GCC; see the file COPYING3.
 #undef  TARGET_OS_CPP_BUILTINS
 #define TARGET_OS_CPP_BUILTINS() FBSD_TARGET_OS_CPP_BUILTINS()
 
+#undef TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("FreeBSD");				\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #undef  CPP_SPEC
 #define CPP_SPEC FBSD_CPP_SPEC
 
--- gcc/config/gnu.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/gnu.h	2013-07-04 18:30:34.000000000 +0200
@@ -39,3 +39,11 @@ along with GCC.  If not, see <http://www
 	builtin_assert ("system=unix");		\
 	builtin_assert ("system=posix");	\
     } while (0)
+
+#undef TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+								\
+	builtin_define ("Hurd");				\
+	builtin_define ("Posix");				\
+    } while (0)
--- gcc/config/i386/cygwin.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/i386/cygwin.h	2013-07-04 18:30:34.000000000 +0200
@@ -20,6 +20,13 @@ along with GCC; see the file COPYING3.
 
 #define EXTRA_OS_CPP_BUILTINS()  /* Nothing.  */
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+      builtin_define ("Windows");				\
+      builtin_define ("Cygwin");				\
+      builtin_define ("Posix");					\
+    } while (0)
+
 #undef CPP_SPEC
 #define CPP_SPEC "%(cpp_cpu) %{posix:-D_POSIX_SOURCE} \
   -D__CYGWIN32__ -D__CYGWIN__ %{!ansi:-Dunix} -D__unix__ -D__unix \
--- gcc/config/i386/linux-common.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/i386/linux-common.h	2013-07-04 18:30:34.000000000 +0200
@@ -27,6 +27,15 @@ along with GCC; see the file COPYING3.
     }                                          \
   while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS() 			\
+  do 						\
+    {						\
+      TARGET_GENERIC_LINUX_OS_D_BUILTINS();	\
+      ANDROID_TARGET_OS_D_BUILTINS();		\
+    }						\
+  while (0)
+
 #undef CC1_SPEC
 #define CC1_SPEC \
   LINUX_OR_ANDROID_CC (GNU_USER_TARGET_CC1_SPEC, \
--- gcc/config/i386/mingw32.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/i386/mingw32.h	2013-07-04 18:30:35.000000000 +0200
@@ -53,6 +53,18 @@ along with GCC; see the file COPYING3.
     }								\
   while (0)
 
+#define TARGET_OS_D_BUILTINS() TARGET_GENERIC_MINGW_OS_D_BUILTINS()
+#define TARGET_GENERIC_MINGW_OS_D_BUILTINS()			\
+    do {							\
+      builtin_define ("Windows");				\
+      builtin_define ("MinGW");					\
+								\
+      if (TARGET_64BIT && ix86_abi == MS_ABI)			\
+	  builtin_define ("Win64");				\
+      else if (!TARGET_64BIT)					\
+        builtin_define ("Win32");				\
+    } while (0)
+
 #ifndef TARGET_USE_PTHREAD_BY_DEFAULT
 #define SPEC_PTHREAD1 "pthread"
 #define SPEC_PTHREAD2 "!no-pthread"
--- gcc/config/i386/mingw-w64.h	2013-07-04 20:28:59.560658513 +0200
+++ gcc/config/i386/mingw-w64.h	2013-07-04 18:30:35.000000000 +0200
@@ -84,3 +84,10 @@ along with GCC; see the file COPYING3.
   %{static:-Bstatic} %{!static:-Bdynamic} \
   %{shared|mdll: " SUB_LINK_ENTRY " --enable-auto-image-base} \
   %(shared_libgcc_undefs)"
+
+#undef TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+      TARGET_GENERIC_MINGW_OS_D_BUILTINS();			\
+      builtin_define ("GNU_MinGW64");				\
+    } while (0)
--- gcc/config/kfreebsd-gnu.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/kfreebsd-gnu.h	2013-07-04 18:30:36.000000000 +0200
@@ -29,6 +29,14 @@ along with GCC; see the file COPYING3.
     }						\
   while (0)
 
+#undef TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("FreeBSD");				\
+	builtin_define ("Posix");				\
+	builtin_define ("GNU_GLibc");				\
+    } while (0)
+
 #define GNU_USER_DYNAMIC_LINKER        GLIBC_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER32      GLIBC_DYNAMIC_LINKER32
 #define GNU_USER_DYNAMIC_LINKER64      GLIBC_DYNAMIC_LINKER64
--- gcc/config/knetbsd-gnu.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/knetbsd-gnu.h	2013-07-04 18:30:36.000000000 +0200
@@ -30,6 +30,16 @@ along with GCC; see the file COPYING3.
     }						\
   while (0)
 
+#undef TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()			\
+  do						\
+    {						\
+      builtin_define ("NetBSD");		\
+      builtin_define ("Posix");			\
+      builtin_define ("GNU_GLibc");		\
+    }						\
+  while (0)
+
 
 #undef GNU_USER_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER "/lib/ld.so.1"
--- gcc/config/kopensolaris-gnu.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/kopensolaris-gnu.h	2013-07-04 18:30:36.000000000 +0200
@@ -30,5 +30,15 @@ along with GCC; see the file COPYING3.
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()				\
+  do							\
+    {							\
+	builtin_define ("Solaris");			\
+	builtin_define ("Posix");			\
+	builtin_define ("GNU_OpenSolaris");		\
+	builtin_define ("GNU_GLibc");			\
+    }							\
+  while (0)
+
 #undef GNU_USER_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER "/lib/ld.so.1"
--- gcc/config/sol2.h	2014-06-13 22:32:59.642262618 -0500
+++ gcc/config/sol2.h	2014-06-13 22:35:08.125209793 -0500
@@ -98,6 +98,14 @@
 	TARGET_SUB_OS_CPP_BUILTINS();			\
     } while (0)
 
+#define	TARGET_OS_D_BUILTINS()				\
+  do							\
+    {							\
+      builtin_define ("Solaris");			\
+      builtin_define ("Posix");				\
+    }							\
+  while (0)
+
 #define SUBTARGET_OVERRIDE_OPTIONS			\
   do {							\
     solaris_override_options ();			\
--- gcc/config/linux-android.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/linux-android.h	2013-07-04 18:30:36.000000000 +0200
@@ -25,6 +25,12 @@
 	  builtin_define ("__ANDROID__");			\
     } while (0)
 
+#define ANDROID_TARGET_OS_D_BUILTINS()				\
+    do {							\
+	if (TARGET_ANDROID)					\
+	  builtin_define ("Android");				\
+    } while (0)
+
 #if ANDROID_DEFAULT
 # define NOANDROID "mno-android"
 #else
--- gcc/config/linux.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/linux.h	2013-07-04 18:30:36.000000000 +0200
@@ -49,6 +49,20 @@ see the files COPYING3 and COPYING.RUNTI
 	builtin_assert ("system=posix");			\
     } while (0)
 
+#define TARGET_OS_D_BUILTINS() TARGET_GENERIC_LINUX_OS_D_BUILTINS()
+#define TARGET_GENERIC_LINUX_OS_D_BUILTINS()			\
+    do {							\
+	if (OPTION_GLIBC)					\
+	  builtin_define ("GNU_GLibc");				\
+	else if (OPTION_UCLIBC)					\
+	  builtin_define ("GNU_UCLibc");			\
+	else if (OPTION_BIONIC)					\
+	  builtin_define ("GNU_Bionic");			\
+								\
+	builtin_define ("linux");				\
+	builtin_define ("Posix");				\
+    } while (0)
+
 /* Determine which dynamic linker to use depending on whether GLIBC or
    uClibc or Bionic is the default C library and whether
    -muclibc or -mglibc or -mbionic has been passed to change the default.  */
--- gcc/config/mips/linux-common.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/mips/linux-common.h	2013-07-04 18:30:36.000000000 +0200
@@ -27,6 +27,15 @@ along with GCC; see the file COPYING3.
     ANDROID_TARGET_OS_CPP_BUILTINS();				\
   } while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS() 			\
+  do 						\
+    {						\
+      TARGET_GENERIC_LINUX_OS_D_BUILTINS();	\
+      ANDROID_TARGET_OS_D_BUILTINS();		\
+    }						\
+  while (0)
+
 #undef  LINK_SPEC
 #define LINK_SPEC							\
   LINUX_OR_ANDROID_LD (GNU_USER_TARGET_LINK_SPEC,			\
--- gcc/config/netbsd.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/netbsd.h	2013-07-04 18:30:36.000000000 +0200
@@ -29,6 +29,14 @@ along with GCC; see the file COPYING3.
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()			\
+  do						\
+    {						\
+      builtin_define ("NetBSD");		\
+      builtin_define ("Posix");			\
+    }						\
+  while (0)
+
 /* CPP_SPEC parts common to all NetBSD targets.  */
 #define NETBSD_CPP_SPEC				\
   "%{posix:-D_POSIX_SOURCE} \
--- gcc/config/openbsd.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/openbsd.h	2013-07-04 18:30:36.000000000 +0200
@@ -84,6 +84,14 @@ along with GCC; see the file COPYING3.
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()			\
+  do						\
+    {						\
+      builtin_define ("OpenBSD");		\
+      builtin_define ("Posix");			\
+    }						\
+  while (0)
+
 /* TARGET_OS_CPP_BUILTINS() common to all OpenBSD ELF targets.  */
 #define OPENBSD_OS_CPP_BUILTINS_ELF()		\
   do						\
--- gcc/config/rs6000/aix.h	2013-07-04 20:28:59.561658533 +0200
+++ gcc/config/rs6000/aix.h	2013-07-04 18:30:36.000000000 +0200
@@ -110,6 +110,13 @@
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+								\
+	builtin_define ("AIX");					\
+	builtin_define ("Posix");				\
+    } while (0)
+
 /* Define appropriate architecture macros for preprocessor depending on
    target switches.  */
 
--- gcc/config/rs6000/linux64.h	2013-07-04 20:28:59.562658553 +0200
+++ gcc/config/rs6000/linux64.h	2013-07-04 20:11:16.000000000 +0200
@@ -333,6 +333,17 @@ extern int dot_symbols;
     }							\
   while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()				\
+  do							\
+    {							\
+      builtin_define ("linux");				\
+      builtin_define ("Posix");				\
+      if (OPTION_GLIBC)					\
+	builtin_define ("GNU_GLibc");			\
+    }							\
+  while (0)
+
 #undef  CPP_OS_DEFAULT_SPEC
 #define CPP_OS_DEFAULT_SPEC "%(cpp_os_linux)"
 
--- gcc/config/rs6000/linux.h	2013-07-04 20:28:59.562658553 +0200
+++ gcc/config/rs6000/linux.h	2013-07-04 20:12:15.000000000 +0200
@@ -52,6 +52,17 @@
     }						\
   while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()			\
+  do						\
+    {						\
+      builtin_define ("linux");			\
+      builtin_define ("Posix");			\
+      if (OPTION_GLIBC)				\
+	builtin_define ("GNU_GLibc");		\
+    }						\
+  while (0)
+
 #undef	CPP_OS_DEFAULT_SPEC
 #define CPP_OS_DEFAULT_SPEC "%(cpp_os_linux)"
 
