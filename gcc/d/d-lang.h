/* d-lang.h -- D frontend for GCC.
   Copyright (C) 2011-2013 Free Software Foundation, Inc.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.
*/


#ifndef GCC_DCMPLR_DC_LANG_H
#define GCC_DCMPLR_DC_LANG_H

/* protect from garbage collection */
extern GTY(()) tree d_keep_list;
extern GTY(()) tree d_eh_personality_decl;

// Array of all global declarations to pass back to the middle-end.
extern GTY(()) vec<tree, va_gc> *global_declarations;
#endif
