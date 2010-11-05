/// Implments gcc.fpmath for IEEE floating point format
module gcc.fpcls;

// This must be kept in sync with gcc/fpmath.d
enum
{
    FP_NAN = 1,
    FP_INFINITE,
    FP_ZERO,
    FP_SUBNORMAL,
    FP_NORMAL,
}

enum RealFormat
{
    SameAsDouble,
    DoubleDouble,
    Intel80,
}

struct Info {
    static if (real.sizeof == double.sizeof) {
        static const RealFormat realFormat = RealFormat.SameAsDouble;
    } else version (PPC) {
        static const RealFormat realFormat = RealFormat.DoubleDouble;
        union real_rec {
            real f;
            struct { double hd, ld; }
        }
    } else version (PPC64) {
        static const RealFormat realFormat = RealFormat.DoubleDouble;
        union real_rec {
            real f;
            struct { double hd, ld; }
        }
    } else version (X86) {
        static const RealFormat realFormat = RealFormat.Intel80;
        union real_rec {
            real f;
            struct { uint li, mi, hi; }
        }
    } else version (X86_64) {
        static const RealFormat realFormat = RealFormat.Intel80;
        union real_rec {
            real f;
            struct { uint li, mi, hi; }
        }
    } else {
        static assert(0);
    }
}

union float_rec {
    float f;
    uint  i;
}

int signbit(float f)
{
    float_rec r = void;
    r.f = f;    
    return r.i & 0x80000000;
}

int fpclassify(float f)
{
    float_rec r = void;
    r.f = f;    
    uint i = r.i & 0x7fffffff;

    if (! i)
        return FP_ZERO;
    else if (i < 0x00800000)
        return FP_SUBNORMAL;
    else if (i < 0x7f800000)
        return FP_NORMAL;
    else if (i < 0x7f800001)
        return FP_INFINITE;
    else
        return FP_NAN;
}

union double_rec {
    double f;
    struct {
        version (BigEndian)
            uint  hi, li;
        else
            uint  li, hi;
    }
}

int signbit(double f)
{
    double_rec r = void;
    r.f = f;    
    return r.hi & 0x80000000;
}

int fpclassify(double f)
{
    double_rec r = void;
    r.f = f;    
    uint i = r.hi & 0x7fffffff;

    if (! (i | r.li))
        return FP_ZERO;
    else if (i <  0x00100000)
        return FP_SUBNORMAL;
    else if (i <  0x7ff00000)
        return FP_NORMAL;
    else if (i == 0x7ff00000 && ! r.li)
        return FP_INFINITE;
    else
        return FP_NAN;
}

int signbit(real f)
{
    static if (Info.realFormat == RealFormat.SameAsDouble) {
        return signbit(cast(double) f);
    } else static if (Info.realFormat == RealFormat.DoubleDouble) {
        Info.real_rec r = void;
        r.f = f;
        return signbit(r.hd);
    } else static if (Info.realFormat == RealFormat.Intel80) {
        Info.real_rec r = void;
        r.f = f;
        return r.hi & 0x00008000;
    }
}

int fpclassify(real f)
{
    static if (Info.realFormat == RealFormat.SameAsDouble) {
        return fpclassify(cast(double) f);
    } else static if (Info.realFormat == RealFormat.DoubleDouble) {
        Info.real_rec r = void;
        r.f = f;
        return fpclassify(r.hd);
    } else static if (Info.realFormat == RealFormat.Intel80) {
        Info.real_rec r = void;
        r.f = f;
        uint i = r.hi & 0x00007fff;
        uint li = r.li | (r.mi & 0x7fffffff) ;
        if (! i && ! li)
            return FP_ZERO;
        else if (i <  0x00000001 && (r.mi & 0x80000000) == 0)
            return FP_SUBNORMAL;
        else if (i <  0x00007fff)
            return FP_NORMAL;
        else if (i == 0x00007fff && ! li)
            return FP_INFINITE;
        else
            return FP_NAN;
    }
}

unittest
{
    static if (Info.realFormat == RealFormat.SameAsDouble) {
        const real xrsn = 0x1p-1050;
    } else static if (Info.realFormat == RealFormat.DoubleDouble) {
        const real xrsn = 0x1p-1050;
    } else static if (Info.realFormat == RealFormat.Intel80) {
        const real xrsn = 0x1p-16390;
    }

    static float[]  xfi = [ float.nan, -float.nan, float.infinity, -float.infinity,
        0.0f, -0.0f, 0x1p-135f, -0x1p-135f, 4.2f, -4.2f ];
    static double[] xdi = [ double.nan, -double.nan, double.infinity, -double.infinity,
        0.0, -0.0, 0x1p-1050, -0x1p-1050, 4.2, -4.2 ];
    static real[]   xri = [ real.nan, -real.nan, real.infinity, -real.infinity,
        0.0L, -0.0L, xrsn, -xrsn, 4.2L, -4.2L ];
    static int[]     xo = [ FP_NAN, FP_NAN, FP_INFINITE, FP_INFINITE,
        FP_ZERO, FP_ZERO, FP_SUBNORMAL, FP_SUBNORMAL, FP_NORMAL, FP_NORMAL];

    foreach (int i, int cls; xo) {
        assert( fpclassify(xfi[i]) == xo[i] );
        assert( fpclassify(xdi[i]) == xo[i] );
        assert( fpclassify(xri[i]) == xo[i] );
        assert( ( signbit(xfi[i]) ?1:0 ) == (i & 1) );
        assert( ( signbit(xdi[i]) ?1:0 ) == (i & 1) );
        assert( ( signbit(xri[i]) ?1:0 ) == (i & 1) );
    }
}
