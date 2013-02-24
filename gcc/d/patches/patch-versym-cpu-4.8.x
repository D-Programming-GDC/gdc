From 27ff9dc9d475b0e71d194818338106702434da0c Mon Sep 17 00:00:00 2001
From: Johannes Pfau <johannespfau@gmail.com>
Date: Fri, 1 Feb 2013 19:13:51 +0100
Subject: [PATCH] Implement D predefined CPU versions

This implements the following versions:
* D_HardFloat
* D_SoftFloat

for all supported architectures. And these where appropriate:
* ARM64
* Alpha
** Alpha_SoftFP
** Alpha_HardFP
* ARM
** ARM_Thumb
** ARM_HardFP
** ARM_SoftFP
** ARM_Soft
* X86
* X86_64
** D_X32
* IA64
* MIPS
** MIPS64
** MIPS32
** MIPS_O32
** MIPS_O64
** MIPS_N32
** MIPS_N64
** MIPS_EABI
** MIPS_NoFloat
** MIPS_HardFloat
** MIPS_SoftFloat
* HPPA
* HPPA64
* PPC
* PPC64
** PPC_HardFP
** PPC_SoftFP
* S390
* S390X
* SH
* SH64
* SPARC
* SPARC64
* SPARC_V8Plus
** SPARC_HardFP
** SPARC_SoftFP
---
 aarch64/aarch64.h |  8 ++++++++
 alpha/alpha.h     | 17 +++++++++++++++++
 arm/arm.h         | 25 +++++++++++++++++++++++++
 i386/i386.h       | 18 ++++++++++++++++++
 ia64/ia64.h       |  7 +++++++
 mips/mips.h       | 51 +++++++++++++++++++++++++++++++++++++++++++++++++++
 pa/pa.h           | 14 ++++++++++++++
 rs6000/rs6000.h   | 22 ++++++++++++++++++++++
 s390/s390.h       | 15 +++++++++++++++
 sh/sh.h           | 16 ++++++++++++++++
 sparc/sparc.h     | 25 +++++++++++++++++++++++++
 11 files changed, 218 insertions(+)

diff --git a/config/aarch64/aarch64.h b/config/aarch64/aarch64.h
index c3efd2a..60ed005 100644
--- a/config/aarch64/aarch64.h
+++ b/config/aarch64/aarch64.h
@@ -51,6 +51,14 @@
 							\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      builtin_define ("ARM64");				\
+      builtin_define ("D_HardFloat");			\
+    } while (0)
+
 
 
 /* Target machine storage layout.  */
diff --git a/config/alpha/alpha.h b/config/alpha/alpha.h
index 2e7c078..6eeeded 100644
--- a/config/alpha/alpha.h
+++ b/config/alpha/alpha.h
@@ -72,6 +72,23 @@ along with GCC; see the file COPYING3.  If not see
 	SUBTARGET_LANGUAGE_CPP_BUILTINS();		\
 } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+	builtin_define ("Alpha");			\
+	if (TARGET_SOFT_FP)				\
+	  {						\
+	    builtin_define ("D_SoftFloat");		\
+	    builtin_define ("Alpha_SoftFP");		\
+	  }						\
+	else						\
+	  {						\
+	    builtin_define ("D_HardFloat");		\
+	    builtin_define ("Alpha_HardFP");		\
+	  }						\
+} while (0)
+
 #ifndef SUBTARGET_LANGUAGE_CPP_BUILTINS
 #define SUBTARGET_LANGUAGE_CPP_BUILTINS()		\
   do							\
diff --git a/config/arm/arm.h b/config/arm/arm.h
index 6d336e8..cbf1cac 100644
--- a/config/arm/arm.h
+++ b/config/arm/arm.h
@@ -158,6 +158,31 @@ extern char arm_arch_name[];
 	  builtin_define ("__ARM_ARCH_EXT_IDIV__");	\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+	builtin_define ("ARM");				\
+							\
+	if (TARGET_THUMB)				\
+	  builtin_define ("ARM_Thumb");			\
+							\
+	if (TARGET_HARD_FLOAT_ABI)			\
+	  builtin_define ("ARM_HardFP");		\
+	else						\
+	  {						\
+	    if(TARGET_SOFT_FLOAT)			\
+	      builtin_define ("ARM_Soft");		\
+	    else if(TARGET_HARD_FLOAT)			\
+	      builtin_define ("ARM_SoftFP");		\
+	  }						\
+							\
+	if(TARGET_SOFT_FLOAT)				\
+	  builtin_define ("D_SoftFloat");		\
+	else if(TARGET_HARD_FLOAT)			\
+	  builtin_define ("D_HardFloat");		\
+    } while (0)
+
 #include "config/arm/arm-opts.h"
 
 enum target_cpus
diff --git a/config/i386/i386.h b/config/i386/i386.h
index af293b4..4f28197 100644
--- a/config/i386/i386.h
+++ b/config/i386/i386.h
@@ -585,6 +585,24 @@ extern const char *host_detect_local_cpu (int argc, const char **argv);
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() ix86_target_macros ()
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+  do {						\
+    if (TARGET_64BIT)				\
+    {						\
+      builtin_define("X86_64");			\
+      if (TARGET_X32)				\
+        builtin_define("D_X32");		\
+    }						\
+    else					\
+      builtin_define("X86");			\
+						\
+    if (TARGET_80387)				\
+      builtin_define("D_HardFloat");		\
+    else					\
+      builtin_define("D_SoftFloat");		\
+  } while (0)
+
 /* Target Pragmas.  */
 #define REGISTER_TARGET_PRAGMAS() ix86_register_pragmas ()
 
diff --git a/config/ia64/ia64.h b/config/ia64/ia64.h
index ae9027c..f29406a 100644
--- a/config/ia64/ia64.h
+++ b/config/ia64/ia64.h
@@ -40,6 +40,13 @@ do {						\
 	  builtin_define("__BIG_ENDIAN__");	\
 } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+do {						\
+	builtin_define ("IA64");		\
+	builtin_define ("D_HardFloat");		\
+} while (0)
+
 #ifndef SUBTARGET_EXTRA_SPECS
 #define SUBTARGET_EXTRA_SPECS
 #endif
diff --git a/config/mips/mips.h b/config/mips/mips.h
index 0acce14..afb6a8a 100644
--- a/config/mips/mips.h
+++ b/config/mips/mips.h
@@ -551,6 +551,57 @@ struct mips_cpu_info {
     }									\
   while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()						\
+  do									\
+  {									\
+    builtin_define("MIPS");						\
+    if (TARGET_64BIT)							\
+      builtin_define("MIPS64");						\
+    else								\
+      builtin_define("MIPS32");						\
+									\
+    switch (mips_abi)							\
+    {									\
+      case ABI_32:							\
+	builtin_define("MIPS_O32");					\
+	break;								\
+									\
+      case ABI_O64:							\
+	builtin_define("MIPS_O64");					\
+	break;								\
+									\
+      case ABI_N32:							\
+	builtin_define("MIPS_N32");					\
+	break;								\
+									\
+      case ABI_64:							\
+	builtin_define("MIPS_N64");					\
+	break;								\
+									\
+      case ABI_EABI:							\
+	builtin_define("MIPS_EABI");					\
+	break;								\
+									\
+      default:								\
+	gcc_unreachable();						\
+    }									\
+									\
+    if (TARGET_NO_FLOAT)						\
+      builtin_define("MIPS_NoFloat");					\
+    else if (TARGET_HARD_FLOAT_ABI)					\
+    {
+      builtin_define("MIPS_HardFloat");					\
+      builtin_define("D_HardFloat");					\
+    }
+    else								\
+    {
+      builtin_define("MIPS_SoftFloat");					\
+      builtin_define("D_SoftFloat");					\
+    }
+  }									\
+  while (0)								\
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
diff --git a/config/pa/pa.h b/config/pa/pa.h
index 216f737..0133a27 100644
--- a/config/pa/pa.h
+++ b/config/pa/pa.h
@@ -185,6 +185,20 @@ do {								\
        builtin_define("_PA_RISC1_0");				\
 } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()					\
+do {								\
+     if(TARGET_64BIT)						\
+       builtin_define("HPPA64");				\
+     else							\
+       builtin_define("HPPA");					\
+								\
+     if(TARGET_SOFT_FLOAT)					\
+       builtin_define ("D_SoftFloat");				\
+     else							\
+       builtin_define ("D_HardFloat");				\
+} while (0)
+
 /* An old set of OS defines for various BSD-like systems.  */
 #define TARGET_OS_CPP_BUILTINS()				\
   do								\
diff --git a/config/rs6000/rs6000.h b/config/rs6000/rs6000.h
index b015652..a16b467 100644
--- a/config/rs6000/rs6000.h
+++ b/config/rs6000/rs6000.h
@@ -607,6 +607,28 @@ extern unsigned char rs6000_recip_bits[];
 #define TARGET_CPU_CPP_BUILTINS() \
   rs6000_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+  do						\
+    {						\
+      if (TARGET_64BIT)				\
+	builtin_define ("PPC64");		\
+      else					\
+	builtin_define ("PPC");			\
+						\
+      if (TARGET_HARD_FLOAT)			\
+	{					\
+	  builtin_define ("PPC_HardFP");	\
+	  builtin_define ("D_HardFloat");	\
+	}					\
+      else if (TARGET_SOFT_FLOAT)		\
+	{					\
+	  builtin_define ("PPC_SoftFP");	\
+	  builtin_define ("D_SoftFloat");	\
+	}					\
+    }						\
+  while (0)
+
 /* This is used by rs6000_cpu_cpp_builtins to indicate the byte order
    we're compiling for.  Some configurations may need to override it.  */
 #define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
diff --git a/config/s390/s390.h b/config/s390/s390.h
index a9b7f5c..fcaf4b1 100644
--- a/config/s390/s390.h
+++ b/config/s390/s390.h
@@ -108,6 +108,21 @@ enum processor_flags
     }							\
   while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      if (TARGET_64BIT)					\
+        builtin_define ("S390X");			\
+      else						\
+        builtin_define ("S390");			\
+							\
+      if(TARGET_SOFT_FLOAT)				\
+        builtin_define ("D_SoftFloat");			\
+      else if(TARGET_HARD_FLOAT)			\
+        builtin_define ("D_HardFloat");			\
+  while (0)
+
 #ifdef DEFAULT_TARGET_64BIT
 #define TARGET_DEFAULT             (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP)
 #else
diff --git a/config/sh/sh.h b/config/sh/sh.h
index 89e6626..e7737fc 100644
--- a/config/sh/sh.h
+++ b/config/sh/sh.h
@@ -31,6 +31,22 @@ extern int code_for_indirect_jump_scratch;
 
 #define TARGET_CPU_CPP_BUILTINS() sh_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+  do						\
+    {						\
+      if (TARGET_SHMEDIA64)			\
+	builtin_define ("SH64");		\
+      else					\
+	builtin_define ("SH");			\
+						\
+      if (TARGET_FPU_ANY)			\
+	builtin_define ("D_HardFloat");		\
+      else					\
+	builtin_define ("D_SoftFloat");		\
+    }						\
+  while (0)
+
 /* Value should be nonzero if functions must have frame pointers.
    Zero means the frame pointer need not be set up (and parms may be accessed
    via the stack pointer) in functions that seem suitable.  */
diff --git a/config/sparc/sparc.h b/config/sparc/sparc.h
index 6b02b45..2d24f9d 100644
--- a/config/sparc/sparc.h
+++ b/config/sparc/sparc.h
@@ -27,6 +27,31 @@ along with GCC; see the file COPYING3.  If not see
 
 #define TARGET_CPU_CPP_BUILTINS() sparc_target_macros ()
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+do						\
+    {						\
+      if (TARGET_64BIT)				\
+	builtin_define ("SPARC64");		\
+      else					\
+	builtin_define ("SPARC");		\
+						\
+      if(TARGET_V8PLUS)				\
+	builtin_define ("SPARC_V8Plus");	\
+						\
+      if(TARGET_FPU)				\
+	{					\
+	  builtin_define ("D_HardFloat");	\
+	  builtin_define ("SPARC_HardFP");	\
+	}					\
+      else					\
+	{					\
+	  builtin_define ("D_SoftFloat");	\
+	  builtin_define ("SPARC_SoftFP");	\
+	}					\
+    }						\
+  while (0)
+
 /* Specify this in a cover file to provide bi-architecture (32/64) support.  */
 /* #define SPARC_BI_ARCH */
 
-- 
1.7.11.7

