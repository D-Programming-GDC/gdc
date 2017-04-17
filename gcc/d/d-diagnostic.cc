/* d-diagnostics.cc -- D frontend interface to gcc diagnostics.
   Copyright (C) 2017 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/globals.h"
#include "dfrontend/errors.h"

#include "tree.h"
#include "options.h"
#include "diagnostic.h"

#include "d-tree.h"
#include "d-objfile.h"


/* Print a hard error message.  */

void
error (const Loc& loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  verror (loc, format, ap);
  va_end (ap);
}

void
verror (const Loc& loc, const char *format, va_list ap,
       const char *p1, const char *p2, const char *)
{
  if (!global.gag || global.params.showGaggedErrors)
    {
      location_t location = get_linemap (loc);
      char *msg;

      /* Build string and emit.  */
      if (vasprintf (&msg, format, ap) >= 0 && msg != NULL)
	{
	  if (p2)
	    msg = concat (p2, " ", msg, NULL);

	  if (p1)
	    msg = concat (p1, " ", msg, NULL);

	  if (global.gag)
	    emit_diagnostic (DK_ANACHRONISM, location, 0,
			    "(spec:%d) %s", global.gag, msg);
	  else
	    error_at (location, "%s", msg);
	}
    }

  if (global.gag)
    global.gaggedErrors++;

  global.errors++;
}

/* Print supplementary message about the last error.
   Doesn't increase error count.  */

void
errorSupplemental (const Loc& loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  verrorSupplemental (loc, format, ap);
  va_end (ap);
}

void
verrorSupplemental (const Loc& loc, const char *format, va_list ap)
{
  if (global.gag && !global.params.showGaggedErrors)
    return;

  location_t location = get_linemap (loc);
  char *msg;

  if (vasprintf (&msg, format, ap) >= 0 && msg != NULL)
    inform (location, "%s", msg);
}

/* Print a warning message.  */

void
warning (const Loc& loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vwarning (loc, format, ap);
  va_end (ap);
}

void
vwarning (const Loc& loc, const char *format, va_list ap)
{
  if (global.params.warnings && !global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      if (vasprintf (&msg, format, ap) >= 0 && msg != NULL)
	warning_at (location, 0, "%s", msg);

      /* Warnings don't count if gagged.  */
      if (global.params.warnings == 1)
	global.warnings++;
    }
}

/* Print supplementary message about the last warning.
   Doesn't increase warning count.  */

void
warningSupplemental (const Loc& loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vwarningSupplemental (loc, format, ap);
  va_end (ap);
}

void
vwarningSupplemental (const Loc& loc, const char *format, va_list ap)
{
  if (global.params.warnings && !global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      if (vasprintf (&msg, format, ap) >= 0 && msg != NULL)
	inform (location, "%s", msg);
    }
}

/* Print a deprecation message.  */

void
deprecation (const Loc& loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vdeprecation (loc, format, ap);
  va_end (ap);
}

void
vdeprecation (const Loc& loc, const char *format, va_list ap,
	      const char *p1, const char *p2)
{
  if (global.params.useDeprecated == 0)
    verror (loc, format, ap, p1, p2);
  else if (global.params.useDeprecated == 2 && !global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      /* Build string and emit.  */
      if (p2)
	format = concat (p2, " ", format, NULL);

      if (p1)
	format = concat (p1, " ", format, NULL);

      if (vasprintf (&msg, format, ap) >= 0 && msg != NULL)
	warning_at (location, OPT_Wdeprecated, "%s", msg);
    }
}

/* Print supplementary message about the last deprecation.  */

void
deprecationSupplemental (const Loc& loc, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  vdeprecationSupplemental (loc, format, ap);
  va_end (ap);
}

void
vdeprecationSupplemental (const Loc& loc, const char *format, va_list ap)
{
  if (global.params.useDeprecated == 0)
    verrorSupplemental (loc, format, ap);
  else if (global.params.useDeprecated == 2 && !global.gag)
    {
      location_t location = get_linemap (loc);
      char *msg;

      if (vasprintf (&msg, format, ap) >= 0 && msg != NULL)
	inform (location, "%s", msg);
    }
}

/* Call this after printing out fatal error messages to clean up and
   exit the compiler.  */

void
fatal (void)
{
  exit (FATAL_EXIT_CODE);
}
