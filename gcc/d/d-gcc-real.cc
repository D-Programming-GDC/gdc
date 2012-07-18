// d-gcc-real.cc -- D frontend for GCC.
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

#include "d-gcc-includes.h"

// d-real_t.cc

#include "mars.h"
#include "lexer.h"
#include "mtype.h"
#include "d-gcc-real.h"

#include "d-lang.h"
#include "d-codegen.h"


static enum machine_mode
machineMode (real_t::Mode mode)
{
  switch (mode)
    {
    case real_t::Float:
      return TYPE_MODE (float_type_node);

    case real_t::Double:
      return TYPE_MODE (double_type_node);

    case real_t::LongDouble:
      return TYPE_MODE (long_double_type_node);

    default:
      gcc_unreachable();
    }
}

real_t_Properties real_t_properties[real_t::NumModes];

#define M_LOG10_2       0.30102999566398119521

void
real_t::init (void)
{
  gcc_assert (sizeof (real_t) >= sizeof (REAL_VALUE_TYPE));

  for (int i = (int) Float; i < (int) NumModes; i++)
    {
      real_t_Properties& p = real_t_properties[i];

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

real_t
real_t::parse (const char *str, Mode mode)
{
  real_t r;
  real_from_string3 (&r.rv(), str, machineMode (mode));
  return r;
}

real_t
real_t::getnan (Mode mode)
{
  real_t r;
  real_nan (&r.rv(), "", 1, machineMode (mode));
  return r;
}

// Same as getnan, except the significand is forced to be a signalling NaN
real_t
real_t::getsnan (Mode mode)
{
  real_t r;
  real_nan (&r.rv(), "", 0, machineMode (mode));
  return r;
}

real_t
real_t::getinfinity (void)
{
  real_t r;
  real_inf (&r.rv());
  return r;
}

const REAL_VALUE_TYPE &
real_t::rv (void) const
{
  REAL_VALUE_TYPE *r = (REAL_VALUE_TYPE *) &this->frv_;
  return *r;
}

REAL_VALUE_TYPE &
real_t::rv (void)
{
  REAL_VALUE_TYPE *r = (REAL_VALUE_TYPE *) &this->frv_;
  return *r;
}

real_t::real_t (const real_t& r)
{
  rv() = r.rv();
}

real_t::real_t (const REAL_VALUE_TYPE& rv)
{
  this->rv() = rv;
}

real_t::real_t (int v)
{
  REAL_VALUE_FROM_INT (rv(), v, (v < 0) ? -1 : 0, machineMode (Float));
}

real_t::real_t (d_uns64 v)
{
  REAL_VALUE_FROM_UNSIGNED_INT (rv(), v, 0, machineMode (Double));
}


real_t::real_t (d_int64 v)
{
  REAL_VALUE_FROM_INT (rv(), v, (v < 0) ? -1 : 0, machineMode (Double));
}

real_t &
real_t::operator= (const real_t& r)
{
  rv() = r.rv();
  return *this;
}

real_t &
real_t::operator= (int v)
{
  REAL_VALUE_FROM_UNSIGNED_INT (rv(), v, (v < 0) ? -1 : 0, machineMode (Float));
  return *this;
}

real_t
real_t::operator+ (const real_t& r)
{
  real_t x;
  REAL_ARITHMETIC (x.rv(), PLUS_EXPR, rv(), r.rv());
  return x;
}

real_t
real_t::operator- (const real_t& r)
{
  real_t x;
  REAL_ARITHMETIC (x.rv(), MINUS_EXPR, rv(), r.rv());
  return x;
}

real_t
real_t::operator- (void)
{
  real_t x;
  x.rv() = real_value_negate (&rv());
  return x;
}

real_t
real_t::operator* (const real_t& r)
{
  real_t x;
  REAL_ARITHMETIC (x.rv(), MULT_EXPR, rv(), r.rv());
  return x;
}

real_t
real_t::operator/ (const real_t& r)
{
  real_t x;
  REAL_ARITHMETIC (x.rv(), RDIV_EXPR, rv(), r.rv());
  return x;
}

// Using darwin fmodl man page for special cases
real_t
real_t::operator% (const real_t& r)
{
  REAL_VALUE_TYPE q, x;
  // %% inf cases..

  // %% signal error?
  if (r.rv().cl == rvc_zero || REAL_VALUE_ISINF (rv()))
    {
      REAL_VALUE_TYPE rvt;
      real_nan (&rvt, "", 1, machineMode (LongDouble));
      return real_t (rvt);
    }

  if (rv().cl == rvc_zero)
    return *this;

  if (REAL_VALUE_ISINF (r.rv()))
    return *this;

  // %% need to check for NaN?
  REAL_ARITHMETIC (q, RDIV_EXPR, rv(), r.rv());
  real_arithmetic (&q, FIX_TRUNC_EXPR, &q, NULL);
  REAL_ARITHMETIC (q, MULT_EXPR, q, r.rv());
  REAL_ARITHMETIC (x, MINUS_EXPR, rv(), q);

  return real_t (x);
}

bool
real_t::operator< (const real_t& r)
{
  return real_compare (LT_EXPR, &rv(), &r.rv());
}

bool
real_t::operator> (const real_t& r)
{
  return real_compare (GT_EXPR, &rv(), &r.rv());
}

bool
real_t::operator<= (const real_t& r)
{
  return real_compare (LE_EXPR, &rv(), &r.rv());
}

bool
real_t::operator>= (const real_t& r)
{
  return real_compare (GE_EXPR, &rv(), &r.rv());
}

bool
real_t::operator== (const real_t& r)
{
  return real_compare (EQ_EXPR, &rv(), &r.rv());
}

bool
real_t::operator!= (const real_t& r)
{
  return real_compare (NE_EXPR, &rv(), &r.rv());
}


d_uns64
real_t::toInt (void) const
{
  HOST_WIDE_INT low, high;
  REAL_VALUE_TYPE r;

  r = rv();
  if (REAL_VALUE_ISNAN (r))
    low = high = 0;
  else
    REAL_VALUE_TO_INT (&low, &high, r);

  return gen.hwi2toli (low, high);
}

d_uns64
real_t::toInt (Type *real_type, Type *int_type) const
{
  tree t;
  double_int cst;
  REAL_VALUE_TYPE r;

  r = rv();
  if (REAL_VALUE_ISNAN (r))
    cst.low = cst.high = 0;
  else
    {
      t = fold_build1 (FIX_TRUNC_EXPR, int_type->toCtype(),
		       gen.floatConstant (r, real_type->toBasetype()));
      // can't use tree_low_cst as it asserts !TREE_OVERFLOW
      cst = TREE_INT_CST (t);
    }
  return gen.hwi2toli (cst);
}

real_t
real_t::convert (Mode to_mode) const
{
  real_t result;
  real_convert (&result.rv(), machineMode (to_mode), &rv());
  return result;
}

real_t
real_t::convert (Type *to_type) const
{
  Type *tb = to_type->toBasetype();
  switch (tb->ty)
    {
    case Tfloat32:
    case Timaginary32:
      return convert (real_t::Float);

    case Tfloat64:
    case Timaginary64:
      return convert (real_t::Double);

    case Tfloat80:
    case Timaginary80:
      return convert (real_t::LongDouble);

    case Tvector:
      tb = ((TypeVector *)tb)->elementType();
      gcc_assert (tb->ty != Tvector);
      return convert (tb);

    default:
      gcc_unreachable();
    }
}

bool
real_t::isConst0 (void)
{
  return REAL_VALUES_EQUAL (rv(), dconst0);
}

bool
real_t::isConst1 (void)
{
  return REAL_VALUES_EQUAL (rv(), dconst1);
}

bool
real_t::isConst2 (void)
{
  return REAL_VALUES_EQUAL (rv(), dconst2);
}

bool
real_t::isConstMinus1 (void)
{
  return REAL_VALUES_EQUAL (rv(), dconstm1);
}

bool
real_t::isConstHalf (void)
{
  return REAL_VALUES_EQUAL (rv(), dconsthalf);
}

bool
real_t::isZero (void)
{
  return rv().cl == rvc_zero;
}

bool
real_t::isNegative (void)
{
  return REAL_VALUE_NEGATIVE (rv());
}

bool
real_t::floatCompare (int op, const real_t& r)
{
  enum tree_code out;

  switch ((enum TOK) op)
    {
    case TOKleg:
	{
	  //n = r1 <>= r2;
	  out = ORDERED_EXPR; break;
	}
    case TOKlg:
	{
	  // n = r1 <> r2;
	  return *this < r || *this > r;
	}
    case TOKunord:
	{
	  // n = r1 !<>= r2;
	  out = UNORDERED_EXPR; break;
	}
    case TOKue:
	{
	  // n = r1 !<> r2;
	  out = UNEQ_EXPR; break;
	}
    case TOKug:
	{
	  // n = r1 !<= r2;
	  out = UNGT_EXPR; break;
	}
    case TOKuge:
	{
	  // n = r1 !< r2;
	  out = UNGE_EXPR; break;
	}
    case TOKul:
	{
	  // n = r1 !>= r2;
	  out = UNLT_EXPR; break;
	}
    case TOKule:
	{
	  // n = r1 !> r2;
	  out = UNLE_EXPR; break;
	}
    default:
      gcc_unreachable();
    }
  return real_compare (out, &rv(), &r.rv());
}

bool
real_t::isIdenticalTo (const real_t& r) const
{
  return REAL_VALUES_IDENTICAL (rv(), r.rv());
}

void
real_t::format (char *buf, unsigned buf_size) const
{
  // %% restricting the precision of significant digits to 18.
  real_to_decimal (buf, &rv(), buf_size, 18, 1);
}

void
real_t::formatHex (char *buf, unsigned buf_size) const
{
  real_to_hexadecimal (buf, &rv(), buf_size, 0, 1);
}

bool
real_t::isInf (void)
{
  return REAL_VALUE_ISINF (rv());
}

bool
real_t::isNan (void)
{
  return REAL_VALUE_ISNAN (rv());
}

bool
real_t::isSignallingNan (void)
{
  // Same as isNan, but also check if is signalling.
  return REAL_VALUE_ISNAN (rv()) && rv().signalling;
}

bool
real_t::isConversionExact (Mode to_mode) const
{
  return exact_real_truncate (machineMode (to_mode), &rv());
}

void
real_t::dump (void)
{
  char buf[128];
  format (buf, sizeof (buf));
  fprintf (stderr, "%s\n", buf);
}

