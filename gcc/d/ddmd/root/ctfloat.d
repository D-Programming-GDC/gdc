/**
 * Compiler implementation of the
 * $(LINK2 http://www.dlang.org, D programming language).
 *
 * Copyright:   Copyright (c) 1999-2017 by Digital Mars, All Rights Reserved
 * Authors:     $(LINK2 http://www.digitalmars.com, Walter Bright)
 * License:     $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:      $(LINK2 https://github.com/dlang/dmd/blob/master/src/ddmd/root/ctfloat.d, root/_ctfloat.d)
 */

module ddmd.root.ctfloat;

// Online documentation: https://dlang.org/phobos/ddmd_root_ctfloat.html

import core.stdc.config;
import core.stdc.stdint;

struct real_value;

// Type used by the front-end for compile-time reals
struct longdouble
{
    this(T)(T r)
    {
        this.set(r);
    }

    // No constructor to be able to use this class in a union.
    longdouble opAssign(T)(T r)
        if (is (T : longdouble))
    {
        this.realvalue = r.realvalue; 
        return this;
    }

    longdouble opAssign(T)(T r)
    {
        this.set(r);
        return this;
    }

    // Arithmetic operators.
    longdouble opBinary(string op, T)(T r) const
        if ((op == "+" || op == "-" || op == "*" || op == "/" || op == "%")
            && is (T : longdouble))
    {
        static if (op == "+")
            return this.add(r);
        else static if (op == "-")
            return this.sub(r);
        else static if (op == "*")
            return this.mul(r);
        else static if (op == "/")
            return this.div(r);
        else static if (op == "%")
            return this.mod(r);
    }

    longdouble opUnary(string op)() const
        if (op == "-")
    {
        return this.neg();
    }

    int opCmp(longdouble r) const
    {
        return this.cmp(r);
    }

    int opEquals(longdouble r) const
    {
        return this.equals(r);
    }

    bool opCast(T : bool)() const
    {
        return this.to_bool();
    }

    T opCast(T)() const
    {
        static if (__traits(isUnsigned, T))
            return cast (T) this.to_uint();
        else
            return cast(T) this.to_int();
    }

    extern (C++):

    void set(int8_t d);
    void set(int16_t d);
    void set(int32_t d);
    void set(int64_t d);
    void set(uint8_t d);
    void set(uint16_t d);
    void set(uint32_t d);
    void set(uint64_t d);
    void set(bool d);

    int64_t to_int() const;
    uint64_t to_uint() const;
    bool to_bool() const;

    longdouble add(const ref longdouble r) const;
    longdouble sub(const ref longdouble r) const;
    longdouble mul(const ref longdouble r) const;
    longdouble div(const ref longdouble r) const;
    longdouble mod(const ref longdouble r) const;
    longdouble neg() const;
    int cmp(const ref longdouble t) const;
    int equals(const ref longdouble t) const;

private:
    // Statically allocate enough space for REAL_VALUE_TYPE.
    enum realvalue_size = (2 + (16 + c_long.sizeof) / c_long.sizeof);
    c_long [realvalue_size] realvalue;
}

alias real_t = longdouble;

// Compile-time floating-point helper
extern (C++) struct CTFloat
{
    static __gshared bool yl2x_supported = false;
    static __gshared bool yl2xp1_supported = false;

    static void yl2x(const real_t* x, const real_t* y, real_t* res);
    static void yl2xp1(const real_t* x, const real_t* y, real_t* res);
    static real_t sin(real_t x);
    static real_t cos(real_t x);
    static real_t tan(real_t x);
    static real_t sqrt(real_t x);
    static real_t fabs(real_t x);
    static real_t ldexp(real_t n, int exp);
    static bool isIdentical(real_t a, real_t b);
    static size_t hash(real_t a);
    static bool isNaN(real_t r);
    static bool isSNaN(real_t r);
    static bool isInfinity(real_t r);
    static real_t parse(const(char)* literal, bool* isOutOfRange = null);
    static int sprint(char* str, char fmt, real_t x);

    static __gshared real_t zero;
    static __gshared real_t one;
    static __gshared real_t minusone;
    static __gshared real_t half;
}
