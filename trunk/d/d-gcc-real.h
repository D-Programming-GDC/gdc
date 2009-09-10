#ifndef GCC_DCMPLR_D_REAL_H
#define GCC_DCMPLR_D_REAL_H

struct real_value;
struct Type;

struct real_t {
    // Including gcc/real.h presents too many problems, so
    // just statically allocate enough space for
    // REAL_VALUE_TYPE.
    typedef enum {
	Float,
	Double,
	LongDouble,
	NumModes
    } MyMode;
    typedef struct {
	int c;
	int s;
	int e;
	long m[ (16 + sizeof(long))/sizeof(long) + 1 ];
    } fake_t;
    
    fake_t frv;    
    
    static void init();
    static real_t parse(const char * str, MyMode mode);
    static real_t getnan(MyMode mode);
    static real_t getinfinity();

    // This constructor prevent the use of the real_t in a union
    real_t() { }
    real_t(const real_t & r);

    const real_value & rv() const { return * (real_value *) & frv; }
    real_value & rv() { return * (real_value *) & frv; }
    real_t(const struct real_value & rv);
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
    real_t convert(MyMode to_mode) const;
    bool isZero();
    bool isNegative();
    bool floatCompare(int op, const real_t & r);
    bool isIdenticalTo(const real_t & r) const;
    void format(char * buf, unsigned buf_size) const;
    void formatHex(char * buf, unsigned buf_size) const;
    // for debugging:
    bool isInf();
    bool isNan();
    bool isConversionExact(MyMode to_mode) const;
    void toBytes(unsigned char * buf, unsigned buf_size);
    void dump();
private:
    // prevent this from being used
    real_t & operator=(float) { return *this; }
    real_t & operator=(double) { return *this; }
    // real_t & operator=(long double v) { return *this; }
};

typedef struct {
    real_t maxval, minval, epsilonval/*, nanval, infval*/;
    d_int64 dig, mant_dig;
    d_int64 max_10_exp, min_10_exp;
    d_int64 max_exp, min_exp;
} real_t_Properties;

extern real_t_Properties real_t_properties[real_t::NumModes];

#endif
