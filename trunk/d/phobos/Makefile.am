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

# AUTOMAKE_OPTIONS = 1.9.6 foreign no-dependencies

OUR_CFLAGS=@DEFS@ -I . -I $(srcdir)/gcc
D_EXTRA_DFLAGS=-nostdinc -pipe
ALL_DFLAGS = $(DFLAGS) $(D_GC_FLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

toolexecdir = $(phobos_toolexecdir)
toolexeclibdir = $(phobos_toolexeclibdir)

config_d_src=$(host_alias)/gcc/config.d
configunix_d_src=$(host_alias)/gcc/configunix.d

# This is a hook to get Automake to build libgphobos.a
BUILT_SOURCES = minimal.c
noinst_PROGRAMS = minimal
minimal_SOURCES = minimal.c
minimal_DEPENDENCIES = libgphobos.a
minimal.c:
	echo "int main() { return 0; }" > $@

all-local: libgphobos.a

SUFFIXES = .d

%.o : %.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS)  \
	-I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<

# %.o : %.c
# Use .c.o to override Automake
.c.o:
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

# boxer is currently broken
std/boxer.t.o: std/boxer.o
	cp $< $@

%.t.o : %.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fno-release -funittest -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<

internal/gc/gcx.t.o: $(D_PREREQ_SRCS) $(srcdir)/internal/gc/gcx.d
	$(GDC) -o $@ $(ALL_DFLAGS) -funittest -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<

%.t.o : %.o
	cp $< $@

unittest: unittest.o libgphobos_t.a libgphobos.a
	$(GDC) -o $@ $(CFLAGS) unittest.o -L./ -lgphobos_t $(LIBS)

internal/gc/testgc.o: $(host_alias)/gcc/config.d $(srcdir)/internal/gc/testgc.d
	$(GDC) -o $@ $(ALL_DFLAGS) -fno-release -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $(srcdir)/internal/gc/testgc.d

testgc: internal/gc/testgc.o libgphobos_t.a libgphobos.a
	$(GDC) -o $@ $(ALL_DFLAGS) internal/gc/testgc.o -L./ -lgphobos_t $(LIBS)

TI=ti_AC.o ti_Ag.o ti_Aint.o ti_Along.o ti_Ashort.o \
	ti_C.o \
	ti_byte.o ti_cdouble.o ti_cfloat.o ti_char.o ti_creal.o \
	ti_dchar.o ti_delegate.o ti_double.o ti_float.o ti_idouble.o ti_ifloat.o \
	ti_int.o  ti_ireal.o ti_long.o ti_ptr.o ti_real.o ti_short.o ti_ubyte.o \
	ti_uint.o ti_ulong.o ti_ushort.o ti_wchar.o \
	ti_Afloat.o ti_Adouble.o ti_Areal.o \
	ti_Acfloat.o ti_Acdouble.o ti_Acreal.o \
	ti_void.o

MAIN_OBJS=std/asserterror.o internal/switch.o gcstats.o \
	internal/critical.o internal/object.o internal/monitor.o internal/arraycat.o internal/invariant.o \
	std/outofmemory.o internal/aaA.o internal/adi.o internal/aApply.o internal/aApplyR.o std/file.o \
	std/compiler.o std/system.o std/moduleinit.o std/md5.o std/base64.o \
	internal/cast.o std/path.o std/string.o internal/memset.o std/math.o std/mmfile.o \
	std/outbuffer.o std/ctype.o std/regexp.o std/random.o \
	std/stream.o std/cstream.o std/switcherr.o std/array.o std/gc.o \
	internal/qsortg.o std/thread.o internal/obj.o std/utf.o std/uri.o \
	crc32.o std/conv.o internal/arraycast.o errno.o \
	std/process.o std/syserror.o std/metastrings.o \
	std/socket.o std/socketstream.o std/c/stdarg.o std/stdio.o std/format.o \
	std/perf.o std/openrj.o std/uni.o std/demangle.o std/bitarray.o \
	$(subst ti_,std/typeinfo/ti_,$(TI)) \
	std/date.o std/dateparse.o std/math2.o etc/c/zlib.o std/zlib.o std/zip.o \
	internal/dgccmain2.o internal/rundmain.o std/stdarg.o \
	std/signals.o std/cpuid.o std/traits.o std/typetuple.o std/bind.o \
	gcc/builtins.o

# This should not be linked into a shared library.
CMAIN_OBJS=internal/cmain.o

ZLIB_OBJS= etc/c/zlib/adler32.o etc/c/zlib/compress.o \
	etc/c/zlib/crc32.o etc/c/zlib/gzio.o \
	etc/c/zlib/uncompr.o etc/c/zlib/deflate.o \
	etc/c/zlib/trees.o etc/c/zlib/zutil.o \
	etc/c/zlib/inflate.o etc/c/zlib/infback.o \
	etc/c/zlib/inftrees.o etc/c/zlib/inffast.o

GC_OBJS= internal/gc/gc.o internal/gc/gcx.o \
	internal/gc/gcbits.o
GC_OBJS += @D_GC_MODULES@

GCC_OBJS = gcc/config.o gcc/unwind.o gcc/deh.o gcc/threadsem.o \
	std/c/dirent.o gcc/cbridge_time.o

# std.c.linux.linux, std.loader, gcc.cbridge*
WINDOWS_OBJS=std/c/windows/windows.o std/c/windows/com.o std/c/windows/winsock.o \
	std/windows/iunknown.o std/windows/registry.o std/windows/syserror.o \
	std/windows/charset.o
# D_EXTRA_OBJS=@D_EXTRA_OBJS@

# needed until instrinsics are implemented
D_EXTRA_OBJS+=std/intrinsic.o

# currently just add compatibility for a bug
D_EXTRA_OBJS+=gcc/support.o

CONFIG_D_FRAGMENTS = config/config-head frag-ac frag-gen frag-math config/config-mid config/config-tail
CONFIG_UNIX_FRAGMENTS = config/unix-head frag-unix config/unix-mid

gen_config1: config/gen_config1.o
	$(CC) $(CFLAGS) -o $@ $^

$(config_d_src): $(CONFIG_D_FRAGMENTS) stamp-tgtdir
	cat $^ > $@

gcc/config.o: $(config_d_src)
	$(GDC) -o $@ $(ALL_DFLAGS) -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<
gcc/config.t.o: gcc/config.o
	cp gcc/config.o gcc/config.t.o


gen_math: config/gen_math.o
	$(CC) $(CFLAGS) -o $@ $^

config/gen_unix.o: config/gen_unix.c config/makestruct.h

gen_unix: config/gen_unix.o
	$(CC) $(CFLAGS) -o $@ $^


# Plain 'make' would use: ifdef D_GENERATE_FRAGMENTS
if D_GENERATE_FRAGMENTS
frag-gen: gen_config1
	./gen_config1 > $@ || rm -f $@
frag-unix: gen_unix
	./gen_unix > $@
frag-math: gen_math
	./gen_math > $@ || rm -f $@
else
frag-gen: $(D_FRAGMENT_SRCDIR)/$@
	cp $(D_FRAGMENT_SRCDIR)/$@ $@
frag-unix: $(D_FRAGMENT_SRCDIR)/$@
	cp $(D_FRAGMENT_SRCDIR)/$@ $@
frag-math: $(D_FRAGMENT_SRCDIR)/$@
	cp $(D_FRAGMENT_SRCDIR)/$@ $@
endif

$(configunix_d_src): $(CONFIG_UNIX_FRAGMENTS) stamp-tgtdir
	cat $^ > $@

gcc/configunix.o: $(configunix_d_src) $(config_d_src)
	$(GDC) -o $@ $(ALL_DFLAGS) -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<
gcc/configunix.t.o: gcc/configunix.o
	cp gcc/configunix.o gcc/configunix.t.o

gcc/cbridge_math.o: gcc/cbridge_math.c
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -fno-strict-aliasing -c $<

std/stream.o: std/stream.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fdeprecated -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<
std/stream.t.o: std/stream.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fdeprecated -I $(srcdir) -I $(srcdir)/internal/gc -I ./$(host_alias) -c $<

# GCC_OBJS (gcc/config.o) first so I don't have to write more deps
ALL_PHOBOS_OBJS = $(D_EXTRA_OBJS) $(GCC_OBJS) $(MAIN_OBJS) $(ZLIB_OBJS) $(GC_OBJS) $(WEAK_OBJS)

libgphobos.a : $(ALL_PHOBOS_OBJS) $(CMAIN_OBJS)
	$(AR) -r $@ $(ALL_PHOBOS_OBJS) $(CMAIN_OBJS)
	$(RANLIB) $@

libgphobos_t.a : $(ALL_PHOBOS_OBJS:.o=.t.o) $(CMAIN_OBJS)
	$(AR) -r $@ $(ALL_PHOBOS_OBJS:.o=.t.o) $(CMAIN_OBJS)
	$(RANLIB) $@

# This has to be an empty file because it is included in the prerequisites of rules
# that use "cat $^" to generate their targets.
# Otherwise, need to specify $srcdir for known source files in CONFIG_xxx_FRAGMENTS..
stamp-tgtdir:
	mkdir -p $(host_alias)/gcc
	touch $@

check-local: unittest testgc
	./unittest
	./testgc

install-exec-local: $(D_PREREQ_SRCS) libgphobos.a
	$(mkinstalldirs) $(DESTDIR)$(toolexeclibdir)
	$(INSTALL) libgphobos.a $(DESTDIR)$(toolexeclibdir)
	$(RANLIB) $(DESTDIR)$(toolexeclibdir)/libgphobos.a

install-data-local: $(D_PREREQ_SRCS) libgphobos.a
	for i in etc etc/c \
		etc/c/zlib \
		gcc std std/c \
                std/c/darwin std/c/linux std/c/mach std/c/skyos std/c/unix std/c/windows \
		std/typeinfo std/windows; do \
		$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
		for f in $(srcdir)/$$i/*.[hd]; do $(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; done; \
	done
	for i in crc32.d gcstats.d object.d; do \
		$(INSTALL_HEADER) $(srcdir)/$$i $(DESTDIR)$(gdc_include_dir); done
	$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/gcc
	$(INSTALL_HEADER) $(config_d_src) $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/gcc
	if test -f $(configunix_d_src); then $(INSTALL_HEADER) $(host_alias)/gcc/configunix.d $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/gcc; fi
	$(INSTALL) phobos-ver-syms $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)

clean-local:
	rm -f $(ALL_PHOBOS_OBJS) $(CMAIN_OBJS)
	rm -f $(ALL_PHOBOS_OBJS:.o=.t.o)
	rm -f unittest.o internal/gc/testgc.o
	rm -f unittest$(EXEEXT) testgc$(EXEEXT)
	rm -f config/gen_config1.o config/gen_unix.o config/gen_math.o
	rm -f gen_config1$(EXEEXT) gen_unix$(EXEEXT) gen_math$(EXEEXT)
	rm -f frag-gen frag-math frag-unix
	rm -f $(config_d_src) $(configunix_d_src)
	rm -f libgphobos.a
	rm -f libgphobos_t.a


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
