// d-incpath.cc -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

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

#include "d-gcc-includes.h"
#include "options.h"
#include "cppdefault.h"

#include "d-lang.h"
#include "d-codegen.h"
#include "d-confdefs.h"

#include "cond.h"

/* Global options removed from d-lang.cc */
const char *iprefix = NULL;
const char *multilib_dir = NULL;

static void add_env_var_paths (const char *);
static void add_import_path (char *);
static void add_file_path (char *);
static char *make_absolute (char *path);


/* Read ENV_VAR for a PATH_SEPARATOR-separated list of file names; and
   append all the names to the import search path.  */

static void
add_env_var_paths (const char *env_var)
{
  char *p, *q, *path;

  q = getenv (env_var);

  if (!q)
    return;

  for (p = q; *q; p = q + 1)
    {
      q = p;
      while (*q != 0 && *q != PATH_SEPARATOR)
	q++;

      if (p == q)
	path = xstrdup (".");
      else
	{
	  path = XNEWVEC (char, q - p + 1);
	  memcpy (path, p, q - p);
	  path[q - p] = '\0';
	}

      global.params.imppath->push (path);
    }
}


/* Look for directories that start with the standard prefix.
   "Translate" them, i.e. replace /usr/local/lib/gcc... with
   IPREFIX and search them first.  */

static char *
prefixed_path (const char *path)
{
  // based on incpath.c
  size_t len = cpp_GCC_INCLUDE_DIR_len;
  if (iprefix && len != 0 && !strncmp (path, cpp_GCC_INCLUDE_DIR, len))
    return concat (iprefix, path + len, NULL);
  // else
  return xstrdup (path);
}


/* Given a pointer to a string containing a pathname, returns a
   canonical version of the filename.
   */

static char *
make_absolute (char *path)
{
#if defined (HAVE_DOS_BASED_FILE_SYSTEM)
  /* Remove unnecessary trailing slashes.  On some versions of MS
     Windows, trailing  _forward_ slashes cause no problems for stat().
     On newer versions, stat() does not recognize a directory that ends
     in a '\\' or '/', unless it is a drive root dir, such as "c:/",
     where it is obligatory.  */
  int pathlen = strlen (path);
  char *end = path + pathlen - 1;
  /* Preserve the lead '/' or lead "c:/".  */
  char *start = path + (pathlen > 2 && path[1] == ':' ? 3 : 1);

  for (; end > start && IS_DIR_SEPARATOR (*end); end--)
    *end = 0;
#endif

  return lrealpath (path);
}


/* Add PATH to the global import lookup path.  */

static void
add_import_path (char *path)
{
  char *target_dir = make_absolute (path);

  if (!global.path)
    global.path = new Strings();

  if (!FileName::exists (target_dir))
    {
      free (target_dir);
      return;
    }

  global.path->push (target_dir);
}


/* Add PATH to the global file lookup path.  */

static void
add_file_path (char *path)
{
  char *target_dir = make_absolute (path);

  if (!global.filePath)
    global.filePath = new Strings();

  if (!FileName::exists (target_dir))
    {
      free (target_dir);
      return;
    }

  global.filePath->push (target_dir);
}


void
add_import_paths (bool stdinc)
{
  // %%TODO: front or back?
  if (stdinc)
    {
      char *phobos_dir = prefixed_path (D_PHOBOS_DIR);
      char *target_dir = prefixed_path (D_PHOBOS_TARGET_DIR);

      if (multilib_dir)
	target_dir = concat (target_dir, "/", multilib_dir, NULL);

      global.params.imppath->shift (phobos_dir);
      global.params.imppath->shift (target_dir);
    }

  /* Language-dependent environment variables may add to the include chain. */
  add_env_var_paths ("D_IMPORT_PATH");

  if (global.params.imppath)
    {
      for (size_t i = 0; i < global.params.imppath->dim; i++)
	{
	  char *path = global.params.imppath->tdata()[i];
	  if (path)
	    add_import_path (path);
	}
    }

  if (global.params.fileImppath)
    {
      for (size_t i = 0; i < global.params.fileImppath->dim; i++)
	{
	  char *path = global.params.fileImppath->tdata()[i];
	  if (path)
	    add_file_path (path);
	}
    }
}


void add_phobos_versyms (void)
{
  char *path = FileName::searchPath (global.path, "phobos-ver-syms", 1);
  if (path)
    {
      FILE *f = fopen (path, "r");
      if (f)
	{
	  char buf[256];
	  while (!feof (f) && fgets (buf, 256, f))
	    {
	      char *p = buf;
	      while (*p && ISSPACE (*p))
		p++;
	      char *q = p;
	      while (*q && !ISSPACE (*q))
		q++;
	      *q = 0;
	      if (p != q)
		{
		  /* Needs to be predefined because we define
		     Unix/Windows this way. */
		  VersionCondition::addPredefinedGlobalIdent (xstrdup (p));
		}
	    }
	  fclose (f);
	}
    }
}

