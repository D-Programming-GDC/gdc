// d-codegen.h -- D frontend for GCC.
// Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

// List of codes for internally recognised D library functions.

enum LibCall
{
#define DEF_D_RUNTIME(CODE, N, P, T, F) LIBCALL_ ## CODE,
#include "runtime.def"
#undef DEF_D_RUNTIME
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

// Code generation routines.
extern void push_binding_level(level_kind kind);
extern tree pop_binding_level();

extern void push_stmt_list();
extern tree pop_stmt_list();
extern void add_stmt(tree t);

extern void start_function(FuncDeclaration *decl);
extern void end_function();

extern tree d_decl_context (Dsymbol *dsym);

extern tree d_mark_addressable (tree exp);
extern tree d_mark_used (tree exp);
extern tree d_mark_read (tree exp);
extern tree build_address (tree exp);

extern bool identity_compare_p(StructDeclaration *sd);
extern tree build_struct_comparison(tree_code code, StructDeclaration *sd, tree t1, tree t2);
extern tree build_array_struct_comparison(tree_code code, StructDeclaration *sd, tree length, tree t1, tree t2);
extern tree build_struct_literal(tree type, vec<constructor_elt, va_gc> *init);

// Routines to handle variables that are references.
extern bool declaration_reference_p (Declaration *decl);
extern tree declaration_type (Declaration *decl);
extern bool argument_reference_p (Parameter *arg);
extern tree type_passed_as (Parameter *arg);

extern tree d_array_type (Type *d_type, uinteger_t size);

extern tree insert_type_attribute (tree type, const char *attrname, tree value = NULL_TREE);
extern void insert_decl_attribute (tree type, const char *attrname, tree value = NULL_TREE);
extern tree build_attributes (Expressions *in_attrs);
extern tree insert_type_modifiers (tree type, unsigned mod);

extern tree build_two_field_type (tree t1, tree t2, Type *type, const char *n1, const char *n2);

extern tree build_float_modulus (tree type, tree t1, tree t2);

extern tree indirect_ref (tree type, tree exp);
extern tree build_deref (tree exp);

extern tree maybe_compound_expr (tree arg0, tree arg1);
extern tree maybe_vcompound_expr (tree arg0, tree arg1);

extern tree bind_expr (tree var_chain, tree body);

extern tree define_label(Statement *s, Identifier *ident = NULL);
extern tree lookup_label(Statement *s, Identifier *ident = NULL);
extern tree lookup_bc_label(Statement *s, bc_kind);
extern void check_goto(Statement *from, Statement *to);

// Type conversion.
// 'd_convert' just to give it a different name from the extern "C" convert.
extern tree d_convert (tree type, tree exp);
extern tree convert_expr (tree exp, Type *exp_type, Type *target_type);

extern tree convert_for_assignment (tree expr, Type *exp_type, Type *target_type);
extern tree convert_for_condition (tree expr, Type *type);

extern tree d_array_convert (Expression *exp);
extern tree d_array_convert (Type *etype, Expression *exp, vec<tree, va_gc> **vars);

// Simple constants
extern tree build_integer_cst (dinteger_t value, tree type = int_type_node);
extern tree build_float_cst (const real_t& value, Type *totype);

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
extern tree build_binary_op (tree_code code, tree type, tree arg0, tree arg1);
extern tree build_binop_assignment(tree_code code, Expression *e1, Expression *e2);
extern tree build_array_index (tree ptr, tree index);
extern tree build_offset_op (tree_code op, tree ptr, tree idx);
extern tree build_offset (tree ptr_node, tree byte_offset);
extern tree build_memref (tree type, tree ptr, tree byte_offset);
extern tree build_array_set(tree ptr, tree length, tree value);
extern tree build_array_from_val(Type *type, tree val);

// Function calls
extern tree d_build_call (FuncDeclaration *fd, tree object, Expressions *args);
extern tree d_build_call (TypeFunction *tf, tree callable, tree object, Expressions *arguments);
extern tree d_build_call_list (tree type, tree callee, tree args);
extern tree d_build_call_nary (tree callee, int n_args, ...);

extern tree d_assert_call (const Loc& loc, LibCall libcall, tree msg = NULL_TREE);

// Closures and frame generation.
extern tree build_frame_type(FuncDeclaration *func);
extern void build_closure(FuncDeclaration *fd);
extern FuncFrameInfo *get_frameinfo(FuncDeclaration *fd);
extern tree get_framedecl(FuncDeclaration *inner, FuncDeclaration *outer);

extern tree build_vthis(AggregateDeclaration *decl);
extern tree build_vthis_type(tree basetype, tree type);

// Static chain for nested functions
extern tree get_frame_for_symbol(Dsymbol *sym);

// Local variables
extern void build_local_var(VarDeclaration *vd);
extern tree build_local_temp(tree type);
extern tree create_temporary_var(tree type);
extern tree maybe_temporary_var(tree exp, tree *out_var);
extern void expand_decl(tree decl);

extern tree get_decl_tree(Declaration *decl);

// Temporaries (currently just SAVE_EXPRs)
extern tree make_temp (tree t);
extern tree maybe_make_temp (tree t);
extern bool d_has_side_effects (tree t);

// Array operations
extern tree build_bounds_condition(const Loc& loc, tree index, tree upr, bool inclusive);
extern bool array_bounds_check();

// Classes
extern tree build_class_binfo (tree super, ClassDeclaration *cd);
extern tree build_interface_binfo (tree super, ClassDeclaration *cd, unsigned& offset);

// Delegates
extern tree delegate_method (tree exp);
extern tree delegate_object (tree exp);
extern tree build_delegate_cst (tree method, tree object, Type *type);

// These are for references to nested functions/methods as opposed to a delegate var.
extern tree build_method_call (tree callee, tree object, Type *type);
extern void extract_from_method_call (tree t, tree& callee, tree& object);
extern tree build_vindex_ref (tree object, tree fndecl, size_t index);

// Built-in and Library functions.
extern FuncDeclaration *get_libcall (LibCall libcall);
extern tree build_libcall (LibCall libcall, unsigned n_args, tree *args, tree force_type = NULL_TREE);

extern void maybe_set_intrinsic (FuncDeclaration *decl);
extern tree expand_intrinsic (tree callexp);

extern tree build_typeinfo (Type *t);

// Record layout
extern void layout_aggregate_type(AggregateDeclaration *decl, tree type, AggregateDeclaration *base);
extern void insert_aggregate_field(const Loc& loc, tree type, tree field, size_t offset);
extern void finish_aggregate_type(unsigned structsize, unsigned alignsize, tree type, UserAttributeDeclaration *declattrs);

extern bool empty_aggregate_p(tree type);

// Type management for D frontend types.
// Returns TRUE if T1 and T2 are mutably the same type.
inline bool
d_types_same (Type *t1, Type *t2)
{
  Type *tb1 = t1->toBasetype()->immutableOf();
  Type *tb2 = t2->toBasetype()->immutableOf();
  return tb1->equals (tb2);
}

// Returns D frontend type 'Object' which all classes are derived from.
inline Type *
build_object_type()
{
  if (ClassDeclaration::object)
    return ClassDeclaration::object->type;

  ::error ("missing or corrupt object.d");
  return Type::terror;
}

// Common codegen helpers.
extern tree component_ref(tree obj, tree field);
extern tree modify_expr(tree dst, tree src);
extern tree modify_expr(tree type, tree dst, tree src);
extern tree vmodify_expr(tree dst, tree src);
extern tree build_vinit(tree dst, tree src);

extern tree build_nop(tree t, tree e);
extern tree build_vconvert(tree t, tree e);
extern tree build_boolop(tree_code code, tree arg0, tree arg1);
extern tree build_condition(tree type, tree arg0, tree arg1, tree arg2);
extern tree build_vcondition(tree arg0, tree arg1, tree arg2);

extern tree compound_expr(tree arg0, tree arg1);
extern tree vcompound_expr(tree arg0, tree arg1);
extern tree return_expr(tree ret);

extern tree size_mult_expr(tree arg0, tree arg1);

// Routines for built in structured types
extern tree real_part(tree c);
extern tree imaginary_part(tree c);
extern tree complex_expr(tree type, tree r, tree i);

// Helpers for call
extern TypeFunction *get_function_type (Type *t);
extern bool call_by_alias_p (FuncDeclaration *caller, FuncDeclaration *callee);


// Globals.
extern Modules builtin_modules;
extern Module *current_module_decl;

#endif


