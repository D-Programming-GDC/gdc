/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GCC_DCMPLR_D_REAL_H
#define GCC_DCMPLR_D_REAL_H

struct real_value;
struct Type;

struct real_t
{
    // Including gcc/real.h presents too many problems, so
    // just statically allocate enough space for
    // REAL_VALUE_TYPE.
    enum Mode
    {
        Float,
        Double,
        LongDouble,
        NumModes
    };

    struct fake_t
    {
        int c;
        int s;
        int e;
        long m[ (16 + sizeof(long))/sizeof(long) + 1 ];
    };

    fake_t frv;

    static void init();
    static real_t parse(const char * str, Mode mode);
    static real_t getnan(Mode mode);
    static real_t getsnan(Mode mode);
    static real_t getinfinity();

    // This constructor prevent the use of the real_t in a union
    real_t() { }
    real_t(const real_t & r);

    const real_value & rv() const;
    real_value & rv();
    real_t(const real_value & rv);
    real_t(int v);
    real_t(d_uns64 v);
    real_t(d_int64 v);
    real_t & operator=(const real_t & r);
    real_t & operator=(int v);
    real_t operator+ (const real_t & r);
    real_t operator- (const real_t & r);
    real_t operator- ();
    real_t operator* (const real_t & r);
    real_t operator/ (const real_t & r);
    real_t operator% (const real_t & r);
    bool operator< (const real_t & r);
    bool operator> (const real_t & r);
    bool operator<= (const real_t & r);
    bool operator>= (const real_t & r);
    bool operator== (const real_t & r);
    bool operator!= (const real_t & r);
    //operator d_uns64(); // avoid bugs, but maybe allow operator bool()
    d_uns64 toInt() const;
    d_uns64 toInt(Type * real_type, Type * int_type) const;
    real_t convert(Mode to_mode) const;
    real_t convert(Type * to_type) const;
    bool isZero();
    bool isNegative();
    bool isConst0();
    bool isConst1();
    bool isConst2();
    bool isConstMinus1();
    bool isConstHalf();
    bool floatCompare(int op, const real_t & r);
    bool isIdenticalTo(const real_t & r) const;
    void format(char * buf, unsigned buf_size) const;
    void formatHex(char * buf, unsigned buf_size) const;
    // for debugging:
    bool isInf();
    bool isNan();
    bool isSignallingNan();
    bool isConversionExact(Mode to_mode) const;
    void toBytes(unsigned char * buf, unsigned buf_size);
    void dump();
private:
    // prevent this from being used
    real_t & operator=(float) { return *this; }
    real_t & operator=(double) { return *this; }
    // real_t & operator=(long double v) { return *this; }
};

struct real_t_Properties
{
    real_t maxval, minval, epsilonval/*, nanval, infval*/;
    d_int64 dig, mant_dig;
    d_int64 max_10_exp, min_10_exp;
    d_int64 max_exp, min_exp;
};

extern real_t_Properties real_t_properties[real_t::NumModes];

#endif
