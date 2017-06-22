This patch implements the following official versions:
* Windows
** Win32
** Win64
** Cygwin
** MinGW
* OSX
** darwin (deprecated)
* FreeBSD
* OpenBSD
* NetBSD
* DragonFlyBSD
* Solaris
* Posix
* AIX
* Hurd
* CRuntime_Glibc
---

--- a/gcc/config/darwin.h
+++ b/gcc/config/darwin.h
@@ -968,4 +968,11 @@ extern void darwin_driver_init (unsigned int *,struct cl_decoded_option **);
 #define DEF_LD64 LD64_VERSION
 #endif
 
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("OSX");					\
+	builtin_define ("darwin");				\
+	builtin_define ("Posix");				\
+    } while (0)
+
 #endif /* CONFIG_DARWIN_H */
--- a/gcc/config/dragonfly.h
+++ b/gcc/config/dragonfly.h
@@ -35,6 +35,15 @@ see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
     }                                       \
   while (0)
 
+#undef  TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()		    \
+  do					    \
+    {					    \
+       builtin_define ("DragonFlyBSD");	    \
+       builtin_define ("Posix");	    \
+    }                                       \
+  while (0)
+
 #undef  CPP_SPEC
 #define CPP_SPEC \
  "%(cpp_cpu) %(cpp_arch) %{posix:-D_POSIX_SOURCE}"
--- a/gcc/config/freebsd.h
+++ b/gcc/config/freebsd.h
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
 
--- a/gcc/config/gnu.h
+++ b/gcc/config/gnu.h
@@ -31,3 +31,11 @@ along with GCC.  If not, see <http://www.gnu.org/licenses/>.
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
--- a/gcc/config/i386/cygwin.h
+++ b/gcc/config/i386/cygwin.h
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
--- a/gcc/config/i386/mingw32.h
+++ b/gcc/config/i386/mingw32.h
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
--- a/gcc/config/kfreebsd-gnu.h
+++ b/gcc/config/kfreebsd-gnu.h
@@ -29,6 +29,14 @@ along with GCC; see the file COPYING3.  If not see
     }						\
   while (0)
 
+#undef TARGET_OS_D_BUILTINS
+#define TARGET_OS_D_BUILTINS()					\
+    do {							\
+	builtin_define ("FreeBSD");				\
+	builtin_define ("Posix");				\
+	builtin_define ("CRuntime_Glibc");			\
+    } while (0)
+
 #define GNU_USER_DYNAMIC_LINKER        GLIBC_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER32      GLIBC_DYNAMIC_LINKER32
 #define GNU_USER_DYNAMIC_LINKER64      GLIBC_DYNAMIC_LINKER64
--- a/gcc/config/kopensolaris-gnu.h
+++ b/gcc/config/kopensolaris-gnu.h
@@ -30,5 +30,14 @@ along with GCC; see the file COPYING3.  If not see
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()				\
+  do							\
+    {							\
+	builtin_define ("Solaris");			\
+	builtin_define ("Posix");			\
+	builtin_define ("CRuntime_Glibc");		\
+    }							\
+  while (0)
+
 #undef GNU_USER_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER "/lib/ld.so.1"
--- a/gcc/config/netbsd.h
+++ b/gcc/config/netbsd.h
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
--- a/gcc/config/openbsd.h
+++ b/gcc/config/openbsd.h
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
--- a/gcc/config/powerpcspe/aix.h
+++ b/gcc/config/powerpcspe/aix.h
@@ -164,6 +164,14 @@
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()			\
+    do						\
+      {						\
+	builtin_define ("AIX");			\
+	builtin_define ("Posix");		\
+      }						\
+    while (0)
+
 /* Define appropriate architecture macros for preprocessor depending on
    target switches.  */
 
--- a/gcc/config/rs6000/aix.h
+++ b/gcc/config/rs6000/aix.h
@@ -164,6 +164,14 @@
     }						\
   while (0)
 
+#define TARGET_OS_D_BUILTINS()			\
+    do						\
+      {						\
+	builtin_define ("AIX");			\
+	builtin_define ("Posix");		\
+      }						\
+    while (0)
+
 /* Define appropriate architecture macros for preprocessor depending on
    target switches.  */
 
