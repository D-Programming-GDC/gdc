/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman
   
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
/* GNU/GCC unwind interface declarations for D.  This must match unwind.h */

module gcc.unwind;
private import gcc.builtins;
private import std.c.process; // for abort


/* This is derived from the C++ ABI for IA-64.  Where we diverge
   for cross-architecture compatibility are noted with "@@@".  */

extern (C):

/* Level 1: Base ABI  */

/* @@@ The IA-64 ABI uses uint64 throughout.  Most places this is
   inefficient for 32-bit and smaller machines.  */

// %% These were typedefs, then made alias -- there are some
// extra casts now

alias __builtin_machine_uint _Unwind_Word;
alias __builtin_machine_int  _Unwind_Sword;
alias __builtin_pointer_uint _Unwind_Internal_Ptr;

version (IA64) {
    version (HPUX) {
	alias __builtin_machine_uint _Unwind_Ptr;
    } else {
	alias __builtin_pointer_uint _Unwind_Ptr;
    }
} else {
    alias __builtin_pointer_uint _Unwind_Ptr;
}


/* @@@ The IA-64 ABI uses a 64-bit word to identify the producer and
   consumer of an exception.  We'll go along with this for now even on
   32-bit machines.  We'll need to provide some other option for
   16-bit machines and for machines with > 8 bits per byte.  */
alias ulong _Unwind_Exception_Class;

/* The unwind interface uses reason codes in several contexts to
   identify the reasons for failures or other actions.  */
enum
{
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8
}
alias uint _Unwind_Reason_Code;


/* The unwind interface uses a pointer to an exception header object
   as its representation of an exception being thrown. In general, the
   full representation of an exception object is language- and
   implementation-specific, but it will be prefixed by a header
   understood by the unwind interface.  */

extern(C) typedef void (*_Unwind_Exception_Cleanup_Fn) (_Unwind_Reason_Code,
					      _Unwind_Exception *);

align struct _Unwind_Exception // D Note: this may not be "maxium alignment required by any type"?
{
  _Unwind_Exception_Class exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;
  _Unwind_Word private_1;
  _Unwind_Word private_2;

  /* @@@ The IA-64 ABI says that this structure must be double-word aligned.
     Taking that literally does not make much sense generically.  Instead we
     provide the maximum alignment required by any type for the machine.  */
}


/* The ACTIONS argument to the personality routine is a bitwise OR of one
   or more of the following constants.  */
typedef int _Unwind_Action;

enum
{
    _UA_SEARCH_PHASE  =	1,
    _UA_CLEANUP_PHASE =	2,
    _UA_HANDLER_FRAME =	4,
    _UA_FORCE_UNWIND  =	8,
    _UA_END_OF_STACK  =	16
}

/* This is an opaque type used to refer to a system-specific data
   structure used by the system unwinder. This context is created and
   destroyed by the system, and passed to the personality routine
   during unwinding.  */
struct _Unwind_Context;

/* Raise an exception, passing along the given exception object.  */
_Unwind_Reason_Code _Unwind_RaiseException (_Unwind_Exception *);

/* Raise an exception for forced unwinding.  */

extern(C) typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn)
     (int, _Unwind_Action, _Unwind_Exception_Class,
      _Unwind_Exception *, _Unwind_Context *, void *);

_Unwind_Reason_Code _Unwind_ForcedUnwind (_Unwind_Exception *,
						 _Unwind_Stop_Fn,
						 void *);

/* Helper to invoke the exception_cleanup routine.  */
void _Unwind_DeleteException (_Unwind_Exception *);

/* Resume propagation of an existing exception.  This is used after
   e.g. executing cleanup code, and not to implement rethrowing.  */
void _Unwind_Resume (_Unwind_Exception *);

/* @@@ Resume propagation of an FORCE_UNWIND exception, or to rethrow
   a normal exception that was handled.  */
_Unwind_Reason_Code _Unwind_Resume_or_Rethrow (_Unwind_Exception *);

/* @@@ Use unwind data to perform a stack backtrace.  The trace callback
   is called for every stack frame in the call chain, but no cleanup
   actions are performed.  */
extern(C) typedef _Unwind_Reason_Code (*_Unwind_Trace_Fn)
     (_Unwind_Context *, void *);

_Unwind_Reason_Code _Unwind_Backtrace (_Unwind_Trace_Fn, void *);

/* These functions are used for communicating information about the unwind
   context (i.e. the unwind descriptors and the user register state) between
   the unwind library and the personality routine and landing pad.  Only
   selected registers maybe manipulated.  */

_Unwind_Word _Unwind_GetGR (_Unwind_Context *, int);
void _Unwind_SetGR (_Unwind_Context *, int, _Unwind_Word);

_Unwind_Ptr _Unwind_GetIP (_Unwind_Context *);
void _Unwind_SetIP (_Unwind_Context *, _Unwind_Ptr);

/* @@@ Retrieve the CFA of the given context.  */
_Unwind_Word _Unwind_GetCFA (_Unwind_Context *);

void *_Unwind_GetLanguageSpecificData (_Unwind_Context *);

_Unwind_Ptr _Unwind_GetRegionStart (_Unwind_Context *);


/* The personality routine is the function in the C++ (or other language)
   runtime library which serves as an interface between the system unwind
   library and language-specific exception handling semantics.  It is
   specific to the code fragment described by an unwind info block, and
   it is always referenced via the pointer in the unwind info block, and
   hence it has no ABI-specified name.

   Note that this implies that two different C++ implementations can
   use different names, and have different contents in the language
   specific data area.  Moreover, that the language specific data
   area contains no version info because name of the function invoked
   provides more effective versioning by detecting at link time the
   lack of code to handle the different data format.  */

extern(C) typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)
     (int, _Unwind_Action, _Unwind_Exception_Class,
      _Unwind_Exception *, _Unwind_Context *);

/* @@@ The following alternate entry points are for setjmp/longjmp
   based unwinding.  */

struct SjLj_Function_Context;
extern void _Unwind_SjLj_Register (SjLj_Function_Context *);
extern void _Unwind_SjLj_Unregister (SjLj_Function_Context *);

_Unwind_Reason_Code _Unwind_SjLj_RaiseException
     (_Unwind_Exception *);
_Unwind_Reason_Code _Unwind_SjLj_ForcedUnwind
     (_Unwind_Exception *, _Unwind_Stop_Fn, void *);
void _Unwind_SjLj_Resume (_Unwind_Exception *);
_Unwind_Reason_Code _Unwind_SjLj_Resume_or_Rethrow (_Unwind_Exception *);

/* @@@ The following provide access to the base addresses for text
   and data-relative addressing in the LDSA.  In order to stay link
   compatible with the standard ABI for IA-64, we inline these.  */

version (IA64) {
private import std.c.process;

_Unwind_Ptr
_Unwind_GetDataRelBase (_Unwind_Context *_C)
{
  /* The GP is stored in R1.  */
  return _Unwind_GetGR (_C, 1);
}

_Unwind_Ptr
_Unwind_GetTextRelBase (_Unwind_Context *)
{
  abort ();
  return 0;
}

/* @@@ Retrieve the Backing Store Pointer of the given context.  */
_Unwind_Word _Unwind_GetBSP (_Unwind_Context *);
} else {
_Unwind_Ptr _Unwind_GetDataRelBase (_Unwind_Context *);
_Unwind_Ptr _Unwind_GetTextRelBase (_Unwind_Context *);
}

/* @@@ Given an address, return the entry point of the function that
   contains it.  */
extern void * _Unwind_FindEnclosingFunction (void *pc);

// D Note: end of bits from unwind.h

/* @@@ Really this should be out of line, but this also causes link
   compatibility problems with the base ABI.  This is slightly better
   than duplicating code, however.  */

/* Pointer encodings, from dwarf2.h.  */
enum {
    DW_EH_PE_absptr = 0x00,
    DW_EH_PE_omit = 0xff,

    DW_EH_PE_uleb128 = 0x01,
    DW_EH_PE_udata2 = 0x02,
    DW_EH_PE_udata4 = 0x03,
    DW_EH_PE_udata8 = 0x04,
    DW_EH_PE_sleb128 = 0x09,
    DW_EH_PE_sdata2 = 0x0A,
    DW_EH_PE_sdata4 = 0x0B,
    DW_EH_PE_sdata8 = 0x0C,
    DW_EH_PE_signed = 0x08,

    DW_EH_PE_pcrel = 0x10,
    DW_EH_PE_textrel = 0x20,
    DW_EH_PE_datarel = 0x30,
    DW_EH_PE_funcrel = 0x40,
    DW_EH_PE_aligned = 0x50,

    DW_EH_PE_indirect = 0x80
}

version (NO_SIZE_OF_ENCODED_VALUE) {
} else {
    /* Given an encoding, return the number of bytes the format occupies.
       This is only defined for fixed-size encodings, and so does not
       include leb128.  */

    uint
    size_of_encoded_value (ubyte encoding)
    {
      if (encoding == DW_EH_PE_omit)
	return 0;

      switch (encoding & 0x07)
	{
	case DW_EH_PE_absptr:
	  return (void *).sizeof;
	case DW_EH_PE_udata2:
	  return 2;
	case DW_EH_PE_udata4:
	  return 4;
	case DW_EH_PE_udata8:
	  return 8;
	}
      abort ();
    }
}

version (NO_BASE_OF_ENCODED_VALUE) {
} else {
    /* Given an encoding and an _Unwind_Context, return the base to which
       the encoding is relative.  This base may then be passed to
       read_encoded_value_with_base for use when the _Unwind_Context is
       not available.  */

    _Unwind_Ptr
    base_of_encoded_value (ubyte encoding, _Unwind_Context *context)
    {
      if (encoding == DW_EH_PE_omit)
	return cast(_Unwind_Ptr) 0;

      switch (encoding & 0x70)
	{
	case DW_EH_PE_absptr:
	case DW_EH_PE_pcrel:
	case DW_EH_PE_aligned:
	  return cast(_Unwind_Ptr) 0;

	case DW_EH_PE_textrel:
	  return _Unwind_GetTextRelBase (context);
	case DW_EH_PE_datarel:
	  return _Unwind_GetDataRelBase (context);
	case DW_EH_PE_funcrel:
	  return _Unwind_GetRegionStart (context);
	}
      abort ();
    }
}

/* Read an unsigned leb128 value from P, store the value in VAL, return
   P incremented past the value.  We assume that a word is large enough to
   hold any value so encoded; if it is smaller than a pointer on some target,
   pointers should not be leb128 encoded on that target.  */

ubyte *
read_uleb128 (ubyte *p, _Unwind_Word *val)
{
  uint shift = 0;
  ubyte a_byte;
  _Unwind_Word result;

  result = cast(_Unwind_Word) 0;
  do
    {
      a_byte = *p++;
      result |= (cast(_Unwind_Word)a_byte & 0x7f) << shift;
      shift += 7;
    }
  while (a_byte & 0x80);

  *val = result;
  return p;
}


/* Similar, but read a signed leb128 value.  */

ubyte *
read_sleb128 (ubyte *p, _Unwind_Sword *val)
{
  uint shift = 0;
  ubyte a_byte;
  _Unwind_Word result;

  result = cast(_Unwind_Word) 0;
  do
    {
      a_byte = *p++;
      result |= (cast(_Unwind_Word)a_byte & 0x7f) << shift;
      shift += 7;
    }
  while (a_byte & 0x80);

  /* Sign-extend a negative value.  */
  if (shift < 8 * result.sizeof && (a_byte & 0x40) != 0)
    result |= -((cast(_Unwind_Word)1L) << shift);

  *val = cast(_Unwind_Sword) result;
  return p;
}

/* Load an encoded value from memory at P.  The value is returned in VAL;
   The function returns P incremented past the value.  BASE is as given
   by base_of_encoded_value for this encoding in the appropriate context.  */

ubyte *
read_encoded_value_with_base (ubyte encoding, _Unwind_Ptr base,
			      ubyte *p, _Unwind_Ptr *val)
{
    // D Notes: Todo -- packed!
  union unaligned
    {
      align(1) void *ptr;
      align(1) ushort u2 ;
      align(1) uint u4 ;
      align(1) ulong u8 ;
      align(1) short s2 ;
      align(1) int s4 ;
      align(1) long s8 ;
    }

  unaligned *u = cast(unaligned *) p;
  _Unwind_Internal_Ptr result;

  if (encoding == DW_EH_PE_aligned)
    {
      _Unwind_Internal_Ptr a = cast(_Unwind_Internal_Ptr) p;
      a = cast(_Unwind_Internal_Ptr)( (a + (void *).sizeof - 1) & - (void *).sizeof );
      result = * cast(_Unwind_Internal_Ptr *) a;
      p = cast(ubyte *) cast(_Unwind_Internal_Ptr) (a + (void *).sizeof);
    }
  else
    {
      switch (encoding & 0x0f)
	{
	case DW_EH_PE_absptr:
	  result = cast(_Unwind_Internal_Ptr) u.ptr;
	  p += (void *).sizeof;
	  break;

	case DW_EH_PE_uleb128:
	  {
	    _Unwind_Word tmp;
	    p = read_uleb128 (p, &tmp);
	    result = cast(_Unwind_Internal_Ptr) tmp;
	  }
	  break;

	case DW_EH_PE_sleb128:
	  {
	    _Unwind_Sword tmp;
	    p = read_sleb128 (p, &tmp);
	    result = cast(_Unwind_Internal_Ptr) tmp;
	  }
	  break;

	case DW_EH_PE_udata2:
	  result = cast(_Unwind_Internal_Ptr) u.u2;
	  p += 2;
	  break;
	case DW_EH_PE_udata4:
	  result = cast(_Unwind_Internal_Ptr) u.u4;
	  p += 4;
	  break;
	case DW_EH_PE_udata8:
	  result = cast(_Unwind_Internal_Ptr) u.u8;
	  p += 8;
	  break;

	case DW_EH_PE_sdata2:
	  result = cast(_Unwind_Internal_Ptr) u.s2;
	  p += 2;
	  break;
	case DW_EH_PE_sdata4:
	  result = cast(_Unwind_Internal_Ptr) u.s4;
	  p += 4;
	  break;
	case DW_EH_PE_sdata8:
	  result = cast(_Unwind_Internal_Ptr) u.s8;
	  p += 8;
	  break;

	default:
	  abort ();
	}

      if (result != 0)
	{
	  result += ((encoding & 0x70) == DW_EH_PE_pcrel
		     ? cast(_Unwind_Internal_Ptr) u : base);
	  if (encoding & DW_EH_PE_indirect)
	    result = *cast(_Unwind_Internal_Ptr *) result;
	}
    }

  *val = result;
  return p;
}

version (NO_BASE_OF_ENCODED_VALUE) {
} else {
    /* Like read_encoded_value_with_base, but get the base from the context
       rather than providing it directly.  */

    ubyte *
    read_encoded_value (_Unwind_Context *context, ubyte encoding,
			ubyte *p, _Unwind_Ptr *val)
    {
      return read_encoded_value_with_base (encoding,
		    base_of_encoded_value (encoding, context),
		    p, val);
    }
}

