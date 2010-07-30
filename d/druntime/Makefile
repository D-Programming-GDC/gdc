# GDC -- D front-end for GCC
# Copyright (C) 2010 Iain Buclaw
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

CC=gcc
DC=gdc
CFLAGS=-g -O2
DFLAGS=-g -O2 -frelease
D_EXTRA_DFLAGS=-nostdinc -pipe
ALL_DFLAGS = $(DFLAGS) $(D_GC_FLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

AR=ar
RANLIB=ranlib

all-local: libgdrt.a

SUFFIXES = .d

%.o : %.d $(D_PREREQ_SRCS)
	$(DC) -o $@ $(ALL_DFLAGS) \
	-I. -I./import -I./compiler/gdc -I./gc/basic -I./../phobos2 -c $<

# %.o : %.c
# Use .c.o to override Automake
.c.o:
	$(CC) -o $@ $(CFLAGS) -I. -c $<

%.t.o : %.d $(D_PREREQ_SRCS)
	$(DC) -o $@ $(ALL_DFLAGS) -fno-release -funittest -fversion=Unittest \
	    -I. -I./import -I./compiler/gdc -I./gc/basic -I./../phobos2 -c $<

%.t.o : %.o
	cp $< $@

# core.thread uses deprecated 'volatile'
common/core/thread.o: common/core/thread.d
	$(DC) -o $@ $(ALL_DFLAGS) -fdeprecated \
	-I. -I./import -I./compiler/gdc -I./gc/basic -I./../phobos2 -c $<

common/core/thread.t.o: common/core/thread.d
	$(DC) -o $@ $(ALL_DFLAGS) -fdeprecated -fno-release -funittest -fversion=Unittest \
	-I. -I./import -I./compiler/gdc -I./gc/basic -I./../phobos2 -c $<


BASE_OBJS=compiler/aaA.o compiler/aApply.o compiler/aApplyR.o compiler/adi.o \
	  compiler/arrayassign.o compiler/arraybyte.o compiler/arraycast.o \
	  compiler/arraycat.o compiler/arraydouble.o compiler/arrayfloat.o \
	  compiler/arrayint.o compiler/arrayreal.o compiler/arrayshort.o \
	  compiler/cast_.o compiler/cmath2.o compiler/complex.o \
	  compiler/cover.o compiler/critical.o compiler/dgccmain2.o \
	  compiler/invariant.o compiler/invariant_.o compiler/lifetime.o \
	  compiler/llmath.o compiler/memory.o compiler/memset.o \
	  compiler/monitor.o compiler/obj.o compiler/object_.o \
	  compiler/qsort.o compiler/switch_.o compiler/trace.o

GCC_OBJS=gcc/config/config.o gcc/config/libc.o gcc/config/errno.o \
	 gcc/config/mathfuncs.o gcc/config/unix.o gcc/fpcls.o \
	 gcc/unwind_pe.o gcc/deh.o gcc/threadsem.o gcc/cbridge_time.o \
	 gcc/cbridge_strerror.o gcc/unwind_generic.o gcc/support.o \
	 gcc/builtins.o

UTIL_OBJS=compiler/util/console.o compiler/util/cpuid.o compiler/util/ctype.o \
	  compiler/util/string.o compiler/util/utf.o

TI=ti_AC.o ti_Acdouble.o ti_Acfloat.o ti_Acreal.o ti_Adouble.o ti_Afloat.o \
   ti_Ag.o ti_Aint.o ti_Along.o ti_Areal.o ti_Ashort.o ti_byte.o ti_C.o \
   ti_cdouble.o ti_cfloat.o ti_char.o ti_creal.o ti_dchar.o ti_delegate.o \
   ti_double.o ti_float.o ti_idouble.o ti_ifloat.o ti_int.o ti_ireal.o \
   ti_long.o ti_ptr.o ti_real.o ti_short.o ti_ubyte.o ti_uint.o ti_ulong.o \
   ti_ushort.o ti_void.o ti_wchar.o

MAIN_OBJS=$(subst compiler,compiler/gdc,$(BASE_OBJS)) \
	  $(subst gcc,compiler/gdc/gcc,$(GCC_OBJS)) \
	  $(subst compiler,compiler/gdc,$(UTIL_OBJS)) \
	  $(subst ti_,compiler/gdc/typeinfo/ti_,$(TI))

# This should not be linked into a shared library.
CMAIN_OBJS=compiler/gdc/cmain.o

COMMON_OBJS=common/core/bitmanip.o common/core/exception.o \
	    common/core/memory.o common/core/runtime.o common/core/thread.o

COMMON_COBJS=common/stdc/errno.o

GC_OBJS=gc/basic/gc.o gc/basic/gcalloc.o gc/basic/gcbits.o \
	gc/basic/gcstats.o gc/basic/gcx.o

ALL_DRUNTIME_OBJS = $(MAIN_OBJS) $(COMMON_OBJS) $(GC_OBJS)

libgdrt.a : $(ALL_DRUNTIME_OBJS) $(COMMON_COBJS) $(CMAIN_OBJS)
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS) $(COMMON_COBJS) $(CMAIN_OBJS)
	$(RANLIB) $@

libgdrt_t.a : $(ALL_DRUNTIME_OBJS:.o=.t.o) $(COMMON_COBJS) $(CMAIN_OBJS)
	$(AR) -r $@ $(ALL_DRUNTIME_OBJS:.o=.t.o) $(COMMON_COBJS) $(CMAIN_OBJS)
	$(RANLIB) $@

clean-local:
	rm -f $(ALL_DRUNTIME_OBJS) $(COMMON_COBJS) $(CMAIN_OBJS)
	rm -f $(ALL_DRUNTIME_OBJS:.o=.t.o)
	rm -f hello
	rm -f libgdrt.a
	rm -f libgdrt_t.a

clean: clean-local

# Test library

hello: hello.d
	$(DC) -o $@ -lpthread -L. -defaultlib gdrt $<

test: hello
	./hello

