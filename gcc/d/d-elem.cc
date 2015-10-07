// d-elem.cc -- D frontend for GCC.
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

#include "dfrontend/aggregate.h"
#include "dfrontend/expression.h"
#include "dfrontend/module.h"
#include "dfrontend/statement.h"
#include "dfrontend/ctfe.h"

#include "d-system.h"
#include "d-tree.h"
#include "d-lang.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "id.h"

elem *
Expression::toElem (IRState *)
{
  error ("abstract Expression::toElem called");
  return error_mark_node;
}

elem *
CondExp::toElem (IRState *)
{
  tree cond = convert_for_condition (econd->toElem(NULL), econd->type);
  tree t1 = e1->toElemDtor(NULL);
  tree t2 = e2->toElemDtor(NULL);

  if (type->ty != Tvoid)
    {
      t1 = convert_expr (t1, e1->type, type);
      t2 = convert_expr (t2, e2->type, type);
    }

  return build3 (COND_EXPR, build_ctype(type), cond, t1, t2);
}

elem *
IdentityExp::toElem (IRState *)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  tree_code code = (op == TOKidentity) ? EQ_EXPR : NE_EXPR;

  if ((tb1->ty == Tsarray || tb1->ty == Tarray)
      && (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
      // Convert arrays to D array types.
      return build2 (code, build_ctype(type),
		     d_array_convert (e1), d_array_convert (e2));
    }
  else if (tb1->isfloating())
    {
      tree t1 = e1->toElem(NULL);
      tree t2 = e2->toElem(NULL);
      // Assume all padding is at the end of the type.
      tree size = build_integer_cst (TYPE_PRECISION (build_ctype(e1->type)) / BITS_PER_UNIT);

      // Do bit compare of floats.
      tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					build_address (t1), build_address (t2), size);

      return build_boolop (code, tmemcmp, integer_zero_node);
    }
  else if (tb1->ty == Tstruct)
    {
      tree t1 = e1->toElem(NULL);
      tree t2 = e2->toElem(NULL);

      if (TYPE_MODE (TREE_TYPE (t1)) != BLKmode)
	{
	  // Bitwise comparison of small structs not returned in memory may
	  // not work due to data holes loosing its zero padding upon return.
	  // Instead do field-by-field comparison of the two structs.
	  StructDeclaration *sd = ((TypeStruct *) tb1)->sym;
	  gcc_assert (d_types_same (tb1, tb2));

	  // Make temporaries to prevent multiple evaluations.
	  t1 = maybe_make_temp (t1);
	  t2 = maybe_make_temp (t2);

	  return build_struct_memcmp (code, sd, t1, t2);
	}
      else
	{
	  // Do bit compare of structs.
	  tree size = build_integer_cst (e1->type->size());

	  tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					    build_address (t1), build_address (t2), size);

	  return build_boolop (code, tmemcmp, integer_zero_node);
	}
    }
  else
    {
      // For operands of other types, identity is defined as being the same as equality.
      tree tcmp = build_boolop (code, e1->toElem(NULL), e2->toElem(NULL));

      return d_convert (build_ctype(type), tcmp);
    }
}

elem *
EqualExp::toElem (IRState *)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  tree_code code = (op == TOKequal) ? EQ_EXPR : NE_EXPR;

  if (tb1->ty == Tstruct)
    {
      // Do bit compare of structs
      tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					build_address (e1->toElem(NULL)),
					build_address (e2->toElem(NULL)),
					build_integer_cst (e1->type->size()));

      return build2 (code, build_ctype(type), tmemcmp, integer_zero_node);
    }
  else if ((tb1->ty == Tsarray || tb1->ty == Tarray)
	   && (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
      Type *t1elem = tb1->nextOf()->toBasetype();
      Type *t2elem = tb1->nextOf()->toBasetype();

      if ((t1elem->isintegral() || t1elem->ty == Tvoid) && t1elem->ty == t2elem->ty)
	{
	  // Optimise comparisons of arrays of basic types.
	  // For arrays of integers/characters, and void[], replace _adEq2 call with:
	  //     e1 == e2  =>  e1.length == e2.length && memcmp (e1.ptr, e2.ptr, size) == 0;
	  //     e1 != e2  =>  e1.length != e2.length || memcmp (e1.ptr, e2.ptr, size) != 0;
	  // 'size' is e1.length * sizeof(e1[0]) for dynamic arrays, or sizeof(e1) for static arrays.
	  tree t1 = e1->toElem(NULL);
	  tree t2 = e2->toElem(NULL);
	  // Length, for comparison.
	  tree t1len, t2len;
	  // Pointer to data and data size, to pass to memcmp.
	  tree t1ptr, t2ptr;
	  tree t1size, t2size;

	  // Make temporaries to prevent multiple evaluations.
	  tree t1saved = make_temp (t1);
	  tree t2saved = make_temp (t2);

	  if (tb1->ty == Tarray)
	    {
	      t1len = d_array_length (t1saved);
	      t1ptr = d_array_ptr (t1saved);
	      t1size = build2 (MULT_EXPR, size_type_node, t1len, size_int (t1elem->size()));
	    }
	  else
	    {
	      t1len = size_int (((TypeSArray *) tb1)->dim->toInteger());
	      t1ptr = build_address (t1saved);
	      t1size = size_int (tb1->size());
	    }

	  if (tb2->ty == Tarray)
	    {
	      t2len = d_array_length (t2saved);
	      t2ptr = d_array_ptr (t2saved);
	      t2size = build2 (MULT_EXPR, size_type_node, t2len, size_int (t2elem->size()));
	    }
	  else
	    {
	      t2len = size_int (((TypeSArray *) tb2)->dim->toInteger());
	      t2ptr = build_address (t2saved);
	      t2size = size_int (tb2->size());
	    }

	  tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					    t1ptr, t2ptr, (tb2->ty == Tsarray) ? t2size : t1size);

	  tree result = build2 (code, build_ctype(type), tmemcmp, integer_zero_node);

	  if (tb1->ty == Tsarray && tb2->ty == Tsarray)
	    gcc_assert (tb1->size() == tb2->size());
	  else
	    {
	      tree_code tcode = (op == TOKequal) ? TRUTH_ANDIF_EXPR : TRUTH_ORIF_EXPR;
	      tree tlencmp = build2 (code, size_type_node, t1len, t2len);

	      result = build_boolop (tcode, tlencmp, result);
	    }

	  // Ensure left-to-right order of evaluation.
	  if (d_has_side_effects (t2))
	    result = compound_expr (t2saved, result);

	  if (d_has_side_effects (t1))
	    result = compound_expr (t1saved, result);

	  return result;
	}
      else
	{
	  // _adEq2 compares each element.
	  tree args[3];
	  tree result;

	  args[0] = d_array_convert (e1);
	  args[1] = d_array_convert (e2);
	  args[2] = build_typeinfo (t1elem->arrayOf());

	  result = d_convert (build_ctype(type), build_libcall (LIBCALL_ADEQ2, 3, args));

	  if (op == TOKnotequal)
	    return build1 (TRUTH_NOT_EXPR, build_ctype(type), result);

	  return result;
	}
    }
  else if (tb1->ty == Taarray && tb2->ty == Taarray)
    {
      TypeAArray *taa1 = (TypeAArray *) tb1;
      tree args[3];
      tree result;

      args[0] = build_typeinfo (taa1);
      args[1] = e1->toElem(NULL);
      args[2] = e2->toElem(NULL);
      result = d_convert (build_ctype(type), build_libcall (LIBCALL_AAEQUAL, 3, args));

      if (op == TOKnotequal)
	return build1 (TRUTH_NOT_EXPR, build_ctype(type), result);

      return result;
    }
  else
    {
      tree tcmp = build_boolop (code, e1->toElem(NULL), e2->toElem(NULL));

      return d_convert (build_ctype(type), tcmp);
    }
}

elem *
InExp::toElem(IRState *)
{
  Type *tb2 = e2->type->toBasetype();
  gcc_assert(tb2->ty == Taarray);

  Type *tkey = ((TypeAArray *) tb2)->index->toBasetype();
  tree key = convert_expr(e1->toElem(NULL), e1->type, tkey);
  tree args[3];

  args[0] = e2->toElem(NULL);
  args[1] = build_typeinfo(tkey);
  args[2] = build_address(key);

  tree call = build_libcall(LIBCALL_AAINX, 3, args);
  return convert(build_ctype(type), call);
}

elem *
CmpExp::toElem (IRState *)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  tree result;
  tree_code code;

  switch (op)
    {
    case TOKue:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNEQ_EXPR : EQ_EXPR;
      break;

    case TOKlg:
      code = tb1->isfloating() && tb2->isfloating() ?
	LTGT_EXPR : NE_EXPR;
      break;

    case TOKule:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNLE_EXPR : LE_EXPR;
      break;

    case TOKul:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNLT_EXPR : LT_EXPR;
      break;

    case TOKuge:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNGE_EXPR : GE_EXPR;
      break;

    case TOKug:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNGT_EXPR : GT_EXPR;
      break;

    case TOKle:
      code = LE_EXPR;
      break;

    case TOKlt:
      code = LT_EXPR;
      break;

    case TOKge:
      code = GE_EXPR;
      break;

    case TOKgt:
      code = GT_EXPR;
      break;

    case TOKleg:
      code = ORDERED_EXPR;
      break;

    case TOKunord:
      code = UNORDERED_EXPR;
      break;

    default:
      gcc_unreachable();
    }

  if ((tb1->ty == Tsarray || tb1->ty == Tarray)
      && (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
      Type *telem = tb1->nextOf()->toBasetype();
      tree args[3];

      args[0] = d_array_convert (e1);
      args[1] = d_array_convert (e2);
      args[2] = build_typeinfo (telem->arrayOf());
      result = build_libcall (LIBCALL_ADCMP2, 3, args);

      // %% For float element types, warn that NaN is not taken into account?

      // %% Could do a check for side effects and drop the unused condition
      if (code == ORDERED_EXPR)
	return build_boolop (COMPOUND_EXPR, result,
			     d_truthvalue_conversion (integer_one_node));

      if (code == UNORDERED_EXPR)
	return build_boolop (COMPOUND_EXPR, result,
			     d_truthvalue_conversion (integer_zero_node));

      result = build_boolop (code, result, integer_zero_node);
      return d_convert (build_ctype(type), result);
    }
  else
    {
      if (!tb1->isfloating() || !tb2->isfloating())
	{
	  // %% is this properly optimized away?
	  if (code == ORDERED_EXPR)
	    return convert (bool_type_node, integer_one_node);

	  if (code == UNORDERED_EXPR)
	    return convert (bool_type_node, integer_zero_node);
	}

      result = build_boolop (code, e1->toElem(NULL), e2->toElem(NULL));
      return d_convert (build_ctype(type), result);
    }
}

elem *
AndAndExp::toElem(IRState *)
{
  if (e2->type->toBasetype()->ty != Tvoid)
    {
      tree t1 = convert_for_condition(e1->toElem(NULL), e1->type);
      tree t2 = convert_for_condition(e2->toElem(NULL), e2->type);

      return d_convert(build_ctype(type), build_boolop(TRUTH_ANDIF_EXPR, t1, t2));
    }
  else
    {
      return build3(COND_EXPR, build_ctype(type),
		    convert_for_condition(e1->toElem(NULL), e1->type),
		    e2->toElemDtor(NULL), void_node);
    }
}

elem *
OrOrExp::toElem(IRState *)
{
  if (e2->type->toBasetype()->ty != Tvoid)
    {
      tree t1 = convert_for_condition(e1->toElem(NULL), e1->type);
      tree t2 = convert_for_condition(e2->toElem(NULL), e2->type);

      return d_convert(build_ctype(type), build_boolop(TRUTH_ORIF_EXPR, t1, t2));
    }
  else
    {
      return build3(COND_EXPR, build_ctype(type),
		    build1(TRUTH_NOT_EXPR, bool_type_node,
			   convert_for_condition(e1->toElem(NULL), e1->type)),
		    e2->toElemDtor(NULL), void_node);
    }
}

elem *
XorExp::toElem(IRState *)
{
  return build_binary_op(BIT_XOR_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
OrExp::toElem(IRState *)
{
  return build_binary_op(BIT_IOR_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
AndExp::toElem(IRState *)
{
  return build_binary_op(BIT_AND_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
UshrExp::toElem(IRState *)
{
  return build_binary_op(UNSIGNED_RSHIFT_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
ShrExp::toElem(IRState *)
{
  return build_binary_op(RSHIFT_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
ShlExp::toElem(IRState *)
{
  return build_binary_op(LSHIFT_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
ModExp::toElem(IRState *)
{
  return build_binary_op(e1->type->isfloating() ? FLOAT_MOD_EXPR : TRUNC_MOD_EXPR,
			 build_ctype(type), e1->toElem(NULL), e2->toElem(NULL));
}

elem *
DivExp::toElem(IRState *)
{
  return build_binary_op(e1->type->isintegral() ? TRUNC_DIV_EXPR : RDIV_EXPR,
			 build_ctype(type), e1->toElem(NULL), e2->toElem(NULL));
}

elem *
MulExp::toElem(IRState *)
{
  return build_binary_op(MULT_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
PowExp::toElem (IRState *)
{
  tree e1_t, e2_t;
  tree powtype, powfn = NULL_TREE;
  Type *tb1 = e1->type->toBasetype();

  // Dictates what version of pow() we call.
  powtype = build_ctype(type->toBasetype());
  // If type is int, implicitly convert to double.
  // This allows backend to fold the call into a constant return value.
  if (type->isintegral())
    powtype = double_type_node;

  // Lookup compatible builtin. %% TODO: handle complex types?
  if (TYPE_MAIN_VARIANT (powtype) == double_type_node)
    powfn = builtin_decl_explicit (BUILT_IN_POW);
  else if (TYPE_MAIN_VARIANT (powtype) == float_type_node)
    powfn = builtin_decl_explicit (BUILT_IN_POWF);
  else if (TYPE_MAIN_VARIANT (powtype) == long_double_type_node)
    powfn = builtin_decl_explicit (BUILT_IN_POWL);

  if (powfn == NULL_TREE)
    {
      if (tb1->ty == Tarray || tb1->ty == Tsarray)
	error ("Array operation %s not implemented", toChars());
      else
	error ("%s ^^ %s is not supported", e1->type->toChars(), e2->type->toChars());
      return error_mark_node;
    }

  e1_t = d_convert (powtype, e1->toElem(NULL));
  e2_t = d_convert (powtype, e2->toElem(NULL));

  return d_convert (build_ctype(type), d_build_call_nary (powfn, 2, e1_t, e2_t));
}

elem *
CatExp::toElem(IRState *)
{
  // One of the operands may be an element instead of an array.
  // Logic copied from CatExp::semantic
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();
  Type *etype;

  if (tb1->ty == Tarray || tb1->ty == Tsarray)
    etype = tb1->nextOf();
  else
    etype = tb2->nextOf();

  // Flatten multiple concatenations
  unsigned n_operands = 2;
  for (Expression *ex = e1; ex->op == TOKcat;)
    {
      if (ex->op == TOKcat)
	{
	  ex = ((CatExp *) ex)->e1;
	  n_operands++;
	}
    }

  unsigned n_args = (n_operands > 2 ? (2 + (n_operands * 2)) : 3);
  tree *args = new tree[n_args];

  args[0] = build_typeinfo(type);

  if (n_operands > 2)
    args[1] = build_integer_cst(n_operands, build_ctype(Type::tuns32));

  unsigned ai = n_args - 1;
  CatExp *ce = this;
  vec<tree, va_gc> *elemvars = NULL;

  // Loop through each concatenation from right to left.
  // Dynamic arrays are not passed directly over varargs, instead they are
  // split between length and ptr values.
  for (Expression *oe = ce->e2; oe != NULL;
       (ce->e1->op != TOKcat
	? (oe = ce->e1)
	: (ce = (CatExp *)ce->e1, oe = ce->e2)))
    {
      tree arg;
      if (d_types_same(oe->type->toBasetype(), etype->toBasetype()))
	{
	  // Convert single element to an array.
	  tree var = NULL_TREE;
	  tree expr = maybe_temporary_var(oe->toElem(NULL), &var);
	  arg = d_array_value(build_ctype(oe->type->arrayOf()),
			      size_int(1), build_address(expr));

	  if (var != NULL_TREE)
	    vec_safe_push(elemvars, var);
	}
      else
	arg = d_array_convert(oe);

      if (n_operands > 2)
	{
	  // Filling the array backwards, so .ptr is pushed first.
	  arg = maybe_make_temp(arg);
	  args[ai--] = d_array_ptr(arg);
	  args[ai--] = d_array_length(arg);
	}
      else
	args[ai--] = maybe_make_temp(arg);

      // Finished pushing all arrays.
      if (oe == ce->e1)
	break;
    }

  tree result = build_libcall(n_operands > 2 ? LIBCALL_ARRAYCATNT : LIBCALL_ARRAYCATT,
			      n_args, args, build_ctype(type));

  for (size_t i = 0; i < vec_safe_length(elemvars); ++i)
    result = bind_expr((*elemvars)[i], result);

  return result;
}

elem *
MinExp::toElem(IRState *)
{
  // %% faster: check if result is complex
  if ((e1->type->isreal() && e2->type->isimaginary())
      || (e1->type->isimaginary() && e2->type->isreal()))
    {
      // %%TODO: need to check size/modes
      tree t1 = e1->toElem(NULL);
      tree t2 = e2->toElem(NULL);

      t2 = build1(NEGATE_EXPR, TREE_TYPE(t2), t2);

      if (e1->type->isreal())
	return complex_expr(build_ctype(type), t1, t2);
      else
	return complex_expr(build_ctype(type), t2, t1);
    }

  // The front end has already taken care of pointer-int and pointer-pointer
  return build_binary_op(MINUS_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
AddExp::toElem(IRState *)
{
  // %% faster: check if result is complex
  if ((e1->type->isreal() && e2->type->isimaginary())
      || (e1->type->isimaginary() && e2->type->isreal()))
    {
      // %%TODO: need to check size/modes
      tree t1 = e1->toElem(NULL);
      tree t2 = e2->toElem(NULL);

      if (e1->type->isreal())
	return complex_expr(build_ctype(type), t1, t2);
      else
	return complex_expr(build_ctype(type), t2, t1);
    }

  // The front end has already taken care of (pointer + integer)
  return build_binary_op(PLUS_EXPR, build_ctype(type),
			 e1->toElem(NULL), e2->toElem(NULL));
}

elem *
XorAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(BIT_XOR_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
OrAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(BIT_IOR_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
AndAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(BIT_AND_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
UshrAssignExp::toElem(IRState *)
{
  // Front-end integer promotions don't work here.
  Expression *e1b = e1;
  while (e1b->op == TOKcast)
    {
      CastExp *ce = (CastExp *) e1b;
      gcc_assert(d_types_same(ce->type, ce->to));
      e1b = ce->e1;
    }

  tree exp = build_binop_assignment(UNSIGNED_RSHIFT_EXPR, e1b, e2);
  return convert_expr(exp, e1b->type, type);
}

elem *
ShrAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(RSHIFT_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
ShlAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(LSHIFT_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
ModAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(e1->type->isfloating()
				    ? FLOAT_MOD_EXPR : TRUNC_MOD_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
DivAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(e1->type->isintegral()
				    ? TRUNC_DIV_EXPR : RDIV_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
MulAssignExp::toElem(IRState *)
{
  tree exp = build_binop_assignment(MULT_EXPR, e1, e2);
  return convert_expr(exp, e1->type, type);
}

elem *
PowAssignExp::toElem(IRState *)
{
  gcc_unreachable();
}

// Determine if type is an array of structs that need a postblit.

static StructDeclaration *
needsPostblit (Type *t)
{
  t = t->baseElemOf();

  if (t->ty == Tstruct)
    {
      StructDeclaration *sd = ((TypeStruct *) t)->sym;
      if (sd->postblit)
	return sd;
    }

  return NULL;
}

elem *
CatAssignExp::toElem(IRState *)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();
  Type *etype = tb1->nextOf()->toBasetype();

  if (tb1->ty == Tarray && tb2->ty == Tdchar
      && (etype->ty == Tchar || etype->ty == Twchar))
    {
      // Append a dchar to a char[] or wchar[]
      tree args[2];
      args[0] = build_address(e1->toElem(NULL));
      args[1] = e2->toElem(NULL);

      LibCall libcall = etype->ty == Tchar ? LIBCALL_ARRAYAPPENDCD : LIBCALL_ARRAYAPPENDWD;
      return build_libcall(libcall, 2, args, build_ctype(type));
    }
  else
    {
      gcc_assert(tb1->ty == Tarray || tb2->ty == Tsarray);

      if ((tb2->ty == Tarray || tb2->ty == Tsarray)
	  && d_types_same(etype, tb2->nextOf()->toBasetype()))
	{
	  // Append an array
	  tree args[3];

	  args[0] = build_typeinfo(type);
	  args[1] = build_address(e1->toElem(NULL));
	  args[2] = d_array_convert(e2);

	  return build_libcall(LIBCALL_ARRAYAPPENDT, 3, args, build_ctype(type));
	}
      else if (d_types_same(etype, tb2))
	{
	  // Append an element
	  tree args[3];

	  args[0] = build_typeinfo(type);
	  args[1] = build_address(e1->toElem(NULL));
	  args[2] = size_one_node;

	  tree result = build_libcall(LIBCALL_ARRAYAPPENDCTX, 3, args, build_ctype(type));
	  result = make_temp(result);

	  // Assign e2 to last element
	  tree off_exp = d_array_length(result);
	  off_exp = build2(MINUS_EXPR, TREE_TYPE (off_exp), off_exp, size_one_node);
	  off_exp = maybe_make_temp(off_exp);

	  tree ptr_exp = d_array_ptr(result);
	  ptr_exp = void_okay_p(ptr_exp);
	  ptr_exp = build_array_index(ptr_exp, off_exp);

	  // Evaluate expression before appending
	  tree e2e = e2->toElem(NULL);
	  e2e = maybe_make_temp(e2e);
	  result = modify_expr(build_ctype(etype), build_deref(ptr_exp), e2e);
	  return compound_expr(e2e, result);
	}
      else
	gcc_unreachable();
    }
}

elem *
MinAssignExp::toElem (IRState *)
{
  tree exp = build_binop_assignment(MINUS_EXPR, e1, e2);
  return convert_expr (exp, e1->type, type);
}

elem *
AddAssignExp::toElem (IRState *)
{
  tree exp = build_binop_assignment(PLUS_EXPR, e1, e2);
  return convert_expr (exp, e1->type, type);
}

elem *
AssignExp::toElem(IRState *)
{
  // First, handle special assignment semantics

  // Look for array.length = n;
  if (e1->op == TOKarraylength)
    {
      // Assignment to an array's length property; resize the array.
      ArrayLengthExp *ale = (ArrayLengthExp *) e1;
      // Don't want ->toBasetype() for the element type.
      Type *etype = ale->e1->type->toBasetype()->nextOf();
      tree args[3];
      LibCall libcall;

      args[0] = build_typeinfo(ale->e1->type);
      args[1] = convert_expr(e2->toElem(NULL), e2->type, Type::tsize_t);
      args[2] = build_address(ale->e1->toElem(NULL));
      libcall = etype->isZeroInit() ? LIBCALL_ARRAYSETLENGTHT : LIBCALL_ARRAYSETLENGTHIT;

      tree result = build_libcall(libcall, 3, args);
      return d_array_length(result);
    }

  // Look for array[] = n;
  if (e1->op == TOKslice)
    {
      SliceExp *se = (SliceExp *) e1;
      Type *stype = se->e1->type->toBasetype();
      Type *etype = stype->nextOf()->toBasetype();

      // Determine if we need to do postblit.
      bool postblit = false;
      if (needsPostblit(etype) != NULL
	  && ((e2->op != TOKslice && e2->isLvalue())
	      || (e2->op == TOKslice && ((UnaExp *) e2)->e1->isLvalue())
	      || (e2->op == TOKcast && ((UnaExp *) e2)->e1->isLvalue())))
	postblit = true;

      if (ismemset)
	{
	  // Set a range of elements to one value.
	  tree t1 = maybe_make_temp(e1->toElem(NULL));
	  tree t2 = e2->toElem(NULL);
	  tree result;

	  if (postblit && op != TOKblit)
	    {
	      tree args[4];
	      LibCall libcall;

	      args[0] = d_array_ptr(t1);
	      args[1] = build_address(t2);
	      args[2] = d_array_length(t1);
	      args[3] = build_typeinfo(etype);
	      libcall = (op == TOKconstruct) ? LIBCALL_ARRAYSETCTOR : LIBCALL_ARRAYSETASSIGN;

	      tree call = build_libcall(libcall, 4, args);
	      return compound_expr(call, t1);
	    }

	  if (integer_zerop(t2))
	    {
	      tree size = fold_build2(MULT_EXPR, size_type_node,
				      d_convert(size_type_node, d_array_length(t1)),
				      size_int(etype->size()));

	      result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMSET), 3,
					 d_array_ptr(t1), integer_zero_node, size);
	    }
	  else
	    result = build_array_set(d_array_ptr(t1), d_array_length(t1), t2);

	  return compound_expr(result, t1);
	}
      else
	{
	  // Perform a memcpy operation.
	  gcc_assert(e2->type->ty != Tpointer);

	  if (!postblit && !array_bounds_check())
	    {
	      tree t1 = maybe_make_temp(d_array_convert(e1));
	      tree t2 = d_array_convert(e2);
	      tree size = fold_build2(MULT_EXPR, size_type_node,
				      d_convert(size_type_node, d_array_length(t1)),
				      size_int(etype->size()));

	      tree result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMCPY), 3,
					      d_array_ptr(t1), d_array_ptr(t2), size);
	      return compound_expr(result, t1);
	    }
	  else if (postblit && op != TOKblit)
	    {
	      // Generate _d_arrayassign() or _d_arrayctor()
	      tree args[3];
	      LibCall libcall;

	      args[0] = build_typeinfo(etype);
	      args[1] = maybe_make_temp(d_array_convert(e1));
	      args[2] = d_array_convert(e2);
	      libcall = (op == TOKconstruct) ? LIBCALL_ARRAYCTOR : LIBCALL_ARRAYASSIGN;

	      return build_libcall(libcall, 3, args, build_ctype(type));
	    }
	  else
	    {
	      // Generate _d_arraycopy()
	      tree args[3];

	      args[0] = build_integer_cst(etype->size(), build_ctype(Type::tsize_t));
	      args[1] = d_array_convert(e2);
	      args[2] = maybe_make_temp(d_array_convert(e1));

	      return build_libcall(LIBCALL_ARRAYCOPY, 3, args, build_ctype(type));
	    }
	}
    }

  // Look for reference initializations
  if (op == TOKconstruct && e1->op == TOKvar)
    {
      Declaration *decl = ((VarExp *) e1)->var;
      if (decl->storage_class & (STCout | STCref))
	{
	  tree t1 = e1->toElem(NULL);
	  tree t2 = convert_for_assignment(e2->toElem(NULL), e2->type, e1->type);
	  // Want reference to lhs, not indirect ref.
	  t1 = TREE_OPERAND(t1, 0);
	  t2 = build_address(t2);

	  return modify_expr(build_ctype(type), t1, t2);
	}
    }

  // Other types of assignments that may require post construction.
  Type *tb1 = e1->type->toBasetype();

  if (tb1->ty == Tstruct)
    {
      tree t1 = e1->toElem(NULL);
      tree t2 = convert_for_assignment(e2->toElem(NULL), e2->type, e1->type);

      if (op == TOKconstruct && TREE_CODE (t2) == CALL_EXPR
	  && aggregate_value_p(TREE_TYPE (t2), t2))
	CALL_EXPR_RETURN_SLOT_OPT (t2) = true;

      if (e2->op == TOKint64)
	{
	  // Use memset to fill struct.
	  StructDeclaration *sd = ((TypeStruct *) tb1)->sym;

	  tree result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMSET), 3,
					 build_address(t1), t2,
					 size_int(sd->structsize));

	  // Maybe set-up hidden pointer to outer scope context.
	  if (sd->isNested())
	    {
	      tree vthis_field = sd->vthis->toSymbol()->Stree;
	      tree vthis_value = build_vthis(sd);

	      tree vthis_exp = modify_expr(component_ref(t1, vthis_field), vthis_value);
	      result = compound_expr(result, vthis_exp);
	    }

	  return compound_expr(result, t1);
	}

      return modify_expr(build_ctype(type), t1, t2);
    }

  if (tb1->ty == Tsarray)
    {
      Type *etype = tb1->nextOf();
      gcc_assert(e2->type->toBasetype()->ty == Tsarray);

      // Determine if we need to do postblit.
      bool postblit = false;
      if (needsPostblit(etype) != NULL
	  && ((e2->op != TOKslice && e2->isLvalue())
	      || (e2->op == TOKslice && ((UnaExp *) e2)->e1->isLvalue())
	      || (e2->op == TOKcast && ((UnaExp *) e2)->e1->isLvalue())))
	postblit = true;
      
      if (postblit && op != TOKblit)
	{
	  // Generate _d_arrayassign() or _d_arrayctor()
	  tree t1 = e1->toElem(NULL);
	  tree args[3];
	  LibCall libcall;

	  args[0] = build_typeinfo(etype);
	  args[1] = d_array_convert(e1);
	  args[2] = d_array_convert(e2);
	  libcall = (op == TOKconstruct) ? LIBCALL_ARRAYCTOR : LIBCALL_ARRAYASSIGN;

	  tree result = build_libcall(libcall, 3, args);
	  return compound_expr(result, t1);
	}

      tree t1 = e1->toElem(NULL);
      tree t2 = convert_for_assignment(e2->toElem(NULL), e2->type, e1->type);

      if (op == TOKconstruct && TREE_CODE (t2) == CALL_EXPR
	  && aggregate_value_p(TREE_TYPE (t2), t2))
	CALL_EXPR_RETURN_SLOT_OPT (t2) = true;

      return modify_expr(build_ctype(type), t1, t2);
    }

  // Simple assignment
  tree t1 = e1->toElem(NULL);
  tree t2 = convert_for_assignment(e2->toElem(NULL), e2->type, e1->type);

  return modify_expr(build_ctype(type), t1, t2);
}

elem *
PostExp::toElem(IRState *)
{
  tree result;

  if (op == TOKplusplus)
    {
      result = build2 (POSTINCREMENT_EXPR, build_ctype(type),
		       e1->toElem(NULL), e2->toElem(NULL));
    }
  else if (op == TOKminusminus)
    {
      result = build2 (POSTDECREMENT_EXPR, build_ctype(type),
		       e1->toElem(NULL), e2->toElem(NULL));
    }
  else
    gcc_unreachable();

  TREE_SIDE_EFFECTS (result) = 1;
  return result;
}

elem *
IndexExp::toElem(IRState *)
{
  Type *tb1 = e1->type->toBasetype();

  if (tb1->ty == Taarray)
    {
      // Get the key for the associative array.
      Type *tkey = ((TypeAArray *) tb1)->index->toBasetype();
      tree key = convert_expr(e2->toElem(NULL), e2->type, tkey);
      LibCall libcall;
      tree args[4];

      if (modifiable)
	{
	  libcall = LIBCALL_AAGETX;
	  args[0] = build_address(e1->toElem(NULL));
	}
      else
	{
	  libcall = LIBCALL_AAGETRVALUEX;
	  args[0] = e1->toElem(NULL);
	}

      args[1] = build_typeinfo(tkey);
      args[2] = build_integer_cst(tb1->nextOf()->size(), build_ctype(Type::tsize_t));
      args[3] = build_address(key);

      // Index the associative array.
      tree result = build_libcall(libcall, 4, args, build_ctype(type->pointerTo()));

      if (!skipboundscheck && array_bounds_check())
	{
	  result = make_temp(result);
	  result = build3(COND_EXPR, TREE_TYPE (result), d_truthvalue_conversion(result),
			  result, d_assert_call(loc, LIBCALL_ARRAY_BOUNDS));
	}

      return indirect_ref(build_ctype(type), result);
    }
  else
    {
      // Get the data pointer and length for static and dynamic arrays.
      tree array = maybe_make_temp(e1->toElem(NULL));
      tree ptr = convert_expr(array, tb1, tb1->nextOf()->pointerTo());

      tree length = NULL_TREE;
      if (tb1->ty != Tpointer)
	length = get_array_length(array, tb1);
      else
	gcc_assert(lengthVar == NULL);

      // The __dollar variable just becomes a placeholder for the actual length.
      if (lengthVar)
	{
	  lengthVar->csym = new Symbol();
	  lengthVar->csym->Stree = length;
	}

      // Generate the index.
      tree index = e2->toElem(NULL);
      if (tb1->ty == Tarray || tb1->ty == Tsarray)
	{
	  // If it's a static array and the index is constant,
	  // the front end has already checked the bounds.
	  if (array_bounds_check() && !(tb1->ty == Tsarray && e2->isConst()))
	    index = d_checked_index(loc, maybe_make_temp(index), length, false);
	}
      else
	gcc_assert(tb1->ty == Tpointer);

      // Index the .ptr
      ptr = void_okay_p(ptr);
      return indirect_ref(TREE_TYPE (TREE_TYPE (ptr)),
			  build_array_index(ptr, index));
    }
}

elem *
CommaExp::toElem (IRState *)
{
  tree t1 = e1->toElem(NULL);
  tree t2 = e2->toElem(NULL);
  tree tt = type ? build_ctype(type) : void_type_node;

  return build2 (COMPOUND_EXPR, tt, t1, t2);
}

elem *
ArrayLengthExp::toElem (IRState *)
{
  if (e1->type->toBasetype()->ty == Tarray)
    return d_array_length (e1->toElem(NULL));
  else
    {
      // Tsarray case seems to be handled by front-end
      error ("unexpected type for array length: %s", type->toChars());
      return error_mark_node;
    }
}

elem *
DelegatePtrExp::toElem(IRState *)
{
  tree t1 = e1->toElem(NULL);
  return delegate_object(t1);
}

elem *
DelegateFuncptrExp::toElem(IRState *)
{
  tree t1 = e1->toElem(NULL);
  return delegate_method(t1);
}

elem *
SliceExp::toElem (IRState *)
{
  Type *tb = type->toBasetype();
  Type *tb1 = e1->type->toBasetype();
  gcc_assert (tb->ty == Tarray || tb->ty == Tsarray);

  // Use convert-to-dynamic-array code if possible
  if(!lwr)
    {
      tree t1 = e1->toElem(NULL);
      if (e1->type->toBasetype()->ty == Tsarray)
	t1 = convert_expr (t1, e1->type, type);

      return t1;
    }
  else
    gcc_assert(upr != NULL);

  // Get the data pointer and length for static and dynamic arrays.
  tree array = maybe_make_temp(e1->toElem(NULL));
  tree ptr = convert_expr(array, tb1, tb1->nextOf()->pointerTo());
  tree length = NULL_TREE;

  // Our array is already a SAVE_EXPR if necessary, so we don't make length
  // a SAVE_EXPR which is, at most, a COMPONENT_REF on top of array.
  if (tb1->ty != Tpointer)
    length = get_array_length(array, tb1);
  else
    gcc_assert(lengthVar == NULL);

  // The __dollar variable just becomes a placeholder for the actual length.
  if (lengthVar)
    {
      lengthVar->csym = new Symbol();
      lengthVar->csym->Stree = length;
    }

  // Generate lower bound.
  tree lwr_tree = maybe_make_temp(lwr->toElem(NULL));

  if (!integer_zerop(lwr_tree))
    {
      // Adjust the .ptr offset.
      tree ptrtype = TREE_TYPE (ptr);
      ptr = build_array_index(void_okay_p(ptr), lwr_tree);
      ptr = build_nop(ptrtype, ptr);
    }
  else
    lwr_tree = NULL_TREE;

  // Nothing more to do for static arrays.
  if (tb->ty == Tsarray)
    return indirect_ref(build_ctype(type), ptr);
  else
    gcc_assert(tb->ty == Tarray);

  // Generate upper bound with bounds checking.
  tree upr_tree = maybe_make_temp(upr->toElem(NULL));
  tree newlength;

  if (array_bounds_check())
    {
      if (length)
	newlength = d_checked_index(loc, upr_tree, length, true);
      else
	{
	  // Still need to check bounds lwr <= upr for pointers.
	  gcc_assert(tb1->ty == Tpointer);
	  newlength = upr_tree;
	}

      // Enforces lwr <= upr. No need to check lwr <= length as
      // we've already ensured that upr <= length.
      if (lwr_tree)
	{
	  tree lwr_bounds = d_checked_index(loc, lwr_tree, upr_tree, true);
	  newlength = compound_expr(lwr_bounds, newlength);
	}
    }
  else
    newlength = upr_tree;

  // Need to ensure lwr always gets evaluated first, as it may be a function call.
  // Does (-lwr + upr) rather than (upr - lwr)
  if (lwr_tree)
    {
      newlength = build2(PLUS_EXPR, TREE_TYPE (newlength),
			 build1(NEGATE_EXPR, TREE_TYPE (lwr_tree), lwr_tree), newlength);
    }

  tree result = d_array_value(build_ctype(type), newlength, ptr);
  return compound_expr(array, result);
}

elem *
CastExp::toElem (IRState *)
{
  Type *ebtype = e1->type->toBasetype();
  Type *tbtype = to->toBasetype();
  tree t = e1->toElem(NULL);

  // Just evaluate e1 if it has any side effects
  if (tbtype->ty == Tvoid)
    return build_nop (build_ctype(tbtype), t);

  return convert_expr (t, ebtype, tbtype);
}

elem *
BoolExp::toElem (IRState *)
{
  // Check, should we instead do truthvalue conversion?
  tree exp = e1->toElem(NULL);
  return d_convert (build_ctype(type), exp);
}

elem *
DeleteExp::toElem (IRState *)
{
  LibCall libcall;
  tree t1 = e1->toElem(NULL);
  Type *tb1 = e1->type->toBasetype();

  if (tb1->ty == Tclass)
    {
      if (e1->op == TOKvar)
        {
	  VarDeclaration *v = ((VarExp *) e1)->var->isVarDeclaration();
	  if (v && v->onstack)
	    {
	      libcall = tb1->isClassHandle()->isInterfaceDeclaration() ?
		LIBCALL_CALLINTERFACEFINALIZER : LIBCALL_CALLFINALIZER;
	      return build_libcall (libcall, 1, &t1);
	    }
	}
      libcall = tb1->isClassHandle()->isInterfaceDeclaration() ?
	LIBCALL_DELINTERFACE : LIBCALL_DELCLASS;

      t1 = build_address (t1);
      return build_libcall (libcall, 1, &t1);
    }
  else if (tb1->ty == Tarray)
    {
      // Might need to run destructor on array contents
      Type *telem = tb1->nextOf()->baseElemOf();
      tree ti = null_pointer_node;
      tree args[2];

      if (telem->ty == Tstruct)
	{
	  TypeStruct *ts = (TypeStruct *) telem;
	  if (ts->sym->dtor)
	    ti = tb1->nextOf()->getTypeInfo(NULL)->toElem(NULL);
	}

      // call _delarray_t (&t1, ti);
      args[0] = build_address (t1);
      args[1] = ti;

      return build_libcall (LIBCALL_DELARRAYT, 2, args);
    }
  else if (tb1->ty == Tpointer)
    {
      t1 = build_address (t1);
      return build_libcall (LIBCALL_DELMEMORY, 1, &t1);
    }
  else
    {
      error ("don't know how to delete %s", e1->toChars());
      return error_mark_node;
    }
}

elem *
RemoveExp::toElem(IRState *)
{
  // Check that the array is actually an associative array
  if (e1->type->toBasetype()->ty == Taarray)
    {
      Type *tb = e1->type->toBasetype();
      Type *tkey = ((TypeAArray *) tb)->index->toBasetype();
      tree index = convert_expr(e2->toElem(NULL), e2->type, tkey);
      tree args[3];

      args[0] = e1->toElem(NULL);
      args[1] = build_typeinfo(tkey);
      args[2] = build_address(index);

      return build_libcall(LIBCALL_AADELX, 3, args);
    }
  else
    {
      error("%s is not an associative array", e1->toChars());
      return error_mark_node;
    }
}

elem *
NotExp::toElem(IRState *)
{
  // Need to convert to boolean type or this will fail.
  tree t = fold_build1(TRUTH_NOT_EXPR, bool_type_node,
		       convert_for_condition(e1->toElem(NULL), e1->type));
  return d_convert(build_ctype(type), t);
}

elem *
ComExp::toElem(IRState *)
{
  TY ty1 = e1->type->toBasetype()->ty;
  gcc_assert(ty1 != Tarray && ty1 != Tsarray);

  return fold_build1(BIT_NOT_EXPR, build_ctype(type), e1->toElem(NULL));
}

elem *
NegExp::toElem(IRState *)
{
  TY ty1 = e1->type->toBasetype()->ty;
  gcc_assert(ty1 != Tarray && ty1 != Tsarray);

  return fold_build1(NEGATE_EXPR, build_ctype(type), e1->toElem(NULL));
}

elem *
PtrExp::toElem (IRState *)
{
  /* Produce better code by converting * (#rec + n) to
     COMPONENT_REFERENCE.  Otherwise, the variable will always be
     allocated in memory because its address is taken. */
  Type *rec_type = 0;
  size_t the_offset;
  tree rec_tree;

  if (e1->op == TOKadd)
    {
      BinExp *add_exp = (BinExp *) e1;
      if (add_exp->e1->op == TOKaddress
	  && add_exp->e2->isConst() && add_exp->e2->type->isintegral())
	{
	  Expression *rec_exp = ((AddrExp *) add_exp->e1)->e1;
	  rec_type = rec_exp->type->toBasetype();
	  rec_tree = rec_exp->toElem(NULL);
	  the_offset = add_exp->e2->toUInteger();
	}
    }
  else if (e1->op == TOKsymoff)
    {
      SymOffExp *sym_exp = (SymOffExp *) e1;
      if (declaration_type_kind(sym_exp->var) != type_reference)
	{
	  rec_type = sym_exp->var->type->toBasetype();
	  rec_tree = get_decl_tree(sym_exp->var);
	  the_offset = sym_exp->offset;
	}
    }

  if (rec_type && rec_type->ty == Tstruct)
    {
      StructDeclaration *sd = ((TypeStruct *) rec_type)->sym;
      for (size_t i = 0; i < sd->fields.dim; i++)
	{
	  VarDeclaration *field = sd->fields[i];
	  if (field->offset == the_offset
	      && d_types_same (field->type, this->type))
	    {
	      // Catch errors, backend will ICE otherwise.
	      if (error_operand_p (rec_tree))
		return rec_tree;

	      return component_ref (rec_tree, field->toSymbol()->Stree);
	    }
	  else if (field->offset > the_offset)
	    break;
	}
    }

  return indirect_ref (build_ctype(type), e1->toElem(NULL));
}

elem *
AddrExp::toElem (IRState *)
{
  tree exp;

  if (e1->op == TOKstructliteral)
    {
      StructLiteralExp *sle = ((StructLiteralExp *) e1)->origin;
      exp = build_address (sle->toElem(NULL));
    }
  else
    exp = build_address (e1->toElem(NULL));

  return build_nop (build_ctype(type), exp);
}

elem *
CallExp::toElem (IRState *)
{
  Type *tb = e1->type->toBasetype();
  Expression *e1b = e1;
  tree object = NULL_TREE;

  // Calls to delegates can sometimes look like this:
  if (e1b->op == TOKcomma)
    {
      e1b = ((CommaExp *) e1b)->e2;
      gcc_assert (e1b->op == TOKvar);

      Declaration *var = ((VarExp *) e1b)->var;
      gcc_assert (var->isFuncDeclaration() && !var->needThis());
    }

  tree callee = e1b->toElem(NULL);
  TypeFunction *tf = NULL;

  if (D_METHOD_CALL_EXPR (callee))
    {
      // This could be a delegate expression (TY == Tdelegate), but not
      // actually a delegate variable.
      if (e1b->op == TOKdotvar)
	{
	  // This gets the true function type, getting the function type from
	  // e1->type can sometimes be incorrect, eg: ref return functions.
	  tf = get_function_type (((DotVarExp *) e1b)->var->type);
	}
      else
	tf = get_function_type (tb);

      extract_from_method_call (callee, callee, object);
    }
  else if (tb->ty == Tdelegate)
    {
      // Delegate call, extract .object and .funcptr from var.
      callee = maybe_make_temp (callee);
      tf = get_function_type (tb);
      object = delegate_object (callee);
      callee = delegate_method (callee);
    }
  else if (e1b->op == TOKvar)
    {
      FuncDeclaration *fd = ((VarExp *) e1b)->var->isFuncDeclaration();
      gcc_assert(fd != NULL);
      tf = get_function_type (fd->type);

      if (fd->isNested())
	{
	  // Maybe re-evaluate symbol storage treating 'fd' as public.
	  if (call_by_alias_p (cfun->language->function, fd))
	    setup_symbol_storage (fd, callee, true);

	  object = get_frame_for_symbol (fd);
	}
      else if (fd->needThis())
	{
	  e1b->error ("need 'this' to access member %s", fd->toChars());
	  // Continue processing...
	  object = null_pointer_node;
	}
    }
  else
    {
      // Normal direct function call.
      tf = get_function_type (tb);
    }

  gcc_assert (tf != NULL);

  // Now we have the type, callee and maybe object reference,
  // build the call expression.
  tree exp = d_build_call (tf, callee, object, arguments);

  if (tf->isref)
    exp = build_deref(exp);

  // Some library calls are defined to return a generic type.
  // this->type is the real type we want to return.
  if (type->isTypeBasic())
    exp = d_convert (build_ctype(type), exp);

  return exp;
}

/*******************************************
 * Evaluate Expression, then call destructors on any temporaries in it.
 */

elem *
Expression::toElemDtor (IRState *)
{
  size_t starti = cfun->language->vars_in_scope.length();
  tree exp = toElem(NULL);
  size_t endi = cfun->language->vars_in_scope.length();

  // Codegen can be improved by determining if no exceptions can be thrown
  // between the ctor and dtor, and eliminating the ctor and dtor.

  // Build an expression that calls destructors on all the variables going
  // going out of the scope starti .. endi.
  tree tdtors = NULL_TREE;
  for (size_t i = starti; i != endi; ++i)
    {
      VarDeclaration *vd = cfun->language->vars_in_scope[i];
      if (vd)
	{
	  cfun->language->vars_in_scope[i] = NULL;
	  tree td = vd->edtor->toElem(NULL);
	  // Execute in reverse order.
	  tdtors = maybe_compound_expr (tdtors, td);
	}
    }

  if (tdtors != NULL_TREE)
    {
      if (op == TOKcall)
	{
	  // Wrap expression and dtors in a try/finally expression.
	  tree body = exp;

	  if (type->ty == Tvoid)
	    exp = build2 (TRY_FINALLY_EXPR, void_type_node, body, tdtors);
	  else
	    {
	      body = maybe_make_temp (body);
	      tree tfexp = build2 (TRY_FINALLY_EXPR, void_type_node, body, tdtors);
	      exp = compound_expr (tfexp, body);
	    }
	}
      else if (op == TOKcomma && ((CommaExp *) this)->e2->op == TOKvar)
	{
	  // Split comma expressions, so as don't require a save_expr.
	  tree lexp = TREE_OPERAND (exp, 0);
	  tree rvalue = TREE_OPERAND (exp, 1);
	  exp = compound_expr (compound_expr (lexp, tdtors), rvalue);
	}
      else
	{
	  exp = maybe_make_temp (exp);
	  exp = compound_expr (compound_expr (exp, tdtors), exp);
	}
    }

  return exp;
}


elem *
DotTypeExp::toElem (IRState *)
{
  // Just a pass through to e1.
  return e1->toElem(NULL);
}

// The result will probably just be converted to a CONSTRUCTOR for a Tdelegate struct
elem *
DelegateExp::toElem (IRState *)
{
  Type *t = e1->type->toBasetype();

  if (func->fbody)
    {
      // Add the function as nested function if it belongs to this module
      // ie, it is a member of this module, or it is a template instance.
      Dsymbol *owner = func->toParent();
      while (!owner->isTemplateInstance() && owner->toParent())
	owner = owner->toParent();
      if (owner->isTemplateInstance() || owner == cfun->language->module)
	cfun->language->deferred_fns.safe_push(func);
    }

  if (t->ty == Tclass || t->ty == Tstruct)
    {
      if (!func->isThis())
	{
	  error ("delegates are only for non-static functions");
	  return error_mark_node;
	}

      return get_object_method (e1->toElem(NULL), e1, func, type);
    }
  else
    {
      tree this_tree;
      if (func->isNested())
	{
	  if (e1->op == TOKnull)
	    this_tree = e1->toElem(NULL);
	  else
	    this_tree = get_frame_for_symbol (func);
	}
      else
	{
	  gcc_assert (func->isThis());
	  this_tree = e1->toElem(NULL);
	}

      return build_method_call (build_address (func->toSymbol()->Stree),
				this_tree, type);
    }
}

elem *
DotVarExp::toElem(IRState *)
{
  Type *tb = e1->type->toBasetype();

  switch (tb->ty)
    {
    case Tpointer:
      if (tb->nextOf()->toBasetype()->ty != Tstruct)
	break;
      // drop through

    case Tstruct:
    case Tclass:
    {
      FuncDeclaration *fd = var->isFuncDeclaration();
      VarDeclaration *vd = var->isVarDeclaration();
      if (fd != NULL)
      {
	// if Tstruct, objInstanceMethod will use the address of e1
	if (fd->isThis())
	  return get_object_method(e1->toElem(NULL), e1, fd, type);
	else
	  {
	    // Static method; ignore the object instance
	    return build_address(fd->toSymbol()->Stree);
    	  }
      }
      else if (vd)
	{
	  if (!vd->isField())
	    return get_decl_tree(vd);
	  else
	    {
	      tree this_tree = e1->toElem(NULL);
	      if (tb->ty != Tstruct)
		this_tree = build_deref(this_tree);
	      return component_ref(this_tree, vd->toSymbol()->Stree);
	    }
	}
      else
	error("%s is not a field, but a %s", var->toChars(), var->kind());
      break;
    }

    default:
      break;
    }

  error("Don't know how to handle %s", toChars());
  return error_mark_node;
}

elem *
AssertExp::toElem (IRState *)
{
  if (global.params.useAssert)
    {
      Type *tb1 = e1->type->toBasetype();
      tree tmsg = NULL_TREE;
      LibCall libcall;

      // Build _d_assert call.
      if (cfun->language->function->isUnitTestDeclaration())
	{
	  if (msg)
	    {
	      tmsg = msg->toElemDtor(NULL);
	      libcall = LIBCALL_UNITTEST_MSG;
	    }
	  else
	    libcall = LIBCALL_UNITTEST;
	}
      else
	{
	  if (msg)
	    {
	      tmsg = msg->toElemDtor(NULL);
	      libcall = LIBCALL_ASSERT_MSG;
	    }
	  else
	    libcall = LIBCALL_ASSERT;
	}

      tree assert_call = d_assert_call(loc, libcall, tmsg);

      // Build condition that we are asserting in this contract.
      if (tb1->ty == Tclass)
	{
	  ClassDeclaration *cd = tb1->isClassHandle();
	  tree arg = e1->toElem(NULL);
	  tree invc = NULL_TREE;

	  if (cd->isCOMclass())
	    {
	      return build3(COND_EXPR, void_type_node,
			    build_boolop(NE_EXPR, arg, null_pointer_node),
			    void_node, assert_call);
	    }
	  else if (cd->isInterfaceDeclaration())
	    arg = convert_expr(arg, tb1, build_object_type());

	  if (global.params.useInvariants && !cd->isCPPclass())
	    {
	      arg = maybe_make_temp(arg);
	      invc = build_libcall(LIBCALL_INVARIANT, 1, &arg);
	    }

	  // This does a null pointer check before calling _d_invariant
	  return build3(COND_EXPR, void_type_node,
			build_boolop(NE_EXPR, arg, null_pointer_node),
			invc ? invc : void_node, assert_call);
	}
      else
	{
	  // build: ((bool) e1  ? (void)0 : _d_assert (...))
	  //    or: (e1 != null ? e1._invariant() : _d_assert (...))
	  tree result;
	  tree invc = NULL_TREE;
	  tree e1_t = e1->toElem(NULL);

	  if (global.params.useInvariants
	      && tb1->ty == Tpointer && tb1->nextOf()->ty == Tstruct)
	    {
	      FuncDeclaration *inv = ((TypeStruct *) tb1->nextOf())->sym->inv;
	      if (inv != NULL)
		{
		  Expressions args;
		  e1_t = maybe_make_temp(e1_t);
		  invc = d_build_call(inv, e1_t, &args);
		}
	    }
	  result = build3(COND_EXPR, void_type_node,
			  convert_for_condition(e1_t, e1->type),
			  invc ? invc : void_node, assert_call);
	  return result;
	}
    }

  return void_node;
}

elem *
DeclarationExp::toElem (IRState *)
{
  VarDeclaration *vd = declaration->isVarDeclaration();

  if (vd != NULL)
    {
      if (!vd->isStatic() && !(vd->storage_class & STCmanifest)
	  && !(vd->storage_class & (STCextern | STCtls | STCgshared)))
	{
	  // Put variable on list of things needing destruction
	  if (vd->edtor && !vd->noscope)
	    cfun->language->vars_in_scope.safe_push (vd);
	}
    }

  push_stmt_list();
  declaration->toObjFile (0);
  tree t = pop_stmt_list();

  /* Construction of an array for typesafe-variadic function arguments
     can cause an empty STMT_LIST here.  This can causes problems
     during gimplification. */
  if (TREE_CODE (t) == STATEMENT_LIST && !STATEMENT_LIST_HEAD (t))
    return build_empty_stmt (input_location);

  return t;
}


elem *
FuncExp::toElem (IRState *)
{
  Type *ftype = type->toBasetype();

  // This check is for lambda's, remove 'vthis' as function isn't nested.
  if (fd->tok == TOKreserved && ftype->ty == Tpointer)
    {
      fd->tok = TOKfunction;
      fd->vthis = NULL;
    }

  // Emit after current function body has finished.
  if (cfun != NULL)
    cfun->language->deferred_fns.safe_push(fd);

  // If nested, this will be a trampoline...
  if (fd->isNested())
    {
      return build_method_call (build_address (fd->toSymbol()->Stree),
				get_frame_for_symbol (fd), type);
    }

  return build_nop (build_ctype(type), build_address (fd->toSymbol()->Stree));
}

elem *
HaltExp::toElem (IRState *)
{
  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_TRAP), 0);
}

elem *
SymbolExp::toElem (IRState *)
{
  tree exp;

  if (op == TOKvar)
    {
      if (var->needThis())
	{
	  error ("need 'this' to access member %s", var->ident->string);
	  return error_mark_node;
	}

      // __ctfe is always false at runtime
      if (var->ident == Id::ctfe)
	return integer_zero_node;

      exp = get_decl_tree (var);
      TREE_USED (exp) = 1;

      // For variables that are references (currently only out/inout arguments;
      // objects don't count), evaluating the variable means we want what it refers to.
      if (declaration_type_kind(var) == type_reference)
	exp = indirect_ref(build_ctype(var->type), exp);

      // The frontend sometimes emits different types for the expression and var.
      // Convert to the expressions type, but don't convert FuncDeclaration as
      // type->ctype sometimes isn't the correct type for functions!
      if (!d_types_same(var->type, type) && !var->isFuncDeclaration())
	exp = build1 (VIEW_CONVERT_EXPR, build_ctype(type), exp);

      return exp;
    }
  else if (op == TOKsymoff)
    {
      size_t offset = ((SymOffExp *) this)->offset;

      exp = get_decl_tree (var);
      TREE_USED (exp) = 1;

      if (declaration_type_kind(var) == type_reference)
	gcc_assert(POINTER_TYPE_P (TREE_TYPE (exp)));
      else
	exp = build_address(exp);

      if (!offset)
	return d_convert (build_ctype(type), exp);

      tree b = build_integer_cst (offset, build_ctype(Type::tsize_t));
      return build_nop (build_ctype(type), build_offset (exp, b));
    }

  gcc_assert (op == TOKvar || op == TOKsymoff);
  return error_mark_node;
}

elem *
NewExp::toElem(IRState *)
{
  Type *tb = type->toBasetype();
  LibCall libcall;
  tree result;

  if (allocator)
    gcc_assert(newargs);

  // New'ing a class.
  if (tb->ty == Tclass)
    {
      tb = newtype->toBasetype();
      gcc_assert(tb->ty == Tclass);
      TypeClass *tclass = (TypeClass *) tb;
      ClassDeclaration *cd = tclass->sym;

      tree new_call;
      tree setup_exp = NULL_TREE;
      // type->ctype is a POINTER_TYPE; we want the RECORD_TYPE
      tree rec_type = TREE_TYPE(build_ctype(tclass));

      // Call allocator (custom allocator or _d_newclass).
      if (onstack)
	{
	  tree stack_var = build_local_temp(rec_type);
	  expand_decl(stack_var);
	  new_call = build_address(stack_var);
	  setup_exp = modify_expr(stack_var, cd->toInitializer()->Stree);
	}
      else if (allocator)
	{
	  new_call = d_build_call(allocator, NULL_TREE, newargs);
	  new_call = maybe_make_temp(new_call);
	  // copy memory...
	  setup_exp = modify_expr(indirect_ref(rec_type, new_call),
				  cd->toInitializer()->Stree);
	}
      else
	{
	  tree arg = build_address(cd->toSymbol()->Stree);
	  new_call = build_libcall(LIBCALL_NEWCLASS, 1, &arg);
	}
      new_call = build_nop(build_ctype(tb), new_call);

      // Set vthis for nested classes.
      if (cd->isNested())
	{
	  tree vthis_value = NULL_TREE;
	  tree vthis_field = cd->vthis->toSymbol()->Stree;
	  if (thisexp)
	    {
	      ClassDeclaration *thisexp_cd = thisexp->type->isClassHandle();
	      Dsymbol *outer = cd->toParent2();
	      int offset = 0;

	      vthis_value = thisexp->toElem(NULL);
	      if (outer != thisexp_cd)
		{
		  ClassDeclaration *outer_cd = outer->isClassDeclaration();
		  gcc_assert(outer_cd->isBaseOf(thisexp_cd, &offset));
		  // could just add offset
		  vthis_value = convert_expr(vthis_value, thisexp->type, outer_cd->type);
		}
	    }
	  else
	    vthis_value = build_vthis(cd);

	  if (vthis_value)
	    {
	      new_call = maybe_make_temp(new_call);
	      vthis_field = component_ref(indirect_ref(rec_type, new_call), vthis_field);
	      setup_exp = maybe_compound_expr(setup_exp, modify_expr(vthis_field, vthis_value));
	    }
	}
      new_call = maybe_compound_expr(setup_exp, new_call);

      // Call constructor.
      if (member)
	result = d_build_call(member, new_call, arguments);
      else
	result = new_call;
    }
  // New'ing a struct.
  else if (tb->ty == Tpointer && tb->nextOf()->toBasetype()->ty == Tstruct)
    {
      Type *htype = newtype->toBasetype();
      gcc_assert(htype->ty == Tstruct);
      gcc_assert(!onstack);

      TypeStruct *stype = (TypeStruct *) htype;
      StructDeclaration *sd = stype->sym;
      tree new_call;

      // Cannot new an opaque struct.
      if (sd->size(loc) == 0)
	return d_convert(build_ctype(type), integer_zero_node);

      if (allocator)
	new_call = d_build_call(allocator, NULL_TREE, newargs);
      else
	{
	  libcall = htype->isZeroInit() ? LIBCALL_NEWITEMT : LIBCALL_NEWITEMIT;
	  tree arg = type->getTypeInfo(NULL)->toElem(NULL);
	  new_call = build_libcall(libcall, 1, &arg);
	}
      new_call = maybe_make_temp(new_call);
      new_call = build_nop(build_ctype(tb), new_call);

      if (member || !arguments)
	{
	  // Set vthis for nested structs.
	  if (sd->isNested())
	    {
	      tree vthis_value = build_vthis(sd);
	      tree vthis_field = component_ref(indirect_ref(build_ctype(stype), new_call),
					       sd->vthis->toSymbol()->Stree);
	      new_call = compound_expr(modify_expr(vthis_field, vthis_value), new_call);
	    }

	  // Call constructor.
	  if (member)
	    result = d_build_call(member, new_call, arguments);
	  else
	    result = new_call;
	}
      else
	{
	  // User supplied initialiser, set-up with a struct literal.
	  StructLiteralExp *se = StructLiteralExp::create(loc, sd, arguments, htype);
	  se->sym = new Symbol();
	  se->sym->Stree = new_call;
	  se->type = sd->type;
	  se->fillHoles = 0;

	  result = compound_expr(se->toElem(NULL), new_call);
	}
    }
  // New'ing a D array.
  else if (tb->ty == Tarray)
    {
      tb = newtype->toBasetype();
      gcc_assert (tb->ty == Tarray);
      TypeDArray *tarray = (TypeDArray *) tb;
      gcc_assert(!allocator);
      gcc_assert(arguments && arguments->dim >= 1);

      if (arguments->dim == 1)
	{
	  // Single dimension array allocations.
	  Expression *arg = (*arguments)[0];
	  tree args[2];

	  // Elem size is unknown.
	  if (tarray->next->size() == 0)
	    return d_array_value(build_ctype(type), size_int(0), null_pointer_node);

	  libcall = tarray->next->isZeroInit() ? LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
	  args[0] = type->getTypeInfo(NULL)->toElem(NULL);
	  args[1] = arg->toElem(NULL);
	  result = build_libcall(libcall, 2, args, build_ctype(tb));
	}
      else
	{
	  // Multidimensional array allocations.
	  vec<constructor_elt, va_gc> *elms = NULL;
	  Type *telem = newtype->toBasetype();
	  tree dims_var = create_temporary_var(d_array_type(Type::tsize_t, arguments->dim));
	  tree dims_init = build_constructor(TREE_TYPE(dims_var), NULL);
	  tree args[3];

	  for (size_t i = 0; i < arguments->dim; i++)
	    {
	      Expression *arg = (*arguments)[i];
	      tree index = build_integer_cst(i, size_type_node);
	      CONSTRUCTOR_APPEND_ELT(elms, index, arg->toElem(NULL));

	      gcc_assert(telem->ty == Tarray);
	      telem = telem->toBasetype()->nextOf();
	      gcc_assert(telem);
	    }

	  CONSTRUCTOR_ELTS(dims_init) = elms;
	  DECL_INITIAL(dims_var) = dims_init;

	  libcall = telem->isZeroInit() ? LIBCALL_NEWARRAYMTX : LIBCALL_NEWARRAYMITX;
	  args[0] = type->getTypeInfo(NULL)->toElem(NULL);
	  args[1] = build_integer_cst(arguments->dim, build_ctype(Type::tint32));
	  args[2] = build_address(dims_var);
	  result = build_libcall(libcall, 3, args, build_ctype(tb));
	  result = bind_expr(dims_var, result);
	}
    }
  // New'ing a pointer
  else if (tb->ty == Tpointer)
    {
      TypePointer *tpointer = (TypePointer *) tb;

      // Elem size is unknown.
      if (tpointer->next->size() == 0)
	return d_convert(build_ctype(type), integer_zero_node);

      libcall = tpointer->next->isZeroInit(loc) ? LIBCALL_NEWITEMT : LIBCALL_NEWITEMIT;

      tree arg = type->getTypeInfo(NULL)->toElem(NULL);
      result = build_libcall(libcall, 1, &arg, build_ctype(tb));

      if (arguments && arguments->dim == 1)
	{
	  result = make_temp(result);
	  tree init = modify_expr(build_deref(result), (*arguments)[0]->toElem(NULL));
	  result = compound_expr(init, result);
	}
    }
  else
    gcc_unreachable();

  return convert_expr(result, tb, type);
}

elem *
ScopeExp::toElem (IRState *)
{
  error ("%s is not an expression", toChars());
  return error_mark_node;
}

elem *
TypeExp::toElem (IRState *)
{
  error ("type %s is not an expression", toChars());
  return error_mark_node;
}

elem *
RealExp::toElem (IRState *)
{
  return build_float_cst (value, type->toBasetype());
}

elem *
IntegerExp::toElem (IRState *)
{
  tree ctype = build_ctype(type->toBasetype());
  return build_integer_cst (value, ctype);
}

elem *
ComplexExp::toElem (IRState *)
{
  TypeBasic *compon_type;
  switch (type->toBasetype()->ty)
    {
    case Tcomplex32:
      compon_type = (TypeBasic *) Type::tfloat32;
      break;

    case Tcomplex64:
      compon_type = (TypeBasic *) Type::tfloat64;
      break;

    case Tcomplex80:
      compon_type = (TypeBasic *) Type::tfloat80;
      break;

    default:
      gcc_unreachable();
    }

  return build_complex (build_ctype(type),
			build_float_cst (creall (value), compon_type),
			build_float_cst (cimagl (value), compon_type));
}

elem *
StringExp::toElem (IRState *)
{
  Type *tb = type->toBasetype();
  // Assuming this->string is null terminated
  dinteger_t dim = len + (tb->ty != Tsarray);

  tree value = build_string (dim * sz, (char *) string);

  // Array type doesn't match string length if null terminated.
  TREE_TYPE (value) = d_array_type (tb->nextOf(), len);
  TREE_CONSTANT (value) = 1;

  switch (tb->ty)
    {
    case Tarray:
      value = d_array_value (build_ctype(type), size_int (len), build_address (value));
      break;

    case Tpointer:
      value = build_address (value);
      break;

    case Tsarray:
      TREE_TYPE (value) = build_ctype(type);
      break;

    default:
      error ("Invalid type for string constant: %s", type->toChars());
      return error_mark_node;
    }

  return value;
}

elem *
TupleExp::toElem (IRState *)
{
  tree exp = NULL_TREE;

  if (e0)
    exp = e0->toElem(NULL);

  for (size_t i = 0; i < exps->dim; ++i)
    {
      Expression *e = (*exps)[i];
      exp = maybe_vcompound_expr (exp, e->toElem(NULL));
    }

  if (exp == NULL_TREE)
    exp = void_node;

  return exp;
}

elem *
ArrayLiteralExp::toElem (IRState *)
{
  Type *tb = type->toBasetype();
  gcc_assert (tb->ty == Tarray || tb->ty == Tsarray || tb->ty == Tpointer);

  // Convert void[n] to ubyte[n]
  if (tb->ty == Tsarray && tb->nextOf()->toBasetype()->ty == Tvoid)
    tb = Type::tuns8->sarrayOf (((TypeSArray *) tb)->dim->toUInteger());

  Type *etype = tb->nextOf();
  tree tsa = d_array_type (etype, elements->dim);
  tree result = NULL_TREE;

  // Handle empty array literals.
  if (elements->dim == 0)
    {
      if (tb->ty == Tarray)
	return d_array_value (build_ctype(type), size_int (0), null_pointer_node);
      else
	return build_constructor (tsa, NULL);
    }

  // Build an expression that assigns the expressions in ELEMENTS to a constructor.
  vec<constructor_elt, va_gc> *elms = NULL;
  vec_safe_reserve (elms, elements->dim);

  for (size_t i = 0; i < elements->dim; i++)
    {
      Expression *e = (*elements)[i];
      tree elem = e->toElem(NULL);

      if (!integer_zerop (elem))
	{
	  elem = maybe_make_temp (elem);
	  CONSTRUCTOR_APPEND_ELT (elms, build_integer_cst (i, size_type_node),
				  convert_expr (elem, e->type, etype));
	}
    }

  tree ctor = build_constructor (tsa, elms);
  tree args[2];

  // Nothing else to do for static arrays.
  if (tb->ty == Tsarray)
    return d_convert (build_ctype(type), ctor);

  args[0] = build_typeinfo (etype->arrayOf());
  args[1] = build_integer_cst (elements->dim, size_type_node);

  // Call _d_arrayliteralTX (ti, dim);
  tree mem = build_libcall (LIBCALL_ARRAYLITERALTX, 2, args, build_ctype(etype->pointerTo()));
  mem = maybe_make_temp (mem);

  // memcpy (mem, &ctor, size)
  tree size = fold_build2 (MULT_EXPR, size_type_node,
			   size_int (elements->dim), size_int (tb->nextOf()->size()));

  result = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCPY), 3,
			      mem, build_address (ctor), size);

  // Returns array pointed to by MEM.
  result = maybe_compound_expr (result, mem);

  if (tb->ty == Tarray)
    result = d_array_value (build_ctype(type), size_int (elements->dim), result);

  return result;
}

elem *
AssocArrayLiteralExp::toElem (IRState *)
{
  // Want mutable type for typeinfo reference.
  Type *tb = type->toBasetype()->mutableOf();
  gcc_assert(tb->ty == Taarray);

  // Handle empty assoc array literals.
  TypeAArray *ta = (TypeAArray *) tb;
  if (keys->dim == 0)
    return build_constructor (build_ctype(ta), NULL);

  // Build an expression that assigns the expressions in KEYS and VALUES to a constructor.
  vec<constructor_elt, va_gc> *ke = NULL;
  vec_safe_reserve (ke, keys->dim);
  for (size_t i = 0; i < keys->dim; i++)
    {
      Expression *e = (*keys)[i];
      tree t = e->toElem(NULL);
      t = maybe_make_temp(t);
      CONSTRUCTOR_APPEND_ELT(ke, build_integer_cst(i, size_type_node),
			     convert_expr(t, e->type, ta->index));
    }
  tree akeys = build_constructor(d_array_type(ta->index, keys->dim), ke);

  vec<constructor_elt, va_gc> *ve = NULL;
  vec_safe_reserve(ve, values->dim);
  for (size_t i = 0; i < values->dim; i++)
    {
      Expression *e = (*values)[i];
      tree t = e->toElem(NULL);
      t = maybe_make_temp(t);
      CONSTRUCTOR_APPEND_ELT(ve, build_integer_cst(i, size_type_node),
			     convert_expr(t, e->type, ta->next));
    }
  tree avals = build_constructor(d_array_type(ta->next, values->dim), ve);

  // Call _d_assocarrayliteralTX (ti, keys, vals);
  tree args[3];
  args[0] = build_typeinfo(ta);
  args[1] = d_array_value(build_ctype(ta->index->arrayOf()), size_int(keys->dim), build_address(akeys));
  args[2] = d_array_value(build_ctype(ta->next->arrayOf()), size_int(values->dim), build_address(avals));

  tree mem = build_libcall(LIBCALL_ASSOCARRAYLITERALTX, 3, args);

  // Returns an AA pointed to by MEM.
  tree aatype = build_ctype(ta);
  vec<constructor_elt, va_gc> *ce = NULL;
  CONSTRUCTOR_APPEND_ELT (ce, TYPE_FIELDS (aatype), mem);

  return build_nop (build_ctype(type), build_constructor(aatype, ce));
}

elem *
StructLiteralExp::toElem(IRState *)
{
  vec<constructor_elt, va_gc> *ve = NULL;
  Type *tb = type->toBasetype();

  gcc_assert(tb->ty == Tstruct);

  if (sinit)
    {
      // Building sinit trees are delayed until after frontend semantic
      // processing has complete.  Build the static initialiser now.
      if (sinit->Stree == NULL_TREE)
	sd->toInitializer();

      gcc_assert(sinit->Stree != NULL);
      return sinit->Stree;
    }

  // CTFE may fill the hidden pointer by NullExp.
  size_t dim = elements ? elements->dim : 0;
  gcc_assert(dim <= sd->fields.dim);

  for (size_t i = 0; i < dim; i++)
    {
      if (!(*elements)[i])
	continue;

      Expression *exp = (*elements)[i];
      Type *exp_type = exp->type->toBasetype();
      tree exp_tree = NULL_TREE;

      VarDeclaration *fld = sd->fields[i];
      Type *fld_type = fld->type->toBasetype();

      if (fld_type->ty == Tsarray)
	{
	  if (d_types_same(exp_type, fld_type))
	    {
	      // %% This would call _d_newarrayT ... use memcpy?
	      exp_tree = convert_expr(exp->toElem(NULL), exp->type, fld->type);
	    }
	  else
	    {
	      exp_tree = build_local_temp(build_ctype(fld_type));
	      Type *etype = fld_type;

	      while (etype->ty == Tsarray)
		etype = etype->nextOf();

	      gcc_assert(fld_type->size() % etype->size() == 0);
	      tree size = fold_build2(TRUNC_DIV_EXPR, size_type_node,
				      size_int(fld_type->size()), size_int(etype->size()));

	      tree ptr_tree = build_nop(build_ctype(etype->pointerTo()),
					build_address(exp_tree));
	      ptr_tree = void_okay_p(ptr_tree);
	      tree set_exp = build_array_set(ptr_tree, size, exp->toElem(NULL));
	      exp_tree = compound_expr(set_exp, exp_tree);
	    }
	}
      else
	exp_tree = convert_expr(exp->toElem(NULL), exp->type, fld->type);

      CONSTRUCTOR_APPEND_ELT (ve, fld->toSymbol()->Stree, exp_tree);
    }

  if (sd->isNested() && dim != sd->fields.dim)
    {
      // Maybe setup hidden pointer to outer scope context.
      tree vthis_field = sd->vthis->toSymbol()->Stree;
      tree vthis_value = build_vthis(sd);
      CONSTRUCTOR_APPEND_ELT (ve, vthis_field, vthis_value);
      gcc_assert(sinit == NULL);
    }

  tree ctor = build_struct_literal(build_ctype(type), build_constructor(unknown_type_node, ve));
  tree var = (sym != NULL)
    ? build_deref(sym->Stree) : build_local_temp(TREE_TYPE(ctor));
  tree init = NULL_TREE;

  if (fillHoles)
    {
      // Initialize all alignment 'holes' to zero.
      init = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMSET), 3,
			       build_address(var), size_zero_node,
			       size_int(sd->structsize));
    }

  init = maybe_compound_expr(init, modify_expr(var, ctor));

  return compound_expr(init, var);
}

elem *
NullExp::toElem(IRState *)
{
  TY base_ty = type->toBasetype()->ty;
  tree null_exp;
  vec<constructor_elt, va_gc> *ce = NULL;

  // 0 -> dynamic array.  This is a special case conversion.
  // Move to convert for convertTo if it shows up elsewhere.
  switch (base_ty)
    {
    case Tarray:
	  null_exp = d_array_value (build_ctype(type), size_int (0), null_pointer_node);
	  break;

    case Taarray:
	{
	  tree ttype = build_ctype(type);
	  tree field = TYPE_FIELDS (ttype);
	  tree value = d_convert (TREE_TYPE (field), integer_zero_node);

	  CONSTRUCTOR_APPEND_ELT (ce, field, value);
	  null_exp = build_constructor (ttype, ce);
	  break;
	}

    case Tdelegate:
	  null_exp = build_delegate_cst (null_pointer_node, null_pointer_node, type);
	  break;

    default:
	  null_exp = d_convert (build_ctype(type), integer_zero_node);
	  break;
    }

  return null_exp;
}

elem *
ThisExp::toElem (IRState *)
{
  tree this_tree = NULL_TREE;
  FuncDeclaration *fd = cfun ? cfun->language->function : NULL;

  if (var)
    {
      gcc_assert(var->isVarDeclaration());
      this_tree = get_decl_tree (var);
    }
  else
    {
      gcc_assert (fd && fd->vthis);
      this_tree = get_decl_tree (fd->vthis);
    }

  if (type->ty == Tstruct)
    this_tree = build_deref (this_tree);

  return this_tree;
}

elem *
VectorExp::toElem (IRState *)
{
  tree vectype = build_ctype(type);
  tree elemtype = TREE_TYPE (vectype);

  // First handle array literal expressions.
  if (e1->op == TOKarrayliteral)
    {
      Expressions *elements = ((ArrayLiteralExp *) e1)->elements;
      vec<constructor_elt, va_gc> *elms = NULL;
      bool constant_p = true;

      vec_safe_reserve (elms, elements->dim);
      for (size_t i = 0; i < elements->dim; i++)
	{
	  Expression *e = (*elements)[i];
	  tree value = d_convert (elemtype, e->toElem(NULL));
	  if (!CONSTANT_CLASS_P (value))
	    constant_p = false;

	  CONSTRUCTOR_APPEND_ELT (elms, size_int (i), value);
	}

      // Build a VECTOR_CST from a constant vector constructor.
      if (constant_p)
	return build_vector_from_ctor (vectype, elms);

      return build_constructor (vectype, elms);
    }
  else
    {
      // Build constructor from single value.
      tree val = d_convert (elemtype, e1->toElem(NULL));

      return build_vector_from_val (vectype, val);
    }
}

elem *
ClassReferenceExp::toElem (IRState *)
{
  // ClassReferenceExp builds the RECORD_TYPE,
  // we want to return a reference to it.
  tree exp = toSymbol()->Stree;

  return build_address (exp);
}

