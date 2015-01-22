// d-target.cc -- D frontend for GCC.
// Copyright (C) 2013 Software Foundation, Inc.

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
#include "d-lang.h"
#include "d-codegen.h"

#include "aggregate.h"
#include "mtype.h"
#include "dfrontend/target.h"

int Target::ptrsize;
int Target::realsize;
int Target::realpad;
int Target::realalignsize;
bool Target::reverseCppOverloads;


void
Target::init()
{
  // Map D frontend type and sizes to GCC backend types.
  realsize = int_size_in_bytes(long_double_type_node);
  realpad = TYPE_PRECISION(long_double_type_node) / BITS_PER_UNIT;
  realalignsize = TYPE_ALIGN_UNIT(long_double_type_node);
  reverseCppOverloads = false;

  // Define what type to use for size_t, ptrdiff_t.
  size_t wordsize = int_size_in_bytes(size_type_node);
  if (wordsize == 2)
    Tsize_t = Tuns16;
  else if (wordsize == 4)
    Tsize_t = Tuns32;
  else if (wordsize == 8)
    Tsize_t = Tuns64;
  else
    gcc_unreachable();

  if (POINTER_SIZE == 32)
    Tptrdiff_t = Tint32;
  else if (POINTER_SIZE == 64)
    Tptrdiff_t = Tint64;
  else
    gcc_unreachable();

  ptrsize = (POINTER_SIZE / BITS_PER_UNIT);

  CLASSINFO_SIZE = 19 * ptrsize;
}

// Return GCC memory alignment size for type TYPE.

unsigned
Target::alignsize(Type *type)
{
  gcc_assert(type->isTypeBasic());
  return TYPE_ALIGN_UNIT(type->toCtype());
}


// Return GCC field alignment size for type TYPE.

unsigned
Target::fieldalign(Type *type)
{
  // Work out the correct alignment for the field decl.
  tree field = make_node(FIELD_DECL);
  DECL_ALIGN(field) = type->alignsize() * BITS_PER_UNIT;

#ifdef BIGGEST_FIELD_ALIGNMENT
  DECL_ALIGN(field)
    = MIN(DECL_ALIGN(field), (unsigned) BIGGEST_FIELD_ALIGNMENT);
#endif
#ifdef ADJUST_FIELD_ALIGN
  if (type->isTypeBasic())
    {
      TREE_TYPE(field) = type->toCtype();
      DECL_ALIGN(field) = ADJUST_FIELD_ALIGN(field, DECL_ALIGN(field));
    }
#endif

  // Also controlled by -fpack-struct=
  if (maximum_field_alignment)
    DECL_ALIGN(field) = MIN(DECL_ALIGN(field), maximum_field_alignment);

  return DECL_ALIGN_UNIT(field);
}

// Return size of OS critical section.
// Can't use the sizeof() calls directly since cross compiling is supported
// and would end up using the host sizes rather than the target sizes.

unsigned
Target::critsecsize()
{
  if (global.params.isWindows)
    {
      // sizeof(CRITICAL_SECTION) for Windows.
      return global.params.isLP64 ? 40 : 24;
    }
  else if (global.params.isLinux)
    {
      // sizeof(pthread_mutex_t) for Linux.
      if (global.params.is64bit)
	return global.params.isLP64 ? 40 : 32;
      else
	return global.params.isLP64 ? 40 : 24;
    }
  else if (global.params.isFreeBSD)
    {
      // sizeof(pthread_mutex_t) for FreeBSD.
      return global.params.isLP64 ? 8 : 4;
    }
  else if (global.params.isOpenBSD)
    {
      // sizeof(pthread_mutex_t) for OpenBSD.
      return global.params.isLP64 ? 8 : 4;
    }
  else if (global.params.isOSX)
    {
      // sizeof(pthread_mutex_t) for OSX.
      return global.params.isLP64 ? 64 : 44;
    }
  else if (global.params.isSolaris)
    {
      // sizeof(pthread_mutex_t) for Solaris.
      return 24;
    }

  gcc_unreachable();
}

// Returns a Type for the va_list type of the target.

Type *
Target::va_listType()
{
  return Type::tvalist;
}

// Perform a reinterpret cast of EXPR to type TYPE for use in CTFE.
// The front end should have already ensured that EXPR is a constant,
// so we just lower the value to GCC and return the converted CST.

Expression *
Target::paintAsType(Expression *expr, Type *type)
{
  /* We support up to 512-bit values.  */
  unsigned char buffer[64];
  tree cst;

  Type *tb = type->toBasetype();

  if (expr->type->isintegral())
    cst = build_integer_cst(expr->toInteger(), expr->type->toCtype());
  else if (expr->type->isfloating())
    cst = build_float_cst(expr->toReal(), expr->type);
  else if (expr->op == TOKarrayliteral)
    {
      // Build array as VECTOR_CST, assumes EXPR is constant.
      Expressions *elements = ((ArrayLiteralExp *) expr)->elements;
      vec<constructor_elt, va_gc> *elms = NULL;

      vec_safe_reserve(elms, elements->dim);
      for (size_t i = 0; i < elements->dim; i++)
	{
	  Expression *e = (*elements)[i];
	  if (e->type->isintegral())
	    {
	      tree value = build_integer_cst(e->toInteger(), e->type->toCtype());
	      CONSTRUCTOR_APPEND_ELT(elms, size_int(i), value);
	    }
	  else if (e->type->isfloating())
	    {
	      tree value = build_float_cst(e->toReal(), e->type);
	      CONSTRUCTOR_APPEND_ELT(elms, size_int(i), value);
	    }
	  else
	    gcc_unreachable();
	}

      // Build vector type.
      int nunits = ((TypeSArray *) expr->type)->dim->toUInteger();
      Type *telem = expr->type->nextOf();
      tree vectype = build_vector_type(telem->toCtype(), nunits);

      cst = build_vector_from_ctor(vectype, elms);
    }
  else
    gcc_unreachable();

  // Encode CST to buffer.
  int len = native_encode_expr(cst, buffer, sizeof(buffer));

  if (tb->ty == Tsarray)
    {
      // Interpret value as a vector of the same size,
      // then return the array literal.
      int nunits = ((TypeSArray *) type)->dim->toUInteger();
      Type *elem = type->nextOf();
      tree vectype = build_vector_type(elem->toCtype(), nunits);

      cst = native_interpret_expr(vectype, buffer, len);

      Expression *e = build_expression(cst);
      gcc_assert(e != NULL && e->op == TOKvector);

      return ((VectorExp *) e)->e1;
    }
  else
    {
      // Normal interpret cast.
      cst = native_interpret_expr(type->toCtype(), buffer, len);

      Expression *e = build_expression(cst);
      gcc_assert(e != NULL);

      return e;
    }
}

