From 35e899a91ac206ce45d46844e4fda202ece95909 Mon Sep 17 00:00:00 2001
From: Johannes Pfau <johannespfau@gmail.com>
Date: Sat, 23 Feb 2013 18:53:38 +0100
Subject: [PATCH] Implement D predefined CPU versions

This implements the following versions:
* D_HardFloat
* D_SoftFloat

for all supported architectures. And these where appropriate:
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

Note: AArch64 (ARM64) is not support in gdc-4.7.
Use gdc-4.8 for that.
---
 alpha/alpha.h   | 17 +++++++++++++++++
 arm/arm.h       | 25 +++++++++++++++++++++++++
 i386/i386.h     | 18 ++++++++++++++++++
 ia64/ia64.h     |  7 +++++++
 mips/mips.h     | 51 +++++++++++++++++++++++++++++++++++++++++++++++++++
 pa/pa.h         | 14 ++++++++++++++
 rs6000/rs6000.h | 22 ++++++++++++++++++++++
 s390/s390.h     | 15 +++++++++++++++
 sh/sh.h         | 16 ++++++++++++++++
 sparc/sparc.h   | 25 +++++++++++++++++++++++++
 10 files changed, 210 insertions(+)

diff --git a/config/alpha/alpha.h b/config/alpha/alpha.h
index 07ffa9f..5fd62bb 100644
--- a/config/alpha/alpha.h
+++ b/config/alpha/alpha.h
@@ -74,6 +74,23 @@ along with GCC; see the file COPYING3.  If not see
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
index 443d2ed..f3147d4 100644
--- a/config/arm/arm.h
+++ b/config/arm/arm.h
@@ -109,6 +109,31 @@ extern char arm_arch_name[];
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
index 3a49803..a605d5e 100644
--- a/config/i386/i386.h
+++ b/config/i386/i386.h
@@ -572,6 +572,24 @@ extern const char *host_detect_local_cpu (int argc, const char **argv);
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
index a3ccd6f..6dbcfea 100644
--- a/config/ia64/ia64.h
+++ b/config/ia64/ia64.h
@@ -41,6 +41,13 @@ do {						\
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
index 1c19f8b..890d317 100644
--- a/config/mips/mips.h
+++ b/config/mips/mips.h
@@ -561,6 +561,57 @@ struct mips_cpu_info {
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
index d977c64..2c6863f 100644
--- a/config/pa/pa.h
+++ b/config/pa/pa.h
@@ -187,6 +187,20 @@ do {								\
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
index 63cbdc8..5587841 100644
--- a/config/rs6000/rs6000.h
+++ b/config/rs6000/rs6000.h
@@ -560,6 +560,28 @@ extern unsigned char rs6000_recip_bits[];
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
index edc6399..49d0b45 100644
--- a/config/s390/s390.h
+++ b/config/s390/s390.h
@@ -104,6 +104,21 @@ enum processor_flags
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
index 10e87f8..3db6341 100644
--- a/config/sh/sh.h
+++ b/config/sh/sh.h
@@ -95,6 +95,22 @@ do { \
 		  ? "__LITTLE_ENDIAN__" : "__BIG_ENDIAN__"); \
 } while (0)
 
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
index a1919b4..8d15778 100644
--- a/config/sparc/sparc.h
+++ b/config/sparc/sparc.h
@@ -29,6 +29,31 @@ along with GCC; see the file COPYING3.  If not see
 
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

