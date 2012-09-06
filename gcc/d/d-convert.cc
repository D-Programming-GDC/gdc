// d-convert.cc -- D frontend for GCC.
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
#include "d-lang.h"


// Creates an expression whose value is that of EXPR, converted to type TYPE.
// This function implements all reasonable scalar conversions.

static tree
d_convert_basic (tree type, tree expr)
{
  // taken from c-convert.c
  tree e = expr;
  enum tree_code code = TREE_CODE (type);
  const char *invalid_conv_diag;
  tree ret;

  if (type == error_mark_node
      || expr == error_mark_node
      || TREE_TYPE (expr) == error_mark_node)
    return error_mark_node;

  invalid_conv_diag = targetm.invalid_conversion (TREE_TYPE (expr), type);
  if (invalid_conv_diag)
    {
      error (invalid_conv_diag);
      return error_mark_node;
    }

  if (type == TREE_TYPE (expr))
    return expr;

  if (TREE_CODE (type) == ARRAY_TYPE
      && TREE_CODE (TREE_TYPE (expr)) == ARRAY_TYPE
      && TYPE_DOMAIN (type) == TYPE_DOMAIN (TREE_TYPE (expr)))
    return expr;

  ret = targetm.convert_to_type (type, expr);
  if (ret)
    return ret;

  STRIP_TYPE_NOPS (e);

  if (TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (TREE_TYPE (expr)))
    return fold_convert (type, expr);
  if (TREE_CODE (TREE_TYPE (expr)) == ERROR_MARK)
    return error_mark_node;
  if (TREE_CODE (TREE_TYPE (expr)) == VOID_TYPE)
    {
      error ("void value not ignored as it ought to be");
      return error_mark_node;
    }

  switch (code)
    {
    case VOID_TYPE:
      return fold_convert (type, e);

    case INTEGER_TYPE:
    case ENUMERAL_TYPE:
      ret = convert_to_integer (type, e);
      goto maybe_fold;

    case BOOLEAN_TYPE:
      return fold_convert (type, d_truthvalue_conversion (expr));

    case POINTER_TYPE:
    case REFERENCE_TYPE:
      ret = convert_to_pointer (type, e);
      goto maybe_fold;

    case REAL_TYPE:
      ret = convert_to_real (type, e);
      goto maybe_fold;

    case COMPLEX_TYPE:
      ret = convert_to_complex (type, e);
      goto maybe_fold;

    case VECTOR_TYPE:
      ret = convert_to_vector (type, e);
      goto maybe_fold;

    case RECORD_TYPE:
    case UNION_TYPE:
      if (lang_hooks.types_compatible_p (type, TREE_TYPE (expr)))
	{
	  ret = build1 (VIEW_CONVERT_EXPR, type, expr);
	  goto maybe_fold;
	}
      break;

    default:
      break;

    maybe_fold:
      if (!TREE_CONSTANT (ret))
	ret = fold (ret);
      return ret;
    }

  error ("conversion to non-scalar type requested");
  return error_mark_node;
}

tree
convert (tree type, tree expr)
{
  enum tree_code code = TREE_CODE (type);
  tree etype = TREE_TYPE (expr);

  switch (code)
    {
    case REAL_TYPE:
      if (TREE_CODE (etype) == REAL_TYPE)
	{
	  // Casts between real and imaginary result in 0.0
	  if ((D_TYPE_IMAGINARY_FLOAT (type) && !D_TYPE_IMAGINARY_FLOAT (etype)) ||
	      (!D_TYPE_IMAGINARY_FLOAT (type) && D_TYPE_IMAGINARY_FLOAT (etype)))
	    {
	      warning (OPT_Wcast_result, "cast from %qT to %qT will produce nil result",
		       etype, type);

	      return build2 (COMPOUND_EXPR, type,
			     build1 (CONVERT_EXPR, void_type_node, expr),
			     build_zero_cst (type));
	    }
	}
      else if (TREE_CODE (etype) == COMPLEX_TYPE)
	{
	  // creal.re, .im implemented by cast to real or ireal
	  // Assumes target type is the same size as the original's components size
	  gcc_assert (TYPE_SIZE (type) == TYPE_SIZE (TREE_TYPE (etype)));

	  if (D_TYPE_IMAGINARY_FLOAT (type))
	    expr = build1 (IMAGPART_EXPR, TREE_TYPE (etype), expr);
	}
      break;

    case COMPLEX_TYPE:
      if (TREE_CODE (etype) == REAL_TYPE)
	{
	  if (D_TYPE_IMAGINARY_FLOAT (etype))
	    return build2 (COMPLEX_EXPR, type,
			   build_zero_cst (TREE_TYPE (type)),
			   d_convert_basic (TREE_TYPE (type), expr));
	}
      break;

    default:
      break;
    }

  return d_convert_basic (type, expr);
}


// Perform default target promotions for data used in expressions.

static tree
d_default_conversion (tree exp)
{
  tree orig_exp;
  tree type = TREE_TYPE (exp);
  tree promoted_type;

  /* Constants can be used directly unless they're not loadable.  */
  if (TREE_CODE (exp) == CONST_DECL)
    exp = DECL_INITIAL (exp);

  /* Strip no-op conversions.  */
  orig_exp = exp;
  STRIP_TYPE_NOPS (exp);

  if (TREE_NO_WARNING (orig_exp))
    TREE_NO_WARNING (exp) = 1;

  promoted_type = targetm.promoted_type (type);
  if (promoted_type)
    exp = d_convert_basic (promoted_type, exp);

  return exp;
}

// Helper for d_build_truthvalue_op, so assumes exp will be converted to bool.

static tree
d_scalar_conversion (tree exp)
{
  exp = d_default_conversion (exp);

  switch (TREE_CODE (exp))
    {
    case ARRAY_TYPE:
    case RECORD_TYPE:
    case UNION_TYPE:
    case VOID_TYPE:
    case FUNCTION_TYPE:
    case VECTOR_TYPE:
      // This should already be handled by the front end.
      gcc_unreachable ();

    default:
      break;
    }

  return exp;
}


// Build CODE expression with operands ORIG_OP0 and ORIG_OP1,
// performing default value conversions if CONVERT_P.
// Helper function for d_truthvalue_conversion, so assumes bool result.

static tree
d_build_truthvalue_op (tree_code code, tree orig_op0,
		       tree orig_op1, int convert_p)
{
  tree type0, type1;
  tree op0, op1;

  tree result_type = NULL_TREE;

  if (convert_p)
    {
      op0 = d_scalar_conversion (orig_op0);
      op1 = d_scalar_conversion (orig_op1);
    }
  else
    {
      op0 = orig_op0;
      op1 = orig_op1;
    }

  type0 = TREE_TYPE (op0);
  type1 = TREE_TYPE (op1);

  /* Strip NON_LVALUE_EXPRs, etc., since we aren't using as an lvalue.  */
  STRIP_TYPE_NOPS (op0);
  STRIP_TYPE_NOPS (op1);

  /* Also need to convert pointer/int comparison for GCC >= 4.1 */
  if (POINTER_TYPE_P (type0) && TREE_CODE (op1) == INTEGER_CST
      && integer_zerop (op1))
    {
      result_type = type0;
    }
  else if (POINTER_TYPE_P (type1) && TREE_CODE (op0) == INTEGER_CST
	   && integer_zerop (op0))
    {
      result_type = type1;
    }
  /* If integral, need to convert unsigned/signed comparison for GCC >= 4.4.x
     Will also need to convert if type precisions differ. */
  else if (INTEGRAL_TYPE_P (type0) && INTEGRAL_TYPE_P (type1))
    {
      if (TYPE_PRECISION (type0) > TYPE_PRECISION (type1))
	result_type = type0;
      else if (TYPE_PRECISION (type0) < TYPE_PRECISION (type1))
	result_type = type1;
      else if (TYPE_UNSIGNED (type0) != TYPE_UNSIGNED (type1))
	result_type = TYPE_UNSIGNED (type0) ? type0 : type1;
    }

  if (result_type)
    {
      if (TREE_TYPE (op0) != result_type)
	op0 = d_convert_basic (result_type, op0);
      if (TREE_TYPE (op1) != result_type)
	op1 = d_convert_basic (result_type, op1);
    }

  return build2 (code, boolean_type_node, op0, op1);
}


// Convert EXPR to be a truth-value, validating its type for this purpose.

tree
d_truthvalue_conversion (tree expr)
{
  switch (TREE_CODE (expr))
    {
    case EQ_EXPR:   case NE_EXPR:   case LE_EXPR:
    case GE_EXPR:   case LT_EXPR:   case GT_EXPR:
      if (TREE_TYPE (expr) == boolean_type_node)
	return expr;
      return build2 (TREE_CODE (expr), boolean_type_node,
		     TREE_OPERAND (expr, 0), TREE_OPERAND (expr, 1));

    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_OR_EXPR:
    case TRUTH_XOR_EXPR:
      if (TREE_TYPE (expr) == boolean_type_node)
	return expr;
      return build2 (TREE_CODE (expr), boolean_type_node,
		     d_truthvalue_conversion (TREE_OPERAND (expr, 0)),
		     d_truthvalue_conversion (TREE_OPERAND (expr, 1)));

    case TRUTH_NOT_EXPR:
      if (TREE_TYPE (expr) == boolean_type_node)
	return expr;
      return build1 (TREE_CODE (expr), boolean_type_node,
		     d_truthvalue_conversion (TREE_OPERAND (expr, 0)));

    case ERROR_MARK:
      return expr;

    case INTEGER_CST:
      /* Avoid integer_zerop to ignore TREE_CONSTANT_OVERFLOW.  */
      return (TREE_INT_CST_LOW (expr) != 0 || TREE_INT_CST_HIGH (expr) != 0)
	? boolean_true_node
	: boolean_false_node;

    case REAL_CST:
      return real_compare (NE_EXPR, &TREE_REAL_CST (expr), &dconst0)
	? boolean_true_node
	: boolean_false_node;

    case ADDR_EXPR:
      /* If we are taking the address of an external decl, it might be zero
	 if it is weak, so we cannot optimize.  */
      if (DECL_P (TREE_OPERAND (expr, 0))
	  && DECL_EXTERNAL (TREE_OPERAND (expr, 0)))
	break;

      if (TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 0)))
	return build2 (COMPOUND_EXPR, boolean_type_node,
		       TREE_OPERAND (expr, 0), boolean_true_node);
      else
	return boolean_true_node;

    case COMPLEX_EXPR:
      return d_build_truthvalue_op ((TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 1))
				     ? TRUTH_OR_EXPR : TRUTH_ORIF_EXPR),
				    d_truthvalue_conversion (TREE_OPERAND (expr, 0)),
				    d_truthvalue_conversion (TREE_OPERAND (expr, 1)),
				    /* convert_p */ 0);

    case NEGATE_EXPR:
    case ABS_EXPR:
    case FLOAT_EXPR:
      // %% there may be other things wrong...
      /* These don't change whether an object is nonzero or zero.  */
      return d_truthvalue_conversion (TREE_OPERAND (expr, 0));

    case LROTATE_EXPR:
    case RROTATE_EXPR:
      /* These don't change whether an object is zero or nonzero, but
	 we can't ignore them if their second arg has side-effects.  */
      if (TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 1)))
	return build2 (COMPOUND_EXPR, boolean_type_node, TREE_OPERAND (expr, 1),
		       d_truthvalue_conversion (TREE_OPERAND (expr, 0)));
      else
	return d_truthvalue_conversion (TREE_OPERAND (expr, 0));

    case COND_EXPR:
      /* Distribute the conversion into the arms of a COND_EXPR.  */
      return fold_build3 (COND_EXPR, boolean_type_node, TREE_OPERAND (expr, 0),
			  d_truthvalue_conversion (TREE_OPERAND (expr, 1)),
			  d_truthvalue_conversion (TREE_OPERAND (expr, 2)));

    case CONVERT_EXPR:
      /* Don't cancel the effect of a CONVERT_EXPR from a REFERENCE_TYPE,
	 since that affects how `default_conversion' will behave.  */
      if (TREE_CODE (TREE_TYPE (expr)) == REFERENCE_TYPE
	  || TREE_CODE (TREE_TYPE (TREE_OPERAND (expr, 0))) == REFERENCE_TYPE)
	break;
      /* Fall through....  */
    case NOP_EXPR:
      /* If this is widening the argument, we can ignore it.  */
      if (TYPE_PRECISION (TREE_TYPE (expr))
	  >= TYPE_PRECISION (TREE_TYPE (TREE_OPERAND (expr, 0))))
	return d_truthvalue_conversion (TREE_OPERAND (expr, 0));
      break;

    case MINUS_EXPR:
      /* Perhaps reduce (x - y) != 0 to (x != y).  The expressions
	 aren't guaranteed to the be same for modes that can represent
	 infinity, since if x and y are both +infinity, or both
	 -infinity, then x - y is not a number.

	 Note that this transformation is safe when x or y is NaN.
	 (x - y) is then NaN, and both (x - y) != 0 and x != y will
	 be false.  */
      if (HONOR_INFINITIES (TYPE_MODE (TREE_TYPE (TREE_OPERAND (expr, 0)))))
	break;
      /* Fall through....  */
    case BIT_XOR_EXPR:
      /* This and MINUS_EXPR can be changed into a comparison of the
	 two objects.  */
      if (TREE_TYPE (TREE_OPERAND (expr, 0))
	  == TREE_TYPE (TREE_OPERAND (expr, 1)))
	return fold_build2 (NE_EXPR, boolean_type_node,
			    TREE_OPERAND (expr, 0), TREE_OPERAND (expr, 1));
      return fold_build2 (NE_EXPR, boolean_type_node,
			  TREE_OPERAND (expr, 0),
			  fold_convert (TREE_TYPE (TREE_OPERAND (expr, 0)),
					TREE_OPERAND (expr, 1)));

    case BIT_AND_EXPR:
      if (integer_onep (TREE_OPERAND (expr, 1))
	  && TREE_TYPE (expr) != boolean_type_node)
	/* Using convert here would cause infinite recursion.  */
	return build1 (NOP_EXPR, boolean_type_node, expr);
      break;

    case MODIFY_EXPR:
      // %% do nothing
      break;

    default:
      break;
    }

  if (TREE_CODE (TREE_TYPE (expr)) == COMPLEX_TYPE)
    {
      tree t = save_expr (expr);
      tree compon_type = TREE_TYPE (TREE_TYPE (expr));
      return (d_build_truthvalue_op ((TREE_SIDE_EFFECTS (expr)
				      ? TRUTH_OR_EXPR : TRUTH_ORIF_EXPR),
				     d_truthvalue_conversion (build1 (REALPART_EXPR, compon_type, t)),
				     d_truthvalue_conversion (build1 (IMAGPART_EXPR, compon_type, t)),
				     /* convert_p */ 0));
    }
  /* Without this, the backend tries to load a float reg with and integer
     value with fails (on i386 and rs6000, at least). */
  else if (SCALAR_FLOAT_TYPE_P (TREE_TYPE (expr)))
    return d_build_truthvalue_op (NE_EXPR, expr,
				  d_convert_basic (TREE_TYPE (expr), integer_zero_node), 1);

  return d_build_truthvalue_op (NE_EXPR, expr, integer_zero_node, 1);
}

