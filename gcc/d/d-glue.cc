// d-glue.cc -- D frontend for GCC.
// Copyright (C) 2013 Free Software Foundation, Inc.

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

#include "d-system.h"
#include "d-objfile.h"

#include "mars.h"
#include "module.h"

Global global;

void
Global::init (void)
{
  this->mars_ext = "d";
  this->sym_ext  = "d";
  this->hdr_ext  = "di";
  this->doc_ext  = "html";
  this->ddoc_ext = "ddoc";
  this->json_ext = "json";
  this->map_ext  = "map";

  this->obj_ext = "o";
  this->lib_ext = "a";
  this->dll_ext = "so";

  this->version = "v"
#include "verstr.h"
    ;

  this->compiler.vendor = "GNU D";
  this->main_d = "__main.d";

  memset (&this->params, 0, sizeof (Param));
}

unsigned
Global::startGagging (void)
{
  this->gag++;
  return this->gaggedErrors;
}

bool
Global::endGagging (unsigned oldGagged)
{
  bool anyErrs = (this->gaggedErrors != oldGagged);
  this->gag--;

  // Restore the original state of gagged errors; set total errors
  // to be original errors + new ungagged errors.
  this->errors -= (this->gaggedErrors - oldGagged);
  this->gaggedErrors = oldGagged;

  return anyErrs;
}

bool
Global::isSpeculativeGagging (void)
{
  if (!this->gag)
    return false;

  if (this->gag != this->speculativeGag)
    return false;

  return true;
}

char *
Loc::toChars (void)
{
  OutBuffer buf;

  if (this->filename)
    buf.printf ("%s", this->filename);

  if (this->linnum)
    buf.printf (":%u", this->linnum);

  buf.writeByte (0);

  return (char *)buf.extractData();
}

Loc::Loc (Module *mod, unsigned linnum)
{
  this->linnum = linnum;
  this->filename = mod ? mod->srcfile->toChars() : NULL;
}

bool
Loc::equals (const Loc& loc)
{
  if (this->linnum != loc.linnum)
    return false;

  if (!FileName::equals (this->filename, loc.filename))
    return false;

  return true;
}


// Print a hard error message.

void
error (Loc loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  verror (loc, format, ap);
  va_end (ap);
}

void
error (const char *filename, unsigned linnum, const char *format, ...)
{
  Loc loc;
  va_list ap;

  loc.filename = CONST_CAST (char *, filename);
  loc.linnum = linnum;

  va_start (ap, format);
  verror (loc, format, ap);
  va_end (ap);
}

void
verror (Loc loc, const char *format, va_list ap,
	const char *p1, const char *p2, const char *)
{
  if (!global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      // Build string and emit.
      if (p2)
	format = concat (p2, " ", format, NULL);

      if (p1)
	format = concat (p1, " ", format, NULL);

      vasprintf (&msg, format, ap);
      error_at (location, msg);

      // Moderate blizzard of cascading messages
      if (global.errors >= 20)
	fatal();
    }
  else
    global.gaggedErrors++;

  global.errors++;
}

// Print supplementary message about the last error.
// Doesn't increase error count.

void
errorSupplemental (Loc loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  verrorSupplemental (loc, format, ap);
  va_end (ap);
}

void
verrorSupplemental (Loc loc, const char *format, va_list ap)
{
  if (!global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      vasprintf (&msg, format, ap);
      inform (location, msg);
    }
}

// Print a warning message.

void
warning (Loc loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vwarning (loc, format, ap);
  va_end (ap);
}

void
vwarning (Loc loc, const char *format, va_list ap)
{
  if (global.params.warnings && !global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      vasprintf (&msg, format, ap);
      warning_at (location, 0, msg);

      // Warnings don't count if gagged.
      if (global.params.warnings == 1)
	global.warnings++;
    }
}

// Print a deprecation message.

void
deprecation (Loc loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vdeprecation (loc, format, ap);
  va_end (ap);
}

void
vdeprecation (Loc loc, const char *format, va_list ap,
	      const char *p1, const char *p2)
{
  if (global.params.useDeprecated == 0)
    verror (loc, format, ap, p1, p2);
  else if (global.params.useDeprecated == 2 && !global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      // Build string and emit.
      if (p2)
	format = concat (p2, " ", format, NULL);

      if (p1)
	format = concat (p1, " ", format, NULL);

      vasprintf (&msg, format, ap);
      warning_at (location, OPT_Wdeprecated, msg);
    }
}

// Call this after printing out fatal error messages to clean up and exit
// the compiler.

void
fatal (void)
{
  exit (FATAL_EXIT_CODE);
}

