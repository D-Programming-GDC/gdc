/**
 * D header file for C99.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License: Distributed under the
 *      $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost Software License 1.0).
 *    (See accompanying file LICENSE)
 * Authors:   Sean Kelly
 * Source:    $(DRUNTIMESRC core/stdc/_config.d)
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 */

/* NOTE: This file has been patched from the original DMD distribution to
 * work with the GDC compiler.
 */
module core.stdc.config;

extern (C):
@trusted: // Types only.
nothrow:
@nogc:

version( GNU )
{
    import gcc.builtins;
    alias __builtin_clong c_long;
    alias __builtin_culong c_ulong;
}
else version( Windows )
{
    alias int   c_long;
    alias uint  c_ulong;
}
else
{
  static if( (void*).sizeof > int.sizeof )
  {
    alias long  c_long;
    alias ulong c_ulong;
  }
  else
  {
    alias int   c_long;
    alias uint  c_ulong;
  }
}
