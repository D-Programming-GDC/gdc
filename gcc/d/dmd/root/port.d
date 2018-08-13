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

extern (C++) struct Port
{
    static int memicmp(const char* s1, const char* s2, size_t n);
    static char* strupr(char* s);
    static bool isFloat32LiteralOutOfRange(const(char)* s);
    static bool isFloat64LiteralOutOfRange(const(char)* s);
    static void writelongLE(uint value, void* buffer);
    static uint readlongLE(void* buffer);
    static void writelongBE(uint value, void* buffer);
    static uint readlongBE(void* buffer);
    static uint readwordLE(void* buffer);
    static uint readwordBE(void* buffer);
    static void valcpy(void *dst, ulong val, size_t size);
}
