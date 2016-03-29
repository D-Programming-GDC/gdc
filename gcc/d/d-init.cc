// d-init.cc -- D frontend for GCC.

// Copyright (C) 2016 Free Software Foundation, Inc.

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
#include "dfrontend/ctfe.h"
#include "dfrontend/expression.h"
#include "dfrontend/init.h"
#include "dfrontend/mtype.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "function.h"
#include "varasm.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"

static tree build_array_single(TypeSArray *, Expression *);

// Handle initialization things for constants to be written to data segment.
// All routines here should return a CST expression or CONSTRUCTOR.

class InitVisitor : public Visitor
{
  Type *type_;
  tree result_;

public:
  InitVisitor(Type *type)
  {
    this->type_ = type;
    this->result_ = NULL_TREE;
  }

  tree result() const
  {
    return this->result_;
  }

  // This should be overriden by each initializer class.
  void visit(Initializer *i)
  {
    set_input_location(i->loc);
    gcc_unreachable();
  }

  // Explicit void initializer is same as zero-init.
  void visit(VoidInitializer *)
  {
  }

  // Only seen when an error occurs in the frontend.
  void visit(ErrorInitializer *)
  {
    this->result_ = error_mark_node;
  }

  // An array literal initializer, convert to an expression.
  void visit(ArrayInitializer *i)
  {
    set_input_location(i->loc);
    Expression *e = i->toExpression(i->type);
    this->result_ = build_cst_expression(e);
  }

  // A constant expression.
  void visit(ExpInitializer *i)
  {
    set_input_location(i->loc);

    // Look for static array that is block initialised.
    Type *tb = this->type_->toBasetype();
    if (tb->ty == Tsarray)
      {
	Type *tbn = tb->nextOf();
	if (!tbn->equals(i->exp->type->toBasetype()->nextOf())
	    && i->exp->implicitConvTo(tbn))
	  {
	    this->result_ = build_array_single((TypeSArray *) tb, i->exp);
	    return;
	  }
      }

    this->result_ = build_cst_expression(i->exp);
  }
};


// Main entry point for the InitVisitor to generate a CST from the
// initializer AST represented from INIT.  Returns the cached result.

tree
build_initializer(Type *type, Initializer *init)
{
  InitVisitor v = InitVisitor(type);
  init->accept(&v);
  return v.result();
}

// Create an integer literal with the given expression.
// Helper for build_cst_expression.

static tree
build_integer_init(IntegerExp *e)
{
  tree type = build_ctype(e->type);
  return build_integer_cst(e->value, type);
}

// Create a floating point literal with the given expression.
// Helper for build_cst_expression.

static tree
build_float_init(RealExp *e)
{
  return build_float_cst(e->value, e->type);
}

//

static tree
build_complex_init(ComplexExp *e)
{
  Type *tnext;

  switch (e->type->toBasetype()->ty)
    {
    case Tcomplex32:
      tnext = Type::tfloat32;
      break;

    case Tcomplex64:
      tnext = Type::tfloat64;
      break;

    case Tcomplex80:
      tnext = Type::tfloat80;
      break;

    default:
      gcc_unreachable();
    }

  return build_complex(build_ctype(e->type),
		       build_float_cst(creall(e->value), tnext),
		       build_float_cst(cimagl(e->value), tnext));
}

// Create a string literal with the given expression.
// Helper for build_cst_expression.

static tree
build_string_init(StringExp *e)
{
  Type *tb = e->type->toBasetype();

  // All strings are null terminated except static arrays.
  size_t length = (e->len * e->sz) + (tb->ty != Tsarray);
  tree ctor = build_string(length, (char *) e->string);
  tree type = build_ctype(e->type);

  if (tb->ty == Tsarray)
    TREE_TYPE (ctor) = type;
  else
    {
      // Array type doesn't match string length if null terminated.
      TREE_TYPE (ctor) = d_array_type(tb->nextOf(), e->len);

      ctor = build_address(ctor);
      if (tb->ty == Tarray)
	ctor = d_array_value(type, size_int(e->len), ctor);
    }

  TREE_CONSTANT (ctor) = 1;
  TREE_STATIC (ctor) = 1;

  return d_convert(type, ctor);
}

// Create a 'null' literal with the given expression.
// Helper for build_cst_expression.

static tree
build_null_init(NullExp *e)
{
  Type *tb = e->type->toBasetype();
  tree ctor;

  // Handle certain special case conversions, where the underlying type is an
  // aggregate with a nullable interior pointer.
  // This is the case for dynamic arrays, associative arrays, and delegates.
  if (tb->ty == Tarray)
    {
      tree type = build_ctype(e->type);
      ctor = d_array_value(type, size_int(0), null_pointer_node);
    }
  else if (tb->ty == Taarray)
    {
      vec<constructor_elt, va_gc> *ce = NULL;
      tree type = build_ctype(e->type);

      CONSTRUCTOR_APPEND_ELT (ce, TYPE_FIELDS (type), null_pointer_node);
      ctor = build_constructor(type, ce);
    }
  else if (tb->ty == Tdelegate)
    ctor = build_delegate_cst(null_pointer_node, null_pointer_node, e->type);
  else
    ctor = d_convert(build_ctype(e->type), integer_zero_node);

  TREE_CONSTANT (ctor) = 1;
  if (AGGREGATE_TYPE_P (TREE_TYPE (ctor)))
    TREE_STATIC (ctor) = 1;

  return ctor;
}

// Create an array literal with the given expression.

static tree
build_array_init(ArrayLiteralExp *e)
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
	return d_array_value(build_ctype(e->type), size_int(0),
			     null_pointer_node);
      else
	return build_constructor(d_array_type(tb->nextOf(), 0), NULL);
    }

  // Build a constructor that assigns the expressions in ELEMENTS at each index.
  vec<constructor_elt, va_gc> *elms = NULL;
  vec_safe_reserve(elms, e->elements->dim);
  bool constant_p = true;
  bool simple_p = true;

  Type *etype = tb->nextOf();
  tree satype = d_array_type(etype, e->elements->dim);

  for (size_t i = 0; i < e->elements->dim; i++)
    {
      Expression *value = (*e->elements)[i];
      tree elem = build_cst_expression(value);

      // Only care about non-zero values, the backend will fill in the rest.
      if (!initializer_zerop(elem))
	{
	  if (!TREE_CONSTANT (elem))
	    {
	      elem = maybe_make_temp(elem);
	      constant_p = false;
	    }

	  // Initializer is not suitable for static data.
	  if (!initializer_constant_valid_p(elem, TREE_TYPE (elem)))
	    simple_p = false;

	  CONSTRUCTOR_APPEND_ELT (elms, size_int(i),
				  convert_expr(elem, value->type, etype));
	}
    }

  // Now return the constructor as the correct type.  For static arrays there
  // is nothing else to do.  For dynamic arrays return a two field struct.
  // And for pointers return the address.
  tree ctor = build_constructor(satype, elms);
  tree type = build_ctype(e->type);

  if (tb->ty != Tsarray)
    {
      ctor = build_address(ctor);
      if (tb->ty == Tarray)
	ctor = d_array_value(type, size_int(e->elements->dim), ctor);
    }

  if (constant_p)
    TREE_CONSTANT (ctor) = 1;
  if (constant_p && simple_p)
    TREE_STATIC (ctor) = 1;

  return d_convert(type, ctor);
}

//

static tree
build_array_single(TypeSArray *type, Expression *value)
{
  tree elem = build_cst_expression(value);
  tree satype = build_ctype(type);

  if (initializer_zerop(elem))
    return build_constructor(satype, NULL);

  size_t dims = type->dim->toInteger();
  vec<constructor_elt, va_gc> *elms = NULL;
  vec_safe_reserve(elms, dims);

  Type *etype = type->nextOf();
  bool constant_p = true;
  bool simple_p = true;

  if (!TREE_CONSTANT (elem))
    {
      elem = maybe_make_temp(elem);
      constant_p = false;
    }

  // Initializer is not suitable for static data.
  if (!initializer_constant_valid_p(elem, TREE_TYPE (elem)))
    simple_p = false;

  for (size_t i = 0; i < dims; i++)
    {
      CONSTRUCTOR_APPEND_ELT (elms, size_int(i),
			      convert_expr(elem, value->type, etype));
    }

  tree ctor = build_constructor(satype, elms);

  if (constant_p)
    TREE_CONSTANT (ctor) = 1;
  if (constant_p && simple_p)
    TREE_STATIC (ctor) = 1;

  return ctor;
}

// Create a struct or union literal with the given expression.
// Helper for build_cst_expression.

static tree
build_aggregate_init(StructLiteralExp *e)
{
  // Handle empty struct literals.
  if (e->elements == NULL || e->sd->fields.dim == 0)
    return build_constructor(build_ctype(e->type), NULL);

  gcc_assert(e->elements->dim <= e->sd->fields.dim);

  // Build a constructor that assigns the expressions in ELEMENTS at each index.
  vec<constructor_elt, va_gc> *elms = NULL;
  vec_safe_reserve(elms, e->elements->dim);

  for (size_t i = 0; i < e->elements->dim; i++)
    {
      Expression *value = (*e->elements)[i];
      if (value == NULL)
	continue;

      tree elem = build_cst_expression(value);
      VarDeclaration *var = e->sd->fields[i];

      CONSTRUCTOR_APPEND_ELT (elms, var->toSymbol()->Stree,
			      convert_expr(elem, value->type, var->type));
    }

  // Return a constructor in the correct shape of the aggregate type.
  return build_struct_literal(build_ctype(e->type),
			      build_constructor(unknown_type_node, elms));
}

// Unlike structs, the TYPE_FIELDS of classes may not be referenced by the
// frontend field decls.  So we need to do more to ensure that each value
// is assigned at the correct field index.

#if 0
static tree
field_for_context(VarDeclaration *field, tree context)
{
  tree var = field->toSymbol()->Stree;

  if (DECL_FIELD_CONTEXT (var) == context)
    return var;

  vec<tree, va_gc> *overrides = DECL_OVERRIDES (var);

  for (size_t i = 0; i < vec_safe_length(overrides); i++)
    {
      var = (*overrides)[i];
      if (DECL_FIELD_CONTEXT (var) == context)
	return var;
    }

  return NULL_TREE;
}
#endif

// Create a static class reference with the given expression.
// Helper for build_cst_expression.

static tree
build_new_class_init(ClassReferenceExp *e)
{
#if 1
  tree var = build_address(e->toSymbol()->Stree);

  // If the typeof this literal is an interface, we must add offset to symbol.
  InterfaceDeclaration *to = ((TypeClass *) e->type)->sym->isInterfaceDeclaration();
  if (to != NULL)
    {
      ClassDeclaration *from = e->originalClass();
      int offset = 0;

      gcc_assert(to->isBaseOf(from, &offset) != 0);

      if (offset != 0)
	var = build_offset(var, size_int(offset));
    }

  return var;
#else
  ClassDeclaration *cd = e->originalClass();
  tree type = build_ctype(cd->type);

  vec<constructor_elt, va_gc> *elms = NULL;
  vec_safe_reserve(elms, e->value->elements->dim + 1);

  // The first field is the __vptr, and needs initializing.
  CONSTRUCTOR_APPEND_ELT (elms, TYPE_FIELDS (TREE_TYPE (type)),
			  build_address(cd->toVtblSymbol()->Stree));

  for (ClassDeclaration *bcd = cd; bcd != NULL; bcd = bcd->baseClass)
    {
      for (size_t i = 0; i < bcd->fields.dim; i++)
	{
	  VarDeclaration *var = bcd->fields[i];
	  int index = e->findFieldIndexByName(var);
	  Expression *value = (*e->value->elements)[index];
	  if (value == NULL)
	    continue;

	  tree elem = build_cst_expression(value);

	  CONSTRUCTOR_APPEND_ELT (elms, field_for_context(var, TREE_TYPE (type)),
				  convert_expr(elem, value->type, var->type));
	}
    }

  tree ctor = build_struct_literal(TREE_TYPE (type),
				   build_constructor(unknown_type_node, elms));

  // Can't be const, as we take the address of it.
  TREE_CONSTANT (ctor) = 0;
  ctor = build_address(ctor);

  return d_convert(type, ctor);
#endif
}

//

static tree
build_offset_init(SymOffExp *e)
{
  if (!(e->var->isDataseg() || e->var->isCodeseg())
      || e->var->needThis() || e->var->isThreadlocal())
    {
      e->error("non-constant expression %s", e->toChars());
      return error_mark_node;
    }

  tree var = build_address(e->var->toSymbol()->Stree);

  if (e->offset)
    return build_offset(var, size_int(e->offset));

  return var;
}

//

static tree
build_address_init(AddrExp *e)
{
  tree type = build_ctype(e->type);
  tree var;

  if (e->e1->op == TOKstructliteral)
    {
      StructLiteralExp *sle = (StructLiteralExp *) e->e1;
      var = sle->toSymbol()->Stree;
    }
  else
    var = build_cst_expression(e->e1);

  TREE_CONSTANT (var) = 0;
  return d_convert(type, build_address(var));
}

//

static tree
build_function_init(FuncExp *e)
{
  // This check is for lambda's, remove 'vthis' as function isn't nested.
  if (e->fd->tok == TOKreserved && e->type->ty == Tpointer)
    {
      e->fd->tok = TOKfunction;
      e->fd->vthis = NULL;
    }

  if (e->fd->isNested())
    {
      e->error("non-constant nested delegate literal expression %s", e->toChars());
      return error_mark_node;
    }

  // Emit after current function body has finished.
  if (cfun != NULL)
    cfun->language->deferred_fns.safe_push(e->fd);
  else
    e->fd->toObjFile();

  return build_address(e->fd->toSymbol()->Stree);
}

// Create a CST or CONSTRUCTOR from the expression E.
// This is the main entrypoint for all build_xxx_init routines.

tree
build_cst_expression(Expression *e)
{
  switch (e->op)
    {
    case TOKint64:
      return build_integer_init((IntegerExp *) e);

    case TOKfloat64:
      return build_float_init((RealExp *) e);

    case TOKcomplex80:
      return build_complex_init((ComplexExp *) e);

    case TOKnull:
      return build_null_init((NullExp *) e);

    case TOKstring:
      return build_string_init((StringExp *) e);

    case TOKarrayliteral:
      return build_array_init((ArrayLiteralExp *) e);

    case TOKstructliteral:
      return build_aggregate_init((StructLiteralExp *) e);

    case TOKclassreference:
      return build_new_class_init((ClassReferenceExp *) e);

    case TOKsymoff:
      return build_offset_init((SymOffExp *) e);

    case TOKaddress:
      return build_address_init((AddrExp *) e);

    case TOKfunction:
      return build_function_init((FuncExp *) e);

    default:
      break;
    }

  e->error("non-constant expression %s", e->toChars());
  return error_mark_node;
}
