// d-ctype.cc -- D frontend for GCC.
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
#include "enum.h"
#include "dfrontend/target.h"

type *
Type::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  switch (ty)
	    {
	    case Tvoid:
	      ctype = void_type_node;
	      break;

	    case Tint8:
	      ctype = byte_type_node;
	      break;

	    case Tuns8:
	      ctype = ubyte_type_node;
	      break;

	    case Tint16:
	      ctype = short_type_node;
	      break;

	    case Tuns16:
	      ctype = ushort_type_node;
	      break;

	    case Tint32:
	      ctype = int_type_node;
	      break;

	    case Tuns32:
	      ctype = uint_type_node;
	      break;

	    case Tint64:
	      ctype = long_type_node;
	      break;

	    case Tuns64:
	      ctype = ulong_type_node;
	      break;

	    case Tint128:
	      ctype = cent_type_node;
	      break;

	    case Tuns128:
	      ctype = ucent_type_node;
	      break;

	    case Tfloat32:
	      ctype = float_type_node;
	      break;

	    case Tfloat64:
	      ctype = double_type_node;
	      break;

	    case Tfloat80:
	      ctype = long_double_type_node;
	      break;

	    case Tcomplex32:
	      ctype = complex_float_type_node;
	      break;

	    case Tcomplex64:
	      ctype = complex_double_type_node;
	      break;

	    case Tcomplex80:
	      ctype = complex_long_double_type_node;
	      break;

	    case Tbool:
	      ctype = bool_type_node;
	      break;

	    case Tchar:
	      ctype = char8_type_node;
	      break;

	    case Twchar:
	      ctype = char16_type_node;
	      break;

	    case Tdchar:
	      ctype = char32_type_node;
	      break;

	    case Timaginary32:
	      ctype = ifloat_type_node;
	      break;

	    case Timaginary64:
	      ctype = idouble_type_node;
	      break;

	    case Timaginary80:
	      ctype = ireal_type_node;
	      break;

	    case Terror:
	      return d_unknown_type_node;

	    case Tident:
	    case Ttypeof:
	      ::error ("forward reference of %s\n", this->toChars());
	      return error_mark_node;

	      // Valid case for Ttuple is in CommaExp::toElem, in instances when
	      // a tuple has been expanded as a large chain of comma expressions.
	    case Ttuple:
	      ctype = void_type_node;
	      break;

	    case Tnull:
	      ctype = ptr_type_node;
	      break;

	    default:
	      ::error ("unexpected call to Type::toCtype() for %s\n", this->toChars());
	      gcc_unreachable();
	    }
	}
    }

  return ctype;
}

type *
TypeEnum::toCtype()
{
  if (!ctype)
    {
      if (!sym->memtype->isintegral() || sym->memtype->ty == Tbool)
	{
	  // Enums in D2 can have a base type that is not necessarily integral.
	  // So don't bother trying to make an ENUMERAL_TYPE using them.
	  ctype = sym->memtype->toCtype();
	}
      else
	{
	  tree cmemtype = sym->memtype->toCtype();

	  ctype = make_node (ENUMERAL_TYPE);
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);

	  if (flag_short_enums)
	    TYPE_PACKED (ctype) = 1;

	  TYPE_PRECISION (ctype) = size (sym->loc) * 8;
	  TYPE_SIZE (ctype) = 0;

	  if (sym->userAttribDecl)
	    {
	      Expressions *attrs = sym->userAttribDecl->getAttributes();
	      decl_attributes (&ctype, build_attributes (attrs),
			       ATTR_FLAG_TYPE_IN_PLACE);
	    }

	  TYPE_MIN_VALUE (ctype) = TYPE_MIN_VALUE (cmemtype);
	  TYPE_MAX_VALUE (ctype) = TYPE_MAX_VALUE (cmemtype);
	  layout_type (ctype);
	  TYPE_UNSIGNED (ctype) = TYPE_UNSIGNED (cmemtype);

	  tree enum_values = NULL_TREE;
	  if (sym->members)
	    {
	      for (size_t i = 0; i < sym->members->dim; i++)
		{
		  EnumMember *member = (*sym->members)[i]->isEnumMember();
		  // Templated functions can seep through to the backend - just ignore for now.
		  if (member == NULL)
		    continue;

		  tree ident = get_identifier (member->ident->string);
		  tree value = build_integer_cst (member->value->toInteger(), cmemtype);

		  // Build a identifier for the enumeration constant.
		  tree decl = build_decl (UNKNOWN_LOCATION, CONST_DECL, ident, cmemtype);
		  set_decl_location (decl, member->loc);
		  DECL_CONTEXT (decl) = ctype;
		  TREE_CONSTANT (decl) = 1;
		  TREE_READONLY (decl) = 1;
		  DECL_INITIAL (decl) = value;

		  // Add this enumeration constant to the list for this type.
		  enum_values = chainon (enum_values, build_tree_list (ident, decl));
		}
	    }

	  TYPE_VALUES (ctype) = enum_values;
	  build_type_decl (ctype, sym);
	}
    }

  return ctype;
}

type *
TypeStruct::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  // need to set this right away in case of self-references
	  ctype = make_node (sym->isUnionDeclaration() ? UNION_TYPE : RECORD_TYPE);
	  d_keep (ctype);
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);

	  /* Must set up the overall size, etc. before determining the
	     context or laying out fields as those types may make references
	     to this type. */
	  TYPE_SIZE (ctype) = bitsize_int (sym->structsize * BITS_PER_UNIT);
	  TYPE_SIZE_UNIT (ctype) = size_int (sym->structsize);
	  TYPE_ALIGN (ctype) = sym->alignsize * BITS_PER_UNIT;
	  TYPE_PACKED (ctype) = (sym->alignsize == 1);
	  compute_record_mode (ctype);

	  AggLayout al = AggLayout (sym, ctype);
	  layout_aggregate_type (&al, sym);
	  finish_aggregate_type (&al, sym->userAttribDecl);

	  build_type_decl (ctype, sym);
	  TYPE_CONTEXT (ctype) = d_decl_context (sym);
	}
    }

  return ctype;
}

type *
TypeFunction::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  tree type_list = NULL_TREE;
	  tree ret_type;

	  if (varargs == 1 && linkage == LINKd)
	    {
	      // hidden _arguments parameter
	      type_list = chainon (type_list, build_tree_list (0, Type::typeinfotypelist->type->toCtype()));
	    }

	  if (parameters)
	    {
	      size_t n_args = Parameter::dim (parameters);

	      for (size_t i = 0; i < n_args; i++)
		{
		  Parameter *arg = Parameter::getNth (parameters, i);
		  type_list = chainon (type_list, build_tree_list (0, type_passed_as (arg)));
		}
	    }

	  /* Last parm if void indicates fixed length list (as opposed to
	     printf style va_* list). */
	  if (varargs != 1)
	    type_list = chainon (type_list, void_list_node);

	  if (next != NULL)
	    {
	      ret_type = next->toCtype();
	      if (this->isref)
		ret_type = build_reference_type (ret_type);
	    }
	  else
	    ret_type = void_type_node;

	  // Function type could be referenced by parameters, so set ctype earlier?
	  ctype = build_function_type (ret_type, type_list);
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);

	  if (ret_type != void_type_node)
	    {
	      Type *tn = next->baseElemOf();
	      if (tn->ty == Tstruct)
		{
		  // Non-POD structs must return in memory.
		  TypeStruct *ts = (TypeStruct *) tn->toBasetype();
		  if (!ts->sym->isPOD())
		    TREE_ADDRESSABLE (ctype) = 1;
		}
	    }

	  switch (linkage)
	    {
	    case LINKpascal:
	    case LINKwindows:
	      if (!global.params.is64bit)
		ctype = insert_type_attribute (ctype, "stdcall");
	      break;

	    case LINKc:
	    case LINKcpp:
	    case LINKd:
	      break;

	    default:
	      fprintf (global.stdmsg, "linkage = %d\n", linkage);
	      gcc_unreachable();
	    }
	}
    }

  return ctype;
}

type *
TypeVector::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  int nunits = ((TypeSArray *) basetype)->dim->toUInteger();
	  tree inner = elementType()->toCtype();

	  if (inner == void_type_node)
	    inner = Type::tuns8->toCtype();

	  ctype = build_vector_type (inner, nunits);
	  layout_type (ctype);

	  /* Give a graceful error if the backend does not support the vector type
	     we are creating.  If backend has support for the inner type mode,
	     then it can safely emulate the vector.  */
	  if (!targetm.vector_mode_supported_p (TYPE_MODE (ctype))
	      && !targetm.scalar_mode_supported_p (TYPE_MODE (inner)))
	    ::error ("vector type %s is not supported on this architechture", toChars());
	}
    }

  return ctype;
}

type *
TypeSArray::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else if (dim->isConst() && dim->type->isintegral())
	{
	  uinteger_t size = dim->toUInteger();
	  if (next->toBasetype()->ty == Tvoid)
	    ctype = d_array_type (Type::tuns8, size);
	  else
	    ctype = d_array_type (next, size);
	}
      else
	{
	  ::error ("invalid expressions for static array dimension: %s", dim->toChars());
	  gcc_unreachable();
	}
    }

  return ctype;
}

type *
TypeDArray::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  tree lentype = Type::tsize_t->toCtype();
	  tree ptrtype = next->toCtype();

	  ctype = build_two_field_type (lentype, build_pointer_type (ptrtype),
					this, "length", "ptr");
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);
	}
    }

  return ctype;
}

type *
TypeAArray::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  /* Library functions expect a struct-of-pointer which could be passed
	     differently from a pointer. */
	  ctype = make_node (RECORD_TYPE);
	  tree ptr = build_decl (BUILTINS_LOCATION, FIELD_DECL,
				 get_identifier ("ptr"), ptr_type_node);
	  DECL_CONTEXT (ptr) = ctype;
	  TYPE_FIELDS (ctype) = ptr;
	  TYPE_NAME (ctype) = get_identifier (toChars());
	  TYPE_TRANSPARENT_AGGR (ctype) = 1;
	  layout_type (ctype);

	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);
	}
    }

  return ctype;
}

type *
TypePointer::toCtype()
{
  if (!ctype)
  {
    if (!isNaked())
      {
	ctype = castMod(0)->toCtype();
	ctype = insert_type_modifiers (ctype, mod);
      }
    else
      {
	ctype = build_pointer_type (next->toCtype());
      }
  }

  return ctype;
}

type *
TypeDelegate::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  gcc_assert (next->toBasetype()->ty == Tfunction);
	  tree nexttype = next->toCtype();
	  tree objtype = Type::tvoidptr->toCtype();
	  // Delegate function types are like method types, in that
	  // they pass around a hidden internal state.
	  tree funtype = build_method_type (void_type_node, nexttype);

	  TYPE_ATTRIBUTES (funtype) = TYPE_ATTRIBUTES (nexttype);
	  TYPE_LANG_SPECIFIC (funtype) = TYPE_LANG_SPECIFIC (nexttype);
	  TREE_ADDRESSABLE (funtype) = TREE_ADDRESSABLE (nexttype);

	  ctype = build_two_field_type (objtype, build_pointer_type (funtype),
					this, "object", "func");
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);
	}
    }

  return ctype;
}

type *
TypeClass::toCtype()
{
  if (!ctype)
    {
      if (!isNaked())
	{
	  ctype = castMod(0)->toCtype();
	  ctype = insert_type_modifiers (ctype, mod);
	}
      else
	{
	  // Need to set ctype right away in case of self-references to
	  // the type during this call.
	  tree basetype = make_node (RECORD_TYPE);
	  ctype = build_reference_type (basetype);
	  d_keep (ctype);

	  // Note that this is set on both the reference type and record type.
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  TYPE_LANG_SPECIFIC (basetype) = TYPE_LANG_SPECIFIC (ctype);

	  // Add the fields of each base class
	  AggLayout al = AggLayout (sym, basetype);
	  layout_aggregate_type (&al, sym);
	  finish_aggregate_type (&al, sym->userAttribDecl);

	  // Type is final, there are no derivations.
	  if (sym->storage_class & STCfinal)
	    TYPE_FINAL_P (basetype) = 1;

	  // Create BINFO even if debugging is off.  This is needed to keep
	  // references to inherited types.
	  if (!sym->isInterfaceDeclaration())
	    TYPE_BINFO (basetype) = build_class_binfo (NULL_TREE, sym);
	  else
	    {
	      unsigned offset = 0;
	      TYPE_BINFO (basetype) = build_interface_binfo (NULL_TREE, sym, offset);
	    }

	  // Same for virtual methods too.
	  for (size_t i = 0; i < sym->vtbl.dim; i++)
	    {
	      FuncDeclaration *fd = sym->vtbl[i]->isFuncDeclaration();
	      tree method = fd ? fd->toSymbol()->Stree : NULL_TREE;

	      if (method && DECL_CONTEXT (method) == basetype)
		{
		  DECL_CHAIN (method) = TYPE_METHODS (basetype);
		  TYPE_METHODS (basetype) = method;
		}
	    }

	  build_type_decl (basetype, sym);
	  TYPE_CONTEXT (basetype) = d_decl_context (sym);
	}
    }

  return ctype;
}

