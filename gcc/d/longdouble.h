/* longdouble.h -- Definitions of floating point access for the frontend.
   Copyright (C) 2015-2017 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_D_LONGDOUBLE_H
#define GCC_D_LONGDOUBLE_H

struct real_value;
class Type;

struct longdouble
{
public:
   /* Return the hidden real_value from the longdouble type.  */
  const real_value& rv (void) const
  { return *(const real_value *) this; }

  real_value& rv (void)
  { return *(real_value *) this; }

  /* No constructor to be able to use this class in a union.  */
  template<typename T> longdouble& operator = (T x)
  { set (x); return *this; }

  /* Lvalue operators.
     We need to list all basic types to avoid ambiguities.  */
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

  /* Rvalue operators.  */
  operator real_value&();
  operator int8_t (void);
  operator int16_t (void);
  operator int32_t (void);
  operator int64_t (void);
  operator uint8_t (void);
  operator uint16_t (void);
  operator uint32_t (void);
  operator uint64_t (void);
  operator bool (void);

  /* Arithmetic operators.  */
  longdouble operator + (const longdouble& r);
  longdouble operator - (const longdouble& r);
  longdouble operator * (const longdouble& r);
  longdouble operator / (const longdouble& r);
  longdouble operator % (const longdouble& r);

  longdouble operator -();

  /* Comparison operators.  */
  bool operator < (const longdouble& r);
  bool operator <= (const longdouble& r);
  bool operator > (const longdouble& r);
  bool operator >= (const longdouble& r);
  bool operator == (const longdouble& r);
  bool operator != (const longdouble& r);

private:
  longdouble from_int (Type *type, int64_t d);
  longdouble from_uint (Type *type, uint64_t d);
  int64_t to_int (Type *type) const;
  uint64_t to_uint (Type *type) const;

  /* Including gcc/real.h presents too many problems, so just
     statically allocate enough space for REAL_VALUE_TYPE.  */
  long realvalue[(2 + (16 + sizeof (long)) / sizeof (long))];
};

/* Declared, but "volatile" is not required.  */
typedef longdouble volatile_longdouble;

/* Use ldouble() to explicitely create a longdouble value.  */
template<typename T>
inline longdouble
ldouble (T x)
{
  longdouble d;
  d.set (x);
  return d;
}

#endif
