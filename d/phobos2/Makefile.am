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

OUR_CFLAGS=@DEFS@ -I . -I $(srcdir)/gcc -I $(srcdir)/zlib
D_EXTRA_DFLAGS=-nostdinc -pipe -fdeprecated -I $(srcdir) -I ./$(host_alias)
ALL_DFLAGS = $(DFLAGS) $(D_GC_FLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

toolexecdir = $(phobos_toolexecdir)
toolexeclibdir = $(phobos_toolexeclibdir)

all-local: libgphobos2.a libgdruntime.a

SUFFIXES = .d

%.o : %.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -c $<

IMPORT = import
$(IMPORT): $(D_PREREQ_SRCS)
	mkdir -p $(IMPORT)
# For core objects, generate .di headers
$(IMPORT)/%.di: %.d $(D_PREREQ_SRCS) $(IMPORT)
	$(GDC) $(ALL_DFLAGS) -fintfc-file=$@ -fsyntax-only $<

# %.o : %.c
# Use .c.o to override Automake
.c.o:
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

.S.o:
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

%.t.o : %.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fno-release -funittest -fversion=Unittest -c $<

%.t.o : %.o
	cp $< $@


#--------------------------------------#
# X3 Config

config_d_src          =$(host_alias)/gcc/config/config.d
config_libc_d_src     =$(host_alias)/gcc/config/libc.d
config_errno_d_src    =$(host_alias)/gcc/config/errno.d
config_fpcls_d_src    =$(host_alias)/gcc/config/fpcls.d
config_mathfuncs_d_src=$(host_alias)/gcc/config/mathfuncs.d
config_unix_d_src     =$(host_alias)/gcc/config/unix.d

X3=CC_FOR_BUILD="$(CC_FOR_BUILD) -I $(srcdir)/config" BUILD_LIBIBERTY=$(BUILD_LIBIBERTY) $(srcdir)/config/x3
X3_GCC_COMMAND=$(CC) $(OUR_CFLAGS) $(MULTIFLAGS)
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
	$(GDC) -o $@ $(ALL_DFLAGS) -c $<
gcc/config/unix.t.o: gcc/config/unix.o
	cp $< $@

gcc/config/mathfuncs.o: $(config_mathfuncs_d_src) $(config_d_src) $(config_libc_d_src)
	$(GDC) -o $@ $(ALL_DFLAGS) -c $<
gcc/config/mathfuncs.t.o: gcc/config/mathfuncs.o
	cp $< $@

# This has to be an empty file because it is included in the prerequisites of rules
# that use "cat $^" to generate their targets.
stamp-tgtdir:
	mkdir -p gcc/config
	mkdir -p $(host_alias)/gcc/config
	touch $@


#--------------------------------------#
# GDC Runtime

gcc/cbridge_math.o: gcc/cbridge_math.c
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -fno-strict-aliasing -c $<

BASE_OBJS=object_.o

RUNTIME_OBJS=rt/aaA.o rt/aApply.o rt/aApplyR.o rt/adi.o rt/arrayassign.o \
	     rt/arraybyte.o rt/arraycast.o rt/arraycat.o rt/arraydouble.o \
	     rt/arrayfloat.o rt/arrayint.o rt/arrayreal.o rt/arrayshort.o \
	     rt/cast_.o rt/critical_.o rt/dmain2.o \
	     rt/memory.o rt/invariant.o rt/invariant_.o rt/lifetime.o \
	     rt/monitor_.o rt/obj.o rt/qsort.o rt/switch_.o

CORE_OBJS=core/atomic.o core/cpuid.o core/demangle.o core/exception.o \
	  core/math.o core/memory.o core/runtime.o core/thread.o \
	  core/time.o core/vararg.o core/sync/barrier.o \
	  core/sync/condition.o core/sync/config.o core/sync/exception.o \
	  core/sync/mutex.o core/sync/rwmutex.o core/sync/semaphore.o

GCC_OBJS=gcc/unwind_pe.o gcc/deh.o gcc/threadsem.o \
	 gcc/builtins.o gcc/config/mathfuncs.o gcc/support.o \
	 gcc/cbridge_time.o gcc/cbridge_strerror.o gcc/atomics.o

UTIL_OBJS=rt/util/console.o rt/util/ctype.o rt/util/hash.o rt/util/string.o \
	  rt/util/utf.o

TI=ti_AC.o ti_Acdouble.o ti_Acfloat.o ti_Acreal.o ti_Adouble.o ti_Afloat.o \
   ti_Ag.o ti_Aint.o ti_Along.o ti_Areal.o ti_Ashort.o ti_byte.o ti_C.o \
   ti_cdouble.o ti_cfloat.o ti_char.o ti_creal.o ti_dchar.o ti_delegate.o \
   ti_double.o ti_float.o ti_idouble.o ti_ifloat.o ti_int.o ti_ireal.o \
   ti_long.o ti_ptr.o ti_real.o ti_short.o ti_ubyte.o ti_uint.o ti_ulong.o \
   ti_ushort.o ti_void.o ti_wchar.o

DRUNTIME_OBJS=@DRUNTIME_OBJS@

DRUNTIME_OBJS+=$(BASE_OBJS) $(RUNTIME_OBJS) $(UTIL_OBJS) \
	       $(subst ti_,rt/typeinfo/ti_,$(TI))


# OS specific
RT_STDC_OBJS=core/stdc/config.o core/stdc/ctype.o core/stdc/errno.o \
	     core/stdc/errno_.o core/stdc/math.o core/stdc/signal.o \
	     core/stdc/stdarg.o core/stdc/stdio.o core/stdc/stdlib.o \
	     core/stdc/stdint.o core/stdc/stddef.o core/stdc/string.o \
	     core/stdc/time.o core/stdc/wchar_.o

RT_OSX_OBJS=core/sys/osx/mach/kern_return.o core/sys/osx/mach/port.o \
	    core/sys/osx/mach/semaphore.o core/sys/osx/mach/thread_act.o \
	    core/sys/osx/pthread.o

RT_POSIX_OBJS=core/sys/posix/netinet/in_.o core/sys/posix/sys/select.o \
	      core/sys/posix/sys/socket.o core/sys/posix/sys/stat.o \
	      core/sys/posix/sys/wait.o core/sys/posix/sys/un.o \
	      core/sys/posix/netdb.o

RT_WINDOWS_OBJS=core/sys/windows/dbghelp.o core/sys/windows/dll.o \
		core/sys/windows/stacktrace.o core/sys/windows/threadaux.o \
		core/sys/windows/windows.o

# This should not be linked into a shared library.
CMAIN_OBJS= #rt/cmain.o

D_GC_MODULES=@D_GC_MODULES@

# Regardless of OS, all import headers are generated.
CORE_IMPORTS=core/atomic.di core/bitop.di core/cpuid.di core/demangle.di \
	     core/exception.di core/math.di core/memory.di core/runtime.di \
	     core/thread.di core/time.di core/vararg.di \
	     \
	     core/stdc/complex.di core/stdc/config.di core/stdc/ctype.di \
	     core/stdc/errno.di core/stdc/fenv.di core/stdc/float_.di \
	     core/stdc/inttypes.di core/stdc/limits.di core/stdc/locale.di \
	     core/stdc/math.di core/stdc/signal.di core/stdc/stdarg.di \
	     core/stdc/stddef.di core/stdc/stdint.di core/stdc/stdio.di \
	     core/stdc/stdlib.di core/stdc/string.di core/stdc/tgmath.di \
	     core/stdc/time.di core/stdc/wchar_.di core/stdc/wctype.di \
	     \
	     core/sync/barrier.di core/sync/condition.di core/sync/config.di \
	     core/sync/exception.di core/sync/mutex.di core/sync/rwmutex.di \
	     core/sync/semaphore.di \
	     \
	     core/sys/osx/mach/kern_return.di core/sys/osx/mach/port.di \
	     core/sys/osx/mach/semaphore.di core/sys/osx/mach/thread_act.di \
	     core/sys/osx/pthread.di \
	     \
	     core/sys/posix/arpa/inet.di core/sys/posix/config.di \
	     core/sys/posix/dirent.di core/sys/posix/dlfcn.di \
	     core/sys/posix/fcntl.di core/sys/posix/inttypes.di \
	     core/sys/posix/netdb.di core/sys/posix/poll.di \
	     core/sys/posix/pthread.di core/sys/posix/pwd.di \
	     core/sys/posix/sched.di core/sys/posix/semaphore.di \
	     core/sys/posix/setjmp.di core/sys/posix/signal.di \
	     core/sys/posix/stdio.di core/sys/posix/stdlib.di \
	     core/sys/posix/termios.di core/sys/posix/time.di \
	     core/sys/posix/ucontext.di core/sys/posix/unistd.di \
	     core/sys/posix/utime.di core/sys/posix/net/if_.di \
	     core/sys/posix/netinet/in_.di core/sys/posix/netinet/tcp.di \
	     core/sys/posix/sys/ipc.di core/sys/posix/sys/mman.di \
	     core/sys/posix/sys/select.di core/sys/posix/sys/shm.di \
	     core/sys/posix/sys/socket.di core/sys/posix/sys/stat.di \
	     core/sys/posix/sys/time.di core/sys/posix/sys/types.di \
	     core/sys/posix/sys/uio.di core/sys/posix/sys/un.di \
	     core/sys/posix/sys/wait.di \
	     \
	     core/sys/windows/dbghelp.di core/sys/windows/dll.di \
	     core/sys/windows/stacktrace.di core/sys/windows/threadaux.di \
	     core/sys/windows/windows.di

ALL_DRUNTIME_OBJS = $(DRUNTIME_OBJS) $(CORE_OBJS) $(D_GC_MODULES) $(GCC_OBJS)

libgdruntime.a : $(ALL_DRUNTIME_OBJS) $(CMAIN_OBJS) $(subst core/,$(IMPORT)/core/,$(CORE_IMPORTS))
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS) $(CMAIN_OBJS)
	$(RANLIB) $@


#--------------------------------------#
# GDC LibPhobos

# boxer is currently broken
std/boxer.t.o: std/boxer.o
	cp $< $@

# Special-case for std.xml, need to emit templates.
std/xml.o : std/xml.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -femit-templates -c $<

unittest: unittest.o libgphobos2_t.a libgphobos2.a
	$(GDC) -o $@ $(CFLAGS) -fdeprecated unittest.o -L./ -lgdruntime -lgphobos2_t $(LIBS) -ldl

MAIN_OBJS=std/algorithm.o std/array.o std/ascii.o std/base64.o std/bigint.o \
	  std/bind.o std/bitarray.o std/bitmanip.o std/compiler.o \
	  std/container.o std/complex.o std/concurrency.o std/contracts.o \
	  std/conv.o std/cpuid.o std/cstream.o std/ctype.o std/date.o \
	  std/datebase.o std/dateparse.o std/datetime.o \
	  std/demangle.o std/encoding.o std/exception.o std/format.o \
	  std/functional.o std/getopt.o std/gregorian.o std/json.o std/math.o \
	  std/mathspecial.o std/md5.o std/metastrings.o std/numeric.o \
	  std/outbuffer.o std/parallelism.o std/signals.o \
	  std/stdarg.o std/stdio.o std/stdiobase.o std/stdint.o std/stream.o \
	  std/string.o std/syserror.o std/system.o std/random.o std/range.o \
	  std/regex.o std/regexp.o std/traits.o std/typecons.o \
	  std/typetuple.o std/uni.o std/uri.o std/utf.o std/variant.o \
	  std/xml.o std/zip.o std/zlib.o \
	  std/c/fenv.o std/c/locale.o std/c/math.o std/c/process.o \
	  std/c/stdarg.o std/c/stddef.o std/c/stdio.o std/c/stdlib.o \
	  std/c/string.o std/c/time.o std/c/wcharh.o \
	  std/internal/math/biguintcore.o std/internal/math/biguintnoasm.o \
	  std/internal/math/biguintx86.o std/internal/math/gammafunction.o \
	  std/internal/math/errorfunction.o \
	  std/net/isemail.o \
	  crc32.o etc/c/curl.o etc/c/zlib.o

ZLIB_OBJS=zlib/adler32.o zlib/compress.o zlib/crc32.o zlib/deflate.o \
	  zlib/gzclose.o zlib/gzlib.o zlib/gzread.o zlib/gzwrite.o \
	  zlib/infback.o zlib/inffast.o zlib/inflate.o zlib/inftrees.o \
	  zlib/trees.o zlib/uncompr.o zlib/zutil.o

# modules which require some kind of operating system
OS_OBJS=std/file.o std/mmfile.o std/path.o std/perf.o std/process.o \
	std/socket.o std/socketstream.o

LINUX_OBJS=std/c/linux/socket.o

OSX_OBJS=std/c/osx/socket.o

FREEBSD_OBJS=std/c/freebsd/socket.o

# std.c.linux.linux, std.loader, gcc.cbridge*
WINDOWS_OBJS=std/c/windows/com.o std/c/windows/stat.o std/c/wcharh.o \
	     std/c/windows/windows.o std/c/windows/winsock.o \
	     std/windows/charset.o std/windows/iunknown.o std/windows/registry.o \
	     std/windows/syserror.o std/__fileinit.o 

D_EXTRA_OBJS=@D_EXTRA_OBJS@

std/stream.t.o: std/stream.d $(D_PREREQ_SRCS)
	$(GDC) -o $@ $(ALL_DFLAGS) -fdeprecated -c $<

ALL_PHOBOS_OBJS = $(D_EXTRA_OBJS) $(MAIN_OBJS) $(ZLIB_OBJS)


libgphobos2.a : libgdruntime.a $(ALL_PHOBOS_OBJS)
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS) $(CMAIN_OBJS) $(ALL_PHOBOS_OBJS)
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
	for i in etc/c \
		gcc std std/c \
		std/c/linux std/c/osx std/c/unix std/c/windows \
		std/internal/math std/windows; do \
		$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
		for f in $(srcdir)/$$i/*.d; do $(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; done; \
	done
	for i in core core/stdc core/sync core/sys/osx/mach core/sys/posix \
		core/sys/posix/arpa core/sys/posix/net core/sys/posix/netinet \
		core/sys/posix/sys core/sys/windows; do \
		$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/$$i; \
		for f in $(IMPORT)/$$i/*.di; do \
			$(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)/$$i; \
		done; \
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
	rm -f $(CORE_IMPORTS)
	rm -rf $(IMPORT)
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
