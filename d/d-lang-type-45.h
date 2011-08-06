/* GDC -- D front-end for GCC
   Copyright (C) 2011 Iain Buclaw

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

/* Split from d-lang-45.h for new GTY signature in GCC-4.6.  */

/* The lang_type field is not set for every GCC type. */
struct Type;
typedef struct Type *TypeGTYP;
struct GTY(()) lang_type
{
    tree GTY((skip(""))) c_type;
    TypeGTYP GTY((skip(""))) d_type;
};

