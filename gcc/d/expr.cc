// expr.cc -- D frontend for GCC.
// Copyright (C) 2011-2016 Free Software Foundation, Inc.

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
#include "dfrontend/init.h"
#include "dfrontend/module.h"
#include "dfrontend/statement.h"
#include "dfrontend/ctfe.h"

#include "d-system.h"
#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "d-dmd-gcc.h"
#include "id.h"

// Implements the visitor interface to build the GCC trees of all Expression
// AST classes emitted from the D Front-end.
// All visit methods accept one parameter E, which holds the frontend AST
// of the expression to compile.  They also don't return any value, instead
// generated code is cached in RESULT_ and returned from the caller.

class ExprVisitor : public Visitor
{
  tree result_;
  bool constp_;

  // Determine if type is an array of structs that need a postblit.
  bool needs_postblit(Type *t)
  {
    t = t->baseElemOf();

    if (t->ty == Tstruct)
      {
	StructDeclaration *sd = ((TypeStruct *) t)->sym;
	if (sd->postblit)
	  return true;
      }

    return false;
  }

public:
  ExprVisitor(bool constp)
  {
    this->result_ = NULL_TREE;
    this->constp_ = constp;
  }

  //
  tree result()
  {
    return this->result_;
  }

  // Visitor interfaces.

  // This should be overridden by each expression class.
  void visit(Expression *e)
  {
    set_input_location(e->loc);
    gcc_unreachable();
  }

  //
  void visit(CondExp *e)
  {
    tree cond = convert_for_condition(build_expr(e->econd),
				      e->econd->type);
    tree t1 = build_expr_dtor(e->e1);
    tree t2 = build_expr_dtor(e->e2);

    if (e->type->ty != Tvoid)
      {
	t1 = convert_expr(t1, e->e1->type, e->type);
	t2 = convert_expr(t2, e->e2->type, e->type);
      }

    this->result_ = build_condition(build_ctype(e->type), cond, t1, t2);
  }

  //
  void visit(IdentityExp *e)
  {
    Type *tb1 = e->e1->type->toBasetype();
    Type *tb2 = e->e2->type->toBasetype();

    tree_code code = (e->op == TOKidentity) ? EQ_EXPR : NE_EXPR;
    if ((tb1->ty == Tsarray || tb1->ty == Tarray)
	&& (tb2->ty == Tsarray || tb2->ty == Tarray))
      {
	// Convert arrays to D array types.
	tree t1 = d_array_convert(e->e1);
	tree t2 = d_array_convert(e->e2);
	this->result_ = d_convert(build_ctype(e->type),
				  build_boolop(code, t1, t2));
      }
    else if (tb1->isfloating())
      {
	tree t1 = build_expr(e->e1);
	t1 = maybe_make_temp(t1);
	tree t2 = build_expr(e->e2);
	t2 = maybe_make_temp(t2);
	// Assume all padding is at the end of the type.
	tree size = size_int(TYPE_PRECISION (TREE_TYPE (t1)) / BITS_PER_UNIT);
	// Do bit compare of floats.
	tree tmemcmp = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMCMP), 3,
					 build_address(t1), build_address(t2), size);
	this->result_ = build_boolop(code, tmemcmp, integer_zero_node);
      }
    else if (tb1->ty == Tstruct)
      {
	StructDeclaration *sd = ((TypeStruct *) tb1)->sym;
	tree t1 = build_expr(e->e1);
	tree t2 = build_expr(e->e2);

	gcc_assert(d_types_same(tb1, tb2));

	this->result_ = build_struct_comparison(code, sd, t1, t2);
      }
    else
      {
	// For operands of other types, identity is defined as being the
	// same as equality expressions.
	tree t1 = build_expr(e->e1);
	tree t2 = build_expr(e->e2);
	this->result_ = d_convert(build_ctype(e->type),
				  build_boolop(code, t1, t2));

      }
  }

  //
  void visit(EqualExp *e)
  {
    Type *tb1 = e->e1->type->toBasetype();
    Type *tb2 = e->e2->type->toBasetype();
    tree_code code = (e->op == TOKequal) ? EQ_EXPR : NE_EXPR;

    if ((tb1->ty == Tsarray || tb1->ty == Tarray)
	&& (tb2->ty == Tsarray || tb2->ty == Tarray))
      {
	Type *t1elem = tb1->nextOf()->toBasetype();
	Type *t2elem = tb1->nextOf()->toBasetype();

	// Check if comparisons of arrays can be optimized using memcmp.
	// This will inline EQ expressions as:
	//    e1.length == e2.length && memcmp(e1.ptr, e2.ptr, size) == 0;
	// Or when generating a NE expression:
	//    e1.length != e2.length || memcmp(e1.ptr, e2.ptr, size) != 0;
	if ((t1elem->isintegral() || t1elem->ty == Tvoid || t1elem->ty == Tstruct)
	    && t1elem->ty == t2elem->ty)
	  {
	    tree t1 = d_array_convert(e->e1);
	    tree t2 = d_array_convert(e->e2);
	    tree result;

	    // Make temporaries to prevent multiple evaluations.
	    tree t1saved = make_temp(t1);
	    tree t2saved = make_temp(t2);

	    // Length of arrays, for comparisons done before calling memcmp.
	    tree t1len = d_array_length(t1saved);
	    tree t2len = d_array_length(t2saved);

	    // Reference to array data.
	    tree t1ptr = d_array_ptr(t1saved);
	    tree t2ptr = d_array_ptr(t2saved);

	    // Compare arrays using memcmp if possible, otherwise for structs,
	    // each field is compared inline.
	    if (t1elem->ty != Tstruct
		|| identity_compare_p(((TypeStruct *) t1elem)->sym))
	      {
		tree tsize = size_mult_expr(t1len, size_int(t1elem->size()));
		tree tmemcmp = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMCMP), 3,
						 t1ptr, t2ptr, tsize);
		result = build2(code, build_ctype(e->type),
				tmemcmp, integer_zero_node);
	      }
	    else
	      {
		result = build_array_struct_comparison(code, ((TypeStruct *) t1elem)->sym,
						       t1len, t1ptr, t2ptr);
	      }

	    // Check array length first before passing to memcmp.
	    // For equality expressions, this becomes:
	    //    (e1.length == 0 || memcmp)
	    // Otherwise for inequality:
	    //    (e1.length != 0 && memcmp)
	    tree tsizecmp = build2(code, size_type_node, t1len, size_zero_node);
	    if (e->op == TOKequal)
	      result = build_boolop(TRUTH_ORIF_EXPR, tsizecmp, result);
	    else
	      result = build_boolop(TRUTH_ANDIF_EXPR, tsizecmp, result);

	    // Finally, check if lengths of both arrays match.  The frontend
	    // should have already guaranteed that static arrays have same size.
	    if (tb1->ty == Tsarray && tb2->ty == Tsarray)
	      gcc_assert(tb1->size() == tb2->size());
	    else
	      {
		tree tlencmp = build2(code, size_type_node, t1len, t2len);
		if (e->op == TOKequal)
		  result = build_boolop(TRUTH_ANDIF_EXPR, tlencmp, result);
		else
		  result = build_boolop(TRUTH_ORIF_EXPR, tlencmp, result);
	      }

	    // Ensure left-to-right order of evaluation.
	    if (d_has_side_effects(t2))
	      result = compound_expr(t2saved, result);

	    if (d_has_side_effects(t1))
	      result = compound_expr(t1saved, result);

	    this->result_ = result;
	  }
	else
	  {
	    // _adEq2 compares each element.
	    tree args[3];

	    args[0] = d_array_convert(e->e1);
	    args[1] = d_array_convert(e->e2);
	    args[2] = build_typeinfo(t1elem->arrayOf());

	    tree result = d_convert(build_ctype(e->type),
				    build_libcall(LIBCALL_ADEQ2, 3, args));

	    if (e->op == TOKnotequal)
	      result = build1(TRUTH_NOT_EXPR, build_ctype(e->type), result);

	    this->result_ = result;
	  }
      }
    else if (tb1->ty == Tstruct)
      {
	StructDeclaration *sd = ((TypeStruct *) tb1)->sym;
	tree t1 = build_expr(e->e1);
	tree t2 = build_expr(e->e2);

	gcc_assert(d_types_same(tb1, tb2));

	this->result_ = build_struct_comparison(code, sd, t1, t2);
      }
    else if (tb1->ty == Taarray && tb2->ty == Taarray)
      {
	TypeAArray *taa1 = (TypeAArray *) tb1;
	tree args[3];

	args[0] = build_typeinfo(taa1);
	args[1] = build_expr(e->e1);
	args[2] = build_expr(e->e2);

	tree result = d_convert(build_ctype(e->type),
				build_libcall(LIBCALL_AAEQUAL, 3, args));

	if (e->op == TOKnotequal)
	  result = build1(TRUTH_NOT_EXPR, build_ctype(e->type), result);

	this->result_ = result;
      }
    else
      {
	tree t1 = build_expr(e->e1);
	tree t2 = build_expr(e->e2);

	this->result_ = d_convert(build_ctype(e->type),
				  build_boolop(code, t1, t2));
      }
  }

  //
  void visit(InExp *e)
  {
    Type *tb2 = e->e2->type->toBasetype();
    gcc_assert(tb2->ty == Taarray);

    Type *tkey = ((TypeAArray *) tb2)->index->toBasetype();
    tree key = convert_expr(build_expr(e->e1), e->e1->type, tkey);
    tree args[3];

    args[0] = build_expr(e->e2);
    args[1] = build_typeinfo(tkey);
    args[2] = build_address(key);

    tree result = build_libcall(LIBCALL_AAINX, 3, args);
    this->result_ = convert(build_ctype(e->type), result);
  }

  //
  void visit(CmpExp *e)
  {
    Type *tb1 = e->e1->type->toBasetype();
    Type *tb2 = e->e2->type->toBasetype();

    tree result;
    tree_code code;

    switch (e->op)
      {
      case TOKue:
	code = tb1->isfloating() && tb2->isfloating() ?
	  UNEQ_EXPR : EQ_EXPR;
	break;

      case TOKlg:
	code = tb1->isfloating() && tb2->isfloating()
	  ? LTGT_EXPR : NE_EXPR;
	break;

      case TOKule:
	code = tb1->isfloating() && tb2->isfloating()
	  ? UNLE_EXPR : LE_EXPR;
	break;

      case TOKul:
	code = tb1->isfloating() && tb2->isfloating()
	  ? UNLT_EXPR : LT_EXPR;
	break;

      case TOKuge:
	code = tb1->isfloating() && tb2->isfloating()
	  ? UNGE_EXPR : GE_EXPR;
	break;

      case TOKug:
	code = tb1->isfloating() && tb2->isfloating()
	  ? UNGT_EXPR : GT_EXPR;
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

	args[0] = d_array_convert(e->e1);
	args[1] = d_array_convert(e->e2);
	args[2] = build_typeinfo(telem->arrayOf());
	result = build_libcall(LIBCALL_ADCMP2, 3, args);

	// %% For float element types, warn that NaN is not taken into account?
	// %% Could do a check for side effects and drop the unused condition
	if (code == ORDERED_EXPR)
	  {
	    this->result_ = build_boolop(COMPOUND_EXPR, result,
					 d_truthvalue_conversion(integer_one_node));
	  }
	else if (code == UNORDERED_EXPR)
	  {
	    this->result_ = build_boolop(COMPOUND_EXPR, result,
					 d_truthvalue_conversion(integer_zero_node));
	  }
	else
	  {
	    result = build_boolop(code, result, integer_zero_node);
	    this->result_ = d_convert(build_ctype(e->type), result);
	  }
	return;
      }

    if (!tb1->isfloating() || !tb2->isfloating())
      {
	// %% is this properly optimized away?
	if (code == ORDERED_EXPR)
	  {
	    this->result_ = convert(bool_type_node, integer_one_node);
	    return;
	  }

	if (code == UNORDERED_EXPR)
	  {
	    this->result_ = convert(bool_type_node, integer_zero_node);
	    return;
	  }
      }

    result = build_boolop(code, build_expr(e->e1), build_expr(e->e2));
    this->result_ = d_convert(build_ctype(e->type), result);
  }

  //
  void visit(AndAndExp *e)
  {
    if (e->e2->type->toBasetype()->ty != Tvoid)
      {
	tree t1 = build_expr(e->e1);
	tree t2 = build_expr(e->e2);

	t1 = convert_for_condition(t1, e->e1->type);
	t2 = convert_for_condition(t2, e->e2->type);

	this->result_ = d_convert(build_ctype(e->type),
				  build_boolop(TRUTH_ANDIF_EXPR, t1, t2));
      }
    else
      {
	tree t1 = convert_for_condition(build_expr(e->e1), e->e1->type);
	tree t2 = build_expr_dtor(e->e2);

	this->result_ = build_condition(build_ctype(e->type), t1, t2, void_zero_node);
      }
  }

  //
  void visit(OrOrExp *e)
  {
    if (e->e2->type->toBasetype()->ty != Tvoid)
      {
	tree t1 = convert_for_condition(build_expr(e->e1), e->e1->type);
	tree t2 = convert_for_condition(build_expr(e->e2), e->e2->type);

	this->result_ = d_convert(build_ctype(e->type),
				  build_boolop(TRUTH_ORIF_EXPR, t1, t2));
      }
    else
      {
	tree t1 = convert_for_condition(build_expr(e->e1), e->e1->type);
	tree t2 = build_expr_dtor(e->e2);
	tree cond = build1(TRUTH_NOT_EXPR, bool_type_node, t1);

	this->result_ = build_condition(build_ctype(e->type), cond, t2, void_zero_node);
      }
  }

  //
  void visit(BinExp *e)
  {
    tree_code code;

    switch (e->op)
      {
      case TOKadd:
      case TOKmin:
	// If the result is complex, then we can shortcut build_binary_op.
	if ((e->e1->type->isreal() && e->e2->type->isimaginary())
	    || (e->e1->type->isimaginary() && e->e2->type->isreal()))
	  {
	    // Frontend should have already validated types and sizes.
	    tree t1 = build_expr(e->e1);
	    tree t2 = build_expr(e->e2);

	    if (e->op == TOKmin)
	      t2 = build1(NEGATE_EXPR, TREE_TYPE(t2), t2);

	    if (e->e1->type->isreal())
	      this->result_ = complex_expr(build_ctype(e->type), t1, t2);
	    else
	      this->result_ = complex_expr(build_ctype(e->type), t2, t1);

	    return;
	  }
	else
	  code = (e->op == TOKadd) ? PLUS_EXPR : MINUS_EXPR;
	break;

      case TOKmul:
	code = MULT_EXPR;
	break;

      case TOKdiv:
	code = e->e1->type->isintegral() ? TRUNC_DIV_EXPR : RDIV_EXPR;
	break;

      case TOKmod:
	code = e->e1->type->isfloating() ? FLOAT_MOD_EXPR : TRUNC_MOD_EXPR;
	break;

      case TOKand:
	code = BIT_AND_EXPR;
	break;

      case TOKor:
	code = BIT_IOR_EXPR;
	break;

      case TOKxor:
	code = BIT_XOR_EXPR;
	break;

      case TOKshl:
	code = LSHIFT_EXPR;
	  break;

      case TOKshr:
	code = RSHIFT_EXPR;
	break;

      case TOKushr:
	code = UNSIGNED_RSHIFT_EXPR;
	break;

      default:
	gcc_unreachable();
      }

    this->result_ = build_binary_op(code, build_ctype(e->type),
				    build_expr(e->e1), build_expr(e->e2));
  }

  //
  void visit(PowExp *e)
  {
    // Dictates what version of pow() we call.
    tree powtype = build_ctype(e->type->toBasetype());

    // If type is int, implicitly convert to double.
    // This allows backend to fold the call into a constant return value.
    if (e->type->isintegral())
      powtype = double_type_node;

    // Lookup compatible builtin. %% TODO: handle complex types?
    tree powfn = NULL_TREE;
    if (TYPE_MAIN_VARIANT (powtype) == double_type_node)
      powfn = builtin_decl_explicit(BUILT_IN_POW);
    else if (TYPE_MAIN_VARIANT (powtype) == float_type_node)
      powfn = builtin_decl_explicit(BUILT_IN_POWF);
    else if (TYPE_MAIN_VARIANT (powtype) == long_double_type_node)
      powfn = builtin_decl_explicit(BUILT_IN_POWL);

    if (powfn == NULL_TREE)
      {
	Type *tb1 = e->e1->type->toBasetype();

	if (tb1->ty == Tarray || tb1->ty == Tsarray)
	  error("Array operation %s not implemented", e->toChars());
	else
	  error("%s ^^ %s is not supported",
		e->e1->type->toChars(), e->e2->type->toChars());

	this->result_ = error_mark_node;
	return;
      }

    tree t1 = d_convert(powtype, build_expr(e->e1));
    tree t2 = d_convert(powtype, build_expr(e->e2));

    this->result_ = d_convert(build_ctype(e->type),
			      d_build_call_nary(powfn, 2, t1, t2));
  }

  //
  void visit(CatExp *e)
  {
    // One of the operands may be an element instead of an array.
    // Logic copied from CatExp::semantic
    Type *tb1 = e->e1->type->toBasetype();
    Type *tb2 = e->e2->type->toBasetype();
    Type *etype;

    if (tb1->ty == Tarray || tb1->ty == Tsarray)
      etype = tb1->nextOf();
    else
      etype = tb2->nextOf();

    vec<tree, va_gc> *elemvars = NULL;
    tree result;

    if (e->e1->op == TOKcat)
      {
	// Flatten multiple concatenations to an array.
	// So the expression ((a ~ b) ~ c) becomes [a, b, c]
	int ndims = 2;
	for (Expression *ex = e->e1; ex->op == TOKcat;)
	  {
	    if (ex->op == TOKcat)
	      {
		ex = ((CatExp *) ex)->e1;
		ndims++;
	      }
	  }

	// Store all concatenation args to a temporary byte[][ndims] array.
	Type *targselem = Type::tint8->arrayOf();
	tree var = create_temporary_var(d_array_type(targselem, ndims));
	tree init = build_constructor(TREE_TYPE(var), NULL);
	vec_safe_push(elemvars, var);

	// Loop through each concatenation from right to left.
	vec<constructor_elt, va_gc> *elms = NULL;
	CatExp *ce = e;
	int dim = ndims - 1;

	for (Expression *oe = ce->e2; oe != NULL;
	     (ce->e1->op != TOKcat
	      ? (oe = ce->e1)
	      : (ce = (CatExp *)ce->e1, oe = ce->e2)))
	  {
	    tree arg = d_array_convert(etype, oe, &elemvars);
	    tree index = size_int(dim);
	    CONSTRUCTOR_APPEND_ELT(elms, index, maybe_make_temp(arg));

	    // Finished pushing all arrays.
	    if (oe == ce->e1)
	      break;

	    dim -= 1;
	  }
	// Check there is no logic bug in constructing byte[][] of arrays.
	gcc_assert(dim == 0);

	CONSTRUCTOR_ELTS(init) = elms;
	DECL_INITIAL(var) = init;

	tree args[2];
	args[0] = build_typeinfo(e->type);
	args[1] = d_array_value(build_ctype(targselem->arrayOf()),
				size_int(ndims), build_address(var));

	result = build_libcall(LIBCALL_ARRAYCATNTX, 2, args,
			       build_ctype(e->type));
      }
    else
      {
	// Handle single concatenation (a ~ b)
	tree args[3];
	args[0] = build_typeinfo(e->type);
	args[1] = d_array_convert(etype, e->e1, &elemvars);
	args[2] = d_array_convert(etype, e->e2, &elemvars);

	result = build_libcall(LIBCALL_ARRAYCATT, 3, args,
			       build_ctype(e->type));
      }

    for (size_t i = 0; i < vec_safe_length(elemvars); ++i)
      result = bind_expr((*elemvars)[i], result);

    this->result_ = result;
  }

  //
  void visit(BinAssignExp *e)
  {
    tree_code code;

    switch (e->op)
      {
      case TOKaddass:
	code = PLUS_EXPR;
	break;

      case TOKminass:
	code = MINUS_EXPR;
	break;

      case TOKmulass:
	code = MULT_EXPR;
	break;

      case TOKdivass:
	code = e->e1->type->isintegral() ? TRUNC_DIV_EXPR : RDIV_EXPR;
	break;

      case TOKmodass:
	code = e->e1->type->isfloating() ? FLOAT_MOD_EXPR : TRUNC_MOD_EXPR;
	break;

      case TOKandass:
	code = BIT_AND_EXPR;
	break;

      case TOKorass:
	code = BIT_IOR_EXPR;
	break;

      case TOKxorass:
	code = BIT_XOR_EXPR;
	break;

      case TOKpowass:
	gcc_unreachable();

      case TOKshlass:
	code = LSHIFT_EXPR;
	break;

      case TOKshrass:
	code = RSHIFT_EXPR;
	break;

      default:
	gcc_unreachable();
      }

    tree exp = build_binop_assignment(code, e->e1, e->e2);
    this->result_ = convert_expr(exp, e->e1->type, e->type);
  }

  //
  void visit(UshrAssignExp *e)
  {
    // Front-end integer promotions don't work here.
    Expression *e1b = e->e1;
    while (e1b->op == TOKcast)
      {
	CastExp *ce = (CastExp *) e1b;
	gcc_assert(d_types_same(ce->type, ce->to));
	e1b = ce->e1;
      }

    tree exp = build_binop_assignment(UNSIGNED_RSHIFT_EXPR, e1b, e->e2);
    this->result_ = convert_expr(exp, e1b->type, e->type);
  }

  //
  void visit(CatAssignExp *e)
  {
    Type *tb1 = e->e1->type->toBasetype();
    Type *tb2 = e->e2->type->toBasetype();
    Type *etype = tb1->nextOf()->toBasetype();

    if (tb1->ty == Tarray && tb2->ty == Tdchar
	&& (etype->ty == Tchar || etype->ty == Twchar))
      {
	// Append a dchar to a char[] or wchar[]
	tree args[2];
	args[0] = build_address(build_expr(e->e1));
	args[1] = build_expr(e->e2);

	LibCall libcall = (etype->ty == Tchar)
	  ? LIBCALL_ARRAYAPPENDCD : LIBCALL_ARRAYAPPENDWD;

	this->result_ = build_libcall(libcall, 2, args, build_ctype(e->type));
      }
    else
      {
	gcc_assert(tb1->ty == Tarray || tb2->ty == Tsarray);

	if ((tb2->ty == Tarray || tb2->ty == Tsarray)
	    && d_types_same(etype, tb2->nextOf()->toBasetype()))
	  {
	    // Append an array
	    tree args[3];

	    args[0] = build_typeinfo(e->type);
	    args[1] = build_address(build_expr(e->e1));
	    args[2] = d_array_convert(e->e2);

	    this->result_ = build_libcall(LIBCALL_ARRAYAPPENDT, 3, args,
					  build_ctype(e->type));
	  }
	else if (d_types_same(etype, tb2))
	  {
	    // Append an element
	    tree args[3];

	    args[0] = build_typeinfo(e->type);
	    args[1] = build_address(build_expr(e->e1));
	    args[2] = size_one_node;

	    tree result = build_libcall(LIBCALL_ARRAYAPPENDCTX, 3, args,
					build_ctype(e->type));
	    result = make_temp(result);

	    // Assign e2 to last element
	    tree offexp = d_array_length(result);
	    offexp = build2(MINUS_EXPR, TREE_TYPE (offexp),
			    offexp, size_one_node);
	    offexp = maybe_make_temp(offexp);

	    tree ptrexp = d_array_ptr(result);
	    ptrexp = void_okay_p(ptrexp);
	    ptrexp = build_array_index(ptrexp, offexp);

	    // Evaluate expression before appending
	    tree t2 = build_expr(e->e2);
	    t2 = maybe_make_temp(t2);
	    result = modify_expr(build_ctype(etype), build_deref(ptrexp), t2);

	    this->result_ = compound_expr(t2, result);
	  }
	else
	  gcc_unreachable();
      }
  }

  //
  void visit(AssignExp *e)
  {
    // First, handle special assignment semantics

    // Look for array.length = n;
    if (e->e1->op == TOKarraylength)
      {
	// Assignment to an array's length property; resize the array.
	ArrayLengthExp *ale = (ArrayLengthExp *) e->e1;
	tree args[3];

	args[0] = build_typeinfo(ale->e1->type);
	args[1] = convert_expr(build_expr(e->e2), e->e2->type, Type::tsize_t);
	args[2] = build_address(build_expr(ale->e1));

	// Don't want ->toBasetype() for the element type.
	Type *etype = ale->e1->type->toBasetype()->nextOf();
	LibCall libcall = etype->isZeroInit()
	  ? LIBCALL_ARRAYSETLENGTHT : LIBCALL_ARRAYSETLENGTHIT;

	tree result = build_libcall(libcall, 3, args);
	this->result_ = d_array_length(result);
	return;
      }

    // Look for array[] = n;
    if (e->e1->op == TOKslice)
      {
	SliceExp *se = (SliceExp *) e->e1;
	Type *stype = se->e1->type->toBasetype();
	Type *etype = stype->nextOf()->toBasetype();

	// Determine if we need to run postblit or dtor.
	bool postblit = false;

	if (this->needs_postblit(etype)
	    && ((e->e2->op != TOKslice && e->e2->isLvalue())
		|| (e->e2->op == TOKslice && ((UnaExp *) e->e2)->e1->isLvalue())
		|| (e->e2->op == TOKcast && ((UnaExp *) e->e2)->e1->isLvalue())))
	  postblit = true;

	if (e->ismemset)
	  {
	    // Set a range of elements to one value.
	    tree t1 = maybe_make_temp(build_expr(e->e1));
	    tree t2 = build_expr(e->e2);
	    tree result;

	    if (postblit && e->op != TOKblit)
	      {
		tree args[4];

		args[0] = d_array_ptr(t1);
		args[1] = build_address(t2);
		args[2] = d_array_length(t1);
		args[3] = build_typeinfo(etype);

		LibCall libcall = (e->op == TOKconstruct)
		  ? LIBCALL_ARRAYSETCTOR : LIBCALL_ARRAYSETASSIGN;
		tree call = build_libcall(libcall, 4, args);

		this->result_ = compound_expr(call, t1);
		return;
	      }

	    if (integer_zerop(t2))
	      {
		tree size = size_mult_expr(d_array_length(t1),
					   size_int(etype->size()));

		result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMSET), 3,
					   d_array_ptr(t1), integer_zero_node, size);
	      }
	    else
	      result = build_array_set(d_array_ptr(t1), d_array_length(t1), t2);

	    this->result_ = compound_expr(result, t1);
	  }
	else
	  {
	    // Perform a memcpy operation.
	    gcc_assert(e->e2->type->ty != Tpointer);

	    if (!postblit && !array_bounds_check())
	      {
		tree t1 = maybe_make_temp(d_array_convert(e->e1));
		tree t2 = d_array_convert(e->e2);
		tree size = size_mult_expr(d_array_length(t1),
					   size_int(etype->size()));

		tree result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMCPY), 3,
						d_array_ptr(t1), d_array_ptr(t2), size);
		this->result_ = compound_expr(result, t1);
	      }
	    else if (postblit && e->op != TOKblit)
	      {
		// Generate:
		//  _d_arrayassign(ti, from, to) or _d_arrayctor(ti, from, to)
		tree args[3];

		args[0] = build_typeinfo(etype);
		args[1] = maybe_make_temp(d_array_convert(e->e2));
		args[2] = d_array_convert(e->e1);

		LibCall libcall = (e->op == TOKconstruct)
		  ? LIBCALL_ARRAYCTOR : LIBCALL_ARRAYASSIGN;

		this->result_ = build_libcall(libcall, 3, args,
					      build_ctype(e->type));
	      }
	    else
	      {
		// Generate _d_arraycopy()
		tree args[3];

		args[0] = size_int(etype->size());
		args[1] = maybe_make_temp(d_array_convert(e->e2));
		args[2] = d_array_convert(e->e1);

		this->result_ = build_libcall(LIBCALL_ARRAYCOPY, 3, args,
					      build_ctype(e->type));
	      }
	  }

	return;
      }

    // Look for reference initializations
    if (e->op == TOKconstruct && e->e1->op == TOKvar)
      {
	Declaration *decl = ((VarExp *) e->e1)->var;
	if (decl->storage_class & (STCout | STCref))
	  {
	    tree t2 = convert_for_assignment(build_expr(e->e2),
					     e->e2->type, e->e1->type);
	    tree t1 = build_expr(e->e1);
	    // Want reference to lhs, not indirect ref.
	    t1 = TREE_OPERAND(t1, 0);
	    t2 = build_address(t2);

	    this->result_ = indirect_ref(build_ctype(e->type),
					 modify_expr(t1, t2));
	    return;
	  }
      }

    // Other types of assignments that may require post construction.
    Type *tb1 = e->e1->type->toBasetype();

    if (tb1->ty == Tstruct)
      {
	tree t1 = build_expr(e->e1);
	tree t2 = convert_for_assignment(build_expr(e->e2),
					 e->e2->type, e->e1->type);

	if (e->op == TOKconstruct && TREE_CODE (t2) == CALL_EXPR
	    && aggregate_value_p(TREE_TYPE (t2), t2))
	  CALL_EXPR_RETURN_SLOT_OPT (t2) = true;

	if (e->e2->op == TOKint64)
	  {
	    // Use memset to fill struct.
	    StructDeclaration *sd = ((TypeStruct *) tb1)->sym;

	    tree result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMSET), 3,
					    build_address(t1), t2,
					    size_int(sd->structsize));

	    // Maybe set-up hidden pointer to outer scope context.
	    if (sd->isNested())
	      {
		tree field = sd->vthis->toSymbol()->Stree;
		tree value = build_vthis(sd);

		tree vthis_exp = modify_expr(component_ref(t1, field), value);
		result = compound_expr(result, vthis_exp);
	      }

	    this->result_ = compound_expr(result, t1);
	  }
	else
	  this->result_ = modify_expr(build_ctype(e->type), t1, t2);

	return;
      }

    if (tb1->ty == Tsarray)
      {
	Type *etype = tb1->nextOf();
	gcc_assert(e->e2->type->toBasetype()->ty == Tsarray);

	// Determine if we need to run postblit.
	bool postblit = this->needs_postblit(etype);
	bool lvalue_p = false;

	if ((e->e2->op != TOKslice && e->e2->isLvalue())
	    || (e->e2->op == TOKslice && ((UnaExp *) e->e2)->e1->isLvalue())
	    || (e->e2->op == TOKcast && ((UnaExp *) e->e2)->e1->isLvalue()))
	  lvalue_p = true;

	// Even if the elements in rhs are all rvalues and don't have to call
	// postblits, this assignment should call dtors on old assigned elements.
	if (!postblit
	    || (e->op == TOKconstruct && !lvalue_p && postblit)
	    || (e->op == TOKblit || e->e1->type->size() == 0))
	  {
	    tree t1 = build_expr(e->e1);
	    tree t2 = convert_for_assignment(build_expr(e->e2),
					     e->e2->type, e->e1->type);

	    if (e->op == TOKconstruct && TREE_CODE (t2) == CALL_EXPR
		&& aggregate_value_p(TREE_TYPE (t2), t2))
	      CALL_EXPR_RETURN_SLOT_OPT (t2) = true;

	    this->result_ = modify_expr(build_ctype(e->type), t1, t2);
	  }
	else if (e->op == TOKconstruct)
	  {
	    // Generate _d_arrayassign(ti, from, to) or _d_arrayctor(ti, from, to)
	    tree args[3];

	    args[0] = build_typeinfo(etype);
	    args[1] = d_array_convert(e->e2);
	    args[2] = d_array_convert(e->e1);

	    tree result = build_libcall(LIBCALL_ARRAYCTOR, 3, args);
	    this->result_ = compound_expr(result, build_expr(e->e1));
	  }
	else
	  {
	    // Generate _d_arrayassign_l() or _d_arrayassign_r()
	    tree args[4];
	    LibCall libcall;
	    tree elembuf = build_local_temp(build_ctype(etype));

	    args[0] = build_typeinfo(etype);
	    args[1] = d_array_convert(e->e2);
	    args[2] = d_array_convert(e->e1);
	    args[3] = build_address(elembuf);
	    libcall = (lvalue_p) ? LIBCALL_ARRAYASSIGN_L : LIBCALL_ARRAYASSIGN_R;

	    tree result = build_libcall(libcall, 4, args);
	    this->result_ = compound_expr(result, build_expr(e->e1));
	  }

	return;
      }

    // Simple assignment
    tree t1 = build_expr(e->e1);
    tree t2 = convert_for_assignment(build_expr(e->e2),
				     e->e2->type, e->e1->type);

    this->result_ = modify_expr(build_ctype(e->type), t1, t2);
  }

  //
  void visit(PostExp *e)
  {
    tree result;

    if (e->op == TOKplusplus)
      {
	result = build2 (POSTINCREMENT_EXPR, build_ctype(e->type),
			 build_expr(e->e1), build_expr(e->e2));
      }
    else if (e->op == TOKminusminus)
      {
	result = build2 (POSTDECREMENT_EXPR, build_ctype(e->type),
			 build_expr(e->e1), build_expr(e->e2));
      }
    else
      gcc_unreachable();

    TREE_SIDE_EFFECTS (result) = 1;
    this->result_ = result;
  }

  //
  void visit(IndexExp *e)
  {
    Type *tb1 = e->e1->type->toBasetype();

    if (tb1->ty == Taarray)
      {
	// Get the key for the associative array.
	Type *tkey = ((TypeAArray *) tb1)->index->toBasetype();
	tree key = convert_expr(build_expr(e->e2), e->e2->type, tkey);
	LibCall libcall;
	tree args[4];

	if (e->modifiable)
	  {
	    libcall = LIBCALL_AAGETY;
	    args[0] = build_address(build_expr(e->e1));
	    args[1] = build_typeinfo(tb1->unSharedOf()->mutableOf());
	  }
	else
	  {
	    libcall = LIBCALL_AAGETRVALUEX;
	    args[0] = build_expr(e->e1);
	    args[1] = build_typeinfo(tkey);
	  }

	args[2] = size_int(tb1->nextOf()->size());
	args[3] = build_address(key);

	// Index the associative array.
	tree result = build_libcall(libcall, 4, args,
				    build_ctype(e->type->pointerTo()));

	if (!e->indexIsInBounds && array_bounds_check())
	  {
	    result = make_temp(result);
	    result = build_condition(TREE_TYPE (result),
				     d_truthvalue_conversion(result), result,
				     d_assert_call(e->loc, LIBCALL_ARRAY_BOUNDS));
	  }

	this->result_ = indirect_ref(build_ctype(e->type), result);
      }
    else
      {
	// Get the data pointer and length for static and dynamic arrays.
	tree array = maybe_make_temp(build_expr(e->e1));
	tree ptr = convert_expr(array, tb1, tb1->nextOf()->pointerTo());

	tree length = NULL_TREE;
	if (tb1->ty != Tpointer)
	  length = get_array_length(array, tb1);
	else
	  gcc_assert(e->lengthVar == NULL);

	// The __dollar variable just becomes a placeholder for the actual length.
	if (e->lengthVar)
	  {
	    e->lengthVar->csym = new Symbol();
	    e->lengthVar->csym->Stree = length;
	  }

	// Generate the index.
	tree index = build_expr(e->e2);

	// If it's a static array and the index is constant,
	// the front end has already checked the bounds.
	if (tb1->ty != Tpointer && !e->indexIsInBounds)
	  index = build_bounds_condition(e->e2->loc, index, length, false);

	// Index the .ptr
	ptr = void_okay_p(ptr);
	this->result_ = indirect_ref(TREE_TYPE (TREE_TYPE (ptr)),
				     build_array_index(ptr, index));
      }
  }

  //
  void visit(CommaExp *e)
  {
    tree t1 = build_expr(e->e1);
    tree t2 = build_expr(e->e2);
    tree type = e->type ? build_ctype(e->type) : void_type_node;

    this->result_ = build2(COMPOUND_EXPR, type, t1, t2);
  }

  //
  void visit(ArrayLengthExp *e)
  {
    if (e->e1->type->toBasetype()->ty == Tarray)
      this->result_ = d_array_length(build_expr(e->e1));
    else
      {
	// Tsarray case seems to be handled by front-end
	error("unexpected type for array length: %s",
	      e->type->toChars());

	this->result_ = error_mark_node;
      }
  }

  //
  void visit(DelegatePtrExp *e)
  {
    tree t1 = build_expr(e->e1);
    this->result_ = delegate_object(t1);
  }

  //
  void visit(DelegateFuncptrExp *e)
  {
    tree t1 = build_expr(e->e1);
    this->result_ = delegate_method(t1);
  }

  //
  void visit(SliceExp *e)
  {
    Type *tb = e->type->toBasetype();
    Type *tb1 = e->e1->type->toBasetype();
    gcc_assert(tb->ty == Tarray || tb->ty == Tsarray);

    // Use convert-to-dynamic-array code if possible
    if(!e->lwr)
      {
	tree result = build_expr(e->e1);
	if (e->e1->type->toBasetype()->ty == Tsarray)
	  result = convert_expr(result, e->e1->type, e->type);

	this->result_ = result;
	return;
      }
    else
      gcc_assert(e->upr != NULL);

    // Get the data pointer and length for static and dynamic arrays.
    tree array = maybe_make_temp(build_expr(e->e1));
    tree ptr = convert_expr(array, tb1, tb1->nextOf()->pointerTo());
    tree length = NULL_TREE;

    // Our array is already a SAVE_EXPR if necessary, so we don't make length
    // a SAVE_EXPR which is, at most, a COMPONENT_REF on top of array.
    if (tb1->ty != Tpointer)
      length = get_array_length(array, tb1);
    else
      gcc_assert(e->lengthVar == NULL);

    // The __dollar variable just becomes a placeholder for the actual length.
    if (e->lengthVar)
      {
	e->lengthVar->csym = new Symbol();
	e->lengthVar->csym->Stree = length;
      }

    // Generate lower bound.
    tree lwr_tree = maybe_make_temp(build_expr(e->lwr));

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
      {
	this->result_ = indirect_ref(build_ctype(e->type), ptr);
	return;
      }
    else
      gcc_assert(tb->ty == Tarray);

    // Generate upper bound with bounds checking.
    tree upr_tree = maybe_make_temp(build_expr(e->upr));
    tree newlength;

    if (!e->upperIsInBounds)
      {
	if (length)
	  {
	    newlength = build_bounds_condition(e->upr->loc, upr_tree,
					       length, true);
	  }
	else
	  {
	    // Still need to check bounds lwr <= upr for pointers.
	    gcc_assert(tb1->ty == Tpointer);
	    newlength = upr_tree;
	  }
      }
    else
      newlength = upr_tree;

    if (lwr_tree)
      {
	// Enforces lwr <= upr. No need to check lwr <= length as
	// we've already ensured that upr <= length.
	if (!e->lowerIsLessThanUpper)
	  {
	    tree cond = build_bounds_condition(e->lwr->loc, lwr_tree,
					       upr_tree, true);

	    // When bounds checking is off, the index value is returned directly.
	    if (cond != lwr_tree)
	      newlength = compound_expr(cond, newlength);
	  }

	// Need to ensure lwr always gets evaluated first, as it may be a
	// function call.  Generates (lwr, upr) - lwr.
	newlength = fold_build2(MINUS_EXPR, TREE_TYPE (newlength),
				compound_expr(lwr_tree, newlength), lwr_tree);
      }

    tree result = d_array_value(build_ctype(e->type), newlength, ptr);
    this->result_ = compound_expr(array, result);
  }

  //
  void visit(CastExp *e)
  {
    Type *ebtype = e->e1->type->toBasetype();
    Type *tbtype = e->to->toBasetype();
    tree result = build_expr(e->e1, this->constp_);

    // Just evaluate e1 if it has any side effects
    if (tbtype->ty == Tvoid)
      this->result_ = build_nop(build_ctype(tbtype), result);
    else
      this->result_ = convert_expr(result, ebtype, tbtype);
  }

  //
  void visit(BoolExp *e)
  {
    // Check, should we instead do truthvalue conversion?
    this->result_ = d_convert(build_ctype(e->type), build_expr(e->e1));
  }

  //
  void visit(DeleteExp *e)
  {
    tree t1 = build_expr(e->e1);
    Type *tb1 = e->e1->type->toBasetype();

    if (tb1->ty == Tclass)
      {
	LibCall libcall;

	if (e->e1->op == TOKvar)
	  {
	    VarDeclaration *v = ((VarExp *) e->e1)->var->isVarDeclaration();
	    if (v && v->onstack)
	      {
		libcall = tb1->isClassHandle()->isInterfaceDeclaration()
		  ? LIBCALL_CALLINTERFACEFINALIZER : LIBCALL_CALLFINALIZER;

		this->result_ = build_libcall(libcall, 1, &t1);
		return;
	      }
	  }

	libcall = tb1->isClassHandle()->isInterfaceDeclaration()
	  ? LIBCALL_DELINTERFACE : LIBCALL_DELCLASS;

	t1 = build_address(t1);
	this->result_ = build_libcall(libcall, 1, &t1);
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
	      ti = build_expr(getTypeInfo(tb1->nextOf(), NULL));
	  }

	// call _delarray_t (&t1, ti);
	args[0] = build_address(t1);
	args[1] = ti;

	this->result_ = build_libcall(LIBCALL_DELARRAYT, 2, args);
      }
    else if (tb1->ty == Tpointer)
      {
	t1 = build_address(t1);
	Type *tnext = ((TypePointer *)tb1)->next->toBasetype();
	if (tnext->ty == Tstruct)
	  {
	    TypeStruct *ts = (TypeStruct *)tnext;
	    if (ts->sym->dtor)
	      {
		tree args[2];
		args[0] = t1;
		args[1] = build_expr(getTypeInfo(tnext, NULL));

		this->result_ = build_libcall(LIBCALL_DELSTRUCT, 2, args);
		return;
	      }
	  }

	this->result_ = build_libcall(LIBCALL_DELMEMORY, 1, &t1);
      }
    else
      {
	error("don't know how to delete %s", e->e1->toChars());
	this->result_ = error_mark_node;
      }
  }

  //
  void visit(RemoveExp *e)
  {
    // Check that the array is actually an associative array
    if (e->e1->type->toBasetype()->ty == Taarray)
      {
	Type *tb = e->e1->type->toBasetype();
	Type *tkey = ((TypeAArray *) tb)->index->toBasetype();
	tree index = convert_expr(build_expr(e->e2), e->e2->type, tkey);
	tree args[3];

	args[0] = build_expr(e->e1);
	args[1] = build_typeinfo(tkey);
	args[2] = build_address(index);

	this->result_ = build_libcall(LIBCALL_AADELX, 3, args);
      }
    else
      {
	error("%s is not an associative array", e->e1->toChars());
	this->result_ = error_mark_node;
      }
  }

  //
  void visit(NotExp *e)
  {
    tree result = convert_for_condition(build_expr(e->e1), e->e1->type);
    // Need to convert to boolean type or this will fail.
    result = fold_build1(TRUTH_NOT_EXPR, bool_type_node, result);

    this->result_ = d_convert(build_ctype(e->type), result);
  }

  //
  void visit(ComExp *e)
  {
    TY ty1 = e->e1->type->toBasetype()->ty;
    gcc_assert(ty1 != Tarray && ty1 != Tsarray);

    this->result_ = fold_build1(BIT_NOT_EXPR, build_ctype(e->type),
				build_expr(e->e1));
  }

  //
  void visit(NegExp *e)
  {
    TY ty1 = e->e1->type->toBasetype()->ty;
    gcc_assert(ty1 != Tarray && ty1 != Tsarray);

    this->result_ = fold_build1(NEGATE_EXPR, build_ctype(e->type),
				build_expr(e->e1));
  }

  //
  void visit(PtrExp *e)
  {
    // Produce better code by converting * (#record + n) to
    // COMPONENT_REFERENCE.  Otherwise, the variable will always be
    // allocated in memory because its address is taken.
    Type *tnext = NULL;
    size_t offset;
    tree result;

    if (e->e1->op == TOKadd)
      {
	BinExp *be = (BinExp *) e->e1;
	if (be->e1->op == TOKaddress
	    && be->e2->isConst() && be->e2->type->isintegral())
	  {
	    Expression *ae = ((AddrExp *) be->e1)->e1;
	    tnext = ae->type->toBasetype();
	    result = build_expr(ae);
	    offset = be->e2->toUInteger();
	  }
      }
    else if (e->e1->op == TOKsymoff)
      {
	SymOffExp *se = (SymOffExp *) e->e1;
	if (!declaration_reference_p(se->var))
	  {
	    tnext = se->var->type->toBasetype();
	    result = get_decl_tree(se->var);
	    offset = se->offset;
	  }
      }

    if (tnext && tnext->ty == Tstruct)
      {
	StructDeclaration *sd = ((TypeStruct *) tnext)->sym;
	for (size_t i = 0; i < sd->fields.dim; i++)
	  {
	    VarDeclaration *field = sd->fields[i];
	    if (field->offset == offset
		&& d_types_same(field->type, e->type))
	      {
		// Catch errors, backend will ICE otherwise.
		if (error_operand_p(result))
		  this->result_ = result;
		else
		  this->result_ = component_ref(result, field->toSymbol()->Stree);

		return;
	      }
	    else if (field->offset > offset)
	      break;
	  }
      }

    this->result_ = indirect_ref(build_ctype(e->type), build_expr(e->e1));
  }

  //
  void visit(AddrExp *e)
  {
    tree type = build_ctype(e->type);
    tree exp;

    // Optimizer can convert const symbol into a struct literal.
    // Taking the address of a struct literal is otherwise illegal.
    if (e->e1->op == TOKstructliteral)
      {
	StructLiteralExp *sle = ((StructLiteralExp *) e->e1)->origin;
	gcc_assert(sle != NULL);
	exp = sle->toSymbol()->Stree;
      }
    else
      exp = build_expr(e->e1, this->constp_);

    TREE_CONSTANT (exp) = 0;
    this->result_ = d_convert(type, build_address(exp));
  }

  //
  void visit(CallExp *e)
  {
    Type *tb = e->e1->type->toBasetype();
    Expression *e1b = e->e1;

    tree callee = NULL_TREE;
    tree object = NULL_TREE;
    TypeFunction *tf = NULL;

    // Calls to delegates can sometimes look like this:
    if (e1b->op == TOKcomma)
      {
	e1b = ((CommaExp *) e1b)->e2;
	gcc_assert(e1b->op == TOKvar);

	Declaration *var = ((VarExp *) e1b)->var;
	gcc_assert (var->isFuncDeclaration() && !var->needThis());
      }

    if (e1b->op == TOKdotvar && tb->ty != Tdelegate)
      {
	DotVarExp *dve = (DotVarExp *) e1b;

	// Is this static method call?
	bool is_dottype = false;
	Expression *ex = dve->e1;

	while (1)
	  {
	    if (ex->op == TOKsuper || ex->op == TOKdottype)
	      {
		// super.member() and type.member() calls directly.
		is_dottype = true;
		break;
	      }
	    else if (ex->op == TOKcast)
	      {
		ex = ((CastExp *) ex)->e1;
		continue;
	      }
	    break;
	  }

	// Don't modify the static initializer for struct literals.
	if (dve->e1->op == TOKstructliteral)
	  {
	    StructLiteralExp *sle = (StructLiteralExp *) dve->e1;
	    sle->sinit = NULL;
	  }

	FuncDeclaration *fd = dve->var->isFuncDeclaration();
	if (fd != NULL)
	  {
	    // Get the correct callee from the DotVarExp object.
	    tree fndecl = fd->toSymbol()->Stree;

	    // Static method; ignore the object instance.
	    if (!fd->isThis())
	      callee = build_address(fndecl);
	    else
	      {
		tree thisexp = build_expr(dve->e1);

		// Want reference to 'this' object.
		if (dve->e1->type->ty != Tclass && dve->e1->type->ty != Tpointer)
		  thisexp = build_address(thisexp);

		// Make the callee a virtual call.
		if (fd->isVirtual() && !fd->isFinalFunc() && !is_dottype)
		  {
		    tree fntype = build_pointer_type(TREE_TYPE (fndecl));
		    fndecl = build_vindex_ref(thisexp, fntype, fd->vtblIndex);
		  }
		else
		  fndecl = build_address(fndecl);

		callee = build_method_call(fndecl, thisexp, fd->type);
	      }
	  }
      }

    if (callee == NULL_TREE)
      callee = build_expr(e1b);

    if (METHOD_CALL_EXPR (callee))
      {
	// This could be a delegate expression (TY == Tdelegate), but not
	// actually a delegate variable.
	if (e1b->op == TOKdotvar)
	  {
	    // This gets the true function type, getting the function type from
	    // e1->type can sometimes be incorrect, eg: ref return functions.
	    tf = get_function_type(((DotVarExp *) e1b)->var->type);
	  }
	else
	  tf = get_function_type(tb);

	extract_from_method_call(callee, callee, object);
      }
    else if (tb->ty == Tdelegate)
      {
	// Delegate call, extract .object and .funcptr from var.
	callee = maybe_make_temp(callee);
	tf = get_function_type(tb);
	object = delegate_object(callee);
	callee = delegate_method(callee);
      }
    else if (e1b->op == TOKvar)
      {
	FuncDeclaration *fd = ((VarExp *) e1b)->var->isFuncDeclaration();
	gcc_assert(fd != NULL);
	tf = get_function_type(fd->type);

	if (fd->isNested())
	  {
	    // Maybe re-evaluate symbol storage treating 'fd' as public.
	    if (call_by_alias_p(cfun->language->function, fd))
	      setup_symbol_storage(fd, callee, true);

	    object = get_frame_for_symbol(fd);
	  }
	else if (fd->needThis())
	  {
	    e1b->error("need 'this' to access member %s", fd->toChars());
	    // Continue processing...
	    object = null_pointer_node;
	  }
      }
    else
      {
	// Normal direct function call.
	tf = get_function_type(tb);
      }

    gcc_assert(tf != NULL);

    // Now we have the type, callee and maybe object reference,
    // build the call expression.
    tree exp = d_build_call(tf, callee, object, e->arguments);

    if (tf->isref)
      exp = build_deref(exp);

    // Some library calls are defined to return a generic type.
    // this->type is the real type we want to return.
    if (e->type->isTypeBasic())
      exp = d_convert(build_ctype(e->type), exp);

    this->result_ = exp;
  }

  //
  void visit(DotTypeExp *e)
  {
    // Just a pass through to e1.
    this->result_ = build_expr(e->e1);
  }

  //
  void visit(DelegateExp *e)
  {
    // %% The result will probably just be converted to a CONSTRUCTOR
    // for a Tdelegate struct.

    if (e->func->fbody)
      {
	// Add the function as nested function if it belongs to this module
	// ie, it is a member of this module, or it is a template instance.
	Dsymbol *owner = e->func->toParent();
	while (!owner->isTemplateInstance() && owner->toParent())
	  owner = owner->toParent();
	if (owner->isTemplateInstance() || owner == cfun->language->module)
	  cfun->language->deferred_fns.safe_push(e->func);
      }

    tree fndecl;
    tree object;

    if (e->func->isNested())
      {
	if (e->e1->op == TOKnull)
	  object = build_expr(e->e1);
	else
	  object = get_frame_for_symbol(e->func);

	fndecl = build_address(e->func->toSymbol()->Stree);
      }
    else
      {
	if (!e->func->isThis())
	  {
	    error("delegates are only for non-static functions");
	    this->result_ = error_mark_node;
	    return;
	  }

	object = build_expr(e->e1);

	// Want reference to 'this' object.
	if (e->e1->type->ty != Tclass && e->e1->type->ty != Tpointer)
	  object = build_address(object);

	fndecl = build_address(e->func->toSymbol()->Stree);

	// Get pointer to function out of the virtual table.
	if (e->func->isVirtual() && !e->func->isFinalFunc()
	    && e->e1->op != TOKsuper && e->e1->op != TOKdottype)
	  {
	    fndecl = build_vindex_ref(object, TREE_TYPE (fndecl),
				      e->func->vtblIndex);
	  }
      }

    this->result_ = build_method_call(fndecl, object, e->type);
  }

  //
  void visit(DotVarExp *e)
  {
    VarDeclaration *vd = e->var->isVarDeclaration();

    // Could also be a function, but relying on that being taken care of
    // by the code generator for CallExp.
    if (vd != NULL)
      {
	if (!vd->isField())
	  this->result_ = get_decl_tree(vd);
	else
	  {
	    tree object = build_expr(e->e1);

	    if (e->e1->type->toBasetype()->ty != Tstruct)
	      object = build_deref(object);

	    this->result_ = component_ref(object, vd->toSymbol()->Stree);
	  }
      }
    else
      {
	error("%s is not a field, but a %s",
	      e->var->toChars(), e->var->kind());
	this->result_ = error_mark_node;
      }
  }

  //
  void visit(AssertExp *e)
  {
    // Not compiling in assert contracts.
    if (!global.params.useAssert)
      {
	this->result_ = void_zero_node;
	return;
      }

    // Build _d_assert call.
    Type *tb1 = e->e1->type->toBasetype();
    tree tmsg = NULL_TREE;
    LibCall libcall;

    if (cfun->language->function->isUnitTestDeclaration())
      {
	if (e->msg)
	  {
	    tmsg = build_expr_dtor(e->msg);
	    libcall = LIBCALL_UNITTEST_MSG;
	  }
	else
	  libcall = LIBCALL_UNITTEST;
      }
    else
      {
	if (e->msg)
	  {
	    tmsg = build_expr_dtor(e->msg);
	    libcall = LIBCALL_ASSERT_MSG;
	  }
	else
	  libcall = LIBCALL_ASSERT;
      }

    tree assert_call = d_assert_call(e->loc, libcall, tmsg);

    // Build condition that we are asserting in this contract.
    if (tb1->ty == Tclass)
      {
	ClassDeclaration *cd = tb1->isClassHandle();
	tree arg = build_expr(e->e1);
	tree invc = NULL_TREE;

	if (cd->isCOMclass())
	  {
	    tree cond = build_boolop(NE_EXPR, arg, null_pointer_node);
	    this->result_ = build_vcondition(cond, void_zero_node, assert_call);
	    return;
	  }
	else if (cd->isInterfaceDeclaration())
	  arg = convert_expr(arg, tb1, build_object_type());

	if (global.params.useInvariants && !cd->isCPPclass())
	  {
	    arg = maybe_make_temp(arg);
	    invc = build_libcall(LIBCALL_INVARIANT, 1, &arg);
	  }

	// This does a null pointer check before calling _d_invariant
	tree cond = build_boolop(NE_EXPR, arg, null_pointer_node);
	this->result_ = build_vcondition(cond, invc ? invc : void_zero_node, assert_call);
      }
    else
      {
	// build: ((bool) e1  ? (void)0 : _d_assert (...))
	//    or: (e1 != null ? e1._invariant() : _d_assert (...))
	tree invc = NULL_TREE;
	tree t1 = build_expr(e->e1);

	if (global.params.useInvariants
	    && tb1->ty == Tpointer && tb1->nextOf()->ty == Tstruct)
	  {
	    FuncDeclaration *inv = ((TypeStruct *) tb1->nextOf())->sym->inv;
	    if (inv != NULL)
	      {
		Expressions args;
		t1 = maybe_make_temp(t1);
		invc = d_build_call(inv, t1, &args);
	      }
	  }

	tree cond = convert_for_condition(t1, e->e1->type);
	this->result_ = build_vcondition(cond, invc ? invc : void_zero_node, assert_call);
      }
  }

  //
  void visit(DeclarationExp *e)
  {
    // Compile the declaration.
    push_stmt_list();
    e->declaration->toObjFile();
    tree result = pop_stmt_list();

    // Construction of an array for typesafe-variadic function arguments
    // can cause an empty STMT_LIST here.
    // This can causes problems during gimplification.
    if (TREE_CODE (result) == STATEMENT_LIST && !STATEMENT_LIST_HEAD (result))
      this->result_ = build_empty_stmt(input_location);
    else
      this->result_ = result;

    // Maybe put variable on list of things needing destruction.
    VarDeclaration *vd = e->declaration->isVarDeclaration();
    if (vd != NULL)
      {
	if (!vd->isStatic() && !(vd->storage_class & STCmanifest)
	    && !(vd->storage_class & (STCextern | STCtls | STCgshared)))
	  {
	    if (vd->edtor && !vd->noscope)
	      cfun->language->vars_in_scope.safe_push(vd);
	  }
      }

  }

  //
  void visit(FuncExp *e)
  {
    Type *ftype = e->type->toBasetype();

    // This check is for lambda's, remove 'vthis' as function isn't nested.
    if (e->fd->tok == TOKreserved && ftype->ty == Tpointer)
      {
	e->fd->tok = TOKfunction;
	e->fd->vthis = NULL;
      }

    // Emit after current function body has finished.
    if (cfun != NULL)
      cfun->language->deferred_fns.safe_push(e->fd);
    else
      e->fd->toObjFile();

    // If nested, this will be a trampoline...
    if (e->fd->isNested())
      {
	if (this->constp_)
	  {
	    e->error("non-constant nested delegate literal expression %s",
		     e->toChars());
	    this->result_ = error_mark_node;
	  }
	else
	  {
	    tree func = build_address(e->fd->toSymbol()->Stree);
	    tree object = get_frame_for_symbol(e->fd);
	    this->result_ = build_method_call(func, object, e->fd->type);
	  }
      }
    else
      {
	this->result_ = build_nop(build_ctype(e->type),
				  build_address(e->fd->toSymbol()->Stree));
      }
  }

  //
  void visit(HaltExp *)
  {
    this->result_ = d_build_call_nary(builtin_decl_explicit(BUILT_IN_TRAP), 0);
  }

  //
  void visit(SymOffExp *e)
  {
    // Build the address and offset of the symbol.
    size_t soffset = ((SymOffExp *) e)->offset;
    tree result = get_decl_tree(e->var);
    TREE_USED (result) = 1;

    if (declaration_reference_p(e->var))
      gcc_assert(POINTER_TYPE_P (TREE_TYPE (result)));
    else
      result = build_address(result);

    if (!soffset)
      result = d_convert(build_ctype(e->type), result);
    else
      {
	tree offset = size_int(soffset);
	result = build_nop(build_ctype(e->type),
			   build_offset(result, offset));
      }

    this->result_ = result;
  }

  //
  void visit(VarExp *e)
  {
    if (e->var->needThis())
      {
	error("need 'this' to access member %s", e->var->ident->string);
	this->result_ = error_mark_node;
      }
    else if (e->var->ident == Id::ctfe)
      {
	// __ctfe is always false at runtime
	this->result_ = integer_zero_node;
      }
    else if (this->constp_)
      {
	// Want the initializer, not the expression.
	VarDeclaration *var = e->var->isVarDeclaration();
	SymbolDeclaration *sd = e->var->isSymbolDeclaration();
	tree init = NULL_TREE;

	if (var && (var->isConst() || var->isImmutable())
	    && e->type->toBasetype()->ty != Tsarray && var->init)
	  {
	    if (var->inuse)
	      e->error("recursive reference %s", e->toChars());
	    else
	      {
		var->inuse++;
		init = var->init->toDt();
		var->inuse--;
	      }
	  }
	else if (sd && sd->dsym)
	  sd->dsym->toDt(&init);
	else
	  e->error("non-constant expression %s", e->toChars());

	if (init != NULL_TREE)
	  this->result_ = dtvector_to_tree(init);
	else
	  this->result_ = error_mark_node;
      }
    else
      {
	tree result = get_decl_tree(e->var);
	TREE_USED (result) = 1;

	// For variables that are references - currently only out/inout
	// arguments; objects don't count - evaluating the variable means
	// we want what it refers to.
	if (declaration_reference_p(e->var))
	  result = indirect_ref(build_ctype(e->var->type), result);

	// The frontend sometimes emits different types for the expression
	// and var declaration.  So we must force convert to the expressions
	// type, but don't convert FuncDeclaration as underlying ctype
	// sometimes isn't the correct type for functions!
	if (!e->var->isFuncDeclaration()
	    && !d_types_same(e->var->type, e->type))
	  result = build1(VIEW_CONVERT_EXPR, build_ctype(e->type), result);

	this->result_ = result;
      }
  }

  //
  void visit(NewExp *e)
  {
    Type *tb = e->type->toBasetype();
    tree result;

    if (e->allocator)
      gcc_assert(e->newargs);

    // New'ing a class.
    if (tb->ty == Tclass)
      {
	tb = e->newtype->toBasetype();
	gcc_assert(tb->ty == Tclass);
	TypeClass *tclass = (TypeClass *) tb;
	ClassDeclaration *cd = tclass->sym;

	tree new_call;
	tree setup_exp = NULL_TREE;
	// type->ctype is a POINTER_TYPE; we want the RECORD_TYPE
	tree rec_type = TREE_TYPE(build_ctype(tclass));

	// Call allocator (custom allocator or _d_newclass).
	if (e->onstack)
	  {
	    tree stack_var = build_local_temp(rec_type);
	    expand_decl(stack_var);
	    new_call = build_address(stack_var);
	    setup_exp = modify_expr(stack_var, cd->toInitializer()->Stree);
	  }
	else if (e->allocator)
	  {
	    new_call = d_build_call(e->allocator, NULL_TREE, e->newargs);
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
	    tree field = cd->vthis->toSymbol()->Stree;
	    tree value = NULL_TREE;

	    if (e->thisexp)
	      {
		ClassDeclaration *tcd = e->thisexp->type->isClassHandle();
		Dsymbol *outer = cd->toParent2();
		int offset = 0;

		value = build_expr(e->thisexp);
		if (outer != tcd)
		  {
		    ClassDeclaration *ocd = outer->isClassDeclaration();
		    gcc_assert(ocd->isBaseOf(tcd, &offset));
		    // could just add offset
		    value = convert_expr(value, e->thisexp->type, ocd->type);
		  }
	      }
	    else
	      value = build_vthis(cd);

	    if (value != NULL_TREE)
	      {
		new_call = maybe_make_temp(new_call);
		field = component_ref(indirect_ref(rec_type, new_call), field);
		setup_exp = maybe_compound_expr(setup_exp,
						modify_expr(field, value));
	      }
	  }
	new_call = maybe_compound_expr(setup_exp, new_call);

	// Call constructor.
	if (e->member)
	  result = d_build_call(e->member, new_call, e->arguments);
	else
	  result = new_call;

	if (e->argprefix)
	  result = compound_expr(build_expr(e->argprefix), result);
      }
    // New'ing a struct.
    else if (tb->ty == Tpointer && tb->nextOf()->toBasetype()->ty == Tstruct)
      {
	Type *htype = e->newtype->toBasetype();
	gcc_assert(htype->ty == Tstruct);
	gcc_assert(!e->onstack);

	TypeStruct *stype = (TypeStruct *) htype;
	StructDeclaration *sd = stype->sym;
	tree new_call;

	// Cannot new an opaque struct.
	if (sd->size(e->loc) == 0)
	  {
	    this->result_ = d_convert(build_ctype(e->type), integer_zero_node);
	    return;
	  }

	if (e->allocator)
	  new_call = d_build_call(e->allocator, NULL_TREE, e->newargs);
	else
	  {
	    LibCall libcall = htype->isZeroInit()
	      ? LIBCALL_NEWITEMT : LIBCALL_NEWITEMIT;
	    tree arg = build_expr(getTypeInfo(e->newtype, NULL));
	    new_call = build_libcall(libcall, 1, &arg);
	  }
	new_call = maybe_make_temp(new_call);
	new_call = build_nop(build_ctype(tb), new_call);

	if (e->member || !e->arguments)
	  {
	    // Set vthis for nested structs.
	    if (sd->isNested())
	      {
		tree value = build_vthis(sd);
		tree field = component_ref(indirect_ref(build_ctype(stype), new_call),
						 sd->vthis->toSymbol()->Stree);
		new_call = compound_expr(modify_expr(field, value), new_call);
	      }

	    // Call constructor.
	    if (e->member)
	      result = d_build_call(e->member, new_call, e->arguments);
	    else
	      result = new_call;
	  }
	else
	  {
	    // User supplied initialiser, set-up with a struct literal.
	    StructLiteralExp *se = StructLiteralExp::create(e->loc, sd,
							    e->arguments, htype);
	    se->sym = new Symbol();
	    se->sym->Stree = new_call;
	    se->type = sd->type;

	    result = compound_expr(build_expr(se), new_call);
	  }

	if (e->argprefix)
	  result = compound_expr(build_expr(e->argprefix), result);
      }
    // New'ing a D array.
    else if (tb->ty == Tarray)
      {
	tb = e->newtype->toBasetype();
	gcc_assert (tb->ty == Tarray);
	TypeDArray *tarray = (TypeDArray *) tb;
	gcc_assert(!e->allocator);
	gcc_assert(e->arguments && e->arguments->dim >= 1);

	if (e->arguments->dim == 1)
	  {
	    // Single dimension array allocations.
	    Expression *arg = (*e->arguments)[0];
	    tree args[2];

	    // Elem size is unknown.
	    if (tarray->next->size() == 0)
	      {
		this->result_ = d_array_value(build_ctype(e->type), size_int(0),
					      null_pointer_node);
		return;
	      }

	    LibCall libcall = tarray->next->isZeroInit()
	      ? LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
	    args[0] = build_expr(getTypeInfo(e->type, NULL));
	    args[1] = build_expr(arg);
	    result = build_libcall(libcall, 2, args, build_ctype(tb));
	  }
	else
	  {
	    // Multidimensional array allocations.
	    vec<constructor_elt, va_gc> *elms = NULL;
	    Type *telem = e->newtype->toBasetype();
	    tree var = create_temporary_var(d_array_type(Type::tsize_t,
							 e->arguments->dim));
	    tree init = build_constructor(TREE_TYPE (var), NULL);
	    tree args[2];

	    for (size_t i = 0; i < e->arguments->dim; i++)
	      {
		Expression *arg = (*e->arguments)[i];
		CONSTRUCTOR_APPEND_ELT (elms, size_int(i), build_expr(arg));

		gcc_assert(telem->ty == Tarray);
		telem = telem->toBasetype()->nextOf();
		gcc_assert(telem);
	      }

	    CONSTRUCTOR_ELTS (init) = elms;
	    DECL_INITIAL (var) = init;

	    LibCall libcall = telem->isZeroInit()
	      ? LIBCALL_NEWARRAYMTX : LIBCALL_NEWARRAYMITX;
	    args[0] = build_expr(getTypeInfo(e->type, NULL));
	    args[1] = d_array_value(build_ctype(Type::tsize_t->arrayOf()),
				    size_int(e->arguments->dim),
				    build_address(var));
	    result = build_libcall(libcall, 2, args, build_ctype(tb));
	    result = bind_expr(var, result);
	  }

	if (e->argprefix)
	  result = compound_expr(build_expr(e->argprefix), result);
      }
    // New'ing a pointer
    else if (tb->ty == Tpointer)
      {
	TypePointer *tpointer = (TypePointer *) tb;

	// Elem size is unknown.
	if (tpointer->next->size() == 0)
	  {
	    this->result_ = d_convert(build_ctype(e->type), integer_zero_node);
	    return;
	  }

	LibCall libcall = tpointer->next->isZeroInit(e->loc)
	  ? LIBCALL_NEWITEMT : LIBCALL_NEWITEMIT;

	tree arg = build_expr(getTypeInfo(e->newtype, NULL));
	result = build_libcall(libcall, 1, &arg, build_ctype(tb));

	if (e->arguments && e->arguments->dim == 1)
	  {
	    result = make_temp(result);
	    tree init = modify_expr(build_deref(result),
				    build_expr((*e->arguments)[0]));
	    result = compound_expr(init, result);
	  }

	if (e->argprefix)
	  result = compound_expr(build_expr(e->argprefix), result);
      }
    else
      gcc_unreachable();

    this->result_ = convert_expr(result, tb, e->type);
  }

  //
  void visit(ScopeExp *e)
  {
    e->error("%s is not an expression", e->toChars());
    this->result_ = error_mark_node;
  }

  //
  void visit(TypeExp *e)
  {
    e->error("type %s is not an expression", e->toChars());
    this->result_ = error_mark_node;
  }

  // Create an integer literal with the given expression.
  void visit(IntegerExp *e)
  {
    tree ctype = build_ctype(e->type->toBasetype());
    this->result_ = build_integer_cst(e->value, ctype);
  }

  //
  void visit(RealExp *e)
  {
    this->result_ = build_float_cst(e->value, e->type->toBasetype());
  }

  //
  void visit(ComplexExp *e)
  {
    Type *tnext;

    switch (e->type->toBasetype()->ty)
      {
      case Tcomplex32:
	tnext = (TypeBasic *) Type::tfloat32;
	break;

      case Tcomplex64:
	tnext = (TypeBasic *) Type::tfloat64;
	break;

      case Tcomplex80:
	tnext = (TypeBasic *) Type::tfloat80;
	break;

      default:
	gcc_unreachable();
      }

    this->result_ = build_complex(build_ctype(e->type),
				  build_float_cst(creall(e->value), tnext),
				  build_float_cst(cimagl(e->value), tnext));
  }

  // Create a string literal with the given expression.
  void visit(StringExp *e)
  {
    Type *tb = e->type->toBasetype();

    // All strings are null terminated except static arrays.
    dinteger_t length = (e->len * e->sz) + (tb->ty != Tsarray);
    tree value = build_string(length, (char *) e->string);
    tree type = build_ctype(e->type);

    if (tb->ty == Tsarray)
      TREE_TYPE (value) = type;
    else
      {
	// Array type doesn't match string length if null terminated.
	TREE_TYPE (value) = d_array_type(tb->nextOf(), e->len);

	value = build_address(value);
	if (tb->ty == Tarray)
	  value = d_array_value(type, size_int(e->len), value);
      }

    TREE_CONSTANT (value) = 1;
    this->result_ = d_convert(type, value);
  }

  //
  void visit(TupleExp *e)
  {
    tree result = NULL_TREE;

    if (e->e0)
      result = build_expr(e->e0);

    for (size_t i = 0; i < e->exps->dim; ++i)
      {
	Expression *exp = (*e->exps)[i];
	result = maybe_vcompound_expr(result, build_expr(exp));
      }

    if (result == NULL_TREE)
      result = void_zero_node;

    this->result_ = result;
  }

  //
  void visit(ArrayLiteralExp *e)
  {
    Type *tb = e->type->toBasetype();

    // Convert void[n] to ubyte[n]
    if (tb->ty == Tsarray && tb->nextOf()->toBasetype()->ty == Tvoid)
      tb = Type::tuns8->sarrayOf(((TypeSArray *) tb)->dim->toUInteger());

    gcc_assert(tb->ty == Tarray || tb->ty == Tsarray || tb->ty == Tpointer);

    // Handle empty array literals.
    if (e->elements->dim == 0)
      {
	if (tb->ty == Tarray)
	  this->result_ = d_array_value(build_ctype(e->type),
					size_int(0), null_pointer_node);
	else
	  this->result_ = build_constructor(d_array_type(tb->nextOf(), 0), NULL);

	return;
      }

    // Build an expression that assigns the expressions in ELEMENTS to a constructor.
    vec<constructor_elt, va_gc> *elms = NULL;
    vec_safe_reserve(elms, e->elements->dim);
    bool constant_p = true;

    Type *etype = tb->nextOf();
    tree satype = d_array_type(etype, e->elements->dim);

    for (size_t i = 0; i < e->elements->dim; i++)
      {
	Expression *expr = (*e->elements)[i];
	tree value = build_expr(expr, this->constp_);

	// Only care about non-zero values, the backend will fill in the rest.
	if (!initializer_zerop(value))
	  {
	    if (!TREE_CONSTANT (value))
	      {
		value = maybe_make_temp(value);
		constant_p = false;
	      }

	    CONSTRUCTOR_APPEND_ELT (elms, size_int(i),
				    convert_expr(value, expr->type, etype));
	  }
      }

    // Now return the constructor as the correct type.  For static arrays there
    // is nothing else to do.  For dynamic arrays, return a two field struct.
    // For pointers, return the address.
    tree ctor = build_constructor(satype, elms);
    tree type = build_ctype(e->type);

    // Nothing else to do for static arrays.
    if (tb->ty == Tsarray || this->constp_)
      {
	// Can't take the address of the constructor, so create an anonymous
	// static symbol, and then refer to it.
	if (tb->ty != Tsarray)
	  {
	    ctor = build_artificial_decl(TREE_TYPE (ctor), ctor, "A");
	    ctor = build_address(ctor);
	    if (tb->ty == Tarray)
	      ctor = d_array_value(type, size_int(e->elements->dim), ctor);
	  }

	// If the array literal is readonly or static.
	if (constant_p)
	  TREE_CONSTANT (ctor) = 1;
	if (constant_p && initializer_constant_valid_p(ctor, TREE_TYPE (ctor)))
	  TREE_STATIC (ctor) = 1;

	this->result_ = d_convert(type, ctor);
      }
    else
      {
	tree args[2];

	args[0] = build_typeinfo(etype->arrayOf());
	args[1] = size_int(e->elements->dim);

	// Call _d_arrayliteralTX (ti, dim);
	tree mem = build_libcall(LIBCALL_ARRAYLITERALTX, 2, args,
				 build_ctype(etype->pointerTo()));
	mem = maybe_make_temp(mem);

	// memcpy (mem, &ctor, size)
	tree size = size_mult_expr(size_int(e->elements->dim),
				   size_int(tb->nextOf()->size()));

	tree result = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMCPY), 3,
					mem, build_address(ctor), size);

	// Returns array pointed to by MEM.
	result = maybe_compound_expr(result, mem);

	if (tb->ty == Tarray)
	  result = d_array_value(type, size_int(e->elements->dim), result);

	this->result_ = result;
      }
  }

  //
  void visit(AssocArrayLiteralExp *e)
  {
    // Want mutable type for typeinfo reference.
    Type *tb = e->type->toBasetype()->mutableOf();
    gcc_assert(tb->ty == Taarray);

    // Handle empty assoc array literals.
    TypeAArray *ta = (TypeAArray *) tb;
    if (e->keys->dim == 0)
      {
	this->result_ = build_constructor(build_ctype(ta), NULL);
	return;
      }

    // Build an expression that assigns the expressions in KEYS and VALUES
    // to a constructor.
    vec<constructor_elt, va_gc> *ke = NULL;
    vec_safe_reserve(ke, e->keys->dim);
    for (size_t i = 0; i < e->keys->dim; i++)
      {
	Expression *key = (*e->keys)[i];
	tree t = build_expr(key);
	t = maybe_make_temp(t);
	CONSTRUCTOR_APPEND_ELT(ke, size_int(i),
			       convert_expr(t, key->type, ta->index));
      }
    tree akeys = build_constructor(d_array_type(ta->index, e->keys->dim), ke);

    vec<constructor_elt, va_gc> *ve = NULL;
    vec_safe_reserve(ve, e->values->dim);
    for (size_t i = 0; i < e->values->dim; i++)
      {
	Expression *value = (*e->values)[i];
	tree t = build_expr(value);
	t = maybe_make_temp(t);
	CONSTRUCTOR_APPEND_ELT(ve, size_int(i),
			       convert_expr(t, value->type, ta->next));
      }
    tree avals = build_constructor(d_array_type(ta->next, e->values->dim), ve);

    // Call _d_assocarrayliteralTX (ti, keys, vals);
    tree args[3];
    args[0] = build_typeinfo(ta);
    args[1] = d_array_value(build_ctype(ta->index->arrayOf()),
			    size_int(e->keys->dim), build_address(akeys));
    args[2] = d_array_value(build_ctype(ta->next->arrayOf()),
			    size_int(e->values->dim), build_address(avals));

    tree mem = build_libcall(LIBCALL_ASSOCARRAYLITERALTX, 3, args);

    // Returns an AA pointed to by MEM.
    tree aatype = build_ctype(ta);
    vec<constructor_elt, va_gc> *ce = NULL;
    CONSTRUCTOR_APPEND_ELT (ce, TYPE_FIELDS (aatype), mem);

    this->result_ = build_nop(build_ctype(e->type),
			      build_constructor(aatype, ce));
  }

  //
  void visit(StructLiteralExp *e)
  {
    // Handle empty struct literals.
    if (e->elements == NULL || e->sd->fields.dim == 0)
      {
	this->result_ = build_constructor(build_ctype(e->type), NULL);
	return;
      }

    // Building sinit trees are delayed until after frontend semantic
    // processing has complete.  Build the static initialiser now.
    if (e->sinit && !this->constp_)
      {
	if (e->sinit->Stree == NULL_TREE)
	  e->sd->toInitializer();

	gcc_assert(e->sinit->Stree != NULL);
	this->result_ = e->sinit->Stree;
	return;
      }

    // Build a constructor that assigns the expressions in ELEMENTS
    // at each field index that has been filled in.
    vec<constructor_elt, va_gc> *ve = NULL;

    // CTFE may fill the hidden pointer by NullExp.
    gcc_assert(e->elements->dim <= e->sd->fields.dim);

    Type *tb = e->type->toBasetype();
    gcc_assert(tb->ty == Tstruct);

    for (size_t i = 0; i < e->elements->dim; i++)
      {
	if (!(*e->elements)[i])
	  continue;

	Expression *exp = (*e->elements)[i];
	Type *type = exp->type->toBasetype();
	tree value = NULL_TREE;

	VarDeclaration *field = e->sd->fields[i];
	Type *ftype = field->type->toBasetype();

	if (ftype->ty == Tsarray && !d_types_same(type, ftype))
	  {
	    // Initialize a static array with a single element.
	    tree elem = build_expr(exp, this->constp_);
	    elem = maybe_make_temp(elem);

	    if (initializer_zerop(elem))
	      value = build_constructor(build_ctype(ftype), NULL);
	    else
	      value = build_array_from_val(ftype, elem);
	  }
	else
	  {
	    value = convert_expr(build_expr(exp, this->constp_),
				 exp->type, field->type);
	  }

	CONSTRUCTOR_APPEND_ELT (ve, field->toSymbol()->Stree, value);
      }

    // Maybe setup hidden pointer to outer scope context.
    if (e->sd->isNested() && e->elements->dim != e->sd->fields.dim
	&& this->constp_ == false)
      {
	tree field = e->sd->vthis->toSymbol()->Stree;
	tree value = build_vthis(e->sd);
	CONSTRUCTOR_APPEND_ELT (ve, field, value);
	gcc_assert(e->sinit == NULL);
      }

    // Build a constructor in the correct shape of the aggregate type.
    tree ctor = build_struct_literal(build_ctype(e->type),
				     build_constructor(unknown_type_node, ve));

    // Nothing more to do for constant literals.
    if (this->constp_)
      {
	// If the struct literal is a valid for static data.
	if (TREE_CONSTANT (ctor)
	    && initializer_constant_valid_p(ctor, TREE_TYPE (ctor)))
	  TREE_STATIC (ctor) = 1;

	this->result_ = ctor;
	return;
      }

    if (e->sym != NULL)
      {
	tree var = build_deref(e->sym->Stree);
	this->result_ = compound_expr(modify_expr(var, ctor), var);
      }
    else if (e->sd->isUnionDeclaration())
      {
	// Initialize all alignment 'holes' to zero.
	tree var = build_local_temp(TREE_TYPE (ctor));
	tree init = d_build_call_nary(builtin_decl_explicit(BUILT_IN_MEMSET), 3,
				      build_address(var), size_zero_node,
				      size_int(e->sd->structsize));

	init = compound_expr(init, modify_expr(var, ctor));
	this->result_  = compound_expr(init, var);
      }
    else
      this->result_ = ctor;
  }

  // Create a 'null' literal with the given expression.
  void visit(NullExp *e)
  {
    Type *tb = e->type->toBasetype();
    tree value;

    // Handle certain special case conversions, where the underlying type is an
    // aggregate with a nullable interior pointer.
    // This is the case for dynamic arrays, associative arrays, and delegates.
    if (tb->ty == Tarray)
      {
	tree type = build_ctype(e->type);
	value = d_array_value(type, size_int(0), null_pointer_node);
      }
    else if (tb->ty == Taarray)
      value = build_constructor(build_ctype(e->type), NULL);
    else if (tb->ty == Tdelegate)
      value = build_delegate_cst(null_pointer_node, null_pointer_node, e->type);
    else
      value = d_convert(build_ctype(e->type), integer_zero_node);

    TREE_CONSTANT (value) = 1;
    this->result_ = value;
  }

  //
  void visit(ThisExp *e)
  {
    FuncDeclaration *fd = cfun ? cfun->language->function : NULL;
    tree result = NULL_TREE;

    if (e->var)
      {
	gcc_assert(e->var->isVarDeclaration());
	result = get_decl_tree(e->var);
      }
    else
      {
	gcc_assert(fd && fd->vthis);
	result = get_decl_tree(fd->vthis);
      }

    if (e->type->ty == Tstruct)
      result = build_deref(result);

    this->result_ = result;
  }

  //
  void visit(VectorExp *e)
  {
    tree type = build_ctype(e->type);
    tree etype = TREE_TYPE (type);

    // First handle array literal expressions.
    if (e->e1->op == TOKarrayliteral)
      {
	Expressions *elements = ((ArrayLiteralExp *) e->e1)->elements;
	vec<constructor_elt, va_gc> *elms = NULL;
	bool constant_p = true;

	vec_safe_reserve(elms, elements->dim);
	for (size_t i = 0; i < elements->dim; i++)
	  {
	    Expression *expr = (*elements)[i];
	    tree value = d_convert(etype, build_expr(expr, this->constp_));
	    if (!CONSTANT_CLASS_P (value))
	      constant_p = false;

	    CONSTRUCTOR_APPEND_ELT (elms, size_int(i), value);
	  }

	// Build a VECTOR_CST from a constant vector constructor.
	if (constant_p)
	  this->result_ = build_vector_from_ctor(type, elms);
	else
	  this->result_ = build_constructor(type, elms);
      }
    else
      {
	// Build constructor from single value.
	tree val = d_convert(etype, build_expr(e->e1, this->constp_));
	this->result_ = build_vector_from_val(type, val);
      }
  }

  //
  void visit(ClassReferenceExp *e)
  {
    // ClassReferenceExp builds the RECORD_TYPE, we want the reference.
    tree var = build_address(e->toSymbol()->Stree);

    // If the typeof this literal is an interface, we must add offset to symbol.
    if (this->constp_)
      {
	InterfaceDeclaration *to = ((TypeClass *) e->type)->sym->isInterfaceDeclaration();
	if (to != NULL)
	  {
	    ClassDeclaration *from = e->originalClass();
	    int offset = 0;

	    gcc_assert(to->isBaseOf(from, &offset) != 0);

	    if (offset != 0)
	      var = build_offset(var, size_int(offset));
	  }
      }

    this->result_ = var;
  }

  // Return expression tree for WrappedExp.
  void visit(WrappedExp *e)
  {
    this->result_ = e->e1;
  }
};


// Main entry point for ExprVisitor interface to generate
// code for the Expression AST class E.
// If CONST_P is true, then E is a constant expression.

tree
build_expr(Expression *e, bool const_p)
{
  ExprVisitor v = ExprVisitor(const_p);
  e->accept(&v);
  tree expr = v.result();

  // Check if initializer expression is valid constant.
  if (const_p && !initializer_constant_valid_p(expr, TREE_TYPE (expr)))
    {
      e->error("non-constant expression %s", e->toChars());
      return error_mark_node;
    }

  return expr;
}

// Same as build_expr, but also calls destructors on any temporaries.

tree
build_expr_dtor(Expression *e)
{
  size_t starti = cfun->language->vars_in_scope.length();
  tree exp = build_expr(e);
  size_t endi = cfun->language->vars_in_scope.length();

  // Codegen can be improved by determining if no exceptions can be thrown
  // between the ctor and dtor, and eliminating the ctor and dtor.

  // Build an expression that calls the destructors on all the variables
  // going out of the scope between starti and endi.
  // All dtors are executed in reverse order.
  tree tdtors = NULL_TREE;
  for (size_t i = starti; i != endi; ++i)
    {
      VarDeclaration *vd = cfun->language->vars_in_scope[i];
      if (vd)
	{
	  cfun->language->vars_in_scope[i] = NULL;
	  tree td = build_expr(vd->edtor);
	  tdtors = maybe_compound_expr(td, tdtors);
	}
    }

  if (tdtors != NULL_TREE)
    {
      TOK rtoken = (e->op != TOKcomma) ? e->op : ((CommaExp *) e)->e2->op;

      // For construction of temporaries, if the constructor throws, then
      // we don't want to run the destructor on incomplete object.
      CallExp *ce = (e->op == TOKcall) ? ((CallExp *) e) : NULL;
      bool catchCtor = true;
      if (ce != NULL && ce->e1->op == TOKdotvar)
	{
	  DotVarExp *dve = (DotVarExp *) ce->e1;
	  if (dve->e1->op == TOKcomma && dve->var->isCtorDeclaration()
	      && ((CommaExp *) dve->e1)->e1->op == TOKdeclaration
	      && ((CommaExp *) dve->e1)->e2->op == TOKvar)
	    catchCtor = false;
	}

      // Wrap function/ctor and dtors in a try/finally expression.
      if (catchCtor && (rtoken == TOKcall || rtoken == TOKnew))
	{
	  if (e->type->ty == Tvoid)
	    return build2(TRY_FINALLY_EXPR, void_type_node, exp, tdtors);
	  else
	    {
	      tree result = maybe_make_temp(exp);
	      exp = build2(TRY_FINALLY_EXPR, void_type_node, result, tdtors);
	      return compound_expr(exp, result);
	    }
	}

      // Split comma expressions, so as don't require a save_expr.
      if (e->op == TOKcomma && rtoken == TOKvar)
	{
	  tree lexp = TREE_OPERAND (exp, 0);
	  tree rvalue = TREE_OPERAND (exp, 1);
	  return compound_expr(compound_expr(lexp, tdtors), rvalue);
	}

      exp = maybe_make_temp(exp);
      return compound_expr(compound_expr(exp, tdtors), exp);
    }

  return exp;
}

