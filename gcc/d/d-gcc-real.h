// d-gcc-real.h -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

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

#ifndef GCC_DCMPLR_D_REAL_H
#define GCC_DCMPLR_D_REAL_H

struct real_value;
struct Type;

struct real_t
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
  static real_t parse (const char *str, Mode mode);
  static real_t getnan (Mode mode);
  static real_t getsnan (Mode mode);
  static real_t getinfinity (void);

  // This constructor prevent the use of the real_t in a union
  real_t (void) { }
  real_t (const real_t& r);

  const real_value& rv (void) const;
  real_value& rv (void);
  real_t (const real_value& rv);
  real_t (int v);
  real_t (d_uns64 v);
  real_t (d_int64 v);
  real_t (double d);
  real_t& operator= (const real_t& r);
  real_t& operator= (int v);
  real_t operator+ (const real_t& r);
  real_t operator- (const real_t& r);
  real_t operator- (void);
  real_t operator* (const real_t& r);
  real_t operator/ (const real_t& r);
  real_t operator% (const real_t& r);
  bool operator< (const real_t& r);
  bool operator> (const real_t& r);
  bool operator<= (const real_t& r);
  bool operator>= (const real_t& r);
  bool operator== (const real_t& r);
  bool operator!= (const real_t& r);
  d_uns64 toInt (void) const;
  d_uns64 toInt (Type *real_type, Type *int_type) const;
  real_t convert (Mode to_mode) const;
  real_t convert (Type *to_type) const;
  bool isZero (void);
  bool isNegative (void);
  bool isConst0 (void);
  bool isConst1 (void);
  bool isConst2 (void);
  bool isConstMinus1 (void);
  bool isConstHalf (void);
  bool floatCompare (int op, const real_t& r);
  bool isIdenticalTo (const real_t& r) const;
  void format (char *buf, unsigned buf_size) const;
  void formatHex (char *buf, unsigned buf_size) const;
  // for debugging:
  bool isInf (void);
  bool isNan (void);
  bool isSignallingNan (void);
  bool isConversionExact (Mode to_mode) const;
  void dump (void);

 private:
  // prevent this from being used
  real_t& operator= (float)
  { return *this; }

  real_t& operator= (double)
  { return *this; }

  fake_t frv_;
};

struct real_t_Properties
{
  real_t maxval, minval, epsilonval /*, nanval, infval */;
  d_int64 dig, mant_dig;
  d_int64 max_10_exp, min_10_exp;
  d_int64 max_exp, min_exp;
};

extern real_t_Properties real_t_properties[real_t::NumModes];

#endif
