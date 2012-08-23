// d-codegen.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_CODEGEN_H
#define GCC_DCMPLR_CODEGEN_H

#include "d-irstate.h"
#include "d-objfile.h"

enum LibCall
{
  LIBCALL_NONE = -1,
  LIBCALL_INVARIANT,
  LIBCALL_AAPPLYRCD1,
  LIBCALL_AAPPLYRCD2,
  LIBCALL_AAPPLYRCW1,
  LIBCALL_AAPPLYRCW2,
  LIBCALL_AAPPLYRDC1,
  LIBCALL_AAPPLYRDC2,
  LIBCALL_AAPPLYRDW1,
  LIBCALL_AAPPLYRDW2,
  LIBCALL_AAPPLYRWC1,
  LIBCALL_AAPPLYRWC2,
  LIBCALL_AAPPLYRWD1,
  LIBCALL_AAPPLYRWD2,
  LIBCALL_AAPPLYCD1,
  LIBCALL_AAPPLYCD2,
  LIBCALL_AAPPLYCW1,
  LIBCALL_AAPPLYCW2,
  LIBCALL_AAPPLYDC1,
  LIBCALL_AAPPLYDC2,
  LIBCALL_AAPPLYDW1,
  LIBCALL_AAPPLYDW2,
  LIBCALL_AAPPLYWC1,
  LIBCALL_AAPPLYWC2,
  LIBCALL_AAPPLYWD1,
  LIBCALL_AAPPLYWD2,
  LIBCALL_AAAPPLY,
  LIBCALL_AAAPPLY2,
  LIBCALL_AADELX,
  LIBCALL_AAEQUAL,
  LIBCALL_AAGETRVALUEX,
  LIBCALL_AAGETX,
  LIBCALL_AAINX,
  LIBCALL_AALEN,
  LIBCALL_ADCMP,
  LIBCALL_ADCMP2,
  LIBCALL_ADCMPCHAR,
  LIBCALL_ADDUPT,
  LIBCALL_ADEQ,
  LIBCALL_ADEQ2,
  LIBCALL_ADREVERSE,
  LIBCALL_ADREVERSECHAR,
  LIBCALL_ADREVERSEWCHAR,
  LIBCALL_ADSORT,
  LIBCALL_ADSORTCHAR,
  LIBCALL_ADSORTWCHAR,
  LIBCALL_ALLOCMEMORY,
  LIBCALL_ARRAY_BOUNDS,
  LIBCALL_ARRAYAPPENDT,
  LIBCALL_ARRAYAPPENDCTX,
  LIBCALL_ARRAYAPPENDCD,
  LIBCALL_ARRAYAPPENDWD,
  LIBCALL_ARRAYASSIGN,
  LIBCALL_ARRAYCAST,
  LIBCALL_ARRAYCATT,
  LIBCALL_ARRAYCATNT,
  LIBCALL_ARRAYCOPY,
  LIBCALL_ARRAYCTOR,
  LIBCALL_ARRAYLITERALTX,
  LIBCALL_ARRAYSETASSIGN,
  LIBCALL_ARRAYSETCTOR,
  LIBCALL_ARRAYSETLENGTHT,
  LIBCALL_ARRAYSETLENGTHIT,
  LIBCALL_ASSERT,
  LIBCALL_ASSERT_MSG,
  LIBCALL_ASSOCARRAYLITERALTX,
  LIBCALL_CALLFINALIZER,
  LIBCALL_CALLINTERFACEFINALIZER,
  LIBCALL_CRITICALENTER,
  LIBCALL_CRITICALEXIT,
  LIBCALL_DELARRAY,
  LIBCALL_DELARRAYT,
  LIBCALL_DELCLASS,
  LIBCALL_DELINTERFACE,
  LIBCALL_DELMEMORY,
  LIBCALL_DYNAMIC_CAST,
  LIBCALL_HIDDEN_FUNC,
  LIBCALL_INTERFACE_CAST,
  LIBCALL_MONITORENTER,
  LIBCALL_MONITOREXIT,
  LIBCALL_NEWARRAYT,
  LIBCALL_NEWARRAYIT,
  LIBCALL_NEWARRAYMTX,
  LIBCALL_NEWARRAYMITX,
  LIBCALL_NEWCLASS,
  LIBCALL_NEWITEMT,
  LIBCALL_NEWITEMIT,
  LIBCALL_SWITCH_DSTRING,
  LIBCALL_SWITCH_ERROR,
  LIBCALL_SWITCH_STRING,
  LIBCALL_SWITCH_USTRING,
  LIBCALL_THROW,
  LIBCALL_UNITTEST,
  LIBCALL_UNITTEST_MSG,
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
  bool creates_frame;     // Function creates nested frame.
  bool static_chain;      // Function has static chain passed via PARM_DECL
  bool is_closure;        // Frame is a closure (initialised on the heap).
  union
  {
    tree closure_rec;   // Frame type for static chain
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
 public:
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

    INTRINSIC_VA_ARG,
    INTRINSIC_C_VA_ARG,
    INTRINSIC_C_VA_START,
    INTRINSIC_count,
  };

  static tree declContext (Dsymbol *d_sym);
  static void doLineNote (const Loc& loc);

  // ** Local variables
  void emitLocalVar (VarDeclaration *v, bool no_init = false);
  tree localVar (tree t_type);
  tree localVar (Type *e_type);

  tree exprVar (tree t_type);
  tree maybeExprVar (tree exp, tree *out_var);
  void expandDecl (tree t_decl);

  tree var (VarDeclaration *v);

  // ** Type conversion

  // 'convertTo' just to give it a different name from the extern "C" convert
  static tree convertTo (tree type, tree exp);
  tree convertTo (Expression *exp, Type *target_type);
  static tree convertTo (tree exp, Type *exp_type, Type *target_type);

  tree convertForAssignment (Expression *exp, Type *target_type);
  tree convertForAssignment (tree exp_tree, Type *exp_type, Type *target_type);
  tree convertForArgument (Expression *exp, Parameter *arg);
  tree convertForCondition (Expression *exp);
  tree convertForCondition (tree exp_tree, Type *exp_type);
  tree toDArray (Expression *exp);

  // ** Type management
  static bool typesSame (Type *t1, Type *t2);
  static bool typesCompatible (Type *t1, Type *t2);
  static Type *getDType (tree t);
  static Type *getObjectType (void);

  // Routines to handle variables that are references.
  static bool isDeclarationReferenceType (Declaration *decl);
  static tree trueDeclarationType (Declaration *decl);
  static bool isArgumentReferenceType (Parameter *arg);
  static tree trueArgumentType (Parameter *arg);

  static tree arrayType (Type *d_type, uinteger_t size); // %% use of dinteger_t
  static tree arrayType (tree type_node, uinteger_t size);

  static tree addTypeAttribute (tree type, const char *attrname, tree value = NULL_TREE);
  static void addDeclAttribute (tree type, const char *attrname, tree value = NULL_TREE);
  static tree attributes (Expressions *in_attrs);
  static tree addTypeModifiers (tree type, unsigned mod);

  // ** Simple constants
  static tree integerConstant (dinteger_t value, Type *type);
  static tree integerConstant (dinteger_t value, tree type = integer_type_node);
  static tree floatConstant (const real_t& value, Type *target_type);

  static dinteger_t hwi2toli (HOST_WIDE_INT low, HOST_WIDE_INT high);
  static dinteger_t hwi2toli (double_int cst);

  // ** Routines for built in structured types
  static tree realPart (tree c);
  static tree imagPart (tree c);

  // ** Dynamic arrays
  static tree darrayLenRef (tree exp);
  static tree darrayPtrRef (tree exp);
  tree darrayPtrRef (Expression *e);

  static tree darrayVal (tree type, tree len, tree data);
  // data may be NULL for a null pointer value
  static tree darrayVal (Type *type, uinteger_t len, tree data);
  static tree darrayVal (tree type, uinteger_t len, tree data);
  static tree darrayString (const char *str);

  // Length of either a static or dynamic array
  tree arrayLength (Expression *exp);
  static tree arrayLength (tree exp, Type *exp_type);

  // Delegates
  static tree delegateMethodRef (tree exp);
  static tree delegateObjectRef (tree exp);
  static tree delegateVal (tree method_exp, tree object_exp, Type *d_type);
  // These are for references to nested functions/methods as opposed to a variable of
  // type Tdelegate
  tree methodCallExpr (tree callee, tree object, Type *d_type);
  void extractMethodCallExpr (tree mcr, tree& callee_out, tree& object_out);
  tree objectInstanceMethod (Expression *obj_exp, FuncDeclaration *func, Type *d_type);

  static tree twoFieldType (tree ft1, tree ft2, Type *d_type = 0, const char *n1 = "_a", const char *n2 = "_b");
  static tree twoFieldType (Type *ft1, Type *ft2, Type *d_type = 0, const char *n1 = "_a", const char *n2 = "_b");
  static tree twoFieldCtor (tree f1, tree f2, int storage_class = 0);

  // ** Temporaries (currently just SAVE_EXPRs)

  // Create a SAVE_EXPR if 't' might have unwanted side effects if referenced
  // more than once in an expression.
  static tree makeTemp (tree t);
  static tree maybeMakeTemp (tree t);

  // Return true if t can be evaluated multiple times (i.e., in a loop body)
  // without unwanted side effects.  This is a stronger test than
  // the one used for maybeMakeTemp.
  static bool isFreeOfSideEffects (tree t);

  // ** Various expressions
  tree toElemLvalue (Expression *e);

  static tree addressOf (Dsymbol *d);
  static tree addressOf (tree exp);

  static tree markAddressable (tree exp);
  static tree markUsed (tree exp);
  static tree markRead (tree exp);

  /* Cast exp (which should be a pointer) to TYPE *and then indirect.  The
     back-end requires this cast in many cases. */
  static tree indirect (tree type, tree exp);
  static tree indirect (tree exp);

  static tree vmodify (tree dst, tree src)
  { return build2 (MODIFY_EXPR, void_type_node, dst, src); }

  static tree vinit (tree dst, tree src)
  { return build2 (INIT_EXPR, void_type_node, dst, src); }

  tree pointerIntSum (Expression *ptr_exp, Expression *idx_exp);
  tree pointerIntSum (tree ptr_node, tree idx_exp);
  static tree pointerOffsetOp (enum tree_code op, tree ptr, tree idx);
  static tree pointerOffset (tree ptr_node, tree byte_offset);

  static tree nop (tree t, tree e)
  { return build1 (NOP_EXPR, t, e); }

  static tree vconvert (tree t, tree e)
  { return build1 (VIEW_CONVERT_EXPR, t, e); }

  // DMD allows { void[] a; &a[3]; }
  static tree pvoidOkay (tree t);

  static tree boolOp (enum tree_code code, tree arg0, tree arg1)
  { return build2 (code, boolean_type_node, arg0, arg1); }

  tree checkedIndex (Loc loc, tree index, tree upper_bound, bool inclusive);
  tree boundsCond (tree index, tree upper_bound, bool inclusive);
  int arrayBoundsCheck (void);

  tree arrayElemRef (IndexExp *aer_exp, ArrayScope *aryscp);

  static tree binding (tree var_chain, tree body);

  static tree compound (tree arg0, tree arg1)
  { return build2 (COMPOUND_EXPR, TREE_TYPE (arg1), arg0, arg1); }

  static tree compound (tree type, tree arg0, tree arg1)
  { return build2 (COMPOUND_EXPR, type, arg0, arg1); }

  static tree voidCompound (tree arg0, tree arg1)
  { return build2 (COMPOUND_EXPR, void_type_node, arg0, arg1); }

  static tree maybeCompound (tree arg0, tree arg1);
  static tree maybeVoidCompound (tree arg0, tree arg1);

  static tree component (tree v, tree f)
  { return build3 (COMPONENT_REF, TREE_TYPE (f), v, f, NULL_TREE); }

  // Giving error_mark_node a type allows for some assumptions about
  // the type of an arbitrary expression.
  static tree errorMark (Type *t)
  { return build1 (NOP_EXPR, t->toCtype(), error_mark_node); }

  static bool isErrorMark (tree t);

  // ** Helpers for call
  static TypeFunction *getFuncType (Type *t);

  static bool isFuncType (tree t)
  { return (TREE_CODE (t) == FUNCTION_TYPE || TREE_CODE (t) == METHOD_TYPE); }

  // ** Function calls
  tree call (Expression *expr, Expressions *arguments);
  tree call (FuncDeclaration *func_decl, Expressions *args);
  tree call (FuncDeclaration *func_decl, tree object, Expressions *args);
  tree call (TypeFunction *guess, tree callable, tree object, Expressions *arguments);

  tree assertCall (Loc loc, LibCall libcall = LIBCALL_ASSERT);
  tree assertCall (Loc loc, Expression *msg);

  static FuncDeclaration *getLibCallDecl (LibCall lib_call);
  // This does not perform conversions on the arguments.  This allows
  // arbitrary data to be passed through varargs without going through the
  // usual conversions.
  static void maybeSetLibCallDecl (FuncDeclaration *decl);
  static tree libCall (LibCall lib_call, unsigned n_args, tree *args, tree force_result_type = NULL_TREE);
  static tree buildCall (tree type, tree callee, tree args);
  static tree buildCall (tree callee, int n_args, ...);

  TemplateEmission emitTemplates;
  bool stdInc;

  // Variables that are in scope that will need destruction later
  VarDeclarations *varsInScope;

  tree floatMod (tree type, tree arg0, tree arg1);

  tree typeinfoReference (Type *t);

  dinteger_t getTargetSizeConst (tree t);

  // Built-in symbols that require special handling.
  Module *intrinsicModule;
  Module *mathModule;
  Module *mathCoreModule;
  TemplateDeclaration *stdargTemplateDecl;
  TemplateDeclaration *cstdargTemplateDecl;
  TemplateDeclaration *cstdargStartTemplateDecl;

  static bool maybeSetUpBuiltin (Declaration *decl);

  // Returns the D object that was thrown.  Different from the generic exception pointer
  static tree exceptionObject (void);

  static tree label (Loc loc, Identifier *ident = 0);

  // Static chain of function, for D2, this is a closure.
  void useChain (FuncDeclaration *f, tree l)
  {
    this->chainLink_ = l;
    this->chainFunc_ = f;
  }

  void useParentChain (void)
  {
    if (this->parent)
      {
	this->chainLink_ = ((IRState *)this->parent)->chainLink_;
	this->chainFunc_ = ((IRState *)this->parent)->chainFunc_;
      }
  }

  tree chainLink (void)
  { return this->chainLink_; }

  FuncDeclaration *chainFunc (void)
  { return this->chainFunc_; }

  void buildChain (FuncDeclaration *func);
  static FuncFrameInfo *getFrameInfo (FuncDeclaration *fd);
  static bool functionNeedsChain (FuncDeclaration *f);

  // Check for nested functions/class/structs
  static FuncDeclaration *isClassNestedInFunction (ClassDeclaration *cd);
  static FuncDeclaration *isStructNestedInFunction (StructDeclaration *sd);

  tree findThis (ClassDeclaration *target_cd);
  tree getVThis (Dsymbol *decl, Expression *e);

  // Static chain for nested functions
  tree getFrameForFunction (FuncDeclaration *f);
  tree getFrameForNestedClass (ClassDeclaration *c);
  tree getFrameForNestedStruct (StructDeclaration *s);

  // ** Instruction stream manipulation
  void startCond (Statement *stmt, tree t_cond);
  void startCond (Statement *stmt, Expression *e_cond);
  void startElse (void);
  void endCond (void);
  void startLoop (Statement *stmt);
  void continueHere (void);
  void setContinueLabel (tree lbl);
  void exitIfFalse (tree t_cond);
  void exitIfFalse (Expression *e_cond);
  void endLoop (void);
  void startCase (Statement *stmt, tree t_cond, int has_vars = 0);
  void doCase (tree t_value, tree t_label);
  void endCase (void);
  void continueLoop (Identifier *ident);
  void exitLoop (Identifier *ident);
  void startTry (Statement *stmt);
  void startCatches (void);
  void startCatch (tree t_type);
  void endCatch (void);
  void endCatches (void);
  void startFinally (void);
  void endFinally (void);
  void doReturn (tree t_value);
  void doJump (Statement *stmt, tree t_label);

  void doExp (tree t);
  void doExp (Expression *e);
  void doAsm (tree insn_tmpl, tree outputs, tree inputs, tree clobbers);

  // ** Goto/Label statement evaluation
  void pushLabel (LabelDsymbol *l);
  void checkSwitchCase (Statement *stmt, int default_flag = 0);
  void checkGoto (Statement *stmt, LabelDsymbol *label);
  void checkPreviousGoto (Array *refs);

 protected:
  tree maybeExpandSpecialCall (tree call_exp);
  static tree expandPortIntrinsic (Intrinsic code, tree port, tree value, int outp);

  tree getFrameForSymbol (Dsymbol *nested_sym);
  tree getFrameRef (FuncDeclaration *outer_func);
  FuncDeclaration *chainFunc_;
  tree chainLink_;
};

struct GlobalValues
{
  ObjectFile *ofile;
  IRState *irs;
  Module *mod;
};

extern GlobalValues g;

extern IRState gen;

// Various helpers that need extra state

struct WrappedExp : Expression
{
  tree exp_node;
  WrappedExp (Loc loc, enum TOK op, tree exp_node, Type *type);
  void toCBuffer (OutBuffer *buf, HdrGenState *hgs);
  elem *toElem (IRState *irs);
};

struct ListMaker
{
 public:
  tree head;

  ListMaker (void)
    : head(NULL_TREE),
      ptail_(&this->head)
  { }

  ListMaker (tree *alt_head)
    : head(NULL_TREE),
      ptail_(alt_head)
  { }

  void chain (tree t)
  {
    *this->ptail_ = t;
    this->ptail_ = &TREE_CHAIN (t);
  }

  void cons (tree p, tree v)
  {
    *this->ptail_ = tree_cons (p,v,NULL_TREE);
    this->ptail_ = &TREE_CHAIN (*this->ptail_);
  }

  void cons (tree v)
  { cons (NULL_TREE, v); }

 private:
  tree *ptail_;
};

struct CtorEltMaker
{
 public:
  VEC(constructor_elt, gc) *head;

  CtorEltMaker (void)
    : head(NULL)
  { }

  void reserve (int i)
  { VEC_reserve (constructor_elt, gc, this->head, i); }

  void cons (tree p, tree v)
  {
    constructor_elt *ce;
    ce = VEC_safe_push (constructor_elt, gc, this->head, NULL);
    ce->index = p;
    ce->value = v;
  }

  void cons (tree v)
  { cons (NULL_TREE, v); }

};

class AggLayout
{
 public:
  AggLayout (AggregateDeclaration *ini_agg_decl, tree ini_agg_type)
    : aggDecl_(ini_agg_decl),
      aggType_(ini_agg_type),
      fieldList_(&TYPE_FIELDS(this->aggType_))
  { }

  void go (void)
  { visit (this->aggDecl_); }

  void visit (AggregateDeclaration *decl);

  void doFields (VarDeclarations *fields, AggregateDeclaration *agg);
  void doInterfaces (BaseClasses *bases);
  void addField (tree field_decl, size_t offset);
  void finish (Expressions *attrs);

 private:
  AggregateDeclaration *aggDecl_;
  tree aggType_;
  ListMaker fieldList_;
};

class ArrayScope
{
 public:
  ArrayScope (IRState *irs, VarDeclaration *ini_v, const Loc& loc);
  tree setArrayExp (IRState *irs, tree e, Type *t);
  tree finish (IRState *irs, tree e);

 private:
  VarDeclaration *var_;
};

class AddrOfExpr
{
 public:
  AddrOfExpr (void)
  { this->var_ = NULL_TREE; }

  tree set (IRState *irs, tree exp)
  { return irs->addressOf (irs->maybeExprVar (exp, &this->var_)); }

  tree finish (IRState *irs, tree e2)
  { return this->var_ ? irs->binding (this->var_, e2) : e2; }

 private:
  tree var_;
};

class CallExpr
{
 public:
  CallExpr (tree ce)
    : ce_(ce), argi_(0)
  { }

  tree callee (void)
  { return CALL_EXPR_FN (this->ce_); }

  tree nextArg (void)
  {
    tree result = this->argi_ < call_expr_nargs (this->ce_)
      ? CALL_EXPR_ARG (this->ce_, this->argi_) : NULL_TREE;
    this->argi_++;
    return result;
  }

 private:
  CallExpr (void)
  { }

  tree ce_;
  int argi_;
};

#endif


