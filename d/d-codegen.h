/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrot, (C) 2009, 2010
    Iain Buclaw, (C) 2011

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

#ifndef GCC_DCMPLR_CODEGEN_H
#define GCC_DCMPLR_CODEGEN_H

#include "d-irstate.h"
#include "d-objfile.h"

enum LibCall
{
    LIBCALL_ASSERT,
    LIBCALL_ASSERT_MSG,
    LIBCALL_ARRAY_BOUNDS,
    LIBCALL_SWITCH_ERROR,
    LIBCALL_INVARIANT,
    LIBCALL_NEWCLASS,
    LIBCALL_NEWARRAYT,
    LIBCALL_NEWARRAYIT,
    //LIBCALL_NEWARRAYMT,
    LIBCALL_NEWARRAYMTP,
    //LIBCALL_NEWARRAYMIT,
    LIBCALL_NEWARRAYMITP,
#if V2
    LIBCALL_ALLOCMEMORY,
#endif
    LIBCALL_DELCLASS,
    LIBCALL_DELINTERFACE,
    LIBCALL_DELARRAY,
    LIBCALL_DELMEMORY,
    LIBCALL_CALLFINALIZER,
    LIBCALL_CALLINTERFACEFINALIZER,
    LIBCALL_ARRAYSETLENGTHT,
    LIBCALL_ARRAYSETLENGTHIT,
    LIBCALL_DYNAMIC_CAST,
    LIBCALL_INTERFACE_CAST,
    LIBCALL_ADEQ,
    LIBCALL_ADEQ2,
    LIBCALL_ADCMP,
    LIBCALL_ADCMP2,
    LIBCALL_ADCMPCHAR,
    LIBCALL_AAEQUAL,
    LIBCALL_AALEN,
    /*LIBCALL_AAIN,
    LIBCALL_AAGET,
    LIBCALL_AAGETRVALUE,
    LIBCALL_AADEL,*/
    LIBCALL_AAINP,
    LIBCALL_AAGETP,
    LIBCALL_AAGETRVALUEP,
    LIBCALL_AADELP,
    LIBCALL_ARRAYCAST,
    LIBCALL_ARRAYCOPY,
    LIBCALL_ARRAYCATT,
    LIBCALL_ARRAYCATNT,
    LIBCALL_ARRAYAPPENDT,
    //LIBCALL_ARRAYAPPENDCT,
    LIBCALL_ARRAYAPPENDCTP,
    LIBCALL_ARRAYAPPENDCD,
    LIBCALL_ARRAYAPPENDWD,
#if V2
    LIBCALL_ARRAYASSIGN,
    LIBCALL_ARRAYCTOR,
    LIBCALL_ARRAYSETASSIGN,
    LIBCALL_ARRAYSETCTOR,
    LIBCALL_DELARRAYT,
#endif
    LIBCALL_MONITORENTER,
    LIBCALL_MONITOREXIT,
    LIBCALL_CRITICALENTER,
    LIBCALL_CRITICALEXIT,
    LIBCALL_THROW,
    LIBCALL_SWITCH_STRING,
    LIBCALL_SWITCH_USTRING,
    LIBCALL_SWITCH_DSTRING,
    LIBCALL_ASSOCARRAYLITERALTP,
    LIBCALL_ARRAYLITERALTP,
#if V2
    LIBCALL_UNITTEST,
    LIBCALL_UNITTEST_MSG,
    LIBCALL_HIDDEN_FUNC,
#endif
    LIBCALL_count
};

enum BinOp
{
    opComp,
    opBinary,
    opAssign,
};

struct FuncFrameInfo
{
    bool creates_closure;
    bool creates_frame;
    union
    {
        tree closure_rec;
        tree frame_rec;
    };
};

class ArrayScope;

// Code generation routines should be in a separate namespace, but so many
// routines need a reference to an IRState to expand Expressions.  Solution
// is to make IRState the code generation namespace with the actual IR state
// routines as a mixin.  There is still the global 'gen' which can be used when
// it is known the full function IR state is not needed. Also a lot of routines
// are static (don't need IR state or expand Expressions), but are still called
// using . or ->.

// Functions without a verb create trees
// Functions with 'do' affect the current instruction stream (or output assembler code.)
// functions with other names are for attribute manipulate, etc.

struct IRState : IRBase
{
    // %% intrinsic and math functions in alphabetical order for bsearch
    enum Intrinsic
    {
        INTRINSIC_BSF, INTRINSIC_BSR,
        INTRINSIC_BSWAP,
        INTRINSIC_BT, INTRINSIC_BTC, INTRINSIC_BTR, INTRINSIC_BTS,
        INTRINSIC_INP, INTRINSIC_INPL, INTRINSIC_INPW,
        INTRINSIC_OUTP, INTRINSIC_OUTPL, INTRINSIC_OUTPW,

        INTRINSIC_COS, INTRINSIC_FABS,
        INTRINSIC_LDEXP, INTRINSIC_RINT,
        INTRINSIC_RNDTOL, INTRINSIC_SIN,
        INTRINSIC_SQRT,

        INTRINSIC_STD_VA_ARG,
        INTRINSIC_C_VA_ARG,
        INTRINSIC_C_VA_START,
        INTRINSIC_count,
    };

    static tree declContext(Dsymbol * d_sym);

    // ** Local variables
    void emitLocalVar(VarDeclaration * v, bool no_init = false);
    tree localVar(tree t_type);

    tree localVar(Type * e_type)
    {
        return localVar(e_type->toCtype());
    }

    tree exprVar(tree t_type);
    tree maybeExprVar(tree exp, tree * out_var);
    void expandDecl(tree t_decl);

    tree var(VarDeclaration * v);

    // ** Type conversion

    // 'convertTo' just to give it a different name from the extern "C" convert
    tree convertTo(Expression * exp, Type * target_type);
    tree convertTo(tree exp, Type * exp_type, Type * target_type);

    tree convertForAssignment(Expression * exp, Type * target_type);
    tree convertForAssignment(tree exp_tree, Type * exp_type, Type * target_type);
    tree convertForArgument(Expression * exp, Parameter * arg);

    tree convertForCondition(Expression * exp)
    {
        return convertForCondition(exp->toElem(this), exp->type);
    }

    tree convertForCondition(tree exp_tree, Type * exp_type);
    tree toDArray(Expression * exp);

    static bool typesSame(Type * t1, Type * t2)
    {
#if V2
        return t1->mutableOf()->equals(t2->mutableOf());
#else
        return t1->equals(t2);
#endif
    }

    static bool typesCompatible(Type * t1, Type * t2)
    {
#if V2
        return t1->implicitConvTo(t2) >= MATCHconst;
#else
        return t1->equals(t2);
#endif
    }


    // ** Type management
    static Type * getDType(tree t)
    {   // %% TODO: assert that its a type node..
        struct lang_type * l = TYPE_LANG_SPECIFIC(t);
        return l ? l->d_type : 0;
    }

    static Type * getObjectType()
    {
        if (ClassDeclaration::object)
            return ClassDeclaration::object->type;
        //error("missing or corrupt object.d");
        // %% apparently this isn't an error, so special-case in outdata.
        return Type::terror;
    }

    // Routines to handle variables that are references.
    static bool isDeclarationReferenceType(Declaration * decl);
    static tree trueDeclarationType(Declaration * decl);
    static bool isArgumentReferenceType(Parameter * arg);
    static tree trueArgumentType(Parameter * arg);

    static tree arrayType(Type * d_type, uinteger_t size) // %% use of dinteger_t
    {
        return arrayType(d_type->toCtype(), size);
    }

    static tree arrayType(tree type_node, uinteger_t size);

    // Can't call this until common types have been built
    static bool haveLongDouble()
    {
        return TYPE_MODE(long_double_type_node) != TYPE_MODE(double_type_node);
    }

    static tree addTypeAttribute(tree type, const char * attrname, tree value = NULL_TREE);
    static void addDeclAttribute(tree type, const char * attrname, tree value = NULL_TREE);
    static tree attributes(Expressions * in_attrs);
    static tree addTypeModifiers(tree type, unsigned mod);

    // ** Simple constants

    static tree nullPointer()
    {
        return d_null_pointer;
    }

    static tree integerConstant(dinteger_t value, tree type = integer_type_node);

    static tree integerConstant(dinteger_t value, Type * type)
    {
        return integerConstant(value, type->toCtype());
    }

    static tree floatConstant(const real_t & value, Type * target_type);

    static dinteger_t hwi2toli(HOST_WIDE_INT low, HOST_WIDE_INT high);

    static dinteger_t hwi2toli(double_int cst)
    {
        return hwi2toli(cst.low, cst.high);
    }

    // ** Routines for built in structured types

    static tree realPart(tree c);
    static tree imagPart(tree c);

    // ** Dynamic arrays
    static tree darrayLenRef(tree exp);
    static tree darrayPtrRef(tree exp);

    tree darrayPtrRef(Expression * e)
    {
        return darrayPtrRef(e->toElem(this));
    }

    tree darrayVal(tree type, tree len, tree data);
    // data may be NULL for a null pointer value
    tree darrayVal(tree type, uinteger_t len, tree data);

    tree darrayVal(Type * type, uinteger_t len, tree data)
    {
        return darrayVal(type->toCtype(), len, data);
    }

    tree darrayString(const char * str);

    static char * hostToTargetString(char * str, size_t length, unsigned unit_size);

    // Length of either a static or dynamic array
    tree arrayLength(Expression * exp)
    {
        return arrayLength(exp->toElem(this), exp->type);
    }

    static tree arrayLength(tree exp, Type * exp_type);

    // Delegates
    static tree delegateMethodRef(tree exp);
    static tree delegateObjectRef(tree exp);
    static tree delegateVal(tree method_exp, tree object_exp, Type * d_type);
    // These are for references to nested functions/methods as opposed to a variable of
    // type Tdelegate
    tree methodCallExpr(tree callee, tree object, Type * d_type)
    {
        tree t = delegateVal(callee, object, d_type);
        D_IS_METHOD_CALL_EXPR(t) = 1;
        return t;
    }

    void extractMethodCallExpr(tree mcr, tree & callee_out, tree & object_out);
    tree objectInstanceMethod(Expression * obj_exp, FuncDeclaration * func, Type * d_type);

    static tree twoFieldType(tree rec_type, tree ft1, tree ft2, Type * d_type = 0, const char * n1 = "_a", const char * n2 = "_b");
    static tree twoFieldType(Type * ft1, Type * ft2, Type * d_type = 0, const char * n1 = "_a", const char * n2 = "_b");
    static tree twoFieldCtor(tree rec_type, tree f1, tree f2, int storage_class = 0);

    // ** Temporaries (currently just SAVE_EXPRs)

    // Create a SAVE_EXPR if 't' might have unwanted side effects if referenced
    // more than once in an expression.
    tree maybeMakeTemp(tree t);

    // Return true if t can be evaluated multiple times (i.e., in a loop body)
    // without unwanted side effects.  This is a stronger test than
    // the one used for maybeMakeTemp.
    static bool isFreeOfSideEffects(tree t);

    // ** Various expressions
    tree toElemLvalue(Expression * e);

    static tree addressOf(Dsymbol *d)
    {
        return addressOf(d->toSymbol()->Stree);
    }

    static tree addressOf(tree exp);

    /* Cast exp (which should be a pointer) to TYPE* and then indirect.  The
       back-end requires this cast in many cases. */
    static tree indirect(tree exp, tree type)
    {
        return build1(INDIRECT_REF, type,
                nop(exp, build_pointer_type(type)));
    }

    static tree indirect(tree exp)
    {
        return indirect(exp, TREE_TYPE(TREE_TYPE(exp)));
    }

    static tree vmodify(tree dst, tree src)
    {
        return build2(MODIFY_EXPR, void_type_node, dst, src);
    }

    tree pointerIntSum(Expression * ptr_exp, Expression * idx_exp)
    {
        return pointerIntSum(ptr_exp->toElem(this), idx_exp->toElem(this));
    }

    tree pointerIntSum(tree ptr_node, tree idx_exp);
//  static tree pointIntOp(int op, tree ptr, tree idx);
    static tree pointerOffsetOp(int op, tree ptr, tree idx);
    static tree pointerOffset(tree ptr_node, tree byte_offset);

    static tree nop(tree e, tree t)
    {
        return build1(NOP_EXPR, t, e);
    }

    static tree vconvert(tree e, tree t)
    {
        return build1(VIEW_CONVERT_EXPR, t, e);
    }

    // DMD allows { void[] a; & a[3]; }
    static tree
    pvoidOkay(tree t)
    {
        if (VOID_TYPE_P(TREE_TYPE(TREE_TYPE(t))))
        {   // ::warning("indexing array of void");
            return convert(Type::tuns8->pointerTo()->toCtype(), t);
        }
        return t;
    }

    tree boolOp(enum tree_code code, tree a, tree b)
    {
        return build2(code, boolean_type_node, a, b);
    }

    tree checkedIndex(Loc loc, tree index, tree upper_bound, bool inclusive);
    tree boundsCond(tree index, tree upper_bound, bool inclusive);
    int arrayBoundsCheck();

    tree arrayElemRef(IndexExp * aer_exp, ArrayScope * aryscp);

    static tree binding(tree var_chain, tree body);

    static tree compound(tree a, tree b, tree type = 0)
    {
        return build2(COMPOUND_EXPR, type ? type : TREE_TYPE(b), a, b);
    }

    tree voidCompound(tree a, tree b)
    {
        return compound(a, b, void_type_node);
    }

    tree maybeCompound(tree a, tree b)
    {
        if (a == NULL_TREE)
            return b;
        else if (b == NULL_TREE)
            return a;
        else
            return build2(COMPOUND_EXPR, TREE_TYPE(b), a, b);
    }

    tree maybeVoidCompound(tree a, tree b)
    {
        if (a == NULL_TREE)
            return b;
        else if (b == NULL_TREE)
            return a;
        else
            return build2(COMPOUND_EXPR, void_type_node, a, b);
    }

    static tree component(tree v, tree f)
    {
        return build3(COMPONENT_REF, TREE_TYPE(f), v, f, NULL_TREE);
    }

    // Giving error_mark_node a type allows for some assumptions about
    // the type of an arbitrary expression.
    static tree errorMark(Type * t);
    static inline bool isErrorMark(tree t)
    {
        return t == error_mark_node
                || (t && TREE_TYPE(t) == error_mark_node)
                || (t && TREE_CODE(t) == NOP_EXPR &&
                    TREE_OPERAND(t, 0) == error_mark_node);
    }

    // ** Helpers for call
    static TypeFunction * getFuncType(Type * t);

    static inline bool isFuncType(tree t)
    {
        return (TREE_CODE(t) == FUNCTION_TYPE ||
                TREE_CODE(t) == METHOD_TYPE);
    }

    // ** Function calls
    tree call(Expression * expr, Expressions * arguments);
    tree call(FuncDeclaration * func_decl, Expressions * args);
    tree call(FuncDeclaration * func_decl, tree object, Expressions * args);
    tree call(TypeFunction *guess, tree callable, tree object, Expressions * arguments);

    tree assertCall(Loc loc, LibCall libcall = LIBCALL_ASSERT);
    tree assertCall(Loc loc, Expression * msg);
    static FuncDeclaration * getLibCallDecl(LibCall lib_call);
    static void replaceLibCallDecl(FuncDeclaration * d_decl);
    // This does not perform conversions on the arguments.  This allows
    // arbitrary data to be passed through varargs without going through the
    // usual conversions.
    static tree libCall(LibCall lib_call, unsigned n_args, tree *args, tree force_result_type = 0);

    // GCC 3.3 does not set TREE_SIDE_EFFECTS call by default. GCC 3.4
    // sets it depending on the const/pure attributes of the funcion
    // and the SIDE_EFFECTS flags of the arguments.
    static tree buildCall(tree type, tree callee, tree args)
    {
#if 0
        Type * t = getDType(TREE_TYPE(TREE_TYPE(callee)));
        if (t != NULL)
        {
            TypeFunction * tf = getFuncType(t);
            if (tf->linkage == LINKd && tf->varargs != 1)
                args = nreverse(args);
        }
#endif
#if D_GCC_VER >= 43
        int nargs = list_length(args);
        tree * pargs = new tree[nargs];
        for (int i = 0; args; args = TREE_CHAIN(args), i++)
            pargs[i] = TREE_VALUE(args);

        return build_call_array(type, callee, nargs, pargs);
#else
        return build3(CALL_EXPR, type, callee, args, NULL_TREE);
#endif
    }

    // Conveniently construct the function arguments for passing
    // to the real buildCall function.
    static tree buildCall(tree callee, int n_args, ...)
    {
        va_list ap;
        tree arg_list = NULL_TREE;
        tree fntype = TREE_TYPE(callee);

        va_start (ap, n_args);
        for (int i = n_args - 1; i >= 0; i--)
            arg_list = tree_cons(NULL_TREE, va_arg(ap, tree), arg_list);
        va_end (ap);

        return buildCall(TREE_TYPE(fntype), addressOf(callee), nreverse(arg_list));
    }

    tree assignValue(Expression * e, VarDeclaration * v);

    static TemplateEmission emitTemplates;
    static bool splitDynArrayVarArgs;
    static bool useInlineAsm;
    static bool useBuiltins;

    // Warnings
    static bool warnSignCompare;

    // %%
    static bool originalOmitFramePointer;

#if V2
    // Variables that are in scope that will need destruction later
    static VarDeclarations * varsInScope;
#endif

protected:
    tree maybeExpandSpecialCall(tree call_exp);
    static tree expandPortIntrinsic(Intrinsic code, tree port, tree value, int outp);
public:
    tree floatMod(tree a, tree b, tree type);

    tree typeinfoReference(Type * t);

    dinteger_t getTargetSizeConst(tree t);

    static Module * builtinsModule;
    static Module * intrinsicModule;
    static Module * intrinsicCoreModule;
    static Module * mathModule;
    static Module * mathCoreModule;
    static TemplateDeclaration * stdargTemplateDecl;
    static TemplateDeclaration * cstdargStartTemplateDecl;
    static TemplateDeclaration * cstdargArgTemplateDecl;

    static void setBuiltinsModule(Module * mod)
    {
        builtinsModule = mod;
    }

    static void setIntrinsicModule(Module * mod, bool coremod)
    {
        if (coremod)
            intrinsicCoreModule = mod;
        else
            intrinsicModule = mod;
    }

    static void setMathModule(Module * mod, bool coremod)
    {
        if (coremod)
            mathCoreModule = mod;
        else
            mathModule = mod;
    }

    static void setStdArg(TemplateDeclaration * td)
    {
        stdargTemplateDecl = td;
    }

    static void setCStdArgStart(TemplateDeclaration * td)
    {
        cstdargStartTemplateDecl = td;
    }

    static void setCStdArgArg(TemplateDeclaration * td)
    {
        cstdargArgTemplateDecl = td;
    }

    static bool maybeSetUpBuiltin(Declaration * decl);

    static tree functionPointer(FuncDeclaration * func_decl)
    {
        return addressOf(func_decl);
    }

    // Returns the D object that was thrown.  Different from the generic exception pointer
    static tree exceptionObject();

    static tree label(Loc loc, Identifier * ident = 0);

    // Static chain of function, for D2, this is a closure.
    void useChain(FuncDeclaration * f, tree l)
    {
        _chainLink = l;
        _chainFunc = f;
    }

    void useParentChain()
    {
        if (parent)
        {
            _chainLink = ((IRState *)parent)->_chainLink;
            _chainFunc = ((IRState *)parent)->_chainFunc;
        }
    }

    tree chainLink()
    {
        return _chainLink;
    }

    FuncDeclaration * chainFunc()
    {
        return _chainFunc;
    }

    void buildChain(FuncDeclaration * func);

protected:
    tree getFrameForSymbol(Dsymbol * nested_sym);
    tree getFrameRef(FuncDeclaration *outer_func);
    FuncDeclaration * _chainFunc;
    tree _chainLink;

public:
    static FuncFrameInfo * getFrameInfo(FuncDeclaration *fd);
public:
    static bool functionNeedsChain(FuncDeclaration *f);

    // Check for nested functions/class/structs
    static bool isClassNestedIn(ClassDeclaration *inner, ClassDeclaration *outer);
    static bool isFuncNestedIn(FuncDeclaration * inner, FuncDeclaration * outer);
    static FuncDeclaration * isClassNestedInFunction(ClassDeclaration * cd);

    tree getVThis(Dsymbol * decl, Expression * e);

    // Static chain for nested functions
    tree getFrameForFunction(FuncDeclaration * f);
    tree getFrameForNestedClass(ClassDeclaration * c);

#if V2
    // %% D2.0 - handle structs too
    static FuncDeclaration * isStructNestedInFunction(StructDeclaration * sd);
    tree getFrameForNestedStruct(StructDeclaration * s);
#endif

    // ** Instruction stream manipulation
    void startCond(Statement * stmt, tree t_cond);

    void startCond(Statement * stmt, Expression * e_cond)
    {
#if V2
        tree t_cond = e_cond->toElemDtor(this);
        startCond(stmt, convertForCondition(t_cond, e_cond->type));
#else
        startCond(stmt, convertForCondition(e_cond));
#endif
    }

    void startElse();
    void endCond();
    void startLoop(Statement * stmt);
    void continueHere();
    void setContinueLabel(tree lbl);
    void exitIfFalse(tree t_cond, bool is_top_cond = 0);

    void exitIfFalse(Expression * e_cond, bool is_top_cond = 0)
    {
#if V2
        tree t_cond = e_cond->toElemDtor(this);
        exitIfFalse(convertForCondition(t_cond, e_cond->type), is_top_cond);
#else
        exitIfFalse(convertForCondition(e_cond), is_top_cond);
#endif
    }

    void endLoop();
    void startCase(Statement * stmt, tree t_cond, int has_vars = 0);
    void doCase(tree t_value, tree t_label);
    void endCase(tree t_cond);
    void continueLoop(Identifier * ident);
    void exitLoop(Identifier * ident);
    void startTry(Statement * stmt);
    void startCatches();
    void startCatch(tree t_type);
    void endCatch();
    void endCatches();
    void startFinally();
    void endFinally();
    /*static*/ void doReturn(tree t_value);
    /*static*/ void doJump(Statement * stmt, tree t_label);

    void doExp(tree t)
    {
        addExp(t);
    }

    void doExp(Expression * e)
    {   // %% should handle volatile...?
        doExp(e->toElem(this));
    }

    void doAsm(tree insn_tmpl, tree outputs, tree inputs, tree clobbers);

    // ** Goto/Label statement evaluation

    void pushLabel(LabelDsymbol * l)
    {
        labels.push(getLabelBlock(l));
    }

    void checkSwitchCase(Statement * stmt, int default_flag = 0);
    void checkGoto(Statement * stmt, LabelDsymbol * label);
    void checkPreviousGoto(Array * refs);

    static void doLineNote(const Loc & loc)
    {
        ObjectFile::doLineNote(loc);
    }
};

struct GlobalValues
{
    ObjectFile * ofile;
    IRState    * irs;
    Module     * mod;
    ModuleInfo * mi()
    {
        return ofile->moduleInfo;
    }
};

extern GlobalValues g;

extern IRState gen;

// Various helpers that need extra state

struct WrappedExp : Expression
{
    tree exp_node;
    WrappedExp(Loc loc, enum TOK op, tree exp_node, Type * type);
    void toCBuffer(OutBuffer *buf);
    elem *toElem(IRState *irs);
};

struct ListMaker
{
    tree head;
    tree * ptail;
    ListMaker() : head(NULL_TREE), ptail(& head) { }
    ListMaker(tree * alt_head) : head(NULL_TREE), ptail(alt_head) { }

    void chain(tree t)
    {
        *ptail = t;
        ptail = & TREE_CHAIN(t);
    }

    void cons(tree p, tree v)
    {
        *ptail = tree_cons(p,v,NULL_TREE);
        ptail = & TREE_CHAIN(*ptail);
    }

    void cons(tree v)
    {
        cons(NULL_TREE, v);
    }
};

struct CtorEltMaker
{
    VEC(constructor_elt,gc) *head;
    CtorEltMaker() : head(NULL) { }

    void reserve(int i)
    {
        VEC_reserve(constructor_elt,gc,head,i);
    }

    void cons(tree p, tree v)
    {
        constructor_elt * ce;
        ce = VEC_safe_push(constructor_elt,gc,head,NULL);
        ce->index = p;
        ce->value = v;
    }

    void cons(tree v)
    {
        cons(NULL_TREE, v);
    }
};

class FieldVisitor
{
public:
    AggregateDeclaration * aggDecl;
    FieldVisitor(AggregateDeclaration * decl) : aggDecl(decl) { }
    //virtual doField(VarDeclaration * field) = 0;
    virtual void doFields(VarDeclarations * fields, AggregateDeclaration * agg) = 0;
    virtual void doInterfaces(BaseClasses * bases, AggregateDeclaration * agg) = 0;

    void go()
    {
        visit(aggDecl);
    }

    void visit(AggregateDeclaration * decl);
};

class AggLayout : public FieldVisitor
{
public:
    tree aggType;
    ListMaker fieldList;
    AggLayout(AggregateDeclaration * ini_agg_decl, tree ini_agg_type) :
        FieldVisitor(ini_agg_decl),
        aggType(ini_agg_type),
        fieldList(& TYPE_FIELDS(aggType)) { }
    void doFields(VarDeclarations * fields, AggregateDeclaration * agg);
    void doInterfaces(BaseClasses * bases, AggregateDeclaration * agg);
    void addField(tree field_decl, size_t offset);
    void finish(Expressions * attrs);
};

class ArrayScope
{
    VarDeclaration * v;
    IRState * irs;
public:
    ArrayScope(IRState * ini_irs, VarDeclaration * ini_v, const Loc & loc);
    tree setArrayExp(tree e, Type * t);
    tree finish(tree e);
};

class AddrOfExpr
{
public:
    tree var;

    AddrOfExpr()
    {
        var = NULL_TREE;
    }

    tree set(IRState * irs, tree exp)
    {
        return irs->addressOf(irs->maybeExprVar(exp, & var));
    }

    tree finish(IRState * irs, tree e2)
    {
        return var ? irs->binding(var, e2) : e2;
    }
};

class CallExpr
{
public:
    tree ce;
#if D_GCC_VER >= 43
    int argi;
    CallExpr(tree ce_) : ce(ce_), argi(0) { }

    tree callee()
    {
        return CALL_EXPR_FN(ce);
    }

    tree nextArg()
    {
        tree result = argi < call_expr_nargs(ce) ?
            CALL_EXPR_ARG(ce, argi) : NULL_TREE;
        ++argi;
        return result;
    }
#else
    tree arge;
    CallExpr(tree ce_) : ce(ce_), arge(TREE_OPERAND(ce, 1)) { }

    tree callee()
    {
        return TREE_OPERAND(ce, 0);
    }

    tree nextArg()
    {
        tree result = arge ? TREE_VALUE(arge) : NULL_TREE;
        arge = TREE_CHAIN(arge);
        return result;
    }
#endif
private:
    CallExpr() { }
};

#endif


