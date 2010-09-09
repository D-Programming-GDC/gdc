/* GDC -- D front-end for GCC
   Copyright (C) 2007 David Friedman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
