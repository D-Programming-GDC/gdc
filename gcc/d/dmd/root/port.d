/* port.d -- A mini library for doing compiler/system specific things.
 * Copyright (C) 2018 Free Software Foundation, Inc.
 *
 * GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC; see the file COPYING3.  If not see
 * <http://www.gnu.org/licenses/>.
 */

module dmd.root.port;

nothrow @nogc:

extern (C++) struct Port
{
    nothrow @nogc:

    static int memicmp(scope const char* s1, scope const char* s2, size_t n) pure;
    static char* strupr(char* s) pure;
    static bool isFloat32LiteralOutOfRange(scope const(char)* s);
    static bool isFloat64LiteralOutOfRange(scope const(char)* s);
    static void writelongLE(uint value, scope void* buffer) pure;
    static uint readlongLE(scope void* buffer) pure;
    static void writelongBE(uint value, scope void* buffer) pure;
    static uint readlongBE(scope void* buffer) pure;
    static uint readwordLE(scope void* buffer) pure;
    static uint readwordBE(scope void* buffer) pure;
    static void valcpy(scope void *dst, ulong val, size_t size) pure;
}
