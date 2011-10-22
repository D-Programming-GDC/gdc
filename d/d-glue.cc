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
#include "id.h"
#include "enum.h"
#include "module.h"
#include "init.h"
#include "symbol.h"
#include "d-lang.h"
#include "d-codegen.h"

// see d-convert.cc
tree
convert (tree type, tree expr)
{
    // Check this first before passing to getDType.
    if (g.irs->isErrorMark(type) || g.irs->isErrorMark(TREE_TYPE(expr)))
        return error_mark_node;

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
    tree cn = irs->convertForCondition(econd);
#if V2
    tree t1 = irs->convertTo(e1->toElemDtor(irs), e1->type, type);
    tree t2 = irs->convertTo(e2->toElemDtor(irs), e2->type, type);
#else
    tree t1 = irs->convertTo(e1, type);
    tree t2 = irs->convertTo(e2, type);
#endif
    return build3(COND_EXPR, type->toCtype(), cn, t1, t2);
}

static tree
build_bool_binop(TOK op, tree type, Expression * e1, Expression * e2, IRState * irs)
{
    tree_code out_code;
    tree t1, t2;

    if (op == TOKandand || op == TOKoror)
    {
        t1 = irs->convertForCondition(e1->toElem(irs), e1->type);
#if V2
        t2 = irs->convertForCondition(e2->toElemDtor(irs), e2->type);
#else
        t2 = irs->convertForCondition(e2->toElem(irs), e2->type);
#endif
    }
    else
    {
        t1 = e1->toElem(irs);
        t2 = e2->toElem(irs);
    }

    if (COMPLEX_FLOAT_TYPE_P(type))
    {   // All other operations are not defined for complex types.
        gcc_assert(op == TOKidentity || op == TOKequal ||
                   op == TOKnotidentity || op == TOKnotequal ||
                   op == TOKandand || op == TOKoror);

        // GCC doesn't handle these.
        // ordering for complex isn't defined, all that is guaranteed is the 'unordered part'
        t1 = irs->maybeMakeTemp(t1);
        t2 = irs->maybeMakeTemp(t2);
    }

    switch (op)
    {
        case TOKidentity:
        case TOKequal:
            out_code = EQ_EXPR;
            break;

        case TOKnotidentity:
        case TOKnotequal:
            out_code = NE_EXPR;
            break;

        case TOKandand:
            out_code = TRUTH_ANDIF_EXPR;
            break;

        case TOKoror:
            out_code = TRUTH_ORIF_EXPR;
            break;

        case TOKleg:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = ORDERED_EXPR;
            else
                // %% is this properly optimized away?
                return convert(boolean_type_node, integer_one_node);
            break;

        case TOKunord:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = UNORDERED_EXPR;
            else
                // %% is this properly optimized away?
                return convert(boolean_type_node, integer_zero_node);
            break;

        case TOKlg:
            t1 = irs->maybeMakeTemp(t1);
            t2 = irs->maybeMakeTemp(t2);
            // %% Write instead as (t1 != t2) ?
            return irs->boolOp(TRUTH_ORIF_EXPR,
                build2(LT_EXPR, boolean_type_node, t1, t2),
                build2(GT_EXPR, boolean_type_node, t1, t2));

        case TOKue:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = UNEQ_EXPR;
            else
                out_code = EQ_EXPR;
            break;

        // From cmath2.d: if imaginary parts are equal,
        // result is comparison of real parts; otherwise, result false
        //
        // Does D define an ordering for complex numbers?

        // make a target-independent _cmplxCmp ?
        case TOKule:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = UNLE_EXPR;
            else
                out_code = LE_EXPR;
            break;

        case TOKle:
            out_code = LE_EXPR;
            break;

        case TOKul:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = UNLT_EXPR;
            else
                out_code = LT_EXPR;
            break;

        case TOKlt:
            out_code = LT_EXPR;
            break;

        case TOKuge:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = UNGE_EXPR;
            else
                out_code = GE_EXPR;
            break;

        case TOKge:
            out_code = GE_EXPR;
            break;

        case TOKug:
            if (FLOAT_TYPE_P(TREE_TYPE(t1)) && FLOAT_TYPE_P(TREE_TYPE(t2)))
                out_code = UNGT_EXPR;
            else
                out_code = GT_EXPR;
            break;

        case TOKgt:
            out_code = GT_EXPR;
            break;

        default:
            gcc_unreachable();
    }

    // Build compare expression.
    /* Need to fold, otherwise 'complex-var == complex-cst' is not
       gimplified correctly.  */
    tree t = fold_build2(out_code, boolean_type_node, t1, t2);

    return convert(type, t);
}

static tree
build_math_op(TOK op, tree type, tree e1, Type * tb1, tree e2, Type * tb2, IRState * irs)
{
    // Integral promotions have already been done in the front end
    tree_code out_code;
    tree e1_type = TREE_TYPE(e1);
    tree e2_type = TREE_TYPE(e2);

    switch (op)
    {
        case TOKadd:    out_code = PLUS_EXPR;    break;
        case TOKmin:    out_code = MINUS_EXPR;   break;
        case TOKmul:    out_code = MULT_EXPR;    break;
        case TOKxor:    out_code = BIT_XOR_EXPR; break;
        case TOKor:     out_code = BIT_IOR_EXPR; break;
        case TOKand:    out_code = BIT_AND_EXPR; break;
        case TOKshl:    out_code = LSHIFT_EXPR;  break;
        case TOKushr: // drop through
        case TOKshr:    out_code = RSHIFT_EXPR;  break;
        case TOKmod:
            if (INTEGRAL_TYPE_P(e1_type))
                out_code = TRUNC_MOD_EXPR;
            else
                return irs->floatMod(e1, e2, e1_type);
            break;

        case TOKdiv:
            if (INTEGRAL_TYPE_P(e1_type))
                out_code = TRUNC_DIV_EXPR;
            else
                out_code = RDIV_EXPR;
            break;

        default:
            gcc_unreachable();
    }

    bool is_unsigned = TYPE_UNSIGNED(e1_type) || TYPE_UNSIGNED(e2_type)
                        || op == TOKushr;
#if D_GCC_VER >= 43
    if (POINTER_TYPE_P(e1_type) && INTEGRAL_TYPE_P(e2_type))
    {
        return irs->nop(irs->pointerOffsetOp(out_code, e1, e2), type);
    }
    else if (INTEGRAL_TYPE_P(e1_type) && POINTER_TYPE_P(e2_type))
    {
        gcc_assert(out_code == PLUS_EXPR);
        return irs->nop(irs->pointerOffsetOp(out_code, e2, e1), type);
    }
    else if (POINTER_TYPE_P(e1_type) && POINTER_TYPE_P(e2_type))
    {   /* Need to convert pointers to integers because tree-vrp asserts
           against (ptr MINUS ptr). */

        // %% need to work on this
        tree tt = d_type_for_mode(ptr_mode, TYPE_UNSIGNED(type));
        e1 = convert(tt, e1);
        e2 = convert(tt, e2);
        return convert(type, build2(out_code, tt, e1, e2));
    }
    else
#endif
    if (INTEGRAL_TYPE_P(type) &&
        (TYPE_UNSIGNED(type) != 0) != is_unsigned)
    {
        tree tt = is_unsigned
                    ? d_unsigned_type(type)
                    : d_signed_type(type);
        if (op == TOKushr)
        {   /* For >>> and >>>= operations, need e1 to be of the same
               signedness as what we are converting to, else we get
               undefined behaviour when optimizations are enabled. */
            e1 = convert(tt, e1);
        }
        tree t = build2(out_code, tt, e1, e2);
        return convert(type, t);
    }
    else
    {   /* Front-end does not do this conversion and GCC does not
           always do it right. */
        if (COMPLEX_FLOAT_TYPE_P(e1_type) && !COMPLEX_FLOAT_TYPE_P(e2_type))
            e2 = irs->convertTo(e2, tb2, tb1);
        else if (COMPLEX_FLOAT_TYPE_P(e2_type) && !COMPLEX_FLOAT_TYPE_P(e1_type))
            e1 = irs->convertTo(e1, tb1, tb2);

        return build2(out_code, type, e1, e2);
    }
}

static tree
build_assign_math_op(TOK op, Type * type, Expression * e1, Expression * e2, IRState * irs)
{
    TOK out_code;
    switch (op)
    {
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
            gcc_unreachable();
    }

    // Skip casts for lhs assignment.
    Expression * e1b = e1;
    while (e1b->op == TOKcast)
    {
        CastExp * ce = (CastExp *) e1b;
        gcc_assert(irs->typesCompatible(ce->type, ce->to)); // %% check, basetype?
        e1b = ce->e1;
    }
    tree lhs_assign = irs->toElemLvalue(e1b);
    lhs_assign = stabilize_reference(lhs_assign);

    tree lhs_var = irs->exprVar(type->toCtype());
    DECL_INITIAL(lhs_var) = lhs_assign;

    tree rhs_assign = build_math_op(out_code, e1->type->toCtype(),
                                    irs->convertTo(lhs_var, e1b->type, e1->type), e1->type,
                                    e2->toElem(irs), e2->type, irs);

    rhs_assign = build2(MODIFY_EXPR, type->toCtype(), lhs_var,
                        irs->convertForAssignment(rhs_assign, e1->type, type));

    lhs_assign = build2(MODIFY_EXPR, type->toCtype(), lhs_assign, lhs_var);

    lhs_assign = irs->compound(rhs_assign, lhs_assign);
    return irs->binding(lhs_var, lhs_assign);
}

elem *
BinExp::toElemBin(IRState* irs, int binop)
{
    TY ty1 = e1->type->toBasetype()->ty;
    TY ty2 = e2->type->toBasetype()->ty;

    if ((ty1 == Tarray || ty1 == Tsarray ||
         ty2 == Tarray || ty2 == Tsarray) &&
        op != TOKandand && op != TOKoror)
    {
        error("Array operation %s not implemented", toChars());
        return irs->errorMark(type);
    }

    switch (binop)
    {
        case opComp:
            return build_bool_binop(op, type->toCtype(), e1, e2, irs);

        case opBinary:
            return build_math_op(op, type->toCtype(),
                                 e1->toElem(irs), e1->type,
                                 e2->toElem(irs), e2->type, irs);

        case opAssign:
            return build_assign_math_op(op, type, e1, e2, irs);

        default:
            gcc_unreachable();
    }
}

elem *
IdentityExp::toElem(IRState* irs)
{
    TY ty1 = e1->type->toBasetype()->ty;

    // Assuming types are the same from typeCombine
    //if (ty1 != e2->type->toBasetype()->ty)
    //abort();

    if (ty1 == Tsarray)
    {
        return build2(op == TOKidentity ? EQ_EXPR : NE_EXPR,
                      type->toCtype(),
                      irs->addressOf(e1->toElem(irs)),
                      irs->addressOf(e2->toElem(irs)));
    }
    else if (ty1 == Treference || ty1 == Tclass || ty1 == Tarray)
    {
        return build2(op == TOKidentity ? EQ_EXPR : NE_EXPR,
                      type->toCtype(),
                      e1->toElem(irs),
                      e2->toElem(irs));
    }
    else if (ty1 == Tstruct || e1->type->isfloating())
    {   // Do bit compare.
        tree t_memcmp = irs->buildCall(built_in_decls[BUILT_IN_MEMCMP], 3,
                                       irs->addressOf(e1->toElem(irs)),
                                       irs->addressOf(e2->toElem(irs)),
                                       irs->integerConstant(e1->type->size()));

        return irs->boolOp(op == TOKidentity ? EQ_EXPR : NE_EXPR,
                           t_memcmp, integer_zero_node);
    }
    else
    {   // For operand types other than class objects, static or dynamic
        // arrays, identity is defined as being the same as equality

        // Assumes object == object has been changed to function call
        // ... impl is really the same as the special cales
        return toElemBin(irs, opComp);
    }
}

elem *
EqualExp::toElem(IRState* irs)
{
    Type * tb1 = e1->type->toBasetype();
    Type * tb2 = e2->type->toBasetype();

    if ((tb1->ty == Tsarray || tb1->ty == Tarray) &&
        (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
        Type * telem = tb1->nextOf()->toBasetype();

        // _adEq compares each element.  If bitwise comparison is ok,
        // use memcmp.
        if (telem->isfloating() || telem->isClassHandle() ||
            telem->ty == Tsarray || telem->ty == Tarray ||
            telem->ty == Tstruct)
        {
            tree result;
            tree args[3] = {
                irs->toDArray(e1),
                irs->toDArray(e2),
                NULL_TREE,
            };
#if V2
            args[2] = irs->typeinfoReference(telem->arrayOf());
            result = irs->libCall(LIBCALL_ADEQ2, 3, args);
#else
            args[2] = irs->typeinfoReference(telem);
            result = irs->libCall(LIBCALL_ADEQ, 3, args);
#endif
            result = convert(type->toCtype(), result);
            if (op == TOKnotequal)
                result = build1(TRUTH_NOT_EXPR, type->toCtype(), result);
            return result;
        }
        else if (tb1->ty == Tsarray && tb2->ty == Tsarray)
        {   // Assuming sizes are equal.
            return build2(op == TOKnotequal ? NE_EXPR : EQ_EXPR,
                          type->toCtype(), e1->toElem(irs), e2->toElem(irs));
        }
        else
        {
            tree len_expr[2];
            tree data_expr[2];

            for (int i = 0; i < 2; i++)
            {
                Expression * exp = i == 0 ? e1 : e2;
                TY ety = i == 0 ? tb1->ty : tb2->ty;

                if (ety == Tarray)
                {
                    tree ae = irs->maybeMakeTemp(exp->toElem(irs));
                    data_expr[i] = irs->darrayPtrRef(ae);
                    len_expr[i] = irs->darrayLenRef(ae);    // may be used twice -- should be okay
                }
                else
                {
                    data_expr[i] = irs->addressOf(exp->toElem(irs));
                    len_expr[i] = ((TypeSArray *) exp->type->toBasetype())->dim->toElem(irs);
                }
            }

            tree t_memcmp = built_in_decls[BUILT_IN_MEMCMP];
            tree result;
            tree size;

            size = fold_build2(MULT_EXPR, size_type_node,
                               convert(size_type_node, len_expr[0]), // should be size_type already, though
                               size_int(telem->size()));

            result = irs->buildCall(t_memcmp, 3, data_expr[0], data_expr[1], size);

            if (op == TOKequal)
                result = irs->boolOp(TRUTH_ANDIF_EXPR,
                    irs->boolOp(EQ_EXPR, len_expr[0], len_expr[1]),
                    irs->boolOp(EQ_EXPR, result, integer_zero_node));
            else
                result = irs->boolOp(TRUTH_ORIF_EXPR,
                    irs->boolOp(NE_EXPR, len_expr[0], len_expr[1]),
                    irs->boolOp(NE_EXPR, result, integer_zero_node));

            return convert(type->toCtype(), result);
        }
    }
    else if (tb1->ty == Taarray && tb2->ty == Taarray)
    {
        TypeAArray * aatype_1 = (TypeAArray *) tb1;
        tree args[3] = {
            irs->typeinfoReference(aatype_1),
            e1->toElem(irs),
            e2->toElem(irs)
        };
        tree result = irs->libCall(LIBCALL_AAEQUAL, 3, args);
        result = convert(type->toCtype(), result);

        if (op == TOKnotequal)
            result = build1(TRUTH_NOT_EXPR, type->toCtype(), result);

        return convert(type->toCtype(), result);
    }
    else
    {
        return toElemBin(irs, opComp);
    }
}

elem *
InExp::toElem(IRState * irs)
{
    Type * e2_base_type = e2->type->toBasetype();
    AddrOfExpr aoe;
    gcc_assert(e2_base_type->ty == Taarray);

    Type * key_type = ((TypeAArray *) e2_base_type)->index->toBasetype();
    tree args[3] = {
        e2->toElem(irs),
        irs->typeinfoReference(key_type),
        aoe.set(irs, irs->convertTo(e1, key_type))
    };
    return d_convert_basic(type->toCtype(),
        aoe.finish(irs, irs->libCall(LIBCALL_AAINP, 3, args)));
}

elem *
CmpExp::toElem(IRState* irs)
{
    Type * tb1 = e1->type->toBasetype();
    Type * tb2 = e2->type->toBasetype();

    if ((tb1->ty == Tsarray || tb1->ty == Tarray) &&
        (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
        Type * telem = tb1->nextOf()->toBasetype();
        unsigned n_args = 2;
        LibCall lib_call;
        tree args[3] = {
            irs->toDArray(e1),
            irs->toDArray(e2),
            NULL_TREE,
        };

        if (telem->ty == Tvoid ||
            (telem->size() == 1 && telem->isscalar() &&
             telem->isunsigned()))
        {   // Tvoid, Tuns8, Tchar, Tbool
            lib_call = LIBCALL_ADCMPCHAR;
        }
        else
        {
#if V2
            args[2] = irs->typeinfoReference(telem->arrayOf());
            n_args = 3;
            lib_call = LIBCALL_ADCMP2;
#else
            args[2] = irs->typeinfoReference(telem);
            n_args = 3;
            lib_call = LIBCALL_ADCMP;
#endif
        }

        tree result = irs->libCall(lib_call, n_args, args);
        enum tree_code out_code;

        // %% For float element types, warn that NaN is not taken into account ?
        switch (op)
        {
            case TOKlt: case TOKul:
                out_code = LT_EXPR;
                break;

            case TOKgt: case TOKug:
                out_code = GT_EXPR;
                break;

            case TOKle: case TOKule:
                out_code = LE_EXPR;
                break;

            case TOKge: case TOKuge:
                out_code = GE_EXPR;
                break;

            case TOKunord: case TOKleg:
                // %% Could do a check for side effects and drop the unused condition
                return build2(COMPOUND_EXPR, boolean_type_node, result,
                    d_truthvalue_conversion(op == TOKunord
                                            ? integer_zero_node
                                            : integer_one_node));

            case TOKlg:
                out_code = NE_EXPR;
                break;

            case TOKue:
                out_code = EQ_EXPR;
                break;

            default:
                gcc_unreachable();
        }
        result = build2(out_code, boolean_type_node, result, integer_zero_node);
        return convert(type->toCtype(), result);
    }
    else
    {
        return toElemBin(irs, opComp);
    }
}

elem *
AndAndExp::toElem(IRState * irs)
{
    if (e2->type->toBasetype()->ty != Tvoid)
        return toElemBin(irs, opComp);
    else
    {
#if V2
        tree t2 = e2->toElemDtor(irs);
#else
        tree t2 = e2->toElem(irs);
#endif
        return build3(COND_EXPR, type->toCtype(),
                      irs->convertForCondition(e1), t2, d_void_zero_node);
    }
}

elem *
OrOrExp::toElem(IRState * irs)
{
    if (e2->type->toBasetype()->ty != Tvoid)
        return toElemBin(irs, opComp);
    else
    {
#if V2
        tree t2 = e2->toElemDtor(irs);
#else
        tree t2 = e2->toElem(irs);
#endif
        return build3(COND_EXPR, type->toCtype(),
                      build1(TRUTH_NOT_EXPR, boolean_type_node,
                             irs->convertForCondition(e1)),
                             t2, d_void_zero_node);
    }
}

elem *
XorExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
OrExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
AndExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
UshrExp::toElem(IRState* irs)
{
    return toElemBin(irs, opBinary);
}

elem *
ShrExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
ShlExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
ModExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
DivExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

elem *
MulExp::toElem(IRState * irs)
{
    return toElemBin(irs, opBinary);
}

#if V2
elem *
PowExp::toElem(IRState * irs)
{
    tree e1_t, e2_t;
    tree powtype, powfn = NULL_TREE;
    Type *tb1 = e1->type->toBasetype();

    // Dictates what version of pow() we call.
    powtype = type->toBasetype()->toCtype();
    // If type is int, implicitly convert to double.
    // This allows backend to fold the call into a constant return value.
    if (type->isintegral())
        powtype = double_type_node;

    // Lookup compatible builtin. %% TODO: handle complex types?
    if (TYPE_MAIN_VARIANT(powtype) == double_type_node)
        powfn = built_in_decls[BUILT_IN_POW];
    else if (TYPE_MAIN_VARIANT(powtype) == float_type_node)
        powfn = built_in_decls[BUILT_IN_POWF];
    else if (TYPE_MAIN_VARIANT(powtype) == long_double_type_node)
        powfn = built_in_decls[BUILT_IN_POWL];

    if (powfn == NULL_TREE)
    {
        if (tb1->ty == Tarray || tb1->ty == Tsarray)
            error("Array operation %s not implemented", toChars());
        else
            error("%s ^^ %s is not supported", e1->type->toChars(), e2->type->toChars());
        return irs->errorMark(type);
    }

    e1_t = convert(powtype, e1->toElem(irs));
    e2_t = convert(powtype, e2->toElem(irs));

    return convert(type->toCtype(), irs->buildCall(powfn, 2, e1_t, e2_t));
}
#endif

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
        {
            elem_type = tb1->nextOf();
        }
        else if ((tb2->ty == Tsarray || tb2->ty == Tarray) &&
            irs->typesCompatible(e1->type, tb2->nextOf()))
        {
            elem_type = tb2->nextOf();
        }
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
        while (e->op == TOKcat)
        {
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

    while (ce)
    {
        Expression *oe = ce->e2;
        while (1)
        {
            tree array_exp;
            if (irs->typesCompatible(oe->type->toBasetype(), elem_type->toBasetype()))
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

            if (ce)
            {
                if (ce->e1->op != TOKcat)
                {
                    oe = ce->e1;
                    ce = NULL;
                    // finish with atomtic lhs
                }
                else
                {
                    ce = (CatExp*) ce->e1;
                    break;  // continue with lhs CatExp
                }
            }
            else
                goto all_done;
        }
    }
 all_done:

    result = irs->libCall(n_operands > 2 ? LIBCALL_ARRAYCATNT : LIBCALL_ARRAYCATT,
        n_args, args, type->toCtype());

    for (size_t i = 0; i < elem_vars.dim; ++i)
    {
        tree elem_var = (tree) elem_vars.data[i];
        result = irs->binding(elem_var, result);
    }

    return result;
}

elem *
MinExp::toElem(IRState* irs)
{
    // %% faster: check if result is complex
    if ((e1->type->isreal() && e2->type->isimaginary()) ||
        (e1->type->isimaginary() && e2->type->isreal()))
    {   // %%TODO: need to check size/modes
        tree t1 = e1->toElem(irs);
        tree t2 = e2->toElem(irs);

        t2 = build1(NEGATE_EXPR, TREE_TYPE(t2), t2);

        if (e1->type->isreal())
            return build2(COMPLEX_EXPR, type->toCtype(), t1, t2);
        else
            return build2(COMPLEX_EXPR, type->toCtype(), t2, t1);
    }

    // The front end has already taken care of pointer-int and pointer-pointer
    return toElemBin(irs, opBinary);
}

elem *
AddExp::toElem(IRState* irs)
{
    // %% faster: check if result is complex
    if ((e1->type->isreal() && e2->type->isimaginary()) ||
        (e1->type->isimaginary() && e2->type->isreal()))
    {   // %%TODO: need to check size/modes
        tree t1 = e1->toElem(irs);
        tree t2 = e2->toElem(irs);

        if (e1->type->isreal())
            return build2(COMPLEX_EXPR, type->toCtype(), t1, t2);
        else
            return build2(COMPLEX_EXPR, type->toCtype(), t2, t1);
    }

    // The front end has already taken care of (pointer + integer)
    return toElemBin(irs, opBinary);
}

elem *
XorAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
OrAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
AndAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
UshrAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
ShrAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
ShlAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
ModAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
DivAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
MulAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

#if V2
elem *
PowAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}
#endif

elem *
CatAssignExp::toElem(IRState * irs)
{
    unsigned n_args;
    tree * args;
    Type * elem_type = e1->type->toBasetype()->nextOf()->toBasetype();
    Type * value_type = e2->type->toBasetype();
    LibCall lib_call;
    AddrOfExpr aoe;
    tree result;

    if (e1->type->toBasetype()->ty == Tarray && value_type->ty == Tdchar &&
        (elem_type->ty == Tchar || elem_type->ty == Twchar))
    {   // append a dchar to a char[] or wchar[]
        n_args = 2;
        args = new tree[n_args];

        args[0] = aoe.set(irs, e1->toElem(irs));
        args[1] = irs->toElemLvalue(e2);
        lib_call = elem_type->ty == Tchar ?
            LIBCALL_ARRAYAPPENDCD : LIBCALL_ARRAYAPPENDWD;

        result = irs->libCall(lib_call, n_args, args, type->toCtype());
    }
    else if (irs->typesCompatible(elem_type, value_type))
    {   // append an element
        n_args = 3;
        args = new tree[n_args];

        args[0] = irs->typeinfoReference(type);
        args[1] = irs->addressOf(irs->toElemLvalue(e1));
        args[2] = size_one_node;
        lib_call = LIBCALL_ARRAYAPPENDCTP;

        result = irs->libCall(lib_call, n_args, args, type->toCtype());
        result = save_expr(result);

        // assign e2 to last element
        tree off_exp = irs->darrayLenRef(result);
        off_exp = build2(MINUS_EXPR, TREE_TYPE(off_exp), off_exp, size_one_node);
        off_exp = irs->maybeMakeTemp(off_exp);

        tree ptr_exp = irs->darrayPtrRef(result);
        ptr_exp = irs->pvoidOkay(ptr_exp);
        ptr_exp = irs->pointerIntSum(ptr_exp, off_exp);

        // evaluate expression before appending
        tree e2e = e2->toElem(irs);
        e2e = save_expr(e2e);

        result = build2(MODIFY_EXPR, elem_type->toCtype(), irs->indirect(ptr_exp), e2e);
        result = build2(COMPOUND_EXPR, elem_type->toCtype(), e2e, result);
    }
    else
    {   // append an array
        n_args = 3;
        args = new tree[n_args];

        args[0] = irs->typeinfoReference(type);
        args[1] = irs->addressOf(irs->toElemLvalue(e1));
        args[2] = irs->toDArray(e2);
        lib_call = LIBCALL_ARRAYAPPENDT;

        result = irs->libCall(lib_call, n_args, args, type->toCtype());
    }
    return aoe.finish(irs, result);
}

elem *
MinAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}

elem *
AddAssignExp::toElem(IRState * irs)
{
    return toElemBin(irs, opAssign);
}


static void
do_array_set(IRState * irs, tree in_ptr, tree in_val, tree in_cnt)
{
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

    irs->exitIfFalse(build2(NE_EXPR, boolean_type_node,
                      convert(TREE_TYPE(count_var), integer_zero_node), count_var));

    irs->doExp(build2(MODIFY_EXPR, void_type_node, irs->indirect(ptr_var), value_to_use));
    irs->doExp(build2(MODIFY_EXPR, void_type_node, ptr_var,
                    irs->pointerOffset(ptr_var,
                        TYPE_SIZE_UNIT(TREE_TYPE(TREE_TYPE(ptr_var))))));
    irs->doExp(build2(MODIFY_EXPR, void_type_node, count_var,
                    build2(MINUS_EXPR, TREE_TYPE(count_var), count_var,
                        convert(TREE_TYPE(count_var), integer_one_node))));

    irs->endLoop();

    irs->endBindings();
}


// Create a tree node to set multiple elements to a single value
static tree
array_set_expr(IRState * irs, tree ptr, tree src, tree count)
{
    irs->pushStatementList();
    do_array_set(irs, ptr, src, count);
    return irs->popStatementList();
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
AssignExp::toElem(IRState* irs)
{   // First, handle special assignment semantics
    if (e1->op == TOKarraylength)
    {   // Assignment to an array's length property; resize the array.
        Type * array_type;
        Type * elem_type;
        tree args[5];
        tree array_exp;
        tree result;
        {
            Expression * ae = ((ArrayLengthExp *) e1)->e1;
            array_type = ae->type;
            elem_type = ae->type->toBasetype()->nextOf(); // don't want ->toBasetype for the element type
            array_exp = irs->addressOf(ae->toElem(irs));
        }
        args[0] = irs->typeinfoReference(array_type);
        args[1] = irs->convertTo(e2, Type::tsize_t);
        args[2] = array_exp;

        LibCall lib_call = elem_type->isZeroInit() ?
            LIBCALL_ARRAYSETLENGTHT : LIBCALL_ARRAYSETLENGTHIT;

        result = irs->libCall(lib_call, 3, args);
        result = irs->darrayLenRef(result);

        return result;
    }
    else if (e1->op == TOKslice)
    {
        Type * elem_type = e1->type->toBasetype()->nextOf()->toBasetype();

        if (irs->typesCompatible(elem_type, e2->type->toBasetype()))
        {   // Set a range of elements to one value.
            // %% This is used for initing on-stack static arrays..
            // should optimize with memset if possible
            // %% vararg issues
            tree dyn_array_exp = irs->maybeMakeTemp(e1->toElem(irs));
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
            tree set_exp = array_set_expr(irs, irs->darrayPtrRef(dyn_array_exp),
                e2->toElem(irs), irs->darrayLenRef(dyn_array_exp));
            return irs->compound(set_exp, dyn_array_exp);
        }
        else
        {
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
            if (irs->arrayBoundsCheck())
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
                    irs->maybeMakeTemp(irs->toDArray(e1)),
                    irs->toDArray(e2) };
                tree t_memcpy = built_in_decls[BUILT_IN_MEMCPY];
                tree result;
                tree size;

                size = fold_build2(MULT_EXPR, size_type_node,
                                   convert(size_type_node, irs->darrayLenRef(array[0])),
                                   size_int(elem_type->size()));

                result = irs->buildCall(t_memcpy, 3, irs->darrayPtrRef(array[0]),
                                        irs->darrayPtrRef(array[1]), size);

                return irs->compound(result, array[0], type->toCtype());
            }
        }
    }
    else if (op == TOKconstruct)
    {
        tree lhs = irs->toElemLvalue(e1);
        tree rhs = irs->convertForAssignment(e2, e1->type);
        Type * tb1 = e1->type->toBasetype();
        tree result = NULL_TREE;

        if (e1->op == TOKvar)
        {
            Declaration * decl = ((VarExp *)e1)->var;
            // Look for reference initializations
            if (decl->storage_class & (STCout | STCref))
            {   // Want reference to lhs, not indirect ref.
                lhs = TREE_OPERAND(lhs, 0);
                rhs = irs->addressOf(rhs);
            }
        }
        result = build2(MODIFY_EXPR, type->toCtype(), lhs, rhs);

        if (tb1->ty == Tstruct)
        {
            if (e2->op == TOKstructliteral)
            {   // Initialize all alignment 'holes' to zero.
                StructLiteralExp * sle = ((StructLiteralExp *)e2);
                if (sle->fillHoles)
                {
                    unsigned sz = sle->type->size();
                    tree init = irs->buildCall(built_in_decls[BUILT_IN_MEMSET], 3,
                                               irs->addressOf(lhs), size_zero_node, size_int(sz));
                    result = irs->maybeCompound(init, result);
                }
            }
#if V2
            else if (e2->op == TOKint64)
            {   // Maybe set-up hidden pointer to outer scope context.
                StructDeclaration * sd = ((TypeStruct *)tb1)->sym;
                if (sd->isNested())
                {
                    tree vthis_field = sd->vthis->toSymbol()->Stree;
                    tree vthis_value = irs->getVThis(sd, this);

                    tree vthis_exp = build2(MODIFY_EXPR, TREE_TYPE(vthis_field),
                                            irs->component(lhs, vthis_field), vthis_value);
                    result = irs->maybeCompound(result, vthis_exp);
                }
            }
#endif
        }
        return result;

    }
    else
    {   // Simple assignment
        tree lhs = irs->toElemLvalue(e1);
        return build2(MODIFY_EXPR, type->toCtype(),
                      lhs, irs->convertForAssignment(e2, e1->type));
    }
    gcc_unreachable();
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

    if (array_type->ty != Taarray)
    {   /* arrayElemRef will call aryscp.finish.  This result
           of this function may be used as an lvalue and we
           do not want it to be a BIND_EXPR. */
        ArrayScope aryscp(irs, lengthVar, loc);
        return irs->arrayElemRef(this, & aryscp);
    }
    else
    {
        Type * key_type = ((TypeAArray *) array_type)->index->toBasetype();
        AddrOfExpr aoe;
        tree args[4] = {
            e1->toElem(irs),
            irs->typeinfoReference(key_type),
            irs->integerConstant(array_type->nextOf()->size(), Type::tsize_t),
            aoe.set(irs, irs->convertTo(e2, key_type))
        };
        LibCall lib_call = modifiable ? LIBCALL_AAGETP : LIBCALL_AAGETRVALUEP;
        tree t = irs->libCall(lib_call, 4, args, type->pointerTo()->toCtype());
        t = aoe.finish(irs, t);

        if (irs->arrayBoundsCheck())
        {
            t = save_expr(t);
            t = build3(COND_EXPR, TREE_TYPE(t), t, t,
                       irs->assertCall(loc, LIBCALL_ARRAY_BOUNDS));
        }
        return irs->indirect(t, type->toCtype());
    }
}

elem *
CommaExp::toElem(IRState * irs)
{
    // CommaExp is used with DotTypeExp..?
    if (e1->op == TOKdottype && e2->op == TOKvar)
    {
        VarExp * ve = (VarExp *) e2;
        VarDeclaration * vd;
        FuncDeclaration * fd;
        /* Handle references to static variable and functions.  Otherwise,
           just let the DotTypeExp report an error. */
        if (((vd = ve->var->isVarDeclaration()) && ! vd->needThis())
            || ((fd = ve->var->isFuncDeclaration()) && ! fd->isThis()))
        {
            return e2->toElem(irs);
        }
    }
    tree t1 = e1->toElem(irs);
    tree t2 = e2->toElem(irs);
    tree tt = type ? type->toCtype() : void_type_node;

    return build2(COMPOUND_EXPR, tt, t1, t2);
}

elem *
ArrayLengthExp::toElem(IRState * irs)
{
    if (e1->type->toBasetype()->ty == Tarray)
    {
        return irs->darrayLenRef(e1->toElem(irs));
    }
    else
    {   // Tsarray case seems to be handled by front-end
        error("unexpected type for array length: %s", type->toChars());
        return irs->errorMark(type);
    }
}

elem *
SliceExp::toElem(IRState * irs)
{   // This function assumes that the front end casts the result to a dynamic array.
    gcc_assert(type->toBasetype()->ty == Tarray);

    // Use convert-to-dynamic-array code if possible
    if (e1->type->toBasetype()->ty == Tsarray && ! upr && ! lwr)
        return irs->convertTo(e1->toElem(irs), e1->type, type);

    Type * orig_array_type = e1->type->toBasetype();

    tree orig_array_expr, orig_pointer_expr;
    tree final_len_expr, final_ptr_expr;
    tree array_len_expr = NULL_TREE;
    tree lwr_tree = NULL_TREE;
    tree upr_tree = NULL_TREE;

    ArrayScope aryscp(irs, lengthVar, loc);

    orig_array_expr = aryscp.setArrayExp(e1->toElem(irs), e1->type);
    orig_array_expr = irs->maybeMakeTemp(orig_array_expr);
    // specs don't say bounds if are checked for error or clipped to current size

    // Get the data pointer for static and dynamic arrays
    orig_pointer_expr = irs->convertTo(orig_array_expr, orig_array_type,
        orig_array_type->nextOf()->pointerTo());

    final_ptr_expr = orig_pointer_expr;

    // orig_array_expr is already a save_expr if necessary, so
    // we don't make array_len_expr a save_expr which is, at most,
    // a COMPONENT_REF on top of orig_array_expr.
    if (orig_array_type->ty == Tarray)
    {
        array_len_expr = irs->darrayLenRef(orig_array_expr);
    }
    else if (orig_array_type->ty == Tsarray)
    {
        array_len_expr  = ((TypeSArray *) orig_array_type)->dim->toElem(irs);
    }
    else
    {   // array_len_expr == NULL indicates no bounds check is possible
    }

    if (lwr)
    {
        lwr_tree = lwr->toElem(irs);

        if (! integer_zerop(lwr_tree))
        {
            lwr_tree = irs->maybeMakeTemp(lwr_tree);
            // Adjust .ptr offset
            final_ptr_expr = irs->pointerIntSum(irs->pvoidOkay(final_ptr_expr), lwr_tree);
            final_ptr_expr = irs->nop(final_ptr_expr, TREE_TYPE(orig_pointer_expr));
        }
        else
            lwr_tree = NULL_TREE;
    }

    if (upr)
    {
        upr_tree = upr->toElem(irs);
        upr_tree = irs->maybeMakeTemp(upr_tree);

        if (irs->arrayBoundsCheck())
        {   // %% && ! is zero
            if (array_len_expr)
            {
                final_len_expr = irs->checkedIndex(loc, upr_tree, array_len_expr, true);
            }
            else
            {   // Still need to check bounds lwr <= upr for pointers.
                gcc_assert(orig_array_type->ty == Tpointer);
                final_len_expr = upr_tree;
            }
            if (lwr_tree)
            {   // Enforces lwr <= upr. No need to check lwr <= length as
                // we've already ensured that upr <= length.
                tree lwr_bounds_check = irs->checkedIndex(loc, lwr_tree, upr_tree, true);
                final_len_expr = irs->compound(lwr_bounds_check, final_len_expr);
            }
        }
        else
        {
            final_len_expr = upr_tree;
        }
        if (lwr_tree)
        {   // %% Need to ensure lwr always gets evaluated first, as it may be a function call.
            // Does (-lwr + upr) rather than (upr - lwr)
            final_len_expr = build2(PLUS_EXPR, TREE_TYPE(final_len_expr),
                    build1(NEGATE_EXPR, TREE_TYPE(lwr_tree), lwr_tree),
                    final_len_expr);
        }
    }
    else
    {   // If this is the case, than there is no lower bound specified and
        // there is no need to subtract.
        switch (orig_array_type->ty)
        {
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

    return aryscp.finish(irs->darrayVal(type->toCtype(), final_len_expr, final_ptr_expr));
}

elem *
CastExp::toElem(IRState * irs)
{
    if (to->ty == Tvoid)
    {   // Just evaluate e1 if it has any side effects
        return build1(NOP_EXPR, void_type_node, e1->toElem(irs));
    }
    return irs->convertTo(e1, to);
}


elem *
DeleteExp::toElem(IRState* irs)
{
    // Does this look like an associative array delete?
    if (e1->op == TOKindex &&
        ((IndexExp*)e1)->e1->type->toBasetype()->ty == Taarray)
    {

        if (! global.params.useDeprecated)
            error("delete aa[key] deprecated, use aa.remove(key)", e1->toChars());

        Expression * e_array = ((BinExp *) e1)->e1;
        Expression * e_index = ((BinExp *) e1)->e2;

        // Check that the array is actually an associative array
        if (e_array->type->toBasetype()->ty == Taarray)
        {
            Type * a_type = e_array->type->toBasetype();
            Type * key_type = ((TypeAArray *) a_type)->index->toBasetype();
            AddrOfExpr aoe;

            tree args[3] = {
                e_array->toElem(irs),
                irs->typeinfoReference(key_type),
                aoe.set(irs, irs->convertTo(e_index, key_type)),
            };
            return aoe.finish(irs, irs->libCall(LIBCALL_AADELP, 3, args));
        }
    }

    // Otherwise, this is normal delete
    LibCall lib_call;
    tree t = e1->toElem(irs);
    Type * base_type = e1->type->toBasetype();

    switch (base_type->ty)
    {
        case Tclass:
        {
            bool is_intfc = base_type->isClassHandle()->isInterfaceDeclaration() != NULL;

            if (e1->op == TOKvar)
            {
                VarDeclaration * v = ((VarExp *)e1)->var->isVarDeclaration();
                if (v && v->onstack)
                {
                    lib_call = is_intfc ?
                        LIBCALL_CALLINTERFACEFINALIZER : LIBCALL_CALLFINALIZER;
                    break;
                }
            }
            lib_call = is_intfc ? LIBCALL_DELINTERFACE : LIBCALL_DELCLASS;
            break;
        }
        case Tarray:
        {
#if V2
            // Might need to run destructor on array contents
            Type * next_type = base_type->nextOf()->toBasetype();
            tree ti = d_null_pointer;

            while (next_type->ty == Tsarray)
                next_type = next_type->nextOf()->toBasetype();
            if (next_type->ty == Tstruct)
            {
                TypeStruct * ts = (TypeStruct *)next_type;
                if (ts->sym->dtor)
                    ti = base_type->nextOf()->getTypeInfo(NULL)->toElem(irs);
            }
            // call _delarray_t(&t, ti);
            tree args[2] = { irs->addressOf(t), ti };
            return irs->libCall(LIBCALL_DELARRAYT, 2, args);
#else
            lib_call = LIBCALL_DELARRAY; break;
#endif
        }
        case Tpointer:
        {
            lib_call = LIBCALL_DELMEMORY; break;
        }
        default:
        {
            error("don't know how to delete %s", e1->toChars());
            return irs->errorMark(type);
        }
    }

    if (lib_call != LIBCALL_CALLFINALIZER && lib_call != LIBCALL_CALLINTERFACEFINALIZER)
        t = irs->addressOf(t);

    return irs->libCall(lib_call, 1, & t);
}

elem *
RemoveExp::toElem(IRState * irs)
{
    Expression * e_array = e1;
    Expression * e_index = e2;
    // Check that the array is actually an associative array
    if (e_array->type->toBasetype()->ty == Taarray)
    {
        Type * a_type = e_array->type->toBasetype();
        Type * key_type = ((TypeAArray *) a_type)->index->toBasetype();
        AddrOfExpr aoe;

        tree args[3] = {
            e_array->toElem(irs),
            irs->typeinfoReference(key_type),
            aoe.set(irs, irs->convertTo(e_index, key_type)),
        };
        return aoe.finish(irs, irs->libCall(LIBCALL_AADELP, 3, args));
    }
    else
    {
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
    return build1(BIT_NOT_EXPR, type->toCtype(), e1->toElem(irs));
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
    size_t the_offset;
    tree rec_tree;

    if (e1->op == TOKadd)
    {
        BinExp * add_exp = (BinExp *) e1;
        if (add_exp->e1->op == TOKaddress &&
            add_exp->e2->isConst() && add_exp->e2->type->isintegral())
        {
            Expression * rec_exp = ((AddrExp*) add_exp->e1)->e1;
            rec_type = rec_exp->type->toBasetype();
            rec_tree = rec_exp->toElem(irs);
            the_offset = add_exp->e2->toUInteger();
        }
    }
    else if (e1->op == TOKsymoff)
    {   // is this ever not a VarDeclaration?
        SymOffExp * sym_exp = (SymOffExp *) e1;
        if (! irs->isDeclarationReferenceType(sym_exp->var))
        {
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

    if (rec_type && rec_type->ty == Tstruct)
    {
        StructDeclaration * sd = ((TypeStruct *)rec_type)->sym;
        for (size_t i = 0; i < sd->fields.dim; i++)
        {
            VarDeclaration * field = sd->fields.tdata()[i];
            if (field->offset == the_offset &&
                irs->typesSame(field->type, this->type))
            {
                if (irs->isErrorMark(rec_tree))
                    return rec_tree; // backend will ICE otherwise
                return irs->component(rec_tree, field->toSymbol()->Stree);
            }
            else if (field->offset > the_offset)
            {
                break;
            }
        }
    }

    tree e = irs->indirect(e1->toElem(irs), type->toCtype());
    if (irs->inVolatile())
        TREE_THIS_VOLATILE(e) = 1;

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
    tree call_exp = irs->call(e1, arguments);

    // Some library calls are defined to return a generic type.
    // this->type is the real type. (See crash2.d)
    if (type->isTypeBasic())
        call_exp = convert(type->toCtype(), call_exp);

    return call_exp;
}

elem *
Expression::toElem(IRState* irs)
{
    error("abstract Expression::toElem called");
    return irs->errorMark(type);
}

#if V2
/*******************************************
 * Evaluate Expression, then call destructors on any temporaries in it.
 */

elem *
Expression::toElemDtor(IRState *irs)
{
    size_t starti = irs->varsInScope ? irs->varsInScope->dim : 0;
    tree t = toElem(irs);
    size_t endi = irs->varsInScope ? irs->varsInScope->dim : 0;

    // Add destructors
    tree tdtors = NULL_TREE;
    for (size_t i = starti; i != endi; ++i)
    {
        VarDeclaration * vd = irs->varsInScope->tdata()[i];
        if (vd)
        {
            irs->varsInScope->tdata()[i] = NULL;
            tree ed = vd->edtor->toElem(irs);
            tdtors = irs->maybeCompound(ed, tdtors); // execute in reverse order
        }
    }
    if (tdtors != NULL_TREE)
    {
        t = save_expr(t);
        t = irs->compound(irs->compound(t, tdtors), t);
    }
    return t;
}
#endif


elem *
DotTypeExp::toElem(IRState *irs)
{
    // The only case in which this seems to be a valid expression is when
    // it is used to specify a non-virtual call (SomeClass.func(...)).
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
    if (t->ty == Tclass || t->ty == Tstruct)
    {   // %% Need to see if DotVarExp ever has legitimate
        // <aggregate>.<static method>.  If not, move this test
        // to objectInstanceMethod.
        if (! func->isThis())
            error("delegates are only for non-static functions");

        return irs->objectInstanceMethod(e1, func, type);
    }
    else
    {
        gcc_assert(func->isNested() || func->isThis());

        return irs->methodCallExpr(irs->functionPointer(func),
                                   (func->isNested()
                                    ? irs->getFrameForFunction(func)
                                    : e1->toElem(irs)),
                                   type);
    }
}

elem *
DotVarExp::toElem(IRState * irs)
{
    FuncDeclaration * func_decl;
    VarDeclaration * var_decl;
    Type * obj_basetype = e1->type->toBasetype();

    switch (obj_basetype->ty)
    {
        case Tpointer:
            if (obj_basetype->nextOf()->toBasetype()->ty != Tstruct)
                break;

            // drop through
        case Tstruct:
            // drop through
        case Tclass:
            if ((func_decl = var->isFuncDeclaration()))
            {   // if Tstruct, objInstanceMethod will use the address of e1
                return irs->objectInstanceMethod(e1, func_decl, type);
            }
            else if ((var_decl = var->isVarDeclaration()))
            {
                if (! (var_decl->storage_class & STCfield))
                    return irs->var(var_decl);
                else
                {
                    tree this_tree = e1->toElem(irs);
                    if (obj_basetype->ty != Tstruct)
                        this_tree = irs->indirect(this_tree);
                    return irs->component(this_tree, var_decl->toSymbol()->Stree);
                }
            }
            else
                error("%s is not a field, but a %s", var->toChars(), var->kind());
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
    if (global.params.useAssert)
    {
        Type * base_type = e1->type->toBasetype();
        TY ty = base_type->ty;
        tree assert_call = msg ?
            irs->assertCall(loc, msg) : irs->assertCall(loc);

        if (ty == Tclass)
        {
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
            // this does a null pointer check before calling _d_invariant
            return build3(COND_EXPR, void_type_node,
                    irs->boolOp(NE_EXPR, arg, d_null_pointer),
                    irs->libCall(LIBCALL_INVARIANT, 1, & arg), assert_call);
        }
        else
        {   // build: ((bool) e1  ? (void)0 : _d_assert(...))
            //    or: (e1 != null ? e1._invariant() : _d_assert(...))
            tree result;
            tree invc = NULL_TREE;
            tree e1_t = e1->toElem(irs);

            if (ty == Tpointer)
            {
                Type * sub_type = base_type->nextOf()->toBasetype();
                if (sub_type->ty == Tstruct)
                {
                    AggregateDeclaration * agg_decl = ((TypeStruct *) sub_type)->sym;
                    if (agg_decl->inv)
                    {
                        Expressions args;
                        e1_t = irs->maybeMakeTemp(e1_t);
                        invc = irs->call(agg_decl->inv, e1_t, & args);
                    }
                }
            }
            result = build3(COND_EXPR, void_type_node,
                    irs->convertForCondition(e1_t, e1->type),
                    invc ? invc : d_void_zero_node, assert_call);
            return result;
        }
    }

    return d_void_zero_node;
}

elem *
DeclarationExp::toElem(IRState* irs)
{
#if V2
    VarDeclaration * vd;
    if ((vd = declaration->isVarDeclaration()) != NULL)
    {   // Put variable on list of things needing destruction
        if (vd->edtor && ! vd->noscope)
        {
            if (! irs->varsInScope)
                irs->varsInScope = new VarDeclarations();
            irs->varsInScope->push(vd);
        }
    }
#endif

    // VarDeclaration::toObjFile was modified to call d_gcc_emit_local_variable
    // if needed.  This assumes irs == g.irs
    irs->pushStatementList();
    declaration->toObjFile(false);
    tree t = irs->popStatementList();

#if D_GCC_VER >= 45
    /* Construction of an array for typesafe-variadic function arguments
       can cause an empty STMT_LIST here.  This can causes problems
       during gimplification. */
    if (TREE_CODE(t) == STATEMENT_LIST && ! STATEMENT_LIST_HEAD(t))
        return build_empty_stmt(input_location);
#elif D_GCC_VER >= 43
    if (TREE_CODE(t) == STATEMENT_LIST && ! STATEMENT_LIST_HEAD(t))
        return build_empty_stmt();
#endif

    return t;
}

// %% check calling this directly?
elem *
FuncExp::toElem(IRState * irs)
{
    Type * func_type = type->toBasetype();

    if (func_type->ty == Tpointer)
        func_type = func_type->nextOf()->toBasetype();

    fd->toObjFile(false);

    switch (func_type->ty)
    {
        case Tfunction:
            return irs->nop(irs->addressOf(fd), type->toCtype());

        case Tdelegate:
            return irs->methodCallExpr(irs->functionPointer(fd),
                                       irs->getFrameForFunction(fd), type);

        default:
            ::error("Unexpected FuncExp type");
            return irs->errorMark(type);
    }

    // If nested, this will be a trampoline...
}

elem *
HaltExp::toElem(IRState* irs)
{
    // Needs improvement.  Avoid library calls if possible..
    tree t_abort = built_in_decls[BUILT_IN_ABORT];
    return irs->buildCall(t_abort, 0);
}

#if V2
elem *
SymbolExp::toElem(IRState * irs)
{
    if (op == TOKvar)
    {
        if (var->needThis())
        {
            error("need 'this' to access member %s", var->ident->string);
            return irs->errorMark(type);
        }

        // __ctfe is always false at runtime
        if (var->ident == Id::ctfe)
            return integer_zero_node;

        // For variables that are references (currently only out/inout arguments;
        // objects don't count), evaluating the variable means we want what it refers to.

        // TODO: is this ever not a VarDeclaration; (four sequences like this...)
        VarDeclaration * v = var->isVarDeclaration();
        tree e;
        if (v)
            e = irs->var(v);
        else
            e = var->toSymbol()->Stree;

        TREE_USED(e) = 1;

        if (irs->isDeclarationReferenceType(var))
        {
            e = irs->indirect(e, var->type->toCtype());
            if (irs->inVolatile())
                TREE_THIS_VOLATILE(e) = 1;
        }
        else
        {
            if (irs->inVolatile())
            {
                e = irs->addressOf(e);
                TREE_THIS_VOLATILE(e) = 1;
                e = irs->indirect(e);
                TREE_THIS_VOLATILE(e) = 1;
            }
        }
        return e;
    }
    else if (op == TOKsymoff)
    {
        size_t offset = ((SymOffExp *) this)->offset;

        VarDeclaration * v = var->isVarDeclaration();
        tree a;
        if (v)
            a = irs->var(v);
        else
            a = var->toSymbol()->Stree;

        TREE_USED(a) = 1;

        if (irs->isDeclarationReferenceType(var))
            gcc_assert(POINTER_TYPE_P(TREE_TYPE(a)));
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
    if (var->needThis())
    {
        error("need 'this' to access member %s", var->ident->string);
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

    TREE_USED(e) = 1;

    if (irs->isDeclarationReferenceType(var))
    {
        e = irs->indirect(e, var->type->toCtype());
        if (irs->inVolatile())
        {
            TREE_THIS_VOLATILE(e) = 1;
        }
    }
    else
    {
        if (irs->inVolatile())
        {
            e = irs->addressOf(e);
            TREE_THIS_VOLATILE(e) = 1;
            e = irs->indirect(e);
            TREE_THIS_VOLATILE(e) = 1;
        }
    }
    return e;
}

elem *
SymOffExp::toElem(IRState * irs)
{
    tree a;
    VarDeclaration * v = var->isVarDeclaration();
    if (v)
        a = irs->var(v);
    else
        a = var->toSymbol()->Stree;

    TREE_USED(a) = 1;

    if (irs->isDeclarationReferenceType(var))
        gcc_assert(POINTER_TYPE_P(TREE_TYPE(a)));
    else
        a = irs->addressOf(var);

    if (! offset)
        return convert(type->toCtype(), a);

    tree b = irs->integerConstant(offset, Type::tsize_t);
    return irs->nop(irs->pointerOffset(a, b), type->toCtype());
}
#endif

elem *
NewExp::toElem(IRState * irs)
{
    Type * base_type = newtype->toBasetype();
    tree result;

    if (allocator)
        gcc_assert(newargs);

    switch (base_type->ty)
    {
        case Tclass:
        {
            TypeClass * class_type = (TypeClass *) base_type;
            ClassDeclaration * class_decl = class_type->sym;

            tree new_call;
            tree setup_exp = NULL_TREE;
            // type->toCtype() is a REFERENCE_TYPE; we want the RECORD_TYPE
            tree rec_type = TREE_TYPE(class_type->toCtype());
            // Allocation call (custom allocator or _d_newclass)
            if (onstack)
            {
                tree stack_var = irs->localVar(rec_type);
                irs->expandDecl(stack_var);
                new_call = irs->addressOf(stack_var);
                setup_exp = build2(MODIFY_EXPR, rec_type,
                        irs->indirect(new_call, rec_type),
                        class_decl->toInitializer()->Stree);
            }
            else if (allocator)
            {
                new_call = irs->call(allocator, newargs);
                new_call = save_expr(new_call);
                // copy memory...
                setup_exp = build2(MODIFY_EXPR, rec_type,
                        irs->indirect(new_call, rec_type),
                        class_decl->toInitializer()->Stree);
            }
            else
            {
                tree arg = irs->addressOf(class_decl->toSymbol()->Stree);
                new_call = irs->libCall(LIBCALL_NEWCLASS, 1, & arg);
            }
            new_call = irs->nop(new_call, class_type->toCtype());

            if (class_type->sym->isNested())
            {
                tree vthis_value = NULL_TREE;
                tree vthis_field = class_type->sym->vthis->toSymbol()->Stree;
                if (thisexp)
                {
                    ClassDeclaration *thisexp_cd = thisexp->type->isClassHandle();
                    Dsymbol *outer = class_decl->toParent2();
                    int offset = 0;

                    vthis_value = thisexp->toElem(irs);
                    if (outer != thisexp_cd)
                    {
                        ClassDeclaration * outer_cd = outer->isClassDeclaration();
                        gcc_assert(outer_cd->isBaseOf(thisexp_cd, & offset));
                        // could just add offset
                        vthis_value = irs->convertTo(vthis_value, thisexp->type, outer_cd->type);
                    }
                }
                else
                {
                    vthis_value = irs->getVThis(class_decl, this);
                }

                if (vthis_value)
                {
                    new_call = save_expr(new_call);
                    setup_exp = irs->maybeCompound(setup_exp,
                            build2(MODIFY_EXPR, TREE_TYPE(vthis_field),
                                irs->component(irs->indirect(new_call, rec_type),
                                    vthis_field),
                                vthis_value));
                }
            }
            new_call = irs->maybeCompound(setup_exp, new_call);
            // Constructor call
            if (member)
            {
                result = irs->call(member, new_call, arguments);
            }
            else
            {
                result = new_call;
            }
            return irs->convertTo(result, base_type, type);
        }
        case Tstruct:
        {
            TypeStruct * struct_type = (TypeStruct *)newtype;
            Type * handle_type = base_type->pointerTo();
            tree new_call;
            tree t;
            bool need_init = true;

            if (onstack)
            {
                tree stack_var = irs->localVar(struct_type);
                irs->expandDecl(stack_var);
                new_call = irs->addressOf(stack_var);
            }
            else if (allocator)
            {
                new_call = irs->call(allocator, newargs);
            }
            else
            {
                tree args[2];
                LibCall lib_call = struct_type->isZeroInit(loc) ?
                    LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
                args[0] = irs->typeinfoReference(struct_type->arrayOf());
                args[1] = irs->integerConstant(1, Type::tsize_t);
                new_call = irs->libCall(lib_call, 2, args);
                new_call = irs->darrayPtrRef(new_call);
                need_init = false;
            }
            new_call = irs->nop(new_call, handle_type->toCtype());
            if (need_init)
            {
                // Save the result allocation call.
                new_call = save_expr(new_call);
                t = irs->indirect(new_call);
                t = build2(MODIFY_EXPR, TREE_TYPE(t), t,
                        irs->convertForAssignment(struct_type->defaultInit(loc), struct_type));
                new_call = irs->compound(t, new_call);
            }
            // Constructor call
            if (member)
            {
                new_call = irs->call(member, new_call, arguments);
#if STRUCTTHISREF
                new_call = irs->addressOf(new_call);
#endif
            }
#if V2
            // %% D2.0 nested structs
            StructDeclaration * struct_decl = struct_type->sym;

            if (struct_decl->isNested())
            {
                tree vthis_value = irs->getVThis(struct_decl, this);
                tree vthis_field = struct_decl->vthis->toSymbol()->Stree;

                new_call = save_expr(new_call);
                tree setup_exp = build2(MODIFY_EXPR, TREE_TYPE(vthis_field),
                        irs->component(irs->indirect(new_call, struct_type->toCtype()),
                            vthis_field),
                        vthis_value);
                new_call = irs->compound(setup_exp, new_call);
            }
#endif
            return irs->nop(new_call, type->toCtype());
        }
        case Tarray:
        {
            gcc_assert(! allocator);
            gcc_assert(arguments && arguments->dim > 0);

            LibCall lib_call;

            Type * elem_init_type = newtype;

            /* First, skip past dynamic array dimensions/types that will be
               allocated by this call. */
            for (size_t i = 0; i < arguments->dim; i++)
                elem_init_type = elem_init_type->toBasetype()->nextOf(); // assert ty == Tarray
            if (arguments->dim == 1)
            {
                lib_call = elem_init_type->isZeroInit() ?
                    LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;

                tree args[2];
                args[0] = irs->typeinfoReference(type);
                args[1] = (arguments->tdata()[0])->toElem(irs);
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

                    elms.reserve(arguments->dim);
                    for (size_t i = 0; i < arguments->dim/* - 1*/; i++)
                        elms.cons(irs->integerConstant(i, size_type_node),
                                (arguments->tdata()[i])->toElem(irs));
                    //elms.cons(final_length);
                    dims_init = build_constructor(TREE_TYPE(dims_var), elms.head);
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
        default:
        {
            Type * object_type = newtype;
            Type * handle_type = base_type->pointerTo();
            tree new_call;
            tree t;
            bool need_init = true;

            if (onstack)
            {
                tree stack_var = irs->localVar(object_type);
                irs->expandDecl(stack_var);
                new_call = irs->addressOf(stack_var);
            }
            else if (allocator)
            {
                new_call = irs->call(allocator, newargs);
            }
            else
            {
                tree args[2];
                LibCall lib_call = object_type->isZeroInit() ?
                    LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
                args[0] = irs->typeinfoReference(object_type->arrayOf());
                args[1] = irs->integerConstant(1, Type::tsize_t);
                new_call = irs->libCall(lib_call, 2, args);
                new_call = irs->darrayPtrRef(new_call);
                need_init = false;
            }
            new_call = irs->nop(new_call, handle_type->toCtype());
            if (need_init)
            {
                // Save the result allocation call.
                new_call = save_expr(new_call);
                t = irs->indirect(new_call);
                t = build2(MODIFY_EXPR, TREE_TYPE(t), t,
                        irs->convertForAssignment(object_type->defaultInit(), object_type));
                new_call = irs->compound(t, new_call);
            }
            return irs->nop(new_call, type->toCtype());
        }
    }
}

elem *
ScopeExp::toElem(IRState* irs)
{
    ::error("%s is not an expression", toChars());
    return irs->errorMark(type);
}

elem *
TypeExp::toElem(IRState* irs)
{
    ::error("type %s is not an expression", toChars());
    return irs->errorMark(type);
}

elem *
StringExp::toElem(IRState * irs)
{
    Type * base_type = type->toBasetype();
    TY base_ty = type ? base_type->ty : (TY) Tvoid;
    tree value;

    switch (base_ty)
    {
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
    Type * elem_type = base_ty != Tvoid ?
        base_type->nextOf() : Type::tchar;
    TREE_TYPE(value) = irs->arrayType(elem_type, len);

    switch (base_ty)
    {
        case Tarray:
            value = irs->darrayVal(type, len, irs->addressOf(value));
            break;
        case Tpointer:
            value = irs->addressOf(value);
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
    if (exps && exps->dim)
    {
        for (size_t i = 0; i < exps->dim; ++i)
        {
            Expression * e = exps->tdata()[i];
            result = irs->maybeVoidCompound(result, e->toElem(irs));
        }
    }
    else
        result = d_void_zero_node;

    return result;
}

elem *
ArrayLiteralExp::toElem(IRState * irs)
{
    Type * typeb = type->toBasetype();
    gcc_assert(typeb->ty == Tarray || typeb->ty == Tsarray || typeb->ty == Tpointer);
    Type * etype = typeb->nextOf();
    tree sa_type = irs->arrayType(etype->toCtype(), elements->dim);
    tree result = NULL_TREE;

    tree args[2] = {
        irs->typeinfoReference(etype->arrayOf()),
        irs->integerConstant(elements->dim, size_type_node)
    };
    // Call _d_arrayliteralTp(ti, dim);
    LibCall lib_call = LIBCALL_ARRAYLITERALTP;
    tree mem = irs->libCall(lib_call, 2, args, etype->pointerTo()->toCtype());
    mem = irs->maybeMakeTemp(mem);
#if V2
    if (var)
    {   // Just copy the array we are referencing, much faster.
        result = irs->var(var);
        if (var->type->ty == Tarray)
            result = irs->darrayPtrRef(result);
        else if (var->type->ty == Tsarray)
            result = irs->addressOf(result);
    }
    else
#endif
    {   /* Build an expression that assigns the expressions in ELEMENTS to a constructor. */
        CtorEltMaker elms;

        elms.reserve(elements->dim);
        for (size_t i = 0; i < elements->dim; i++)
        {
            elms.cons(irs->integerConstant(i, size_type_node),
                      (elements->tdata()[i])->toElem(irs));
        }
        tree ctor = build_constructor(sa_type, elms.head);
        result = irs->addressOf(ctor);
    }
    // memcpy(mem, &ctor, size)
    tree size = fold_build2(MULT_EXPR, size_type_node,
                            size_int(elements->dim), size_int(typeb->nextOf()->size()));

    result = irs->buildCall(built_in_decls[BUILT_IN_MEMCPY], 3,
                            mem, result, size);

    // Returns array pointed to by MEM.
    result = irs->maybeCompound(result, mem);

    if (typeb->ty == Tarray)
        result = irs->darrayVal(type, elements->dim, result);
    else if (typeb->ty == Tsarray)
        result = irs->indirect(result, sa_type);

    return result;
}

elem *
AssocArrayLiteralExp::toElem(IRState * irs)
{
    Type * tb = type->toBasetype();
    gcc_assert (tb->ty == Taarray);
#if V2
    // %% want mutable type for typeinfo reference.
    tb = tb->mutableOf();
#endif
    TypeAArray * aa_type = (TypeAArray *)tb;
    Type * index = aa_type->index;
    Type * next = aa_type->next;
    gcc_assert(keys != NULL);
    gcc_assert(values != NULL);

    tree keys_var = irs->exprVar(irs->arrayType(index, keys->dim)); //?
    tree vals_var = irs->exprVar(irs->arrayType(next, keys->dim));
    tree keys_ptr = irs->nop(irs->addressOf(keys_var),
                             index->pointerTo()->toCtype());
    tree vals_ptr = irs->nop(irs->addressOf(vals_var),
                             next->pointerTo()->toCtype());
    tree keys_offset = size_zero_node;
    tree vals_offset = size_zero_node;
    tree keys_size = size_int(index->size());
    tree vals_size = size_int(next->size());
    tree result = NULL_TREE;

    for (size_t i = 0; i < keys->dim; i++)
    {
        Expression * e;
        tree elemp_e, assgn_e;

        e = keys->tdata()[i];
        elemp_e = irs->pointerOffset(keys_ptr, keys_offset);
        assgn_e = irs->vmodify(irs->indirect(elemp_e), e->toElem(irs));
        keys_offset = size_binop(PLUS_EXPR, keys_offset, keys_size);
        result = irs->maybeCompound(result, assgn_e);

        e = values->tdata()[i];
        elemp_e = irs->pointerOffset(vals_ptr, vals_offset);
        assgn_e = irs->vmodify(irs->indirect(elemp_e), e->toElem(irs));
        vals_offset = size_binop(PLUS_EXPR, vals_offset, vals_size);
        result = irs->maybeCompound(result, assgn_e);
    }

    tree args[3] = {
        irs->typeinfoReference(aa_type),
        irs->darrayVal(index->arrayOf(), keys->dim, keys_ptr),
        irs->darrayVal(next->arrayOf(), keys->dim, vals_ptr),
    };
    result = irs->maybeCompound(result,
            irs->libCall(LIBCALL_ASSOCARRAYLITERALTP, 3, args));

    CtorEltMaker ce;
    tree aat_type = aa_type->toCtype();
    ce.cons(TYPE_FIELDS(aat_type), result);
    tree ctor = build_constructor(aat_type, ce.head);

    result = irs->binding(keys_var, irs->binding(vals_var, ctor));
    return irs->nop(result, type->toCtype());
}

elem *
StructLiteralExp::toElem(IRState *irs)
{
    // %% Brings false positives on D2, maybe uncomment for debug builds...
    //gcc_assert(irs->typesSame(type->toBasetype(), sd->type->toBasetype()));
    CtorEltMaker ce;

    if (elements)
    {
        gcc_assert(elements->dim <= sd->fields.dim);

        for (size_t i = 0; i < elements->dim; ++i)
        {
            if (! elements->tdata()[i])
                continue;

            Expression * exp = elements->tdata()[i];
            Type * exp_type = exp->type->toBasetype();
            tree exp_tree = NULL_TREE;
            tree call_exp = NULL_TREE;

            VarDeclaration * fld = sd->fields.tdata()[i];
            Type * fld_type = fld->type->toBasetype();

            if (fld_type->ty == Tsarray)
            {
                if (irs->typesCompatible(exp_type, fld_type))
                {
#if V2
                    StructDeclaration * sd;
                    if ((sd = needsPostblit(fld_type)) != NULL)
                    {   // Generate _d_arrayctor(ti, from = exp, to = exp_tree)
                        Type * ti = fld_type->nextOf();
                        exp_tree = irs->localVar(exp_type);
                        tree args[3] = {
                            irs->typeinfoReference(ti),
                            irs->toDArray(exp),
                            irs->convertTo(exp_tree, exp_type, ti->arrayOf())
                        };
                        call_exp = irs->libCall(LIBCALL_ARRAYCTOR, 3, args);
                    }
                    else
#endif
                    {   // %% This would call _d_newarrayT ... use memcpy?
                        exp_tree = irs->convertTo(exp, fld->type);
                    }
                }
                else
                {   // %% Could use memset if is zero init...
                    exp_tree = irs->localVar(fld_type);
                    Type * etype = fld_type;
                    while (etype->ty == Tsarray)
                        etype = etype->nextOf();

                    gcc_assert(fld_type->size() % etype->size() == 0);
                    tree size = fold_build2(TRUNC_DIV_EXPR, size_type_node,
                                            size_int(fld_type->size()), size_int(etype->size()));

                    tree ptr_tree = irs->nop(irs->addressOf(exp_tree),
                                    etype->pointerTo()->toCtype());
                    tree set_exp = array_set_expr(irs, ptr_tree, exp->toElem(irs), size);
                    exp_tree = irs->compound(set_exp, exp_tree);
                }
            }
            else
            {
                exp_tree = irs->convertTo(exp, fld->type);
#if V2
                StructDeclaration * sd;
                if ((sd = needsPostblit(fld_type)) != NULL)
                {   // Call __postblit(&exp_tree)
                    Expressions args;
                    call_exp = irs->call(sd->postblit, irs->addressOf(exp_tree), & args);
                }
#endif
            }

            if (call_exp)
                irs->addExp(call_exp);

            ce.cons(fld->toSymbol()->Stree, exp_tree);
        }
    }
#if V2
    if (sd->isNested())
    {   // Maybe setup hidden pointer to outer scope context.
        tree vthis_field = sd->vthis->toSymbol()->Stree;
        tree vthis_value = irs->getVThis(sd, this);
        ce.cons(vthis_field, vthis_value);
    }
#endif
    tree ctor = build_constructor(type->toCtype(), ce.head);
    return ctor;
}

elem *
NullExp::toElem(IRState * irs)
{
    TY base_ty = type->toBasetype()->ty;
    tree null_exp;
    // 0 -> dynamic array.  This is a special case conversion.
    // Move to convert for convertTo if it shows up elsewhere.
    switch (base_ty)
    {
        case Tarray:
        {
            null_exp = irs->darrayVal(type, 0, NULL);
            break;
        }
        case Taarray:
        {
            CtorEltMaker ce;
            tree ttype = type->toCtype();
            tree fa = TYPE_FIELDS(ttype);

            ce.cons(fa, convert(TREE_TYPE(fa), integer_zero_node));
            null_exp = build_constructor(ttype, ce.head);
            break;
        }
        case Tdelegate:
        {   // makeDelegateExpression ?
            null_exp = irs->delegateVal(d_null_pointer, d_null_pointer, type);
            break;
        }
        default:
        {
            null_exp = convert(type->toCtype(), integer_zero_node);
            break;
        }
    }

    return null_exp;
}

elem *
ThisExp::toElem(IRState * irs)
{
    tree this_tree = NULL_TREE;

    if (var)
    {
        this_tree = irs->var(var->isVarDeclaration());
    }
    else
    {
        // %% DMD issue -- creates ThisExp without setting var to vthis
        // %%TODO: updated in 0.79-0.81?
        gcc_assert(irs->func);
        gcc_assert(irs->func->vthis);
        this_tree = irs->var(irs->func->vthis);
    }
#if STRUCTTHISREF
    if (type->ty == Tstruct)
        this_tree = irs->indirect(this_tree);
#endif
    return this_tree;
}

elem *
ComplexExp::toElem(IRState * irs)
{
    TypeBasic * compon_type;
    switch (type->toBasetype()->ty)
    {
        case Tcomplex32: compon_type = (TypeBasic *) Type::tfloat32; break;
        case Tcomplex64: compon_type = (TypeBasic *) Type::tfloat64; break;
        case Tcomplex80: compon_type = (TypeBasic *) Type::tfloat80; break;
        default:
            gcc_unreachable();
    }
    return build_complex(type->toCtype(),
            irs->floatConstant(creall(value), compon_type),
            irs->floatConstant(cimagl(value), compon_type));
}

elem *
RealExp::toElem(IRState * irs)
{
    return irs->floatConstant(value, type->toBasetype());
}

elem *
IntegerExp::toElem(IRState * irs)
{
    return irs->integerConstant(value, type->toBasetype());
}


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
                (!DECL_ASSEMBLER_NAME_SET_P (fndecl) ? "null"
                 : IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl))));
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


void
FuncDeclaration::toObjFile(int /*multiobj*/)
{
    if (! global.params.useUnitTests && isUnitTestDeclaration())
        return;
    if (! g.ofile->shouldEmit(this))
        return;

    Symbol * this_sym = toSymbol();
    if (this_sym->outputStage)
        return;

    if (g.irs->shouldDeferFunction(this))
        return;

    this_sym->outputStage = InProgress;

    if (global.params.verbose)
        fprintf(stderr, "function %s\n", this->toChars());

    tree fn_decl = this_sym->Stree;

    if (! fbody)
    {
        if (! isNested())
        {   // %% Should set this earlier...
            DECL_EXTERNAL(fn_decl) = 1;
            TREE_PUBLIC(fn_decl) = 1;
        }
        g.ofile->rodc(fn_decl, 1);
        return;
    }

    announce_function(fn_decl);
    IRState * irs = IRState::startFunction(this);

    irs->useChain(NULL, NULL_TREE);
    tree chain_expr = NULL;
    FuncDeclaration * chain_func = NULL;

    // in 4.0, doesn't use push_function_context
    tree old_current_function_decl = current_function_decl;
    function * old_cfun = cfun;

    current_function_decl = fn_decl;
    TREE_STATIC(fn_decl) = 1;

    TypeFunction * tf = (TypeFunction *)type;
    tree result_decl = NULL_TREE;
    {
        Type * func_type = tintro ? tintro : tf;
        Type * ret_type = func_type->nextOf()->toBasetype();

        if (isMain() && ret_type->ty == Tvoid)
            ret_type = Type::tint32;

        result_decl = ret_type->toCtype();
#if V2
        /* Build reference type */
        if (tf->isref)
            result_decl = build_reference_type(result_decl);
#endif
        result_decl = d_build_decl(RESULT_DECL, NULL_TREE, result_decl);
    }
    g.ofile->setDeclLoc(result_decl, this);
    DECL_RESULT(fn_decl) = result_decl;
    DECL_CONTEXT(result_decl) = fn_decl;
    //layout_decl(result_decl, 0);

#if D_GCC_VER >= 43
    allocate_struct_function(fn_decl, false);
#else
    allocate_struct_function(fn_decl);
#endif
    // assuming the above sets cfun
    g.ofile->setCfunEndLoc(endloc);

    AggregateDeclaration * ad = isThis();
    tree parm_decl = NULL_TREE;
    tree param_list = NULL_TREE;

    int n_parameters = parameters ? parameters->dim : 0;

    int reverse_args = 0; //tf->linkage == LINKd && tf->varargs != 1;

    // Special arguments...
    static const int VTHIS = -2;
    static const int VARGUMENTS = -1;

    for (int i = VTHIS; i < (int) n_parameters; i++)
    {
        VarDeclaration * param = 0;
        tree parm_type = 0;

        parm_decl = 0;

        if (i == VTHIS)
        {
            if (ad != NULL)
            {
                param = vthis;
            }
            else if (isNested())
            {   /* DMD still generates a vthis, but it should not be
                   referenced in any expression.

                   This parameter is hidden from the debugger.
                 */
                parm_type = ptr_type_node;
                parm_decl = d_build_decl(PARM_DECL, NULL_TREE, parm_type);
                DECL_ARTIFICIAL(parm_decl) = 1;
                DECL_IGNORED_P(parm_decl) = 1;
                // %% doc need this arg silently disappears
                DECL_ARG_TYPE (parm_decl) = TREE_TYPE (parm_decl);

                chain_func = toParent2()->isFuncDeclaration();
                chain_expr = parm_decl;
            }
            else
                continue;
        }
        else if (i == VARGUMENTS)
        {
            if (v_arguments /*varargs && linkage == LINKd*/)
            {
                param = v_arguments;
            }
            else
                continue;
        }
        else
        {
            param = parameters->tdata()[i];
        }
        if (param)
        {
            parm_decl = param->toSymbol()->Stree;
        }

        gcc_assert(parm_decl != NULL_TREE);
        DECL_CONTEXT (parm_decl) = fn_decl;
        // param->loc is not set, so use the function's loc
        // %%doc not setting this crashes debug generating code
        g.ofile->setDeclLoc(parm_decl, param ? (Dsymbol*) param : (Dsymbol*) this);

        // chain them in the correct order
        if (reverse_args)
        {
            param_list = chainon (parm_decl, param_list);
        }
        else
        {
            param_list = chainon (param_list, parm_decl);
        }
    }

    // param_list is a number of PARM_DECL trees chained together (*not* a TREE_LIST of PARM_DECLs).
    // The leftmost parameter is the first in the chain.  %% varargs?
    // %% in treelang, useless ? because it just sets them to getdecls() later
    DECL_ARGUMENTS(fn_decl) = param_list;

    // http://www.tldp.org/HOWTO/GCC-Frontend-HOWTO-7.html
    rest_of_decl_compilation(fn_decl, /*toplevel*/1, /*atend*/0);

    // ... has this here, but with more args...

    DECL_INITIAL(fn_decl) = error_mark_node; // Just doing what they tell me to do...

    IRState::initFunctionStart(fn_decl, loc);
    cfun->naked = naked ? 1 : 0;
    pushlevel(0);
    irs->pushStatementList();

    irs->startScope();
    irs->doLineNote(loc);

    /* If this is a member function that nested (possibly indirectly) in another
       function, construct an expession for this member function's static chain
       by going through parent link of nested classes.
    */
    //if (! irs->functionNeedsChain(this))
    {
        /* D 2.0 Closures: this->vthis is passed as a normal parameter and
           is valid to access as Stree before the closure frame is created. */
        ClassDeclaration * cd = ad ? ad->isClassDeclaration() : NULL;
        StructDeclaration * sd = ad ? ad->isStructDeclaration() : NULL;

        if (cd != NULL)
        {
            FuncDeclaration *f = NULL;
            tree t = vthis->toSymbol()->Stree;
            while (cd->isNested())
            {
                Dsymbol * d = cd->toParent2();
                tree vthis_field = cd->vthis->toSymbol()->Stree;
                t = irs->component(irs->indirect(t), vthis_field);
                if ((f = d->isFuncDeclaration()))
                {
                    chain_expr = t;
                    chain_func = f;
                    break;
                }
                else if (! (cd = d->isClassDeclaration()))
                    gcc_unreachable();
            }
        }
#if V2
        else if (sd != NULL)
        {
            FuncDeclaration *f = NULL;
            tree t = vthis->toSymbol()->Stree;
            while (sd->isNested())
            {
                Dsymbol * d = sd->toParent2();
                tree vthis_field = sd->vthis->toSymbol()->Stree;
                t = irs->component(irs->indirect(t), vthis_field);
                if ((f = d->isFuncDeclaration()))
                {
                    chain_expr = t;
                    chain_func = f;
                    break;
                }
                else if (! (sd = d->isStructDeclaration()))
                    gcc_unreachable();
            }
        }
#endif
    }

    if (isNested() || chain_expr)
        irs->useParentChain();

    if (chain_expr)
    {
        if (! DECL_P(chain_expr))
        {
            tree c = irs->localVar(ptr_type_node);
            DECL_INITIAL(c) = chain_expr;
            irs->expandDecl(c);
            chain_expr = c;
        }
        irs->useChain(chain_func, chain_expr);
    }

    irs->buildChain(this); // may change irs->chainLink and irs->chainFunc
    DECL_LANG_SPECIFIC(fn_decl) = build_d_decl_lang_specific(this);

    if (vresult)
        irs->emitLocalVar(vresult);

    if (v_argptr)
        irs->pushStatementList();
    if (v_arguments_var)
        irs->emitLocalVar(v_arguments_var, true);

    Statement * the_body = fbody;
    if (isSynchronized())
    {
        AggregateDeclaration * asym;
        ClassDeclaration * sym;

        if ((asym = isMember()) && (sym = asym->isClassDeclaration()))
        {
            if (vthis != NULL)
            {
                VarExp * ve = new VarExp(fbody->loc, vthis);
                the_body = new SynchronizedStatement(fbody->loc, ve, fbody);
            }
            else
            {
                if (!sym->vclassinfo)
                {
#if V2
                    sym->vclassinfo = new TypeInfoClassDeclaration(sym->type);
#else
                    sym->vclassinfo = new ClassInfoDeclaration(sym);
#endif
                }
                Expression * e = new VarExp(fbody->loc, sym->vclassinfo);
                e = new AddrExp(fbody->loc, e);
                e->type = sym->type;
                the_body = new SynchronizedStatement(fbody->loc, e, fbody);
            }
        }
        else
        {
            error("synchronized function %s must be a member of a class", toChars());
        }
    }

    the_body->toIR(irs);

    if (this_sym->otherNestedFuncs)
    {
        for (size_t i = 0; i < this_sym->otherNestedFuncs->dim; ++i)
        {
            (this_sym->otherNestedFuncs->tdata()[i])->toObjFile(false);
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
    /* Also applies to integral types, where the result in EAX gets
       written with '0' before return under certain optimisations.
     */
    /* This would apply to complex types as well, but GDC currently
       returns complex types as a struct instead of in ST(0) and ST(1).
     */
    if (hasReturnExp & 8 /*inlineAsm*/ && ! naked)
    {
        tree cns_str = NULL_TREE;

        if (type->nextOf()->isreal())
            cns_str = build_string(2, "=t");
        else
        {   // On 32bit, can't return 'long' value in EAX.
            if (type->nextOf()->isintegral() &&
                type->nextOf()->size() <= Type::tsize_t->size())
            {
                cns_str = build_string(2, "=a");
            }
        }

        if (cns_str != NULL_TREE)
        {
            tree result_var = irs->localVar(TREE_TYPE(result_decl));
            tree nop_str = build_string(0, "");

            tree out_arg = tree_cons(tree_cons(NULL_TREE, cns_str, NULL_TREE),
                                     result_var, NULL_TREE);

            irs->expandDecl(result_var);
            irs->doAsm(nop_str, out_arg, NULL_TREE, NULL_TREE);
            irs->doReturn(build2(MODIFY_EXPR, TREE_TYPE(result_decl),
                          result_decl, result_var));
        }
    }
#endif


    if (v_argptr)
    {
        tree body = irs->popStatementList();
        tree var = irs->var(v_argptr);
        var = irs->addressOf(var);

        tree init_exp = irs->buildCall(built_in_decls[BUILT_IN_VA_START], 2, var, parm_decl);
        v_argptr->init = NULL; // VoidInitializer?
        irs->emitLocalVar(v_argptr, true);
        irs->addExp(init_exp);

        tree cleanup = irs->buildCall(built_in_decls[BUILT_IN_VA_END], 1, var);
        irs->addExp(build2(TRY_FINALLY_EXPR, void_type_node, body, cleanup));
    }

    irs->endScope();

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
        if (TREE_CODE(t) != STATEMENT_LIST)
        {
            tree sl = alloc_stmt_list();
            append_to_statement_list_force(t, & sl);
            TREE_OPERAND(body, 1) = sl;
        }
        else if (! STATEMENT_LIST_HEAD(t))
        {   /* For empty functions: Without this, there is a
               segfault when inlined.  Seen on build=ppc-linux but
               not others (why?). */
            append_to_statement_list_force(
               build1(RETURN_EXPR,void_type_node,NULL_TREE), & t);
        }
    }
    //block = (*lang_hooks.decls.poplevel) (1, 0, 1);
    tree block = poplevel(1, 0, 1);

    DECL_INITIAL (fn_decl) = block; // %% redundant, see poplevel
    BLOCK_SUPERCONTEXT(DECL_INITIAL (fn_decl)) = fn_decl; // done in C, don't know effect

    if (! errorcount && ! global.errors)
        genericize_function (fn_decl);

    this_sym->outputStage = Finished;
    if (! errorcount && ! global.errors)
        g.ofile->outputFunction(this);

    current_function_decl = old_current_function_decl; // must come before endFunction
    set_cfun(old_cfun);

    irs->endFunction();
}

#if V2
void
FuncDeclaration::buildClosure(IRState * irs)
{
    FuncFrameInfo * ffi = irs->getFrameInfo(this);
    if (! ffi->creates_closure)
        return;

    tree closure_rec_type = make_node(RECORD_TYPE);
    char *name = concat ("CLOSURE.",
                         IDENTIFIER_POINTER (DECL_NAME (toSymbol()->Stree)),
                         NULL);
    TYPE_NAME (closure_rec_type) = get_identifier (name);
    free(name);

    tree ptr_field = d_build_decl_loc(BUILTINS_LOCATION, FIELD_DECL,
                                      get_identifier("__closptr"), ptr_type_node);
    DECL_CONTEXT(ptr_field) = closure_rec_type;
    ListMaker fields;
    fields.chain(ptr_field);

    for (size_t i = 0; i < closureVars.dim; ++i)
    {
        VarDeclaration *v = closureVars.tdata()[i];
        Symbol * s = v->toSymbol();
        tree field = d_build_decl(FIELD_DECL,
                                  v->ident ? get_identifier(v->ident->string) : NULL_TREE,
                                  gen.trueDeclarationType(v));
        s->SframeField = field;
        g.ofile->setDeclLoc(field, v);
        DECL_CONTEXT(field) = closure_rec_type;
        fields.chain(field);
        TREE_USED(s->Stree) = 1;
    }
    TYPE_FIELDS(closure_rec_type) = fields.head;
    layout_type(closure_rec_type);

    ffi->closure_rec = closure_rec_type;

    tree closure_ptr = irs->localVar(build_pointer_type(closure_rec_type));
    DECL_NAME(closure_ptr) = get_identifier("__closptr");
    DECL_IGNORED_P(closure_ptr) = 0;

    tree arg = d_convert_basic(Type::tsize_t->toCtype(),
        TYPE_SIZE_UNIT(closure_rec_type));

    DECL_INITIAL(closure_ptr) =
        irs->nop(irs->libCall(LIBCALL_ALLOCMEMORY, 1, & arg),
                 TREE_TYPE(closure_ptr));
    irs->expandDecl(closure_ptr);

    // set the first entry to the parent closure, if any
    tree cl = irs->chainLink();
    if (cl)
        irs->doExp(irs->vmodify(irs->component(irs->indirect(closure_ptr),
                   ptr_field), cl));

    // copy parameters that are referenced nonlocally
    for (size_t i = 0; i < closureVars.dim; i++)
    {
        VarDeclaration *v = closureVars.tdata()[i];
        if (! v->isParameter())
            continue;

        Symbol * vsym = v->toSymbol();
        irs->doExp(irs->vmodify(irs->component(irs->indirect(closure_ptr),
                   vsym->SframeField), vsym->Stree));
    }

    irs->useChain(this, closure_ptr);
}
#endif

void
Module::genobjfile(int multiobj)
{
    /* Normally would create an ObjFile here, but gcc is limited to one obj file
       per pass and there may be more than one module per obj file. */
    gcc_assert(g.ofile);

    g.ofile->beginModule(this);
    g.ofile->setupStaticStorage(this, toSymbol()->Stree);

    if (members)
    {
        for (size_t i = 0; i < members->dim; i++)
        {
            Dsymbol * dsym = members->tdata()[i];
            dsym->toObjFile(multiobj);
        }
    }

    // Always generate module info.
    if (1 || needModuleInfo())
    {
        ModuleInfo & mi = * g.mi();
        if (mi.ctors.dim || mi.ctorgates.dim)
            sctor = g.ofile->doCtorFunction("*__modctor", & mi.ctors, & mi.ctorgates)->toSymbol();
        if (mi.dtors.dim)
            sdtor = g.ofile->doDtorFunction("*__moddtor", & mi.dtors)->toSymbol();
#if V2
        if (mi.sharedctors.dim || mi.sharedctorgates.dim)
            ssharedctor = g.ofile->doCtorFunction("*__modsharedctor",
                                                  & mi.sharedctors, & mi.sharedctorgates)->toSymbol();
        if (mi.shareddtors.dim)
            sshareddtor = g.ofile->doDtorFunction("*__modshareddtor", & mi.shareddtors)->toSymbol();
#endif
        if (mi.unitTests.dim)
            stest = g.ofile->doUnittestFunction("*__modtest", & mi.unitTests)->toSymbol();

        genmoduleinfo();
    }

    g.ofile->endModule();
}

unsigned
Type::totym()
{
    return 0;   // Unused
}

type *
Type::toCtype()
{
    if (! ctype)
    {
        switch (ty)
        {
            case Tvoid:
                ctype = void_type_node;
                break;
            case Tint8:
                ctype = intQI_type_node;
                break;
            case Tuns8:
                ctype = unsigned_intQI_type_node;
                break;
            case Tint16:
                ctype = intHI_type_node;
                break;
            case Tuns16:
                ctype = unsigned_intHI_type_node;
                break;
            case Tint32:
                ctype = intSI_type_node;
                break;
            case Tuns32:
                ctype = unsigned_intSI_type_node;
                break;
            case Tint64:
                ctype = intDI_type_node;
                break;
            case Tuns64:
                ctype = unsigned_intDI_type_node;
                break;
            case Tfloat32:
                ctype = float_type_node;
                break;
            case Tfloat64:
                ctype = double_type_node;
                break;
            case Tfloat80:
                ctype = long_double_type_node;
                break;
            case Tcomplex32:
                ctype = complex_float_type_node;
                break;
            case Tcomplex64:
                ctype = complex_double_type_node;
                break;
            case Tcomplex80:
                ctype = complex_long_double_type_node;
                break;
            case Tbool:
                if (int_size_in_bytes(boolean_type_node) == 1)
                    ctype = boolean_type_node;
                else
                    ctype = d_boolean_type_node;
                break;
            case Tchar:
                ctype = d_char_type_node;
                break;
            case Twchar:
                ctype = d_wchar_type_node;
                break;
            case Tdchar:
                ctype = d_dchar_type_node;
                break;
            case Timaginary32:
                ctype = d_ifloat_type_node;
                break;
            case Timaginary64:
                ctype = d_idouble_type_node;
                break;
            case Timaginary80:
                ctype = d_ireal_type_node;
                break;

            case Terror:
                return error_mark_node;

                /* We can get Tident with forward references.  There seems to
                   be a legitame case (dstress:debug_info_03).  I have not seen this
                   happen for an error, so maybe it's okay...
                   
                   Update:
                   Seems to be fixed in the frontend, so changed to an error
                   to catch potential bugs.
                 */
            case Tident:
            case Ttypeof:
                ::error("forward reference of %s\n", this->toChars());
                return error_mark_node;

                /* Valid case for Ttuple is in CommaExp::toElem, in instances when
                   a tuple has been expanded as a large chain of comma expressions.
                 */
            case Ttuple:
                ctype = void_type_node;
                break;

            default:
                ::error("unexpected call to Type::toCtype() for %s\n", this->toChars());
                gcc_unreachable();
                return NULL_TREE;
        }
    }
    return gen.addTypeModifiers(ctype, mod);
}

// This is not used for GCC
type *
Type::toCParamtype()
{
    return toCtype();
}

// This is not used for GCC
Symbol *
Type::toSymbol()
{
    return NULL;
}

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
    if (! ctype)
    {
        tree base_type = sym->basetype->toCtype();
        const char * name = toChars();

        tree ident = get_identifier(name);
        tree type_node = build_variant_type_copy(base_type);
        tree type_decl = d_build_decl(TYPE_DECL, ident, type_node);
        TYPE_NAME(type_node) = type_decl;
        apply_type_attributes(sym->attributes, type_node);

        /* %% TODO ?
        DECL_CONTEXT(type_decl) =
        rest_of_decl_compilation(type_decl, NULL, ?context?, 0); //%% flag
         */
        ctype = type_node;
    }
    return ctype;
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
    if (! ctype)
    {   /* Enums in D2 can have a base type that is not necessarily integral.
           So don't bother trying to make an ENUMERAL_TYPE using them.  */
        if (! sym->memtype->isintegral())
        {
            ctype = sym->memtype->toCtype();
        }
        else
        {
            tree enum_mem_type_node = sym->memtype->toCtype();

            ctype = make_node(ENUMERAL_TYPE);
            // %% c-decl.c: if (flag_short_enums) TYPE_PACKED(enumtype) = 1;
            TYPE_PRECISION(ctype) = size(0) * 8;
            TYPE_SIZE(ctype) = 0; // as in c-decl.c
            TREE_TYPE(ctype) = enum_mem_type_node;
            apply_type_attributes(sym->attributes, ctype, true);
#if V2
            /* Because minval and maxval are of this type,
               ctype needs to be completed enough for
               build_int_cst to work properly. */
            TYPE_MIN_VALUE(ctype) = sym->minval->toElem(& gen);
            TYPE_MAX_VALUE(ctype) = sym->maxval->toElem(& gen);
#else
            TYPE_MIN_VALUE(ctype) = gen.integerConstant(sym->minval, enum_mem_type_node);
            TYPE_MAX_VALUE(ctype) = gen.integerConstant(sym->maxval, enum_mem_type_node);
#endif
            layout_type(ctype);
            TYPE_UNSIGNED(ctype) = isunsigned() != 0; // layout_type can change this

            // Move this to toDebug() ?
            ListMaker enum_values;
            if (sym->members)
            {
                for (size_t i = 0; i < sym->members->dim; i++)
                {
                    EnumMember * member = (sym->members->tdata()[i])->isEnumMember();
                    gcc_assert(member);

                    char * ident = NULL;
                    if (sym->ident)
                        ident = concat(sym->ident->string, ".",
                                       member->ident->string, NULL);

                    enum_values.cons(get_identifier(ident ? ident : member->ident->string),
                            gen.integerConstant(member->value->toInteger(), ctype));

                    if (sym->ident)
                        free(ident);
                }
            }
            TYPE_VALUES(ctype) = enum_values.head;

            g.ofile->initTypeDecl(ctype, sym);
            g.ofile->declareType(ctype, sym);
        }
    }
    return ctype;
}

type *
TypeStruct::toCtype()
{
    if (! ctype)
    {   // need to set this right away in case of self-references
        ctype = make_node(sym->isUnionDeclaration() ? UNION_TYPE : RECORD_TYPE);

        TYPE_LANG_SPECIFIC(ctype) = build_d_type_lang_specific(this);

        /* %% copied from AggLayout::finish -- also have to set the size
           for (indirect) self-references. */

        /* Must set up the overall size, etc. before determining the
           context or laying out fields as those types may make references
           to this type. */
        TYPE_SIZE(ctype) = bitsize_int(sym->structsize * BITS_PER_UNIT);
        TYPE_SIZE_UNIT(ctype) = size_int(sym->structsize);
        TYPE_ALIGN(ctype) = sym->alignsize * BITS_PER_UNIT; // %%doc int, not a tree
        // TYPE_ALIGN_UNIT is not an lvalue
        TYPE_PACKED (ctype) = TYPE_PACKED (ctype); // %% todo
        apply_type_attributes(sym->attributes, ctype, true);
        compute_record_mode (ctype);

        // %%  stor-layout.c:finalize_type_size ... it's private to that file

        TYPE_CONTEXT(ctype) = gen.declContext(sym);

#if D_GCC_VER >= 43
        // For aggregates GCC now relies on TYPE_CANONICAL exclusively
        // to show that two variant types are structurally equal.
        // %% Now conversions now handled in d_gimplify_expr.
        SET_TYPE_STRUCTURAL_EQUALITY(ctype);
#endif
        g.ofile->initTypeDecl(ctype, sym);

        AggLayout agg_layout(sym, ctype);
        agg_layout.go();

        /* On PowerPC 64, GCC may not always clear the padding at the end
           of the struct. Adding 32-bit words at the end helps. */
        if (global.params.is64bit && ! sym->isUnionDeclaration() && sym->fields.dim)
        {
            size_t ofs;
            {
                VarDeclaration * last_decl = sym->fields.tdata()[sym->fields.dim-1];
                ofs = last_decl->offset + last_decl->size(0);
            }
            while (ofs & 3)
                ++ofs;
            while (ofs < sym->structsize && sym->structsize - ofs >= 4)
            {
                tree f = d_build_decl(FIELD_DECL, get_identifier("_pad"), d_type_for_size(32, 1));
                DECL_FCONTEXT(f) = ctype;
                DECL_ARTIFICIAL(f) = DECL_IGNORED_P(f) = 1;
                DECL_IGNORED_P(f) = 1;
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
    rest_of_type_compilation(ctype, /*toplevel*/1);
}

Symbol *
TypeClass::toSymbol()
{
    return sym->toSymbol();
}

unsigned
TypeFunction::totym()
{
    return 0;   // Unused
}

type *
TypeFunction::toCtype()
{
    // %%TODO: If x86, and D linkage, use regparm(1)

    if (! ctype)
    {
        ListMaker type_list;
        tree ret_type;

        if (varargs == 1 && linkage == LINKd)
        {   // hidden _arguments parameter
#if BREAKABI
            type_list.cons(Type::typeinfotypelist->type->toCtype());
#else
            type_list.cons(Type::typeinfo->type->arrayOf()->toCtype());
#endif
        }

        if (parameters)
        {
            size_t n_args = Parameter::dim(parameters);
            
            for (size_t i = 0; i < n_args; i++)
            {
                Parameter * arg = Parameter::getNth(parameters, i);
                type_list.cons(IRState::trueArgumentType(arg));
            }
        }

        /* Last parm if void indicates fixed length list (as opposed to
           printf style va_* list). */
        if (varargs != 1)
            type_list.cons(void_type_node);

        if (next)
            ret_type = next->toCtype();
        else
            ret_type = void_type_node;
#if V2
        if (isref)
            ret_type = build_reference_type(ret_type);
#endif

        // Function type can be reference by parameters, etc.  Set ctype earlier?
        ctype = build_function_type(ret_type, type_list.head);
        TYPE_LANG_SPECIFIC(ctype) = build_d_type_lang_specific(this);

        switch (linkage)
        {
            case LINKpascal:
                // stdcall and reverse params?
            case LINKwindows:
                if (! global.params.is64bit)
                    ctype = gen.addTypeAttribute(ctype, "stdcall");
                break;

            case LINKc:
            case LINKcpp:
                break;

            case LINKd:
#if D_DMD_CALLING_CONVENTIONS \
        && defined(TARGET_386)
                ctype = gen.addTypeAttribute(ctype, "optlink");
#endif
                break;

            default:
                fprintf(stderr, "linkage = %d\n", linkage);
                gcc_unreachable();
        }
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
    if (! ctype)
    {
        if (dim->isConst() && dim->type->isintegral())
        {
            uinteger_t size = dim->toUInteger();
            if (next->toBasetype()->ty == Tvoid)
                ctype = gen.arrayType(Type::tuns8, size);
            else
                ctype = gen.arrayType(next, size);
        }
        else
        {
            ::error("invalid expressions for static array dimension: %s", dim->toChars());
            gcc_unreachable();
        }
    }
    return gen.addTypeModifiers(ctype, mod);
}

type *
TypeSArray::toCParamtype()
{
#if SARRAYVALUE
    return toCtype();
#else
    return next->pointerTo()->toCtype();
#endif
}

type *
TypeDArray::toCtype()
{
    if (! ctype)
    {
        ctype = gen.twoFieldType(Type::tsize_t, next->pointerTo(), this,
                                 "length", "ptr");
#if D_GCC_VER >= 43
        // Don't rely on tree type information to determine whether
        // two distinct D types: ie: const(T[]) and const(T)[]
        // should be treated as two distinct types in GCC.
        SET_TYPE_STRUCTURAL_EQUALITY(ctype);
#endif
        TYPE_LANG_SPECIFIC(ctype) = build_d_type_lang_specific(this);
    }
    return gen.addTypeModifiers(ctype, mod);
}

type *
TypeAArray::toCtype()
{
    /* Dependencies:

       IRState::convertForCondition
       more...

    */

    if (! ctype)
    {   /* Library functions expect a struct-of-pointer which could be passed
           differently from a pointer. */
        static tree aa_type = NULL_TREE;
        if (! aa_type)
        {
            aa_type = make_node(RECORD_TYPE);
            tree f0 = d_build_decl_loc(BUILTINS_LOCATION, FIELD_DECL,
                                       get_identifier("ptr"), ptr_type_node);
            DECL_CONTEXT(f0) = aa_type;
            TYPE_FIELDS(aa_type) = f0;
            TYPE_NAME(aa_type) = get_identifier(toChars());
            layout_type(aa_type);
            TYPE_LANG_SPECIFIC(aa_type) = build_d_type_lang_specific(this);
        }
        ctype = aa_type;
    }
    return gen.addTypeModifiers(ctype, mod);
}

type *
TypePointer::toCtype()
{
    if (! ctype)
    {
        ctype = build_pointer_type(next->toCtype());
    }
    return gen.addTypeModifiers(ctype, mod);
}

type *
TypeDelegate::toCtype()
{
    if (! ctype)
    {
        gcc_assert(next->toBasetype()->ty == Tfunction);
        ctype = gen.twoFieldType(Type::tvoidptr, next->pointerTo(),
                                 this, "object", "func");
        TYPE_LANG_SPECIFIC(ctype) = build_d_type_lang_specific(this);
    }
    return gen.addTypeModifiers(ctype, mod);
}

/* Create debug information for a ClassDeclaration's inheritance tree.
   Interfaces are not included. */
static tree
binfo_for(tree tgt_binfo, ClassDeclaration * cls)
{
    tree binfo = make_tree_binfo(1);
    TREE_TYPE              (binfo) = TREE_TYPE(cls->type->toCtype()); // RECORD_TYPE, not REFERENCE_TYPE
    BINFO_INHERITANCE_CHAIN(binfo) = tgt_binfo;
    BINFO_OFFSET           (binfo) = size_zero_node; // %% type?, otherwize, integer_zero_node

    if (cls->baseClass)
    {
        BINFO_BASE_APPEND(binfo, binfo_for(binfo, cls->baseClass));
#ifdef BINFO_BASEACCESSES
#error update vector stuff
        tree prot_tree;

        BINFO_BASEACCESSES(binfo) = make_tree_vec(1);
        BaseClass * bc = (BaseClass *) cls->baseclasses->data[0];
        switch (bc->protection)
        {
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
    tree binfo = make_tree_binfo(iface->baseclasses->dim);

    TREE_TYPE              (binfo) = TREE_TYPE(iface->type->toCtype()); // RECORD_TYPE, not REFERENCE_TYPE
    BINFO_INHERITANCE_CHAIN(binfo) = tgt_binfo;
    BINFO_OFFSET           (binfo) = size_int(inout_offset * PTRSIZE);

    if (iface->baseclasses->dim)
    {
#ifdef BINFO_BASEACCESSES
        BINFO_BASEACCESSES(binfo) = make_tree_vec(iface->baseclasses->dim);
#endif
    }
    for (size_t i = 0; i < iface->baseclasses->dim; i++)
    {
        BaseClass * bc = iface->baseclasses->tdata()[i];

        if (i)
            inout_offset++;

        BINFO_BASE_APPEND(binfo, intfc_binfo_for(binfo, bc->base, inout_offset));
#ifdef BINFO_BASEACCESSES
        tree prot_tree;
        switch (bc->protection)
        {
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

    return binfo;
}

type *
TypeClass::toCtype()
{
    if (! ctype)
    {
        tree rec_type;
        Array base_class_decls;
        bool inherited = sym->baseClass != 0;
        tree obj_rec_type;
        tree vfield;

        /* Need to set ctype right away in case of self-references to
           the type during this call. */
        rec_type = make_node(RECORD_TYPE);
        //apply_type_attributes(sym->attributes, rec_type, true);
        ctype = build_reference_type(rec_type);
        dkeep(ctype); // because BINFO moved out to toDebug
        g.ofile->initTypeDecl(rec_type, sym);

        obj_rec_type = TREE_TYPE(gen.getObjectType()->toCtype());

        // Note that this is set on the reference type, not the record type.
        TYPE_LANG_SPECIFIC(ctype) = build_d_type_lang_specific(this);

        AggLayout agg_layout(sym, rec_type);

        // Most of this silly code is just to produce correct debugging information.

        /* gdb apparently expects the vtable field to be named
           "_vptr$...." (stabsread.c) Otherwise, the debugger gives
           lots of annoying error messages.  C++ appends the class
           name of the first base witht that field after the '$'. */
        /* update: annoying messages might not appear anymore after making
           other changes */
        // Add the virtual table pointer
        tree decl = d_build_decl(FIELD_DECL, get_identifier("_vptr$"), /*vtbl_type*/d_vtbl_ptr_type_node);
        agg_layout.addField(decl, 0); // %% target stuff..

        if (inherited)
        {
            vfield = copy_node(decl);
            DECL_ARTIFICIAL(decl) = DECL_IGNORED_P(decl) = 1;
        }
        else
        {
            vfield = decl;
        }
        DECL_VIRTUAL_P(vfield) = 1;
        TYPE_VFIELD(rec_type) = vfield; // This only seems to affect debug info

        if (! sym->isInterfaceDeclaration())
        {
            DECL_FCONTEXT(vfield) = obj_rec_type;

            // Add the monitor
            // %% target type
            decl = d_build_decl(FIELD_DECL, get_identifier("_monitor"), ptr_type_node);
            DECL_FCONTEXT(decl) = obj_rec_type;
            DECL_ARTIFICIAL(decl) = DECL_IGNORED_P(decl) = inherited;
            agg_layout.addField(decl, PTRSIZE);

            // Add the fields of each base class
            agg_layout.go();
        }
        else
        {
            ClassDeclaration * p = sym;
            while (p->baseclasses->dim)
                p = (p->baseclasses->tdata()[0])->base;

            DECL_FCONTEXT(vfield) = TREE_TYPE(p->type->toCtype());
        }

        TYPE_CONTEXT(rec_type) = gen.declContext(sym);

        agg_layout.finish(sym->attributes);
    }
    return ctype;
}

void
ClassDeclaration::toDebug()
{
    tree rec_type = TREE_TYPE(type->toCtype());
    /* Used to create BINFO even if debugging was off.  This was needed to keep
       references to inherited types. */

    g.ofile->addAggMethods(rec_type, this);

    if (! isInterfaceDeclaration())
        TYPE_BINFO(rec_type) = binfo_for(NULL_TREE, this);
    else {
        unsigned offset = 0;
        BaseClass bc;
        bc.base = this;
        TYPE_BINFO(rec_type) = intfc_binfo_for(NULL_TREE, this, offset);
    }

    g.ofile->declareType(rec_type, this);
}

void
LabelStatement::toIR(IRState* irs)
{
    FuncDeclaration * func = irs->func;
    LabelDsymbol * label = irs->isReturnLabel(ident) ? func->returnLabel : func->searchLabel(ident);
    tree t_label;

    if ((t_label = irs->getLabelTree(label)))
    {
        irs->pushLabel(label);
        irs->doLabel(t_label);
#if D_GCC_VER < 45
        if (label->asmLabelNum)
            d_expand_priv_asm_label(irs, label->asmLabelNum);
#endif
        if (irs->isReturnLabel(ident) && func->fensure)
            func->fensure->toIR(irs);
        else if (statement)
            statement->toIR(irs);
        if (fwdrefs)
        {
            irs->checkPreviousGoto(fwdrefs);
            delete fwdrefs;
            fwdrefs = NULL;
        }
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

    if ((t_label = irs->getLabelTree(label)))
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
    irs->doLineNote(loc);
    irs->doExp(irs->assertCall(loc, LIBCALL_SWITCH_ERROR));
}

void
VolatileStatement::toIR(IRState* irs)
{
    if (statement)
    {
        irs->pushVolatile();
        statement->toIR(irs);
        irs->popVolatile();
    }
}

void
ThrowStatement::toIR(IRState* irs)
{
    ClassDeclaration * class_decl = exp->type->toBasetype()->isClassHandle();
    // Front end already checks for isClassHandle
    InterfaceDeclaration * intfc_decl = class_decl->isInterfaceDeclaration();
#if V2
    tree arg = exp->toElemDtor(irs);
#else
    tree arg = exp->toElem(irs);
#endif

    if (! flag_exceptions)
    {
        static int warned = 0;
        if (! warned)
        {
            error ("exception handling disabled, use -fexceptions to enable");
            warned = 1;
        }
    }

    if (intfc_decl)
    {
        if (! intfc_decl->isCOMclass())
        {
            arg = irs->convertTo(arg, exp->type, irs->getObjectType());
        }
        else
        {
            error("cannot throw COM interfaces");
        }
    }
    irs->doLineNote(loc);
    irs->doExp(irs->libCall(LIBCALL_THROW, 1, & arg));
    // %%TODO: somehow indicate flow stops here? -- set attribute noreturn on _d_throw
}

void
TryFinallyStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    irs->startTry(this);
    if (body)
    {
        body->toIR(irs);
    }
    irs->startFinally();
    if (finalbody)
    {
        finalbody->toIR(irs);
    }
    irs->endFinally();
}

void
TryCatchStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    irs->startTry(this);
    if (body)
    {
        body->toIR(irs);
    }
    irs->startCatches();
    if (catches)
    {
        for (size_t i = 0; i < catches->dim; i++)
        {
#if V2
            Catch * a_catch = catches->tdata()[i];
#else
            Catch * a_catch = (Catch *) catches->data[i];
#endif

            irs->startCatch(a_catch->type->toCtype()); //expand_start_catch(xxx);
            irs->doLineNote(a_catch->loc);
            irs->startScope();

            if (a_catch->var)
            {
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
    irs->startScope();
    if (wthis)
    {
        irs->emitLocalVar(wthis);
    }
    if (body)
    {
        body->toIR(irs);
    }
    irs->endScope();
}

void
SynchronizedStatement::toIR(IRState * irs)
{
    if (exp)
    {
        InterfaceDeclaration * iface;

        irs->startBindings();
        tree decl = irs->localVar(IRState::getObjectType());

        DECL_IGNORED_P(decl) = 1;
        // assuming no conversions needed
        tree init_exp;

        gcc_assert(exp->type->toBasetype()->ty == Tclass);
        iface = ((TypeClass *) exp->type->toBasetype())->sym->isInterfaceDeclaration();
        if (iface)
        {
            if (! iface->isCOMclass())
            {
                init_exp = irs->convertTo(exp, irs->getObjectType());
            }
            else
            {
                error("cannot synchronize on a COM interface");
                init_exp = error_mark_node;
            }
        }
        else
        {
            init_exp = exp->toElem(irs);
        }
        DECL_INITIAL(decl) = init_exp;
        irs->doLineNote(loc);

        irs->expandDecl(decl);
        irs->doExp(irs->libCall(LIBCALL_MONITORENTER, 1, & decl));
        irs->startTry(this);
        if (body)
        {
            body->toIR(irs);
        }
        irs->startFinally();
        irs->doExp(irs->libCall(LIBCALL_MONITOREXIT, 1, & decl));
        irs->endFinally();
        irs->endBindings();
    }
    else
    {
#ifndef D_CRITSEC_SIZE
#define D_CRITSEC_SIZE 64
#endif
        static tree critsec_type = 0;

        if (! critsec_type)
        {
            critsec_type = irs->arrayType(Type::tuns8, D_CRITSEC_SIZE);
        }
        tree critsec_decl = d_build_decl(VAR_DECL, NULL_TREE, critsec_type);
        // name is only used to prevent ICEs
        g.ofile->giveDeclUniqueName(critsec_decl, "__critsec");
        tree critsec_ref = irs->addressOf(critsec_decl); // %% okay to use twice?
        dkeep(critsec_decl);

        TREE_STATIC(critsec_decl) = 1;
        TREE_PRIVATE(critsec_decl) = 1;
        DECL_ARTIFICIAL(critsec_decl) = 1;
        DECL_IGNORED_P(critsec_decl) = 1;

        g.ofile->rodc(critsec_decl, 1);

        irs->startTry(this);
        irs->doExp(irs->libCall(LIBCALL_CRITICALENTER, 1, & critsec_ref));
        if (body)
        {
            body->toIR(irs);
        }
        irs->startFinally();
        irs->doExp(irs->libCall(LIBCALL_CRITICALEXIT, 1, & critsec_ref));
        irs->endFinally();
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

    if (exp && exp->type->toBasetype()->ty != Tvoid)
    {   // %% == Type::tvoid ?
        FuncDeclaration * func = irs->func;
        TypeFunction * tf = (TypeFunction *)func->type;
        Type * ret_type = func->tintro ?
            func->tintro->nextOf() : tf->nextOf();

        if (func->isMain() && ret_type->toBasetype()->ty == Tvoid)
            ret_type = Type::tint32;

        tree result_decl = DECL_RESULT(irs->func->toSymbol()->Stree);
#if V1
        tree result_value = irs->convertForAssignment(exp, ret_type);
#else
        tree result_value = irs->convertForAssignment(exp->toElemDtor(irs),
                                                      exp->type, ret_type);
        // %% convert for init -- if we were returning a reference,
        // would want to take the address...
        if (tf->isref)
        {
            result_value = irs->addressOf(result_value);
        }
        else if (exp->type->toBasetype()->ty == Tstruct &&
                 (exp->op == TOKvar || exp->op == TOKdotvar || exp->op == TOKstar || exp->op == TOKthis))
        {   // Maybe call postblit on result_value
            StructDeclaration * sd;
            if ((sd = needsPostblit(exp->type->toBasetype())) != NULL)
            {
                Expressions args;
                FuncDeclaration * fd = sd->postblit;
                if (fd->storage_class & STCdisable)
                {
                    fd->toParent()->error(loc, "is not copyable because it is annotated with @disable");
                }
                tree call_exp = irs->call(sd->postblit, irs->addressOf(result_value), & args);
                irs->addExp(call_exp);
            }
        }
#endif
        tree result_assign = build2(MODIFY_EXPR, TREE_TYPE(result_decl),
                result_decl, result_value);

        irs->doReturn(result_assign); // expand_return(result_assign);
    }
    else
    {   //irs->doExp(exp);
        irs->doReturn(NULL_TREE);
    }
}

void
DefaultStatement::toIR(IRState * irs)
{
    irs->checkSwitchCase(this, 1);
    irs->doCase(NULL_TREE, cblock);
    if (statement)
    {
        statement->toIR(irs);
    }
}

void
CaseStatement::toIR(IRState * irs)
{
    tree case_value;

    if (exp->type->isscalar())
        case_value = exp->toElem(irs);
    else
        case_value = irs->integerConstant(index, Type::tint32);

    irs->checkSwitchCase(this);
    irs->doCase(case_value, cblock);
    if (statement)
    {
        statement->toIR(irs);
    }
}

void
SwitchStatement::toIR(IRState * irs)
{
    tree cond_tree;
    // %% also what about c-semantics doing emit_nop() ?
    irs->doLineNote(loc);
#if V2
    cond_tree = condition->toElemDtor(irs);
#else
    cond_tree = condition->toElem(irs);
#endif

    Type * cond_type = condition->type->toBasetype();
    if (cond_type->ty == Tarray)
    {
        Type * elem_type = cond_type->nextOf()->toBasetype();
        LibCall lib_call;
        switch (elem_type->ty)
        {
            case Tchar:  lib_call = LIBCALL_SWITCH_STRING; break;
            case Twchar: lib_call = LIBCALL_SWITCH_USTRING; break;
            case Tdchar: lib_call = LIBCALL_SWITCH_DSTRING; break;
            default:
                ::error("switch statement value must be an array of some character type, not %s",
                        elem_type->toChars());
                gcc_unreachable();
        }

        // Apparently the backend is supposed to sort and set the indexes
        // on the case array
        // have to change them to be useable
        cases->sort(); // %%!!

        Symbol * s = static_sym();
        dt_t **  pdt = & s->Sdt;
        s->Sseg = CDATA;
        for (size_t case_i = 0; case_i < cases->dim; case_i++)
        {
            CaseStatement * case_stmt = cases->tdata()[case_i];
            pdt = case_stmt->exp->toDt(pdt);
            case_stmt->index = case_i;
        }
        outdata(s);
        tree p_table = irs->addressOf(s->Stree);

        tree args[2] = {
            irs->darrayVal(cond_type->arrayOf()->toCtype(),
                            cases->dim, p_table),
            cond_tree
        };

        cond_tree = irs->libCall(lib_call, 2, args);
    }
    else if (! cond_type->isscalar())
    {
        ::error("cannot handle switch condition of type %s", cond_type->toChars());
        gcc_unreachable();
    }
    if (cases)
    {   // Build LABEL_DECLs now so they can be refered to by goto case
        for (size_t i = 0; i < cases->dim; i++)
        {
            CaseStatement * case_stmt = cases->tdata()[i];
            case_stmt->cblock = irs->label(case_stmt->loc); //make_case_label(case_stmt->loc);
        }
        if (sdefault)
        {
            sdefault->cblock = irs->label(sdefault->loc); //make_case_label(sdefault->loc);
        }
    }
    cond_tree = fold(cond_tree);
#if V2
    if (hasVars)
    {   // Write cases as a series of if-then-else blocks.
        for (size_t i = 0; i < cases->dim; i++)
        {
            CaseStatement * case_stmt = cases->tdata()[i];
            tree case_cond = build2(EQ_EXPR, cond_type->toCtype(), cond_tree,
                                    case_stmt->exp->toElemDtor(irs));
            irs->startCond(this, case_cond);
            irs->doJump(NULL, case_stmt->cblock);
            irs->endCond();
        }
        if (sdefault)
        {
            irs->doJump(NULL, sdefault->cblock);
        }
    }
    irs->startCase(this, cond_tree, hasVars);
#else
    irs->startCase(this, cond_tree);
#endif
    if (body)
    {
        body->toIR(irs);
    }
    irs->endCase(cond_tree);
}


void
Statement::toIR(IRState*)
{
    ::error("Statement::toIR: don't know what to do (%s)", toChars());
    gcc_unreachable();
}

void
IfStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    irs->startScope();
#if V1
    if (match)
    {
        irs->emitLocalVar(match);
    }
#endif
    irs->startCond(this, condition);
    if (ifbody)
    {
        ifbody->toIR(irs);
    }
    if (elsebody)
    {
        irs->startElse();
        elsebody->toIR(irs);
    }
    irs->endCond();
    irs->endScope();
}

void
ForeachStatement::toIR(IRState *)
{
    // Frontend rewrites this to ForStatement
    ::error("ForeachStatement::toIR: we shouldn't emit this (%s)", toChars());
    gcc_unreachable();
#if 0
    // %% better?: set iter to start - 1 and use result of increment for condition?

    // side effects?

    Type * agg_type = aggr->type->toBasetype();
    Type * elem_type = agg_type->nextOf()->toBasetype();
    tree iter_decl;
    tree bound_expr;
    tree iter_init_expr;
    tree aggr_expr = irs->maybeMakeTemp(aggr->toElem(irs));

    gcc_assert(value);

    irs->startScope();
    irs->startBindings(); /* Variables created by the function will probably
                             end up in a contour created by emitLocalVar.  This
                             startBindings call is just to be safe */
    irs->doLineNote(loc);

    Loc default_loc;
    if (loc.filename)
        default_loc = loc;
    else {
        fprintf(stderr, "EXPER: I need this\n");
        default_loc = Loc(g.mod, 1); // %% fix
    }

    if (! value->loc.filename)
        g.ofile->setDeclLoc(value->toSymbol()->Stree, default_loc);

    irs->emitLocalVar(value, true);

    if (key) {
        if (! key->loc.filename)
            g.ofile->setDeclLoc(key->toSymbol()->Stree, default_loc);
        if (! key->init)
            DECL_INITIAL(key->toSymbol()->Stree) = op == TOKforeach ?
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

    if (agg_type->ty == Tsarray) {
        bound_expr = ((TypeSArray *) agg_type)->dim->toElem(irs);
        iter_init_expr = irs->addressOf(aggr_expr);
        // Type needs to be pointer-to-element to get pointerIntSum
        // to work
        iter_init_expr = irs->nop(iter_init_expr,
            agg_type->nextOf()->pointerTo()->toCtype());
    } else {
        bound_expr = irs->darrayLenRef(aggr_expr);
        iter_init_expr = irs->darrayPtrRef(aggr_expr);
    }
    iter_init_expr = save_expr(iter_init_expr);
    bound_expr = irs->pointerIntSum(iter_init_expr, bound_expr);
    // aggr. isn't supposed to be modified, so...
    bound_expr = save_expr(bound_expr);

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

    irs->doExp(build2(MODIFY_EXPR, void_type_node, iter_decl, iter_init_expr));

    irs->startLoop(this);
    irs->exitIfFalse(condition);
    if (op == TOKforeach_reverse)
        irs->doExp(incr_expr);
    if (! iter_is_value)
        irs->doExp(build2(MODIFY_EXPR, void_type_node, irs->var(value),
                        irs->indirect(iter_decl)));
    if (body)
        body->toIR(irs);
    irs->continueHere();

    if (op == TOKforeach)
        irs->doExp(incr_expr);

    irs->endLoop();

    irs->endBindings(); // not really needed
    irs->endScope();
#endif
}

#if V2
void
ForeachRangeStatement::toIR(IRState *)
{
    // Frontend rewrites this to ForStatement
    ::error("ForeachRangeStatement::toIR: we shouldn't emit this (%s)", toChars());
    gcc_unreachable();
#if 0
    bool fwd = op == TOKforeach;
    Type * key_type = key->type->toBasetype();

    irs->startScope();
    irs->startBindings(); /* Variables created by the function will probably
                             end up in a contour created by emitLocalVar.  This
                             startBindings call is just to be safe */
    irs->doLineNote(loc);

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
    if (POINTER_TYPE_P (TREE_TYPE (key_decl)))
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
    irs->doExp(irs->vmodify(lwr_decl, lwr->toElem(irs)));
    irs->doExp(irs->vmodify(upr_decl, upr->toElem(irs)));

    condition = build2(fwd ? LT_EXPR : GT_EXPR, boolean_type_node,
        key_decl, fwd ? upr_decl : lwr_decl);

    irs->doExp(irs->vmodify(key_decl, fwd ? lwr_decl : upr_decl));

    irs->startLoop(this);
    if (! fwd)
        irs->continueHere();
    irs->exitIfFalse(condition);
    if (! fwd)
        irs->doExp(iter_expr);
    if (body)
        body->toIR(irs);
    if (fwd) {
        irs->continueHere();
        irs->doExp(iter_expr);
    }
    irs->endLoop();

    irs->endBindings(); // not really needed
    irs->endScope();
#endif
}
#endif

void
ForStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    if (init)
    {   // %% scope
        init->toIR(irs);
    }
    irs->startLoop(this);
    if (condition)
    {
        irs->doLineNote(condition->loc);
        irs->exitIfFalse(condition);
    }
    if (body)
    {
        body->toIR(irs);
    }
    irs->continueHere();
    if (increment)
    {   // force side effects?
        irs->doLineNote(increment->loc);
#if V2
        irs->doExp(increment->toElemDtor(irs));
#else
        irs->doExp(increment->toElem(irs));
#endif
    }
    irs->endLoop();
}

void
DoStatement::toIR(IRState * irs)
{
    irs->doLineNote(loc);
    irs->startLoop(this);
    if (body)
    {
        body->toIR(irs);
    }
    irs->continueHere();
    irs->doLineNote(condition->loc);
    irs->exitIfFalse(condition);
    irs->endLoop();
}

void
WhileStatement::toIR(IRState *)
{
    // Frontend rewrites this to ForStatement
    ::error("WhileStatement::toIR: we shouldn't emit this (%s)", toChars());
    gcc_unreachable();
#if 0
    irs->doLineNote(loc); // store for next statement...
    irs->startLoop(this);
    irs->exitIfFalse(condition, 1); // 1 == is topcond .. good as deprecated..
    if (body)
        body->toIR(irs);
    irs->continueHere();
    irs->endLoop();
#endif
}

void
ScopeStatement::toIR(IRState* irs)
{
    if (statement)
    {
        irs->startScope();
        statement->toIR(irs);
        irs->endScope();
    }
}

void
CompoundStatement::toIR(IRState* irs)
{
    if (! statements)
        return;

    for (size_t i = 0; i < statements->dim; i++)
    {
        Statement * statement = statements->tdata()[i];
        if (statement)
        {
            statement->toIR(irs);
        }
    }
}

void
UnrolledLoopStatement::toIR(IRState* irs)
{
    if (! statements)
        return;

    irs->startLoop(this);
    irs->continueHere();
    for (size_t i = 0; i < statements->dim; i++)
    {
        Statement * statement = statements->tdata()[i];
        if (statement)
        {
            irs->setContinueLabel(irs->label(loc));
            statement->toIR(irs);
            irs->continueHere();
        }
    }
    irs->exitLoop(NULL);
    irs->endLoop();
}

void
ExpStatement::toIR(IRState * irs)
{
    if (exp)
    {
        gen.doLineNote(loc);
#if V2
        tree exp_tree = exp->toElemDtor(irs);
#else
        tree exp_tree = exp->toElem(irs);
#endif
        irs->doExp(exp_tree);
    }
}

#if V2
void
DtorExpStatement::toIR(IRState * irs)
{
    ExpStatement::toIR(irs);    // %% ??
}

void
PragmaStatement::toIR(IRState *)
{
    // nothing
}

void
ImportStatement::toIR(IRState *)
{
    // nothing
}
#endif

void
EnumDeclaration::toDebug()
{
    TypeEnum * tc = (TypeEnum *)type;
    if (!tc->sym->defaultval || type->isZeroInit())
        return;

    tree ctype = type->toCtype();

    // %% For D2 - ctype is not necessarily enum, which doesn't
    // sit well with rotc.  Can call this on structs though.
    if (AGGREGATE_TYPE_P(ctype) || TREE_CODE(ctype) == ENUMERAL_TYPE)
        rest_of_type_compilation(ctype, /*toplevel*/1);
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


// Backend init

// Array of d type/decl nodes.
tree d_global_trees[DTI_MAX];

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
#if D_GCC_VER >= 46
    build_common_tree_nodes (flag_signed_char);
#else
    build_common_tree_nodes (flag_signed_char, false);
#endif

    // This is also required (or the manual equivalent) or crashes
    // will occur later
    char_type_node = d_type_for_size(CHAR_TYPE_SIZE, 1);
    size_type_node = d_type_for_mode(ptr_mode, 1);

    // If this is called after the next statements, you'll get an ICE.
    set_sizetype(size_type_node);

    // need this for void.. %% but this crashes... probably need to impl
    // some things in dc-lang.cc
    build_common_tree_nodes_2 (0 /* %% support the option */);

    // Specific to D (but so far all taken from C)
    d_void_zero_node = make_node(INTEGER_CST);
    TREE_TYPE(d_void_zero_node) = void_type_node;

    // %%TODO: we are relying on default boolean_type_node being 8bit / same as Tbit

    d_null_pointer = convert(ptr_type_node, integer_zero_node);

    // %% D variant types of Ctypes.
    d_boolean_type_node = make_unsigned_type(1);
    TREE_SET_CODE(d_boolean_type_node, BOOLEAN_TYPE);

    d_char_type_node = build_variant_type_copy(unsigned_intQI_type_node);
    d_wchar_type_node = build_variant_type_copy(unsigned_intHI_type_node);
    d_dchar_type_node = build_variant_type_copy(unsigned_intSI_type_node);
    d_ifloat_type_node = build_variant_type_copy(float_type_node);
    d_idouble_type_node = build_variant_type_copy(double_type_node);
    d_ireal_type_node = build_variant_type_copy(long_double_type_node);

    TYPE_NAME(integer_type_node) = d_build_decl(TYPE_DECL, get_identifier("int"), integer_type_node);
    TYPE_NAME(char_type_node) = d_build_decl(TYPE_DECL, get_identifier("char"), char_type_node);

    REALSIZE = int_size_in_bytes(long_double_type_node);
    REALPAD = 0;
    PTRSIZE = int_size_in_bytes(ptr_type_node);

    switch (PTRSIZE)
    {
        case 4:
            gcc_assert(POINTER_SIZE == 32);
            Tptrdiff_t = Tint32;
            break;
        case 8:
            gcc_assert(POINTER_SIZE == 64);
            Tptrdiff_t = Tint64;
            break;
        default:
            gcc_unreachable();
    }

    // %% May get changed later anyway...
    CLASSINFO_SIZE_64 = 19 * PTRSIZE;
    CLASSINFO_SIZE = 19 * PTRSIZE;

    d_init_builtins();

    if (flag_exceptions)
        d_init_exceptions();

    /* copied and modified from cp/decl.c; only way for vtable to work in gdb... */
    // or not, I'm feeling very confused...
    {
        /* Make sure we get a unique function type, so we can give
           its pointer type a name.  (This wins for gdb.) */
        tree vfunc_type = build_function_type(integer_type_node, NULL_TREE);
        tree vtable_entry_type = build_pointer_type(vfunc_type);

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
