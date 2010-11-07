/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, (C) 2009
    Iain Buclaw, (C) 2010

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

#include <assert.h>
#include "dt.h"

dt_t**
dtval(dt_t** pdt, DT t, dinteger_t i, const void * p)
{
    dt_t * d = new dt_t;
    d->dt = t;
    d->DTnext = 0;
    d->DTint = i;
    d->DTpointer = (void*)p;
    return dtcat(pdt, d);
}

dt_t**
dtcat(dt_t** pdt, dt_t * d)
{
    assert(d);
    // wasted time and mem touching... shortcut DTend field?
    while (*pdt)
        pdt = & (*pdt)->DTnext;
    *pdt = d;
    return & d->DTnext;
}

typedef unsigned bitunit_t;

dt_t**
dtnbits(dt_t** pdt, size_t count, char * pbytes, unsigned unit_size)
{
    assert(unit_size == sizeof(bitunit_t));
    assert(count % unit_size == 0);

    bitunit_t * p_unit = (bitunit_t *) pbytes,
        * p_unit_end = (bitunit_t *) (pbytes + count);
    char * pbits = new char[count];
    char * p_out = pbits;
    unsigned b = 0;
    char outv = 0;

    while (p_unit < p_unit_end) {
        bitunit_t inv = *p_unit++;

        for (unsigned i = 0; i < sizeof(bitunit_t)*8; i++) {
            outv |= ((inv >> i) & 1) << b;
            if (++b == 8) {
                *p_out++ = outv;
                b = 0;
                outv = 0;
            }
        }
    }
    assert( (unsigned)(p_out - pbits) == count);

    return dtnbytes(pdt, count, pbits);
}

