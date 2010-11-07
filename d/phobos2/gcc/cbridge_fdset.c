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

#include <sys/types.h>

#ifndef FD_SET
#include <sys/select.h>
#endif

void _d_gnu_fd_set(int n, fd_set * p)
{
    FD_SET(n, p);
}

void _d_gnu_fd_clr(int n, fd_set * p)
{
    FD_CLR(n, p);
}

int  _d_gnu_fd_isset(int n, fd_set * p)
{
    return FD_ISSET(n, p);
}

void _d_gnu_fd_copy(fd_set * f, fd_set * t)
{
    FD_COPY(f, t);
}

void _d_gnu_fd_zero(fd_set * p)
{
    FD_ZERO(p);
}

