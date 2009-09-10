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

module std.c.dirent;

private import std.string;
private import gcc.config;

extern(C) {

struct dirent {
    byte[gcc.config.dirent_d_name_offset] opaque1;
    char[gcc.config.dirent_d_name_size] d_name;
    byte[gcc.config.dirent_remaining_size] opaque2;
}

struct DIR {
    byte[gcc.config.DIR_struct_size] opaque;
}

DIR * opendir(char *);
dirent * readdir(DIR *);
void rewinddir(DIR *);
int closedir(DIR *);
Coff_t telldir(DIR* dir);
void seekdir(DIR* dir, Coff_t offset);

}

char[] readdirD(DIR * dir)
{
    dirent* ent = readdir(dir);
    if (ent)
        return toString(ent.d_name.ptr);
    else
        return null;
}
