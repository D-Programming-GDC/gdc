/* d-convert.cc -- Data type conversion routines.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "langhooks.h"
#include "target.h"
#include "convert.h"
#include "stor-layout.h"

#include "d-tree.h"


/* Build CODE expression with operands OP0 and OP1.
   Helper function for d_truthvalue_conversion, so assumes bool result.  */

static tree
d_build_truthvalue_op (tree_code code, tree op0, tree op1)
{
  tree type0, type1;

  tree result_type = NULL_TREE;

  type0 = TREE_TYPE (op0);
  type1 = TREE_TYPE (op1);

  /* Strip NON_LVALUE_EXPRs, etc., since we aren't using as an lvalue.  */
  STRIP_TYPE_NOPS (op0);
  STRIP_TYPE_NOPS (op1);

  /* Also need to convert pointer/int comparison.  */
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
  /* If integral, need to convert unsigned/signed comparison.
     Will also need to convert if type precisions differ.  */
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
	op0 = convert (result_type, op0);
      if (TREE_TYPE (op1) != result_type)
	op1 = convert (result_type, op1);
    }

  return fold_build2 (code, bool_type_node, op0, op1);
}


/* Convert EXPR to be a truth-value, validating its type for this purpose.  */

tree
d_truthvalue_conversion (tree expr)
{
  switch (TREE_CODE (expr))
    {
    case EQ_EXPR:   case NE_EXPR:   case LE_EXPR:
    case GE_EXPR:   case LT_EXPR:   case GT_EXPR:
      if (TREE_TYPE (expr) == bool_type_node)
	return expr;
      return build2 (TREE_CODE (expr), bool_type_node,
		     TREE_OPERAND (expr, 0), TREE_OPERAND (expr, 1));

    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_OR_EXPR:
    case TRUTH_XOR_EXPR:
      if (TREE_TYPE (expr) == bool_type_node)
	return expr;
      return build2 (TREE_CODE (expr), bool_type_node,
		     d_truthvalue_conversion (TREE_OPERAND (expr, 0)),
		     d_truthvalue_conversion (TREE_OPERAND (expr, 1)));

    case TRUTH_NOT_EXPR:
      if (TREE_TYPE (expr) == bool_type_node)
	return expr;
      return build1 (TREE_CODE (expr), bool_type_node,
		     d_truthvalue_conversion (TREE_OPERAND (expr, 0)));

    case ERROR_MARK:
      return expr;

    case INTEGER_CST:
      return integer_zerop (expr) ? boolean_false_node
				  : boolean_true_node;

    case REAL_CST:
      return real_compare (NE_EXPR, &TREE_REAL_CST (expr), &dconst0)
	     ? boolean_true_node
	     : boolean_false_node;

    case ADDR_EXPR:
      /* If we are taking the address of a decl that can never be null,
	 then the return result is always true.  */
      if (DECL_P (TREE_OPERAND (expr, 0))
	  && (TREE_CODE (TREE_OPERAND (expr, 0)) == PARM_DECL
	      || TREE_CODE (TREE_OPERAND (expr, 0)) == LABEL_DECL
	      || !DECL_WEAK (TREE_OPERAND (expr, 0))))
	return boolean_true_node;
      break;

    case COMPLEX_EXPR:
      return d_build_truthvalue_op ((TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 1))
				     ? TRUTH_OR_EXPR : TRUTH_ORIF_EXPR),
			d_truthvalue_conversion (TREE_OPERAND (expr, 0)),
			d_truthvalue_conversion (TREE_OPERAND (expr, 1)));

    case NEGATE_EXPR:
    case ABS_EXPR:
    case FLOAT_EXPR:
      /* These don't change whether an object is nonzero or zero.  */
      return d_truthvalue_conversion (TREE_OPERAND (expr, 0));

    case LROTATE_EXPR:
    case RROTATE_EXPR:
      /* These don't change whether an object is zero or nonzero, but
	 we can't ignore them if their second arg has side-effects.  */
      if (TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 1)))
	{
	  return build2 (COMPOUND_EXPR, bool_type_node, TREE_OPERAND (expr, 1),
			 d_truthvalue_conversion (TREE_OPERAND (expr, 0)));
	}
      else
	return d_truthvalue_conversion (TREE_OPERAND (expr, 0));

    case COND_EXPR:
      /* Distribute the conversion into the arms of a COND_EXPR.  */
      return fold_build3 (COND_EXPR, bool_type_node, TREE_OPERAND (expr, 0),
			  d_truthvalue_conversion (TREE_OPERAND (expr, 1)),
			  d_truthvalue_conversion (TREE_OPERAND (expr, 2)));

    case CONVERT_EXPR:
      /* Don't cancel the effect of a CONVERT_EXPR from a REFERENCE_TYPE,
	 since that affects how `default_conversion' will behave.  */
      if (TREE_CODE (TREE_TYPE (expr)) == REFERENCE_TYPE
	  || TREE_CODE (TREE_TYPE (TREE_OPERAND (expr, 0))) == REFERENCE_TYPE)
	break;
      /* Fall through.  */

    case NOP_EXPR:
      /* If this isn't narrowing the argument, we can ignore it.  */
      if (TYPE_PRECISION (TREE_TYPE (expr))
	  >= TYPE_PRECISION (TREE_TYPE (TREE_OPERAND (expr, 0))))
	return d_truthvalue_conversion (TREE_OPERAND (expr, 0));
      break;

    default:
      break;
    }

  if (TREE_CODE (TREE_TYPE (expr)) == COMPLEX_TYPE)
    {
      tree t = save_expr (expr);
      tree type = TREE_TYPE (TREE_TYPE (expr));
      return d_build_truthvalue_op ((TREE_SIDE_EFFECTS (expr)
				     ? TRUTH_OR_EXPR : TRUTH_ORIF_EXPR),
			d_truthvalue_conversion (build1 (REALPART_EXPR, type, t)),
			d_truthvalue_conversion (build1 (IMAGPART_EXPR, type, t)));
    }
  else
    return d_build_truthvalue_op (NE_EXPR, expr,
				  build_zero_cst (TREE_TYPE (expr)));
}


/* Creates an expression whose value is that of EXPR, converted to type TYPE.
   This function implements all reasonable scalar conversions.  */

tree
convert (tree type, tree expr)
{
  tree e = expr;
  tree_code code = TREE_CODE (type);

  if (type == error_mark_node
      || expr == error_mark_node
      || TREE_TYPE (expr) == error_mark_node)
    return error_mark_node;

  const char *invalid_conv_diag
    = targetm.invalid_conversion (TREE_TYPE (expr), type);

  if (invalid_conv_diag)
    {
      error ("%s", invalid_conv_diag);
      return error_mark_node;
    }

  if (type == TREE_TYPE (expr))
    return expr;

  if (TREE_CODE (type) == ARRAY_TYPE
      && TREE_CODE (TREE_TYPE (expr)) == ARRAY_TYPE
      && TYPE_DOMAIN (type) == TYPE_DOMAIN (TREE_TYPE (expr)))
    return expr;

  tree ret = targetm.convert_to_type (type, expr);
  if (ret)
    return ret;

  STRIP_TYPE_NOPS (e);
  tree etype = TREE_TYPE (e);

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
      if (TREE_CODE (etype) == POINTER_TYPE
	  || TREE_CODE (etype) == REFERENCE_TYPE)
	{
	  if (integer_zerop (e))
	    return build_int_cst (type, 0);

	  /* Convert to an unsigned integer of the correct width first, and
	     from there widen/truncate to the required type.  */
	  tree utype = lang_hooks.types.type_for_size (TYPE_PRECISION (etype),
						       1);
	  ret = fold_build1 (CONVERT_EXPR, utype, e);
	  return fold_convert (type, ret);
	}

      ret = convert_to_integer (type, e);
      goto maybe_fold;

    case BOOLEAN_TYPE:
      return fold_convert (type, d_truthvalue_conversion (expr));

    case POINTER_TYPE:
    case REFERENCE_TYPE:
      ret = convert_to_pointer (type, e);
      goto maybe_fold;

    case REAL_TYPE:
      if (TREE_CODE (etype) == COMPLEX_TYPE && TYPE_IMAGINARY_FLOAT (type))
	e = build1 (IMAGPART_EXPR, TREE_TYPE (etype), e);

      ret = convert_to_real (type, e);
      goto maybe_fold;

    case COMPLEX_TYPE:
      if (TREE_CODE (etype) == REAL_TYPE && TYPE_IMAGINARY_FLOAT (etype))
	ret = build2 (COMPLEX_EXPR, type,
		      build_zero_cst (TREE_TYPE (type)),
		      convert (TREE_TYPE (type), expr));
      else
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
