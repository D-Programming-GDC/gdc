/* lang-specs.h -- gcc driver specs for D frontend.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* This is the contribution to the `default_compilers' array in gcc.c
   for the D language.  */

{".d", "@d", 0, 1, 0 },
{".D", "@d", 0, 1, 0 },
{".dd", "@d", 0, 1, 0 },
{".DD", "@d", 0, 1, 0 },
{".di", "@d", 0, 1, 0 },
{".DI", "@d", 0, 1, 0 },
{"@d",
  "%{!E:cc1d %i %(cc1_options) %I %{nostdinc*} %{I*} %{J*} \
    %{MD:-MD %b.deps} %{MMD:-MMD %b.deps} \
    %{M} %{MM} %{MF*} %{MG} %{MP} %{MQ*} %{MT*} \
    %{X:-Xf %b.json} %{Xf*} \
    %{v} %{!fsyntax-only:%(invoke_as)}}", 0, 1, 0 },

