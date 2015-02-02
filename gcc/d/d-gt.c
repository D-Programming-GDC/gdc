/* d-gt.c -- D frontend for GCC.
   Copyright (C) 2011, 2012 Free Software Foundation, Inc.

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

/* ggc.h and debug.h needed to support gt*.h includes below */
#include "config.h"
#include "system.h"

#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "ggc.h"
#include "debug.h"

#include "d-lang.h"

/* This is in a .c file because gentype produces C code with prototypes */
#include "gtype-d.h"
