This implements the following versions:
* D_HardFloat
* D_SoftFloat

for all supported architectures. And these where appropriate:
* ARM
** Thumb (deprecated)
** ARM_Thumb
** ARM_HardFloat
** ARM_SoftFloat
** ARM_SoftFP
* AArch64
* Alpha
** Alpha_SoftFloat
** Alpha_HardFloat
* Epiphany
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
* NVPTX
* NVPTX64
* HPPA
* HPPA64
* RISCV32
* RISCV64
* PPC
* PPC64
** PPC_HardFloat
** PPC_SoftFloat
* S390
* S390X (deprecated)
* SystemZ
* SH
* SPARC
* SPARC64
* SPARC_V8Plus
** SPARC_HardFloat
** SPARC_SoftFloat
---

--- a/gcc/config/aarch64/aarch64.h
+++ b/gcc/config/aarch64/aarch64.h
@@ -26,6 +26,14 @@
 #define TARGET_CPU_CPP_BUILTINS()	\
   aarch64_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      builtin_define ("AArch64");			\
+      builtin_define ("D_HardFloat");			\
+    } while (0)
+
 
 
 #define REGISTER_TARGET_PRAGMAS() aarch64_register_pragmas ()
--- a/gcc/config/alpha/alpha.h
+++ b/gcc/config/alpha/alpha.h
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
--- a/gcc/config/arm/arm.h
+++ b/gcc/config/arm/arm.h
@@ -47,6 +47,34 @@ extern char arm_arch_name[];
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() arm_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+	builtin_define ("ARM");				\
+							\
+	if (TARGET_THUMB || TARGET_THUMB2)		\
+	  {						\
+	    builtin_define ("Thumb");			\
+	    builtin_define ("ARM_Thumb");		\
+	  }						\
+							\
+	if (TARGET_HARD_FLOAT_ABI)			\
+	  builtin_define ("ARM_HardFloat");		\
+	else						\
+	  {						\
+	    if (TARGET_SOFT_FLOAT)			\
+	      builtin_define ("ARM_SoftFloat");		\
+	    else if (TARGET_HARD_FLOAT)			\
+	      builtin_define ("ARM_SoftFP");		\
+	  }						\
+							\
+	if (TARGET_SOFT_FLOAT)				\
+	  builtin_define ("D_SoftFloat");		\
+	else if (TARGET_HARD_FLOAT)			\
+	  builtin_define ("D_HardFloat");		\
+    } while (0)
+
 #include "config/arm/arm-opts.h"
 
 /* The processor for which instructions should be scheduled.  */
--- a/gcc/config/epiphany/epiphany.h
+++ b/gcc/config/epiphany/epiphany.h
@@ -41,6 +41,14 @@ along with GCC; see the file COPYING3.  If not see
 	builtin_assert ("machine=epiphany");	\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      builtin_define ("Epiphany");			\
+      builtin_define ("D_HardFloat");			\
+    } while (0)
+
 /* Pick up the libgloss library. One day we may do this by linker script, but
    for now its static.
    libgloss might use errno/__errno, which might not have been needed when we
--- a/gcc/config/ia64/ia64.h
+++ b/gcc/config/ia64/ia64.h
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
--- a/gcc/config/mips/mips.h
+++ b/gcc/config/mips/mips.h
@@ -644,6 +644,39 @@ struct mips_cpu_info {
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
+    if (mips_abi == ABI_32)						\
+      builtin_define("MIPS_O32");					\
+    else if (mips_abi == ABI_EABI)					\
+      builtin_define("MIPS_EABI");					\
+    else if (mips_abi == ABI_N32)					\
+      builtin_define("MIPS_N32");					\
+    else if (mips_abi == ABI_64)					\
+      builtin_define("MIPS_N64");					\
+    else if (mips_abi == ABI_O64)					\
+      builtin_define("MIPS_O64");					\
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
--- a/gcc/config/nvptx/nvptx.h
+++ b/gcc/config/nvptx/nvptx.h
@@ -37,6 +37,16 @@
         builtin_define ("__nvptx_unisimt__");	\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+  do						\
+    {						\
+      if (TARGET_ABI64)				\
+        builtin_define ("NVPTX64");		\
+      else					\
+        builtin_define ("NVPTX");		\
+    } while (0)
+
 /* Avoid the default in ../../gcc.c, which adds "-pthread", which is not
    supported for nvptx.  */
 #define GOMP_SELF_SPECS ""
--- a/gcc/config/pa/pa.h
+++ b/gcc/config/pa/pa.h
@@ -179,6 +179,20 @@ do {								\
        builtin_define("_PA_RISC1_0");				\
 } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()					\
+do {								\
+     if (TARGET_64BIT)						\
+       builtin_define("HPPA64");				\
+     else							\
+       builtin_define("HPPA");					\
+								\
+     if (TARGET_SOFT_FLOAT)					\
+       builtin_define ("D_SoftFloat");				\
+     else							\
+       builtin_define ("D_HardFloat");				\
+} while (0)
+
 /* An old set of OS defines for various BSD-like systems.  */
 #define TARGET_OS_CPP_BUILTINS()				\
   do								\
--- a/gcc/config/riscv/riscv.h
+++ b/gcc/config/riscv/riscv.h
@@ -27,6 +27,21 @@ along with GCC; see the file COPYING3.  If not see
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() riscv_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      if (TARGET_64BIT)					\
+        builtin_define ("RISCV64");			\
+      else						\
+        builtin_define ("RISCV32");			\
+							\
+      if (TARGET_HARDFLOAT)				\
+        builtin_define ("D_HardFloat");			\
+      else						\
+        builtin_define ("D_SoftFloat");			\
+    } while (0)
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- a/gcc/config/rs6000/rs6000.h
+++ b/gcc/config/rs6000/rs6000.h
@@ -822,6 +822,28 @@ extern unsigned char rs6000_recip_bits[];
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
--- a/gcc/config/s390/s390.h
+++ b/gcc/config/s390/s390.h
@@ -194,6 +194,24 @@ enum processor_flags
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() s390_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()				\
+  do							\
+    {							\
+      if (TARGET_ZARCH)					\
+        builtin_define ("SystemZ");			\
+      else if (TARGET_64BIT)				\
+        builtin_define ("S390X");			\
+      else						\
+        builtin_define ("S390");			\
+							\
+      if (TARGET_SOFT_FLOAT)				\
+        builtin_define ("D_SoftFloat");			\
+      else if (TARGET_HARD_FLOAT)			\
+        builtin_define ("D_HardFloat");			\
+    }							\
+  while (0)
+
 #ifdef DEFAULT_TARGET_64BIT
 #define TARGET_DEFAULT     (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP	\
                             | MASK_OPT_HTM | MASK_OPT_VX)
--- a/gcc/config/sh/sh.h
+++ b/gcc/config/sh/sh.h
@@ -31,6 +31,19 @@ extern int code_for_indirect_jump_scratch;
 
 #define TARGET_CPU_CPP_BUILTINS() sh_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_CPU_D_BUILTINS()			\
+  do						\
+    {						\
+      builtin_define ("SH");			\
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
--- a/gcc/config/sparc/sparc.h
+++ b/gcc/config/sparc/sparc.h
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
+      if (TARGET_V8PLUS)			\
+	builtin_define ("SPARC_V8Plus");	\
+						\
+      if (TARGET_FPU)				\
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
 
