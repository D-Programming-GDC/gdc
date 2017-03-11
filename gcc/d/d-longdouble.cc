// d-longdouble.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/lexer.h"
#include "dfrontend/mtype.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/target.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "stor-layout.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "longdouble.h"


// Return the hidden real_value from the longdouble type.

const real_value &
longdouble::rv() const
{
  return *(const real_value *) this;
}

real_value &
longdouble::rv()
{
  return *(real_value *) this;
}

// Return conversion of signed integer value D to longdouble.
// Conversion is done at precision mode of TYPE.

longdouble
longdouble::from_int(Type *type, int64_t d)
{
  tree t = build_ctype(type);
  real_from_integer(&rv(), TYPE_MODE (t), d, SIGNED);
  return *this;
}

// Return conversion of unsigned integer value D to longdouble.
// Conversion is done at precision mode of TYPE.

longdouble
longdouble::from_uint(Type *type, uint64_t d)
{
  tree t = build_ctype(type);
  real_from_integer(&rv(), TYPE_MODE (t), d, UNSIGNED);
  return *this;
}

// Return conversion of longdouble value to int64_t.
// Conversion is done at precision mode of TYPE.

int64_t
longdouble::to_int(Type *type) const
{
  if (REAL_VALUE_ISNAN (rv()))
    return 0;

  tree t = fold_build1(FIX_TRUNC_EXPR, build_ctype(type),
		       build_float_cst(*this, Type::tfloat64));
  return TREE_INT_CST_LOW (t);
}

// Same as longdouble::to_int, but returns a uint64_t.

uint64_t
longdouble::to_uint(Type *type) const
{
  return (uint64_t) to_int(type);
}

// Helper functions which set longdouble to value D.

void
longdouble::set(real_value& r)
{
  real_convert(&rv(), TYPE_MODE (long_double_type_node), &r);
}

longdouble::operator
real_value&()
{
  return rv();
}

// Conversion routines between longdouble and host float types.

void
longdouble::set(float d)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%f", d);
  real_from_string3(&rv(), buf, TYPE_MODE (double_type_node));
}

void
longdouble::set(double d)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%lf", d);
  real_from_string3(&rv(), buf, TYPE_MODE (long_double_type_node));
}

// These functions should never be called.

longdouble::operator
float()
{
  gcc_unreachable();
}

longdouble::operator
double()
{
  gcc_unreachable();
}

// For conversion between boolean, only need to check if is zero.

void
longdouble::set(bool d)
{
  rv() = (d == false) ? dconst0 : dconst1;
}

longdouble::operator
bool()
{
  return rv().cl != rvc_zero;
}

// Conversion routines between longdouble and integer types.

void longdouble::set(int8_t d)  { from_int(Type::tfloat32, d); }
void longdouble::set(int16_t d) { from_int(Type::tfloat32, d); }
void longdouble::set(int32_t d) { from_int(Type::tfloat64, d); }
void longdouble::set(int64_t d) { from_int(Type::tfloat80, d); }

longdouble::operator int8_t()  { return to_int(Type::tint8); }
longdouble::operator int16_t() { return to_int(Type::tint16); }
longdouble::operator int32_t() { return to_int(Type::tint32); }
longdouble::operator int64_t() { return to_int(Type::tint64); }

void longdouble::set(uint8_t d)  { from_uint(Type::tfloat32, d); }
void longdouble::set(uint16_t d) { from_uint(Type::tfloat32, d); }
void longdouble::set(uint32_t d) { from_uint(Type::tfloat64, d); }
void longdouble::set(uint64_t d) { from_uint(Type::tfloat80, d); }

longdouble::operator uint8_t()  { return to_uint(Type::tuns8); }
longdouble::operator uint16_t() { return to_uint(Type::tuns16); }
longdouble::operator uint32_t() { return to_uint(Type::tuns32); }
longdouble::operator uint64_t() { return to_uint(Type::tuns64); }

// Overload numeric operators for longdouble types.

longdouble
longdouble::operator + (const longdouble& r)
{
  real_value x;
  real_arithmetic(&x, PLUS_EXPR, &rv(), &r.rv());
  return ldouble(x);
}

longdouble
longdouble::operator - (const longdouble& r)
{
  real_value x;
  real_arithmetic(&x, MINUS_EXPR, &rv(), &r.rv());
  return ldouble(x);
}

longdouble
longdouble::operator * (const longdouble& r)
{
  real_value x;
  real_arithmetic(&x, MULT_EXPR, &rv(), &r.rv());
  return ldouble(x);
}

longdouble
longdouble::operator / (const longdouble& r)
{
  real_value x;
  real_arithmetic(&x, RDIV_EXPR, &rv(), &r.rv());
  return ldouble(x);
}

longdouble
longdouble::operator % (const longdouble& r)
{
  real_value q, x;

  if (r.rv().cl == rvc_zero || REAL_VALUE_ISINF (rv()))
    {
      real_value rvt;
      real_nan(&rvt, "", 1, TYPE_MODE (long_double_type_node));
      return ldouble(rvt);
    }

  if (rv().cl == rvc_zero)
    return *this;

  if (REAL_VALUE_ISINF (r.rv()))
    return *this;

  // Need to check for NaN?
  real_arithmetic(&q, RDIV_EXPR, &rv(), &r.rv());
  real_arithmetic(&q, FIX_TRUNC_EXPR, &q, NULL);
  real_arithmetic(&q, MULT_EXPR, &q, &r.rv());
  real_arithmetic(&x, MINUS_EXPR, &rv(), &q);

  return ldouble(x);
}

longdouble
longdouble::operator -()
{
  real_value x = real_value_negate(&rv());
  return ldouble(x);
}

// Overload equality operators for longdouble types.

bool
longdouble::operator < (const longdouble& r)
{
  return real_compare(LT_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator > (const longdouble& r)
{
  return real_compare(GT_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator <= (const longdouble& r)
{
  return real_compare(LE_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator >= (const longdouble& r)
{
  return real_compare(GE_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator == (const longdouble& r)
{
  return real_compare(EQ_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator != (const longdouble& r)
{
  return real_compare(NE_EXPR, &rv(), &r.rv());
}

// Format longdouble value into decimal string BUF of size BUF_SIZE.

int
longdouble::format(char *buf, unsigned buf_size) const
{
  // %% Restricting the precision of significant digits to 18.
  real_to_decimal(buf, &rv(), buf_size, 18, 1);
  return strlen(buf);
}

// Format longdouble value into hex string BUF of size BUF_SIZE,
// converting the result to uppercase if FMT requests it.

int
longdouble::formatHex(char fmt, char *buf, unsigned buf_size) const
{
  real_to_hexadecimal(buf, &rv(), buf_size, 0, 1);
  int buflen;

  switch (fmt)
    {
    case 'A':
      buflen = strlen(buf);
      for (int i = 0; i < buflen; i++)
	buf[i] = TOUPPER(buf[i]);

      return buflen;

    case 'a':
      return strlen(buf);

    default:
      gcc_unreachable();
    }
}

// Dump value of longdouble for debugging purposes.

void
longdouble::dump()
{
  char buf[128];
  format(buf, sizeof(buf));
  fprintf(global.stdmsg, "%s\n", buf);
}

/* Compile-time floating-pointer helper functions.  */

real_t
CTFloat::fabs (real_t r)
{
  real_value x = real_value_abs (&r.rv ());
  return ldouble (x);
}

/* Returns TRUE if longdouble value X is identical to Y.  */

bool
CTFloat::isIdentical (real_t x, real_t y)
{
  real_value rx = x.rv ();
  real_value ry = y.rv ();
  return (REAL_VALUE_ISNAN (rx) && REAL_VALUE_ISNAN (ry))
    || real_identical (&rx, &ry);
}

/* Returns TRUE if real_t value R is NaN.  */

bool
CTFloat::isNaN (real_t r)
{
  return REAL_VALUE_ISNAN (r.rv ());
}

/* Same as isNaN, but also check if is signalling.  */

bool
CTFloat::isSNaN (real_t r)
{
  return REAL_VALUE_ISSIGNALING_NAN (r.rv ());
}

/* Returns TRUE if real_t value is +Inf.  */

bool
CTFloat::isInfinity (real_t r)
{
  return REAL_VALUE_ISINF (r.rv ());
}

/* Return a real_t value from string BUFFER rounded to long double mode.  */

real_t
CTFloat::parse(const char *buffer, bool *overflow)
{
  real_t r;
  real_from_string3(&r.rv(), buffer, TYPE_MODE (long_double_type_node));

  /* Front-end checks overflow to see if the value is representable.  */
  if (overflow && r == Target::RealProperties::infinity)
    *overflow = true;

  return r;
}

/* Format the real_t value X to string BUFFER based on FMT.  */

int
CTFloat::sprint (char *buffer, char fmt, real_t x)
{
  if (fmt == 'a' || fmt == 'A')
    return x.formatHex (fmt, buffer, 32);

  return x.format (buffer, 32);
}
