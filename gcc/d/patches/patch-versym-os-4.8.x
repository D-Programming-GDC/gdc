From ca4e858708b1f7d6700e40e51c1fe3075f145cbd Mon Sep 17 00:00:00 2001
From: Johannes Pfau <johannespfau@gmail.com>
Date: Sat, 2 Feb 2013 22:53:21 +0100
Subject: [PATCH] Implement D predefined OS versions

This implements the following official versions:
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
* SysV4
* Hurd
* Android

These gdc specific versions are also implemented:
* GNU_MinGW64 (for mingw-w64)
* GNU_OpenSolaris (for opensolaris)
* GNU_GLibc (implemented for linux & bsd & opensolaris)
* GNU_UCLibc (implemented for linux)
* GNU_Bionic (implemented for linux)

These official OS versions are not implemented:
* DragonFlyBSD
* BSD (other BSDs)
* Haiku
* SkyOS
* SysV3
---
 arm/linux-eabi.h    |  9 +++++++++
 darwin.h            |  6 ++++++
 freebsd.h           |  7 +++++++
 gnu.h               |  8 ++++++++
 i386/cygwin.h       |  7 +++++++
 i386/linux-common.h |  9 +++++++++
 i386/mingw-w64.h    |  7 +++++++
 i386/mingw32.h      | 12 ++++++++++++
 i386/sysv4.h        |  7 +++++++
 ia64/sysv4.h        |  7 +++++++
 kfreebsd-gnu.h      |  8 ++++++++
 knetbsd-gnu.h       | 10 ++++++++++
 kopensolaris-gnu.h  | 10 ++++++++++
 linux-android.h     |  6 ++++++
 linux.h             | 14 ++++++++++++++
 mips/linux-common.h |  9 +++++++++
 netbsd.h            |  8 ++++++++
 openbsd.h           |  8 ++++++++
 rs6000/aix.h        |  7 +++++++
 rs6000/sysv4.h      |  7 +++++++
 sparc/sysv4.h       |  7 +++++++
 21 files changed, 173 insertions(+)

diff --git a/config/arm/linux-eabi.h b/config/arm/linux-eabi.h
index 4a425c8..968930d 100644
--- a/config/arm/linux-eabi.h
+++ b/config/arm/linux-eabi.h
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
diff --git a/config/darwin.h b/config/darwin.h
index 1e7b9c7..300fd07 100644
--- a/config/darwin.h
+++ b/config/darwin.h
@@ -920,4 +920,10 @@ extern void darwin_driver_init (unsigned int *,struct cl_decoded_option **);
    providing an osx-version-min of this unless overridden by the User.  */
 #define DEF_MIN_OSX_VERSION "10.4"
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("OSX");					\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #endif /* CONFIG_DARWIN_H */
diff --git a/config/freebsd.h b/config/freebsd.h
index 87c0acf..c1e5248 100644
--- a/config/freebsd.h
+++ b/config/freebsd.h
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
 
diff --git a/config/gnu.h b/config/gnu.h
index 604ba8d..224ce8e 100644
--- a/config/gnu.h
+++ b/config/gnu.h
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
diff --git a/config/i386/cygwin.h b/config/i386/cygwin.h
index de0a3fd..90d255a 100644
--- a/config/i386/cygwin.h
+++ b/config/i386/cygwin.h
@@ -20,6 +20,13 @@ along with GCC; see the file COPYING3.  If not see
 
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
diff --git a/config/i386/linux-common.h b/config/i386/linux-common.h
index 1e8bf6b..cbad04e 100644
--- a/config/i386/linux-common.h
+++ b/config/i386/linux-common.h
@@ -27,6 +27,15 @@ along with GCC; see the file COPYING3.  If not see
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
diff --git a/config/i386/mingw-w64.h b/config/i386/mingw-w64.h
index 633009b..ef95b9a 100644
--- a/config/i386/mingw-w64.h
+++ b/config/i386/mingw-w64.h
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
diff --git a/config/i386/mingw32.h b/config/i386/mingw32.h
index 1ac5544..f693210 100644
--- a/config/i386/mingw32.h
+++ b/config/i386/mingw32.h
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
+        builtin_define_std ("Win32");				\
+    } while (0)
+
 #ifndef TARGET_USE_PTHREAD_BY_DEFAULT
 #define SPEC_PTHREAD1 "pthread"
 #define SPEC_PTHREAD2 "!no-pthread"
diff --git a/config/i386/sysv4.h b/config/i386/sysv4.h
index ff24575..0b90d39 100644
--- a/config/i386/sysv4.h
+++ b/config/i386/sysv4.h
@@ -70,3 +70,10 @@ along with GCC; see the file COPYING3.  If not see
 		   "|%0,_GLOBAL_OFFSET_TABLE_+(.-.LPR%=)}"		\
 	   : "=d"(BASE))
 #endif
+
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+								\
+	builtin_define ("SysV4");				\
+	builtin_define ("Posix");				\
+    } while (0)
diff --git a/config/ia64/sysv4.h b/config/ia64/sysv4.h
index e55551d..6230022 100644
--- a/config/ia64/sysv4.h
+++ b/config/ia64/sysv4.h
@@ -142,3 +142,10 @@ do {									\
 
 #define SDATA_SECTION_ASM_OP "\t.sdata"
 #define SBSS_SECTION_ASM_OP "\t.sbss"
+
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+								\
+	builtin_define ("SysV4");				\
+	builtin_define ("Posix");				\
+    } while (0)
diff --git a/config/kfreebsd-gnu.h b/config/kfreebsd-gnu.h
index 4f07a24..14e991f 100644
--- a/config/kfreebsd-gnu.h
+++ b/config/kfreebsd-gnu.h
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
diff --git a/config/knetbsd-gnu.h b/config/knetbsd-gnu.h
index 75d9e11..3ba5742 100644
--- a/config/knetbsd-gnu.h
+++ b/config/knetbsd-gnu.h
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
diff --git a/config/kopensolaris-gnu.h b/config/kopensolaris-gnu.h
index 1d800bb..43e420f 100644
--- a/config/kopensolaris-gnu.h
+++ b/config/kopensolaris-gnu.h
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
diff --git a/config/linux-android.h b/config/linux-android.h
index 2c87c84..73eea55 100644
--- a/config/linux-android.h
+++ b/config/linux-android.h
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
diff --git a/config/linux.h b/config/linux.h
index 2be1079..865ac93 100644
--- a/config/linux.h
+++ b/config/linux.h
@@ -49,6 +49,20 @@ see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
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
diff --git a/config/mips/linux-common.h b/config/mips/linux-common.h
index ca4ea07..d6a9931 100644
--- a/config/mips/linux-common.h
+++ b/config/mips/linux-common.h
@@ -27,6 +27,15 @@ along with GCC; see the file COPYING3.  If not see
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
diff --git a/config/netbsd.h b/config/netbsd.h
index 71c9183..e564007 100644
--- a/config/netbsd.h
+++ b/config/netbsd.h
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
diff --git a/config/openbsd.h b/config/openbsd.h
index 6537451..d8fbb6b 100644
--- a/config/openbsd.h
+++ b/config/openbsd.h
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
diff --git a/config/rs6000/aix.h b/config/rs6000/aix.h
index f81666a..5352f23 100644
--- a/config/rs6000/aix.h
+++ b/config/rs6000/aix.h
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
 
diff --git a/config/rs6000/sysv4.h b/config/rs6000/sysv4.h
index 54bdb21..c55470e 100644
--- a/config/rs6000/sysv4.h
+++ b/config/rs6000/sysv4.h
@@ -523,6 +523,13 @@ extern int fixuplabelno;
   while (0)
 #endif
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+								\
+	builtin_define ("SysV4");				\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #undef	ASM_SPEC
 #define	ASM_SPEC "%(asm_cpu) \
 %{,assembler|,assembler-with-cpp: %{mregnames} %{mno-regnames}} \
diff --git a/config/sparc/sysv4.h b/config/sparc/sysv4.h
index 1849414..4be48f0 100644
--- a/config/sparc/sysv4.h
+++ b/config/sparc/sysv4.h
@@ -117,3 +117,10 @@ do { ASM_OUTPUT_ALIGN ((FILE), Pmode == SImode ? 2 : 3);		\
 
 #undef MCOUNT_FUNCTION
 #define MCOUNT_FUNCTION "*_mcount"
+
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+								\
+	builtin_define ("SysV4");				\
+	builtin_define ("Posix");				\
+    } while (0)
-- 
1.7.11.7

