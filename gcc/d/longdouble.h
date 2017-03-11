// longdouble.h -- D frontend for GCC.
// Copyright (C) 2011-2013 Free Software Foundation, Inc.

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
class Type;

struct longdouble
{
 public:
  const real_value& rv() const;
  real_value& rv();

  // No constructor to be able to use this class in a union.
  template<typename T> longdouble& operator = (T x)
  { set (x); return *this;}

  // We need to list all basic types to avoid ambiguities
  void set (float d);
  void set (double d);
  void set (real_value& rv);

  void set (int8_t d);
  void set (int16_t d);
  void set (int32_t d);
  void set (int64_t d);

  void set (uint8_t d);
  void set (uint16_t d);
  void set (uint32_t d);
  void set (uint64_t d);

  void set (bool d);

  // Rvalue operators.
  operator float();
  operator double();
  operator real_value&();

  operator int8_t();
  operator int16_t();
  operator int32_t();
  operator int64_t();

  operator uint8_t();
  operator uint16_t();
  operator uint32_t();
  operator uint64_t();

  operator bool();

  // Arithmetic operators.
  longdouble operator + (const longdouble& r);
  longdouble operator - (const longdouble& r);
  longdouble operator * (const longdouble& r);
  longdouble operator / (const longdouble& r);
  longdouble operator % (const longdouble& r);

  longdouble operator -();

  // Comparison operators.
  bool operator < (const longdouble& r);
  bool operator <= (const longdouble& r);
  bool operator > (const longdouble& r);
  bool operator >= (const longdouble& r);
  bool operator == (const longdouble& r);
  bool operator != (const longdouble& r);

  int format (char *buf, unsigned buf_size) const;
  int formatHex (char fmt, char *buf, unsigned buf_size) const;

  // for debugging:
  void dump();

 private:
  longdouble from_int (Type *type, int64_t d);
  longdouble from_uint (Type *type, uint64_t d);
  int64_t to_int (Type *type) const;
  uint64_t to_uint (Type *type) const;

  // Including gcc/real.h presents too many problems, so
  // just statically allocate enough space for REAL_VALUE_TYPE.
  long realvalue[(2 + (16 + sizeof(long)) / sizeof(long))];
};

// "volatile" is not required.
typedef longdouble volatile_longdouble;

// Use ldouble() to explicitely create a longdouble value.
template<typename T>
inline longdouble
ldouble (T x)
{
  longdouble d;
  d.set (x);
  return d;
}

template<typename T> inline longdouble operator + (longdouble ld, T x) { return ld + ldouble (x); }
template<typename T> inline longdouble operator - (longdouble ld, T x) { return ld - ldouble (x); }
template<typename T> inline longdouble operator * (longdouble ld, T x) { return ld * ldouble (x); }
template<typename T> inline longdouble operator / (longdouble ld, T x) { return ld / ldouble (x); }

template<typename T> inline longdouble operator + (T x, longdouble ld) { return ldouble (x) + ld; }
template<typename T> inline longdouble operator - (T x, longdouble ld) { return ldouble (x) - ld; }
template<typename T> inline longdouble operator * (T x, longdouble ld) { return ldouble (x) * ld; }
template<typename T> inline longdouble operator / (T x, longdouble ld) { return ldouble (x) / ld; }

template<typename T> inline longdouble& operator += (longdouble& ld, T x) { return ld = ld + x; }
template<typename T> inline longdouble& operator -= (longdouble& ld, T x) { return ld = ld - x; }
template<typename T> inline longdouble& operator *= (longdouble& ld, T x) { return ld = ld * x; }
template<typename T> inline longdouble& operator /= (longdouble& ld, T x) { return ld = ld / x; }

template<typename T> inline bool operator <  (longdouble ld, T x) { return ld <  ldouble (x); }
template<typename T> inline bool operator <= (longdouble ld, T x) { return ld <= ldouble (x); }
template<typename T> inline bool operator >  (longdouble ld, T x) { return ld >  ldouble (x); }
template<typename T> inline bool operator >= (longdouble ld, T x) { return ld >= ldouble (x); }
template<typename T> inline bool operator == (longdouble ld, T x) { return ld == ldouble (x); }
template<typename T> inline bool operator != (longdouble ld, T x) { return ld != ldouble (x); }

template<typename T> inline bool operator < (T x, longdouble ld) { return ldouble (x) <  ld; }
template<typename T> inline bool operator <= (T x, longdouble ld) { return ldouble (x) <= ld; }
template<typename T> inline bool operator > (T x, longdouble ld) { return ldouble (x) >  ld; }
template<typename T> inline bool operator >= (T x, longdouble ld) { return ldouble (x) >= ld; }
template<typename T> inline bool operator == (T x, longdouble ld) { return ldouble (x) == ld; }
template<typename T> inline bool operator != (T x, longdouble ld) { return ldouble (x) != ld; }

#endif
