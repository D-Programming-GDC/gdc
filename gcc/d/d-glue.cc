// d-glue.cc -- D frontend for GCC.
// Copyright (C) 2013-2015 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/mars.h"
#include "dfrontend/errors.h"
#include "dfrontend/module.h"
#include "dfrontend/scope.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"
#include "dfrontend/statement.h"

#include "tree.h"
#include "options.h"
#include "fold-const.h"
#include "diagnostic.h"

#include "d-tree.h"
#include "d-objfile.h"
#include "d-codegen.h"

Global global;

void
Global::init()
{
  this->mars_ext = "d";
  this->hdr_ext  = "di";
  this->doc_ext  = "html";
  this->ddoc_ext = "ddoc";
  this->json_ext = "json";
  this->map_ext  = "map";

  this->obj_ext = "o";
  this->lib_ext = "a";
  this->dll_ext = "so";

  this->run_noext = true;
  this->version = "v"
#include "verstr.h"
    ;

  this->compiler.vendor = "GNU D";
  this->stdmsg = stdout;
  this->main_d = "__main.d";

  this->errorLimit = 20;

  memset(&this->params, 0, sizeof(Param));
}

unsigned
Global::startGagging()
{
  this->gag++;
  return this->gaggedErrors;
}

bool
Global::endGagging(unsigned oldGagged)
{
  bool anyErrs = (this->gaggedErrors != oldGagged);
  this->gag--;

  // Restore the original state of gagged errors; set total errors
  // to be original errors + new ungagged errors.
  this->errors -= (this->gaggedErrors - oldGagged);
  this->gaggedErrors = oldGagged;

  return anyErrs;
}

void
Global::increaseErrorCount()
{
  if (gag)
    this->gaggedErrors++;

  this->errors++;
}

char *
Loc::toChars()
{
  OutBuffer buf;

  if (this->filename)
    buf.printf("%s", this->filename);

  if (this->linnum)
    {
      buf.printf(":%u", this->linnum);
      if (this->charnum)
	buf.printf(":%u", this->charnum);
    }

  return buf.extractString();
}

Loc::Loc(const char *filename, unsigned linnum, unsigned charnum)
{
  this->linnum = linnum;
  this->charnum = charnum;
  this->filename = filename;
}

bool
Loc::equals(const Loc& loc)
{
  if (this->linnum != loc.linnum || this->charnum != loc.charnum)
    return false;

  if (!FileName::equals(this->filename, loc.filename))
    return false;

  return true;
}


// Print a hard error message.

void
error(Loc loc, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  verror(loc, format, ap);
  va_end(ap);
}

void
verror(Loc loc, const char *format, va_list ap,
       const char *p1, const char *p2, const char *)
{
  if (!global.gag)
    {
      location_t location = get_linemap(loc);
      char *msg;

      // Build string and emit.
      if (vasprintf(&msg, format, ap) >= 0 && msg != NULL)
	{
	  if (p2)
	    msg = concat(p2, " ", msg, NULL);

	  if (p1)
	    msg = concat(p1, " ", msg, NULL);

	  error_at(location, "%s", msg);
	}

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
errorSupplemental(Loc loc, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  verrorSupplemental(loc, format, ap);
  va_end(ap);
}

void
verrorSupplemental(Loc loc, const char *format, va_list ap)
{
  if (!global.gag)
    {
      location_t location = get_linemap(loc);
      char *msg;

      if (vasprintf(&msg, format, ap) >= 0 && msg != NULL)
	inform(location, "%s", msg);
    }
}

// Print a warning message.

void
warning(Loc loc, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vwarning(loc, format, ap);
  va_end(ap);
}

void
vwarning(Loc loc, const char *format, va_list ap)
{
  if (global.params.warnings && !global.gag)
    {
      location_t location = get_linemap(loc);
      char *msg;

      if (vasprintf(&msg, format, ap) >= 0 && msg != NULL)
	warning_at(location, 0, "%s", msg);

      // Warnings don't count if gagged.
      if (global.params.warnings == 1)
	global.warnings++;
    }
}

// Print a deprecation message.

void
deprecation(Loc loc, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vdeprecation(loc, format, ap);
  va_end(ap);
}

void
vdeprecation(Loc loc, const char *format, va_list ap,
	      const char *p1, const char *p2)
{
  if (global.params.useDeprecated == 0)
    verror(loc, format, ap, p1, p2);
  else if (global.params.useDeprecated == 2 && !global.gag)
    {
      location_t location = get_linemap(loc);
      char *msg;

      // Build string and emit.
      if (p2)
	format = concat(p2, " ", format, NULL);

      if (p1)
	format = concat(p1, " ", format, NULL);

      if (vasprintf(&msg, format, ap) >= 0 && msg != NULL)
	warning_at(location, OPT_Wdeprecated, "%s", msg);
    }
}

// Call this after printing out fatal error messages to clean up and exit
// the compiler.

void
fatal()
{
  exit(FATAL_EXIT_CODE);
}


void
escapePath(OutBuffer *buf, const char *fname)
{
  while (1)
    {
      switch (*fname)
	{
	case 0:
	  return;

	case '(':
	case ')':
	case '\\':
	  buf->writeByte('\\');

	default:
	  buf->writeByte(*fname);
	  break;
	}
      fname++;
    }
}

void
readFile(Loc loc, File *f)
{
  if (f->read())
    {
      error(loc, "Error reading file '%s'", f->name->toChars());
      fatal();
    }
}

void
writeFile(Loc loc, File *f)
{
  if (f->write())
    {
      error(loc, "Error writing file '%s'", f->name->toChars());
      fatal();
    }
}

void
ensurePathToNameExists(Loc loc, const char *name)
{
  const char *pt = FileName::path(name);
  if (*pt)
    {
      if (FileName::ensurePathExists(pt))
	{
	  error(loc, "cannot create directory %s", pt);
	  fatal();
	}
    }
}

// Semantically analyze AsmStatement where SC is the scope.

Statement *
asmSemantic(AsmStatement *s, Scope *sc)
{
  sc->func->hasReturnExp |= 8;
  return s;
}

// Determine return style of function - whether in registers or through a
// hidden pointer to the caller's stack.

RET
retStyle(TypeFunction *)
{
  // Need the backend type to determine this, but this is called from the
  // front end before semantic processing is finished.  An accurate value
  // is not currently needed anyway.
  return RETstack;
}

// Determine if function is a builtin one that we can evaluate at compile time.

BUILTIN
isBuiltin(FuncDeclaration *fd)
{
  if (fd->builtin != BUILTINunknown)
    return fd->builtin;

  fd->builtin = BUILTINno;

  // Intrinsics have no function body.
  if (fd->fbody)
    return BUILTINno;

  maybe_set_intrinsic(fd);
  return fd->builtin;
}

// Evaluate builtin D function FD whose argument list is ARGUMENTS.
// Return result; NULL if cannot evaluate it.

Expression *
eval_builtin(Loc loc, FuncDeclaration *fd, Expressions *arguments)
{
  if (fd->builtin != BUILTINyes)
    return NULL;

  tree decl = get_symbol_decl (fd);
  gcc_assert(DECL_BUILT_IN(decl));

  TypeFunction *tf = (TypeFunction *) fd->type;
  Expression *e = NULL;
  set_input_location(loc);

  tree result = d_build_call(tf, decl, NULL, arguments);
  result = fold(result);

  // Builtin should be successfully evaluated.
  // Will only return NULL if we can't convert it.
  if (TREE_CONSTANT(result) && TREE_CODE(result) != CALL_EXPR)
    e = build_expression(result);

  return e;
}

