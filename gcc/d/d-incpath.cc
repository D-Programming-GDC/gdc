// d-incpath.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "dfrontend/init.h"
#include "dfrontend/cond.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"

#include "tree.h"
#include "diagnostic.h"
#include "options.h"
#include "cppdefault.h"

#include "d-tree.h"
#include "d-lang.h"
#include "d-codegen.h"

// Read ENV_VAR for a PATH_SEPARATOR-separated list of file names; and
// append all the names to the import search path.

static void
add_env_var_paths(const char *env_var)
{
  char *p, *q, *path;

  q = getenv(env_var);

  if (!q)
    return;

  for (p = q; *q; p = q + 1)
    {
      q = p;
      while (*q != 0 && *q != PATH_SEPARATOR)
	q++;

      if (p == q)
	path = xstrdup(".");
      else
	{
	  path = XNEWVEC(char, q - p + 1);
	  memcpy(path, p, q - p);
	  path[q - p] = '\0';
	}

      global.params.imppath->push(path);
    }
}


// Look for directories that start with the standard prefix.
// "Translate" them, i.e. replace /usr/local/lib/gcc... with
// IPREFIX and search them first.

static char *
prefixed_path(const char *path, const char *iprefix)
{
  // based on incpath.c
  size_t len;

  if (cpp_relocated() && (len = cpp_PREFIX_len) != 0)
  {
    if (!strncmp(path, cpp_PREFIX, len))
      {
	static const char *relocated_prefix;
	/* If this path starts with the configure-time prefix,
	   but the compiler has been relocated, replace it
	   with the run-time prefix.  The run-time exec prefix
	   is GCC_EXEC_PREFIX.  Compute the path from there back
	   to the toplevel prefix.  */
	if (!relocated_prefix)
	  {
	    char *dummy;
	    /* Make relative prefix expects the first argument
	       to be a program, not a directory.  */
	    dummy = concat(gcc_exec_prefix, "dummy", NULL);
	    relocated_prefix
	      = make_relative_prefix(dummy,
				     cpp_EXEC_PREFIX,
				     cpp_PREFIX);
	    free(dummy);
	  }
	return concat(relocated_prefix, path + len, NULL);
      }
  }

  if (iprefix && (len = cpp_GCC_INCLUDE_DIR_len) != 0)
    {
      if (!strncmp(path, cpp_GCC_INCLUDE_DIR, len))
	return concat(iprefix, path + len, NULL);
    }

  return xstrdup(path);
}


// Given a pointer to a string containing a pathname, returns a
// canonical version of the filename.

static char *
make_absolute(char *path)
{
#if defined (HAVE_DOS_BASED_FILE_SYSTEM)
  /* Remove unnecessary trailing slashes.  On some versions of MS
     Windows, trailing  _forward_ slashes cause no problems for stat().
     On newer versions, stat() does not recognize a directory that ends
     in a '\\' or '/', unless it is a drive root dir, such as "c:/",
     where it is obligatory.  */
  int pathlen = strlen(path);
  char *end = path + pathlen - 1;
  /* Preserve the lead '/' or lead "c:/".  */
  char *start = path + (pathlen > 2 && path[1] == ':' ? 3 : 1);

  for (; end > start && IS_DIR_SEPARATOR(*end); end--)
    *end = 0;
#endif

  return lrealpath(path);
}

/* Add PATHS to the global import lookup path.  */

static void
add_import_path(Strings *paths)
{
  if (paths)
    {
      if (!global.path)
	global.path = new Strings();

      for (size_t i = 0; i < paths->dim; i++)
	{
	  const char *path = (*paths)[i];
	  char *target_dir = make_absolute(CONST_CAST(char *, path));

	  if (!FileName::exists(target_dir))
	    {
	      free(target_dir);
	      continue;
	    }

	  global.path->push(target_dir);
	}
    }
}

/* Add PATHS to the global file import lookup path.  */

static void
add_fileimp_path(Strings *paths)
{
  if (paths)
    {
      if (!global.filePath)
	global.filePath = new Strings();

      for (size_t i = 0; i < paths->dim; i++)
	{
	  const char *path = (*paths)[i];
	  char *target_dir = make_absolute(CONST_CAST(char *, path));

	  if (!FileName::exists(target_dir))
	    {
	      free(target_dir);
	      continue;
	    }

	  global.filePath->push(target_dir);
	}
    }
}


// Add all search directories to compiler runtime.
// if STDINC, also include standard library paths.

void
add_import_paths(const char *iprefix, const char *imultilib, bool stdinc)
{
  if (stdinc)
    {
      for (const default_include *p = cpp_include_defaults; p->fname; p++)
	{
	  char *import_path;

	  // Ignore C++
	  if (p->cplusplus)
	    continue;

	  if (!p->add_sysroot)
	    import_path = prefixed_path(p->fname, iprefix);
	  else
	    import_path = xstrdup(p->fname);

	  // Add D-specific suffix.
	  import_path = concat(import_path, "/d", NULL);

	  // Ignore duplicate entries.
	  bool found = false;
	  for (size_t i = 0; i < global.params.imppath->dim; i++)
	    {
	      if (strcmp(import_path, (*global.params.imppath)[i]) == 0)
		{
		  found = true;
		  break;
		}
	    }

	  if (found)
	    {
	      free(import_path);
	      continue;
	    }

	  // Multilib support.
	  if (imultilib)
	    {
	      char *target_path = concat(import_path, "/", imultilib, NULL);
	      global.params.imppath->shift(target_path);
	    }

	  global.params.imppath->shift(import_path);
	}
    }

  // Language-dependent environment variables may add to the include chain.
  add_env_var_paths("D_IMPORT_PATH");

  // Add import search paths
  if (global.params.imppath)
    {
      for (size_t i = 0; i < global.params.imppath->dim; i++)
	{
	  const char *path = (*global.params.imppath)[i];
	  if (path)
	    add_import_path(FileName::splitPath(path));
	}
    }

  // Add string import search paths
  if (global.params.fileImppath)
    {
      for (size_t i = 0; i < global.params.fileImppath->dim; i++)
	{
	  const char *path = (*global.params.fileImppath)[i];
	  if (path)
	    add_fileimp_path(FileName::splitPath(path));
	}
    }
}

