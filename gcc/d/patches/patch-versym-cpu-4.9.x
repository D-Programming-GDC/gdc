This implements the following versions:
* D_HardFloat
* D_SoftFloat

for all supported architectures. And these where appropriate:
* ARM
** ARM_Thumb
** ARM_HardFloat
** ARM_SoftFloat
** ARM_SoftFP
* AArch64
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

--- gcc/config/aarch64/aarch64.h	2013-07-23 13:33:14.000000000 +0100
+++ gcc/config/aarch64/aarch64.h	2013-09-01 11:02:44.857080581 +0100
@@ -56,6 +56,14 @@
 	}						\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      builtin_define ("AArch64");			\
+      builtin_define ("D_HardFloat");			\
+    } while (0)
+
 
 
 /* Target machine storage layout.  */
--- gcc/config/alpha/alpha.h	2013-01-10 20:38:27.000000000 +0000
+++ gcc/config/alpha/alpha.h	2013-09-01 11:02:44.861080581 +0100
@@ -72,6 +72,23 @@ along with GCC; see the file COPYING3.
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
--- gcc/config/arm/arm.h	2013-08-08 09:15:57.000000000 +0100
+++ gcc/config/arm/arm.h	2013-09-01 11:02:44.861080581 +0100
@@ -158,6 +158,31 @@ extern char arm_arch_name[];
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
--- gcc/config/i386/i386.h	2013-08-22 07:06:03.000000000 +0100
+++ gcc/config/i386/i386.h	2013-09-01 11:02:44.865080581 +0100
@@ -539,6 +539,24 @@ extern const char *host_detect_local_cpu
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
 
--- gcc/config/ia64/ia64.h	2013-01-10 20:38:27.000000000 +0000
+++ gcc/config/ia64/ia64.h	2013-09-01 11:02:44.865080581 +0100
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
--- gcc/config/mips/mips.h	2013-07-29 15:37:30.000000000 +0100
+++ gcc/config/mips/mips.h	2013-09-01 11:02:44.869080581 +0100
@@ -574,6 +574,54 @@ struct mips_cpu_info {
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
+    else if (TARGET_SOFT_FLOAT_ABI)					\
+    {									\
+      builtin_define("MIPS_SoftFloat");					\
+      builtin_define("D_SoftFloat");					\
+    }									\
+  }									\
+  while (0)
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- gcc/config/pa/pa.h	2013-05-05 21:47:24.000000000 +0100
+++ gcc/config/pa/pa.h	2013-09-01 11:02:44.873080581 +0100
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
--- gcc/config/rs6000/rs6000.h	2013-07-23 22:19:03.000000000 +0100
+++ gcc/config/rs6000/rs6000.h	2013-09-01 11:02:44.873080581 +0100
@@ -669,6 +669,28 @@ extern unsigned char rs6000_recip_bits[]
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
--- gcc/config/s390/s390.h	2013-07-17 12:15:24.000000000 +0100
+++ gcc/config/s390/s390.h	2013-09-01 11:02:44.877080581 +0100
@@ -114,6 +114,22 @@ enum processor_flags
     }									\
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
 #define TARGET_DEFAULT             (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP | MASK_OPT_HTM)
 #else
--- gcc/config/sh/sh.h	2013-04-22 08:16:21.000000000 +0100
+++ gcc/config/sh/sh.h	2013-09-01 11:02:44.877080581 +0100
@@ -31,6 +31,22 @@ extern int code_for_indirect_jump_scratc
 
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
--- gcc/config/sparc/sparc.h	2013-08-09 10:13:32.000000000 +0100
+++ gcc/config/sparc/sparc.h	2013-09-01 11:02:44.881080581 +0100
@@ -27,6 +27,31 @@ along with GCC; see the file COPYING3.
 
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
 
