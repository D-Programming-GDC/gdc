/** These functions are built-in intrinsics to the DMD compiler.
 *  Patched from the original DMD distribution to work with the
 *  GDC compiler.
 *
 * Copyright: Public Domain
 * License:   Public Domain
 * Authors:   David Friedman, May 2006
 *            Frits van Bommel, May 2007 (added x86 and x86_64 asm versions)
 *            Iain Buclaw, November 2010 (ported to D2 and moved into gcc.bitmanip)
 */

module gcc.bitmanip;

version (GNU) { } else { static assert(false, "Module only for GDC"); }


// The asm functions work for both x86 and x86_64, so introduce a common version
version (X86)       version = X86_32_or_64;
version (X86_64)    version = X86_32_or_64;


/**
 * Scans the bits in v starting with bit 0, looking
 * for the first set bit.
 * Returns:
 *      The bit number of the first bit set.
 *      The return value is undefined if v is zero.
 */
pure int bsf(size_t v)
{
    size_t m = 1;
    uint i;
    for (i = 0; i < 32; i++, m <<= 1)
    {
        if (v & m)
            return i;
    }
    return i; // supposed to be undefined
}

/**
 * Scans the bits in v from the most significant bit
 * to the least significant bit, looking
 * for the first set bit.
 * Returns:
 *      The bit number of the first bit set.
 *      The return value is undefined if v is zero.
 * Example:
 * ---
 * import std.stdio;
 * import gcc.bitmanip;
 *
 * int main()
 * {
 *     size_t v;
 *     int x;
 *
 *     v = 0x21;
 *     x = bsf(v);
 *     writefln("bsf(x%x) = %d", v, x);
 *     x = bsr(v);
 *     writefln("bsr(x%x) = %d", v, x);
 *     return 0;
 * }
 * ---
 * Output:
 *  bsf(x21) = 0<br>
 *  bsr(x21) = 5
 */
pure int bsr(size_t v)
{
    size_t m = 0x80000000;
    uint i;
    for (i = 32; i; i--, m >>>= 1)
    {
        if (v & m)
            return i - 1;
    }
    return i; // supposed to be undefined
}

/**
 * Tests the bit.
 */
pure int bt(in size_t *p, size_t bitnum)
{
    return (p[bitnum / (size_t.sizeof*8)] & (1<<(bitnum & ((size_t.sizeof*8)-1)))) ? -1 : 0 ;
}

/**
 * Tests and complements the bit.
 */
int btc(size_t *p, size_t bitnum)
{
    size_t * q = p + (bitnum / (size_t.sizeof*8));
    size_t mask = 1 << (bitnum & ((size_t.sizeof*8) - 1));
    sizediff_t result = *q & mask;
    *q ^= mask;
    return result ? -1 : 0;
}

/**
 * Tests and resets (sets to 0) the bit.
 */
int btr(size_t *p, size_t bitnum)
{
    size_t * q = p + (bitnum / (size_t.sizeof*8));
    size_t mask = 1 << (bitnum & ((size_t .sizeof*8) - 1));
    sizediff_t result = *q & mask;
    *q &= ~mask;
    return result ? -1 : 0;
}

/**
 * Tests and sets the bit.
 * Params:
 * p = a non-NULL pointer to an array of size_ts.
 * index = a bit number, starting with bit 0 of p[0],
 * and progressing. It addresses bits like the expression:
---
p[index / (size_t.sizeof*8)] & (1 << (index & ((size_t.sizeof*8) - 1)))
---
 * Returns:
 *      A non-zero value if the bit was set, and a zero
 *      if it was clear.
 *
 * Example:
 * ---
import std.stdio;
import gcc.bitmanip;

int main()
{
    size_t array[2];

    array[0] = 2;
    array[1] = 0x100;

    writefln("btc(array, 35) = %d", <b>btc</b>(array, 35));
    writefln("array = [0]:x%x, [1]:x%x", array[0], array[1]);

    writefln("btc(array, 35) = %d", <b>btc</b>(array, 35));
    writefln("array = [0]:x%x, [1]:x%x", array[0], array[1]);

    writefln("bts(array, 35) = %d", <b>bts</b>(array, 35));
    writefln("array = [0]:x%x, [1]:x%x", array[0], array[1]);

    writefln("btr(array, 35) = %d", <b>btr</b>(array, 35));
    writefln("array = [0]:x%x, [1]:x%x", array[0], array[1]);

    writefln("bt(array, 1) = %d", <b>bt</b>(array, 1));
    writefln("array = [0]:x%x, [1]:x%x", array[0], array[1]);

    return 0;
}
 * ---
 * Output:
<pre>
btc(array, 35) = 0
array = [0]:x2, [1]:x108
btc(array, 35) = -1
array = [0]:x2, [1]:x100
bts(array, 35) = 0
array = [0]:x2, [1]:x108
btr(array, 35) = -1
array = [0]:x2, [1]:x100
bt(array, 1) = -1
array = [0]:x2, [1]:x100
</pre>
 */
int bts(size_t *p, size_t bitnum)
{
    size_t * q = p + (bitnum / (size_t.sizeof*8));
    size_t mask = 1 << (bitnum & ((size_t.sizeof*8) - 1));
    sizediff_t result = *q & mask;
    *q |= mask;
    return result ? -1 : 0;
}


/**
 * Swaps bytes in a 4 byte uint end-to-end, i.e. byte 0 becomes
 * byte 3, byte 1 becomes byte 2, byte 2 becomes byte 1, byte 3
 * becomes byte 0.
 */
version (X86_32_or_64)
uint bswap(uint v)
{
    uint retval = void;
    asm {
        "bswap %[val]"
        : [val] "=r" retval
        : "0" v;
    }
    return retval;
}
else
uint bswap(uint v)
{
    return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v&0xFF0000)>>>8)|((v&0xFF000000)>>>24);
}


/**
 * Reads I/O port at port_address.
 */
version (X86_32_or_64)
ubyte inp (uint p)
{
    ubyte retval = void;
    asm {
        "inb %[port], %[result]"
        : [result] "=a" retval
        : [port] "Nd" cast(ushort)p;
    }
    return retval;
}
else
ubyte inp(uint p) { return 0; }

/**
 * ditto
 */
version (X86_32_or_64)
ushort inpw (uint p)
{
    ushort retval = void;
    asm {
        "inw %[port], %[result]"
        : [result] "=a" retval
        : [port] "Nd" cast(ushort)p;
    }
    return retval;
}
else
ushort inpw(uint p) { return 0; }

/**
 * ditto
 */
version (X86_32_or_64)
uint inpl (uint p)
{
    uint retval = void;
    asm {
        "inl %[port], %[result]"
        : [result] "=a" retval
        : [port] "Nd" cast(ushort)p;
    }
    return retval;
}
else
uint inpl(uint p) { return 0; }


/**
 * Writes and returns value to I/O port at port_address.
 */
version (X86_32_or_64)
ubyte outp (uint p, ubyte v)
{
    ubyte retval = void;
    asm {
        "outb %[val], %[port]"
        :
        : [port] "Nd" cast(ushort)p,
          [val] "a" v;
    }
    return v;
}
else
ubyte outp(uint p, ubyte v) { return v; }

/**
 * ditto
 */
version (X86_32_or_64)
ushort outpw (uint p, ushort v)
{
    ushort retval = void;
    asm {
        "outw %[val], %[port]"
        :
        : [port] "Nd" cast(ushort)p,
          [val] "a" v;
    }
    return v;
}
else
ushort outpw(uint p, ushort v) { return v; }

/**
 * ditto
 */
version (X86_32_or_64)
uint outpl (uint p, uint v)
{
    uint retval = void;
    asm {
        "outl %[val], %[port]"
        :
        : [port] "Nd" cast(ushort)p,
          [val] "a" v;
    }
    return v;
}
else
uint outpl(uint p, uint v) { return v; }
