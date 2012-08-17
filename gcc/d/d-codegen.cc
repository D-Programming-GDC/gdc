// d-codegen.cc -- D frontend for GCC.
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

#include "d-gcc-includes.h"
#include "d-lang.h"
#include "d-codegen.h"

#include "template.h"
#include "init.h"
#include "symbol.h"
#include "dt.h"
#include "id.h"

GlobalValues g;
IRState gen;

// Public routine called from D frontend to hide from glue interface.
// Returns TRUE if all templates are being emitted, either publicly
// or privately, into the current compilation.

bool
d_gcc_force_templates (void)
{
  return gen.emitTemplates == TEprivate || gen.emitTemplates == TEall;
}

// Public routine called from D frontend to hide from glue interface.
// Add local variable V into the current body.

void
d_gcc_emit_local_variable (VarDeclaration *v)
{
  g.irs->emitLocalVar (v);
}

// Return the DECL_CONTEXT for symbol D_SYM.

tree
IRState::declContext (Dsymbol *d_sym)
{
  Dsymbol *orig_sym = d_sym;
  AggregateDeclaration *ad;

  while ((d_sym = d_sym->toParent2()))
    {
      if (d_sym->isFuncDeclaration())
	{
	  // dwarf2out chokes without this check... (output_pubnames)
	  FuncDeclaration *f = orig_sym->isFuncDeclaration();
	  if (f && !gen.functionNeedsChain (f))
	    return NULL_TREE;

	  return d_sym->toSymbol()->Stree;
	}
      else if ((ad = d_sym->isAggregateDeclaration()))
	{
	  tree ctx = ad->type->toCtype();
	  if (ad->isClassDeclaration())
	    {
	      // RECORD_TYPE instead of REFERENCE_TYPE
	      ctx = TREE_TYPE (ctx);
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

// Update current source file location to LOC.

void
IRState::doLineNote (const Loc& loc)
{
  ObjectFile::doLineNote (loc);
}

// Add local variable V into the current body.  If NO_INIT,
// then variable does not have a default initialiser.

void
IRState::emitLocalVar (VarDeclaration *v, bool no_init)
{
  if (v->isDataseg() || v->isMember())
    return;

  Symbol *sym = v->toSymbol();
  tree var_decl = sym->Stree;

  gcc_assert (!TREE_STATIC (var_decl));
  if (TREE_CODE (var_decl) == CONST_DECL)
    return;

  DECL_CONTEXT (var_decl) = getLocalContext();

  // Compiler generated symbols
  if (v == this->func->vresult || v == this->func->v_argptr || v == this->func->v_arguments_var)
    DECL_ARTIFICIAL (var_decl) = 1;

  tree var_exp;
  if (sym->SframeField)
    {
      // Fixes debugging local variables.
      SET_DECL_VALUE_EXPR (var_decl, var (v));
      DECL_HAS_VALUE_EXPR_P (var_decl) = 1;
    }
  var_exp = var_decl;
  pushdecl (var_decl);

  tree init_exp = NULL_TREE; // complete initializer expression (include MODIFY_EXPR, e.g.)
  tree init_val = NULL_TREE;

  if (!no_init && !DECL_INITIAL (var_decl) && v->init)
    {
      if (!v->init->isVoidInitializer())
	{
	  ExpInitializer *exp_init = v->init->isExpInitializer();
	  Expression *ie = exp_init->toExpression();
	  init_exp = ie->toElem (this);
	}
      else
	{
	  no_init = true;
	}
    }

  if (!no_init)
    {
      g.ofile->doLineNote (v->loc);

      if (!init_val)
	{
	  init_val = DECL_INITIAL (var_decl);
	  DECL_INITIAL (var_decl) = NULL_TREE; // %% from expandDecl
	}
      if (!init_exp && init_val)
	init_exp = vinit (var_exp, init_val);

      if (init_exp)
	addExp (init_exp);
      else if (!init_val && v->size (v->loc)) // Zero-length arrays do not have an initializer
	d_warning (OPT_Wuninitialized, "uninitialized variable '%s'", v->ident ? v->ident->string : "(no name)");
    }
}

// Return an unnamed local temporary of type T_TYPE.

tree
IRState::localVar (tree t_type)
{
  tree t_decl = build_decl (BUILTINS_LOCATION, VAR_DECL, NULL_TREE, t_type);
  DECL_CONTEXT (t_decl) = getLocalContext();
  DECL_ARTIFICIAL (t_decl) = 1;
  DECL_IGNORED_P (t_decl) = 1;
  pushdecl (t_decl);
  return t_decl;
}

// Return an unnamed local temporary of type E_TYPE.

tree
IRState::localVar (Type *e_type)
{
  return localVar (e_type->toCtype());
}

// Return an undeclared local temporary of type T_TYPE
// for use with BIND_EXPR.

tree
IRState::exprVar (tree t_type)
{
  tree t_decl = build_decl (BUILTINS_LOCATION, VAR_DECL, NULL_TREE, t_type);
  DECL_CONTEXT (t_decl) = getLocalContext();
  DECL_ARTIFICIAL (t_decl) = 1;
  DECL_IGNORED_P (t_decl) = 1;
  layout_decl (t_decl, 0);
  return t_decl;
}

// Return an undeclared local temporary OUT_VAR initialised
// with result of expression EXP.

tree
IRState::maybeExprVar (tree exp, tree *out_var)
{
  tree t = exp;

  // Get the base component.
  while (TREE_CODE (t) == COMPONENT_REF)
    t = TREE_OPERAND (t, 0);

  if (!DECL_P (t) && !REFERENCE_CLASS_P (t))
    {
      *out_var = exprVar (TREE_TYPE (exp));
      DECL_INITIAL (*out_var) = exp;
      return *out_var;
    }
  else
    {
      *out_var = NULL_TREE;
      return exp;
    }
}

// Emit an INIT_EXPR for decl T_DECL.

void
IRState::expandDecl (tree t_decl)
{
  // nothing, pushdecl will add t_decl to a BIND_EXPR
  if (DECL_INITIAL (t_decl))
    {
      doExp (build2 (INIT_EXPR, void_type_node, t_decl, DECL_INITIAL (t_decl)));
      DECL_INITIAL (t_decl) = NULL_TREE;
    }
}

// Return the correct decl to be used for variable V.
// Could be a VAR_DECL, or a FIELD_DECL from a closure.

tree
IRState::var (VarDeclaration *v)
{
  bool is_frame_var = v->toSymbol()->SframeField != NULL;

  if (is_frame_var)
    {
      FuncDeclaration *f = v->toParent2()->isFuncDeclaration();

      tree cf = getFrameRef (f);
      tree field = v->toSymbol()->SframeField;
      gcc_assert (field != NULL_TREE);
      return component (indirect (cf), field);
    }
  else
    {
      // Static var or auto var that the back end will handle for us
      return v->toSymbol()->Stree;
    }
}

// Return expression EXP, whose type has been converted to TYPE.

tree
IRState::convertTo (tree type, tree exp)
{
  // Check this first before passing to getDType.
  if (isErrorMark (type) || isErrorMark (TREE_TYPE (exp)))
    return error_mark_node;

  Type *target_type = getDType (type);
  Type *expr_type = getDType (TREE_TYPE (exp));

  if (target_type && expr_type)
    return convertTo (exp, expr_type, target_type);

  return d_convert_basic (type, exp);
}

// Return a TREE representation of EXP implictly converted to TARGET_TYPE.

tree
IRState::convertTo (Expression *exp, Type *target_type)
{
  return convertTo (exp->toElem (this), exp->type, target_type);
}

// Return expression EXP, whose type has been convert from EXP_TYPE to TARGET_TYPE.

tree
IRState::convertTo (tree exp, Type *exp_type, Type *target_type)
{
  tree result = NULL_TREE;

  gcc_assert (exp_type && target_type);
  Type *ebtype = exp_type->toBasetype();
  Type *tbtype = target_type->toBasetype();

  if (typesSame (exp_type, target_type))
    return exp;

  if (isErrorMark (exp))
    return exp;

  switch (ebtype->ty)
    {
    case Tdelegate:
      if (tbtype->ty == Tdelegate)
	{
	  exp = maybeMakeTemp (exp);
	  return delegateVal (delegateMethodRef (exp), delegateObjectRef (exp),
			      target_type);
	}
      else if (tbtype->ty == Tpointer)
	{
	  // The front-end converts <delegate>.ptr to cast (void *)<delegate>.
	  // Maybe should only allow void* ?
	  exp = delegateObjectRef (exp);
	}
      else
	{
	  ::error ("can't convert a delegate expression to %s", target_type->toChars());
	  return error_mark_node;
	}
      break;

    case Tstruct:
      if (tbtype->ty == Tstruct)
      {
	if (target_type->size() == exp_type->size())
	  {
	    // Allowed to cast to structs with same type size.
	    result = vconvert (target_type->toCtype(), exp);
	  }
	else
	  {
	    ::error ("can't convert struct %s to %s", exp_type->toChars(), target_type->toChars());
	    return error_mark_node;
	  }
      }
      // else, default conversion, which should produce an error
      break;

    case Tclass:
      if (tbtype->ty == Tclass)
      {
	ClassDeclaration *target_class_decl = ((TypeClass *) tbtype)->sym;
	ClassDeclaration *obj_class_decl = ((TypeClass *) ebtype)->sym;
	bool use_dynamic = false;
	int offset;

	if (target_class_decl->isBaseOf (obj_class_decl, &offset))
	  {
	    // Casting up the inheritance tree: Don't do anything special.
	    // Cast to an implemented interface: Handle at compile time.
	    if (offset == OFFSET_RUNTIME)
	      use_dynamic = true;
	    else if (offset)
	      {
		tree t = target_type->toCtype();
		exp = maybeMakeTemp (exp);
		return build3 (COND_EXPR, t,
			       boolOp (NE_EXPR, exp, d_null_pointer),
			       nop (t, pointerOffset (exp, size_int (offset))),
			       nop (t, d_null_pointer));
	      }
	    else
	      {
		// d_convert will make a NOP cast
		break;
	      }
	  }
	else if (target_class_decl == obj_class_decl)
	  {
	    // d_convert will make a NOP cast
	    break;
	  }
	else if (!obj_class_decl->isCOMclass())
	  use_dynamic = true;

	if (use_dynamic)
	  {
	    // Otherwise, do dynamic cast
	    tree args[2] = {
		exp,
		addressOf (target_class_decl->toSymbol()->Stree)
	    }; // %% (and why not just addressOf (target_class_decl)
	    return libCall (obj_class_decl->isInterfaceDeclaration()
			    ? LIBCALL_INTERFACE_CAST : LIBCALL_DYNAMIC_CAST, 2, args);
	  }
	else
	  {
	    d_warning (0, "cast to %s will yield null result", target_type->toChars());
	    result = convertTo (target_type->toCtype(), d_null_pointer);
	    if (TREE_SIDE_EFFECTS (exp))
	      {
		// make sure the expression is still evaluated if necessary
		result = compound (exp, result);
	      }
	    return result;
	  }
      }
      // else default conversion
      break;

    case Tsarray:
      if (tbtype->ty == Tpointer)
	{
	  result = nop (target_type->toCtype(), addressOf (exp));
	}
      else if (tbtype->ty == Tarray)
	{
	  TypeSArray *a_type = (TypeSArray *) ebtype;

	  uinteger_t array_len = a_type->dim->toUInteger();
	  d_uns64 sz_a = a_type->next->size();
	  d_uns64 sz_b = tbtype->nextOf()->size();

	  // conversions to different sizes
	  // Assumes tvoid->size() == 1
	  // %% TODO: handle misalign like _d_arraycast_xxx ?
	  if (sz_a != sz_b)
	    array_len = array_len * sz_a / sz_b;

	  tree pointer_value = nop (tbtype->nextOf()->pointerTo()->toCtype(),
				    addressOf (exp));

	  // Assumes casting to dynamic array of same type or void
	  return darrayVal (target_type, array_len, pointer_value);
	}
      else if (tbtype->ty == Tsarray)
	{
	  // DMD apparently allows casting a static array to any static array type
	  return vconvert (target_type->toCtype(), exp);
	}
      else if (tbtype->ty == Tstruct)
	{
	  // And allows casting a static array to any struct type too.
	  // %% type sizes should have already been checked by the frontend.
	  gcc_assert (target_type->size() == exp_type->size());
	  result = vconvert (target_type->toCtype(), exp);
	}
      else
	{
	  ::error ("cannot cast expression of type %s to type %s",
		   exp_type->toChars(), target_type->toChars());
	  return error_mark_node;
	}
      break;

    case Tarray:
      if (tbtype->ty == Tpointer)
	{
	  return convertTo (target_type->toCtype(), darrayPtrRef (exp));
	}
      else if (tbtype->ty == Tarray)
	{
	  // assume tvoid->size() == 1
	  Type *src_elem_type = ebtype->nextOf()->toBasetype();
	  Type *dst_elem_type = tbtype->nextOf()->toBasetype();
	  d_uns64 sz_src = src_elem_type->size();
	  d_uns64 sz_dst = dst_elem_type->size();

	  if (/*src_elem_type->ty == Tvoid ||*/ sz_src == sz_dst)
	    {
	      // Convert from void[] or elements are the same size -- don't change length
	      return vconvert (target_type->toCtype(), exp);
	    }
	  else
	    {
	      unsigned mult = 1;
	      tree args[3] = {
		  // assumes Type::tbit->size() == 1
		  integerConstant (sz_dst, Type::tsize_t),
		  integerConstant (sz_src * mult, Type::tsize_t),
		  exp
	      };
	      return libCall (LIBCALL_ARRAYCAST, 3, args, target_type->toCtype());
	    }
	}
      else if (tbtype->ty == Tsarray)
	{
	  // %% Strings are treated as dynamic arrays D2.
	  if (ebtype->isString() && tbtype->isString())
	    return indirect (target_type->toCtype(), darrayPtrRef (exp));
	}
      else
	{
	  ::error ("cannot cast expression of type %s to %s",
		   exp_type->toChars(), target_type->toChars());
	  return error_mark_node;
	}
      break;

    case Taarray:
      if (tbtype->ty == Taarray)
	return vconvert (target_type->toCtype(), exp);
      // else, default conversion, which should product an error
      break;

    case Tpointer:
      /* For some reason, convert_to_integer converts pointers
	 to a signed type. */
      if (tbtype->isintegral())
	exp = d_convert_basic (d_type_for_size (POINTER_SIZE, 1), exp);
      // Can convert void pointers to associative arrays too...
      else if (tbtype->ty == Taarray && ebtype == Type::tvoidptr)
	return vconvert (target_type->toCtype(), exp);
      break;

    case Tnull:
      if (tbtype->ty == Tarray)
	{
	  Type *pointer_type = tbtype->nextOf()->pointerTo();
	  return darrayVal (target_type, 0, nop (pointer_type->toCtype(), exp));
	}
      break;

    case Tvector:
      if (tbtype->ty == Tsarray)
	{
	  if (tbtype->size() == ebtype->size())
	    return vconvert (target_type->toCtype(), exp);
	}
      break;

    default:
	{
	  if ((ebtype->isreal() && tbtype->isimaginary())
	      || (ebtype->isimaginary() && tbtype->isreal()))
	    {
	      // warn? handle in front end?
	      result = build_real_from_int_cst (target_type->toCtype(), integer_zero_node);
	      if (TREE_SIDE_EFFECTS (exp))
		result = compound (exp, result);
	      return result;
	    }
	  else if (ebtype->iscomplex())
	    {
	      Type *part_type;
	      // creal.re, .im implemented by cast to real or ireal
	      // Assumes target type is the same size as the original's components size
	      if (tbtype->isreal())
		{
		  // maybe install lang_specific...
		  switch (ebtype->ty)
		    {
		    case Tcomplex32:
		      part_type = Type::tfloat32;
		      break;

		    case Tcomplex64:
		      part_type = Type::tfloat64;
		      break;

		    case Tcomplex80:
		      part_type = Type::tfloat80;
		      break;

		    default:
		      gcc_unreachable();
		    }
		  result = realPart (exp);
		}
	      else if (tbtype->isimaginary())
		{
		  switch (ebtype->ty)
		    {
		    case Tcomplex32:
		      part_type = Type::timaginary32;
		      break;

		    case Tcomplex64:
		      part_type = Type::timaginary64;
		      break;

		    case Tcomplex80:
		      part_type = Type::timaginary80;
		      break;

		    default:
		      gcc_unreachable();
		    }
		  result = imagPart (exp);
		}
	      else
		{
		  // default conversion
		  break;
		}
	      result = convertTo (result, part_type, target_type);
	    }
	  else if (tbtype->iscomplex())
	    {
	      tree c1 = convertTo (TREE_TYPE (target_type->toCtype()), exp);
	      tree c2 = build_real_from_int_cst (TREE_TYPE (target_type->toCtype()), integer_zero_node);

	      if (ebtype->isreal())
		{
		  // nothing
		}
	      else if (ebtype->isimaginary())
		{
		  tree swap = c1;
		  c1 = c2;
		  c2 = swap;
		}
	      else
		{
		  // default conversion
		  break;
		}
	      result = build2 (COMPLEX_EXPR, target_type->toCtype(), c1, c2);
	    }
	  else
	    {
	      gcc_assert (TREE_CODE (exp) != STRING_CST);
	      // default conversion
	    }
	}
    }

  if (!result)
    result = d_convert_basic (target_type->toCtype(), exp);
#if ENABLE_CHECKING
  if (isErrorMark (result))
    error ("type: %s, target: %s", exp_type->toChars(), target_type->toChars());
#endif
  return result;
}


// Apply semantics of assignment to a values of type TARGET_TYPE to EXPR
// (e.g., pointer = array -> pointer = &array[0])

// Return a TREE representation of EXPR implictly converted to TARGET_TYPE
// for use in assignment expressions MODIFY_EXPR, INIT_EXPR...

tree
IRState::convertForAssignment (Expression *expr, Type *target_type)
{
  Type *exp_base_type = expr->type->toBasetype();
  Type *target_base_type = target_type->toBasetype();
  tree exp_tree = NULL_TREE;

  // Assuming this only has to handle converting a non Tsarray type to
  // arbitrarily dimensioned Tsarrays.
  if (target_base_type->ty == Tsarray)
    {
      Type *sa_elem_type = target_base_type->nextOf()->toBasetype();

      while (sa_elem_type->ty == Tsarray)
	sa_elem_type = sa_elem_type->nextOf()->toBasetype();

      if (typesCompatible (sa_elem_type, exp_base_type))
	{
	  // %% what about implicit converions...?
	  TypeSArray *sa_type = (TypeSArray *) target_base_type;
	  uinteger_t count = sa_type->dim->toUInteger();

	  tree ctor = build_constructor (target_type->toCtype(), 0);
	  if (count)
	    {
	      CtorEltMaker ce;
	      ce.cons (build2 (RANGE_EXPR, Type::tsize_t->toCtype(),
			       integer_zero_node, integerConstant (count - 1)),
		       g.ofile->stripVarDecl (convertForAssignment (expr, sa_type->next)));
	      CONSTRUCTOR_ELTS (ctor) = ce.head;
	    }
	  TREE_READONLY (ctor) = 1;
	  TREE_CONSTANT (ctor) = 1;
	  return ctor;
	}
    }

  if (!target_type->isscalar() && exp_base_type->isintegral())
    {
      // D Front end uses IntegerExp (0) to mean zero-init a structure
      // This could go in convert for assignment, but we only see this for
      // internal init code -- this also includes default init for _d_newarrayi...
      if (expr->toInteger() == 0)
	{
	  tree empty = build_constructor (target_type->toCtype(), NULL);
	  TREE_CONSTANT (empty) = 1;
	  TREE_STATIC (empty) = 1;
	  return empty;
	}
	
      gcc_unreachable();
    }

  exp_tree = expr->toElem (this);
  return convertForAssignment (exp_tree, expr->type, target_type);
}

// Return expression EXPR, whose type has been convert from EXPR_TYPE to TARGET_TYPE.

tree
IRState::convertForAssignment (tree expr, Type *expr_type, Type *target_type)
{
  return convertTo (expr, expr_type, target_type);
}

// Return a TREE representation of EXPR converted to represent parameter type ARG.

tree
IRState::convertForArgument (Expression *expr, Parameter *arg)
{
  if (isArgumentReferenceType (arg))
    {
      tree exp_tree = this->toElemLvalue (expr);
      // front-end already sometimes automatically takes the address
      // TODO: Make this safer?  Can this be confused by a non-zero SymOff?
      if (expr->op != TOKaddress && expr->op != TOKsymoff && expr->op != TOKadd)
	return addressOf (exp_tree);
      else
	return exp_tree;
    }
  else
    {
      // Lazy arguments: expr should already be a delegate
      tree exp_tree = expr->toElem (this);
      return exp_tree;
    }
}

// Return a TREE representation of EXPR implictly converted to
// BOOLEAN_TYPE for use in conversion expressions.

tree
IRState::convertForCondition (Expression *expr)
{
  return convertForCondition (expr->toElem (this), expr->type);
}

// Perform default promotions for data used in expressions.
// Arrays and functions are converted to pointers;
// enumeral types or short or char, to int.
// In addition, manifest constants symbols are replaced by their values.

// Return truth-value conversion of expression EXPR from value type EXP_TYPE.

tree
IRState::convertForCondition (tree exp_tree, Type *exp_type)
{
  tree result = NULL_TREE;
  tree obj, func, tmp;

  switch (exp_type->toBasetype()->ty)
    {
    case Taarray:
      // Shouldn't this be...
      //  result = libCall (LIBCALL_AALEN, 1, &exp_tree);
      result = component (exp_tree, TYPE_FIELDS (TREE_TYPE (exp_tree)));
      break;

    case Tarray:
      // DMD checks (length || ptr) (i.e ary !is null)
      tmp = maybeMakeTemp (exp_tree);
      obj = delegateObjectRef (tmp);
      func = delegateMethodRef (tmp);
      if (TYPE_MODE (TREE_TYPE (obj)) == TYPE_MODE (TREE_TYPE (func)))
	{
	  result = build2 (BIT_IOR_EXPR, TREE_TYPE (obj), obj,
			   convertTo (TREE_TYPE (obj), func));
	}
      else
	{
	  obj = d_truthvalue_conversion (obj);
	  func = d_truthvalue_conversion (func);
	  // probably not worth using TRUTH_OROR ...
	  result = build2 (TRUTH_OR_EXPR, TREE_TYPE (obj), obj, func);
	}
      break;

    case Tdelegate:
      // DMD checks (function || object), but what good
      // is if if there is a null function pointer?
      if (D_IS_METHOD_CALL_EXPR (exp_tree))
	{
	  extractMethodCallExpr (exp_tree, obj, func);
	}
      else
	{
	  tmp = maybeMakeTemp (exp_tree);
	  obj = delegateObjectRef (tmp);
	  func = delegateMethodRef (tmp);
	}
      obj = d_truthvalue_conversion (obj);
      func = d_truthvalue_conversion (func);
      // probably not worth using TRUTH_ORIF ...
      result = build2 (BIT_IOR_EXPR, TREE_TYPE (obj), obj, func);
      break;

    default:
      result = exp_tree;
      break;
    }

  return d_truthvalue_conversion (result);
}


// Convert EXP to a dynamic array.
// EXP must be a static array or dynamic array.

tree
IRState::toDArray (Expression *exp)
{
  TY ty = exp->type->toBasetype()->ty;
  tree val;
  if (ty == Tsarray)
    val = convertTo (exp, exp->type->toBasetype()->nextOf()->arrayOf());
  else if (ty == Tarray)
    val = exp->toElem (this);
  else
    {
      gcc_assert (ty == Tsarray || ty == Tarray);
      return NULL_TREE;
    }
  return val;
}

// Type management for D frontend types.
// Returns TRUE if T1 and T2 are mutably the same type.

bool
IRState::typesSame (Type *t1, Type *t2)
{
  return t1->mutableOf()->equals (t2->mutableOf());
}

// Returns TRUE if T1 and T2 don't require special conversions.

bool
IRState::typesCompatible (Type *t1, Type *t2)
{
  return t1->implicitConvTo (t2) >= MATCHconst;
}

// Returns D Frontend type for GCC type T.

Type *
IRState::getDType (tree t)
{
  struct lang_type *l;

  gcc_assert (TYPE_P (t));
  l = TYPE_LANG_SPECIFIC (t);
  return l ? l->d_type : NULL;
}

// Returns D frontend type 'Object' which all classes
// are derived from.

Type *
IRState::getObjectType()
{
  if (ClassDeclaration::object)
    return ClassDeclaration::object->type;

  ::error ("missing or corrupt object.d");
  return Type::terror;
}

// Return TRUE if declaration DECL is a reference type.

bool
IRState::isDeclarationReferenceType (Declaration *decl)
{
  Type *base_type = decl->type->toBasetype();
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

// Returns the real type for declaration DECL.
// Reference decls are converted to reference-to-types.
// Lazy decls are converted into delegates.

tree
IRState::trueDeclarationType (Declaration *decl)
{
  tree decl_type = decl->type->toCtype();
  if (isDeclarationReferenceType (decl))
    {
      decl_type = build_reference_type (decl_type);
      if (decl->isParameter())
	D_TYPE_ADDRESSABLE (decl_type) = 1;
    }
  else if (decl->storage_class & STClazy)
    {
      TypeFunction *tf = new TypeFunction (NULL, decl->type, false, LINKd);
      TypeDelegate *t = new TypeDelegate (tf);
      decl_type = t->merge()->toCtype();
    }
  return decl_type;
}


// These should match the Declaration versions above

// Return TRUE if parameter ARG is a reference type.

bool
IRState::isArgumentReferenceType (Parameter *arg)
{
  Type *base_type = arg->type->toBasetype();

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

// Returns the real type for parameter ARG.
// Reference parameters are converted to reference-to-types.
// Lazy parameters are converted into delegates.

tree
IRState::trueArgumentType (Parameter *arg)
{
  tree arg_type = arg->type->toCtype();
  if (isArgumentReferenceType (arg))
    {
      arg_type = build_reference_type (arg_type);
    }
  else if (arg->storageClass & STClazy)
    {
      TypeFunction *tf = new TypeFunction (NULL, arg->type, false, LINKd);
      TypeDelegate *t = new TypeDelegate (tf);
      arg_type = t->merge()->toCtype();
    }
  return arg_type;
}

// Returns an array of type D_TYPE which has SIZE number of elements.

tree
IRState::arrayType (Type *d_type, uinteger_t size)
{
  return arrayType (d_type->toCtype(), size);
}

// Returns an array of type TYPE_NODE which has SIZE number of elements.

tree
IRState::arrayType (tree type_node, uinteger_t size)
{
  tree index_type_node;
  if (size > 0)
    {
      index_type_node = size_int (size - 1);
      index_type_node = build_index_type (index_type_node);
    }
  else
    {
      // See c-decl.c grokdeclarator for zero-length arrays
      index_type_node = build_range_type (sizetype, size_zero_node,
					  NULL_TREE);
    }

  tree array_type = build_array_type (type_node, index_type_node);
  if (size == 0)
    {
      TYPE_SIZE (array_type) = bitsize_zero_node;
      TYPE_SIZE_UNIT (array_type) = size_zero_node;
    }
  return array_type;
}

// Appends the type attribute ATTRNAME with value VALUE onto type TYPE.

tree
IRState::addTypeAttribute (tree type, const char *attrname, tree value)
{
  // types built by functions in tree.c need to be treated as immutable
  if (!TYPE_ATTRIBUTES (type))
    type = build_variant_type_copy (type);

  if (value)
    value = tree_cons (NULL_TREE, value, NULL_TREE);
  TYPE_ATTRIBUTES (type) = tree_cons (get_identifier (attrname), value,
				      TYPE_ATTRIBUTES (type));
  return type;
}

// Appends the decl attribute ATTRNAME with value VALUE onto decl DECL.

void
IRState::addDeclAttribute (tree decl, const char *attrname, tree value)
{
  tree ident = get_identifier (attrname);
  if (value)
    value = tree_cons (NULL_TREE, value, NULL_TREE);

  DECL_ATTRIBUTES (decl) = tree_cons (ident, value, DECL_ATTRIBUTES (decl));
}

// Return chain of all GCC attributes found in list IN_ATTRS.

tree
IRState::attributes (Expressions *in_attrs)
{
  if (!in_attrs)
    return NULL_TREE;

  ListMaker out_attrs;

  for (size_t i = 0; i < in_attrs->dim; i++)
    {
      Expression *e = in_attrs->tdata()[i];
      IdentifierExp *ident_e = NULL;

      ListMaker args;

      if (e->op == TOKidentifier)
	ident_e = (IdentifierExp *) e;
      else if (e->op == TOKcall)
	{
	  CallExp *c = (CallExp *) e;
	  gcc_assert (c->e1->op == TOKidentifier);
	  ident_e = (IdentifierExp *) c->e1;

	  if (c->arguments)
	    {
	      for (size_t ai = 0; ai < c->arguments->dim; ai++)
		{
		  Expression *ae = c->arguments->tdata()[ai];
		  tree aet;
		  if (ae->op == TOKstring && ((StringExp *) ae)->sz == 1)
		    {
		      StringExp *s = (StringExp *) ae;
		      aet = build_string (s->len, (const char *) s->string);
		    }
		  else
		    aet = ae->toElem (&gen);
		  args.cons (aet);
		}
	    }
	}
      else
	{
	  gcc_unreachable();
	  continue;
	}
      out_attrs.cons (get_identifier (ident_e->ident->string), args.head);
    }

  return out_attrs.head;
}

// Return qualified type variant of TYPE determined by modifier value MOD.

tree
IRState::addTypeModifiers (tree type, unsigned mod)
{
  int quals = 0;
  gcc_assert (type);

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

  return build_qualified_type (type, quals);
}

// Build INTEGER_CST of type TYPE with the value VALUE.

tree
IRState::integerConstant (dinteger_t value, Type *type)
{
  return integerConstant (value, type->toCtype());
}

tree
IRState::integerConstant (dinteger_t value, tree type)
{
  // The type is error_mark_node, we can't do anything.
  if (isErrorMark (type))
    return type;

  return build_int_cst_type (type, value);
}

// Build REAL_CST of type TARGET_TYPE with the value VALUE.

tree
IRState::floatConstant (const real_t& value, Type *target_type)
{
  real_t new_value;
  TypeBasic *tb = target_type->isTypeBasic();

  gcc_assert (tb != NULL);

  tree type_node = tb->toCtype();
  real_convert (&new_value.rv(), TYPE_MODE (type_node), &value.rv());

  if (new_value > value)
    {
      // value grew as a result of the conversion. %% precision bug ??
      // For now just revert back to original.
      new_value = value;
    }

  return build_real (type_node, new_value.rv());
}

// Convert LOW / HIGH pair into dinteger_t type.

dinteger_t
IRState::hwi2toli (HOST_WIDE_INT low, HOST_WIDE_INT high)
{
  if (high == 0 || (high == -1 && low < 0))
    return low;
  else if (low == 0 && high == 1)
    return (~(dinteger_t) 0);

  gcc_unreachable();
}

// Convert CST into into dinteger_t type.

dinteger_t
IRState::hwi2toli (double_int cst)
{
  return hwi2toli (cst.low, cst.high);
}

// Returns the REAPART of COMPLEX_CST C.

tree
IRState::realPart (tree c)
{
  return build1 (REALPART_EXPR, TREE_TYPE (TREE_TYPE (c)), c);
}

// Returns the IMAGPART of COMPLEX_CST C.

tree
IRState::imagPart (tree c)
{
  return build1 (IMAGPART_EXPR, TREE_TYPE (TREE_TYPE (c)), c);
}

// Returns the .length component from the D dynamic array EXP.

tree
IRState::darrayLenRef (tree exp)
{
  // backend will ICE otherwise
  if (isErrorMark (exp))
    return exp;

  // Get the backend type for the array and pick out the array
  // length field (assumed to be the first field.)
  tree len_field = TYPE_FIELDS (TREE_TYPE (exp));
  return component (exp, len_field);
}

// Returns the .ptr component from the D dynamic array EXP.

tree
IRState::darrayPtrRef (tree exp)
{
  // backend will ICE otherwise
  if (isErrorMark (exp))
    return exp;

  // Get the backend type for the array and pick out the array
  // data pointer field (assumed to be the second field.)
  tree ptr_field = TREE_CHAIN (TYPE_FIELDS (TREE_TYPE (exp)));
  return component (exp, ptr_field);
}

// Builds D dynamic array expression E and returns the .ptr component.

tree
IRState::darrayPtrRef (Expression *e)
{
  return darrayPtrRef (e->toElem (this));
}

// Returns a constructor for D dynamic array type TYPE of .length LEN
// and .ptr pointing to DATA.

tree
IRState::darrayVal (tree type, tree len, tree data)
{
  // %% assert type is a darray
  tree len_field, ptr_field;
  CtorEltMaker ce;

  len_field = TYPE_FIELDS (type);
  ptr_field = TREE_CHAIN (len_field);

  ce.cons (len_field, len);
  ce.cons (ptr_field, data); // shouldn't need to convert the pointer...

  tree ctor = build_constructor (type, ce.head);
  TREE_STATIC (ctor) = 0;   // can be set by caller if needed
  TREE_CONSTANT (ctor) = 0; // "

  return ctor;
}

tree
IRState::darrayVal (Type *type, uinteger_t len, tree data)
{
  return darrayVal (type->toCtype(), len, data);
}

tree
IRState::darrayVal (tree type, uinteger_t len, tree data)
{
  // %% assert type is a darray
  tree len_value, ptr_value, len_field, ptr_field;
  CtorEltMaker ce;

  len_field = TYPE_FIELDS (type);
  ptr_field = TREE_CHAIN (len_field);

  if (data)
    {
      gcc_assert (POINTER_TYPE_P (TREE_TYPE (data)));
      ptr_value = data;
    }
  else
    {
      ptr_value = convertTo (TREE_TYPE (ptr_field), d_null_pointer);
    }

  len_value = integerConstant (len, TREE_TYPE (len_field));
  ce.cons (len_field, len_value);
  ce.cons (ptr_field, ptr_value); // shouldn't need to convert the pointer...

  tree ctor = build_constructor (type, ce.head);
  // TREE_STATIC and TREE_CONSTANT can be set by caller if needed
  TREE_STATIC (ctor) = 0;
  TREE_CONSTANT (ctor) = 0;

  return ctor;
}

// Builds a D string value from the C string STR.

tree
IRState::darrayString (const char *str)
{
  unsigned len = strlen (str);
  // %% assumes str is null-terminated
  tree str_tree = build_string (len + 1, str);

  TREE_TYPE (str_tree) = arrayType (Type::tchar, len);
  return darrayVal (Type::tchar->arrayOf()->toCtype(), len, addressOf (str_tree));
}

// Returns array length of expression EXP.

tree
IRState::arrayLength (Expression *exp)
{
  return arrayLength (exp->toElem (this), exp->type);
}

// Returns value representing the array length of expression EXP.
// EXP_TYPE could be a dynamic or static array.

tree
IRState::arrayLength (tree exp, Type *exp_type)
{
  Type *base_type = exp_type->toBasetype();
  switch (base_type->ty)
    {
    case Tsarray:
      return size_int (((TypeSArray *) base_type)->dim->toUInteger());
    case Tarray:
      return darrayLenRef (exp);
    default:
      ::error ("can't determine the length of a %s", exp_type->toChars());
      return error_mark_node;
    }
}

// Returns the .funcptr component from the D delegate EXP.

tree
IRState::delegateMethodRef (tree exp)
{
  // Get the backend type for the array and pick out the array length
  // field (assumed to be the second field.)
  tree method_field = TREE_CHAIN (TYPE_FIELDS (TREE_TYPE (exp)));
  return component (exp, method_field);
}

// Returns the .object component from the D delegate EXP.

tree
IRState::delegateObjectRef (tree exp)
{
  // Get the backend type for the array and pick out the array data
  // pointer field (assumed to be the first field.)
  tree obj_field = TYPE_FIELDS (TREE_TYPE (exp));
  return component (exp, obj_field);
}

// Converts pointer types of METHOD_EXP and OBJECT_EXP to match D_TYPE.

tree
IRState::delegateVal (tree method_exp, tree object_exp, Type *d_type)
{
  Type *base_type = d_type->toBasetype();
  if (base_type->ty == Tfunction)
    {
      // Called from DotVarExp.  These are just used to
      // make function calls and not to make Tdelegate variables.
      // Clearing the type makes sure of this.
      base_type = 0;
    }
  else
    {
      gcc_assert (base_type->ty == Tdelegate);
    }

  tree type = base_type ? base_type->toCtype() : NULL_TREE;
  tree ctor = make_node (CONSTRUCTOR);
  tree obj_field = NULL_TREE;
  tree func_field = NULL_TREE;
  CtorEltMaker ce;

  if (type)
    {
      TREE_TYPE (ctor) = type;
      obj_field = TYPE_FIELDS (type);
      func_field = TREE_CHAIN (obj_field);
    }
  ce.cons (obj_field, object_exp);
  ce.cons (func_field, method_exp);

  CONSTRUCTOR_ELTS (ctor) = ce.head;
  return ctor;
}

// Builds a temporary tree to store the CALLEE and OBJECT
// of a method call expression of type D_TYPE.

tree
IRState::methodCallExpr (tree callee, tree object, Type *d_type)
{
  tree t = delegateVal (callee, object, d_type);
  D_IS_METHOD_CALL_EXPR (t) = 1;
  return t;
}

// Extract callee and object from MCR and return in to CALLEE_OUT and OBJECT_OUT.

void
IRState::extractMethodCallExpr (tree mcr, tree& callee_out, tree& object_out)
{
  gcc_assert (D_IS_METHOD_CALL_EXPR (mcr));

  VEC (constructor_elt,gc) *elts = CONSTRUCTOR_ELTS (mcr);
  object_out = VEC_index (constructor_elt, elts, 0)->value;
  callee_out = VEC_index (constructor_elt, elts, 1)->value;
}

// Return correct callee for method FUNC, which is dereferenced from
// the 'this' pointer OBJ_EXP.  D_TYPE is the return type for the method.

tree
IRState::objectInstanceMethod (Expression *obj_exp, FuncDeclaration *func, Type *d_type)
{
  Type *obj_type = obj_exp->type->toBasetype();
  if (func->isThis())
    {
      bool is_dottype;
      tree this_expr;

      if (obj_exp->op == TOKdottype)
	{
	  is_dottype = true;
	  this_expr = obj_exp->toElem (this);
	}
      else if (obj_exp->op == TOKcast &&
	       ((CastExp *) obj_exp)->e1->op == TOKdottype)
	{
	  is_dottype = true;
	  this_expr = ((CastExp *) obj_exp)->e1->toElem (this);
	}
      else
	{
	  is_dottype = false;
	  this_expr = obj_exp->toElem (this);
	}

      // Calls to super are static (func is the super's method)
      // Structs don't have vtables.
      // Final and non-virtual methods can be called directly.
      // DotTypeExp means non-virtual

      if (obj_exp->op == TOKsuper ||
	  obj_type->ty == Tstruct || obj_type->ty == Tpointer ||
	  func->isFinal() || !func->isVirtual() || is_dottype)
	{
	  if (obj_type->ty == Tstruct)
	    this_expr = addressOf (this_expr);
	  return methodCallExpr (addressOf (func), this_expr, d_type);
	}
      else
	{
	  // Interface methods are also in the class's vtable, so we don't
	  // need to convert from a class pointer to an interface pointer.
	  this_expr = maybeMakeTemp (this_expr);

	  tree vtbl_ref;
	  /* Folding of *&<static var> fails because of the type of the
	     address expression is 'Object' while the type of the static
	     var is a particular class (why?). This prevents gimplification
	     of the expression.
	   */
	  if (TREE_CODE (this_expr) == ADDR_EXPR)
	    vtbl_ref = TREE_OPERAND (this_expr, 0);
	  else
	    vtbl_ref = indirect (this_expr);

	  tree field = TYPE_FIELDS (TREE_TYPE (vtbl_ref)); // the vtbl is the first field
	  vtbl_ref = component (vtbl_ref, field); // vtbl field (a pointer)
	  // %% better to do with array ref?
	  vtbl_ref = pointerOffset (vtbl_ref,
				   size_int (PTRSIZE * func->vtblIndex));
	  vtbl_ref = indirect (TREE_TYPE (addressOf (func)), vtbl_ref);

	  return methodCallExpr (vtbl_ref, this_expr, d_type);
	}
    }
  else
    {
      // Static method; ignore the object instance
      return addressOf (func);
    }
}


// Builds a record type from field types FT1 and FT2.
// D_TYPE is the D frontend type we are building.
// N1 and N2 are the names of the two fields.

tree
IRState::twoFieldType (tree ft1, tree ft2, Type *d_type, const char *n1, const char *n2)
{
  tree rec_type = make_node (RECORD_TYPE);
  tree f0 = build_decl (BUILTINS_LOCATION, FIELD_DECL, get_identifier (n1), ft1);
  tree f1 = build_decl (BUILTINS_LOCATION, FIELD_DECL, get_identifier (n2), ft2);
  DECL_CONTEXT (f0) = rec_type;
  DECL_CONTEXT (f1) = rec_type;
  TYPE_FIELDS (rec_type) = chainon (f0, f1);
  layout_type (rec_type);
  if (d_type)
    {
      /* This is needed so that maybeExpandSpecialCall knows to
	 split dynamic array varargs. */
      TYPE_LANG_SPECIFIC (rec_type) = build_d_type_lang_specific (d_type);

      /* ObjectFile::declareType will try to declare it as top-level type
	 which can break debugging info for element types. */
      tree stub_decl = build_decl (BUILTINS_LOCATION, TYPE_DECL,
				   get_identifier (d_type->toChars()), rec_type);
      TYPE_STUB_DECL (rec_type) = stub_decl;
      TYPE_NAME (rec_type) = stub_decl;
      DECL_ARTIFICIAL (stub_decl) = 1;
      rest_of_decl_compilation (stub_decl, 0, 0);
    }
  return rec_type;
}

tree
IRState::twoFieldType (Type *ft1, Type *ft2, Type *d_type, const char *n1, const char *n2)
{
  return twoFieldType (ft1->toCtype(), ft2->toCtype(), d_type, n1, n2);
}

// Builds a record constructor from two field types F1 and F2.
// STORAGE_CLASS tells us whether constructor is static / manifest.

tree
IRState::twoFieldCtor (tree f1, tree f2, int storage_class)
{
  tree rec_type = make_node (RECORD_TYPE);
  CtorEltMaker ce;
  ce.cons (TYPE_FIELDS (rec_type), f1);
  ce.cons (TREE_CHAIN (TYPE_FIELDS (rec_type)), f2);

  tree ctor = build_constructor (rec_type, ce.head);
  TREE_STATIC (ctor) = (storage_class & STCstatic) != 0;
  TREE_CONSTANT (ctor) = (storage_class & STCconst) != 0;
  TREE_READONLY (ctor) = (storage_class & STCconst) != 0;

  return ctor;
}

tree
IRState::makeTemp (tree t)
{
  if (TREE_CODE (TREE_TYPE (t)) != ARRAY_TYPE)
    return save_expr (t);
  else
    return stabilize_reference (t);
}

tree
IRState::maybeMakeTemp (tree t)
{
  if (!isFreeOfSideEffects (t))
    return makeTemp (t);

  return t;
}

// Return TRUE if T is free of side effects.

bool
IRState::isFreeOfSideEffects (tree expr)
{
  tree t = STRIP_NOPS (expr);

  // SAVE_EXPR is safe to reference more than once, but not to
  // expand in a loop.
  if (TREE_CODE (t) == SAVE_EXPR)
    return true;

  if (DECL_P (t) ||
      CONSTANT_CLASS_P (t) ||
      EXCEPTIONAL_CLASS_P (t))
    return true;

  if (INDIRECT_REF_P (t) ||
      TREE_CODE (t) == ADDR_EXPR ||
      TREE_CODE (t) == COMPONENT_REF)
    return isFreeOfSideEffects (TREE_OPERAND (t, 0));

  return !TREE_SIDE_EFFECTS (t);
}

// Evaluates expression E as an Lvalue.

tree
IRState::toElemLvalue (Expression *e)
{
  if (e->op == TOKindex)
    {
      IndexExp *ie = (IndexExp *) e;
      Expression *e1 = ie->e1;
      Expression *e2 = ie->e2;
      Type *type = e->type;
      Type *array_type = e1->type->toBasetype();

      if (array_type->ty == Taarray)
	{
	  Type *key_type = ((TypeAArray *) array_type)->index->toBasetype();
	  AddrOfExpr aoe;

	  tree args[4];
	  args[0] = this->addressOf (this->toElemLvalue (e1));
	  args[1] = this->typeinfoReference (key_type);
	  args[2] = this->integerConstant (array_type->nextOf()->size(), Type::tsize_t);
	  args[3] = aoe.set (this, this->convertTo (e2, key_type));
	  return build1 (INDIRECT_REF, type->toCtype(),
			 aoe.finish (this,
				     this->libCall (LIBCALL_AAGETX, 4, args, type->pointerTo()->toCtype())));
	}
    }
  return e->toElem (this);
}

// Returns the address of symbol D.

tree
IRState::addressOf (Dsymbol *d)
{
  return addressOf (d->toSymbol()->Stree);
}

// Returns the address of the expression EXP.

tree
IRState::addressOf (tree exp)
{
  tree t, ptrtype;
  tree exp_type = TREE_TYPE (exp);
  d_mark_addressable (exp);

  // Gimplify doesn't like &(* (ptr-to-array-type)) with static arrays
  if (TREE_CODE (exp) == INDIRECT_REF)
    {
      t = TREE_OPERAND (exp, 0);
      ptrtype = build_pointer_type (exp_type);
      t = nop (ptrtype, t);
    }
  else
    {
      /* Just convert string literals (char[]) to C-style strings (char *), otherwise
	 the latter method (char[]*) causes conversion problems during gimplification. */
      if (TREE_CODE (exp) == STRING_CST)
	{
	  ptrtype = build_pointer_type (TREE_TYPE (exp_type));
	}
      /* Special case for va_list. The backends will be expecting a pointer to vatype,
       * but some targets use an array. So fix it.  */
      else if (TYPE_MAIN_VARIANT (exp_type) == TYPE_MAIN_VARIANT (va_list_type_node))
	{
	  if (TREE_CODE (TYPE_MAIN_VARIANT (exp_type)) == ARRAY_TYPE)
	    ptrtype = build_pointer_type (TREE_TYPE (exp_type));
	  else
	    ptrtype = build_pointer_type (exp_type);
	}
      else
	ptrtype = build_pointer_type (exp_type);

      t = build1 (ADDR_EXPR, ptrtype, exp);
    }
  if (TREE_CODE (exp) == FUNCTION_DECL)
    TREE_NO_TRAMPOLINE (t) = 1;

  return t;
}

// Cast EXP (which should be a pointer) to TYPE * and then indirect.  The
// back-end requires this cast in many cases.

tree
IRState::indirect (tree type, tree exp)
{
  if (TREE_CODE (TREE_TYPE (exp)) == REFERENCE_TYPE)
    return build1 (INDIRECT_REF, type, exp);

  return build1 (INDIRECT_REF, type,
		 nop (build_pointer_type (type), exp));
}

// Returns indirect reference of EXP, which must be a pointer type.

tree
IRState::indirect (tree exp)
{
  tree type = TREE_TYPE (exp);
  gcc_assert (POINTER_TYPE_P (type));

  return build1 (INDIRECT_REF, TREE_TYPE (type), exp);
}

// Builds pointer offset expression PTR_EXP[IDX_EXP]

tree
IRState::pointerIntSum (Expression *ptr_exp, Expression *idx_exp)
{
  return pointerIntSum (ptr_exp->toElem (this), idx_exp->toElem (this));
}

tree
IRState::pointerIntSum (tree ptr_node, tree idx_exp)
{
  tree result_type_node = TREE_TYPE (ptr_node);
  tree elem_type_node = TREE_TYPE (result_type_node);
  tree intop = idx_exp;
  tree size_exp;

  tree prod_result_type;
  prod_result_type = sizetype;

  size_exp = size_in_bytes (elem_type_node); // array element size

  if (integer_zerop (size_exp))
    {
      // Test for void case...
      if (TYPE_MODE (elem_type_node) == TYPE_MODE (void_type_node))
	intop = fold_convert (prod_result_type, intop);
      else
	{
	  // FIXME: should catch this earlier.
	  error ("invalid use of incomplete type %qD", TYPE_NAME (elem_type_node));
	  result_type_node = error_mark_node;
	}
    }
  else if (integer_onep (size_exp))
    {
      // ...or byte case -- No need to multiply.
      intop = fold_convert (prod_result_type, intop);
    }
  else
    {
      if (TYPE_PRECISION (TREE_TYPE (intop)) != TYPE_PRECISION (sizetype)
	  || TYPE_UNSIGNED (TREE_TYPE (intop)) != TYPE_UNSIGNED (sizetype))
	{
	  intop = convertTo (d_type_for_size (TYPE_PRECISION (sizetype),
					      TYPE_UNSIGNED (sizetype)), intop);
	}
      intop = fold_convert (prod_result_type,
			    fold_build2 (MULT_EXPR, TREE_TYPE (size_exp), // the type here may be wrong %%
					 intop, convertTo (TREE_TYPE (intop), size_exp)));
    }

  // backend will ICE otherwise
  if (isErrorMark (result_type_node))
    return result_type_node;

  if (integer_zerop (intop))
    return ptr_node;

  return build2 (POINTER_PLUS_EXPR, result_type_node, ptr_node, intop);
}

// Builds pointer offset expression *(PTR OP IDX)
// OP could be a plus or minus expression.

tree
IRState::pointerOffsetOp (enum tree_code op, tree ptr, tree idx)
{
  gcc_assert (op == MINUS_EXPR || op == PLUS_EXPR);

  if (op == MINUS_EXPR)
    idx = fold_build1 (NEGATE_EXPR, sizetype, idx);

  return build2 (POINTER_PLUS_EXPR, TREE_TYPE (ptr), ptr,
		 convertTo (sizetype, idx));
}

tree
IRState::pointerOffset (tree ptr_node, tree byte_offset)
{
  tree ofs = fold_convert (sizetype, byte_offset);
  return fold_build2 (POINTER_PLUS_EXPR, TREE_TYPE (ptr_node), ptr_node, ofs);
}


// Implicitly converts void* T to byte* as D allows { void[] a; &a[3]; }

tree
IRState::pvoidOkay (tree t)
{
  if (VOID_TYPE_P (TREE_TYPE (TREE_TYPE (t))))
    {
      // ::warning ("indexing array of void");
      return convertTo (Type::tuns8->pointerTo()->toCtype(), t);
    }
  return t;
}


// Builds an array bounds checking condition, returning INDEX if true,
// else throws a RangeError exception.

tree
IRState::checkedIndex (Loc loc, tree index, tree upper_bound, bool inclusive)
{
  if (arrayBoundsCheck())
    {
      return build3 (COND_EXPR, TREE_TYPE (index),
		     this->boundsCond (index, upper_bound, inclusive),
		     index, this->assertCall (loc, LIBCALL_ARRAY_BOUNDS));
    }
  else
    {
      return index;
    }
}

// Builds the condition [INDEX < UPPER_BOUND] and optionally [INDEX >= 0]
// if INDEX is a signed type.  For use in array bound checking routines.
// If INCLUSIVE, we allow equality to return true also.
// INDEX must be wrapped in a SAVE_EXPR to prevent multiple evaluation...

tree
IRState::boundsCond (tree index, tree upper_bound, bool inclusive)
{
  tree bound_check;

  bound_check = build2 (inclusive ? LE_EXPR : LT_EXPR, boolean_type_node,
			convertTo (d_unsigned_type (TREE_TYPE (index)), index),
			upper_bound);

  if (!TYPE_UNSIGNED (TREE_TYPE (index)))
    {
      bound_check = build2 (TRUTH_ANDIF_EXPR, boolean_type_node, bound_check,
			    // %% conversions
			    build2 (GE_EXPR, boolean_type_node, index, integer_zero_node));
    }
  return bound_check;
}

// Returns TRUE if array bounds checking code generation is turned on.

int
IRState::arrayBoundsCheck (void)
{
  int result = global.params.useArrayBounds;

  if (result == 1)
    {
      // For D2 safe functions only
      result = 0;
      if (this->func && this->func->type->ty == Tfunction)
	{
	  TypeFunction *tf = (TypeFunction *)this->func->type;
	  if (tf->trust == TRUSTsafe)
	    result = 1;
	}
    }

  return result;
}

// Builds an array index expression from AE.  ASC may build a
// BIND_EXPR if temporaries were created for bounds checking.

tree
IRState::arrayElemRef (IndexExp *ae, ArrayScope *asc)
{
  Expression *e1 = ae->e1;
  Expression *e2 = ae->e2;

  Type *base_type = e1->type->toBasetype();
  TY base_type_ty = base_type->ty;
  // expression that holds the array data.
  tree array_expr = e1->toElem (this);
  // expression that indexes the array data
  tree subscript_expr = e2->toElem (this);
  // base pointer to the elements
  tree ptr_exp;
  // reference the the element
  tree elem_ref;

  switch (base_type_ty)
    {
    case Tarray:
    case Tsarray:
      array_expr = asc->setArrayExp (this, array_expr, e1->type);

      // If it's a static array and the index is constant,
      // the front end has already checked the bounds.
      if (arrayBoundsCheck() &&
	  !(base_type_ty == Tsarray && e2->isConst()))
	{
	  tree array_len_expr;
	  // implement bounds check as a conditional expression:
	  // array [inbounds(index) ? index : { throw ArrayBoundsError }]

	  // First, set up the index expression to only be evaluated once.
	  tree index_expr = maybeMakeTemp (subscript_expr);

	  if (base_type_ty == Tarray)
	    {
	      array_expr = maybeMakeTemp (array_expr);
	      array_len_expr = darrayLenRef (array_expr);
	    }
	  else
	    array_len_expr = ((TypeSArray *) base_type)->dim->toElem (this);

	  subscript_expr = checkedIndex (ae->loc, index_expr,
					 array_len_expr, false);
	}

      if (base_type_ty == Tarray)
	ptr_exp = darrayPtrRef (array_expr);
      else
	ptr_exp = addressOf (array_expr);

      // This conversion is required for static arrays and is just-to-be-safe
      // for dynamic arrays
      ptr_exp = d_convert_basic (base_type->nextOf()->pointerTo()->toCtype(), ptr_exp);
      break;

    case Tpointer:
      // Ignores array scope.
      ptr_exp = array_expr;
      break;

    default:
      gcc_unreachable();
    }

  ptr_exp = pvoidOkay (ptr_exp);
  subscript_expr = asc->finish (this, subscript_expr);
  elem_ref = indirect (TREE_TYPE (TREE_TYPE (ptr_exp)),
		       pointerIntSum (ptr_exp, subscript_expr));

  return elem_ref;
}

// Builds a BIND_EXPR around BODY for the variables VAR_CHAIN.

tree
IRState::binding (tree var_chain, tree body)
{
  // TODO: only handles one var
  gcc_assert (TREE_CHAIN (var_chain) == NULL_TREE);

  if (DECL_INITIAL (var_chain))
    {
      tree ini = build2 (INIT_EXPR, void_type_node, var_chain, DECL_INITIAL (var_chain));
      DECL_INITIAL (var_chain) = NULL_TREE;
      body = compound (ini, body);
    }

  return save_expr (build3 (BIND_EXPR, TREE_TYPE (body), var_chain, body, NULL_TREE));
}

// Like IRState::compound, but ARG0 or ARG1 might be NULL_TREE.

tree
IRState::maybeCompound (tree arg0, tree arg1)
{
  if (arg0 == NULL_TREE)
    return arg1;
  else if (arg1 == NULL_TREE)
    return arg0;
  else
    return build2 (COMPOUND_EXPR, TREE_TYPE (arg1), arg0, arg1);
}

// Like IRState::voidCompound, but ARG0 or ARG1 might be NULL_TREE.

tree
IRState::maybeVoidCompound (tree arg0, tree arg1)
{
  if (arg0 == NULL_TREE)
    return arg1;
  else if (arg1 == NULL_TREE)
    return arg0;
  else
    return build2 (COMPOUND_EXPR, void_type_node, arg0, arg1);
}

// Returns TRUE if T is an ERROR_MARK node.

bool
IRState::isErrorMark (tree t)
{
  return (t == error_mark_node
	  || (t && TREE_TYPE (t) == error_mark_node)
	  || (t && TREE_CODE (t) == NOP_EXPR &&
	      TREE_OPERAND (t, 0) == error_mark_node));
}

// Returns the TypeFunction class for Type T.
// Assumes T is already ->toBasetype()

TypeFunction *
IRState::getFuncType (Type *t)
{
  TypeFunction *tf = NULL;
  if (t->ty == Tpointer)
    t = t->nextOf()->toBasetype();
  if (t->ty == Tfunction)
    tf = (TypeFunction *) t;
  else if (t->ty == Tdelegate)
    tf = (TypeFunction *) ((TypeDelegate *) t)->next;
  return tf;
}

// Entry point for call routines.  Extracts the callee, object,
// and function type from expression EXPR, passing down ARGUMENTS.

tree
IRState::call (Expression *expr, Expressions *arguments)
{
  // Calls to delegates can sometimes look like this:
  if (expr->op == TOKcomma)
    {
      CommaExp *ce = (CommaExp *) expr;
      expr = ce->e2;

      VarExp *ve;
      gcc_assert (ce->e2->op == TOKvar);
      ve = (VarExp *) ce->e2;
      gcc_assert (ve->var->isFuncDeclaration() && !ve->var->needThis());
    }

  Type *t = expr->type->toBasetype();
  TypeFunction *tf = NULL;
  tree callee = expr->toElem (this);
  tree object = NULL_TREE;

  if (D_IS_METHOD_CALL_EXPR (callee))
    {
      /* This could be a delegate expression (TY == Tdelegate), but not
	 actually a delegate variable. */
      // %% Is this ever not a DotVarExp ?
      if (expr->op == TOKdotvar)
	{
	  /* This gets the true function type, the latter way can sometimes
	     be incorrect. Example: ref functions in D2. */
	  tf = getFuncType (((DotVarExp *)expr)->var->type);
	}
      else
	tf = getFuncType (t);

      extractMethodCallExpr (callee, callee, object);
    }
  else if (t->ty == Tdelegate)
    {
      tf = (TypeFunction *) ((TypeDelegate *) t)->next;
      callee = maybeMakeTemp (callee);
      object = delegateObjectRef (callee);
      callee = delegateMethodRef (callee);
    }
  else if (expr->op == TOKvar)
    {
      FuncDeclaration *fd = ((VarExp *) expr)->var->isFuncDeclaration();
      gcc_assert (fd);
      tf = (TypeFunction *) fd->type;
      if (fd->isNested())
	{
	  object = getFrameForFunction (fd);
	}
      else if (fd->needThis())
	{
	  expr->error ("need 'this' to access member %s", fd->toChars());
	  object = d_null_pointer; // continue processing...
	}
    }
  else
    {
      tf = getFuncType (t);
    }
  return call (tf, callee, object, arguments);
}

// Like above, but is assumed to be a direct call to FUNC_DECL.
// ARGS are the arguments passed.

tree
IRState::call (FuncDeclaration *func_decl, Expressions *args)
{
  // Otherwise need to copy code from above
  gcc_assert (!func_decl->isNested());

  return call (getFuncType (func_decl->type), func_decl->toSymbol()->Stree, NULL_TREE, args);
}

// Like above, but FUNC_DECL is a nested function, method, delegate or lambda.
// OBJECT is the 'this' reference passed and ARGS are the arguments passed.

tree
IRState::call (FuncDeclaration *func_decl, tree object, Expressions *args)
{
  return call (getFuncType (func_decl->type), addressOf (func_decl), object, args);
}

// Builds a CALL_EXPR of type FUNC_TYPE to CALLABLE. OBJECT holds the 'this' pointer,
// ARGUMENTS are evaluated in left to right order, saved and promoted before passing.

tree
IRState::call (TypeFunction *func_type, tree callable, tree object, Expressions *arguments)
{
  // Using TREE_TYPE (callable) instead of func_type->toCtype can save a build_method_type
  tree func_type_node = TREE_TYPE (callable);
  tree actual_callee = callable;
  tree saved_args = NULL_TREE;

  ListMaker actual_arg_list;

  if (POINTER_TYPE_P (func_type_node))
    func_type_node = TREE_TYPE (func_type_node);
  else
    actual_callee = addressOf (callable);

  gcc_assert (isFuncType (func_type_node));
  gcc_assert (func_type != NULL);
  gcc_assert (func_type->ty == Tfunction);

  bool is_d_vararg = func_type->varargs == 1 && func_type->linkage == LINKd;

  // Account for the hidden object/frame pointer argument

  if (TREE_CODE (func_type_node) == FUNCTION_TYPE)
    {
      if (object != NULL_TREE)
	{
	  // Happens when a delegate value is called
	  tree method_type = build_method_type (TREE_TYPE (object), func_type_node);
	  TYPE_ATTRIBUTES (method_type) = TYPE_ATTRIBUTES (func_type_node);
	  func_type_node = method_type;
	}
    }
  else
    {
      /* METHOD_TYPE */
      if (!object)
	{
	  // Front-end apparently doesn't check this.
	  if (TREE_CODE (callable) == FUNCTION_DECL)
	    {
	      error ("need 'this' to access member %s", IDENTIFIER_POINTER (DECL_NAME (callable)));
	      return error_mark_node;
	    }
	  else
	    {
	      // Probably an internal error
	      gcc_unreachable();
	    }
	}
    }
  /* If this is a delegate call or a nested function being called as
     a delegate, the object should not be NULL. */
  if (object != NULL_TREE)
    actual_arg_list.cons (object);

  Parameters *formal_args = func_type->parameters; // can be NULL for genCfunc decls
  size_t n_formal_args = formal_args ? (int) Parameter::dim (formal_args) : 0;
  size_t n_actual_args = arguments ? arguments->dim : 0;
  size_t fi = 0;

  // assumes arguments->dim <= formal_args->dim if (!this->varargs)
  for (size_t ai = 0; ai < n_actual_args; ++ai)
    {
      tree actual_arg_tree;
      Expression *actual_arg_exp = arguments->tdata()[ai];

      if (ai == 0 && is_d_vararg)
	{
	  // The hidden _arguments parameter
	  actual_arg_tree = actual_arg_exp->toElem (this);
	}
      else if (fi < n_formal_args)
	{
	  // Actual arguments for declared formal arguments
	  Parameter *formal_arg = Parameter::getNth (formal_args, fi);
	  actual_arg_tree = convertForArgument (actual_arg_exp, formal_arg);
	  actual_arg_tree = d_convert_basic (trueArgumentType (formal_arg), actual_arg_tree);
	  ++fi;
	}
      else
	{
	  if (flag_split_darrays && actual_arg_exp->type->toBasetype()->ty == Tarray)
	    {
	      tree da_exp = maybeMakeTemp (actual_arg_exp->toElem (this));
	      actual_arg_list.cons (darrayLenRef (da_exp));
	      actual_arg_list.cons (darrayPtrRef (da_exp));
	      continue;
	    }
	  else
	    {
	      actual_arg_tree = actual_arg_exp->toElem (this);
	      /* Not all targets support passing unpromoted types, so
		 promote anyway. */
	      tree prom_type = d_type_promotes_to (TREE_TYPE (actual_arg_tree));
	      if (prom_type != TREE_TYPE (actual_arg_tree))
		actual_arg_tree = d_convert_basic (prom_type, actual_arg_tree);
	    }
	}
      /* Evaluate the argument before passing to the function.
	 Needed for left to right evaluation.  */
      if (func_type->linkage == LINKd && !isFreeOfSideEffects (actual_arg_tree))
	{
	  actual_arg_tree = makeTemp (actual_arg_tree);
	  saved_args = maybeVoidCompound (saved_args, actual_arg_tree);
	}

      actual_arg_list.cons (actual_arg_tree);
    }

  tree result = buildCall (TREE_TYPE (func_type_node), actual_callee, actual_arg_list.head);
  result = maybeExpandSpecialCall (result);

  return maybeCompound (saved_args, result);
}

// Builds a call to AssertError.

tree
IRState::assertCall (Loc loc, LibCall libcall)
{
  tree args[2] = {
      darrayString (loc.filename ? loc.filename : ""),
      integerConstant (loc.linnum, Type::tuns32)
  };

  if (libcall == LIBCALL_ASSERT && this->func->isUnitTestDeclaration())
    libcall = LIBCALL_UNITTEST;

  return libCall (libcall, 2, args);
}

// Builds a call to AssertErrorMsg.

tree
IRState::assertCall (Loc loc, Expression *msg)
{
  tree args[3] = {
      msg->toElem (this),
      darrayString (loc.filename ? loc.filename : ""),
      integerConstant (loc.linnum, Type::tuns32)
  };

  LibCall libcall = this->func->isUnitTestDeclaration() ?
    LIBCALL_UNITTEST_MSG : LIBCALL_ASSERT_MSG;

  return libCall (libcall, 3, args);
}


// Our internal list of library functions.
// Most are extern(C) - for those that are not, correct mangling must be ensured.
// List kept in ascii collating order to allow binary search

static const char *libcall_ids[LIBCALL_count] = {
    /*"_d_invariant",*/ "_D9invariant12_d_invariantFC6ObjectZv",
    "_aApplyRcd1", "_aApplyRcd2", "_aApplyRcw1", "_aApplyRcw2",
    "_aApplyRdc1", "_aApplyRdc2", "_aApplyRdw1", "_aApplyRdw2",
    "_aApplyRwc1", "_aApplyRwc2", "_aApplyRwd1", "_aApplyRwd2",
    "_aApplycd1", "_aApplycd2", "_aApplycw1", "_aApplycw2",
    "_aApplydc1", "_aApplydc2", "_aApplydw1", "_aApplydw2",
    "_aApplywc1", "_aApplywc2", "_aApplywd1", "_aApplywd2",
    "_aaApply", "_aaApply2",
    "_aaDelX", "_aaEqual",
    "_aaGetRvalueX", "_aaGetX",
    "_aaInX", "_aaLen",
    "_adCmp", "_adCmp2", "_adCmpChar",
    "_adDupT", "_adEq", "_adEq2",
    "_adReverse", "_adReverseChar", "_adReverseWchar",
    "_adSort", "_adSortChar", "_adSortWchar",
    "_d_allocmemory", "_d_array_bounds",
    "_d_arrayappendT", "_d_arrayappendcTX",
    "_d_arrayappendcd", "_d_arrayappendwd",
    "_d_arrayassign", "_d_arraycast",
    "_d_arraycatT", "_d_arraycatnT",
    "_d_arraycopy", "_d_arrayctor",
    "_d_arrayliteralTX",
    "_d_arraysetassign", "_d_arraysetctor",
    "_d_arraysetlengthT", "_d_arraysetlengthiT",
    "_d_assert", "_d_assert_msg",
    "_d_assocarrayliteralTX",
    "_d_callfinalizer", "_d_callinterfacefinalizer",
    "_d_criticalenter", "_d_criticalexit",
    "_d_delarray", "_d_delarray_t", "_d_delclass",
    "_d_delinterface", "_d_delmemory",
    "_d_dynamic_cast", "_d_hidden_func", "_d_interface_cast",
    "_d_monitorenter", "_d_monitorexit",
    "_d_newarrayT", "_d_newarrayiT",
    "_d_newarraymTX", "_d_newarraymiTX",
    "_d_newclass", "_d_newitemT", "_d_newitemiT",
    "_d_switch_dstring", "_d_switch_error",
    "_d_switch_string", "_d_switch_ustring",
    "_d_throw", "_d_unittest", "_d_unittest_msg",
};

static FuncDeclaration *libcall_decls[LIBCALL_count];

// Library functions are generated as needed.
// This could probably be changed in the future to be
// more like GCC builtin trees.

FuncDeclaration *
IRState::getLibCallDecl (LibCall lib_call)
{
  FuncDeclaration *decl = libcall_decls[lib_call];

  static Type *aa_type = NULL;
  static Type *dg_type = NULL;
  static Type *dg2_type = NULL;

  if (!decl)
    {
      Types targs;
      Type *treturn = Type::tvoid;
      bool varargs = false;

      // Build generic AA type void*[void*]
      if (aa_type == NULL)
	aa_type = new TypeAArray (Type::tvoidptr, Type::tvoidptr);

      // Build generic delegate type int(void*)
      if (dg_type == NULL)
	{
	  Parameters *fn_parms = new Parameters;
	  fn_parms->push (new Parameter (STCin, Type::tvoidptr, NULL, NULL));
	  Type *fn_type = new TypeFunction (fn_parms, Type::tint32, false, LINKd);
	  dg_type = new TypeDelegate (fn_type);
	}

      // Build generic delegate type int(void*, void*)
      if (dg2_type == NULL)
	{
	  Parameters *fn_parms = new Parameters;
	  fn_parms->push (new Parameter (STCin, Type::tvoidptr, NULL, NULL));
	  fn_parms->push (new Parameter (STCin, Type::tvoidptr, NULL, NULL));
	  Type *fn_type = new TypeFunction (fn_parms, Type::tint32, false, LINKd);
	  dg2_type = new TypeDelegate (fn_type);
	}

      switch (lib_call)
	{
	case LIBCALL_ASSERT:
	case LIBCALL_ARRAY_BOUNDS:
	case LIBCALL_SWITCH_ERROR:
	  // need to spec chararray/string because internal code passes string constants
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tuns32);
	  break;

	case LIBCALL_ASSERT_MSG:
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tuns32);
	  break;

	case LIBCALL_UNITTEST:
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tuns32);
	  break;

	case LIBCALL_UNITTEST_MSG:
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tuns32);
	  break;

	case LIBCALL_NEWCLASS:
	  targs.push (ClassDeclaration::classinfo->type->constOf());
	  treturn = getObjectType();
	  break;

	case LIBCALL_NEWARRAYT:
	case LIBCALL_NEWARRAYIT:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_NEWARRAYMTX:
	case LIBCALL_NEWARRAYMITX:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_NEWITEMT:
	case LIBCALL_NEWITEMIT:
	  targs.push (Type::typeinfo->type->constOf());
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_ALLOCMEMORY:
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_DELCLASS:
	case LIBCALL_DELINTERFACE:
	  targs.push (Type::tvoidptr);
	  break;

	case LIBCALL_DELARRAY:
	  targs.push (Type::tvoid->arrayOf()->pointerTo());
	  break;

	case LIBCALL_DELARRAYT:
	  targs.push (Type::tvoid->arrayOf()->pointerTo());
	  targs.push (Type::typeinfo->type->constOf());
	  break;

	case LIBCALL_DELMEMORY:
	  targs.push (Type::tvoidptr->pointerTo());
	  break;

	case LIBCALL_CALLFINALIZER:
	case LIBCALL_CALLINTERFACEFINALIZER:
	  targs.push (Type::tvoidptr);
	  break;

	case LIBCALL_ARRAYSETLENGTHT:
	case LIBCALL_ARRAYSETLENGTHIT:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoid->arrayOf()->pointerTo());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_DYNAMIC_CAST:
	case LIBCALL_INTERFACE_CAST:
	  targs.push (getObjectType());
	  targs.push (ClassDeclaration::classinfo->type);
	  treturn = getObjectType();
	  break;

	case LIBCALL_ADEQ:
	case LIBCALL_ADEQ2:
	case LIBCALL_ADCMP:
	case LIBCALL_ADCMP2:
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::typeinfo->type->constOf());
	  treturn = Type::tint32;
	  break;

	case LIBCALL_ADCMPCHAR:
	  targs.push (Type::tchar->arrayOf());
	  targs.push (Type::tchar->arrayOf());
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAEQUAL:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (aa_type);
	  targs.push (aa_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AALEN:
	  targs.push (aa_type);
	  treturn = Type::tsize_t;
	  break;

	case LIBCALL_AAINX:
	  targs.push (aa_type);
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tvoidptr);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_AAGETX:
	  targs.push (aa_type->pointerTo());
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoidptr);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_AAGETRVALUEX:
	  targs.push (aa_type);
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoidptr);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_AADELX:
	  targs.push (aa_type);
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tvoidptr);
	  treturn = Type::tbool;
	  break;

	case LIBCALL_ARRAYCAST:
	  targs.push (Type::tsize_t);
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoid->arrayOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYCOPY:
	  targs.push (Type::tsize_t);
	  targs.push (Type::tint8->arrayOf());
	  targs.push (Type::tint8->arrayOf());
	  treturn = Type::tint8->arrayOf();
	  break;

	case LIBCALL_ARRAYCATT:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tint8->arrayOf());
	  targs.push (Type::tint8->arrayOf());
	  treturn = Type::tint8->arrayOf();
	  break;

	case LIBCALL_ARRAYCATNT:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tuns32); // Currently 'uint', even if 64-bit
	  varargs = true;
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYAPPENDT:
	  targs.push (Type::typeinfo->type); //->constOf());
	  targs.push (Type::tint8->arrayOf()->pointerTo());
	  targs.push (Type::tint8->arrayOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYAPPENDCTX:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tint8->arrayOf()->pointerTo());
	  targs.push (Type::tsize_t);
	  treturn = Type::tint8->arrayOf();
	  break;

	case LIBCALL_ARRAYAPPENDCD:
	  targs.push (Type::tint8->arrayOf()->pointerTo());
	  targs.push (Type::tdchar);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYAPPENDWD:
	  targs.push (Type::tint8->arrayOf()->pointerTo());
	  targs.push (Type::tdchar);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYASSIGN:
	case LIBCALL_ARRAYCTOR:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tvoid->arrayOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYSETASSIGN:
	case LIBCALL_ARRAYSETCTOR:
	  targs.push (Type::tvoidptr);
	  targs.push (Type::tvoidptr);
	  targs.push (Type::tsize_t);
	  targs.push (Type::typeinfo->type->constOf());
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_MONITORENTER:
	case LIBCALL_MONITOREXIT:
	case LIBCALL_THROW:
	case LIBCALL_INVARIANT:
	  targs.push (getObjectType());
	  break;

	case LIBCALL_CRITICALENTER:
	case LIBCALL_CRITICALEXIT:
	  targs.push (Type::tvoidptr);
	  break;

	case LIBCALL_SWITCH_USTRING:
	  targs.push (Type::twchar->arrayOf()->arrayOf());
	  targs.push (Type::twchar->arrayOf());
	  treturn = Type::tint32;
	  break;

	case LIBCALL_SWITCH_DSTRING:
	  targs.push (Type::tdchar->arrayOf()->arrayOf());
	  targs.push (Type::tdchar->arrayOf());
	  treturn = Type::tint32;
	  break;

	case LIBCALL_SWITCH_STRING:
	  targs.push (Type::tchar->arrayOf()->arrayOf());
	  targs.push (Type::tchar->arrayOf());
	  treturn = Type::tint32;
	  break;
	case LIBCALL_ASSOCARRAYLITERALTX:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tvoid->arrayOf());
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_ARRAYLITERALTX:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_ADSORTCHAR:
	case LIBCALL_ADREVERSECHAR:
	  targs.push (Type::tchar->arrayOf());
	  treturn = Type::tchar->arrayOf();
	  break;

	case LIBCALL_ADSORTWCHAR:
	case LIBCALL_ADREVERSEWCHAR:
	  targs.push (Type::twchar->arrayOf());
	  treturn = Type::twchar->arrayOf();
	  break;

	case LIBCALL_ADDUPT:
	  targs.push (Type::typeinfo->type->constOf());
	  targs.push (Type::tvoid->arrayOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ADREVERSE:
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ADSORT:
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::typeinfo->type->constOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_AAAPPLY:
	  targs.push (aa_type);
	  targs.push (Type::tsize_t);
	  targs.push (dg_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAAPPLY2:
	  targs.push (aa_type);
	  targs.push (Type::tsize_t);
	  targs.push (dg2_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAPPLYCD1:
	case LIBCALL_AAPPLYCW1:
	case LIBCALL_AAPPLYRCD1:
	case LIBCALL_AAPPLYRCW1:
	  targs.push (Type::tchar->arrayOf());
	  targs.push (dg_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAPPLYCD2:
	case LIBCALL_AAPPLYCW2:
	case LIBCALL_AAPPLYRCD2:
	case LIBCALL_AAPPLYRCW2:
	  targs.push (Type::tchar->arrayOf());
	  targs.push (dg2_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAPPLYDC1:
	case LIBCALL_AAPPLYDW1:
	case LIBCALL_AAPPLYRDC1:
	case LIBCALL_AAPPLYRDW1:
	  targs.push (Type::tdchar->arrayOf());
	  targs.push (dg_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAPPLYDC2:
	case LIBCALL_AAPPLYDW2:
	case LIBCALL_AAPPLYRDC2:
	case LIBCALL_AAPPLYRDW2:
	  targs.push (Type::tdchar->arrayOf());
	  targs.push (dg2_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAPPLYWC1:
	case LIBCALL_AAPPLYWD1:
	case LIBCALL_AAPPLYRWC1:
	case LIBCALL_AAPPLYRWD1:
	  targs.push (Type::twchar->arrayOf());
	  targs.push (dg_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAPPLYWC2:
	case LIBCALL_AAPPLYWD2:
	case LIBCALL_AAPPLYRWC2:
	case LIBCALL_AAPPLYRWD2:
	  targs.push (Type::twchar->arrayOf());
	  targs.push (dg2_type);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_HIDDEN_FUNC:
	  /* Argument is an Object, but can't use that as
	     LIBCALL_HIDDEN_FUNC is needed before the Object type is
	     created. */
	  targs.push (Type::tvoidptr);
	  break;

	default:
	  gcc_unreachable();
	}

      // Build extern(C) function.
      Identifier *id = Lexer::idPool(libcall_ids[lib_call]);
      TypeFunction *tf = new TypeFunction(NULL, treturn, 0, LINKc);
      tf->varargs = varargs ? 1 : 0;

      decl = new FuncDeclaration(0, 0, id, STCstatic, tf);
      decl->protection = PROTpublic;
      decl->linkage = LINKc;

      // Add parameter types.
      Parameters *args = new Parameters;
      args->setDim (targs.dim);
      for (size_t i = 0; i < targs.dim; i++)
	(*args)[i] = new Parameter (0, targs[i], NULL, NULL);

      tf->parameters = args;
      libcall_decls[lib_call] = decl;
    }

  return decl;
}

void
IRState::maybeSetLibCallDecl (FuncDeclaration *decl)
{
  if (!decl->ident)
    return;

  LibCall lib_call = (LibCall) binary (decl->ident->string, libcall_ids, LIBCALL_count);
  if (lib_call == LIBCALL_NONE)
    return;

  if (libcall_decls[lib_call] == decl)
    return;

  TypeFunction *tf = (TypeFunction *) decl->type;
  if (tf->parameters == NULL)
    {
      FuncDeclaration *new_decl = getLibCallDecl (lib_call);
      new_decl->toSymbol();

      decl->type = new_decl->type;
      decl->csym = new_decl->csym;
    }

  libcall_decls[lib_call] = decl;
}

// Build call to LIB_CALL. N_ARGS is the number of call arguments
// which are specified in as a tree array ARGS.  The caller can
// force the return type of the call to FORCE_RESULT_TYPE if the
// library call returns a generic value.

tree
IRState::libCall (LibCall lib_call, unsigned n_args, tree *args, tree force_result_type)
{
  FuncDeclaration *lib_decl = getLibCallDecl (lib_call);
  Type *type = lib_decl->type->nextOf();
  tree callee = addressOf (lib_decl);
  tree arg_list = NULL_TREE;

  for (int i = n_args - 1; i >= 0; i--)
    arg_list = tree_cons (NULL_TREE, args[i], arg_list);

  tree result = buildCall (type->toCtype(), callee, arg_list);

  // for force_result_type, assumes caller knows what it is doing %%
  if (force_result_type != NULL_TREE)
    return d_convert_basic (force_result_type, result);

  return result;
}

// Build a call to CALLEE, passing ARGS as arguments.
// The expected return type is TYPE.
// TREE_SIDE_EFFECTS gets set depending on the const/pure attributes
// of the funcion and the SIDE_EFFECTS flags of the arguments.

tree
IRState::buildCall (tree type, tree callee, tree args)
{
  int nargs = list_length (args);
  tree *pargs = new tree[nargs];
  for (size_t i = 0; args; args = TREE_CHAIN (args), i++)
    pargs[i] = TREE_VALUE (args);

  return build_call_array (type, callee, nargs, pargs);
}

// Conveniently construct the function arguments for passing
// to the real buildCall function.

tree
IRState::buildCall (tree callee, int n_args, ...)
{
  va_list ap;
  tree arg_list = NULL_TREE;
  tree fntype = TREE_TYPE (callee);

  va_start (ap, n_args);
  for (int i = n_args - 1; i >= 0; i--)
    arg_list = tree_cons (NULL_TREE, va_arg (ap, tree), arg_list);
  va_end (ap);

  return buildCall (TREE_TYPE (fntype), addressOf (callee), nreverse (arg_list));
}

// If CALL_EXP is a BUILT_IN_FRONTEND, expand and return inlined
// compiler generated instructions. Most map onto GCC builtins,
// others require a little extra work around them.

tree
IRState::maybeExpandSpecialCall (tree call_exp)
{
  // More code duplication from C
  CallExpr ce (call_exp);
  tree callee = ce.callee();
  tree op1 = NULL_TREE, op2 = NULL_TREE;
  tree exp = NULL_TREE;

  if (POINTER_TYPE_P (TREE_TYPE (callee)))
    callee = TREE_OPERAND (callee, 0);

  if (TREE_CODE (callee) == FUNCTION_DECL
      && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_FRONTEND)
    {
      Intrinsic intrinsic = (Intrinsic) DECL_FUNCTION_CODE (callee);
      tree type;
      Type *d_type;
      switch (intrinsic)
	{
	case INTRINSIC_BSF:
	  /* builtin count_trailing_zeros matches behaviour of bsf.
	     %% TODO: The return value is supposed to be undefined if op1 is zero. */
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_CTZL), 1, op1);

	case INTRINSIC_BSR:
	  /* bsr becomes 31-(clz), but parameter passed to bsf may not be a 32bit type!!
	     %% TODO: The return value is supposed to be undefined if op1 is zero. */
	  op1 = ce.nextArg();
	  type = TREE_TYPE (op1);

	  op2 = integerConstant (tree_low_cst (TYPE_SIZE (type), 1) - 1, type);
	  exp = buildCall (d_built_in_decls (BUILT_IN_CLZL), 1, op1);

	  // Handle int -> long conversions.
	  if (TREE_TYPE (exp) != type)
	    exp = fold_convert (type, exp);

	  return fold_build2 (MINUS_EXPR, type, op2, exp);

	case INTRINSIC_BT:
	case INTRINSIC_BTC:
	case INTRINSIC_BTR:
	case INTRINSIC_BTS:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  type = TREE_TYPE (TREE_TYPE (op1));

	  exp = integerConstant (tree_low_cst (TYPE_SIZE (type), 1), type);

	  // op1[op2 / exp]
	  op1 = pointerIntSum (op1, fold_build2 (TRUNC_DIV_EXPR, type, op2, exp));
	  op1 = indirect (type, op1);

	  // mask = 1 << (op2 % exp);
	  op2 = fold_build2 (TRUNC_MOD_EXPR, type, op2, exp);
	  op2 = fold_build2 (LSHIFT_EXPR, type, size_one_node, op2);

	  // cond = op1[op2 / size] & mask;
	  exp = fold_build2 (BIT_AND_EXPR, type, op1, op2);

	  // cond ? -1 : 0;
	  exp = build3 (COND_EXPR, TREE_TYPE (call_exp), d_truthvalue_conversion (exp),
			integer_minus_one_node, integer_zero_node);

	  if (intrinsic == INTRINSIC_BT)
	    {
	      // Only testing the bit.
	      return exp;
	    }
	  else
	    {
	      // Update the bit as needed.
	      tree result = localVar (TREE_TYPE (call_exp));
	      enum tree_code code = (intrinsic == INTRINSIC_BTC) ? BIT_XOR_EXPR :
		(intrinsic == INTRINSIC_BTR) ? BIT_AND_EXPR :
		(intrinsic == INTRINSIC_BTS) ? BIT_IOR_EXPR : ERROR_MARK;
	      gcc_assert (code != ERROR_MARK);

	      // op1[op2 / size] op= mask
	      if (intrinsic == INTRINSIC_BTR)
		op2 = build1 (BIT_NOT_EXPR, TREE_TYPE (op2), op2);

	      exp = vmodify (result, exp);
	      op1 = vmodify (op1, fold_build2 (code, TREE_TYPE (op1), op1, op2));
	      op1 = compound (op1, result);
	      return compound (exp, op1);
	    }

	case INTRINSIC_BSWAP:
	  /* Backend provides builtin bswap32.
	     Assumes first argument and return type is uint. */
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_BSWAP32), 1, op1);

	case INTRINSIC_INP:
	case INTRINSIC_INPL:
	case INTRINSIC_INPW:
#ifdef TARGET_386
	  type = TREE_TYPE (call_exp);
	  d_type = Type::tuns16;

	  op1 = ce.nextArg();
	  // %% Port is always cast to ushort
	  op1 = d_convert_basic (d_type->toCtype(), op1);
	  op2 = localVar (type);
	  return expandPortIntrinsic (intrinsic, op1, op2, 0);
#endif
	  // else fall through to error below.
	case INTRINSIC_OUTP:
	case INTRINSIC_OUTPL:
	case INTRINSIC_OUTPW:
#ifdef TARGET_386
	  type = TREE_TYPE (call_exp);
	  d_type = Type::tuns16;

	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  // %% Port is always cast to ushort
	  op1 = d_convert_basic (d_type->toCtype(), op1);
	  return expandPortIntrinsic (intrinsic, op1, op2, 1);
#else
	  ::error ("Port I/O intrinsic '%s' is only available on ix86 targets",
		   IDENTIFIER_POINTER (DECL_NAME (callee)));
	  break;
#endif

	case INTRINSIC_COS:
	  // Math intrinsics just map to their GCC equivalents.
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_COSL), 1, op1);

	case INTRINSIC_SIN:
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_SINL), 1, op1);

	case INTRINSIC_RNDTOL:
	  // %% not sure if llroundl stands as a good replacement
	  // for the expected behaviour of rndtol.
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_LLROUNDL), 1, op1);

	case INTRINSIC_SQRT:
	  // Have float, double and real variants of sqrt.
	  op1 = ce.nextArg();
	  type = TREE_TYPE (op1);
	  // Could have used mathfn_built_in, but that only returns
	  // implicit built in decls.
	  if (TYPE_MAIN_VARIANT (type) == double_type_node)
	    exp = d_built_in_decls (BUILT_IN_SQRT);
	  else if (TYPE_MAIN_VARIANT (type) == float_type_node)
	    exp = d_built_in_decls (BUILT_IN_SQRTF);
	  else if (TYPE_MAIN_VARIANT (type) == long_double_type_node)
	    exp = d_built_in_decls (BUILT_IN_SQRTL);
	  // op1 is an integral type - use double precision.
	  else if (INTEGRAL_TYPE_P (TYPE_MAIN_VARIANT (type)))
	    {
	      op1 = d_convert_basic (double_type_node, op1);
	      exp = d_built_in_decls (BUILT_IN_SQRT);
	    }

	  gcc_assert (exp);    // Should never trigger.
	  return buildCall (exp, 1, op1);

	case INTRINSIC_LDEXP:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_LDEXPL), 2, op1, op2);

	case INTRINSIC_FABS:
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_FABSL), 1, op1);

	case INTRINSIC_RINT:
	  op1 = ce.nextArg();
	  return buildCall (d_built_in_decls (BUILT_IN_RINTL), 1, op1);

	case INTRINSIC_VA_ARG:
	case INTRINSIC_C_VA_ARG:
	  op1 = ce.nextArg();
	  STRIP_NOPS (op1);
	  gcc_assert (TREE_CODE (op1) == ADDR_EXPR);
	  op1 = TREE_OPERAND (op1, 0);

	  if (intrinsic == INTRINSIC_C_VA_ARG)
	    type = TREE_TYPE (TREE_TYPE (callee));
	  else
	    {
	      op2 = ce.nextArg();
	      STRIP_NOPS (op2);
	      gcc_assert (TREE_CODE (op2) == ADDR_EXPR);
	      op2 = TREE_OPERAND (op2, 0);
	      type = TREE_TYPE (op2);
	    }

	  d_type = getDType (type);
	  if (flag_split_darrays &&
	      (d_type && d_type->toBasetype()->ty == Tarray))
	    {
	      /* should create a temp var of type TYPE and move the binding
		 to outside this expression.  */
	      tree ltype = TREE_TYPE (TYPE_FIELDS (type));
	      tree ptype = TREE_TYPE (TREE_CHAIN (TYPE_FIELDS (type)));
	      tree lvar = exprVar (ltype);
	      tree pvar = exprVar (ptype);

	      op1 = stabilize_reference (op1);
	      tree e1 = vmodify (lvar, build1 (VA_ARG_EXPR, ltype, op1));
	      tree e2 = vmodify (pvar, build1 (VA_ARG_EXPR, ptype, op1));
	      exp = compound (compound (e1, e2), darrayVal (type, lvar, pvar));
	      exp = binding (lvar, binding (pvar, exp));
	    }
	  else
	    {
	      tree type2 = d_type_promotes_to (type);
	      exp = build1 (VA_ARG_EXPR, type2, op1);
	      // silently convert promoted type...
	      if (type != type2)
		exp = d_convert_basic (type, exp);
	    }

	  if (intrinsic == INTRINSIC_VA_ARG)
	    exp = vmodify (op2, exp);

	  return exp;

	case INTRINSIC_C_VA_START:
	  /* The va_list argument should already have its
	     address taken.  The second argument, however, is
	     inout and that needs to be fixed to prevent a warning.  */
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  type = TREE_TYPE (op1);

	  // could be casting... so need to check type too?
	  STRIP_NOPS (op1);
	  STRIP_NOPS (op2);
	  gcc_assert (TREE_CODE (op1) == ADDR_EXPR &&
		      TREE_CODE (op2) == ADDR_EXPR);

	  op2 = TREE_OPERAND (op2, 0);
	  // assuming nobody tries to change the return type
	  return buildCall (d_built_in_decls (BUILT_IN_VA_START), 2, op1, op2);

	default:
	  gcc_unreachable();
	}
    }

  return call_exp;
}

// Expand a call to CODE with argument PORT and return value VALUE.
// These are x86 / x86_64 instructions, and so calls to this function,
// should be safe guarded from use on other architectures.

tree
IRState::expandPortIntrinsic (Intrinsic code, tree port, tree value, int outp)
{
  const char *insn_string;
  tree insn_tmpl;
  ListMaker outputs;
  ListMaker inputs;

  if (outp)
    {
      /* Writing outport operand:
	 asm ("op %[value], %[port]" :: "a" value, "Nd" port);
       */
      const char *valconstr = "a";
      tree val = tree_cons (NULL_TREE, build_string (strlen (valconstr), valconstr), NULL_TREE);
      inputs.cons (val, value);
    }
  else
    {
      /* Writing inport operand:
	 asm ("op %[port], %[value]" : "=a" value : "Nd" port);
       */
      const char *outconstr = "=a";
      tree out = tree_cons (NULL_TREE, build_string (strlen (outconstr), outconstr), NULL_TREE);
      outputs.cons (out, value);
    }

  const char *inconstr = "Nd";
  tree in = tree_cons (NULL_TREE, build_string (strlen (inconstr), inconstr), NULL_TREE);
  inputs.cons (in, port);

  switch (code)
    {
    case INTRINSIC_INP:
      insn_string = "inb %w1, %0";
      break;

    case INTRINSIC_INPL:
      insn_string = "inl %w1, %0";
      break;

    case INTRINSIC_INPW:
      insn_string = "inw %w1, %0";
      break;

    case INTRINSIC_OUTP:
      insn_string = "outb %b0, %w1";
      break;

    case INTRINSIC_OUTPL:
      insn_string = "outl %0, %w1";
      break;

    case INTRINSIC_OUTPW:
      insn_string = "outw %w0, %w1";
      break;

    default:
      gcc_unreachable();
    }
  insn_tmpl = build_string (strlen (insn_string), insn_string);

  // ::doAsm
  tree exp = d_build_asm_stmt (insn_tmpl, outputs.head, inputs.head, NULL_TREE);
  ASM_VOLATILE_P (exp) = 1;

  // These functions always return the contents of 'value'
  return build2 (COMPOUND_EXPR, TREE_TYPE (value), exp, value);
}


// Build and return the correct call to fmod depending on TYPE.
// ARG0 and ARG1 are the arguments pass to the function.

tree
IRState::floatMod (tree type, tree arg0, tree arg1)
{
  tree fmodfn = NULL_TREE;
  tree basetype = type;

  if (COMPLEX_FLOAT_TYPE_P (basetype))
    basetype = TREE_TYPE (basetype);

  if (TYPE_MAIN_VARIANT (basetype) == double_type_node)
    fmodfn = d_built_in_decls (BUILT_IN_FMOD);
  else if (TYPE_MAIN_VARIANT (basetype) == float_type_node)
    fmodfn = d_built_in_decls (BUILT_IN_FMODF);
  else if (TYPE_MAIN_VARIANT (basetype) == long_double_type_node)
    fmodfn = d_built_in_decls (BUILT_IN_FMODL);

  if (!fmodfn)
    {
      // %qT pretty prints the tree type.
      ::error ("tried to perform floating-point modulo division on %qT", type);
      return error_mark_node;
    }

  if (COMPLEX_FLOAT_TYPE_P (type))
    {
      return build2 (COMPLEX_EXPR, type,
		     buildCall (fmodfn, 2, realPart (arg0), arg1),
		     buildCall (fmodfn, 2, imagPart (arg0), arg1));
    }
  else if (SCALAR_FLOAT_TYPE_P (type))
    {
      // %% assuming no arg conversion needed
      // %% bypassing buildCall since this shouldn't have
      // side effects
      return buildCall (fmodfn, 2, arg0, arg1);
    }
  else
    {
      // Should have caught this above.
      gcc_unreachable();
    }
}

// 

tree
IRState::typeinfoReference (Type *t)
{
  tree ti_ref = t->getInternalTypeInfo (NULL)->toElem (this);
  gcc_assert (POINTER_TYPE_P (TREE_TYPE (ti_ref)));
  return ti_ref;
}

// Return host integer value for INT_CST T.

dinteger_t
IRState::getTargetSizeConst (tree t)
{
  if (host_integerp (t, 0) || host_integerp (t, 1))
    return tree_low_cst (t, 1);

  return hwi2toli (TREE_INT_CST (t));
}

// Returns TRUE if DECL is an intrinsic function that requires
// special processing.  Marks the generated trees for DECL
// as BUILT_IN_FRONTEND so can be identified later.

bool
IRState::maybeSetUpBuiltin (Declaration *decl)
{
  Dsymbol *dsym;
  TemplateInstance *ti;

  // Don't use toParent2.  We are looking for a template below.
  dsym = decl->toParent();

  if (!dsym)
    return false;

  if ((gen.intrinsicModule && dsym->getModule() == gen.intrinsicModule))
    {
      // Matches order of Intrinsic enum
      static const char *intrinsic_names[] = {
	  "bsf", "bsr",
	  "bswap",
	  "bt", "btc", "btr", "bts",
	  "inp", "inpl", "inpw",
	  "outp", "outpl", "outpw",
      };
      const size_t sz = sizeof (intrinsic_names) / sizeof (char *);
      int i = binary (decl->ident->string, intrinsic_names, sz);
      if (i == -1)
	return false;

      // Make sure 'i' is within the range we require.
      gcc_assert (i >= INTRINSIC_BSF && i <= INTRINSIC_OUTPW);
      tree t = decl->toSymbol()->Stree;

      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
      DECL_FUNCTION_CODE (t) = (built_in_function) i;
      return true;
    }
  else if ((gen.mathModule && dsym->getModule() == gen.mathModule) ||
	   (gen.mathCoreModule && dsym->getModule() == gen.mathCoreModule))
    {
      // Matches order of Intrinsic enum
      static const char *math_names[] = {
	  "cos", "fabs", "ldexp",
	  "rint", "rndtol", "sin",
	  "sqrt",
      };
      const size_t sz = sizeof (math_names) / sizeof (char *);
      int i = binary (decl->ident->string, math_names, sz);
      if (i == -1)
	return false;

      // Adjust 'i' for this range of enums
      i += INTRINSIC_COS;
      gcc_assert (i >= INTRINSIC_COS && i <= INTRINSIC_SQRT);
      tree t = decl->toSymbol()->Stree;

      // rndtol returns a long, sqrt any real value,
      // every other math builtin returns an 80bit float.
      Type *tf = decl->type->nextOf();
      if ((i == INTRINSIC_RNDTOL && tf->ty == Tint64)
	  || (i == INTRINSIC_SQRT && tf->isreal())
	  || (i != INTRINSIC_RNDTOL && tf->ty == Tfloat80))
	{
	  DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	  DECL_FUNCTION_CODE (t) = (built_in_function) i;
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
	  if (ti->tempdecl == gen.stdargTemplateDecl)
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) INTRINSIC_VA_ARG;
	      return true;
	    }
	  if (ti->tempdecl == gen.cstdargTemplateDecl)
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) INTRINSIC_C_VA_ARG;
	      return true;
	    }
	  else if (ti->tempdecl == gen.cstdargStartTemplateDecl)
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) INTRINSIC_C_VA_START;
	      return true;
	    }
	}
    }
  return false;
}

// Build and return D's internal exception Object.

tree
IRState::exceptionObject (void)
{
  tree obj_type = getObjectType()->toCtype();
  if (TREE_CODE (TREE_TYPE (obj_type)) == REFERENCE_TYPE)
    obj_type = TREE_TYPE (obj_type);
  // Like Java, the actual D exception object is one
  // pointer behind the exception header
  tree t = buildCall (d_built_in_decls (BUILT_IN_EH_POINTER),
		      1, integer_zero_node);
  // treat exception header as (Object *)
  t = build1 (NOP_EXPR, build_pointer_type (obj_type), t);
  t = pointerOffsetOp (MINUS_EXPR, t, TYPE_SIZE_UNIT (TREE_TYPE (t)));
  t = build1 (INDIRECT_REF, obj_type, t);
  return t;
}

// Build LABEL_DECL for IDENT given.

tree
IRState::label (Loc loc, Identifier *ident)
{
  tree t_label = build_decl (UNKNOWN_LOCATION, LABEL_DECL,
			     ident ? get_identifier (ident->string) : NULL_TREE, void_type_node);
  DECL_CONTEXT (t_label) = current_function_decl;
  DECL_MODE (t_label) = VOIDmode;
  if (loc.filename)
    g.ofile->setDeclLoc (t_label, loc);
  return t_label;
}

// Entry points for protected getFrameForSymbol.

tree
IRState::getFrameForFunction (FuncDeclaration *f)
{
  if (f->fbody)
    {
      return getFrameForSymbol (f);
    }
  else
    {
      // Should instead error on line that references f
      f->error ("nested function missing body");
      return d_null_pointer;
    }
}

tree
IRState::getFrameForNestedClass (ClassDeclaration *c)
{
  return getFrameForSymbol (c);
}

tree
IRState::getFrameForNestedStruct (StructDeclaration *s)
{
  return getFrameForSymbol (s);
}

// If NESTED_SYM is a nested function, return the static chain to be
// used when invoking that function.

// If NESTED_SYM is a nested class or struct, return the static chain
// to be used when creating an instance of the class.

// This method is protected to enforce the type checking of getFrameForFunction,
// getFrameForNestedClass, and getFrameForNestedStruct.
// getFrameForFunction also checks that the nested function is properly defined.

tree
IRState::getFrameForSymbol (Dsymbol *nested_sym)
{
  FuncDeclaration *nested_func = NULL;
  FuncDeclaration *outer_func = NULL;

  if ((nested_func = nested_sym->isFuncDeclaration()))
    {
      // gcc_assert (nested_func->isNested())
      outer_func = nested_func->toParent2()->isFuncDeclaration();
      gcc_assert (outer_func != NULL);

      if (this->func != outer_func)
	{
	  Dsymbol *this_func = this->func;
	  if (!this->func->vthis) // if no frame pointer for this function
	    {
	      nested_sym->error ("is a nested function and cannot be accessed from %s", this->func->toChars());
	      return d_null_pointer;
	    }
	  /* Make sure we can get the frame pointer to the outer function,
	     else we'll ICE later in tree-ssa.  */
	  while (nested_func != this_func)
	    {
	      FuncDeclaration *fd;
	      ClassDeclaration *cd;
	      StructDeclaration *sd;

	      // Special case for __ensure and __require.
	      if (nested_func->ident == Id::ensure || nested_func->ident == Id::require)
		{
		  outer_func = this->func;
		  break;
		}

	      if ((fd = this_func->isFuncDeclaration()))
		{
		  if (outer_func == fd->toParent2())
		    break;
		  gcc_assert (fd->isNested() || fd->vthis);
		}
	      else if ((cd = this_func->isClassDeclaration()))
		{
		  if (!cd->isNested() || !cd->vthis)
		    goto cannot_get_frame;
		  if (outer_func == cd->toParent2())
		    break;
		}
	      else if ((sd = this_func->isStructDeclaration()))
		{
		  if (!sd->isNested() || !sd->vthis)
		    goto cannot_get_frame;
		  if (outer_func == sd->toParent2())
		    break;
		}
	      else
		{
	    cannot_get_frame:
		  this->func->error ("cannot get frame pointer to %s", nested_sym->toChars());
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

      Dsymbol *sym = nested_sym;

      while (sym && !(outer_func = sym->isFuncDeclaration()))
	sym = sym->toParent2();

      /* Make sure we can access the frame of outer_func.  */
      if (outer_func != this->func)
	{
	  Dsymbol *o = nested_func = this->func;
	  do {
	      if (!nested_func->isNested())
		{
		  if (!nested_func->isMember2())
		    goto cannot_access_frame;
		}
	      while ((o = o->toParent2()))
		{
		  if ((nested_func = o->isFuncDeclaration()))
		    break;
		}
	  } while (o && o != outer_func);

	  if (!o)
	    {
	cannot_access_frame:
	      error ("cannot access frame of function '%s' from '%s'",
		     outer_func->toChars(), this->func->toChars());
	      return d_null_pointer;
	    }
	}
    }

  if (!outer_func)
    outer_func = nested_func->toParent2()->isFuncDeclaration();
  gcc_assert (outer_func != NULL);

  FuncFrameInfo *ffo = getFrameInfo (outer_func);
  if (ffo->creates_frame || ffo->static_chain)
    return getFrameRef (outer_func);

  return d_null_pointer;
}

// Starting from the current function, try to find a suitable value of
// 'this' in nested function instances.

// A suitable 'this' value is an instance of TARGET_CD or a class that
// has TARGET_CD as a base.

tree
IRState::findThis (ClassDeclaration *target_cd)
{
  FuncDeclaration *fd = func;

  while (fd)
    {
      AggregateDeclaration *fd_ad;
      ClassDeclaration *fd_cd;

      if ((fd_ad = fd->isThis()) &&
	  (fd_cd = fd_ad->isClassDeclaration()))
	{
	  if (target_cd == fd_cd)
	    {
	      return var (fd->vthis);
	    }
	  else if (target_cd->isBaseOf (fd_cd, NULL))
	    {
	      gcc_assert (fd->vthis); // && fd->vthis->csym->Stree
	      return convertTo (var (fd->vthis),
				fd_cd->type, target_cd->type);
	    }
	  else
	    {
	      fd = isClassNestedInFunction (fd_cd);
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

// Return the outer class/struct 'this' value.
// This is here mostly due to removing duplicate code,
// and clean implementation purposes.

tree
IRState::getVThis (Dsymbol *decl, Expression *e)
{
  tree vthis_value = NULL_TREE;

  ClassDeclaration *class_decl;
  StructDeclaration *struct_decl;

  if ((class_decl = decl->isClassDeclaration()))
    {
      Dsymbol *outer = class_decl->toParent2();
      ClassDeclaration *cd_outer = outer->isClassDeclaration();
      FuncDeclaration *fd_outer = outer->isFuncDeclaration();

      if (cd_outer)
	{
	  vthis_value = findThis (cd_outer);
	  if (vthis_value == NULL_TREE)
	    {
	      e->error ("outer class %s 'this' needed to 'new' nested class %s",
			cd_outer->toChars(), class_decl->toChars());
	    }
	}
      else if (fd_outer)
	{
	  /* If a class nested in a function has no methods
	     and there are no other nested functions,
	     lower_nested_functions is never called and any
	     STATIC_CHAIN_EXPR created here will never be
	     translated. Use a null pointer for the link in
	     this case. */
	  FuncFrameInfo *ffo = getFrameInfo (fd_outer);
	  if (ffo->creates_frame || ffo->static_chain ||
	      fd_outer->hasNestedFrameRefs())
	    {
	      // %% V2: rec_type->class_type
	      vthis_value = getFrameForNestedClass (class_decl);
	    }
	  else if (fd_outer->vthis)
	    vthis_value = var (fd_outer->vthis);
	  else
	    vthis_value = d_null_pointer;
	}
      else
	gcc_unreachable();
    }
  else if ((struct_decl = decl->isStructDeclaration()))
    {
      Dsymbol *outer = struct_decl->toParent2();
      FuncDeclaration *fd_outer = outer->isFuncDeclaration();
      // Assuming this is kept as trivial as possible.
      // NOTE: what about structs nested in structs nested in functions?
      if (fd_outer)
	{
	  FuncFrameInfo *ffo = getFrameInfo (fd_outer);
	  if (ffo->creates_frame || ffo->static_chain ||
	      fd_outer->hasNestedFrameRefs())
	    {
	      vthis_value = getFrameForNestedStruct (struct_decl);
	    }
	  else if (fd_outer->vthis)
	    vthis_value = var (fd_outer->vthis);
	  else
	    vthis_value = d_null_pointer;
	}
      else
	gcc_unreachable();
    }

  return vthis_value;
}

// Return the parent function of a nested class.

FuncDeclaration *
IRState::isClassNestedInFunction (ClassDeclaration *cd)
{
  FuncDeclaration *fd = NULL;
  while (cd && cd->isNested())
    {
      Dsymbol *dsym = cd->toParent2();
      if ((fd = dsym->isFuncDeclaration()))
	return fd;
      else
	cd = dsym->isClassDeclaration();
    }
  return NULL;
}

// Return the parent function of a nested struct.

FuncDeclaration *
IRState::isStructNestedInFunction (StructDeclaration *sd)
{
  FuncDeclaration *fd = NULL;
  while (sd && sd->isNested())
    {
      Dsymbol *dsym = sd->toParent2();
      if ((fd = dsym->isFuncDeclaration()))
	return fd;
      else
	sd = dsym->isStructDeclaration();
    }
  return NULL;
}


// Build static chain decl for FUNC to be passed to nested functions in D.

void
IRState::buildChain (FuncDeclaration *func)
{
  FuncFrameInfo *ffi = getFrameInfo (func);

  if (ffi->is_closure)
    {
      // Build closure pointer, which is initialised on heap.
      func->buildClosure (this);
      return;
    }

  if (!ffi->creates_frame)
    {
      if (ffi->static_chain)
	{
	  tree link = chainLink();
	  useChain (func, link);
	}
      return;
    }

  VarDeclarations *nestedVars = &func->closureVars;

  tree frame_rec_type = ffi->frame_rec;
  tree chain_link = chainLink();
  tree ptr_field;
  ListMaker fields;

  ptr_field = build_decl (BUILTINS_LOCATION, FIELD_DECL,
			  get_identifier ("__chain"), ptr_type_node);
  DECL_CONTEXT (ptr_field) = frame_rec_type;
  fields.chain (ptr_field);

  /* __ensure never becomes a closure, but could still be referencing parameters
     of the calling function.  So we add all parameters as nested refs. This is
     written as such so that all parameters appear at the front of the frame so
     that overriding methods match the same layout when inheriting a contract.  */
  if (global.params.useOut && func->fensure)
    {
      for (size_t i = 0; func->parameters && i < func->parameters->dim; i++)
	{
	  VarDeclaration *v = (*func->parameters)[i];
	  // Remove if already in nestedVars so can push to front.
	  for (size_t j = i; j < nestedVars->dim; j++)
	    {
	      Dsymbol *s = (*nestedVars)[j];
	      if (s == v)
		{
		  nestedVars->remove (j);
		  break;
		}
	    }
	  nestedVars->insert (i, v);
	}

      // Also add hidden 'this' to outer context.
      if (func->vthis)
	{
	  for (size_t i = 0; i < nestedVars->dim; i++)
	    {
	      Dsymbol *s = (*nestedVars)[i];
	      if (s == func->vthis)
		{
		  nestedVars->remove (i);
		  break;
		}
	    }
	  nestedVars->insert (0, func->vthis);
	}
    }

  for (size_t i = 0; i < nestedVars->dim; i++)
    {
      VarDeclaration *v = (*nestedVars)[i];
      Symbol *s = v->toSymbol();
      tree field = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
			       v->ident ? get_identifier (v->ident->string) : NULL_TREE,
			       gen.trueDeclarationType (v));
      s->SframeField = field;
      g.ofile->setDeclLoc (field, v);
      DECL_CONTEXT (field) = frame_rec_type;
      fields.chain (field);
      TREE_USED (s->Stree) = 1;

      /* Can't do nrvo if the variable is put in a frame.  */
      if (func->nrvo_can && func->nrvo_var == v)
	this->func->nrvo_can = 0;
    }

  TYPE_FIELDS (frame_rec_type) = fields.head;
  layout_type (frame_rec_type);

  tree frame_decl = localVar (frame_rec_type);
  tree frame_ptr = addressOf (frame_decl);
  DECL_IGNORED_P (frame_decl) = 0;
  expandDecl (frame_decl);

  // set the first entry to the parent frame, if any
  if (chain_link != NULL_TREE)
    {
      doExp (vmodify (component (indirect (frame_ptr), ptr_field),
		      chain_link));
    }

  // copy parameters that are referenced nonlocally
  for (size_t i = 0; i < nestedVars->dim; i++)
    {
      VarDeclaration *v = (*nestedVars)[i];
      if (!v->isParameter())
	continue;

      Symbol *vsym = v->toSymbol();
      doExp (vmodify (component (indirect (frame_ptr), vsym->SframeField),
		      vsym->Stree));
    }

  useChain (this->func, frame_ptr);
}

// Return the frame of FD.  This could be a static chain or a closure
// passed via the hidden 'this' pointer.

FuncFrameInfo *
IRState::getFrameInfo (FuncDeclaration *fd)
{
  Symbol *fds = fd->toSymbol();
  if (fds->frameInfo)
    return fds->frameInfo;

  FuncFrameInfo *ffi = new FuncFrameInfo;
  ffi->creates_frame = false;
  ffi->static_chain = false;
  ffi->is_closure = false;
  ffi->frame_rec = NULL_TREE;

  fds->frameInfo = ffi;

  VarDeclarations *nestedVars = &fd->closureVars;

  // Nested functions, or functions with nested refs must create
  // a static frame for local variables to be referenced from.
  if (nestedVars->dim != 0 || fd->isNested())
    {
      ffi->creates_frame = true;
    }
  else
    {
      AggregateDeclaration *ad = fd->isThis();
      if (ad && ad->isNested())
	{
	  ffi->creates_frame = true;
	}
    }

  // Functions with In/Out contracts pass parameters to nested frame.
  if (fd->fensure || fd->frequire)
    ffi->creates_frame = true;

  // D2 maybe setup closure instead.
  if (fd->needsClosure())
    {
      ffi->creates_frame = true;
      ffi->is_closure = true;
    }
  else if (nestedVars->dim == 0)
    {
      /* If fd is nested (deeply) in a function that creates a closure,
	 then fd inherits that closure via hidden vthis pointer, and
	 doesn't create a stack frame at all.  */
      FuncDeclaration *ff = fd;

      while (ff)
	{
	  FuncFrameInfo *ffo = getFrameInfo (ff);

	  if (ff != fd && ffo->creates_frame)
	    {
	      gcc_assert (ffo->frame_rec);
	      ffi->creates_frame = false;
	      ffi->static_chain = true;
	      ffi->is_closure = ffo->is_closure;
	      gcc_assert (COMPLETE_TYPE_P (ffo->frame_rec));
	      ffi->frame_rec = copy_node (ffo->frame_rec);
	      break;
	    }

	  // Stop looking if no frame pointer for this function.
	  if (ff->vthis == NULL)
	    break;

	  ff = ff->toParent2()->isFuncDeclaration();
	}
    }

  // Build type now as may be referenced from another module.
  if (ffi->creates_frame)
    {
      tree frame_rec = make_node (RECORD_TYPE);
      char *name = concat (ffi->is_closure ? "CLOSURE." : "FRAME.",
			   fd->toPrettyChars(), NULL);
      TYPE_NAME (frame_rec) = get_identifier (name);
      free (name);

      d_keep (frame_rec);
      ffi->frame_rec = frame_rec;
    }

  return ffi;
}

// Return a pointer to the frame/closure block of OUTER_FUNC.

tree
IRState::getFrameRef (FuncDeclaration *outer_func)
{
  tree result = chainLink();
  FuncDeclaration *fd = chainFunc();

  while (fd && fd != outer_func)
    {
      AggregateDeclaration *ad;
      ClassDeclaration *cd;
      StructDeclaration *sd;

      if (getFrameInfo (fd)->creates_frame)
	{
	  // like compon (indirect, field0) parent frame link is the first field;
	  result = indirect (ptr_type_node, result);
	}

      if (fd->isNested())
	fd = fd->toParent2()->isFuncDeclaration();
      /* getFrameRef is only used to get the pointer to a function's frame
	 (not a class instances.)  With the current implementation, the link
	 the frame/closure record always points to the outer function's frame even
	 if there are intervening nested classes or structs.
	 So, we can just skip over those... */
	 else if ((ad = fd->isThis()) && (cd = ad->isClassDeclaration()))
	   fd = isClassNestedInFunction (cd);
	 else if ((ad = fd->isThis()) && (sd = ad->isStructDeclaration()))
	   fd = isStructNestedInFunction (sd);
	 else
	   break;
    }

  if (fd == outer_func)
    {
      tree frame_rec = getFrameInfo (outer_func)->frame_rec;

      if (frame_rec != NULL_TREE)
	{
	  result = nop (build_pointer_type (frame_rec), result);
	  return result;
	}
      else
	{
	  this->func->error ("forward reference to frame of %s", outer_func->toChars());
	  return d_null_pointer;
	}
    }
  else
    {
      this->func->error ("cannot access frame of %s", outer_func->toChars());
      return d_null_pointer;
    }
}

// Return true if function F needs to have the static chain passed to
// it.  This only applies to nested function handling provided by the
// GCC back end (not D closures.)

bool
IRState::functionNeedsChain (FuncDeclaration *f)
{
  Dsymbol *s;
  ClassDeclaration *a;
  FuncDeclaration *pf = NULL;
  TemplateInstance *ti = NULL;

  if (f->isNested())
    {
      s = f->toParent();
      ti = s->isTemplateInstance();
      if (ti && ti->isnested == NULL)
	return false;

      pf = f->toParent2()->isFuncDeclaration();
      if (pf && !getFrameInfo (pf)->is_closure)
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
	  && !getFrameInfo (pf)->is_closure)
	{
	  return true;
	}
    }

  return false;
}


// Routines for building statement lists around if/else conditions.
// STMT contains the statement to be executed if T_COND is true.

void
IRState::startCond (Statement *stmt, tree t_cond)
{
  Flow *f = beginFlow (stmt);
  f->condition = t_cond;
}

void
IRState::startCond (Statement *stmt, Expression *e_cond)
{
  tree t_cond = e_cond->toElemDtor (this);
  startCond (stmt, convertForCondition (t_cond, e_cond->type));
}

// Start a new statement list for the false condition branch.

void
IRState::startElse (void)
{
  currentFlow()->trueBranch = popStatementList();
  pushStatementList();
}

// Wrap up our constructed if condition into a COND_EXPR.

void
IRState::endCond (void)
{
  Flow *f = currentFlow();
  tree t_brnch = popStatementList();
  tree t_false_brnch = NULL_TREE;

  if (f->trueBranch == NULL_TREE)
    f->trueBranch = t_brnch;
  else
    t_false_brnch = t_brnch;

  g.ofile->doLineNote (f->statement->loc);
  tree t_stmt = build3 (COND_EXPR, void_type_node,
			f->condition, f->trueBranch, t_false_brnch);
  endFlow();
  addExp (t_stmt);
}


// Routines for building statement lists around for/while loops.
// STMT is the body of the loop.

void
IRState::startLoop (Statement *stmt)
{
  Flow *f = beginFlow (stmt);
  // should be end for 'do' loop
  f->continueLabel = label (stmt ? stmt->loc : 0);
}

// Emit continue label for loop.

void
IRState::continueHere (void)
{
  doLabel (currentFlow()->continueLabel);
}

// Set LBL as the continue label for the current loop.
// Used in unrolled loop statements.

void
IRState::setContinueLabel (tree lbl)
{
  currentFlow()->continueLabel = lbl;
}

// Emit exit loop condition.

void
IRState::exitIfFalse (tree t_cond)
{
  addExp (build1 (EXIT_EXPR, void_type_node,
		  build1 (TRUTH_NOT_EXPR, TREE_TYPE (t_cond), t_cond)));
}

void
IRState::exitIfFalse (Expression *e_cond)
{
  tree t_cond = e_cond->toElemDtor (this);
  exitIfFalse (convertForCondition (t_cond, e_cond->type));
}

// Emit a goto to the continue label IDENT of a loop.

void
IRState::continueLoop (Identifier *ident)
{
  doJump (NULL, getLoopForLabel (ident, true)->continueLabel);
}

// Emit a goto to the exit label IDENT of a loop.

void
IRState::exitLoop (Identifier *ident)
{
  Flow *flow = getLoopForLabel (ident);
  if (!flow->exitLabel)
    flow->exitLabel = label (flow->statement->loc);
  doJump (NULL, flow->exitLabel);
}

// Wrap up constructed loop body in a LOOP_EXPR.

void
IRState::endLoop (void)
{
  // says must contain an EXIT_EXPR -- what about while (1)..goto;? something other thand LOOP_EXPR?
  tree t_body = popStatementList();
  tree t_loop = build1 (LOOP_EXPR, void_type_node, t_body);
  addExp (t_loop);
  endFlow();
}


// Routines for building statement lists around switches.  STMT is the body
// of the switch statement, T_COND is the condition to the switch. If HAS_VARS
// is true, then the switch statement has been converted to an if-then-else.

void
IRState::startCase (Statement *stmt, tree t_cond, int has_vars)
{
  Flow *f = beginFlow (stmt);
  f->condition = t_cond;
  f->kind = level_switch;
  if (has_vars)
    {
      // %% dummy value so the tree is not NULL
      f->hasVars = integer_one_node;
    }
}

// Emit a case statement for T_VALUE.

void
IRState::doCase (tree t_value, tree t_label)
{
  if (currentFlow()->hasVars)
    {
      // SwitchStatement has already taken care of label jumps.
      doLabel (t_label);
    }
  else
    {
      tree t_case = build_case_label (t_value, NULL_TREE, t_label);
      addExp (t_case);
    }
}

// Wrap up constructed body into a SWITCH_EXPR.

void
IRState::endCase (void)
{
  Flow *f = currentFlow();
  tree t_body = popStatementList();
  tree t_condtype = TREE_TYPE (f->condition);
  if (f->hasVars)
    {
      // %% switch was converted to if-then-else expression
      addExp (t_body);
    }
  else
    {
      tree t_stmt = build3 (SWITCH_EXPR, t_condtype, f->condition,
			    t_body, NULL_TREE);
      addExp (t_stmt);
    }
  endFlow();
}

// Routines for building statement lists around try/catch/finally.
// Start a try statement, STMT is the body of the try expression.

void
IRState::startTry (Statement *stmt)
{
  beginFlow (stmt);
  currentFlow()->kind = level_try;
}

// Pops the try body and starts a new statement list for all catches.

void
IRState::startCatches (void)
{
  currentFlow()->tryBody = popStatementList();
  currentFlow()->kind = level_catch;
  pushStatementList();
}

// Start a new catch expression for exception type T_TYPE.

void
IRState::startCatch (tree t_type)
{
  currentFlow()->catchType = t_type;
  pushStatementList();
}

// Wrap up catch expression into a CATCH_EXPR.

void
IRState::endCatch (void)
{
  tree t_body = popStatementList();
  // % Wrong loc... can set pass statement to startCatch, set
  // The loc on t_type and then use it here...
  doExp (build2 (CATCH_EXPR, void_type_node,
		 currentFlow()->catchType, t_body));
}

// Wrap up try/catch into a TRY_CATCH_EXPR.

void
IRState::endCatches (void)
{
  tree t_catches = popStatementList();
  g.ofile->doLineNote (currentFlow()->statement->loc);
  doExp (build2 (TRY_CATCH_EXPR, void_type_node,
		 currentFlow()->tryBody, t_catches));
  endFlow();
}

// Start a new finally expression.

void
IRState::startFinally (void)
{
  currentFlow()->tryBody = popStatementList();
  currentFlow()->kind = level_finally;
  pushStatementList();
}

// Wrap-up try/finally into a TRY_FINALLY_EXPR.

void
IRState::endFinally (void)
{
  tree t_finally = popStatementList();
  g.ofile->doLineNote (currentFlow()->statement->loc);
  doExp (build2 (TRY_FINALLY_EXPR, void_type_node,
		 currentFlow()->tryBody, t_finally));
  endFlow();
}

// Emit a return expression of value T_VALUE.

void
IRState::doReturn (tree t_value)
{
  addExp (build1 (RETURN_EXPR, void_type_node, t_value));
}

// Emit goto expression to T_LABEL.

void
IRState::doJump (Statement *stmt, tree t_label)
{
  if (stmt)
    g.ofile->doLineNote (stmt->loc);
  addExp (build1 (GOTO_EXPR, void_type_node, t_label));
  TREE_USED (t_label) = 1;
}

// Emit statement T to function body.

void
IRState::doExp (tree t)
{
  addExp (t);
}

void
IRState::doExp (Expression *e)
{
  // %% should handle volatile...?
  addExp (e->toElem (this));
}

// Emit assembler statement INSN_TMPL into current body.

void
IRState::doAsm (tree insn_tmpl, tree outputs, tree inputs, tree clobbers)
{
  tree t = d_build_asm_stmt (insn_tmpl, outputs, inputs, clobbers);
  ASM_VOLATILE_P (t) = 1;
  addExp (t);
}

// Routines for checking goto statements don't jump to invalid locations.
// In particular, it is illegal for a goto to be used to skip initializations. 
// Saves the block label L is declared in for analysis later.

void
IRState::pushLabel (LabelDsymbol *l)
{
  this->labels.push (getLabelBlock (l));
}

// Error if STMT is in it's own try statement separate from other
// cases in the switch statement.

void
IRState::checkSwitchCase (Statement *stmt, int default_flag)
{
  Flow *flow = currentFlow();

  gcc_assert (flow);
  if (flow->kind != level_switch && flow->kind != level_block)
    {
      stmt->error ("%s cannot be in different try block level from switch",
		   default_flag ? "default" : "case");
    }
}

// Error if the goto referencing LABEL is jumping into a try or
// catch block.  STMT is required to error on the correct line.

void
IRState::checkGoto (Statement *stmt, LabelDsymbol *label)
{
  Statement *curBlock = NULL;
  unsigned curLevel = this->loops.dim;
  int found = 0;

  if (curLevel)
    curBlock = currentFlow()->statement;

  for (size_t i = 0; i < this->labels.dim; i++)
    {
      Label *linfo = this->labels[i];
      gcc_assert (linfo);

      if (label == linfo->label)
	{
	  // No need checking for finally, should have already been handled.
	  if (linfo->kind == level_try &&
	      curLevel <= linfo->level && curBlock != linfo->block)
	    {
	      stmt->error ("cannot goto into try block");
	    }
	  // %% doc: It is illegal for goto to be used to skip initializations,
	  // %%      so this should include all gotos into catches...
	  if (linfo->kind == level_catch && curBlock != linfo->block)
	    stmt->error ("cannot goto into catch block");

	  found = 1;
	  break;
	}
    }
  // Push forward referenced gotos.
  if (!found)
    {
      if (!label->statement->fwdrefs)
	label->statement->fwdrefs = new Array();
      label->statement->fwdrefs->push (getLabelBlock (label, stmt));
    }
}

// Check all forward references REFS for a label, and error
// if goto is jumping into a try or catch block.

void
IRState::checkPreviousGoto (Array *refs)
{
  Statement *stmt; // Our forward reference.

  for (size_t i = 0; i < refs->dim; i++)
    {
      Label *ref = (Label *) refs->data[i];
      int found = 0;

      gcc_assert (ref && ref->from);
      stmt = ref->from;

      for (size_t i = 0; i < this->labels.dim; i++)
	{
	  Label *linfo = this->labels[i];
	  gcc_assert (linfo);

	  if (ref->label == linfo->label)
	    {
	      // No need checking for finally, should have already been handled.
	      if (linfo->kind == level_try &&
		  ref->level <= linfo->level && ref->block != linfo->block)
		{
		  stmt->error ("cannot goto into try block");
		}
	      // %% doc: It is illegal for goto to be used to skip initializations,
	      // %%      so this should include all gotos into catches...
	      if (linfo->kind == level_catch &&
		  (ref->block != linfo->block || ref->kind != linfo->kind))
		stmt->error ("cannot goto into catch block");

	      found = 1;
	      break;
	    }
	}
      gcc_assert (found);
    }
}

// Construct a WrappedExp, whose components are an EXP_NODE, which contains
// a list of instructions in GCC to be passed through.

WrappedExp::WrappedExp (Loc loc, enum TOK op, tree exp_node, Type *type)
    : Expression (loc, op, sizeof (WrappedExp))
{
  this->exp_node = exp_node;
  this->type = type;
}

// Write C-style representation of WrappedExp to BUF.

void
WrappedExp::toCBuffer (OutBuffer *buf, HdrGenState *hgs)
{
  buf->printf ("<wrapped expression>");
}

// Build and return expression tree for WrappedExp.

elem *
WrappedExp::toElem (IRState *)
{
  return exp_node;
}

// Write out all fields for aggregate DECL.  For classes, write
// out base class fields first, and adds all interfaces last.

void
AggLayout::visit (AggregateDeclaration *decl)
{
  ClassDeclaration *class_decl = decl->isClassDeclaration();

  if (class_decl && class_decl->baseClass)
    AggLayout::visit (class_decl->baseClass);

  if (decl->fields.dim)
    doFields (&decl->fields, decl);

  if (class_decl && class_decl->vtblInterfaces)
    doInterfaces (class_decl->vtblInterfaces);
}


// Add all FIELDS into aggregate AGG.

void
AggLayout::doFields (VarDeclarations *fields, AggregateDeclaration *agg)
{
  bool inherited = agg != this->aggDecl_;
  tree fcontext;

  fcontext = agg->type->toCtype();
  if (POINTER_TYPE_P (fcontext))
    fcontext = TREE_TYPE (fcontext);

  for (size_t i = 0; i < fields->dim; i++)
    {
      // %% D anonymous unions just put the fields into the outer struct...
      // does this cause problems?
      VarDeclaration *var_decl = fields->tdata()[i];
      gcc_assert (var_decl && var_decl->storage_class & STCfield);

      tree ident = var_decl->ident ? get_identifier (var_decl->ident->string) : NULL_TREE;
      tree field_decl = build_decl (UNKNOWN_LOCATION, FIELD_DECL, ident,
				    gen.trueDeclarationType (var_decl));
      g.ofile->setDeclLoc (field_decl, var_decl);
      var_decl->csym = new Symbol;
      var_decl->csym->Stree = field_decl;

      DECL_CONTEXT (field_decl) = this->aggType_;
      DECL_FCONTEXT (field_decl) = fcontext;
      DECL_FIELD_OFFSET (field_decl) = size_int (var_decl->offset);
      DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;

      DECL_ARTIFICIAL (field_decl) = DECL_IGNORED_P (field_decl) = inherited;

      // GCC requires DECL_OFFSET_ALIGN to be set
      // %% using TYPE_ALIGN may not be same as DMD ...
      SET_DECL_OFFSET_ALIGN (field_decl, TYPE_ALIGN (TREE_TYPE (field_decl)));
      layout_decl (field_decl, 0);

      if (var_decl->size (var_decl->loc))
	{
	  gcc_assert (DECL_MODE (field_decl) != VOIDmode);
	  gcc_assert (DECL_SIZE (field_decl) != NULL_TREE);
	}
      this->fieldList_.chain (field_decl);
    }
}

// Write out all interfaces BASES for a class.

void
AggLayout::doInterfaces (BaseClasses *bases)
{
  for (size_t i = 0; i < bases->dim; i++)
    {
      BaseClass *bc = bases->tdata()[i];
      tree decl = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE,
			      Type::tvoidptr->pointerTo()->toCtype());
      DECL_ARTIFICIAL (decl) = 1;
      DECL_IGNORED_P (decl) = 1;
      addField (decl, bc->offset);
    }
}

// Add single field FIELD_DECL at OFFSET into aggregate.

void
AggLayout::addField (tree field_decl, size_t offset)
{
  DECL_CONTEXT (field_decl) = this->aggType_;
  SET_DECL_OFFSET_ALIGN (field_decl, TYPE_ALIGN (TREE_TYPE (field_decl)));
  DECL_FIELD_OFFSET (field_decl) = size_int (offset);
  DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;
  Loc l (this->aggDecl_->getModule(), 1); // Must set this or we crash with DWARF debugging
  g.ofile->setDeclLoc (field_decl, l);

  layout_decl (field_decl, 0);
  this->fieldList_.chain (field_decl);
}

// Wrap-up and compute finalised aggregate type.  ATTRS are
// if any GCC attributes were applied to the type declaration.

void
AggLayout::finish (Expressions *attrs)
{
  unsigned size_to_use = this->aggDecl_->structsize;
  unsigned align_to_use = this->aggDecl_->alignsize;

  TYPE_SIZE (this->aggType_) = bitsize_int (size_to_use * BITS_PER_UNIT);
  TYPE_SIZE_UNIT (this->aggType_) = size_int (size_to_use);
  TYPE_ALIGN (this->aggType_) = align_to_use * BITS_PER_UNIT;
  TYPE_PACKED (this->aggType_) = TYPE_PACKED (this->aggType_); // %% todo

  if (attrs)
    {
      decl_attributes (&this->aggType_, gen.attributes (attrs),
		       ATTR_FLAG_TYPE_IN_PLACE);
    }

  compute_record_mode (this->aggType_);
  
  // Set up variants.
  for (tree x = TYPE_MAIN_VARIANT (this->aggType_); x; x = TYPE_NEXT_VARIANT (x))
    {
      TYPE_FIELDS (x) = TYPE_FIELDS (this->aggType_);
      TYPE_LANG_SPECIFIC (x) = TYPE_LANG_SPECIFIC (this->aggType_);
      TYPE_ALIGN (x) = TYPE_ALIGN (this->aggType_);
      TYPE_USER_ALIGN (x) = TYPE_USER_ALIGN (this->aggType_);
    }
}

// Routines for getting an index or slice of an array where '$' was used
// in the slice.  A temp var INI_V would have been created that needs to
// be bound into it's own scope.

ArrayScope::ArrayScope (IRState *irs, VarDeclaration *ini_v, const Loc& loc) :
  var_(ini_v)
{
  if (this->var_)
    {
      /* Need to set the location or the expand_decl in the BIND_EXPR will
	 cause the line numbering for the statement to be incorrect. */
      /* The variable itself is not included in the debugging information. */
      this->var_->loc = loc;
      Symbol *s = this->var_->toSymbol();
      tree decl = s->Stree;
      DECL_CONTEXT (decl) = irs->getLocalContext();
    }
}

// Set index expression E of type T as the initialiser for
// the temp var decl to be used.

tree
ArrayScope::setArrayExp (IRState *irs, tree e, Type *t)
{
  /* If STCconst, the value will be assigned in d-decls.cc
     of the runtime length of the array expression. */
  if (this->var_ && !(this->var_->storage_class & STCconst))
    {
      tree v = this->var_->toSymbol()->Stree;
      if (t->toBasetype()->ty != Tsarray)
	e = irs->maybeMakeTemp (e);
      DECL_INITIAL (v) = irs->arrayLength (e, t);
    }
  return e;
}

// Wrap-up temp var into a BIND_EXPR.

tree
ArrayScope::finish (IRState *irs, tree e)
{
  if (this->var_)
    {
      Symbol *s = this->var_->toSymbol();
      tree t = s->Stree;
      if (TREE_CODE (t) == VAR_DECL)
	{
	  gcc_assert (!s->SframeField);
	  return gen.binding (t, e);
	}
      else
	gcc_unreachable();
    }
  return e;
}

