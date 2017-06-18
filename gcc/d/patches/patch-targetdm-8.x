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
@@ -897,9 +897,11 @@ EXCEPT_H = except.h $(HASHTAB_H)
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
@@ -1172,6 +1174,9 @@ C_TARGET_OBJS=@c_target_objs@
 # Target specific, C++ specific object file
 CXX_TARGET_OBJS=@cxx_target_objs@
 
+# Target specific, D specific object file
+D_TARGET_OBJS=@d_target_objs@
+
 # Target specific, Fortran specific object file
 FORTRAN_TARGET_OBJS=@fortran_target_objs@
 
@@ -2115,6 +2120,12 @@ default-c.o: config/default-c.c
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
@@ -2403,6 +2414,15 @@ s-common-target-hooks-def-h: build/genhooks$(build_exeext)
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
@@ -2426,6 +2446,7 @@ s-tm-texi: build/genhooks$(build_exeext) $(srcdir)/doc/tm.texi.in
 	  && ( test $(srcdir)/doc/tm.texi -nt $(srcdir)/target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/c-family/c-target.def \
 	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/common/common-target.def \
+	    || test $(srcdir)/doc/tm.texi -nt $(srcdir)/d/d-target.def \
 	  ); then \
 	  echo >&2 ; \
 	  echo You should edit $(srcdir)/doc/tm.texi.in rather than $(srcdir)/doc/tm.texi . >&2 ; \
@@ -2570,7 +2591,8 @@ generated_files = config.h tm.h $(TM_P_H) $(TM_H) multilib.h \
        $(ALL_GTFILES_H) gtype-desc.c gtype-desc.h gcov-iov.h \
        options.h target-hooks-def.h insn-opinit.h \
        common/common-target-hooks-def.h pass-instances.def \
-       c-family/c-target-hooks-def.h params.list params.options case-cfn-macros.h \
+       c-family/c-target-hooks-def.h d/d-target-hooks-def.h \
+       params.list params.options case-cfn-macros.h \
        cfn-operators.pd
 
 #
@@ -2712,7 +2734,7 @@ build/genrecog.o : genrecog.c $(RTL_BASE_H) $(BCONFIG_H) $(SYSTEM_H)	\
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
@@ -139,6 +139,9 @@
 #  cxx_target_objs	List of extra target-dependent objects that be
 #			linked into the C++ compiler only.
 #
+#  d_target_objs	List of extra target-dependent objects that be
+#			linked into the D compiler only.
+#
 #  fortran_target_objs	List of extra target-dependent objects that be
 #			linked into the fortran compiler only.
 #
@@ -191,6 +194,9 @@
 #
 #  target_has_targetm_common	Set to yes or no depending on whether the
 #			target has its own definition of targetm_common.
+#
+#  target_has_targetdm	Set to yes or no depending on whether the target
+#			has its own definition of targetdm.
 
 out_file=
 common_out_file=
@@ -206,9 +212,11 @@ extra_gcc_objs=
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
@@ -3117,6 +3125,10 @@ if [ "$common_out_file" = "" ]; then
   fi
 fi
 
+if [ "$target_has_targetdm" = "no" ]; then
+  d_target_objs="$d_target_objs default-d.o"
+fi
+
 # Support for --with-cpu and related options (and a few unrelated options,
 # too).
 case ${with_cpu} in
--- /dev/null
+++ b/gcc/config/default-d.c
@@ -0,0 +1,31 @@
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
+#include "d/d-target.h"
+#include "d/d-target-def.h"
+
+/* Do not include tm.h or tm_p.h here; if it is useful for a target to
+   define some macros for the initializer in a header without defining
+   targetdm itself (for example, because of interactions with some
+   hooks depending on the target OS and others on the target
+   architecture), create a separate tm_c.h for only the relevant
+   definitions.  */
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
@@ -18440,7 +18441,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18443 "configure"
+#line 18444 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -18546,7 +18547,7 @@ else
   lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
   lt_status=$lt_dlunknown
   cat > conftest.$ac_ext <<_LT_EOF
-#line 18549 "configure"
+#line 18550 "configure"
 #include "confdefs.h"
 
 #if HAVE_DLFCN_H
@@ -29410,6 +29411,7 @@ fi
 
 
 
+
 # Echo link setup.
 if test x${build} = x${host} ; then
   if test x${host} = x${target} ; then
--- a/gcc/configure.ac
+++ b/gcc/configure.ac
@@ -6160,6 +6160,7 @@ AC_SUBST(use_gcc_stdint)
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
 
