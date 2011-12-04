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

#ifndef D_D_SPEC
#define D_D_SPEC 0
#endif

/* %{!M} probably doesn't make sense because we would need
   to do that -- -MD and -MMD doesn't sound like a plan for D.... */

/* %(d_options) ? */

#if D_DRIVER_ONLY
{".html", "@d", 0, 1, 0 },
{".HTML", "@d", 0, 1, 0 },
{".htm", "@d", 0, 1, 0 },
{".HTM", "@d", 0, 1, 0 },
{".xhtml", "@d", 0, 1, 0 },
{".XHTML", "@d", 0, 1, 0 },
{".d", "@d", 0, 1, 0 },
{".D", "@d", 0, 1, 0 },
{".dd", "@d", 0, 1, 0 },
{".DD", "@d", 0, 1, 0 },
{".di", "@d", 0, 1, 0 },
{".DI", "@d", 0, 1, 0 },
{"@d",
     "%{!E:cc1d %i %(cc1_options) %(cc1d) %I %{nostdinc*} %{+e*} %{I*} %{J*}\
      %{M} %{MM} %{!fsyntax-only:%(invoke_as)}}", D_D_SPEC, 1, 0 },
#else
{".d", "@d", 0, 1, 0 },
{".D", "@d", 0, 1, 0 },
{".di", "@d", 0, 1, 0 },
{".DI", "@d", 0, 1, 0 },
{"@d",
     "%{!E:cc1d %i %(cc1_options) %(cc1d) %I %{nostdinc*} %{+e*} %{I*} %{J*}\
      %{M} %{MM} %{!fsyntax-only:%(invoke_as)}}", D_D_SPEC, 1, 0 },
#endif

