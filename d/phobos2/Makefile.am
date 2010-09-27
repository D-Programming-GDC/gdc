# GDC -- D front-end for GCC
# Copyright (C) 2004 David Friedman
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AUTOMAKE_OPTIONS = 1.9.4 foreign no-dependencies

ACLOCAL_AMFLAGS = -I . -I ..

OUR_CFLAGS=@DEFS@ -I . -I $(srcdir)/gcc
D_EXTRA_DFLAGS=-nostdinc -pipe
D_RT_FLAGS = -I $(srcdir)/compiler/gdc -I $(srcdir)/gc/basic -I $(srcdir)/import
ALL_DFLAGS = $(DFLAGS) $(D_GC_FLAGS) $(D_RT_FLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

toolexecdir = $(phobos_toolexecdir)
toolexeclibdir = $(phobos_toolexeclibdir)

all-local: libgphobos2.a libgdruntime.a

SUFFIXES = .d

%.o : %.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS)  \
	-I $(srcdir) -I ./$(host_alias) -c $<

# %.o : %.c
# Use .c.o to override Automake
.c.o:
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

%.t.o : %.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fno-release -funittest -fversion=Unittest -I $(srcdir) -I ./$(host_alias) -c $<

%.t.o : %.o
	cp $< $@


#-------------------#
#-    X3 Config    -#
#-------------------#

config_d_src          =$(host_alias)/gcc/config/config.d
config_libc_d_src     =$(host_alias)/gcc/config/libc.d
config_errno_d_src    =$(host_alias)/gcc/config/errno.d
config_fpcls_d_src    =$(host_alias)/gcc/config/fpcls.d
config_mathfuncs_d_src=$(host_alias)/gcc/config/mathfuncs.d
config_unix_d_src     =$(host_alias)/gcc/config/unix.d

X3=CC_FOR_BUILD="$(CC_FOR_BUILD) -I $(srcdir)/config" BUILD_LIBIBERTY=$(BUILD_LIBIBERTY) $(srcdir)/config/x3
X3_GCC_COMMAND=$(CC) $(OUR_CFLAGS)
X3_DEPS=config/x3.o config/x3main.o

config/x3.o: config/x3.c config/x3.h
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) -g -Wall -I $(LIBIBERTY_H_PATH) -c -o $@ $<

config/x3main.o: config/x3main.c config/x3.h
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) -g -Wall -c -o $@ $<

$(config_d_src): frag-ac stamp-tgtdir
	cp $< $@

$(config_libc_d_src): config/libc.x3 $(X3_DEPS) stamp-tgtdir
	$(X3) $< -o $@ $(X3_GCC_COMMAND)

$(config_errno_d_src): config/errno.x3 $(X3_DEPS) stamp-tgtdir
	$(X3) $< -o $@ $(X3_GCC_COMMAND)

$(config_fpcls_d_src): config/fpcls.x3 $(X3_DEPS) stamp-tgtdir
	$(X3) $< -o $@ $(X3_GCC_COMMAND)

$(config_mathfuncs_d_src): frag-math stamp-tgtdir
	cp $< $@

$(config_unix_d_src): config/unix.x3 $(X3_DEPS) stamp-tgtdir
	$(X3) $< -o $@ $(X3_GCC_COMMAND)

# because these are in the $(host_alias) subdir, pattern match rules will not work
gcc/config/unix.o: $(config_unix_d_src) $(config_d_src) $(config_libc_d_src)
	$(GDC) -o $@ $(ALL_DFLAGS) -I $(srcdir) -I ./$(host_alias) -c $<
gcc/config/unix.t.o: gcc/config/unix.o
	cp $< $@

gcc/config/mathfuncs.o: $(config_mathfuncs_d_src) $(config_d_src) $(config_libc_d_src)
	$(GDC) -o $@ $(ALL_DFLAGS) -I $(srcdir) -I ./$(host_alias) -c $<
gcc/config/mathfuncs.t.o: gcc/config/mathfuncs.o
	cp $< $@

# This has to be an empty file because it is included in the prerequisites of rules
# that use "cat $^" to generate their targets.
stamp-tgtdir:
	mkdir -p gcc/config
	mkdir -p $(host_alias)/gcc/config
	touch $@


#-------------------#
#-    D Runtime    -#
#-------------------#

# core.thread uses deprecated 'volatile'
common/core/thread.o: common/core/thread.d
	$(GDC) -o $@ $(ALL_DFLAGS) -fdeprecated -I $(srcdir) -I ./$(host_alias) -c $<

gcc/cbridge_math.o: gcc/cbridge_math.c
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -fno-strict-aliasing -c $<

BASE_OBJS=compiler/aaA.o compiler/aApply.o compiler/aApplyR.o compiler/adi.o \
	  compiler/arrayassign.o compiler/arraybyte.o compiler/arraycast.o \
	  compiler/arraycat.o compiler/arraydouble.o compiler/arrayfloat.o \
	  compiler/arrayint.o compiler/arrayreal.o compiler/arrayshort.o \
	  compiler/cast_.o compiler/complex.o \
	  compiler/critical.o compiler/dgccmain2.o \
	  compiler/invariant.o compiler/invariant_.o compiler/lifetime.o \
	  compiler/gccmemory.o \
	  compiler/monitor.o compiler/obj.o compiler/object_.o \
	  compiler/qsort.o compiler/switch_.o

GCC_OBJS=gcc/unwind_pe.o gcc/deh.o gcc/threadsem.o \
	 gcc/builtins.o gcc/config/mathfuncs.o gcc/support.o \
	 gcc/cbridge_time.o gcc/cbridge_strerror.o

UTIL_OBJS=compiler/util/console.o compiler/util/cpuid.o compiler/util/ctype.o \
	  compiler/util/string.o compiler/util/utf.o

TI=ti_AC.o ti_Acdouble.o ti_Acfloat.o ti_Acreal.o ti_Adouble.o ti_Afloat.o \
   ti_Ag.o ti_Aint.o ti_Along.o ti_Areal.o ti_Ashort.o ti_byte.o ti_C.o \
   ti_cdouble.o ti_cfloat.o ti_char.o ti_creal.o ti_dchar.o ti_delegate.o \
   ti_double.o ti_float.o ti_idouble.o ti_ifloat.o ti_int.o ti_ireal.o \
   ti_long.o ti_ptr.o ti_real.o ti_short.o ti_ubyte.o ti_uint.o ti_ulong.o \
   ti_ushort.o ti_void.o ti_wchar.o

DRUNTIME_OBJS=@DRUNTIME_OBJS@

DRUNTIME_OBJS+=$(subst compiler,compiler/gdc,$(BASE_OBJS)) \
	       $(subst compiler,compiler/gdc,$(UTIL_OBJS)) \
	       $(subst ti_,compiler/gdc/typeinfo/ti_,$(TI))

COMMON_OBJS=common/core/bitmanip.o common/core/exception.o common/core/memory.o \
	    common/core/runtime.o common/core/thread.o common/core/stdc/errno.o

# This should not be linked into a shared library.
#CMAIN_OBJS=compiler/gdc/cmain.o
CMAIN_OBJS=compiler/gdc/dmain2.o

D_GC_MODULES=@D_GC_MODULES@

# Underscore added for subst to correctly work
D_GC_MODULES+=gc_/gc.o gc_/gcalloc.o gc_/gcbits.o \
	      gc_/gcstats.o gc_/gcx.o

GC_OBJS=$(subst gc_,gc/basic,$(D_GC_MODULES))

ALL_DRUNTIME_OBJS = $(DRUNTIME_OBJS) $(COMMON_OBJS) $(GC_OBJS) $(GCC_OBJS)

libgdruntime.a : $(ALL_DRUNTIME_OBJS) $(CMAIN_OBJS)
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS) $(CMAIN_OBJS)
	$(RANLIB) $@


#-------------------#
#-    LibPhobos    -#
#-------------------#

# boxer is currently broken
std/boxer.t.o: std/boxer.o
	cp $< $@

unittest: unittest.o libgphobos2_t.a libgphobos2.a
	$(GDC) -o $@ $(CFLAGS) unittest.o -L./ -lgphobos2_t $(LIBS)

MAIN_OBJS=std/compiler.o std/system.o std/md5.o std/base64.o \
	std/string.o std/math.o std/array.o \
	std/outbuffer.o std/ctype.o std/regexp.o std/random.o \
	std/stream.o std/cstream.o \
	std/utf.o std/uri.o \
	crc32.o std/conv.o \
	std/syserror.o std/metastrings.o \
	std/c/stdarg.o std/c/stdio.o std/stdio.o std/format.o \
	std/openrj.o std/uni.o std/demangle.o std/bitarray.o \
	std/date.o std/dateparse.o etc/c/zlib.o std/zlib.o std/zip.o \
	std/stdarg.o \
	std/signals.o std/cpuid.o std/traits.o std/typetuple.o std/bind.o \
	std/algorithm.o std/bitmanip.o std/contracts.o std/functional.o \
	std/getopt.o std/numeric.o std/typecons.o std/variant.o \
	std/cover.o std/complex.o std/encoding.o std/iterator.o std/xml.o \
	std/atomics.o std/bigint.o

ZLIB_OBJS= zlib/adler32.o zlib/compress.o zlib/crc32.o \
	zlib/gzio.o zlib/uncompr.o zlib/deflate.o \
	zlib/trees.o zlib/zutil.o zlib/inflate.o \
	zlib/infback.o zlib/inftrees.o zlib/inffast.o

# modules which require some kind of operating system	
OS_OBJS = std/file.o std/mmfile.o std/path.o std/perf.o std/process.o std/socket.o std/socketstream.o

# std.c.linux.linux, std.loader, gcc.cbridge*
WINDOWS_OBJS=std/c/windows/windows.o std/c/windows/com.o std/c/windows/winsock.o \
	std/windows/iunknown.o std/windows/registry.o std/windows/syserror.o \
	std/windows/charset.o

D_EXTRA_OBJS=@D_EXTRA_OBJS@

# needed until instrinsics are implemented
D_EXTRA_OBJS+=std/intrinsic.o

std/stream.o: std/stream.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fdeprecated -I $(srcdir) -I ./$(host_alias) -c $<
std/stream.t.o: std/stream.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fdeprecated -I $(srcdir) -I ./$(host_alias) -c $<

ALL_PHOBOS_OBJS = $(D_EXTRA_OBJS) $(MAIN_OBJS) $(ZLIB_OBJS) $(WEAK_OBJS)

libgphobos2.a : libgdruntime.a $(ALL_PHOBOS_OBJS)
	$(AR) -r $@ $(ALL_PHOBOS_OBJS)
	$(RANLIB) $@

libgphobos2_t.a : $(ALL_PHOBOS_OBJS:.o=.t.o)
	$(AR) -r $@ $(ALL_PHOBOS_OBJS:.o=.t.o)
	$(RANLIB) $@

check-local: unittest testgc
	./unittest
	./testgc

install-exec-local: $(D_PREREQ_SRCS) libgphobos2.a libgdruntime.a
	$(mkinstalldirs) $(DESTDIR)$(toolexeclibdir)
	$(INSTALL) libgphobos2.a $(DESTDIR)$(toolexeclibdir)
	$(RANLIB) $(DESTDIR)$(toolexeclibdir)/libgphobos2.a
	$(INSTALL) libgdruntime.a $(DESTDIR)$(toolexeclibdir)
	$(RANLIB) $(DESTDIR)$(toolexeclibdir)/libgdruntime.a

install-data-local: $(D_PREREQ_SRCS) libgphobos2.a libgdruntime.a
	for i in etc etc/c \
		gcc std std/c \
                std/c/darwin std/c/linux std/c/mach std/c/skyos std/c/unix std/c/windows \
		std/windows; do \
		$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
		for f in $(srcdir)/$$i/*.d; do $(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; done; \
	done
	for i in core; do \
		$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
		for f in $(srcdir)/common/$$i/*.d; do $(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; done; \
	done
	for i in crc32.d object.di; do \
		$(INSTALL_HEADER) $(srcdir)/$$i $(DESTDIR)$(gdc_include_dir); done
	$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/gcc/config
	for i in $(host_alias)/gcc/config/*.d ; \
		do $(INSTALL_HEADER) $$i $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/gcc/config; done
	$(INSTALL) phobos-ver-syms $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)

clean-local:
	rm -f $(ALL_PHOBOS_OBJS) $(CMAIN_OBJS)
	rm -f $(ALL_PHOBOS_OBJS:.o=.t.o)
	rm -f $(ALL_DRUNTIME_OBJS)
	rm -f unittest.o
	rm -f unittest$(EXEEXT) testgc$(EXEEXT)
	rm -f $(config_d_src) $(config_errno_d_src) $(config_libc_d_src)
	rm -f $(config_unix_d_src)
	rm -f libgdruntime.a
	rm -f libgphobos2.a
	rm -f libgphobos2_t.a


# Work around what appears to be a GNU make bug handling MAKEFLAGS
# values defined in terms of make variables, as is the case for CC and
# friends when we are called from the top level Makefile.
AM_MAKEFLAGS = \
	"AR_FLAGS=$(AR_FLAGS)" \
	"CC_FOR_BUILD=$(CC_FOR_BUILD)" \
	"CC_FOR_TARGET=$(CC_FOR_TARGET)" \
	"CFLAGS=$(CFLAGS)" \
	"CXXFLAGS=$(CXXFLAGS)" \
	"CFLAGS_FOR_BUILD=$(CFLAGS_FOR_BUILD)" \
	"CFLAGS_FOR_TARGET=$(CFLAGS_FOR_TARGET)" \
	"INSTALL=$(INSTALL)" \
	"INSTALL_DATA=$(INSTALL_DATA)" \
	"INSTALL_PROGRAM=$(INSTALL_PROGRAM)" \
	"INSTALL_SCRIPT=$(INSTALL_SCRIPT)" \
	"LDFLAGS=$(LDFLAGS)" \
	"LIBCFLAGS=$(LIBCFLAGS)" \
	"LIBCFLAGS_FOR_TARGET=$(LIBCFLAGS_FOR_TARGET)" \
	"MAKE=$(MAKE)" \
	"MAKEINFO=$(MAKEINFO) $(MAKEINFOFLAGS)" \
	"PICFLAG=$(PICFLAG)" \
	"PICFLAG_FOR_TARGET=$(PICFLAG_FOR_TARGET)" \
	"SHELL=$(SHELL)" \
	"RUNTESTFLAGS=$(RUNTESTFLAGS)" \
	"exec_prefix=$(exec_prefix)" \
	"infodir=$(infodir)" \
	"libdir=$(libdir)" \
	"includedir=$(includedir)" \
	"prefix=$(prefix)" \
	"tooldir=$(tooldir)" \
	"gdc_include_dir=$(gdc_include_dir)" \
	"AR=$(AR)" \
	"AS=$(AS)" \
	"LD=$(LD)" \
	"RANLIB=$(RANLIB)" \
	"NM=$(NM)" \
	"NM_FOR_BUILD=$(NM_FOR_BUILD)" \
	"NM_FOR_TARGET=$(NM_FOR_TARGET)" \
	"DESTDIR=$(DESTDIR)" \
	"WERROR=$(WERROR)"

# Subdir rules rely on $(FLAGS_TO_PASS)
FLAGS_TO_PASS = $(AM_MAKEFLAGS)
