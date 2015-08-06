// d-irstate.h -- D frontend for GCC.
// Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

#ifndef GCC_DCMPLR_IRSTATE_H
#define GCC_DCMPLR_IRSTATE_H

// IRState contains the core functionality of code generation utilities.
//
// Currently, each function gets its own IRState when emitting code.  There is
// also a global IRState current_irstate.
//
// Most toElem calls don't actually need the IRState because they create GCC
// expression trees rather than emit instructions.

// Functions without a verb create trees
// Functions with 'do' affect the current instruction stream (or output assembler code).
// functions with other names are for attribute manipulate, etc.

struct IRState
{
};


#endif
