// Exception handling and frame unwind runtime interface routines.
// Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

// extern(C) interface for the C6X EABI unwinder library.
// This corresponds to unwind-c6x.h

module gcc.unwind.c6x;

import gcc.config;

static if (GNU_ARM_EABI_Unwinder):

// Not really the ARM EABI, but pretty close.
public import gcc.unwind.arm_common;

extern (C):

enum int UNWIND_STACK_REG = 31;
// Use A0 as a scratch register within the personality routine.
enum int UNWIND_POINTER_REG = 0;

_Unwind_Reason_Code __gnu_unwind_24bit(_Unwind_Context*, _uw, int);

// Decode an EH table reference to a typeinfo object.
_Unwind_Word _Unwind_decode_typeinfo_ptr(_Unwind_Word base, _Unwind_Word ptr)
{
    _Unwind_Word tmp;

    tmp = *cast(_Unwind_Word*) ptr;
    // Zero values are always NULL.
    if (!tmp)
        return 0;

    // SB-relative indirect.  Propagate the bottom 2 bits, which can
    // contain referenceness information in gnu unwinding tables.
    tmp += base;
    tmp = *cast(_Unwind_Word*)(tmp & ~cast(_Unwind_Word)3) | (tmp & 3);
    return tmp;
}

// Return the address of the instruction, not the actual IP value.
_Unwind_Word _Unwind_GetIP(_Unwind_Context* context)
{
    return _Unwind_GetGR(context, 33);
}

// The dwarf unwinder doesn't understand arm/thumb state.  We assume the
// landing pad uses the same instruction set as the call site.
void _Unwind_SetIP(_Unwind_Context* context, _Unwind_Word val)
{
    return _Unwind_SetGR(context, 33, val);
}
