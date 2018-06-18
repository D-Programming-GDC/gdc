/**
 * Compiler implementation of the D programming language
 * http://dlang.org
 *
 * Copyright: Copyright (c) 1999-2017 by Digital Mars, All Rights Reserved
 * Authors:   Walter Bright, http://www.digitalmars.com
 * License:   $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:    $(LINK2 https://github.com/dlang/dmd/blob/master/src/ddmd/root/port.d, root/_port.d)
 */

module ddmd.root.port;

// Online documentation: https://dlang.org/phobos/ddmd_root_port.html

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
