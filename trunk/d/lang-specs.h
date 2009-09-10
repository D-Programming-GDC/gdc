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

#if GCC_SPEC_FORMAT_4
#define D_D_SPEC_REST 0, 1, 0
#else
#define D_D_SPEC_REST 0
#endif

#if D_DRIVER_ONLY
#define D_USE_EXTRA_SPEC_FUNCTIONS 1
{".html", "@d", D_D_SPEC_REST },
{".HTML", "@d", D_D_SPEC_REST },
{".htm", "@d", D_D_SPEC_REST },
{".HTM", "@d", D_D_SPEC_REST },
{".xhtml", "@d", D_D_SPEC_REST },
{".XHTML", "@d", D_D_SPEC_REST },
{".d", "@d", D_D_SPEC_REST },
{".D", "@d", D_D_SPEC_REST },
{"@d",
     "%{!E:cc1d %i %:d-all-sources() %(cc1_options) %(cc1d) %I %N %{nostdinc*} %{+e*} %{I*} %{J*}\
      %{M} %{MM} %{!fsyntax-only:%(invoke_as)}}", D_D_SPEC_REST },
#else
{".d", "@d", D_D_SPEC_REST },
{".D", "@d", D_D_SPEC_REST },
{"@d",
     "%{!E:cc1d %i %(cc1_options) %(cc1d) %I %N %{nostdinc*} %{+e*} %{I*} %{J*}\
      %{M} %{MM} %{!fsyntax-only:%(invoke_as)}}", D_D_SPEC_REST },
#endif

