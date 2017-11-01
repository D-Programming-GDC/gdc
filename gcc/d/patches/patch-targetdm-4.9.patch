This patch implements the support for the D language specific target hooks.

The following versions are available for all supported architectures.
* D_HardFloat
* D_SoftFloat

The following CPU versions are implemented:
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
* X86
* X86_64
** D_X32
* MIPS32
* MIPS64
** MIPS_O32
** MIPS_O64
** MIPS_N32
** MIPS_N64
** MIPS_EABI
** MIPS_HardFloat
** MIPS_SoftFloat
* PPC
* PPC64
** PPC_HardFloat
** PPC_SoftFloat
* S390
* S390X (deprecated)
* SystemZ
* SPARC
* SPARC64
* SPARC_V8Plus
** SPARC_HardFloat
** SPARC_SoftFloat

The following OS versions are implemented:
* linux
* Posix
* Hurd
* Android
* CRuntime_Bionic
* CRuntime_Glibc
* CRuntime_UClibc
---
 
--- a/gcc/Makefile.in
+++ b/gcc/Makefile.in
@@ -497,6 +497,8 @@ tm_include_list=@tm_include_list@
 tm_defines=@tm_defines@
 tm_p_file_list=@tm_p_file_list@
 tm_p_include_list=@tm_p_include_list@
+tm_d_file_list=@tm_d_file_list@
+tm_d_include_list=@tm_d_include_list@
 build_xm_file_list=@build_xm_file_list@
 build_xm_include_list=@build_xm_include_list@
 build_xm_defines=@build_xm_defines@
@@ -806,6 +808,7 @@ BCONFIG_H = bconfig.h $(build_xm_file_list)
 CONFIG_H  = config.h  $(host_xm_file_list)
 TCONFIG_H = tconfig.h $(xm_file_list)
 TM_P_H    = tm_p.h    $(tm_p_file_list)
+TM_D_H    = tm_d.h    $(tm_d_file_list)
 GTM_H     = tm.h      $(tm_file_list) insn-constants.h
 TM_H      = $(GTM_H) insn-flags.h $(OPTIONS_H)
 
@@ -856,9 +859,11 @@ EXCEPT_H = except.h $(HASHTAB_H)
 TARGET_DEF = target.def target-hooks-macros.h
 C_TARGET_DEF = c-family/c-target.def target-hooks-macros.h
 COMMON_TARGET_DEF = common/common-target.def target-hooks-macros.h
+D_TARGET_DEF = d/d-target.def target-hooks-macros.h
 TARGET_H = $(TM_H) target.h $(TARGET_DEF) insn-modes.h insn-codes.h
 C_TARGET_H = c-family/c-target.h $(C_TARGET_DEF)
 COMMON_TARGET_H = common/common-target.h $(INPUT_H) $(COMMON_TARGET_DEF)
+D_TARGET_H = d/d-target.h $(D_TARGET_DEF)
 MACHMODE_H = machmode.h mode-classes.def insn-modes.h
 HOOKS_H = hooks.h $(MACHMODE_H)
 HOSTHOOKS_DEF_H = hosthooks-def.h $(HOOKS_H)
@@ -1124,6 +1129,9 @@ C_TARGET_OBJS=@c_target_objs@
 # Target specific, C++ specific object file
 CXX_TARGET_OBJS=@cxx_target_objs@
 
+# Target specific, D specific object file
+D_TARGET_OBJS=@d_target_objs@
+
 # Target specific, Fortran specific object file
 FORTRAN_TARGET_OBJS=@fortran_target_objs@
 
@@ -1624,6 +1632,7 @@ bconfig.h: cs-bconfig.h ; @true
 tconfig.h: cs-tconfig.h ; @true
 tm.h: cs-tm.h ; @true
 tm_p.h: cs-tm_p.h ; @true
+tm_d.h: cs-tm_d.h ; @true
 
 cs-config.h: Makefile
 	TARGET_CPU_DEFAULT="" \
@@ -1650,6 +1659,11 @@ cs-tm_p.h: Makefile
 	HEADERS="$(tm_p_include_list)" DEFINES="" \
 	$(SHELL) $(srcdir)/mkconfig.sh tm_p.h
 
+cs-tm_d.h: Makefile
+	TARGET_CPU_DEFAULT="" \
+	HEADERS="$(tm_d_include_list)" DEFINES="" \
+	$(SHELL) $(srcdir)/mkconfig.sh tm_d.h
+
 # Don't automatically run autoconf, since configure.ac might be accidentally
 # newer than configure.  Also, this writes into the source directory which
 # might be on a read-only file system.  If configured for maintainer mode
@@ -1927,6 +1941,12 @@ default-c.o: config/default-c.c
 CFLAGS-prefix.o += -DPREFIX=\"$(prefix)\" -DBASEVER=$(BASEVER_s)
 prefix.o: $(BASEVER)
 
+# Files used by the D language front end.
+
+default-d.o: config/default-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 # Language-independent files.
 
 DRIVER_DEFINES = \
@@ -2193,6 +2213,15 @@ s-common-target-hooks-def-h: build/genhooks$(build_exeext)
 					     common/common-target-hooks-def.h
 	$(STAMP) s-common-target-hooks-def-h
 
+d/d-target-hooks-def.h: s-d-target-hooks-def-h; @true
+
+s-d-target-hooks-def-h: build/genhooks$(build_exeext)
+	$(RUN_GEN) build/genhooks$(build_exeext) "D Target Hook" \
+					     > tmp-d-target-hooks-def.h
+	$(SHELL) $(srcdir)/../move-if-change tmp-d-target-hooks-def.h \
+					     d/d-target-hooks-def.h
+	$(STAMP) s-d-target-hooks-def-h
+
 # check if someone mistakenly only changed tm.texi.
 # We use a different pathname here to avoid a circular dependency.
 s-tm-texi: $(srcdir)/doc/../doc/tm.texi
@@ -2216,6 +2245,7 @@ s-tm-texi: build/genhooks$(build_exeext) $(srcdir)/doc/tm.texi.in
 	  && ( test $(srcdir)/doc/tm.texi -nt $(srcdir)/target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/c-family/c-target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/common/common-target.def \
+	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/d/d-target.def \
 	  ); then \
 	  echo >&2 ; \
 	  echo You should edit $(srcdir)/doc/tm.texi.in rather than $(srcdir)/doc/tm.texi . >&2 ; \
@@ -2335,7 +2365,7 @@ generated_files = config.h tm.h $(TM_P_H) $(TM_H) multilib.h \
        $(ALL_GTFILES_H) gtype-desc.c gtype-desc.h gcov-iov.h \
        options.h target-hooks-def.h insn-opinit.h \
        common/common-target-hooks-def.h pass-instances.def \
-       c-family/c-target-hooks-def.h
+       c-family/c-target-hooks-def.h d/d-target-hooks-def.h
 
 #
 # How to compile object files to run on the build machine.
@@ -2468,7 +2498,7 @@ build/genpreds.o : genpreds.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
 build/genrecog.o : genrecog.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
   coretypes.h $(GTM_H) errors.h $(READ_MD_H) gensupport.h
 build/genhooks.o : genhooks.c $(TARGET_DEF) $(C_TARGET_DEF)		\
-  $(COMMON_TARGET_DEF) $(BCONFIG_H) $(SYSTEM_H) errors.h
+  $(COMMON_TARGET_DEF) $(D_TARGET_DEF) $(BCONFIG_H) $(SYSTEM_H) errors.h
 build/genmddump.o : genmddump.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
   coretypes.h $(GTM_H) errors.h $(READ_MD_H) gensupport.h
 
--- a/gcc/config.gcc
+++ b/gcc/config.gcc
@@ -86,6 +86,9 @@
 #  tm_p_file		Location of file with declarations for functions
 #			in $out_file.
 #
+#  tm_d_file		A list of headers with definitions of target hook
+#			macros for the D compiler.
+#
 #  out_file		The name of the machine description C support
 #			file, if different from "$cpu_type/$cpu_type.c".
 #
@@ -139,6 +142,9 @@
 #  cxx_target_objs	List of extra target-dependent objects that be
 #			linked into the C++ compiler only.
 #
+#  d_target_objs	List of extra target-dependent objects that be
+#			linked into the D compiler only.
+#
 #  fortran_target_objs	List of extra target-dependent objects that be
 #			linked into the fortran compiler only.
 #
@@ -198,6 +204,9 @@
 #
 #  target_has_targetm_common	Set to yes or no depending on whether the
 #			target has its own definition of targetm_common.
+#
+#  target_has_targetdm	Set to yes or no depending on whether the target
+#			has its own definition of targetdm.
 
 out_file=
 common_out_file=
@@ -213,9 +222,11 @@ extra_gcc_objs=
 extra_options=
 c_target_objs=
 cxx_target_objs=
+d_target_objs=
 fortran_target_objs=
 target_has_targetcm=no
 target_has_targetm_common=yes
+target_has_targetdm=no
 tm_defines=
 xm_defines=
 # Set this to force installation and use of collect2.
@@ -313,11 +324,13 @@ aarch64*-*-*)
 	cpu_type=aarch64
 	need_64bit_hwint=yes
 	extra_headers="arm_neon.h"
+	d_target_objs="aarch64-d.o"
 	extra_objs="aarch64-builtins.o aarch-common.o"
 	target_has_targetm_common=yes
 	;;
 alpha*-*-*)
 	cpu_type=alpha
+	d_target_objs="alpha-d.o"
 	need_64bit_hwint=yes
 	extra_options="${extra_options} g.opt"
 	;;
@@ -333,6 +346,7 @@ arm*-*-*)
 	extra_headers="mmintrin.h arm_neon.h arm_acle.h"
 	target_type_format_char='%'
 	c_target_objs="arm-c.o"
+	d_target_objs="arm-d.o"
 	cxx_target_objs="arm-c.o"
 	need_64bit_hwint=yes
 	extra_options="${extra_options} arm/arm-tables.opt"
@@ -382,6 +396,8 @@ x86_64-*-*)
 	cpu_type=i386
 	c_target_objs="i386-c.o"
 	cxx_target_objs="i386-c.o"
+	d_target_objs="i386-d.o"
+	d_target_objs="i386-d.o"
 	extra_options="${extra_options} fused-madd.opt"
 	extra_headers="cpuid.h mmintrin.h mm3dnow.h xmmintrin.h emmintrin.h
 		       pmmintrin.h tmmintrin.h ammintrin.h smmintrin.h
@@ -421,6 +437,7 @@ microblaze*-*-*)
         ;;
 mips*-*-*)
 	cpu_type=mips
+	d_target_objs="mips-d.o"
 	need_64bit_hwint=yes
 	extra_headers="loongson.h"
 	extra_options="${extra_options} g.opt mips/mips-tables.opt"
@@ -459,6 +476,7 @@ sparc*-*-*)
 	cpu_type=sparc
 	c_target_objs="sparc-c.o"
 	cxx_target_objs="sparc-c.o"
+	d_target_objs="sparc-d.o"
 	extra_headers="visintrin.h"
 	need_64bit_hwint=yes
 	;;
@@ -468,6 +486,7 @@ spu*-*-*)
 	;;
 s390*-*-*)
 	cpu_type=s390
+	d_target_objs="s390-d.o"
 	need_64bit_hwint=yes
 	extra_options="${extra_options} fused-madd.opt"
 	extra_headers="s390intrin.h htmintrin.h htmxlintrin.h"
@@ -501,10 +520,13 @@ tilepro*-*-*)
 esac
 
 tm_file=${cpu_type}/${cpu_type}.h
+tm_d_file=${cpu_type}/${cpu_type}.h
 if test -f ${srcdir}/config/${cpu_type}/${cpu_type}-protos.h
 then
 	tm_p_file=${cpu_type}/${cpu_type}-protos.h
+	tm_d_file="${tm_d_file} ${cpu_type}/${cpu_type}-protos.h"
 fi
+
 extra_modes=
 if test -f ${srcdir}/config/${cpu_type}/${cpu_type}-modes.def
 then
@@ -739,8 +761,10 @@ case ${target} in
   esac
   c_target_objs="${c_target_objs} glibc-c.o"
   cxx_target_objs="${cxx_target_objs} glibc-c.o"
+  d_target_objs="${d_target_objs} glibc-d.o"
   tmake_file="${tmake_file} t-glibc"
   target_has_targetcm=yes
+  target_has_targetdm=yes
   ;;
 *-*-netbsd*)
   tmake_file="t-slibgcc"
@@ -938,7 +962,7 @@ aarch64*-*-linux*)
 	;;
 alpha*-*-linux*)
 	tm_file="elfos.h ${tm_file} alpha/elf.h alpha/linux.h alpha/linux-elf.h glibc-stdint.h"
-	tmake_file="${tmake_file} alpha/t-linux"
+	tmake_file="${tmake_file} alpha/t-linux alpha/t-alpha"
 	extra_options="${extra_options} alpha/elf.opt"
 	;;
 alpha*-*-freebsd*)
@@ -2445,6 +2469,7 @@ s390-*-linux*)
 	if test x$enable_targets = xall; then
 		tmake_file="${tmake_file} s390/t-linux64"
 	fi
+	tmake_file="${tmake_file} s390/t-s390"
 	;;
 s390x-*-linux*)
 	default_gnu_indirect_function=yes
@@ -2453,7 +2478,7 @@ s390x-*-linux*)
 	md_file=s390/s390.md
 	extra_modes=s390/s390-modes.def
 	out_file=s390/s390.c
-	tmake_file="${tmake_file} s390/t-linux64"
+	tmake_file="${tmake_file} s390/t-linux64 s390/t-s390"
 	;;
 s390x-ibm-tpf*)
         tm_file="s390/s390x.h s390/s390.h dbxelf.h elfos.h s390/tpf.h"
@@ -2916,6 +2941,10 @@ if [ "$common_out_file" = "" ]; then
   fi
 fi
 
+if [ "$target_has_targetdm" = "no" ]; then
+  d_target_objs="$d_target_objs default-d.o"
+fi
+
 # Support for --with-cpu and related options (and a few unrelated options,
 # too).
 case ${with_cpu} in
@@ -4152,6 +4181,7 @@ case ${target} in
 		out_file=rs6000/rs6000.c
 		c_target_objs="${c_target_objs} rs6000-c.o"
 		cxx_target_objs="${cxx_target_objs} rs6000-c.o"
+		d_target_objs="${d_target_objs} rs6000-d.o"
 		tmake_file="rs6000/t-rs6000 ${tmake_file}"
 		;;
 
--- /dev/null
+++ b/gcc/config/aarch64/aarch64-d.c
@@ -0,0 +1,31 @@
+/* Subroutines for the D front end on the AArch64 architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for AArch64 targets.  */
+
+void
+aarch64_d_target_versions (void)
+{
+  d_add_builtin_version ("AArch64");
+  d_add_builtin_version ("D_HardFloat");
+}
--- a/gcc/config/aarch64/aarch64-linux.h
+++ b/gcc/config/aarch64/aarch64-linux.h
@@ -63,6 +63,8 @@
     }						\
   while (0)
 
+#define GNU_USER_TARGET_D_CRITSEC_SIZE 48
+
 #define TARGET_ASM_FILE_END file_end_indicate_exec_stack
 
 #endif  /* GCC_AARCH64_LINUX_H */
--- a/gcc/config/aarch64/aarch64-protos.h
+++ b/gcc/config/aarch64/aarch64-protos.h
@@ -291,4 +291,8 @@ extern bool aarch64_madd_needs_nop (rtx);
 extern void aarch64_final_prescan_insn (rtx);
 extern bool
 aarch64_expand_vec_perm_const (rtx target, rtx op0, rtx op1, rtx sel);
+
+/* Defined in aarch64-d.c  */
+extern void aarch64_d_target_versions (void);
+
 #endif /* GCC_AARCH64_PROTOS_H */
--- a/gcc/config/aarch64/aarch64.h
+++ b/gcc/config/aarch64/aarch64.h
@@ -61,6 +61,9 @@
 	builtin_define ("__ARM_FEATURE_CRYPTO");	\
     } while (0)
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS aarch64_d_target_versions
+
 
 
 /* Target machine storage layout.  */
--- a/gcc/config/aarch64/t-aarch64
+++ b/gcc/config/aarch64/t-aarch64
@@ -40,6 +40,10 @@ aarch-common.o: $(srcdir)/config/arm/aarch-common.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/arm/aarch-common.c
 
+aarch64-d.o: $(srcdir)/config/aarch64/aarch64-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 comma=,
 MULTILIB_OPTIONS    = $(subst $(comma),/, $(patsubst %, mabi=%, $(subst $(comma),$(comma)mabi=,$(TM_MULTILIB_CONFIG))))
 MULTILIB_DIRNAMES   = $(subst $(comma), ,$(TM_MULTILIB_CONFIG))
--- /dev/null
+++ b/gcc/config/alpha/alpha-d.c
@@ -0,0 +1,41 @@
+/* Subroutines for the D front end on the Alpha architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for Alpha targets.  */
+
+void
+alpha_d_target_versions (void)
+{
+  d_add_builtin_version ("Alpha");
+  if (TARGET_SOFT_FP)
+    {
+      d_add_builtin_version ("D_SoftFloat");
+      d_add_builtin_version ("Alpha_SoftFloat");
+    }
+  else
+    {
+      d_add_builtin_version ("D_HardFloat");
+      d_add_builtin_version ("Alpha_HardFloat");
+    }
+}
--- a/gcc/config/alpha/alpha-protos.h
+++ b/gcc/config/alpha/alpha-protos.h
@@ -115,3 +115,6 @@ extern rtx unicosmk_add_call_info_word (rtx);
 extern int some_small_symbolic_operand_int (rtx *, void *);
 extern int tls_symbolic_operand_1 (rtx, int, int);
 extern rtx resolve_reload_operand (rtx);
+
+/* Routines implemented in alpha-d.c  */
+extern void alpha_d_target_versions (void);
--- a/gcc/config/alpha/alpha.h
+++ b/gcc/config/alpha/alpha.h
@@ -94,6 +94,9 @@ along with GCC; see the file COPYING3.  If not see
   while (0)
 #endif
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS alpha_d_target_versions
+
 /* Run-time compilation parameters selecting different hardware subsets.  */
 
 /* Which processor to schedule for. The cpu attribute defines a list that
--- a/gcc/config/alpha/linux.h
+++ b/gcc/config/alpha/linux.h
@@ -33,6 +33,9 @@ along with GCC; see the file COPYING3.  If not see
 	  builtin_define ("_GNU_SOURCE");			\
     } while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()				\
+  builtin_version ("linux")
+
 #undef LIB_SPEC
 #define LIB_SPEC \
   "%{pthread:-lpthread} \
--- /dev/null
+++ b/gcc/config/alpha/t-alpha
@@ -0,0 +1,21 @@
+# Copyright (C) 2016 Free Software Foundation, Inc.
+#
+# This file is part of GCC.
+#
+# GCC is free software; you can redistribute it and/or modify
+# it under the terms of the GNU General Public License as published by
+# the Free Software Foundation; either version 3, or (at your option)
+# any later version.
+#
+# GCC is distributed in the hope that it will be useful,
+# but WITHOUT ANY WARRANTY; without even the implied warranty of
+# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+# GNU General Public License for more details.
+#
+# You should have received a copy of the GNU General Public License
+# along with GCC; see the file COPYING3.  If not see
+# <http://www.gnu.org/licenses/>.
+
+alpha-d.o: $(srcdir)/config/alpha/alpha-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/arm/arm-d.c
@@ -0,0 +1,53 @@
+/* Subroutines for the D front end on the ARM architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "tm_p.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for ARM targets.  */
+
+void
+arm_d_target_versions (void)
+{
+  d_add_builtin_version ("ARM");
+
+  if (TARGET_THUMB || TARGET_THUMB2)
+    {
+      d_add_builtin_version ("Thumb");
+      d_add_builtin_version ("ARM_Thumb");
+    }
+
+  if (TARGET_HARD_FLOAT_ABI)
+    d_add_builtin_version ("ARM_HardFloat");
+  else
+    {
+      if (TARGET_SOFT_FLOAT)
+	d_add_builtin_version ("ARM_SoftFloat");
+      else if (TARGET_HARD_FLOAT)
+	d_add_builtin_version ("ARM_SoftFP");
+    }
+
+  if (TARGET_SOFT_FLOAT)
+    d_add_builtin_version ("D_SoftFloat");
+  else if (TARGET_HARD_FLOAT)
+    d_add_builtin_version ("D_HardFloat");
+}
--- a/gcc/config/arm/arm-protos.h
+++ b/gcc/config/arm/arm-protos.h
@@ -296,6 +296,9 @@ extern void arm_emit_eabi_attribute (const char *, int, int);
 /* Defined in gcc/common/config/arm-common.c.  */
 extern const char *arm_rewrite_selected_cpu (const char *name);
 
+/* Defined in arm-d.c  */
+extern void arm_d_target_versions (void);
+
 extern bool arm_is_constant_pool_ref (rtx);
 
 #endif /* ! GCC_ARM_PROTOS_H */
--- a/gcc/config/arm/arm.h
+++ b/gcc/config/arm/arm.h
@@ -169,6 +169,9 @@ extern char arm_arch_name[];
 	  builtin_define ("__ARM_ARCH_EXT_IDIV__");	\
     } while (0)
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS arm_d_target_versions
+
 #include "config/arm/arm-opts.h"
 
 enum target_cpus
--- a/gcc/config/arm/linux-eabi.h
+++ b/gcc/config/arm/linux-eabi.h
@@ -30,6 +30,9 @@
     }						\
   while (false)
 
+#define EXTRA_TARGET_D_OS_VERSIONS()		\
+  ANDROID_TARGET_D_OS_VERSIONS();
+
 /* We default to a soft-float ABI so that binaries can run on all
    target hardware.  If you override this to use the hard-float ABI then
    change the setting of GLIBC_DYNAMIC_LINKER_DEFAULT as well.  */
--- a/gcc/config/arm/t-arm
+++ b/gcc/config/arm/t-arm
@@ -98,3 +98,7 @@ arm-c.o: $(srcdir)/config/arm/arm-c.c $(CONFIG_H) $(SYSTEM_H) \
     coretypes.h $(TM_H) $(TREE_H) output.h $(C_COMMON_H)
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/arm/arm-c.c
+
+arm-d.o: $(srcdir)/config/arm/arm-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/default-d.c
@@ -0,0 +1,25 @@
+/* Default D language target hooks initializer.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify it under
+the terms of the GNU General Public License as published by the Free
+Software Foundation; either version 3, or (at your option) any later
+version.
+
+GCC is distributed in the hope that it will be useful, but WITHOUT ANY
+WARRANTY; without even the implied warranty of MERCHANTABILITY or
+FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
+for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm_d.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
--- /dev/null
+++ b/gcc/config/glibc-d.c
@@ -0,0 +1,71 @@
+/* Glibc support needed only by D front-end.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify it under
+the terms of the GNU General Public License as published by the Free
+Software Foundation; either version 3, or (at your option) any later
+version.
+
+GCC is distributed in the hope that it will be useful, but WITHOUT ANY
+WARRANTY; without even the implied warranty of MERCHANTABILITY or
+FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
+for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "input.h"
+#include "tm.h"
+#include "tm_p.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_OS_VERSIONS for Glibc targets.  */
+
+static void
+glibc_d_os_builtins (void)
+{
+  if (OPTION_GLIBC)
+    d_add_builtin_version ("CRuntime_Glibc");
+  else if (OPTION_UCLIBC)
+    d_add_builtin_version ("CRuntime_UClibc");
+  else if (OPTION_BIONIC)
+    d_add_builtin_version ("CRuntime_Bionic");
+
+  d_add_builtin_version ("Posix");
+
+#define builtin_version(TXT) d_add_builtin_version (TXT)
+
+#ifdef GNU_USER_TARGET_D_OS_VERSIONS
+  GNU_USER_TARGET_D_OS_VERSIONS ();
+#endif
+
+#ifdef EXTRA_TARGET_D_OS_VERSIONS
+  EXTRA_TARGET_D_OS_VERSIONS ();
+#endif
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for Glibc targets.  */
+
+static unsigned
+glibc_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t.  */
+#ifdef GNU_USER_TARGET_D_CRITSEC_SIZE
+  return GNU_USER_TARGET_D_CRITSEC_SIZE;
+#else
+  return (POINTER_SIZE == 64) ? 40 : 24;
+#endif
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS glibc_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE glibc_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
--- a/gcc/config/gnu.h
+++ b/gcc/config/gnu.h
@@ -39,3 +39,6 @@ along with GCC.  If not, see <http://www.gnu.org/licenses/>.
 	builtin_assert ("system=unix");		\
 	builtin_assert ("system=posix");	\
     } while (0)
+
+#define GNU_USER_TARGET_D_OS_VERSIONS()		\
+  builtin_version ("Hurd")
--- /dev/null
+++ b/gcc/config/i386/i386-d.c
@@ -0,0 +1,44 @@
+/* Subroutines for the D front end on the x86 architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for x86 targets.  */
+
+void
+ix86_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    {
+      d_add_builtin_version ("X86_64");
+
+      if (TARGET_X32)
+	d_add_builtin_version ("D_X32");
+    }
+  else
+    d_add_builtin_version ("X86");
+
+  if (TARGET_80387)
+    d_add_builtin_version ("D_HardFloat");
+  else
+    d_add_builtin_version ("D_SoftFloat");
+}
--- a/gcc/config/i386/i386-protos.h
+++ b/gcc/config/i386/i386-protos.h
@@ -244,6 +244,9 @@ extern void ix86_expand_sse2_abs (rtx, rtx);
 extern void ix86_target_macros (void);
 extern void ix86_register_pragmas (void);
 
+/* In i386-d.c  */
+extern void ix86_d_target_versions (void);
+
 /* In winnt.c  */
 extern void i386_pe_unique_section (tree, int);
 extern void i386_pe_declare_function_type (FILE *, const char *, int);
--- a/gcc/config/i386/i386.h
+++ b/gcc/config/i386/i386.h
@@ -621,6 +621,9 @@ extern const char *host_detect_local_cpu (int argc, const char **argv);
 /* Target Pragmas.  */
 #define REGISTER_TARGET_PRAGMAS() ix86_register_pragmas ()
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS ix86_d_target_versions
+
 #ifndef CC1_SPEC
 #define CC1_SPEC "%(cc1_cpu) "
 #endif
--- a/gcc/config/i386/linux-common.h
+++ b/gcc/config/i386/linux-common.h
@@ -27,6 +27,12 @@ along with GCC; see the file COPYING3.  If not see
     }                                          \
   while (0)
 
+#define EXTRA_TARGET_D_OS_VERSIONS()		\
+  ANDROID_TARGET_D_OS_VERSIONS();
+
+#define GNU_USER_TARGET_D_CRITSEC_SIZE		\
+  (TARGET_64BIT ? (POINTER_SIZE == 64 ? 40 : 32) : 24)
+
 #undef CC1_SPEC
 #define CC1_SPEC \
   LINUX_OR_ANDROID_CC (GNU_USER_TARGET_CC1_SPEC, \
--- a/gcc/config/i386/t-i386
+++ b/gcc/config/i386/t-i386
@@ -23,6 +23,10 @@ i386-c.o: $(srcdir)/config/i386/i386-c.c i386-builtin-types.inc
 	  $(COMPILE) $<
 	  $(POSTCOMPILE)
 
+i386-d.o: $(srcdir)/config/i386/i386-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 i386-builtin-types.inc: s-i386-bt ; @true
 s-i386-bt: $(srcdir)/config/i386/i386-builtin-types.awk \
   $(srcdir)/config/i386/i386-builtin-types.def
--- a/gcc/config/kfreebsd-gnu.h
+++ b/gcc/config/kfreebsd-gnu.h
@@ -29,6 +29,9 @@ along with GCC; see the file COPYING3.  If not see
     }						\
   while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()		\
+  builtin_version ("FreeBSD")
+
 #define GNU_USER_DYNAMIC_LINKER        GLIBC_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER32      GLIBC_DYNAMIC_LINKER32
 #define GNU_USER_DYNAMIC_LINKER64      GLIBC_DYNAMIC_LINKER64
--- a/gcc/config/kopensolaris-gnu.h
+++ b/gcc/config/kopensolaris-gnu.h
@@ -30,5 +30,8 @@ along with GCC; see the file COPYING3.  If not see
     }						\
   while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()		\
+  builtin_version ("Solaris")
+
 #undef GNU_USER_DYNAMIC_LINKER
 #define GNU_USER_DYNAMIC_LINKER "/lib/ld.so.1"
--- a/gcc/config/linux-android.h
+++ b/gcc/config/linux-android.h
@@ -25,6 +25,12 @@
 	  builtin_define ("__ANDROID__");			\
     } while (0)
 
+#define ANDROID_TARGET_D_OS_VERSIONS()				\
+    do {							\
+	if (TARGET_ANDROID)					\
+	  builtin_version ("Android");				\
+    } while (0)
+
 #if ANDROID_DEFAULT
 # define NOANDROID "mno-android"
 #else
--- a/gcc/config/linux.h
+++ b/gcc/config/linux.h
@@ -49,6 +49,9 @@ see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
 	builtin_assert ("system=posix");			\
     } while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()				\
+  builtin_version ("linux")
+
 /* Determine which dynamic linker to use depending on whether GLIBC or
    uClibc or Bionic is the default C library and whether
    -muclibc or -mglibc or -mbionic has been passed to change the default.  */
--- a/gcc/config/mips/linux-common.h
+++ b/gcc/config/mips/linux-common.h
@@ -27,6 +27,9 @@ along with GCC; see the file COPYING3.  If not see
     ANDROID_TARGET_OS_CPP_BUILTINS();				\
   } while (0)
 
+#define EXTRA_TARGET_D_OS_VERSIONS()				\
+  ANDROID_TARGET_D_OS_VERSIONS();
+
 #undef  LINK_SPEC
 #define LINK_SPEC							\
   LINUX_OR_ANDROID_LD (GNU_USER_TARGET_LINK_SPEC,			\
--- /dev/null
+++ b/gcc/config/mips/mips-d.c
@@ -0,0 +1,56 @@
+/* Subroutines for the D front end on the MIPS architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for MIPS targets.  */
+
+void
+mips_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    d_add_builtin_version ("MIPS64");
+  else
+    d_add_builtin_version ("MIPS32");
+
+  if (mips_abi == ABI_32)
+    d_add_builtin_version ("MIPS_O32");
+  else if (mips_abi == ABI_EABI)
+    d_add_builtin_version ("MIPS_EABI");
+  else if (mips_abi == ABI_N32)
+    d_add_builtin_version ("MIPS_N32");
+  else if (mips_abi == ABI_64)
+    d_add_builtin_version ("MIPS_N64");
+  else if (mips_abi == ABI_O64)
+    d_add_builtin_version ("MIPS_O64");
+
+  if (TARGET_HARD_FLOAT_ABI)
+    {
+      d_add_builtin_version ("MIPS_HardFloat");
+      d_add_builtin_version ("D_HardFloat");
+    }
+  else if (TARGET_SOFT_FLOAT_ABI)
+    {
+      d_add_builtin_version ("MIPS_SoftFloat");
+      d_add_builtin_version ("D_SoftFloat");
+    }
+}
--- a/gcc/config/mips/mips-protos.h
+++ b/gcc/config/mips/mips-protos.h
@@ -359,4 +359,7 @@ typedef rtx (*mulsidi3_gen_fn) (rtx, rtx, rtx);
 extern mulsidi3_gen_fn mips_mulsidi3_gen_fn (enum rtx_code);
 #endif
 
+/* Routines implemented in mips-d.c  */
+extern void mips_d_target_versions (void);
+
 #endif /* ! GCC_MIPS_PROTOS_H */
--- a/gcc/config/mips/mips.h
+++ b/gcc/config/mips/mips.h
@@ -574,6 +574,9 @@ struct mips_cpu_info {
     }									\
   while (0)
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS mips_d_target_versions
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- a/gcc/config/mips/t-mips
+++ b/gcc/config/mips/t-mips
@@ -20,3 +20,7 @@ $(srcdir)/config/mips/mips-tables.opt: $(srcdir)/config/mips/genopt.sh \
   $(srcdir)/config/mips/mips-cpus.def
 	$(SHELL) $(srcdir)/config/mips/genopt.sh $(srcdir)/config/mips > \
 		$(srcdir)/config/mips/mips-tables.opt
+
+mips-d.o: $(srcdir)/config/mips/mips-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- a/gcc/config/rs6000/linux.h
+++ b/gcc/config/rs6000/linux.h
@@ -53,6 +53,9 @@
     }						\
   while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()		\
+  builtin_version ("linux")
+
 #undef	CPP_OS_DEFAULT_SPEC
 #define CPP_OS_DEFAULT_SPEC "%(cpp_os_linux)"
 
--- a/gcc/config/rs6000/linux64.h
+++ b/gcc/config/rs6000/linux64.h
@@ -336,6 +336,9 @@ extern int dot_symbols;
     }							\
   while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()			\
+  builtin_version ("linux")
+
 #undef  CPP_OS_DEFAULT_SPEC
 #define CPP_OS_DEFAULT_SPEC "%(cpp_os_linux)"
 
--- /dev/null
+++ b/gcc/config/rs6000/rs6000-d.c
@@ -0,0 +1,45 @@
+/* Subroutines for the D front end on the PowerPC architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for PowerPC targets.  */
+
+void
+rs6000_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    d_add_builtin_version ("PPC64");
+  else
+    d_add_builtin_version ("PPC");
+
+  if (TARGET_HARD_FLOAT)
+    {
+      d_add_builtin_version ("PPC_HardFloat");
+      d_add_builtin_version ("D_HardFloat");
+    }
+  else if (TARGET_SOFT_FLOAT)
+    {
+      d_add_builtin_version ("PPC_SoftFloat");
+      d_add_builtin_version ("D_SoftFloat");
+    }
+}
--- a/gcc/config/rs6000/rs6000-protos.h
+++ b/gcc/config/rs6000/rs6000-protos.h
@@ -208,6 +208,9 @@ extern void rs6000_target_modify_macros (bool, HOST_WIDE_INT, HOST_WIDE_INT);
 extern void (*rs6000_target_modify_macros_ptr) (bool, HOST_WIDE_INT,
 						HOST_WIDE_INT);
 
+/* Declare functions in rs6000-d.c  */
+extern void rs6000_d_target_versions (void);
+
 #if TARGET_MACHO
 char *output_call (rtx, rtx *, int, int);
 #endif
--- a/gcc/config/rs6000/rs6000.h
+++ b/gcc/config/rs6000/rs6000.h
@@ -704,6 +704,9 @@ extern unsigned char rs6000_recip_bits[];
 #define TARGET_CPU_CPP_BUILTINS() \
   rs6000_cpu_cpp_builtins (pfile)
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS rs6000_d_target_versions
+
 /* This is used by rs6000_cpu_cpp_builtins to indicate the byte order
    we're compiling for.  Some configurations may need to override it.  */
 #define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
--- a/gcc/config/rs6000/t-rs6000
+++ b/gcc/config/rs6000/t-rs6000
@@ -24,6 +24,10 @@ rs6000-c.o: $(srcdir)/config/rs6000/rs6000-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
 
+rs6000-d.o: $(srcdir)/config/rs6000/rs6000-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 $(srcdir)/config/rs6000/rs6000-tables.opt: $(srcdir)/config/rs6000/genopt.sh \
   $(srcdir)/config/rs6000/rs6000-cpus.def
 	$(SHELL) $(srcdir)/config/rs6000/genopt.sh $(srcdir)/config/rs6000 > \
--- /dev/null
+++ b/gcc/config/s390/s390-d.c
@@ -0,0 +1,41 @@
+/* Subroutines for the D front end on the IBM S/390 and zSeries architectures.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for S/390 and zSeries targets.  */
+
+void
+s390_d_target_versions (void)
+{
+  if (TARGET_ZARCH)
+    d_add_builtin_version ("SystemZ");
+  else if (TARGET_64BIT)
+    d_add_builtin_version ("S390X");
+  else
+    d_add_builtin_version ("S390");
+
+  if (TARGET_SOFT_FLOAT)
+    d_add_builtin_version ("D_SoftFloat");
+  else if (TARGET_HARD_FLOAT)
+    d_add_builtin_version ("D_HardFloat");
+}
--- a/gcc/config/s390/s390-protos.h
+++ b/gcc/config/s390/s390-protos.h
@@ -114,4 +114,7 @@ extern int s390_compare_and_branch_condition_mask (rtx);
 extern bool s390_extzv_shift_ok (int, int, unsigned HOST_WIDE_INT);
 extern void s390_asm_output_function_label (FILE *, const char *, tree);
 
+/* Routines for s390-d.c  */
+extern void s390_d_target_versions (void);
+
 #endif /* RTX_CODE */
--- a/gcc/config/s390/s390.h
+++ b/gcc/config/s390/s390.h
@@ -114,6 +114,9 @@ enum processor_flags
     }									\
   while (0)
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS s390_d_target_versions
+
 #ifdef DEFAULT_TARGET_64BIT
 #define TARGET_DEFAULT             (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP | MASK_OPT_HTM)
 #else
--- /dev/null
+++ b/gcc/config/s390/t-s390
@@ -0,0 +1,21 @@
+# Copyright (C) 2015 Free Software Foundation, Inc.
+#
+# This file is part of GCC.
+#
+# GCC is free software; you can redistribute it and/or modify
+# it under the terms of the GNU General Public License as published by
+# the Free Software Foundation; either version 3, or (at your option)
+# any later version.
+#
+# GCC is distributed in the hope that it will be useful,
+# but WITHOUT ANY WARRANTY; without even the implied warranty of
+# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+# GNU General Public License for more details.
+#
+# You should have received a copy of the GNU General Public License
+# along with GCC; see the file COPYING3.  If not see
+# <http://www.gnu.org/licenses/>.
+
+s390-d.o: $(srcdir)/config/s390/s390-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/sparc/sparc-d.c
@@ -0,0 +1,48 @@
+/* Subroutines for the D front end on the SPARC architecture.
+   Copyright (C) 2017 Free Software Foundation, Inc.
+
+GCC is free software; you can redistribute it and/or modify
+it under the terms of the GNU General Public License as published by
+the Free Software Foundation; either version 3, or (at your option)
+any later version.
+
+GCC is distributed in the hope that it will be useful,
+but WITHOUT ANY WARRANTY; without even the implied warranty of
+MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+GNU General Public License for more details.
+
+You should have received a copy of the GNU General Public License
+along with GCC; see the file COPYING3.  If not see
+<http://www.gnu.org/licenses/>.  */
+
+#include "config.h"
+#include "system.h"
+#include "coretypes.h"
+#include "tm.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for SPARC targets.  */
+
+void
+sparc_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    d_add_builtin_version ("SPARC64");
+  else
+    d_add_builtin_version ("SPARC");
+
+  if (TARGET_V8PLUS)
+    d_add_builtin_version ("SPARC_V8Plus");
+
+  if (TARGET_FPU)
+    {
+      d_add_builtin_version ("D_HardFloat");
+      d_add_builtin_version ("SPARC_HardFloat");
+    }
+  else
+    {
+      d_add_builtin_version ("D_SoftFloat");
+      d_add_builtin_version ("SPARC_SoftFloat");
+    }
+}
--- a/gcc/config/sparc/sparc-protos.h
+++ b/gcc/config/sparc/sparc-protos.h
@@ -112,4 +112,7 @@ bool sparc_modes_tieable_p (enum machine_mode, enum machine_mode);
 
 extern void sparc_emit_membar_for_model (enum memmodel, int, int);
 
+/* Routines implemented in sparc-d.c  */
+extern void sparc_d_target_versions (void);
+
 #endif /* __SPARC_PROTOS_H__ */
--- a/gcc/config/sparc/sparc.h
+++ b/gcc/config/sparc/sparc.h
@@ -27,6 +27,9 @@ along with GCC; see the file COPYING3.  If not see
 
 #define TARGET_CPU_CPP_BUILTINS() sparc_target_macros ()
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS sparc_d_target_versions
+
 /* Specify this in a cover file to provide bi-architecture (32/64) support.  */
 /* #define SPARC_BI_ARCH */
 
--- a/gcc/config/sparc/t-sparc
+++ b/gcc/config/sparc/t-sparc
@@ -21,3 +21,7 @@
 sparc-c.o: $(srcdir)/config/sparc/sparc-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
+
+sparc-d.o: $(srcdir)/config/sparc/sparc-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- a/gcc/config/t-glibc
+++ b/gcc/config/t-glibc
@@ -19,3 +19,7 @@
 glibc-c.o: config/glibc-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
+
+glibc-d.o: config/glibc-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- a/gcc/configure
+++ b/gcc/configure
@@ -611,6 +611,7 @@ ISLLIBS
 GMPINC
 GMPLIBS
 target_cpu_default
+d_target_objs
 fortran_target_objs
 cxx_target_objs
 c_target_objs
@@ -618,6 +619,8 @@ use_gcc_stdint
 xm_defines
 xm_include_list
 xm_file_list
+tm_d_include_list
+tm_d_file_list
 tm_p_include_list
 tm_p_file_list
 tm_defines
@@ -11331,6 +11334,7 @@ fi
 
 tm_file="${tm_file} defaults.h"
 tm_p_file="${tm_p_file} tm-preds.h"
+tm_d_file="${tm_d_file} defaults.h"
 host_xm_file="auto-host.h ansidecl.h ${host_xm_file}"
 build_xm_file="${build_auto} ansidecl.h ${build_xm_file}"
 # We don't want ansidecl.h in target files, write code there in ISO/GNU C.
@@ -11670,6 +11674,21 @@ for f in $tm_p_file; do
   esac
 done
 
+tm_d_file_list=
+tm_d_include_list="options.h insn-constants.h"
+for f in $tm_d_file; do
+  case $f in
+    defaults.h )
+       tm_d_file_list="${tm_d_file_list} \$(srcdir)/$f"
+       tm_d_include_list="${tm_d_include_list} $f"
+       ;;
+    * )
+       tm_d_file_list="${tm_d_file_list} \$(srcdir)/config/$f"
+       tm_d_include_list="${tm_d_include_list} config/$f"
+       ;;
+  esac
+done
+
 xm_file_list=
 xm_include_list=
 for f in $xm_file; do
@@ -15917,7 +15936,7 @@ _LT_EOF
 	if $NM -V 2>&1 | $GREP 'GNU' > /dev/null; then
 	  export_symbols_cmds='$NM -Bpg $libobjs $convenience | awk '\''{ if (((\$ 2 == "T") || (\$ 2 == "D") || (\$ 2 == "B") || (\$ 2 == "W")) && (substr(\$ 3,1,1) != ".")) { print \$ 3 } }'\'' | sort -u > $export_symbols'
 	else
-	  export_symbols_cmds='$NM -BCpg $libobjs $convenience | awk '\''{ if (((\$ 2 == "T") || (\$ 2 == "D") || (\$ 2 == "B") || (\$ 2 == "L")) && (substr(\$ 3,1,1) != ".")) { print \$ 3 } }'\'' | sort -u > $export_symbols'
+	  export_symbols_cmds='$NM -BCpg $libobjs $convenience | awk '\''{ if (((\$ 2 == "T") || (\$ 2 == "D") || (\$ 2 == "B")) && (substr(\$ 3,1,1) != ".")) { print \$ 3 } }'\'' | sort -u > $export_symbols'
 	fi
 	aix_use_runtimelinking=no
 
@@ -17945,7 +17964,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 17948 "configure"
+#line 17967 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -18051,7 +18070,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18054 "configure"
+#line 18073 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -20403,7 +20422,7 @@ $as_echo_n "checking whether the $compiler linker ($LD) supports shared librarie
     if $NM -V 2>&1 | $GREP 'GNU' > /dev/null; then
       export_symbols_cmds_CXX='$NM -Bpg $libobjs $convenience | awk '\''{ if (((\$ 2 == "T") || (\$ 2 == "D") || (\$ 2 == "B") || (\$ 2 == "W")) && (substr(\$ 3,1,1) != ".")) { print \$ 3 } }'\'' | sort -u > $export_symbols'
     else
-      export_symbols_cmds_CXX='$NM -BCpg $libobjs $convenience | awk '\''{ if (((\$ 2 == "T") || (\$ 2 == "D") || (\$ 2 == "B") || (\$ 2 == "L")) && (substr(\$ 3,1,1) != ".")) { print \$ 3 } }'\'' | sort -u > $export_symbols'
+      export_symbols_cmds_CXX='$NM -BCpg $libobjs $convenience | awk '\''{ if (((\$ 2 == "T") || (\$ 2 == "D") || (\$ 2 == "B")) && (substr(\$ 3,1,1) != ".")) { print \$ 3 } }'\'' | sort -u > $export_symbols'
     fi
     ;;
   pw32*)
@@ -27924,6 +27943,9 @@ fi
 
 
 
+
+
+
 
 
 
--- a/gcc/configure.ac
+++ b/gcc/configure.ac
@@ -1555,6 +1555,7 @@ AC_SUBST(build_subdir)
 
 tm_file="${tm_file} defaults.h"
 tm_p_file="${tm_p_file} tm-preds.h"
+tm_d_file="${tm_d_file} defaults.h"
 host_xm_file="auto-host.h ansidecl.h ${host_xm_file}"
 build_xm_file="${build_auto} ansidecl.h ${build_xm_file}"
 # We don't want ansidecl.h in target files, write code there in ISO/GNU C.
@@ -1764,6 +1765,21 @@ for f in $tm_p_file; do
   esac
 done
 
+tm_d_file_list=
+tm_d_include_list="options.h insn-constants.h"
+for f in $tm_d_file; do
+  case $f in
+    defaults.h )
+       tm_d_file_list="${tm_d_file_list} \$(srcdir)/$f"
+       tm_d_include_list="${tm_d_include_list} $f"
+       ;;
+    * )
+       tm_d_file_list="${tm_d_file_list} \$(srcdir)/config/$f"
+       tm_d_include_list="${tm_d_include_list} config/$f"
+       ;;
+  esac
+done
+
 xm_file_list=
 xm_include_list=
 for f in $xm_file; do
@@ -5498,6 +5514,8 @@ AC_SUBST(tm_include_list)
 AC_SUBST(tm_defines)
 AC_SUBST(tm_p_file_list)
 AC_SUBST(tm_p_include_list)
+AC_SUBST(tm_d_file_list)
+AC_SUBST(tm_d_include_list)
 AC_SUBST(xm_file_list)
 AC_SUBST(xm_include_list)
 AC_SUBST(xm_defines)
@@ -5505,6 +5523,7 @@ AC_SUBST(use_gcc_stdint)
 AC_SUBST(c_target_objs)
 AC_SUBST(cxx_target_objs)
 AC_SUBST(fortran_target_objs)
+AC_SUBST(d_target_objs)
 AC_SUBST(target_cpu_default)
 
 AC_SUBST_FILE(language_hooks)
--- a/gcc/doc/tm.texi
+++ b/gcc/doc/tm.texi
@@ -53,6 +53,7 @@ through the macros defined in the @file{.h} file.
 * MIPS Coprocessors::   MIPS coprocessor support and how to customize it.
 * PCH Target::          Validity checking for precompiled headers.
 * C++ ABI::             Controlling C++ ABI changes.
+* D Language and ABI::  Controlling D ABI changes.
 * Named Address Spaces:: Adding support for named address spaces
 * Misc::                Everything else.
 @end menu
@@ -107,6 +108,14 @@ documented as ``Common Target Hook''.  This is declared in
 @code{target_has_targetm_common=yes} in @file{config.gcc}; otherwise a
 default definition is used.
 
+Similarly, there is a @code{targetdm} variable for hooks that are
+specific to the D language front end, documented as ``D Target Hook''.
+This is declared in @file{d/d-target.h}, the initializer
+@code{TARGETDM_INITIALIZER} in @file{d/d-target-def.h}.  If targets
+initialize @code{targetdm} themselves, they should set
+@code{target_has_targetdm=yes} in @file{config.gcc}; otherwise a default
+definition is used.
+
 @node Driver
 @section Controlling the Compilation Driver, @file{gcc}
 @cindex driver
@@ -10203,6 +10212,22 @@ unloaded. The default is to return false.
 Return target-specific mangling context of @var{decl} or @code{NULL_TREE}.
 @end deftypefn
 
+@node D Language and ABI
+@section D ABI parameters
+@cindex parameters, d abi
+
+@deftypefn {D Target Hook} void TARGET_D_CPU_VERSIONS (void)
+Declare all environmental version identifiers relating to the target CPU using the function @code{builtin_version}, which takes a string representing the name of the version.  Version identifiers predefined by this hook apply to all modules and being compiled and imported.
+@end deftypefn
+
+@deftypefn {D Target Hook} void TARGET_D_OS_VERSIONS (void)
+Similarly to @code{TARGET_D_CPU_VERSIONS}, but is used for versions relating to the target operating system.
+@end deftypefn
+
+@deftypefn {D Target Hook} unsigned TARGET_D_CRITSEC_SIZE (void)
+Returns the size of the data structure used by the targeted operating system for critical sections and monitors.  For example, on Microsoft Windows this would return the @code{sizeof(CRITICAL_SECTION)}, while other platforms that implement pthreads would return @code{sizeof(pthread_mutex_t)}.
+@end deftypefn
+
 @node Named Address Spaces
 @section Adding support for named address spaces
 @cindex named address spaces
--- a/gcc/doc/tm.texi.in
+++ b/gcc/doc/tm.texi.in
@@ -53,6 +53,7 @@ through the macros defined in the @file{.h} file.
 * MIPS Coprocessors::   MIPS coprocessor support and how to customize it.
 * PCH Target::          Validity checking for precompiled headers.
 * C++ ABI::             Controlling C++ ABI changes.
+* D Language and ABI::  Controlling D ABI changes.
 * Named Address Spaces:: Adding support for named address spaces
 * Misc::                Everything else.
 @end menu
@@ -107,6 +108,14 @@ documented as ``Common Target Hook''.  This is declared in
 @code{target_has_targetm_common=yes} in @file{config.gcc}; otherwise a
 default definition is used.
 
+Similarly, there is a @code{targetdm} variable for hooks that are
+specific to the D language front end, documented as ``D Target Hook''.
+This is declared in @file{d/d-target.h}, the initializer
+@code{TARGETDM_INITIALIZER} in @file{d/d-target-def.h}.  If targets
+initialize @code{targetdm} themselves, they should set
+@code{target_has_targetdm=yes} in @file{config.gcc}; otherwise a default
+definition is used.
+
 @node Driver
 @section Controlling the Compilation Driver, @file{gcc}
 @cindex driver
@@ -7622,6 +7631,16 @@ you may want to override in subtargets; it is described below.
 
 @hook TARGET_CXX_DECL_MANGLING_CONTEXT
 
+@node D Language and ABI
+@section D ABI parameters
+@cindex parameters, d abi
+
+@hook TARGET_D_CPU_VERSIONS
+
+@hook TARGET_D_OS_VERSIONS
+
+@hook TARGET_D_CRITSEC_SIZE
+
 @node Named Address Spaces
 @section Adding support for named address spaces
 @cindex named address spaces
--- a/gcc/genhooks.c
+++ b/gcc/genhooks.c
@@ -35,6 +35,7 @@ static struct hook_desc hook_array[] = {
 #include "target.def"
 #include "c-family/c-target.def"
 #include "common/common-target.def"
+#include "d/d-target.def"
 #undef DEFHOOK
 };
 
