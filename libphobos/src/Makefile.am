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
D_EXTRA_DFLAGS=-nostdinc -pipe -Wno-deprecated -fproperty -I$(srcdir) -I$(srcdir)/../libdruntime -I./libdruntime/$(host_alias)
ALL_DFLAGS=$(DFLAGS) $(D_EXTRA_DFLAGS) $(MULTIFLAGS)

toolexecdir = $(phobos_toolexecdir)
toolexeclibdir = $(phobos_toolexeclibdir)

all-local: libgphobos2.a

SUFFIXES = .d

%.o : %.d
	$(GDC) -o $@ $(ALL_DFLAGS) -c $<

# %.o : %.c
zlib/%.o: $(top_srcdir)/../zlib/%.c
	@test -d zlib || mkdir -p zlib
	$(CC) -o $@ $(OUR_CFLAGS) $(CFLAGS) -c $<

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
# GDC LibPhobos

# Special-case for std.xml, need to emit templates.
std/xml.o : std/xml.d
	$(GDC) -o $@ $(ALL_DFLAGS) -femit-templates -c $<

MAIN_OBJS=std/algorithm.o std/array.o std/ascii.o std/base64.o std/bigint.o \
	  std/bitmanip.o std/compiler.o \
	  std/container.o std/complex.o std/concurrency.o \
	  std/conv.o std/cpuid.o std/cstream.o std/ctype.o std/csv.o \
	  std/datetime.o std/demangle.o std/encoding.o \
	  std/exception.o std/format.o \
	  std/functional.o std/getopt.o std/json.o \
	  std/math.o std/mathspecial.o std/md5.o \
	  std/metastrings.o std/numeric.o \
	  std/outbuffer.o std/parallelism.o std/signals.o \
	  std/stdio.o std/stdiobase.o std/stdint.o std/stream.o \
	  std/string.o std/syserror.o std/system.o std/random.o std/range.o \
	  std/regex.o std/regexp.o std/traits.o std/typecons.o \
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
	  std/net/isemail.o \
	  crc32.o etc/c/curl.o etc/c/zlib.o

Z_OBJS=zlib/adler32.o zlib/compress.o zlib/crc32.o zlib/deflate.o \
       zlib/gzclose.o zlib/gzlib.o zlib/gzread.o zlib/gzwrite.o \
       zlib/infback.o zlib/inffast.o zlib/inflate.o zlib/inftrees.o \
       zlib/trees.o zlib/uncompr.o zlib/zutil.o

ZLIB_OBJS=@ZLIB_OBJS@


# modules which require some kind of operating system
OS_OBJS=std/file.o std/mmfile.o std/path.o std/perf.o std/process.o \
	std/socket.o std/socketstream.o

LINUX_OBJS=std/c/linux/linux.o std/c/linux/socket.o

OSX_OBJS=std/c/osx/socket.o

FREEBSD_OBJS=std/c/freebsd/socket.o

# std.c.linux.linux, std.loader, gcc.cbridge*
WINDOWS_OBJS=std/c/windows/com.o std/c/windows/stat.o std/c/wcharh.o \
	     std/c/windows/windows.o std/c/windows/winsock.o \
	     std/windows/charset.o std/windows/iunknown.o std/windows/registry.o \
	     std/windows/syserror.o std/__fileinit.o std/internal/windows/advapi32.o

D_EXTRA_OBJS=@D_EXTRA_OBJS@

ALL_PHOBOS_OBJS = $(D_EXTRA_OBJS) $(MAIN_OBJS) $(ZLIB_OBJS)


libgphobos2.a : $(ALL_PHOBOS_OBJS) ../libdruntime/libgdruntime.a
	cp ../libdruntime/libgdruntime.a libgphobos2.a
	$(AR) -q $@ $(ALL_PHOBOS_OBJS)
	$(RANLIB) $@

libgphobos2_t.a : $(ALL_PHOBOS_OBJS:.o=.t.o)
	$(AR) -r $@ $(ALL_PHOBOS_OBJS:.o=.t.o)
	$(RANLIB) $@

unittest: libgphobos2.a libgphobos2_t.a unittest.o
	$(GDC) -o $@ $(CFLAGS) unittest.o -L./ -L../libdruntime -lgphobos2_t -lgdruntime $(LIBS) -ldl

#--------------------------------------#
# Install, doc, etc targets

install-exec-local: libgphobos2.a
	$(mkinstalldirs) $(DESTDIR)$(toolexeclibdir)
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
		for f in $(srcdir)/$$i/*.d; do $(INSTALL_HEADER) $$f $(DESTDIR)$(gdc_include_dir)/$$i; done; \
	done
	for i in crc32.d; do \
		$(INSTALL_HEADER) $(srcdir)/$$i $(DESTDIR)$(gdc_include_dir); done
#disabled, already installed by druntime
#$(INSTALL) phobos-ver-syms $(DESTDIR)$(gdc_include_dir)/$(host_alias)/$(MULTISUBDIR)

clean-local:
	rm -f $(ALL_PHOBOS_OBJS)
	rm -f $(ALL_PHOBOS_OBJS:.o=.t.o)
	rm -f unittest.o
	rm -f unittest$(EXEEXT)
	rm -f libgphobos2.a
	rm -f libgphobos2_t.a

check-local: unittest
	./unittest


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
