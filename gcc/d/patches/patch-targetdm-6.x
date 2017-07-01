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
* Hurd
* Android
* CRuntime_Bionic
* CRuntime_Glibc
* CRuntime_Musl
* CRuntime_UClibc

These official OS versions are not implemented:
* AIX
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
@@ -543,6 +543,8 @@ tm_include_list=@tm_include_list@
 tm_defines=@tm_defines@
 tm_p_file_list=@tm_p_file_list@
 tm_p_include_list=@tm_p_include_list@
+tm_d_file_list=@tm_d_file_list@
+tm_d_include_list=@tm_d_include_list@
 build_xm_file_list=@build_xm_file_list@
 build_xm_include_list=@build_xm_include_list@
 build_xm_defines=@build_xm_defines@
@@ -837,6 +839,7 @@ BCONFIG_H = bconfig.h $(build_xm_file_list)
 CONFIG_H  = config.h  $(host_xm_file_list)
 TCONFIG_H = tconfig.h $(xm_file_list)
 TM_P_H    = tm_p.h    $(tm_p_file_list)
+TM_D_H    = tm_d.h    $(tm_d_file_list)
 GTM_H     = tm.h      $(tm_file_list) insn-constants.h
 TM_H      = $(GTM_H) insn-flags.h $(OPTIONS_H)
 
@@ -894,9 +897,11 @@ EXCEPT_H = except.h $(HASHTAB_H)
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
@@ -1169,6 +1174,9 @@ C_TARGET_OBJS=@c_target_objs@
 # Target specific, C++ specific object file
 CXX_TARGET_OBJS=@cxx_target_objs@
 
+# Target specific, D specific object file
+D_TARGET_OBJS=@d_target_objs@
+
 # Target specific, Fortran specific object file
 FORTRAN_TARGET_OBJS=@fortran_target_objs@
 
@@ -1754,6 +1762,7 @@ bconfig.h: cs-bconfig.h ; @true
 tconfig.h: cs-tconfig.h ; @true
 tm.h: cs-tm.h ; @true
 tm_p.h: cs-tm_p.h ; @true
+tm_d.h: cs-tm_d.h ; @true
 
 cs-config.h: Makefile
 	TARGET_CPU_DEFAULT="" \
@@ -1780,6 +1789,11 @@ cs-tm_p.h: Makefile
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
@@ -2109,6 +2123,12 @@ default-c.o: config/default-c.c
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
@@ -2396,6 +2416,15 @@ s-common-target-hooks-def-h: build/genhooks$(build_exeext)
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
@@ -2419,6 +2448,7 @@ s-tm-texi: build/genhooks$(build_exeext) $(srcdir)/doc/tm.texi.in
 	  && ( test $(srcdir)/doc/tm.texi -nt $(srcdir)/target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/c-family/c-target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/common/common-target.def \
+	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/d/d-target.def \
 	  ); then \
 	  echo >&2 ; \
 	  echo You should edit $(srcdir)/doc/tm.texi.in rather than $(srcdir)/doc/tm.texi . >&2 ; \
@@ -2556,13 +2586,14 @@ s-gtype: build/gengtype$(build_exeext) $(filter-out [%], $(GTFILES)) \
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
@@ -2704,7 +2735,7 @@ build/genrecog.o : genrecog.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
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
 powerpc*-*-*)
 	cpu_type=rs6000
@@ -455,6 +477,7 @@ powerpc*-*-*)
 riscv*)
 	cpu_type=riscv
 	extra_objs="riscv-builtins.o riscv-c.o"
+	d_target_objs="riscv-d.o"
 	;;
 rs6000*-*-*)
 	extra_options="${extra_options} g.opt fused-madd.opt rs6000/rs6000-tables.opt"
@@ -463,6 +486,7 @@ sparc*-*-*)
 	cpu_type=sparc
 	c_target_objs="sparc-c.o"
 	cxx_target_objs="sparc-c.o"
+	d_target_objs="sparc-d.o"
 	extra_headers="visintrin.h"
 	;;
 spu*-*-*)
@@ -470,6 +494,7 @@ spu*-*-*)
 	;;
 s390*-*-*)
 	cpu_type=s390
+	d_target_objs="s390-d.o"
 	extra_options="${extra_options} fused-madd.opt"
 	extra_headers="s390intrin.h htmintrin.h htmxlintrin.h vecintrin.h"
 	;;
@@ -499,10 +524,13 @@ tilepro*-*-*)
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
@@ -635,8 +663,10 @@ case ${target} in
   extra_options="${extra_options} darwin.opt"
   c_target_objs="${c_target_objs} darwin-c.o"
   cxx_target_objs="${cxx_target_objs} darwin-c.o"
+  d_target_objs="${d_target_objs} darwin-d.o"
   fortran_target_objs="darwin-f.o"
   target_has_targetcm=yes
+  target_has_targetdm=yes
   extra_objs="darwin.o"
   extra_gcc_objs="darwin-driver.o"
   default_use_cxa_atexit=yes
@@ -661,6 +691,9 @@ case ${target} in
       exit 1
       ;;
   esac
+  d_target_objs="${d_target_objs} dragonfly-d.o"
+  target_has_targetdm=yes
+  tmake_file="${tmake_file} t-dragonfly"
   extra_options="$extra_options rpath.opt dragonfly.opt"
   default_use_cxa_atexit=yes
   use_gcc_stdint=wrap
@@ -704,6 +737,9 @@ case ${target} in
       ;;
   esac
   fbsd_tm_file="${fbsd_tm_file} freebsd-spec.h freebsd.h freebsd-stdint.h"
+  d_target_objs="${d_target_objs} freebsd-d.o"
+  target_has_targetdm=yes
+  tmake_file="${tmake_file} t-freebsd"
   extra_options="$extra_options rpath.opt freebsd.opt"
   case ${target} in
     *-*-freebsd[345].*)
@@ -771,14 +807,19 @@ case ${target} in
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
   gas=yes
   gnu_ld=yes
   use_gcc_stdint=wrap
+  d_target_objs="${d_target_objs} netbsd-d.o"
+  target_has_targetdm=yes
+  tmake_file="${tmake_file} t-netbsd"
 
   # NetBSD 2.0 and later get POSIX threads enabled by default.
   # Allow them to be explicitly enabled on any other version.
@@ -807,6 +848,8 @@ case ${target} in
   ;;
 *-*-openbsd*)
   tmake_file="t-openbsd"
+  d_target_objs="${d_target_objs} netbsd-d.o"
+  target_has_targetdm=yes
   case ${enable_threads} in
     yes)
       thread_file='posix'
@@ -872,6 +915,8 @@ case ${target} in
   tmake_file="${tmake_file} t-sol2 t-slibgcc"
   c_target_objs="${c_target_objs} sol2-c.o"
   cxx_target_objs="${cxx_target_objs} sol2-c.o sol2-cxx.o"
+  d_target_objs="${d_target_objs} sol2-d.o"
+  target_has_targetdm="yes"
   extra_objs="sol2.o sol2-stubs.o"
   extra_options="${extra_options} sol2.opt"
   case ${enable_threads}:${have_pthread_h}:${have_thread_h} in
@@ -1708,7 +1753,9 @@ i[34567]86-*-mingw* | x86_64-*-mingw*)
 	xm_file=i386/xm-mingw32.h
 	c_target_objs="${c_target_objs} winnt-c.o"
 	cxx_target_objs="${cxx_target_objs} winnt-c.o"
+	d_target_objs="${d_target_objs} winnt-d.o"
 	target_has_targetcm="yes"
+	target_has_targetdm="yes"
 	case ${target} in
 		x86_64-*-* | *-w64-*)
 			need_64bit_isa=yes
@@ -3081,6 +3128,10 @@ if [ "$common_out_file" = "" ]; then
   fi
 fi
 
+if [ "$target_has_targetdm" = "no" ]; then
+  d_target_objs="$d_target_objs default-d.o"
+fi
+
 # Support for --with-cpu and related options (and a few unrelated options,
 # too).
 case ${with_cpu} in
@@ -4483,6 +4534,8 @@ case ${target} in
 		then
 			target_cpu_default2="MASK_GAS"
 		fi
+		d_target_objs="${d_target_objs} pa-d.o"
+		tmake_file="pa/t-pa ${tmake_file}"
 		;;
 
 	fido*-*-* | m68k*-*-*)
@@ -4568,12 +4621,14 @@ case ${target} in
 		out_file=rs6000/rs6000.c
 		c_target_objs="${c_target_objs} rs6000-c.o"
 		cxx_target_objs="${cxx_target_objs} rs6000-c.o"
+		d_target_objs="${d_target_objs} rs6000-d.o"
 		tmake_file="rs6000/t-rs6000 ${tmake_file}"
 		;;
 
 	sh[123456ble]*-*-* | sh-*-*)
 		c_target_objs="${c_target_objs} sh-c.o"
 		cxx_target_objs="${cxx_target_objs} sh-c.o"
+		d_target_objs="${d_target_objs} sh-d.o"
 		;;
 
 	sparc*-*-*)
--- /dev/null
+++ b/gcc/config/aarch64/aarch64-d.c
@@ -0,0 +1,31 @@
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
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for ARM64 targets.  */
+
+void
+aarch64_d_target_versions (void)
+{
+  d_add_builtin_version ("AArch64");
+  d_add_builtin_version ("D_HardFloat");
+}
--- a/gcc/config/aarch64/aarch64-linux.h
+++ b/gcc/config/aarch64/aarch64-linux.h
@@ -80,6 +80,8 @@
     }						\
   while (0)
 
+#define GNU_USER_TARGET_D_CRITSEC_SIZE 48
+
 #define TARGET_ASM_FILE_END file_end_indicate_exec_stack
 
 /* Uninitialized common symbols in non-PIE executables, even with
--- a/gcc/config/aarch64/aarch64-protos.h
+++ b/gcc/config/aarch64/aarch64-protos.h
@@ -473,6 +473,9 @@ enum aarch64_parse_opt_result aarch64_parse_extension (const char *,
 std::string aarch64_get_extension_string_for_isa_flags (unsigned long,
 							unsigned long);
 
+/* Defined in aarch64-d.c  */
+extern void aarch64_d_target_versions (void);
+
 rtl_opt_pass *make_pass_fma_steering (gcc::context *ctxt);
 
 #endif /* GCC_AARCH64_PROTOS_H */
--- a/gcc/config/aarch64/aarch64.h
+++ b/gcc/config/aarch64/aarch64.h
@@ -26,6 +26,9 @@
 #define TARGET_CPU_CPP_BUILTINS()	\
   aarch64_cpu_cpp_builtins (pfile)
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS aarch64_d_target_versions
+
 
 
 #define REGISTER_TARGET_PRAGMAS() aarch64_register_pragmas ()
--- a/gcc/config/aarch64/t-aarch64
+++ b/gcc/config/aarch64/t-aarch64
@@ -56,6 +56,10 @@ aarch64-c.o: $(srcdir)/config/aarch64/aarch64-c.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/aarch64/aarch64-c.c
 
+aarch64-d.o: $(srcdir)/config/aarch64/aarch64-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 PASSES_EXTRA += $(srcdir)/config/aarch64/aarch64-passes.def
 
 cortex-a57-fma-steering.o: $(srcdir)/config/aarch64/cortex-a57-fma-steering.c \
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
+#include "target.h"
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS alpha_d_target_versions
+
 /* Run-time compilation parameters selecting different hardware subsets.  */
 
 /* Which processor to schedule for. The cpu attribute defines a list that
--- a/gcc/config/alpha/t-alpha
+++ b/gcc/config/alpha/t-alpha
@@ -16,4 +16,8 @@
 # along with GCC; see the file COPYING3.  If not see
 # <http://www.gnu.org/licenses/>.
 
+alpha-d.o: $(srcdir)/config/alpha/alpha-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 PASSES_EXTRA += $(srcdir)/config/alpha/alpha-passes.def
--- /dev/null
+++ b/gcc/config/arm/arm-d.c
@@ -0,0 +1,52 @@
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
+#include "target.h"
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS arm_d_target_versions
+
 #include "config/arm/arm-opts.h"
 
 /* The processor for which instructions should be scheduled.  */
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
@@ -130,4 +130,8 @@ arm-c.o: $(srcdir)/config/arm/arm-c.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/arm/arm-c.c
 
+arm-d.o: $(srcdir)/config/arm/arm-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 arm-common.o: $(srcdir)/config/arm/arm-cpu-cdata.h
--- /dev/null
+++ b/gcc/config/darwin-d.c
@@ -0,0 +1,55 @@
+/* Darwin support needed only by D front-end.
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
+/* Implement TARGET_D_OS_VERSIONS for Darwin targets.  */
+
+static void
+darwin_d_os_builtins (void)
+{
+  d_add_builtin_version ("OSX");
+  d_add_builtin_version ("darwin");
+  d_add_builtin_version ("Posix");
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for Darwin targets.  */
+
+static unsigned
+darwin_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t.  */
+  if (TYPE_PRECISION (long_integer_type_node) == 64
+      && POINTER_SIZE == 64
+      && TYPE_PRECISION (integer_type_node) == 32)
+    return 64;
+  else
+    return 44;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS darwin_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE darwin_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
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
+++ b/gcc/config/dragonfly-d.c
@@ -0,0 +1,49 @@
+/* DragonFly support needed only by D front-end.
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
+/* Implement TARGET_D_OS_VERSIONS for DragonFly targets.  */
+
+static void
+dragonfly_d_os_builtins (void)
+{
+  d_add_builtin_version ("DragonFlyBSD");
+  d_add_builtin_version ("Posix");
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for DragonFly targets.  */
+
+static unsigned
+dragonfly_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t, an opaque pointer.  */
+  return POINTER_SIZE_UNITS;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS dragonfly_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE dragonfly_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
--- /dev/null
+++ b/gcc/config/epiphany/epiphany-d.c
@@ -0,0 +1,31 @@
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
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for EPIPHANY targets.  */
+
+void
+epiphany_d_target_versions (void)
+{
+  d_add_builtin_version ("Epiphany");
+  d_add_builtin_version ("D_HardFloat");
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS epiphany_d_target_versions
+
 /* Pick up the libgloss library. One day we may do this by linker script, but
    for now its static.
    libgloss might use errno/__errno, which might not have been needed when we
--- a/gcc/config/epiphany/t-epiphany
+++ b/gcc/config/epiphany/t-epiphany
@@ -36,3 +36,7 @@ specs: specs.install
 	sed -e 's,epiphany_library_extra_spec,epiphany_library_stub_spec,' \
 	-e 's,epiphany_library_build_spec,epiphany_library_extra_spec,' \
 	  < specs.install > $@ ; \
+
+epiphany-d.o: $(srcdir)/config/epiphany/epiphany-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/freebsd-d.c
@@ -0,0 +1,49 @@
+/* FreeBSD support needed only by D front-end.
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
+/* Implement TARGET_D_OS_VERSIONS for FreeBSD targets.  */
+
+static void
+freebsd_d_os_builtins (void)
+{
+  d_add_builtin_version ("FreeBSD");
+  d_add_builtin_version ("Posix");
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for FreeBSD targets.  */
+
+static unsigned
+freebsd_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t, an opaque pointer.  */
+  return POINTER_SIZE_UNITS;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS freebsd_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE freebsd_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
--- /dev/null
+++ b/gcc/config/glibc-d.c
@@ -0,0 +1,72 @@
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
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+#include "tm_p.h"
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
+  else if (OPTION_MUSL)
+    d_add_builtin_version ("CRuntime_Musl");
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
@@ -31,3 +31,6 @@ along with GCC.  If not, see <http://www.gnu.org/licenses/>.
 	builtin_assert ("system=unix");		\
 	builtin_assert ("system=posix");	\
     } while (0)
+
+#define GNU_USER_TARGET_D_OS_VERSIONS()		\
+  builtin_version ("Hurd")
--- a/gcc/config/i386/cygwin.h
+++ b/gcc/config/i386/cygwin.h
@@ -29,6 +29,12 @@ along with GCC; see the file COPYING3.  If not see
     }								\
   while (0)
 
+#define EXTRA_TARGET_D_OS_VERSIONS()				\
+    do {							\
+      builtin_version ("Cygwin");				\
+      builtin_version ("Posix");				\
+    } while (0)
+
 #undef CPP_SPEC
 #define CPP_SPEC "%(cpp_cpu) %{posix:-D_POSIX_SOURCE} \
   %{!ansi:-Dunix} \
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
--- a/gcc/config/i386/mingw32.h
+++ b/gcc/config/i386/mingw32.h
@@ -53,6 +53,16 @@ along with GCC; see the file COPYING3.  If not see
     }								\
   while (0)
 
+#define EXTRA_TARGET_D_OS_VERSIONS()				\
+    do {							\
+      builtin_version ("MinGW");				\
+								\
+      if (TARGET_64BIT && ix86_abi == MS_ABI)			\
+	  builtin_version ("Win64");				\
+      else if (!TARGET_64BIT)					\
+        builtin_version ("Win32");				\
+    } while (0)
+
 #ifndef TARGET_USE_PTHREAD_BY_DEFAULT
 #define SPEC_PTHREAD1 "pthread"
 #define SPEC_PTHREAD2 "!no-pthread"
--- a/gcc/config/i386/t-cygming
+++ b/gcc/config/i386/t-cygming
@@ -32,6 +32,9 @@ winnt-cxx.o: $(srcdir)/config/i386/winnt-cxx.c $(CONFIG_H) $(SYSTEM_H) coretypes
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 	$(srcdir)/config/i386/winnt-cxx.c
 
+winnt-d.o: config/winnt-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
 
 winnt-stubs.o: $(srcdir)/config/i386/winnt-stubs.c $(CONFIG_H) $(SYSTEM_H) coretypes.h \
   $(TM_H) $(RTL_H) $(REGS_H) hard-reg-set.h output.h $(TREE_H) flags.h \
--- a/gcc/config/i386/t-i386
+++ b/gcc/config/i386/t-i386
@@ -24,6 +24,10 @@ i386-c.o: $(srcdir)/config/i386/i386-c.c
 	  $(COMPILE) $<
 	  $(POSTCOMPILE)
 
+i386-d.o: $(srcdir)/config/i386/i386-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 i386.o: i386-builtin-types.inc
 
 i386-builtin-types.inc: s-i386-bt ; @true
--- /dev/null
+++ b/gcc/config/ia64/ia64-d.c
@@ -0,0 +1,31 @@
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
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for IA64 targets.  */
+
+void
+ia64_d_target_versions (void)
+{
+  d_add_builtin_version ("IA64");
+  d_add_builtin_version ("D_HardFloat");
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS ia64_d_target_versions
+
 #ifndef SUBTARGET_EXTRA_SPECS
 #define SUBTARGET_EXTRA_SPECS
 #endif
--- a/gcc/config/ia64/t-ia64
+++ b/gcc/config/ia64/t-ia64
@@ -21,6 +21,10 @@ ia64-c.o: $(srcdir)/config/ia64/ia64-c.c $(CONFIG_H) $(SYSTEM_H) \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/ia64/ia64-c.c
 
+ia64-d.o: $(srcdir)/config/ia64/ia64-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 # genattrtab generates very long string literals.
 insn-attrtab.o-warn = -Wno-error
 
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
@@ -53,6 +53,9 @@ see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
 	builtin_assert ("system=posix");			\
     } while (0)
 
+#define GNU_USER_TARGET_D_OS_VERSIONS()				\
+  builtin_version ("linux")
+
 /* Determine which dynamic linker to use depending on whether GLIBC or
    uClibc or Bionic or musl is the default C library and whether
    -muclibc or -mglibc or -mbionic or -mmusl has been passed to change
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
+#include "target.h"
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS mips_d_target_versions
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- a/gcc/config/mips/t-mips
+++ b/gcc/config/mips/t-mips
@@ -24,3 +24,7 @@ $(srcdir)/config/mips/mips-tables.opt: $(srcdir)/config/mips/genopt.sh \
 frame-header-opt.o: $(srcdir)/config/mips/frame-header-opt.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
+
+mips-d.o: $(srcdir)/config/mips/mips-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/netbsd-d.c
@@ -0,0 +1,49 @@
+/* NetBSD support needed only by D front-end.
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
+/* Implement TARGET_D_OS_VERSIONS for NetBSD targets.  */
+
+static void
+netbsd_d_os_builtins (void)
+{
+  d_add_builtin_version ("NetBSD");
+  d_add_builtin_version ("Posix");
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for NetBSD targets.  */
+
+static unsigned
+netbsd_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t.  */
+  return 48;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS netbsd_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE netbsd_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
--- /dev/null
+++ b/gcc/config/nvptx/nvptx-d.c
@@ -0,0 +1,34 @@
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
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for NVPTX targets.  */
+
+void
+nvptx_d_target_versions (void)
+{
+  if (TARGET_ABI64)
+    d_add_builtin_version ("NVPTX64");
+  else
+    d_add_builtin_version ("NVPTX");
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS nvptx_d_target_versions
+
 /* Avoid the default in ../../gcc.c, which adds "-pthread", which is not
    supported for nvptx.  */
 #define GOMP_SELF_SPECS ""
--- a/gcc/config/nvptx/t-nvptx
+++ b/gcc/config/nvptx/t-nvptx
@@ -10,3 +10,7 @@ mkoffload$(exeext): mkoffload.o collect-utils.o libcommon-target.a $(LIBIBERTY)
 	  mkoffload.o collect-utils.o libcommon-target.a $(LIBIBERTY) $(LIBS)
 
 MULTILIB_OPTIONS = mgomp
+
+nvptx-d.o: $(srcdir)/config/nvptx/nvptx-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/openbsd-d.c
@@ -0,0 +1,49 @@
+/* OpenBSD support needed only by D front-end.
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
+/* Implement TARGET_D_OS_VERSIONS for OpenBSD targets.  */
+
+static void
+openbsd_d_os_builtins (void)
+{
+  d_add_builtin_version ("OpenBSD");
+  d_add_builtin_version ("Posix");
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for OpenBSD targets.  */
+
+static unsigned
+openbsd_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t, an opaque pointer.  */
+  return POINTER_SIZE_UNITS;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS openbsd_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE openbsd_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
--- /dev/null
+++ b/gcc/config/pa/pa-d.c
@@ -0,0 +1,39 @@
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
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for HPPA targets.  */
+
+void
+pa_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    d_add_builtin_version ("HPPA64");
+  else
+    d_add_builtin_version("HPPA");
+
+  if (TARGET_SOFT_FLOAT)
+    d_add_builtin_version ("D_SoftFloat");
+  else
+    d_add_builtin_version ("D_HardFloat");
+}
--- a/gcc/config/pa/pa-linux.h
+++ b/gcc/config/pa/pa-linux.h
@@ -27,6 +27,8 @@ along with GCC; see the file COPYING3.  If not see
     }						\
   while (0)
 
+#define GNU_USER_TARGET_D_CRITSEC_SIZE 48
+
 #undef CPP_SPEC
 #define CPP_SPEC "%{posix:-D_POSIX_SOURCE} %{pthread:-D_REENTRANT}"
 
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS pa_d_target_versions
+
 #define CC1_SPEC "%{pg:} %{p:}"
 
 #define LINK_SPEC "%{mlinker-opt:-O} %{!shared:-u main} %{shared:-b}"
--- /dev/null
+++ b/gcc/config/pa/t-pa
@@ -0,0 +1,3 @@
+pa-d.o: $(srcdir)/config/pa/pa-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/riscv/riscv-d.c
@@ -0,0 +1,39 @@
+/* Subroutines for the D front end on the RISC-V architecture.
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
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for RISC-V targets.  */
+
+void
+riscv_d_target_versions (void)
+{
+  if (TARGET_64BIT)
+    d_add_builtin_version ("RISCV64");
+  else
+    d_add_builtin_version ("RISCV32");
+
+  if (TARGET_HARDFLOAT)
+    d_add_builtin_version ("D_HardFloat");
+  else
+    d_add_builtin_version ("D_SoftFloat");
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS riscv_d_target_versions
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- a/gcc/config/riscv/t-riscv
+++ b/gcc/config/riscv/t-riscv
@@ -9,3 +9,7 @@ riscv-c.o: $(srcdir)/config/riscv/riscv-c.c $(CONFIG_H) $(SYSTEM_H) \
     coretypes.h $(TM_H) $(TREE_H) output.h $(C_COMMON_H) $(TARGET_H)
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/riscv/riscv-c.c
+
+riscv-d.o: $(srcdir)/config/riscv/riscv-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
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
+#include "target.h"
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
@@ -243,6 +243,9 @@ extern void rs6000_target_modify_macros (bool, HOST_WIDE_INT, HOST_WIDE_INT);
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS rs6000_d_target_versions
+
 /* This is used by rs6000_cpu_cpp_builtins to indicate the byte order
    we're compiling for.  Some configurations may need to override it.  */
 #define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
--- a/gcc/config/rs6000/t-rs6000
+++ b/gcc/config/rs6000/t-rs6000
@@ -26,6 +26,10 @@ rs6000-c.o: $(srcdir)/config/rs6000/rs6000-c.c
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
+#include "target.h"
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS s390_d_target_versions
+
 #ifdef DEFAULT_TARGET_64BIT
 #define TARGET_DEFAULT     (MASK_64BIT | MASK_ZARCH | MASK_HARD_DFP	\
                             | MASK_OPT_HTM | MASK_OPT_VX)
--- a/gcc/config/s390/t-s390
+++ b/gcc/config/s390/t-s390
@@ -25,3 +25,7 @@ s390-c.o: $(srcdir)/config/s390/s390-c.c \
   $(TARGET_H) $(TARGET_DEF_H) $(CPPLIB_H) $(C_PRAGMA_H)
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/s390/s390-c.c
+
+s390-d.o: $(srcdir)/config/s390/s390-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/sh/sh-d.c
@@ -0,0 +1,36 @@
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
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Implement TARGET_D_CPU_VERSIONS for SuperH targets.  */
+
+void
+sh_d_target_versions (void)
+{
+  d_add_builtin_version ("SH");
+
+  if (TARGET_FPU_ANY)
+    d_add_builtin_version ("D_HardFloat");
+  else
+    d_add_builtin_version ("D_SoftFloat");
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS sh_d_target_versions
+
 /* Value should be nonzero if functions must have frame pointers.
    Zero means the frame pointer need not be set up (and parms may be accessed
    via the stack pointer) in functions that seem suitable.  */
--- a/gcc/config/sh/t-sh
+++ b/gcc/config/sh/t-sh
@@ -25,6 +25,10 @@ sh-c.o: $(srcdir)/config/sh/sh-c.c \
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) \
 		$(srcdir)/config/sh/sh-c.c
 
+sh-d.o: $(srcdir)/config/sh/sh-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 sh_treg_combine.o: $(srcdir)/config/sh/sh_treg_combine.cc \
   $(CONFIG_H) $(SYSTEM_H) $(TREE_H) $(TM_H) $(TM_P_H) coretypes.h
 	$(COMPILER) -c $(ALL_COMPILERFLAGS) $(ALL_CPPFLAGS) $(INCLUDES) $<
--- /dev/null
+++ b/gcc/config/sol2-d.c
@@ -0,0 +1,49 @@
+/* Solaris support needed only by D front-end.
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
+/* Implement TARGET_D_OS_VERSIONS for Solaris targets.  */
+
+static void
+solaris_d_os_builtins (void)
+{
+  d_add_builtin_version ("Solaris");
+  d_add_builtin_version ("Posix");
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for Solaris targets.  */
+
+static unsigned
+solaris_d_critsec_size (void)
+{
+  /* This is the sizeof pthread_mutex_t.  */
+  return 24;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS solaris_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE solaris_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
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
+#include "target.h"
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
 
+/* Target CPU versions for D.  */
+#define TARGET_D_CPU_VERSIONS sparc_d_target_versions
+
 /* Specify this in a cover file to provide bi-architecture (32/64) support.  */
 /* #define SPARC_BI_ARCH */
 
--- a/gcc/config/sparc/t-sparc
+++ b/gcc/config/sparc/t-sparc
@@ -23,3 +23,7 @@ PASSES_EXTRA += $(srcdir)/config/sparc/sparc-passes.def
 sparc-c.o: $(srcdir)/config/sparc/sparc-c.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
+
+sparc-d.o: $(srcdir)/config/sparc/sparc-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- a/gcc/config/t-darwin
+++ b/gcc/config/t-darwin
@@ -26,6 +26,9 @@ darwin-c.o: $(srcdir)/config/darwin-c.c
 	$(COMPILE) $(PREPROCESSOR_DEFINES) $<
 	$(POSTCOMPILE)
 
+darwin-d.o: $(srcdir)/config/darwin-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
 
 darwin-f.o: $(srcdir)/config/darwin-f.c
 	$(COMPILE) $<
--- /dev/null
+++ b/gcc/config/t-dragonfly
@@ -0,0 +1,3 @@
+dragonfly-d.o: config/dragonfly-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- /dev/null
+++ b/gcc/config/t-freebsd
@@ -0,0 +1,3 @@
+freebsd-d.o: config/freebsd-d.c
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
--- /dev/null
+++ b/gcc/config/t-netbsd
@@ -0,0 +1,3 @@
+netbsd-d.o: config/netbsd-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- a/gcc/config/t-openbsd
+++ b/gcc/config/t-openbsd
@@ -1,2 +1,6 @@
 # We don't need GCC's own include files.
 USER_H = $(EXTRA_HEADERS)
+
+openbsd-d.o: config/openbsd-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
--- a/gcc/config/t-sol2
+++ b/gcc/config/t-sol2
@@ -26,6 +26,11 @@ sol2-cxx.o: $(srcdir)/config/sol2-cxx.c
 	$(COMPILE) $<
 	$(POSTCOMPILE)
 
+# Solaris-specific D support.
+sol2-d.o: $(srcdir)/config/sol2-d.c
+	$(COMPILE) $<
+	$(POSTCOMPILE)
+
 # Corresponding stub routines.
 sol2-stubs.o: $(srcdir)/config/sol2-stubs.c
 	$(COMPILE) $<
--- /dev/null
+++ b/gcc/config/winnt-d.c
@@ -0,0 +1,60 @@
+/* Windows support needed only by D front-end.
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
+#include "target.h"
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+#include "tm_p.h"
+
+/* Implement TARGET_D_OS_VERSIONS for Windows targets.  */
+
+static void
+winnt_d_os_builtins (void)
+{
+  d_add_builtin_version ("Windows");
+
+#define builtin_version(TXT) d_add_builtin_version (TXT)
+
+#ifdef EXTRA_TARGET_D_OS_VERSIONS
+  EXTRA_TARGET_D_OS_VERSIONS ();
+#endif
+}
+
+/* Implement TARGET_D_CRITSEC_SIZE for Windows targets.  */
+
+static unsigned
+winnt_d_critsec_size (void)
+{
+  /* This is the sizeof CRITICAL_SECTION.  */
+  if (TYPE_PRECISION (long_integer_type_node) == 64
+      && POINTER_SIZE == 64
+      && TYPE_PRECISION (integer_type_node) == 32)
+    return 40;
+  else
+    return 24;
+}
+
+#undef TARGET_D_OS_VERSIONS
+#define TARGET_D_OS_VERSIONS winnt_d_os_builtins
+
+#undef TARGET_D_CRITSEC_SIZE
+#define TARGET_D_CRITSEC_SIZE winnt_d_critsec_size
+
+struct gcc_targetdm targetdm = TARGETDM_INITIALIZER;
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
@@ -11798,6 +11801,7 @@ fi
 
 tm_file="${tm_file} defaults.h"
 tm_p_file="${tm_p_file} tm-preds.h"
+tm_d_file="${tm_d_file} defaults.h"
 host_xm_file="auto-host.h ansidecl.h ${host_xm_file}"
 build_xm_file="${build_auto} ansidecl.h ${build_xm_file}"
 # We don't want ansidecl.h in target files, write code there in ISO/GNU C.
@@ -12152,6 +12156,21 @@ for f in $tm_p_file; do
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
@@ -18433,7 +18452,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18436 "configure"
+#line 18455 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -18539,7 +18558,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18542 "configure"
+#line 18561 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -29403,6 +29422,9 @@ fi
 
 
 
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
@@ -1936,6 +1937,21 @@ for f in $tm_p_file; do
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
@@ -6155,6 +6171,8 @@ AC_SUBST(tm_include_list)
 AC_SUBST(tm_defines)
 AC_SUBST(tm_p_file_list)
 AC_SUBST(tm_p_include_list)
+AC_SUBST(tm_d_file_list)
+AC_SUBST(tm_d_include_list)
 AC_SUBST(xm_file_list)
 AC_SUBST(xm_include_list)
 AC_SUBST(xm_defines)
@@ -6162,6 +6180,7 @@ AC_SUBST(use_gcc_stdint)
 AC_SUBST(c_target_objs)
 AC_SUBST(cxx_target_objs)
 AC_SUBST(fortran_target_objs)
+AC_SUBST(d_target_objs)
 AC_SUBST(target_cpu_default)
 
 AC_SUBST_FILE(language_hooks)
--- a/gcc/doc/tm.texi
+++ b/gcc/doc/tm.texi
@@ -52,6 +52,7 @@ through the macros defined in the @file{.h} file.
 * MIPS Coprocessors::   MIPS coprocessor support and how to customize it.
 * PCH Target::          Validity checking for precompiled headers.
 * C++ ABI::             Controlling C++ ABI changes.
+* D Language and ABI::  Controlling D ABI changes.
 * Named Address Spaces:: Adding support for named address spaces
 * Misc::                Everything else.
 @end menu
@@ -106,6 +107,14 @@ documented as ``Common Target Hook''.  This is declared in
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
@@ -10473,6 +10482,22 @@ unloaded. The default is to return false.
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
@@ -52,6 +52,7 @@ through the macros defined in the @file{.h} file.
 * MIPS Coprocessors::   MIPS coprocessor support and how to customize it.
 * PCH Target::          Validity checking for precompiled headers.
 * C++ ABI::             Controlling C++ ABI changes.
+* D Language and ABI::  Controlling D ABI changes.
 * Named Address Spaces:: Adding support for named address spaces
 * Misc::                Everything else.
 @end menu
@@ -106,6 +107,14 @@ documented as ``Common Target Hook''.  This is declared in
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
@@ -7471,6 +7480,16 @@ floating-point support; they are not included in this mechanism.
 
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
@@ -34,6 +34,7 @@ static struct hook_desc hook_array[] = {
 #include "target.def"
 #include "c-family/c-target.def"
 #include "common/common-target.def"
+#include "d/d-target.def"
 #undef DEFHOOK
 };
 
