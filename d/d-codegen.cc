/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Iain Buclaw, Michael Parrott, (C) 2010, 2011

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

#include "template.h"
#include "init.h"
#include "symbol.h"
#include "dt.h"
#include "id.h"

GlobalValues g;
IRState gen;
TemplateEmission IRState::emitTemplates;
bool IRState::splitDynArrayVarArgs;
bool IRState::useInlineAsm;
bool IRState::useBuiltins;
bool IRState::warnSignCompare = false;
bool IRState::originalOmitFramePointer;

#if V2
VarDeclarations * IRState::varsInScope;
#endif

bool
d_gcc_force_templates()
{
    return IRState::emitTemplates == TEprivate ||
        IRState::emitTemplates == TEall;
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

void
IRState::emitLocalVar(VarDeclaration * v, bool no_init)
{
    if (v->isDataseg() || v->isMember())
        return;

    Symbol * sym = v->toSymbol();
    tree var_decl = sym->Stree;

    gcc_assert(! TREE_STATIC(var_decl));
    if (TREE_CODE(var_decl) == CONST_DECL)
        return;

    DECL_CONTEXT(var_decl) = getLocalContext();

    // Compiler generated symbols
    if (v == func->vresult || v == func->v_argptr || v == func->v_arguments_var)
        DECL_ARTIFICIAL(var_decl) = 1;

    tree var_exp;
    if (sym->SframeField)
    {   // Fixes debugging local variables.
        SET_DECL_VALUE_EXPR(var_decl, var(v));
        DECL_HAS_VALUE_EXPR_P(var_decl) = 1;
    }
    var_exp = var_decl;
    pushdecl(var_decl);

    tree init_exp = NULL_TREE; // complete initializer expression (include MODIFY_EXPR, e.g.)
    tree init_val = NULL_TREE;

    if (! no_init && ! DECL_INITIAL(var_decl) && v->init)
    {
        if (! v->init->isVoidInitializer())
        {
            ExpInitializer * exp_init = v->init->isExpInitializer();
            Expression * ie = exp_init->toExpression();
            if (! (init_val = assignValue(ie, v)))
                init_exp = ie->toElem(this);
        }
        else
        {
            no_init = true;
        }
    }

    if (! no_init)
    {
        g.ofile->doLineNote(v->loc);

        if (! init_val)
        {
            init_val = DECL_INITIAL(var_decl);
            DECL_INITIAL(var_decl) = NULL_TREE; // %% from expandDecl
        }
        if (! init_exp && init_val)
            init_exp = vmodify(var_exp, init_val);

        if (init_exp)
            addExp(init_exp);
        else if (! init_val && v->size(v->loc)) // Zero-length arrays do not have an initializer
            d_warning(OPT_Wuninitialized, "uninitialized variable '%s'", v->ident ? v->ident->string : "(no name)");
    }
}

tree
IRState::localVar(tree t_type)
{
    tree t_decl = d_build_decl_loc(BUILTINS_LOCATION, VAR_DECL, NULL_TREE, t_type);
    DECL_CONTEXT(t_decl) = getLocalContext();
    DECL_ARTIFICIAL(t_decl) = 1;
    DECL_IGNORED_P(t_decl) = 1;
    pushdecl(t_decl);
    return t_decl;
}

tree
IRState::exprVar(tree t_type)
{
    tree t_decl = d_build_decl_loc(BUILTINS_LOCATION, VAR_DECL, NULL_TREE, t_type);
    DECL_CONTEXT(t_decl) = getLocalContext();
    DECL_ARTIFICIAL(t_decl) = 1;
    DECL_IGNORED_P(t_decl) = 1;
    return t_decl;
}

static bool
needs_expr_var(tree exp)
{
    switch (TREE_CODE(exp))
    {
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
    if (needs_expr_var(exp))
    {
        *out_var = exprVar(TREE_TYPE(exp));
        DECL_INITIAL(*out_var) = exp;
        return *out_var;
    }
    else
    {
        *out_var = NULL_TREE;
        return exp;
    }
}

tree
IRState::declContext(Dsymbol * d_sym)
{
    Dsymbol * orig_sym = d_sym;
    AggregateDeclaration * ad;

    while ((d_sym = d_sym->toParent2()))
    {
        if (d_sym->isFuncDeclaration())
        {   // 3.3.x (others?) dwarf2out chokes without this check... (output_pubnames)
            FuncDeclaration * f = orig_sym->isFuncDeclaration();
            if (f && ! gen.functionNeedsChain(f))
                return NULL_TREE;

            return d_sym->toSymbol()->Stree;
        }
        else if ((ad = d_sym->isAggregateDeclaration()))
        {
            tree ctx = ad->type->toCtype();
            if (ad->isClassDeclaration())
            {   // RECORD_TYPE instead of REFERENCE_TYPE
                ctx = TREE_TYPE(ctx);
            }

            return ctx;
        }
        else if (d_sym->isModule())
        {
            return d_sym->toSymbol()->ScontextDecl;
        }
    }
    return NULL_TREE;
}

void
IRState::expandDecl(tree t_decl)
{
    // nothing, pushdecl will add t_decl to a BIND_EXPR
    if (DECL_INITIAL(t_decl))
    {   // void_type_node%%?
        doExp(build2(MODIFY_EXPR, void_type_node, t_decl, DECL_INITIAL(t_decl)));
        DECL_INITIAL(t_decl) = NULL_TREE;
    }
}

tree
IRState::var(VarDeclaration * v)
{
    bool is_frame_var = v->toSymbol()->SframeField != NULL;

    if (is_frame_var)
    {
        FuncDeclaration * f = v->toParent2()->isFuncDeclaration();

        tree cf = getFrameRef(f);
        tree field = v->toSymbol()->SframeField;
        gcc_assert(field != NULL_TREE);
        return component(indirect(cf), field);
    }
    else
    {   // Static var or auto var that the back end will handle for us
        return v->toSymbol()->Stree;
    }
}


tree
IRState::convertTo(Expression * exp, Type * target_type)
{
    return convertTo(exp->toElem(this), exp->type, target_type);
}

tree
IRState::convertTo(tree exp, Type * exp_type, Type * target_type)
{
    tree result = NULL_TREE;

    gcc_assert(exp_type && target_type);
    Type * ebtype = exp_type->toBasetype();
    Type * tbtype = target_type->toBasetype();

#if V2
    if (ebtype->ty == Taarray)
        exp_type = ((TypeAArray*)ebtype)->getImpl()->type;
    if (tbtype->ty == Taarray)
        target_type = ((TypeAArray*)tbtype)->getImpl()->type;
#endif

    if (typesSame(exp_type, target_type))
        return exp;

    switch (ebtype->ty)
    {
        case Tdelegate:
            if (tbtype->ty == Tdelegate)
            {
                exp = maybeMakeTemp(exp);
                return delegateVal(delegateMethodRef(exp), delegateObjectRef(exp),
                                   target_type);
            }
            else if (tbtype->ty == Tpointer)
            {   // The front-end converts <delegate>.ptr to cast(void*)<delegate>.
                // Maybe should only allow void*?
                exp = delegateObjectRef(exp);
            }
            else
            {
                ::error("can't convert a delegate expression to %s", target_type->toChars());
                return error_mark_node;
            }
            break;
        case Tstruct:
            if (tbtype->ty == Tstruct)
            {
                if (target_type->size() == exp_type->size())
                {   // Allowed to cast to structs with same type size.
                    result = vconvert(exp, target_type->toCtype());
                }
                else
                {
                    ::error("can't convert struct %s to %s", exp_type->toChars(), target_type->toChars());
                    return error_mark_node;
                }
            }
            // else, default conversion, which should produce an error
            break;
        case Tclass:
            if (tbtype->ty == Tclass)
            {
                ClassDeclaration * target_class_decl = ((TypeClass *) tbtype)->sym;
                ClassDeclaration * obj_class_decl = ((TypeClass *) ebtype)->sym;
                bool use_dynamic = false;
                int offset;

                if (target_class_decl->isBaseOf(obj_class_decl, & offset))
                {   // Casting up the inheritance tree: Don't do anything special.
                    // Cast to an implemented interface: Handle at compile time.
                    if (offset == OFFSET_RUNTIME)
                        use_dynamic = true;
                    else if (offset)
                    {
                        tree t = target_type->toCtype();
                        exp = maybeMakeTemp(exp);
                        return build3(COND_EXPR, t,
                                boolOp(NE_EXPR, exp, nullPointer()),
                                nop(pointerOffset(exp, size_int(offset)), t),
                                nop(nullPointer(), t));
                    }
                    else
                    {   // d_convert will make a NOP cast
                        break;
                    }
                }
                else if (target_class_decl == obj_class_decl)
                {   // d_convert will make a NOP cast
                    break;
                }
                else if (! obj_class_decl->isCOMclass())
                    use_dynamic = true;

                if (use_dynamic)
                {   // Otherwise, do dynamic cast
                    tree args[2] = {
                        exp,
                        addressOf(target_class_decl->toSymbol()->Stree)
                    }; // %% (and why not just addressOf(target_class_decl)
                    return libCall(obj_class_decl->isInterfaceDeclaration()
                            ? LIBCALL_INTERFACE_CAST : LIBCALL_DYNAMIC_CAST, 2, args);
                }
                else
                {
                    d_warning(0, "cast to %s will yield null result", target_type->toChars());
                    result = convert(target_type->toCtype(), d_null_pointer);
                    if (TREE_SIDE_EFFECTS(exp))
                    {   // make sure the expression is still evaluated if necessary
                        result = compound(exp, result);
                    }
                    return result;
                }
            }
            else
            {   // nothing; default
            }
            break;
        case Tsarray:
            if (tbtype->ty == Tpointer)
            {
                result = nop(addressOf(exp), target_type->toCtype());
            }
            else if (tbtype->ty == Tarray)
            {
                TypeSArray * a_type = (TypeSArray*) ebtype;

                uinteger_t array_len = a_type->dim->toUInteger();
                d_uns64 sz_a = a_type->next->size();
                d_uns64 sz_b = tbtype->nextOf()->size();

                // conversions to different sizes
                // Assumes tvoid->size() == 1
                // %% TODO: handle misalign like _d_arraycast_xxx ?
                if (sz_a != sz_b)
                    array_len = array_len * sz_a / sz_b;

                tree pointer_value = nop(addressOf(exp),
                        tbtype->nextOf()->pointerTo()->toCtype());

                // Assumes casting to dynamic array of same type or void
                return darrayVal(target_type, array_len, pointer_value);
            }
            else if (tbtype->ty == Tsarray)
            {   // DMD apparently allows casting a static array to any static array type
                return vconvert(exp, target_type->toCtype());
            }
            else if (tbtype->ty == Tstruct)
            {   // And allows casting a static array to any struct type too.
                // %% type sizes should have already been checked by the frontend.
                gcc_assert(target_type->size() == exp_type->size());
                result = vconvert(exp, target_type->toCtype());
            }
            else
            {
                ::error("cannot cast expression of type %s to type %s",
                        exp_type->toChars(), target_type->toChars());
                return error_mark_node;
            }
            break;
        case Tarray:
            if (tbtype->ty == Tpointer)
            {
                return convert(target_type->toCtype(), darrayPtrRef(exp));
            }
            else if (tbtype->ty == Tarray)
            {   // assume tvoid->size() == 1
                Type * src_elem_type = ebtype->nextOf()->toBasetype();
                Type * dst_elem_type = tbtype->nextOf()->toBasetype();
                d_uns64 sz_src = src_elem_type->size();
                d_uns64 sz_dst = dst_elem_type->size();

                if (src_elem_type->ty == Tvoid || sz_src == sz_dst)
                {   // Convert from void[] or elements are the same size -- don't change length
                    return vconvert(exp, target_type->toCtype());
                }
                else
                {
                    unsigned mult = 1;
#if V1
                    if (dst_elem_type->isbit())
                        mult = 8;
#endif
                    tree args[3] = {
                        // assumes Type::tbit->size() == 1
                        integerConstant(sz_dst, Type::tsize_t),
                        integerConstant(sz_src * mult, Type::tsize_t),
                        exp
                    };
                    return libCall(LIBCALL_ARRAYCAST, 3, args, target_type->toCtype());
                }
            }
#if V2
            else if (tbtype->ty == Tsarray)
            {   // %% Strings are treated as dynamic arrays D2.
                if (ebtype->isString() && tbtype->isString())
                    return indirect(darrayPtrRef(exp), target_type->toCtype());
            }
#endif
            else
            {
                ::error("cannot cast expression of type %s to %s",
                        exp_type->toChars(), target_type->toChars());
                return error_mark_node;
            }
            break;
        case Taarray:
            if (tbtype->ty == Taarray)
                return vconvert(exp, target_type->toCtype());
            // else, default conversion, which should product an error
            break;
        case Tpointer:
            /* For some reason, convert_to_integer converts pointers
               to a signed type. */
            if (tbtype->isintegral())
                exp = d_convert_basic(d_type_for_size(POINTER_SIZE, 1), exp);
            // Can convert void pointers to associative arrays too...
            else if (tbtype->ty == Taarray && ebtype == Type::tvoidptr)
                return vconvert(exp, target_type->toCtype());
            break;
        default:
        {
            if ((ebtype->isreal() && tbtype->isimaginary())
                || (ebtype->isimaginary() && tbtype->isreal()))
            {   // warn? handle in front end?
                result = build_real_from_int_cst(target_type->toCtype(), integer_zero_node);
                if (TREE_SIDE_EFFECTS(exp))
                    result = compound(exp, result);
                return result;
            }
            else if (ebtype->iscomplex())
            {
                Type * part_type;
                // creal.re, .im implemented by cast to real or ireal
                // Assumes target type is the same size as the original's components size
                if (tbtype->isreal())
                {   // maybe install lang_specific...
                    switch (ebtype->ty)
                    {
                        case Tcomplex32: part_type = Type::tfloat32; break;
                        case Tcomplex64: part_type = Type::tfloat64; break;
                        case Tcomplex80: part_type = Type::tfloat80; break;
                        default:
                            gcc_unreachable();
                    }
                    result = realPart(exp);
                }
                else if (tbtype->isimaginary())
                {
                    switch (ebtype->ty)
                    {
                        case Tcomplex32: part_type = Type::timaginary32; break;
                        case Tcomplex64: part_type = Type::timaginary64; break;
                        case Tcomplex80: part_type = Type::timaginary80; break;
                        default:
                            gcc_unreachable();
                    }
                    result = imagPart(exp);
                }
                else
                {   // default conversion
                    break;
                }
                result = convertTo(result, part_type, target_type);
            }
            else if (tbtype->iscomplex())
            {
                tree c1 = convert(TREE_TYPE(target_type->toCtype()), exp);
                tree c2 = build_real_from_int_cst(TREE_TYPE(target_type->toCtype()), integer_zero_node);

                if (ebtype->isreal())
                {   // nothing
                }
                else if (ebtype->isimaginary())
                {
                    tree swap = c1;
                    c1 = c2;
                    c2 = swap;
                } else
                {   // default conversion
                    break;
                }
                result = build2(COMPLEX_EXPR, target_type->toCtype(), c1, c2);
            }
            else
            {
                gcc_assert(TREE_CODE(exp) != STRING_CST);
                // default conversion
            }
        }
    }

    if (! result)
        result = d_convert_basic(target_type->toCtype(), exp);
#if ENABLE_CHECKING
    if (isErrorMark(result))
        error("type: %s, target: %s", exp_type->toChars(), target_type->toChars());
#endif
    return result;
}

tree
IRState::convertForArgument(Expression * exp, Parameter * arg)
{
    if (isArgumentReferenceType(arg))
    {
        tree exp_tree = this->toElemLvalue(exp);
        // front-end already sometimes automatically takes the address
        // TODO: Make this safer?  Can this be confused by a non-zero SymOff?
        if (exp->op != TOKaddress && exp->op != TOKsymoff && exp->op != TOKadd)
            return addressOf(exp_tree);
        else
            return exp_tree;
    }
    else
    {   // Lazy arguments: exp should already be a delegate
        tree exp_tree = exp->toElem(this);
        return exp_tree;
    }
}

// Apply semantics of assignment to a values of type <target_type> to <exp>
// (e.g., pointer = array -> pointer = & array[0])

// Expects base type to be passed
static Type *
final_sa_elem_type(Type * type)
{
    while (type->ty == Tsarray)
        type = type->nextOf()->toBasetype();

    return type;
}

tree
IRState::convertForAssignment(Expression * exp, Type * target_type)
{
    Type * exp_base_type = exp->type->toBasetype();
    Type * target_base_type = target_type->toBasetype();
    tree exp_tree = NULL_TREE;

    // Assuming this only has to handle converting a non Tsarray type to
    // arbitrarily dimensioned Tsarrays.
    if (target_base_type->ty == Tsarray &&
        typesCompatible(final_sa_elem_type(target_base_type), exp_base_type))
    {   // %% what about implicit converions...?
        TypeSArray * sa_type = (TypeSArray *) target_base_type;
        uinteger_t count = sa_type->dim->toUInteger();

        tree ctor = build_constructor(target_type->toCtype(), 0);
        if (count)
        {
            CtorEltMaker ce;
            ce.cons(build2(RANGE_EXPR, Type::tsize_t->toCtype(),
                    integer_zero_node, integerConstant(count - 1)),
                g.ofile->stripVarDecl(convertForAssignment(exp, sa_type->next)));
            CONSTRUCTOR_ELTS(ctor) = ce.head;
        }
        TREE_READONLY(ctor) = 1;
        TREE_CONSTANT(ctor) = 1;
        return ctor;
    }
    else if (! target_type->isscalar() && exp_base_type->isintegral())
    {   // D Front end uses IntegerExp(0) to mean zero-init a structure
        // This could go in convert for assignment, but we only see this for
        // internal init code -- this also includes default init for _d_newarrayi...
        if (exp->toInteger() == 0)
        {
            CtorEltMaker ce;
            tree empty = build_constructor(target_type->toCtype(), ce.head); // %% will this zero init?
            TREE_CONSTANT(empty) = 1;
            TREE_STATIC(empty) = 1;
            return empty;
            // %%TODO: Use a code (lang_specific in decl or flag) to memset instead?
        }
        else
        {
            gcc_unreachable();
        }
    }

    exp_tree = exp->toElem(this);
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
IRState::convertForCondition(tree exp_tree, Type * exp_type)
{
    tree result = NULL_TREE;
    tree a, b, tmp;

    switch (exp_type->toBasetype()->ty)
    {
        case Taarray:
            // Shouldn't this be...
            //  result = libCall(LIBCALL_AALEN, 1, & exp_tree);
            result = component(exp_tree, TYPE_FIELDS(TREE_TYPE(exp_tree)));
            break;

        case Tarray:
            // DMD checks (length || ptr) (i.e ary !is null)
            tmp = maybeMakeTemp(exp_tree);
            a = delegateObjectRef(tmp);
            b = delegateMethodRef(tmp);
            if (TYPE_MODE(TREE_TYPE(a)) == TYPE_MODE(TREE_TYPE(b)))
            {
                result = build2(BIT_IOR_EXPR, TREE_TYPE(a), a,
                        convert(TREE_TYPE(a), b));
            }
            else
            {
                a = d_truthvalue_conversion(a);
                b = d_truthvalue_conversion(b);
                // probably not worth TRUTH_OROR ...
                result = build2(TRUTH_OR_EXPR, TREE_TYPE(a), a, b);
            }
            break;

        case Tdelegate:
            // DMD checks (function || object), but what good
            // is if if there is a null function pointer?
            if (D_IS_METHOD_CALL_EXPR(exp_tree))
            {
                extractMethodCallExpr(exp_tree, a, b);
            }
            else
            {
                tmp = maybeMakeTemp(exp_tree);
                a = delegateObjectRef(tmp);
                b = delegateMethodRef(tmp);
            }
            // not worth using  or TRUTH_ORIF...
            // %%TODO: Is this okay for all targets?
            result = build2(BIT_IOR_EXPR, TREE_TYPE(a), a, b);
            break;

        default:
            result = exp_tree;
            break;
    }
    // see expr.c:do_jump for the types of trees that can be passed to expand_start_cond
    //TREE_USED(<result>) = 1; // %% maybe not.. expr optimized away because constant expression?
    return d_truthvalue_conversion(result);
}

/* Convert EXP to a dynamic array.  EXP must be a static array or
   dynamic array. */
tree
IRState::toDArray(Expression * exp)
{
    TY ty = exp->type->toBasetype()->ty;
    tree val;
    if (ty == Tsarray)
        val = convertTo(exp, exp->type->toBasetype()->nextOf()->arrayOf());
    else if (ty == Tarray)
        val = exp->toElem(this);
    else
    {
        gcc_assert(ty == Tsarray || ty == Tarray);
        return NULL_TREE;
    }
    return val;
}


tree
IRState::pointerIntSum(tree ptr_node, tree idx_exp)
{
    tree result_type_node = TREE_TYPE(ptr_node);
    tree elem_type_node = TREE_TYPE(result_type_node);
    tree intop = idx_exp;
    tree size_exp;

    tree prod_result_type;
#if D_GCC_VER >= 43
    prod_result_type = sizetype;
#else
    prod_result_type = result_type_node;
#endif

    // %% TODO: real-not-long-double issues...

    size_exp = size_in_bytes(elem_type_node); // array element size

    if (integer_zerop(size_exp))
    {   // Test for void case...
        if (TYPE_MODE(elem_type_node) == TYPE_MODE(void_type_node))
            intop = fold_convert(prod_result_type, intop);
        else
        {   // FIXME: should catch this earlier.
            error("invalid use of incomplete type %qD", TYPE_NAME(elem_type_node));
            result_type_node = error_mark_node;
        }
    }
    else if (integer_onep(size_exp))
    {   // ...or byte case -- No need to multiply.
        intop = fold_convert(prod_result_type, intop);
    }
    else
    {
        if (TYPE_PRECISION (TREE_TYPE (intop)) != TYPE_PRECISION (sizetype)
            || TYPE_UNSIGNED (TREE_TYPE (intop)) != TYPE_UNSIGNED (sizetype))
        {
            intop = convert (d_type_for_size (TYPE_PRECISION (sizetype),
                                 TYPE_UNSIGNED (sizetype)), intop);
        }
        intop = fold_convert (prod_result_type,
                              fold_build2 (MULT_EXPR, TREE_TYPE(size_exp), // the type here may be wrong %%
                                           intop, convert (TREE_TYPE (intop), size_exp)));
    }

    if (isErrorMark(result_type_node))
        return result_type_node; // backend will ICE otherwise

    if (integer_zerop(intop))
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
    {   /* nothing */
    }
    else if (op == MINUS_EXPR)
    {   // %% sign extension...
        idx = fold_build1(NEGATE_EXPR, sizetype, idx);
    }
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
    tree ofs = fold_convert(sizetype, byte_offset);
    tree t;
#if D_GCC_VER >= 43
    t = fold_build2(POINTER_PLUS_EXPR, TREE_TYPE(ptr_node), ptr_node, ofs);
#else
    t = fold_build2(PLUS_EXPR, TREE_TYPE(ptr_node), ptr_node, ofs);
#endif
    return t;
}


// Doesn't do some of the optimizations in ::makeArrayElemRef
tree
IRState::checkedIndex(Loc loc, tree index, tree upper_bound, bool inclusive)
{
    if (arrayBoundsCheck())
    {
        return build3(COND_EXPR, TREE_TYPE(index),
                boundsCond(index, upper_bound, inclusive),
                index,
                assertCall(loc, LIBCALL_ARRAY_BOUNDS));
    }
    else
    {
        return index;
    }
}


// index must be wrapped in a SAVE_EXPR to prevent multiple evaluation...
tree
IRState::boundsCond(tree index, tree upper_bound, bool inclusive)
{
    tree bound_check;

    bound_check = build2(inclusive ? LE_EXPR : LT_EXPR, boolean_type_node,
            convert(d_unsigned_type(TREE_TYPE(index)), index),
            upper_bound);

    if (! TYPE_UNSIGNED(TREE_TYPE(index)))
    {
        bound_check = build2(TRUTH_ANDIF_EXPR, boolean_type_node, bound_check,
                // %% conversions
                build2(GE_EXPR, boolean_type_node, index, integer_zero_node));
    }
    return bound_check;
}

// Return != 0 if do array bounds checking
int
IRState::arrayBoundsCheck()
{
    int result = global.params.useArrayBounds;
#if V2
    if (result == 1)
    {
        // For D2 safe functions only
        result = 0;
        if (func && func->type->ty == Tfunction)
        {
            TypeFunction * tf = (TypeFunction *)func->type;
            if (tf->trust == TRUSTsafe)
                result = 1;
        }
    }
#endif
    return result;
}

tree
IRState::assertCall(Loc loc, LibCall libcall)
{
    tree args[2] = {
        darrayString(loc.filename ? loc.filename : ""),
        integerConstant(loc.linnum, Type::tuns32)
    };
#if V2
    if (libcall == LIBCALL_ASSERT && func->isUnitTestDeclaration())
        libcall = LIBCALL_UNITTEST;
#endif
    return libCall(libcall, 2, args);
}

tree
IRState::assertCall(Loc loc, Expression * msg)
{
    tree args[3] = {
        msg->toElem(this),
        darrayString(loc.filename ? loc.filename : ""),
        integerConstant(loc.linnum, Type::tuns32)
    };
#if V2
    LibCall libcall = func->isUnitTestDeclaration() ?
        LIBCALL_UNITTEST_MSG : LIBCALL_ASSERT_MSG;
#else
    LibCall libcall = LIBCALL_ASSERT_MSG;
#endif
    return libCall(libcall, 3, args);
}

tree
IRState::floatConstant(const real_t & value, Type * target_type)
{
    real_t new_value;
    TypeBasic * tb = target_type->isTypeBasic();

    gcc_assert(tb != NULL);

    tree type_node = tb->toCtype();
    real_convert(& new_value.rv(), TYPE_MODE(type_node), & value.rv());

    if (new_value > value)
    {   // value grew as a result of the conversion. %% precision bug ??
        // For now just revert back to original.
        new_value = value;
    }

    return build_real(type_node, new_value.rv());
}

dinteger_t
IRState::hwi2toli(HOST_WIDE_INT low, HOST_WIDE_INT high)
{
    dinteger_t result;

    if (high == 0 || sizeof(HOST_WIDE_INT) >= sizeof(dinteger_t))
        result = low;
    else
    {
        gcc_assert(sizeof(HOST_WIDE_INT) * 2 == sizeof(dinteger_t));
        result = (unsigned HOST_WIDE_INT) low;
        result += ((uinteger_t) (unsigned HOST_WIDE_INT) high) << HOST_BITS_PER_WIDE_INT;
    }
    return result;
}


tree
IRState::binding(tree var_chain, tree body)
{
    // BIND_EXPR/DECL_INITIAL not supported in 4.0?
    gcc_assert(TREE_CHAIN(var_chain) == NULL_TREE); // TODO: only handles one var

#if D_GCC_VER >= 43
    // %%EXPER -- if a BIND_EXPR is an a SAVE_EXPR, gimplify dies
    // in mostly_copy_tree_r.  prevent the latter from seeing
    // our shameful BIND_EXPR by wrapping it in a TARGET_EXPR
    if (DECL_INITIAL(var_chain))
    {
        tree ini = build2(MODIFY_EXPR, void_type_node, var_chain, DECL_INITIAL(var_chain));
        DECL_INITIAL(var_chain) = NULL_TREE;
        body = compound(ini, body);
    }
    return save_expr(build3(BIND_EXPR, TREE_TYPE(body), var_chain, body, NULL_TREE));
#else
    if (DECL_INITIAL(var_chain))
    {
        tree ini = build2(MODIFY_EXPR, void_type_node, var_chain, DECL_INITIAL(var_chain));
        DECL_INITIAL(var_chain) = NULL_TREE;
        body = compound(ini, body);
    }

    return build3(BIND_EXPR, TREE_TYPE(body), var_chain, body, NULL_TREE);
#endif
}

tree
IRState::libCall(LibCall lib_call, unsigned n_args, tree *args, tree force_result_type)
{
    FuncDeclaration * lib_decl = getLibCallDecl(lib_call);
    Type * type = lib_decl->type->nextOf();
    tree callee = functionPointer(lib_decl);
    tree arg_list = NULL_TREE;

    for (int i = n_args - 1; i >= 0; i--)
        arg_list = tree_cons(NULL_TREE, args[i], arg_list);

    tree result = buildCall(type->toCtype(), callee, arg_list);

    // for force_result_type, assumes caller knows what it is doing %%
    if (force_result_type != NULL_TREE)
        return vconvert(result, force_result_type);

    return result;
}

// Assumes T is already ->toBasetype()
TypeFunction *
IRState::getFuncType(Type * t)
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
IRState::call(Expression * expr, Expressions * arguments)
{
    // Calls to delegates can sometimes look like this:
    if (expr->op == TOKcomma)
    {
        CommaExp * ce = (CommaExp *) expr;
        expr = ce->e2;

        VarExp * ve;
        gcc_assert(ce->e2->op == TOKvar);
        ve = (VarExp *) ce->e2;
        gcc_assert(ve->var->isFuncDeclaration() && ! ve->var->needThis());
    }

    Type* t = expr->type->toBasetype();
    TypeFunction* tf = NULL;
    tree callee = expr->toElem(this);
    tree object = NULL_TREE;

    if (D_IS_METHOD_CALL_EXPR(callee))
    {
        /* This could be a delegate expression (TY == Tdelegate), but not
           actually a delegate variable. */
        // %% Is this ever not a DotVarExp ?
        if (expr->op == TOKdotvar)
        {   /* This gets the true function type, the latter way can sometimes
               be incorrect. Example: ref functions in D2. */
            tf = getFuncType(((DotVarExp *)expr)->var->type);
        }
        else
            tf = getFuncType(t);

        extractMethodCallExpr(callee, callee, object);
    }
    else if (t->ty == Tdelegate)
    {
        tf = (TypeFunction*) ((TypeDelegate *) t)->next;
        callee = maybeMakeTemp(callee);
        object = delegateObjectRef(callee);
        callee = delegateMethodRef(callee);
    }
    else if (expr->op == TOKvar)
    {
        FuncDeclaration * fd = ((VarExp *) expr)->var->isFuncDeclaration();
        gcc_assert(fd);
        tf = (TypeFunction *) fd->type;
        if (fd->isNested())
        {
            object = getFrameForFunction(fd);
        }
        else if (fd->needThis())
        {
            expr->error("need 'this' to access member %s", fd->toChars());
            object = d_null_pointer; // continue processing...
        }
    }
    else
    {
        tf = getFuncType(t);
    }
    return call(tf, callee, object, arguments);
}

tree
IRState::call(FuncDeclaration * func_decl, Expressions * args)
{
    gcc_assert(! func_decl->isNested()); // Otherwise need to copy code from above
    return call((TypeFunction *) func_decl->type, func_decl->toSymbol()->Stree, NULL_TREE, args);
}

tree
IRState::call(FuncDeclaration * func_decl, tree object, Expressions * args)
{
    return call((TypeFunction *)func_decl->type, functionPointer(func_decl),
            object, args);
}

tree
IRState::call(TypeFunction *func_type, tree callable, tree object, Expressions * arguments)
{
    // Using TREE_TYPE(callable) instead of func_type->toCtype can save a build_method_type
    tree func_type_node = TREE_TYPE(callable);
    tree actual_callee  = callable;

    ListMaker actual_arg_list;

    if (POINTER_TYPE_P(func_type_node))
        func_type_node = TREE_TYPE(func_type_node);
    else
        actual_callee = addressOf(callable);

    gcc_assert(isFuncType(func_type_node));
    gcc_assert(func_type != NULL);
    gcc_assert(func_type->ty == Tfunction);

    bool is_d_vararg = func_type->varargs == 1 && func_type->linkage == LINKd;

    // Account for the hidden object/frame pointer argument

    if (TREE_CODE(func_type_node) == FUNCTION_TYPE)
    {
        if (object != NULL_TREE)
        {   // Happens when a delegate value is called
            tree method_type = build_method_type(TREE_TYPE(object), func_type_node);
            TYPE_ATTRIBUTES(method_type) = TYPE_ATTRIBUTES(func_type_node);
            func_type_node = method_type;
        }
    }
    else
    {   /* METHOD_TYPE */
        if (! object)
        {   // Front-end apparently doesn't check this.
            if (TREE_CODE(callable) == FUNCTION_DECL)
            {
                error("need 'this' to access member %s", IDENTIFIER_POINTER(DECL_NAME(callable)));
                return error_mark_node;
            }
            else
            {   // Probably an internal error
                gcc_unreachable();
            }
        }
    }
    /* If this is a delegate call or a nested function being called as
       a delegate, the object should not be NULL. */
    if (object != NULL_TREE)
        actual_arg_list.cons(object);

    Parameters * formal_args = func_type->parameters; // can be NULL for genCfunc decls
    size_t n_formal_args = formal_args ? (int) Parameter::dim(formal_args) : 0;
    size_t n_actual_args = arguments ? arguments->dim : 0;
    size_t fi = 0;

    // assumes arguments->dim <= formal_args->dim if (! this->varargs)
    for (size_t ai = 0; ai < n_actual_args; ++ai)
    {
        tree actual_arg_tree;
        Expression * actual_arg_exp = arguments->tdata()[ai];

        if (ai == 0 && is_d_vararg)
        {   // The hidden _arguments parameter
            actual_arg_tree = actual_arg_exp->toElem(this);
        }
        else if (fi < n_formal_args)
        {   // Actual arguments for declared formal arguments
            Parameter * formal_arg = Parameter::getNth(formal_args, fi);
            actual_arg_tree = convertForArgument(actual_arg_exp, formal_arg);

            // from c-typeck.c: convert_arguments, default_conversion, ...
            if (INTEGRAL_TYPE_P (TREE_TYPE(actual_arg_tree))
                    && (TYPE_PRECISION (TREE_TYPE(actual_arg_tree)) <
                        TYPE_PRECISION (integer_type_node)))
            {
                actual_arg_tree = d_convert_basic(integer_type_node, actual_arg_tree);
            }
            ++fi;
        }
        else
        {
            if (splitDynArrayVarArgs && actual_arg_exp->type->toBasetype()->ty == Tarray)
            {
                tree da_exp = maybeMakeTemp(actual_arg_exp->toElem(this));
                actual_arg_list.cons(darrayLenRef(da_exp));
                actual_arg_list.cons(darrayPtrRef(da_exp));
                continue;
            }
            else
            {
                actual_arg_tree = actual_arg_exp->toElem(this);
                /* Not all targets support passing unpromoted types, so
                   promote anyway. */
                tree prom_type = d_type_promotes_to(TREE_TYPE(actual_arg_tree));
                if (prom_type != TREE_TYPE(actual_arg_tree))
                    actual_arg_tree = d_convert_basic(prom_type, actual_arg_tree);
            }
        }

        actual_arg_list.cons(actual_arg_tree);
    }

    tree result = buildCall(TREE_TYPE(func_type_node), actual_callee, actual_arg_list.head);
    result = maybeExpandSpecialCall(result);
#if V2
    if (func_type->isref)
        result = indirect(result);
#endif
    return result;
}

static const char * libcall_ids[LIBCALL_count] = {
    "_d_assert", "_d_assert_msg", "_d_array_bounds", "_d_switch_error",
    /*"_d_invariant",*/ "_D9invariant12_d_invariantFC6ObjectZv",
    "_d_newclass", "_d_newarrayT",
    "_d_newarrayiT",
    "_d_newarraymTp", "_d_newarraymiTp",
#if V2
    "_d_allocmemory",
#endif
    "_d_delclass", "_d_delinterface", "_d_delarray",
    "_d_delmemory", "_d_callfinalizer", "_d_callinterfacefinalizer",
    "_d_arraysetlengthT", "_d_arraysetlengthiT",
    "_d_dynamic_cast", "_d_interface_cast",
    "_adEq", "_adEq2", "_adCmp", "_adCmp2", "_adCmpChar",
    "_aaEqual", "_aaLen",
    //"_aaIn", "_aaGet", "_aaGetRvalue", "_aaDel",
    "_aaInp", "_aaGetp", "_aaGetRvaluep", "_aaDelp",
    "_d_arraycast",
    "_d_arraycopy",
    "_d_arraycatT", "_d_arraycatnT",
    "_d_arrayappendT",
    /*"_d_arrayappendc", */"_d_arrayappendcTp",
    "_d_arrayappendcd", "_d_arrayappendwd",
#if V2
    "_d_arrayassign", "_d_arrayctor", "_d_arraysetassign",
    "_d_arraysetctor", "_d_delarray_t",
#endif
    "_d_monitorenter", "_d_monitorexit",
    "_d_criticalenter", "_d_criticalexit",
    "_d_throw",
    "_d_switch_string", "_d_switch_ustring", "_d_switch_dstring",
    "_d_assocarrayliteralTp",
    "_d_arrayliteralTp"
#if V2
        ,
    "_d_unittest", "_d_unittest_msg",
    "_d_hidden_func"
#endif
};

static FuncDeclaration * libcall_decls[LIBCALL_count];

void
IRState::replaceLibCallDecl(FuncDeclaration * d_decl)
{
    if (! d_decl->ident)
        return;
    for (size_t i = 0; i < LIBCALL_count; i++)
    {
        if (strcmp(d_decl->ident->string, libcall_ids[i]) == 0)
        {   // %% warn if libcall already set?
            // Only do this for the libcalls where it's a problem, otherwise
            // it causes other problems...
            switch ((LibCall) i)
            {
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
    Types arg_types;
    bool varargs = false;

    if (! decl)
    {
        Type * return_type = Type::tvoid;

        switch (lib_call)
        {
            case LIBCALL_ASSERT:
            case LIBCALL_ARRAY_BOUNDS:
            case LIBCALL_SWITCH_ERROR:
                // need to spec chararray/string because internal code passes string constants
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tuns32);
                break;

            case LIBCALL_ASSERT_MSG:
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tuns32);
                break;
#if V2
            case LIBCALL_UNITTEST:
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tuns32);
                break;

            case LIBCALL_UNITTEST_MSG:
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tuns32);
                break;
#endif
            case LIBCALL_NEWCLASS:
                arg_types.push(ClassDeclaration::classinfo->type);
                return_type = getObjectType();
                break;

            case LIBCALL_NEWARRAYT:
            case LIBCALL_NEWARRAYIT:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tsize_t);
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_NEWARRAYMTP:
            case LIBCALL_NEWARRAYMITP:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tsize_t);
                arg_types.push(Type::tsize_t);
                return_type = Type::tvoid->arrayOf();
                break;
#if V2
            case LIBCALL_ALLOCMEMORY:
                arg_types.push(Type::tsize_t);
                return_type = Type::tvoidptr;
                break;
#endif
            case LIBCALL_DELCLASS:
            case LIBCALL_DELINTERFACE:
                arg_types.push(Type::tvoidptr);
                break;

            case LIBCALL_DELARRAY:
                arg_types.push(Type::tvoid->arrayOf()->pointerTo());
                break;
#if V2
            case LIBCALL_DELARRAYT:
                arg_types.push(Type::tvoid->arrayOf()->pointerTo());
                arg_types.push(Type::typeinfo->type);
                break;
#endif
            case LIBCALL_DELMEMORY:
                arg_types.push(Type::tvoidptr->pointerTo());
                break;

            case LIBCALL_CALLFINALIZER:
            case LIBCALL_CALLINTERFACEFINALIZER:
                arg_types.push(Type::tvoidptr);
                break;

            case LIBCALL_ARRAYSETLENGTHT:
            case LIBCALL_ARRAYSETLENGTHIT:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tsize_t);
                arg_types.push(Type::tvoid->arrayOf()->pointerTo());
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_DYNAMIC_CAST:
            case LIBCALL_INTERFACE_CAST:
                arg_types.push(getObjectType());
                arg_types.push(ClassDeclaration::classinfo->type);
                return_type = getObjectType();
                break;

            case LIBCALL_ADEQ:
            case LIBCALL_ADEQ2:
            case LIBCALL_ADCMP:
            case LIBCALL_ADCMP2:
                arg_types.push(Type::tvoid->arrayOf());
                arg_types.push(Type::tvoid->arrayOf());
                arg_types.push(Type::typeinfo->type);
                return_type = Type::tint32;
                break;

            case LIBCALL_ADCMPCHAR:
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tchar->arrayOf());
                return_type = Type::tint32;
                break;
//          case LIBCALL_AAIN:
//          case LIBCALL_AAGET:
//          case LIBCALL_AAGETRVALUE:
//          case LIBCALL_AADEL:
            case LIBCALL_AAEQUAL:
            case LIBCALL_AALEN:
            case LIBCALL_AAINP:
            case LIBCALL_AAGETP:
            case LIBCALL_AAGETRVALUEP:
            case LIBCALL_AADELP:
            {
                static Type * aa_type = NULL;
                if (! aa_type)
                    aa_type = new TypeAArray(Type::tvoidptr, Type::tvoidptr);

                if (lib_call == LIBCALL_AAEQUAL)
                {
                    arg_types.push(Type::typeinfo->type);
                    arg_types.push(aa_type);
                    arg_types.push(aa_type);
                    return_type = Type::tint32;
                    break;
                }

                if (lib_call == LIBCALL_AALEN)
                {
                    arg_types.push(aa_type);
                    return_type = Type::tsize_t;
                    break;
                }

                if (lib_call == LIBCALL_AAGETP)
                    aa_type = aa_type->pointerTo();

                arg_types.push(aa_type);
                arg_types.push(Type::typeinfo->type); // typeinfo reference
                if (lib_call == LIBCALL_AAGETP || lib_call == LIBCALL_AAGETRVALUEP)
                    arg_types.push(Type::tsize_t);

                arg_types.push(Type::tvoidptr);

                switch (lib_call)
                {
                    case LIBCALL_AAINP:
                    case LIBCALL_AAGETP:
                    case LIBCALL_AAGETRVALUEP:
                        return_type = Type::tvoidptr;
                        break;
                    case LIBCALL_AADELP:
                        return_type = Type::tvoid;
                        break;
                    default:
                        gcc_unreachable();
                }
                break;
            }

            case LIBCALL_ARRAYCAST:
                arg_types.push(Type::tsize_t);
                arg_types.push(Type::tsize_t);
                arg_types.push(Type::tvoid->arrayOf());
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_ARRAYCOPY:
                arg_types.push(Type::tsize_t);
                arg_types.push(Type::tvoid->arrayOf());
                arg_types.push(Type::tvoid->arrayOf());
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_ARRAYCATT:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tvoid->arrayOf());
                arg_types.push(Type::tvoid->arrayOf());
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_ARRAYCATNT:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tuns32); // Currently 'uint', even if 64-bit
                varargs = true;
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_ARRAYAPPENDT:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tuns8->arrayOf()->pointerTo());
                arg_types.push(Type::tuns8->arrayOf());
                return_type = Type::tvoid->arrayOf();
                break;

//          case LIBCALL_ARRAYAPPENDCT:
            case LIBCALL_ARRAYAPPENDCTP:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tuns8->arrayOf()->pointerTo());
                arg_types.push(Type::tsize_t);
                return_type = Type::tuns8->arrayOf();
                break;

            case LIBCALL_ARRAYAPPENDCD:
                arg_types.push(Type::tchar->arrayOf());
                arg_types.push(Type::tdchar);
                return_type = Type::tvoid->arrayOf();
                break;

            case LIBCALL_ARRAYAPPENDWD:
                arg_types.push(Type::twchar->arrayOf());
                arg_types.push(Type::tdchar);
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
                arg_types.push(Type::tvoidptr);
                arg_types.push(Type::tvoidptr);
                arg_types.push(Type::tsize_t);
                arg_types.push(Type::typeinfo->type);
                return_type = Type::tvoidptr;
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
                arg_types.push(Type::tvoidptr);
                break;

            case LIBCALL_SWITCH_USTRING:
                arg_types.push(Type::twchar->arrayOf()->arrayOf());
                arg_types.push(Type::twchar->arrayOf());
                return_type = Type::tint32;
                break;

            case LIBCALL_SWITCH_DSTRING:
                arg_types.push(Type::tdchar->arrayOf()->arrayOf());
                arg_types.push(Type::tdchar->arrayOf());
                return_type = Type::tint32;
                break;

            case LIBCALL_SWITCH_STRING:
                arg_types.push(Type::tchar->arrayOf()->arrayOf());
                arg_types.push(Type::tchar->arrayOf());
                return_type = Type::tint32;
                break;
            case LIBCALL_ASSOCARRAYLITERALTP:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tvoid->arrayOf());
                arg_types.push(Type::tvoid->arrayOf());
                return_type = Type::tvoidptr;
                break;

            case LIBCALL_ARRAYLITERALTP:
                arg_types.push(Type::typeinfo->type);
                arg_types.push(Type::tsize_t);
                return_type = Type::tvoidptr;
                break;
#if V2
            case LIBCALL_HIDDEN_FUNC:
                /* Argument is an Object, but can't use that as
                   LIBCALL_HIDDEN_FUNC is needed before the Object type is
                   created. */
                arg_types.push(Type::tvoidptr);
                break;
#endif
            default:
                gcc_unreachable();
        }
        decl = FuncDeclaration::genCfunc(return_type, libcall_ids[lib_call]);
        {
            TypeFunction * tf = (TypeFunction *) decl->type;
            tf->varargs = varargs ? 1 : 0;
            Parameters * args = new Parameters;
            args->setDim(arg_types.dim);
            for (size_t i = 0; i < arg_types.dim; i++)
                args->tdata()[i] = new Parameter(STCin, arg_types.tdata()[i], NULL, NULL);

            tf->parameters = args;
        }
        libcall_decls[lib_call] = decl;
    }
    return decl;
}

static tree
fix_d_va_list_type(tree val)
{
    if (POINTER_TYPE_P(va_list_type_node)
            || INTEGRAL_TYPE_P(va_list_type_node))
        return build1(NOP_EXPR, va_list_type_node, val);
    else
        return val;
}

tree
IRState::maybeExpandSpecialCall(tree call_exp)
{
    // More code duplication from C
    CallExpr ce(call_exp);
    tree callee = ce.callee();
    tree exp = NULL_TREE, op1, op2;
    enum built_in_function fcode;

    if (POINTER_TYPE_P(TREE_TYPE(callee)))
        callee = TREE_OPERAND(callee, 0);

    if (TREE_CODE(callee) == FUNCTION_DECL
        && DECL_BUILT_IN_CLASS(callee) == BUILT_IN_FRONTEND)
    {
        Intrinsic intrinsic = (Intrinsic) DECL_FUNCTION_CODE(callee);
        tree type;
        Type *d_type;
        switch (intrinsic)
        {
            case INTRINSIC_BSF:
                /* builtin count_trailing_zeros matches behaviour of bsf.
                   %% TODO: The return value is supposed to be undefined if op1 is zero. */
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_CTZL], 1, op1);

            case INTRINSIC_BSR:
                /* bsr becomes 31-(clz), but parameter passed to bsf may not be a 32bit type!!
                   %% TODO: The return value is supposed to be undefined if op1 is zero. */
                op1 = ce.nextArg();
                type = TREE_TYPE(op1);

                // %% Using TYPE_ALIGN, should be ok for size_t on 64bit...
                op2 = integerConstant(TYPE_ALIGN(type) - 1, TREE_TYPE(op1));
                exp = buildCall(built_in_decls[BUILT_IN_CLZL], 1, op1);

                // Handle int -> long conversions.
                if (TREE_TYPE(exp) != type)
                    exp = fold_convert (type, exp);

                return build2(MINUS_EXPR, type, op2, exp);

            case INTRINSIC_BT:
            case INTRINSIC_BTC:
            case INTRINSIC_BTR:
            case INTRINSIC_BTS:
                op1 = ce.nextArg();
                op2 = ce.nextArg();
                type = TREE_TYPE(TREE_TYPE(op1));

                // op1[op2 / align]
                op1 = pointerIntSum(op1, build2(TRUNC_DIV_EXPR, type, op2,
                                    integerConstant(TYPE_ALIGN(type), type)));
                op1 = maybeMakeTemp(op1);

                // mask = 1 << (op2 & (align - 1));
                op2 = build2(BIT_AND_EXPR, type, op2, integerConstant(TYPE_ALIGN(type) - 1, type));
                op2 = build2(LSHIFT_EXPR, type, integerConstant(1, type), op2);
                op2 = maybeMakeTemp(op2);

                // Update the bit as needed, bt adds zero to value.
                if (PTRSIZE == 4)
                    fcode = (intrinsic == INTRINSIC_BT)  ? BUILT_IN_FETCH_AND_ADD_4 :
                            (intrinsic == INTRINSIC_BTC) ? BUILT_IN_FETCH_AND_XOR_4 :
                            (intrinsic == INTRINSIC_BTR) ? BUILT_IN_FETCH_AND_AND_4 :
                          /* intrinsic == INTRINSIC_BTS */ BUILT_IN_FETCH_AND_OR_4;
                else if (PTRSIZE == 8)
                    fcode = (intrinsic == INTRINSIC_BT)  ? BUILT_IN_FETCH_AND_ADD_8 :
                            (intrinsic == INTRINSIC_BTC) ? BUILT_IN_FETCH_AND_XOR_8 :
                            (intrinsic == INTRINSIC_BTR) ? BUILT_IN_FETCH_AND_AND_8 :
                          /* intrinsic == INTRINSIC_BTS */ BUILT_IN_FETCH_AND_OR_8;
                else
                    gcc_unreachable();

                exp = (intrinsic == INTRINSIC_BT) ? integer_zero_node : op2;
                if (intrinsic == INTRINSIC_BTR)
                    exp = build1(BIT_NOT_EXPR, TREE_TYPE(exp), exp);

                exp = buildCall(built_in_decls[fcode], 2, op1, exp);

                // cond = (exp & mask) ? -1 : 0;
                exp = build2(BIT_AND_EXPR, TREE_TYPE(exp), exp, op2);
                exp = fold_convert(TREE_TYPE(call_exp), exp);
                return build3(COND_EXPR, TREE_TYPE(call_exp), d_truthvalue_conversion(exp),
                              integer_minus_one_node, integer_zero_node);

            case INTRINSIC_BSWAP:
#if D_GCC_VER >= 43
                /* Backend provides builtin bswap32.
                   Assumes first argument and return type is uint. */
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_BSWAP32], 1, op1);
#else
                /* Expand a call to bswap intrinsic with argument op1.
                    TODO: use asm if 386?
                 */
                op1 = ce.nextArg();
                type = TREE_TYPE(op1);
                // exp = (op1 & 0xFF) << 24
                exp = build2(BIT_AND_EXPR, type, op1, integerConstant(0xff, type));
                exp = build2(LSHIFT_EXPR, type, exp, integerConstant(24, type));

                // exp |= (op1 & 0xFF00) << 8
                op2 = build2(BIT_AND_EXPR, type, op1, integerConstant(0xff00, type));
                op2 = build2(LSHIFT_EXPR, type, op2, integerConstant(8, type));
                exp = build2(BIT_IOR_EXPR, type, exp, op2);

                // exp |= (op1 & 0xFF0000) >>> 8
                op2 = build2(BIT_AND_EXPR, type, op1, integerConstant(0xff0000, type));
                op2 = build2(RSHIFT_EXPR, type, op2, integerConstant(8, type));
                exp = build2(BIT_IOR_EXPR, type, exp, op2);

                // exp |= op1 & 0xFF000000) >>> 24
                op2 = build2(BIT_AND_EXPR, type, op1, integerConstant(0xff000000, type));
                op2 = build2(RSHIFT_EXPR, type, op2, integerConstant(24, type));
                exp = build2(BIT_IOR_EXPR, type, exp, op2);

                return exp;
#endif

            case INTRINSIC_INP:
            case INTRINSIC_INPL:
            case INTRINSIC_INPW:
#ifdef TARGET_386
                type = TREE_TYPE(call_exp);
                d_type = Type::tuns16;

                op1 = ce.nextArg();
                // %% Port is always cast to ushort
                op1 = d_convert_basic(d_type->toCtype(), op1);
                op2 = localVar(type);
                return expandPortIntrinsic(intrinsic, op1, op2, 0);
#endif
                // else fall through to error below.
            case INTRINSIC_OUTP:
            case INTRINSIC_OUTPL:
            case INTRINSIC_OUTPW:
#ifdef TARGET_386
                type = TREE_TYPE(call_exp);
                d_type = Type::tuns16;

                op1 = ce.nextArg();
                op2 = ce.nextArg();
                // %% Port is always cast to ushort
                op1 = d_convert_basic(d_type->toCtype(), op1);
                return expandPortIntrinsic(intrinsic, op1, op2, 1);
#else
                ::error("Port I/O intrinsic '%s' is only available on ix86 targets",
                        IDENTIFIER_POINTER(DECL_NAME(callee)));
                break;
#endif

            case INTRINSIC_COS:
                // Math intrinsics just map to their GCC equivalents.
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_COSL], 1, op1);

            case INTRINSIC_SIN:
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_SINL], 1, op1);

            case INTRINSIC_RNDTOL:
                // %% not sure if llroundl stands as a good replacement
                // for the expected behaviour of rndtol.
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_LLROUNDL], 1, op1);

            case INTRINSIC_SQRT:
                // Have float, double and real variants of sqrt.
                op1 = ce.nextArg();
                type = TREE_TYPE(op1);
                // Could have used mathfn_built_in, but that only returns
                // implicit built in decls.
                if (TYPE_MAIN_VARIANT(type) == double_type_node)
                    exp = built_in_decls[BUILT_IN_SQRT];
                else if (TYPE_MAIN_VARIANT(type) == float_type_node)
                    exp = built_in_decls[BUILT_IN_SQRTF];
                else if (TYPE_MAIN_VARIANT(type) == long_double_type_node)
                    exp = built_in_decls[BUILT_IN_SQRTL];
                // op1 is an integral type - use double precision.
                else if (INTEGRAL_TYPE_P(TYPE_MAIN_VARIANT(type)))
                {
                    op1 = d_convert_basic(double_type_node, op1);
                    exp = built_in_decls[BUILT_IN_SQRT];
                }

                gcc_assert(exp);    // Should never trigger.
                return buildCall(exp, 1, op1);

            case INTRINSIC_LDEXP:
                op1 = ce.nextArg();
                op2 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_LDEXPL], 2, op1, op2);

            case INTRINSIC_FABS:
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_FABSL], 1, op1);

            case INTRINSIC_RINT:
                op1 = ce.nextArg();
                return buildCall(built_in_decls[BUILT_IN_RINTL], 1, op1);


            case INTRINSIC_C_VA_ARG:
                // %% should_check c_promotes_to as in va_arg now
                // just drop though for now...
            case INTRINSIC_STD_VA_ARG:
                op1 = ce.nextArg();
                /* signature is (inout va_list), but VA_ARG_EXPR expects the
                   list itself... but not if the va_list type is an array.  In that
                   case, it should be a pointer.  */
                if (TREE_CODE(d_va_list_type_node) != ARRAY_TYPE)
                {
                    if (TREE_CODE(op1) == ADDR_EXPR)
                    {
                        op1 = TREE_OPERAND(op1, 0);
                    }
                    else
                    {   /* this probably doesn't happen... passing an inout va_list argument,
                           but again,  it's probably { & (* inout_arg) }  */
                        op1 = indirect(op1);
                    }
                }
#if SARRAYVALUE
                else
                {   // list would be passed by value, convert to reference.
                    tree t = TREE_TYPE(op1);
                    gcc_assert(TREE_CODE(t) == ARRAY_TYPE);
                    op1 = addressOf(op1);
                }
#endif
                op1 = fix_d_va_list_type(op1);
                type = TREE_TYPE(TREE_TYPE(callee));
                if (splitDynArrayVarArgs && (d_type = getDType(type)) &&
                        d_type->toBasetype()->ty == Tarray)
                {   /* should create a temp var of type TYPE and move the binding
                       to outside this expression.  */
                    op1 = stabilize_reference(op1);
                    tree ltype = TREE_TYPE(TYPE_FIELDS(type));
                    tree ptype = TREE_TYPE(TREE_CHAIN(TYPE_FIELDS(type)));
                    tree lvar = exprVar(ltype);
                    tree pvar = exprVar(ptype);
                    tree e1 = vmodify(lvar, build1(VA_ARG_EXPR, ltype, op1));
                    tree e2 = vmodify(pvar, build1(VA_ARG_EXPR, ptype, op1));
                    tree b = compound(compound(e1, e2), darrayVal(type, lvar, pvar));
                    return binding(lvar, binding(pvar, b));
                }
                else
                {
                    tree type2 = d_type_promotes_to(type);
                    op1 = build1(VA_ARG_EXPR, type2, op1);
                    if (type != type2)
                    {   // silently convert promoted type...
                        op1 = d_convert_basic(type, op1);
                    }
                    return op1;
                }
                break;

            case INTRINSIC_C_VA_START:
                /* The va_list argument should already have its
                   address taken.  The second argument, however, is
                   inout and that needs to be fixed to prevent a warning.  */
                op1 = ce.nextArg();
                op2 = ce.nextArg();
                type = TREE_TYPE(op1);
                // kinda wrong... could be casting.. so need to check type too?
                while (TREE_CODE(op1) == NOP_EXPR)
                    op1 = TREE_OPERAND(op1, 0);

                if (TREE_CODE(op1) == ADDR_EXPR)
                {
                    op1 = TREE_OPERAND(op1, 0);
                    op1 = fix_d_va_list_type(op1);
                    op1 = addressOf(op1);
                }
#if SARRAYVALUE
                else if (TREE_CODE(type) == ARRAY_TYPE)
                {   // pass list by reference.
                    op1 = fix_d_va_list_type(op1);
                    op1 = addressOf(op1);
                }
#endif
                else
                {
                    op1 = fix_d_va_list_type(op1);
                }

                if (TREE_CODE(op2) == ADDR_EXPR)
                    op2 = TREE_OPERAND(op2, 0);
                // assuming nobody tries to change the return type
                return buildCall(built_in_decls[BUILT_IN_VA_START], 2, op1, op2);

            default:
                gcc_unreachable();
        }
    }

    return call_exp;
}


/* Expand a call to inp or outp with argument 'port' and return value 'value'.
 */
tree
IRState::expandPortIntrinsic(Intrinsic code, tree port, tree value, int outp)
{
    const char * insn_string;
    tree insn_tmpl;
    ListMaker outputs;
    ListMaker inputs;

    if (outp)
    {   /* Writing outport operand:
            asm ("op %[value], %[port]" :: "a" value, "Nd" port);
         */
        const char * valconstr = "a";
        tree val = tree_cons(NULL_TREE, build_string(strlen(valconstr), valconstr), NULL_TREE);
        inputs.cons(val, value);
    }
    else
    {   /* Writing inport operand:
            asm ("op %[port], %[value]" : "=a" value : "Nd" port);
         */
        const char * outconstr = "=a";
        tree out = tree_cons(NULL_TREE, build_string(strlen(outconstr), outconstr), NULL_TREE);
        outputs.cons(out, value);
    }

    const char * inconstr = "Nd";
    tree in = tree_cons(NULL_TREE, build_string(strlen(inconstr), inconstr), NULL_TREE);
    inputs.cons(in, port);

    switch(code)
    {
        case INTRINSIC_INP:     insn_string = "inb %w1, %0"; break;
        case INTRINSIC_INPL:    insn_string = "inl %w1, %0"; break;
        case INTRINSIC_INPW:    insn_string = "inw %w1, %0"; break;
        case INTRINSIC_OUTP:    insn_string = "outb %b0, %w1"; break;
        case INTRINSIC_OUTPL:   insn_string = "outl %0, %w1";  break;
        case INTRINSIC_OUTPW:   insn_string = "outw %w0, %w1"; break;
        default:
            gcc_unreachable();
    }
    insn_tmpl = build_string(strlen(insn_string), insn_string);

    // ::doAsm
    tree exp = d_build_asm_stmt(insn_tmpl, outputs.head, inputs.head,
                                NULL_TREE, NULL_TREE);
    ASM_VOLATILE_P(exp) = 1;

    // These functions always return the contents of 'value'
    return build2(COMPOUND_EXPR, TREE_TYPE(value), exp, value);
}


tree
IRState::arrayElemRef(IndexExp * aer_exp, ArrayScope * aryscp)
{
    Expression * e1 = aer_exp->e1;
    Expression * e2 = aer_exp->e2;

    Type * base_type = e1->type->toBasetype();
    TY base_type_ty = base_type->ty;
    tree index_expr; // logical index
    tree subscript_expr; // expr that indexes the array data
    tree ptr_exp;  // base pointer to the elements
    tree elem_ref; // reference the the element

    index_expr = e2->toElem(this);
    subscript_expr = index_expr;

    switch (base_type_ty)
    {
        case Tarray:
        case Tsarray:
        {
            tree e1_tree = e1->toElem(this);
            e1_tree = aryscp->setArrayExp(e1_tree, e1->type);

            // If it's a static array and the index is constant,
            // the front end has already checked the bounds.
            if (arrayBoundsCheck() &&
                ! (base_type_ty == Tsarray && e2->isConst()))
            {
                tree array_len_expr, throw_expr, oob_cond;
                // implement bounds check as a conditional expression:
                // a[ inbounds(index) ? index : { throw ArrayBoundsError } ]
                //
                // First, set up the index expression to only be evaluated
                // once.
                // %% save_expr does this check: if (! TREE_CONSTANT(index_expr))
                //   %% so we don't do a <0 check for a[2]...
                index_expr = maybeMakeTemp(index_expr);

                if (base_type_ty == Tarray)
                {
                    e1_tree = maybeMakeTemp(e1_tree);
                    array_len_expr = darrayLenRef(e1_tree);
                }
                else
                    array_len_expr = ((TypeSArray *) base_type)->dim->toElem(this);

                oob_cond = boundsCond(index_expr, array_len_expr, false);
                throw_expr = assertCall(aer_exp->loc, LIBCALL_ARRAY_BOUNDS);

                subscript_expr = build3(COND_EXPR, TREE_TYPE(index_expr),
                        oob_cond, index_expr, throw_expr);
            }

            // %% TODO: make this an ARRAY_REF?
            if (base_type_ty == Tarray)
                ptr_exp = darrayPtrRef(e1_tree); // %% do convert in darrayPtrRef?
            else
                ptr_exp = addressOf(e1_tree);
            // This conversion is required for static arrays and is just-to-be-safe
            // for dynamic arrays
            ptr_exp = d_convert_basic(base_type->nextOf()->pointerTo()->toCtype(), ptr_exp);
            break;
        }
        case Tpointer:
        {   // Ignores aryscp
            ptr_exp = e1->toElem(this);
            break;
        }
        default:
            gcc_unreachable();
    }

    ptr_exp = pvoidOkay(ptr_exp);
    subscript_expr = aryscp->finish(subscript_expr);
    elem_ref = indirect(pointerIntSum(ptr_exp, subscript_expr),
            TREE_TYPE(TREE_TYPE(ptr_exp)));

    return elem_ref;
}

tree
IRState::darrayPtrRef(tree exp)
{
    if (isErrorMark(exp))
    {   // backend will ICE otherwise
        return exp;
    }
    // Get the backend type for the array and pick out the array data
    // pointer field (assumed to be the second field.)
    tree ptr_field = TREE_CHAIN(TYPE_FIELDS(TREE_TYPE(exp)));
    //return build2(COMPONENT_REF, TREE_TYPE(ptr_field), exp, ptr_field);
    return component(exp, ptr_field);
}

tree
IRState::darrayLenRef(tree exp)
{
    if (isErrorMark(exp))
    {   // backend will ICE otherwise
        return exp;
    }
    // Get the backend type for the array and pick out the array length
    // field (assumed to be the first field.)
    tree len_field = TYPE_FIELDS(TREE_TYPE(exp));
    return component(exp, len_field);
}


tree
IRState::darrayVal(tree type, tree len, tree data)
{
    // %% assert type is a darray
    tree len_field, ptr_field;
    CtorEltMaker ce;

    len_field = TYPE_FIELDS(type);
    ptr_field = TREE_CHAIN(len_field);

    ce.cons(len_field, len);
    ce.cons(ptr_field, data); // shouldn't need to convert the pointer...

    tree ctor = build_constructor(type, ce.head);
    TREE_STATIC(ctor) = 0;   // can be set by caller if needed
    TREE_CONSTANT(ctor) = 0; // "

    return ctor;
}

tree
IRState::darrayVal(tree type, uinteger_t len, tree data)
{
    // %% assert type is a darray
    tree len_value, ptr_value, len_field, ptr_field;
    CtorEltMaker ce;

    len_field = TYPE_FIELDS(type);
    ptr_field = TREE_CHAIN(len_field);

    if (data)
    {
        gcc_assert(POINTER_TYPE_P(TREE_TYPE(data)));
        ptr_value = data;
    }
    else
    {
        ptr_value = convert(TREE_TYPE(ptr_field), d_null_pointer);
    }

    len_value = integerConstant(len, TREE_TYPE(len_field));
    ce.cons(len_field, len_value);
    ce.cons(ptr_field, ptr_value); // shouldn't need to convert the pointer...

    tree ctor = build_constructor(type, ce.head);
    TREE_STATIC(ctor) = 0;   // can be set by caller if needed
    TREE_CONSTANT(ctor) = 0; // "

    return ctor;
}

tree
IRState::darrayString(const char * str)
{
    unsigned len = strlen(str);
    // %% assumes str is null-terminated
    tree str_tree = build_string(len + 1, str);

    TREE_TYPE(str_tree) = arrayType(Type::tchar, len);
    return darrayVal(Type::tchar->arrayOf()->toCtype(), len, addressOf(str_tree));
}

char *
IRState::hostToTargetString(char * str, size_t length, unsigned unit_size)
{
    if (unit_size == 1)
        return str;
    gcc_assert(unit_size == 2 || unit_size == 4);

    bool flip;
    if (WORDS_BIG_ENDIAN)
        flip = (bool) ! BYTES_BIG_ENDIAN;
    else
        flip = (bool) BYTES_BIG_ENDIAN;

    if (flip)
    {
        char * out_str = (char *) xmalloc(length * unit_size);
        const d_uns8 * p_src = (const d_uns8 *) str;
        d_uns8 * p_out = (d_uns8 *) out_str;

        while (length--)
        {
            if (unit_size == 2)
            {
                p_out[0] = p_src[1];
                p_out[1] = p_src[0];
            } else
            {   /* unit_size == 4 */
                p_out[0] = p_src[3];
                p_out[1] = p_src[2];
                p_out[2] = p_src[1];
                p_out[3] = p_src[0];
            }
            p_src += unit_size;
            p_out += unit_size;
        }
        return out_str;
    }
    // else
    return str;
}


tree
IRState::arrayLength(tree exp, Type * exp_type)
{
    Type * base_type = exp_type->toBasetype();
    switch (base_type->ty)
    {
        case Tsarray:
            return size_int(((TypeSArray *) base_type)->dim->toUInteger());
        case Tarray:
            return darrayLenRef(exp);
        default:
            ::error("can't determine the length of a %s", exp_type->toChars());
            return error_mark_node;
    }
}

tree
IRState::floatMod(tree a, tree b, tree type)
{
    tree fmodfn = NULL_TREE;
    tree basetype = type;

    if (COMPLEX_FLOAT_TYPE_P(basetype))
        basetype = TREE_TYPE(basetype);

    if (TYPE_MAIN_VARIANT(basetype) == double_type_node)
        fmodfn = built_in_decls[BUILT_IN_FMOD];
    else if (TYPE_MAIN_VARIANT(basetype) == float_type_node)
        fmodfn = built_in_decls[BUILT_IN_FMODF];
    else if (TYPE_MAIN_VARIANT(basetype) == long_double_type_node)
        fmodfn = built_in_decls[BUILT_IN_FMODL];

    if (! fmodfn)
    {   // %qT pretty prints the tree type.
        ::error("tried to perform floating-point modulo division on %qT",
                type);
        return error_mark_node;
    }

    if (COMPLEX_FLOAT_TYPE_P(type))
    {
        return build2(COMPLEX_EXPR, type,
                      buildCall(fmodfn, 2, realPart(a), b),
                      buildCall(fmodfn, 2, imagPart(a), b));
    }
    else if (SCALAR_FLOAT_TYPE_P(type))
    {   // %% assuming no arg conversion needed
        // %% bypassing buildCall since this shouldn't have
        // side effects
        return buildCall(fmodfn, 2, a, b);
    }
    else
    {   // Should have caught this above.
        gcc_unreachable();
    }
}

tree
IRState::typeinfoReference(Type * t)
{
    tree ti_ref = t->getInternalTypeInfo(NULL)->toElem(this);
    gcc_assert(POINTER_TYPE_P(TREE_TYPE(ti_ref)));
    return ti_ref;
}

dinteger_t
IRState::getTargetSizeConst(tree t)
{
    dinteger_t result;
    if (sizeof(HOST_WIDE_INT) >= sizeof(dinteger_t))
        result = tree_low_cst(t, 1);
    else
    {
        gcc_assert(sizeof(HOST_WIDE_INT) * 2 == sizeof(dinteger_t));
        result = (unsigned HOST_WIDE_INT) TREE_INT_CST_LOW(t);
        result += ((dinteger_t) (unsigned HOST_WIDE_INT) TREE_INT_CST_HIGH(t))
            << HOST_BITS_PER_WIDE_INT;
    }
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
    tree obj_field = TYPE_FIELDS(TREE_TYPE(exp));
    //return build2(COMPONENT_REF, TREE_TYPE(obj_field), exp, obj_field);
    return component(exp, obj_field);
}

tree
IRState::delegateMethodRef(tree exp)
{
    // Get the backend type for the array and pick out the array length
    // field (assumed to be the second field.)
    tree method_field = TREE_CHAIN(TYPE_FIELDS(TREE_TYPE(exp)));
    //return build2(COMPONENT_REF, TREE_TYPE(method_field), exp, method_field);
    return component(exp, method_field);
}

// Converts pointer types of method_exp and object_exp to match d_type
tree
IRState::delegateVal(tree method_exp, tree object_exp, Type * d_type)
{
    Type * base_type = d_type->toBasetype();
    if (base_type->ty == Tfunction)
    {   // Called from DotVarExp.  These are just used to
        // make function calls and not to make Tdelegate variables.
        // Clearing the type makes sure of this.
        base_type = 0;
    }
    else
    {
        gcc_assert(base_type->ty == Tdelegate);
    }

    tree type = base_type ? base_type->toCtype() : NULL_TREE;
    tree ctor = make_node(CONSTRUCTOR);
    tree obj_field = NULL_TREE;
    tree func_field = NULL_TREE;
    CtorEltMaker ce;

    if (type)
    {
        TREE_TYPE(ctor) = type;
        obj_field = TYPE_FIELDS(type);
        func_field = TREE_CHAIN(obj_field);
    }
#if ENABLE_CHECKING
    if (obj_field)
        ce.cons(obj_field, convert (TREE_TYPE(obj_field), object_exp));
    else
        ce.cons(obj_field, object_exp);

    if (func_field)
        ce.cons(func_field, convert (TREE_TYPE(func_field), method_exp));
    else
        ce.cons(func_field, method_exp);
#else
    ce.cons(obj_field, object_exp);
    ce.cons(func_field, method_exp);
#endif
    CONSTRUCTOR_ELTS(ctor) = ce.head;
    return ctor;
}

void
IRState::extractMethodCallExpr(tree mcr, tree & callee_out, tree & object_out)
{
    gcc_assert(D_IS_METHOD_CALL_EXPR(mcr));

    VEC(constructor_elt,gc) *elts = CONSTRUCTOR_ELTS(mcr);
    object_out = VEC_index(constructor_elt, elts, 0)->value;
    callee_out = VEC_index(constructor_elt, elts, 1)->value;
}

tree
IRState::objectInstanceMethod(Expression * obj_exp, FuncDeclaration * func, Type * d_type)
{
    Type * obj_type = obj_exp->type->toBasetype();
    if (func->isThis())
    {
        bool is_dottype;
        tree this_expr;

        // DotTypeExp cannot be evaluated
        if (obj_exp->op == TOKdottype)
        {
            is_dottype = true;
            this_expr = ((DotTypeExp *) obj_exp)->e1->toElem(this);
        }
        else if (obj_exp->op == TOKcast &&
                ((CastExp*) obj_exp)->e1->op == TOKdottype)
        {
            is_dottype = true;
            // see expression.c:"See if we need to adjust the 'this' pointer"
            this_expr = ((DotTypeExp *) ((CastExp*) obj_exp)->e1)->e1->toElem(this);
        }
        else
        {
            is_dottype = false;
            this_expr = obj_exp->toElem(this);
        }

        // Calls to super are static (func is the super's method)
        // Structs don't have vtables.
        // Final and non-virtual methods can be called directly.
        // DotTypeExp means non-virtual

        if (obj_exp->op == TOKsuper ||
                obj_type->ty == Tstruct || obj_type->ty == Tpointer ||
                func->isFinal() || ! func->isVirtual() || is_dottype)
        {
            if (obj_type->ty == Tstruct)
                this_expr = addressOf(this_expr);
            return methodCallExpr(functionPointer(func), this_expr, d_type);
        }
        else
        {   // Interface methods are also in the class's vtable, so we don't
            // need to convert from a class pointer to an interface pointer.
            this_expr = maybeMakeTemp(this_expr);

            tree vtbl_ref;
            /* Folding of *&<static var> fails because of the type of the
               address expression is 'Object' while the type of the static
               var is a particular class (why?). This prevents gimplification
               of the expression.
            */
            if (TREE_CODE(this_expr) == ADDR_EXPR /*&&
                // can't use this check
                TREE_TYPE(TREE_OPERAND(this_expr, 0)) ==
                TREE_TYPE(TREE_TYPE(this_expr))*/)
            {
                vtbl_ref = TREE_OPERAND(this_expr, 0);
            }
            else
            {
                vtbl_ref = indirect(this_expr);
            }

            tree field = TYPE_FIELDS(TREE_TYPE(vtbl_ref)); // the vtbl is the first field
            //vtbl_ref = build2(COMPONENT_REF, TREE_TYPE(field), vtbl_ref, field); // vtbl field (a pointer)
            vtbl_ref = component(vtbl_ref, field); // vtbl field (a pointer)
            // %% better to do with array ref?
            vtbl_ref = pointerOffset(vtbl_ref,
                    size_int(PTRSIZE * func->vtblIndex));
            vtbl_ref = indirect(vtbl_ref, TREE_TYPE(functionPointer(func)));

            return methodCallExpr(vtbl_ref, this_expr, d_type);
        }
    }
    else
    {   // Static method; ignore the object instance
        return addressOf(func);
    }
}


tree
IRState::realPart(tree c)
{
    return build1(REALPART_EXPR, TREE_TYPE(TREE_TYPE(c)), c);
}

tree
IRState::imagPart(tree c)
{
    return build1(IMAGPART_EXPR, TREE_TYPE(TREE_TYPE(c)), c);
}

tree
IRState::assignValue(Expression * e, VarDeclaration * v)
{
    if (e->op == TOKassign || e->op == TOKblit)
    {
        AssignExp * a_exp = (AssignExp *) e;
        if (a_exp->e1->op == TOKvar && ((VarExp *) a_exp->e1)->var == v)
        {
            tree a_val = convertForAssignment(a_exp->e2, v->type);
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
    tree f0 = d_build_decl_loc(BUILTINS_LOCATION, FIELD_DECL, get_identifier(n1), ft1);
    tree f1 = d_build_decl_loc(BUILTINS_LOCATION, FIELD_DECL, get_identifier(n2), ft2);
    DECL_CONTEXT(f0) = rec_type;
    DECL_CONTEXT(f1) = rec_type;
    TYPE_FIELDS(rec_type) = chainon(f0, f1);
    layout_type(rec_type);
    if (d_type)
    {   /* This is needed so that maybeExpandSpecialCall knows to
           split dynamic array varargs. */
        TYPE_LANG_SPECIFIC(rec_type) = build_d_type_lang_specific(d_type);

        /* ObjectFile::declareType will try to declare it as top-level type
           which can break debugging info for element types. */
        tree stub_decl = d_build_decl_loc(BUILTINS_LOCATION, TYPE_DECL,
                get_identifier(d_type->toChars()), rec_type);
        TYPE_STUB_DECL(rec_type) = stub_decl;
        TYPE_NAME(rec_type) = stub_decl;
        DECL_ARTIFICIAL(stub_decl) = 1;
        g.ofile->rodc(stub_decl, 0);
    }
    return rec_type;
}

// Create a record type from two field types
tree
IRState::twoFieldType(Type * ft1, Type * ft2, Type * d_type, const char * n1, const char * n2)
{
    return twoFieldType(make_node(RECORD_TYPE), ft1->toCtype(), ft2->toCtype(), d_type, n1, n2);
}

tree
IRState::twoFieldCtor(tree rec_type, tree f1, tree f2, int storage_class)
{
    CtorEltMaker ce;
    ce.cons(TYPE_FIELDS(rec_type), f1);
    ce.cons(TREE_CHAIN(TYPE_FIELDS(rec_type)), f2);

    tree ctor = build_constructor(rec_type, ce.head);
    TREE_STATIC(ctor) = (storage_class & STCstatic) != 0;
    TREE_CONSTANT(ctor) = (storage_class & STCconst) != 0;
    TREE_READONLY(ctor) = (storage_class & STCconst) != 0;

    return ctor;
}

// This could be made more lax to allow better CSE (?)
bool
needs_temp(tree t)
{
    // %%TODO: check for anything with TREE_SIDE_EFFECTS?
    switch (TREE_CODE(t))
    {
        case VAR_DECL:
        case FUNCTION_DECL:
        case PARM_DECL:
        case CONST_DECL:
        case SAVE_EXPR:
            return false;

        case ADDR_EXPR:
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
        {
            if (TREE_CODE_CLASS(TREE_CODE(t)) == tcc_constant)
                return false;
            else
                return true;
        }
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
    if (needs_temp(t))
    {
        if (TREE_CODE(TREE_TYPE(t)) != ARRAY_TYPE)
            return save_expr(t);
        else
            return stabilize_reference(t);
    }
    else
    {
        return t;
    }
}

Module * IRState::builtinsModule = 0;
Module * IRState::intrinsicModule = 0;
Module * IRState::intrinsicCoreModule = 0;
Module * IRState::mathModule = 0;
Module * IRState::mathCoreModule = 0;
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

    if (! dsym)
        return false;

    if ((intrinsicModule && dsym->getModule() == intrinsicModule) ||
        (intrinsicCoreModule && dsym->getModule() == intrinsicCoreModule))
    {   // Matches order of Intrinsic enum
        static const char * intrinsic_names[] = {
            "bsf", "bsr",
            "bswap",
            "bt", "btc", "btr", "bts",
            "inp", "inpl", "inpw",
            "outp", "outpl", "outpw",
        };
        const size_t sz = sizeof(intrinsic_names) / sizeof(char *);
        int i = binary(decl->ident->string, intrinsic_names, sz);
        if (i == -1)
            return false;

        // Make sure 'i' is within the range we require.
        gcc_assert(i >= INTRINSIC_BSF && i <= INTRINSIC_OUTPW);
        tree t = decl->toSymbol()->Stree;

        DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
        DECL_FUNCTION_CODE(t) = (built_in_function) i;
        return true;
    }
    else if ((mathModule && dsym->getModule() == mathModule) ||
             (mathCoreModule && dsym->getModule() == mathCoreModule))
    {   // Matches order of Intrinsic enum
        static const char * math_names[] = {
            "cos", "fabs", "ldexp",
            "rint", "rndtol", "sin",
            "sqrt",
        };
        const size_t sz = sizeof(math_names) / sizeof(char *);
        int i = binary(decl->ident->string, math_names, sz);
        if (i == -1)
            return false;

        // Adjust 'i' for this range of enums
        i += INTRINSIC_COS;
        gcc_assert(i >= INTRINSIC_COS && i <= INTRINSIC_SQRT);
        tree t = decl->toSymbol()->Stree;

        // rndtol returns a long, sqrt any real value,
        // every other math builtin returns an 80bit float.
        Type * tf = decl->type->nextOf();
        if ((i == INTRINSIC_RNDTOL && tf->ty == Tint64)
            || (i == INTRINSIC_SQRT && tf->isreal())
            || (i != INTRINSIC_RNDTOL && tf->ty == Tfloat80))
        {
            DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
            DECL_FUNCTION_CODE(t) = (built_in_function) i;
            return true;
        }
        return false;
    }
    else
    {
        ti = dsym->isTemplateInstance();
        if (ti)
        {
            tree t = decl->toSymbol()->Stree;
            if (ti->tempdecl == stdargTemplateDecl)
            {
                DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
                DECL_FUNCTION_CODE(t) = (built_in_function) INTRINSIC_STD_VA_ARG;
                return true;
            }
            else if (ti->tempdecl == cstdargArgTemplateDecl)
            {
                DECL_BUILT_IN_CLASS(t) = BUILT_IN_FRONTEND;
                DECL_FUNCTION_CODE(t) = (built_in_function) INTRINSIC_C_VA_ARG;
                return true;
            }
            else if (ti->tempdecl == cstdargStartTemplateDecl)
            {
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
    if (base_type->ty == Treference)
        return true;

    if (decl->isOut() || decl->isRef())
        return true;

#if !SARRAYVALUE
    if (decl->isParameter() && base_type->ty == Tsarray)
        return true;
#endif
    return false;
}

tree
IRState::trueDeclarationType(Declaration * decl)
{
    // If D supported references, we would have to check twice for
    //   (out T &) -- disallow, maybe or make isDeclarationReferenceType return
    //   the number of levels to reference
    tree decl_type = decl->type->toCtype();
    if (isDeclarationReferenceType(decl))
    {
        return build_reference_type(decl_type);
    }
    else if (decl->storage_class & STClazy)
    {
        TypeFunction *tf = new TypeFunction(NULL, decl->type, 0, LINKd);
        TypeDelegate *t = new TypeDelegate(tf);
        return t->merge()->toCtype();
    }
    return decl_type;
}

// These should match the Declaration versions above
bool
IRState::isArgumentReferenceType(Parameter * arg)
{
    Type * base_type = arg->type->toBasetype();

    if (base_type->ty == Treference)
        return true;

    if (arg->storageClass & (STCout | STCref))
        return true;

#if !SARRAYVALUE
    if (base_type->ty == Tsarray)
        return true;
#endif
    return false;
}

tree
IRState::trueArgumentType(Parameter * arg)
{
    tree arg_type = arg->type->toCtype();
    if (isArgumentReferenceType(arg))
    {
        return build_reference_type(arg_type);
    }
    else if (arg->storageClass & STClazy)
    {
        TypeFunction *tf = new TypeFunction(NULL, arg->type, 0, LINKd);
        TypeDelegate *t = new TypeDelegate(tf);
        return t->merge()->toCtype();
    }
    return arg_type;
}

tree
IRState::arrayType(tree type_node, uinteger_t size)
{
    tree index_type_node;
    if (size > 0)
    {
        index_type_node = size_int(size - 1);
        index_type_node = build_index_type(index_type_node);
    }
    else
    {   // See c-decl.c grokdeclarator for zero-length arrays
        index_type_node = build_range_type (sizetype, size_zero_node,
                NULL_TREE);
    }

    tree array_type = build_array_type(type_node, index_type_node);
    if (size == 0)
    {
        TYPE_SIZE(array_type) = bitsize_zero_node;
        TYPE_SIZE_UNIT(array_type) = size_zero_node;
    }
    return array_type;
}

tree
IRState::addTypeAttribute(tree type, const char * attrname, tree value)
{
    // use build_variant_type_copy / build_type_attribute_variant

    // types built by functions in tree.c need to be treated as immutable
    if (! TYPE_ATTRIBUTES(type))
    {   // ! TYPE_ATTRIBUTES -- need a better check
        type = build_variant_type_copy(type);
        // TYPE_STUB_DECL(type) = .. if we need this for structs, etc.. since
        // TREE_CHAIN is cleared by COPY_NODE
    }
    if (value)
    {
        value = tree_cons(NULL_TREE, value, NULL_TREE);
    }
    TYPE_ATTRIBUTES(type) = tree_cons(get_identifier(attrname), value,
            TYPE_ATTRIBUTES(type));
    return type;
}

void
IRState::addDeclAttribute(tree type, const char * attrname, tree value)
{
    if (value)
    {
        value = tree_cons(NULL_TREE, value, NULL_TREE);
    }
    DECL_ATTRIBUTES(type) = tree_cons(get_identifier(attrname), value,
            DECL_ATTRIBUTES(type));
}

tree
IRState::attributes(Expressions * in_attrs)
{
    if (! in_attrs)
        return NULL_TREE;

    ListMaker out_attrs;

    for (size_t i = 0; i < in_attrs->dim; i++)
    {
        Expression * e = in_attrs->tdata()[i];
        IdentifierExp * ident_e = NULL;

        ListMaker args;

        if (e->op == TOKidentifier)
            ident_e = (IdentifierExp *) e;
        else if (e->op == TOKcall)
        {
            CallExp * c = (CallExp *) e;
            gcc_assert(c->e1->op == TOKidentifier);
            ident_e = (IdentifierExp *) c->e1;

            if (c->arguments)
            {
                for (size_t ai = 0; ai < c->arguments->dim; ai++)
                {
                    Expression * ae = c->arguments->tdata()[ai];
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
IRState::addTypeModifiers(tree type, unsigned mod)
{
    int quals = 0;
    gcc_assert(type);

    switch (mod)
    {
        case 0:
            break;

        case MODconst:
        case MODwild:
        case MODimmutable:
            quals |= TYPE_QUAL_CONST;
            break;

        case MODshared:
            quals |= TYPE_QUAL_VOLATILE;
            break;

        case MODshared | MODwild:
        case MODshared | MODconst:
            quals |= TYPE_QUAL_CONST;
            quals |= TYPE_QUAL_VOLATILE;
            break;

        default:
            gcc_unreachable();
    }

    return build_qualified_type(type, quals);
}

tree
IRState::integerConstant(dinteger_t value, tree type)
{
    // The type is error_mark_node, we can't do anything.
    if (isErrorMark(type))
        return type;

    tree tree_value = NULL_TREE;

#if HOST_BITS_PER_WIDE_INT == 32
    double_int cst = { value & 0xffffffff, (value >> 32) & 0xffffffff };
    tree_value = double_int_to_tree(type, cst);
#elif HOST_BITS_PER_WIDE_INT == 64
    tree_value = build_int_cst_type(type, value);
#else
#  error Fix This
#endif

#if D_GCC_VER < 43
    /* VALUE may be an incorrect representation for TYPE.  Example:
       uint x = cast(uint) -3; // becomes "-3u" -- value=0xfffffffffffffd type=Tuns32
       Constant folding will not work correctly unless this is done. */
    tree_value = force_fit_type(tree_value, 0, 0, 0);
#endif

    return tree_value;
}

tree
IRState::exceptionObject()
{
    tree obj_type = getObjectType()->toCtype();
    // Like gjc, the actual D exception object is one
    // pointer behind the exception header
#if D_GCC_VER >= 45
    tree t = buildCall(built_in_decls[BUILT_IN_EH_POINTER],
                       1, integer_zero_node);
#else
    tree t = build0(EXC_PTR_EXPR, ptr_type_node);
#endif
    t = build1(NOP_EXPR, build_pointer_type(obj_type), t); // treat exception header as (Object*)
    t = pointerOffsetOp(MINUS_EXPR, t, TYPE_SIZE_UNIT(TREE_TYPE(t)));
    t = build1(INDIRECT_REF, obj_type, t);
    return t;
}

tree
IRState::label(Loc loc, Identifier * ident)
{
    tree t_label = d_build_decl(LABEL_DECL,
            ident ? get_identifier(ident->string) : NULL_TREE, void_type_node);
    DECL_CONTEXT(t_label) = current_function_decl;
    DECL_MODE(t_label) = VOIDmode;
    if (loc.filename)
        g.ofile->setDeclLoc(t_label, loc);
    return t_label;
}

tree
IRState::getFrameForFunction(FuncDeclaration * f)
{
    if (f->fbody)
    {
        return getFrameForSymbol(f);
    }
    else
    {   // Should error on line that references f
        f->error("nested function missing body");
        return d_null_pointer;
    }
}

tree
IRState::getFrameForNestedClass(ClassDeclaration *c)
{
    return getFrameForSymbol(c);
}

#if V2
tree
IRState::getFrameForNestedStruct(StructDeclaration *s)
{
    return getFrameForSymbol(s);
}
#endif

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

    if ((nested_func = nested_sym->isFuncDeclaration()))
    {
        // gcc_assert(nested_func->isNested())
        outer_func = nested_func->toParent2()->isFuncDeclaration();
        gcc_assert(outer_func != NULL);

        if (func != outer_func)
        {
            Dsymbol * this_func = func;
            if (!func->vthis) // if no frame pointer for this function
            {
                nested_sym->error("is a nested function and cannot be accessed from %s", func->toChars());
                return d_null_pointer;
            }
            /* Make sure we can get the frame pointer to the outer function,
               else we'll ICE later in tree-ssa.  */
            while (nested_func != this_func)
            {
                FuncDeclaration * fd;
                ClassDeclaration * cd;
                StructDeclaration * sd;

                if ((fd = this_func->isFuncDeclaration()))
                {
                    if (outer_func == fd->toParent2())
                        break;
                    gcc_assert(fd->isNested() || fd->vthis);
                }
                else if ((cd = this_func->isClassDeclaration()))
                {
                    if (!cd->isNested() || !cd->vthis)
                        goto cannot_get_frame;
                    if (outer_func == cd->toParent2())
                        break;
                }
#if V2
                else if ((sd = this_func->isStructDeclaration()))
                {
                    if (!sd->isNested() || !sd->vthis)
                        goto cannot_get_frame;
                    if (outer_func == sd->toParent2())
                        break;
                }
#endif
                else
                {
                  cannot_get_frame:
                    func->error("cannot get frame pointer to %s", nested_sym->toChars());
                    return d_null_pointer;
                }
                this_func = this_func->toParent2();
            }
        }
    }
    else
    {
        /* It's a class (or struct).  NewExp::toElem has already determined its
           outer scope is not another class, so it must be a function. */

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

        if (outer_func != func)
        {
            Dsymbol * o = nested_func = func;
            do {
                if (! nested_func->isNested())
                {
#if V2
                    if (! nested_func->isMember())
#endif
                    goto cannot_access_frame;
                }
                while ((o = o->toParent2()))
                {
                    if ((nested_func = o->isFuncDeclaration()))
                        break;
                }
            } while (o && o != outer_func);

            if (! o)
            {
              cannot_access_frame:
                error("cannot access frame of function '%s' from '%s'",
                      outer_func->toChars(), func->toChars());
                return d_null_pointer;
            }
        }
        // else, the answer is 'virtual_stack_vars_rtx'
    }

    if (! outer_func)
        outer_func = nested_func->toParent2()->isFuncDeclaration();
    gcc_assert(outer_func != NULL);

    if (getFrameInfo(outer_func)->creates_frame)
        return getFrameRef(outer_func);

    return d_null_pointer;
}

bool
IRState::isClassNestedIn(ClassDeclaration *inner, ClassDeclaration *outer)
{
    // not implemented yet
    return false;
}

/* For the purposes this is used, inner is assumed to be a nested
   function or a method of a class or struct that is (eventually) nested in a
   function.
*/
bool
IRState::isFuncNestedIn(FuncDeclaration * inner, FuncDeclaration * outer)
{
    if (inner == outer)
        return false;

    while (inner)
    {
        AggregateDeclaration * ad;
        ClassDeclaration * cd;
        StructDeclaration * sd;

        if (inner == outer)
        {   //fprintf(stderr, "%s is nested in %s\n", inner->toChars(), outer->toChars());
            return true;
        }
        else if (inner->isNested())
        {
            inner = inner->toParent2()->isFuncDeclaration();
        }
        else if ((ad = inner->isThis()) && (cd = ad->isClassDeclaration()))
        {
            inner = isClassNestedInFunction(cd);
        }
#if V2
        else if ((ad = inner->isThis()) && (sd = ad->isStructDeclaration()))
        {
            inner = isStructNestedInFunction(sd);
        }
#endif
        else
        {
            break;
        }
    }
    //fprintf(stderr, "%s is NOT nested in %s\n", inner->toChars(), outer->toChars());
    return false;
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

    while (fd)
    {
        AggregateDeclaration * fd_ad;
        ClassDeclaration * fd_cd;

        if ((fd_ad = fd->isThis()) &&
            (fd_cd = fd_ad->isClassDeclaration()))
        {
            if (target_cd == fd_cd)
            {
                return irs->var(fd->vthis);
            }
            else if (target_cd->isBaseOf(fd_cd, NULL))
            {
                gcc_assert(fd->vthis); // && fd->vthis->csym->Stree
                return irs->convertTo(irs->var(fd->vthis),
                        fd_cd->type, target_cd->type);
            }
            else if (irs->isClassNestedIn(fd_cd, target_cd))
            {
                gcc_unreachable(); // not implemented
            }
            else
            {
                fd = irs->isClassNestedInFunction(fd_cd);
            }
        }
        else if (fd->isNested())
        {
            fd = fd->toParent2()->isFuncDeclaration();
        }
        else
        {
            fd = NULL;
        }
    }
    return NULL_TREE;
}

/* Return the outer class/struct 'this' value.
   This is here mostly due to removing duplicate code,
   and clean implementation purposes.  */
tree
IRState::getVThis(Dsymbol * decl, Expression * e)
{
    tree vthis_value = NULL_TREE;

    ClassDeclaration * class_decl;
    StructDeclaration * struct_decl;

    if ((class_decl = decl->isClassDeclaration()))
    {
        Dsymbol * outer = class_decl->toParent2();
        ClassDeclaration * cd_outer = outer->isClassDeclaration();
        FuncDeclaration * fd_outer = outer->isFuncDeclaration();

        if (cd_outer)
        {
            vthis_value = findThis(this, cd_outer);
            if (vthis_value == NULL_TREE)
            {
                e->error("outer class %s 'this' needed to 'new' nested class %s",
                        cd_outer->toChars(), class_decl->toChars());
            }
        }
        else if (fd_outer)
        {   /* If a class nested in a function has no methods
               and there are no other nested functions,
               lower_nested_functions is never called and any
               STATIC_CHAIN_EXPR created here will never be
               translated. Use a null pointer for the link in
               this case. */
            if (getFrameInfo(fd_outer)->creates_frame ||
#if V2
                fd_outer->closureVars.dim
#else
                fd_outer->nestedFrameRef
#endif
               )
            {   // %% V2: rec_type->class_type
                vthis_value = getFrameForNestedClass(class_decl);
            }
            else if (fd_outer->vthis)
                vthis_value = var(fd_outer->vthis);
            else
                vthis_value = d_null_pointer;
        }
        else
            gcc_unreachable();
    }
#if V2
    else if ((struct_decl = decl->isStructDeclaration()))
    {
        Dsymbol *outer = struct_decl->toParent2();
        FuncDeclaration *fd_outer = outer->isFuncDeclaration();
        // Assuming this is kept as trivial as possible.
        // NOTE: what about structs nested in structs nested in functions?
        gcc_assert(fd_outer);
        if (fd_outer->closureVars.dim ||
            getFrameInfo(fd_outer)->creates_frame)
        {
            vthis_value = getFrameForNestedStruct(struct_decl);
        }
        else if (fd_outer->vthis)
            vthis_value = var(fd_outer->vthis);
        else
            vthis_value = d_null_pointer;
    }
#endif
    return vthis_value;
}


/* Return the parent function of a nested class. */
FuncDeclaration *
IRState::isClassNestedInFunction(ClassDeclaration * cd)
{
    FuncDeclaration * fd = NULL;
    while (cd && cd->isNested())
    {
        Dsymbol * dsym = cd->toParent2();
        if ((fd = dsym->isFuncDeclaration()))
            return fd;
        else
            cd = dsym->isClassDeclaration();
    }
    return NULL;
}

#if V2
/* Return the parent function of a nested struct. */
FuncDeclaration *
IRState::isStructNestedInFunction(StructDeclaration * sd)
{
    FuncDeclaration * fd = NULL;
    while (sd && sd->isNested())
    {
        Dsymbol * dsym = sd->toParent2();
        if ((fd = dsym->isFuncDeclaration()))
            return fd;
        else
            sd = dsym->isStructDeclaration();
    }
    return NULL;
}
#endif


FuncFrameInfo *
IRState::getFrameInfo(FuncDeclaration *fd)
{
    Symbol * fds = fd->toSymbol();
    if (fds->frameInfo)
        return fds->frameInfo;

    FuncFrameInfo * ffi = new FuncFrameInfo;
    ffi->creates_closure = false;
    ffi->frame_rec = NULL_TREE;

    fds->frameInfo = ffi;

#if V2
    VarDeclarations * nestedVars = & fd->closureVars;
#else
    VarDeclarations * nestedVars = & fd->frameVars;
#endif

    // Nested functions, or functions with nested refs must create
    // a static frame for local variables to be referenced from.
    ffi->creates_frame = nestedVars->dim != 0 || fd->isNested();

#if V2
    // D2 maybe setup closure instead.
    if (fd->needsClosure())
    {
        ffi->creates_frame = true;
        ffi->creates_closure = true;
    }
    else
    {   /* If fd is nested (deeply) in a function 'g' that creates a
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
            StructDeclaration * sd;
            FuncFrameInfo * ffo = getFrameInfo(ff);

            if (ff != fd && ffo->creates_closure)
            {
                for (size_t i = 0; i < ff->closureVars.dim; i++)
                {
                    VarDeclaration * v = ff->closureVars.tdata()[i];
                    gcc_assert(v->isVarDeclaration());
                    for (size_t j = 0; j < v->nestedrefs.dim; j++)
                    {
                        FuncDeclaration * fi = v->nestedrefs.tdata()[j];
                        if (isFuncNestedIn(fi, fd))
                        {
                            ffi->creates_frame = true;
                            ffi->creates_closure = true;
                            goto L_done;
                        }
                    }
                }
            }

            if (ff->isNested())
                ff = ff->toParent2()->isFuncDeclaration();
            else if ((ad = ff->isThis()) && (cd = ad->isClassDeclaration()))
                ff = isClassNestedInFunction(cd);
            else if ((ad = ff->isThis()) && (sd = ad->isStructDeclaration()))
                ff = isStructNestedInFunction(sd);
            else
                break;
        }
        L_done: ;
    }
#endif

    return ffi;
}

// Return a pointer to the frame/closure block of outer_func
tree
IRState::getFrameRef(FuncDeclaration * outer_func)
{
    tree result = chainLink();
    FuncDeclaration * fd = chainFunc();

    while (fd && fd != outer_func)
    {
        AggregateDeclaration * ad;
        ClassDeclaration * cd;
        StructDeclaration * sd;

        gcc_assert(getFrameInfo(fd)->creates_frame);

        // like compon(indirect, field0) parent frame/closure link is the first field;
        result = indirect(result, ptr_type_node);

        if (fd->isNested())
            fd = fd->toParent2()->isFuncDeclaration();
        /* getFrameRef is only used to get the pointer to a function's frame
           (not a class instances.)  With the current implementation, the link
           the frame/closure record always points to the outer function's frame even
           if there are intervening nested classes or structs.
           So, we can just skip over those... */
        else if ((ad = fd->isThis()) && (cd = ad->isClassDeclaration()))
            fd = isClassNestedInFunction(cd);
#if V2
        else if ((ad = fd->isThis()) && (sd = ad->isStructDeclaration()))
            fd = isStructNestedInFunction(sd);
#endif
        else
            break;
    }

    if (fd == outer_func)
    {
        tree frame_rec = getFrameInfo(outer_func)->frame_rec;

        if (frame_rec != NULL_TREE)
        {
            result = nop(result, build_pointer_type(frame_rec));
            return result;
        }
        else
        {
            func->error("forward reference to frame of %s", outer_func->toChars());
            return d_null_pointer;
        }
    }
    else
    {
        func->error("cannot access frame of %s", outer_func->toChars());
        return d_null_pointer;
    }
}


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
        && (pf = f->toParent2()->isFuncDeclaration())
            && ! getFrameInfo(pf)->creates_closure
#endif
       )
    {
        return true;
    }
    if (f->isStatic())
    {
        return false;
    }

    s = f->toParent2();

    while (s && (a = s->isClassDeclaration()) && a->isNested())
    {
        s = s->toParent2();
        if ((pf = s->isFuncDeclaration())
#if V2
            && ! getFrameInfo(pf)->creates_closure
#endif
           )
        {
            return true;
        }
    }

    return false;
}


// Build static chain decl to be passed to nested functions in D.
void
IRState::buildChain(FuncDeclaration * func)
{
    FuncFrameInfo * ffi = getFrameInfo(func);

#if V2
    if (ffi->creates_closure)
    {   // Build closure pointer, which is initialised on heap.
        func->buildClosure(this);
        return;
    }
#endif

    if (! ffi->creates_frame)
        return;

#if V2
    VarDeclarations * nestedVars = & func->closureVars;
#else
    VarDeclarations * nestedVars = & func->frameVars;
#endif

    char *name = concat ("FRAME.",
                         IDENTIFIER_POINTER (DECL_NAME (func->toSymbol()->Stree)),
                         NULL);
    tree frame_rec_type = make_node(RECORD_TYPE);
    TYPE_NAME (frame_rec_type) = get_identifier (name);
    free (name);

    tree chain_link = chainLink();
    tree ptr_field;
    ListMaker fields;

    ptr_field = d_build_decl_loc(BUILTINS_LOCATION, FIELD_DECL,
                                 get_identifier("__chain"), ptr_type_node);
    DECL_CONTEXT(ptr_field) = frame_rec_type;
    fields.chain(ptr_field);

    for (size_t i = 0; i < nestedVars->dim; ++i)
    {
        VarDeclaration *v = nestedVars->tdata()[i];
        Symbol * s = v->toSymbol();
        tree field = d_build_decl(FIELD_DECL,
                                  v->ident ? get_identifier(v->ident->string) : NULL_TREE,
                                  gen.trueDeclarationType(v));
        s->SframeField = field;
        g.ofile->setDeclLoc(field, v);
        DECL_CONTEXT(field) = frame_rec_type;
        fields.chain(field);
        TREE_USED(s->Stree) = 1;
    }

    TYPE_FIELDS(frame_rec_type) = fields.head;
    layout_type(frame_rec_type);
    ffi->frame_rec = frame_rec_type;

    tree frame_decl = localVar(frame_rec_type);
    tree frame_ptr = addressOf(frame_decl);
    DECL_IGNORED_P(frame_decl) = 0;
    expandDecl(frame_decl);

    // set the first entry to the parent frame, if any
    if (chain_link != NULL_TREE)
    {
        doExp(vmodify(component(indirect(frame_ptr), ptr_field),
                      chain_link));
    }

    // copy parameters that are referenced nonlocally
    for (size_t i = 0; i < nestedVars->dim; i++)
    {
        VarDeclaration * v = nestedVars->tdata()[i];
        if (! v->isParameter())
            continue;

        Symbol * vsym = v->toSymbol();
        doExp(vmodify(component(indirect(frame_ptr), vsym->SframeField),
                      vsym->Stree));
    }

    useChain(func, frame_ptr); 
}

tree
IRState::toElemLvalue(Expression * e)
{
    /*
    if (e->op == TOKcast)
        fprintf(stderr, "IRState::toElemLvalue TOKcast\n");
    else
    */
    if (e->op == TOKindex)
    {
        IndexExp * ie = (IndexExp *) e;
        Expression * e1 = ie->e1;
        Expression * e2 = ie->e2;
        Type * type = e->type;
        Type * array_type = e1->type->toBasetype();

        if (array_type->ty == Taarray)
        {
            Type * key_type = ((TypeAArray *) array_type)->index->toBasetype();
            AddrOfExpr aoe;

            tree args[4];
            args[0] = this->addressOf(this->toElemLvalue(e1));
            args[1] = this->typeinfoReference(key_type);
            args[2] = this->integerConstant(array_type->nextOf()->size(), Type::tsize_t);
            args[3] = aoe.set(this, this->convertTo(e2, key_type));
            return build1(INDIRECT_REF, type->toCtype(),
                aoe.finish(this,
                    this->libCall(LIBCALL_AAGETP, 4, args, type->pointerTo()->toCtype())));
        }
    }
    return e->toElem(this);
}


tree
IRState::addressOf(tree exp)
{
    tree t, ptrtype;
    tree exp_type = TREE_TYPE(exp);
    d_mark_addressable(exp);

    // Gimplify doesn't like &(*(ptr-to-array-type)) with static arrays
    if (TREE_CODE(exp) == INDIRECT_REF)
    {
        t = TREE_OPERAND(exp, 0);
        ptrtype = build_pointer_type(exp_type);
        t = nop(t, ptrtype);
    }
    else
    {   /* Just convert string literals (char[]) to C-style strings (char *), otherwise
           the latter method (char[]*) causes conversion problems during gimplification. */
        if (TREE_CODE (exp) == STRING_CST)
        {
            ptrtype = build_pointer_type(TREE_TYPE(exp_type));
        }
        /* Special case for va_list. The backends will be expecting a pointer to vatype,
         * but some targets use an array. So fix it.  */
        else if (TYPE_MAIN_VARIANT(exp_type) == TYPE_MAIN_VARIANT(va_list_type_node))
        {
            if (TREE_CODE(TYPE_MAIN_VARIANT(exp_type)) == ARRAY_TYPE)
                ptrtype = build_pointer_type(TREE_TYPE(exp_type));
            else
                ptrtype = build_pointer_type(exp_type);
        }
        else
            ptrtype = build_pointer_type(exp_type);

        t = build1(ADDR_EXPR, ptrtype, exp);
    }
    if (TREE_CODE(exp) == FUNCTION_DECL)
        TREE_NO_TRAMPOLINE(t) = 1;

    return t;
}


void
IRState::startCond(Statement * stmt, tree t_cond)
{
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
IRState::startLoop(Statement * stmt)
{
    Flow * f = beginFlow(stmt);
    f->continueLabel = label(stmt ? stmt->loc : 0); // should be end for 'do' loop
}

void
IRState::continueHere()
{
    doLabel(currentFlow()->continueLabel);
}

void
IRState::setContinueLabel(tree lbl)
{
    currentFlow()->continueLabel = lbl;
}

void
IRState::exitIfFalse(tree t_cond, bool /*unused*/)
{
    addExp(build1(EXIT_EXPR, void_type_node,
               build1(TRUTH_NOT_EXPR, TREE_TYPE(t_cond), t_cond)));
}

void
IRState::startCase(Statement * stmt, tree t_cond, int has_vars)
{
    Flow * f = beginFlow(stmt);
    f->condition = t_cond;
    f->kind = level_switch;
#if V2
    if (has_vars)
    {   // %% dummy value so the tree is not NULL
        f->hasVars = integer_one_node;
    }
#endif
}

void
IRState::doCase(tree t_value, tree t_label)
{
#if V2
    if (currentFlow()->hasVars)
    {   // SwitchStatement has already taken care of label jumps.
        doLabel(t_label);
    }
    else
#endif
    {
        addExp(build3(CASE_LABEL_EXPR, void_type_node,
                    t_value, NULL_TREE, t_label));
    }
}

void
IRState::endCase(tree /*t_cond*/)
{
    Flow * f = currentFlow();
    tree t_body = popStatementList();
#if V2
    if (f->hasVars)
    {   // %% switch was converted to if-then-else expression
        addExp(t_body);
    }
    else
#endif
    {
        tree t_stmt = build3(SWITCH_EXPR, void_type_node, f->condition,
                t_body, NULL_TREE);
        addExp(t_stmt);
    }
    endFlow();
}

void
IRState::endLoop()
{
    // says must contain an EXIT_EXPR -- what about while(1)..goto;? something other thand LOOP_EXPR?
    tree t_body = popStatementList();
    tree t_loop = build1(LOOP_EXPR, void_type_node, t_body);
    addExp(t_loop);
    endFlow();
}

void
IRState::continueLoop(Identifier * ident)
{
    //doFlowLabel(stmt, ident, Continue);
    doJump(NULL, getLoopForLabel(ident, true)->continueLabel);
}

void
IRState::exitLoop(Identifier * ident)
{
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
    TREE_USED(t_label) = 1;
}

void
IRState::doAsm(tree insn_tmpl, tree outputs, tree inputs, tree clobbers)
{
    tree t = d_build_asm_stmt(insn_tmpl, outputs, inputs, clobbers, NULL_TREE);
    ASM_VOLATILE_P(t) = 1;
    addExp(t);
}


void
IRState::checkSwitchCase(Statement * stmt, int default_flag)
{
    Flow * flow = currentFlow();

    gcc_assert(flow);
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

    for (size_t i = 0; i < labels.dim; i++)
    {
        Label * linfo = labels.tdata()[i];
        gcc_assert(linfo);

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
    // Push forward referenced gotos.
    if (! found)
    {
        if (! label->statement->fwdrefs)
            label->statement->fwdrefs = new Array();
        label->statement->fwdrefs->push(getLabelBlock(label, stmt));
    }
}

void
IRState::checkPreviousGoto(Array * refs)
{
    Statement * stmt; // Our forward reference.

    for (size_t i = 0; i < refs->dim; i++)
    {
        Label * ref = (Label *) refs->data[i];
        int found = 0;

        gcc_assert(ref && ref->from);
        stmt = ref->from;

        for (size_t i = 0; i < labels.dim; i++)
        {
            Label * linfo = labels.tdata()[i];
            gcc_assert(linfo);

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
        gcc_assert(found);
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


void
FieldVisitor::visit(AggregateDeclaration * decl)
{
    ClassDeclaration * class_decl = decl->isClassDeclaration();

    if (class_decl && class_decl->baseClass)
        FieldVisitor::visit(class_decl->baseClass);

    if (decl->fields.dim)
        doFields(& decl->fields, decl);

    if (class_decl && class_decl->vtblInterfaces)
        doInterfaces(class_decl->vtblInterfaces, decl);
}


void
AggLayout::doFields(VarDeclarations * fields, AggregateDeclaration * agg)
{
    bool inherited = agg != this->aggDecl;
    tree fcontext;

    fcontext = agg->type->toCtype();
    if (POINTER_TYPE_P(fcontext))
        fcontext = TREE_TYPE(fcontext);

    // tree new_field_chain = NULL_TREE;
    for (size_t i = 0; i < fields->dim; i++)
    {   // %% D anonymous unions just put the fields into the outer struct...
        // does this cause problems?
        VarDeclaration * var_decl = fields->tdata()[i];
        gcc_assert(var_decl && var_decl->storage_class & STCfield);

        tree ident = var_decl->ident ? get_identifier(var_decl->ident->string) : NULL_TREE;
        tree field_decl = d_build_decl(FIELD_DECL, ident,
            gen.trueDeclarationType(var_decl));
        g.ofile->setDeclLoc(field_decl, var_decl);
        var_decl->csym = new Symbol;
        var_decl->csym->Stree = field_decl;

        DECL_CONTEXT(field_decl) = aggType;
        DECL_FCONTEXT(field_decl) = fcontext;
        DECL_FIELD_OFFSET (field_decl) = size_int(var_decl->offset);
        DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;

        DECL_ARTIFICIAL(field_decl) = DECL_IGNORED_P(field_decl) = inherited;

        // GCC 4.0 requires DECL_OFFSET_ALIGN to be set
        // %% .. using TYPE_ALIGN may not be same as DMD..
        SET_DECL_OFFSET_ALIGN(field_decl, TYPE_ALIGN(TREE_TYPE(field_decl)));

        //SET_DECL_OFFSET_ALIGN (field_decl, BIGGEST_ALIGNMENT); // ?
        layout_decl(field_decl, 0);

        // get_inner_reference doesn't check these, leaves a variable unitialized
        // DECL_SIZE is NULL if size is zero.
        if (var_decl->size(var_decl->loc))
        {
            gcc_assert(DECL_MODE(field_decl) != VOIDmode);
            gcc_assert(DECL_SIZE(field_decl) != NULL_TREE);
        }
        fieldList.chain(field_decl);
    }
}

void
AggLayout::doInterfaces(BaseClasses * bases, AggregateDeclaration * /*agg*/)
{
    //tree fcontext = TREE_TYPE(agg->type->toCtype());
    for (size_t i = 0; i < bases->dim; i++)
    {
        BaseClass * bc = bases->tdata()[i];
        tree decl = d_build_decl(FIELD_DECL, NULL_TREE,
            Type::tvoidptr->pointerTo()->toCtype() /* %% better */);
        //DECL_VIRTUAL_P(decl) = 1; %% nobody cares, boo hoo
        DECL_ARTIFICIAL(decl) = DECL_IGNORED_P(decl) = 1;
        // DECL_FCONTEXT(decl) = fcontext; // shouldn't be needed since it's ignored
        addField(decl, bc->offset);
    }
}

void
AggLayout::addField(tree field_decl, size_t offset)
{
    DECL_CONTEXT(field_decl) = aggType;
    // DECL_FCONTEXT(field_decl) = aggType; // caller needs to set this
    SET_DECL_OFFSET_ALIGN(field_decl, TYPE_ALIGN(TREE_TYPE(field_decl)));
    DECL_FIELD_OFFSET (field_decl) = size_int(offset);
    DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;
    Loc l(aggDecl->getModule(), 1); // Must set this or we crash with DWARF debugging
    // gen.setDeclLoc(field_decl, aggDecl->loc); // aggDecl->loc isn't set
    g.ofile->setDeclLoc(field_decl, l);

    layout_decl(field_decl, 0);
    fieldList.chain(field_decl);
}

void
AggLayout::finish(Expressions * attrs)
{
    unsigned size_to_use = aggDecl->structsize;
    unsigned align_to_use = aggDecl->alignsize;

    /* probably doesn't do anything */
    /*
    if (aggDecl->structsize == 0 && aggDecl->isInterfaceDeclaration())
        size_to_use = Type::tvoid->pointerTo()->size();
    */

    TYPE_SIZE(aggType) = bitsize_int(size_to_use * BITS_PER_UNIT);
    TYPE_SIZE_UNIT(aggType) = size_int(size_to_use);
    TYPE_ALIGN(aggType) = align_to_use * BITS_PER_UNIT;
    // TYPE_ALIGN_UNIT is not an lvalue
    TYPE_PACKED (aggType) = TYPE_PACKED (aggType); // %% todo

    if (attrs)
    {
        decl_attributes(& aggType, gen.attributes(attrs),
            ATTR_FLAG_TYPE_IN_PLACE);
    }

    compute_record_mode (aggType);
    // %%  stor-layout.c:finalize_type_size ... it's private to that file
    // c-decl.c -- set up variants ? %%
    for (tree x = TYPE_MAIN_VARIANT(aggType); x; x = TYPE_NEXT_VARIANT(x))
    {
        TYPE_FIELDS (x) = TYPE_FIELDS (aggType);
        TYPE_LANG_SPECIFIC (x) = TYPE_LANG_SPECIFIC (aggType);
        TYPE_ALIGN (x) = TYPE_ALIGN (aggType);
        TYPE_USER_ALIGN (x) = TYPE_USER_ALIGN (aggType);
    }
}


ArrayScope::ArrayScope(IRState * ini_irs, VarDeclaration * ini_v, const Loc & loc) :
    v(ini_v), irs(ini_irs)
{
    if (v)
    {   /* Need to set the location or the expand_decl in the BIND_EXPR will
           cause the line numbering for the statement to be incorrect. */
        /* The variable itself is not included in the debugging information. */
        v->loc = loc;
        Symbol * s = v->toSymbol();
        tree decl = s->Stree;
        DECL_CONTEXT(decl) = irs->getLocalContext();
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
            if (s->SframeField)
            {
                return irs->compound(irs->vmodify(irs->var(v),
                            DECL_INITIAL(t)), e);
            }
            else
            {
                return gen.binding(v->toSymbol()->Stree, e);
            }
        }
    }
    return e;
}

