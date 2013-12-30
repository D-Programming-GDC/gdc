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

#include "template.h"
#include "init.h"
#include "id.h"
#include "dfrontend/target.h"


Module *current_module_decl;
IRState *current_irstate;


// Return the DECL_CONTEXT for symbol DSYM.

tree
d_decl_context (Dsymbol *dsym)
{
  Dsymbol *orig_sym = dsym;
  AggregateDeclaration *ad;

  while ((dsym = dsym->toParent2()))
    {
      if (dsym->isFuncDeclaration())
	{
	  // dwarf2out chokes without this check... (output_pubnames)
	  FuncDeclaration *f = orig_sym->isFuncDeclaration();
	  if (f && !needs_static_chain (f))
	    return NULL_TREE;

	  return dsym->toSymbol()->Stree;
	}
      else if ((ad = dsym->isAggregateDeclaration()))
	{
	  tree context = ad->type->toCtype();
	  if (ad->isClassDeclaration())
	    {
	      // RECORD_TYPE instead of REFERENCE_TYPE
	      context = TREE_TYPE (context);
	    }
	  return context;
	}
      else if (dsym->isModule())
	return dsym->toSymbol()->ScontextDecl;
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
    }

  // Static var or auto var that the back end will handle for us
  return decl->toSymbol()->Stree;
}

// Return expression EXP, whose type has been converted to TYPE.

tree
d_convert (tree type, tree exp)
{
  // Check this first before passing to build_dtype.
  if (error_mark_p (type) || error_mark_p (TREE_TYPE (exp)))
    return error_mark_node;

  Type *totype = build_dtype (type);
  Type *etype = build_dtype (TREE_TYPE (exp));

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

  if (error_mark_p (exp))
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
	  return error_mark (totype);
	}
      break;

    case Tstruct:
      if (tbtype->ty == Tstruct)
      {
	if (totype->size() == etype->size())
	  {
	    // Allowed to cast to structs with same type size.
	    result = build_vconvert (totype->toCtype(), exp);
	  }
	else if (tbtype->ty == Taarray)
	  {
	    tbtype = ((TypeAArray *) tbtype)->getImpl()->type;
	    return convert_expr (exp, etype, tbtype);
	  }
	else
	  {
	    error ("can't convert struct %s to %s", etype->toChars(), totype->toChars());
	    return error_mark (totype);
	  }
      }
      // else, default conversion, which should produce an error
      break;

    case Tclass:
      if (tbtype->ty == Tclass)
      {
	ClassDeclaration *cdfrom = tbtype->isClassHandle();
	ClassDeclaration *cdto = ebtype->isClassHandle();
	int offset;

	if (cdfrom->isBaseOf (cdto, &offset) && offset != OFFSET_RUNTIME)
	  {
	    // Casting up the inheritance tree: Don't do anything special.
	    // Cast to an implemented interface: Handle at compile time.
	    if (offset)
	      {
		tree t = totype->toCtype();
		exp = maybe_make_temp (exp);
		return build3 (COND_EXPR, t,
			       build_boolop (NE_EXPR, exp, d_null_pointer),
			       build_nop (t, build_offset (exp, size_int (offset))),
			       build_nop (t, d_null_pointer));
	      }

	    // d_convert will make a no-op cast
	    break;
	  }

	// More cases for no-op cast
	if (cdfrom == cdto)
	  break;

	if (cdfrom->cpp && cdto->cpp)
	  break;

	// Casting from a C++ interface to a class/non-C++ interface
	// always results in null as there is no runtime information,
	// and no way one can derive from the other.
	if (cdto->isCOMclass() || cdfrom->cpp != cdto->cpp)
	  {
	    warning (OPT_Wcast_result, "cast to %s will produce null result", totype->toChars());
	    result = d_convert (totype->toCtype(), d_null_pointer);
	    // Make sure the expression is still evaluated if necessary
	    if (TREE_SIDE_EFFECTS (exp))
	      result = compound_expr (exp, result);

	    return result;
	  }

	// The offset can only be determined at runtime, do dynamic cast
	tree args[2];
	args[0] = exp;
	args[1] = build_address (cdfrom->toSymbol()->Stree);

	return build_libcall (cdto->isInterfaceDeclaration()
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
	      return error_mark (totype);
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
	  return error_mark (totype);
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
	  return error_mark (totype);
	}
      break;

    case Taarray:
      if (tbtype->ty == Taarray)
	return build_vconvert (totype->toCtype(), exp);
      else if (tbtype->ty == Tstruct)
	{
	  ebtype = ((TypeAArray *) ebtype)->getImpl()->type;
	  return convert_expr (exp, ebtype, totype);
	}
      // Can convert associative arrays to void pointers.
      else if (tbtype == Type::tvoidptr)
	return build_vconvert (totype->toCtype(), exp);
      // else, default conversion, which should product an error
      break;

    case Tpointer:
      // Can convert void pointers to associative arrays too...
      if (tbtype->ty == Taarray && ebtype == Type::tvoidptr)
	return build_vconvert (totype->toCtype(), exp);
      break;

    case Tnull:
      if (tbtype->ty == Tarray)
	{
	  tree ptrtype = tbtype->nextOf()->pointerTo()->toCtype();
	  return d_array_value (totype->toCtype(), size_int (0),
				build_nop (ptrtype, exp));
	}
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

      if (d_types_compatible (telem, ebtype))
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

  if(!table)
    {
      size_t n = 0;
      for (const attribute_spec *p = d_attribute_table; p->name; p++)
        n++;

      if(n == 0)
        return false;

      table = new StringTable();
      table->_init(n);

      for (const attribute_spec *p = d_attribute_table; p->name; p++)
        table->insert(p->name, strlen(p->name));
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
          && mod->parent 
          && strcmp(mod->parent->toChars(), "gcc") == 0
          && !mod->parent->parent))
        continue;

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
build_integer_cst (dinteger_t value, tree type)
{
  // The type is error_mark_node, we can't do anything.
  if (error_mark_p (type))
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

// Convert LOW / HIGH pair into dinteger_t type.

dinteger_t
cst_to_hwi (double_int cst)
{
  if (cst.high == 0 || (cst.high == -1 && (HOST_WIDE_INT) cst.low < 0))
    return cst.low;
  else if (cst.low == 0 && cst.high == 1)
    return (~(dinteger_t) 0);

  gcc_unreachable();
}

// Return host integer value for INT_CST T.

dinteger_t
tree_to_hwi (tree t)
{
  if (TREE_INT_CST_HIGH (t) == 0
      || (TREE_INT_CST_HIGH (t) == -1
	  && (HOST_WIDE_INT) TREE_INT_CST_LOW (t) < 0
	  && !TYPE_UNSIGNED (TREE_TYPE (t))))
    return TREE_INT_CST_LOW (t);

  return cst_to_hwi (TREE_INT_CST (t));
}

// Returns the .length component from the D dynamic array EXP.

tree
d_array_length (tree exp)
{
  // backend will ICE otherwise
  if (error_mark_p (exp))
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
  if (error_mark_p (exp))
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
      return error_mark (type);
    }
}

// Return TRUE if binary expression EXP is an unhandled array operation,
// in which case we error that it is not implemented.

bool
unhandled_arrayop_p (BinExp *exp)
{
  TY ty1 = exp->e1->type->toBasetype()->ty;
  TY ty2 = exp->e2->type->toBasetype()->ty;

  if ((ty1 == Tarray || ty1 == Tsarray
       || ty2 == Tarray || ty2 == Tsarray))
    {
      exp->error ("Array operation %s not implemented", exp->toChars());
      return true;
    }
  return false;
}

// Create BINFO for a ClassDeclaration's inheritance tree.
// Interfaces are not included.

tree
build_class_binfo (tree super, ClassDeclaration *cd)
{
  tree binfo = make_tree_binfo (1);
  // Want RECORD_TYPE, not REFERENCE_TYPE
  BINFO_TYPE (binfo) = TREE_TYPE (cd->type->toCtype());
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

  // Want RECORD_TYPE, not REFERENCE_TYPE
  BINFO_TYPE (binfo) = TREE_TYPE (cd->type->toCtype());
  BINFO_INHERITANCE_CHAIN (binfo) = super;
  BINFO_OFFSET (binfo) = size_int (offset * Target::ptrsize);
  BINFO_VIRTUAL_P (binfo) = 1;

  for (size_t i = 0; i < cd->baseclasses->dim; i++, offset++)
    {
      BaseClass *bc = cd->baseclasses->tdata()[i];
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
build_two_field_type (tree t1, tree t2, Type *type, const char *n1, const char *n2)
{
  tree rectype = make_node (RECORD_TYPE);
  tree f0 = build_decl (BUILTINS_LOCATION, FIELD_DECL, get_identifier (n1), t1);
  tree f1 = build_decl (BUILTINS_LOCATION, FIELD_DECL, get_identifier (n2), t2);

  DECL_CONTEXT (f0) = rectype;
  DECL_CONTEXT (f1) = rectype;
  TYPE_FIELDS (rectype) = chainon (f0, f1);
  layout_type (rectype);

  if (type)
    {
      tree ident = get_identifier (type->toChars());
      tree stubdecl = build_decl (BUILTINS_LOCATION, TYPE_DECL, ident, rectype);

      TYPE_STUB_DECL (rectype) = stubdecl;
      TYPE_NAME (rectype) = stubdecl;
      DECL_ARTIFICIAL (stubdecl) = 1;
      rest_of_decl_compilation (stubdecl, 1, 0);
    }

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
  if (error_mark_p (result_type_node))
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
  tree condition = build2 (inclusive ? LE_EXPR : LT_EXPR, boolean_type_node, uindex, upr);

  // Build condition to test that INDEX >= 0.
  if (!TYPE_UNSIGNED (TREE_TYPE (index)))
    condition = build2 (TRUTH_ANDIF_EXPR, boolean_type_node, condition,
			build2 (GE_EXPR, boolean_type_node, index, integer_zero_node));

  return condition;
}

// Returns TRUE if array bounds checking code generation is turned on.

bool
array_bounds_check (void)
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

// Returns TRUE if T is an ERROR_MARK node.

bool
error_mark_p (tree t)
{
  return (t == error_mark_node
	  || (t && TREE_TYPE (t) == error_mark_node)
	  || (t && TREE_CODE (t) == NOP_EXPR
	      && TREE_OPERAND (t, 0) == error_mark_node));
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
  tree actual_callee = callable;
  tree saved_args = NULL_TREE;

  tree arg_list = NULL_TREE;

  if (POINTER_TYPE_P (ctype))
    ctype = TREE_TYPE (ctype);
  else
    actual_callee = build_address (callable);

  gcc_assert (function_type_p (ctype));
  gcc_assert (tf != NULL);
  gcc_assert (tf->ty == Tfunction);

  // Evaluate the callee before calling it.
  if (TREE_SIDE_EFFECTS (actual_callee))
    {
      actual_callee = maybe_make_temp (actual_callee);
      saved_args = actual_callee;
    }

  bool is_d_vararg = tf->varargs == 1 && tf->linkage == LINKd;

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
	  return error_mark (tf);
	}

      // Probably an internal error
      gcc_unreachable();
    }
  /* If this is a delegate call or a nested function being called as
     a delegate, the object should not be NULL. */
  if (object != NULL_TREE)
    arg_list = build_tree_list (NULL_TREE, object);

  // tf->parameters can be NULL for genCfunc decls
  Parameters *formal_args = tf->parameters;
  size_t n_formal_args = formal_args ? (int) Parameter::dim (formal_args) : 0;
  size_t n_actual_args = arguments ? arguments->dim : 0;
  size_t fi = 0;

  // assumes arguments->dim <= formal_args->dim if (!this->varargs)
  for (size_t ai = 0; ai < n_actual_args; ++ai)
    {
      tree arg_tree;
      Expression *arg_exp = (*arguments)[ai];

      if (ai == 0 && is_d_vararg)
	{
	  // The hidden _arguments parameter
	  arg_tree = arg_exp->toElem (irs);
	}
      else if (fi < n_formal_args)
	{
	  // Actual arguments for declared formal arguments
	  Parameter *formal_arg = Parameter::getNth (formal_args, fi);
	  arg_tree = convert_for_argument (arg_exp->toElem (irs),
					   arg_exp, formal_arg);
	  ++fi;
	}
      else
	{
	  if (flag_split_darrays && arg_exp->type->toBasetype()->ty == Tarray)
	    {
	      tree da_exp = maybe_make_temp (arg_exp->toElem (irs));
	      arg_list = chainon (arg_list, build_tree_list (0, d_array_length (da_exp)));
	      arg_list = chainon (arg_list, build_tree_list (0, d_array_ptr (da_exp)));
	      continue;
	    }
	  else
	    {
	      arg_tree = arg_exp->toElem (irs);
	      /* Not all targets support passing unpromoted types, so
		 promote anyway. */
	      tree prom_type = lang_hooks.types.type_promotes_to (TREE_TYPE (arg_tree));
	      if (prom_type != TREE_TYPE (arg_tree))
		arg_tree = convert (prom_type, arg_tree);
	    }
	}
      /* Evaluate the argument before passing to the function.
	 Needed for left to right evaluation.  */
      if (tf->linkage == LINKd && TREE_SIDE_EFFECTS (arg_tree))
	{
	  arg_tree = maybe_make_temp (arg_tree);
	  saved_args = maybe_vcompound_expr (saved_args, arg_tree);
	}

      arg_list = chainon (arg_list, build_tree_list (0, arg_tree));
    }

  tree result = d_build_call_list (TREE_TYPE (ctype), actual_callee, arg_list);
  result = maybe_expand_builtin (result);

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
// Most are extern(C) - for those that are not, correct mangling must be ensured.
// List kept in ascii collating order to allow binary search

static const char *libcall_ids[LIBCALL_count] = {
    "_D9invariant12_d_invariantFC6ObjectZv",
    "_aaDelX", "_aaEqual",
    "_aaGetRvalueX", "_aaGetX",
    "_aaInX",
    "_adCmp2", "_adEq2",
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
    "_d_delarray", "_d_delarray_t", "_d_delclass",
    "_d_delinterface", "_d_delmemory",
    "_d_dynamic_cast", "_d_hidden_func", "_d_interface_cast",
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
get_libcall (LibCall libcall)
{
  FuncDeclaration *decl = libcall_decls[libcall];

  static Type *aatype = NULL;

  if (!decl)
    {
      Types targs;
      Type *treturn = Type::tvoid;
      bool varargs = false;

      // Build generic AA type void*[void*]
      if (aatype == NULL)
	aatype = new TypeAArray (Type::tvoidptr, Type::tvoidptr);

      switch (libcall)
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
	  targs.push (Type::typeinfoclass->type->constOf());
	  treturn = build_object_type();
	  break;

	case LIBCALL_NEWARRAYT:
	case LIBCALL_NEWARRAYIT:
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_NEWARRAYMTX:
	case LIBCALL_NEWARRAYMITX:
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_NEWITEMT:
	case LIBCALL_NEWITEMIT:
	  targs.push (Type::dtypeinfo->type->constOf());
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
	  targs.push (Type::dtypeinfo->type->constOf());
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
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoid->arrayOf()->pointerTo());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_DYNAMIC_CAST:
	case LIBCALL_INTERFACE_CAST:
	  targs.push (build_object_type());
	  targs.push (Type::typeinfoclass->type);
	  treturn = build_object_type();
	  break;

	case LIBCALL_ADEQ2:
	case LIBCALL_ADCMP2:
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::dtypeinfo->type->constOf());
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAEQUAL:
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (aatype);
	  targs.push (aatype);
	  treturn = Type::tint32;
	  break;

	case LIBCALL_AAINX:
	  targs.push (aatype);
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tvoidptr);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_AAGETX:
	  targs.push (aatype->pointerTo());
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoidptr);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_AAGETRVALUEX:
	  targs.push (aatype);
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  targs.push (Type::tvoidptr);
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_AADELX:
	  targs.push (aatype);
	  targs.push (Type::dtypeinfo->type->constOf());
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
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tint8->arrayOf());
	  targs.push (Type::tint8->arrayOf());
	  treturn = Type::tint8->arrayOf();
	  break;

	case LIBCALL_ARRAYCATNT:
	  targs.push (Type::dtypeinfo->type->constOf());
	  // Currently 'uint', even if 64-bit
	  targs.push (Type::tuns32);
	  varargs = true;
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYAPPENDT:
	  targs.push (Type::dtypeinfo->type); //->constOf());
	  targs.push (Type::tint8->arrayOf()->pointerTo());
	  targs.push (Type::tint8->arrayOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYAPPENDCTX:
	  targs.push (Type::dtypeinfo->type->constOf());
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
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tvoid->arrayOf());
	  treturn = Type::tvoid->arrayOf();
	  break;

	case LIBCALL_ARRAYSETASSIGN:
	case LIBCALL_ARRAYSETCTOR:
	  targs.push (Type::tvoidptr);
	  targs.push (Type::tvoidptr);
	  targs.push (Type::tsize_t);
	  targs.push (Type::dtypeinfo->type->constOf());
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_THROW:
	case LIBCALL_INVARIANT:
	  targs.push (build_object_type());
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
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tvoid->arrayOf());
	  targs.push (Type::tvoid->arrayOf());
	  treturn = Type::tvoidptr;
	  break;

	case LIBCALL_ARRAYLITERALTX:
	  targs.push (Type::dtypeinfo->type->constOf());
	  targs.push (Type::tsize_t);
	  treturn = Type::tvoidptr;
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

      // Add parameter types.
      Parameters *args = new Parameters;
      args->setDim (targs.dim);
      for (size_t i = 0; i < targs.dim; i++)
	(*args)[i] = new Parameter (0, targs[i], NULL, NULL);

      // Build extern(C) function.
      decl = FuncDeclaration::genCfunc (args, treturn, libcall_ids[libcall]);

      TypeFunction *tf = (TypeFunction *) decl->type;
      tf->varargs = varargs ? 1 : 0;
      libcall_decls[libcall] = decl;

      // These functions do not return except through catching a thrown exception.
      if (libcall == LIBCALL_ASSERT || libcall == LIBCALL_ASSERT_MSG
	  || libcall == LIBCALL_UNITTEST || libcall == LIBCALL_UNITTEST_MSG
	  || libcall == LIBCALL_ARRAY_BOUNDS || libcall == LIBCALL_SWITCH_ERROR)
	TREE_THIS_VOLATILE (decl->toSymbol()->Stree) = 1;
    }

  return decl;
}

// Build call to LIBCALL. N_ARGS is the number of call arguments which are
// specified in as a tree array ARGS.  The caller can force the return type
// of the call to FORCE_TYPE if the library call returns a generic value.

// This does not perform conversions on the arguments.  This allows arbitrary data
// to be passed through varargs without going through the usual conversions.

tree
build_libcall (LibCall libcall, unsigned n_args, tree *args, tree force_type)
{
  FuncDeclaration *lib_decl = get_libcall (libcall);
  Type *type = lib_decl->type->nextOf();
  tree callee = build_address (lib_decl->toSymbol()->Stree);
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

// If CALL_EXP is a BUILT_IN_FRONTEND, expand and return inlined
// compiler generated instructions. Most map onto GCC builtins,
// others require a little extra work around them.

tree
maybe_expand_builtin (tree call_exp)
{
  // More code duplication from C
  CallExpr ce (call_exp);
  tree callee = ce.callee();
  tree op1 = NULL_TREE, op2 = NULL_TREE;
  tree exp = NULL_TREE, val;
  tree_code code;

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
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_CTZL), 1, op1);

	case INTRINSIC_BSR:
	  /* bsr becomes 31-(clz), but parameter passed to bsf may not be a 32bit type!!
	     %% TODO: The return value is supposed to be undefined if op1 is zero. */
	  op1 = ce.nextArg();
	  type = TREE_TYPE (op1);

	  op2 = build_integer_cst (TREE_INT_CST_LOW (TYPE_SIZE (type)) - 1, type);
	  exp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_CLZL), 1, op1);

	  // Handle int -> long conversions.
	  if (TREE_TYPE (exp) != type)
	    exp = fold_convert (type, exp);

	  return fold_build2 (MINUS_EXPR, type, op2, exp);

	case INTRINSIC_BTC:
	case INTRINSIC_BTR:
	case INTRINSIC_BTS:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  type = TREE_TYPE (TREE_TYPE (op1));

	  exp = build_integer_cst (TREE_INT_CST_LOW (TYPE_SIZE (type)), type);

	  // op1[op2 / exp]
	  op1 = build_array_index (op1, fold_build2 (TRUNC_DIV_EXPR, type, op2, exp));
	  op1 = indirect_ref (type, op1);

	  // mask = 1 << (op2 % exp);
	  op2 = fold_build2 (TRUNC_MOD_EXPR, type, op2, exp);
	  op2 = fold_build2 (LSHIFT_EXPR, type, size_one_node, op2);

	  // cond = op1[op2 / size] & mask;
	  exp = fold_build2 (BIT_AND_EXPR, type, op1, op2);

	  // cond ? -1 : 0;
	  exp = build3 (COND_EXPR, TREE_TYPE (call_exp), d_truthvalue_conversion (exp),
			integer_minus_one_node, integer_zero_node);

	  // Update the bit as needed.
	  code = (intrinsic == INTRINSIC_BTC) ? BIT_XOR_EXPR :
	    (intrinsic == INTRINSIC_BTR) ? BIT_AND_EXPR :
	    (intrinsic == INTRINSIC_BTS) ? BIT_IOR_EXPR : ERROR_MARK;
	  gcc_assert (code != ERROR_MARK);

	  // op1[op2 / size] op= mask
	  if (intrinsic == INTRINSIC_BTR)
	    op2 = build1 (BIT_NOT_EXPR, TREE_TYPE (op2), op2);

	  val = build_local_temp (TREE_TYPE (call_exp));
	  exp = vmodify_expr (val, exp);
	  op1 = vmodify_expr (op1, fold_build2 (code, TREE_TYPE (op1), op1, op2));
	  return compound_expr (exp, compound_expr (op1, val));

	case INTRINSIC_BSWAP:
	  /* Backend provides builtin bswap32.
	     Assumes first argument and return type is uint. */
	  op1 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_BSWAP32), 1, op1);

	case INTRINSIC_COS:
	  // Math intrinsics just map to their GCC equivalents.
	  op1 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_COSL), 1, op1);

	case INTRINSIC_SIN:
	  op1 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_SINL), 1, op1);

	case INTRINSIC_RNDTOL:
	  // %% not sure if llroundl stands as a good replacement
	  // for the expected behaviour of rndtol.
	  op1 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_LLROUNDL), 1, op1);

	case INTRINSIC_SQRT:
	  // Have float, double and real variants of sqrt.
	  op1 = ce.nextArg();
	  type = TREE_TYPE (op1);
	  // Could have used mathfn_built_in, but that only returns
	  // implicit built in decls.
	  if (TYPE_MAIN_VARIANT (type) == double_type_node)
	    exp = builtin_decl_explicit (BUILT_IN_SQRT);
	  else if (TYPE_MAIN_VARIANT (type) == float_type_node)
	    exp = builtin_decl_explicit (BUILT_IN_SQRTF);
	  else if (TYPE_MAIN_VARIANT (type) == long_double_type_node)
	    exp = builtin_decl_explicit (BUILT_IN_SQRTL);
	  // op1 is an integral type - use double precision.
	  else if (INTEGRAL_TYPE_P (TYPE_MAIN_VARIANT (type)))
	    {
	      op1 = convert (double_type_node, op1);
	      exp = builtin_decl_explicit (BUILT_IN_SQRT);
	    }

	  gcc_assert (exp);    // Should never trigger.
	  return d_build_call_nary (exp, 1, op1);

	case INTRINSIC_LDEXP:
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_LDEXPL), 2, op1, op2);

	case INTRINSIC_FABS:
	  op1 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_FABSL), 1, op1);

	case INTRINSIC_RINT:
	  op1 = ce.nextArg();
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_RINTL), 1, op1);

	case INTRINSIC_VA_ARG:
	case INTRINSIC_C_VA_ARG:
	  op1 = ce.nextArg();
	  STRIP_NOPS (op1);

	  if (TREE_CODE (op1) == ADDR_EXPR)
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

	  d_type = build_dtype (type);
	  if (flag_split_darrays
	      && (d_type && d_type->toBasetype()->ty == Tarray))
	    {
	      /* should create a temp var of type TYPE and move the binding
		 to outside this expression.  */
	      tree ltype = TREE_TYPE (TYPE_FIELDS (type));
	      tree ptype = TREE_TYPE (TREE_CHAIN (TYPE_FIELDS (type)));
	      tree lvar = create_temporary_var (ltype);
	      tree pvar = create_temporary_var (ptype);

	      op1 = stabilize_reference (op1);

	      tree e1 = vmodify_expr (lvar, build1 (VA_ARG_EXPR, ltype, op1));
	      tree e2 = vmodify_expr (pvar, build1 (VA_ARG_EXPR, ptype, op1));
	      tree val = d_array_value (type, lvar, pvar);

	      exp = compound_expr (compound_expr (e1, e2), val);
	      exp = bind_expr (lvar, bind_expr (pvar, exp));
	    }
	  else
	    {
	      tree type2 = lang_hooks.types.type_promotes_to (type);
	      exp = build1 (VA_ARG_EXPR, type2, op1);
	      // silently convert promoted type...
	      if (type != type2)
		exp = convert (type, exp);
	    }

	  if (intrinsic == INTRINSIC_VA_ARG)
	    exp = vmodify_expr (op2, exp);

	  return exp;

	case INTRINSIC_VA_START:
	  /* The va_list argument should already have its
	     address taken.  The second argument, however, is
	     inout and that needs to be fixed to prevent a warning.  */
	  op1 = ce.nextArg();
	  op2 = ce.nextArg();
	  type = TREE_TYPE (op1);

	  // could be casting... so need to check type too?
	  STRIP_NOPS (op1);
	  STRIP_NOPS (op2);
	  gcc_assert (TREE_CODE (op1) == ADDR_EXPR
		      && TREE_CODE (op2) == ADDR_EXPR);

	  op2 = TREE_OPERAND (op2, 0);
	  // assuming nobody tries to change the return type
	  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_VA_START), 2, op1, op2);

	default:
	  gcc_unreachable();
	}
    }

  return call_exp;
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

// Checks if DECL is an intrinsic or runtime library function that
// requires special processing.  Marks the generated trees for DECL
// as BUILT_IN_FRONTEND so can be identified later.

void
maybe_set_builtin_frontend (FuncDeclaration *decl)
{
  if (!decl->ident)
    return;

  LibCall libcall = (LibCall) binary (decl->ident->string, libcall_ids, LIBCALL_count);

  if (libcall != LIBCALL_NONE)
    {
      // It's a runtime library function, add to libcall_decls.
      if (libcall_decls[libcall] == decl)
	return;

      // This should have been done either by the front-end or get_libcall.
      TypeFunction *tf = (TypeFunction *) decl->type;
      gcc_assert (tf->parameters != NULL);

      libcall_decls[libcall] = decl;
    }
  else
    {
      // Check if it's a front-end builtin.
      static const char FeZe [] = "FNaNbNfeZe";		    // @safe pure nothrow real function(real)
      static const char FeZe2[] = "FNaNbNeeZe";		    // @trusted pure nothrow real function(real)
      static const char FuintZint[] = "FNaNbNfkZi";	    // @safe pure nothrow int function(uint)
      static const char FuintZuint[] = "FNaNbNfkZk";	    // @safe pure nothrow uint function(uint)
      static const char FulongZint[] = "FNaNbNfmZi";	    // @safe pure nothrow int function(uint)
      static const char FrealZlong [] = "FNaNbNfeZl";	    // @safe pure nothrow long function(real)
      static const char FlongplongZint [] = "FNaNbPmmZi";   // pure nothrow int function(long*, long)
      static const char FintpintZint [] = "FNaNbPkkZi";	    // pure nothrow int function(int*, int)
      static const char FrealintZint [] = "FNaNbNfeiZe";    // @safe pure nothrow real function(real, int)

      Dsymbol *dsym = decl->toParent();
      TypeFunction *ftype = (TypeFunction *) (decl->tintro ? decl->tintro : decl->type);
      Module *mod;

      if (dsym == NULL)
	return;

      mod = dsym->getModule();

      if (is_intrinsic_module_p (mod))
	{
	  // Matches order of Intrinsic enum
	  static const char *intrinsic_names[] = {
	      "bsf", "bsr", "bswap",
	      "btc", "btr", "bts",
	  };
	  const size_t sz = sizeof (intrinsic_names) / sizeof (char *);
	  int i = binary (decl->ident->string, intrinsic_names, sz);

	  if (i == -1)
	    return;

	  switch (i)
	    {
	    case INTRINSIC_BSF:
	    case INTRINSIC_BSR:
	      if (!(strcmp (ftype->deco, FuintZint) == 0 || strcmp (ftype->deco, FulongZint) == 0))
		return;
	      break;

	    case INTRINSIC_BSWAP:
	      if (!(strcmp (ftype->deco, FuintZuint) == 0))
		return;
	      break;

	    case INTRINSIC_BTC:
	    case INTRINSIC_BTR:
	    case INTRINSIC_BTS:
	      if (!(strcmp (ftype->deco, FlongplongZint) == 0 || strcmp (ftype->deco, FintpintZint) == 0))
		return;
	      break;
	    }

	  // Make sure 'i' is within the range we require.
	  gcc_assert (i >= INTRINSIC_BSF && i <= INTRINSIC_BTS);
	  tree t = decl->toSymbol()->Stree;

	  DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	  DECL_FUNCTION_CODE (t) = (built_in_function) i;
	}
      else if (is_math_module_p (mod))
	{
	  // Matches order of Intrinsic enum
	  static const char *math_names[] = {
	      "cos", "fabs", "ldexp",
	      "rint", "rndtol", "sin", "sqrt",
	  };
	  const size_t sz = sizeof (math_names) / sizeof (char *);
	  int i = binary (decl->ident->string, math_names, sz);

	  if (i == -1)
	    return;

	  // Adjust 'i' for this range of enums
	  i += INTRINSIC_COS;
	  gcc_assert (i >= INTRINSIC_COS && i <= INTRINSIC_SQRT);

	  switch (i)
	    {
	    case INTRINSIC_COS:
	    case INTRINSIC_FABS:
	    case INTRINSIC_RINT:
	    case INTRINSIC_SIN:
	      if (!(strcmp (ftype->deco, FeZe) == 0 || strcmp (ftype->deco, FeZe2) == 0))
		return;
	      break;

	    case INTRINSIC_LDEXP:
	      if (!(strcmp (ftype->deco, FrealintZint) == 0))
		return;
	      break;

	    case INTRINSIC_RNDTOL:
	      if (!(strcmp (ftype->deco, FrealZlong) == 0))
		return;
	      break;

	    case INTRINSIC_SQRT:
	      if (!(strcmp (ftype->deco, "FNaNbNfdZd") == 0
		    || strcmp (ftype->deco, "FNaNbNffZf") == 0
		    || strcmp (ftype->deco, FeZe) == 0
		    || strcmp (ftype->deco, FeZe2) == 0))
		return;
	      break;
	    }

	  tree t = decl->toSymbol()->Stree;

	  // rndtol returns a long type, sqrt any float type,
	  // every other math builtin returns a real type.
	  Type *tf = decl->type->nextOf();
	  if ((i == INTRINSIC_RNDTOL && tf->ty == Tint64)
	      || (i == INTRINSIC_SQRT && tf->isreal())
	      || (i != INTRINSIC_RNDTOL && tf->ty == Tfloat80))
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) i;
	    }
	}
      else
	{
	  TemplateInstance *ti = dsym->isTemplateInstance();

	  if (ti == NULL)
	    return;

	  tree t = decl->toSymbol()->Stree;

	  if (is_builtin_va_arg_p (ti->tempdecl, false))
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) INTRINSIC_VA_ARG;
	    }
	  else if (is_builtin_va_arg_p (ti->tempdecl, true))
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) INTRINSIC_C_VA_ARG;
	    }
	  else if (is_builtin_va_start_p (ti->tempdecl))
	    {
	      DECL_BUILT_IN_CLASS (t) = BUILT_IN_FRONTEND;
	      DECL_FUNCTION_CODE (t) = (built_in_function) INTRINSIC_VA_START;
	    }
	}
    }
}

// Build and return D's internal exception Object.
// Different from the generic exception pointer.

tree
build_exception_object (void)
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
  FuncDeclaration *nested_func = sym->isFuncDeclaration();
  FuncDeclaration *outer_func = NULL;

  if (nested_func != NULL)
    {
      // Check that the nested function is properly defined.
      if (!nested_func->fbody)
	{
	  // Should instead error on line that references nested_func
	  nested_func->error ("nested function missing body");
	  return d_null_pointer;
	}

      outer_func = nested_func->toParent2()->isFuncDeclaration();
      gcc_assert (outer_func != NULL);

      if (func != outer_func)
	{
	  // If no frame pointer for this function
	  if (!func->vthis)
	    {
	      sym->error ("is a nested function and cannot be accessed from %s", func->toChars());
	      return d_null_pointer;
	    }

	  Dsymbol *this_func = func;

	  // Make sure we can get the frame pointer to the outer function,
	  // else we'll ICE later in tree-ssa.
	  while (nested_func != this_func)
	    {
	      FuncDeclaration *fd;
	      ClassDeclaration *cd;
	      StructDeclaration *sd;

	      // Special case for __ensure and __require.
	      if (nested_func->ident == Id::ensure || nested_func->ident == Id::require)
		{
		  outer_func = func;
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
		  func->error ("cannot get frame pointer to %s", sym->toChars());
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

      while (sym && !sym->isFuncDeclaration())
	sym = sym->toParent2();

      outer_func = (FuncDeclaration *) sym;

      /* Make sure we can access the frame of outer_func.  */
      if (outer_func != func)
	{
	  nested_func = func;
	  while (nested_func && nested_func != outer_func)
	    {
	      Dsymbol *outer = nested_func->toParent2();

	      if (!nested_func->isNested())
		{
		  if (!nested_func->isMember2())
		    goto cannot_access_frame;
		}

	      while (outer)
		{
		  if (outer->isFuncDeclaration())
		    break;

		  outer = outer->toParent2();
		}

	      nested_func = (FuncDeclaration *) outer;
	    }

	  if (!nested_func)
	    {
	    cannot_access_frame:
	      error ("cannot access frame of function '%s' from '%s'",
		     outer_func->toChars(), func->toChars());
	      return d_null_pointer;
	    }
	}
    }

  if (!outer_func)
    outer_func = nested_func->toParent2()->isFuncDeclaration();

  gcc_assert (outer_func != NULL);

  FuncFrameInfo *ffo = get_frameinfo (outer_func);
  if (ffo->creates_frame || ffo->static_chain)
    return get_framedecl (func, outer_func);

  return d_null_pointer;
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
find_this_tree (FuncDeclaration *func, ClassDeclaration *ocd)
{
  while (func)
    {
      AggregateDeclaration *ad = func->isThis();
      ClassDeclaration *cd = ad ? ad->isClassDeclaration() : NULL;

      if (cd != NULL)
	{
	  if (ocd == cd)
	    return get_decl_tree (func->vthis, func);
	  else if (ocd->isBaseOf (cd, NULL))
	    return convert_expr (get_decl_tree (func->vthis, func), cd->type, ocd->type);

	  func = d_nested_class (cd);
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

// Retrieve the outer class/struct 'this' value of DECL from the function FD
// where E is the expression requiring 'this'.

tree
build_vthis (Dsymbol *decl, FuncDeclaration *fd, Expression *e)
{
  ClassDeclaration *cd = decl->isClassDeclaration();
  StructDeclaration *sd = decl->isStructDeclaration();

  tree vthis_value = d_null_pointer;

  if (cd)
    {
      Dsymbol *outer = cd->toParent2();
      ClassDeclaration *cdo = outer->isClassDeclaration();
      FuncDeclaration *fdo = outer->isFuncDeclaration();

      if (cdo)
	{
	  vthis_value = find_this_tree (fd, cdo);
	  if (vthis_value == NULL_TREE)
	    e->error ("outer class %s 'this' needed to 'new' nested class %s",
		      cdo->toChars(), cd->toChars());
	}
      else if (fdo)
	{
	  // If a class nested in a function has no methods and there
	  // are no other nested functions, any static chain created
	  // here will never be translated.  Use a null pointer for the
	  // link in this case.
	  FuncFrameInfo *ffo = get_frameinfo (fdo);
	  if (ffo->creates_frame || ffo->static_chain
	      || fdo->hasNestedFrameRefs())
	    vthis_value = get_frame_for_symbol (fd, cd);
	  else if (fdo->vthis && fdo->vthis->type != Type::tvoidptr)
	    vthis_value = get_decl_tree (fdo->vthis, fd);
	  else
	    vthis_value = d_null_pointer;
	}
      else
	gcc_unreachable();
    }
  else if (sd)
    {
      Dsymbol *outer = sd->toParent2();
      ClassDeclaration *cdo = outer->isClassDeclaration();
      FuncDeclaration *fdo = outer->isFuncDeclaration();

      if (cdo)
	{
	  vthis_value = find_this_tree (fd, cdo);
	  if (vthis_value == NULL_TREE)
	    e->error ("outer class %s 'this' needed to create nested struct %s",
		      cdo->toChars(), sd->toChars());
	}
      else if (fdo)
	{
	  FuncFrameInfo *ffo = get_frameinfo (fdo);
	  if (ffo->creates_frame || ffo->static_chain
	      || fdo->hasNestedFrameRefs())
	    vthis_value = get_frame_for_symbol (fd, sd);
	  else
	    vthis_value = d_null_pointer;
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

// Return the frame of FD.  This could be a static chain or a closure
// passed via the hidden 'this' pointer.

FuncFrameInfo *
get_frameinfo (FuncDeclaration *fd)
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
	      ffi->frame_rec = copy_node (ffo->frame_rec);
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

      if (get_frameinfo (fd)->creates_frame)
	{
	  // like compon (indirect, field0) parent frame link is the first field;
	  result = indirect_ref (ptr_type_node, result);
	}

      if (fd->isNested())
	fd = fd->toParent2()->isFuncDeclaration();
      /* get_framedecl is only used to get the pointer to a function's frame
	 (not a class instances).  With the current implementation, the link
	 the frame/closure record always points to the outer function's frame even
	 if there are intervening nested classes or structs.
	 So, we can just skip over those... */
      else if ((ad = fd->isThis()) && (cd = ad->isClassDeclaration()))
	fd = d_nested_class (cd);
      else if ((ad = fd->isThis()) && (sd = ad->isStructDeclaration()))
	fd = d_nested_struct (sd);
      else
	break;
    }

  if (fd == outer)
    {
      tree frame_rec = get_frameinfo (outer)->frame_rec;

      if (frame_rec != NULL_TREE)
	{
	  result = build_nop (build_pointer_type (frame_rec), result);
	  return result;
	}
      else
	{
	  inner->error ("forward reference to frame of %s", outer->toChars());
	  return d_null_pointer;
	}
    }
  else
    {
      inner->error ("cannot access frame of %s", outer->toChars());
      return d_null_pointer;
    }
}

// Special case: If a function returns a nested class with functions
// but there are no "closure variables" the frontend (needsClosure) 
// returns false even though the nested class _is_ returned from the
// function. (See case 4 in needsClosure)
// A closure is strictly speaking not necessary, but we also can not
// use a static function chain for functions in the nested class as
// they can be called from outside. GCC's nested functions can't deal
// with those kind of functions. We have to detect them manually here
// and make sure we neither construct a static chain nor a closure.

static bool
is_degenerate_closure (FuncDeclaration *f)
{
  if (!f->needsClosure() && f->closureVars.dim == 0)
  {
    Type *tret = ((TypeFunction *) f->type)->next;
    gcc_assert(tret);
    tret = tret->toBasetype();
    if (tret->ty == Tclass || tret->ty == Tstruct)
    { 
      Dsymbol *st = tret->toDsymbol(NULL);
      for (Dsymbol *s = st->parent; s; s = s->parent)
      {
	if (s == f)
	  return true;
      }
    }
  }
  return false;
}

// Return true if function F needs to have the static chain passed to it.
// This only applies to nested function handling provided by the GDC
// front end (not D closures).

bool
needs_static_chain (FuncDeclaration *f)
{
  Dsymbol *s;
  FuncDeclaration *pf = NULL;
  TemplateInstance *ti = NULL;

  if (f->isNested())
    {
      s = f->toParent();
      ti = s->isTemplateInstance();
      if (ti && ti->enclosing == NULL && ti->parent->isModule())
	return false;

      pf = f->toParent2()->isFuncDeclaration();
      if (pf && !get_frameinfo (pf)->is_closure)
	return true;
    }

  if (f->isStatic())
    return false;

  s = f->toParent2();

  while (s)
    {
      AggregateDeclaration *ad = s->isAggregateDeclaration();
      if (!ad || !ad->isNested())
	break;

      if (!s->isTemplateInstance())
	break;

      s = s->toParent2();
      if ((pf = s->isFuncDeclaration())
	  && !get_frameinfo (pf)->is_closure
	  && !is_degenerate_closure (pf))
	return true;
    }

  return false;
}


// Construct a WrappedExp, whose components are an EXP_NODE, which contains
// a list of instructions in GCC to be passed through.

WrappedExp::WrappedExp (Loc loc, TOK op, tree exp_node, Type *type)
    : Expression (loc, op, sizeof (WrappedExp))
{
  this->exp_node = exp_node;
  this->type = type;
}

// Write C-style representation of WrappedExp to BUF.

void
WrappedExp::toCBuffer (OutBuffer *buf, HdrGenState *hgs ATTRIBUTE_UNUSED)
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
      VarDeclaration *var_decl = (*fields)[i];
      gcc_assert (var_decl && var_decl->isField());

      tree ident = var_decl->ident ? get_identifier (var_decl->ident->string) : NULL_TREE;
      tree field_decl = build_decl (UNKNOWN_LOCATION, FIELD_DECL, ident,
				    declaration_type (var_decl));
      set_decl_location (field_decl, var_decl);
      var_decl->csym = new Symbol;
      var_decl->csym->Stree = field_decl;

      DECL_CONTEXT (field_decl) = this->aggType_;
      DECL_FCONTEXT (field_decl) = fcontext;
      DECL_FIELD_OFFSET (field_decl) = size_int (var_decl->offset);
      DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;

      DECL_ARTIFICIAL (field_decl) = DECL_IGNORED_P (field_decl) = inherited;
      SET_DECL_OFFSET_ALIGN (field_decl, TYPE_ALIGN (TREE_TYPE (field_decl)));

      layout_decl (field_decl, 0);

      TREE_THIS_VOLATILE (field_decl) = TYPE_VOLATILE (TREE_TYPE (field_decl));

      if (var_decl->size (var_decl->loc))
	{
	  gcc_assert (DECL_MODE (field_decl) != VOIDmode);
	  gcc_assert (DECL_SIZE (field_decl) != NULL_TREE);
	}

      TYPE_FIELDS(this->aggType_) = chainon (TYPE_FIELDS (this->aggType_), field_decl);
    }
}

// Write out all interfaces BASES for a class.

void
AggLayout::doInterfaces (BaseClasses *bases)
{
  for (size_t i = 0; i < bases->dim; i++)
    {
      BaseClass *bc = (*bases)[i];
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
  Loc l (this->aggDecl_->getModule(), 1);

  DECL_CONTEXT (field_decl) = this->aggType_;
  SET_DECL_OFFSET_ALIGN (field_decl, TYPE_ALIGN (TREE_TYPE (field_decl)));
  DECL_FIELD_OFFSET (field_decl) = size_int (offset);
  DECL_FIELD_BIT_OFFSET (field_decl) = bitsize_zero_node;

  // Must set this or we crash with DWARF debugging.
  set_decl_location (field_decl, l);

  TREE_THIS_VOLATILE (field_decl) = TYPE_VOLATILE (TREE_TYPE (field_decl));

  layout_decl (field_decl, 0);
  TYPE_FIELDS(this->aggType_) = chainon (TYPE_FIELDS (this->aggType_), field_decl);
}

// Wrap-up and compute finalised aggregate type.  ATTRS are
// if any GCC attributes were applied to the type declaration.

void
AggLayout::finish (Expressions *attrs)
{
  unsigned structsize = this->aggDecl_->structsize;
  unsigned alignsize = this->aggDecl_->alignsize;

  TYPE_SIZE (this->aggType_) = NULL_TREE;

  if (attrs)
    decl_attributes (&this->aggType_, build_attributes (attrs),
		     ATTR_FLAG_TYPE_IN_PLACE);

  TYPE_SIZE (this->aggType_) = bitsize_int (structsize * BITS_PER_UNIT);
  TYPE_SIZE_UNIT (this->aggType_) = size_int (structsize);
  TYPE_ALIGN (this->aggType_) = alignsize * BITS_PER_UNIT;
  TYPE_PACKED (this->aggType_) = (alignsize == 1);

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

