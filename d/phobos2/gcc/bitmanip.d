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


// Some helper functions for similar functions:
version (X86_32_or_64) {    
    private template gen_bs_variant(string suffix, string type) {
        const gen_bs_variant = `
            int bs` ~ suffix ~ `(` ~ type ~ ` v) {
                int retval = void;
                asm {"bs` ~ suffix ~ ` %[val], %[result]"
                     :
                     [result] "=r" retval
                     :
                     [val] "rm" v
                     ;
                }
                return retval;
            }
        `;
    }

    private template gen_bt_variant(string suffix, bool locked) {
        const gen_bt_variant = `
            int ` ~ (locked ? "locked_" : "") ~ `bt` ~ suffix ~ `(in uint* p, size_t bit) {
                bool retval = void;
                asm {"` ~ (locked ? "lock " : "") ~ `
                     bt` ~ suffix ~ ` %[bit], (%[addr])
                     setc %[result]
                     "
                     :
                     [result] "=g" retval
                     :
                     [addr] "p" p,
                     [bit] "rI" bit
                     ;
                }
                return retval ? -1 : 0;
            }
        `;
    }

    private template gen_in_variant(string suffix, string type) {
        const gen_in_variant = `
            ` ~ type ~ ` inp` ~ suffix ~ `(uint p) {
                ` ~ type ~ ` retval = void;
                asm {"in` ~ (suffix.length ? suffix : "b") ~ ` %[port], %[result]"
                     :
                     [result] "=a" retval
                     :
                     [port] "Nd" cast(ushort)p
                     ;
                }
                return retval;
            }
        `;
    }
    
    private template gen_out_variant(string suffix, string type) {
        const gen_out_variant = `
            ` ~ type ~ ` outp` ~ suffix ~ `(uint p, ` ~ type ~ ` v) {
                ` ~ type ~ ` retval = void;
                asm {"out` ~ (suffix.length ? suffix : "b") ~ ` %[val], %[port]"
                     :
                     :
                     [port] "Nd" cast(ushort)p,
                     [val] "a" v
                     ;
                }
                return v;
            }
        `;
    }
}


/**
 * Scans the bits in v starting with bit 0, looking
 * for the first set bit.
 * Returns:
 *      The bit number of the first bit set.
 *      The return value is undefined if v is zero.
 */
version (X86_32_or_64) mixin(gen_bs_variant!("f", "size_t")); else
int bsf(size_t v)
{
    uint m = 1;
    uint i;
    for (i = 0; i < 32; i++,m<<=1) {
        if (v&m)
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
 *     uint v;
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
version (X86_32_or_64) mixin(gen_bs_variant!("r", "uint")); else
int bsr(uint v)
{
    uint m = 0x80000000;
    uint i;
    for (i = 32; i ; i--,m>>>=1) {
        if (v&m)
            return i-1;
    }
    return i; // supposed to be undefined
}

/**
 * Tests the bit.
 */
version (X86_32_or_64) mixin(gen_bt_variant!("", false)); else
int bt(in uint *p, size_t bitnum)
{
    return (p[bitnum / (uint.sizeof*8)] & (1<<(bitnum & ((uint.sizeof*8)-1)))) ? -1 : 0 ;
}

/**
 * Tests and complements the bit.
 */
version (X86_32_or_64) mixin(gen_bt_variant!("c", false)); else
int btc(uint *p, size_t bitnum)
{
    uint * q = p + (bitnum / (uint.sizeof*8));
    uint mask = 1 << (bitnum & ((uint.sizeof*8) - 1));
    int result = *q & mask;
    *q ^= mask;
    return result ? -1 : 0;
}

/**
 * Tests and resets (sets to 0) the bit.
 */
version (X86_32_or_64) mixin(gen_bt_variant!("r", false)); else
int btr(uint *p, size_t bitnum)
{
    uint * q = p + (bitnum / (uint.sizeof*8));
    uint mask = 1 << (bitnum & ((uint.sizeof*8) - 1));
    int result = *q & mask;
    *q &= ~mask;
    return result ? -1 : 0;
}

/**
 * Tests and sets the bit.
 * Params:
 * p = a non-NULL pointer to an array of uints.
 * index = a bit number, starting with bit 0 of p[0],
 * and progressing. It addresses bits like the expression:
---
p[index / (uint.sizeof*8)] & (1 << (index & ((uint.sizeof*8) - 1)))
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
    uint array[2];

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
version (X86_32_or_64) mixin(gen_bt_variant!("s", false)); else
int bts(uint *p, size_t bitnum)
{
    uint * q = p + (bitnum / (uint.sizeof*8));
    uint mask = 1 << (bitnum & ((uint.sizeof*8) - 1));
    int result = *q & mask;
    *q |= mask;
    return result ? -1 : 0;
}


/**
 * Swaps bytes in a 4 byte uint end-to-end, i.e. byte 0 becomes
        byte 3, byte 1 becomes byte 2, byte 2 becomes byte 1, byte 3
        becomes byte 0.
 */
version (X86_32_or_64)
    uint bswap(uint v)
    {
        uint retval = void;
        asm {"bswap %[val]" : [val] "=r" retval : "0" v; }
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
version (X86_32_or_64) mixin(gen_in_variant!("", "ubyte")); else
ubyte inp(uint p) { return 0; }

/**
 * ditto
 */
version (X86_32_or_64) mixin(gen_in_variant!("w", "ushort")); else
ushort inpw(uint p) { return 0; }

/**
 * ditto
 */
version (X86_32_or_64) mixin(gen_in_variant!("l", "uint")); else
uint inpl(uint p) { return 0; }


/**
 * Writes and returns value to I/O port at port_address.
 */
version (X86_32_or_64) mixin(gen_out_variant!("", "ubyte")); else
ubyte outp(uint p, ubyte v) { return v; }

/**
 * ditto
 */
version (X86_32_or_64) mixin(gen_out_variant!("w", "ushort")); else
ushort outpw(uint p, ushort v) { return v; }

/**
 * ditto
 */
version (X86_32_or_64) mixin(gen_out_variant!("l", "uint")); else
uint outpl(uint p, uint v) { return v; }
