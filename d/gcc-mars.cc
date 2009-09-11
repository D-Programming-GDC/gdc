/* Pieces of mars.c needed for for GCC

   Original copyright from mars.c:

// Copyright (c) 1999-2007 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "mem.h"
#include "root.h"
#include "mars.h"
#include "module.h"
#include "mtype.h"

Global global;

Global::Global()
{
    mars_ext = "d";
    sym_ext  = "d";
    hdr_ext  = "di";
    doc_ext  = "html";
    ddoc_ext = "ddoc";

    obj_ext  = "o";

    copyright = "Copyright (c) 1999-2007 by Digital Mars";
    written = "written by Walter Bright";
    version = "v1.020";
    global.structalign = 8;

    memset(&params, 0, sizeof(Param));
}

char *Loc::toChars()
{
    OutBuffer buf;

    if (filename)
    {
	buf.printf("%s", filename);
    }

    if (linnum)
	buf.printf(":%u", linnum);
    buf.writeByte(0);
    return (char *)buf.extractData();
}

Loc::Loc(Module *mod, unsigned linnum)
{
    this->linnum = linnum;
    this->filename = mod ? mod->srcfile->toChars() : NULL;
}

/**************************************
 * Print error message and exit.
 */

void error(Loc loc, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    verror(loc, format, ap);
    va_end( ap );
}

void verror(Loc loc, const char *format, va_list ap)
{
    if (!global.gag)
    {
	char *p = loc.toChars();

	if (*p)
	    fprintf(stderr, "%s: ", p);
	mem.free(p);

	fprintf(stderr, "Error: ");
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
    }
    global.errors++;
}

/***************************************
 * Call this after printing out fatal error messages to clean up and exit
 * the compiler.
 */

void fatal()
{
#if 0
    *(char *)0 = 0;
#endif
    exit(EXIT_FAILURE);
}
