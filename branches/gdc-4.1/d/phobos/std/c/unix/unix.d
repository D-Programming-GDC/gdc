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

module std.c.unix.unix;

/* This module imports the unix module for the currect
   target system.  Currently, all targets can be
   handled with the autoconf'd version. */

public import gcc.configunix;

// DMD linux.d has dirent.h declarations
public import std.c.dirent;
int dirfd(DIR*);

public import std.c.stdio;
int fseeko(FILE*, off_t, int);
off_t ftello(FILE*);
