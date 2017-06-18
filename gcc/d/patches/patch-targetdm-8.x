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
* Epiphany
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

The following OS versions are implemented:
* Windows
** Win32
** Win64
** Cygwin
** MinGW
* linux
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
* Android
* CRuntime_Bionic
* CRuntime_Glibc
* CRuntime_Musl
* CRuntime_UClibc

These official OS versions are not implemented:
* BSD (other BSDs)
* Haiku
* PlayStation
* PlayStation4
* SkyOS
* SysV3
* SysV4
* CRuntime_DigitalMars
* CRuntime_Microsoft
---

--- a/gcc/Makefile.in
+++ b/gcc/Makefile.in
@@ -546,6 +546,8 @@ tm_include_list=@tm_include_list@
 tm_defines=@tm_defines@
 tm_p_file_list=@tm_p_file_list@
 tm_p_include_list=@tm_p_include_list@
+tm_d_file_list=@tm_d_file_list@
+tm_d_include_list=@tm_d_include_list@
 build_xm_file_list=@build_xm_file_list@
 build_xm_include_list=@build_xm_include_list@
 build_xm_defines=@build_xm_defines@
@@ -840,6 +842,7 @@ BCONFIG_H = bconfig.h $(build_xm_file_list)
 CONFIG_H  = config.h  $(host_xm_file_list)
 TCONFIG_H = tconfig.h $(xm_file_list)
 TM_P_H    = tm_p.h    $(tm_p_file_list)
+TM_D_H    = tm_d.h    $(tm_d_file_list)
 GTM_H     = tm.h      $(tm_file_list) insn-constants.h
 TM_H      = $(GTM_H) insn-flags.h $(OPTIONS_H)
 
@@ -897,9 +900,11 @@ EXCEPT_H = except.h $(HASHTAB_H)
 TARGET_DEF = target.def target-hooks-macros.h target-insns.def
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
@@ -1172,6 +1177,9 @@ C_TARGET_OBJS=@c_target_objs@
 # Target specific, C++ specific object file
 CXX_TARGET_OBJS=@cxx_target_objs@
 
+# Target specific, D specific object file
+D_TARGET_OBJS=@d_target_objs@
+
 # Target specific, Fortran specific object file
 FORTRAN_TARGET_OBJS=@fortran_target_objs@
 
@@ -1760,6 +1768,7 @@ bconfig.h: cs-bconfig.h ; @true
 tconfig.h: cs-tconfig.h ; @true
 tm.h: cs-tm.h ; @true
 tm_p.h: cs-tm_p.h ; @true
+tm_d.h: cs-tm_d.h ; @true
 
 cs-config.h: Makefile
 	TARGET_CPU_DEFAULT="" \
@@ -1786,6 +1795,11 @@ cs-tm_p.h: Makefile
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
@@ -2115,6 +2129,12 @@ default-c.o: config/default-c.c
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
@@ -2403,6 +2423,15 @@ s-common-target-hooks-def-h: build/genhooks$(build_exeext)
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
@@ -2426,6 +2455,7 @@ s-tm-texi: build/genhooks$(build_exeext) $(srcdir)/doc/tm.texi.in
 	  && ( test $(srcdir)/doc/tm.texi -nt $(srcdir)/target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/c-family/c-target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/common/common-target.def \
+	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/d/d-target.def \
 	  ); then \
 	  echo >&2 ; \
 	  echo You should edit $(srcdir)/doc/tm.texi.in rather than $(srcdir)/doc/tm.texi . >&2 ; \
@@ -2564,13 +2594,14 @@ s-gtype: build/gengtype$(build_exeext) $(filter-out [%], $(GTFILES)) \
                     -r gtype.state
 	$(STAMP) s-gtype
 
-generated_files = config.h tm.h $(TM_P_H) $(TM_H) multilib.h \
+generated_files = config.h tm.h $(TM_P_H) $(TM_D_H) $(TM_H) multilib.h \
        $(simple_generated_h) specs.h \
        tree-check.h genrtl.h insn-modes.h tm-preds.h tm-constrs.h \
        $(ALL_GTFILES_H) gtype-desc.c gtype-desc.h gcov-iov.h \
        options.h target-hooks-def.h insn-opinit.h \
        common/common-target-hooks-def.h pass-instances.def \
-       c-family/c-target-hooks-def.h params.list params.options case-cfn-macros.h \
+       c-family/c-target-hooks-def.h d/d-target-hooks-def.h \
+       params.list params.options case-cfn-macros.h \
        cfn-operators.pd
 
 #
@@ -2712,7 +2743,7 @@ build/genrecog.o : genrecog.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
   coretypes.h $(GTM_H) errors.h $(READ_MD_H) $(GENSUPPORT_H)		\
   $(HASH_TABLE_H) inchash.h
 build/genhooks.o : genhooks.c $(TARGET_DEF) $(C_TARGET_DEF)		\
-  $(COMMON_TARGET_DEF) $(BCONFIG_H) $(SYSTEM_H) errors.h
+  $(COMMON_TARGET_DEF) $(D_TARGET_DEF) $(BCONFIG_H) $(SYSTEM_H) errors.h
 build/genmddump.o : genmddump.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
   coretypes.h $(GTM_H) errors.h $(READ_MD_H) $(GENSUPPORT_H)
 build/genmatch.o : genmatch.c $(BCONFIG_H) $(SYSTEM_H) \
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
@@ -191,6 +197,9 @@
 #
 #  target_has_targetm_common	Set to yes or no depending on whether the
 #			target has its own definition of targetm_common.
+#
+#  target_has_targetdm	Set to yes or no depending on whether the target
+#			has its own definition of targetdm.
 
 out_file=
 common_out_file=
@@ -206,9 +215,11 @@ extra_gcc_objs=
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
@@ -303,12 +314,14 @@ aarch64*-*-*)
 	extra_headers="arm_fp16.h arm_neon.h arm_acle.h"
 	c_target_objs="aarch64-c.o"
 	cxx_target_objs="aarch64-c.o"
+	d_target_objs="aarch64-d.o"
 	extra_objs="aarch64-builtins.o aarch-common.o cortex-a57-fma-steering.o"
 	target_gtfiles="\$(srcdir)/config/aarch64/aarch64-builtins.c"
 	target_has_targetm_common=yes
 	;;
 alpha*-*-*)
 	cpu_type=alpha
+	d_target_objs="alpha-d.o"
 	extra_options="${extra_options} g.opt"
 	;;
 am33_2.0-*-linux*)
@@ -328,6 +341,7 @@ arm*-*-*)
 	target_type_format_char='%'
 	c_target_objs="arm-c.o"
 	cxx_target_objs="arm-c.o"
+	d_target_objs="arm-d.o"
 	extra_options="${extra_options} arm/arm-tables.opt"
 	target_gtfiles="\$(srcdir)/config/arm/arm-builtins.c"
 	;;
@@ -342,6 +356,9 @@ bfin*-*)
 crisv32-*)
 	cpu_type=cris
 	;;
+epiphany-*-* )
+	d_target_objs="epiphany-d.o"
+	;;
 frv*)	cpu_type=frv
 	extra_options="${extra_options} g.opt"
 	;;
@@ -360,6 +377,7 @@ i[34567]86-*-*)
 	cpu_type=i386
 	c_target_objs="i386-c.o"
 	cxx_target_objs="i386-c.o"
+	d_target_objs="i386-d.o"
 	extra_options="${extra_options} fused-madd.opt"
 	extra_headers="cpuid.h mmintrin.h mm3dnow.h xmmintrin.h emmintrin.h
 		       pmmintrin.h tmmintrin.h ammintrin.h smmintrin.h
@@ -383,6 +401,7 @@ x86_64-*-*)
 	cpu_type=i386
 	c_target_objs="i386-c.o"
 	cxx_target_objs="i386-c.o"
+	d_target_objs="i386-d.o"
 	extra_options="${extra_options} fused-madd.opt"
 	extra_headers="cpuid.h mmintrin.h mm3dnow.h xmmintrin.h emmintrin.h
 		       pmmintrin.h tmmintrin.h ammintrin.h smmintrin.h
@@ -403,6 +422,7 @@ x86_64-*-*)
 		       clzerointrin.h pkuintrin.h sgxintrin.h"
 	;;
 ia64-*-*)
+	d_target_objs="ia64-d.o"
 	extra_headers=ia64intrin.h
 	extra_options="${extra_options} g.opt fused-madd.opt"
 	;;
@@ -426,6 +446,7 @@ microblaze*-*-*)
         ;;
 mips*-*-*)
 	cpu_type=mips
+	d_target_objs="mips-d.o"
 	extra_headers="loongson.h msa.h"
 	extra_objs="frame-header-opt.o"
 	extra_options="${extra_options} g.opt fused-madd.opt mips/mips-tables.opt"
@@ -441,6 +462,7 @@ nios2-*-*)
 	;;
 nvptx-*-*)
 	cpu_type=nvptx
+	d_target_objs="nvptx-d.o"
 	;;
 powerpc*-*-*spe*)
 	cpu_type=powerpcspe
@@ -468,6 +490,7 @@ powerpc*-*-*)
 riscv*)
 	cpu_type=riscv
 	extra_objs="riscv-builtins.o riscv-c.o"
+	d_target_objs="riscv-d.o"
 	;;
 rs6000*-*-*)
 	extra_options="${extra_options} g.opt fused-madd.opt rs6000/rs6000-tables.opt"
@@ -476,6 +499,7 @@ sparc*-*-*)
 	cpu_type=sparc
 	c_target_objs="sparc-c.o"
 	cxx_target_objs="sparc-c.o"
+	d_target_objs="sparc-d.o"
 	extra_headers="visintrin.h"
 	;;
 spu*-*-*)
@@ -483,6 +507,7 @@ spu*-*-*)
 	;;
 s390*-*-*)
 	cpu_type=s390
+	d_target_objs="s390-d.o"
 	extra_options="${extra_options} fused-madd.opt"
 	extra_headers="s390intrin.h htmintrin.h htmxlintrin.h vecintrin.h"
 	;;
@@ -512,10 +537,13 @@ tilepro*-*-*)
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
@@ -3117,6 +3145,10 @@ if [ "$common_out_file" = "" ]; then
   fi
 fi
 
+if [ "$target_has_targetdm" = "no" ]; then
+  d_target_objs="$d_target_objs default-d.o"
+fi
+
 # Support for --with-cpu and related options (and a few unrelated options,
 # too).
 case ${with_cpu} in
@@ -4505,6 +4537,8 @@ case ${target} in
 		then
 			target_cpu_default2="MASK_GAS"
 		fi
+		d_target_objs="${d_target_objs} pa-d.o"
+		tmake_file="pa/t-pa ${tmake_file}"
 		;;
 
 	fido*-*-* | m68k*-*-*)
@@ -4590,12 +4624,14 @@ case ${target} in
 		out_file="${cpu_type}/${cpu_type}.c"
 		c_target_objs="${c_target_objs} ${cpu_type}-c.o"
 		cxx_target_objs="${cxx_target_objs} ${cpu_type}-c.o"
+		d_target_objs="${d_target_objs} ${cpu_type}-d.o"
 		tmake_file="${cpu_type}/t-${cpu_type} ${tmake_file}"
 		;;
 
 	sh[123456ble]*-*-* | sh-*-*)
 		c_target_objs="${c_target_objs} sh-c.o"
 		cxx_target_objs="${cxx_target_objs} sh-c.o"
+		d_target_objs="${d_target_objs} sh-d.o"
 		;;
 
 	sparc*-*-*)
--- a/gcc/config/aarch64/aarch64-d.c
+++ b/gcc/config/aarch64/aarch64-d.c
@@ -0,0 +1,36 @@
+/* Subroutines for the D front end on the ARM64 architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+aarch64_d_target_versions (void)
+{
+  VersionCondition::addPredefinedGlobalIdent ("AArch64");
+  VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+}
--- a/gcc/config/aarch64/aarch64-protos.h
+++ b/gcc/config/aarch64/aarch64-protos.h
@@ -475,6 +475,9 @@ enum aarch64_parse_opt_result aarch64_parse_extension (const char *,
 std::string aarch64_get_extension_string_for_isa_flags (unsigned long,
 							unsigned long);
 
+/* Defined in aarch64-d.c  */
+extern void aarch64_d_target_versions (void);
+
 rtl_opt_pass *make_pass_fma_steering (gcc::context *ctxt);
 
 #endif /* GCC_AARCH64_PROTOS_H */
--- a/gcc/config/aarch64/aarch64.h
+++ b/gcc/config/aarch64/aarch64.h
@@ -26,6 +26,10 @@
 #define TARGET_CPU_CPP_BUILTINS()	\
   aarch64_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS		\
+  aarch_d_target_versions
+
 
 
 #define REGISTER_TARGET_PRAGMAS() aarch64_register_pragmas ()
--- a/gcc/config/aarch64/t-aarch64
+++ b/gcc/config/aarch64/t-aarch64
@@ -56,6 +56,11 @@ aarch64-c.o: $(srcdir)/config/aarch64/aarch64-c.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/aarch64/aarch64-c.c
 
+aarch64-d.o: $(srcdir)/config/aarch64/aarch64-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-aarch64-d.o += -I$(srcdir)/d
+
 PASSES_EXTRA += $(srcdir)/config/aarch64/aarch64-passes.def
 
 cortex-a57-fma-steering.o: $(srcdir)/config/aarch64/cortex-a57-fma-steering.c \
--- a/gcc/config/alpha/alpha-d.c
+++ b/gcc/config/alpha/alpha-d.c
@@ -0,0 +1,46 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+alpha_d_target_versions (void)
+{
+  VersionCondition::addPredefinedGlobalIdent ("Alpha");
+  if (TARGET_SOFT_FP)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+      VersionCondition::addPredefinedGlobalIdent ("Alpha_SoftFloat");
+    }
+  else
+    {
+      VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+      VersionCondition::addPredefinedGlobalIdent ("Alpha_HardFloat");
+    }
+}
--- a/gcc/config/alpha/alpha-protos.h
+++ b/gcc/config/alpha/alpha-protos.h
@@ -118,3 +118,6 @@ class rtl_opt_pass;
 
 extern rtl_opt_pass *make_pass_handle_trap_shadows (gcc::context *);
 extern rtl_opt_pass *make_pass_align_insns (gcc::context *);
+
+/* Routines implemented in alpha-d.c  */
+extern void alpha_d_target_versions (void);
--- a/gcc/config/alpha/alpha.h
+++ b/gcc/config/alpha/alpha.h
@@ -94,6 +94,9 @@ along with GCC; see the file COPYING3.  If not see
   while (0)
 #endif
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS alpha_d_target_versions
+
 /* Run-time compilation parameters selecting different hardware subsets.  */
 
 /* Which processor to schedule for. The cpu attribute defines a list that
--- a/gcc/config/alpha/t-alpha
+++ b/gcc/config/alpha/t-alpha
@@ -16,4 +16,9 @@
 # along with GCC; see the file COPYING3.  If not see
 # <http://www.gnu.org/licenses/>.
 
+alpha-d.o: $(srcdir)/config/alpha/alpha-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-alpha-d.o += -I$(srcdir)/d
+
 PASSES_EXTRA += $(srcdir)/config/alpha/alpha-passes.def
--- a/gcc/config/arm/arm-d.c
+++ b/gcc/config/arm/arm-d.c
@@ -0,0 +1,57 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+arm_d_target_versions (void)
+{
+  VersionCondition::addPredefinedGlobalIdent ("ARM");
+
+  if (TARGET_THUMB || TARGET_THUMB2)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("Thumb");
+      VersionCondition::addPredefinedGlobalIdent ("ARM_Thumb");
+    }
+
+  if (TARGET_HARD_FLOAT_ABI)
+    VersionCondition::addPredefinedGlobalIdent ("ARM_HardFloat");
+  else
+    {
+      if (TARGET_SOFT_FLOAT)
+	VersionCondition::addPredefinedGlobalIdent ("ARM_SoftFloat");
+      else if (TARGET_HARD_FLOAT)
+	VersionCondition::addPredefinedGlobalIdent ("ARM_SoftFP");
+    }
+
+  if (TARGET_SOFT_FLOAT)
+    VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+  else if (TARGET_HARD_FLOAT)
+    VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+}
--- a/gcc/config/arm/arm-protos.h
+++ b/gcc/config/arm/arm-protos.h
@@ -355,6 +355,9 @@ extern void arm_lang_object_attributes_init (void);
 extern void arm_register_target_pragmas (void);
 extern void arm_cpu_cpp_builtins (struct cpp_reader *);
 
+/* Defined in arm-d.c  */
+extern void arm_d_target_versions (void);
+
 extern bool arm_is_constant_pool_ref (rtx);
 
 /* The bits in this mask specify which instruction scheduling options should
--- a/gcc/config/arm/arm.h
+++ b/gcc/config/arm/arm.h
@@ -47,6 +47,9 @@ extern char arm_arch_name[];
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() arm_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS arm_d_target_versions
+
 #include "config/arm/arm-opts.h"
 
 /* The processor for which instructions should be scheduled.  */
--- a/gcc/config/arm/t-arm
+++ b/gcc/config/arm/t-arm
@@ -130,4 +130,9 @@ arm-c.o: $(srcdir)/config/arm/arm-c.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/arm/arm-c.c
 
+arm-d.o: $(srcdir)/config/arm/arm-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-arm-d.o += -I$(srcdir)/d
+
 arm-common.o: $(srcdir)/config/arm/arm-cpu-cdata.h
--- a/gcc/config/default-d.c
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
--- a/gcc/config/epiphany/epiphany-d.c
+++ b/gcc/config/epiphany/epiphany-d.c
@@ -0,0 +1,36 @@
+/* Subroutines for the D front end on the EPIPHANY architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+epiphany_d_target_versions (void)
+{
+  VersionCondition::addPredefinedGlobalIdent ("Epiphany");
+  VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+}
--- a/gcc/config/epiphany/epiphany-protos.h
+++ b/gcc/config/epiphany/epiphany-protos.h
@@ -61,3 +61,5 @@ extern bool epiphany_regno_rename_ok (unsigned src, unsigned dst);
    it uses peephole2 predicates without having all the necessary headers.  */
 extern int get_attr_sched_use_fpu (rtx_insn *);
 
+/* Routines implemented in epiphany-d.c  */
+extern void epiphany_d_target_versions (void);
--- a/gcc/config/epiphany/epiphany.h
+++ b/gcc/config/epiphany/epiphany.h
@@ -41,6 +41,9 @@ along with GCC; see the file COPYING3.  If not see
 	builtin_assert ("machine=epiphany");	\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS epiphany_d_target_versions
+
 /* Pick up the libgloss library. One day we may do this by linker script, but
    for now its static.
    libgloss might use errno/__errno, which might not have been needed when we
--- a/gcc/config/epiphany/t-epiphany
+++ b/gcc/config/epiphany/t-epiphany
@@ -36,3 +36,8 @@ specs: specs.install
 	sed -e 's,epiphany_library_extra_spec,epiphany_library_stub_spec,' \
 	-e 's,epiphany_library_build_spec,epiphany_library_extra_spec,' \
 	  < specs.install > $@ ; \
+
+epiphany-d.o: $(srcdir)/config/epiphany/epiphany-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-epiphany-d.o += -I$(srcdir)/d
--- a/gcc/config/i386/i386-d.c
+++ b/gcc/config/i386/i386-d.c
@@ -0,0 +1,49 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+ix86_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("X86_64");
+      global.params.is64bit = true;
+      if (TARGET_X32)
+	VersionCondition::addPredefinedGlobalIdent ("D_X32");
+    }
+  else
+    VersionCondition::addPredefinedGlobalIdent ("X86");
+
+  if (TARGET_80387)
+    VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+}
--- a/gcc/config/i386/i386-protos.h
+++ b/gcc/config/i386/i386-protos.h
@@ -246,6 +246,9 @@ extern bool ix86_bnd_prefixed_insn_p (rtx);
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
@@ -672,6 +672,9 @@ extern const char *host_detect_local_cpu (int argc, const char **argv);
 /* Target Pragmas.  */
 #define REGISTER_TARGET_PRAGMAS() ix86_register_pragmas ()
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS ix86_d_target_versions
+
 #ifndef CC1_SPEC
 #define CC1_SPEC "%(cc1_cpu) "
 #endif
--- a/gcc/config/i386/t-i386
+++ b/gcc/config/i386/t-i386
@@ -24,6 +24,11 @@ i386-c.o: $(srcdir)/config/i386/i386-c.c
 	  $(COMPILE) $<
 	  $(POSTCOMPILE)
 
+i386-d.o: $(srcdir)/config/i386/i386-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-i386-d.o += -I$(srcdir)/d
+
 i386.o: i386-builtin-types.inc
 
 i386-builtin-types.inc: s-i386-bt ; @true
--- a/gcc/config/ia64/ia64-d.c
+++ b/gcc/config/ia64/ia64-d.c
@@ -0,0 +1,36 @@
+/* Subroutines for the D front end on the IA64 architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+ia64_d_target_versions (void)
+{
+  VersionCondition::addPredefinedGlobalIdent ("IA64");
+  VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+}
--- a/gcc/config/ia64/ia64-protos.h
+++ b/gcc/config/ia64/ia64-protos.h
@@ -98,6 +98,9 @@ extern void ia64_hpux_handle_builtin_pragma (struct cpp_reader *);
 extern void ia64_output_function_profiler (FILE *, int);
 extern void ia64_profile_hook (int);
 
+/* Routines implemented in ia64-d.c  */
+extern void ia64_d_target_versions (void);
+
 extern void ia64_init_expanders (void);
 
 extern rtx ia64_dconst_0_5 (void);
--- a/gcc/config/ia64/ia64.h
+++ b/gcc/config/ia64/ia64.h
@@ -40,6 +40,9 @@ do {						\
 	  builtin_define("__BIG_ENDIAN__");	\
 } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS ia64_d_target_versions
+
 #ifndef SUBTARGET_EXTRA_SPECS
 #define SUBTARGET_EXTRA_SPECS
 #endif
--- a/gcc/config/ia64/t-ia64
+++ b/gcc/config/ia64/t-ia64
@@ -21,6 +21,11 @@ ia64-c.o: $(srcdir)/config/ia64/ia64-c.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/ia64/ia64-c.c
 
+ia64-d.o: $(srcdir)/config/ia64/ia64-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-ia64-d.o += -I$(srcdir)/d
+
 # genattrtab generates very long string literals.
 insn-attrtab.o-warn = -Wno-error
 
--- a/gcc/config/mips/mips-d.c
+++ b/gcc/config/mips/mips-d.c
@@ -0,0 +1,61 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+mips_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("MIPS64");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("MIPS32");
+
+  if (mips_abi == ABI_32)
+    VersionCondition::addPredefinedGlobalIdent ("MIPS_O32");
+  else if (mips_abi == ABI_EABI)
+    VersionCondition::addPredefinedGlobalIdent ("MIPS_EABI");
+  else if (mips_abi == ABI_N32)
+    VersionCondition::addPredefinedGlobalIdent ("MIPS_N32");
+  else if (mips_abi == ABI_64)
+    VersionCondition::addPredefinedGlobalIdent ("MIPS_N64");
+  else if (mips_abi == ABI_O64)
+    VersionCondition::addPredefinedGlobalIdent ("MIPS_O64");
+
+  if (TARGET_HARD_FLOAT_ABI)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("MIPS_HardFloat");
+      VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+    }
+  else if (TARGET_SOFT_FLOAT_ABI)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("MIPS_SoftFloat");
+      VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+    }
+}
--- a/gcc/config/mips/mips-protos.h
+++ b/gcc/config/mips/mips-protos.h
@@ -393,4 +393,7 @@ extern mulsidi3_gen_fn mips_mulsidi3_gen_fn (enum rtx_code);
 extern void mips_register_frame_header_opt (void);
 extern void mips_expand_vec_cond_expr (machine_mode, machine_mode, rtx *);
 
+/* Routines implemented in mips-d.c  */
+extern void mips_d_target_versions (void);
+
 #endif /* ! GCC_MIPS_PROTOS_H */
--- a/gcc/config/mips/mips.h
+++ b/gcc/config/mips/mips.h
@@ -644,6 +644,9 @@ struct mips_cpu_info {
     }									\
   while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS mips_d_target_versions
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- a/gcc/config/mips/t-mips
+++ b/gcc/config/mips/t-mips
@@ -24,3 +24,8 @@ $(srcdir)/config/mips/mips-tables.opt: $(srcdir)/config/mips/genopt.sh \
 frame-header-opt.o: $(srcdir)/config/mips/frame-header-opt.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
+
+mips-d.o: $(srcdir)/config/mips/mips-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-mips-d.o += -I$(srcdir)/d
--- a/gcc/config/nvptx/nvptx-d.c
+++ b/gcc/config/nvptx/nvptx-d.c
@@ -0,0 +1,39 @@
+/* Subroutines for the D front end on the NVPTX architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+nvptx_d_target_versions (void)
+{
+  if (TARGET_ABI64)
+    VersionCondition::addPredefinedGlobalIdent ("NVPTX64");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("NVPTX");
+}
--- a/gcc/config/nvptx/nvptx-protos.h
+++ b/gcc/config/nvptx/nvptx-protos.h
@@ -42,6 +42,9 @@ extern void nvptx_output_skip (FILE *, unsigned HOST_WIDE_INT);
 extern void nvptx_output_ascii (FILE *, const char *, unsigned HOST_WIDE_INT);
 extern void nvptx_register_pragmas (void);
 
+/* Routines implemented in nvptx-d.c  */
+extern void nvptx_d_target_versions (void);
+
 #ifdef RTX_CODE
 extern void nvptx_expand_oacc_fork (unsigned);
 extern void nvptx_expand_oacc_join (unsigned);
--- a/gcc/config/nvptx/nvptx.h
+++ b/gcc/config/nvptx/nvptx.h
@@ -37,6 +37,9 @@
         builtin_define ("__nvptx_unisimt__");	\
     } while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS nvptx_d_target_versions
+
 /* Avoid the default in ../../gcc.c, which adds "-pthread", which is not
    supported for nvptx.  */
 #define GOMP_SELF_SPECS ""
--- a/gcc/config/nvptx/t-nvptx
+++ b/gcc/config/nvptx/t-nvptx
@@ -10,3 +10,8 @@ mkoffload$(exeext): mkoffload.o collect-utils.o libcommon-target.a $(LIBIBERTY)
 	  mkoffload.o collect-utils.o libcommon-target.a $(LIBIBERTY) $(LIBS)
 
 MULTILIB_OPTIONS = mgomp
+
+nvptx-d.o: $(srcdir)/config/nvptx/nvptx-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-nvptx-d.o += -I$(srcdir)/d
--- a/gcc/config/pa/pa-d.c
+++ b/gcc/config/pa/pa-d.c
@@ -0,0 +1,44 @@
+/* Subroutines for the D front end on the HPPA architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+pa_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("HPPA64");
+  else
+    VersionCondition::addPredefinedGlobalIdent("HPPA");
+
+  if (TARGET_SOFT_FLOAT)
+    VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+}
--- a/gcc/config/pa/pa-protos.h
+++ b/gcc/config/pa/pa-protos.h
@@ -118,3 +118,6 @@ extern bool pa_modes_tieable_p (machine_mode, machine_mode);
 extern HOST_WIDE_INT pa_initial_elimination_offset (int, int);
 
 extern const int pa_magic_milli[];
+
+/* Routines implemented in pa-d.c  */
+extern void pa_d_target_versions (void);
--- a/gcc/config/pa/pa.h
+++ b/gcc/config/pa/pa.h
@@ -196,6 +196,9 @@ do {								\
     }								\
   while (0)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS pa_d_target_versions
+
 #define CC1_SPEC "%{pg:} %{p:}"
 
 #define LINK_SPEC "%{mlinker-opt:-O} %{!shared:-u main} %{shared:-b}"
--- a/gcc/config/pa/t-pa
+++ b/gcc/config/pa/t-pa
@@ -0,0 +1,4 @@
+pa-d.o: $(srcdir)/config/pa/pa-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-pa-d.o += -I$(srcdir)/d
--- a/gcc/config/powerpcspe/powerpcspe-d.c
+++ b/gcc/config/powerpcspe/powerpcspe-d.c
@@ -0,0 +1,49 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+rs6000_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("PPC64");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("PPC");
+
+  if (TARGET_HARD_FLOAT)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("PPC_HardFloat");
+      VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+    }
+  else if (TARGET_SOFT_FLOAT)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("PPC_SoftFloat");
+      VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+    }
+}
--- a/gcc/config/powerpcspe/powerpcspe-protos.h
+++ b/gcc/config/powerpcspe/powerpcspe-protos.h
@@ -244,6 +244,9 @@ extern void rs6000_target_modify_macros (bool, HOST_WIDE_INT, HOST_WIDE_INT);
 extern void (*rs6000_target_modify_macros_ptr) (bool, HOST_WIDE_INT,
 						HOST_WIDE_INT);
 
+/* Declare functions in powerpcspe-d.c  */
+extern void rs6000_d_target_versions (void);
+
 #if TARGET_MACHO
 char *output_call (rtx_insn *, rtx *, int, int);
 #endif
--- a/gcc/config/powerpcspe/powerpcspe.h
+++ b/gcc/config/powerpcspe/powerpcspe.h
@@ -822,6 +822,9 @@ extern unsigned char rs6000_recip_bits[];
 #define TARGET_CPU_CPP_BUILTINS() \
   rs6000_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS rs6000_d_target_versions
+
 /* This is used by rs6000_cpu_cpp_builtins to indicate the byte order
    we're compiling for.  Some configurations may need to override it.  */
 #define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
--- a/gcc/config/powerpcspe/t-powerpcspe
+++ b/gcc/config/powerpcspe/t-powerpcspe
@@ -26,6 +26,11 @@ powerpcspe-c.o: $(srcdir)/config/powerpcspe/powerpcspe-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
 
+powerpcspe-d.o: $(srcdir)/config/powerpcspe/powerpcspe-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-powerpcspe-d.o += -I$(srcdir)/d
+
 $(srcdir)/config/powerpcspe/powerpcspe-tables.opt: $(srcdir)/config/powerpcspe/genopt.sh \
   $(srcdir)/config/powerpcspe/powerpcspe-cpus.def
 	$(SHELL) $(srcdir)/config/powerpcspe/genopt.sh $(srcdir)/config/powerpcspe > \
--- a/gcc/config/riscv/riscv-d.c
+++ b/gcc/config/riscv/riscv-d.c
@@ -0,0 +1,44 @@
+/* Subroutines for the D front end on the ARM64 architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+riscv_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("RISCV64");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("RISCV32");
+
+  if (TARGET_HARDFLOAT)
+    VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+}
--- a/gcc/config/riscv/riscv-protos.h
+++ b/gcc/config/riscv/riscv-protos.h
@@ -74,6 +74,9 @@ extern unsigned int riscv_hard_regno_nregs (int, enum machine_mode);
 /* Routines implemented in riscv-c.c.  */
 void riscv_cpu_cpp_builtins (cpp_reader *);
 
+/* Routines implemented in riscv-d.c  */
+extern void sh_d_target_versions (void);
+
 /* Routines implemented in riscv-builtins.c.  */
 extern void riscv_atomic_assign_expand_fenv (tree *, tree *, tree *);
 extern rtx riscv_expand_builtin (tree, rtx, rtx, enum machine_mode, int);
--- a/gcc/config/riscv/riscv.h
+++ b/gcc/config/riscv/riscv.h
@@ -27,6 +27,9 @@ along with GCC; see the file COPYING3.  If not see
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() riscv_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS riscv_d_target_versions
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- a/gcc/config/riscv/t-riscv
+++ b/gcc/config/riscv/t-riscv
@@ -9,3 +9,8 @@ riscv-c.o: $(srcdir)/config/riscv/riscv-c.c $(CONFIG_H) $(SYSTEM_H) \
     coretypes.h $(TM_H) $(TREE_H) output.h $(C_COMMON_H) $(TARGET_H)
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/riscv/riscv-c.c
+
+riscv-d.o: $(srcdir)/config/riscv/riscv-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-riscv-d.o += -I$(srcdir)/d
--- a/gcc/config/rs6000/rs6000-d.c
+++ b/gcc/config/rs6000/rs6000-d.c
@@ -0,0 +1,50 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+rs6000_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("PPC64");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("PPC");
+
+  if (TARGET_HARD_FLOAT)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("PPC_HardFloat");
+      VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+    }
+  else if (TARGET_SOFT_FLOAT)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("PPC_SoftFloat");
+      VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+    }
+}
--- a/gcc/config/rs6000/rs6000-protos.h
+++ b/gcc/config/rs6000/rs6000-protos.h
@@ -244,6 +244,9 @@ extern void rs6000_target_modify_macros (bool, HOST_WIDE_INT, HOST_WIDE_INT);
 extern void (*rs6000_target_modify_macros_ptr) (bool, HOST_WIDE_INT,
 						HOST_WIDE_INT);
 
+/* Declare functions in rs6000-d.c  */
+extern void rs6000_d_target_versions (void);
+
 #if TARGET_MACHO
 char *output_call (rtx_insn *, rtx *, int, int);
 #endif
--- a/gcc/config/rs6000/rs6000.h
+++ b/gcc/config/rs6000/rs6000.h
@@ -822,6 +822,9 @@ extern unsigned char rs6000_recip_bits[];
 #define TARGET_CPU_CPP_BUILTINS() \
   rs6000_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS rs6000_d_target_versions
+
 /* This is used by rs6000_cpu_cpp_builtins to indicate the byte order
    we're compiling for.  Some configurations may need to override it.  */
 #define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
--- a/gcc/config/rs6000/t-rs6000
+++ b/gcc/config/rs6000/t-rs6000
@@ -26,6 +26,11 @@ rs6000-c.o: $(srcdir)/config/rs6000/rs6000-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
 
+rs6000-d.o: $(srcdir)/config/rs6000/rs6000-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-rs6000-d.o += -I$(srcdir)/d
+
 $(srcdir)/config/rs6000/rs6000-tables.opt: $(srcdir)/config/rs6000/genopt.sh \
   $(srcdir)/config/rs6000/rs6000-cpus.def
 	$(SHELL) $(srcdir)/config/rs6000/genopt.sh $(srcdir)/config/rs6000 > \
--- a/gcc/config/s390/s390-d.c
+++ b/gcc/config/s390/s390-d.c
@@ -0,0 +1,46 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+s390_d_target_versions (void)
+{
+  if (TARGET_ZARCH)
+    VersionCondition::addPredefinedGlobalIdent ("SystemZ");
+  else if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("S390X");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("S390");
+
+  if (TARGET_SOFT_FLOAT)
+    VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+  else if (TARGET_HARD_FLOAT)
+    VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+}
--- a/gcc/config/s390/s390-protos.h
+++ b/gcc/config/s390/s390-protos.h
@@ -155,3 +155,6 @@ extern void s390_register_target_pragmas (void);
 
 /* Routines for s390-c.c */
 extern bool s390_const_operand_ok (tree, int, int, tree);
+
+/* Routines for s390-d.c  */
+extern void s390_d_target_versions (void);
--- a/gcc/config/s390/s390.h
+++ b/gcc/config/s390/s390.h
@@ -194,6 +194,9 @@ enum processor_flags
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() s390_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS s390_d_target_versions
+
 #ifdef DEFAULT_TARGET_64BIT
 #define TARGET_DEFAULT     (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP	\
                             | MASK_OPT_HTM | MASK_OPT_VX)
--- a/gcc/config/s390/t-s390
+++ b/gcc/config/s390/t-s390
@@ -25,3 +25,8 @@ s390-c.o: $(srcdir)/config/s390/s390-c.c \
   $(TARGET_H) $(TARGET_DEF_H) $(CPPLIB_H) $(C_PRAGMA_H)
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/s390/s390-c.c
+
+s390-d.o: $(srcdir)/config/s390/s390-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-s390-d.o += -I$(srcdir)/d
--- a/gcc/config/sh/sh-d.c
+++ b/gcc/config/sh/sh-d.c
@@ -0,0 +1,41 @@
+/* Subroutines for the D front end on the SuperH architecture.
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+sh_d_target_versions (void)
+{
+  VersionCondition::addPredefinedGlobalIdent ("SH");
+
+  if (TARGET_FPU_ANY)
+    VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+}
--- a/gcc/config/sh/sh-protos.h
+++ b/gcc/config/sh/sh-protos.h
@@ -366,4 +366,7 @@ extern machine_mode sh_hard_regno_caller_save_mode (unsigned int, unsigned int,
 						    machine_mode);
 extern bool sh_can_use_simple_return_p (void);
 extern rtx sh_load_function_descriptor (rtx);
+
+/* Routines implemented in sh-d.c  */
+extern void sh_d_target_versions (void);
 #endif /* ! GCC_SH_PROTOS_H */
--- a/gcc/config/sh/sh.h
+++ b/gcc/config/sh/sh.h
@@ -31,6 +31,9 @@ extern int code_for_indirect_jump_scratch;
 
 #define TARGET_CPU_CPP_BUILTINS() sh_cpu_cpp_builtins (pfile)
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS sh_d_target_versions
+
 /* Value should be nonzero if functions must have frame pointers.
    Zero means the frame pointer need not be set up (and parms may be accessed
    via the stack pointer) in functions that seem suitable.  */
--- a/gcc/config/sh/t-sh
+++ b/gcc/config/sh/t-sh
@@ -25,6 +25,11 @@ sh-c.o: $(srcdir)/config/sh/sh-c.c \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/sh/sh-c.c
 
+sh-d.o: $(srcdir)/config/sh/sh-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-sh-d.o += -I$(srcdir)/d
+
 sh_treg_combine.o: $(srcdir)/config/sh/sh_treg_combine.cc \
   $(CONFIG_H) $(SYSTEM_H) $(TREE_H) $(TM_H) $(TM_P_H) coretypes.h
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) $<
--- a/gcc/config/sparc/sparc-d.c
+++ b/gcc/config/sparc/sparc-d.c
@@ -0,0 +1,53 @@
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
+
+#include "d/dfrontend/globals.h"
+#include "d/dfrontend/visitor.h"
+#include "d/dfrontend/cond.h"
+
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_BUILTINS.  */
+
+void
+sparc_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    VersionCondition::addPredefinedGlobalIdent ("SPARC64");
+  else
+    VersionCondition::addPredefinedGlobalIdent ("SPARC");
+
+  if (TARGET_V8PLUS)
+    VersionCondition::addPredefinedGlobalIdent ("SPARC_V8Plus");
+
+  if (TARGET_FPU)
+    {
+      VersionCondition::addPredefinedGlobalIdent ("D_HardFloat");
+      VersionCondition::addPredefinedGlobalIdent ("SPARC_HardFloat");
+    }
+  else
+    {
+      VersionCondition::addPredefinedGlobalIdent ("D_SoftFloat");
+      VersionCondition::addPredefinedGlobalIdent ("SPARC_SoftFloat");
+    }
+}
--- a/gcc/config/sparc/sparc-protos.h
+++ b/gcc/config/sparc/sparc-protos.h
@@ -116,4 +116,7 @@ bool sparc_modes_tieable_p (machine_mode, machine_mode);
 
 extern rtl_opt_pass *make_pass_work_around_errata (gcc::context *);
 
+/* Routines implemented in sparc-d.c  */
+extern void sparc_d_target_versions (void);
+
 #endif /* __SPARC_PROTOS_H__ */
--- a/gcc/config/sparc/sparc.h
+++ b/gcc/config/sparc/sparc.h
@@ -27,6 +27,9 @@ along with GCC; see the file COPYING3.  If not see
 
 #define TARGET_CPU_CPP_BUILTINS() sparc_target_macros ()
 
+/* Target CPU builtins for D.  */
+#define TARGET_D_CPU_BUILTINS sparc_d_target_versions
+
 /* Specify this in a cover file to provide bi-architecture (32/64) support.  */
 /* #define SPARC_BI_ARCH */
 
--- a/gcc/config/sparc/t-sparc
+++ b/gcc/config/sparc/t-sparc
@@ -23,3 +23,8 @@ PASSES_EXTRA += $(srcdir)/config/sparc/sparc-passes.def
 sparc-c.o: $(srcdir)/config/sparc/sparc-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
+
+sparc-d.o: $(srcdir)/config/sparc/sparc-d.c
+	  $(COMPILE) $<
+	  $(POSTCOMPILE)
+CFLAGS-sparc-d.o += -I$(srcdir)/d
--- a/gcc/configure
+++ b/gcc/configure
@@ -612,6 +612,7 @@ ISLLIBS
 GMPINC
 GMPLIBS
 target_cpu_default
+d_target_objs
 fortran_target_objs
 cxx_target_objs
 c_target_objs
@@ -619,6 +620,8 @@ use_gcc_stdint
 xm_defines
 xm_include_list
 xm_file_list
+tm_d_include_list
+tm_d_file_list
 tm_p_include_list
 tm_p_file_list
 tm_defines
@@ -11802,6 +11805,7 @@ fi
 
 tm_file="${tm_file} defaults.h"
 tm_p_file="${tm_p_file} tm-preds.h"
+tm_d_file="${tm_d_file} defaults.h"
 host_xm_file="auto-host.h ansidecl.h ${host_xm_file}"
 build_xm_file="${build_auto} ansidecl.h ${build_xm_file}"
 # We don't want ansidecl.h in target files, write code there in ISO/GNU C.
@@ -12171,6 +12175,21 @@ for f in $tm_p_file; do
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
@@ -18440,7 +18459,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18443 "configure"
+#line 18462 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -18546,7 +18565,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18549 "configure"
+#line 18568 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -29410,6 +29429,9 @@ fi
 
 
 
+
+
+
 # Echo link setup.
 if test x${build} = x${host} ; then
   if test x${host} = x${target} ; then
--- a/gcc/configure.ac
+++ b/gcc/configure.ac
@@ -1724,6 +1724,7 @@ AC_SUBST(build_subdir)
 
 tm_file="${tm_file} defaults.h"
 tm_p_file="${tm_p_file} tm-preds.h"
+tm_d_file="${tm_d_file} defaults.h"
 host_xm_file="auto-host.h ansidecl.h ${host_xm_file}"
 build_xm_file="${build_auto} ansidecl.h ${build_xm_file}"
 # We don't want ansidecl.h in target files, write code there in ISO/GNU C.
@@ -1946,6 +1947,21 @@ for f in $tm_p_file; do
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
@@ -6153,6 +6169,8 @@ AC_SUBST(tm_include_list)
 AC_SUBST(tm_defines)
 AC_SUBST(tm_p_file_list)
 AC_SUBST(tm_p_include_list)
+AC_SUBST(tm_d_file_list)
+AC_SUBST(tm_d_include_list)
 AC_SUBST(xm_file_list)
 AC_SUBST(xm_include_list)
 AC_SUBST(xm_defines)
@@ -6160,6 +6178,7 @@ AC_SUBST(use_gcc_stdint)
 AC_SUBST(c_target_objs)
 AC_SUBST(cxx_target_objs)
 AC_SUBST(fortran_target_objs)
+AC_SUBST(d_target_objs)
 AC_SUBST(target_cpu_default)
 
 AC_SUBST_FILE(language_hooks)
--- a/gcc/doc/tm.texi
+++ b/gcc/doc/tm.texi
@@ -106,6 +106,14 @@ documented as ``Common Target Hook''.  This is declared in
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
@@ -662,6 +670,18 @@ This variable specifies the initial value of @code{target_flags}.
 Its default setting is 0.
 @end deftypevr
 
+@deftypefn {D Target Hook} void TARGET_D_OS_BUILTINS (void)
+Declare predefined version identifiers
+@end deftypefn
+
+@deftypefn {D Target Hook} void TARGET_D_CPU_BUILTINS (void)
+Declare predefined version identifiers
+@end deftypefn
+
+@deftypefn {D Target Hook} unsigned TARGET_D_CRITSEC_SIZE (void)
+Declare size of critical section
+@end deftypefn
+
 @cindex optional hardware or system features
 @cindex features, optional, in system conventions
 
--- a/gcc/doc/tm.texi.in
+++ b/gcc/doc/tm.texi.in
@@ -106,6 +106,14 @@ documented as ``Common Target Hook''.  This is declared in
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
@@ -660,6 +668,12 @@ This variable specifies the initial value of @code{target_flags}.
 Its default setting is 0.
 @end deftypevr
 
+@hook TARGET_D_OS_BUILTINS
+
+@hook TARGET_D_CPU_BUILTINS
+
+@hook TARGET_D_CRITSEC_SIZE
+
 @cindex optional hardware or system features
 @cindex features, optional, in system conventions
 
--- a/gcc/genhooks.c
+++ b/gcc/genhooks.c
@@ -34,6 +34,7 @@ static struct hook_desc hook_array[] = {
 #include "target.def"
 #include "c-family/c-target.def"
 #include "common/common-target.def"
+#include "d/d-target.def"
 #undef DEFHOOK
 };
 
