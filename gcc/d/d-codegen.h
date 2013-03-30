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


struct FuncFrameInfo
{
  bool creates_frame;	    // Function creates nested frame.
  bool static_chain;	    // Function has static chain passed via PARM_DECL
  bool is_closure;	    // Frame is a closure (initialised on the heap).
  union
  {
    tree closure_rec;	    // Frame type for static chain
    tree frame_rec;
  };
};

class ArrayScope;

// Code generation routines.

extern tree d_mark_addressable (tree exp);
extern tree d_mark_used (tree exp);
extern tree d_mark_read (tree exp);
extern tree build_address (tree exp);

// Routines to handle variables that are references.
extern bool decl_reference_p (Declaration *decl);
extern tree declaration_type (Declaration *decl);
extern bool arg_reference_p (Parameter *arg);
extern tree type_passed_as (Parameter *arg);

extern tree d_array_type (Type *d_type, uinteger_t size);

extern tree insert_type_attribute (tree type, const char *attrname, tree value = NULL_TREE);
extern void insert_decl_attribute (tree type, const char *attrname, tree value = NULL_TREE);
extern tree build_attributes (Expressions *in_attrs);
extern tree insert_type_modifiers (tree type, unsigned mod);

extern tree build_two_field_type (tree t1, tree t2, Type *type, const char *n1, const char *n2);

extern tree build_exception_object (void);

extern tree indirect_ref (tree type, tree exp);
extern tree build_deref (tree exp);

extern tree maybe_compound_expr (tree arg0, tree arg1);
extern tree maybe_vcompound_expr (tree arg0, tree arg1);

extern bool error_mark_p (tree t);

// Simple constants
extern tree build_integer_cst (dinteger_t value, tree type = integer_type_node);
extern tree build_float_cst (const real_t& value, Type *target_type);

extern dinteger_t cst_to_hwi (double_int cst);
extern dinteger_t tree_to_hwi (tree t);

// Dynamic arrays
extern tree d_array_length (tree exp);
extern tree d_array_ptr (tree exp);

extern tree d_array_value (tree type, tree len, tree data);
extern tree d_array_string (const char *str);

// Length of either a static or dynamic array
extern tree get_array_length (tree exp, Type *exp_type);

// D allows { void[] a; &a[3]; }
extern tree void_okay_p (tree t);

// Various expressions
extern tree build_offset_op (enum tree_code op, tree ptr, tree idx);
extern tree build_offset (tree ptr_node, tree byte_offset);

// ** Function calls
extern tree d_build_call (tree type, tree callee, tree args);
extern tree d_build_call_nary (tree callee, int n_args, ...);

// Temporaries (currently just SAVE_EXPRs)
extern tree maybe_make_temp (tree t);
extern bool d_has_side_effects (tree t);

extern bool unhandled_arrayop_p (BinExp *exp);

// Delegates
extern tree delegate_method (tree exp);
extern tree delegate_object (tree exp);
extern tree build_delegate_cst (tree method, tree object, Type *type);

// These are for references to nested functions/methods as opposed to a delegate var.
extern tree build_method_call (tree callee, tree object, Type *type);
extern void extract_from_method_call (tree t, tree& callee, tree& object);
extern tree get_object_method (Expression *exp, FuncDeclaration *func, Type *type);

// Type management for D frontend types.
// Returns TRUE if T1 and T2 are mutably the same type.
inline bool
d_types_same (Type *t1, Type *t2)
{
  return t1->mutableOf()->equals (t2->mutableOf());
}

// Returns TRUE if T1 and T2 don't require special conversions.
inline bool
d_types_compatible (Type *t1, Type *t2)
{
  return t1->implicitConvTo (t2) >= MATCHconst;
}

// Returns D Frontend type for GCC type T.
inline Type *
build_dtype (tree t)
{
  gcc_assert (TYPE_P (t));
  struct lang_type *l = TYPE_LANG_SPECIFIC (t);
  return l ? l->d_type : NULL;
}

// Returns D frontend type 'Object' which all classes are derived from.
inline Type *
build_object_type (void)
{
  if (ClassDeclaration::object)
    return ClassDeclaration::object->type;

  ::error ("missing or corrupt object.d");
  return Type::terror;
}

inline tree
component_ref (tree v, tree f)
{
  return build3_loc (input_location, COMPONENT_REF, TREE_TYPE (f), v, f, NULL_TREE);
}

inline tree
modify_expr (tree dst, tree src)
{
  return build2_loc (input_location, MODIFY_EXPR, TREE_TYPE (dst), dst, src);
}

inline tree
modify_expr (tree type, tree dst, tree src)
{
  return build2_loc (input_location, MODIFY_EXPR, type, dst, src);
}

inline tree
vmodify_expr (tree dst, tree src)
{
  return build2_loc (input_location, MODIFY_EXPR, void_type_node, dst, src);
}

inline tree
build_vinit (tree dst, tree src)
{
  return build2_loc (input_location, INIT_EXPR, void_type_node, dst, src);
}

inline tree
build_nop (tree t, tree e)
{
  return build1_loc (input_location, NOP_EXPR, t, e);
}

inline tree
build_vconvert (tree t, tree e)
{
  return indirect_ref (t, build_address (e));
}

inline tree
build_boolop (enum tree_code code, tree arg0, tree arg1)
{
  return fold_build2_loc (input_location, code, boolean_type_node, arg0, arg1);
}

inline tree
compound_expr (tree arg0, tree arg1)
{
  return build2_loc (input_location, COMPOUND_EXPR, TREE_TYPE (arg1), arg0, arg1);
}

inline tree
vcompound_expr (tree arg0, tree arg1)
{
  return build2_loc (input_location, COMPOUND_EXPR, void_type_node, arg0, arg1);
}

// Giving error_mark_node a type allows for some assumptions about
// the type of an arbitrary expression.
inline tree
error_mark (Type *t)
{
  return build1_loc (input_location, NOP_EXPR, t->toCtype(), error_mark_node);
}

// Routines for built in structured types
inline tree
real_part (tree c)
{
  return build1_loc (input_location, REALPART_EXPR, TREE_TYPE (TREE_TYPE (c)), c);
}

inline tree
imaginary_part (tree c)
{
  return build1_loc (input_location, IMAGPART_EXPR, TREE_TYPE (TREE_TYPE (c)), c);
}

// Helpers for call
inline bool
function_type_p (tree t)
{
  return (TREE_CODE (t) == FUNCTION_TYPE || TREE_CODE (t) == METHOD_TYPE);
}

extern TypeFunction *get_function_type (Type *t);
extern bool call_by_alias_p (FuncDeclaration *caller, FuncDeclaration *callee);


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
    INTRINSIC_BTC, INTRINSIC_BTR, INTRINSIC_BTS,

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

  tree var (Declaration *decl);

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

  // ** Various expressions
  tree toElemLvalue (Expression *e);

  tree pointerIntSum (Expression *ptr_exp, Expression *idx_exp);
  tree pointerIntSum (tree ptr_node, tree idx_exp);

  static tree buildOp (enum tree_code code, tree type, tree arg0, tree arg1);
  tree buildAssignOp (enum tree_code code, Type *type, Expression *e1, Expression *e2);

  tree checkedIndex (Loc loc, tree index, tree upper_bound, bool inclusive);
  tree boundsCond (tree index, tree upper_bound, bool inclusive);
  int arrayBoundsCheck (void);

  tree arrayElemRef (IndexExp *aer_exp, ArrayScope *aryscp);

  void doArraySet (tree in_ptr, tree in_value, tree in_count);
  tree arraySetExpr (tree ptr, tree value, tree count);

  static tree binding (tree var_chain, tree body);

  // ** Function calls
  tree call (Expression *expr, Expressions *arguments);
  tree call (FuncDeclaration *func_decl, Expressions *args);
  tree call (FuncDeclaration *func_decl, tree object, Expressions *args);
  tree call (TypeFunction *guess, tree callable, tree object, Expressions *arguments);

  tree assertCall (Loc loc, LibCall libcall = LIBCALL_ASSERT);
  tree assertCall (Loc loc, Expression *msg);

  static FuncDeclaration *getLibCallDecl (LibCall libcall);
  // This does not perform conversions on the arguments.  This allows
  // arbitrary data to be passed through varargs without going through the
  // usual conversions.
  static void maybeSetLibCallDecl (FuncDeclaration *decl);
  static tree libCall (LibCall libcall, unsigned n_args, tree *args, tree force_result_type = NULL_TREE);

  TemplateEmission emitTemplates;

  // Variables that are in scope that will need destruction later
  VarDeclarations *varsInScope;

  static tree floatMod (tree type, tree arg0, tree arg1);

  tree typeinfoReference (Type *t);

  // Built-in symbols that require special handling.
  Module *intrinsicModule;
  Module *mathModule;
  Module *mathCoreModule;
  TemplateDeclaration *stdargTemplateDecl;
  TemplateDeclaration *cstdargTemplateDecl;
  TemplateDeclaration *cstdargStartTemplateDecl;

  static bool maybeSetUpBuiltin (Declaration *decl);

  static tree label (Loc loc, Identifier *ident = 0);

  // Static chain of function, for D2, this is a closure.
  void useChain (FuncDeclaration *f, tree l)
  {
    this->chainLink_ = l;
    this->chainFunc_ = f;
  }

  tree chainLink (void)
  { return this->chainLink_; }

  FuncDeclaration *chainFunc (void)
  { return this->chainFunc_; }

  void buildChain (FuncDeclaration *func);
  tree buildFrameForFunction (FuncDeclaration *func);
  FuncFrameInfo *getFrameInfo (FuncDeclaration *fd);
  bool functionNeedsChain (FuncDeclaration *f);

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

  // ** Goto/Label statement evaluation
  void pushLabel (LabelDsymbol *l);
  void checkSwitchCase (Statement *stmt, int default_flag = 0);
  void checkGoto (Statement *stmt, LabelDsymbol *label);
  void checkPreviousGoto (Array *refs);

 protected:
  tree maybeExpandSpecialCall (tree call_exp);

  tree getFrameForSymbol (Dsymbol *nested_sym);
  tree getFrameRef (FuncDeclaration *outer_func);
  FuncDeclaration *chainFunc_;
  tree chainLink_;
};

// 
extern Module *current_module;
extern IRState *current_irs;
extern ObjectFile *object_file;

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
  vec<constructor_elt, va_gc>* head;

  CtorEltMaker (void)
    : head(NULL)
  { }

  void reserve (int i)
  { vec_safe_reserve (this->head, i); }

  void cons (tree p, tree v)
  {
    constructor_elt ce = { p, v };
    vec_safe_push (this->head, ce);
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
  tree setArrayExp (tree e, Type *t);
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
  { return build_address (irs->maybeExprVar (exp, &this->var_)); }

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


