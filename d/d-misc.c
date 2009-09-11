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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "except.h"
#include "tree.h"

#include "d-lang.h"


/* The value of USING_SJLJ_EXCEPTIONS relies on insn-flags.h, but insn-flags.h
   cannot be compiled by C++. */

int
d_using_sjlj_exceptions(void)
{
    return USING_SJLJ_EXCEPTIONS;
}

/* ASM_FORMAT_PRIVATE_NAME implicitly casts to char* from void*
   (from an alloca call).  This is fixed in GCC 4.x */

char *
d_asm_format_private_name(const char * name, int value)
{
    char * out_name;
    ASM_FORMAT_PRIVATE_NAME(out_name, name, value);
    // and, since it's alloca...
    return xstrdup(out_name);
}
