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


// Intrinsic bitop, intrinsic math, and internally recognised runtime library functions
// are listed in alphabetical order for use of bsearch.
enum Intrinsic
{
  INTRINSIC_BSF, INTRINSIC_BSR,
  INTRINSIC_BSWAP,
  INTRINSIC_BTC, INTRINSIC_BTR, INTRINSIC_BTS,

  INTRINSIC_COS, INTRINSIC_FABS,
  INTRINSIC_LDEXP, INTRINSIC_RINT,
  INTRINSIC_RNDTOL, INTRINSIC_SIN,
  INTRINSIC_SQRT,

  INTRINSIC_VA_START,
  INTRINSIC_VA_ARG,
  INTRINSIC_C_VA_ARG,
  INTRINSIC_count,
};

enum LibCall
{
  LIBCALL_NONE = -1,
  LIBCALL_INVARIANT,
  LIBCALL_AAPPLYRCD1, LIBCALL_AAPPLYRCD2,
  LIBCALL_AAPPLYRCW1, LIBCALL_AAPPLYRCW2,
  LIBCALL_AAPPLYRDC1, LIBCALL_AAPPLYRDC2,
  LIBCALL_AAPPLYRDW1, LIBCALL_AAPPLYRDW2,
  LIBCALL_AAPPLYRWC1, LIBCALL_AAPPLYRWC2,
  LIBCALL_AAPPLYRWD1, LIBCALL_AAPPLYRWD2,
  LIBCALL_AAPPLYCD1, LIBCALL_AAPPLYCD2,
  LIBCALL_AAPPLYCW1, LIBCALL_AAPPLYCW2,
  LIBCALL_AAPPLYDC1, LIBCALL_AAPPLYDC2,
  LIBCALL_AAPPLYDW1, LIBCALL_AAPPLYDW2,
  LIBCALL_AAPPLYWC1, LIBCALL_AAPPLYWC2,
  LIBCALL_AAPPLYWD1, LIBCALL_AAPPLYWD2,
  LIBCALL_AAAPPLY, LIBCALL_AAAPPLY2,

  LIBCALL_AADELX, LIBCALL_AAEQUAL,
  LIBCALL_AAGETRVALUEX, LIBCALL_AAGETX,
  LIBCALL_AAINX,
  LIBCALL_AALEN,

  LIBCALL_ADCMP, LIBCALL_ADCMP2,
  LIBCALL_ADDUPT,
  LIBCALL_ADEQ, LIBCALL_ADEQ2,
  LIBCALL_ADREVERSE,
  LIBCALL_ADREVERSECHAR,
  LIBCALL_ADREVERSEWCHAR,
  LIBCALL_ADSORT,
  LIBCALL_ADSORTCHAR,
  LIBCALL_ADSORTWCHAR,

  LIBCALL_ALLOCMEMORY,
  LIBCALL_ARRAY_BOUNDS,

  LIBCALL_ARRAYAPPENDT, LIBCALL_ARRAYAPPENDCTX,
  LIBCALL_ARRAYAPPENDCD, LIBCALL_ARRAYAPPENDWD,
  LIBCALL_ARRAYASSIGN,
  LIBCALL_ARRAYCAST,
  LIBCALL_ARRAYCATT, LIBCALL_ARRAYCATNT,
  LIBCALL_ARRAYCOPY,
  LIBCALL_ARRAYCTOR,
  LIBCALL_ARRAYLITERALTX,
  LIBCALL_ARRAYSETASSIGN, LIBCALL_ARRAYSETCTOR,
  LIBCALL_ARRAYSETLENGTHT, LIBCALL_ARRAYSETLENGTHIT,

  LIBCALL_ASSERT,
  LIBCALL_ASSERT_MSG,

  LIBCALL_ASSOCARRAYLITERALTX,

  LIBCALL_CALLFINALIZER,
  LIBCALL_CALLINTERFACEFINALIZER,

  LIBCALL_CRITICALENTER,
  LIBCALL_CRITICALEXIT,

  LIBCALL_DELARRAY, LIBCALL_DELARRAYT,
  LIBCALL_DELCLASS, LIBCALL_DELINTERFACE,
  LIBCALL_DELMEMORY,

  LIBCALL_DYNAMIC_CAST,
  LIBCALL_HIDDEN_FUNC,
  LIBCALL_INTERFACE_CAST,

  LIBCALL_MONITORENTER,
  LIBCALL_MONITOREXIT,

  LIBCALL_NEWARRAYT, LIBCALL_NEWARRAYIT,
  LIBCALL_NEWARRAYMTX, LIBCALL_NEWARRAYMITX,
  LIBCALL_NEWCLASS, LIBCALL_NEWITEMT,
  LIBCALL_NEWITEMIT,

  LIBCALL_SWITCH_DSTRING, LIBCALL_SWITCH_ERROR,
  LIBCALL_SWITCH_STRING, LIBCALL_SWITCH_USTRING,

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
extern tree d_decl_context (Dsymbol *dsym);

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
extern tree build_float_modulus (tree type, tree t1, tree t2);

extern tree indirect_ref (tree type, tree exp);
extern tree build_deref (tree exp);

extern tree maybe_compound_expr (tree arg0, tree arg1);
extern tree maybe_vcompound_expr (tree arg0, tree arg1);

extern tree bind_expr (tree var_chain, tree body);

extern bool error_mark_p (tree t);

extern tree d_build_label (Loc loc, Identifier *ident);

// Type conversion.
// 'd_convert' just to give it a different name from the extern "C" convert.
extern tree d_convert (tree type, tree exp);
extern tree convert_expr (tree exp, Type *exp_type, Type *target_type);

extern tree convert_for_assignment (tree expr, Type *exp_type, Type *target_type);
extern tree convert_for_condition (tree expr, Type *type);

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
extern tree build_array_index (tree ptr, tree index);
extern tree build_offset_op (enum tree_code op, tree ptr, tree idx);
extern tree build_offset (tree ptr_node, tree byte_offset);

// Function calls
extern tree d_build_call (tree type, tree callee, tree args);
extern tree d_build_call_nary (tree callee, int n_args, ...);

extern tree d_assert_call (Loc loc, LibCall libcall, tree msg = NULL_TREE);

// Closures and frame generation.
extern tree build_frame_type (FuncDeclaration *func);
extern FuncFrameInfo *get_frameinfo (FuncDeclaration *fd);
extern tree get_framedecl (FuncDeclaration *inner, FuncDeclaration *outer);

// Static chain for nested functions
extern tree get_frame_for_symbol (FuncDeclaration *func, Dsymbol *sym);

extern bool needs_static_chain (FuncDeclaration *f);

// Local variables
extern tree build_local_var (tree type);
extern tree create_temporary_var (tree type);
extern tree maybe_temporary_var (tree exp, tree *out_var);

extern tree get_decl_tree (Declaration *decl, FuncDeclaration *func);

// Temporaries (currently just SAVE_EXPRs)
extern tree maybe_make_temp (tree t);
extern bool d_has_side_effects (tree t);

// Array operations
extern bool unhandled_arrayop_p (BinExp *exp);

extern bool array_bounds_check (void);
extern tree d_checked_index (Loc loc, tree index, tree upr, bool inclusive);
extern tree d_bounds_condition (tree index, tree upr, bool inclusive);

// Delegates
extern tree delegate_method (tree exp);
extern tree delegate_object (tree exp);
extern tree build_delegate_cst (tree method, tree object, Type *type);

// These are for references to nested functions/methods as opposed to a delegate var.
extern tree build_method_call (tree callee, tree object, Type *type);
extern void extract_from_method_call (tree t, tree& callee, tree& object);
extern tree get_object_method (tree thisexp, Expression *objexp, FuncDeclaration *func, Type *type);

// Built-in and Library functions.
extern FuncDeclaration *get_libcall (LibCall libcall);
extern tree build_libcall (LibCall libcall, unsigned n_args, tree *args, tree force_type = NULL_TREE);
extern tree maybe_expand_builtin (tree call_exp);

extern void maybe_set_builtin_frontend (FuncDeclaration *decl);

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
  struct lang_type *lt = TYPE_LANG_SPECIFIC (t);
  return lt ? lt->d_type : NULL;
}

// Returns D Frontend decl for GCC decl T.
inline Declaration *
build_ddecl (tree t)
{
  gcc_assert (DECL_P (t));
  struct lang_decl *ld = DECL_LANG_SPECIFIC (t);
  return ld ? ld->d_decl : NULL;
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
// routines as a mixin.  Also a lot of routines are static (don't need IR 
// state or expand Expressions), but are still called using . or ->.

// Functions without a verb create trees
// Functions with 'do' affect the current instruction stream (or output assembler code).
// functions with other names are for attribute manipulate, etc.

struct IRState : IRBase
{
 public:
  // ** Local variables
  void emitLocalVar (VarDeclaration *v, bool no_init);

  void expandDecl (tree t_decl);

  // ** Type conversion
  tree toDArray (Expression *exp);

  // ** Various expressions
  static tree buildOp (enum tree_code code, tree type, tree arg0, tree arg1);
  tree buildAssignOp (enum tree_code code, Type *type, Expression *e1, Expression *e2);

  void doArraySet (tree in_ptr, tree in_value, tree in_count);
  tree arraySetExpr (tree ptr, tree value, tree count);

  // ** Function calls
  tree call (Expression *expr, Expressions *arguments);
  tree call (FuncDeclaration *func_decl, Expressions *args);
  tree call (FuncDeclaration *func_decl, tree object, Expressions *args);
  tree call (TypeFunction *guess, tree callable, tree object, Expressions *arguments);

  tree typeinfoReference (Type *t);

  void buildChain (FuncDeclaration *func);

  tree getVThis (Dsymbol *decl, Expression *e);

 protected:
  tree maybeExpandSpecialCall (tree call_exp);
};

// Globals.
extern Module *current_module_decl;
extern IRState *cirstate;

// Various helpers that need extra state

struct WrappedExp : Expression
{
  tree exp_node;
  WrappedExp (Loc loc, enum TOK op, tree exp_node, Type *type);
  void toCBuffer (OutBuffer *buf, HdrGenState *hgs);
  elem *toElem (IRState *irs);
};

class AggLayout
{
 public:
  AggLayout (AggregateDeclaration *ini_agg_decl, tree ini_agg_type)
    : aggDecl_(ini_agg_decl),
      aggType_(ini_agg_type)
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
};

class ArrayScope
{
 public:
  ArrayScope (VarDeclaration *ini_v, const Loc& loc);
  tree setArrayExp (tree e, Type *t);
  tree finish (tree e);

 private:
  VarDeclaration *var_;
};

class AddrOfExpr
{
 public:
  AddrOfExpr (void)
  { this->var_ = NULL_TREE; }

  tree set (tree exp)
  { return build_address (maybe_temporary_var (exp, &this->var_)); }

  tree finish (tree e2)
  { return this->var_ ? bind_expr (this->var_, e2) : e2; }

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


