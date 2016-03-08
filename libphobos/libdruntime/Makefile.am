# GDC -- D front-end for GCC
# Copyright (C) 2012 Iain Buclaw
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

OUR_CFLAGS=@DEFS@ -I ../ -I $(srcdir)/gcc -I $(srcdir)/../zlib
D_EXTRA_DFLAGS=-nostdinc -pipe -Wno-deprecated -I $(srcdir) -I ./$(host_alias) -I .
ALL_DFLAGS = $(DFLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

toolexecdir = $(phobos_toolexecdir)
toolexeclibdir = $(phobos_toolexeclibdir)

all-local: libgdruntime.a

SUFFIXES = .d

%.o : %.d
	$(GDC) -o $@ $(ALL_DFLAGS) -c $<

# %.o : %.c
# Use .c.o to override Automake
.c.o:
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

.S.o:
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

%.t.o : %.d
	$(GDC) -o $@ $(DFLAGSX) $(D_EXTRA_DFLAGS) $(MULTIFLAGS) -c $<

%.t.o : %.o
	cp $< $@


#--------------------------------------#
# GDC Runtime

BASE_OBJS=object_.o

RUNTIME_OBJS=rt/aaA.o rt/aApply.o rt/aApplyR.o rt/adi.o rt/arrayassign.o \
	     rt/arraycast.o rt/arraycat.o rt/cast_.o rt/critical_.o \
	     rt/deh.o rt/dmain2.o rt/minfo.o rt/memory.o rt/invariant.o \
	     rt/lifetime.o rt/monitor_.o rt/obj.o rt/qsort.o rt/switch_.o \
	     rt/tlsgc.o

CORE_OBJS=core/atomic.o core/bitop.o core/checkedint.o core/cpuid.o \
	  core/demangle.o core/exception.o core/math.o core/memory.o \
	  core/runtime.o core/simd.o core/thread.o core/threadasm.o \
	  core/time.o core/vararg.o core/internal/traits.o \
	  core/sync/barrier.o core/sync/condition.o core/sync/config.o \
	  core/sync/exception.o core/sync/mutex.o core/sync/rwmutex.o \
	  core/sync/semaphore.o

GCC_OBJS=gcc/atomics.o gcc/backtrace.o gcc/builtins.o gcc/deh.o \
	 gcc/libbacktrace.o gcc/unwind/pe.o

UTIL_OBJS=rt/util/array.o rt/util/hash.o rt/util/random.o rt/util/string.o \
	  rt/util/utf.o rt/util/container/array.o rt/util/container/common.o \
	  rt/util/container/hashtab.o rt/util/container/treap.o

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

RT_LINUX_OBJS=

RT_FREEBSD_OBJS=core/sys/freebsd/execinfo.o core/sys/freebsd/sys/event.o

RT_OSX_OBJS=core/sys/osx/mach/kern_return.o core/sys/osx/mach/port.o \
	    core/sys/osx/mach/semaphore.o core/sys/osx/mach/thread_act.o \
	    core/sys/osx/pthread.o

RT_POSIX_OBJS=core/sys/posix/dirent.o core/sys/posix/netdb.o \
	      core/sys/posix/signal.o core/sys/posix/sys/ioctl.o \
	      core/sys/posix/sys/resource.o core/sys/posix/sys/select.o \
	      core/sys/posix/sys/socket.o core/sys/posix/sys/stat.o \
	      core/sys/posix/sys/utsname.o core/sys/posix/sys/wait.o \
	      core/sys/posix/arpa/inet.o core/sys/posix/netinet/in_.o

RT_WINDOWS_OBJS=core/sys/windows/dbghelp.o core/sys/windows/dll.o \
		core/sys/windows/stacktrace.o core/sys/windows/threadaux.o \
		core/sys/windows/windows.o

D_GC_MODULES=@D_GC_MODULES@

ALL_DRUNTIME_OBJS = $(DRUNTIME_OBJS) $(CORE_OBJS) $(D_GC_MODULES) $(GCC_OBJS)

libgdruntime.a : $(ALL_DRUNTIME_OBJS)
if BACKTRACE_SUPPORTED
	cp -f $(LIBBACKTRACE_LIB) $@
	$(AR) -q $@ $(ALL_DRUNTIME_OBJS)
else
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS)
endif
	$(RANLIB) $@

libgdruntime_t.a : $(ALL_DRUNTIME_OBJS:.o=.t.o)
if BACKTRACE_SUPPORTED
	cp -f $(LIBBACKTRACE_LIB) $@
	$(AR) -q $@ $(ALL_DRUNTIME_OBJS:.o=.t.o)
else
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS:.o=.t.o)
endif
	$(RANLIB) $@

unittest: libgdruntime.a libgdruntime_t.a unittest.o
	$(GDC) -o $@ $(CFLAGS) unittest.o -nophoboslib -L./ -lgdruntime_t $(LIBS) $(DL_LIBS)

#--------------------------------------#
# Install, doc, etc targets

install-exec-local: libgdruntime.a
	$(mkinstalldirs) $(DESTDIR)$(toolexeclibdir)
	$(INSTALL) libgdruntime.a $(DESTDIR)$(toolexeclibdir)
	$(RANLIB) $(DESTDIR)$(toolexeclibdir)/libgdruntime.a

install-data-local: libgdruntime.a
	$(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)
	for i in __entrypoint.di object.di; do \
	    $(INSTALL_HEADER) $(srcdir)/$$i $(DESTDIR)$(gdc_include_dir); \
	done
	for i in core core/internal core/stdc core/sync \
	    core/sys/freebsd core/sys/freebsd/sys \
	    core/sys/linux core/sys/linux/sys \
	    core/sys/osx core/sys/osx/mach \
	    core/sys/posix core/sys/posix/arpa core/sys/posix/net \
	    core/sys/posix/netinet core/sys/posix/sys \
	    core/sys/windows; do \
	    $(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
	    for f in $(srcdir)/$$i/*.d; do \
		$(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; \
	    done; \
	done
	for i in gcc gcc/unwind gcc/gthreads; do \
	    $(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
	    for f in $(srcdir)/$$i/*.d; do \
		$(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; \
	    done; \
	done
# Install files build generated by configure script
	for i in gcc; do \
	    $(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
	    for f in $$i/*.d; do \
		$(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; \
	    done; \
	done

clean-local:
	rm -f $(ALL_DRUNTIME_OBJS)
	rm -f $(ALL_DRUNTIME_OBJS:.o=.t.o)
	rm -f unittest.o
	rm -f unittest$(EXEEXT)
	rm -f libgdruntime.a
	rm -f libgdruntime_t.a

check-local: unittest
	./unittest

# Work around what appears to be a GNU make bug handling MAKEFLAGS
# values defined in terms of make variables, as is the case for CC and
# friends when we are called from the top level Makefile.
AM_MAKEFLAGS = \
	"gdc_include_dir=$(gdc_include_dir)"

# Subdir rules rely on $(FLAGS_TO_PASS)
FLAGS_TO_PASS = $(AM_MAKEFLAGS)
