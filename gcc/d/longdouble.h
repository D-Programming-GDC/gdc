// longdouble.h -- D frontend for GCC.
// Copyright (C) 2011, 2012, 2013 Free Software Foundation, Inc.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#ifndef GCC_D_LONGDOUBLE_H
#define GCC_D_LONGDOUBLE_H

struct real_value;
struct Type;

struct longdouble
{
 public:
  // Including gcc/real.h presents too many problems, so
  // just statically allocate enough space for REAL_VALUE_TYPE.
#define REAL_T_SIZE (16 + sizeof (long))/sizeof (long) + 1

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
    long m[REAL_T_SIZE];
  };

  static void init (void);
  static longdouble parse (const char *str, Mode mode);

  // This constructor prevent the use of the longdouble in a union
  longdouble (void) { }
  longdouble (const longdouble& r);

  const real_value& rv (void) const;
  real_value& rv (void);
  longdouble (const real_value& rv);
  longdouble (int v);
  longdouble (uint64_t v);
  longdouble (int64_t v);
  longdouble (double d);

  longdouble& operator= (const longdouble& r);
  longdouble& operator= (int v);
  longdouble operator+ (const longdouble& r);
  longdouble operator- (const longdouble& r);
  longdouble operator- (void);
  longdouble operator* (const longdouble& r);
  longdouble operator/ (const longdouble& r);
  longdouble operator% (const longdouble& r);

  bool operator< (const longdouble& r);
  bool operator> (const longdouble& r);
  bool operator<= (const longdouble& r);
  bool operator>= (const longdouble& r);
  bool operator== (const longdouble& r);
  bool operator!= (const longdouble& r);

  uint64_t toInt (void) const;
  uint64_t toInt (Type *rt, Type *it) const;
  bool isZero (void);
  bool isNegative (void);
  bool isIdenticalTo (const longdouble& r) const;
  int format (char *buf, unsigned buf_size) const;
  int formatHex (char *buf, unsigned buf_size) const;
  // for debugging:
  void dump (void);

 private:
  // prevent this from being used
  longdouble& operator= (float)
  { return *this; }

  longdouble& operator= (double)
  { return *this; }

  fake_t frv_;
};


template<typename T> longdouble ldouble(T x)
{
  return (longdouble) x;
}

inline int ld_sprint (char* str, int fmt, longdouble x)
{
  if(fmt == 'a' || fmt == 'A')
    return x.formatHex(str, 32);

  return x.format(str, 32);
}


// List of values for .max, .min, etc, for floats in D.

struct real_properties
{
  longdouble maxval, minval, epsilonval;
  int64_t dig, mant_dig;
  int64_t max_10_exp, min_10_exp;
  int64_t max_exp, min_exp;
};

extern real_properties real_limits[longdouble::NumModes];

// Macros are used by the D frontend, so map to longdouble property values instead of host long double.
#undef FLT_MAX
#undef DBL_MAX
#undef LDBL_MAX
#undef FLT_MIN
#undef DBL_MIN
#undef LDBL_MIN
#undef FLT_DIG
#undef DBL_DIG
#undef LDBL_DIG
#undef FLT_MANT_DIG
#undef DBL_MANT_DIG
#undef LDBL_MANT_DIG
#undef FLT_MAX_10_EXP
#undef DBL_MAX_10_EXP
#undef LDBL_MAX_10_EXP
#undef FLT_MIN_10_EXP
#undef DBL_MIN_10_EXP
#undef LDBL_MIN_10_EXP
#undef FLT_MAX_EXP
#undef DBL_MAX_EXP
#undef LDBL_MAX_EXP
#undef FLT_MIN_EXP
#undef DBL_MIN_EXP
#undef LDBL_MIN_EXP
#undef FLT_EPSILON
#undef DBL_EPSILON
#undef LDBL_EPSILON

#define FLT_MAX real_limits[longdouble::Float].maxval;
#define DBL_MAX real_limits[longdouble::Double].maxval;
#define LDBL_MAX real_limits[longdouble::LongDouble].maxval;
#define FLT_MIN real_limits[longdouble::Float].minval;
#define DBL_MIN real_limits[longdouble::Double].minval;
#define LDBL_MIN real_limits[longdouble::LongDouble].minval;
#define FLT_DIG real_limits[longdouble::Float].dig;
#define DBL_DIG real_limits[longdouble::Double].dig;
#define LDBL_DIG real_limits[longdouble::LongDouble].dig;
#define FLT_MANT_DIG real_limits[longdouble::Float].mant_dig;
#define DBL_MANT_DIG real_limits[longdouble::Double].mant_dig;
#define LDBL_MANT_DIG real_limits[longdouble::LongDouble].mant_dig;
#define FLT_MAX_10_EXP real_limits[longdouble::Float].max_10_exp;
#define DBL_MAX_10_EXP real_limits[longdouble::Double].max_10_exp;
#define LDBL_MAX_10_EXP real_limits[longdouble::LongDouble].max_10_exp;
#define FLT_MIN_10_EXP real_limits[longdouble::Float].min_10_exp;
#define DBL_MIN_10_EXP real_limits[longdouble::Double].min_10_exp;
#define LDBL_MIN_10_EXP real_limits[longdouble::LongDouble].min_10_exp;
#define FLT_MAX_EXP real_limits[longdouble::Float].max_exp;
#define DBL_MAX_EXP real_limits[longdouble::Double].max_exp;
#define LDBL_MAX_EXP real_limits[longdouble::LongDouble].max_exp;
#define FLT_MIN_EXP real_limits[longdouble::Float].min_exp;
#define DBL_MIN_EXP real_limits[longdouble::Double].min_exp;
#define LDBL_MIN_EXP real_limits[longdouble::LongDouble].min_exp;
#define FLT_EPSILON real_limits[longdouble::Float].epsilonval;
#define DBL_EPSILON real_limits[longdouble::Double].epsilonval;
#define LDBL_EPSILON real_limits[longdouble::LongDouble].epsilonval;

#endif
