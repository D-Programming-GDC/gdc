/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Iain Buclaw, Michael Parrott, (C) 2010

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
#include "d-codegen.h"
#include <math.h>
#include <limits.h>
#include "total.h"
#include "template.h"
#include "init.h"
#include "symbol.h"
#include "dt.h"

GlobalValues g;
IRState gen;
Array IRState::stmtExprList;
TemplateEmission IRState::emitTemplates;
bool IRState::splitDynArrayVarArgs;
bool IRState::useBuiltins;
bool IRState::warnSignCompare = false;
bool IRState::originalOmitFramePointer;

bool
d_gcc_force_templates()
{
    return IRState::emitTemplates == TEprivate ||
	IRState::emitTemplates == TEall;
}

void
IRState::emitLocalVar(VarDeclaration * v, bool no_init)
{
    if (v->isDataseg() || v->isMember())
	return;

    Symbol * sym = v->toSymbol();
    tree var_decl = sym->Stree;

    gcc_assert(! TREE_STATIC( var_decl ));
    if (TREE_CODE(var_decl) == CONST_DECL)
	return;

    DECL_CONTEXT( var_decl ) = getLocalContext();

    tree var_exp;
#if V2
    if (sym->SclosureField)
	var_exp = var(v);
    else
#endif
    {
	var_exp = var_decl;
	pushdecl(var_decl);
#if D_GCC_VER < 40
	expand_decl(var_decl);
#endif
    }

    tree init_exp = NULL_TREE; // complete initializer expression (include MODIFY_EXPR, e.g.)
    tree init_val = NULL_TREE;

    if (! no_init && ! DECL_INITIAL( var_decl ) && v->init ) {
	if (! v->init->isVoidInitializer()) {
	    ExpInitializer * exp_init = v->init->isExpInitializer();
	    Expression * ie = exp_init->toExpression();
	    if ( ! (init_val = assignValue(ie, v)) )
		init_exp = ie->toElem(this);
	}
	else
	    no_init = true;
    }

    if (! no_init) {
	g.ofile->doLineNote(v->loc);

	if (! init_val)
	    init_val = DECL_INITIAL(var_decl);
#if D_GCC_VER < 40
	if (init_val
#if V2
	    && ! sym->SclosureField
#endif
	    )
	{
	    // not sure if there is any advantage to doing this...
	    DECL_INITIAL(var_decl) = init_val;
	    expand_decl_init(var_decl);
	}
	else
#endif
	if (! init_exp && init_val)
	    init_exp = vmodify(var_exp, init_val);

	if (init_exp)
	    addExp(init_exp);
	else if (! init_val && v->size(v->loc)) // Zero-length arrays do not have an initializer
	    d_warning(OPT_Wuninitialized, "uninitialized variable '%s'", v->ident ? v->ident->string : "(no name)");
    }
}

tree
IRState::localVar(tree t_type) {
    tree t_decl = build_decl(VAR_DECL, NULL_TREE, t_type);
    DECL_CONTEXT( t_decl ) = getLocalContext();
    DECL_ARTIFICIAL( t_decl ) = 1;
    DECL_IGNORED_P( t_decl ) = 1;
    pushdecl(t_decl);
    return t_decl;
}

tree
IRState::exprVar(tree t_type)
{
    tree t_decl = build_decl(VAR_DECL, NULL_TREE, t_type);
    DECL_CONTEXT( t_decl ) = getLocalContext();
    DECL_ARTIFICIAL( t_decl ) = 1;
    DECL_IGNORED_P( t_decl ) = 1;
    return t_decl;
}

static bool
needs_expr_var(tree exp)
{
    switch (TREE_CODE(exp)) {
    case VAR_DECL:
    case FUNCTION_DECL:
    case PARM_DECL:
    case CONST_DECL:
    case INDIRECT_REF:
    case ARRAY_REF:
	return false;
    case COMPONENT_REF:
	return needs_expr_var(TREE_OPERAND(exp,0));
    default:
	return true;
    }
}

tree
IRState::maybeExprVar(tree exp, tree * out_var)
{
    if ( needs_expr_var(exp) ) {
	*out_var = exprVar(TREE_TYPE(exp));
	DECL_INITIAL( *out_var ) = exp;
	return *out_var;
    } else {
	*out_var = NULL_TREE;
	return exp;
    }
}

tree
IRState::declContext(Dsymbol * d_sym)
{
    Dsymbol * orig_sym = d_sym;
    AggregateDeclaration * agg_decl;

    while ( (d_sym = d_sym->toParent2()) ) {
	if (d_sym->isFuncDeclaration()) {
	    // 3.3.x (others?) dwarf2out chokes without this check... (output_pubnames)
	    FuncDeclaration * f = orig_sym->isFuncDeclaration();
	    if (f && ! gen.functionNeedsChain(f))
		return 0;
	    return d_sym->toSymbol()->Stree;
	} else if ( (agg_decl = d_sym->isAggregateDeclaration()) ) {
	    ClassDeclaration * cd;

	    tree ctx = agg_decl->type->toCtype();
	    if ( (cd = d_sym->isClassDeclaration()) )
		ctx = TREE_TYPE(ctx); // RECORD_TYPE instead of REFERENCE_TYPE
	    return ctx;
	} else if ( d_sym->isModule() ) {
	    return d_sym->toSymbol()->ScontextDecl;
	}
    }
    return NULL_TREE;
}

void
IRState::expandDecl(tree t_decl)
{
#if D_GCC_VER < 40
    expand_decl(t_decl);
    if (DECL_INITIAL(t_decl))
        expand_decl_init(t_decl);
#else
    // nothing, pushdecl will add t_decl to a BIND_EXPR
    if (DECL_INITIAL(t_decl)) {
        // void_type_node%%?
        doExp(build2(MODIFY_EXPR, void_type_node, t_decl, DECL_INITIAL(t_decl)));
        DECL_INITIAL(t_decl) = NULL_TREE;
    }
#endif
}

#if V2
tree
IRState::var(VarDeclaration * v)
{
    bool is_closure_var = v->toSymbol()->SclosureField != NULL;

    if (is_closure_var)
    {
	FuncDeclaration * f = v->toParent2()->isFuncDeclaration();

	tree cf = getClosureRef(f);
	tree field = v->toSymbol()->SclosureField;
	gcc_assert(field != NULL_TREE);
	return component(indirect(cf), field);
    }
    else
	// Static var or auto var that the back end will handle for us
	return v->toSymbol()->Stree;
}
#endif


tree
IRState::convertTo(Expression * exp, Type * target_type)
{
    return convertTo(exp->toElem( this ), exp->type, target_type);
}

tree
IRState::convertTo(tree exp, Type * exp_type, Type * target_type)
{
    tree result = NULL_TREE;
    Type * orig_exp_type = exp_type;

    assert(exp_type && target_type);
    target_type = target_type->toBasetype();
    exp_type = exp_type->toBasetype();

    if (typesSame(exp_type, target_type) &&
	typesSame(orig_exp_type, target_type))
	return exp;

    switch (exp_type->ty) {
    case Tdelegate:
	// %%TODO:NOP/VIEW_CONVERT
	if (target_type->ty == Tdelegate) {
	    exp = maybeMakeTemp(exp);
	    return delegateVal( delegateMethodRef(exp), delegateObjectRef(exp),
		target_type);
	} else if (target_type->ty == Tpointer) {
	    // The front-end converts <delegate>.ptr to cast(void*)<delegate>.
	    // Maybe should only allow void*?
	    exp = delegateObjectRef(exp);
	} else {
	    ::error("can't convert a delegate expression to %s", target_type->toChars());
	    return error_mark_node;
	}
	break;
    case Tclass:
	if (target_type->ty == Tclass) {
	    ClassDeclaration * target_class_decl = ((TypeClass *) target_type)->sym;
	    ClassDeclaration * obj_class_decl = ((TypeClass *) exp_type)->sym;
	    bool use_dynamic = false;

	    target_ptrdiff_t offset;
	    if (target_class_decl->isBaseOf(obj_class_decl, & offset)) {
		// Casting up the inheritance tree: Don't do anything special.
		// Cast to an implemented interface: Handle at compile time.
		if (offset == OFFSET_RUNTIME) {
		    use_dynamic = true;
		} else if (offset) {
		    tree t = target_type->toCtype();
		    exp = maybeMakeTemp(exp);
		    return build3(COND_EXPR, t,
			boolOp(NE_EXPR, exp, nullPointer()),
			nop(pointerOffset(exp, size_int(offset)), t),
			nop(nullPointer(), t));
		} else {
		    // d_convert will make a NOP cast
		    break;
		}
	    } else if ( target_class_decl == obj_class_decl ) {
		// d_convert will make a NOP cast
		break;
	    } else if ( ! obj_class_decl->isCOMclass() ) {
		use_dynamic = true;
	    }

	    if (use_dynamic) {
		// Otherwise, do dynamic cast
		tree args[2] = { exp, addressOf( target_class_decl->toSymbol()->Stree ) }; // ##v (and why not just addresOf(target_class_decl)
		return libCall(obj_class_decl->isInterfaceDeclaration()
		    ? LIBCALL_INTERFACE_CAST : LIBCALL_DYNAMIC_CAST, 2, args);
	    } else {
		d_warning(0, "cast to %s will yield null result", target_type->toChars());
		result = convert(target_type->toCtype(), d_null_pointer);
		if (TREE_SIDE_EFFECTS( exp )) { // make sure the expression is still evaluated if necessary
		    result = compound(exp, result);
		}
		return result;
	    }
	} else {
	    // nothing; default
	}
	break;
    case Tsarray:
	{
	    if (target_type->ty == Tpointer) {
		result = nop( addressOf( exp ), target_type->toCtype() );
	    } else if (target_type->ty == Tarray) {
		TypeSArray * a_type = (TypeSArray*) exp_type;

		uinteger_t array_len = a_type->dim->toUInteger();
		d_uns64 sz_a = a_type->next->size();
		d_uns64 sz_b = target_type->nextOf()->size();

		// conversions to different sizes
		// Assumes tvoid->size() == 1
		// %% TODO: handle misalign like _d_arraycast_xxx ?
#if ! V2
		gcc_assert(a_type->next->isbit() == target_type->next->isbit());
#endif

		if (sz_a != sz_b)
		    array_len = array_len * sz_a / sz_b;

		tree pointer_value = nop( addressOf( exp ),
		    target_type->nextOf()->pointerTo()->toCtype() );

		// Assumes casting to dynamic array of same type or void
		return darrayVal(target_type, array_len, pointer_value);
	    } else if (target_type->ty == Tsarray) {
		// DMD apparently allows casting a static array to any static array type
		return indirect(addressOf(exp), target_type->toCtype());
	    }
	    // %% else error?
	}
	break;
    case Tarray:
	if (target_type->ty == Tpointer) {
	    return convert(target_type->toCtype(), darrayPtrRef( exp ));
	} else if (target_type->ty == Tarray) {
	    // assume tvoid->size() == 1

	    Type * src_elem_type = exp_type->nextOf()->toBasetype();
	    Type * dst_elem_type = target_type->nextOf()->toBasetype();
	    d_uns64 sz_a = src_elem_type->size();
	    d_uns64 sz_b = dst_elem_type->size();

	    gcc_assert((src_elem_type->ty == Tbit) ==
		(dst_elem_type->ty == Tbit));

	    if (sz_a  != sz_b) {
		unsigned mult = 1;
#if ! V2
		if (dst_elem_type->isbit())
		    mult = 8;
#endif
		tree args[3] = {
		    // assumes Type::tbit->size() == 1
		    integerConstant(sz_b, Type::tsize_t),
		    integerConstant(sz_a * mult,
			Type::tsize_t),
		    exp
		};
		return libCall(LIBCALL_ARRAYCAST, 3, args, target_type->toCtype());
	    } else {
		// %% VIEW_CONVERT_EXPR or NOP_EXPR ? (and the two cases below)
		// Convert to/from void[] or elements are the same size -- don't change length
		return build1(NOP_EXPR, target_type->toCtype(), exp);
	    }
	}
	// else, default conversion, which should produce an error
	break;
    case Taarray:
	if (target_type->ty == Taarray)
	    return build1(NOP_EXPR, target_type->toCtype(), exp);
	// else, default conversion, which should product an error
	break;
    case Tpointer:
	/* For some reason, convert_to_integer converts pointers
	   to a signed type. */
	if (target_type->isintegral())
	    exp = d_convert_basic(d_type_for_size(POINTER_SIZE, 1), exp);
	break;
    default:
	if (( exp_type->isreal() && target_type->isimaginary() )
	    || ( exp_type->isimaginary() && target_type->isreal() )) {
	    // warn? handle in front end?

	    result = build_real_from_int_cst( target_type->toCtype(), integer_zero_node );
	    if (TREE_SIDE_EFFECTS( exp ))
		result = compound(exp, result);
	    return result;
	} else if (exp_type->iscomplex()) {
	    Type * part_type;
	    // creal.re, .im implemented by cast to real or ireal
	    // Assumes target type is the same size as the original's components size
	    if (target_type->isreal()) {
		// maybe install lang_specific...
		switch (exp_type->ty) {
		case Tcomplex32: part_type = Type::tfloat32; break;
		case Tcomplex64: part_type = Type::tfloat64; break;
		case Tcomplex80: part_type = Type::tfloat80; break;
		default:
		    abort();
		}
		result = realPart(exp);
	    } else if (target_type->isimaginary()) {
		switch (exp_type->ty) {
		case Tcomplex32: part_type = Type::timaginary32; break;
		case Tcomplex64: part_type = Type::timaginary64; break;
		case Tcomplex80: part_type = Type::timaginary80; break;
		default:
		    abort();
		}
		result = imagPart(exp);
	    } else {
		// default conversion
		break;
	    }
	    result = convertTo(result, part_type, target_type);
	} else if (target_type->iscomplex()) {
	    tree c1, c2, t;
	    c1 = convert(TREE_TYPE(target_type->toCtype()), exp);
	    c2 = build_real_from_int_cst( TREE_TYPE(target_type->toCtype()), integer_zero_node);

	    if (exp_type->isreal()) {
		// nothing
	    } else if (exp_type->isimaginary()) {
		t = c1;
		c1 = c2;
		c2 = t;
	    } else {
		// default conversion
		break;
	    }
	    result = build2(COMPLEX_EXPR, target_type->toCtype(), c1, c2);
	} else {
	    assert( TREE_CODE( exp ) != STRING_CST );
	    // default conversion
	}
    }

    if (! result)
	result = d_convert_basic(target_type->toCtype(), exp);
    return result;
}

tree
#if V2 //Until 2.037
IRState::convertForArgument(Expression * exp, Argument * arg)
#else
IRState::convertForArgument(Expression * exp, Parameter * arg)
#endif
{
    if ( isArgumentReferenceType(arg) ) {
	tree exp_tree = this->toElemLvalue(exp);
	// front-end already sometimes automatically takes the address
	// TODO: Make this safer?  Can this be confused by a non-zero SymOff?
	if (exp->op != TOKaddress && exp->op != TOKsymoff && exp->op != TOKadd)
	    return addressOf( exp_tree );
	else
	    return exp_tree;
    } else {
	// Lazy arguments: exp should already be a delegate
	/*
	Type * et = exp->type->toBasetype();
	Type * at = arg->type->toBasetype();
	if (et != at) {
	    if ((et->ty == Taarray && at == Type::tvoid->arrayOf()) ||
		(et->ty == Tarray && at == Type::tvoid->arrayOf()) ||
		(et->ty == Tdelegate && at->ty == Tdelegate) ||
		(et->ty == Tclass && at->ty == Tpointer) ||
		(et->ty == Tpointer && at->ty == Tpointer)
		) {
	    } else {
		g.ofile->setLoc(exp->loc);
		::warning("ackthpbpt: must convert %s to %s\n",
		    exp->type->toChars(), arg->type->toChars());
	    }
	}
	else
	*/
	return exp->toElem(this);
    }
}

// Apply semantics of assignment to a values of type <target_type> to <exp>
// (e.g., pointer = array -> pointer = & array[0])

// Expects base type to be passed
static Type *
final_sa_elem_type(Type * type)
{
    while (type->ty == Tsarray) {
	type = type->nextOf()->toBasetype();
    }
    return type;
}

tree
IRState::convertForAssignment(Expression * exp, Type * target_type)
{
    Type * exp_base_type = exp->type->toBasetype();
    Type * target_base_type = target_type->toBasetype();

    // Assuming this only has to handle converting a non Tsarray type to
    // arbitrarily dimensioned Tsarrays.
    if (target_base_type->ty == Tsarray &&
	typesCompatible(final_sa_elem_type(target_base_type), exp_base_type)) { // %% what about implicit converions...?

	TypeSArray * sa_type = (TypeSArray *) target_base_type;
	uinteger_t count = sa_type->dim->toUInteger();

	tree ctor = make_node( CONSTRUCTOR );
	TREE_TYPE( ctor ) = target_type->toCtype();
	if (count) {
	    CtorEltMaker ce;
	    ce.cons( build2(RANGE_EXPR, Type::tsize_t->toCtype(),
		    integer_zero_node, integerConstant( count - 1 )),
		g.ofile->stripVarDecl(convertForAssignment(exp, sa_type->next)));
	    CONSTRUCTOR_ELTS( ctor ) = ce.head;
	}
	TREE_READONLY( ctor ) = 1;
	TREE_CONSTANT( ctor ) = 1;
	return ctor;
    } else if (! target_type->isscalar() && exp_base_type->isintegral()) {
	// D Front end uses IntegerExp(0) to mean zero-init a structure
	// This could go in convert for assignment, but we only see this for
	// internal init code -- this also includes default init for _d_newarrayi...

	if (exp->toInteger() == 0) {
	    CtorEltMaker ce;
	    tree empty = make_node( CONSTRUCTOR );
	    TREE_TYPE( empty ) = target_type->toCtype();
	    CONSTRUCTOR_ELTS( empty ) = ce.head; // %% will this zero-init?
	    TREE_CONSTANT( empty ) = 1;
	    // static?
	    return empty;
	    // %%TODO: Use a code (lang_specific in decl or flag) to memset instead?
	} else {
	    abort();
	}
    }

    tree exp_tree = exp->toElem(this);
    return convertForAssignment(exp_tree, exp->type, target_type);
}

tree
IRState::convertForAssignment(tree expr, Type * expr_type, Type * target_type)
{
    return convertTo(expr, expr_type, target_type);
}

// could be like C c-typeck.c:default_conversion
// todo:

/* Perform default promotions for C data used in expressions.
   Arrays and functions are converted to pointers;
   enumeral types or short or char, to int.
   In addition, manifest constants symbols are replaced by their values.  */

// what about float->double?

tree
IRState::convertForCondition(tree exp_tree, Type * exp_type) {
    tree result = exp_tree;
    tree a, b, tmp;

    switch (exp_type->toBasetype()->ty) {
    case Taarray:
	// Shouldn't this be...
	//  result = libCall(LIBCALL_AALEN, 1, & exp_tree);

	result = component(exp_tree, TYPE_FIELDS(TREE_TYPE(exp_tree)));
	break;
    case Tarray:
	// DMD checks (length || ptr) (i.e ary !is null)
	tmp = maybeMakeTemp(result);
	a = delegateObjectRef(tmp);
	b = delegateMethodRef(tmp);
	if (TYPE_MODE(TREE_TYPE(a)) == TYPE_MODE(TREE_TYPE(b)))
	    result = build2(BIT_IOR_EXPR, TREE_TYPE(a), a,
			    convert(TREE_TYPE(a), b));
	else {
	    a = d_truthvalue_conversion(a);
	    b = d_truthvalue_conversion(b);
	    // probably not worth TRUTH_OROR ...
	    result = build2(TRUTH_OR_EXPR, TREE_TYPE(a), a, b);
	}
	break;
    case Tdelegate:
	// DMD checks (function || object), but what good
	// is if if there is a null function pointer?
	if ( D_IS_METHOD_CALL_EXPR(result) ) {
	    extractMethodCallExpr(result, a, b);
	} else {
	    tmp = maybeMakeTemp(result);
	    a = delegateObjectRef(tmp);
	    b = delegateMethodRef(tmp);
	}
	// not worth using  or TRUTH_ORIF...
	// %%TODO: Is this okay for all targets?
	result = build2(BIT_IOR_EXPR, TREE_TYPE(a), a, b);
	break;
    default:
	break;
    }
    // see expr.c:do_jump for the types of trees that can be passed to expand_start_cond
    //TREE_USED( <result> ) = 1; // %% maybe not.. expr optimized away because constant expression?
    return d_truthvalue_conversion( result );
}

/* Convert EXP to a dynamic array.  EXP must be a static array or
   dynamic array. */
tree
IRState::toDArray(Expression * exp)
{
    TY ty = exp->type->toBasetype()->ty;
    tree val;
    if (ty == Tsarray) {
	val = convertTo(exp, exp->type->toBasetype()->nextOf()->arrayOf());
    } else if (ty == Tarray) {
	val = exp->toElem(this);
    } else {
	gcc_assert(ty == Tsarray || ty == Tarray);
	return NULL_TREE;
    }
    return val;
}


tree
IRState::pointerIntSum(tree ptr_node, tree idx_exp)
{
    tree result_type_node = TREE_TYPE( ptr_node );
    tree intop = idx_exp;
    tree size_exp;

    tree prod_result_type;
#if D_GCC_VER >= 43
    prod_result_type = sizetype;
#else
    prod_result_type = result_type_node;
#endif

    // %% TODO: real-not-long-double issues...

    size_exp = size_in_bytes( TREE_TYPE( result_type_node ) ); // array element size
    if (integer_zerop(size_exp) || // Test for void case...
	integer_onep(size_exp))    // ...or byte case -- No need to multiply.
    {
	intop = fold(convert(prod_result_type, intop));
    }
    else
    {

	if (TYPE_PRECISION (TREE_TYPE (intop)) != TYPE_PRECISION (sizetype)
	    || TYPE_UNSIGNED (TREE_TYPE (intop)) != TYPE_UNSIGNED (sizetype))
	    intop = convert (d_type_for_size (TYPE_PRECISION (sizetype),
				 TYPE_UNSIGNED (sizetype)), intop);
	intop = convert (prod_result_type,
	    build2/*_binary_op*/ (MULT_EXPR, TREE_TYPE(size_exp), intop,  // the type here may be wrong %%
		convert (TREE_TYPE (intop), size_exp)));
	intop = fold(intop);
    }

    if (result_type_node == error_mark_node)
	return result_type_node; // else we'll ICE.

    if ( integer_zerop(intop) )
	return ptr_node;
    else
    {
#if D_GCC_VER >= 43
	return build2(POINTER_PLUS_EXPR, result_type_node, ptr_node, intop);
#else
	return build2(PLUS_EXPR, result_type_node, ptr_node, intop);
#endif
    }
}

// %% op should be tree_code
tree
IRState::pointerOffsetOp(int op, tree ptr, tree idx)
{
#if D_GCC_VER >= 43
    if (op == PLUS_EXPR)
	{ /* nothing */ }
    else if (op == MINUS_EXPR)
	// %% sign extension...
	idx = fold_build1(NEGATE_EXPR, sizetype, idx);
    else
    {
	gcc_unreachable();
	return error_mark_node;
    }
    return build2(POINTER_PLUS_EXPR, TREE_TYPE(ptr), ptr,
	convert(sizetype, idx));
#else
    return build2((enum tree_code) op, TREE_TYPE(ptr), ptr, idx);
#endif
}

tree
IRState::pointerOffset(tree ptr_node, tree byte_offset)
{
    tree ofs = fold(convert(sizetype, byte_offset));
    tree t;
#if D_GCC_VER >= 43
    t = build2(POINTER_PLUS_EXPR, TREE_TYPE(ptr_node), ptr_node, ofs);
#else
    t = build2(PLUS_EXPR, TREE_TYPE(ptr_node), ptr_node, ofs);
#endif
    return fold(t);
}


// Doesn't do some of the omptimizations in ::makeArrayElemRef
tree
IRState::checkedIndex(Loc loc, tree index, tree upper_bound, bool inclusive)
{
    if (global.params.useArrayBounds) {
	return build3(COND_EXPR, TREE_TYPE(index),
	    boundsCond(index, upper_bound, inclusive),
	    index,
	    assertCall(loc, LIBCALL_ARRAY_BOUNDS));
    } else {
	return index;
    }
}


// index must be wrapped in a SAVE_EXPR to prevent multiple evaluation...
tree
IRState::boundsCond(tree index, tree upper_bound, bool inclusive)
{
    tree bound_check;

    bound_check = build2(inclusive ? LE_EXPR : LT_EXPR, boolean_type_node,
	convert( d_unsigned_type(TREE_TYPE(index)), index ),
	upper_bound);

    if (! TYPE_UNSIGNED( TREE_TYPE( index ))) {
	bound_check = build2(TRUTH_ANDIF_EXPR, boolean_type_node, bound_check,
	    // %% conversions
	    build2(GE_EXPR, boolean_type_node, index, integer_zero_node));
    }

    return bound_check;
}

tree
IRState::assertCall(Loc loc, LibCall libcall)
{
    tree args[2] = { darrayString(loc.filename ? loc.filename : ""),
		     integerConstant(loc.linnum, Type::tuns32) };
    return libCall(libcall, 2, args);
}

tree
IRState::assertCall(Loc loc, Expression * msg)
{
    tree args[3] = { msg->toElem(this),
		     darrayString(loc.filename ? loc.filename : ""),
		     integerConstant(loc.linnum, Type::tuns32) };
    return libCall(LIBCALL_ASSERT_MSG, 3, args);
}

tree
IRState::floatConstant(const real_t & value, TypeBasic * target_type )
{
    REAL_VALUE_TYPE converted_val;

    tree type_node = target_type->toCtype();
    real_convert(& converted_val, TYPE_MODE(type_node), & value.rv());

    return build_real(type_node, converted_val);
}

xdmd_integer_t
IRState::hwi2toli(HOST_WIDE_INT low, HOST_WIDE_INT high)
{
    uinteger_t result;
    if (sizeof(HOST_WIDE_INT) < sizeof(xdmd_integer_t))
    {
	gcc_assert(sizeof(HOST_WIDE_INT) * 2 == sizeof(xdmd_integer_t));
	result = (unsigned HOST_WIDE_INT) low;
	result += ((uinteger_t) (unsigned HOST_WIDE_INT) high) << HOST_BITS_PER_WIDE_INT;
    }
    else
	result = low;
    return result;
}


#if D_GCC_VER >= 40
tree
IRState::binding(tree var_chain, tree body)
{
    // BIND_EXPR/DECL_INITIAL not supported in 4.0?
    gcc_assert(TREE_CHAIN(var_chain) == NULL_TREE); // TODO: only handles one var

#if D_GCC_VER >= 43
    // %%EXPER -- if a BIND_EXPR is an a SAVE_EXPR, gimplify dies
    // in mostly_copy_tree_r.  prevent the latter from seeing
    // our shameful BIND_EXPR by wrapping it in a TARGET_EXPR
    if ( DECL_INITIAL(var_chain) )
    {
	tree ini = build2(MODIFY_EXPR, void_type_node, var_chain, DECL_INITIAL(var_chain));
	DECL_INITIAL(var_chain) = NULL_TREE;
	body = compound(ini, body);
    }
    return save_expr(build3(BIND_EXPR, TREE_TYPE(body), var_chain, body, NULL_TREE));
#else
    if ( DECL_INITIAL(var_chain) )
    {
	tree ini = build2(MODIFY_EXPR, void_type_node, var_chain, DECL_INITIAL(var_chain));
	DECL_INITIAL(var_chain) = NULL_TREE;
	body = compound(ini, body);
    }

    return build3(BIND_EXPR, TREE_TYPE(body), var_chain, body, NULL_TREE);
#endif
}
#endif

tree
IRState::libCall(LibCall lib_call, unsigned n_args, tree *args, tree force_result_type)
{
    tree result;
    tree callee = functionPointer( getLibCallDecl( lib_call ));
    // for force_result_type, assumes caller knows what it is doing %%
    tree result_type = force_result_type != NULL_TREE ?
	force_result_type : TREE_TYPE(TREE_TYPE(TREE_OPERAND(callee, 0)));
    tree arg_list = NULL_TREE;
    for (int i = n_args - 1; i >= 0; i--) {
	arg_list = tree_cons(NULL_TREE, args[i], arg_list);
    }

    result = buildCall(result_type, callee, arg_list);
    return result;
}

static inline bool
function_type_p(tree t)
{
    return TREE_CODE(t) == FUNCTION_TYPE || TREE_CODE(t) == METHOD_TYPE;
}

// Assumes T is already ->toBasetype()
static TypeFunction *
get_function_type(Type * t)
{
    TypeFunction* tf = NULL;
    if (t->ty == Tpointer)
	t = t->nextOf()->toBasetype();
    if (t->ty == Tfunction)
	tf = (TypeFunction *) t;
    else if (t->ty == Tdelegate)
	tf = (TypeFunction *) ((TypeDelegate *) t)->next;
    return tf;
}

tree IRState::errorMark(Type * t)
{
    return nop(error_mark_node, t->toCtype());
}

tree
IRState::call(Expression * expr, /*TypeFunction * func_type, */ Array * arguments)
{
    // Calls to delegates can sometimes look like this:
    if (expr->op == TOKcomma)
    {
	CommaExp * ce = (CommaExp *) expr;
	expr = ce->e2;

	VarExp * ve;
	gcc_assert( ce->e2->op == TOKvar );
	ve = (VarExp *) ce->e2;
	gcc_assert(ve->var->isFuncDeclaration() && ! ve->var->needThis());
    }

    Type* t = expr->type->toBasetype();
    TypeFunction* tf = NULL;
    tree callee = expr->toElem(this);
    tree object = NULL_TREE;

    if ( D_IS_METHOD_CALL_EXPR( callee ) ) {
	/* This could be a delegate expression (TY == Tdelegate), but not
	   actually a delegate variable. */
	tf = get_function_type(t);
	extractMethodCallExpr(callee, callee, object);
    } else if ( t->ty == Tdelegate) {
	tf = (TypeFunction*) ((TypeDelegate *) t)->next;
	callee = maybeMakeTemp(callee);
	object = delegateObjectRef(callee);
	callee = delegateMethodRef(callee);
    } else if (expr->op == TOKvar) {
	FuncDeclaration * fd = ((VarExp *) expr)->var->isFuncDeclaration();
	tf = (TypeFunction *) fd->type;
	if (fd) {
	    if (fd->isNested()) {
#if D_NO_TRAMPOLINES
		object = getFrameForFunction(fd);
#else
		// Pass fake argument for nested functions
		object = d_null_pointer;
#endif
	    } else if (fd->needThis()) {
		expr->error("need 'this' to access member %s", fd->toChars());
		object = d_null_pointer; // continue processing...
	    }
	}
    } else {
	tf = get_function_type(t);
    }
    return call(tf, callee, object, arguments);
}

tree
IRState::call(FuncDeclaration * func_decl, Array * args)
{
    assert(! func_decl->isNested()); // Otherwise need to copy code from above
    return call((TypeFunction *) func_decl->type, func_decl->toSymbol()->Stree, NULL_TREE, args);
}

tree
IRState::call(FuncDeclaration * func_decl, tree object, Array * args)
{
    return call((TypeFunction *)func_decl->type, functionPointer(func_decl),
	object, args);
}

tree
IRState::call(TypeFunction *func_type, tree callable, tree object, Array * arguments)
{
    // Using TREE_TYPE( callable ) instead of func_type->toCtype can save a build_method_type
    tree func_type_node = TREE_TYPE( callable );
    tree actual_callee  = callable;

    if ( POINTER_TYPE_P( func_type_node ) )
	func_type_node = TREE_TYPE( func_type_node );
    else
	actual_callee = addressOf( callable );

    assert( function_type_p( func_type_node ) );
    assert( func_type != NULL );
    assert( func_type->ty == Tfunction );

    bool is_d_vararg = func_type->varargs == 1 && func_type->linkage == LINKd;

    // Account for the hidden object/frame pointer argument

    if ( TREE_CODE( func_type_node ) == FUNCTION_TYPE ) {
	if ( object != NULL_TREE ) {
	    // Happens when a delegate value is called
	    tree method_type = build_method_type( TREE_TYPE( object ), func_type_node );
	    TYPE_ATTRIBUTES( method_type ) = TYPE_ATTRIBUTES( func_type_node );
	    func_type_node = method_type;
	}
    } else /* METHOD_TYPE */ {
	if ( ! object ) {
	    // Front-end apparently doesn't check this.
	    if (TREE_CODE(callable) == FUNCTION_DECL ) {
		error("need 'this' to access member %s", IDENTIFIER_POINTER( DECL_NAME( callable )));
		return error_mark_node;
	    } else {
		// Probably an internal error
		assert(object != NULL_TREE);
	    }
	}
    }

    ListMaker actual_arg_list;

    /* If this is a delegate call or a nested function being called as
       a delegate, the object should not be NULL. */
    if (object != NULL_TREE)
	actual_arg_list.cons( object );

#if V2 //Until 2.037
    Arguments * formal_args = func_type->parameters; // can be NULL for genCfunc decls
    size_t n_formal_args = formal_args ? (int) Argument::dim(formal_args) : 0;
#else
    Parameters * formal_args = func_type->parameters; // can be NULL for genCfunc decls
    size_t n_formal_args = formal_args ? (int) Parameter::dim(formal_args) : 0;
#endif
    size_t n_actual_args = arguments ? arguments->dim : 0;
    size_t fi = 0;

    // assumes arguments->dim <= formal_args->dim if (! this->varargs)
    for (size_t ai = 0; ai < n_actual_args; ++ai) {
	tree actual_arg_tree;

	Expression * actual_arg_exp = (Expression *) arguments->data[ai];
	if (ai == 0 && is_d_vararg) {
	    // The hidden _arguments parameter
	    actual_arg_tree = actual_arg_exp->toElem(this);
	} else if (fi < n_formal_args) {
	    // Actual arguments for declared formal arguments
#if V2 //Until 2.037
	    Argument * formal_arg = Argument::getNth(formal_args, fi);
#else
	    Parameter * formal_arg = Parameter::getNth(formal_args, fi);
#endif
	    actual_arg_tree = convertForArgument(actual_arg_exp, formal_arg);

	    // from c-typeck.c: convert_arguments, default_conversion, ...
	    if (INTEGRAL_TYPE_P (TREE_TYPE(actual_arg_tree))
		&& (TYPE_PRECISION (TREE_TYPE(actual_arg_tree)) <
		    TYPE_PRECISION (integer_type_node))) {

		actual_arg_tree = d_convert_basic(integer_type_node, actual_arg_tree);
	    }
	    ++fi;
	} else {
	    if (splitDynArrayVarArgs && actual_arg_exp->type->toBasetype()->ty == Tarray)
	    {
		tree da_exp = maybeMakeTemp( actual_arg_exp->toElem(this) );
		actual_arg_list.cons( darrayLenRef( da_exp ) );
		actual_arg_list.cons( darrayPtrRef( da_exp ) );
		continue;
	    }
	    else
	    {
		actual_arg_tree = actual_arg_exp->toElem( this );

		/* Not all targets support passing unpromoted types, so
		   promote anyway. */
		tree prom_type = d_type_promotes_to( TREE_TYPE( actual_arg_tree ));
		if (prom_type != TREE_TYPE( actual_arg_tree ))
		    actual_arg_tree = d_convert_basic(prom_type, actual_arg_tree);
	    }
	}

	//TREE_USED( actual_arg_tree ) = 1; // needed ?
	actual_arg_list.cons( actual_arg_tree );
    }

    tree result = buildCall(TREE_TYPE(func_type_node), actual_callee, actual_arg_list.head);
    return maybeExpandSpecialCall(result);
}

static const char * libcall_ids[LIBCALL_count] =
    { "_d_assert", "_d_assert_msg", "_d_array_bounds", "_d_switch_error",
      "_D9invariant12_d_invariantFC6ObjectZv",
      "_d_newclass", "_d_newarrayT",
      "_d_newarrayiT",
      "_d_newarraymTp", "_d_newarraymiTp", "_d_allocmemory",
      "_d_delclass", "_d_delinterface", "_d_delarray",
      "_d_delmemory", "_d_callfinalizer", "_d_callinterfacefinalizer",
      "_d_arraysetlengthT", "_d_arraysetlengthiT",
      "_d_dynamic_cast", "_d_interface_cast",
      "_adEq", "_adCmp", "_adCmpChar",
      "_aaLen",
      //"_aaIn", "_aaGet", "_aaGetRvalue", "_aaDel",
      "_aaInp", "_aaGetp", "_aaGetRvaluep", "_aaDelp",
      "_d_arraycast",
      "_d_arraycopy",
      "_d_arraycatT", "_d_arraycatnT",
      "_d_arrayappendT",
      /*"_d_arrayappendc", */"_d_arrayappendcTp",
#if V2
      "_d_arrayassign", "_d_arrayctor", "_d_arraysetassign",
      "_d_arraysetctor",
#endif
      "_d_monitorenter", "_d_monitorexit",
      "_d_criticalenter", "_d_criticalexit",
      "_d_throw",
      "_d_switch_string", "_d_switch_ustring", "_d_switch_dstring",
      "_d_assocarrayliteralTp"
#if V2
      ,
      "_d_hidden_func"
#endif
    };

static FuncDeclaration * libcall_decls[LIBCALL_count];

void
IRState::replaceLibCallDecl(FuncDeclaration * d_decl)
{
    if ( ! d_decl->ident )
	return;
    for (unsigned i = 0; i < LIBCALL_count; i++) {
	if ( strcmp(d_decl->ident->string, libcall_ids[i]) == 0 ) {
	    // %% warn if libcall already set?
	    // Only do this for the libcalls where it's a problem, otherwise
	    // it causes other problems...
	    switch ((LibCall) i) {
		// case LIBCALL_GNU_BITARRAYSLICEP:
	    case LIBCALL_ARRAYCOPY: // this could be solved by turning copy of char into memcpy
	    case LIBCALL_ARRAYCAST:
		// replace the function declaration
		break;
	    default:
		// don't replace
		return;
	    }
	    libcall_decls[i] = d_decl;
	    break;
	}
    }
}


FuncDeclaration *
IRState::getLibCallDecl(LibCall lib_call)
{
    FuncDeclaration * decl = libcall_decls[lib_call];
    Array arg_types;
    Type * t = NULL;
    bool varargs = false;

    if (! decl) {
	Type * return_type = Type::tvoid;

	switch (lib_call) {
	case LIBCALL_ASSERT:
	case LIBCALL_ARRAY_BOUNDS:
	case LIBCALL_SWITCH_ERROR:
	    // need to spec chararray/string because internal code passes string constants
	    arg_types.reserve(2);
	    arg_types.push( Type::tchar->arrayOf() );
	    arg_types.push( Type::tuns32 );
	    break;
	case LIBCALL_ASSERT_MSG:
	    arg_types.reserve(3);
	    arg_types.push( Type::tchar->arrayOf() );
	    arg_types.push( Type::tchar->arrayOf() );
	    arg_types.push( Type::tuns32 );
	    break;
	case LIBCALL_NEWCLASS:
	    arg_types.push( ClassDeclaration::classinfo->type );
	    return_type = getObjectType();
	    break;
	case LIBCALL_NEWARRAYT:
	case LIBCALL_NEWARRAYIT:
	    arg_types.push( Type::typeinfo->type );
	    arg_types.push( Type::tsize_t );
	    return_type = Type::tvoid->arrayOf();
	    break;
	case LIBCALL_NEWARRAYMTP:
	case LIBCALL_NEWARRAYMITP:
	    arg_types.push( Type::typeinfo->type );
	    arg_types.push( Type::tint32 ); // Currently 'int', even if 64-bit
	    arg_types.push( Type::tsize_t );
	    return_type = Type::tvoid->arrayOf();
	    break;
	case LIBCALL_ALLOCMEMORY:
	    arg_types.push( Type::tsize_t );
	    return_type = Type::tvoidptr;
	    break;
	case LIBCALL_DELCLASS:
	case LIBCALL_DELINTERFACE:
	    arg_types.push(Type::tvoid->pointerTo());
	    break;
	case LIBCALL_DELARRAY:
	    arg_types.push(Type::tvoid->arrayOf()->pointerTo());
	    break;
	case LIBCALL_DELMEMORY:
	    arg_types.push(Type::tvoid->pointerTo()->pointerTo());
	    break;
	case LIBCALL_CALLFINALIZER:
	case LIBCALL_CALLINTERFACEFINALIZER:
	    arg_types.push(Type::tvoid->pointerTo());
	    break;
	case LIBCALL_ARRAYSETLENGTHT:
	case LIBCALL_ARRAYSETLENGTHIT:
	    arg_types.push( Type::typeinfo->type );
	    arg_types.push( Type::tsize_t );
	    arg_types.push( Type::tvoid->arrayOf()->pointerTo() );
	    return_type = Type::tvoid->arrayOf();
	    break;
	case LIBCALL_DYNAMIC_CAST:
	case LIBCALL_INTERFACE_CAST:
	    arg_types.push( getObjectType() );
	    arg_types.push( ClassDeclaration::classinfo->type );
	    return_type = getObjectType();
	    break;
	case LIBCALL_ADEQ:
	case LIBCALL_ADCMP:
	    arg_types.reserve(3);
	    arg_types.push(Type::tvoid->arrayOf());
	    arg_types.push(Type::tvoid->arrayOf());
	    arg_types.push(Type::typeinfo->type);
	    return_type = Type::tint32;
	    break;
	case LIBCALL_ADCMPCHAR:
	    arg_types.reserve(2);
	    arg_types.push(Type::tchar->arrayOf());
	    arg_types.push(Type::tchar->arrayOf());
	    return_type = Type::tint32;
	    break;
// 	case LIBCALL_AAIN:
// 	case LIBCALL_AAGET:
// 	case LIBCALL_AAGETRVALUE:
// 	case LIBCALL_AADEL:
	case LIBCALL_AALEN:
	case LIBCALL_AAINP:
	case LIBCALL_AAGETP:
	case LIBCALL_AAGETRVALUEP:
	case LIBCALL_AADELP:
	    {
		static Type * aa_type = NULL;
		if (! aa_type)
		    aa_type = new TypeAArray(Type::tvoid->pointerTo(),
			Type::tvoid->pointerTo());

		if (lib_call == LIBCALL_AALEN)
		{
		    arg_types.push(aa_type);
		    return_type = Type::tsize_t;
		    break;
		}

		if (lib_call == LIBCALL_AAGETP)
		    aa_type = aa_type->pointerTo();

		arg_types.reserve(3);
		arg_types.push(aa_type);
		arg_types.push(Type::typeinfo->type); // typeinfo reference
		if ( lib_call == LIBCALL_AAGETP || lib_call == LIBCALL_AAGETRVALUEP)
		    arg_types.push(Type::tsize_t);

		arg_types.push(Type::tvoid->pointerTo());

		switch (lib_call) {
		case LIBCALL_AAINP:
		case LIBCALL_AAGETP:
		case LIBCALL_AAGETRVALUEP:
		    return_type = Type::tvoid->pointerTo();
		    break;
		case LIBCALL_AADELP:
		    return_type = Type::tvoid;
		    break;
		default:
		    assert(0);
		}
	    }
	    break;
	case LIBCALL_ARRAYCAST:
	    t = Type::tvoid->arrayOf();
	    arg_types.push(Type::tsize_t);
	    arg_types.push(Type::tsize_t);
	    arg_types.push(t);
	    return_type = t;
	    break;
	case LIBCALL_ARRAYCOPY:
	    t = Type::tvoid->arrayOf();
	    arg_types.push(Type::tsize_t);
	    arg_types.push(t);
	    arg_types.push(t);
	    return_type = t;
	    break;
	case LIBCALL_ARRAYCATT:
	    arg_types.push(Type::typeinfo->type);
	    t = Type::tvoid->arrayOf();
	    arg_types.push(t);
	    arg_types.push(t);
	    return_type = t;
	    break;
	case LIBCALL_ARRAYCATNT:
	    arg_types.push(Type::typeinfo->type);
	    arg_types.push(Type::tuns32); // Currently 'uint', even if 64-bit
	    varargs = true;
	    return_type = Type::tvoid->arrayOf();
	    break;
	case LIBCALL_ARRAYAPPENDT:
	    arg_types.push(Type::typeinfo->type);
	    t = Type::tuns8->arrayOf();
	    arg_types.push(t->pointerTo());
	    arg_types.push(t);
	    return_type = Type::tvoid->arrayOf();
	    break;
// 	case LIBCALL_ARRAYAPPENDCT:
	case LIBCALL_ARRAYAPPENDCTP:
	    arg_types.push(Type::typeinfo->type);
	    t = Type::tuns8->arrayOf();
	    arg_types.push(t);
	    arg_types.push(Type::tvoid->pointerTo()); // varargs = true;
	    return_type = Type::tvoid->arrayOf();
	    break;
#if V2
	case LIBCALL_ARRAYASSIGN:
	case LIBCALL_ARRAYCTOR:
	    arg_types.push(Type::typeinfo->type);
	    arg_types.push(Type::tvoid->arrayOf());
	    arg_types.push(Type::tvoid->arrayOf());
	    return_type = Type::tvoid->arrayOf();
	    break;
	case LIBCALL_ARRAYSETASSIGN:
	case LIBCALL_ARRAYSETCTOR:
	    arg_types.push(Type::tvoid->pointerTo());
	    arg_types.push(Type::tvoid->pointerTo());
	    arg_types.push(Type::tsize_t);
	    arg_types.push(Type::typeinfo->type);
	    return_type = Type::tvoid->pointerTo();
	    break;
#endif
	case LIBCALL_MONITORENTER:
	case LIBCALL_MONITOREXIT:
	case LIBCALL_THROW:
	case LIBCALL_INVARIANT:
	    arg_types.push(getObjectType());
	    break;
	case LIBCALL_CRITICALENTER:
	case LIBCALL_CRITICALEXIT:
	    arg_types.push(Type::tvoid->pointerTo());
	    break;
	case LIBCALL_SWITCH_USTRING:
	    t = Type::twchar;
	    goto do_switch_string;
	case LIBCALL_SWITCH_DSTRING:
	    t = Type::tdchar;
	    goto do_switch_string;
	case LIBCALL_SWITCH_STRING:
	    t = Type::tchar;
	do_switch_string:
	    t = t->arrayOf();
	    arg_types.push(t->arrayOf());
	    arg_types.push(t);
	    return_type = Type::tint32;
	    break;
	case LIBCALL_ASSOCARRAYLITERALTP:
	    arg_types.push(Type::typeinfo->type);
	    arg_types.push(Type::tsize_t);
	    arg_types.push(Type::tvoid->pointerTo());
	    arg_types.push(Type::tvoid->pointerTo());
	    return_type = Type::tvoid->pointerTo();
	    break;
#if V2
	case LIBCALL_HIDDEN_FUNC:
	    /* Argument is an Object, but can't use that as
	       LIBCALL_HIDDEN_FUNC is needed before the Object type is
	       created. */
	    arg_types.push(Type::tvoid->pointerTo());
	    break;
#endif
	default:
	    gcc_unreachable();
	}
	decl = FuncDeclaration::genCfunc(return_type, (char *) libcall_ids[lib_call]);
	{
	    TypeFunction * tf = (TypeFunction *) decl->type;
	    tf->varargs = varargs ? 1 : 0;
#if V2 //Until 2.037
	    Arguments * args = new Arguments;
#else
	    Parameters * args = new Parameters;
#endif
	    args->setDim( arg_types.dim );
	    for (unsigned i = 0; i < arg_types.dim; i++)
#if V2 //Until 2.037
		args->data[i] = new Argument( STCin, (Type *) arg_types.data[i],
#else
		args->data[i] = new Parameter( STCin, (Type *) arg_types.data[i],
#endif
		    NULL, NULL);
	    tf->parameters = args;
	}
	libcall_decls[lib_call] = decl;
    }
    return decl;
}

static tree
fix_d_va_list_type(tree val)
{
    if (POINTER_TYPE_P(va_list_type_node) ||
	INTEGRAL_TYPE_P(va_list_type_node))
	return build1(NOP_EXPR, va_list_type_node, val);
    else
	return val;
}

tree IRState::maybeExpandSpecialCall(tree call_exp)
{
    // More code duplication from C
    CallExpr ce(call_exp);
    tree callee = ce.callee();
    tree t, op;
    if (POINTER_TYPE_P(TREE_TYPE( callee ))) {
	callee = TREE_OPERAND(callee, 0);
    }
    if (TREE_CODE(callee) == FUNCTION_DECL) {
	if (DECL_BUILT_IN_CLASS(callee) == NOT_BUILT_IN) {
	    // the most common case
	    return call_exp;
	} else if (DECL_BUILT_IN_CLASS(callee) == BUILT_IN_NORMAL) {
	    switch (DECL_FUNCTION_CODE(callee)) {
	    case BUILT_IN_ABS:
	    case BUILT_IN_LABS:
	    case BUILT_IN_LLABS:
	    case BUILT_IN_IMAXABS:
		/* The above are required for both 3.4.  Not sure about later
		   versions. */
		/* OLDOLDOLD below supposedly for 3.3 only */
		/*
	    case BUILT_IN_FABS:
	    case BUILT_IN_FABSL:
	    case BUILT_IN_FABSF:
		*/
		op = ce.nextArg();
		t = build1(ABS_EXPR, TREE_TYPE(op), op);
		return d_convert_basic(TREE_TYPE(call_exp), t);
		// probably need a few more cases:
	    default:
		return call_exp;
	    }
	} else if (DECL_BUILT_IN_CLASS(callee) == BUILT_IN_FRONTEND) {
	    Intrinsic intrinsic = (Intrinsic) DECL_FUNCTION_CODE(callee);
	    tree type;
	    Type *d_type;
	    switch (intrinsic) {
	    case INTRINSIC_C_VA_ARG:
		// %% should_check c_promotes_to as in va_arg now
		// just drop though for now...
	    case INTRINSIC_STD_VA_ARG:
		t = ce.nextArg();
		// signature is (inout va_list), but VA_ARG_EXPR expects the
		// list itself... but not if the va_list type is an array.  In that
		// case, it should be a pointer
		if ( TREE_CODE( va_list_type_node ) != ARRAY_TYPE ) {
		    if ( TREE_CODE( t ) == ADDR_EXPR ) {
			t = TREE_OPERAND(t, 0);
		    } else {
			// this probably doesn't happen... passing an inout va_list argument,
			// but again,  it's probably { & ( * inout_arg ) }
			t = build1(INDIRECT_REF, TREE_TYPE(TREE_TYPE(t)), t);
		    }
		}
		t = fix_d_va_list_type(t);
		type = TREE_TYPE(TREE_TYPE(callee));
		if (splitDynArrayVarArgs && (d_type = getDType(type)) &&
		    d_type->toBasetype()->ty == Tarray)
		{
		    // should create a temp var of type TYPE and move the binding
		    // to outside this expression.
		    t = stabilize_reference(t);
		    tree ltype = TREE_TYPE( TYPE_FIELDS( type ));
		    tree ptype = TREE_TYPE( TREE_CHAIN( TYPE_FIELDS( type )));
		    tree lvar = exprVar(ltype);
		    tree pvar = exprVar(ptype);
		    tree e1 = vmodify(lvar, build1(VA_ARG_EXPR, ltype, t));
		    tree e2 = vmodify(pvar, build1(VA_ARG_EXPR, ptype, t));
		    tree b = compound( compound( e1, e2 ), darrayVal(type, lvar, pvar) );
		    return binding(lvar, binding(pvar, b));
		}
		else
		{
		    tree type2 = d_type_promotes_to(type);
		    t = build1(VA_ARG_EXPR, type2, t);
		    if (type != type2)
			// silently convert promoted type...
			t = d_convert_basic(type, t);
		    return t;
		}
		break;
	    case INTRINSIC_C_VA_START:
		/*
		t = TREE_VALUE();
		// signature is (inout va_list), but VA_ARG_EXPR expects the
		// list itself...
		if ( TREE_CODE( t ) == ADDR_EXPR ) {
		    t = TREE_OPERAND(t, 0);
		} else {
		    // this probably doesn't happen... passing an inout va_list argument,
		    // but again,  it's probably { & ( * inout_arg ) }
		    t = build1(INDIRECT_REF, TREE_TYPE(TREE_TYPE(t)), t);
		}
		*/
		// The va_list argument should already have its
		// address taken.  The second argument, however, is
		// inout and that needs to be fixed to prevent a warning.
		{
		    tree val_arg = ce.nextArg();
		    // kinda wrong... could be casting.. so need to check type too?
		    while ( TREE_CODE( val_arg ) == NOP_EXPR )
			val_arg = TREE_OPERAND(val_arg, 0);
		    if ( TREE_CODE( val_arg ) == ADDR_EXPR ) {
			val_arg = TREE_OPERAND(val_arg, 0);
			val_arg = fix_d_va_list_type(val_arg);
			val_arg = addressOf( val_arg );
		    } else
			val_arg = fix_d_va_list_type(val_arg);

		    t = ce.nextArg();
		    if ( TREE_CODE( t ) == ADDR_EXPR ) {
			t = TREE_OPERAND(t, 0);
		    }

		    return buildCall( void_type_node, // assuming nobody tries to change the return type
			addressOf( built_in_decls[BUILT_IN_VA_START] ),
			tree_cons( NULL_TREE, val_arg,
			tree_cons( NULL_TREE, t, NULL_TREE )));
		}
	    default:
		abort();
		break;
	    }

	} else if (0 /** WIP **/ && DECL_BUILT_IN_CLASS(callee) == BUILT_IN_FRONTEND) {
	    // %%TODO: need to handle BITS_BIG_ENDIAN
	    // %%TODO: need to make expressions unsigned

	    Intrinsic intrinsic = (Intrinsic) DECL_FUNCTION_CODE(callee);
	    // Might as well do intrinsics here...
	    switch (intrinsic) {
	    case INTRINSIC_BSF: // This will use bsf on x86, but BSR becomes 31-(bsf)!!
		// drop through
	    case INTRINSIC_BSR:
		// %% types should be correct, but should still check..
		// %% 64-bit..
		return call_exp;
		//YOWZA1
#if D_GCC_VER >= 34
		return buildCall(TREE_TYPE(call_exp),
		    built_in_decls[intrinsic == INTRINSIC_BSF ? BUILT_IN_CTZ : BUILT_IN_CLZ],
		    tree_cons(NULL_TREE, ce.nextArg(), NULL_TREE));
#else
		return call_exp;
#endif
	    case INTRINSIC_BT:
	    case INTRINSIC_BTC:
	    case INTRINSIC_BTR:
	    case INTRINSIC_BTS:
		break;
	    case INTRINSIC_BSWAP:
#if defined(TARGET_386)

#endif
		break;
	    case INTRINSIC_INP:
	    case INTRINSIC_INPW:
	    case INTRINSIC_INPL:
	    case INTRINSIC_OUTP:
	    case INTRINSIC_OUTPW:
	    case INTRINSIC_OUTPL:
#ifdef TARGET_386
#else
		::error("Port I/O intrinsic '%s' is only available on ix86 targets",
		    IDENTIFIER_POINTER(DECL_NAME(callee)));
#endif
		break;
	    default:
		abort();
	    }
	}
    }

    return call_exp;
}

tree
IRState::arrayElemRef(IndexExp * aer_exp, ArrayScope * aryscp)
{
    Expression * e1 = aer_exp->e1;
    Expression * e2 = aer_exp->e2;

    Type * base_type = e1->type->toBasetype();
    TY base_type_ty = base_type->ty;

#if ! V2
    gcc_assert(! base_type->next->isbit());
#endif

    tree index_expr; // logical index
    tree subscript_expr; // expr that indexes the array data
    tree ptr_exp;  // base pointer to the elements
    tree elem_ref; // reference the the element

    index_expr = e2->toElem( this );
    subscript_expr = index_expr;

    switch (base_type_ty) {
    case Tarray:
    case Tsarray:
	{
	    tree e1_tree = e1->toElem(this);

	    e1_tree = aryscp->setArrayExp(e1_tree, e1->type);

	    if ( global.params.useArrayBounds &&
		// If it's a static array and the index is
		// constant, the front end has already
		// checked the bounds.
		! (base_type_ty == Tsarray && e2->isConst()) ) {


		tree array_len_expr, throw_expr, oob_cond;
		// implement bounds check as a conditional expression:
		// a[ inbounds(index) ? index : { throw ArrayBoundsError } ]
		//
		// First, set up the index expression to only be evaluated
		// once.
		// %% save_expr does this check: if (! TREE_CONSTANT( index_expr ))
		//   %% so we don't do a <0 check for a[2]...
		index_expr = maybeMakeTemp( index_expr );

		if (base_type_ty == Tarray) {
		    e1_tree = maybeMakeTemp(e1_tree);
		    array_len_expr = darrayLenRef(e1_tree);
		} else {
		    array_len_expr = ((TypeSArray *) base_type)->dim->toElem(this);
		}

		oob_cond = boundsCond(index_expr, array_len_expr, false);
		throw_expr = assertCall(aer_exp->loc, LIBCALL_ARRAY_BOUNDS);

		subscript_expr = build3( COND_EXPR, TREE_TYPE( index_expr ),
		    oob_cond, index_expr, throw_expr );
	    }

	    // %% TODO: make this an ARRAY_REF?
	    if (base_type_ty == Tarray)
		ptr_exp = darrayPtrRef( e1_tree ); // %% do convert in darrayPtrRef?
	    else
		ptr_exp = addressOf( e1_tree );
	    // This conversion is required for static arrays and is just-to-be-safe
	    // for dynamic arrays
	    ptr_exp = d_convert_basic(base_type->nextOf()->pointerTo()->toCtype(), ptr_exp);
	}
	break;
    case Tpointer:
	// Ignores aryscp
	ptr_exp = e1->toElem( this );
	break;
    default:
	abort();
    }

    ptr_exp = pvoidOkay( ptr_exp );
    subscript_expr = aryscp->finish( subscript_expr );
    elem_ref = indirect(pointerIntSum( ptr_exp, subscript_expr),
	TREE_TYPE(TREE_TYPE(ptr_exp)));

    return elem_ref;
}

tree
IRState::darrayPtrRef(tree exp)
{
    if (exp == error_mark_node)
	return exp; // else we'll ICE.

    // Get the backend type for the array and pick out the array data
    // pointer field (assumed to be the second field.)
    tree ptr_field = TREE_CHAIN( TYPE_FIELDS( TREE_TYPE( exp )));
    //return build2(COMPONENT_REF, TREE_TYPE( ptr_field ), exp, ptr_field);
    return component(exp, ptr_field);
}

tree
IRState::darrayLenRef(tree exp)
{
    if (exp == error_mark_node)
	return exp; // else we'll ICE.

    // Get the backend type for the array and pick out the array length
    // field (assumed to be the first field.)
    tree len_field = TYPE_FIELDS( TREE_TYPE( exp ));
    return component(exp, len_field);
}


tree
IRState::darrayVal(tree type, tree len, tree data)
{
    // %% assert type is a darray
    tree ctor = make_node( CONSTRUCTOR );
    tree len_field, ptr_field;
    CtorEltMaker ce;

    TREE_TYPE( ctor ) = type;
    TREE_STATIC( ctor ) = 0;   // can be set by caller if needed
    TREE_CONSTANT( ctor ) = 0; // "
    len_field = TYPE_FIELDS( TREE_TYPE( ctor ));
    ptr_field = TREE_CHAIN( len_field );

    ce.cons(len_field, len);
    ce.cons(ptr_field, data); // shouldn't need to convert the pointer...
    CONSTRUCTOR_ELTS( ctor ) = ce.head;

    return ctor;
}

tree
IRState::darrayVal(tree type, uinteger_t len, tree data)
{
    // %% assert type is a darray
    tree ctor = make_node( CONSTRUCTOR );
    tree len_value, ptr_value, len_field, ptr_field;
    CtorEltMaker ce;

    TREE_TYPE( ctor ) = type;
    TREE_STATIC( ctor ) = 0;   // can be set by caller if needed
    TREE_CONSTANT( ctor ) = 0; // "
    len_field = TYPE_FIELDS( TREE_TYPE( ctor ));
    ptr_field = TREE_CHAIN( len_field );

    if (data ) {
	assert( POINTER_TYPE_P( TREE_TYPE( data )));
	ptr_value = data;
    } else {
	ptr_value = d_null_pointer;
    }

    len_value = integerConstant(len, TREE_TYPE(len_field));

    ce.cons(len_field, len_value);
    ce.cons(ptr_field, convert(TREE_TYPE(ptr_field), ptr_value));

    CONSTRUCTOR_ELTS( ctor ) = ce.head;

    return ctor;
}

tree
IRState::darrayString(const char * str)
{
    unsigned len = strlen(str);
    // %% assumes str is null-terminated
    tree str_tree = build_string(len + 1, str);

    TREE_TYPE( str_tree ) = arrayType(Type::tchar, len);
    return darrayVal(Type::tchar->arrayOf()->toCtype(), len, addressOf(str_tree));
}

char *
IRState::hostToTargetString(char * str, size_t length, unsigned unit_size)
{
    if (unit_size == 1)
	return str;
    assert(unit_size == 2 || unit_size == 4);

    bool flip;
#if D_GCC_VER < 41
# ifdef HOST_WORDS_BIG_ENDIAN
    flip = (bool) ! BYTES_BIG_ENDIAN;
# else
    flip = (bool) BYTES_BIG_ENDIAN;
# endif
#else
# if WORDS_BIG_ENDIAN
    flip = (bool) ! BYTES_BIG_ENDIAN;
# else
    flip = (bool) BYTES_BIG_ENDIAN;
# endif
#endif

    if (flip) {
	char * out_str = (char *) xmalloc(length * unit_size);
	const d_uns8 * p_src = (const d_uns8 *) str;
	d_uns8 * p_out = (d_uns8 *) out_str;

	while (length--) {
	    if (unit_size == 2) {
		p_out[0] = p_src[1];
		p_out[1] = p_src[0];
	    } else /* unit_size == 4 */ {
		p_out[0] = p_src[3];
		p_out[1] = p_src[2];
		p_out[2] = p_src[1];
		p_out[3] = p_src[0];
	    }
	    p_src += unit_size;
	    p_out += unit_size;
	}
	return out_str;
    } else {
	return str;
    }
}


tree
IRState::arrayLength(tree exp, Type * exp_type)
{
    Type * base_type = exp_type->toBasetype();
    switch (base_type->ty) {
    case Tsarray:
	return size_int( ((TypeSArray *) base_type)->dim->toUInteger() );
    case Tarray:
	return darrayLenRef(exp);
    default:
	::error("can't determine the length of a %s", exp_type->toChars());
	return error_mark_node;
    }
}

tree
IRState::floatMod(tree a, tree b, Type * d_type)
{
    enum built_in_function fn;
    switch (d_type->toBasetype()->ty) {
    case Tfloat32:
    case Timaginary32:
	fn = BUILT_IN_FMODF;
	break;
    case Tfloat64:
    case Timaginary64:
    no_long_double:
	fn = BUILT_IN_FMOD;
	break;
    case Tfloat80:
    case Timaginary80:
	if (! haveLongDouble())
	    goto no_long_double;
	fn = BUILT_IN_FMODL;
	break;
    default:
	::error("tried to perform floating-point modulo division on %s",
	    d_type->toChars());
	return error_mark_node;
    }
    tree decl = built_in_decls[fn];
    // %% assuming no arg conversion needed
    // %% bypassing buildCall since this shouldn't have
    // side effects
    return buildCall(TREE_TYPE(TREE_TYPE(decl)),
	addressOf(decl),
	tree_cons(NULL_TREE, a,
	    tree_cons(NULL_TREE, b, NULL_TREE)));
}

tree
IRState::typeinfoReference(Type * t)
{
    tree ti_ref = t->getInternalTypeInfo(NULL)->toElem(this);
    assert( POINTER_TYPE_P( TREE_TYPE( ti_ref )) );
    return ti_ref;
}

target_size_t
IRState::getTargetSizeConst(tree t)
{
    target_size_t result;
    if (sizeof(HOST_WIDE_INT) < sizeof(target_size_t))
    {
	gcc_assert(sizeof(HOST_WIDE_INT) * 2 == sizeof(target_size_t));
	result = (unsigned HOST_WIDE_INT) TREE_INT_CST_LOW( t );
	result += ((target_size_t) (unsigned HOST_WIDE_INT) TREE_INT_CST_HIGH( t ))
	    << HOST_BITS_PER_WIDE_INT;
    }
    else
	result = tree_low_cst( t, 1 );
    return result;
}

// delegate is
// struct delegate {
//   void * frame_or_object;
//   void * function;
// }

tree
IRState::delegateObjectRef(tree exp)
{
    // Get the backend type for the array and pick out the array data
    // pointer field (assumed to be the first field.)
    tree obj_field = TYPE_FIELDS( TREE_TYPE( exp ));
    //return build2(COMPONENT_REF, TREE_TYPE( obj_field ), exp, obj_field);
    return component(exp, obj_field);
}

tree
IRState::delegateMethodRef(tree exp)
{
    // Get the backend type for the array and pick out the array length
    // field (assumed to be the second field.)
    tree method_field = TREE_CHAIN( TYPE_FIELDS( TREE_TYPE( exp )));
    //return build2(COMPONENT_REF, TREE_TYPE( method_field ), exp, method_field);
    return component(exp, method_field);
}

// Converts pointer types of method_exp and object_exp to match d_type
tree
IRState::delegateVal(tree method_exp, tree object_exp, Type * d_type)
{
    Type * base_type = d_type->toBasetype();
    if ( base_type->ty == Tfunction ) {
	// Called from DotVarExp.  These are just used to
	// make function calls and not to make Tdelegate variables.
	// Clearing the type makes sure of this.
	base_type = 0;
    } else {
	assert(base_type->ty == Tdelegate);
    }

    tree type = base_type ? base_type->toCtype() : NULL_TREE;
    tree ctor = make_node( CONSTRUCTOR );
    tree obj_field = NULL_TREE;
    tree func_field = NULL_TREE;
    CtorEltMaker ce;

    if (type) {
	TREE_TYPE( ctor ) = type;
	obj_field = TYPE_FIELDS( type );
	func_field = TREE_CHAIN( obj_field );
    }

    if (obj_field)
	ce.cons(obj_field, convert (TREE_TYPE(obj_field), object_exp));
    else
	ce.cons(obj_field, object_exp);

    if (func_field)
	ce.cons(func_field, convert (TREE_TYPE(func_field), method_exp));
    else
	ce.cons(func_field, method_exp);

    CONSTRUCTOR_ELTS( ctor ) = ce.head;
    return ctor;
}

void
IRState::extractMethodCallExpr(tree mcr, tree & callee_out, tree & object_out) {
    assert( D_IS_METHOD_CALL_EXPR( mcr ));
#if D_GCC_VER < 41
    tree elts = CONSTRUCTOR_ELTS( mcr );
    object_out = TREE_VALUE( elts );
    callee_out = TREE_VALUE( TREE_CHAIN( elts ));
#else
    VEC(constructor_elt,gc) *elts = CONSTRUCTOR_ELTS( mcr );
    object_out = VEC_index(constructor_elt, elts, 0)->value;
    callee_out = VEC_index(constructor_elt, elts, 1)->value;
#endif
}

tree
IRState::objectInstanceMethod(Expression * obj_exp, FuncDeclaration * func, Type * d_type)
{
    Type * obj_type = obj_exp->type->toBasetype();
    if (func->isThis()) {
	bool is_dottype;
	tree this_expr;

	// DotTypeExp cannot be evaluated
	if (obj_exp->op == TOKdottype) {
	    is_dottype = true;
	    this_expr = ((DotTypeExp *) obj_exp)->e1->toElem( this );
	} else if (obj_exp->op == TOKcast &&
	    ((CastExp*) obj_exp)->e1->op == TOKdottype) {
	    is_dottype = true;
	    // see expression.c:"See if we need to adjust the 'this' pointer"
	    this_expr = ((DotTypeExp *) ((CastExp*) obj_exp)->e1)->e1->toElem( this );
	} else {
	    is_dottype = false;
	    this_expr = obj_exp->toElem( this );
	}

	// Calls to super are static (func is the super's method)
	// Structs don't have vtables.
	// Final and non-virtual methods can be called directly.
	// DotTypeExp means non-virtual

	if (obj_exp->op == TOKsuper ||
	    obj_type->ty == Tstruct || obj_type->ty == Tpointer ||
	    func->isFinal() || ! func->isVirtual() || is_dottype) {

	    if (obj_type->ty == Tstruct)
		this_expr = addressOf(this_expr);
	    return methodCallExpr(functionPointer(func), this_expr, d_type);
	} else {
	    // Interface methods are also in the class's vtable, so we don't
	    // need to convert from a class pointer to an interface pointer.
	    this_expr = maybeMakeTemp( this_expr );

	    tree vtbl_ref;
	    //#if D_GCC_VER >= 40
	    /* Folding of *&<static var> fails because of the type of the
	       address expression is 'Object' while the type of the static
	       var is a particular class (why?). This prevents gimplification
	       of the expression.
	    */
	    if (TREE_CODE(this_expr) == ADDR_EXPR /*&&
		// can't use this check
		TREE_TYPE(TREE_OPERAND(this_expr, 0)) ==
		TREE_TYPE(TREE_TYPE(this_expr))*/)
		vtbl_ref = TREE_OPERAND(this_expr, 0);
	    else
		//#endif
		vtbl_ref = indirect(this_expr);

	    tree field = TYPE_FIELDS( TREE_TYPE( vtbl_ref )); // the vtbl is the first field
	    //vtbl_ref = build2( COMPONENT_REF, TREE_TYPE( field ), vtbl_ref, field ); // vtbl field (a pointer)
	    vtbl_ref = component( vtbl_ref, field ); // vtbl field (a pointer)
	    // %% better to do with array ref?
	    vtbl_ref = pointerOffset(vtbl_ref,
		size_int( PTRSIZE * func->vtblIndex ));
	    vtbl_ref = indirect(vtbl_ref, TREE_TYPE( functionPointer(func) ));

	    return methodCallExpr(vtbl_ref, this_expr, d_type);
	}
    } else {
	// Static method; ignore the object instance
	return addressOf(func);
    }
}


tree
IRState::realPart(tree c) {
    return build1(REALPART_EXPR, TREE_TYPE(TREE_TYPE(c)), c);
}
tree
IRState::imagPart(tree c) {
    return build1(IMAGPART_EXPR, TREE_TYPE(TREE_TYPE(c)), c);
}

tree
IRState::assignValue(Expression * e, VarDeclaration * v)
{
    if (e->op == TOKassign || e->op == TOKconstruct || e->op == TOKblit)
    {
	AssignExp * a_exp = (AssignExp *) e;
	if (a_exp->e1->op == TOKvar && ((VarExp *) a_exp->e1)->var == v)
	{
	    tree a_val = convertForAssignment(a_exp->e2, v->type);
	    // Look for reference initializations
	    if (e->op == TOKconstruct && v->storage_class & (STCout | STCref))
		return addressOf(a_val);
	    else
		return a_val;
	}
	//else
	    //return e->toElem(this);
    }
    return NULL_TREE;
}


tree
IRState::twoFieldType(tree rec_type, tree ft1, tree ft2, Type * d_type, const char * n1, const char * n2)
{
    tree f0 = build_decl(FIELD_DECL, get_identifier(n1), ft1);
    tree f1 = build_decl(FIELD_DECL, get_identifier(n2), ft2);
    DECL_CONTEXT(f0) = rec_type;
    DECL_CONTEXT(f1) = rec_type;
    TYPE_FIELDS(rec_type) = chainon(f0, f1);
    layout_type(rec_type);
    if (d_type) {
	/* This is needed so that maybeExpandSpecialCall knows to
	   split dynamic array varargs. */
	TYPE_LANG_SPECIFIC( rec_type ) = build_d_type_lang_specific(d_type);

	/* ObjectFile::declareType will try to declare it as top-level type
	   which can break debugging info for element types. */
	tree stub_decl = build_decl(TYPE_DECL, get_identifier(d_type->toChars()), rec_type);
	TYPE_STUB_DECL(rec_type) = stub_decl;
	TYPE_NAME(rec_type) = stub_decl;
	DECL_ARTIFICIAL(stub_decl) = 1;
#if D_USE_MAPPED_LOCATION
	DECL_SOURCE_LOCATION(stub_decl) = BUILTINS_LOCATION;
	DECL_SOURCE_LOCATION(f0) = BUILTINS_LOCATION;
	DECL_SOURCE_LOCATION(f1) = BUILTINS_LOCATION;
#endif
	g.ofile->rodc(stub_decl, 0);
    }
    return rec_type;
}

// Create a record type from two field types
tree
IRState::twoFieldType(Type * ft1, Type * ft2, Type * d_type, const char * n1, const char * n2)
{
    return twoFieldType( make_node( RECORD_TYPE ), ft1->toCtype(), ft2->toCtype(), d_type, n1, n2 );
}

tree
IRState::twoFieldCtor(tree rec_type, tree f1, tree f2, int storage_class)
{
    tree ctor = make_node( CONSTRUCTOR );
    tree ft1, ft2;
    CtorEltMaker ce;

    TREE_TYPE( ctor ) = rec_type;
    TREE_STATIC( ctor ) = (storage_class & STCstatic) != 0;
    TREE_CONSTANT( ctor ) = (storage_class & STCconst) != 0;
    TREE_READONLY( ctor ) = (storage_class & STCconst) != 0;
    ft1 = TYPE_FIELDS( rec_type );
    ft2 = TREE_CHAIN( ft1 );

    ce.cons(ft1, f1);
    ce.cons(ft2, f2);
    CONSTRUCTOR_ELTS( ctor ) = ce.head;

    return ctor;
}

// This could be made more lax to allow better CSE (?)
bool
needs_temp(tree t) {
    // %%TODO: check for anything with TREE_SIDE_EFFECTS?
    switch (TREE_CODE(t)) {
    case VAR_DECL:
    case FUNCTION_DECL:
    case PARM_DECL:
    case CONST_DECL:
    case SAVE_EXPR:
	return false;

    case ADDR_EXPR:
#if D_GCC_VER < 40
    case REFERENCE_EXPR:
#endif
	/* This check is needed for 4.0.  Without it, typeinfo.methodCall may not be
	 */
	return ! (DECL_P(TREE_OPERAND(t, 0)));

    case INDIRECT_REF:
    case COMPONENT_REF:
    case NOP_EXPR:
    case NON_LVALUE_EXPR:
    case VIEW_CONVERT_EXPR:
	return needs_temp(TREE_OPERAND(t, 0));
    case ARRAY_REF:
	return true;
    default:
	if (
#if D_GCC_VER >= 40
	    TREE_CODE_CLASS(TREE_CODE(t)) == tcc_constant)
#else
	    TREE_CODE_CLASS(TREE_CODE(t)) == 'c')
#endif
	    return false;
	else
	    return true;
    }
}

bool
IRState::isFreeOfSideEffects(tree t)
{
    // SAVE_EXPR is safe to reference more than once, but not to
    // expand in a loop.
    return TREE_CODE(t) != SAVE_EXPR && ! needs_temp(t);
}

tree
IRState::maybeMakeTemp(tree t)
{
    if (needs_temp(t)) {
	if (TREE_CODE(TREE_TYPE(t)) != ARRAY_TYPE)
	    return save_expr(t);
	else
	    return stabilize_reference(t);
    } else
	return t;
}

Module * IRState::builtinsModule = 0;
Module * IRState::intrinsicModule = 0;
TemplateDeclaration * IRState::stdargTemplateDecl = 0;
TemplateDeclaration * IRState::cstdargStartTemplateDecl = 0;
TemplateDeclaration * IRState::cstdargArgTemplateDecl = 0;

bool
IRState::maybeSetUpBuiltin(Declaration * decl)
{
    Dsymbol * dsym;
    TemplateInstance * ti;

    // Don't use toParent2.  We are looking for a template below.
    dsym = decl->toParent();

    if (dsym->getModule() == intrinsicModule) {
	// Matches order of Intrinsic enum
	static const char * intrinsic_names[] = {
	    "bsf", "bsr",
	    "bt", "btc", "btr", "bts",
	    "bswap",
	    "inp", "inpw", "inpl",
	    "outp", "outw", "outl", NULL
	};
	for (int i = 0; intrinsic_names[i]; i++) {
	    if ( ! strcmp( decl->ident->string, intrinsic_names[i] ) ) {
		bool have_intrinsic = false;
		tree t = decl->toSymbol()->Stree;

		switch ( (Intrinsic) i ) {
		case INTRINSIC_BSF:
		case INTRINSIC_BSR:
		case INTRINSIC_BT:
		case INTRINSIC_BTC:
		case INTRINSIC_BTR:
		case INTRINSIC_BTS:
		    break;
		case INTRINSIC_BSWAP:
#if defined(TARGET_386)
		    //have_intrinsic = true;
#endif
		    break;
		case INTRINSIC_INP:
		case INTRINSIC_INPW:
		case INTRINSIC_INPL:
		case INTRINSIC_OUTP:
		case INTRINSIC_OUTPW:
		case INTRINSIC_OUTPL:
		    // Only on ix86, but need to given error message on others
		    have_intrinsic = true;
		    break;
		default:
		    abort();
		}

		if (have_intrinsic) {
		    DECL_BUILT_IN_CLASS( t ) = BUILT_IN_FRONTEND;
		    DECL_FUNCTION_CODE( t ) = (built_in_function) i;
		    return true;
		} else
		    return false;
	    }
	}
    } else if (dsym) {
	ti = dsym->isTemplateInstance();
	if (ti) {
	    tree t = decl->toSymbol()->Stree;
	    if (ti->tempdecl == stdargTemplateDecl) {
		DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
		DECL_FUNCTION_CODE(t) = (built_in_function) INTRINSIC_STD_VA_ARG;
		return true;
	    } else if (ti->tempdecl == cstdargArgTemplateDecl) {
		DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
		DECL_FUNCTION_CODE(t) = (built_in_function) INTRINSIC_C_VA_ARG;
		return true;
	    } else if (ti->tempdecl == cstdargStartTemplateDecl) {
		DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
		DECL_FUNCTION_CODE(t) = (built_in_function) INTRINSIC_C_VA_START;
		return true;
	    }
	}
    }
    return false;
}

bool
IRState::isDeclarationReferenceType(Declaration * decl)
{
    Type * base_type = decl->type->toBasetype();

    // D doesn't do this now..
    if ( base_type->ty == Treference ) {
	return true;
    }

    if (  decl->isOut() || decl->isRef() ||
	( decl->isParameter() && base_type->ty == Tsarray ) ) {
	return true;
    }

    return false;
}

tree
IRState::trueDeclarationType(Declaration * decl)
{
    // If D supported references, we would have to check twice for
    //   (out T &) -- disallow, maybe or make isDeclarationReferenceType return
    //   the number of levels to reference
    tree decl_type = decl->type->toCtype();
    if ( isDeclarationReferenceType( decl )) {
	return build_reference_type( decl_type );
    } else if (decl->storage_class & STClazy) {
	TypeFunction *tf = new TypeFunction(NULL, decl->type, 0, LINKd);
	TypeDelegate *t = new TypeDelegate(tf);
	return t->merge()->toCtype();
    } else {
	return decl_type;
    }
}

// These should match the Declaration versions above
bool
#if V2 //Until 2.037
IRState::isArgumentReferenceType(Argument * arg)
#else
IRState::isArgumentReferenceType(Parameter * arg)
#endif
{
    Type * base_type = arg->type->toBasetype();

    if ( base_type->ty == Treference ) {
	return true;
    }

    if ( (arg->storageClass & (STCout | STCref)) || base_type->ty == Tsarray ) {
	return true;
    }

    return false;
}

tree
#if V2 //Until 2.037
IRState::trueArgumentType(Argument * arg)
#else
IRState::trueArgumentType(Parameter * arg)
#endif
{
    tree arg_type = arg->type->toCtype();
    if ( isArgumentReferenceType( arg )) {
	return build_reference_type( arg_type );
    } else if (arg->storageClass & STClazy) {
	TypeFunction *tf = new TypeFunction(NULL, arg->type, 0, LINKd);
	TypeDelegate *t = new TypeDelegate(tf);
	return t->merge()->toCtype();
    } else {
	return arg_type;
    }
}

tree
IRState::arrayType(tree type_node, uinteger_t size)
{
    tree index_type_node;
    if (size > 0) {
	index_type_node = size_int( size - 1 );
	index_type_node = build_index_type(index_type_node);
    } else {
	// See c-decl.c grokdeclarator for zero-length arrays
	index_type_node = build_range_type (sizetype, size_zero_node,
	    NULL_TREE);
    }

    tree array_type = build_array_type(type_node, index_type_node);
    if (size == 0) {
#if D_GCC_VER < 40
	layout_type(array_type);
#endif
	TYPE_SIZE(array_type) = bitsize_zero_node;
	TYPE_SIZE_UNIT(array_type) = size_zero_node;
    }
    return array_type;
}

tree
IRState::addTypeAttribute(tree type, const char * attrname, tree value)
{
    // use build_type_copy / build_type_attribute_variant

    // types built by functions in tree.c need to be treated as immutable
    if ( ! TYPE_ATTRIBUTES( type )) { // ! TYPE_ATTRIBUTES -- need a better check
	type = build_type_copy( type );
	// TYPE_STUB_DECL( type ) = .. if we need this for structs, etc.. since
	// TREE_CHAIN is cleared by COPY_NODE
    }
    if (value)
	value = tree_cons(NULL_TREE, value, NULL_TREE);
    TYPE_ATTRIBUTES( type ) = tree_cons( get_identifier(attrname), value,
	TYPE_ATTRIBUTES( type ));
    return type;
}

void
IRState::addDeclAttribute(tree type, const char * attrname, tree value)
{
    if (value)
	value = tree_cons(NULL_TREE, value, NULL_TREE);
    DECL_ATTRIBUTES( type ) = tree_cons( get_identifier(attrname), value,
	DECL_ATTRIBUTES( type ));
}

tree
IRState::attributes(Expressions * in_attrs)
{
    if (! in_attrs)
	return NULL_TREE;

    ListMaker out_attrs;

    for (unsigned i = 0; i < in_attrs->dim; i++)
    {
	Expression * e = (Expression *) in_attrs->data[i];
	IdentifierExp * ident_e = NULL;

	ListMaker args;

	if (e->op == TOKidentifier)
	    ident_e = (IdentifierExp *) e;
	else if (e->op == TOKcall)
	{
	    CallExp * c = (CallExp *) e;
	    assert(c->e1->op == TOKidentifier);
	    ident_e = (IdentifierExp *) c->e1;

	    if (c->arguments) {
		for (unsigned ai = 0; ai < c->arguments->dim; ai++) {
		    Expression * ae = (Expression *) c->arguments->data[ai];
		    tree aet;
		    if (ae->op == TOKstring && ((StringExp *) ae)->sz == 1)
		    {
			StringExp * s = (StringExp *) ae;
			aet = build_string(s->len, (const char*) s->string);
		    }
		    else
			aet = ae->toElem(&gen);
		    args.cons(aet);
		}
	    }
	}
	else
	{
	    gcc_unreachable();
	    continue;
	}
	out_attrs.cons(get_identifier(ident_e->ident->string), args.head);
    }

    return out_attrs.head;
}

tree
IRState::integerConstant(xdmd_integer_t value, tree type)
{
    // The type is error_mark_node, we can't do anything.
    if (type == error_mark_node) {
	return type;
    }
#if D_GCC_VER < 40
    // Assuming xdmd_integer_t is 64 bits
# if HOST_BITS_PER_WIDE_INT == 32
    tree tree_value = build_int_2(value & 0xffffffff, (value >> 32) & 0xffffffff);
# elif HOST_BITS_PER_WIDE_INT == 64
    tree tree_value = build_int_2(value,
	type && ! TYPE_UNSIGNED(type) && (value & 0x8000000000000000ULL) ?
	~(unsigned HOST_WIDE_INT) 0 : 0);
# else
#  error Fix This
# endif
    if (type) {
	TREE_TYPE( tree_value ) = type;
	// May not to call force_fit_type for 3.3.x and 3.4.x, but being safe.
	force_fit_type(tree_value, 0);
    }
#else
# if HOST_BITS_PER_WIDE_INT == 32
#  if D_GCC_VER >= 43
    tree tree_value = build_int_cst_wide_type(type, value & 0xffffffff,
					      (value >> 32) & 0xffffffff);
#  else
    tree tree_value = build_int_cst_wide(type, value & 0xffffffff,
					 (value >> 32) & 0xffffffff);
#  endif
# elif HOST_BITS_PER_WIDE_INT == 64
    tree tree_value = build_int_cst_type(type, value);
# else
#  error Fix This
# endif

# if D_GCC_VER < 43
    /* VALUE may be an incorrect representation for TYPE.  Example:
       uint x = cast(uint) -3; // becomes "-3u" -- value=0xfffffffffffffd type=Tuns32
       Constant folding will not work correctly unless this is done. */
    tree_value = force_fit_type(tree_value, 0, 0, 0);
#endif

#endif
    return tree_value;
}

tree
IRState::exceptionObject()
{
    tree obj_type = getObjectType()->toCtype();
    // Like gjc, the actual D exception object is one
    // pointer behind the exception header
    tree t = build0 (EXC_PTR_EXPR, ptr_type_node);
    t = build1(NOP_EXPR, build_pointer_type(obj_type), t); // treat exception header as ( Object* )
    //t = build2(MINUS_EXPR, TREE_TYPE(t), t, TYPE_SIZE_UNIT(TREE_TYPE(t)));
    t = pointerOffsetOp(MINUS_EXPR, t, TYPE_SIZE_UNIT(TREE_TYPE(t)));
    t = build1(INDIRECT_REF, obj_type, t);
    return t;
}

tree
IRState::label(Loc loc, Identifier * ident) {
    tree t_label = build_decl(LABEL_DECL,
	ident ? get_identifier(ident->string) : NULL_TREE, void_type_node);
    DECL_CONTEXT( t_label ) = current_function_decl;
    DECL_MODE( t_label ) = VOIDmode;
    if (loc.filename)
	g.ofile->setDeclLoc(t_label, loc);
    return t_label;
}

tree
IRState::getFrameForFunction(FuncDeclaration * f)
{
    if (f->fbody)
	return getFrameForSymbol(f);
    else
    {
	// Should error on line that references f
	f->error("nested function missing body");
	return d_null_pointer;
    }
}
tree
IRState::getFrameForNestedClass(ClassDeclaration *c)
{
    return getFrameForSymbol(c);
}

/* If nested_sym is a nested function, return the static chain to be
   used when invoking that function.

   If nested_sym is a nested class, return the static chain to be used
   when creating an instance of the class.

   This method is protected to enforce the type checking of
   getFrameForFunction and getFrameForNestedClass.
   getFrameForFunction also checks that the nestd function is properly
   defined.
*/

tree
IRState::getFrameForSymbol(Dsymbol * nested_sym)
{
    FuncDeclaration * nested_func = 0;
    FuncDeclaration * outer_func = 0;

    if ( (nested_func = nested_sym->isFuncDeclaration()) )
    {
	// gcc_assert(nested_func->isNested())
	outer_func = nested_func->toParent2()->isFuncDeclaration();
	gcc_assert(outer_func != NULL);

	if (func != outer_func)
	{
	    Dsymbol * this_func = func;
	    Dsymbol * parent_sym = nested_sym->toParent2();
	    if (!func->vthis) // if no frame pointer for this function
	    {
		nested_func->error("is a nested function and cannot be accessed from %s", func->toChars());
		return d_null_pointer;
	    }
	    /* Search for frame pointer, make sure we can reach it,
	       else we'll ICE later in tree-ssa.  */
	    while (nested_func != this_func)
	    {
		FuncDeclaration * fndecl;
		if ( (fndecl = this_func->isFuncDeclaration()) )
		{
		    if (parent_sym == fndecl->toParent2())
			break;
		    assert(fndecl->isNested() || fndecl->vthis);
		}
		else
		{
		    func->error("cannot get frame pointer to %s", nested_sym->toChars());
		    return d_null_pointer;
		}
		this_func = this_func->toParent2();
	    }
	}
    }
    else
    {
	/* It's a class.  NewExp::toElem has already determined its
	   outer scope is not another class, so it must be a
	   function. */

	Dsymbol * sym = nested_sym;

	while (sym && ! (outer_func = sym->isFuncDeclaration()))
	    sym = sym->toParent2();

	/* Make sure we can access the frame of outer_func.

	   For GCC < 4.0:

	    f() {
	      class X {
		m() { }
	      }
	      g() {
		h() {
		   new X // <-- you are here
		}
	      }
	    }

	   In order to get the static chain we must know a
	   function nested in F.  If we are currently in such a
	   nested function, use that.

	   f() {
	       class X { m() { } }
	       new X // <-- you are here
	   }

	   If we are at the level of the function containing the
	   class, the answer is just 'virtual_stack_vars_rtx'.
	*/

	if (outer_func != func) {

	    Dsymbol * o = nested_func = func;
	    do {
		if (! nested_func->isNested())
		    goto cannot_access_frame;
		while ( (o = o->toParent2()) )
		    if ( (nested_func = o->isFuncDeclaration()) )
			break;
	    } while (o && o != outer_func);

	    if (! o) {
	    cannot_access_frame:
		error("cannot access frame of function '%s' from '%s'",
		      outer_func->toChars(), func->toChars());
		return d_null_pointer;
	    }
	}
	// else, the answer is 'virtual_stack_vars_rtx'
    }

#if V2
    if (getFrameInfo(outer_func)->creates_closure)
	return getClosureRef(outer_func);
#endif

#if D_GCC_VER < 40
    tree result = make_node (RTL_EXPR);
    TREE_TYPE (result) = ptr_type_node;
    RTL_EXPR_RTL (result) = nested_func ?
	lookup_static_chain(nested_func->toSymbol()->Stree) :
	virtual_stack_vars_rtx;
    return result;
#else
    if (! outer_func)
	outer_func = nested_func->toParent2()->isFuncDeclaration();
    gcc_assert(outer_func != NULL);
    return build1(STATIC_CHAIN_EXPR, ptr_type_node, outer_func->toSymbol()->Stree);
#endif
}

#if V2

/* For the purposes this is used, fd is assumed to be a nested
   function or a method of a class that is (eventually) nested in a
   function.
*/
static bool
isFuncNestedInFunc(FuncDeclaration * fd, FuncDeclaration *fo)
{
    if (fd == fo)
	return false;

    while (fd)
    {
	AggregateDeclaration * ad;
	ClassDeclaration * cd;

	if (fd == fo)
	{
	    //fprintf(stderr, "%s is nested in %s\n", fd->toChars(), fo->toChars());
	    return true;
	}
	else if (fd->isNested())
	    fd = fd->toParent2()->isFuncDeclaration();
	else if ( (ad = fd->isThis()) && (cd = ad->isClassDeclaration()) )
	{
	    fd = NULL;
	    while (cd && cd->isNested())
	    {
		Dsymbol * dsym = cd->toParent2();
		if ( (fd = dsym->isFuncDeclaration()) )
		    break;
		else
		    cd = dsym->isClassDeclaration();
	    }
	}
	else
	    break;
    }

    //fprintf(stderr, "%s is NOT nested in %s\n", fd->toChars(), fo->toChars());
    return false;
}

FuncFrameInfo *
IRState::getFrameInfo(FuncDeclaration *fd)
{
    Symbol * fds = fd->toSymbol();
    if (fds->frameInfo)
	return fds->frameInfo;

    FuncFrameInfo * ffi = new FuncFrameInfo;
    ffi->creates_closure = false;
    ffi->closure_rec = NULL_TREE;

    fds->frameInfo = ffi;

    Dsymbol * s = fd->toParent2();

    if (fd->needsClosure())
	ffi->creates_closure = true;
    else
    {
	/* If fd is nested (deeply) in a function 'g' that creates a
	   closure and there exists a function 'h' nested (deeply) in
	   fd and 'h' accesses the frame of 'g', then fd must also
	   create a closure.

	   This is for the sake of a simple implementation.  An alternative
	   is, when determining the frame to pass to 'h', pass the pointer
	   to 'g' (the deepest 'g' whose frame is accessed by 'h') instead
	   of the usual frame that 'h' would take.
	*/
	FuncDeclaration * ff = fd;

	while (ff)
	{
	    AggregateDeclaration * ad;
	    ClassDeclaration * cd;

	    if (ff != fd)
	    {
		if (getFrameInfo(ff)->creates_closure)
		{
		    for (int i = 0; i < ff->closureVars.dim; i++)
		    {   VarDeclaration *v = (VarDeclaration *)ff->closureVars.data[i];
			for (int j = 0; j < v->nestedrefs.dim; j++)
			{   FuncDeclaration *fi = (FuncDeclaration *)v->nestedrefs.data[j];
			    if (isFuncNestedInFunc(fi, fd))
			    {
				ffi->creates_closure = true;
				goto L_done;
			    }
			}
		    }
		}
	    }

	    if (ff->isNested())
		ff = ff->toParent2()->isFuncDeclaration();
	    else if ( (ad = ff->isThis()) && (cd = ad->isClassDeclaration()) )
	    {
		ff = NULL;
		while (cd && cd->isNested())
		{
		    Dsymbol * dsym = cd->toParent2();
		    if ( (ff = dsym->isFuncDeclaration()) )
			break;
		    else
			cd = dsym->isClassDeclaration();
		}
	    }
	    else
		break;
	}

	L_done:
	    ;
    }

    /*fprintf(stderr, "%s  %s\n", ffi->creates_closure ? "YES" : "NO ",
      fd->toChars());*/

    return ffi;
}

// Return a pointer to the closure block of outer_func
tree
IRState::getClosureRef(FuncDeclaration * outer_func)
{
    tree result = closureLink();
    FuncDeclaration * fd = closureFunc;

    while (fd && fd != outer_func)
    {
	AggregateDeclaration * ad;
	ClassDeclaration * cd;

	gcc_assert(getFrameInfo(fd)->creates_closure); // remove this if we loosen creates_closure

	// like compon(indirect, field0) parent closure link is the first field;
	result = indirect(result, ptr_type_node);

	if (fd->isNested())
	{
	    fd = fd->toParent2()->isFuncDeclaration();
	}
	else if ( (ad = fd->isThis()) && (cd = ad->isClassDeclaration()) )
	{
	    fd = NULL;
	    while (cd && cd->isNested())
	    {
		/* Shouldn't need to do this.  getClosureRef is only
		   used to get the pointer to a function's frame (not
		   a class instances.)  With the current implementation,
		   the link the closure record always points to the
		   outer function's frame even if there are intervening
		   nested classes.  So, we can just skip over those... */
		/*
		tree vthis_field = cd->vthis->toSymbol()->Stree;
		result = nop(result, cd->type->toCtype());
		result = component(indirect(result), vthis_field);
		*/

		Dsymbol * dsym = cd->toParent2();
		if ( (fd = dsym->isFuncDeclaration()) )
		    break;
		else
		    cd = dsym->isClassDeclaration();
	    }
	}
	else
	    break;
    }

    if (fd == outer_func)
    {
	tree closure_rec = getFrameInfo(outer_func)->closure_rec;
	result = nop(result, build_pointer_type(closure_rec));
	return result;
    }
    else
    {
	func->error("cannot access frame of %s", outer_func->toChars());
	return d_null_pointer;
    }
}

#endif

/* Return true if function F needs to have the static chain passed to
   it.  This only applies to nested function handling provided by the
   GCC back end (not D closures.)
*/
bool
IRState::functionNeedsChain(FuncDeclaration *f)
{
    Dsymbol * s;
    ClassDeclaration * a;
    FuncDeclaration *pf = 0;

    if (f->isNested()
#if V2
	&& ! getFrameInfo(f->toParent2()->isFuncDeclaration())->creates_closure
#endif
	)
	return true;
    if (f->isStatic())
	return false;

    s = f->toParent2();

    while ( s && (a = s->isClassDeclaration()) && a->isNested() ) {
	s = s->toParent2();
	if ( (pf = s->isFuncDeclaration())
#if V2
	    && ! getFrameInfo(pf)->creates_closure
#endif
	     )
	    return true;
    }
    return false;
}

 tree
IRState::toElemLvalue(Expression * e)
{
    /*
    if (e->op == TOKcast)
	fprintf(stderr, "IRState::toElemLvalue TOKcast\n");
    else
    */

    if (e->op == TOKindex) {
	IndexExp * ie = (IndexExp *) e;
	Expression * e1 = ie->e1;
	Expression * e2 = ie->e2;
	Type * type = e->type;
	Type * array_type = e1->type->toBasetype();

	if (array_type->ty == Taarray) {
	    Type * key_type = ((TypeAArray *) array_type)->index->toBasetype();
	    AddrOfExpr aoe;

	    tree args[4];
	    args[0] = this->addressOf( this->toElemLvalue(e1) );
	    args[1] = this->typeinfoReference(key_type);
	    args[2] = this->integerConstant( array_type->nextOf()->size(), Type::tsize_t );
	    args[3] = aoe.set(this, this->convertTo( e2, key_type ));
	    return build1(INDIRECT_REF, type->toCtype(),
		aoe.finish(this,
		    this->libCall(LIBCALL_AAGETP, 4, args, type->pointerTo()->toCtype())));
	}
    }
    return e->toElem(this);
}


#if D_GCC_VER < 40

void
IRState::startCond(Statement * stmt, Expression * e_cond) {
    clear_last_expr ();
    g.ofile->doLineNote(stmt->loc);
    expand_start_cond( convertForCondition( e_cond ), 0 );
}

void
IRState::startElse() { expand_start_else(); }

void
IRState::endCond() { expand_end_cond(); }

void
IRState::startLoop(Statement * stmt) {
    beginFlow(stmt, expand_start_loop_continue_elsewhere(1));
}

void
IRState::continueHere()
{
    Flow * f = currentFlow();
    if (f->overrideContinueLabel)
	doLabel(f->overrideContinueLabel);
    else
	expand_loop_continue_here();
}

void
IRState::setContinueLabel(tree lbl)
{
    currentFlow()->overrideContinueLabel = lbl;
}

void
IRState::exitIfFalse(tree t_cond, bool is_top_cond) {

    // %% topcond compaitble with continue_elswehre?
    expand_exit_loop_if_false(currentFlow()->loop, t_cond);
}

void
IRState::startCase(Statement * stmt, tree t_cond)
{
    clear_last_expr ();
    g.ofile->doLineNote(stmt->loc);
    expand_start_case( 1, t_cond, TREE_TYPE( t_cond ), "switch statement" );
    Flow * f = beginFlow(stmt, NULL);
    f->kind = level_switch;
}

void
IRState::doCase(tree t_value, tree t_label)
{
    tree dummy;
    // %% not the same convert that is in d-glue!!
    pushcase( t_value, convert, t_label, & dummy);
}

void
IRState::endCase(tree t_cond)
{
    expand_end_case( t_cond );
    endFlow();
}

void
IRState::endLoop() {
    expand_end_loop();
    endFlow();
}

void
IRState::continueLoop(Identifier * ident) {
    Flow * f = getLoopForLabel( ident, true );
    if (f->overrideContinueLabel)
	doJump(NULL, f->overrideContinueLabel);
    else
	expand_continue_loop( f->loop  );
}

void
IRState::exitLoop(Identifier * ident)
{
    if (ident) {
	Flow * flow = getLoopForLabel( ident );
	if (flow->loop)
	    expand_exit_loop( flow->loop );
	else {
	    if (! flow->exitLabel)
		flow->exitLabel = label(flow->statement->loc);
	    expand_goto( flow->exitLabel );
	}
    } else {
	expand_exit_something();
    }
}

void
IRState::startTry(Statement * stmt)
{
    beginFlow(stmt, NULL);
    currentFlow()->kind = level_try;
    expand_eh_region_start();
}

void
IRState::startCatches()
{
    currentFlow()->kind = level_catch;
    expand_start_all_catch();
}

void
IRState::startCatch(tree t_type)
{
    expand_start_catch( t_type );
}

void
IRState::endCatch()
{
    expand_end_catch();
}

void
IRState::endCatches()
{
    expand_end_all_catch();
    endFlow();
}

void
IRState::startFinally()
{
    abort();
}

void
IRState::endFinally()
{
    abort();
}

void
IRState::doReturn(tree t_value)
{
    if (t_value)
	expand_return(t_value);
    else
	expand_null_return();
}

void
IRState::doJump(Statement * stmt, tree t_label)
{
    // %%TODO: c-semantics.c:expand_stmt GOTO_STMT branch prediction
    if (stmt)
	g.ofile->doLineNote(stmt->loc);
    expand_goto(t_label);
    D_LABEL_IS_USED(t_label) = 1;
}

tree
IRState::makeStmtExpr(Statement * statement)
{
    tree t = build1((enum tree_code) D_STMT_EXPR, void_type_node, NULL_TREE);
    TREE_SIDE_EFFECTS(t) = 1; // %%
    stmtExprList.push(t);
    stmtExprList.push(statement);
    stmtExprList.push(this);
    return t;
}

void
IRState::retrieveStmtExpr(tree t, Statement ** s_out, IRState ** i_out)
{
    for (int i = stmtExprList.dim - 3; i >= 0 ; i -= 3) {
	if ( (tree) stmtExprList.data[i] == t ) {
	    *s_out = (Statement *) stmtExprList.data[i + 1];
	    *i_out = (IRState *)   stmtExprList.data[i + 2];
	    // The expression could be evaluated multiples times, so we must
	    // keep the values forever --- %% go back to per-function list
	    return;
	}
    }
    abort();
    return;
}

#else

void
IRState::startCond(Statement * stmt, Expression * e_cond)
{
    tree t_cond = convertForCondition(e_cond);

    Flow * f = beginFlow(stmt);
    f->condition = t_cond;
}

void
IRState::startElse()
{
    currentFlow()->trueBranch = popStatementList();
    pushStatementList();
}

void
IRState::endCond()
{
    Flow * f = currentFlow();
    tree t_brnch = popStatementList(); // endFlow(); -- can't pop -- need the info?
    tree t_false_brnch = NULL_TREE;

    if (f->trueBranch == NULL_TREE)
	f->trueBranch = t_brnch;
    else
	t_false_brnch = t_brnch;

    g.ofile->doLineNote(f->statement->loc);
    tree t_stmt = build3(COND_EXPR, void_type_node,
	f->condition, f->trueBranch, t_false_brnch);
    endFlow();
    addExp(t_stmt);
}

void
IRState::startLoop(Statement * stmt) {
    Flow * f = beginFlow(stmt);
    f->continueLabel = label(stmt ? stmt->loc : 0); // should be end for 'do' loop
}

void
IRState::continueHere() {
    doLabel(currentFlow()->continueLabel);
}

void
IRState::setContinueLabel(tree lbl)
{
    currentFlow()->continueLabel = lbl;
}

void
IRState::exitIfFalse(tree t_cond, bool /*unused*/) {
    addExp(build1(EXIT_EXPR, void_type_node,
	       build1(TRUTH_NOT_EXPR, TREE_TYPE(t_cond), t_cond)));
}

void
IRState::startCase(Statement * stmt, tree t_cond)
{
    Flow * f = beginFlow(stmt);
    f->condition = t_cond;
    f->kind = level_switch;
}

void
IRState::doCase(tree t_value, tree t_label)
{
    addExp(build3(CASE_LABEL_EXPR, void_type_node,
	       t_value, NULL_TREE, t_label));
}

void
IRState::endCase(tree /*t_cond*/)
{
    Flow * f = currentFlow();
    tree t_body = popStatementList();
    tree t_stmt = build3(SWITCH_EXPR, void_type_node, f->condition,
	t_body, NULL_TREE);
    addExp(t_stmt);
    endFlow();
}

void
IRState::endLoop() {
    // says must contain an EXIT_EXPR -- what about while(1)..goto;? something other thand LOOP_EXPR?
    tree t_body = popStatementList();
    tree t_loop = build1(LOOP_EXPR, void_type_node, t_body);
    addExp(t_loop);
    endFlow();
}

void
IRState::continueLoop(Identifier * ident) {
    //doFlowLabel(stmt, ident, Continue);
    doJump(NULL, getLoopForLabel(ident, true)->continueLabel );
}

void
IRState::exitLoop(Identifier * ident) {
    Flow * flow = getLoopForLabel(ident);
    if (! flow->exitLabel)
	flow->exitLabel = label(flow->statement->loc);
    doJump(NULL, flow->exitLabel);
}


void
IRState::startTry(Statement * stmt)
{
    beginFlow(stmt);
    currentFlow()->kind = level_try;
}

void
IRState::startCatches()
{
    currentFlow()->tryBody = popStatementList();
    currentFlow()->kind = level_catch;
    pushStatementList();
}

void
IRState::startCatch(tree t_type)
{
    currentFlow()->catchType = t_type;
    pushStatementList();
}

void
IRState::endCatch()
{
    tree t_body = popStatementList();
    // % wrong loc.. can set pass statement to startCatch, set
    // the loc on t_type and then use it here..
    // may not be important?
    doExp(build2(CATCH_EXPR, void_type_node,
	      currentFlow()->catchType, t_body));
}

void
IRState::endCatches()
{
    tree t_catches = popStatementList();
    g.ofile->doLineNote(currentFlow()->statement->loc);
    doExp(build2(TRY_CATCH_EXPR, void_type_node,
	      currentFlow()->tryBody, t_catches));
    endFlow();
}

void
IRState::startFinally()
{
    currentFlow()->tryBody = popStatementList();
    currentFlow()->kind = level_finally;
    pushStatementList();
}

void
IRState::endFinally()
{
    tree t_finally = popStatementList();
    g.ofile->doLineNote(currentFlow()->statement->loc);
    doExp(build2(TRY_FINALLY_EXPR, void_type_node,
	      currentFlow()->tryBody, t_finally));
    endFlow();
}

void
IRState::doReturn(tree t_value)
{
    addExp(build1(RETURN_EXPR, void_type_node, t_value));
}

void
IRState::doJump(Statement * stmt, tree t_label)
{
    if (stmt)
	g.ofile->doLineNote(stmt->loc);
    addExp(build1(GOTO_EXPR, void_type_node, t_label));
    D_LABEL_IS_USED(t_label) = 1;
}

tree
IRState::makeStmtExpr(Statement * statement)
{
    tree t_list;

    pushStatementList();
    statement->toIR(this);
    t_list = popStatementList();

    // addExp(t_list);

    return t_list;
}

#endif

void
IRState::doAsm(tree insn_tmpl, tree outputs, tree inputs, tree clobbers)
{
#if D_GCC_VER < 40
    expand_asm_operands(insn_tmpl, outputs, inputs, clobbers, 1, input_location);
#else
    tree t = d_build_asm_stmt(insn_tmpl, outputs, inputs, clobbers);
    ASM_VOLATILE_P( t ) = 1;
    addExp( t );
#endif
}


void
IRState::checkSwitchCase(Statement * stmt, int default_flag)
{
    Flow * flow = currentFlow();

    assert(flow);
    if (flow->kind != level_switch && flow->kind != level_block)
    {
	stmt->error("%s cannot be in different try block level from switch",
		    default_flag ? "default" : "case");
    }
}

void
IRState::checkGoto(Statement * stmt, LabelDsymbol * label)
{
    Statement * curBlock = NULL;
    unsigned curLevel = loops.dim;
    int found = 0;

    if (curLevel)
	curBlock = currentFlow()->statement;

    for (unsigned i = 0; i < Labels.dim; i++)
    {
	Label * linfo = (Label *)Labels.data[i];
	assert(linfo);

	if (label == linfo->label)
	{
	    // No need checking for finally, should have already been handled.
	    if (linfo->kind == level_try &&
		curLevel <= linfo->level && curBlock != linfo->block)
	    {
		stmt->error("cannot goto into try block");
	    }
	    // %% doc: It is illegal for goto to be used to skip initializations,
	    // %%      so this should include all gotos into catches...
	    if (linfo->kind == level_catch && curBlock != linfo->block)
		stmt->error("cannot goto into catch block");

	    found = 1;
	    break;
	}
    }
#if V1
    // Push forward referenced gotos.
    if (! found)
    {
	if (! label->statement->fwdrefs)
	    label->statement->fwdrefs = new Array();
	label->statement->fwdrefs->push(getLabelBlock(label, stmt));
    }
#endif
}

void
IRState::checkPreviousGoto(Array * refs)
{
    Statement * stmt; // Our forward reference.

    for (unsigned i = 0; i < refs->dim; i++)
    {
	Label * ref = (Label *)refs->data[i];
	int found = 0;

	assert(ref && ref->from);
	stmt = ref->from;

	for (unsigned i = 0; i < Labels.dim; i++)
	{
	    Label * linfo = (Label *)Labels.data[i];
	    assert(linfo);

	    if (ref->label == linfo->label)
	    {
		// No need checking for finally, should have already been handled.
		if (linfo->kind == level_try &&
		    ref->level <= linfo->level && ref->block != linfo->block)
		{
		    stmt->error("cannot goto into try block");
		}
		// %% doc: It is illegal for goto to be used to skip initializations,
		// %%      so this should include all gotos into catches...
		if (linfo->kind == level_catch &&
		    (ref->block != linfo->block || ref->kind != linfo->kind))
		    stmt->error("cannot goto into catch block");

		found = 1;
		break;
	    }
	}
	assert(found);
    }
}


WrappedExp::WrappedExp(Loc loc, enum TOK op, tree exp_node, Type * type)
    : Expression(loc, op, sizeof(WrappedExp))
{
    this->exp_node = exp_node;
    this->type = type;
}

void
WrappedExp::toCBuffer(OutBuffer *buf)
{
    buf->printf("< wrapped exprission >");
}

elem *
WrappedExp::toElem(IRState *)
{
    return exp_node;
}


void AggLayout::doFields(Array * fields, AggregateDeclaration * agg)
{
    bool inherited = agg != this->aggDecl;
    tree fcontext;

    fcontext = agg->type->toCtype();
    if ( POINTER_TYPE_P( fcontext ))
	fcontext = TREE_TYPE(fcontext);

    // tree new_field_chain = NULL_TREE;
    for (unsigned i = 0; i < fields->dim; i++) {
	// %% D anonymous unions just put the fields into the outer struct...
	// does this cause problems?

	VarDeclaration * var_decl = (VarDeclaration *) fields->data[i];

	assert( var_decl->storage_class & STCfield );

	tree ident = var_decl->ident ? get_identifier(var_decl->ident->string) : NULL_TREE;
	tree field_decl = build_decl(FIELD_DECL, ident,
	    gen.trueDeclarationType( var_decl ));
	g.ofile->setDeclLoc( field_decl, var_decl );
	var_decl->csym = new Symbol;
	var_decl->csym->Stree = field_decl;

	DECL_CONTEXT( field_decl ) = aggType;
	DECL_FCONTEXT( field_decl ) = fcontext;
	DECL_FIELD_OFFSET (field_decl) = size_int( var_decl->offset );
	DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;

	DECL_ARTIFICIAL( field_decl ) =
	    DECL_IGNORED_P( field_decl ) = inherited;


	// GCC 4.0 requires DECL_OFFSET_ALIGN to be set
	// %% .. using TYPE_ALIGN may not be same as DMD..
	SET_DECL_OFFSET_ALIGN( field_decl,
	    TYPE_ALIGN( TREE_TYPE( field_decl )));

	//SET_DECL_OFFSET_ALIGN (field_decl, BIGGEST_ALIGNMENT); // ?
	layout_decl( field_decl, 0 );

	// get_inner_reference doesn't check these, leaves a variable unitialized
	// DECL_SIZE is NULL if size is zero.
	if (var_decl->size(var_decl->loc)) {
	    assert(DECL_MODE(field_decl) != VOIDmode);
	    assert(DECL_SIZE(field_decl) != NULL_TREE);
	}
	fieldList.chain( field_decl );
    }
}

void AggLayout::doInterfaces(Array * bases, AggregateDeclaration * /*agg*/)
{
    //tree fcontext = TREE_TYPE( agg->type->toCtype() );
    for (unsigned i = 0; i < bases->dim; i++) {
	BaseClass * bc = (BaseClass *) bases->data[i];
	tree decl = build_decl(FIELD_DECL, NULL_TREE,
	    Type::tvoid->pointerTo()->pointerTo()->toCtype() /* %% better */ );
	//DECL_VIRTUAL_P( decl ) = 1; %% nobody cares, boo hoo
	DECL_ARTIFICIAL( decl ) =
	    DECL_IGNORED_P( decl ) = 1;
	// DECL_FCONTEXT( decl ) = fcontext; // shouldn't be needed since it's ignored
	addField(decl, bc->offset);
    }
}

void AggLayout::addField(tree field_decl, target_size_t offset)
{
    DECL_CONTEXT( field_decl ) = aggType;
    // DECL_FCONTEXT( field_decl ) = aggType; // caller needs to set this
    SET_DECL_OFFSET_ALIGN( field_decl,
	TYPE_ALIGN( TREE_TYPE( field_decl )));
    DECL_FIELD_OFFSET (field_decl) = size_int( offset );
    DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_int( 0 );
    Loc l(aggDecl->getModule(), 1); // Must set this or we crash with DWARF debugging
    // gen.setDeclLoc( field_decl, aggDecl->loc ); // aggDecl->loc isn't set
    g.ofile->setDeclLoc(field_decl, l);

    layout_decl( field_decl, 0 );
    fieldList.chain( field_decl );
}

void AggLayout::finish(Expressions * attrs)
{
    unsigned size_to_use = aggDecl->structsize;
    unsigned align_to_use = aggDecl->alignsize;

    /* probably doesn't do anything */
    /*
    if (aggDecl->structsize == 0 && aggDecl->isInterfaceDeclaration())
	size_to_use = Type::tvoid->pointerTo()->size();
    */

    TYPE_SIZE( aggType ) = bitsize_int( size_to_use * BITS_PER_UNIT );
    TYPE_SIZE_UNIT( aggType ) = size_int( size_to_use );
    TYPE_ALIGN( aggType ) = align_to_use * BITS_PER_UNIT;
    // TYPE_ALIGN_UNIT is not an lvalue
    TYPE_PACKED (aggType ) = TYPE_PACKED (aggType ); // %% todo

    if (attrs)
	decl_attributes(& aggType, gen.attributes(attrs),
	    ATTR_FLAG_TYPE_IN_PLACE);

    compute_record_mode ( aggType );
    // %%  stor-layout.c:finalize_type_size ... it's private to that file

    // c-decl.c -- set up variants ? %%
    for (tree x = TYPE_MAIN_VARIANT( aggType ); x; x = TYPE_NEXT_VARIANT( x )) {
	TYPE_FIELDS (x) = TYPE_FIELDS (aggType);
	TYPE_LANG_SPECIFIC (x) = TYPE_LANG_SPECIFIC (aggType);
	TYPE_ALIGN (x) = TYPE_ALIGN (aggType);
	TYPE_USER_ALIGN (x) = TYPE_USER_ALIGN (aggType);
    }
}

ArrayScope::ArrayScope(IRState * ini_irs, VarDeclaration * ini_v, const Loc & loc) :
    v(ini_v), irs(ini_irs)
{
    if (v) {
	/* Need to set the location or the expand_decl in the BIND_EXPR will
	   cause the line numbering for the statement to be incorrect. */
	/* The variable itself is not included in the debugging information. */
	v->loc = loc;
	Symbol * s = v->toSymbol();
	tree decl = s->Stree;
	DECL_CONTEXT( decl ) = irs->getLocalContext();
    }
}

tree
ArrayScope::setArrayExp(tree e, Type * t)
{
    /* If STCconst, the value will be assigned in d-decls.cc
       of the runtime length of the array expression. */
    if (v && ! (v->storage_class & STCconst))
    {
	if (t->toBasetype()->ty != Tsarray)  // %%
	    e = irs->maybeMakeTemp(e);
	DECL_INITIAL(v->toSymbol()->Stree) = irs->arrayLength(e, t);
    }
    return e;
}

tree
ArrayScope::finish(tree e)
{
    if (v)
    {
	Symbol * s = v->toSymbol();
	tree t = s->Stree;
	if (TREE_CODE(t) == VAR_DECL)
	{
#if V2
	    if (s->SclosureField)
		return irs->compound( irs->vmodify(irs->var(v),
			DECL_INITIAL(t) ), e);
	    else
#endif
	    return gen.binding(v->toSymbol()->Stree, e);
	}
    }

    return e;
}

void
FieldVisitor::visit(AggregateDeclaration * decl)
{
    ClassDeclaration * class_decl = decl->isClassDeclaration();

    if (class_decl && class_decl->baseClass)
	FieldVisitor::visit(class_decl->baseClass);

    doFields(& decl->fields, decl);

    if (class_decl && class_decl->vtblInterfaces)
	doInterfaces(class_decl->vtblInterfaces, decl);
}
