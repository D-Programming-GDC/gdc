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

ACLOCAL_AMFLAGS = -I. -I..

OUR_CFLAGS=@DEFS@ -I. -I$(srcdir)/../libdruntime/gcc -I$(top_srcdir)/../zlib
D_EXTRA_DFLAGS=-nostdinc -pipe -Wno-deprecated -I$(srcdir) -I$(srcdir)/../libdruntime -I../libdruntime/$(host_alias) -I../libdruntime
ALL_DFLAGS=$(DFLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

toolexecdir = $(phobos_toolexecdir)
toolexeclibdir = $(phobos_toolexeclibdir)

PHOBOS_SO = libgphobos2.$(SO_EXT)
DRUNTIME_SO = libgdruntime.$(SO_EXT)
PHOBOS_T_SO = libgphobos2_t.$(SO_EXT)
PHOBOS_LIBS = libgphobos2.a
if ENABLE_SHARED
  PHOBOS_LIBS += $(PHOBOS_SO)
endif
PHOBOS_TEST = unittest
if ENABLE_SHARED
  PHOBOS_TEST += unittest-pic
endif

all-local: $(PHOBOS_LIBS)

SUFFIXES = .d

%.o : %.d
	$(GDC) -o $@ $(ALL_DFLAGS) -c $<

%.pic.o : %.d
	$(GDC) -o $@ $(ALL_DFLAGS) -fPIC -c $<

# %.o : %.c
zlib/%.o: $(top_srcdir)/../zlib/%.c
	@test -d zlib || mkdir -p zlib
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

# Use .c.o to override Automake
%.c.o: %.c
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

%.c.pic.o: %.c
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -fPIC -c $<

%.S.o: %.S
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

%.S.pic.o: %.S
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -fPIC -c $<

%.t.o : %.d
	$(GDC) -o $@ $(DFLAGSX) -fno-invariants $(D_EXTRA_DFLAGS) $(MULTIFLAGS) -c $<

%.t.o : %.o
	cp $< $@

%.pic.t.o : %.d
	$(GDC) -o $@ $(DFLAGSX) -fno-invariants $(D_EXTRA_DFLAGS) $(MULTIFLAGS) -fPIC -c $<

%.pic.t.o : %.pic.o
	cp $< $@


#--------------------------------------#
# GDC LibPhobos

MAIN_OBJS=std/algorithm.o std/array.o std/ascii.o std/base64.o std/bigint.o \
	  std/bitmanip.o std/compiler.o \
	  std/container.o std/complex.o std/concurrency.o \
	  std/conv.o std/cstream.o std/csv.o \
	  std/datetime.o std/demangle.o std/encoding.o \
	  std/exception.o std/format.o \
	  std/functional.o std/getopt.o std/json.o \
	  std/math.o std/mathspecial.o \
	  std/metastrings.o std/numeric.o \
	  std/outbuffer.o std/parallelism.o std/signals.o \
	  std/stdio.o std/stdiobase.o std/stdint.o std/stream.o \
	  std/string.o std/syserror.o std/system.o std/random.o std/range.o \
	  std/regex.o std/traits.o std/typecons.o \
	  std/typetuple.o std/uni.o std/uri.o std/utf.o \
	  std/uuid.o std/variant.o std/xml.o std/zip.o std/zlib.o \
	  std/c/fenv.o std/c/locale.o std/c/math.o std/c/process.o \
	  std/c/stdarg.o std/c/stddef.o std/c/stdio.o std/c/stdlib.o \
	  std/c/string.o std/c/time.o std/c/wcharh.o \
	  std/digest/digest.o std/digest/crc.o std/digest/md.o \
	  std/digest/ripemd.o std/digest/sha.o \
	  std/internal/digest/sha_SSSE3.o \
	  std/internal/math/biguintcore.o std/internal/math/biguintnoasm.o \
	  std/internal/math/biguintx86.o std/internal/math/gammafunction.o \
	  std/internal/math/errorfunction.o std/internal/processinit.o \
	  std/internal/uni.o std/internal/uni_tab.o \
	  std/internal/unicode_comp.o std/internal/unicode_decomp.o \
	  std/internal/unicode_grapheme.o std/internal/unicode_norm.o \
	  std/internal/unicode_tables.o \
	  std/net/curl.o std/net/isemail.o \
	  etc/c/curl.o etc/c/zlib.o

Z_OBJS=zlib/adler32.o zlib/compress.o zlib/crc32.o zlib/deflate.o \
       zlib/gzclose.o zlib/gzlib.o zlib/gzread.o zlib/gzwrite.o \
       zlib/infback.o zlib/inffast.o zlib/inflate.o zlib/inftrees.o \
       zlib/trees.o zlib/uncompr.o zlib/zutil.o

ZLIB_OBJS=@ZLIB_OBJS@


# modules which require some kind of operating system
OS_OBJS=std/file.o std/mmfile.o std/path.o std/process.o \
	std/socket.o std/socketstream.o

LINUX_OBJS=std/c/linux/socket.o

OSX_OBJS=std/c/osx/socket.o

FREEBSD_OBJS=std/c/freebsd/socket.o

# std.c.linux.linux, std.loader, gcc.cbridge*
WINDOWS_OBJS=std/c/windows/com.o std/c/windows/stat.o std/c/wcharh.o \
	     std/c/windows/windows.o std/c/windows/winsock.o \
	     std/windows/charset.o std/windows/iunknown.o std/windows/registry.o \
	     std/windows/syserror.o std/internal/windows/advapi32.o

D_EXTRA_OBJS=@D_EXTRA_OBJS@

ALL_PHOBOS_OBJS = $(D_EXTRA_OBJS) $(MAIN_OBJS) $(ZLIB_OBJS)
ALL_PHOBOS_PIC_OBJS = $(subst .o,.pic.o,$(D_EXTRA_OBJS) $(MAIN_OBJS))

libgphobos2.a : $(ALL_PHOBOS_OBJS) ../libdruntime/libgdruntime.a
	cp ../libdruntime/libgdruntime.a libgphobos2.a
	$(AR) -q $@ $(ALL_PHOBOS_OBJS)
	$(RANLIB) $@

$(PHOBOS_SO) : $(ALL_PHOBOS_PIC_OBJS) ../libdruntime/$(DRUNTIME_SO)
	$(GDC) -o $@ -shared -B../libdruntime -nophoboslib -lgdruntime -lz -lcurl $(ALL_PHOBOS_PIC_OBJS)

libgphobos2_t.a : $(ALL_PHOBOS_OBJS:.o=.t.o)
	$(AR) -r $@ $(ALL_PHOBOS_OBJS:.o=.t.o)
	$(RANLIB) $@

$(PHOBOS_T_SO) : $(ALL_PHOBOS_PIC_OBJS:.o=.t.o) ../libdruntime/$(DRUNTIME_SO)
	$(GDC) -o $@ -shared -B../libdruntime -nophoboslib -lgdruntime -lz -lcurl $(ALL_PHOBOS_PIC_OBJS:.o=.t.o)

unittest: libgphobos2.a libgphobos2_t.a unittest.o
	$(GDC) -o $@ $(CFLAGS) unittest.o -L./ -B../libdruntime -nophoboslib -Wl,-static -lgphobos2_t -lgdruntime -Wl,-Bdynamic $(LIBS) -ldl

unittest-pic: $(PHOBOS_T_SO) unittest.pic.o
	$(GDC) -o $@ $(CFLAGS) unittest.pic.o -L./ -nophoboslib -B../libdruntime -lgphobos2_t -lgdruntime $(LIBS)

#--------------------------------------#
# Install, doc, etc targets

install-exec-local: libgphobos2.a
	$(mkinstalldirs) $(DESTDIR)$(toolexeclibdir)
if ENABLE_SHARED
	$(INSTALL) $(PHOBOS_SO) $(DESTDIR)$(toolexeclibdir)
endif
	$(INSTALL) libgphobos2.a $(DESTDIR)$(toolexeclibdir)
	$(RANLIB) $(DESTDIR)$(toolexeclibdir)/libgphobos2.a

install-data-local: libgphobos2.a
	for i in etc/c \
	    std std/c std/c/freebsd \
	    std/c/linux std/c/osx std/c/windows \
	    std/internal std/internal/math std/internal/windows \
	    std/digest std/internal/digest \
	    std/net std/windows; do \
	    $(mkinstalldirs) $(DESTDIR)$(gdc_include_dir)/$$i; \
	    for f in $(srcdir)/$$i/*.d; do \
		$(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; \
	    done; \
	done

clean-local:
	rm -f $(ALL_PHOBOS_OBJS)
	rm -f $(ALL_PHOBOS_OBJS:.o=.t.o)
	rm -f $(ALL_PHOBOS_PIC_OBJS)
	rm -f $(ALL_PHOBOS_PIC_OBJS:.o=.t.o)
	rm -f unittest.o
	rm -f unittest$(EXEEXT)
	rm -f unittest.pic.o
	rm -f unittest-pic$(EXEEXT)
	rm -f libgphobos2.a
	rm -f libgphobos2_t.a
	rm -f $(PHOBOS_SO)
	rm -f $(PHOBOS_T_SO)

check-local: $(PHOBOS_TEST)
if ENABLE_SHARED
	LD_LIBRARY_PATH=".:../libdruntime" ./unittest-pic
endif


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
