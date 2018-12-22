/* longdouble.d -- Software floating point emulation for the D frontend.
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

module dmd.root.longdouble;

import core.stdc.config;
import core.stdc.stdint;

nothrow:
@nogc:

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
        if (!is (T : longdouble))
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

