// GDC -- D front-end for GCC
// Copyright (C) 2011, 2012, 2014 Free Software Foundation, Inc.

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

// This code is based on the libstdc++ exception handling routines.

module gcc.deh;

import gcc.unwind;
import gcc.unwind.pe;
import gcc.builtins;

import core.memory;
import core.stdc.stdlib;

extern(C)
{
  int _d_isbaseof(ClassInfo, ClassInfo);
  void _d_createTrace(Object *);
}

version(GNU_SEH_Exceptions)
{
    enum EXCEPTION_DISPOSITION
    {
        ExceptionContinueExecution,
        ExceptionContinueSearch,
        ExceptionNestedException,
        ExceptionCollidedUnwind
    }

    //Pointer types. We're lazy, exact definition in MinGW/winnt.h
    alias PEXCEPTION_RECORD = void*;
    alias PCONTEXT = void*;
    alias PDISPATCHER_CONTEXT = void*;

    extern(C) EXCEPTION_DISPOSITION _GCC_specific_handler(PEXCEPTION_RECORD ms_exc,
        void *this_frame, PCONTEXT ms_orig_context, PDISPATCHER_CONTEXT ms_disp,
        _Unwind_Personality_Fn gcc_per);
}

// This is the primary exception class we report -- "GNUCD__\0".
version (GNU_ARM_EABI_Unwinder)
{
  const _Unwind_Exception_Class __gdc_exception_class
    = ['G', 'N', 'U', 'C', 'D', '_', '_', '\0'];
}
else
{
  const _Unwind_Exception_Class __gdc_exception_class
    = (((((((cast(_Unwind_Exception_Class) 'G'
	     << 8 | cast(_Unwind_Exception_Class) 'N')
	    << 8 | cast(_Unwind_Exception_Class) 'U')
	   << 8 | cast(_Unwind_Exception_Class) 'C')
	  << 8 | cast(_Unwind_Exception_Class) 'D')
	 << 8 | cast(_Unwind_Exception_Class) '_')
	<< 8 | cast(_Unwind_Exception_Class) '_')
       << 8 | cast(_Unwind_Exception_Class) '\0');
}


// A D exception object consists of a header, which is a wrapper
// around an unwind object header with additional D specific
// information, followed by the exception object itself.

struct d_exception_header
{
  // The object being thrown.  Like GCJ, the compiled code expects this to 
  // be immediately before the generic exception header.
  // (See build_exception_object)
  enum UNWIND_PAD = (Object.alignof < _Unwind_Exception.alignof)
    ? _Unwind_Exception.alignof - Object.alignof : 0;

  // Because of a lack of __aligned__ style attribute, our object
  // and the unwind object are the first two fields.
  ubyte[UNWIND_PAD] pad;

  Object object;

  // The generic exception header.
  _Unwind_Exception unwindHeader;

  static assert(unwindHeader.offsetof - object.offsetof == object.sizeof);

  version (GNU_ARM_EABI_Unwinder)
  {
    // Nothing here yet.
  }
  else
  {
    // Cache handler details between Phase 1 and Phase 2.
    int handlerSwitchValue;
    ubyte *actionRecord;
    ubyte *languageSpecificData;
    _Unwind_Ptr catchTemp;
  }
}

private d_exception_header *
get_exception_header_from_ue(_Unwind_Exception *exc)
{
  return cast(d_exception_header *)
    (cast(void *) exc - d_exception_header.unwindHeader.offsetof);
}

// D doesn't define these, so they are private for now.

private void
__gdc_terminate()
{
  // Replaces std::terminate and terminating with a specific handler
  abort();
}

// This is called by the unwinder.
extern(C) private void
__gdc_exception_cleanup(_Unwind_Reason_Code code, _Unwind_Exception *exc)
{
  // If we haven't been caught by a foreign handler, then this is
  // some sort of unwind error.  In that case just die immediately.
  // _Unwind_DeleteException in the HP-UX IA64 libunwind library
  //  returns _URC_NO_REASON and not _URC_FOREIGN_EXCEPTION_CAUGHT
  // like the GCC _Unwind_DeleteException function does.
  if (code != _URC_FOREIGN_EXCEPTION_CAUGHT && code != _URC_NO_REASON)
    __gdc_terminate();

  d_exception_header *p = get_exception_header_from_ue (exc);
  destroy (p);
}


// Perform a throw, D style. Throw will unwind through this call,
// so there better not be any handlers or exception thrown here.

extern(C) void
_d_throw(Object object)
{
  // FIXME: OOM errors will throw recursively.
  d_exception_header *xh = new d_exception_header();

  xh.object = object;

  static if ( is(typeof(xh.unwindHeader.exception_class = __gdc_exception_class)) )
    xh.unwindHeader.exception_class = __gdc_exception_class;
  else
    xh.unwindHeader.exception_class[] = __gdc_exception_class[];

  xh.unwindHeader.exception_cleanup = & __gdc_exception_cleanup;

  // Runtime now expects us to do this first before unwinding.
  _d_createTrace (cast(Object *) xh.object);

  // We're happy with setjmp/longjmp exceptions or region-based
  // exception handlers: entry points are provided here for both.
  version (GNU_SjLj_Exceptions)
    _Unwind_SjLj_RaiseException (&xh.unwindHeader);
  else
    _Unwind_RaiseException (&xh.unwindHeader);

  // If code == _URC_END_OF_STACK, then we reached top of stack without
  // finding a handler for the exception.  Since each thread is run in
  // a try/catch, this oughtn't happen.  If code is something else, we
  // encountered some sort of heinous lossage from which we could not
  // recover.  As is the way of such things, almost certainly we will have
  // crashed before now, rather than actually being able to diagnose the
  // problem.
  __gdc_terminate();
}


struct lsda_header_info
{
  _Unwind_Ptr Start;
  _Unwind_Ptr LPStart;
  _Unwind_Ptr ttype_base;
  ubyte *TType;
  ubyte *action_table;
  ubyte ttype_encoding;
  ubyte call_site_encoding;
}

private ubyte *
parse_lsda_header (_Unwind_Context *context, ubyte *p,
		   lsda_header_info *info)
{
  _uleb128_t tmp;
  ubyte lpstart_encoding;

  info.Start = (context ? _Unwind_GetRegionStart (context) : 0);

  // Find @LPStart, the base to which landing pad offsets are relative.
  lpstart_encoding = *p++;
  if (lpstart_encoding != DW_EH_PE_omit)
    p = read_encoded_value (context, lpstart_encoding, p, &info.LPStart);
  else
    info.LPStart = info.Start;

  // Find @TType, the base of the handler and exception spec type data.
  info.ttype_encoding = *p++;
  if (info.ttype_encoding != DW_EH_PE_omit)
    {
      version (GNU_ARM_EABI_Unwinder)
      {
	// Older ARM EABI toolchains set this value incorrectly, so use a
	// hardcoded OS-specific format.
	info.ttype_encoding = _TTYPE_ENCODING;
      }
      p = read_uleb128 (p, &tmp);
      info.TType = p + tmp;
    }
  else
    info.TType = null;

  // The encoding and length of the call-site table; the action table
  // immediately follows.
  info.call_site_encoding = *p++;
  p = read_uleb128 (p, &tmp);
  info.action_table = p + tmp;

  return p;
}

private ClassInfo
get_classinfo_entry(lsda_header_info *info, _uleb128_t i)
{
  _Unwind_Ptr ptr;

  i *= size_of_encoded_value (info.ttype_encoding);
  read_encoded_value_with_base (info.ttype_encoding, info.ttype_base,
				info.TType - i, &ptr);

  return cast(ClassInfo)cast(void *)(ptr);
}

private void
save_caught_exception(_Unwind_Exception *ue_header,
		      _Unwind_Context *context,
		      int handler_switch_value,
		      ubyte *language_specific_data,
		      _Unwind_Ptr landing_pad,
		      ubyte *action_record)
{
  version (GNU_ARM_EABI_Unwinder)
  {
    ue_header.barrier_cache.sp = _Unwind_GetGR(context, UNWIND_STACK_REG);
    ue_header.barrier_cache.bitpattern[1] = cast(_uw) handler_switch_value;
    ue_header.barrier_cache.bitpattern[2] = cast(_uw) language_specific_data;
    ue_header.barrier_cache.bitpattern[3] = cast(_uw) landing_pad;
  }
  else
  {
    d_exception_header *xh = get_exception_header_from_ue (ue_header);

    xh.handlerSwitchValue = handler_switch_value;
    xh.actionRecord = action_record;
    xh.languageSpecificData = language_specific_data;
    xh.catchTemp = landing_pad;
  }
}

private void
restore_caught_exception(_Unwind_Exception *ue_header,
			 ref int handler_switch_value,
			 ref ubyte *language_specific_data,
			 ref _Unwind_Ptr landing_pad)
{
  version (GNU_ARM_EABI_Unwinder)
  {
    handler_switch_value = cast(int) ue_header.barrier_cache.bitpattern[1];
    language_specific_data = cast(ubyte *) ue_header.barrier_cache.bitpattern[2];
    landing_pad = cast(_Unwind_Ptr) ue_header.barrier_cache.bitpattern[3];
  }
  else
  {
    d_exception_header *xh = get_exception_header_from_ue (ue_header);

    handler_switch_value = xh.handlerSwitchValue;
    language_specific_data = xh.languageSpecificData;
    landing_pad = cast(_Unwind_Ptr) xh.catchTemp;
  }
}

// Using a different personality function name causes link failures
// when trying to mix code using different exception handling models.
// extern(C) alias __gdc_personality_impl ...; would be nice
version(GNU_SEH_Exceptions)
{
  extern(C) EXCEPTION_DISPOSITION
  __gdc_personality_seh0 (PEXCEPTION_RECORD ms_exc, void *this_frame,
			PCONTEXT ms_orig_context, PDISPATCHER_CONTEXT ms_disp)
  {
    return _GCC_specific_handler (ms_exc, this_frame, ms_orig_context,
				ms_disp, &__gdc_personality_imp);
  }
  extern(C) _Unwind_Reason_Code
  __gdc_personality_imp(int iversion,
		       _Unwind_Action actions,
		       _Unwind_Exception_Class exception_class,
		       _Unwind_Exception *ue_header,
		       _Unwind_Context *context)
  {
    return __gdc_personality_impl (iversion, actions,
				   exception_class != __gdc_exception_class,
				   ue_header, context);
  }
}
else version (GNU_SjLj_Exceptions)
{
  extern(C) _Unwind_Reason_Code
  __gdc_personality_sj0(int iversion,
			_Unwind_Action actions,
			_Unwind_Exception_Class exception_class,
			_Unwind_Exception *ue_header,
			_Unwind_Context *context)
  {
    return __gdc_personality_impl (iversion, actions,
				   exception_class != __gdc_exception_class,
				   ue_header, context);
  }

  private int __builtin_eh_return_data_regno(int x) { return x; }
}
else version (GNU_ARM_EABI_Unwinder)
{
  extern(C) _Unwind_Reason_Code
  __gdc_personality_v0(_Unwind_State state,
		       _Unwind_Exception* ue_header,
		       _Unwind_Context* context)
  {
    _Unwind_Action actions;

    switch (state & _US_ACTION_MASK)
      {
      case _US_VIRTUAL_UNWIND_FRAME:
	actions = _UA_SEARCH_PHASE;
	break;

      case _US_UNWIND_FRAME_STARTING:
	actions = _UA_CLEANUP_PHASE;
	if (!(state & _US_FORCE_UNWIND)
	    && ue_header.barrier_cache.sp == _Unwind_GetGR (context, UNWIND_STACK_REG))
	  actions |= _UA_HANDLER_FRAME;
	break;

      case _US_UNWIND_FRAME_RESUME:
	if (__gnu_unwind_frame (ue_header, context) != _URC_OK)
	  return _URC_FAILURE;
	return _URC_CONTINUE_UNWIND;

      default:
	abort();
      }
    actions |= state & _US_FORCE_UNWIND;

    // We don't know which runtime we're working with, so can't check this.
    // However the ABI routines hide this from us, and we don't actually need to knowa
    bool foreign_exception = false;

    return __gdc_personality_impl (1, actions, foreign_exception, ue_header, context);
  }
}
else
{
  extern(C) _Unwind_Reason_Code
  __gdc_personality_v0(int iversion,
		       _Unwind_Action actions,
		       _Unwind_Exception_Class exception_class,
		       _Unwind_Exception *ue_header,
		       _Unwind_Context *context)
  {
    return __gdc_personality_impl (iversion, actions,
				   exception_class != __gdc_exception_class,
				   ue_header, context);
  }
}

private _Unwind_Reason_Code
__gdc_personality_impl(int iversion,
		       _Unwind_Action actions,
		       bool foreign_exception,
		       _Unwind_Exception *ue_header,
		       _Unwind_Context *context)
{
  enum Found
  {
    nothing,
    terminate,
    cleanup,
    handler
  }

  Found found_type;
  lsda_header_info info;
  ubyte *language_specific_data;
  ubyte *action_record;
  ubyte *p;
  _Unwind_Ptr landing_pad, ip;
  int handler_switch_value;
  int ip_before_insn = 0;

  version (GNU_ARM_EABI_Unwinder)
  {
    // The dwarf unwinder assumes the context structure holds things like the
    // function and LSDA pointers.  The ARM implementation caches these in
    // the exception header (UCB).  To avoid rewriting everything we make the
    // virtual IP register point at the UCB.
    ip = cast(_Unwind_Ptr) ue_header;
    _Unwind_SetGR (context, UNWIND_POINTER_REG, ip);
  }
  else
  {
    // Interface version check.
    if (iversion != 1)
      return _URC_FATAL_PHASE1_ERROR;
  }

  d_exception_header *xh = get_exception_header_from_ue (ue_header);

  // Shortcut for phase 2 found handler for domestic exception.
  if (actions == (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME)
      && ! foreign_exception)
    {
      restore_caught_exception(ue_header, handler_switch_value,
			       language_specific_data, landing_pad);
      found_type = (landing_pad == 0 ? Found.terminate : Found.handler);
      goto install_context;
    }

  // NOTE: In Phase 1, record _Unwind_GetIPInfo in xh.object as a part of
  // the stack trace for this exception.  This will only collect D frames,
  // but perhaps that is acceptable.
  language_specific_data = cast(ubyte *)
    _Unwind_GetLanguageSpecificData (context);

  // If no LSDA, then there are no handlers or cleanups.
  if (! language_specific_data)
    {
      version (GNU_ARM_EABI_Unwinder)
	if (__gnu_unwind_frame (ue_header, context) != _URC_OK)
	  return _URC_FAILURE;
      return _URC_CONTINUE_UNWIND;
    }

  // Parse the LSDA header
  p = parse_lsda_header (context, language_specific_data, &info);
  info.ttype_base = base_of_encoded_value (info.ttype_encoding, context);
  ip = _Unwind_GetIPInfo (context, &ip_before_insn);

  if (! ip_before_insn)
    --ip;
  landing_pad = 0;
  action_record = null;
  handler_switch_value = 0;

  version (GNU_SjLj_Exceptions)
  {
    // The given "IP" is an index into the call-site table, with two
    // exceptions -- -1 means no-action, and 0 means terminate.  But
    // since we're using uleb128 values, we've not got random access
    // to the array.
    if (cast(int) ip < 0)
      return _URC_CONTINUE_UNWIND;
    else if (ip == 0)
      {
	// Fall through to set Found.terminate.
      }
    else
      {
	_uleb128_t cs_lp, cs_action;
	do
	  {
	    p = read_uleb128 (p, &cs_lp);
	    p = read_uleb128 (p, &cs_action);
	  }
	while (--ip);

	// Can never have null landing pad for sjlj -- that would have
	// been indicated by a -1 call site index.
	landing_pad = cs_lp + 1;
	if (cs_action)
	  action_record = info.action_table + cs_action - 1;
	goto found_something;
      }
  }
  else
  {
    // Search the call-site table for the action associated with this IP.
    while (p < info.action_table)
      {
	_Unwind_Ptr cs_start, cs_len, cs_lp;
	_uleb128_t cs_action;

	// Note that all call-site encodings are "absolute" displacements.
	p = read_encoded_value (null, info.call_site_encoding, p, &cs_start);
	p = read_encoded_value (null, info.call_site_encoding, p, &cs_len);
	p = read_encoded_value (null, info.call_site_encoding, p, &cs_lp);
	p = read_uleb128 (p, &cs_action);

	// The table is sorted, so if we've passed the ip, stop.
	if (ip < info.Start + cs_start)
	  p = info.action_table;
	else if (ip < info.Start + cs_start + cs_len)
	  {
	    if (cs_lp)
	      landing_pad = info.LPStart + cs_lp;
	    if (cs_action)
	      action_record = info.action_table + cs_action - 1;
	    goto found_something;
	  }
      }
  }

  // If ip is not present in the table, C++ would call terminate.
  // This is for a destructor inside a cleanup, or a library routine
  // the compiler was not expecting to throw.
  found_type = Found.terminate;
  goto do_something;

 found_something:
  if (landing_pad == 0)
    {
      // If ip is present, and has a null landing pad, there are
      // no cleanups or handlers to be run.
      found_type = Found.nothing;
    }
  else if (action_record == null)
    {
      // If ip is present, has a non-null landing pad, and a null
      // action table offset, then there are only cleanups present.
      // Cleanups use a zero switch value, as set above.
      found_type = Found.cleanup;
    }
  else
    {
      // Otherwise we have a catch handler or exception specification.
      _sleb128_t ar_filter, ar_disp;
      bool saw_cleanup = false;
      bool saw_handler = false;

      while (1)
	{
	  p = action_record;
	  p = read_sleb128 (p, &ar_filter);
	  read_sleb128 (p, &ar_disp);

	  if (ar_filter == 0)
	    {
	      // Zero filter values are cleanups.
	      saw_cleanup = true;
	    }
	  else if ((actions & _UA_FORCE_UNWIND) || foreign_exception)
	    {
	      // During forced unwinding, we only run cleanups.  With a
	      // foreign exception class, we have no class info to match.
	      // ??? What to do about GNU Java and GNU Ada exceptions.
	    }
	  else if (ar_filter > 0)
	    {
	      // Positive filter values are handlers.
	      ClassInfo catch_type = get_classinfo_entry (&info, ar_filter);

	      // Null catch type is a catch-all handler; we can catch foreign
	      // exceptions with this.  Otherwise we must match types.
	      // D Note: will be performing dynamic cast twice, potentially
	      // Once here and once at the landing pad .. unless we cached
	      // here and had a begin_catch call.
	      if (catch_type is null || _d_isbaseof (xh.object.classinfo, catch_type))
		{
		  saw_handler = true;
		  break;
		}
	    }
	  else
	    {
	      // D Note: we don't have these...
	      break;
	    }

	  if (ar_disp == 0)
	    break;
	  action_record = p + ar_disp;
	}

      if (saw_handler)
	{
	  handler_switch_value = cast(int) ar_filter;
	  found_type = Found.handler;
	}
      else
	found_type = (saw_cleanup ? Found.cleanup : Found.nothing);
    }

 do_something:
  if (found_type == Found.nothing)
    {
      version (GNU_ARM_EABI_Unwinder)
	if (__gnu_unwind_frame (ue_header, context) != _URC_OK)
	  return _URC_FAILURE;
      return _URC_CONTINUE_UNWIND;
    }

  if (actions & _UA_SEARCH_PHASE)
    {
      if (found_type == Found.cleanup)
	{
	  version (GNU_ARM_EABI_Unwinder)
	    if (__gnu_unwind_frame (ue_header, context) != _URC_OK)
	      return _URC_FAILURE;
	  return _URC_CONTINUE_UNWIND;
	}

      // For domestic exceptions, we cache data from phase 1 for phase 2.
      if (! foreign_exception)
	{
	  save_caught_exception (ue_header, context, handler_switch_value,
				 language_specific_data, landing_pad,
				 action_record);
	}
      return _URC_HANDLER_FOUND;
    }

 install_context:
  // We can't use any of the deh routines with foreign exceptions,
  // because they all expect ue_header to be an d_exception_header.
  // So in that case, call terminate or unexpected directly.
  if ((actions & _UA_FORCE_UNWIND) || foreign_exception)
    {
      if (found_type == Found.terminate || handler_switch_value < 0)
	__gdc_terminate();
    }
  else
    {
      if (found_type == Found.terminate)
	__gdc_terminate();

      // Cache the TType base value for unexpected calls, as we won't
      // have an _Unwind_Context then.
      if (handler_switch_value < 0)
	{
	  parse_lsda_header (context, language_specific_data, &info);
	  info.ttype_base = base_of_encoded_value (info.ttype_encoding, context);

	  version (GNU_ARM_EABI_Unwinder)
	    ue_header.barrier_cache.bitpattern[1] = info.ttype_base;
	  else
	    xh.catchTemp = info.ttype_base;
	}
    }

  // For targets with pointers smaller than the word size, we must extend the
  // pointer, and this extension is target dependent.
  _Unwind_SetGR (context, __builtin_eh_return_data_regno (0),
		 cast(_Unwind_Ptr) ue_header);
  _Unwind_SetGR (context, __builtin_eh_return_data_regno (1),
		 handler_switch_value);
  _Unwind_SetIP (context, landing_pad);

  return _URC_INSTALL_CONTEXT;
}

