This implements the following versions:
* D_HardFloat
* D_SoftFloat

for all supported architectures. And these where appropriate:
* ARM
** ARM_Thumb
** ARM_HardFloat
** ARM_SoftFloat
** ARM_SoftFP
* Alpha
** Alpha_SoftFloat
** Alpha_HardFloat
* X86
* X86_64
** D_X32
* IA64
* MIPS32
* MIPS64
** MIPS_O32
** MIPS_O64
** MIPS_N32
** MIPS_N64
** MIPS_EABI
** MIPS_HardFloat
** MIPS_SoftFloat
* HPPA
* HPPA64
* PPC
* PPC64
** PPC_HardFloat
** PPC_SoftFloat
* S390
* S390X
* SH
* SH64
* SPARC
* SPARC64
* SPARC_V8Plus
** SPARC_HardFloat
** SPARC_SoftFloat
---

--- gcc.orig/config/alpha/alpha.h	2013-03-27 14:22:29.790732704 +0100
+++ gcc/config/alpha/alpha.h	2013-03-27 14:17:20.346873301 +0100
@@ -74,6 +74,23 @@ along with GCC; see the file COPYING3.
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
+	    builtin_define ("Alpha_SoftFloat");		\
+	  }						\
+	else						\
+	  {						\
+	    builtin_define ("D_HardFloat");		\
+	    builtin_define ("Alpha_HardFloat");		\
+	  }						\
+} while (0)
+
 #ifndef SUBTARGET_LANGUAGE_CPP_BUILTINS
 #define SUBTARGET_LANGUAGE_CPP_BUILTINS()		\
   do							\
diff -urp gcc.orig/config/arm/arm.h gcc/config/arm/arm.h
--- gcc.orig/config/arm/arm.h	2013-03-27 14:22:29.751731966 +0100
+++ gcc/config/arm/arm.h	2013-03-27 14:18:38.374353590 +0100
@@ -109,6 +109,31 @@ extern char arm_arch_name[];
 	  builtin_define ("__ARM_ARCH_EXT_IDIV__");	\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+	builtin_define ("ARM");				\
+							\
+	if (TARGET_THUMB || TARGET_THUMB2)		\
+	  builtin_define ("ARM_Thumb");			\
+							\
+	if (TARGET_HARD_FLOAT_ABI)			\
+	  builtin_define ("ARM_HardFloat");		\
+	else						\
+	  {						\
+	    if(TARGET_SOFT_FLOAT)			\
+	      builtin_define ("ARM_SoftFloat");		\
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
diff -urp gcc.orig/config/i386/i386.h gcc/config/i386/i386.h
--- gcc.orig/config/i386/i386.h	2013-03-27 14:22:29.725731475 +0100
+++ gcc/config/i386/i386.h	2013-03-27 12:54:56.518631277 +0100
@@ -572,6 +572,24 @@ extern const char *host_detect_local_cpu
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
 
diff -urp gcc.orig/config/ia64/ia64.h gcc/config/ia64/ia64.h
--- gcc.orig/config/ia64/ia64.h	2013-03-27 14:22:29.719731361 +0100
+++ gcc/config/ia64/ia64.h	2013-03-27 12:54:56.520631314 +0100
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
diff -urp gcc.orig/config/mips/mips.h gcc/config/mips/mips.h
--- gcc.orig/config/mips/mips.h	2013-03-27 14:22:29.756732061 +0100
+++ gcc/config/mips/mips.h	2013-03-27 14:20:50.537856311 +0100
@@ -561,6 +561,54 @@ struct mips_cpu_info {
     }									\
   while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()						\
+  do									\
+  {									\
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
+    if (TARGET_HARD_FLOAT_ABI)						\
+    {									\
+      builtin_define("MIPS_HardFloat");					\
+      builtin_define("D_HardFloat");					\
+    }									\
+    else if(TARGET_SOFT_FLOAT_ABI)					\
+    {									\
+      builtin_define("MIPS_SoftFloat");					\
+      builtin_define("D_SoftFloat");					\
+    }									\
+  }									\
+  while (0)
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
diff -urp gcc.orig/config/pa/pa.h gcc/config/pa/pa.h
--- gcc.orig/config/pa/pa.h	2013-03-27 14:22:29.792732741 +0100
+++ gcc/config/pa/pa.h	2013-03-27 12:54:56.525631405 +0100
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
diff -urp gcc.orig/config/rs6000/rs6000.h gcc/config/rs6000/rs6000.h
--- gcc.orig/config/rs6000/rs6000.h	2013-03-27 14:22:29.764732213 +0100
+++ gcc/config/rs6000/rs6000.h	2013-03-27 14:21:30.601614010 +0100
@@ -560,6 +560,28 @@ extern unsigned char rs6000_recip_bits[]
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
+	  builtin_define ("PPC_HardFloat");	\
+	  builtin_define ("D_HardFloat");	\
+	}					\
+      else if (TARGET_SOFT_FLOAT)		\
+	{					\
+	  builtin_define ("PPC_SoftFloat");	\
+	  builtin_define ("D_SoftFloat");	\
+	}					\
+    }						\
+  while (0)
+
 /* This is used by rs6000_cpu_cpp_builtins to indicate the byte order
    we're compiling for.  Some configurations may need to override it.  */
 #define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
diff -urp gcc.orig/config/s390/s390.h gcc/config/s390/s390.h
--- gcc.orig/config/s390/s390.h	2013-03-27 14:22:29.744731834 +0100
+++ gcc/config/s390/s390.h	2013-03-27 12:54:56.527631442 +0100
@@ -104,6 +104,22 @@ enum processor_flags
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
+    }							\
+  while (0)
+
 #ifdef DEFAULT_TARGET_64BIT
 #define TARGET_DEFAULT             (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP)
 #else
diff -urp gcc.orig/config/sh/sh.h gcc/config/sh/sh.h
--- gcc.orig/config/sh/sh.h	2013-03-27 14:22:29.727731512 +0100
+++ gcc/config/sh/sh.h	2013-03-27 12:54:56.528631460 +0100
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
diff -urp gcc.orig/config/sparc/sparc.h gcc/config/sparc/sparc.h
--- gcc.orig/config/sparc/sparc.h	2013-03-27 14:22:29.794732778 +0100
+++ gcc/config/sparc/sparc.h	2013-03-27 14:22:06.164286252 +0100
@@ -29,6 +29,31 @@ along with GCC; see the file COPYING3.
 
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
+	  builtin_define ("SPARC_HardFloat");	\
+	}					\
+      else					\
+	{					\
+	  builtin_define ("D_SoftFloat");	\
+	  builtin_define ("SPARC_SoftFloat");	\
+	}					\
+    }						\
+  while (0)
+
 /* Specify this in a cover file to provide bi-architecture (32/64) support.  */
 /* #define SPARC_BI_ARCH */
 
