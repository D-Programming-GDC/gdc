# GDC -- D front-end for GCC
# Copyright (C) 2016 Johannes Pfau
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

# Make sure GDC can find libdruntime and libphobos include files
D_EXTRA_DFLAGS=-nostdinc -pipe -Wno-deprecated -I $(top_srcdir)/libdruntime \
	-I ../libdruntime
# Make sure linker can find libdruntime and libphobos libraries
D_LFLAGS=-L../libdruntime/.libs -L../src/.libs -B../src

clean-local:
	make -C shared -f $(abs_srcdir)/shared/Makefile clean

check-local: shared/Makefile
if ENABLE_SHARED
	$(GDC) $(DFLAGS) $(MULTIFLAGS) $(D_EXTRA_DFLAGS) $(D_LFLAGS) \
    -shared-libphobos $(srcdir)/supports_loading.d \
    &> /dev/null; \
    if [ $$? -eq 0 ]; then \
        make -C shared -f $(abs_srcdir)/shared/Makefile \
        abs_srcdir="$(abs_srcdir)/shared" abs_top_srcdir="$(abs_top_srcdir)" \
        DMD="$(GDC)" GCC_CFLAGS="$(CFLAGS)" GCC_DFLAGS="$(DFLAGS)" CC="$(CC)"; \
    fi
endif
