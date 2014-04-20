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

--- gcc/config/alpha/linux.h
+++ gcc/config/alpha/linux.h
@@ -33,6 +33,16 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/arm/linux-eabi.h
+++ gcc/config/arm/linux-eabi.h
@@ -30,6 +30,15 @@
     }						\
   while (false)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS() 			\
+  do 						\
+    {						\
+      TARGET_BASIC_LINUX_OS_D_BUILTINS();	\
+      ANDROID_TARGET_OS_D_BUILTINS();		\
+    }						\
+  while (false)
+
 /* We default to a soft-float ABI so that binaries can run on all
    target hardware.  If you override this to use the hard-float ABI then
    change the setting of GLIBC_DYNAMIC_LINKER_DEFAULT as well.  */
--- gcc/config/darwin.h
+++ gcc/config/darwin.h
@@ -918,4 +918,10 @@ extern void darwin_driver_init (unsigned int *,struct cl_decoded_option **);
    providing an osx-version-min of this unless overridden by the User.  */
 #define DEF_MIN_OSX_VERSION "10.4"
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("OSX");					\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #endif /* CONFIG_DARWIN_H */
--- gcc/config/freebsd.h
+++ gcc/config/freebsd.h
@@ -32,6 +32,13 @@ along with GCC; see the file COPYING3.  If not see
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
 
--- gcc/config/gnu.h
+++ gcc/config/gnu.h
@@ -39,3 +39,11 @@ along with GCC.  If not, see <http://www.gnu.org/licenses/>.
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
--- gcc/config/i386/cygwin.h
+++ gcc/config/i386/cygwin.h
@@ -29,6 +29,13 @@ along with GCC; see the file COPYING3.  If not see
     }								\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+      builtin_define ("Windows");				\
+      builtin_define ("Cygwin");				\
+      builtin_define ("Posix");					\
+    } while (0)
+
 #undef CPP_SPEC
 #define CPP_SPEC "%(cpp_cpu) %{posix:-D_POSIX_SOURCE} \
   %{!ansi:-Dunix} \
--- gcc/config/i386/linux-common.h
+++ gcc/config/i386/linux-common.h
@@ -27,6 +27,15 @@ along with GCC; see the file COPYING3.  If not see
     }                                          \
   while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS() 			\
+  do 						\
+    {						\
+      TARGET_BASIC_LINUX_OS_D_BUILTINS();	\
+      ANDROID_TARGET_OS_D_BUILTINS();		\
+    }						\
+  while (0)
+
 #undef CC1_SPEC
 #define CC1_SPEC \
   LINUX_OR_ANDROID_CC (GNU_USER_TARGET_CC1_SPEC, \
--- gcc/config/i386/mingw-w64.h
+++ gcc/config/i386/mingw-w64.h
@@ -84,3 +84,10 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/i386/mingw32.h
+++ gcc/config/i386/mingw32.h
@@ -53,6 +53,18 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/kfreebsd-gnu.h
+++ gcc/config/kfreebsd-gnu.h
@@ -29,6 +29,14 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/knetbsd-gnu.h
+++ gcc/config/knetbsd-gnu.h
@@ -30,6 +30,16 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/kopensolaris-gnu.h
+++ gcc/config/kopensolaris-gnu.h
@@ -30,5 +30,15 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/linux-android.h
+++ gcc/config/linux-android.h
@@ -25,6 +25,14 @@
 	  builtin_define ("__ANDROID__");			\
     } while (0)
 
+#define ANDROID_TARGET_OS_D_BUILTINS()				\
+    do {							\
+	if (TARGET_ANDROID)					\
+	  builtin_define ("Android");				\
+	else							\
+	  builtin_define ("linux");				\
+    } while (0)
+
 #if ANDROID_DEFAULT
 # define NOANDROID "mno-android"
 #else
--- gcc/config/linux.h
+++ gcc/config/linux.h
@@ -49,6 +49,24 @@
 	builtin_assert ("system=posix");			\
     } while (0)
 
+#define TARGET_OS_D_BUILTINS() TARGET_GENERIC_LINUX_OS_D_BUILTINS()
+#define TARGET_GENERIC_LINUX_OS_D_BUILTINS()			\
+    do {							\
+	builtin_define ("linux");				\
+	TARGET_BASIC_LINUX_OS_D_BUILTINS();			\
+    } while (0)
+#define TARGET_BASIC_LINUX_OS_D_BUILTINS()			\
+    do {							\
+	if (OPTION_GLIBC)					\
+	  builtin_define ("GNU_GLibc");				\
+	else if (OPTION_UCLIBC)					\
+	  builtin_define ("GNU_UCLibc");			\
+	else if (OPTION_BIONIC)					\
+	  builtin_define ("GNU_Bionic");			\
+								\
+	builtin_define ("Posix");				\
+    } while (0)
+
 /* Determine which dynamic linker to use depending on whether GLIBC or
    uClibc or Bionic is the default C library and whether
    -muclibc or -mglibc or -mbionic has been passed to change the default.  */
--- gcc/config/mips/linux-common.h
+++ gcc/config/mips/linux-common.h
@@ -27,6 +27,15 @@ along with GCC; see the file COPYING3.  If not see
     ANDROID_TARGET_OS_CPP_BUILTINS();				\
   } while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS() 			\
+  do 						\
+    {						\
+      TARGET_BASIC_LINUX_OS_D_BUILTINS();	\
+      ANDROID_TARGET_OS_D_BUILTINS();		\
+    }						\
+  while (0)
+
 #undef  LINK_SPEC
 #define LINK_SPEC							\
   LINUX_OR_ANDROID_LD (GNU_USER_TARGET_LINK_SPEC,			\
--- gcc/config/netbsd.h
+++ gcc/config/netbsd.h
@@ -29,6 +29,14 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/openbsd.h
+++ gcc/config/openbsd.h
@@ -84,6 +84,14 @@ along with GCC; see the file COPYING3.  If not see
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
--- gcc/config/rs6000/aix.h
+++ gcc/config/rs6000/aix.h
@@ -115,6 +115,13 @@
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
 
--- gcc/config/rs6000/linux.h
+++ gcc/config/rs6000/linux.h
@@ -53,6 +53,17 @@
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
 
--- gcc/config/rs6000/linux64.h
+++ gcc/config/rs6000/linux64.h
@@ -340,6 +340,17 @@ extern int dot_symbols;
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
 
