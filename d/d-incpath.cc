/* GDC -- D front-end for GCC
   Based on the patch by heromyth who based in off 'incpath.c'.

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

#include "d-gcc-includes.h"
#include "options.h"
#include "cppdefault.h"

#include "d-lang.h"
#include "d-codegen.h"
#include "d-confdefs.h"

/* Global options removed from d-lang.cc */
bool std_inc;
const char * iprefix = NULL;
const char * multilib_dir = NULL;

static void add_env_var_paths (const char *, bool verbose=false);
static void add_import_path (const char *, bool verbose=false);
static void add_standard_paths (bool verbose=false);
static void free_path (char *path, int reason, bool verbose=false);
static char* make_absolute(const char *path, bool verbose = false);

enum { REASON_QUIET = 0, REASON_NOENT, REASON_DUP, REASON_DUP_SYS };


/* Free an element of the include chain, possibly giving a reason.  */
static void
free_path (char *path, int reason, bool verbose)
{
  switch (reason)
    {
    case REASON_DUP:
    case REASON_DUP_SYS:
        if(verbose)
        {
            fprintf (stderr, "ignoring duplicate directory \"%s\"\n", path);
            if (reason == REASON_DUP_SYS)
                fprintf (stderr, 
                    "  as it is a non-system directory that duplicates a system directory\n");
        }
      break;

    case REASON_NOENT:
//        if(verbose)
          fprintf (stderr, "ignoring nonexistent directory \"%s\"\n",
               path);
      break;

    case REASON_QUIET:
    default:
      break;
    }

  free (path);
}

/* Read ENV_VAR for a PATH_SEPARATOR-separated list of file names; and
   append all the names to the search path CHAIN.  */
static void
add_env_var_paths (const char *env_var, bool verbose)
{
    char *p, *q, *path;

    GET_ENVIRONMENT (q, env_var);

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

        add_import_path (path, verbose);
    }
}

static char *
prefixed_path(const char * path)
{
    // based on incpath.c
    size_t len = cpp_GCC_INCLUDE_DIR_len;
    if (iprefix && len != 0 && ! strncmp(path, cpp_GCC_INCLUDE_DIR, len))
        return concat(iprefix, path + len, NULL);
    // else
    return xstrdup(path);
}


/* Append the standard import chain defined in d-confdefs.h  */
static void add_standard_paths(bool verbose)
{
    add_import_path(D_PHOBOS_DIR, verbose);
    
    char * target_dir = xstrdup(D_PHOBOS_TARGET_DIR);

    if (multilib_dir)
        target_dir = reconcat(target_dir, target_dir, "/", multilib_dir, NULL);
    
    add_import_path(target_dir, verbose);
    free(target_dir);
}


static bool check_duplicates(const char *path)
{
    if(!global.path)
        return false;

    char *addedPath;

    for (unsigned i = 0; i < global.path->dim; i++)
    {
        addedPath = (char *)global.path->data[i];
        if(!FileName::compare(addedPath, path))
            return true;
    }

    return false;
}


/*
reorganize 
    /mingw/lib/gcc/i686-pc-mingw32/4.5.2/../../../../include/d2/4.5.2
to
    /mingw/include/d2/4.5.2

return null, if the path can't be handled;

char* str = "/abc/../gcc-4.5.2/libphobos";        // ok
char* str = "/abc/../../gcc-4.5.2/libphobos";     // error
char* str = "/../gcc-4.5.2/libphobos";            // error
char* str = "D:/../gcc-4.5.2/libphobos";          // error
char* str = "D:/D/../gcc-4.5.2/libphobos";        // ok
char* str = "A/../gcc-4.5.2/libphobos";           // ok
char* str = "aa/bb/cc/../../gcc-4.5.2/libphobos"; // ok
*/
static char* make_absolute(const char *path, bool verbose)
{
    size_t pathLength = strlen(path);
    char *result = NULL;

    if(pathLength<3)
    {
        result = xstrdup(path);
        return result;
    }

    char pathBuffer[PATH_MAX];
    char splitters[PATH_MAX];

    size_t i=0;
    size_t splliterCount = 0;
    size_t bufferLength = 0;

    while( i<pathLength)
    {
        pathBuffer[bufferLength] = path[i];

        if(IS_DIR_SEPARATOR(path[i]))
        {
            // convert '\\' to '/'
            pathBuffer[bufferLength] = '/';
            splitters[splliterCount] = i;
            splliterCount++;
        }
        else if(i<pathLength-1 && path[i] == '.' && path[i+1] == '.')
        {
            if(splliterCount == 0 )
            {
                if(verbose)
                    fprintf(stderr, "It's a relative path: %s\n", path);
                return NULL;
            }

            if( i==pathLength-2 || (i<pathLength-2 && IS_DIR_SEPARATOR(path[i+2])))
            {
                splliterCount--;
                if(splliterCount == 0)
                {
                    if(IS_DIR_SEPARATOR(pathBuffer[0]) || (pathBuffer[1] == ':' && IS_DIR_SEPARATOR(pathBuffer[2])))
                    {
//                        if(verbose)
                            fprintf(stderr, "Warnning: can't handle path: %s\n", path);
                        return NULL;
                    }

                    // backup to the root
                    bufferLength = -1;
                }
                else
                {
                    // backup a level
                    bufferLength = splitters[splliterCount-1];
                }

                // skip ../
                i += 2;
            }
        }

        bufferLength++;
        i++;
    }

    pathBuffer[bufferLength] = '\0';
    result = xstrdup(pathBuffer);

    return result;
}

/* Add PATH to the include chain CHAIN. PATH must be malloc-ed and
   NUL-terminated.  */
static void add_import_path (const char *path, bool verbose)
{
    char *backup_dir = NULL;
    char *target_dir = prefixed_path(path);


#if defined (HAVE_DOS_BASED_FILE_SYSTEM)
  /* Remove unnecessary trailing slashes.  On some versions of MS
     Windows, trailing  _forward_ slashes cause no problems for stat().
     On newer versions, stat() does not recognize a directory that ends
     in a '\\' or '/', unless it is a drive root dir, such as "c:/",
     where it is obligatory.  */
    int pathlen = strlen (target_dir);
    char* end = target_dir + pathlen - 1;
    /* Preserve the lead '/' or lead "c:/".  */
    char* start = target_dir + (pathlen > 2 && target_dir[1] == ':' ? 3 : 1);

    for (; end > start && IS_DIR_SEPARATOR (*end); end--)
    *end = 0;
#endif

    if (!global.path)
        global.path = new Array();

    backup_dir = target_dir;
    target_dir = make_absolute(backup_dir, verbose);
    if(target_dir)
        free_path(backup_dir, REASON_QUIET, verbose);
    else
        target_dir = backup_dir;        

    if(!FileName::exists(target_dir))
    {
        free_path(target_dir, REASON_NOENT, verbose);
        return;
    }


    if(check_duplicates(target_dir))
        free_path(target_dir, REASON_DUP, verbose);
    else
        global.path->push(target_dir);        
}



void register_import_chains()
{
    static const char *const lang_env_vars[] =
        { "D_IMPORT_PATH" };

    size_t idx = 0;
    bool verbose = global.params.verbose;

    // %%TODO: front or back?
    if (std_inc)
        add_standard_paths(verbose);

    /* Language-dependent environment variables may add to the include chain. */
    add_env_var_paths (lang_env_vars[idx]);

    if (global.params.imppath)
    {
        for (unsigned i = 0; i < global.params.imppath->dim; i++)
        {
            char *path = (char *)global.params.imppath->data[i];
            // We would do this for D_INCLUDE_PATH env var, but not for '-I'
            // command line args.
            //Array *a = FileName::splitPath(path);

            if (path)
                add_import_path(path, verbose);
        }
    }

    if (global.params.fileImppath)
    {
        for (unsigned i = 0; i < global.params.fileImppath->dim; i++)
        {
            char *path = (char *)global.params.fileImppath->data[i];
            if (path)
                add_import_path(path, verbose);
        }
    }
}
