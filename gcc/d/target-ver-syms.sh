#!/bin/sh

# GDC -- D front-end for GCC
# Copyright (C) 2011, 2012 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.

target=$1
target_cpu=`echo $target | sed 's/^\([^-]*\)-\([^-]*\)-\(.*\)$/\1/'`
target_vendor=`echo $target | sed 's/^\([^-]*\)-\([^-]*\)-\(.*\)$/\2/'`
target_os=`echo $target | sed 's/^\([^-]*\)-\([^-]*\)-\(.*\)$/\3/'`

d_target_os=`echo $target_os | sed 's/^\([A-Za-z_]+\)/\1/'`

# In DMD, this is usually defined in the target's Makefile.
case "$d_target_os" in
darwin*)
  echo "#define TARGET_OSX       1"
  echo "#define THREAD_LIBRARY   \"\""
  echo "#define TIME_LIBRARY     \"\""
  ;;
freebsd* | k*bsd*-gnu)
  echo "#define TARGET_FREEBSD   1"
  ;;
*androideabi)
  echo "#define TARGET_LINUX     1"
  echo "#define THREAD_LIBRARY   \"\""
  echo "#define TIME_LIBRARY     \"\""
  ;;
linux*)
  echo "#define TARGET_LINUX     1"
  ;;
openbsd*)
  echo "#define TARGET_OPENBSD   1"
  ;;
solaris* | kopensolaris*-gnu)
  echo "#define TARGET_SOLARIS   1"
  ;;
mingw32*)
  echo "#define TARGET_WINDOS    1"
  echo "#define THREAD_LIBRARY   \"\""
  echo "#define TIME_LIBRARY     \"\""
  echo "#define MATH_LIBRARY     \"\""
  ;;
esac
