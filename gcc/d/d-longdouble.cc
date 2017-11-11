/* d-longdouble.cc -- Software floating point emulation for the frontend.
   Copyright (C) 2006-2017 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/mtype.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "stor-layout.h"

#include "d-tree.h"
#include "longdouble.h"


/* Constant real values 0, 1, -1 and 0.5.  */
real_t CTFloat::zero;
real_t CTFloat::one;
real_t CTFloat::minusone;
real_t CTFloat::half;


/* Return conversion of signed integer value D to longdouble.
   Conversion is done at precision mode of TYPE.  */

longdouble
longdouble::from_int (Type *type, int64_t d)
{
  tree t = build_ctype (type);
  real_from_integer (&this->rv (), TYPE_MODE (t), d, SIGNED);
  return *this;
}

/* Return conversion of unsigned integer value D to longdouble.
   Conversion is done at precision mode of TYPE.  */

longdouble
longdouble::from_uint (Type *type, uint64_t d)
{
  tree t = build_ctype (type);
  real_from_integer (&this->rv (), TYPE_MODE (t), d, UNSIGNED);
  return *this;
}

/* Return conversion of longdouble value to int64_t.
   Conversion is done at precision mode of TYPE.  */

int64_t
longdouble::to_int (Type *type) const
{
  if (REAL_VALUE_ISNAN (this->rv ()))
    return 0;

  tree t = fold_build1 (FIX_TRUNC_EXPR, build_ctype (type),
		       build_float_cst (*this, Type::tfloat80));
  return TREE_INT_CST_LOW (t);
}

/* Same as longdouble::to_int, but returns a uint64_t.  */

uint64_t
longdouble::to_uint (Type *type) const
{
  return (uint64_t) to_int (type);
}

/* Conversion routines between longdouble and real_value.  */

void
longdouble::set (real_value& d)
{
  real_convert (&this->rv (), TYPE_MODE (long_double_type_node), &d);
}

longdouble::operator
real_value& (void)
{
  return this->rv ();
}

/* Conversion routines between longdouble and integer types.  */

void
longdouble::set (int8_t d)
{
  from_int (Type::tfloat32, d);
}

void
longdouble::set (int16_t d)
{
  from_int (Type::tfloat32, d);
}

void
longdouble::set (int32_t d)
{
  from_int (Type::tfloat64, d);
}

void
longdouble::set (int64_t d)
{
  from_int (Type::tfloat80, d);
}

longdouble::operator int8_t (void)
{
  return to_int (Type::tint8);
}

longdouble::operator int16_t (void)
{
  return to_int (Type::tint16);
}

longdouble::operator int32_t (void)
{
  return to_int (Type::tint32);
}

longdouble::operator int64_t (void)
{
  return to_int (Type::tint64);
}

/* Unsigned variants of the same conversion routines.  */

void
longdouble::set (uint8_t d)
{
  from_uint (Type::tfloat32, d);
}

void
longdouble::set (uint16_t d)
{
  from_uint (Type::tfloat32, d);
}

void
longdouble::set (uint32_t d)
{
  from_uint (Type::tfloat64, d);
}

void
longdouble::set (uint64_t d)
{
  from_uint (Type::tfloat80, d);
}

longdouble::operator uint8_t (void)
{
  return to_uint (Type::tuns8);
}

longdouble::operator uint16_t (void)
{
  return to_uint (Type::tuns16);
}

longdouble::operator uint32_t (void)
{
  return to_uint (Type::tuns32);
}

longdouble::operator uint64_t (void)
{
  return to_uint (Type::tuns64);
}

/* For conversion between boolean, only need to check if is zero.  */

void
longdouble::set (bool d)
{
  this->rv () = (d == false) ? dconst0 : dconst1;
}

longdouble::operator
bool (void)
{
  return this->rv ().cl != rvc_zero;
}

/* Overload numeric operators for longdouble types.  */

longdouble
longdouble::operator + (const longdouble& r)
{
  real_value x;
  real_arithmetic (&x, PLUS_EXPR, &this->rv (), &r.rv ());
  return ldouble (x);
}

longdouble
longdouble::operator - (const longdouble& r)
{
  real_value x;
  real_arithmetic (&x, MINUS_EXPR, &this->rv (), &r.rv ());
  return ldouble (x);
}

longdouble
longdouble::operator * (const longdouble& r)
{
  real_value x;
  real_arithmetic (&x, MULT_EXPR, &this->rv (), &r.rv ());
  return ldouble (x);
}

longdouble
longdouble::operator / (const longdouble& r)
{
  real_value x;
  real_arithmetic (&x, RDIV_EXPR, &this->rv (), &r.rv ());
  return ldouble (x);
}

longdouble
longdouble::operator % (const longdouble& r)
{
  real_value q, x;

  if (r.rv ().cl == rvc_zero || REAL_VALUE_ISINF (this->rv ()))
    {
      real_value rvt;
      real_nan (&rvt, "", 1, TYPE_MODE (long_double_type_node));
      return ldouble (rvt);
    }

  if (this->rv ().cl == rvc_zero)
    return *this;

  if (REAL_VALUE_ISINF (r.rv ()))
    return *this;

  /* Need to check for NaN?  */
  real_arithmetic (&q, RDIV_EXPR, &this->rv (), &r.rv ());
  real_arithmetic (&q, FIX_TRUNC_EXPR, &q, NULL);
  real_arithmetic (&q, MULT_EXPR, &q, &r.rv ());
  real_arithmetic (&x, MINUS_EXPR, &this->rv (), &q);

  return ldouble (x);
}

longdouble
longdouble::operator -(void)
{
  real_value x = real_value_negate (&this->rv ());
  return ldouble (x);
}

/* Overload equality operators for longdouble types.  */

bool
longdouble::operator < (const longdouble& r)
{
  return real_compare (LT_EXPR, &this->rv (), &r.rv ());
}

bool
longdouble::operator > (const longdouble& r)
{
  return real_compare (GT_EXPR, &this->rv (), &r.rv ());
}

bool
longdouble::operator <= (const longdouble& r)
{
  return real_compare (LE_EXPR, &this->rv (), &r.rv ());
}

bool
longdouble::operator >= (const longdouble& r)
{
  return real_compare (GE_EXPR, &this->rv (), &r.rv ());
}

bool
longdouble::operator == (const longdouble& r)
{
  return real_compare (EQ_EXPR, &this->rv (), &r.rv ());
}

bool
longdouble::operator != (const longdouble& r)
{
  return real_compare (NE_EXPR, &this->rv (), &r.rv ());
}
