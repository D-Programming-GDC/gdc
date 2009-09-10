/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "d-gcc-includes.h"
#include "d-lang.h"

/* Perform default promotions for C data used in expressions.
   Arrays and functions are converted to pointers;
   enumeral types or short or char, to int.
   In addition, manifest constants symbols are replaced by their values.  */

extern "C" tree
default_conversion (tree exp)
{
#if 0
  tree orig_exp;
  tree type = TREE_TYPE (exp);
  enum tree_code code = TREE_CODE (type);

  if (code == FUNCTION_TYPE || code == ARRAY_TYPE)
    return default_function_array_conversion (exp);

  /* Constants can be used directly unless they're not loadable.  */
  if (TREE_CODE (exp) == CONST_DECL)
    exp = DECL_INITIAL (exp);

  /* Replace a nonvolatile const static variable with its value unless
     it is an array, in which case we must be sure that taking the
     address of the array produces consistent results.  */
  else if (gcc_optimize() && TREE_CODE (exp) == VAR_DECL && code != ARRAY_TYPE)
    {
      exp = decl_constant_value_for_broken_optimization (exp);
      type = TREE_TYPE (exp);
    }

  /* Strip NON_LVALUE_EXPRs and no-op conversions, since we aren't using as
     an lvalue. 

     Do not use STRIP_NOPS here!  It will remove conversions from pointer
     to integer and cause infinite recursion.  */
  orig_exp = exp;
  while (TREE_CODE (exp) == NON_LVALUE_EXPR
	 || (TREE_CODE (exp) == NOP_EXPR
	     && TREE_TYPE (TREE_OPERAND (exp, 0)) == TREE_TYPE (exp)))
    exp = TREE_OPERAND (exp, 0);

  /* Preserve the original expression code.  */
  if (IS_EXPR_CODE_CLASS (TREE_CODE_CLASS (TREE_CODE (exp))))
    C_SET_EXP_ORIGINAL_CODE (exp, C_EXP_ORIGINAL_CODE (orig_exp));

  /* Normally convert enums to int,
     but convert wide enums to something wider.  */
  if (code == ENUMERAL_TYPE)
    {
      type = c_common_type_for_size (MAX (TYPE_PRECISION (type),
					  TYPE_PRECISION (integer_type_node)),
				     ((TYPE_PRECISION (type)
				       >= TYPE_PRECISION (integer_type_node))
				      && TREE_UNSIGNED (type)));

      return convert (type, exp);
    }

  if (TREE_CODE (exp) == COMPONENT_REF
      && DECL_C_BIT_FIELD (TREE_OPERAND (exp, 1))
      /* If it's thinner than an int, promote it like a
	 c_promoting_integer_type_p, otherwise leave it alone.  */
      && 0 > compare_tree_int (DECL_SIZE (TREE_OPERAND (exp, 1)),
			       TYPE_PRECISION (integer_type_node)))
    return convert (integer_type_node, exp);

  if (c_promoting_integer_type_p (type))
    {
      /* Preserve unsignedness if not really getting any wider.  */
      if (TREE_UNSIGNED (type)
	  && TYPE_PRECISION (type) == TYPE_PRECISION (integer_type_node))
	return convert (unsigned_type_node, exp);

      return convert (integer_type_node, exp);
    }

  if (code == VOID_TYPE)
    {
      error ("void value not ignored as it ought to be");
      return error_mark_node;
    }
  return exp;
#else
  return exp;
#endif
}

// copied this over just to support d_truthvalue_conversion, so assumes bool
static tree
build_buul_binary_op(tree_code code, tree orig_op0, tree orig_op1, int convert_p)
{
    tree op0, op1;
    tree result_type = NULL_TREE;
    if (convert_p)
    {
      op0 = default_conversion (orig_op0);
      op1 = default_conversion (orig_op1);
    }
    else
    {
      op0 = orig_op0;
      op1 = orig_op1;
    }

    /* Also need to convert pointer/int comparison for GCC >= 4.1 */
    tree type0 = TREE_TYPE (op0);
    tree type1 = TREE_TYPE (op1);
    if (POINTER_TYPE_P(type0) && TREE_CODE(op1) == INTEGER_CST
	&& integer_zerop (op1))
    {
	result_type = type0;
    }
    else if (POINTER_TYPE_P(type1) && TREE_CODE(op0) == INTEGER_CST
	&& integer_zerop (op0))
    {
	result_type = type1;
    }

    if (result_type)
    {
	if (TREE_TYPE (op0) != result_type)
	    op0 = convert (result_type, op0);
	if (TREE_TYPE (op1) != result_type)
	    op1 = convert (result_type, op1);
    }

    return build(code, boolean_type_node, op0, op1);
}

// These functions support calls from the backend.  This happens
// for build_common_tree_nodes_2 and (anything else?%% if so, can
// pare this down or ..just do the work of build_common_tree_nodes_2
// OTOH, might want vector extensions some day.

// probably can go back into dc-lang.cc

// Because this is called by the backend, there may be cases when IRState::converTo
// has been bypassed.  If we have type.lang_specific set on both args, try using IRState::converTo.

// tree convert (tree type, tree expr) is in d-glue.cc to get the current IRState...

tree
d_convert_basic (tree type, tree expr)
{
    // take from c-convert.c

  tree e = expr;
  enum tree_code code = TREE_CODE (type);

  if (type == TREE_TYPE (expr)
      || TREE_CODE (expr) == ERROR_MARK
      || code == ERROR_MARK || TREE_CODE (TREE_TYPE (expr)) == ERROR_MARK)
    return expr;

  if (TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (TREE_TYPE (expr)))
    return fold (build1 (NOP_EXPR, type, expr));
  if (TREE_CODE (TREE_TYPE (expr)) == ERROR_MARK)
    return error_mark_node;
  if (TREE_CODE (TREE_TYPE (expr)) == VOID_TYPE)
    {
      error ("void value not ignored as it ought to be");
      return error_mark_node;
    }
  if (code == VOID_TYPE)
    return build1 (CONVERT_EXPR, type, e);
#if 0
  /* This is incorrect.  A truncation can't be stripped this way.
     Extensions will be stripped by the use of get_unwidened.  */
  if (TREE_CODE (expr) == NOP_EXPR)
    return convert (type, TREE_OPERAND (expr, 0));
#endif
  if (code == INTEGER_TYPE || code == ENUMERAL_TYPE)
    return fold (convert_to_integer (type, e));
  if (code == BOOLEAN_TYPE)
    {
      tree t = d_truthvalue_conversion (expr);
      /* If it returns a NOP_EXPR, we must fold it here to avoid
	 infinite recursion between fold () and convert ().  */
      if (TREE_CODE (t) == NOP_EXPR)
	return fold (build1 (NOP_EXPR, type, TREE_OPERAND (t, 0)));
      else
	return fold (build1 (NOP_EXPR, type, t));
    }
  if (code == POINTER_TYPE || code == REFERENCE_TYPE)
    return fold (convert_to_pointer (type, e));
  if (code == REAL_TYPE)
    return fold (convert_to_real (type, e));
  if (code == COMPLEX_TYPE)
    return fold (convert_to_complex (type, e));
  if (code == VECTOR_TYPE)
    return fold (convert_to_vector (type, e));

  error ("conversion to non-scalar type requested");
  return error_mark_node;
}

tree
d_truthvalue_conversion (tree expr)
{
  if (TREE_CODE (expr) == ERROR_MARK)
    return expr;

#if 0 /* This appears to be wrong for C++.  */
  /* These really should return error_mark_node after 2.4 is stable.
     But not all callers handle ERROR_MARK properly.  */
  switch (TREE_CODE (TREE_TYPE (expr)))
    {
    case RECORD_TYPE:
      error ("struct type value used where scalar is required");
      return boolean_false_node;

    case UNION_TYPE:
      error ("union type value used where scalar is required");
      return boolean_false_node;

    case ARRAY_TYPE:
      error ("array type value used where scalar is required");
      return boolean_false_node;

    default:
      break;
    }
#endif /* 0 */

  switch (TREE_CODE (expr))
    {
    case EQ_EXPR:
    case NE_EXPR: case LE_EXPR: case GE_EXPR: case LT_EXPR: case GT_EXPR:
    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_OR_EXPR:
    case TRUTH_XOR_EXPR:
    case TRUTH_NOT_EXPR:
      TREE_TYPE (expr) = boolean_type_node;
      return expr;

    case ERROR_MARK:
      return expr;

    case INTEGER_CST:
      return integer_zerop (expr) ? boolean_false_node : boolean_true_node;

    case REAL_CST:
      return real_zerop (expr) ? boolean_false_node : boolean_true_node;

    case ADDR_EXPR:
      /* If we are taking the address of an external decl, it might be zero
	 if it is weak, so we cannot optimize.  */
      if (DECL_P (TREE_OPERAND (expr, 0))
	  && DECL_EXTERNAL (TREE_OPERAND (expr, 0)))
	break;

      if (TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 0)))
	return build (COMPOUND_EXPR, boolean_type_node,
		      TREE_OPERAND (expr, 0), boolean_true_node);
      else
	return boolean_true_node;

    case COMPLEX_EXPR:
      return build_buul_binary_op ((TREE_SIDE_EFFECTS (TREE_OPERAND (expr, 1))
				  ? TRUTH_OR_EXPR : TRUTH_ORIF_EXPR),
	  d_truthvalue_conversion (TREE_OPERAND (expr, 0)),
	  d_truthvalue_conversion (TREE_OPERAND (expr, 1)),
	  0);

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
	return build (COMPOUND_EXPR, boolean_type_node, TREE_OPERAND (expr, 1),
		      d_truthvalue_conversion (TREE_OPERAND (expr, 0)));
      else
	return d_truthvalue_conversion (TREE_OPERAND (expr, 0));

    case COND_EXPR:
      /* Distribute the conversion into the arms of a COND_EXPR.  */
      return fold (build (COND_EXPR, boolean_type_node, TREE_OPERAND (expr, 0),
		d_truthvalue_conversion (TREE_OPERAND (expr, 1)),
		d_truthvalue_conversion (TREE_OPERAND (expr, 2))));

    case CONVERT_EXPR:
      /* Don't cancel the effect of a CONVERT_EXPR from a REFERENCE_TYPE,
	 since that affects how `default_conversion' will behave.  */
      if (TREE_CODE (TREE_TYPE (expr)) == REFERENCE_TYPE
	  || TREE_CODE (TREE_TYPE (TREE_OPERAND (expr, 0))) == REFERENCE_TYPE)
	break;
      /* fall through...  */
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
      /* fall through...  */
    case BIT_XOR_EXPR:
      /* This and MINUS_EXPR can be changed into a comparison of the
	 two objects.  */
      if (TREE_TYPE (TREE_OPERAND (expr, 0))
	  == TREE_TYPE (TREE_OPERAND (expr, 1)))
	  return build_buul_binary_op (NE_EXPR, TREE_OPERAND (expr, 0),
	      TREE_OPERAND (expr, 1), 1);
      return build_buul_binary_op (NE_EXPR, TREE_OPERAND (expr, 0),
	  fold (build1 (NOP_EXPR,
		    TREE_TYPE (TREE_OPERAND (expr, 0)),
		    TREE_OPERAND (expr, 1))), 1);
    case BIT_AND_EXPR:
      if (integer_onep (TREE_OPERAND (expr, 1))
	  && TREE_TYPE (expr) != boolean_type_node)
	/* Using convert here would cause infinite recursion.  */
	return build1 (NOP_EXPR, boolean_type_node, expr);
      break;

    case MODIFY_EXPR:
	/*
      if (warn_parentheses && C_EXP_ORIGINAL_CODE (expr) == MODIFY_EXPR)
	warning ("suggest parentheses around assignment used as truth value");
	*/
      break;

    default:
      break;
    }

  tree t_zero = integer_zero_node;
  
  if (TREE_CODE (TREE_TYPE (expr)) == COMPLEX_TYPE)
    {
      tree t = save_expr (expr);
      tree compon_type = TREE_TYPE( TREE_TYPE( expr ));
      return (build_buul_binary_op
	      ((TREE_SIDE_EFFECTS (expr)
		? TRUTH_OR_EXPR : TRUTH_ORIF_EXPR),
	d_truthvalue_conversion (build1 (REALPART_EXPR, compon_type, t)),
	d_truthvalue_conversion (build1 (IMAGPART_EXPR, compon_type, t)),
	       0));
    }

#if D_GCC_VER >= 40
  /* Without this, the backend tries to load a float reg with and integer
     value with fails (on i386 and rs6000, at least). */
  else if ( SCALAR_FLOAT_TYPE_P( TREE_TYPE( expr )))
    {
	t_zero = convert( TREE_TYPE(expr), t_zero );
    }
#endif
  

  return build_buul_binary_op (NE_EXPR, expr, t_zero, 1);
}

