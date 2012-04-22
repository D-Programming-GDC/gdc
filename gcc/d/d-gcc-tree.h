// d-gcc-tree.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_DC_GCC_TREE_H
#define GCC_DCMPLR_DC_GCC_TREE_H

// normally include config.h (hconfig.h, tconfig.h?), but that
// includes things that cause problems, so...

union tree_node;
typedef union tree_node *tree;

#endif
