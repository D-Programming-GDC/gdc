// gdc_alloca.h -- D frontend for GCC.
// Originally contributed by David Friedman
// Maintained by Iain Buclaw

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

/* This should be autoconf'd, but I want to avoid
   patching the configure script. */
#include <stdlib.h>
#ifndef alloca
# if _WIN32
#  include <malloc.h>
# elif __sun__
#  include <alloca.h>
# elif SKYOS
#  define alloca __builtin_alloca
# elif defined(__APPLE__) && (GCC_VER <= 33) || defined(__OpenBSD__)
#  include <stdlib.h>
# else
/* guess... */
#  include <alloca.h>
# endif
#endif
