// d-codegen.cc -- D frontend for GCC.
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

#include "d-system.h"
#include "d-lang.h"
#include "d-codegen.h"

#include "attrib.h"
#include "template.h"
#include "init.h"
#include "id.h"
#include "module.h"
#include "dfrontend/target.h"


Module *current_module_decl;
IRState *current_irstate;


// Return the DECL_CONTEXT for symbol DSYM.

tree
d_decl_context (Dsymbol *dsym)
{
  Dsymbol *parent = dsym;

  while ((parent = parent->toParent2()))
    {
      // Nested functions.
      if (parent->isFuncDeclaration())
	return parent->toSymbol()->Stree;

      // Methods of classes or structs.
      AggregateDeclaration *ad = parent->isAggregateDeclaration();
      if (ad != NULL)
	{
	  tree context = ad->type->toCtype();
	  // Want the underlying RECORD_TYPE.
	  if (ad->isClassDeclaration())
	    context = TREE_TYPE (context);

	  return context;
	}

      // We've reached the top-level module namespace.
      // Set DECL_CONTEXT as the NAMESPACE_DECL of the enclosing module,
      // but only for extern(D) symbols.
      if (parent->isModule())
	{
	  Declaration *decl = dsym->isDeclaration();
	  if (decl != NULL && decl->linkage != LINKd)
	    return NULL_TREE;

	  return parent->toImport()->Stree;
	}
    }

  return NULL_TREE;
}

// Add local variable VD into the current body of function fd.

void
build_local_var (VarDeclaration *vd, FuncDeclaration *fd)
{
  gcc_assert (!vd->isDataseg() && !vd->isMember());

  Symbol *sym = vd->toSymbol();
  tree var = sym->Stree;

  gcc_assert (!TREE_STATIC (var));

  set_input_location (vd->loc);
  d_pushdecl (var);
  DECL_CONTEXT (var) = fd->toSymbol()->Stree;

  // Compiler generated symbols
  if (vd == fd->vresult || vd == fd->v_argptr || vd == fd->v_arguments_var)
    DECL_ARTIFICIAL (var) = 1;

  if (sym->SframeField)
    {
      // Fixes debugging local variables.
      SET_DECL_VALUE_EXPR (var, get_decl_tree (vd, fd));
      DECL_HAS_VALUE_EXPR_P (var) = 1;
    }
}

// Return an unnamed local temporary of type TYPE.

tree
build_local_temp (tree type)
{
  tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL, NULL_TREE, type);

  DECL_CONTEXT (decl) = current_function_decl;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_IGNORED_P (decl) = 1;
  d_pushdecl (decl);

  return decl;
}

// Return an undeclared local temporary of type TYPE
// for use with BIND_EXPR.

tree
create_temporary_var (tree type)
{
  tree decl = build_decl (BUILTINS_LOCATION, VAR_DECL, NULL_TREE, type);
  DECL_CONTEXT (decl) = current_function_decl;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_IGNORED_P (decl) = 1;
  layout_decl (decl, 0);
  return decl;
}

// Return an undeclared local temporary OUT_VAR initialised
// with result of expression EXP.

tree
maybe_temporary_var (tree exp, tree *out_var)
{
  tree t = exp;

  // Get the base component.
  while (TREE_CODE (t) == COMPONENT_REF)
    t = TREE_OPERAND (t, 0);

  if (!DECL_P (t) && !REFERENCE_CLASS_P (t))
    {
      *out_var = create_temporary_var (TREE_TYPE (exp));
      DECL_INITIAL (*out_var) = exp;
      return *out_var;
    }
  else
    {
      *out_var = NULL_TREE;
      return exp;
    }
}

// Emit an INIT_EXPR for decl DECL.

void
expand_decl (tree decl)
{
  // Nothing, d_pushdecl will add decl to a BIND_EXPR
  if (DECL_INITIAL (decl))
    {
      tree exp = build_vinit (decl, DECL_INITIAL (decl));
      current_irstate->addExp (exp);
      DECL_INITIAL (decl) = NULL_TREE;
    }
}

// Return the correct decl to be used for variable DECL accessed from
// function FUNC.  Could be a VAR_DECL, or a FIELD_DECL from a closure.

tree
get_decl_tree (Declaration *decl, FuncDeclaration *func)
{
  VarDeclaration *vd = decl->isVarDeclaration();

  if (vd)
    {
      Symbol *vsym = vd->toSymbol();
      if (vsym->SnamedResult != NULL_TREE)
	{
	  // Get the named return value.
	  gcc_assert (TREE_CODE (vsym->SnamedResult) == RESULT_DECL);
	  return vsym->SnamedResult;
	}
      else if (vsym->SframeField != NULL_TREE)
	{
    	  // Get the closure holding the var decl.
    	  FuncDeclaration *parent = vd->toParent2()->isFuncDeclaration();
    	  tree frame_ref = get_framedecl (func, parent);

    	  return component_ref (build_deref (frame_ref), vsym->SframeField);
    	}
      else if (vd->parent != func && vd->isThisDeclaration() && func->isThis())
	{
	  // Get the non-local 'this' value by going through parent link
	  // of nested classes.
	  AggregateDeclaration *ad = func->isThis();
	  tree this_tree = func->vthis->toSymbol()->Stree;

	  while (ad->isNested())
	    {
	      Dsymbol *outer = ad->toParent2();
	      tree vthis_field = ad->vthis->toSymbol()->Stree;
	      this_tree = component_ref (build_deref (this_tree), vthis_field);

	      ad = outer->isAggregateDeclaration();
	      if (ad == NULL)
		{
		  gcc_assert (outer == vd->parent);
		  return this_tree;
		}
	    }

	  gcc_unreachable();
	}
    }

  // Static var or auto var that the back end will handle for us
  return decl->toSymbol()->Stree;
}

// Return expression EXP, whose type has been converted to TYPE.

tree
d_convert (tree type, tree exp)
{
  // Check this first before passing to lang_dtype.
  if (error_operand_p (type) || error_operand_p (exp))
    return error_mark_node;

  Type *totype = lang_dtype (type);
  Type *etype = lang_dtype (TREE_TYPE (exp));

  if (totype && etype)
    return convert_expr (exp, etype, totype);

  return convert (type, exp);
}

// Return expression EXP, whose type has been convert from ETYPE to TOTYPE.

tree
convert_expr (tree exp, Type *etype, Type *totype)
{
  tree result = NULL_TREE;

  gcc_assert (etype && totype);
  Type *ebtype = etype->toBasetype();
  Type *tbtype = totype->toBasetype();

  if (d_types_same (etype, totype))
    return exp;

  if (error_operand_p (exp))
    return exp;

  switch (ebtype->ty)
    {
    case Tdelegate:
      if (tbtype->ty == Tdelegate)
	{
	  exp = maybe_make_temp (exp);
	  return build_delegate_cst (delegate_method (exp), delegate_object (exp), totype);
	}
      else if (tbtype->ty == Tpointer)
	{
	  // The front-end converts <delegate>.ptr to cast (void *)<delegate>.
	  // Maybe should only allow void* ?
	  exp = delegate_object (exp);
	}
      else
	{
	  error ("can't convert a delegate expression to %s", totype->toChars());
	  return error_mark_node;
	}
      break;

    case Tstruct:
      if (tbtype->ty == Tstruct)
      {
	if (totype->size() == etype->size())
	  {
	    // Allowed to cast to structs with same type size.
	    result = build_vconvert(totype->toCtype(), exp);
	  }
	else
	  {
	    error("can't convert struct %s to %s", etype->toChars(), totype->toChars());
	    return error_mark_node;
	  }
      }
      // else, default conversion, which should produce an error
      break;

    case Tclass:
      if (tbtype->ty == Tclass)
      {
	ClassDeclaration *cdfrom = ebtype->isClassHandle();
	ClassDeclaration *cdto = tbtype->isClassHandle();
	int offset;

	if (cdfrom->cpp)
	  {
	    // Downcasting in C++ is a no-op.
	    if (cdto->cpp)
	      break;

	    // Casting from a C++ interface to a class/non-C++ interface
	    // always results in null as there is no runtime information,
	    // and no way one can derive from the other.
	    warning(OPT_Wcast_result, "cast to %s will produce null result", totype->toChars());
	    result = d_convert(totype->toCtype(), null_pointer_node);

	    // Make sure the expression is still evaluated if necessary
	    if (TREE_SIDE_EFFECTS(exp))
	      result = compound_expr(exp, result);

	    return result;
	  }

	if (cdto->isBaseOf(cdfrom, &offset) && offset != OFFSET_RUNTIME)
	  {
	    // Casting up the inheritance tree: Don't do anything special.
	    // Cast to an implemented interface: Handle at compile time.
	    if (offset)
	      {
		tree t = totype->toCtype();
		exp = maybe_make_temp(exp);
		return build3(COND_EXPR, t,
			      build_boolop(NE_EXPR, exp, null_pointer_node),
			      build_nop(t, build_offset(exp, size_int(offset))),
			      build_nop(t, null_pointer_node));
	      }

	    // d_convert will make a no-op cast
	    break;
	  }

	// The offset can only be determined at runtime, do dynamic cast
	tree args[2];
	args[0] = exp;
	args[1] = build_address(cdto->toSymbol()->Stree);

	return build_libcall(cdfrom->isInterfaceDeclaration()
			     ? LIBCALL_INTERFACE_CAST : LIBCALL_DYNAMIC_CAST, 2, args);
      }
      // else default conversion
      break;

    case Tsarray:
      if (tbtype->ty == Tpointer)
	{
	  result = build_nop (totype->toCtype(), build_address (exp));
	}
      else if (tbtype->ty == Tarray)
	{
	  dinteger_t dim = ((TypeSArray *) ebtype)->dim->toInteger();
	  dinteger_t esize = ebtype->nextOf()->size();
	  dinteger_t tsize = tbtype->nextOf()->size();

	  tree ptrtype = tbtype->nextOf()->pointerTo()->toCtype();

	  if ((dim * esize) % tsize != 0)
	    {
	      error ("cannot cast %s to %s since sizes don't line up",
		     etype->toChars(), totype->toChars());
	      return error_mark_node;
	    }
	  dim = (dim * esize) / tsize;

	  // Assumes casting to dynamic array of same type or void
	  return d_array_value (totype->toCtype(), size_int (dim),
				build_nop (ptrtype, build_address (exp)));
	}
      else if (tbtype->ty == Tsarray)
	{
	  // D apparently allows casting a static array to any static array type
	  return build_vconvert (totype->toCtype(), exp);
	}
      else if (tbtype->ty == Tstruct)
	{
	  // And allows casting a static array to any struct type too.
	  // %% type sizes should have already been checked by the frontend.
	  gcc_assert (totype->size() == etype->size());
	  result = build_vconvert (totype->toCtype(), exp);
	}
      else
	{
	  error ("cannot cast expression of type %s to type %s",
		 etype->toChars(), totype->toChars());
	  return error_mark_node;
	}
      break;

    case Tarray:
      if (tbtype->ty == Tpointer)
	{
	  return d_convert (totype->toCtype(), d_array_ptr (exp));
	}
      else if (tbtype->ty == Tarray)
	{
	  // assume tvoid->size() == 1
	  Type *src_elem_type = ebtype->nextOf()->toBasetype();
	  Type *dst_elem_type = tbtype->nextOf()->toBasetype();
	  d_uns64 sz_src = src_elem_type->size();
	  d_uns64 sz_dst = dst_elem_type->size();

	  if (sz_src == sz_dst)
	    {
	      // Convert from void[] or elements are the same size -- don't change length
	      return build_vconvert (totype->toCtype(), exp);
	    }
	  else
	    {
	      unsigned mult = 1;
	      tree args[3];

	      args[0] = build_integer_cst (sz_dst, Type::tsize_t->toCtype());
	      args[1] = build_integer_cst (sz_src * mult, Type::tsize_t->toCtype());
	      args[2] = exp;

	      return build_libcall (LIBCALL_ARRAYCAST, 3, args, totype->toCtype());
	    }
	}
      else if (tbtype->ty == Tsarray)
	{
	  // %% Strings are treated as dynamic arrays D2.
	  if (ebtype->isString() && tbtype->isString())
	    return indirect_ref (totype->toCtype(), d_array_ptr (exp));
	}
      else
	{
	  error ("cannot cast expression of type %s to %s",
		 etype->toChars(), totype->toChars());
	  return error_mark_node;
	}
      break;

    case Taarray:
      if (tbtype->ty == Taarray)
	return build_vconvert (totype->toCtype(), exp);
      // Can convert associative arrays to void pointers.
      else if (tbtype->ty == Tpointer && tbtype->nextOf()->ty == Tvoid)
	return build_vconvert (totype->toCtype(), exp);
      // else, default conversion, which should product an error
      break;

    case Tpointer:
      // Can convert void pointers to associative arrays too...
      if (tbtype->ty == Taarray && ebtype->nextOf()->ty == Tvoid)
	return build_vconvert (totype->toCtype(), exp);
      break;

    case Tnull:
      if (tbtype->ty == Tarray)
	{
	  tree ptrtype = tbtype->nextOf()->pointerTo()->toCtype();
	  return d_array_value(totype->toCtype(), size_int(0),
			       build_nop(ptrtype, exp));
	}
      else if (tbtype->ty == Taarray)
	  return build_vconvert (totype->toCtype(), exp);
      else if (tbtype->ty == Tdelegate)
	  return build_delegate_cst(exp, null_pointer_node, totype);
      break;

    case Tvector:
      if (tbtype->ty == Tsarray)
	{
	  if (tbtype->size() == ebtype->size())
	    return build_vconvert (totype->toCtype(), exp);
	}
      break;

    default:
      exp = fold_convert (etype->toCtype(), exp);
      gcc_assert (TREE_CODE (exp) != STRING_CST);
      break;
    }

  return result ? result :
    convert (totype->toCtype(), exp);
}


// Apply semantics of assignment to a values of type TOTYPE to EXPR
// (e.g., pointer = array -> pointer = &array[0])

// Return a TREE representation of EXPR implictly converted to TOTYPE
// for use in assignment expressions MODIFY_EXPR, INIT_EXPR...

tree
convert_for_assignment (tree expr, Type *etype, Type *totype)
{
  Type *ebtype = etype->toBasetype();
  Type *tbtype = totype->toBasetype();

  // Assuming this only has to handle converting a non Tsarray type to
  // arbitrarily dimensioned Tsarrays.
  if (tbtype->ty == Tsarray)
    {
      Type *telem = tbtype->nextOf()->baseElemOf();

      if (d_types_same (telem, ebtype))
	{
	  // %% what about implicit converions...?
	  TypeSArray *sa_type = (TypeSArray *) tbtype;
	  uinteger_t count = sa_type->dim->toUInteger();

	  tree ctor = build_constructor (totype->toCtype(), NULL);
	  if (count)
	    {
	      vec<constructor_elt, va_gc> *ce = NULL;
	      tree index = build2 (RANGE_EXPR, Type::tsize_t->toCtype(),
				   integer_zero_node, build_integer_cst (count - 1));
	      tree value = convert_for_assignment (expr, etype, sa_type->next);

	      // Can't use VAR_DECLs in CONSTRUCTORS.
	      if (TREE_CODE (value) == VAR_DECL)
		{
		  value = DECL_INITIAL (value);
		  gcc_assert (value);
		}

	      CONSTRUCTOR_APPEND_ELT (ce, index, value);
	      CONSTRUCTOR_ELTS (ctor) = ce;
	    }
	  TREE_READONLY (ctor) = 1;
	  TREE_CONSTANT (ctor) = 1;
	  return ctor;
	}
    }

  if (tbtype->ty == Tstruct && ebtype->isintegral())
    {
      // D Front end uses IntegerExp (0) to mean zero-init a structure.
      // Use memset to fill struct.
      if (integer_zerop (expr))
	{
	  StructDeclaration *sd = ((TypeStruct *) tbtype)->sym;
	  tree var = build_local_temp (totype->toCtype());

	  tree init = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMSET), 3,
					 build_address (var), expr,
					 size_int (sd->structsize));

	  return compound_expr (init, var);
	}
      else
	gcc_unreachable();
    }

  return convert_expr (expr, etype, totype);
}

// Return a TREE representation of EXPR converted to represent parameter type ARG.

tree
convert_for_argument (tree exp_tree, Expression *expr, Parameter *arg)
{
  if (arg_reference_p (arg))
    {
      // Front-end already sometimes automatically takes the address
      if (expr->op != TOKaddress && expr->op != TOKsymoff && expr->op != TOKadd)
	exp_tree = build_address (exp_tree);

      return convert (type_passed_as (arg), exp_tree);
    }

  // Lazy arguments: expr should already be a delegate
  return exp_tree;
}

// Perform default promotions for data used in expressions.
// Arrays and functions are converted to pointers;
// enumeral types or short or char, to int.
// In addition, manifest constants symbols are replaced by their values.

// Return truth-value conversion of expression EXPR from value type TYPE.

tree
convert_for_condition (tree expr, Type *type)
{
  tree result = NULL_TREE;
  tree obj, func, tmp;

  switch (type->toBasetype()->ty)
    {
    case Taarray:
      // Shouldn't this be...
      //  result = _aaLen (&expr);
      result = component_ref (expr, TYPE_FIELDS (TREE_TYPE (expr)));
      break;

    case Tarray:
      // Checks (length || ptr) (i.e ary !is null)
      tmp = maybe_make_temp (expr);
      obj = delegate_object (tmp);
      func = delegate_method (tmp);
      if (TYPE_MODE (TREE_TYPE (obj)) == TYPE_MODE (TREE_TYPE (func)))
	{
	  result = build2 (BIT_IOR_EXPR, TREE_TYPE (obj), obj,
			   d_convert (TREE_TYPE (obj), func));
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
      // Checks (function || object), but what good is it
      // if there is a null function pointer?
      if (D_METHOD_CALL_EXPR (expr))
	extract_from_method_call (expr, obj, func);
      else
	{
	  tmp = maybe_make_temp (expr);
	  obj = delegate_object (tmp);
	  func = delegate_method (tmp);
	}

      obj = d_truthvalue_conversion (obj);
      func = d_truthvalue_conversion (func);
      // probably not worth using TRUTH_ORIF ...
      result = build2 (BIT_IOR_EXPR, TREE_TYPE (obj), obj, func);
      break;

    default:
      result = expr;
      break;
    }

  return d_truthvalue_conversion (result);
}


// Convert EXP to a dynamic array.
// EXP must be a static array or dynamic array.

tree
d_array_convert (Expression *exp)
{
  TY ty = exp->type->toBasetype()->ty;

  if (ty == Tarray)
    return exp->toElem (current_irstate);
  else if (ty == Tsarray)
    {
      Type *totype = exp->type->toBasetype()->nextOf()->arrayOf();
      return convert_expr (exp->toElem (current_irstate), exp->type, totype);
    }

  // Invalid type passed.
  gcc_unreachable();
}

// Return TRUE if declaration DECL is a reference type.

bool
decl_reference_p (Declaration *decl)
{
  Type *base_type = decl->type->toBasetype();

  if (base_type->ty == Treference)
    return true;

  if (decl->storage_class & (STCout | STCref))
    return true;

  return false;
}

// Returns the real type for declaration DECL.
// Reference decls are converted to reference-to-types.
// Lazy decls are converted into delegates.

tree
declaration_type (Declaration *decl)
{
  tree decl_type = decl->type->toCtype();

  if (decl_reference_p (decl))
    decl_type = build_reference_type (decl_type);
  else if (decl->storage_class & STClazy)
    {
      TypeFunction *tf = new TypeFunction (NULL, decl->type, false, LINKd);
      TypeDelegate *t = new TypeDelegate (tf);
      decl_type = t->merge()->toCtype();
    }
  else if (decl->isThisDeclaration())
    decl_type = insert_type_modifiers (decl_type, MODconst);

  return decl_type;
}

// These should match the Declaration versions above
// Return TRUE if parameter ARG is a reference type.

bool
arg_reference_p (Parameter *arg)
{
  Type *base_type = arg->type->toBasetype();

  if (base_type->ty == Treference)
    return true;

  if (arg->storageClass & (STCout | STCref))
    return true;

  return false;
}

// Returns the real type for parameter ARG.
// Reference parameters are converted to reference-to-types.
// Lazy parameters are converted into delegates.

tree
type_passed_as (Parameter *arg)
{
  tree arg_type = arg->type->toCtype();

  if (arg_reference_p (arg))
    arg_type = build_reference_type (arg_type);
  else if (arg->storageClass & STClazy)
    {
      TypeFunction *tf = new TypeFunction (NULL, arg->type, false, LINKd);
      TypeDelegate *t = new TypeDelegate (tf);
      arg_type = t->merge()->toCtype();
    }

  return arg_type;
}

// Returns an array of type TYPE_NODE which has SIZE number of elements.

tree
d_array_type (Type *d_type, uinteger_t size)
{
  tree index_type_node;
  tree type_node = d_type->toCtype();

  if (size > 0)
    {
      index_type_node = size_int (size - 1);
      index_type_node = build_index_type (index_type_node);
    }
  else
    index_type_node = build_range_type (sizetype, size_zero_node,
					NULL_TREE);

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
insert_type_attribute (tree type, const char *attrname, tree value)
{
  tree attrib;
  tree ident = get_identifier (attrname);

  if (value)
    value = tree_cons (NULL_TREE, value, NULL_TREE);

  // types built by functions in tree.c need to be treated as immutabl
  if (!TYPE_ATTRIBUTES (type))
    type = build_variant_type_copy (type);

  attrib = tree_cons (ident, value, NULL_TREE);
  TYPE_ATTRIBUTES (type) = merge_attributes (TYPE_ATTRIBUTES (type), attrib);

  return type;
}

// Appends the decl attribute ATTRNAME with value VALUE onto decl DECL.

void
insert_decl_attribute (tree decl, const char *attrname, tree value)
{
  tree attrib;
  tree ident = get_identifier (attrname);

  if (value)
    value = tree_cons (NULL_TREE, value, NULL_TREE);

  attrib = tree_cons (ident, value, NULL_TREE);
  DECL_ATTRIBUTES (decl) = merge_attributes (DECL_ATTRIBUTES (decl), attrib);
}

bool
d_attribute_p (const char* name)
{
  static StringTable* table;

  if(table == NULL)
    {
      // Build the table of attributes exposed to the language.
      // Common and format attributes are kept internal.
      size_t n = 0;
      table = new StringTable();

      for (const attribute_spec *p = lang_hooks.attribute_table; p->name; p++)
	n++;

      for (const attribute_spec *p = targetm.attribute_table; p->name; p++)
	n++;

      if(n != 0)
	{
	  table->_init(n);

	  for (const attribute_spec *p = lang_hooks.attribute_table; p->name; p++)
	    table->insert(p->name, strlen(p->name));

	  for (const attribute_spec *p = targetm.attribute_table; p->name; p++)
	    table->insert(p->name, strlen(p->name));
	}
    }

  return table->lookup(name, strlen(name)) != NULL;
}

// Return chain of all GCC attributes found in list IN_ATTRS.

tree
build_attributes (Expressions *in_attrs)
{
  if (!in_attrs)
    return NULL_TREE;

  expandTuples(in_attrs);

  tree out_attrs = NULL_TREE;

  for (size_t i = 0; i < in_attrs->dim; i++)
    {
      Expression *attr = (*in_attrs)[i]->optimize (WANTexpand);
      Dsymbol *sym = attr->type->toDsymbol (0);

      if (!sym)
	continue;

      Dsymbol *mod = (Dsymbol*) sym->getModule();
      if (!(strcmp(mod->toChars(), "attribute") == 0
          && mod->parent != NULL
          && strcmp(mod->parent->toChars(), "gcc") == 0
          && !mod->parent->parent))
        continue;

      if (attr->op == TOKcall)
	attr = attr->ctfeInterpret();

      gcc_assert(attr->op == TOKstructliteral);
      Expressions *elem = ((StructLiteralExp*) attr)->elements;

      if ((*elem)[0]->op == TOKnull)
	{
	  error ("expected string attribute, not null");
	  return error_mark_node;
	}

      gcc_assert((*elem)[0]->op == TOKstring);
      StringExp *nameExp = (StringExp*) (*elem)[0];
      gcc_assert(nameExp->sz == 1);
      const char* name = (const char*) nameExp->string;

      if (!d_attribute_p (name))
      {
        error ("unknown attribute %s", name);
        return error_mark_node;
      }

      tree args = NULL_TREE;

      for (size_t j = 1; j < elem->dim; j++)
        {
	  Expression *ae = (*elem)[j];
	  tree aet;
	  if (ae->op == TOKstring && ((StringExp *) ae)->sz == 1)
	    {
	      StringExp *s = (StringExp *) ae;
	      aet = build_string (s->len, (const char *) s->string);
	    }
	  else
	    aet = ae->toElem (current_irstate);

	  args = chainon (args, build_tree_list (0, aet));
        }

      tree list = build_tree_list (get_identifier (name), args);
      out_attrs =  chainon (out_attrs, list);
    }

  return out_attrs;
}

// Return qualified type variant of TYPE determined by modifier value MOD.

tree
insert_type_modifiers (tree type, unsigned mod)
{
  int quals = 0;
  gcc_assert (type);

  switch (mod)
    {
    case 0:
      break;

    case MODconst:
    case MODwild:
    case MODwildconst:
    case MODimmutable:
      quals |= TYPE_QUAL_CONST;
      break;

    case MODshared:
      quals |= TYPE_QUAL_VOLATILE;
      break;

    case MODshared | MODconst:
    case MODshared | MODwild:
    case MODshared | MODwildconst:
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
build_integer_cst (dinteger_t value, tree type)
{
  // The type is error_mark_node, we can't do anything.
  if (error_operand_p (type))
    return type;

  return build_int_cst_type (type, value);
}

// Build REAL_CST of type TOTYPE with the value VALUE.

tree
build_float_cst (const real_t& value, Type *totype)
{
  real_t new_value;
  TypeBasic *tb = totype->isTypeBasic();

  gcc_assert (tb != NULL);

  tree type_node = tb->toCtype();
  real_convert (&new_value.rv(), TYPE_MODE (type_node), &value.rv());

  // Value grew as a result of the conversion. %% precision bug ??
  // For now just revert back to original.
  if (new_value > value)
    new_value = value;

  return build_real (type_node, new_value.rv());
}

// Returns the .length component from the D dynamic array EXP.

tree
d_array_length (tree exp)
{
  // backend will ICE otherwise
  if (error_operand_p (exp))
    return exp;

  // Get the backend type for the array and pick out the array
  // length field (assumed to be the first field.)
  tree len_field = TYPE_FIELDS (TREE_TYPE (exp));
  return component_ref (exp, len_field);
}

// Returns the .ptr component from the D dynamic array EXP.

tree
d_array_ptr (tree exp)
{
  // backend will ICE otherwise
  if (error_operand_p (exp))
    return exp;

  // Get the backend type for the array and pick out the array
  // data pointer field (assumed to be the second field.)
  tree ptr_field = TREE_CHAIN (TYPE_FIELDS (TREE_TYPE (exp)));
  return component_ref (exp, ptr_field);
}

// Returns a constructor for D dynamic array type TYPE of .length LEN
// and .ptr pointing to DATA.

tree
d_array_value (tree type, tree len, tree data)
{
  // %% assert type is a darray
  tree len_field, ptr_field;
  vec<constructor_elt, va_gc> *ce = NULL;

  len_field = TYPE_FIELDS (type);
  ptr_field = TREE_CHAIN (len_field);

  len = convert (TREE_TYPE (len_field), len);
  data = convert (TREE_TYPE (ptr_field), data);

  CONSTRUCTOR_APPEND_ELT (ce, len_field, len);
  CONSTRUCTOR_APPEND_ELT (ce, ptr_field, data);

  tree ctor = build_constructor (type, ce);
  // TREE_STATIC and TREE_CONSTANT can be set by caller if needed
  TREE_STATIC (ctor) = 0;
  TREE_CONSTANT (ctor) = 0;

  return ctor;
}

// Builds a D string value from the C string STR.

tree
d_array_string (const char *str)
{
  unsigned len = strlen (str);
  // Assumes STR is 0-terminated.
  tree str_tree = build_string (len + 1, str);

  TREE_TYPE (str_tree) = d_array_type (Type::tchar, len);

  return d_array_value (Type::tchar->arrayOf()->toCtype(),
			size_int (len), build_address (str_tree));
}

// Returns value representing the array length of expression EXP.
// TYPE could be a dynamic or static array.

tree
get_array_length (tree exp, Type *type)
{
  Type *tb = type->toBasetype();

  switch (tb->ty)
    {
    case Tsarray:
      return size_int (((TypeSArray *) tb)->dim->toUInteger());

    case Tarray:
      return d_array_length (exp);

    default:
      error ("can't determine the length of a %s", type->toChars());
      return error_mark_node;
    }
}

// Create BINFO for a ClassDeclaration's inheritance tree.
// Interfaces are not included.

tree
build_class_binfo (tree super, ClassDeclaration *cd)
{
  tree binfo = make_tree_binfo (1);
  tree ctype = cd->type->toCtype();

  // Want RECORD_TYPE, not REFERENCE_TYPE
  BINFO_TYPE (binfo) = TREE_TYPE (ctype);
  BINFO_INHERITANCE_CHAIN (binfo) = super;
  BINFO_OFFSET (binfo) = integer_zero_node;

  if (cd->baseClass)
    BINFO_BASE_APPEND (binfo, build_class_binfo (binfo, cd->baseClass));

  return binfo;
}

// Create BINFO for an InterfaceDeclaration's inheritance tree.
// In order to access all inherited methods in the debugger,
// the entire tree must be described.
// This function makes assumptions about interface layout.

tree
build_interface_binfo (tree super, ClassDeclaration *cd, unsigned& offset)
{
  tree binfo = make_tree_binfo (cd->baseclasses->dim);
  tree ctype = cd->type->toCtype();

  // Want RECORD_TYPE, not REFERENCE_TYPE
  BINFO_TYPE (binfo) = TREE_TYPE (ctype);
  BINFO_INHERITANCE_CHAIN (binfo) = super;
  BINFO_OFFSET (binfo) = size_int (offset * Target::ptrsize);
  BINFO_VIRTUAL_P (binfo) = 1;

  for (size_t i = 0; i < cd->baseclasses->dim; i++, offset++)
    {
      BaseClass *bc = (*cd->baseclasses)[i];
      BINFO_BASE_APPEND (binfo, build_interface_binfo (binfo, bc->base, offset));
    }

  return binfo;
}

// Returns the .funcptr component from the D delegate EXP.

tree
delegate_method (tree exp)
{
  // Get the backend type for the array and pick out the array length
  // field (assumed to be the second field.)
  tree method_field = TREE_CHAIN (TYPE_FIELDS (TREE_TYPE (exp)));
  return component_ref (exp, method_field);
}

// Returns the .object component from the delegate EXP.

tree
delegate_object (tree exp)
{
  // Get the backend type for the array and pick out the array data
  // pointer field (assumed to be the first field.)
  tree obj_field = TYPE_FIELDS (TREE_TYPE (exp));
  return component_ref (exp, obj_field);
}

// Build a delegate literal of type TYPE whose pointer function is
// METHOD, and hidden object is OBJECT.

tree
build_delegate_cst (tree method, tree object, Type *type)
{
  Type *base_type = type->toBasetype();

  // Called from DotVarExp.  These are just used to make function calls
  // and not to make Tdelegate variables.  Clearing the type makes sure of this.
  if (base_type->ty == Tfunction)
    base_type = NULL;
  else
    gcc_assert (base_type->ty == Tdelegate);

  tree ctype = base_type ? base_type->toCtype() : NULL_TREE;
  tree ctor = make_node (CONSTRUCTOR);
  tree obj_field = NULL_TREE;
  tree func_field = NULL_TREE;
  vec<constructor_elt, va_gc> *ce = NULL;

  if (ctype)
    {
      TREE_TYPE (ctor) = ctype;
      obj_field = TYPE_FIELDS (ctype);
      func_field = TREE_CHAIN (obj_field);
    }
  CONSTRUCTOR_APPEND_ELT (ce, obj_field, object);
  CONSTRUCTOR_APPEND_ELT (ce, func_field, method);

  CONSTRUCTOR_ELTS (ctor) = ce;
  return ctor;
}

// Builds a temporary tree to store the CALLEE and OBJECT
// of a method call expression of type TYPE.

tree
build_method_call (tree callee, tree object, Type *type)
{
  tree t = build_delegate_cst (callee, object, type);
  D_METHOD_CALL_EXPR (t) = 1;
  return t;
}

// Extract callee and object from T and return in to CALLEE and OBJECT.

void
extract_from_method_call (tree t, tree& callee, tree& object)
{
  gcc_assert (D_METHOD_CALL_EXPR (t));
  object = CONSTRUCTOR_ELT (t, 0)->value;
  callee = CONSTRUCTOR_ELT (t, 1)->value;
}

// Return correct callee for method FUNC, which is dereferenced from
// the 'this' pointer OBJEXP.  TYPE is the return type for the method.
// THISEXP is the tree representation of OBJEXP.

tree
get_object_method (tree thisexp, Expression *objexp, FuncDeclaration *func, Type *type)
{
  Type *objtype = objexp->type->toBasetype();
  bool is_dottype = false;

  gcc_assert (func->isThis());

  Expression *ex = objexp;

  while (1)
    {
      if (ex->op == TOKsuper || ex->op == TOKdottype)
	{
	  // super.member() and type.member() calls directly.
	  is_dottype = true;
	  break;
	}
      else if (ex->op == TOKcast)
	{
	  ex = ((CastExp *) ex)->e1;
	  continue;
	}
      break;
    }

  // Calls to super are static (func is the super's method)
  // Structs don't have vtables.
  // Final and non-virtual methods can be called directly.
  // DotTypeExp means non-virtual

  if (objexp->op == TOKsuper
      || objtype->ty == Tstruct || objtype->ty == Tpointer
      || func->isFinalFunc() || !func->isVirtual() || is_dottype)
    {
      if (objtype->ty == Tstruct)
	thisexp = build_address (thisexp);

      return build_method_call (build_address (func->toSymbol()->Stree),
				thisexp, type);
    }
  else
    {
      // Interface methods are also in the class's vtable, so we don't
      // need to convert from a class pointer to an interface pointer.
      thisexp = maybe_make_temp (thisexp);
      tree vtbl_ref = build_deref (thisexp);
      // The vtable is the first field.
      tree field = TYPE_FIELDS (TREE_TYPE (vtbl_ref));
      tree fntype = TREE_TYPE (func->toSymbol()->Stree);

      vtbl_ref = component_ref (vtbl_ref, field);
      vtbl_ref = build_offset (vtbl_ref, size_int (Target::ptrsize * func->vtblIndex));
      vtbl_ref = indirect_ref (build_pointer_type (fntype), vtbl_ref);

      return build_method_call (vtbl_ref, thisexp, type);
    }
}


// Builds a record type from field types T1 and T2.  TYPE is the D frontend
// type we are building. N1 and N2 are the names of the two fields.

tree
build_two_field_type(tree t1, tree t2, Type *type, const char *n1, const char *n2)
{
  tree rectype = make_node(RECORD_TYPE);
  tree f0 = build_decl(BUILTINS_LOCATION, FIELD_DECL, get_identifier(n1), t1);
  tree f1 = build_decl(BUILTINS_LOCATION, FIELD_DECL, get_identifier(n2), t2);

  DECL_CONTEXT(f0) = rectype;
  DECL_CONTEXT(f1) = rectype;
  TYPE_FIELDS(rectype) = chainon(f0, f1);
  if (type != NULL)
    TYPE_NAME(rectype) = get_identifier(type->toChars());
  layout_type(rectype);

  return rectype;
}

// Create a SAVE_EXPR if EXP might have unwanted side effects if referenced
// more than once in an expression.

tree
make_temp (tree exp)
{
  if (TREE_CODE (exp) == CALL_EXPR
      || TREE_CODE (TREE_TYPE (exp)) != ARRAY_TYPE)
    return save_expr (exp);
  else
    return stabilize_reference (exp);
}

tree
maybe_make_temp (tree exp)
{
  if (d_has_side_effects (exp))
    return make_temp (exp);

  return exp;
}

// Return TRUE if EXP can not be evaluated multiple times (i.e., in a loop body)
// without unwanted side effects.

bool
d_has_side_effects (tree exp)
{
  tree t = STRIP_NOPS (exp);

  // SAVE_EXPR is safe to reference more than once, but not to
  // expand in a loop.
  if (TREE_CODE (t) == SAVE_EXPR)
    return false;

  if (DECL_P (t)
      || CONSTANT_CLASS_P (t))
    return false;

  if (INDIRECT_REF_P (t)
      || TREE_CODE (t) == ADDR_EXPR
      || TREE_CODE (t) == COMPONENT_REF)
    return d_has_side_effects (TREE_OPERAND (t, 0));

  return TREE_SIDE_EFFECTS (t);
}


// Returns the address of the expression EXP.

tree
build_address (tree exp)
{
  tree t, ptrtype;
  tree type = TREE_TYPE (exp);
  d_mark_addressable (exp);

  // Gimplify doesn't like &(* (ptr-to-array-type)) with static arrays
  if (TREE_CODE (exp) == INDIRECT_REF)
    {
      t = TREE_OPERAND (exp, 0);
      ptrtype = build_pointer_type (type);
      t = build_nop (ptrtype, t);
    }
  else
    {
      /* Just convert string literals (char[]) to C-style strings (char *), otherwise
	 the latter method (char[]*) causes conversion problems during gimplification. */
      if (TREE_CODE (exp) == STRING_CST)
	ptrtype = build_pointer_type (TREE_TYPE (type));
      /* Special case for va_list. The backends will be expecting a pointer to vatype,
       * but some targets use an array. So fix it.  */
      else if (TYPE_MAIN_VARIANT (type) == TYPE_MAIN_VARIANT (va_list_type_node))
	{
	  if (TREE_CODE (TYPE_MAIN_VARIANT (type)) == ARRAY_TYPE)
	    ptrtype = build_pointer_type (TREE_TYPE (type));
	  else
	    ptrtype = build_pointer_type (type);
	}
      else
	ptrtype = build_pointer_type (type);

      t = build1 (ADDR_EXPR, ptrtype, exp);
    }

  if (TREE_CODE (exp) == FUNCTION_DECL)
    TREE_NO_TRAMPOLINE (t) = 1;

  return t;
}

tree
d_mark_addressable (tree exp)
{
  switch (TREE_CODE (exp))
    {
    case ADDR_EXPR:
    case COMPONENT_REF:
      /* If D had bit fields, we would need to handle that here */
    case ARRAY_REF:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
      d_mark_addressable (TREE_OPERAND (exp, 0));
      break;

      /* %% C++ prevents {& this} .... */
    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case COMPOUND_EXPR:
      d_mark_addressable (TREE_OPERAND (exp, 1));
      break;

    case COND_EXPR:
      d_mark_addressable (TREE_OPERAND (exp, 1));
      d_mark_addressable (TREE_OPERAND (exp, 2));
      break;

    case CONSTRUCTOR:
      TREE_ADDRESSABLE (exp) = 1;
      break;

    case INDIRECT_REF:
      /* %% this was in Java, not sure for D */
      /* We sometimes add a cast *(TYPE *)&FOO to handle type and mode
	 incompatibility problems.  Handle this case by marking FOO.  */
      if (TREE_CODE (TREE_OPERAND (exp, 0)) == NOP_EXPR
	  && TREE_CODE (TREE_OPERAND (TREE_OPERAND (exp, 0), 0)) == ADDR_EXPR)
	{
	  d_mark_addressable (TREE_OPERAND (TREE_OPERAND (exp, 0), 0));
	  break;
	}
      if (TREE_CODE (TREE_OPERAND (exp, 0)) == ADDR_EXPR)
	{
	  d_mark_addressable (TREE_OPERAND (exp, 0));
	  break;
	}
      break;

    case VAR_DECL:
    case CONST_DECL:
    case PARM_DECL:
    case RESULT_DECL:
    case FUNCTION_DECL:
      TREE_USED (exp) = 1;
      TREE_ADDRESSABLE (exp) = 1;

      /* drops through */
    default:
      break;
    }

  return exp;
}

/* Mark EXP as "used" in the program for the benefit of
   -Wunused warning purposes.  */

tree
d_mark_used (tree exp)
{
  switch (TREE_CODE (exp))
    {
    case VAR_DECL:
    case PARM_DECL:
      TREE_USED (exp) = 1;
      break;

    case ARRAY_REF:
    case COMPONENT_REF:
    case MODIFY_EXPR:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case NOP_EXPR:
    case CONVERT_EXPR:
    case ADDR_EXPR:
      d_mark_used (TREE_OPERAND (exp, 0));
      break;

    case COMPOUND_EXPR:
      d_mark_used (TREE_OPERAND (exp, 0));
      d_mark_used (TREE_OPERAND (exp, 1));
      break;

    default:
      break;
    }
  return exp;
}

/* Mark EXP as read, not just set, for set but not used -Wunused
   warning purposes.  */

tree
d_mark_read (tree exp)
{
  switch (TREE_CODE (exp))
    {
    case VAR_DECL:
    case PARM_DECL:
      TREE_USED (exp) = 1;
      DECL_READ_P (exp) = 1;
      break;

    case ARRAY_REF:
    case COMPONENT_REF:
    case MODIFY_EXPR:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case NOP_EXPR:
    case CONVERT_EXPR:
    case ADDR_EXPR:
      d_mark_read (TREE_OPERAND (exp, 0));
      break;

    case COMPOUND_EXPR:
      d_mark_read (TREE_OPERAND (exp, 1));
      break;

    default:
      break;
    }
  return exp;
}

// Build equality expression between two RECORD_TYPES T1 and T2.
// CODE is the EQ_EXPR or NE_EXPR comparison.
// SD is the front-end struct type.

tree
build_struct_memcmp (tree_code code, StructDeclaration *sd, tree t1, tree t2)
{
  tree_code tcode = (code == EQ_EXPR) ? TRUTH_ANDIF_EXPR : TRUTH_ORIF_EXPR;
  tree tmemcmp = NULL_TREE;

  // Let backend take care of empty struct or union comparisons.
  if (!sd->fields.dim || sd->isUnionDeclaration())
    {
      tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
				   build_address (t1), build_address (t2),
				   size_int (sd->structsize));

      return build_boolop (code, tmemcmp, integer_zero_node);
    }

  for (size_t i = 0; i < sd->fields.dim; i++)
    {
      VarDeclaration *vd = sd->fields[i];
      tree sfield = vd->toSymbol()->Stree;

      tree t1ref = component_ref (t1, sfield);
      tree t2ref = component_ref (t2, sfield);
      tree tcmp;

      if (vd->type->ty == Tstruct)
	{
	  // Compare inner data structures.
	  StructDeclaration *decl = ((TypeStruct *) vd->type)->sym;
	  tcmp = build_struct_memcmp (code, decl, t1ref, t2ref);
	}
      else
	{
	  tree stype = vd->type->toCtype();
	  machine_mode mode = int_mode_for_mode (TYPE_MODE (stype));

	  if (vd->type->isintegral())
	    {
	      // Integer comparison, no special handling required.
	      tcmp = build_boolop (code, t1ref, t2ref);
	    }
	  else if (mode != BLKmode)
	    {
	      // Compare field bits as their corresponding integer type.
	      //   *((T*) &t1) == *((T*) &t2)
	      tree tmode = lang_hooks.types.type_for_mode (mode, 1);

	      t1ref = build_vconvert (tmode, t1ref);
	      t2ref = build_vconvert (tmode, t2ref);

	      tcmp = build_boolop (code, t1ref, t2ref);
	    }
	  else
	    {
	      // Simple memcmp between types.
	      tcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					build_address (t1ref), build_address (t2ref),
					TYPE_SIZE_UNIT (stype));

	      tcmp = build_boolop (code, tcmp, integer_zero_node);
	    }
	}

      tmemcmp = (tmemcmp) ? build_boolop (tcode, tmemcmp, tcmp) : tcmp;
    }

  return tmemcmp;
}

// Cast EXP (which should be a pointer) to TYPE * and then indirect.  The
// back-end requires this cast in many cases.

tree
indirect_ref (tree type, tree exp)
{
  if (TREE_CODE (TREE_TYPE (exp)) == REFERENCE_TYPE)
    return build1 (INDIRECT_REF, type, exp);

  return build1 (INDIRECT_REF, type,
		 build_nop (build_pointer_type (type), exp));
}

// Returns indirect reference of EXP, which must be a pointer type.

tree
build_deref (tree exp)
{
  tree type = TREE_TYPE (exp);
  gcc_assert (POINTER_TYPE_P (type));

  if (TREE_CODE (exp) == ADDR_EXPR)
    return TREE_OPERAND (exp, 0);

  return build1 (INDIRECT_REF, TREE_TYPE (type), exp);
}

// Builds pointer offset expression PTR[INDEX]

tree
build_array_index (tree ptr, tree index)
{
  tree result_type_node = TREE_TYPE (ptr);
  tree elem_type_node = TREE_TYPE (result_type_node);
  tree size_exp;

  tree prod_result_type;
  prod_result_type = sizetype;

  // array element size
  size_exp = size_in_bytes (elem_type_node);

  if (integer_zerop (size_exp))
    {
      // Test for void case...
      if (TYPE_MODE (elem_type_node) == TYPE_MODE (void_type_node))
	index = fold_convert (prod_result_type, index);
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
      index = fold_convert (prod_result_type, index);
    }
  else
    {
      if (TYPE_PRECISION (TREE_TYPE (index)) != TYPE_PRECISION (sizetype)
	  || TYPE_UNSIGNED (TREE_TYPE (index)) != TYPE_UNSIGNED (sizetype))
	{
	  tree type = lang_hooks.types.type_for_size (TYPE_PRECISION (sizetype),
						      TYPE_UNSIGNED (sizetype));
	  index = d_convert (type, index);
	}
      index = fold_convert (prod_result_type,
			    fold_build2 (MULT_EXPR, TREE_TYPE (size_exp),
					 index, d_convert (TREE_TYPE (index), size_exp)));
    }

  // backend will ICE otherwise
  if (error_operand_p (result_type_node))
    return result_type_node;

  if (integer_zerop (index))
    return ptr;

  return build2 (POINTER_PLUS_EXPR, result_type_node, ptr, index);
}

// Builds pointer offset expression *(PTR OP IDX)
// OP could be a plus or minus expression.

tree
build_offset_op (tree_code op, tree ptr, tree idx)
{
  gcc_assert (op == MINUS_EXPR || op == PLUS_EXPR);

  if (op == MINUS_EXPR)
    idx = fold_build1 (NEGATE_EXPR, sizetype, idx);

  return build2 (POINTER_PLUS_EXPR, TREE_TYPE (ptr), ptr,
		 fold_convert (sizetype, idx));
}

tree
build_offset (tree ptr_node, tree byte_offset)
{
  tree ofs = fold_convert (Type::tsize_t->toCtype(), byte_offset);
  return fold_build2 (POINTER_PLUS_EXPR, TREE_TYPE (ptr_node), ptr_node, ofs);
}


// Implicitly converts void* T to byte* as D allows { void[] a; &a[3]; }

tree
void_okay_p (tree t)
{
  tree type = TREE_TYPE (t);
  tree totype = Type::tuns8->pointerTo()->toCtype();

  if (VOID_TYPE_P (TREE_TYPE (type)))
    return convert (totype, t);

  return t;
}

// Build an expression of code CODE, data type TYPE, and operands ARG0
// and ARG1. Perform relevant conversions needs for correct code operations.

tree
build_binary_op (tree_code code, tree type, tree arg0, tree arg1)
{
  tree t0 = TREE_TYPE (arg0);
  tree t1 = TREE_TYPE (arg1);

  bool unsignedp = TYPE_UNSIGNED (t0) || TYPE_UNSIGNED (t1);

  tree t = NULL_TREE;

  // Deal with float mod expressions immediately.
  if (code == FLOAT_MOD_EXPR)
    return build_float_modulus (TREE_TYPE (arg0), arg0, arg1);

  if (POINTER_TYPE_P (t0) && INTEGRAL_TYPE_P (t1))
    return build_nop (type, build_offset_op (code, arg0, arg1));

  if (INTEGRAL_TYPE_P (t0) && POINTER_TYPE_P (t1))
    return build_nop (type, build_offset_op (code, arg1, arg0));

  if (POINTER_TYPE_P (t0) && POINTER_TYPE_P (t1))
    {
      // Need to convert pointers to integers because tree-vrp asserts
      // against (ptr MINUS ptr).
      tree ptrtype = lang_hooks.types.type_for_mode (ptr_mode, TYPE_UNSIGNED (type));
      arg0 = d_convert (ptrtype, arg0);
      arg1 = d_convert (ptrtype, arg1);

      t = build2 (code, ptrtype, arg0, arg1);
    }
  else if (INTEGRAL_TYPE_P (type) && (TYPE_UNSIGNED (type) != unsignedp))
    {
      tree inttype = unsignedp ? d_unsigned_type (type) : d_signed_type (type);
      t = build2 (code, inttype, arg0, arg1);
    }
  else
    {
      // Front-end does not do this conversion and GCC does not
      // always do it right.
      if (COMPLEX_FLOAT_TYPE_P (t0) && !COMPLEX_FLOAT_TYPE_P (t1))
	arg1 = d_convert (t0, arg1);
      else if (COMPLEX_FLOAT_TYPE_P (t1) && !COMPLEX_FLOAT_TYPE_P (t0))
	arg0 = d_convert (t1, arg0);

      t = build2 (code, type, arg0, arg1);
    }

  return d_convert (type, t);
}

// Build a binary expression of code CODE, assigning the result into E1.

tree
build_binop_assignment(tree_code code, Expression *e1, Expression *e2)
{
  // Skip casts for lhs assignment.
  Expression *e1b = e1;
  while (e1b->op == TOKcast)
    {
      CastExp *ce = (CastExp *) e1b;
      gcc_assert(d_types_same(ce->type, ce->to));
      e1b = ce->e1;
    }

  // Prevent multiple evaluations of LHS, but watch out!
  // The LHS expression could be an assignment, to which
  // it's operation gets lost during gimplification.
  tree lexpr = NULL_TREE;
  tree lhs;

  if (e1b->op == TOKcomma)
    {
      CommaExp *ce = (CommaExp *) e1b;
      lexpr = ce->e1->toElem(current_irstate);
      lhs = ce->e2->toElem(current_irstate);
    }
  else
    lhs = e1b->toElem(current_irstate);

  tree rhs = e2->toElem(current_irstate);

  // Build assignment expression. Stabilize lhs for assignment.
  lhs = stabilize_reference(lhs);

  rhs = build_binary_op(code, e1->type->toCtype(),
			convert_expr(lhs, e1b->type, e1->type), rhs);

  tree expr = modify_expr(lhs, convert_expr(rhs, e1->type, e1b->type));

  if (lexpr)
    expr = compound_expr(lexpr, expr);

  return expr;
}

// Builds an array bounds checking condition, returning INDEX if true,
// else throws a RangeError exception.

tree
d_checked_index (Loc loc, tree index, tree upr, bool inclusive)
{
  if (!array_bounds_check())
    return index;

  return build3 (COND_EXPR, TREE_TYPE (index),
		 d_bounds_condition (index, upr, inclusive),
		 index, d_assert_call (loc, LIBCALL_ARRAY_BOUNDS));
}

// Builds the condition [INDEX < UPR] and optionally [INDEX >= 0]
// if INDEX is a signed type.  For use in array bound checking routines.
// If INCLUSIVE, we allow equality to return true also.
// INDEX must be wrapped in a SAVE_EXPR to prevent multiple evaluation.

tree
d_bounds_condition (tree index, tree upr, bool inclusive)
{
  tree uindex = d_convert (d_unsigned_type (TREE_TYPE (index)), index);

  // Build condition to test that INDEX < UPR.
  tree condition = build2 (inclusive ? LE_EXPR : LT_EXPR, bool_type_node, uindex, upr);

  // Build condition to test that INDEX >= 0.
  if (!TYPE_UNSIGNED (TREE_TYPE (index)))
    condition = build2 (TRUTH_ANDIF_EXPR, bool_type_node, condition,
			build2 (GE_EXPR, bool_type_node, index, integer_zero_node));

  return condition;
}

// Returns TRUE if array bounds checking code generation is turned on.

bool
array_bounds_check()
{
  int result = global.params.useArrayBounds;

  if (result == 2)
    return true;

  if (result == 1)
    {
      // For D2 safe functions only
      FuncDeclaration *func = current_irstate->func;
      if (func && func->type->ty == Tfunction)
	{
	  TypeFunction *tf = (TypeFunction *) func->type;
	  if (tf->trust == TRUSTsafe)
	    return true;
	}
    }

  return false;
}

// Builds a BIND_EXPR around BODY for the variables VAR_CHAIN.

tree
bind_expr (tree var_chain, tree body)
{
  // TODO: only handles one var
  gcc_assert (TREE_CHAIN (var_chain) == NULL_TREE);

  if (DECL_INITIAL (var_chain))
    {
      tree ini = build_vinit (var_chain, DECL_INITIAL (var_chain));
      DECL_INITIAL (var_chain) = NULL_TREE;
      body = compound_expr (ini, body);
    }

  return make_temp (build3 (BIND_EXPR, TREE_TYPE (body), var_chain, body, NULL_TREE));
}

// Like compound_expr, but ARG0 or ARG1 might be NULL_TREE.

tree
maybe_compound_expr (tree arg0, tree arg1)
{
  if (arg0 == NULL_TREE)
    return arg1;
  else if (arg1 == NULL_TREE)
    return arg0;
  else
    return compound_expr (arg0, arg1);
}

// Like vcompound_expr, but ARG0 or ARG1 might be NULL_TREE.

tree
maybe_vcompound_expr (tree arg0, tree arg1)
{
  if (arg0 == NULL_TREE)
    return arg1;
  else if (arg1 == NULL_TREE)
    return arg0;
  else
    return vcompound_expr (arg0, arg1);
}

// Returns the TypeFunction class for Type T.
// Assumes T is already ->toBasetype()

TypeFunction *
get_function_type (Type *t)
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

// Returns TRUE if CALLEE is a plain nested function outside the scope of CALLER.
// In which case, CALLEE is being called through an alias that was passed to CALLER.

bool
call_by_alias_p (FuncDeclaration *caller, FuncDeclaration *callee)
{
  if (!callee->isNested())
    return false;

  Dsymbol *dsym = callee;

  while (dsym)
    {
      if (dsym->isTemplateInstance())
	return false;
      else if (dsym->isFuncDeclaration() == caller)
	return false;
      dsym = dsym->toParent();
    }

  return true;
}

// Entry point for call routines.  Builds a function call to FD.
// OBJECT is the 'this' reference passed and ARGS are the arguments to FD.

tree
d_build_call (FuncDeclaration *fd, tree object, Expressions *args)
{
  return d_build_call (get_function_type (fd->type),
		       build_address (fd->toSymbol()->Stree), object, args);
}

// Builds a CALL_EXPR of type TF to CALLABLE. OBJECT holds the 'this' pointer,
// ARGUMENTS are evaluated in left to right order, saved and promoted before passing.

tree
d_build_call (TypeFunction *tf, tree callable, tree object, Expressions *arguments)
{
  IRState *irs = current_irstate;
  tree ctype = TREE_TYPE (callable);
  tree callee = callable;
  tree saved_args = NULL_TREE;

  tree arg_list = NULL_TREE;

  if (POINTER_TYPE_P (ctype))
    ctype = TREE_TYPE (ctype);
  else
    callee = build_address (callable);

  gcc_assert (function_type_p (ctype));
  gcc_assert (tf != NULL);
  gcc_assert (tf->ty == Tfunction);

  // Evaluate the callee before calling it.
  if (TREE_SIDE_EFFECTS (callee))
    {
      callee = maybe_make_temp (callee);
      saved_args = callee;
    }

  if (TREE_CODE (ctype) == FUNCTION_TYPE)
    {
      if (object != NULL_TREE)
	gcc_unreachable();
    }
  else if (object == NULL_TREE)
    {
      // Front-end apparently doesn't check this.
      if (TREE_CODE (callable) == FUNCTION_DECL)
	{
	  error ("need 'this' to access member %s", IDENTIFIER_POINTER (DECL_NAME (callable)));
	  return error_mark_node;
	}

      // Probably an internal error
      gcc_unreachable();
    }

  // If this is a delegate call or a nested function being called as
  // a delegate, the object should not be NULL.
  if (object != NULL_TREE)
    {
      if (TREE_SIDE_EFFECTS (object))
	{
	  object = maybe_make_temp (object);
	  saved_args = maybe_vcompound_expr (saved_args, object);
	}
      arg_list = build_tree_list (NULL_TREE, object);
    }

  if (arguments)
    {
      // First pass, evaluated expanded tuples in function arguments.
      for (size_t i = 0; i < arguments->dim; ++i)
	{
	Lagain:
	  Expression *arg = (*arguments)[i];
	  gcc_assert (arg->op != TOKtuple);

	  if (arg->op == TOKcomma)
	    {
	      CommaExp *ce = (CommaExp *) arg;
	      tree tce = ce->e1->toElem (irs);
	      saved_args = maybe_vcompound_expr (saved_args, tce);
	      (*arguments)[i] = ce->e2;
	      goto Lagain;
	    }
	}

      // if _arguments[] is the first argument.
      size_t dvarargs = (tf->linkage == LINKd && tf->varargs == 1);
      size_t nparams = Parameter::dim (tf->parameters);

      // Assumes arguments->dim <= formal_args->dim if (!this->varargs)
      for (size_t i = 0; i < arguments->dim; ++i)
	{
	  Expression *arg = (*arguments)[i];
	  tree targ;

	  if (i < dvarargs)
	    {
	      // The hidden _arguments parameter
	      targ = arg->toElem (irs);
	    }
	  else if (i - dvarargs < nparams && i >= dvarargs)
	    {
	      // Actual arguments for declared formal arguments
	      Parameter *parg = Parameter::getNth (tf->parameters, i - dvarargs);
	      targ = convert_for_argument (arg->toElem (irs), arg, parg);
	    }
	  else
	    {
	      // Not all targets support passing unpromoted types, so
	      // promote anyway.
	      targ = arg->toElem (irs);
	      tree ptype = lang_hooks.types.type_promotes_to (TREE_TYPE (targ));

	      if (ptype != TREE_TYPE (targ))
		targ = convert (ptype, targ);
	    }

	  // Evaluate the argument before passing to the function.
	  // Needed for left to right evaluation.
	  if (tf->linkage == LINKd && TREE_SIDE_EFFECTS (targ))
	    {
	      targ = maybe_make_temp (targ);
	      saved_args = maybe_vcompound_expr (saved_args, targ);
	    }
	  arg_list = chainon (arg_list, build_tree_list (0, targ));
	}
    }

  tree result = d_build_call_list (TREE_TYPE (ctype), callee, arg_list);
  result = expand_intrinsic (result);

  return maybe_compound_expr (saved_args, result);
}

// Builds a call to AssertError or AssertErrorMsg.

tree
d_assert_call (Loc loc, LibCall libcall, tree msg)
{
  tree args[3];
  int nargs;

  if (msg != NULL)
    {
      args[0] = msg;
      args[1] = d_array_string (loc.filename ? loc.filename : "");
      args[2] = build_integer_cst (loc.linnum, Type::tuns32->toCtype());
      nargs = 3;
    }
  else
    {
      args[0] = d_array_string (loc.filename ? loc.filename : "");
      args[1] = build_integer_cst (loc.linnum, Type::tuns32->toCtype());
      args[2] = NULL_TREE;
      nargs = 2;
    }

  return build_libcall (libcall, nargs, args);
}


// Our internal list of library functions.

static FuncDeclaration *libcall_decls[LIBCALL_count];

// Build and return a function symbol to be used by libcall_decls.

static FuncDeclaration *
get_libcall(const char *name, Type *type, int flags, int nparams, ...)
{
  // Add parameter types.
  Parameters *args = new Parameters;
  args->setDim(nparams);

  va_list ap;
  va_start (ap, nparams);
  for (int i = 0; i < nparams; i++)
    (*args)[i] = new Parameter(0, va_arg(ap, Type *), NULL, NULL);
  va_end(ap);

  // Build extern(C) function.
  FuncDeclaration *decl = FuncDeclaration::genCfunc(args, type, name);

  // Apply flags to the decl.
  tree t = decl->toSymbol()->Stree;
  DECL_ARTIFICIAL(t) = 1;

  // Whether the function accepts a variable list of arguments.
  TypeFunction *tf = (TypeFunction *) decl->type;
  tf->varargs = (flags & LCFvarargs);
  // Whether the function does not return except through catching a thrown exception.
  TREE_THIS_VOLATILE(t) = (flags & LCFthrows);
  // Whether the function performs a malloc-like operation.
  DECL_IS_MALLOC(t) = (flags & LCFmalloc);

  return decl;
}

// Library functions are generated as needed.
// This could probably be changed in the future to be more like GCC builtin
// trees, but we depend on runtime initialisation of front-end types.

FuncDeclaration *
get_libcall(LibCall libcall)
{
  // Build generic AA type void*[void*] for runtime.def
  static Type *AA = NULL;
  if (AA == NULL)
    AA = new TypeAArray(Type::tvoidptr, Type::tvoidptr);

  switch (libcall)
    {
#define DEF_D_RUNTIME(CODE, NAME, PARAMS, TYPE, FLAGS) \
    case LIBCALL_ ## CODE:	\
      libcall_decls[libcall] = get_libcall(NAME, TYPE, FLAGS, PARAMS); \
      break;
#include "runtime.def"
#undef DEF_D_RUNTIME

    default:
      gcc_unreachable();
    }

  return libcall_decls[libcall];
}

// Build call to LIBCALL. N_ARGS is the number of call arguments which are
// specified in as a tree array ARGS.  The caller can force the return type
// of the call to FORCE_TYPE if the library call returns a generic value.

// This does not perform conversions on the arguments.  This allows arbitrary data
// to be passed through varargs without going through the usual conversions.

tree
build_libcall (LibCall libcall, unsigned n_args, tree *args, tree force_type)
{
  // Build the call expression to the runtime function.
  FuncDeclaration *decl = get_libcall(libcall);
  Type *type = decl->type->nextOf();
  tree callee = build_address (decl->toSymbol()->Stree);
  tree arg_list = NULL_TREE;

  for (int i = n_args - 1; i >= 0; i--)
    arg_list = tree_cons (NULL_TREE, args[i], arg_list);

  tree result = d_build_call_list (type->toCtype(), callee, arg_list);

  // Assumes caller knows what it is doing.
  if (force_type != NULL_TREE)
    return convert (force_type, result);

  return result;
}

// Build a call to CALLEE, passing ARGS as arguments.  The expected return
// type is TYPE.  TREE_SIDE_EFFECTS gets set depending on the const/pure
// attributes of the funcion and the SIDE_EFFECTS flags of the arguments.

tree
d_build_call_list (tree type, tree callee, tree args)
{
  int nargs = list_length (args);
  tree *pargs = new tree[nargs];
  for (size_t i = 0; args; args = TREE_CHAIN (args), i++)
    pargs[i] = TREE_VALUE (args);

  return build_call_array (type, callee, nargs, pargs);
}

// Conveniently construct the function arguments for passing
// to the d_build_call_list function.

tree
d_build_call_nary (tree callee, int n_args, ...)
{
  va_list ap;
  tree arg_list = NULL_TREE;
  tree fntype = TREE_TYPE (callee);

  va_start (ap, n_args);
  for (int i = n_args - 1; i >= 0; i--)
    arg_list = tree_cons (NULL_TREE, va_arg (ap, tree), arg_list);
  va_end (ap);

  return d_build_call_list (TREE_TYPE (fntype), build_address (callee), nreverse (arg_list));
}

// List of codes for internally recognised compiler intrinsics.

enum intrinsic_code
{
#define DEF_D_INTRINSIC(CODE, A, N, M, D) INTRINSIC_ ## CODE,
#include "intrinsics.def"
#undef DEF_D_INTRINSIC
  INTRINSIC_LAST
};

// An internal struct used to hold information on D intrinsics.

struct intrinsic_decl
{
  // The DECL_FUNCTION_CODE of this decl.
  intrinsic_code code;

  // The name of the intrinsic.
  const char *name;

  // The module where the intrinsic is located.
  const char *module;

  // The mangled signature decoration of the intrinsic.
  const char *deco;
};

static const intrinsic_decl intrinsic_decls[] =
{
#define DEF_D_INTRINSIC(CODE, ALIAS, NAME, MODULE, DECO) \
    { INTRINSIC_ ## ALIAS, NAME, MODULE, DECO },
#include "intrinsics.def"
#undef DEF_D_INTRINSIC
};

// Call an fold the intrinsic call CALLEE with the argument ARG
// with the built-in function CODE passed.

static tree
expand_intrinsic_op (built_in_function code, tree callee, tree arg)
{
  tree exp = d_build_call_nary (builtin_decl_explicit (code), 1, arg);
  return fold_convert (TREE_TYPE (callee), fold (exp));
}

// Like expand_intrinsic_op, but takes two arguments.

static tree
expand_intrinsic_op2 (built_in_function code, tree callee, tree arg1, tree arg2)
{
  tree exp = d_build_call_nary (builtin_decl_explicit (code), 2, arg1, arg2);
  return fold_convert (TREE_TYPE (callee), fold (exp));
}

// Expand a front-end instrinsic call to bsr whose arguments are ARG.
// The original call expression is held in CALLEE.

static tree
expand_intrinsic_bsr (tree callee, tree arg)
{
  // intrinsic_code bsr gets turned into (size - 1) - count_leading_zeros(arg).
  // %% TODO: The return value is supposed to be undefined if arg is zero.
  tree type = TREE_TYPE (arg);
  tree tsize = build_integer_cst (TREE_INT_CST_LOW (TYPE_SIZE (type)) - 1, type);
  tree exp = expand_intrinsic_op (BUILT_IN_CLZL, callee, arg);

  // Handle int -> long conversions.
  if (TREE_TYPE (exp) != type)
    exp = fold_convert (type, exp);

  exp = fold_build2 (MINUS_EXPR, type, tsize, exp);
  return fold_convert (TREE_TYPE (callee), exp);
}

// Expand the front-end built-in function INTRINSIC, which is either a
// call to bt, btc, btr, or bts.  These intrinsics take two arguments,
// ARG1 and ARG2, and the original call expression is held in CALLEE.

static tree
expand_intrinsic_bt (intrinsic_code intrinsic, tree callee, tree arg1, tree arg2)
{
  tree type = TREE_TYPE (TREE_TYPE (arg1));
  tree exp = build_integer_cst (TREE_INT_CST_LOW (TYPE_SIZE (type)), type);
  tree_code code;
  tree tval;

  // arg1[arg2 / exp]
  arg1 = build_array_index (arg1, fold_build2 (TRUNC_DIV_EXPR, type, arg2, exp));
  arg1 = indirect_ref (type, arg1);

  // mask = 1 << (arg2 % exp);
  arg2 = fold_build2 (TRUNC_MOD_EXPR, type, arg2, exp);
  arg2 = fold_build2 (LSHIFT_EXPR, type, size_one_node, arg2);

  // cond = arg1[arg2 / size] & mask;
  exp = fold_build2 (BIT_AND_EXPR, type, arg1, arg2);

  // cond ? -1 : 0;
  exp = fold_build3 (COND_EXPR, TREE_TYPE (callee), d_truthvalue_conversion (exp),
                    integer_minus_one_node, integer_zero_node);

  // Update the bit as needed.
  code = (intrinsic == INTRINSIC_BTC) ? BIT_XOR_EXPR :
    (intrinsic == INTRINSIC_BTR) ? BIT_AND_EXPR :
    (intrinsic == INTRINSIC_BTS) ? BIT_IOR_EXPR : ERROR_MARK;
  gcc_assert (code != ERROR_MARK);

  // arg1[arg2 / size] op= mask
  if (intrinsic == INTRINSIC_BTR)
    arg2 = fold_build1 (BIT_NOT_EXPR, TREE_TYPE (arg2), arg2);

  tval = build_local_temp (TREE_TYPE (callee));
  exp = vmodify_expr (tval, exp);
  arg1 = vmodify_expr (arg1, fold_build2 (code, TREE_TYPE (arg1), arg1, arg2));

  return compound_expr (exp, compound_expr (arg1, tval));
}

// Expand a front-end built-in call to va_arg, whose arguments are
// ARG1 and optionally ARG2.
// The original call expression is held in CALLEE.

// The cases handled here are:
//	va_arg!T(ap);
//	=>	return (T) VA_ARG_EXP<ap>
//
//	va_arg!T(ap, T arg);
//	=>	return arg = (T) VA_ARG_EXP<ap>;

static tree
expand_intrinsic_vaarg(tree callee, tree arg1, tree arg2)
{
  tree type;

  STRIP_NOPS(arg1);

  if (TREE_CODE(arg1) == ADDR_EXPR)
    arg1 = TREE_OPERAND(arg1, 0);
  else if (TREE_CODE(TREE_TYPE(arg1)) == REFERENCE_TYPE)
    arg1 = build_deref(arg1);

  if (arg2 == NULL_TREE)
    type = TREE_TYPE(callee);
  else
    {
      STRIP_NOPS(arg2);
      gcc_assert(TREE_CODE(arg2) == ADDR_EXPR);
      arg2 = TREE_OPERAND(arg2, 0);
      type = TREE_TYPE(arg2);
    }

  // Silently convert promoted types.
  tree ptype = lang_hooks.types.type_promotes_to(type);
  tree exp = build1(VA_ARG_EXPR, ptype, arg1);

  if (type != ptype)
    exp = fold_convert(type, exp);

  if (arg2 != NULL_TREE)
    exp = vmodify_expr(arg2, exp);

  return exp;
}

// Expand a front-end built-in call to va_start, whose arguments are
// ARG1 and ARG2.  The original call expression is held in CALLEE.

static tree
expand_intrinsic_vastart (tree callee, tree arg1, tree arg2)
{
  // The va_list argument should already have its address taken.
  // The second argument, however, is inout and that needs to be
  // fixed to prevent a warning.

  // Could be casting... so need to check type too?
  STRIP_NOPS (arg1);
  STRIP_NOPS (arg2);
  gcc_assert (TREE_CODE (arg1) == ADDR_EXPR && TREE_CODE (arg2) == ADDR_EXPR);

  arg2 = TREE_OPERAND (arg2, 0);
  // Assuming nobody tries to change the return type.
  return expand_intrinsic_op2 (BUILT_IN_VA_START, callee, arg1, arg2);
}

// Checks if DECL is an intrinsic or runtime library function that
// requires special processing.  Marks the generated trees for DECL
// as BUILT_IN_FRONTEND so can be identified later.

void
maybe_set_intrinsic (FuncDeclaration *decl)
{
  if (!decl->ident || decl->builtin == BUILTINyes)
    return;

  // Check if it's a compiler intrinsic.  We only require that any
  // internally recognised intrinsics are declared in a module with
  // an explicit module declaration.
  Module *m = decl->getModule();
  if (!m || !m->md)
    return;

  // Look through all D intrinsics.
  TemplateInstance *ti = decl->isInstantiated();
  TemplateDeclaration *td = ti ? ti->tempdecl->isTemplateDeclaration() : NULL;
  const char *tname = decl->ident->string;
  const char *tmodule = m->md->toChars();
  const char *tdeco = decl->type->deco;

  for (size_t i = 0; i < (int) INTRINSIC_LAST; i++)
    {
      if (strcmp (intrinsic_decls[i].name, tname) != 0
	  || strcmp (intrinsic_decls[i].module, tmodule) != 0)
	continue;

      if (td && td->onemember)
	{
	  FuncDeclaration *fd = td->onemember->isFuncDeclaration();
	  if (fd != NULL
	      && strcmp (fd->type->toChars(), intrinsic_decls[i].deco) == 0)
	    goto Lfound;
	}
      else if (strcmp (intrinsic_decls[i].deco, tdeco) == 0)
	{
	Lfound:
	  intrinsic_code code = intrinsic_decls[i].code;

	  if (decl->csym == NULL)
	    {
	      // Store a stub BUILT_IN_FRONTEND decl.
	      decl->csym = new Symbol();
	      decl->csym->Stree = build_decl (BUILTINS_LOCATION, FUNCTION_DECL,
					      NULL_TREE, NULL_TREE);
	      DECL_NAME (decl->csym->Stree) = get_identifier (tname);
	      TREE_TYPE (decl->csym->Stree) = decl->type->toCtype();
	      d_keep (decl->csym->Stree);
	    }

	  DECL_BUILT_IN_CLASS (decl->csym->Stree) = BUILT_IN_FRONTEND;
	  DECL_FUNCTION_CODE (decl->csym->Stree) = (built_in_function) code;
	  decl->builtin = BUILTINyes;
	  break;
	}
    }
}

// If CALLEXP is a BUILT_IN_FRONTEND, expand and return inlined
// compiler generated instructions. Most map onto GCC builtins,
// others require a little extra work around them.

tree
expand_intrinsic (tree callexp)
{
  CallExpr ce (callexp);
  tree callee = ce.callee();

  if (POINTER_TYPE_P (TREE_TYPE (callee)))
    callee = TREE_OPERAND (callee, 0);

  if (TREE_CODE (callee) == FUNCTION_DECL
      && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_FRONTEND)
    {
      intrinsic_code intrinsic = (intrinsic_code) DECL_FUNCTION_CODE (callee);
      tree op1, op2;
      tree type;

      switch (intrinsic)
	{
	case INTRINSIC_BSF:
	  // Builtin count_trailing_zeros matches behaviour of bsf
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_CTZL, callexp, op1);

	case INTRINSIC_BSR:
	  op1 = ce.nextArg();
	  return expand_intrinsic_bsr (callexp, op1);

	case INTRINSIC_BTC:
	case INTRINSIC_BTR:
	case INTRINSIC_BTS:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  return expand_intrinsic_bt (intrinsic, callexp, op1, op2);

	case INTRINSIC_BSWAP:
	  // Backend provides builtin bswap32.
	  // Assumes first argument and return type is uint.
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_BSWAP32, callexp, op1);

	  // Math intrinsics just map to their GCC equivalents.
	case INTRINSIC_COS:
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_COSL, callexp, op1);

	case INTRINSIC_SIN:
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_SINL, callexp, op1);

	case INTRINSIC_RNDTOL:
	  // Not sure if llroundl stands as a good replacement for the
	  // expected behaviour of rndtol.
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_LLROUNDL, callexp, op1);

	case INTRINSIC_SQRT:
	case INTRINSIC_SQRTF:
	case INTRINSIC_SQRTL:
	  // Have float, double and real variants of sqrt.
	  op1 = ce.nextArg();
	  type = TYPE_MAIN_VARIANT (TREE_TYPE (op1));
	  // op1 is an integral type - use double precision.
	  if (INTEGRAL_TYPE_P (type))
	    op1 = convert (double_type_node, op1);

	  if (intrinsic == INTRINSIC_SQRT)
	    return expand_intrinsic_op (BUILT_IN_SQRT, callexp, op1);
	  else if (intrinsic == INTRINSIC_SQRTF)
	    return expand_intrinsic_op (BUILT_IN_SQRTF, callexp, op1);
	  else if (intrinsic == INTRINSIC_SQRTL)
	    return expand_intrinsic_op (BUILT_IN_SQRTL, callexp, op1);

	  gcc_unreachable();
	  break;

	case INTRINSIC_LDEXP:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  return expand_intrinsic_op2 (BUILT_IN_LDEXPL, callexp, op1, op2);

	case INTRINSIC_FABS:
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_FABSL, callexp, op1);

	case INTRINSIC_RINT:
	  op1 = ce.nextArg();
	  return expand_intrinsic_op (BUILT_IN_RINTL, callexp, op1);

	case INTRINSIC_VA_ARG:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  return expand_intrinsic_vaarg (callexp, op1, op2);

	case INTRINSIC_C_VA_ARG:
	  op1 = ce.nextArg();
	  return expand_intrinsic_vaarg (callexp, op1, NULL_TREE);

	case INTRINSIC_VASTART:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  return expand_intrinsic_vastart (callexp, op1, op2);

	default:
	  gcc_unreachable();
	}
    }

  return callexp;
}

// Build and return the correct call to fmod depending on TYPE.
// ARG0 and ARG1 are the arguments pass to the function.

tree
build_float_modulus (tree type, tree arg0, tree arg1)
{
  tree fmodfn = NULL_TREE;
  tree basetype = type;

  if (COMPLEX_FLOAT_TYPE_P (basetype))
    basetype = TREE_TYPE (basetype);

  if (TYPE_MAIN_VARIANT (basetype) == double_type_node)
    fmodfn = builtin_decl_explicit (BUILT_IN_FMOD);
  else if (TYPE_MAIN_VARIANT (basetype) == float_type_node)
    fmodfn = builtin_decl_explicit (BUILT_IN_FMODF);
  else if (TYPE_MAIN_VARIANT (basetype) == long_double_type_node)
    fmodfn = builtin_decl_explicit (BUILT_IN_FMODL);

  if (!fmodfn)
    {
      // %qT pretty prints the tree type.
      error ("tried to perform floating-point modulo division on %qT", type);
      return error_mark_node;
    }

  if (COMPLEX_FLOAT_TYPE_P (type))
    return build2 (COMPLEX_EXPR, type,
		   d_build_call_nary (fmodfn, 2, real_part (arg0), arg1),
		   d_build_call_nary (fmodfn, 2, imaginary_part (arg0), arg1));

  if (SCALAR_FLOAT_TYPE_P (type))
    return d_build_call_nary (fmodfn, 2, arg0, arg1);

  // Should have caught this above.
  gcc_unreachable();
}

// Returns typeinfo reference for type T.

tree
build_typeinfo (Type *t)
{
  tree tinfo = t->getInternalTypeInfo (NULL)->toElem (current_irstate);
  gcc_assert (POINTER_TYPE_P (TREE_TYPE (tinfo)));
  return tinfo;
}

// Build and return D's internal exception Object.
// Different from the generic exception pointer.

tree
build_exception_object()
{
  tree obj_type = build_object_type()->toCtype();

  if (TREE_CODE (TREE_TYPE (obj_type)) == REFERENCE_TYPE)
    obj_type = TREE_TYPE (obj_type);

  // Like Java, the actual D exception object is one
  // pointer behind the exception header
  tree eh = d_build_call_nary (builtin_decl_explicit (BUILT_IN_EH_POINTER),
			       1, integer_zero_node);

  // treat exception header as (Object *)
  eh = build1 (NOP_EXPR, build_pointer_type (obj_type), eh);
  eh = build_offset_op (MINUS_EXPR, eh, TYPE_SIZE_UNIT (TREE_TYPE (eh)));

  return build1 (INDIRECT_REF, obj_type, eh);
}

// Build LABEL_DECL at location LOC for IDENT given.

tree
d_build_label (Loc loc, Identifier *ident)
{
  tree decl = build_decl (UNKNOWN_LOCATION, LABEL_DECL,
			  ident ? get_identifier (ident->string) : NULL_TREE, void_type_node);
  DECL_CONTEXT (decl) = current_function_decl;
  DECL_MODE (decl) = VOIDmode;

  // Not setting this doesn't seem to cause problems (unlike VAR_DECLs).
  if (loc.filename)
    set_decl_location (decl, loc);

  return decl;
}

// If SYM is a nested function, return the static chain to be
// used when calling that function from FUNC.

// If SYM is a nested class or struct, return the static chain
// to be used when creating an instance of the class from FUNC.

tree
get_frame_for_symbol (FuncDeclaration *func, Dsymbol *sym)
{
  FuncDeclaration *thisfd = sym->isFuncDeclaration();
  FuncDeclaration *parentfd = NULL;

  if (thisfd != NULL)
    {
      // Check that the nested function is properly defined.
      if (!thisfd->fbody)
	{
	  // Should instead error on line that references 'thisfd'.
	  thisfd->error ("nested function missing body");
	  return null_pointer_node;
	}

      // Special case for __ensure and __require.
      if (thisfd->ident == Id::ensure || thisfd->ident == Id::require)
	parentfd = func;
      else
	parentfd = thisfd->toParent2()->isFuncDeclaration();
    }
  else
    {
      // It's a class (or struct).  NewExp::toElem has already determined its
      // outer scope is not another class, so it must be a function.
      while (sym && !sym->isFuncDeclaration())
	sym = sym->toParent2();

      parentfd = (FuncDeclaration *) sym;
    }

  gcc_assert (parentfd != NULL);

  if (func != parentfd)
    {
      // If no frame pointer for this function
      if (!func->vthis)
	{
	  sym->error ("is a nested function and cannot be accessed from %s", func->toChars());
	  return null_pointer_node;
	}

      // Make sure we can get the frame pointer to the outer function.
      // Go up each nesting level until we find the enclosing function.
      Dsymbol *dsym = func;

      while (thisfd != dsym)
	{
	  // Check if enclosing function is a function.
	  FuncDeclaration *fd = dsym->isFuncDeclaration();

	  if (fd != NULL)
	    {
	      if (parentfd == fd->toParent2())
		break;

	      gcc_assert (fd->isNested() || fd->vthis);
	      dsym = dsym->toParent2();
	      continue;
	    }

	  // Check if enclosed by an aggregate. That means the current
	  // function must be a member function of that aggregate.
	  AggregateDeclaration *ad = dsym->isAggregateDeclaration();

	  if (ad == NULL)
	    goto Lnoframe;
	  if (ad->isClassDeclaration() && parentfd == ad->toParent2())
	    break;
	  if (ad->isStructDeclaration() && parentfd == ad->toParent2())
	    break;

	  if (!ad->isNested() || !ad->vthis)
	    {
	    Lnoframe:
	      func->error ("cannot get frame pointer to %s", sym->toChars());
	      return null_pointer_node;
	    }

	  dsym = dsym->toParent2();
	}
    }

  FuncFrameInfo *ffo = get_frameinfo (parentfd);
  if (ffo->creates_frame || ffo->static_chain)
    return get_framedecl (func, parentfd);

  return null_pointer_node;
}

// Return the parent function of a nested class CD.

static FuncDeclaration *
d_nested_class (ClassDeclaration *cd)
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

// Return the parent function of a nested struct SD.

static FuncDeclaration *
d_nested_struct (StructDeclaration *sd)
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


// Starting from the current function FUNC, try to find a suitable value of
// 'this' in nested function instances.  A suitable 'this' value is an
// instance of OCD or a class that has OCD as a base.

static tree
find_this_tree(FuncDeclaration *func, ClassDeclaration *ocd)
{
  while (func)
    {
      AggregateDeclaration *ad = func->isThis();
      ClassDeclaration *cd = ad ? ad->isClassDeclaration() : NULL;

      if (cd != NULL)
	{
	  if (ocd == cd)
	    return get_decl_tree(func->vthis, func);
	  else if (ocd->isBaseOf(cd, NULL))
	    return convert_expr(get_decl_tree(func->vthis, func), cd->type, ocd->type);

	  func = d_nested_class(cd);
	}
      else
	{
	  if (func->isNested())
	    {
	      func = func->toParent2()->isFuncDeclaration();
	      continue;
	    }

	  func = NULL;
	}
    }

  return NULL_TREE;
}

// Retrieve the outer class/struct 'this' value of DECL from the function FD.

tree
build_vthis(AggregateDeclaration *decl, FuncDeclaration *fd)
{
  ClassDeclaration *cd = decl->isClassDeclaration();
  StructDeclaration *sd = decl->isStructDeclaration();

  // If an aggregate nested in a function has no methods and there are no
  // other nested functions, any static chain created here will never be
  // translated.  Use a null pointer for the link in this case.
  tree vthis_value = null_pointer_node;

  if (cd != NULL || sd != NULL)
    {
      Dsymbol *outer = decl->toParent2();

      // If the parent is a templated struct, the outer context is instead
      // the enclosing symbol of where the instantiation happened.
      if (outer->isStructDeclaration())
	{
	  gcc_assert(outer->parent && outer->parent->isTemplateInstance());
	  outer = ((TemplateInstance *) outer->parent)->enclosing;
	}

      // For outer classes, get a suitable 'this' value.
      // For outer functions, get a suitable frame/closure pointer.
      ClassDeclaration *cdo = outer->isClassDeclaration();
      FuncDeclaration *fdo = outer->isFuncDeclaration();

      if (cdo)
	{
	  vthis_value = find_this_tree(fd, cdo);
	  gcc_assert(vthis_value != NULL_TREE);
	}
      else if (fdo)
	{
	  FuncFrameInfo *ffo = get_frameinfo(fdo);
	  if (ffo->creates_frame || ffo->static_chain
	      || fdo->hasNestedFrameRefs())
	    vthis_value = get_frame_for_symbol(fd, decl);
	  else if (cd != NULL)
	    {
	      // Classes nested in methods are allowed to access any outer
	      // class fields, use the function chain in this case.
	      if (fdo->vthis && fdo->vthis->type != Type::tvoidptr)
		vthis_value = get_decl_tree(fdo->vthis, fd);
	    }
	}
      else
	gcc_unreachable();
    }

  return vthis_value;
}

tree
build_frame_type (FuncDeclaration *func)
{
  FuncFrameInfo *ffi = get_frameinfo (func);

  if (ffi->frame_rec != NULL_TREE)
    return ffi->frame_rec;

  tree frame_rec_type = make_node (RECORD_TYPE);
  char *name = concat (ffi->is_closure ? "CLOSURE." : "FRAME.",
		       func->toPrettyChars(), NULL);
  TYPE_NAME (frame_rec_type) = get_identifier (name);
  free (name);

  tree ptr_field = build_decl (BUILTINS_LOCATION, FIELD_DECL,
			       get_identifier ("__chain"), ptr_type_node);
  DECL_CONTEXT (ptr_field) = frame_rec_type;
  TYPE_READONLY (frame_rec_type) = 1;

  tree fields = chainon (NULL_TREE, ptr_field);

  if (!ffi->is_closure)
    {
      // __ensure and __require never becomes a closure, but could still be referencing
      // parameters of the calling function.  So we add all parameters as nested refs.
      // This is written as such so that all parameters appear at the front of the frame
      // so that overriding methods match the same layout when inheriting a contract.
      if ((global.params.useIn && func->frequire) || (global.params.useOut && func->fensure))
	{
	  for (size_t i = 0; func->parameters && i < func->parameters->dim; i++)
	    {
	      VarDeclaration *v = (*func->parameters)[i];
	      // Remove if already in closureVars so can push to front.
	      for (size_t j = i; j < func->closureVars.dim; j++)
		{
		  Dsymbol *s = func->closureVars[j];
		  if (s == v)
		    {
		      func->closureVars.remove (j);
		      break;
		    }
		}
	      func->closureVars.insert (i, v);
	    }

	  // Also add hidden 'this' to outer context.
	  if (func->vthis)
	    {
	      for (size_t i = 0; i < func->closureVars.dim; i++)
		{
		  Dsymbol *s = func->closureVars[i];
		  if (s == func->vthis)
		    {
		      func->closureVars.remove (i);
		      break;
		    }
		}
	      func->closureVars.insert (0, func->vthis);
	    }
	}
    }

  for (size_t i = 0; i < func->closureVars.dim; i++)
    {
      VarDeclaration *v = func->closureVars[i];
      Symbol *s = v->toSymbol();
      tree field = build_decl (BUILTINS_LOCATION, FIELD_DECL,
			       v->ident ? get_identifier (v->ident->string) : NULL_TREE,
			       declaration_type (v));
      s->SframeField = field;
      set_decl_location (field, v);
      DECL_CONTEXT (field) = frame_rec_type;
      fields = chainon (fields, field);
      TREE_USED (s->Stree) = 1;

      // Can't do nrvo if the variable is put in a frame.
      if (func->nrvo_can && func->nrvo_var == v)
	func->nrvo_can = 0;

      // Because the value needs to survive the end of the scope.
      if (ffi->is_closure && v->needsAutoDtor())
	v->error("has scoped destruction, cannot build closure");
    }

  TYPE_FIELDS (frame_rec_type) = fields;
  layout_type (frame_rec_type);
  d_keep (frame_rec_type);

  return frame_rec_type;
}

// Closures are implemented by taking the local variables that
// need to survive the scope of the function, and copying them
// into a gc allocated chuck of memory. That chunk, called the
// closure here, is inserted into the linked list of stack
// frames instead of the usual stack frame.

// If a closure is not required, but FD still needs a frame to lower
// nested refs, then instead build custom static chain decl on stack.

void
build_closure(FuncDeclaration *fd, IRState *irs)
{
  FuncFrameInfo *ffi = get_frameinfo(fd);

  if (!ffi->creates_frame)
    return;

  tree type = build_frame_type(fd);
  gcc_assert(COMPLETE_TYPE_P(type));

  tree decl, decl_ref;

  if (ffi->is_closure)
    {
      decl = build_local_temp(build_pointer_type(type));
      DECL_NAME(decl) = get_identifier("__closptr");
      decl_ref = build_deref(decl);

      // Allocate memory for closure.
      tree arg = convert(Type::tsize_t->toCtype(), TYPE_SIZE_UNIT(type));
      tree init = build_libcall(LIBCALL_ALLOCMEMORY, 1, &arg);

      DECL_INITIAL(decl) = build_nop(TREE_TYPE(decl), init);
    }
  else
    {
      decl = build_local_temp(type);
      DECL_NAME(decl) = get_identifier("__frame");
      decl_ref = decl;
    }

  DECL_IGNORED_P(decl) = 0;
  expand_decl(decl);

  // Set the first entry to the parent closure/frame, if any.
  tree chain_field = component_ref(decl_ref, TYPE_FIELDS(type));
  tree chain_expr = vmodify_expr(chain_field, irs->sthis);
  irs->addExp(chain_expr);

  // Copy parameters that are referenced nonlocally.
  for (size_t i = 0; i < fd->closureVars.dim; i++)
    {
      VarDeclaration *v = fd->closureVars[i];

      if (!v->isParameter())
	continue;

      Symbol *vsym = v->toSymbol();

      tree field = component_ref (decl_ref, vsym->SframeField);
      tree expr = vmodify_expr (field, vsym->Stree);
      irs->addExp (expr);
    }

  if (!ffi->is_closure)
    decl = build_address (decl);

  irs->sthis = decl;
}

// Return the frame of FD.  This could be a static chain or a closure
// passed via the hidden 'this' pointer.

FuncFrameInfo *
get_frameinfo(FuncDeclaration *fd)
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

  // Nested functions, or functions with nested refs must create
  // a static frame for local variables to be referenced from.
  if (fd->closureVars.dim != 0)
    ffi->creates_frame = true;

  if (fd->vthis && fd->vthis->type == Type::tvoidptr)
    ffi->creates_frame = true;

  // Functions with In/Out contracts pass parameters to nested frame.
  if (fd->fensure || fd->frequire)
    ffi->creates_frame = true;

  // D2 maybe setup closure instead.
  if (fd->needsClosure())
    {
      ffi->creates_frame = true;
      ffi->is_closure = true;
    }
  else if (fd->closureVars.dim == 0)
    {
      /* If fd is nested (deeply) in a function that creates a closure,
	 then fd inherits that closure via hidden vthis pointer, and
	 doesn't create a stack frame at all.  */
      FuncDeclaration *ff = fd;

      while (ff)
	{
	  FuncFrameInfo *ffo = get_frameinfo (ff);
	  AggregateDeclaration *ad;

	  if (ff != fd && ffo->creates_frame)
	    {
	      gcc_assert (ffo->frame_rec);
	      ffi->creates_frame = false;
	      ffi->static_chain = true;
	      ffi->is_closure = ffo->is_closure;
	      gcc_assert (COMPLETE_TYPE_P (ffo->frame_rec));
	      ffi->frame_rec = ffo->frame_rec;
	      break;
	    }

	  // Stop looking if no frame pointer for this function.
	  if (ff->vthis == NULL)
	    break;

	  ad = ff->isThis();
	  if (ad && ad->isNested())
	    {
	      while (ad->isNested())
		{
		  Dsymbol *d = ad->toParent2();
		  ad = d->isAggregateDeclaration();
		  ff = d->isFuncDeclaration();

		  if (ad == NULL)
		    break;
		}
	    }
	  else
	    ff = ff->toParent2()->isFuncDeclaration();
	}
    }

  // Build type now as may be referenced from another module.
  if (ffi->creates_frame)
    ffi->frame_rec = build_frame_type (fd);

  return ffi;
}

// Return a pointer to the frame/closure block of OUTER
// so can be accessed from the function INNER.

tree
get_framedecl (FuncDeclaration *inner, FuncDeclaration *outer)
{
  tree result = current_irstate->sthis;
  FuncDeclaration *fd = inner;

  while (fd && fd != outer)
    {
      AggregateDeclaration *ad;
      ClassDeclaration *cd;
      StructDeclaration *sd;

      // Parent frame link is the first field.
      if (get_frameinfo (fd)->creates_frame)
	result = indirect_ref (ptr_type_node, result);

      if (fd->isNested())
	fd = fd->toParent2()->isFuncDeclaration();
      // The frame/closure record always points to the outer function's
      // frame, even if there are intervening nested classes or structs.
      // So, we can just skip over these...
      else if ((ad = fd->isThis()) && (cd = ad->isClassDeclaration()))
	fd = d_nested_class (cd);
      else if ((ad = fd->isThis()) && (sd = ad->isStructDeclaration()))
	fd = d_nested_struct (sd);
      else
	break;
    }

  // Go get our frame record.
  gcc_assert (fd == outer);
  tree frame_rec = get_frameinfo (outer)->frame_rec;

  if (frame_rec != NULL_TREE)
    {
      result = build_nop (build_pointer_type (frame_rec), result);
      return result;
    }
  else
    {
      inner->error ("forward reference to frame of %s", outer->toChars());
      return null_pointer_node;
    }
}

// Build and return expression tree for WrappedExp.

elem *
WrappedExp::toElem (IRState *)
{
  return this->e1;
}

// Write out all fields for aggregate DECL.  For classes, write
// out base class fields first, and adds all interfaces last.

void
layout_aggregate_type(AggLayout *al, AggregateDeclaration *decl)
{
  ClassDeclaration *cd = decl->isClassDeclaration();
  bool inherited_p = (al->decl != decl);

  if (cd != NULL)
    {
      if (cd->baseClass)
	layout_aggregate_type(al, cd->baseClass);
      else
	{
	  // This is the base class (Object) or interface.
	  tree objtype = TREE_TYPE(cd->type->toCtype());

	  // Add the virtual table pointer, and optionally the monitor fields.
	  tree field = build_decl(UNKNOWN_LOCATION, FIELD_DECL,
				  get_identifier("__vptr"), vtbl_ptr_type_node);
	  DECL_ARTIFICIAL(field) = 1;
	  DECL_IGNORED_P(field) = inherited_p;

	  insert_aggregate_field(al, field, 0);

	  DECL_VIRTUAL_P(field) = 1;
	  DECL_FCONTEXT(field) = objtype;
	  TYPE_VFIELD(al->type) = field;

	  if (!cd->cpp)
	    {
	      field = build_decl(UNKNOWN_LOCATION, FIELD_DECL,
				 get_identifier("__monitor"), ptr_type_node);
	      DECL_FCONTEXT(field) = objtype;
	      DECL_ARTIFICIAL(field) = 1;
	      DECL_IGNORED_P(field) = inherited_p;
	      insert_aggregate_field(al, field, Target::ptrsize);
	    }
	}
    }

  if (decl->fields.dim)
    {
      tree fcontext = decl->type->toCtype();

      if (POINTER_TYPE_P(fcontext))
	fcontext = TREE_TYPE(fcontext);

      for (size_t i = 0; i < decl->fields.dim; i++)
	{
	  // D anonymous unions just put the fields into the outer struct...
	  // Does this cause problems?
	  VarDeclaration *var = decl->fields[i];
	  gcc_assert(var && var->isField());

	  tree ident = var->ident ? get_identifier(var->ident->string) : NULL_TREE;
	  tree field = build_decl(UNKNOWN_LOCATION, FIELD_DECL, ident,
				  declaration_type(var));
	  set_decl_location(field, var);
	  var->csym = new Symbol;
	  var->csym->Stree = field;

	  DECL_CONTEXT(field) = al->type;
	  DECL_FCONTEXT(field) = fcontext;
	  DECL_FIELD_OFFSET(field) = size_int(var->offset);
	  DECL_FIELD_BIT_OFFSET(field) = bitsize_zero_node;

	  DECL_ARTIFICIAL(field) = inherited_p;
	  DECL_IGNORED_P(field) = inherited_p;
	  SET_DECL_OFFSET_ALIGN(field, TYPE_ALIGN(TREE_TYPE(field)));

	  TREE_THIS_VOLATILE(field) = TYPE_VOLATILE(TREE_TYPE(field));
	  layout_decl(field, 0);

	  if (var->size(var->loc))
	    {
	      gcc_assert(DECL_MODE(field) != VOIDmode);
	      gcc_assert(DECL_SIZE(field) != NULL_TREE);
	    }

	  TYPE_FIELDS(al->type) = chainon(TYPE_FIELDS(al->type), field);
	}
    }

  if (cd && cd->vtblInterfaces)
    {
      for (size_t i = 0; i < cd->vtblInterfaces->dim; i++)
	{
	  BaseClass *bc = (*cd->vtblInterfaces)[i];
	  tree field = build_decl(UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE,
				  Type::tvoidptr->pointerTo()->toCtype());
	  DECL_ARTIFICIAL(field) = 1;
	  DECL_IGNORED_P(field) = 1;
	  insert_aggregate_field(al, field, bc->offset);
	}
    }
}

// Add a compiler generated field DECL at OFFSET into aggregate.

void
insert_aggregate_field (AggLayout *al, tree decl, size_t offset)
{
  DECL_CONTEXT (decl) = al->type;
  SET_DECL_OFFSET_ALIGN (decl, TYPE_ALIGN (TREE_TYPE (decl)));
  DECL_FIELD_OFFSET (decl) = size_int (offset);
  DECL_FIELD_BIT_OFFSET (decl) = bitsize_zero_node;

  // Must set this or we crash with DWARF debugging.
  set_decl_location (decl, al->decl->loc);

  TREE_THIS_VOLATILE (decl) = TYPE_VOLATILE (TREE_TYPE (decl));

  layout_decl (decl, 0);
  TYPE_FIELDS(al->type) = chainon (TYPE_FIELDS (al->type), decl);
}

// Wrap-up and compute finalised aggregate type.  Writing out
// any GCC attributes that were applied to the type declaration.

void
finish_aggregate_type (AggLayout *al, UserAttributeDeclaration *declattrs)
{
  unsigned structsize = al->decl->structsize;
  unsigned alignsize = al->decl->alignsize;

  TYPE_SIZE (al->type) = NULL_TREE;

  if (declattrs)
    {
      Expressions *attrs = declattrs->getAttributes();
      decl_attributes (&al->type, build_attributes (attrs),
		       ATTR_FLAG_TYPE_IN_PLACE);
    }

  TYPE_SIZE (al->type) = bitsize_int (structsize * BITS_PER_UNIT);
  TYPE_SIZE_UNIT (al->type) = size_int (structsize);
  TYPE_ALIGN (al->type) = alignsize * BITS_PER_UNIT;
  TYPE_PACKED (al->type) = (alignsize == 1);

  compute_record_mode (al->type);

  // Set up variants.
  for (tree x = TYPE_MAIN_VARIANT (al->type); x; x = TYPE_NEXT_VARIANT (x))
    {
      TYPE_FIELDS (x) = TYPE_FIELDS (al->type);
      TYPE_LANG_SPECIFIC (x) = TYPE_LANG_SPECIFIC (al->type);
      TYPE_ALIGN (x) = TYPE_ALIGN (al->type);
      TYPE_USER_ALIGN (x) = TYPE_USER_ALIGN (al->type);
    }
}

// Routines for getting an index or slice of an array where '$' was used
// in the slice.  A temp var INI_V would have been created that needs to
// be bound into it's own scope.

ArrayScope::ArrayScope (VarDeclaration *ini_v, const Loc& loc) :
  var_(ini_v)
{
  /* If STCconst, the temp var is not required.  */
  if (this->var_ && !(this->var_->storage_class & STCconst))
    {
      /* Need to set the location or the expand_decl in the BIND_EXPR will
	 cause the line numbering for the statement to be incorrect. */
      /* The variable itself is not included in the debugging information. */
      this->var_->loc = loc;
      Symbol *s = this->var_->toSymbol();
      tree decl = s->Stree;
      DECL_CONTEXT (decl) = current_function_decl;
    }
  else
    this->var_ = NULL;
}

// Set index expression E of type T as the initialiser for
// the temp var decl to be used.

tree
ArrayScope::setArrayExp (tree e, Type *t)
{
  if (this->var_)
    {
      tree v = this->var_->toSymbol()->Stree;
      if (t->toBasetype()->ty != Tsarray)
	e = maybe_make_temp (e);
      DECL_INITIAL (v) = get_array_length (e, t);
    }
  return e;
}

// Wrap-up temp var into a BIND_EXPR.

tree
ArrayScope::finish (tree e)
{
  if (this->var_)
    {
      Symbol *s = this->var_->toSymbol();
      tree t = s->Stree;
      if (TREE_CODE (t) == VAR_DECL)
	{
	  gcc_assert (!s->SframeField);
	  return bind_expr (t, e);
	}
      else
	gcc_unreachable();
    }
  return e;
}

