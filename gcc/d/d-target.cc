/* d-target.cc -- Target interface for the D front end.
   Copyright (C) 2013-2018 Free Software Foundation, Inc.

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

#include "dfrontend/aggregate.h"
#include "dfrontend/module.h"
#include "dfrontend/mtype.h"
#include "dfrontend/tokens.h"
#include "dfrontend/target.h"

#include "d-system.h"

#include "d-tree.h"
#include "d-frontend.h"
#include "d-target.h"

/* Implements the Target interface defined by the front end.
   Used for retrieving target-specific information.  */

/* Type size information used by frontend.  */
int Target::ptrsize;
int Target::c_longsize;
int Target::realsize;
int Target::realpad;
int Target::realalignsize;
bool Target::reverseCppOverloads;
bool Target::cppExceptions;
int Target::classinfosize;
unsigned long long Target::maxStaticDataSize;

/* Floating point constants for for .max, .min, and other properties.  */
template <typename T> real_t Target::FPTypeProperties<T>::max;
template <typename T> real_t Target::FPTypeProperties<T>::min_normal;
template <typename T> real_t Target::FPTypeProperties<T>::nan;
template <typename T> real_t Target::FPTypeProperties<T>::snan;
template <typename T> real_t Target::FPTypeProperties<T>::infinity;
template <typename T> real_t Target::FPTypeProperties<T>::epsilon;
template <typename T> d_int64 Target::FPTypeProperties<T>::dig;
template <typename T> d_int64 Target::FPTypeProperties<T>::mant_dig;
template <typename T> d_int64 Target::FPTypeProperties<T>::max_exp;
template <typename T> d_int64 Target::FPTypeProperties<T>::min_exp;
template <typename T> d_int64 Target::FPTypeProperties<T>::max_10_exp;
template <typename T> d_int64 Target::FPTypeProperties<T>::min_10_exp;


/* Initialize the floating point constants for TYPE.  */

template <typename T>
static void
define_float_constants (tree type)
{
  const double log10_2 = 0.30102999566398119521;
  char buf[128];

  /* Get backend real mode format.  */
  const machine_mode mode = TYPE_MODE (type);
  const real_format *fmt = REAL_MODE_FORMAT (mode);

  /* The largest representable value that's not infinity.  */
  get_max_float (fmt, buf, sizeof (buf));
  real_from_string (&T::max.rv (), buf);

  /* The smallest representable normalized value that's not 0.  */
  snprintf (buf, sizeof (buf), "0x1p%d", fmt->emin - 1);
  real_from_string (&T::min_normal.rv (), buf);

  /* Floating-point NaN.  */
  real_nan (&T::nan.rv (), "", 1, mode);

  /* Signalling floating-point NaN.  */
  real_nan (&T::snan.rv (), "", 0, mode);

  /* Floating-point +Infinity if the target supports infinities.  */
  real_inf (&T::infinity.rv ());

  /* The smallest increment to the value 1.  */
  if (fmt->pnan < fmt->p)
    snprintf (buf, sizeof (buf), "0x1p%d", fmt->emin - fmt->p);
  else
    snprintf (buf, sizeof (buf), "0x1p%d", 1 - fmt->p);
  real_from_string (&T::epsilon.rv (), buf);

  /* The number of decimal digits of precision.  */
  T::dig = (fmt->p - 1) * log10_2;

  /* The number of bits in mantissa.  */
  T::mant_dig = fmt->p;

  /* The maximum int value such that 2** (value-1) is representable.  */
  T::max_exp = fmt->emax;

  /* The minimum int value such that 2** (value-1) is representable as a
     normalized value.  */
  T::min_exp = fmt->emin;

  /* The maximum int value such that 10**value is representable.  */
  T::max_10_exp = fmt->emax * log10_2;

  /* The minimum int value such that 10**value is representable as a
     normalized value.  */
  T::min_10_exp = (fmt->emin - 1) * log10_2;
}

/* Initialize all variables of the Target structure.  */

void
Target::_init (void)
{
  /* Map D frontend type and sizes to GCC backend types.  */
  Target::realsize = int_size_in_bytes (long_double_type_node);
  Target::realpad = (Target::realsize -
		     (TYPE_PRECISION (long_double_type_node) / BITS_PER_UNIT));
  Target::realalignsize = TYPE_ALIGN_UNIT (long_double_type_node);
  Target::reverseCppOverloads = false;
  Target::cppExceptions = true;

  /* Allow data sizes up to half of the address space.  */
  Target::maxStaticDataSize = tree_to_shwi (TYPE_MAXVAL (ptrdiff_type_node));

  /* Define what type to use for size_t, ptrdiff_t.  */
  if (POINTER_SIZE == 64)
    {
      global.params.isLP64 = true;
      Tsize_t = Tuns64;
      Tptrdiff_t = Tint64;
    }
  else
    {
      Tsize_t = Tuns32;
      Tptrdiff_t = Tint32;
    }

  Type::tsize_t = Type::basic[Tsize_t];
  Type::tptrdiff_t = Type::basic[Tptrdiff_t];
  Type::thash_t = Type::tsize_t;

  Target::ptrsize = (POINTER_SIZE / BITS_PER_UNIT);
  Target::c_longsize = int_size_in_bytes (long_integer_type_node);

  Target::classinfosize = 19 * Target::ptrsize;

  /* Initialize all compile-time properties for floating point types.
     Should ensure that our real_t type is able to represent real_value.  */
  gcc_assert (sizeof (real_t) >= sizeof (real_value));

  define_float_constants <Target::FloatProperties> (float_type_node);
  define_float_constants <Target::DoubleProperties> (double_type_node);
  define_float_constants <Target::RealProperties> (long_double_type_node);

  /* Commonly used floating point constants.  */
  const machine_mode mode = TYPE_MODE (long_double_type_node);
  real_convert (&CTFloat::zero.rv (), mode, &dconst0);
  real_convert (&CTFloat::one.rv (), mode, &dconst1);
  real_convert (&CTFloat::minusone.rv (), mode, &dconstm1);
  real_convert (&CTFloat::half.rv (), mode, &dconsthalf);
}

/* Return GCC memory alignment size for type TYPE.  */

unsigned
Target::alignsize (Type *type)
{
  gcc_assert (type->isTypeBasic ());
  return TYPE_ALIGN_UNIT (build_ctype (type));
}

/* Return GCC field alignment size for type TYPE.  */

unsigned
Target::fieldalign (Type *type)
{
  /* Work out the correct alignment for the field decl.  */
  tree field = make_node (FIELD_DECL);
  DECL_ALIGN (field) = type->alignsize () * BITS_PER_UNIT;

#ifdef BIGGEST_FIELD_ALIGNMENT
  DECL_ALIGN (field) = MIN (DECL_ALIGN (field),
			    (unsigned) BIGGEST_FIELD_ALIGNMENT);
#endif
#ifdef ADJUST_FIELD_ALIGN
  if (type->isTypeBasic ())
    {
      TREE_TYPE (field) = build_ctype (type);
      DECL_ALIGN (field) = ADJUST_FIELD_ALIGN (field, DECL_ALIGN (field));
    }
#endif

  /* Also controlled by -fpack-struct=  */
  if (maximum_field_alignment)
    DECL_ALIGN (field) = MIN (DECL_ALIGN (field), maximum_field_alignment);

  return DECL_ALIGN_UNIT (field);
}

/* Return size of OS critical section.
   Can't use the sizeof () calls directly since cross compiling is supported
   and would end up using the host sizes rather than the target sizes.  */

unsigned
Target::critsecsize (void)
{
  return targetdm.d_critsec_size ();
}

/* Returns a Type for the va_list type of the target.  */

Type *
Target::va_listType (void)
{
  return Type::tvalist;
}

/* Perform a reinterpret cast of EXPR to type TYPE for use in CTFE.
   The front end should have already ensured that EXPR is a constant,
   so we just lower the value to GCC and return the converted CST.  */

Expression *
Target::paintAsType (Expression *expr, Type *type)
{
  /* We support up to 512-bit values.  */
  unsigned char buffer[64];
  tree cst;

  Type *tb = type->toBasetype ();

  if (expr->type->isintegral ())
    cst = build_integer_cst (expr->toInteger (), build_ctype (expr->type));
  else if (expr->type->isfloating ())
    cst = build_float_cst (expr->toReal (), expr->type);
  else if (expr->op == TOKarrayliteral)
    {
      /* Build array as VECTOR_CST, assumes EXPR is constant.  */
      Expressions *elements = ((ArrayLiteralExp *) expr)->elements;
      vec<constructor_elt, va_gc> *elms = NULL;

      vec_safe_reserve (elms, elements->dim);
      for (size_t i = 0; i < elements->dim; i++)
	{
	  Expression *e = (*elements)[i];
	  if (e->type->isintegral ())
	    {
	      tree value = build_integer_cst (e->toInteger (),
					      build_ctype (e->type));
	      CONSTRUCTOR_APPEND_ELT (elms, size_int (i), value);
	    }
	  else if (e->type->isfloating ())
	    {
	      tree value = build_float_cst (e->toReal (), e->type);
	      CONSTRUCTOR_APPEND_ELT (elms, size_int (i), value);
	    }
	  else
	    gcc_unreachable ();
	}

      /* Build vector type.  */
      int nunits = ((TypeSArray *) expr->type)->dim->toUInteger ();
      Type *telem = expr->type->nextOf ();
      tree vectype = build_vector_type (build_ctype (telem), nunits);

      cst = build_vector_from_ctor (vectype, elms);
    }
  else
    gcc_unreachable ();

  /* Encode CST to buffer.  */
  int len = native_encode_expr (cst, buffer, sizeof (buffer));

  if (tb->ty == Tsarray)
    {
      /* Interpret value as a vector of the same size,
	 then return the array literal.  */
      int nunits = ((TypeSArray *) type)->dim->toUInteger ();
      Type *elem = type->nextOf ();
      tree vectype = build_vector_type (build_ctype (elem), nunits);

      cst = native_interpret_expr (vectype, buffer, len);

      Expression *e = d_eval_constant_expression (cst);
      gcc_assert (e != NULL && e->op == TOKvector);

      return ((VectorExp *) e)->e1;
    }
  else
    {
      /* Normal interpret cast.  */
      cst = native_interpret_expr (build_ctype (type), buffer, len);

      Expression *e = d_eval_constant_expression (cst);
      gcc_assert (e != NULL);

      return e;
    }
}

/* Check imported module M for any special processing.
   Modules we look out for are:
    - object: For D runtime type information.
    - gcc.builtins: For all gcc builtins.
    - core.stdc.*: For all gcc library builtins.  */

void
Target::loadModule (Module *m)
{
  ModuleDeclaration *md = m->md;

  if (!md || !md->id || !md->packages)
    {
      Identifier *id = (md && md->id) ? md->id : m->ident;
      if (!strcmp (id->toChars (), "object"))
	create_tinfo_types (m);
    }
  else if (md->packages->dim == 1)
    {
      if (!strcmp ((*md->packages)[0]->toChars (), "gcc")
	  && !strcmp (md->id->toChars (), "builtins"))
	d_build_builtins_module (m);
    }
  else if (md->packages->dim == 2)
    {
      if (!strcmp ((*md->packages)[0]->toChars (), "core")
	  && !strcmp ((*md->packages)[1]->toChars (), "stdc"))
	d_add_builtin_module (m);
    }
}

/* Checks whether the target supports a vector type with total size SZ
   (in bytes) and element type TYPE.  */

int
Target::isVectorTypeSupported (int sz, Type *type)
{
  /* Size must be greater than zero, and a power of two.  */
  if (sz <= 0 || sz & (sz - 1))
    return 2;

  /* __vector(void[]) is treated same as __vector(ubyte[])  */
  if (type == Type::tvoid)
    type = Type::tuns8;

  /* No support for non-trivial types.  */
  if (!type->isTypeBasic ())
    return 3;

  /* If there is no hardware support, check if we an safely emulate it.  */
  tree ctype = build_ctype (type);
  machine_mode mode = TYPE_MODE (ctype);

  if (!targetm.vector_mode_supported_p (mode)
      && !targetm.scalar_mode_supported_p (mode))
    return 3;

  return 0;
}

/* Checks whether the target supports operation OP for vectors of type TYPE.
   For binary ops T2 is the type of the right-hand operand.
   Returns true if the operation is supported or type is not a vector.  */

bool
Target::isVectorOpSupported (Type *type, TOK op, Type *)
{
  if (type->ty != Tvector)
    return true;

  /* Don't support if type is non-scalar, such as __vector(void[]).  */
  if (!type->isscalar())
    return false;

  /* Don't support if expression cannot be represented.  */
  switch (op)
    {
    case TOKpow:
    case TOKpowass:
      /* pow() is lowered as a function call.  */
      return false;

    case TOKmod:
    case TOKmodass:
      /* fmod() is lowered as a function call.  */
      if (type->isfloating())
	return false;
      break;

    case TOKandand:
    case TOKoror:
      /* Logical operators must have a result type of bool.  */
      return false;

    case TOKue:
    case TOKlg:
    case TOKule:
    case TOKul:
    case TOKuge:
    case TOKug:
    case TOKle:
    case TOKlt:
    case TOKge:
    case TOKgt:
    case TOKleg:
    case TOKunord:
    case TOKequal:
    case TOKnotequal:
    case TOKidentity:
    case TOKnotidentity:
      /* Comparison operators must have a result type of bool.  */
      return false;

    default:
      break;
    }

  return true;
}

/* Apply any target-specific prefixes based on the given linkage.  */

void
Target::prefixName (OutBuffer *, LINK)
{
}

/* Return the symbol mangling of S for C++ linkage. */

const char *
Target::toCppMangle (Dsymbol *s)
{
  return toCppMangleItanium (s);
}

/* Return the symbol mangling of CD for C++ linkage.  */

const char *
Target::cppTypeInfoMangle(ClassDeclaration *cd)
{
  return cppTypeInfoMangleItanium (cd);
}

/* For a vendor-specific type, return a string containing the C++ mangling.
   In all other cases, return NULL.  */

const char *
Target::cppTypeMangle (Type *type)
{
    if (type->isTypeBasic () || type->ty == Tvector || type->ty == Tstruct)
    {
        tree ctype = build_ctype (type);
        return targetm.mangle_type (ctype);
    }

    return NULL;
}

/* Return the default system linkage for the target.  */

LINK
Target::systemLinkage (void)
{
  return LINKc;
}
