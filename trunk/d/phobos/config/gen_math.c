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

#include <stdio.h>
#include <math.h>

#include "makestruct.h"

void c_fpclassify()
{
#ifndef HAVE_FP_CLASSIFY_SIGNBIT
    // from ingernal/fpmath.d
    enum Blah
    {
	FP_NAN = 1,
	FP_INFINITE,
	FP_ZERO,
	FP_SUBNORMAL,
	FP_NORMAL
    };
#endif
    printf("enum {\n");
    CES( FP_NAN );
    CES( FP_INFINITE );
    CES( FP_ZERO );
    CES( FP_SUBNORMAL );
    CES( FP_NORMAL );
    printf("}\n\n");
}

int main()
{
    printf("extern(C) {\n\n");
    c_fpclassify();
    printf("}\n\n");
    return 0;
}

