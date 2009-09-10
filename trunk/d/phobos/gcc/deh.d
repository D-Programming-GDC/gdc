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
/*
  This code is based on the libstdc++ exception handling routines.
*/

module gcc.deh; 
private import gcc.unwind;
private import gcc.builtins;
private import std.gc;
private import std.c.stdlib;
private import std.c.process;

// User code creates the exception and there's no copy method, so we'll
// just use the (probably) GC'd object (..shouldn't allow throwing auto classes)
// so how to make sure exception is referenced? -- for now, just use GC'd memory

// "GNUCD__\0"
const _Unwind_Exception_Class GDC_Exception_Class = 0x005f5f4443554e47L;


struct OurUnwindException {
    // Cache parsed handler data from the personality routine Phase 1
    // for Phase 2 and __cxa_call_unexpected.
    // DNote: Added a separate field for landingPad (and removed adjusted_ptr)
    int handlerSwitchValue;
    ubyte *actionRecord;
    ubyte *languageSpecificData;
    _Unwind_Ptr catchTemp;
    _Unwind_Ptr landingPad;

    // This must be directly behind unwindHeader. (IRState::exceptionObject)
    Object            obj;

    _Unwind_Exception unwindHeader;
    
    static OurUnwindException * fromHeader(_Unwind_Exception * p_ue) {
	return (cast(OurUnwindException *)(p_ue + 1)) - 1;
    }
}


// D doesn't define these, so they are private for now.
private void dehTerminate() {
    //  replaces std::terminate and terminating with a specific handler
    abort();
}
private void dehUnexpected() {
}
private void dehBeginCatch(_Unwind_Exception *exc)
{
    // nothing
}

// This is called by the unwinder.

private extern (C) void
_gdc_cleanupException (_Unwind_Reason_Code code, _Unwind_Exception *exc)
{
  // If we haven't been caught by a foreign handler, then this is
  // some sort of unwind error.  In that case just die immediately.
  // _Unwind_DeleteException in the HP-UX IA64 libunwind library
  //  returns _URC_NO_REASON and not _URC_FOREIGN_EXCEPTION_CAUGHT
  // like the GCC _Unwind_DeleteException function does.
  if (code != _URC_FOREIGN_EXCEPTION_CAUGHT && code != _URC_NO_REASON)
      dehTerminate(); // __terminate (header->terminateHandler);

  OurUnwindException * p = OurUnwindException.fromHeader( exc );
  delete p;
}

// This is called by compiler-generated code for throw
// statements.

extern (C) public void
_d_throw(Object obj)
{
    OurUnwindException * exc = new OurUnwindException;
    exc.obj = obj;
    exc.unwindHeader.exception_class = GDC_Exception_Class;
    exc.unwindHeader.exception_cleanup = & _gdc_cleanupException;
    
    version (GNU_SjLj_Exceptions) {
	_Unwind_SjLj_RaiseException (&exc.unwindHeader);
    } else {
	_Unwind_RaiseException (&exc.unwindHeader);
    }

    // Some sort of unwinding error.  Note that terminate is a handler.
    dehBeginCatch (&exc.unwindHeader);
    dehTerminate(); // std::terminate ();
}

extern (C) int _d_isbaseof(ClassInfo, ClassInfo);

// rethrow?

// extern(C) alias personalityImpl ...; would be nice
version (GNU_SjLj_Exceptions) {
    extern (C) _Unwind_Reason_Code __gdc_personality_sj0(int iversion,
	_Unwind_Action actions,
	_Unwind_Exception_Class exception_class,
	_Unwind_Exception *ue_header,
	_Unwind_Context *context) {
	return personalityImpl(iversion, actions, exception_class, ue_header, context);
    }
    
    private int __builtin_eh_return_data_regno(int x) { return x; }
} else {
    extern (C) _Unwind_Reason_Code __gdc_personality_v0(int iversion,
	_Unwind_Action actions,
	_Unwind_Exception_Class exception_class,
	_Unwind_Exception *ue_header,
	_Unwind_Context *context) {
	return personalityImpl(iversion, actions, exception_class, ue_header, context);
    }
}

private _Unwind_Reason_Code personalityImpl(int iversion,
    _Unwind_Action actions,
    _Unwind_Exception_Class exception_class,
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
    OurUnwindException * xh = OurUnwindException.fromHeader(ue_header);
    ubyte *language_specific_data;
    ubyte *p;
    ubyte *action_record;
    int handler_switch_value;
    _Unwind_Ptr landing_pad, ip;
	
    if (iversion != 1)
	return _URC_FATAL_PHASE1_ERROR;

    // Shortcut for phase 2 found handler for domestic exception.
    if (actions == (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME)
	&& exception_class == GDC_Exception_Class) {

	handler_switch_value = xh.handlerSwitchValue;
	language_specific_data = xh.languageSpecificData;
	landing_pad = cast(_Unwind_Ptr) xh.landingPad;
	found_type = (landing_pad == 0 ? Found.terminate : Found.handler);
	goto install_context;
    }

    language_specific_data = cast(ubyte *) _Unwind_GetLanguageSpecificData( context );
    
    // If no LSDA, then there are no handlers or cleanups.
    if ( ! language_specific_data )
	return _URC_CONTINUE_UNWIND;

    // Parse the LSDA header
    p = parse_lsda_header(context, language_specific_data, & info);
    info.ttype_base = base_of_encoded_value(info.ttype_encoding, context);
    ip = _Unwind_GetIP(context) - 1;
    landing_pad = 0;
    action_record = null;
    handler_switch_value = 0;

    version (GNU_SjLj_Exceptions) {
	// The given "IP" is an index into the call-site table, with two
	// exceptions -- -1 means no-action, and 0 means terminate.  But
	// since we're using uleb128 values, we've not got random access
	// to the array.
	if (cast(int) ip < 0)
	    return _URC_CONTINUE_UNWIND;
	else if (ip == 0)
	    {
		// Fall through to set found_terminate.
	    }
	else
	    {
		_Unwind_Word cs_lp, cs_action;
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
    } else {
	while (p < info.action_table) {
	    _Unwind_Ptr cs_start, cs_len, cs_lp;
	    _Unwind_Word cs_action;

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

  // If ip is not present in the table, call terminate.  This is for
  // a destructor inside a cleanup, or a library routine the compiler
  // was not expecting to throw.
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

      _Unwind_Sword ar_filter, ar_disp;
      ClassInfo throw_type, catch_type;
      bool saw_cleanup = false;
      bool saw_handler = false;

      // During forced unwinding, we only run cleanups.  With a foreign
      // exception class, there's no exception type.
      // ??? What to do about GNU Java and GNU Ada exceptions.

      if ((actions & _UA_FORCE_UNWIND)
	  || exception_class != GDC_Exception_Class)
	throw_type = null;
      else
	throw_type = xh.obj.classinfo;

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
	  else if (ar_filter > 0)
	    {
	      // Positive filter values are handlers.
		catch_type = get_classinfo_entry(& info, ar_filter);
		//%%//catch_type = get_ttype_entry (&info, ar_filter);

	      // Null catch type is a catch-all handler; we can catch foreign
	      // exceptions with this.  Otherwise we must match types.
		// D Note: will be performing dynamic cast twice, potentially
		// Once here and once at the landing pad .. unless we cached
		// here and had a begin_catch call.
	      if (! catch_type
		  || (throw_type
		      && _d_isbaseof( throw_type, catch_type )))
		{
		  saw_handler = true;
		  break;
		}
	    }
	  else
	    {
		// D Note: we don't have these...
		break;
		/*
	      // Negative filter values are exception specifications.
	      // ??? How do foreign exceptions fit in?  As far as I can
	      // see we can't match because there's no __cxa_exception
	      // object to stuff bits in for __cxa_call_unexpected to use.
	      // Allow them iff the exception spec is non-empty.  I.e.
	      // a throw() specification results in __unexpected.
	      if (throw_type
		  ? ! check_exception_spec (&info, throw_type, thrown_ptr,
					    ar_filter)
		  : empty_exception_spec (&info, ar_filter))
		{
		  saw_handler = true;
		  break;
		  }
		*/
	    }

	  if (ar_disp == 0)
	    break;
	  action_record = p + ar_disp;
	}

      if (saw_handler)
	{
	  handler_switch_value = ar_filter;
	  found_type = Found.handler;
	}
      else
	found_type = (saw_cleanup ? Found.cleanup : Found.nothing);
    }

 do_something:
   if (found_type == Found.nothing)
     return _URC_CONTINUE_UNWIND;

  if (actions & _UA_SEARCH_PHASE)
    {
      if (found_type == Found.cleanup)
	return _URC_CONTINUE_UNWIND;

      // For domestic exceptions, we cache data from phase 1 for phase 2.
      if (exception_class == GDC_Exception_Class)
        {
          xh.handlerSwitchValue = handler_switch_value;
          xh.actionRecord = action_record;
          xh.languageSpecificData = language_specific_data;

	  // D Note: changed this to 'landingPad'; comment doesn't really apply
          // ??? Completely unknown what this field is supposed to be for.
          // ??? Need to cache TType encoding base for call_unexpected.
          xh.landingPad = landing_pad;
	}
      return _URC_HANDLER_FOUND;
    }
    
 install_context:
    if ( (actions & _UA_FORCE_UNWIND)
	|| exception_class != GDC_Exception_Class) {

	if (found_type == Found.terminate) {
	    dehTerminate();
	} else if (handler_switch_value < 0) {
	    dehUnexpected();
	}
	
    } else {
      if (found_type == Found.terminate)
	{
	  dehBeginCatch (&xh.unwindHeader);
	  dehTerminate(); // __terminate (xh.terminateHandler);
	}

      // Cache the TType base value for __cxa_call_unexpected, as we won't
      // have an _Unwind_Context then.
      if (handler_switch_value < 0)
	{
	  parse_lsda_header (context, language_specific_data, &info);
	  xh.catchTemp = base_of_encoded_value (info.ttype_encoding, context);
	}
    }

    // going to need a builtins module
    // ..handle in toSymbol
    /*
    _Unwind_SetGR (context, eh_return_data_regno (0),
	(_Unwind_Ptr) &xh.unwindHeader);
    _Unwind_SetGR (context, eh_return_data_regno (1),
	handler_switch_value);
    */
    //int header_reg, switch_reg;
    //__deh_get_return_data_regnos(& header_reg, & switch_reg);
    //_Unwind_SetGR (context, header_reg,	(_Unwind_Ptr) &xh.unwindHeader);
    //    _Unwind_SetGR (context, switch_reg, handler_switch_value);
    _Unwind_SetGR (context, __builtin_eh_return_data_regno (0),
	cast(_Unwind_Ptr) &xh.unwindHeader);
    _Unwind_SetGR (context, __builtin_eh_return_data_regno (1),
	handler_switch_value);
    _Unwind_SetIP (context, landing_pad);
    return _URC_INSTALL_CONTEXT;
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
};

private ubyte *
parse_lsda_header (_Unwind_Context *context, ubyte *p,
		   lsda_header_info *info)
{
  _Unwind_Word tmp;
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
get_classinfo_entry (lsda_header_info *info, _Unwind_Word i)
{
  _Unwind_Ptr ptr;

  i *= size_of_encoded_value (info.ttype_encoding);
  read_encoded_value_with_base (info.ttype_encoding, info.ttype_base,
				info.TType - i, &ptr);

  return cast(ClassInfo)cast(void *)(ptr);
}
