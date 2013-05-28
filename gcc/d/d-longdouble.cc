// d-longdouble.cc -- D frontend for GCC.
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

#include "d-system.h"

#include "lexer.h"
#include "mtype.h"
#include "longdouble.h"

#include "d-lang.h"
#include "d-codegen.h"


// Return backend machine_mode for frontend mode MODE.

static enum machine_mode
machineMode (longdouble::Mode mode)
{
  switch (mode)
    {
    case longdouble::Float:
      return TYPE_MODE (float_type_node);

    case longdouble::Double:
      return TYPE_MODE (double_type_node);

    case longdouble::LongDouble:
      return TYPE_MODE (long_double_type_node);

    default:
      gcc_unreachable();
    }
}

real_properties real_limits[longdouble::NumModes];

#define M_LOG10_2       0.30102999566398119521

// Initialise D floating point property values.

void
longdouble::init (void)
{
  gcc_assert (sizeof (longdouble) >= sizeof (REAL_VALUE_TYPE));

  for (int i = (int) Float; i < (int) NumModes; i++)
    {
      real_properties& p = real_limits[i];

      enum machine_mode mode = machineMode ((Mode) i);
      const struct real_format& rf = *REAL_MODE_FORMAT (mode);
      char buf[128];

      /* .max:
	 The largest representable value that's not infinity.  */
      get_max_float (&rf, buf, sizeof (buf));
      real_from_string (&p.maxval.rv(), buf);

      /* .min, .min_normal:
	 The smallest representable normalized value that's not 0.  */
      snprintf (buf, sizeof (buf), "0x1p%d", rf.emin - 1);
      real_from_string (&p.minval.rv(), buf);

      /* .epsilon:
	 The smallest increment to the value 1.  */
      if (rf.pnan < rf.p)
	snprintf (buf, sizeof (buf), "0x1p%d", rf.emin - rf.p);
      else
	snprintf (buf, sizeof (buf), "0x1p%d", 1 - rf.p);
      real_from_string (&p.epsilonval.rv(), buf);

      /* .dig:
	 The number of decimal digits of precision.  */
      p.dig = (rf.p - 1) * M_LOG10_2;

      /* .mant_dig:
	 The number of bits in mantissa.  */
      p.mant_dig = rf.p;

      /* .max_10_exp:
	 The maximum int value such that 10**value is representable.  */
      p.max_10_exp = rf.emax * M_LOG10_2;

      /* .min_10_exp:
	 The minimum int value such that 10**value is representable as a normalized value.  */
      p.min_10_exp = (rf.emin - 1) * M_LOG10_2;

      /* .max_exp:
	 The maximum int value such that 2** (value-1) is representable.  */
      p.max_exp = rf.emax;

      /* .min_exp:
	 The minimum int value such that 2** (value-1) is representable as a normalized value.  */
      p.min_exp = rf.emin;
    }
}

// Return a longdouble value from string STR of type MODE.

longdouble
longdouble::parse (const char *str, Mode mode)
{
  longdouble r;
  real_from_string3 (&r.rv(), str, machineMode (mode));
  return r;
}

// Return the hidden REAL_VALUE_TYPE from the longdouble type.

const REAL_VALUE_TYPE &
longdouble::rv (void) const
{
  const REAL_VALUE_TYPE *r = (const REAL_VALUE_TYPE *) &this->frv_;
  return *r;
}

REAL_VALUE_TYPE &
longdouble::rv (void)
{
  REAL_VALUE_TYPE *r = (REAL_VALUE_TYPE *) &this->frv_;
  return *r;
}

// Construct a new longdouble from longdouble value R.

longdouble::longdouble (const longdouble& r)
{
  rv() = r.rv();
}

// Construct a new longdouble from REAL_VALUE_TYPE RV.

longdouble::longdouble (const REAL_VALUE_TYPE& rv)
{
  real_convert (&this->rv(), TYPE_MODE (long_double_type_node), &rv);
}

// Construct a new longdouble from int V.

longdouble::longdouble (int v)
{
  REAL_VALUE_FROM_INT (rv(), v, (v < 0) ? -1 : 0, TYPE_MODE (double_type_node));
}

// Construct a new longdouble from uint64_t V.

longdouble::longdouble (uint64_t v)
{
  REAL_VALUE_FROM_UNSIGNED_INT (rv(), v, 0, TYPE_MODE (long_double_type_node));
}

// Construct a new longdouble from int64_t V.

longdouble::longdouble (int64_t v)
{
  REAL_VALUE_FROM_INT (rv(), v, (v < 0) ? -1 : 0, TYPE_MODE (long_double_type_node));
}

// Construct a new longdouble from double D.

longdouble::longdouble (double d)
{
  char buf[48];
  snprintf(buf, sizeof (buf), "%lf", d);
  real_from_string3 (&rv(), buf, TYPE_MODE (long_double_type_node));
}

// Overload assignment operator for longdouble types.

longdouble &
longdouble::operator= (const longdouble& r)
{
  rv() = r.rv();
  return *this;
}

longdouble &
longdouble::operator= (int v)
{
  REAL_VALUE_FROM_UNSIGNED_INT (rv(), v, (v < 0) ? -1 : 0, TYPE_MODE (double_type_node));
  return *this;
}

// Overload numeric operators for longdouble types.

longdouble
longdouble::operator+ (const longdouble& r)
{
  REAL_VALUE_TYPE x;
  REAL_ARITHMETIC (x, PLUS_EXPR, rv(), r.rv());
  return longdouble (x);
}

longdouble
longdouble::operator- (const longdouble& r)
{
  REAL_VALUE_TYPE x;
  REAL_ARITHMETIC (x, MINUS_EXPR, rv(), r.rv());
  return longdouble (x);
}

longdouble
longdouble::operator- (void)
{
  REAL_VALUE_TYPE x = real_value_negate (&rv());
  return longdouble (x);
}

longdouble
longdouble::operator* (const longdouble& r)
{
  REAL_VALUE_TYPE x;
  REAL_ARITHMETIC (x, MULT_EXPR, rv(), r.rv());
  return longdouble (x);
}

longdouble
longdouble::operator/ (const longdouble& r)
{
  REAL_VALUE_TYPE x;
  REAL_ARITHMETIC (x, RDIV_EXPR, rv(), r.rv());
  return longdouble (x);
}

longdouble
longdouble::operator% (const longdouble& r)
{
  REAL_VALUE_TYPE q, x;

  if (r.rv().cl == rvc_zero || REAL_VALUE_ISINF (rv()))
    {
      REAL_VALUE_TYPE rvt;
      real_nan (&rvt, "", 1, TYPE_MODE (long_double_type_node));
      return longdouble (rvt);
    }

  if (rv().cl == rvc_zero)
    return *this;

  if (REAL_VALUE_ISINF (r.rv()))
    return *this;

  // Need to check for NaN?
  REAL_ARITHMETIC (q, RDIV_EXPR, rv(), r.rv());
  real_arithmetic (&q, FIX_TRUNC_EXPR, &q, NULL);
  REAL_ARITHMETIC (q, MULT_EXPR, q, r.rv());
  REAL_ARITHMETIC (x, MINUS_EXPR, rv(), q);

  return longdouble (x);
}

// Overload equality operators for longdouble types.

bool
longdouble::operator< (const longdouble& r)
{
  return real_compare (LT_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator> (const longdouble& r)
{
  return real_compare (GT_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator<= (const longdouble& r)
{
  return real_compare (LE_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator>= (const longdouble& r)
{
  return real_compare (GE_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator== (const longdouble& r)
{
  return real_compare (EQ_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator!= (const longdouble& r)
{
  return real_compare (NE_EXPR, &rv(), &r.rv());
}

// Return conversion of longdouble value to uint64_t.

uint64_t
longdouble::toInt (void) const
{
  HOST_WIDE_INT low, high;
  REAL_VALUE_TYPE r;

  r = rv();
  if (REAL_VALUE_ISNAN (r))
    low = high = 0;
  else
    REAL_VALUE_TO_INT (&low, &high, r);

  return cst_to_hwi (double_int::from_pair (high, low));
}

// Return conversion of longdouble value to uint64_t.
// Value is converted from real type RT to int type IT.

uint64_t
longdouble::toInt (Type *rt, Type *it) const
{
  tree t;
  double_int cst;
  REAL_VALUE_TYPE r = rv();

  if (REAL_VALUE_ISNAN (r))
    cst.low = cst.high = 0;
  else
    {
      t = fold_build1 (FIX_TRUNC_EXPR, it->toCtype(),
		       build_float_cst (r, rt->toBasetype()));
      // can't use tree_low_cst as it asserts !TREE_OVERFLOW
      cst = TREE_INT_CST (t);
    }
  return cst_to_hwi (cst);
}

// Returns TRUE if longdouble value is zero.

bool
longdouble::isZero (void)
{
  return rv().cl == rvc_zero;
}

// Returns TRUE if longdouble value is negative.

bool
longdouble::isNegative (void)
{
  return REAL_VALUE_NEGATIVE (rv());
}

// Returns TRUE if longdouble value is identical to R.

bool
longdouble::isIdenticalTo (const longdouble& r) const
{
  return REAL_VALUES_IDENTICAL (rv(), r.rv());
}

// Format longdouble value into decimal string BUF of size BUF_SIZE.

int
longdouble::format (char *buf, unsigned buf_size) const
{
  // %% restricting the precision of significant digits to 18.
  real_to_decimal (buf, &rv(), buf_size, 18, 1);
  return strlen (buf);
}

// Format longdouble value into hex string BUF of size BUF_SIZE.

int
longdouble::formatHex (char *buf, unsigned buf_size) const
{
  real_to_hexadecimal (buf, &rv(), buf_size, 0, 1);
  return strlen (buf);
}

// Dump value of longdouble for debugging purposes.

void
longdouble::dump (void)
{
  char buf[128];
  format (buf, sizeof (buf));
  fprintf (stderr, "%s\n", buf);
}

