/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, Vincenzo Ampolo, Iain Buclaw, (C) 2010

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

// from DMD
#include "total.h"
#include "init.h"
#include "symbol.h"
#include "d-lang.h"
#include "d-codegen.h"

// see d-convert.cc
tree
convert (tree type, tree expr)
{
    Type * target_type = g.irs->getDType(type);
    Type * expr_type = g.irs->getDType(TREE_TYPE(expr));
    if (target_type && expr_type)
	return g.irs->convertTo(expr, expr_type, target_type);
    else
	return d_convert_basic(type, expr);
}

elem *
CondExp::toElem(IRState * irs)
{
    tree cn = irs->convertForCondition( econd );
    tree t1 = e1->toElem( irs );
    tree t2 = e2->toElem( irs );
    return build3(COND_EXPR, type->toCtype(), cn, t1, t2);
}

static void
signed_compare_check(tree * e1, tree * e2)
{
    tree t1 = TREE_TYPE( *e1 );
    tree t2 = TREE_TYPE( *e2 );
    if (INTEGRAL_TYPE_P( t1 ) &&
	INTEGRAL_TYPE_P( t2 )) {
	int u1 = TYPE_UNSIGNED( t1 );
	int u2 = TYPE_UNSIGNED( t2 );

	if (u1 ^ u2) {
	    if (gen.warnSignCompare) {
		d_warning (0, "unsigned comparison with signed operand");
	    }
	    if (! u1)
		* e1 = convert( d_unsigned_type( t1 ), * e1 );
	    if (! u2)
		* e2 = convert( d_unsigned_type( t2 ), * e2 );
	}
    }
}

static tree
make_bool_binop(TOK op, tree e1, tree e2, IRState * irs)
{
    bool is_compare = false; // %% should this be true for unordered comparisons?
    tree_code out_code;

    switch (op) {
    case TOKidentity: // fall through
    case TOKequal:
	is_compare = true;
	out_code = EQ_EXPR;
	break;
    case TOKnotidentity: // fall through
    case TOKnotequal:
	is_compare = true;
	out_code = NE_EXPR;
	break;
    case TOKandand:
	out_code = TRUTH_ANDIF_EXPR;
	break;
    case TOKoror:
	out_code = TRUTH_ORIF_EXPR;
	break;
    default:
	/*
	    // ordering for complex isn't defined, all that is guaranteed is the 'unordered part'
	    case TOKule:
	    case TOKul:
	    case TOKuge:
	    case TOKug:
		*/
	if ( COMPLEX_FLOAT_TYPE_P( TREE_TYPE( e1 )) ) {
	    // GCC doesn't handle these.
	    e1 = irs->maybeMakeTemp(e1);
	    e2 = irs->maybeMakeTemp(e2);
	    switch (op) {
	    case TOKleg:
		return irs->boolOp(TRUTH_ANDIF_EXPR,
		    make_bool_binop(TOKleg, irs->realPart(e1), irs->realPart(e2), irs),
		    make_bool_binop(TOKleg, irs->imagPart(e1), irs->imagPart(e2), irs));
	    case TOKunord:
		return irs->boolOp(TRUTH_ORIF_EXPR,
		    make_bool_binop(TOKunord, irs->realPart(e1), irs->realPart(e2), irs),
		    make_bool_binop(TOKunord, irs->imagPart(e1), irs->imagPart(e2), irs));
	    case TOKlg:
		return irs->boolOp(TRUTH_ANDIF_EXPR,
		    make_bool_binop(TOKleg, e1, e2, irs),
		    make_bool_binop(TOKnotequal, e1, e2, irs));
	    case TOKue:
		return irs->boolOp(TRUTH_ORIF_EXPR,
		    make_bool_binop(TOKunord, e1, e2, irs),
		    make_bool_binop(TOKequal, e1, e2, irs));
	    default:
		{
		    // From cmath2.d: if imaginary parts are equal,
		    // result is comparison of real parts; otherwise, result false
		    //
		    // Does D define an ordering for complex numbers?

		    // make a target-independent _cmplxCmp ?
		    tree it, rt;
		    TOK hard, soft;
		    bool unordered = false;
		    switch (op) {
		    case TOKule:
		    case TOKul:
		    case TOKuge:
		    case TOKug:
			unordered = true;
		    default:
			break;
		    }

		    switch (op) {
		    case TOKule:
		    case TOKle:
			hard = TOKlt;
			soft = TOKle;
			break;
		    case TOKul:
		    case TOKlt:
			hard = soft = TOKlt;
			break;
		    case TOKuge:
		    case TOKge:
			hard = TOKlt;
			soft = TOKle;
			break;
		    case TOKug:
		    case TOKgt:
			hard = soft = TOKgt;
			break;
		    default:
			assert(0);
		    }

		    it = make_bool_binop(hard, irs->imagPart(e2), irs->imagPart(e1), irs);
		    if (! unordered)
			it = irs->boolOp(TRUTH_ANDIF_EXPR,
			    make_bool_binop(TOKleg, irs->realPart(e2), irs->realPart(e1), irs),
			    it);
		    rt = irs->boolOp(TRUTH_ANDIF_EXPR,
			make_bool_binop(TOKequal, irs->imagPart(e2), irs->imagPart(e1), irs),
			make_bool_binop(soft, irs->realPart(e2), irs->realPart(e1), irs));
		    it = irs->boolOp(TRUTH_ANDIF_EXPR, it, rt);
		    if (unordered)
			it = irs->boolOp(TRUTH_ORIF_EXPR,
			    make_bool_binop(TOKunord, e1, e2, irs),
			    it);
		    return it;
		}
	    }
	}
	// else, normal

	switch (op) {
	case TOKlt: out_code = LT_EXPR; is_compare = true; break;
	case TOKgt: out_code = GT_EXPR; is_compare = true; break;
	case TOKle: out_code = LE_EXPR; is_compare = true; break;
	case TOKge: out_code = GE_EXPR; is_compare = true; break;
	case TOKunord: out_code = UNORDERED_EXPR; break;
	case TOKlg:
	    {
		e1 = irs->maybeMakeTemp(e1);
		e2 = irs->maybeMakeTemp(e2);
		return irs->boolOp(TRUTH_ORIF_EXPR,
		    build2(LT_EXPR, boolean_type_node, e1, e2),
		    build2(GT_EXPR, boolean_type_node, e1, e2));
	    }
	    break;
	default:
	    /* GCC 3.4 (others?) chokes on these unless
	       at least one operand is floating point. */
	    if (FLOAT_TYPE_P( TREE_TYPE( e1 )) &&
		FLOAT_TYPE_P( TREE_TYPE( e2 ))) {
		switch (op) {
		case TOKleg: out_code = ORDERED_EXPR; break;
		case TOKule: out_code = UNLE_EXPR; break;
		case TOKul:  out_code = UNLT_EXPR; break;
		case TOKuge: out_code = UNGE_EXPR; break;
		case TOKug:  out_code = UNGT_EXPR; break;
		case TOKue:  out_code = UNEQ_EXPR; break;
		default:
		    abort();
		}
	    } else {
		switch (op) {
		case TOKleg:
		    // %% is this properly optimized away?
		    return irs->voidCompound(irs->compound(e1,e2),
			convert(boolean_type_node, integer_one_node));
		    break;
		case TOKule: out_code = LE_EXPR; break;
		case TOKul:  out_code = LT_EXPR; break;
		case TOKuge: out_code = GE_EXPR; break;
		case TOKug:  out_code = GT_EXPR; break;
		case TOKue:  out_code = EQ_EXPR; break;
		default:
		    abort();
		}
	    }
	}
    }

    if (is_compare)
	signed_compare_check(& e1, & e2);

    tree t = build2(out_code, boolean_type_node, // exp->type->toCtype(),
	e1, e2);
#if D_GCC_VER >= 40
    /* Need to use fold().  Otherwise, complex-var == complex-cst is not
       gimplified correctly. */

    if (COMPLEX_FLOAT_TYPE_P( TREE_TYPE( e1 )) ||
	COMPLEX_FLOAT_TYPE_P( TREE_TYPE( e2 )))
	t = fold(t);
#endif
    return t;
}

static tree
make_bool_binop(BinExp * exp, IRState * irs)
{
    tree t1 = exp->e1->toElem(irs);
    tree t2 = exp->e2->toElem(irs);
    if (exp->op == TOKandand || exp->op == TOKoror) {
	t1 = irs->convertForCondition(t1, exp->e1->type);
	t2 = irs->convertForCondition(t2, exp->e2->type);
    }
    tree t = make_bool_binop(exp->op, t1, t2, irs);
    return convert(exp->type->toCtype(), t);
}

elem *
IdentityExp::toElem(IRState* irs)
{
    TY ty1 = e1->type->toBasetype()->ty;

    // Assuming types are the same from typeCombine
    //if (ty1 != e2->type->toBasetype()->ty)
    //abort();

    switch (ty1) {
    case Tsarray:
	return build2(op == TOKidentity ? EQ_EXPR : NE_EXPR,
	    type->toCtype(),
	    irs->addressOf(e1->toElem(irs)),
	    irs->addressOf(e2->toElem(irs)));
    case Treference:
    case Tclass:
    case Tarray:
	return make_bool_binop(this, irs);
    default:
	// For operand types other than class objects, static or dynamic
	// arrays, identity is defined as being the same as equality

	// Assumes object == object has been changed to function call
	// ... impl is really the same as the special cales
	return make_bool_binop(this, irs);
    }
}

elem *
EqualExp::toElem(IRState* irs)
{
    Type * base_type_1 = e1->type->toBasetype();
    TY base_ty_1 = base_type_1->ty;
    TY base_ty_2 = e2->type->toBasetype()->ty;

    if ( (base_ty_1 == Tsarray || base_ty_1 == Tarray ||
	     base_ty_2 == Tsarray || base_ty_2 == Tarray) ) {

	Type * elem_type = base_type_1->nextOf()->toBasetype();

	// _adCmp compares each element.  If bitwise comparison is ok,
	// use memcmp.

	if (elem_type->isfloating() || elem_type->isClassHandle() ||
	    elem_type->ty == Tsarray || elem_type->ty == Tarray ||
	    elem_type->ty == Tstruct)
	{
	    tree args[3] = {
		irs->toDArray(e1),
		irs->toDArray(e2),
		irs->typeinfoReference(elem_type) };
	    tree result = irs->libCall(LIBCALL_ADEQ, 3, args);
	    result = convert(type->toCtype(), result);
	    if (op == TOKnotequal)
		result = build1(TRUTH_NOT_EXPR, type->toCtype(), result);
	    return result;
	} else if (base_ty_1 == Tsarray && base_ty_2 == Tsarray) {
	    // assuming sizes are equal
	    // shouldn't need to check for Tbit
	    return make_bool_binop(this, irs);
	} else {
	    tree len_expr[2];
	    tree data_expr[2];

	    gcc_assert(elem_type->ty != Tbit);

	    for (int i = 0; i < 2; i++) {
		Expression * e = i == 0 ? e1 : e2;
		TY e_base_ty = i == 0 ? base_ty_1 : base_ty_2;

		if ( e_base_ty == Tarray ) {
		    tree array_expr = irs->maybeMakeTemp( e->toElem(irs) );

		    data_expr[i] = irs->darrayPtrRef( array_expr );
		    len_expr[i]  = irs->darrayLenRef( array_expr ); // may be used twice -- should be okay
		} else {
		    data_expr[i] = irs->addressOf( e->toElem(irs) );
		    len_expr[i]  = ((TypeSArray *) e->type->toBasetype())->dim->toElem(irs);
		}
	    }

	    tree t_memcmp = built_in_decls[BUILT_IN_MEMCMP];
	    tree result;
	    tree size;

	    size = build2(MULT_EXPR, size_type_node,
		convert(size_type_node, len_expr[0]), // should be size_type already, though
		size_int(elem_type->size()));
	    size = fold( size );

	    result = irs->buildCall( TREE_TYPE(TREE_TYPE( t_memcmp )),
		irs->addressOf( t_memcmp ),
		tree_cons( NULL_TREE, data_expr[0],
		    tree_cons( NULL_TREE, data_expr[1],
			tree_cons( NULL_TREE, size, NULL_TREE ))));

	    result = irs->boolOp(op == TOKequal ? TRUTH_ANDIF_EXPR : TRUTH_ORIF_EXPR,
		irs->boolOp(op == TOKequal ? EQ_EXPR : NE_EXPR, len_expr[0], len_expr[1]),
		irs->boolOp(op == TOKequal ? EQ_EXPR : NE_EXPR, result,	integer_zero_node));

	    return convert(type->toCtype(), result);
	}
    } else {
	// Taarray case not defined in spec, probably should be a library call
	return make_bool_binop(this, irs);
    }
}

elem *
InExp::toElem(IRState * irs)
{
    Type * e2_base_type = e2->type->toBasetype();
    AddrOfExpr aoe;
    assert( e2_base_type->ty == Taarray );

    tree args[3];
    Type * key_type = ((TypeAArray *) e2_base_type)->index->toBasetype();
    args[0] = e2->toElem(irs);
    args[1] = irs->typeinfoReference(key_type);
    args[2] = aoe.set(irs, irs->convertTo( e1, key_type ) );
    return d_convert_basic(type->toCtype(),
	aoe.finish(irs, irs->libCall(LIBCALL_AAINP, 3, args) ));
}

elem *
CmpExp::toElem(IRState* irs)
{
    Type * base_type_1 = e1->type->toBasetype();
    TY base_ty_1 = base_type_1->ty;
    TY base_ty_2 = e2->type->toBasetype()->ty;

    if ( (base_ty_1 == Tsarray || base_ty_1 == Tarray ||
	     base_ty_2 == Tsarray || base_ty_2 == Tarray) ) {

	Type * elem_type = base_type_1->nextOf()->toBasetype();
	tree args[3];
	unsigned n_args = 2;
	LibCall lib_call;

	args[0] = irs->toDArray(e1);
	args[1] = irs->toDArray(e2);

	switch (elem_type->ty) {
	case Tvoid:
	    lib_call = LIBCALL_ADCMPCHAR;
	    break;
	default:
	    gcc_assert(elem_type->ty != Tbit);

	    // Tuns8, Tchar, Tbool
	    if (elem_type->size() == 1 && elem_type->isscalar() &&
		elem_type->isunsigned())
		lib_call = LIBCALL_ADCMPCHAR;
	    else
	    {
		args[2] = irs->typeinfoReference(elem_type);
		n_args = 3;
		lib_call = LIBCALL_ADCMP;
	    }
	    break;
	}

	tree result = irs->libCall(lib_call, n_args, args);
	enum tree_code out_code;

	// %% For float element types, warn that NaN is not taken into account ?

	switch (this->op) {
	case TOKlt: out_code = LT_EXPR; break;
	case TOKgt: out_code = GT_EXPR; break;
	case TOKle: out_code = LE_EXPR; break;
	case TOKge: out_code = GE_EXPR; break;

	case TOKlg: out_code = NE_EXPR; break;
	case TOKunord:
	case TOKleg:
	    // %% Could do a check for side effects and drop the unused condition
	    return build2(COMPOUND_EXPR, boolean_type_node,
		result,
		d_truthvalue_conversion( this->op == TOKunord ? integer_zero_node : integer_one_node ));
	case TOKule: out_code = LE_EXPR; break;
	case TOKul:  out_code = LT_EXPR; break;
	case TOKuge: out_code = GE_EXPR; break;
	case TOKug:  out_code = GT_EXPR; break;
	case TOKue:  out_code = EQ_EXPR; break;
	    break;
	default:
	    abort();
	    return 0;
	}

	result = build2(out_code, boolean_type_node, result, integer_zero_node);
	return convert(type->toCtype(), result);
    } else {
	return make_bool_binop(this, irs);
    }
}

static tree
make_math_op(TOK op, tree e1, Type * e1_type, tree e2, Type * e2_type, Type * exp_type, IRState * irs)
{
    // Integral promotions have already been done in the front end
    tree_code out_code;

    // %% faster: check if result is complex
    if (( ( e1_type->isreal() && e2_type->isimaginary() ) ||
	  ( e1_type->isimaginary() && e2_type->isreal() )) &&
	(op == TOKadd || op == TOKmin )) {
	// %%TODO: need to check size/modes
	tree e2_adj;
	tree e_real, e_imag;

	if (op == TOKadd) {
	    e2_adj = e2;
	} else {
	    e2_adj = build1(NEGATE_EXPR, TREE_TYPE(e2), e2);
	}

	if (e1_type->isreal()) {
	    e_real = e1;
	    e_imag = e2_adj;
	} else {
	    e_real = e2_adj;
	    e_imag = e1;
	}

	return build2(COMPLEX_EXPR, exp_type->toCtype(), e_real, e_imag);

    } else {
	switch (op) {
	case TOKadd: out_code = PLUS_EXPR; break;
	case TOKmin: out_code = MINUS_EXPR; break;
	case TOKmul: out_code = MULT_EXPR; break;
	case TOKxor: out_code = BIT_XOR_EXPR; break;
	case TOKor:  out_code = BIT_IOR_EXPR; break;
	case TOKand: out_code = BIT_AND_EXPR; break;
	case TOKshl: out_code = LSHIFT_EXPR; break;
	case TOKushr: // drop through
	case TOKshr: out_code = RSHIFT_EXPR; break;
	case TOKmod:
	    if (e1_type->isintegral())
		out_code = TRUNC_MOD_EXPR;
	    else {
		return irs->floatMod(e1, e2, e1_type);
	    }
	    break;
	case TOKdiv:
	    if (e1_type->isintegral())
		out_code = TRUNC_DIV_EXPR;
	    else {
		out_code = RDIV_EXPR;
	    }
	    break;
	default:
	    abort();
	}
    }

    bool is_unsigned = e1_type->isunsigned() || e2_type->isunsigned()
	|| op == TOKushr;
#if D_GCC_VER >= 43
    if (POINTER_TYPE_P(TREE_TYPE(e1)) && e2_type->isintegral())
    {
	return irs->nop(irs->pointerOffsetOp(out_code, e1, e2), exp_type->toCtype());
    }
    else if (e1_type->isintegral() && POINTER_TYPE_P(TREE_TYPE(e2)))
    {
	gcc_assert(out_code == PLUS_EXPR);
	return irs->nop(irs->pointerOffsetOp(out_code, e2, e1), exp_type->toCtype());
    }
    else if (POINTER_TYPE_P(TREE_TYPE(e1)) && POINTER_TYPE_P(TREE_TYPE(e2)))
    {
	/* Need to convert pointers to integers because tree-vrp asserts
	   against (ptr MINUS ptr). */

	// %% need to work on this
	tree tt = d_type_for_mode(ptr_mode, TYPE_UNSIGNED(exp_type->toCtype()));
	e1 = convert(tt, e1);
	e2 = convert(tt, e2);
	return convert(exp_type->toCtype(),
	    build2(out_code, tt, e1, e2));
    }
    else
#endif
    if (exp_type->isintegral() &&
	( exp_type->isunsigned() != 0 ) != is_unsigned) {
	tree e_new_type_1 = is_unsigned ?
	    d_unsigned_type(exp_type->toCtype()) :
	    d_signed_type(exp_type->toCtype());
	/* For >>> and >>>= operations, need e1 to be of the same
	   signedness as what we are converting to, else we get
	   undefined behaviour when optimizations are enabled. */
	if (op == TOKushr) {
	    e1 = convert(e_new_type_1, e1);
	}
	tree t = build2(out_code, e_new_type_1, e1, e2);
	return convert(exp_type->toCtype(), t);
    } else {
	/* Front-end does not do this conversion and GCC does not
	   always do it right. */
	tree_code tc1 = TREE_CODE(TREE_TYPE(e1));
	tree_code tc2 = TREE_CODE(TREE_TYPE(e2));
	if (tc1 == COMPLEX_TYPE && tc2 != COMPLEX_TYPE)
	    e2 = irs->convertTo(e2, e2_type, e1_type);
	else if (tc2 == COMPLEX_TYPE && tc1 != COMPLEX_TYPE)
	    e1 = irs->convertTo(e1, e1_type, e2_type);
#if ENABLE_CHECKING
	else {
	    if (!irs->typesSame(e1_type, exp_type))
		e1 = irs->convertTo(e1, e1_type, exp_type);
	    if (!irs->typesSame(e2_type, exp_type))
		e2 = irs->convertTo(e2, e2_type, exp_type);
	}
#endif
	return build2(out_code, exp_type->toCtype(), e1, e2);
    }
}

tree
make_math_op(BinExp * exp, IRState * irs)
{
    TY ty1 = exp->e1->type->toBasetype()->ty;
    TY ty2 = exp->e2->type->toBasetype()->ty;

    if ((ty1 == Tarray || ty1 == Tsarray) ||
	(ty2 == Tarray || ty2 == Tsarray))
    {
	error("Array operation %s not implemented", exp->toChars());
	return irs->errorMark(exp->type);
    }

    return make_math_op(exp->op,
	exp->e1->toElem(irs), exp->e1->type,
	exp->e2->toElem(irs), exp->e2->type,
	exp->type, irs);
}


elem *
AndAndExp::toElem(IRState * irs)
{
    if (e2->type->toBasetype()->ty != Tvoid)
	return make_bool_binop(this, irs);
    else
	return build3(COND_EXPR, type->toCtype(),
	    e1->toElem(irs), e2->toElem(irs), d_void_zero_node);
}

elem *
OrOrExp::toElem(IRState * irs)
{
    if (e2->type->toBasetype()->ty != Tvoid)
	return make_bool_binop(this, irs);
    else
	return build3(COND_EXPR, type->toCtype(),
	    build1(TRUTH_NOT_EXPR, boolean_type_node, e1->toElem(irs)),
	    e2->toElem(irs), d_void_zero_node);
}

elem *
XorExp::toElem(IRState * irs) { return make_math_op(this, irs); }
elem *
OrExp::toElem(IRState * irs) { return make_math_op(this, irs); }
elem *
AndExp::toElem(IRState * irs) { return make_math_op(this, irs); }
elem *
UshrExp::toElem(IRState* irs) { return make_math_op(this, irs); }
elem *
ShrExp::toElem(IRState * irs) { return make_math_op(this, irs); }
elem *
ShlExp::toElem(IRState * irs) { return make_math_op(this, irs); }

elem *
ModExp::toElem(IRState * irs)
{
    return make_math_op(this, irs);
}
elem *
DivExp::toElem(IRState * irs)
{
    return make_math_op(this, irs);
}
elem *
MulExp::toElem(IRState * irs)
{
    return make_math_op(this, irs);
}

static tree
one_elem_array(IRState * irs, Expression * value, tree & var_decl_out)
{
    tree v = irs->maybeExprVar(value->toElem(irs), & var_decl_out);
    return irs->darrayVal(value->type->arrayOf(), 1, irs->addressOf(v));
}

elem *
CatExp::toElem(IRState * irs)
{
    Type * elem_type;

    // One of the operands may be an element instead of an array.
    // Logic copied from CatExp::semantic
    {
	Type * tb1 = e1->type->toBasetype();
	Type * tb2 = e2->type->toBasetype();

	if ((tb1->ty == Tsarray || tb1->ty == Tarray) &&
	    irs->typesCompatible(e2->type, tb1->nextOf()))

	    elem_type = tb1->nextOf();
	else if ((tb2->ty == Tsarray || tb2->ty == Tarray) &&
	    irs->typesCompatible(e1->type, tb2->nextOf()))

	    elem_type = tb2->nextOf();
	else
	    elem_type = tb1->nextOf();
    }

    // Flatten multiple concatenations

    unsigned n_operands = 2;
    unsigned n_args;
    tree * args;
    Array elem_vars;
    tree result;

    {
	Expression * e = e1;
	while (e->op == TOKcat) {
	    e = ((CatExp*) e)->e1;
	    n_operands += 1;
	}
    }

    n_args = 1 + (n_operands > 2 ? 1 : 0) +
	n_operands * (n_operands > 2 && irs->splitDynArrayVarArgs ? 2 : 1);
    args = new tree[n_args];
    args[0] = irs->typeinfoReference(type);
    if (n_operands > 2)
	args[1] = irs->integerConstant(n_operands, Type::tuns32);

    unsigned ai = n_args - 1;
    CatExp * ce = this;

    while (ce) {
	Expression *oe = ce->e2;
	while (1)
	{
	    tree array_exp;
	    if (irs->typesCompatible(oe->type->toBasetype(), elem_type))
	    {
		tree elem_var = NULL_TREE;
		array_exp = one_elem_array(irs, oe, elem_var);
		if (elem_var)
		    elem_vars.push(elem_var);
	    }
	    else
		array_exp = irs->toDArray(oe);
	    if (n_operands > 2 && irs->splitDynArrayVarArgs)
	    {
		array_exp = irs->maybeMakeTemp(array_exp);
		args[ai--] = irs->darrayPtrRef(array_exp); // note: filling array
		args[ai--] = irs->darrayLenRef(array_exp); // backwards, so ptr 1st
	    }
	    else
		args[ai--] = array_exp;

	    if (ce) {
		if (ce->e1->op != TOKcat) {
		    oe = ce->e1;
		    ce = NULL;
		    // finish with atomtic lhs
		} else {
		    ce = (CatExp*) ce->e1;
		    break;  // continue with lhs CatExp
		}
	    } else
		goto all_done;
	}
    }
 all_done:

    result = irs->libCall(n_operands > 2 ? LIBCALL_ARRAYCATNT : LIBCALL_ARRAYCATT,
	n_args, args, type->toCtype());

    for (unsigned i = 0; i < elem_vars.dim; ++i)
    {
	tree elem_var = (tree) elem_vars.data[i];
	result = irs->binding(elem_var, result);
    }

    return result;
}

elem *
MinExp::toElem(IRState* irs)
{
    // The front end has already taken care of pointer-int and pointer-pointer
    return make_math_op(this, irs);
}

elem *
AddExp::toElem(IRState* irs)
{
    // The front end has already taken care of (pointer + integer)
    return make_math_op(this, irs);
}

tree chain_cvt(tree t, Type * typ, Array & casts, IRState * irs)
{
    for (int i = casts.dim - 1; i >= 0; i--) {
	t = irs->convertTo(t, typ, (Type *) casts.data[i]);
	typ = (Type *) casts.data[i];
    }
    return t;
}

tree
make_assign_math_op(BinExp * exp, IRState * irs)
{
    TY ty1 = exp->e1->type->toBasetype()->ty;
    TY ty2 = exp->e2->type->toBasetype()->ty;

    if ((ty1 == Tarray || ty1 == Tsarray) ||
	(ty2 == Tarray || ty2 == Tsarray))
    {
	error("Array operation %s not implemented", exp->toChars());
	return irs->errorMark(exp->type);
    }

    Expression * e1_to_use;
    Type * lhs_type = 0;
    tree result;
    TOK out_code;
    Array lhs_casts; // no more than two casts?

    switch (exp->op) {
    case TOKaddass:  out_code = TOKadd; break;
    case TOKminass:  out_code = TOKmin; break;
    case TOKmulass:  out_code = TOKmul; break;
    case TOKxorass:  out_code = TOKxor; break;
    case TOKorass:   out_code = TOKor; break;
    case TOKandass:  out_code = TOKand; break;
    case TOKshlass:  out_code = TOKshl; break;
    case TOKushrass: out_code = TOKushr; break;
    case TOKshrass:  out_code = TOKshr; break;
    case TOKmodass:  out_code = TOKmod; break;
    case TOKdivass:  out_code = TOKdiv; break;
    default:
	abort();
    }

    e1_to_use = exp->e1;
    lhs_type = e1_to_use->type;
    while (e1_to_use->op == TOKcast) {
	CastExp * cast_exp = (CastExp *) e1_to_use;
	assert(irs->typesCompatible(cast_exp->type, cast_exp->to)); // %% check, basetype?
	lhs_casts.push(cast_exp->to);
	e1_to_use = cast_exp->e1;
    }

    tree tgt = stabilize_reference( irs->toElemLvalue(e1_to_use) );
    tree lhs = chain_cvt(tgt, e1_to_use->type, lhs_casts, irs);

    Type * src_type	= lhs_type;

    {
	/* Determine the correct combined type from BinExp::typeCombine.  */
	TY ty = (TY) Type::impcnvResult[lhs_type->toBasetype()->ty][exp->e2->type->toBasetype()->ty];
	if (ty != Terror)
	    src_type = Type::basic[ty];
    }
    if ((out_code == TOKmul || out_code == TOKdiv) && exp->e1->type->isimaginary())
    {
	assert( exp->e2->type->isfloating() );
	if ( ! exp->e2->type->isimaginary() && ! exp->e2->type->iscomplex() )
	{
	    assert( exp->e1->type->size() == exp->e2->type->size() );
	    src_type = exp->e1->type;
	}
    }
    tree src = make_math_op(out_code, lhs, lhs_type,
	exp->e2->toElem(irs), exp->e2->type,
	src_type, irs);
    result = build2(MODIFY_EXPR, exp->type->toCtype(),
	tgt, irs->convertForAssignment(src, src_type, e1_to_use->type));

    return result;
}

elem *
XorAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
OrAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
AndAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
UshrAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
ShrAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
ShlAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
ModAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
DivAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
MulAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }

elem *
CatAssignExp::toElem(IRState * irs) {
    tree args[3];
    Type * elem_type = e1->type->toBasetype()->nextOf()->toBasetype();
    LibCall lib_call;
    AddrOfExpr aoe;

    args[0] = irs->typeinfoReference( type );
    args[1] = irs->addressOf( irs->toElemLvalue(e1) );

    gcc_assert(elem_type->ty != Tbit);

    if (irs->typesCompatible(elem_type, e2->type->toBasetype())) {
	// append an element
	args[2] = aoe.set(irs, e2->toElem(irs) );
	lib_call = LIBCALL_ARRAYAPPENDCTP;
    } else {
	// append an array
	args[2] = irs->toDArray(e2);
	lib_call = LIBCALL_ARRAYAPPENDT;
    }
    return aoe.finish(irs, irs->libCall(lib_call, 3, args, type->toCtype()));
}

elem *
MinAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }
elem *
AddAssignExp::toElem(IRState * irs) { return make_assign_math_op(this, irs); }


void do_array_set(IRState * irs, tree in_ptr, tree in_val, tree in_cnt) {
    irs->startBindings(); // %%maybe not

    tree count_var = irs->localVar(Type::tsize_t);
    tree ptr_var = irs->localVar(TREE_TYPE(in_ptr));
    tree val_var = NULL_TREE;
    tree value_to_use = NULL_TREE;

    DECL_INITIAL(count_var) = in_cnt;
    DECL_INITIAL(ptr_var) = in_ptr;

    irs->expandDecl(count_var);
    irs->expandDecl(ptr_var);

    if (irs->isFreeOfSideEffects(in_val)) {
	value_to_use = in_val;
    } else {
	val_var = irs->localVar(TREE_TYPE(in_val));
	DECL_INITIAL(val_var) = in_val;
	irs->expandDecl(val_var);
	value_to_use = val_var;
    }

    irs->startLoop(NULL);

    irs->continueHere();

    irs->exitIfFalse( build2(NE_EXPR, boolean_type_node,
		      convert(TREE_TYPE(count_var), integer_zero_node), count_var) );

    irs->doExp( build2(MODIFY_EXPR, void_type_node, irs->indirect(ptr_var), value_to_use));
    irs->doExp( build2(MODIFY_EXPR, void_type_node, ptr_var,
		    irs->pointerOffset(ptr_var,
			TYPE_SIZE_UNIT(TREE_TYPE(TREE_TYPE(ptr_var)))) ));
    irs->doExp( build2(MODIFY_EXPR, void_type_node, count_var,
		    build2(MINUS_EXPR, TREE_TYPE(count_var), count_var,
			convert(TREE_TYPE(count_var), integer_one_node))) );

    irs->endLoop();

    irs->endBindings();
}


// Create a tree node to set multiple elements to a single value
tree array_set_expr(IRState * irs, tree ptr, tree src, tree count) {
#if D_GCC_VER < 40
    tree exp = build3( (enum tree_code) D_ARRAY_SET_EXPR, void_type_node,
	ptr, src, count);
    TREE_SIDE_EFFECTS( exp ) = 1;
    return exp;
#else
    irs->pushStatementList();
    do_array_set(irs, ptr, src, count);
    return irs->popStatementList();
#endif
}

#if V2
// Determine if type is an array of structs that need a postblit.
static StructDeclaration *
needsPostblit(Type *t)
{
    t = t->toBasetype();
    while (t->ty == Tsarray)
	t = t->nextOf()->toBasetype();
    if (t->ty == Tstruct)
    {   StructDeclaration * sd = ((TypeStruct *)t)->sym;
	if (sd->postblit)
	    return sd;
    }
    return NULL;
}
#endif


elem *
AssignExp::toElem(IRState* irs) {
    // First, handle special assignment semantics
    if (e1->op == TOKarraylength) {
	// Assignment to an array's length property; resize the array.

	Type * array_type;
	Type * elem_type;
	tree args[5];
	tree array_exp;
	tree result;

	{
	    Expression * ae = ((ArrayLengthExp *) e1)->e1;
	    array_type = ae->type;
	    elem_type = ae->type->toBasetype()->nextOf(); // don't want ->toBasetype for the element type
	    array_exp = irs->addressOf( ae->toElem( irs ));
	}

#if ! V2
	gcc_assert(! elem_type->isbit());
#endif

	args[0] = irs->typeinfoReference(array_type);
	args[1] = irs->convertTo(e2, Type::tsize_t);
	args[2] = array_exp;

	LibCall lib_call = elem_type->isZeroInit() ?
	    LIBCALL_ARRAYSETLENGTHT : LIBCALL_ARRAYSETLENGTHIT;

	result = irs->libCall(lib_call, 3, args);
	result = irs->darrayLenRef( result );

	return result;
    } else if (e1->op == TOKslice) {
	Type * elem_type = e1->type->toBasetype()->nextOf()->toBasetype();

	gcc_assert(elem_type->ty != Tbit);

	if (irs->typesCompatible(elem_type, e2->type->toBasetype())) {
	    // Set a range of elements to one value.

	    // %% This is used for initing on-stack static arrays..
	    // should optimize with memset if possible
	    // %% vararg issues

	    tree dyn_array_exp = irs->maybeMakeTemp( e1->toElem(irs) );
#if V2
	    if (op != TOKblit)
	    {
		if (needsPostblit(elem_type) != NULL)
		{
		    tree e;
		    AddrOfExpr aoe;
		    tree args[4];
		    args[0] = irs->darrayPtrRef(dyn_array_exp);
		    args[1] = aoe.set(irs, e2->toElem(irs));
		    args[2] = irs->darrayLenRef(dyn_array_exp);
		    args[3] = irs->typeinfoReference(elem_type);
		    e = irs->libCall(op == TOKconstruct ?
			    LIBCALL_ARRAYSETCTOR : LIBCALL_ARRAYSETASSIGN,
			    4, args);
		    e = irs->compound(aoe.finish(irs, e), dyn_array_exp);
		    return e;
		}
	    }
#endif

	    tree set_exp = array_set_expr( irs, irs->darrayPtrRef(dyn_array_exp),
		e2->toElem(irs), irs->darrayLenRef(dyn_array_exp));
	    return irs->compound(set_exp, dyn_array_exp);
	} else {
#if V2
	    if (op != TOKblit && needsPostblit(elem_type) != NULL)
	    {
		tree args[3] = {
		    irs->typeinfoReference(elem_type),
		    irs->toDArray(e1),
		    irs->toDArray(e2)
		};
		return irs->libCall(op == TOKconstruct ?
		    LIBCALL_ARRAYCTOR : LIBCALL_ARRAYASSIGN,
		    3, args, type->toCtype());
	    }
	    else
#endif
	    if (global.params.useArrayBounds)
	    {
		tree args[3] = {
		    irs->integerConstant(elem_type->size(), Type::tsize_t),
		    irs->toDArray(e2),
		    irs->toDArray(e1) };
		return irs->libCall(LIBCALL_ARRAYCOPY, 3, args, type->toCtype());
	    }
	    else
	    {
		tree array[2] = {
		    irs->maybeMakeTemp( irs->toDArray(e1) ),
		    irs->toDArray(e2) };
		tree t_memcpy = built_in_decls[BUILT_IN_MEMCPY];
		tree result;
		tree size;

		size = build2(MULT_EXPR, size_type_node,
		    convert(size_type_node, irs->darrayLenRef(array[0])),
		    size_int(elem_type->size()));
		size = fold( size );

		result = irs->buildCall( TREE_TYPE(TREE_TYPE( t_memcpy )),
		    irs->addressOf( t_memcpy ),
		    tree_cons( NULL_TREE, irs->darrayPtrRef(array[0]),
			tree_cons( NULL_TREE, irs->darrayPtrRef(array[1]),
			    tree_cons( NULL_TREE, size, NULL_TREE))));

		return irs->compound( result, array[0], type->toCtype() );
	    }
	}
    } else {
	// Simple assignment

	tree lhs = irs->toElemLvalue(e1);
	tree result = build2(MODIFY_EXPR, type->toCtype(),
	    lhs, irs->convertForAssignment(e2, e1->type));

	return result;
    }
}

elem *
PostExp::toElem(IRState* irs)
{
    enum tree_code tc;
    if (op == TOKplusplus)
	tc = POSTINCREMENT_EXPR;
    else if (op == TOKminusminus)
	tc = POSTDECREMENT_EXPR;
    else
    {
	gcc_unreachable();
	return irs->errorMark(type);
    }
    tree result = build2(tc, type->toCtype(),
	irs->toElemLvalue(e1), e2->toElem(irs));
    TREE_SIDE_EFFECTS(result) = 1;
    return result;
}

elem *
IndexExp::toElem(IRState* irs)
{
    Type * array_type = e1->type->toBasetype();

    if (array_type->ty != Taarray) {
	ArrayScope aryscp(irs, lengthVar, loc);
	/* arrayElemRef will call aryscp.finish.  This result
	   of this function may be used as an lvalue and we
	   do not want it to be a BIND_EXPR. */
	return irs->arrayElemRef( this, & aryscp );
    } else {
	Type * key_type = ((TypeAArray *) array_type)->index->toBasetype();
	AddrOfExpr aoe;
	tree args[4];
	tree t;
	args[0] = e1->toElem(irs);
	args[1] = irs->typeinfoReference(key_type);
	args[2] = irs->integerConstant( array_type->nextOf()->size(), Type::tsize_t );
	args[3] = aoe.set(irs, irs->convertTo( e2, key_type ) );
	t = irs->libCall(LIBCALL_AAGETRVALUEP, 4, args, type->pointerTo()->toCtype());
	t = aoe.finish(irs, t);
	if (global.params.useArrayBounds) {
	    t = save_expr(t);
	    t = build3(COND_EXPR, TREE_TYPE(t), t, t,
		irs->assertCall(loc, LIBCALL_ARRAY_BOUNDS));
	}
	t = irs->indirect(t, type->toCtype());
	return t;
    }
}

elem *
CommaExp::toElem(IRState * irs)
{
    // CommaExp is used with DotTypeExp..?
    if (e1->op == TOKdottype && e2->op == TOKvar) {
	VarExp * ve = (VarExp *) e2;
	VarDeclaration * vd;
	FuncDeclaration * fd;
	/* Handle references to static variable and functions.  Otherwise,
	   just let the DotTypeExp report an error. */
	if (( (vd = ve->var->isVarDeclaration()) && ! vd->needThis() ) ||
	    ( (fd = ve->var->isFuncDeclaration()) && ! fd->isThis() ))
	    return e2->toElem(irs);
    }
    tree t1 = e1->toElem( irs );
    tree t2 = e2->toElem( irs );

    assert(type);
    return build2(COMPOUND_EXPR, type->toCtype(), t1, t2);
}

elem *
ArrayLengthExp::toElem(IRState * irs)
{
    if (e1->type->toBasetype()->ty == Tarray) {
	return irs->darrayLenRef(e1->toElem(irs));
    } else {
	// Tsarray case seems to be handled by front-end

	error("unexpected type for array length: %s", type->toChars());
	return irs->errorMark(type);
    }
}

elem *
SliceExp::toElem(IRState * irs)
{
    // This function assumes that the front end casts the result to a dynamic array.
    assert(type->toBasetype()->ty == Tarray);

    // Use convert-to-dynamic-array code if possible
    if (e1->type->toBasetype()->ty == Tsarray && ! upr && ! lwr)
	return irs->convertTo(e1->toElem(irs), e1->type, type);

    Type * orig_array_type = e1->type->toBasetype();

    tree orig_array_expr = NULL;
    tree orig_pointer_expr;
    tree final_len_expr = NULL;
    tree final_ptr_expr = NULL;
    tree array_len_expr = NULL;
    tree lwr_tree = NULL;
    tree upr_tree = NULL;

    ArrayScope aryscp(irs, lengthVar, loc);

    orig_array_expr = aryscp.setArrayExp( e1->toElem(irs), e1->type );
    orig_array_expr = irs->maybeMakeTemp( orig_array_expr );
    // specs don't say bounds if are checked for error or clipped to current size

    // Get the data pointer for static and dynamic arrays
    orig_pointer_expr = irs->convertTo(orig_array_expr, orig_array_type,
	orig_array_type->nextOf()->pointerTo());

    final_ptr_expr = orig_pointer_expr;

    // orig_array_expr is already a save_expr if necessary, so
    // we don't make array_len_expr a save_expr which is, at most,
    // a COMPONENT_REF on top of orig_array_expr.
    if ( orig_array_type->ty == Tarray ) {
	array_len_expr = irs->darrayLenRef( orig_array_expr );
    } else if ( orig_array_type->ty == Tsarray ) {
	array_len_expr  = ((TypeSArray *) orig_array_type)->dim->toElem(irs);
    } else {
	// array_len_expr == NULL indicates no bounds check is possible
    }

    if (lwr) {
	lwr_tree = lwr->toElem(irs);
	if (integer_zerop(lwr_tree))
	    lwr_tree = NULL_TREE;
    }
    if (upr) {
	upr_tree = upr->toElem(irs);
	if (global.params.useArrayBounds && array_len_expr) {
	    upr_tree = irs->maybeMakeTemp(upr_tree);
	    final_len_expr = irs->checkedIndex(loc, upr_tree, array_len_expr, true);
	}
	if (lwr_tree) {
	    lwr_tree = irs->maybeMakeTemp(lwr_tree);
	    // %% type
	    final_len_expr = build2(MINUS_EXPR, TREE_TYPE(final_len_expr), final_len_expr, lwr_tree);
	}
    } else {
	// If this is the case, than there is no lower bound specified and
	// there is no need to subtract.
	switch (orig_array_type->ty) {
	case Tarray:
	    final_len_expr = irs->darrayLenRef(orig_array_expr);
	    break;
	case Tsarray:
	    final_len_expr = ((TypeSArray *) orig_array_type)->dim->toElem(irs);
	    break;
	default:
	    ::error("Attempt to take length of something that was not an array");
	    return irs->errorMark(type);
	}
    }
    if (lwr_tree) {
	if (global.params.useArrayBounds && array_len_expr) { // %% && ! is zero
	    lwr_tree = irs->maybeCompound(
			    irs->checkedIndex(loc, lwr_tree, array_len_expr, true), // lower bound can equal length
			    upr_tree ? irs->checkedIndex(loc, lwr_tree, upr_tree, true) // implements check upr < lwr
				     : NULL_TREE);
	}

#if ! V2
	gcc_assert(! orig_array_type->next->isbit());
#endif

	final_ptr_expr = irs->pointerIntSum(irs->pvoidOkay(final_ptr_expr), lwr_tree);
	final_ptr_expr = irs->nop(final_ptr_expr, TREE_TYPE( orig_pointer_expr ));
    }

    return aryscp.finish( irs->darrayVal(type->toCtype(), final_len_expr, final_ptr_expr) );
}

elem *
CastExp::toElem(IRState * irs)
{
    return irs->convertTo(e1, to);
}

static tree
make_aa_del(IRState * irs, Expression * e_array, Expression * e_index)
{
    tree args[3];
    Type * key_type = ((TypeAArray *) e_array->type->toBasetype())->index->toBasetype();
    AddrOfExpr aoe;

    args[0] = e_array->toElem(irs);
    args[1] = irs->typeinfoReference(key_type);
    args[2] = aoe.set(irs, irs->convertTo( e_index, key_type ));
    return aoe.finish(irs, irs->libCall(LIBCALL_AADELP, 3, args) );
}

elem *
DeleteExp::toElem(IRState* irs)
{
    // Does this look like an associative array delete?
    if (e1->op == TOKindex &&
	((IndexExp*)e1)->e1->type->toBasetype()->ty == Taarray) {

	if (! global.params.useDeprecated)
	    error("delete aa[key] deprecated, use aa.remove(key)", e1->toChars());

	Expression * e_array = ((BinExp *) e1)->e1;
	Expression * e_index = ((BinExp *) e1)->e2;

	// Check that the array is actually an associative array
	if (e_array->type->toBasetype()->ty == Taarray)
	    return make_aa_del(irs, e_array, e_index);
    }

    // Otherwise, this is normal delete
    LibCall lib_call;
    tree t = e1->toElem(irs);
    Type * base_type = e1->type->toBasetype();

    switch (base_type->ty) {
    case Tclass:
	{
	    VarDeclaration * v;
	    bool is_intfc =
		base_type->isClassHandle()->isInterfaceDeclaration() != NULL;

	    if (e1->op == TOKvar &&
		( v = ((VarExp*) e1)->var->isVarDeclaration() ) && v->onstack)
		lib_call = is_intfc ?
		    LIBCALL_CALLINTERFACEFINALIZER : LIBCALL_CALLFINALIZER;
	    else
		lib_call = is_intfc ? LIBCALL_DELINTERFACE : LIBCALL_DELCLASS;
	}
	break;
    case Tarray: lib_call = LIBCALL_DELARRAY; break;
    case Tpointer: lib_call = LIBCALL_DELMEMORY; break;
    default:
	error("don't know how to delete %s", e1->toChars());
	return irs->errorMark(type);
    }

    if (lib_call != LIBCALL_CALLFINALIZER && lib_call != LIBCALL_CALLINTERFACEFINALIZER)
	t = irs->addressOf( t );

    return irs->libCall(lib_call, 1, & t);
}

elem *
RemoveExp::toElem(IRState * irs)
{
    Expression * e_array = e1;
    Expression * e_index = e2;

    // Check that the array is actually an associative array
    if (e_array->type->toBasetype()->ty == Taarray) {
	return make_aa_del(irs, e_array, e_index);
    } else {
	error("%s is not an associative array", e_array->toChars());
	return irs->errorMark(type);
    }
}

elem *
BoolExp::toElem(IRState * irs)
{
    if (e1->op == TOKcall && e1->type->toBasetype()->ty == Tvoid)
    {
	/* This could happen as '&&' is allowed as a shorthand for 'if'
	    eg:  (condition) && callexpr();  */
	return e1->toElem(irs);
    }

    return d_convert_basic(type->toCtype(), irs->convertForCondition(e1));
}

elem *
NotExp::toElem(IRState * irs)
{
    // %% doc: need to convert to boolean type or this will fail.
    tree t = build1(TRUTH_NOT_EXPR, boolean_type_node,
	irs->convertForCondition(e1));
    return convert(type->toCtype(), t);
}

elem *
ComExp::toElem(IRState * irs)
{
    return build1(BIT_NOT_EXPR, type->toCtype(), e1->toElem( irs ));
}

elem *
NegExp::toElem(IRState * irs)
{
    // %% GCC B.E. won't optimize (NEGATE_EXPR (INTEGER_CST ..))..
    // %% is type correct?
    TY ty1 = e1->type->toBasetype()->ty;

    if (ty1 == Tarray || ty1 == Tsarray)
    {
	error("Array operation %s not implemented", toChars());
	return irs->errorMark(type);
    }

    return build1(NEGATE_EXPR, type->toCtype(), e1->toElem(irs));
}

elem *
PtrExp::toElem(IRState * irs)
{
    /* add this from c-typeck.c:
	  TREE_READONLY (ref) = TYPE_READONLY (t);
	  TREE_SIDE_EFFECTS (ref)
	    = TYPE_VOLATILE (t) || TREE_SIDE_EFFECTS (pointer);
	  TREE_THIS_VOLATILE (ref) = TYPE_VOLATILE (t);
    */

    /* Produce better code by converting *(#rec + n) to
       COMPONENT_REFERENCE.  Otherwise, the variable will always be
       allocated in memory because its address is taken. */
    Type * rec_type = 0;
    target_size_t the_offset;
    tree rec_tree;

    if (e1->op == TOKadd) {
	BinExp * add_exp = (BinExp *) e1;
	if (add_exp->e1->op == TOKaddress &&
	    add_exp->e2->isConst() && add_exp->e2->type->isintegral()) {
	    Expression * rec_exp = ((AddrExp*) add_exp->e1)->e1;
	    rec_type = rec_exp->type->toBasetype();
	    rec_tree = rec_exp->toElem(irs);
	    the_offset = add_exp->e2->toUInteger();
	}
    } else if (e1->op == TOKsymoff) {
	// is this ever not a VarDeclaration?
	SymOffExp * sym_exp = (SymOffExp *) e1;
	if ( ! irs->isDeclarationReferenceType(sym_exp->var)) {
	    rec_type = sym_exp->var->type->toBasetype();
	    VarDeclaration * v = sym_exp->var->isVarDeclaration();
	    if (v)
		rec_tree = irs->var(v);
	    else
		rec_tree = sym_exp->var->toSymbol()->Stree;
	    the_offset = sym_exp->offset;
	}
	// otherwise, no real benefit?
    }

    if (rec_type && rec_type->ty == Tstruct) {
	StructDeclaration * sd = ((TypeStruct *)rec_type)->sym;
	for (unsigned i = 0; i < sd->fields.dim; i++) {
	    VarDeclaration * field = (VarDeclaration *) sd->fields.data[i];
	    if (field->offset == the_offset &&
		irs->typesSame(field->type, this->type)) {
		if (irs->isErrorMark(rec_tree))
		    return rec_tree; // backend will ICE otherwise
		return irs->component(rec_tree, field->toSymbol()->Stree);
	    } else if (field->offset > the_offset) {
		break;
	    }
	}
    }

    tree e = irs->indirect(e1->toElem(irs), type->toCtype());
    if (irs->inVolatile())
	TREE_THIS_VOLATILE( e ) = 1;
    return e;
}

elem *
AddrExp::toElem(IRState * irs)
{
    return irs->nop(irs->addressOf(e1->toElem(irs)), type->toCtype());
}

elem *
CallExp::toElem(IRState* irs)
{
    tree t = irs->call(e1, arguments);
#if V2
    TypeFunction * tf = (TypeFunction *)e1->type;
    if (tf->isref && e1->type->ty == Tfunction)
    {
	TREE_TYPE(t) = type->pointerTo()->toCtype();
	return irs->indirect(t);
    }
    //else
#endif
    // Some library calls are defined to return a generic type.
    // this->type is the real type. (See crash2.d)
    TREE_TYPE(t) = type->toCtype();
    return t;
}

elem *
Expression::toElem(IRState* irs)
{
    error("abstract Expression::toElem called");
    return irs->errorMark(type);
}

elem *
DotTypeExp::toElem(IRState *irs)
{
    // The only case in which this seems to be a valid expression is when
    // it is used to specify a non-virtual call ( SomeClass.func(...) ).
    // This case is handled in IRState::objectInstanceMethod.

    error("cannot use \"%s\" as an expression", toChars());

    // Can cause ICEs later; should just exit now.
    return irs->errorMark(type);
}

// The result will probably just be converted to a CONSTRUCTOR for a Tdelegate struct
elem *
DelegateExp::toElem(IRState* irs)
{
    Type * t = e1->type->toBasetype();
    if (t->ty == Tclass || t->ty == Tstruct) {
	// %% Need to see if DotVarExp ever has legitimate
	// <aggregate>.<static method>.  If not, move this test
	// to objectInstanceMethod.
	if (! func->isThis())
	    error("delegates are only for non-static functions");
	return irs->objectInstanceMethod(e1, func, type);
    } else {
	assert(func->isNested() || func->isThis());
	return irs->methodCallExpr(irs->functionPointer(func),
	    func->isNested() ?
#if D_NO_TRAMPOLINES
	    irs->getFrameForFunction(func)
#else
	    d_null_pointer
#endif
	    : e1->toElem(irs), type);
    }
}

elem *
DotVarExp::toElem(IRState * irs)
{
    FuncDeclaration * func_decl;
    VarDeclaration * var_decl;
    Type * obj_basetype = e1->type->toBasetype();
    TY obj_basetype_ty = obj_basetype->ty;
    switch (obj_basetype_ty) {
    case Tpointer:
	if (obj_basetype->nextOf()->toBasetype()->ty != Tstruct) {
	    break;
	}
	// drop through
    case Tstruct:
	// drop through
    case Tclass:
	if ( (func_decl = var->isFuncDeclaration()) ) {
	    // if Tstruct, objInstanceMethod will use the address of e1
	    return irs->objectInstanceMethod(e1, func_decl, type);
	} else if ( (var_decl = var->isVarDeclaration()) ) {
	    if (var_decl->storage_class & STCfield) {
		tree this_tree = e1->toElem(irs);
		if ( obj_basetype_ty != Tstruct )
		    this_tree = irs->indirect(this_tree);
		return irs->component(this_tree, var_decl->toSymbol()->Stree);
	    } else {
		return irs->var(var_decl);
	    }
	} else {
	    error("%s is not a field, but a %s", var->toChars(), var->kind());
	}
	break;
    default:
	break;
    }
    ::error("Don't know how to handle %s", toChars());
    return irs->errorMark(type);
}

elem *
AssertExp::toElem(IRState* irs)
{
    // %% todo: Do we call a Tstruct's invariant if
    // e1 is a pointer to the struct?
    if (global.params.useAssert) {
	Type * base_type = e1->type->toBasetype();
	TY ty = base_type->ty;
	tree assert_call = msg ?
	    irs->assertCall(loc, msg) : irs->assertCall(loc);

	if (ty == Tclass) {
	    ClassDeclaration * cd = base_type->isClassHandle();
	    tree arg = e1->toElem(irs);
	    if (cd->isCOMclass())
	    {
		return build3(COND_EXPR, void_type_node,
		    irs->boolOp(NE_EXPR, arg, d_null_pointer),
		    d_void_zero_node, assert_call);
	    }
	    else if (cd->isInterfaceDeclaration())
	    {
		arg = irs->convertTo(arg, base_type, irs->getObjectType());
	    }
	    return irs->libCall(LIBCALL_INVARIANT, 1, & arg);  // this does a null pointer check
	} else {
	    // build: ( (bool) e1  ? (void)0 : _d_assert(...) )
	    //    or: ( e1 != null ? (void)0 : _d_assert(...), e1._invariant() )
	    tree result;
	    tree invc = NULL_TREE;
	    tree e1_t = e1->toElem(irs);

	    if (ty == Tpointer) {
		Type * sub_type = base_type->nextOf()->toBasetype();
		if (sub_type->ty == Tstruct) {
		    AggregateDeclaration * agg_decl = ((TypeStruct *) sub_type)->sym;
		    if (agg_decl->inv) {
			Array args;
			e1_t = irs->maybeMakeTemp(e1_t);
			invc = irs->call(agg_decl->inv, e1_t, & args );
		    }
		}
	    }

	    result = build3(COND_EXPR, void_type_node,
		irs->convertForCondition( e1_t, e1->type ),
		d_void_zero_node, assert_call);
	    if (invc)
		result = build2(COMPOUND_EXPR, void_type_node, result, invc);
	    return result;
	}
    } else
	return d_void_zero_node;
}

elem *
DeclarationExp::toElem(IRState* irs)
{
    // VarDeclaration::toObjFile was modified to call d_gcc_emit_local_variable
    // if needed.  This assumes irs == g.irs
#if D_GCC_VER < 40
    tree rtl_expr = expand_start_stmt_expr(0);
    declaration->toObjFile(false);
    expand_end_stmt_expr (rtl_expr);
    return rtl_expr;
#else
    irs->pushStatementList();
    declaration->toObjFile(false);
    tree t = irs->popStatementList();
#if D_GCC_VER >= 43
    /* Construction of an array for typesafe-variadic function arguments
       can cause an empty STMT_LIST here.  This can causes problems
       during gimplification. */
    if (TREE_CODE(t) == STATEMENT_LIST && ! STATEMENT_LIST_HEAD(t))
	return build_empty_stmt();
#endif
    return t;
#endif
}

void
d_gcc_emit_local_variable(VarDeclaration * v)
{
    g.irs->emitLocalVar(v);
}

bool
d_gcc_supports_weak()
{
    return SUPPORTS_WEAK;
}

// %% check calling this directly?
elem *
FuncExp::toElem(IRState * irs)
{
    fd->toObjFile(false);

    Type * func_type = type->toBasetype();

    if (func_type->ty == Tpointer)
	func_type = func_type->nextOf()->toBasetype();

    switch (func_type->ty) {
    case Tfunction:
	return irs->nop(irs->addressOf(fd), type->toCtype());
    case Tdelegate:
	return irs->methodCallExpr(irs->functionPointer(fd), // trampoline or not
#if D_NO_TRAMPOLINES
	    irs->getFrameForFunction(fd),
#else
	    convert(ptr_type_node, integer_one_node),
#endif
	    type);
    default:
	::error("Unexpected FunExp type");
	return irs->errorMark(type);
    }

    // If nested, this will be a trampoline...
}

elem *
HaltExp::toElem(IRState* irs)
{
    // Needs improvement.  Avoid library calls if possible..
    tree t_abort = built_in_decls[BUILT_IN_ABORT];
    return irs->buildCall( TREE_TYPE(TREE_TYPE(t_abort)),
	irs->addressOf(t_abort), NULL_TREE);
}

#if V2
elem *
SymbolExp::toElem(IRState * irs)
{
    if (op == TOKvar) {
	if (var->storage_class & STCfield) {
	    /*::*/error("Need 'this' to access member %s", var->ident->string);
	    return irs->errorMark(type);
	}

	// For variables that are references (currently only out/inout arguments;
	// objects don't count), evaluating the variable means we want what it refers to.

	// TODO: is this ever not a VarDeclaration; (four sequences like this...)
	VarDeclaration * v = var->isVarDeclaration();
	tree e;
	if (v)
	    e = irs->var(v);
	else
	    e = var->toSymbol()->Stree;

	if ( irs->isDeclarationReferenceType(var) ) {
	    e = irs->indirect(e, var->type->toCtype());
	    if (irs->inVolatile()) {
		TREE_THIS_VOLATILE(e) = 1;
	    }
	} else {
	    if (irs->inVolatile()) {
		e = irs->addressOf(e);
		TREE_THIS_VOLATILE(e) = 1;
		e = irs->indirect(e);
		TREE_THIS_VOLATILE(e) = 1;
	    }
	}
#if ENABLE_CHECKING
	if (!irs->typesSame(var->type, type))
	    e = irs->convertTo(e, var->type, type);
#endif
	return e;
    } else if (op == TOKsymoff) {
	target_size_t offset = ((SymOffExp *) this)->offset;

	VarDeclaration * v = var->isVarDeclaration();
	tree a;
	if (v)
	    a = irs->var(v);
	else
	    a = var->toSymbol()->Stree;

	if ( irs->isDeclarationReferenceType(var) )
	    assert(POINTER_TYPE_P(TREE_TYPE(a)));
	else
	    a = irs->addressOf(a);

	if (! offset)
	    return convert(type->toCtype(), a);

	tree b = irs->integerConstant(offset, Type::tsize_t);
	return irs->nop(irs->pointerOffset(a, b), type->toCtype());
    }
    else
	gcc_assert(op == TOKvar || op == TOKsymoff);
    return error_mark_node;
}
#else
elem *
VarExp::toElem(IRState* irs)
{
    if (var->storage_class & STCfield) {
	/*::*/error("Need 'this' to access member %s", var->ident->string);
	return irs->errorMark(type);
    }

    // For variables that are references (currently only out/inout arguments;
    // objects don't count), evaluating the variable means we want what it refers to.
    //tree e = irs->var(var);
    VarDeclaration * v = var->isVarDeclaration();
    tree e;
    if (v)
	e = irs->var(v);
    else
	e = var->toSymbol()->Stree;

    if ( irs->isDeclarationReferenceType(var) ) {
	e = irs->indirect(e, var->type->toCtype());
	if (irs->inVolatile()) {
	    TREE_THIS_VOLATILE(e) = 1;
	}
    } else {
	if (irs->inVolatile()) {
	    e = irs->addressOf(e);
	    TREE_THIS_VOLATILE(e) = 1;
	    e = irs->indirect(e);
	    TREE_THIS_VOLATILE(e) = 1;
	}
    }
#if ENABLE_CHECKING
    if (!irs->typesSame(var->type, type))
	e = irs->convertTo(e, var->type, type);
#endif
    return e;
}

elem *
SymOffExp::toElem(IRState * irs) {
    //tree a = irs->var(var);
    tree a;
    VarDeclaration * v = var->isVarDeclaration();
    if (v)
	a = irs->var(v);
    else
	a = var->toSymbol()->Stree;

    if ( irs->isDeclarationReferenceType(var) )
	assert(POINTER_TYPE_P(TREE_TYPE(a)));
    else
	a = irs->addressOf(var);

    if (! offset)
	return convert(type->toCtype(), a);

    tree b = irs->integerConstant(offset, Type::tsize_t);
    return irs->nop(irs->pointerOffset(a, b), type->toCtype());
}
#endif

bool
isClassNestedIn(ClassDeclaration *inner, ClassDeclaration *outer)
{
    // not implemented yet
    return false;
}

static FuncDeclaration *
isClassNestedInFunction(ClassDeclaration * cd)
{
    while (cd) {
	if (cd->isNested()) {
	    Dsymbol * s = cd->toParent2();
	    FuncDeclaration * fd = s->isFuncDeclaration();

	    if (fd)
		return fd;
	    else {
		cd = s->isClassDeclaration();
		assert(cd);
	    }
	} else
	    break;
    }
    return NULL;
}


/*
  findThis

  Starting from the current function, try to find a suitable value of
  'this' in nested functions and (not implemented yet:) nested class
  instances.

  A suitable 'this' value is an instance of target_cd or a class that
  has target_cd as a base.
*/

static tree
findThis(IRState * irs, ClassDeclaration * target_cd)
{
    FuncDeclaration * fd = irs->func;

    while (fd) {
	AggregateDeclaration * fd_ad;
	ClassDeclaration * fd_cd;

	if ((fd_ad = fd->isThis()) &&
	    (fd_cd = fd_ad->isClassDeclaration())) {
	    if (target_cd == fd_cd) {
		return irs->var(fd->vthis);
	    } else if (target_cd->isBaseOf(fd_cd, NULL)) {
		assert(fd->vthis); // && fd->vthis->csym->Stree
		return irs->convertTo(irs->var(fd->vthis),
		    fd_cd->type, target_cd->type);
	    } else if (isClassNestedIn(fd_cd, target_cd)) {
		// not implemented
		assert(0);
	    } else {
		fd = isClassNestedInFunction(fd_cd);
	    }
	} else if (fd->isNested()) {
	    fd = fd->toParent2()->isFuncDeclaration();
	} else
	    fd = NULL;
    }
    return NULL_TREE;
}

elem *
NewExp::toElem(IRState * irs)
{
    Type * base_type = newtype->toBasetype();
    tree result;

    if (allocator)
	assert(newargs);

    switch (base_type->ty) {
    case Tclass:
	{
	    TypeClass * class_type = (TypeClass *) base_type;
	    ClassDeclaration * class_decl = class_type->sym;

	    tree new_call;
	    tree setup_exp = NULL_TREE;
	    // type->toCtype() is a REFERENCE_TYPE; we want the RECORD_TYPE
	    tree rec_type = TREE_TYPE( class_type->toCtype() );

	    // Allocation call (custom allocator or _d_newclass)
	    if (onstack) {
		tree stack_var = irs->localVar( rec_type );
		irs->expandDecl(stack_var);
		new_call = irs->addressOf(stack_var);
		setup_exp = build2(MODIFY_EXPR, rec_type,
		    irs->indirect(new_call, rec_type),
		    class_decl->toInitializer()->Stree);
	    } else if (allocator) {
		new_call = irs->call(allocator, newargs);
		new_call = save_expr( new_call );
		// copy memory...
		setup_exp = build2(MODIFY_EXPR, rec_type,
		    irs->indirect(new_call, rec_type),
		    class_decl->toInitializer()->Stree);
	    } else {
		tree arg = irs->addressOf( class_decl->toSymbol()->Stree );
		new_call = irs->libCall(LIBCALL_NEWCLASS, 1, & arg);
	    }

	    if (class_type->sym->isNested()) {
		tree vthis_value = NULL_TREE;
		tree vthis_field = class_type->sym->vthis->toSymbol()->Stree;

		if (thisexp) {
		    ClassDeclaration *thisexp_cd = thisexp->type->isClassHandle();
		    Dsymbol *outer = class_decl->toParent2();
		    target_ptrdiff_t offset = 0;

		    vthis_value = thisexp->toElem(irs);
		    if (outer != thisexp_cd) {
			ClassDeclaration * outer_cd = outer->isClassDeclaration();
			int i = outer_cd->isBaseOf(thisexp_cd, & offset);
			assert(i);
			// could just add offset
			vthis_value = irs->convertTo(vthis_value, thisexp->type, outer_cd->type);
		    }
		} else {
		    Dsymbol *outer = class_decl->toParent2();
		    ClassDeclaration *cd_outer = outer->isClassDeclaration();
		    FuncDeclaration *fd_outer = outer->isFuncDeclaration();

		    if (cd_outer) {
			vthis_value = findThis(irs, cd_outer);
			if (vthis_value == NULL_TREE)
			    error("outer class %s 'this' needed to 'new' nested class %s",
				cd_outer->ident->string, class_decl->ident->string);
		    } else if (fd_outer) {
			/* If a class nested in a function has no methods
			   and there are no other nested functions,
			   lower_nested_functions is never called and any
			   STATIC_CHAIN_EXPR created here will never be
			   translated. Use a null pointer for the link in
			   this case. */
			if (fd_outer->vthis) {
#if V2
			    if (fd_outer->closureVars.dim ||
				irs->getFrameInfo(fd_outer)->creates_closure)
#else
			    if(fd_outer->nestedFrameRef)
#endif
				vthis_value = irs->getFrameForNestedClass(class_decl); // %% V2: rec_type->class_type
			    else
				vthis_value = irs->var(fd_outer->vthis);
			}
			else {
			    vthis_value = d_null_pointer;
			}

		    } else {
			assert(0);
		    }

		}

		if (vthis_value) {
		    new_call = save_expr( new_call );
		    setup_exp = irs->maybeCompound(setup_exp,
			build2(MODIFY_EXPR, TREE_TYPE(vthis_field),
			    irs->component( irs->indirect(new_call, rec_type),
				vthis_field ),
			    vthis_value));
		}
	    }

	    new_call = irs->maybeCompound(setup_exp, new_call);

	    // Constructor call
	    if (member) {
		result = irs->call(member, new_call, arguments);
	    } else {
		result = new_call;
	    }
	    return irs->convertTo(result, base_type, type);
	}
    case Tarray:
	{
	    assert( ! allocator );
	    assert( arguments && arguments->dim > 0 );

	    LibCall lib_call;

	    Type * elem_init_type = newtype;

	    /* First, skip past dynamic array dimensions/types that will be
	       allocated by this call. */
	    for (unsigned i = 0; i < arguments->dim; i++)
		elem_init_type = elem_init_type->toBasetype()->nextOf(); // assert ty == Tarray

#if ! V2
	    gcc_assert(! elem_init_type->isbit());
#endif

	    if (arguments->dim == 1)
	    {
		lib_call = elem_init_type->isZeroInit() ?
		    LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;

		tree args[2];
		args[0] = irs->typeinfoReference(type);
		args[1] = ((Expression *) arguments->data[0])->toElem(irs);
		result = irs->libCall(lib_call, 2, args, type->toCtype());
	    }
	    else
	    {
		lib_call = elem_init_type->isZeroInit() ?
		    LIBCALL_NEWARRAYMTP : LIBCALL_NEWARRAYMITP;

		tree dims_var = irs->exprVar(irs->arrayType(size_type_node, arguments->dim));
		{
		    tree dims_init;
		    CtorEltMaker elms;

		    elms.reserve( arguments->dim );
		    for (unsigned i = 0; i < arguments->dim/* - 1*/; i++)
			elms.cons( irs->integerConstant(i, size_type_node),
			    ((Expression*) arguments->data[i])->toElem(irs) );
		    //elms.cons(final_length);
		    dims_init = make_node(CONSTRUCTOR);
		    TREE_TYPE( dims_init ) = TREE_TYPE( dims_var);
		    CONSTRUCTOR_ELTS( dims_init ) = elms.head;
		    DECL_INITIAL(dims_var) = dims_init;
		}

		tree args[3];
		args[0] = irs->typeinfoReference(type);
		args[1] = irs->integerConstant(arguments->dim, Type::tint32); // The ndims arg is declared as 'int'
		args[2] = irs->addressOf(dims_var);

		result = irs->libCall(lib_call, 3, args, type->toCtype());
		result = irs->binding(dims_var, result);
	    }

	    return irs->convertTo(result, base_type, type);
	}
	break;
    default:
	{
	    Type * object_type = newtype;
	    Type * handle_type = base_type->pointerTo();
	    tree new_call;
	    tree t;
	    bool need_init = true;

	    if (onstack) {
		tree stack_var = irs->localVar( object_type );
		irs->expandDecl(stack_var);
		new_call = irs->addressOf( stack_var );
	    } else if (allocator) {
		new_call = irs->call(allocator, newargs);
	    } else {
		tree args[2];
		LibCall lib_call = object_type->isZeroInit() ?
		    LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
		args[0] = irs->typeinfoReference( object_type->arrayOf() );
		args[1] = irs->integerConstant(1, Type::tsize_t);
		new_call = irs->libCall(lib_call, 2, args);
		new_call = irs->darrayPtrRef(new_call);
		need_init = false;
	    }
	    new_call = irs->nop(new_call, handle_type->toCtype());
	    if ( need_init ) {
		// Save the result allocation call.
		new_call = save_expr( new_call );
		t = irs->indirect(new_call);
		t = build2(MODIFY_EXPR, TREE_TYPE(t), t,
		    irs->convertForAssignment(object_type->defaultInit(), object_type) );
		new_call = irs->compound(t, new_call);
	    }
	    return irs->nop(new_call, type->toCtype());
	}
	break;
    }
}

elem * ScopeExp::toElem(IRState* irs) {
    ::error("%s is not an expression", toChars());
    return irs->errorMark(type);
}

elem * TypeExp::toElem(IRState* irs) {
    ::error("type %s is not an expression", toChars());
    return irs->errorMark(type);
}

#if V2 //keep D2 compatibility
elem * TypeDotIdExp::toElem(IRState* irs) {
    ::error("TypeDotIdExp::toElem: don't know what to do (%s)", toChars());
    return irs->errorMark(type);
}
#endif

elem *
StringExp::toElem(IRState * irs)
{
    Type * base_type = type->toBasetype();
    TY base_ty = type ? base_type->ty : (TY) Tvoid;
    tree value;

    switch (base_ty) {
    case Tarray:
    case Tpointer:
	// Assuming this->string is null terminated
	// .. need to terminate with more nulls for wchar and dchar?
	value = build_string((len + 1) * sz,
	    gen.hostToTargetString((char *) string, len + 1, sz));
	break;
    case Tsarray:
    case Tvoid:
	value = build_string(len * sz,
	    gen.hostToTargetString((char *) string, len, sz));
	break;
    default:
	error("Invalid type for string constant: %s", type->toChars());
	return irs->errorMark(type);
    }

    // %% endianess of wchar and dchar
    TREE_CONSTANT (value) = 1;
    TREE_READONLY (value) = 1;
    // %% array type doesn't match string length if null term'd...
    TREE_TYPE( value ) = irs->arrayType(base_ty != Tvoid ?
	base_type->nextOf() : Type::tchar, len);

    switch (base_ty) {
    case Tarray:
	value = irs->darrayVal(type, len, irs->addressOf( value ));
	break;
    case Tpointer:
	value = irs->addressOf( value );
	break;
    case Tsarray:
	// %% needed?
	TREE_TYPE(value) = type->toCtype();
	break;
    default:
	// nothing
	break;
    }
    return value;
}

elem *
TupleExp::toElem(IRState * irs)
{
    tree result = NULL_TREE;
    if (exps && exps->dim) {
	for (unsigned i = 0; i < exps->dim; ++i) {
	    Expression * e = (Expression *) exps->data[i];
	    result = irs->maybeVoidCompound(result, e->toElem(irs));
	}
    } else
	result = d_void_zero_node;

    return result;
}

/* Returns an expression that assignes the expressions in ALE to static
   array pointed to by MEM. */

// probably will need to pass the D array element type to get assignmets correct...
// [ [1,2,3], 4 ]; // doesn't work, so maybe not..
// opt: if all/most constant, should make a var and do array copy
tree
array_literal_assign(IRState * irs, tree mem, ArrayLiteralExp * ale)
{
    tree result = NULL_TREE;
    tree offset = size_int(0);
    tree elem_size = size_int( ale->type->toBasetype()->nextOf()->size() );

    for (unsigned i = 0; i < ale->elements->dim; i++) {
	Expression * e = (Expression *) ale->elements->data[i];
	tree elemp_e = irs->pointerOffset(mem, offset);
	tree assgn_e = irs->vmodify( irs->indirect(elemp_e),
	    e->toElem(irs));
	result = irs->maybeCompound(result, assgn_e);

	offset = size_binop(PLUS_EXPR, offset, elem_size);
    }
    return result;
}

elem *
ArrayLiteralExp::toElem(IRState * irs)
{
    Type * array_type = type->toBasetype();
    assert( array_type->ty == Tarray || array_type->ty == Tsarray ||
	    array_type->ty == Tpointer );
    tree elem_type = array_type->nextOf()->toCtype();
    tree d_array_type = array_type->nextOf()->arrayOf()->toCtype();

    tree args[2] = { irs->typeinfoReference(array_type->nextOf()->arrayOf()),
		     irs->integerConstant(elements->dim, size_type_node) };
    // Unfortunately, this does a useles initialization
    LibCall lib_call = array_type->nextOf()->isZeroInit() ?
	LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
    tree d_array = irs->libCall(lib_call, 2, args, d_array_type);

    tree mem = irs->maybeMakeTemp( irs->darrayPtrRef( d_array ));
    tree result = irs->maybeCompound( array_literal_assign(irs, mem, this),
	mem );
    if ( array_type->ty == Tarray ) {
	result = irs->darrayVal(d_array_type, elements->dim, result);
    } else {
	tree s_array_type = irs->arrayType(elem_type, elements->dim);
	if (array_type->ty == Tsarray)
	    result = irs->indirect(result, s_array_type);
    }

    return result;
}

elem *
AssocArrayLiteralExp::toElem(IRState * irs)
{
#if ! V2
    TypeAArray * aa_type = (TypeAArray *)type->toBasetype();
    assert(aa_type->ty == Taarray);
#else
    Type * a_type = type->toBasetype()->mutableOf();
    assert(a_type->ty == Taarray);
    TypeAArray * aa_type = (TypeAArray *)a_type;
    /* As of 2.020, the hash function for Aa (array of chars) is custom
     * and different from Axa and Aya, which get the generic hash function.
     * So, rewrite the type of the AArray so that if it's key type is an
     * array of const or invariant, make it an array of mutable.
     *
     * This gets fixed around 2.035, so we won't need this afterwards.
     */
    Type * tkey = aa_type->index->toBasetype();
    if (tkey->ty == Tarray)
    {
	tkey = tkey->nextOf()->mutableOf()->arrayOf();
	tkey = tkey->semantic(0, NULL);
	aa_type = new TypeAArray(aa_type->nextOf(), tkey);
	aa_type = (TypeAArray *)aa_type->merge();
    }
#endif
    assert(keys != NULL);
    assert(values != NULL);

    tree keys_var = irs->exprVar(irs->arrayType(aa_type->index, keys->dim)); //?
    tree vals_var = irs->exprVar(irs->arrayType(aa_type->next, keys->dim));
    tree keys_ptr = irs->nop(irs->addressOf(keys_var),
	aa_type->index->pointerTo()->toCtype());
    tree vals_ptr = irs->nop(irs->addressOf(vals_var),
	aa_type->next->pointerTo()->toCtype());
    tree keys_offset = size_int(0);
    tree vals_offset = size_int(0);
    tree keys_size = size_int( aa_type->index->size() );
    tree vals_size = size_int( aa_type->next->size() );
    tree result = NULL_TREE;

    for (unsigned i = 0; i < keys->dim; i++)
    {
	Expression * e;
	tree elemp_e, assgn_e;

	e = (Expression *) keys->data[i];
	elemp_e = irs->pointerOffset(keys_ptr, keys_offset);
	assgn_e = irs->vmodify( irs->indirect(elemp_e), e->toElem(irs) );
	keys_offset = size_binop(PLUS_EXPR, keys_offset, keys_size);
	result = irs->maybeCompound(result, assgn_e);

	e = (Expression *) values->data[i];
	elemp_e = irs->pointerOffset(vals_ptr, vals_offset);
	assgn_e = irs->vmodify( irs->indirect(elemp_e), e->toElem(irs) );
	vals_offset = size_binop(PLUS_EXPR, vals_offset, vals_size);
	result = irs->maybeCompound(result, assgn_e);
    }

    tree args[4] = { irs->typeinfoReference(aa_type),
		     irs->integerConstant(keys->dim, Type::tsize_t),
		     keys_ptr, vals_ptr };
    result = irs->maybeCompound(result,
	irs->libCall(LIBCALL_ASSOCARRAYLITERALTP, 4, args));

    tree ctor = make_node( CONSTRUCTOR );
    CtorEltMaker ce;
    TREE_TYPE( ctor ) = aa_type->toCtype();
    ce.cons(TYPE_FIELDS(TREE_TYPE( ctor )), result);
    CONSTRUCTOR_ELTS( ctor ) = ce.head;

    result = irs->binding(keys_var, irs->binding(vals_var, ctor));
    return irs->nop(result, type->toCtype());
}

elem *
StructLiteralExp::toElem(IRState *irs)
{
    assert(irs->typesSame(type->toBasetype(), sd->type->toBasetype()));

    tree ctor = make_node( CONSTRUCTOR );
    CtorEltMaker ce;
    TREE_TYPE( ctor ) = type->toCtype();

    if (elements)
	for (unsigned i = 0; i < elements->dim; ++i)
	{
	    Expression * e = (Expression *) elements->data[i];
	    if (e)
	    {
		VarDeclaration * fld = (VarDeclaration *) sd->fields.data[i];
#if V2
		Type * tb = fld->type->toBasetype();
		StructDeclaration * sd;
		if ((sd = needsPostblit(tb)) != NULL)
		{
		    tree ec;
		    if (tb->ty == Tsarray)
		    {
			/* Generate _d_arrayctor(ti, from = e, to = ec) */
			Type * ti = tb->nextOf();
			ec = irs->localVar(e->type->toCtype());
			tree args[3] = {
			    irs->typeinfoReference(ti),
			    irs->toDArray(e),
			    irs->convertTo(ec, e->type, ti->arrayOf())
			};
			irs->doExp(irs->libCall(LIBCALL_ARRAYCTOR, 3, args));
		    }
		    else
		    {
			/* Call __postblit(&ec) */
			Array args;
			FuncDeclaration * fd = sd->postblit;
			ec = e->toElem(irs);
			irs->doExp(irs->call(fd, irs->addressOf(ec), & args));
		    }
		    ce.cons(fld->csym->Stree, irs->convertTo(ec, e->type, fld->type));
		}
		else
#endif
		ce.cons(fld->csym->Stree, irs->convertTo(e, fld->type));
	    }
	}

    CONSTRUCTOR_ELTS( ctor ) = ce.head;
    return ctor;
}

elem *
NullExp::toElem(IRState * irs)
{
    TY base_ty = type->toBasetype()->ty;
    // 0 -> dynamic array.  This is a special case conversion.
    // Move to convert for convertTo if it shows up elsewhere.
    switch (base_ty) {
    case Tarray:
	return irs->darrayVal( type, 0, NULL );
    case Taarray:
	{
	    tree ctor = make_node(CONSTRUCTOR);
	    tree fa;
	    CtorEltMaker ce;

	    TREE_TYPE(ctor) = type->toCtype();
	    fa = TYPE_FIELDS(TREE_TYPE(ctor));
	    ce.cons(fa, convert(TREE_TYPE(fa), integer_zero_node));
	    CONSTRUCTOR_ELTS(ctor) = ce.head;
	    return ctor;
	}
	break;
    case Tdelegate:
	// makeDelegateExpression ?
	return irs->delegateVal(convert(ptr_type_node, integer_zero_node),
	    convert(ptr_type_node, integer_zero_node), type);
    default:
	return convert( type->toCtype(), integer_zero_node );
    }
}

elem *
ThisExp::toElem(IRState * irs) {
    if (var)
	return irs->var(var->isVarDeclaration());
    // %% DMD issue -- creates ThisExp without setting var to vthis
    // %%TODO: updated in 0.79-0.81?
    assert( irs->func );
    assert( irs->func->vthis );
    return irs->var(irs->func->vthis);
}

elem *
ComplexExp::toElem(IRState * irs)
{
    TypeBasic * compon_type;
    switch (type->toBasetype()->ty) {
    case Tcomplex32: compon_type = (TypeBasic *) Type::tfloat32; break;
    case Tcomplex64: compon_type = (TypeBasic *) Type::tfloat64; break;
    case Tcomplex80: compon_type = (TypeBasic *) Type::tfloat80; break;
    default:
	abort();
    }
    return build_complex(type->toCtype(),
	irs->floatConstant(creall(value), compon_type),
	irs->floatConstant(cimagl(value), compon_type));
}

elem *
RealExp::toElem(IRState * irs)
{
    return irs->floatConstant(value, type->toBasetype()->isTypeBasic());
}

elem *
IntegerExp::toElem(IRState * irs)
{
    return irs->integerConstant(value, type);
}

#if D_GCC_VER >= 40

static void
genericize_function(tree fndecl)
{
  FILE *dump_file;
  int local_dump_flags;

  /* Dump the C-specific tree IR.  */
  dump_file = dump_begin (TDI_original, &local_dump_flags);
  if (dump_file)
    {
      fprintf (dump_file, "\n;; Function %s",
	       lang_hooks.decl_printable_name (fndecl, 2));
      fprintf (dump_file, " (%s)\n",
	       IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl)));
      fprintf (dump_file, ";; enabled by -%s\n", dump_flag_name (TDI_original));
      fprintf (dump_file, "\n");

      //if (local_dump_flags & TDF_RAW)
	dump_node (DECL_SAVED_TREE (fndecl),
		   TDF_SLIM | local_dump_flags, dump_file);
	//else
	//print_c_tree (dump_file, DECL_SAVED_TREE (fndecl));
      fprintf (dump_file, "\n");

      dump_end (TDI_original, dump_file);
    }

  /* Go ahead and gimplify for now.  */
  //push_context ();
  gimplify_function_tree (fndecl);
  //pop_context ();

  /* Dump the genericized tree IR.  */
  dump_function (TDI_generic, fndecl);
}

#endif

void
FuncDeclaration::toObjFile(int multiobj)
{
    if (!g.ofile->shouldEmit(this))
	return;
    if (! global.params.useUnitTests &&	isUnitTestDeclaration())
	return;

    Symbol * this_sym = toSymbol();
    if (this_sym->outputStage)
	return;

    if (g.irs->shouldDeferFunction(this))
	return;

    this_sym->outputStage = InProgress;

    tree fn_decl = this_sym->Stree;

    if (! fbody) {
	if (! isNested()) {
	    // %% Should set this earlier...
	    DECL_EXTERNAL (fn_decl) = 1;
	    TREE_PUBLIC( fn_decl ) = 1;
	}
	g.ofile->rodc(fn_decl, 1);
	return;
    }

    tree param_list;
    tree result_decl;
    tree parm_decl = NULL_TREE;
    tree block;
#if D_NO_TRAMPOLINES
    tree static_chain_expr = NULL_TREE;
#if V2
    FuncDeclaration * closure_func = NULL;
    tree closure_expr = NULL_TREE;
#endif
#endif
    ClassDeclaration * cd;
    AggregateDeclaration * ad = NULL;

    announce_function( fn_decl );
    IRState * irs = IRState::startFunction(this);
#if V2
    TypeFunction * tf = (TypeFunction *)type;
    irs->useClosure(NULL, NULL_TREE);
#endif

#if D_GCC_VER < 40
    bool saved_omit_frame_pointer = flag_omit_frame_pointer;
    flag_omit_frame_pointer = gen.originalOmitFramePointer || naked;

    if (gen.functionNeedsChain(this))
	push_function_context();
#else
    // in 4.0, doesn't use push_function_context
    tree old_current_function_decl = current_function_decl;
    function * old_cfun = cfun;
#endif

    current_function_decl = fn_decl;

    TREE_STATIC( fn_decl ) = 1;
    {
	Type * func_type = tintro ? tintro : type;
	Type * ret_type = func_type->nextOf()->toBasetype();

	if (isMain() && ret_type->ty == Tvoid)
	    ret_type = Type::tint32;
#if V2
	/* Convert 'ref int foo' into 'int* foo' */
	if (tf->isref)
	    ret_type = ret_type->pointerTo();
#endif
	result_decl = build_decl( RESULT_DECL, NULL_TREE, ret_type->toCtype() );
    }
    g.ofile->setDeclLoc(result_decl, this);
    DECL_RESULT( fn_decl ) = result_decl;
    DECL_CONTEXT( result_decl ) = fn_decl;
    //layout_decl( result_decl, 0 );

#if D_GCC_VER >= 40
# if D_GCC_VER >= 43
    allocate_struct_function( fn_decl, false );
# else
    allocate_struct_function( fn_decl );
# endif
    // assuming the above sets cfun
    g.ofile->setCfunEndLoc(endloc);
#endif

    param_list = NULL_TREE;

#if V2
    bool needs_static_chain = irs->functionNeedsChain(this);
#endif

    int n_parameters = parameters ? parameters->dim : 0;

    // Special arguments...
    static const int VTHIS = -2;
    static const int VARGUMENTS = -1;

    for (int i = VTHIS; i < (int) n_parameters; i++) {
	VarDeclaration * param = 0;
	tree parm_type = 0;

	parm_decl = 0;

	if (i == VTHIS) {
	    if ( (ad = isThis()) )
		param = vthis;
	    else if (isNested()) {
		/* DMD still generates a vthis, but it should not be
		   referenced in any expression.

		   This parameter is hidden from the debugger.
		*/
		parm_type = ptr_type_node;
		parm_decl = build_decl(PARM_DECL, NULL_TREE, parm_type);
		DECL_ARTIFICIAL( parm_decl ) = 1;
		DECL_IGNORED_P( parm_decl ) = 1;
		DECL_ARG_TYPE (parm_decl) = TREE_TYPE (parm_decl); // %% doc need this arg silently disappears
#if D_NO_TRAMPOLINES
#if V2
		if (! needs_static_chain)
		{
		    closure_func = toParent2()->isFuncDeclaration();
		    closure_expr = parm_decl;
		}
		else
#endif
		{
		    static_chain_expr = parm_decl;
		}
#endif
	    } else
		continue;
	} else if (i == VARGUMENTS) {
	    if (v_arguments /*varargs && linkage == LINKd*/)
		param = v_arguments;
	    else
		continue;
	} else {
	    param = (VarDeclaration *) parameters->data[i];
	}
	if (param) {
	    parm_decl = param->toSymbol()->Stree;
	}

	DECL_CONTEXT (parm_decl) = fn_decl;
	// param->loc is not set, so use the function's loc
	// %%doc not setting this crashes debug generating code
	g.ofile->setDeclLoc( parm_decl, param ? (Dsymbol*) param : (Dsymbol*) this );

	// chain them in the correct order
	param_list = chainon (param_list, parm_decl);
    }

    // param_list is a number of PARM_DECL trees chained together (*not* a TREE_LIST of PARM_DECLs).
    // The leftmost parameter is the first in the chain.  %% varargs?
    DECL_ARGUMENTS( fn_decl ) = param_list; // %% in treelang, useless ? because it just sets them to getdecls() later

#if D_GCC_VER < 40
    rest_of_decl_compilation(fn_decl, NULL, /*toplevel*/1, /*atend*/0); // http://www.tldp.org/HOWTO/GCC-Frontend-HOWTO-7.html
    make_decl_rtl (fn_decl, NULL); // %% needed?
#else
    rest_of_decl_compilation(fn_decl, /*toplevel*/1, /*atend*/0);
#endif
    // ... has this here, but with more args...

    DECL_INITIAL( fn_decl ) = error_mark_node; // Just doing what they tell me to do...

    IRState::initFunctionStart(fn_decl, loc);
#if D_NO_TRAMPOLINES
    /* If this is a member function that nested (possibly indirectly) in another
       function, construct an expession for this member function's static chain
       by going through parent link of nested classes.
    */
    if (ad && (cd = ad->isClassDeclaration()) && ! (static_chain_expr
#if V2
	    || closure_expr
#endif
						    )) {
	/* D 2.0 Closures: this->vthis is passed as a normal parameter and
	   is valid to access as Stree before the closure frame is created. */
	tree t = vthis->toSymbol()->Stree;
	while ( cd->isNested() ) {
	    Dsymbol * d = cd->toParent2();


	    tree vthis_field = cd->vthis->toSymbol()->Stree;
	    t = irs->component(irs->indirect(t), vthis_field);
	    FuncDeclaration * f;
	    if ( (f = d->isFuncDeclaration() )) {
#if V2
		if (! needs_static_chain)
		{
		    closure_expr = t;
		    closure_func = f;
		}
		else
#endif
		{
		    static_chain_expr = t;
		}
		break;
	    } else if ( (cd = d->isClassDeclaration()) ) {
		// nothing
	    } else {
		assert(0);
	    }
	}
    }
#endif

#if D_GCC_VER >= 40
    cfun->naked = naked ? 1 : 0;
#endif
#if D_GCC_VER < 40
    // Must be done before expand_function_start.
    cfun->static_chain_expr = static_chain_expr;

    expand_function_start (fn_decl, 0);

    /* If this function is the C `main', emit a call to `__main'
       to run global initializers, etc.  */
    if (linkage == LINKc &&
	DECL_ASSEMBLER_NAME (fn_decl)
	&& MAIN_NAME_P (DECL_ASSEMBLER_NAME (fn_decl)) // other langs use DECL_NAME..
	&& DECL_FILE_SCOPE_P (fn_decl)
	)
	expand_main_function ();


    //cfun->x_whole_function_mode_p = 1; // %% I gues...
    //cfun->function_frequency = ; // %% it'd be nice to do something with this..
    //need DECL_RESULT ?

    // Start a binding level for the function/arguments
    (*lang_hooks.decls.pushlevel) (0);
    expand_start_bindings (2);

    // Add the argument declarations to the symbol table for the back end
    set_decl_binding_chain( DECL_ARGUMENTS( fn_decl ));

    // %% TREE_ADDRESSABLE and TREE_USED...

    // Start a binding level for the function body
    //(*lang_hooks.decls.pushlevel) (0);
#else
    pushlevel(0);
    irs->pushStatementList();
#endif

    irs->startScope();
    irs->doLineNote(loc);

#if D_GCC_VER >= 40
    if (static_chain_expr) {
	cfun->custom_static_chain = 1;
	irs->doExp( build2(MODIFY_EXPR, ptr_type_node,
		build0( STATIC_CHAIN_DECL, ptr_type_node ), static_chain_expr) );
    }
#endif

#if V2
    if (static_chain_expr || closure_expr)
	irs->useParentClosure();

    if (closure_expr)
    {
	if (! DECL_P(closure_expr)) {
	    tree c = irs->localVar(ptr_type_node);
	    DECL_INITIAL(c) = closure_expr;
	    irs->expandDecl(c);
	    closure_expr = c;
	}
	irs->useClosure(closure_func, closure_expr);
    }

    buildClosure(irs); // may change irs->closureLink and irs->closureFunc
#endif

    if (vresult)
	irs->emitLocalVar(vresult);

    if (v_argptr) {
#if D_GCC_VER < 40
	tree var = irs->var(v_argptr);
	tree init_exp = irs->buildCall(void_type_node,
	    irs->addressOf( built_in_decls[BUILT_IN_VA_START] ),
	    tree_cons(NULL_TREE, irs->addressOf(var),
		tree_cons( NULL_TREE, parm_decl, NULL_TREE)));
	tree cleanup = irs->buildCall(void_type_node,
	    irs->addressOf( built_in_decls[BUILT_IN_VA_END] ),
	    tree_cons(NULL_TREE, irs->addressOf(var), NULL_TREE));
	v_argptr->init = NULL; // VoidInitializer?
	irs->emitLocalVar(v_argptr, true);

#if V2
	/* Note: cleanup will not run if v_argptr is a closure variable.
	   Probably okay for now because va_end doesn't do anything for any
	   GCC 3.3.x target.
	*/
	if (! v_argptr->toSymbol()->SclosureField)
#endif
	{
	    expand_decl_cleanup(var, cleanup);
	    expand_expr_stmt_value(init_exp, 0, 1);
	}
#else
	irs->pushStatementList();
#endif
    }
    if (v_arguments_var)
	irs->emitLocalVar(v_arguments_var, true);

    Statement * the_body = fbody;
    if (isSynchronized()) {
	AggregateDeclaration * asym;
	ClassDeclaration * sym;

	if ( (asym = isMember()) && (sym = asym->isClassDeclaration()) ) {
	    if (vthis != NULL) {
		VarExp * ve = new VarExp(fbody->loc, vthis);
		the_body = new SynchronizedStatement(fbody->loc, ve, fbody);
	    } else {
		if (!sym->vclassinfo)
		    sym->vclassinfo = new ClassInfoDeclaration(sym);
		Expression * e = new VarExp(fbody->loc, sym->vclassinfo);
		e = new AddrExp(fbody->loc, e);
		e->type = sym->type;
		the_body = new SynchronizedStatement(fbody->loc, e, fbody);
	    }
	} else {
	    error("synchronized function %s must be a member of a class", toChars());
	}
    }
    the_body->toIR(irs);

    if (this_sym->otherNestedFuncs)
    {
	for (unsigned i = 0; i < this_sym->otherNestedFuncs->dim; ++i)
	{
	    ((FuncDeclaration *) this_sym->otherNestedFuncs->data[i])->toObjFile(false);
	}
    }

#ifdef TARGET_80387
    /* Users of inline assembler statements expect to be able to leave
       the result in ST(0).  Because GCC does not know about this, it
       will load NaN before generating the return instruction.

       Solve this by faking an instruction that we claim loads a value
       into ST(0), make GCC store it into a temp variable, and then
       return the temp variable.

       When optimization is turned on, this whole process results in
       no extra code!
    */
    /* This would apply to complex types as well, but GDC currently
       returns complex types as a struct instead of in ST(0) and ST(1).
     */
    if (inlineAsm && ! naked && type->nextOf()->isfloating() &&
	! type->nextOf()->iscomplex())
    {
	tree result_var = irs->localVar(TREE_TYPE(result_decl));

	tree nop_str = build_string(0, "");
	tree cns_str = build_string(2, "=t");
	tree out_arg = tree_cons(tree_cons(NULL_TREE, cns_str, NULL_TREE),
	    result_var, NULL_TREE);

	irs->expandDecl(result_var);
	irs->doAsm(nop_str, out_arg, NULL_TREE, NULL_TREE);
	irs->doReturn( build2(MODIFY_EXPR, TREE_TYPE(result_decl),
			   result_decl, result_var) );
    }
#endif


#if D_GCC_VER >= 40
    if (v_argptr) {
	tree body = irs->popStatementList();
	tree var = irs->var(v_argptr);
	tree init_exp = irs->buildCall(void_type_node,
	    irs->addressOf( built_in_decls[BUILT_IN_VA_START] ),
	    tree_cons(NULL_TREE, irs->addressOf(var),
		tree_cons( NULL_TREE, parm_decl, NULL_TREE)));
	v_argptr->init = NULL; // VoidInitializer?
	irs->emitLocalVar(v_argptr, true);
	irs->addExp(init_exp);

	tree cleanup = irs->buildCall(void_type_node,
	    irs->addressOf( built_in_decls[BUILT_IN_VA_END] ),
	    tree_cons(NULL_TREE, irs->addressOf(var), NULL_TREE));
	irs->addExp( build2( TRY_FINALLY_EXPR, void_type_node, body, cleanup ));
    }
#endif

    irs->endScope();

#if D_GCC_VER < 40
    expand_function_end ();
    block = (*lang_hooks.decls.poplevel) (1, 0, 1);
#else
    DECL_SAVED_TREE(fn_decl) = irs->popStatementList();

    /* In tree-nested.c, init_tmp_var expects a statement list to come
       from somewhere.  popStatementList returns expressions when
       there is a single statement.  This code creates a statemnt list
       unconditionally because the DECL_SAVED_TREE will always be a
       BIND_EXPR. */
    {
	tree body = DECL_SAVED_TREE(fn_decl);
	tree t;

	gcc_assert(TREE_CODE(body) == BIND_EXPR);

	t = TREE_OPERAND(body, 1);
	if (TREE_CODE(t) != STATEMENT_LIST) {
	    tree sl = alloc_stmt_list();
	    append_to_statement_list_force(t, & sl);
	    TREE_OPERAND(body, 1) = sl;
	} else if (! STATEMENT_LIST_HEAD(t)) {
	    /* For empty functions: Without this, there is a
	       segfault when inlined.  Seen on build=ppc-linux but
	       not others (why?). */
	    append_to_statement_list_force(
	       build1(RETURN_EXPR,void_type_node,NULL_TREE), & t);
	}
    }

    //block = (*lang_hooks.decls.poplevel) (1, 0, 1);
    block = poplevel(1, 0, 1);
#endif

    DECL_INITIAL (fn_decl) = block; // %% redundant, see poplevel
    BLOCK_SUPERCONTEXT( DECL_INITIAL (fn_decl) ) = fn_decl; // done in C, don't know effect

#if D_GCC_VER < 40
    expand_end_bindings (NULL_TREE, 0, 1);
#else
    if (! errorcount && ! global.errors)
	genericize_function (fn_decl);
#endif

    this_sym->outputStage = Finished;
    if (! errorcount && ! global.errors)
    	g.ofile->outputFunction(this);

#if D_GCC_VER < 40
    //rest_of_compilation( fn_decl );
    if (gen.functionNeedsChain(this))
	pop_function_context();
    else
	current_function_decl = NULL_TREE; // must come before endFunction

    g.irs->endFunction();

    flag_omit_frame_pointer = saved_omit_frame_pointer;
#else
    current_function_decl = old_current_function_decl; // must come before endFunction
    set_cfun(old_cfun);

    irs->endFunction();
#endif
}

#if V2

void
FuncDeclaration::buildClosure(IRState * irs)
{
    FuncFrameInfo * ffi = irs->getFrameInfo(this);
    if (! ffi->creates_closure)
	return;

    tree closure_rec_type = make_node(RECORD_TYPE);
    tree ptr_field = build_decl(FIELD_DECL, get_identifier("__closptr"), ptr_type_node);
    DECL_CONTEXT(ptr_field) = closure_rec_type;
    ListMaker fields;
    fields.chain(ptr_field);

    for (unsigned i = 0; i < closureVars.dim; ++i)
    {
	VarDeclaration *v = (VarDeclaration *)closureVars.data[i];
	tree field = build_decl(FIELD_DECL,
	    v->ident ? get_identifier(v->ident->string) : NULL_TREE,
	    gen.trueDeclarationType(v));
	v->toSymbol()->SclosureField = field;
	g.ofile->setDeclLoc( field, v );
	DECL_CONTEXT(field) = closure_rec_type;
	fields.chain(field);
    }
    TYPE_FIELDS(closure_rec_type) = fields.head;
    layout_type(closure_rec_type);

    ffi->closure_rec = closure_rec_type;

    tree closure_ptr = irs->localVar(build_pointer_type(closure_rec_type));
    DECL_NAME(closure_ptr) = get_identifier("__closptr");
    DECL_ARTIFICIAL(closure_ptr) = DECL_IGNORED_P(closure_ptr) = 0;

    tree arg = d_convert_basic(Type::tsize_t->toCtype(),
	TYPE_SIZE_UNIT(closure_rec_type));

    DECL_INITIAL(closure_ptr) =
	irs->nop(irs->libCall(LIBCALL_ALLOCMEMORY, 1, & arg),
	    TREE_TYPE(closure_ptr));
    irs->expandDecl(closure_ptr);

    // set the first entry to the parent closure, if any
    tree cl = irs->closureLink();
    if (cl)
	irs->doExp(irs->vmodify(irs->component(irs->indirect(closure_ptr),
		    ptr_field),	cl));

    // copy parameters that are referenced nonlocally
    for (unsigned i = 0; i < closureVars.dim; i++)
    {
	VarDeclaration *v = (VarDeclaration *)closureVars.data[i];

	if (! v->isParameter())
	    continue;

	Symbol * vsym = v->toSymbol();
	irs->doExp(irs->vmodify(irs->component(irs->indirect(closure_ptr),
		    vsym->SclosureField), vsym->Stree));
    }

    irs->useClosure(this, closure_ptr);
}

#endif

void
Module::genobjfile(int multiobj)
{
    /* Normally would create an ObjFile here, but gcc is limited to one obj file
       per pass and there may be more than one module per obj file. */
    assert(g.ofile);

    g.ofile->beginModule(this);

    if (members) {
	for (unsigned i = 0; i < members->dim; i++) {
	    Dsymbol * dsym = (Dsymbol *) members->data[i];

	    dsym->toObjFile(multiobj);
	}
    }

    if (needModuleInfo()) {
	{
	    ModuleInfo & mi = * g.mi();

	    if (mi.ctors.dim)
		sctor = g.ofile->doFunctionToCallFunctions("*__modctor", & mi.ctors)->toSymbol();
	    if (mi.dtors.dim)
		sdtor = g.ofile->doFunctionToCallFunctions("*__moddtor", & mi.dtors)->toSymbol();
	    if (mi.unitTests.dim)
		stest = g.ofile->doFunctionToCallFunctions("*__modtest", & mi.unitTests)->toSymbol();
	}

	genmoduleinfo();
    }

    g.ofile->endModule();
}

// This is not used for GCC
unsigned Type::totym() { return 0; }

type *
Type::toCtype() {
    if (! ctype) {
	switch (ty) {
	case Tvoid: return void_type_node;
	case Tint8: return intQI_type_node;
	case Tuns8: return unsigned_intQI_type_node;
	case Tint16: return intHI_type_node;
	case Tuns16: return unsigned_intHI_type_node;
	case Tint32: return intSI_type_node;
	case Tuns32: return unsigned_intSI_type_node;
	case Tint64: return intDI_type_node;
	case Tuns64: return unsigned_intDI_type_node;
	case Tfloat32: return float_type_node;
	case Tfloat64: return double_type_node;
	case Tfloat80: return long_double_type_node;
	case Tcomplex32: return complex_float_type_node;
	case Tcomplex64: return complex_double_type_node;
	case Tcomplex80: return complex_long_double_type_node;
	case Tbool:
	    if (int_size_in_bytes( boolean_type_node ) == 1)
		return boolean_type_node;
	    // else, drop through
	case Tbit:
	    ctype = make_unsigned_type(1);
	    TREE_SET_CODE(ctype, BOOLEAN_TYPE);
	    assert(int_size_in_bytes( ctype ) == 1);
	    dkeep(ctype);
	    return ctype;
	case Tchar:
	    ctype = build_type_copy( unsigned_intQI_type_node );
	    return ctype;
	case Twchar:
	    ctype = build_type_copy( unsigned_intHI_type_node );
	    return ctype;
	case Tdchar:
	    ctype = build_type_copy( unsigned_intSI_type_node );
	    return ctype;
	case Timaginary32:
	    ctype = build_type_copy( float_type_node );
	    return ctype;
	case Timaginary64:
	    ctype = build_type_copy( double_type_node );
	    return ctype;
	case Timaginary80:
	    ctype = build_type_copy( long_double_type_node );
	    return ctype;

	case Terror: return error_mark_node;

	/* We can get Tident with forward references.  There seems to
	   be a legitame case (dstress:debug_info_03).  I have not seen this
	   happen for an error, so maybe it's okay...

	   A way to handle this would be to partially construct
	   function types and not complete it until it was actually
	   used in a call. */
	case Tident: return void_type_node;

	/* We can get Ttuple from void (T...)(T t) */
	case Ttuple: return void_type_node;

	default:
	    ::error("unexpected call to Type::toCtype() for %s\n", this->toChars());
	    abort();
	    return NULL_TREE;
	}
    }
    return ctype;
}

// This is not used for GCC
type * Type::toCParamtype() { return 0; }
// This is not used for GCC
Symbol * Type::toSymbol() { return 0; }

static void
apply_type_attributes(Expressions * attrs, tree & type_node, bool in_place = false)
{
    if (attrs)
	decl_attributes(& type_node, gen.attributes(attrs),
	    in_place ? ATTR_FLAG_TYPE_IN_PLACE : 0);
}

type *
TypeTypedef::toCtype()
{
    // %%TODO: create info for debugging
    tree type_node = sym->basetype->toCtype();
    apply_type_attributes(sym->attributes, type_node);
    return type_node;
    /*
    tree type_decl = build_decl(TYPE_DECL, get_identifier( sym->ident->string ),
	type_node);
    DECL_CONTEXT( type_decl ) =
    rest_of_decl_compilation(type_decl, NULL, ?context?, 0); //%% flag
    */
}

type *
TypeTypedef::toCParamtype()
{
    return toCtype();
}

void
TypedefDeclaration::toDebug()
{
}


type *
TypeEnum::toCtype()
{
    if (! ctype) {
	tree enum_mem_type_node = sym->memtype->toCtype();

	ctype = make_node( ENUMERAL_TYPE );
	// %% c-decl.c: if (flag_short_enums) TYPE_PACKED(enumtype) = 1;
	TYPE_PRECISION( ctype ) = size(0) * 8;
	TYPE_SIZE( ctype ) = 0; // as in c-decl.c
	TREE_TYPE( ctype ) = enum_mem_type_node;
	apply_type_attributes(sym->attributes, ctype, true);
#if V2
	/* Because minval and maxval are of this type,
	   ctype needs to be completed enough for
	   build_int_cst to work properly. */
	TYPE_MIN_VALUE( ctype ) = sym->minval->toElem(& gen);
	TYPE_MAX_VALUE( ctype ) = sym->maxval->toElem(& gen);
#else
	TYPE_MIN_VALUE( ctype ) = gen.integerConstant(sym->minval, enum_mem_type_node);
	TYPE_MAX_VALUE( ctype ) = gen.integerConstant(sym->maxval, enum_mem_type_node);
#endif
	layout_type( ctype );
	TYPE_UNSIGNED( ctype ) = isunsigned() != 0; // layout_type can change this

	// Move this to toDebug() ?
	ListMaker enum_values;
	if (sym->members) {
	    for (unsigned i = 0; i < sym->members->dim; i++) {
		EnumMember * member = (EnumMember *) sym->members->data[i];
		char * ident;

		if (sym->ident)
		    ident = concat(sym->ident->string, ".",
			member->ident->string, NULL);
		else
		    ident = (char *) member->ident->string;

		enum_values.cons( get_identifier(ident),
		    gen.integerConstant(member->value->toInteger(), ctype) );

		if (sym->ident)
		    free(ident);
	    }
	}
	TYPE_VALUES( ctype ) = enum_values.head;

	g.ofile->initTypeDecl(ctype, sym);
	g.ofile->declareType(ctype, sym);
    }
    return ctype;
}

type *
TypeStruct::toCtype()
{
    if (! ctype) {
	// need to set this right away in case of self-references
	ctype = make_node( sym->isUnionDeclaration() ? UNION_TYPE : RECORD_TYPE );

	TYPE_LANG_SPECIFIC( ctype ) = build_d_type_lang_specific(this);

	/* %% copied from AggLayout::finish -- also have to set the size
	   for (indirect) self-references. */

	/* Must set up the overall size, etc. before determining the
	   context or laying out fields as those types may make references
	   to this type. */
	TYPE_SIZE( ctype ) = bitsize_int( sym->structsize * BITS_PER_UNIT );
	TYPE_SIZE_UNIT( ctype ) = size_int( sym->structsize );
	TYPE_ALIGN( ctype ) = sym->alignsize * BITS_PER_UNIT; // %%doc int, not a tree
	// TYPE_ALIGN_UNIT is not an lvalue
	TYPE_PACKED ( ctype ) = TYPE_PACKED ( ctype ); // %% todo
	apply_type_attributes(sym->attributes, ctype, true);
	compute_record_mode ( ctype );

	// %%  stor-layout.c:finalize_type_size ... it's private to that file

	TYPE_CONTEXT( ctype ) = gen.declContext(sym);

	g.ofile->initTypeDecl(ctype, sym);

	AggLayout agg_layout(sym, ctype);
	agg_layout.go();

	/* On PowerPC 64, GCC may not always clear the padding at the end
	   of the struct. Adding 32-bit words at the end helps. */
	if (global.params.isX86_64 && ! sym->isUnionDeclaration() && sym->fields.dim)
	{
	    target_size_t ofs;
	    {
		VarDeclaration * last_decl = ((VarDeclaration*)(sym->fields.data[sym->fields.dim-1]));
		ofs = last_decl->offset + last_decl->size(0);
	    }
	    while (ofs & 3)
		++ofs;
	    while (ofs < sym->structsize && sym->structsize - ofs >= 4)
	    {
		tree f = build_decl(FIELD_DECL, get_identifier("_pad"), d_type_for_size(32, 1));
		DECL_FCONTEXT( f ) = ctype;
		DECL_ARTIFICIAL( f ) = DECL_IGNORED_P( f ) = 1;
		DECL_IGNORED_P( f ) = 1;
		agg_layout.addField(f, ofs);
		ofs += 4;
	    }
	}

	agg_layout.finish(sym->attributes);
    }
    return ctype;
}

void
StructDeclaration::toDebug()
{
    tree ctype = type->toCtype();
    g.ofile->addAggMethods(ctype, this);
    g.ofile->declareType(ctype, this);
}

Symbol * TypeClass::toSymbol() { return sym->toSymbol(); }

unsigned TypeFunction::totym() { return 0; } // Unused

type *
TypeFunction::toCtype() {
    // %%TODO: If x86, and D linkage, use regparm(1)

    if (! ctype) {
	ListMaker type_list;
	tree ret_type;

	// Function type can be reference by parameters, etc.  Set ctype early.
	ctype = make_node(FUNCTION_TYPE);

	if (varargs == 1 && linkage == LINKd) {
	    // hidden _arguments parameter
#if BREAKABI
	    type_list.cons( Type::typeinfotypelist->type->toCtype() );
#else
	    type_list.cons( Type::typeinfo->type->arrayOf()->toCtype() );
#endif
	}

	if (parameters) {
#if V2 //Until 2.037
	    size_t n_args = Argument::dim(parameters);
#else
	    size_t n_args = Parameter::dim(parameters);
#endif
	    for (size_t i = 0; i < n_args; i++) {
#if V2 //Until 2.037
		Argument * arg = Argument::getNth(parameters, i);
#else
		Parameter * arg = Parameter::getNth(parameters, i);
#endif
		type_list.cons( IRState::trueArgumentType(arg) );
	    }
	}

	/* Last parm if void indicates fixed length list (as opposed to
	   printf style va_* list). */
	if (varargs != 1)
	    type_list.cons( void_type_node );

	if (next) {
	    ret_type = next->toCtype();
	} else {
	    ret_type = void_type_node;
	}
#if V2
	if (isref)
	    ret_type = build_pointer_type(ret_type);
#endif

	TREE_TYPE( ctype ) = ret_type;
	TYPE_ARG_TYPES( ctype ) = type_list.head;
	layout_type(ctype);

	if (linkage == LINKwindows)
	    ctype = gen.addTypeAttribute(ctype, "stdcall");

#ifdef D_DMD_CALLING_CONVENTIONS
	// W.I.P.
	/* Setting this on all targets.  TARGET_RETURN_IN_MEMORY has precedence
	   over this attribute.  So, only targets on which flag_pcc_struct_return
	   is considered will be affected. */
	if ( (linkage == LINKd && next->size() <= 8) ||
	     (next && next->toBasetype()->ty == Tarray))
	    ctype = gen.addTypeAttribute(ctype, "no_pcc_struct_return");

#ifdef TARGET_386
	if (linkage == LINKd && ! TARGET_64BIT)
	    ctype = gen.addTypeAttribute(ctype, "regparm", integer_one_node);
#endif
#endif
	dkeep(ctype);
    }
    return ctype;
}

enum RET
TypeFunction::retStyle()
{
#if V2
    /* Return by reference. Needed? */
    if (isref)
	return RETregs;
#endif
    /* Need the ctype to determine this, but this is called from
       the front end before semantic processing is finished.  An
       accurate value is not currently needed anyway. */
    return RETstack;
}

type *
TypeSArray::toCtype()
{
    if (! ctype) {
	if (dim->isConst() && dim->type->isintegral()) {
	    uinteger_t size = dim->toUInteger();

#if ! V2
	    gcc_assert(! next->isbit());
#endif

	    if (next->toBasetype()->ty == Tvoid)
		ctype = gen.arrayType(Type::tuns8, size);
	    else
		ctype = gen.arrayType(next, size);

	} else {
	    ::error("invalid expressions for static array dimension: %s", dim->toChars());
	    abort();
	}
    }
    return ctype;
}

type *TypeSArray::toCParamtype() { return 0; }

type *
TypeDArray::toCtype()
{
    if (! ctype)
    {
	ctype = gen.twoFieldType(Type::tsize_t, next->pointerTo(), this,
	    "length", "ptr");
	dkeep(ctype);
    }
    return ctype;
}

type *
TypeAArray::toCtype()
{
    /* Dependencies:

       IRState::convertForCondition
       more...

    */

    if (! ctype)
    {
	/* Library functions expect a struct-of-pointer which could be passed
	   differently from a pointer. */
	static tree aa_type = NULL_TREE;
	if (! aa_type)
	{
	    aa_type = make_node( RECORD_TYPE );
	    tree f0 = build_decl(FIELD_DECL, get_identifier("ptr"), ptr_type_node);
	    DECL_CONTEXT(f0) = aa_type;
#if D_USE_MAPPED_LOCATION
	    DECL_SOURCE_LOCATION(f0) = BUILTINS_LOCATION;
#endif
	    TYPE_FIELDS(aa_type) = f0;
	    layout_type(aa_type);

	    dkeep(aa_type);
	}
	ctype = aa_type;
    }
    return ctype;
}

type *
TypePointer::toCtype()
{
    if (! ctype)
	ctype = build_pointer_type( next->toCtype() );
    return ctype;
}

type *
TypeDelegate::toCtype()
{
    if (! ctype) {
	assert(next->toBasetype()->ty == Tfunction);
	ctype = gen.twoFieldType(Type::tvoid->pointerTo(), next->pointerTo(),
	    this, "object", "func");
	dkeep(ctype);
    }
    return ctype;
}

/* Create debug information for a ClassDeclaration's inheritance tree.
   Interfaces are not included. */
static tree
binfo_for(tree tgt_binfo, ClassDeclaration * cls)
{
    tree binfo =
#if D_GCC_VER < 40
	make_tree_vec(BINFO_ELTS)
#else
	make_tree_binfo(1)
#endif
	;
    TREE_TYPE              (binfo) = TREE_TYPE( cls->type->toCtype() ); // RECORD_TYPE, not REFERENCE_TYPE
    BINFO_INHERITANCE_CHAIN(binfo) = tgt_binfo;
    BINFO_OFFSET           (binfo) = size_zero_node; // %% type?, otherwize, integer_zero_node

    if (cls->baseClass) {
#if D_GCC_VER < 40
	BINFO_BASETYPES(binfo)    = make_tree_vec(1);
	BINFO_BASETYPE(binfo, 0)  = binfo_for(binfo, cls->baseClass);
#else
	BINFO_BASE_APPEND(binfo, binfo_for(binfo, cls->baseClass));
#endif
#ifdef BINFO_BASEACCESSES
#if D_GCC_VER >= 40
#error update vector stuff
#endif
	tree prot_tree;

	BINFO_BASEACCESSES(binfo) = make_tree_vec(1);
#if V1
	BaseClass * bc = (BaseClass *) cls->baseclasses->data[0];
#else
	BaseClass * bc = (BaseClass *) cls->baseclasses.data[0];
#endif
	switch (bc->protection) {
	case PROTpublic:
	    prot_tree = access_public_node;
	    break;
	case PROTprotected:
	    prot_tree = access_protected_node;
	    break;
	case PROTprivate:
	    prot_tree = access_private_node;
	    break;
	default:
	    prot_tree = access_public_node;
	    break;
	}
	BINFO_BASEACCESS(binfo,0) = prot_tree;
#endif
    }

    return binfo;
}

/* Create debug information for an InterfaceDeclaration's inheritance
   tree.  In order to access all inherited methods in the debugger,
   the entire tree must be described.

   This function makes assumptions about inherface layout. */
static tree
intfc_binfo_for(tree tgt_binfo, ClassDeclaration * iface, unsigned & inout_offset)
{
#if V1
#define BASECLASSES_DIM  iface->baseclasses->dim
#define BASECLASSES_DATA iface->baseclasses->data
#else
#define BASECLASSES_DIM  iface->baseclasses.dim
#define BASECLASSES_DATA iface->baseclasses.data
#endif
    tree binfo =
#if D_GCC_VER < 40
	make_tree_vec(BINFO_ELTS)
#else
	make_tree_binfo(BASECLASSES_DIM)
#endif
	;
    TREE_TYPE              (binfo) = TREE_TYPE( iface->type->toCtype() ); // RECORD_TYPE, not REFERENCE_TYPE
    BINFO_INHERITANCE_CHAIN(binfo) = tgt_binfo;
    BINFO_OFFSET           (binfo) = size_int(inout_offset * PTRSIZE);

    if (BASECLASSES_DIM) {
#if D_GCC_VER < 40
	BINFO_BASETYPES(binfo)    = make_tree_vec(BASECLASSES_DIM);
#endif
#ifdef BINFO_BASEACCESSES
	BINFO_BASEACCESSES(binfo) = make_tree_vec(BASECLASSES_DIM);
#endif
    }
    for (unsigned i = 0; i < BASECLASSES_DIM; i++) {
	BaseClass * bc = (BaseClass *) BASECLASSES_DATA[i];

	if (i)
	    inout_offset++;

#if D_GCC_VER < 40
	BINFO_BASETYPE(binfo, i)  = intfc_binfo_for(binfo, bc->base, inout_offset);
#else
	BINFO_BASE_APPEND(binfo, intfc_binfo_for(binfo, bc->base, inout_offset));
#endif
#ifdef BINFO_BASEACCESSES
	tree prot_tree;
	switch ( bc->protection ) {
	case PROTpublic:
	    prot_tree = access_public_node;
	    break;
	case PROTprotected:
	    prot_tree = access_protected_node;
	    break;
	case PROTprivate:
	    prot_tree = access_private_node;
	    break;
	default:
	    prot_tree = access_public_node;
	    break;
	}
	BINFO_BASEACCESS(binfo, i) = prot_tree;
#endif
    }
#undef BASECLASSES_DIM
#undef BASECLASSES_DATA

    return binfo;
}

type *
TypeClass::toCtype()
{
    if (! ctype) {
	tree rec_type;
	Array base_class_decls;
	bool inherited = sym->baseClass != 0;
	tree obj_rec_type;
	tree vfield;

	/* Need to set ctype right away in case of self-references to
	   the type during this call. */
	rec_type = make_node( RECORD_TYPE );
	//apply_type_attributes(sym->attributes, rec_type, true);
	ctype = build_reference_type( rec_type );
	dkeep(ctype); // because BINFO moved out to toDebug
	g.ofile->initTypeDecl(rec_type, sym);

	obj_rec_type = TREE_TYPE( gen.getObjectType()->toCtype() );

	// Note that this is set on the reference type, not the record type.
	TYPE_LANG_SPECIFIC( ctype ) = build_d_type_lang_specific( this );

	AggLayout agg_layout(sym, rec_type);

	// Most of this silly code is just to produce correct debugging information.

	/* gdb apparently expects the vtable field to be named
	   "_vptr$...." (stabsread.c) Otherwise, the debugger gives
	   lots of annoying error messages.  C++ appends the class
	   name of the first base witht that field after the '$'. */
	/* update: annoying messages might not appear anymore after making
	   other changes */
	// Add the virtual table pointer
	tree decl = build_decl(FIELD_DECL, get_identifier("_vptr$"), /*vtbl_type*/d_vtbl_ptr_type_node);
	agg_layout.addField( decl, 0 ); // %% target stuff..

	if (inherited) {
	    vfield = copy_node( decl );
	    DECL_ARTIFICIAL( decl ) = DECL_IGNORED_P( decl ) = 1;
	} else {
	    vfield = decl;
	}
	DECL_VIRTUAL_P( vfield ) = 1;
	TYPE_VFIELD( rec_type ) = vfield; // This only seems to affect debug info

	if (! sym->isInterfaceDeclaration()) {
	    DECL_FCONTEXT( vfield ) = obj_rec_type;

	    // Add the monitor
	    // %% target type
	    decl = build_decl(FIELD_DECL, get_identifier("_monitor"), ptr_type_node);
	    DECL_FCONTEXT( decl ) = obj_rec_type;
	    DECL_ARTIFICIAL( decl ) = DECL_IGNORED_P( decl ) = inherited;
	    agg_layout.addField( decl, PTRSIZE);

	    // Add the fields of each base class
	    agg_layout.go();
	} else {
	    ClassDeclaration * p = sym;
#if V1
	    while (p->baseclasses->dim) {
		p = ((BaseClass *) p->baseclasses->data[0])->base;
	    }
#else
	    while (p->baseclasses.dim) {
		p = ((BaseClass *) p->baseclasses.data[0])->base;
	    }
#endif
	    DECL_FCONTEXT( vfield ) = TREE_TYPE( p->type->toCtype() );
	}

	TYPE_CONTEXT( rec_type ) = gen.declContext(sym);

	agg_layout.finish(sym->attributes);

    }
    return ctype;
}

void
ClassDeclaration::toDebug()
{
    tree rec_type = TREE_TYPE( type->toCtype() );
    /* Used to create BINFO even if debugging was off.  This was needed to keep
       references to inherited types. */

    g.ofile->addAggMethods(rec_type, this);

    if ( ! isInterfaceDeclaration() )
	TYPE_BINFO( rec_type ) = binfo_for(NULL_TREE, this);
    else {
	unsigned offset = 0;
	BaseClass bc;
	bc.base = this;
	TYPE_BINFO( rec_type ) = intfc_binfo_for(NULL_TREE, this, offset);
    }

    g.ofile->declareType(rec_type, this);
}

void
LabelStatement::toIR(IRState* irs)
{
    FuncDeclaration * func = irs->func;
    LabelDsymbol * label = irs->isReturnLabel(ident) ? func->returnLabel : func->searchLabel(ident);
    tree t_label;

    if ( (t_label = irs->getLabelTree(label)) )
    {
	irs->pushLabel(label);
	irs->doLabel(t_label);
	if (label->asmLabelNum)
	    d_expand_priv_asm_label(irs, label->asmLabelNum);
	if (irs->isReturnLabel(ident) && func->fensure)
	    func->fensure->toIR(irs);
	else if (statement)
	    statement->toIR(irs);
#if V1
	if (fwdrefs)
	{
	    irs->checkPreviousGoto(fwdrefs);
	    delete fwdrefs;
	    fwdrefs = NULL;
	}
#endif
    }
    // else, there was an error
}

void
GotoStatement::toIR(IRState* irs)
{
    tree t_label;

    g.ofile->setLoc(loc); /* This makes the 'undefined label' error show up on the correct line...
			     The extra doLineNote in doJump shouldn't cause a problem. */
    if (!label->statement)
	error("label %s is undefined", label->toChars());
    else if (tf != label->statement->tf)
	error("cannot goto forward out of or into finally block");
    else
	irs->checkGoto(this, label);

    if ( (t_label = irs->getLabelTree(label)) )
	irs->doJump(this, t_label);
    // else, there was an error
}

void
GotoCaseStatement::toIR(IRState * irs)
{
    // assumes cblocks have been set in SwitchStatement::toIR
    irs->doJump(this, cs->cblock);
}

void
GotoDefaultStatement::toIR(IRState * irs)
{
    // assumes cblocks have been set in SwitchStatement::toIR
    irs->doJump(this, sw->sdefault->cblock);
}

void
SwitchErrorStatement::toIR(IRState* irs)
{
    irs->doLineNote( loc );
    irs->doExp( irs->assertCall(loc, LIBCALL_SWITCH_ERROR) );
}

void
VolatileStatement::toIR(IRState* irs)
{
    irs->pushVolatile();
    statement->toIR( irs );
    irs->popVolatile();
}

void
ThrowStatement::toIR(IRState* irs)
{
    ClassDeclaration * class_decl = exp->type->toBasetype()->isClassHandle();
    // Front end already checks for isClassHandle
    InterfaceDeclaration * intfc_decl = class_decl->isInterfaceDeclaration();

    tree arg = exp->toElem(irs);

    if (intfc_decl) {
	if ( ! intfc_decl->isCOMclass()) {
	    arg = irs->convertTo(arg, exp->type, irs->getObjectType());
	} else {
	    error("cannot throw COM interfaces");
	}
    }

    irs->doLineNote(loc);
    irs->doExp( irs->libCall(LIBCALL_THROW, 1, & arg) );
    // %%TODO: somehow indicate flow stops here? -- set attribute noreturn on _d_throw
}

void
TryFinallyStatement::toIR(IRState * irs)
{
#if D_GCC_VER < 40
    // %% doc: this is not the same as a start_eh/end_eh_cleanup sequence
    tree t_body = body ? irs->makeStmtExpr(body) : d_void_zero_node;
    tree t_finl = finalbody ? irs->makeStmtExpr(finalbody) : d_void_zero_node;
    tree tf = build2(TRY_FINALLY_EXPR, void_type_node, t_body, t_finl);
    // TREE_SIDE_EFFECTS(tf) = 1; // probably not needed
    irs->doLineNote(loc);
    irs->beginFlow(this, NULL);
    irs->doExp(tf);
    irs->endFlow();
#else
    irs->doLineNote(loc);
    irs->startTry(this);
    if (body)
	body->toIR(irs);
    irs->startFinally();
    if (finalbody)
	finalbody->toIR(irs);
    irs->endFinally();
#endif
}

void
TryCatchStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    irs->startTry(this);
    if (body)
	body->toIR(irs);
    irs->startCatches();
    if (catches) {
	for (unsigned i = 0; i < catches->dim; i++) {
	    Catch * a_catch = (Catch *) catches->data[i];

	    irs->startCatch(a_catch->type->toCtype()); //expand_start_catch( xxx );

	    irs->doLineNote(a_catch->loc);
	    irs->startScope();

	    if ( a_catch->var ) {
		tree exc_obj = irs->convertTo(irs->exceptionObject(),
					      irs->getObjectType(), a_catch->type);
		// need to override initializer...
		// set DECL_INITIAL now and emitLocalVar will know not to change it
		DECL_INITIAL(a_catch->var->toSymbol()->Stree) = exc_obj;
		irs->emitLocalVar(a_catch->var);
	    }

	    if (a_catch->handler)
		a_catch->handler->toIR(irs);
	    irs->endScope();
	    irs->endCatch();
	}
    }
    irs->endCatches();
}

void
OnScopeStatement::toIR(IRState *)
{
    // nothing (?)
}

void
WithStatement::toIR(IRState * irs)
{
    if (wthis) {
	irs->startScope();
	irs->emitLocalVar(wthis);
    }
    body->toIR(irs);
    if (wthis) {
	irs->endScope();
    }
}

void
SynchronizedStatement::toIR(IRState * irs)
{
    if (exp) {
	InterfaceDeclaration * iface;

	irs->startBindings();
	tree decl = irs->localVar(IRState::getObjectType());

	DECL_IGNORED_P( decl ) = 1;
	tree cleanup = irs->libCall(LIBCALL_MONITOREXIT, 1, & decl);
	// assuming no conversions needed
	tree init_exp;

	assert(exp->type->toBasetype()->ty == Tclass);
	iface = ((TypeClass *) exp->type->toBasetype())->sym->isInterfaceDeclaration();
	if (iface) {
	    if (! iface->isCOMclass()) {
		init_exp = irs->convertTo(exp, irs->getObjectType());
	    } else {
		error("cannot synchronize on a COM interface");
		init_exp = error_mark_node;
	    }
	} else {
	    init_exp = exp->toElem(irs);
	}

	DECL_INITIAL(decl) = init_exp;

	irs->doLineNote(loc);

#if D_GCC_VER < 40
	irs->expandDecl(decl);
	irs->doExp( irs->libCall(LIBCALL_MONITORENTER, 1, & decl));
	expand_decl_cleanup(decl, cleanup); // nope,nope just do it diffrent ways or just jump the cleanup like below..
	if (body)
	    body->toIR( irs );
#else
	irs->expandDecl(decl);
	irs->doExp( irs->libCall(LIBCALL_MONITORENTER, 1, & decl));
	irs->startTry(this);
	if (body)
	    body->toIR( irs );
	irs->startFinally();
	irs->doExp( cleanup );
	irs->endFinally();
#endif
	irs->endBindings();
    } else {
#ifndef D_CRITSEC_SIZE
#define D_CRITSEC_SIZE 64
#endif
	static tree critsec_type = 0;

	if (! critsec_type ) {
	    critsec_type = irs->arrayType(Type::tuns8, D_CRITSEC_SIZE);
	}

	tree critsec_decl = build_decl(VAR_DECL, NULL_TREE, critsec_type);
	// name is only used to prevent ICEs
	g.ofile->giveDeclUniqueName(critsec_decl, "__critsec");
	tree critsec_ref = irs->addressOf(critsec_decl); // %% okay to use twice?
	dkeep(critsec_decl);

	TREE_STATIC(critsec_decl) = 1;
	TREE_PRIVATE(critsec_decl) = 1;
	DECL_ARTIFICIAL(critsec_decl) = 1;
	DECL_IGNORED_P(critsec_decl) = 1;

	g.ofile->rodc(critsec_decl, 1);

#if D_GCC_VER < 40
	expand_eh_region_start();
	expand_expr_stmt_value(irs->libCall(LIBCALL_CRITICALENTER, 1, & critsec_ref), 0, 1);
	if (body)
	    body->toIR( irs );
	expand_expr_stmt_value(irs->libCall(LIBCALL_CRITICALEXIT, 1, & critsec_ref), 0, 1);
	expand_eh_region_end_cleanup(irs->libCall(LIBCALL_CRITICALEXIT, 1, & critsec_ref));
#else
	irs->startTry(this);
	irs->doExp( irs->libCall(LIBCALL_CRITICALENTER, 1, & critsec_ref) );
	if (body)
	    body->toIR( irs );
	irs->startFinally();
	irs->doExp( irs->libCall(LIBCALL_CRITICALEXIT, 1, & critsec_ref) );
	irs->endFinally();
#endif
    }
}

void
ContinueStatement::toIR(IRState* irs)
{
    irs->doLineNote(loc);
    irs->continueLoop(ident);
}

void
BreakStatement::toIR(IRState* irs)
{
    irs->doLineNote(loc);
    irs->exitLoop(ident);
}

void
ReturnStatement::toIR(IRState* irs)
{
    irs->doLineNote(loc);

    if (exp) {
	if (exp->type->toBasetype()->ty != Tvoid) { // %% == Type::tvoid ?
	    FuncDeclaration * func = irs->func;
	    TypeFunction * tf = (TypeFunction *)func->type;
	    Type * ret_type = func->tintro ?
		func->tintro->nextOf() : tf->nextOf();

            if (func->isMain() && ret_type->toBasetype()->ty == Tvoid)
                ret_type = Type::tint32;

	    tree result_decl = DECL_RESULT( irs->func->toSymbol()->Stree );
	    // %% convert for init -- if we were returning a reference,
	    // would want to take the address...
#if V2
	    if (tf->isref) {
		exp = exp->addressOf(NULL);
		ret_type = ret_type->pointerTo();
	    }
#endif
	    tree result_value = irs->convertForAssignment(exp, (Type*)ret_type);
	    tree result_assign = build2(MODIFY_EXPR, TREE_TYPE(result_decl),
					result_decl, result_value);

	    irs->doReturn(result_assign); // expand_return(result_assign);
	} else {
	    //irs->doExp(exp);
	    irs->doReturn(NULL_TREE);
	}
    } else {
	irs->doReturn(NULL_TREE);
    }
}

void
DefaultStatement::toIR(IRState * irs)
{
    irs->checkSwitchCase(this, 1);
    irs->doCase(NULL_TREE, cblock);
    if (statement)
	statement->toIR( irs );
}

void
CaseStatement::toIR(IRState * irs)
{
    tree case_value;

    if ( exp->type->isscalar() )
	case_value = exp->toElem(irs);
    else
	case_value = irs->integerConstant(index, Type::tint32);

    irs->checkSwitchCase(this);
    irs->doCase(case_value, cblock);
    if (statement)
	statement->toIR( irs );
}

void
SwitchStatement::toIR(IRState * irs)
{
    tree cond_tree;
    // %% also what about c-semantics doing emit_nop() ?
    irs->doLineNote( loc );

    cond_tree = condition->toElem( irs );

    Type * cond_type = condition->type->toBasetype();
    if (cond_type->ty == Tarray) {
	Type * elem_type = cond_type->nextOf()->toBasetype();
	LibCall lib_call;
	switch (elem_type->ty) {
	case Tchar:  lib_call = LIBCALL_SWITCH_STRING; break;
	case Twchar: lib_call = LIBCALL_SWITCH_USTRING; break;
	case Tdchar: lib_call = LIBCALL_SWITCH_DSTRING; break;
	default:
	    ::error("switch statement value must be an array of some character type, not %s", elem_type->toChars());
	    abort();
	}

	// Apparently the backend is supposed to sort and set the indexes
	// on the case array
	// have to change them to be useable
	cases->sort(); // %%!!

	Symbol * s = static_sym();
	dt_t **  pdt = & s->Sdt;
	s->Sseg = CDATA;
	for (unsigned case_i = 0; case_i < cases->dim; case_i++) {
	    CaseStatement * case_stmt = (CaseStatement *) cases->data[case_i];
	    pdt = case_stmt->exp->toDt( pdt );
	    case_stmt->index = case_i;
	}
	outdata(s);
	tree p_table = irs->addressOf(s->Stree);

	tree args[2] = {
	    irs->darrayVal(cond_type->arrayOf()->toCtype(), cases->dim,
		p_table),
	    cond_tree };

	cond_tree = irs->libCall(lib_call, 2, args);
    } else if (! cond_type->isscalar()) {
	::error("cannot handle switch condition of type %s", cond_type->toChars());
	abort();
    }

    // Build LABEL_DECLs now so they can be refered to by goto case
    if (cases) {
	for (unsigned i = 0; i < cases->dim; i++) {
	    CaseStatement * case_stmt = (CaseStatement *) cases->data[i];
	    case_stmt->cblock = irs->label(case_stmt->loc); //make_case_label(case_stmt->loc);
	}
	if (sdefault)
	    sdefault->cblock = irs->label(sdefault->loc); //make_case_label(sdefault->loc);
    }

    cond_tree = fold(cond_tree);
    irs->startCase(this, cond_tree);
    if (body)
	body->toIR( irs );
    irs->endCase(cond_tree);
}


void
Statement::toIR(IRState*)
{
    ::error("Statement::toIR: don't know what to do (%s)", toChars());
    abort();
}

void
IfStatement::toIR(IRState * irs)
{
    if (match) {
	irs->startScope();
	irs->emitLocalVar(match);
    }
    irs->startCond(this, condition);
    if (ifbody)
	ifbody->toIR( irs );
    if ( elsebody ) {
	irs->startElse();
	elsebody->toIR ( irs );
    }
    irs->endCond();
    if (match)
	irs->endScope();
}

void
ForeachStatement::toIR(IRState* irs)
{
    // %% better?: set iter to start - 1 and use result of increment for condition?

    // side effects?

    Type * agg_type = aggr->type->toBasetype();
    Type * elem_type = agg_type->nextOf()->toBasetype();
    tree iter_decl;
    tree bound_expr;
    tree iter_init_expr;
    tree aggr_expr = irs->maybeMakeTemp( aggr->toElem(irs) );

    assert(value);

    gcc_assert(elem_type->ty != Tbit);

    irs->startScope();
    irs->startBindings(); /* Variables created by the function will probably
			     end up in a contour created by emitLocalVar.  This
			     startBindings call is just to be safe */
    irs->doLineNote( loc );

    Loc default_loc;
    if (loc.filename)
	default_loc = loc;
    else {
	fprintf(stderr, "EXPER: I need this\n");
	default_loc = Loc(g.mod, 1); // %% fix
    }

    if (! value->loc.filename)
	g.ofile->setDeclLoc( value->toSymbol()->Stree, default_loc );

    irs->emitLocalVar(value, true);

    if (key) {
	if (! key->loc.filename)
	    g.ofile->setDeclLoc( key->toSymbol()->Stree, default_loc );
	if (! key->init)
	    DECL_INITIAL( key->toSymbol()->Stree ) = op == TOKforeach ?
		irs->integerConstant(0, key->type) :
		irs->arrayLength(aggr_expr, agg_type);

	irs->emitLocalVar(key); // %% getExpInitializer causes uneeded initialization
    }

    bool iter_is_value;
    if (value->isRef() || value->isOut()) {
	iter_is_value = true;
	iter_decl = irs->var(value);
    } else {
	iter_is_value = false;
	iter_decl = irs->localVar(elem_type->pointerTo());
	irs->expandDecl(iter_decl);
    }

    if ( agg_type->ty == Tsarray) {
	bound_expr = ((TypeSArray *) agg_type)->dim->toElem(irs);
	iter_init_expr = irs->addressOf( aggr_expr );
	// Type needs to be pointer-to-element to get pointerIntSum
	// to work
	iter_init_expr = irs->nop(iter_init_expr,
	    agg_type->nextOf()->pointerTo()->toCtype());
    } else {
	bound_expr = irs->darrayLenRef( aggr_expr );
	iter_init_expr = irs->darrayPtrRef( aggr_expr );
    }
    iter_init_expr = save_expr( iter_init_expr );
    bound_expr = irs->pointerIntSum(iter_init_expr, bound_expr);
    // aggr. isn't supposed to be modified, so...
    bound_expr = save_expr( bound_expr );

    enum tree_code iter_op = PLUS_EXPR;

    if (op == TOKforeach_reverse)
    {
	tree t = iter_init_expr;
	iter_init_expr = bound_expr;
	bound_expr = t;

	iter_op = MINUS_EXPR;
    }

    tree condition = build2(NE_EXPR, boolean_type_node, iter_decl, bound_expr);
    tree incr_expr;
#if D_GCC_VER >= 43
    incr_expr = irs->pointerOffsetOp(iter_op, iter_decl,
	size_int(elem_type->size()));
#else
    incr_expr = build2(iter_op, TREE_TYPE(iter_decl), iter_decl,
	size_int(elem_type->size()));
#endif
    incr_expr = irs->vmodify(iter_decl, incr_expr);

    if (key) {
	tree key_decl = irs->var(key);
	tree key_incr_expr =
	    build2(MODIFY_EXPR, void_type_node, key_decl,
		build2(iter_op, TREE_TYPE(key_decl), key_decl,
		    irs->integerConstant(1, TREE_TYPE(key_decl))));
	incr_expr = irs->compound(incr_expr, key_incr_expr);
    }

    irs->doExp( build2(MODIFY_EXPR, void_type_node, iter_decl, iter_init_expr) );

    irs->startLoop(this);
    irs->exitIfFalse(condition);
    if ( op == TOKforeach_reverse )
	irs->doExp( incr_expr );
    if ( ! iter_is_value )
	irs->doExp( build2(MODIFY_EXPR, void_type_node, irs->var(value),
			irs->indirect(iter_decl)) );
    if (body)
	body->toIR( irs );
    irs->continueHere();

    if ( op == TOKforeach )
	irs->doExp( incr_expr );

    irs->endLoop();

    irs->endBindings(); // not really needed
    irs->endScope();
}

#if V2

void
ForeachRangeStatement::toIR(IRState * irs)
{
    bool fwd = op == TOKforeach;
    Type * key_type = key->type->toBasetype();

    irs->startScope();
    irs->startBindings(); /* Variables created by the function will probably
			     end up in a contour created by emitLocalVar.  This
			     startBindings call is just to be safe */
    irs->doLineNote( loc );

    gcc_assert(key != NULL);
    gcc_assert(lwr != NULL);
    gcc_assert(upr != NULL);

    // Front end ensures no storage class
    irs->emitLocalVar(key, true);
    tree key_decl = irs->var(key);
    tree lwr_decl = irs->localVar(lwr->type);
    tree upr_decl = irs->localVar(upr->type);
    tree iter_expr;
    tree condition;

#if D_GCC_VER >= 43
    if ( POINTER_TYPE_P (TREE_TYPE (key_decl)) )
    {
    	iter_expr = irs->vmodify(key_decl,
	    irs->pointerOffsetOp(fwd ? PLUS_EXPR : MINUS_EXPR,
		key_decl, size_int(key_type->nextOf()->size())));
    }
    else
#endif
    iter_expr = irs->vmodify(key_decl,
	build2(fwd ? PLUS_EXPR : MINUS_EXPR, TREE_TYPE(key_decl),
	    key_decl, irs->integerConstant(1, TREE_TYPE(key_decl))));

    irs->expandDecl(lwr_decl);
    irs->expandDecl(upr_decl);
    irs->doExp( irs->vmodify(lwr_decl, lwr->toElem(irs)) );
    irs->doExp( irs->vmodify(upr_decl, upr->toElem(irs)) );

    condition = build2(fwd ? LT_EXPR : GT_EXPR, boolean_type_node,
	key_decl, fwd ? upr_decl : lwr_decl);

    irs->doExp( irs->vmodify(key_decl, fwd ? lwr_decl : upr_decl) );

    irs->startLoop(this);
    if (! fwd)
	irs->continueHere();
    irs->exitIfFalse(condition);
    if (! fwd)
	irs->doExp(iter_expr);
    if (body)
	body->toIR( irs );
    if ( fwd ) {
	irs->continueHere();
	irs->doExp(iter_expr);
    }
    irs->endLoop();

    irs->endBindings(); // not really needed
    irs->endScope();
}

#endif

void
ForStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    // %% scope
    if (init)
	init->toIR( irs );
    irs->startLoop(this);
    if (condition)
	irs->exitIfFalse(condition);
    if (body)
	body->toIR( irs );
    irs->continueHere();
    if (increment)
	irs->doExp(increment->toElem(irs)); // force side effects?
    irs->endLoop();
}

void
DoStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    irs->startLoop(this);
    if (body)
	body->toIR( irs );
    irs->continueHere();
    irs->exitIfFalse(condition);
    irs->endLoop();
}

void
WhileStatement::toIR(IRState* irs)
{
    irs->doLineNote(loc); // store for next statement...
    irs->startLoop(this);
    irs->exitIfFalse(condition, 1); // 1 == is topcond .. good as deprecated..
    if (body)
	body->toIR( irs );
    irs->continueHere();
    irs->endLoop();
}

void
ScopeStatement::toIR(IRState* irs)
{
    if (statement) {
	irs->startScope();
	statement->toIR( irs );
	irs->endScope();
    }
}

void
CompoundStatement::toIR(IRState* irs)
{
    if (statements) {
	for (unsigned i = 0; i < statements->dim; i++) {
	    Statement * statement = (Statement *) statements->data[i];

	    if (statement)
		statement->toIR(irs);
	}
    }
}

void
UnrolledLoopStatement::toIR(IRState* irs)
{
    if (statements) {
	irs->startLoop(this);
	irs->continueHere();
	for (unsigned i = 0; i < statements->dim; i++) {
	    Statement * statement = (Statement *) statements->data[i];

	    if (statement)
	    {
		irs->setContinueLabel( irs->label(loc) );
		statement->toIR(irs);
		irs->continueHere();
	    }
	}
	irs->exitLoop(NULL);
	irs->endLoop();
    }
}

void
ExpStatement::toIR(IRState * irs)
{
    if (exp) {
	gen.doLineNote(loc);

	tree exp_tree = exp->toElem(irs);

	irs->doExp(exp_tree);
    } else {
	// nothing
    }
}

#if V2
void
PragmaStatement::toIR(IRState *)
{
    // nothing
}
#endif

void
EnumDeclaration::toDebug()
{

}

int
Dsymbol::cvMember(unsigned char*)
{
    return 0;
}
int
EnumDeclaration::cvMember(unsigned char*)
{
    return 0;
}
int
FuncDeclaration::cvMember(unsigned char*)
{
    return 0;
}
int
VarDeclaration::cvMember(unsigned char*)
{
    return 0;
}
int
TypedefDeclaration::cvMember(unsigned char*)
{
    return 0;
}

#if D_GCC_VER < 40

rtx
d_expand_expr(tree exp, rtx target , enum machine_mode tmode, int modifier, rtx *)
{
    if ( TREE_CODE(exp) == (enum tree_code) D_STMT_EXPR ) {
	IRState * irs;
	Statement * stmt;

	gen.retrieveStmtExpr(exp, & stmt, & irs);
	// need push_temp_slots()?

	tree rtl_expr = expand_start_stmt_expr(1);
	// This startBindings call is needed so get_last_insn() doesn't return NULL
	// in expand_start_case().
	irs->startBindings();
	// preserve_temp_slots as in c-common.c:c_expand_expr

	stmt->toIR(irs);
	irs->endBindings();

	expand_end_stmt_expr (rtl_expr);

	rtx result = expand_expr (rtl_expr, target, tmode, (enum expand_modifier) modifier);
	pop_temp_slots();
	return result;
    } else if ( TREE_CODE(exp) == (enum tree_code) D_ARRAY_SET_EXPR ){
	// %% if single byte element, expand to memset

	assert( POINTER_TYPE_P( TREE_TYPE( TREE_OPERAND( exp, 0 ))));
	assert( INTEGRAL_TYPE_P( TREE_TYPE( TREE_OPERAND( exp, 2 ))));
	// assuming unsigned source is unsigned

	push_temp_slots (); // will this work? maybe expand_start_binding
	tree rtl_expr = expand_start_stmt_expr(1);

	do_array_set(g.irs,   // %% fix!
	    TREE_OPERAND(exp, 0), TREE_OPERAND(exp, 1), TREE_OPERAND(exp, 2));

	expand_end_stmt_expr(rtl_expr);
	rtx result = expand_expr(rtl_expr, target, tmode, (enum expand_modifier) modifier);
	pop_temp_slots ();
	return result;
    } else {
	abort();
    }
}

#endif


static tree
d_build_eh_type_type(tree type)
{
    TypeClass * d_type = (TypeClass *) IRState::getDType(type);
    assert(d_type);
    d_type = (TypeClass *) d_type->toBasetype();
    assert(d_type->ty == Tclass);
    return IRState::addressOf( d_type->sym->toSymbol()->Stree );
}

tree d_void_zero_node;

tree d_null_pointer;
tree d_vtbl_ptr_type_node;

void
gcc_d_backend_init()
{
    // %% need this here to add the type decls ...
    init_global_binding_level();

    // This allows the code in d-builtins2 to not have to worry about
    // converting (C signed char *) to (D char *) for string arguments of
    // built-in functions.
    flag_signed_char = 0;
    // This is required or we'll crash pretty early on. %%log
#if D_GCC_VER >= 40
    build_common_tree_nodes (flag_signed_char, false);
#else
    build_common_tree_nodes (flag_signed_char);
#endif

    // This is also required (or the manual equivalent) or crashes
    // will occur later
    size_type_node = d_type_for_mode(ptr_mode, 1);

    // c was: TREE_TYPE (identifier_global_value (get_identifier (SIZE_TYPE)));
    //signed_size_type_node = c_common_signed_type (size_type_node);

    // If this is called after the next statements, you'll get an ICE.
    set_sizetype(size_type_node);


    // need this for void.. %% but this crashes... probably need to impl
    // some things in dc-lang.cc
    build_common_tree_nodes_2 (0 /* %% support the option */);

    // Specific to D (but so far all taken from C)
#if D_GCC_VER < 40
    d_void_zero_node = build_int_2 (0, 0);
    TREE_TYPE (d_void_zero_node) = void_type_node;
#else
    d_void_zero_node = make_node(INTEGER_CST);
    TREE_TYPE(d_void_zero_node) = void_type_node;
#endif
    // %%TODO: we are relying on default boolean_type_node being 8bit / same as Tbit

    d_null_pointer = convert(ptr_type_node, integer_zero_node);

    TYPE_NAME( integer_type_node ) = build_decl(TYPE_DECL, get_identifier("int"), integer_type_node);
    TYPE_NAME( char_type_node ) = build_decl(TYPE_DECL, get_identifier("char"), char_type_node);

    REALSIZE = int_size_in_bytes(long_double_type_node);
    REALPAD = 0;
    PTRSIZE = int_size_in_bytes(ptr_type_node);
    switch (int_size_in_bytes(size_type_node)) {
    case 4:
	Tsize_t = Tuns32;
	Tindex = Tint32;
	break;
    case 8:
	Tsize_t = Tuns64;
	Tindex = Tint64;
	break;
    default:
	abort();
    }
    switch ( PTRSIZE ) {
    case 4:
	assert(POINTER_SIZE == 32);
	Tptrdiff_t = Tint32;
	break;
    case 8:
	assert(POINTER_SIZE == 64);
	Tptrdiff_t = Tint64;
	break;
    default:
	abort();
    }

    CLASSINFO_SIZE = 19 * PTRSIZE;

    d_init_builtins();

    if (flag_exceptions) {
	eh_personality_libfunc = init_one_libfunc(d_using_sjlj_exceptions()
	    ? "__gdc_personality_sj0" : "__gdc_personality_v0");
#if D_GCC_VER >= 41
	default_init_unwind_resume_libfunc ();
#endif
	lang_eh_runtime_type = d_build_eh_type_type;
	using_eh_for_cleanups ();
	// lang_proctect_cleanup_actions = ...; // no need? ... probably needed for autos
    }

    /* copied and modified from cp/decl.c; only way for vtable to work in gdb... */
    // or not, I'm feeling very confused...
    {
	/* Make sure we get a unique function type, so we can give
	   its pointer type a name.  (This wins for gdb.) */
	tree vfunc_type = make_node (FUNCTION_TYPE);
	TREE_TYPE (vfunc_type) = Type::tint32->toCtype(); // integer_type_node; messed up built in types?
	TYPE_ARG_TYPES (vfunc_type) = NULL_TREE;
	layout_type (vfunc_type);

	tree vtable_entry_type = build_pointer_type (vfunc_type);
	d_vtbl_ptr_type_node = build_pointer_type(vtable_entry_type);
	layout_type (d_vtbl_ptr_type_node);// %%TODO: check if needed
    }

    // This also allows virtual functions to be called, but when vtbl entries,
    // are inspected, function symbol names do not appear.
    // d_vtbl_ptr_type_node = Type::tvoid->pointerTo()->pointerTo()->toCtype();

    // This is the C main, not the D main
    main_identifier_node = get_identifier ("main");
}

void
gcc_d_backend_term()
{
}
