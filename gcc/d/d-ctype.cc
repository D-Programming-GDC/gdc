// d-ctype.cc -- D frontend for GCC.
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

#include "d-system.h"
#include "d-lang.h"
#include "d-codegen.h"

#include "enum.h"
#include "dfrontend/target.h"

type *
Type::toCtype (void)
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
	      ctype = intQI_type_node;
	      break;

	    case Tuns8:
	      ctype = unsigned_intQI_type_node;
	      break;

	    case Tint16:
	      ctype = intHI_type_node;
	      break;

	    case Tuns16:
	      ctype = unsigned_intHI_type_node;
	      break;

	    case Tint32:
	      ctype = intSI_type_node;
	      break;

	    case Tuns32:
	      ctype = unsigned_intSI_type_node;
	      break;

	    case Tint64:
	      ctype = intDI_type_node;
	      break;

	    case Tint128:
	      ctype = intTI_type_node;
	      break;

	    case Tuns128:
	      ctype = unsigned_intTI_type_node;
	      break;

	    case Tuns64:
	      ctype = unsigned_intDI_type_node;
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
	      ctype = d_boolean_type_node;
	      break;

	    case Tchar:
	      ctype = d_char_type_node;
	      break;

	    case Twchar:
	      ctype = d_wchar_type_node;
	      break;

	    case Tdchar:
	      ctype = d_dchar_type_node;
	      break;

	    case Timaginary32:
	      ctype = d_ifloat_type_node;
	      break;

	    case Timaginary64:
	      ctype = d_idouble_type_node;
	      break;

	    case Timaginary80:
	      ctype = d_ireal_type_node;
	      break;


	    case Terror:
	      return error_mark_node;

	      /* We can get Tident with forward references.  There seems to
		be a legitame case (dstress:debug_info_03).  I have not seen this
		happen for an error, so maybe it's okay...

		Update:
		Seems to be fixed in the frontend, so changed to an error
		to catch potential bugs.
	       */
	    case Tident:
	    case Ttypeof:
	      ::error ("forward reference of %s\n", this->toChars());
	      return error_mark_node;

	      /* Valid case for Ttuple is in CommaExp::toElem, in instances when
		a tuple has been expanded as a large chain of comma expressions.
		*/
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
Type::toCParamtype (void)
{
  return toCtype();
}

type *
TypeTypedef::toCtype (void)
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
	  tree basetype = sym->basetype->toCtype();
	  const char *name = toChars();

	  tree ident = get_identifier (name);
	  tree type_node = build_variant_type_copy (basetype);
	  tree type_decl = build_decl (UNKNOWN_LOCATION, TYPE_DECL, ident, type_node);
	  TYPE_NAME (type_node) = type_decl;

	  if (sym->userAttributes)
	    decl_attributes (&type_node, build_attributes (sym->userAttributes), 0);

	  ctype = type_node;
	}
    }

  return ctype;
}

type *
TypeTypedef::toCParamtype (void)
{
  return toCtype();
}

type *
TypeEnum::toCtype (void)
{
  if (!ctype)
    {
      /* Enums in D2 can have a base type that is not necessarily integral.
	 So don't bother trying to make an ENUMERAL_TYPE using them.  */
      if (!sym->memtype->isintegral() || sym->memtype->ty == Tbool)
	{
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

	  TYPE_PRECISION (ctype) = size (0) * 8;
	  TYPE_SIZE (ctype) = 0;
	  TYPE_MAIN_VARIANT (ctype) = TYPE_MAIN_VARIANT (cmemtype);

	  if (sym->userAttributes)
	    decl_attributes (&ctype, build_attributes (sym->userAttributes),
			     ATTR_FLAG_TYPE_IN_PLACE);

	  TYPE_MIN_VALUE (ctype) = TYPE_MIN_VALUE (cmemtype);
	  TYPE_MAX_VALUE (ctype) = TYPE_MAX_VALUE (cmemtype);
	  layout_type (ctype);
	  TYPE_UNSIGNED (ctype) = TYPE_UNSIGNED (cmemtype);

	  // Move this to toDebug() ?
	  tree enum_values = NULL_TREE;
	  if (sym->members)
	    {
	      for (size_t i = 0; i < sym->members->dim; i++)
		{
		  EnumMember *member = (sym->members->tdata()[i])->isEnumMember();
		  // Templated functions can seep through to the backend - just ignore for now.
		  if (member == NULL)
		    continue;

		  char *ident = NULL;
		  if (sym->ident)
		    ident = concat (sym->ident->string, ".",
				    member->ident->string, NULL);

		  tree tident = get_identifier (ident ? ident : member->ident->string);
		  tree tvalue = build_integer_cst (member->value->toInteger(), ctype);
		  enum_values = chainon (enum_values, build_tree_list (tident, tvalue));

		  if (sym->ident)
		    free (ident);
		}
	    }
	  TYPE_VALUES (ctype) = enum_values;
	  build_type_decl (ctype, sym);
	}
    }

  return ctype;
}

type *
TypeStruct::toCtype (void)
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

	  AggLayout agg_layout (sym, ctype);
	  agg_layout.go();
	  agg_layout.finish (sym->userAttributes);

	  build_type_decl (ctype, sym);
	  TYPE_CONTEXT (ctype) = d_decl_context (sym);
	}
    }

  return ctype;
}

Symbol *
TypeClass::toSymbol (void)
{
  return sym->toSymbol();
}

type *
TypeFunction::toCtype (void)
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

	  ret_type = next ? next->toCtype() : void_type_node;

	  if (isref)
	    ret_type = build_reference_type (ret_type);

	  // Function type can be reference by parameters, etc.  Set ctype earlier?
	  ctype = build_function_type (ret_type, type_list);
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);

	  if (next->toBasetype()->ty == Tstruct)
	    {
	      // Non-POD structs must return in memory.
	      TypeStruct *ts = (TypeStruct *) next->toBasetype();
	      if (!ts->sym->isPOD())
		TREE_ADDRESSABLE (ctype) = 1;
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
	      fprintf (stderr, "linkage = %d\n", linkage);
	      gcc_unreachable();
	    }
	}
    }

  return ctype;
}

enum RET
TypeFunction::retStyle (void)
{
  /* Return by reference or pointer. */
  if (isref || next->ty == Tclass || next->ty == Tpointer)
    return RETregs;

  /* Need the ctype to determine this, but this is called from
     the front end before semantic processing is finished.  An
     accurate value is not currently needed anyway. */
  return RETstack;
}

type *
TypeVector::toCtype (void)
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
	    inner = Type::tint8->toCtype();

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
TypeSArray::toCtype (void)
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
TypeSArray::toCParamtype (void)
{
  return toCtype();
}

type *
TypeDArray::toCtype (void)
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
TypeAArray::toCtype (void)
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
	  layout_type (ctype);

	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);
	}
    }

  return ctype;
}

type *
TypePointer::toCtype (void)
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
TypeDelegate::toCtype (void)
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

	  ctype = build_two_field_type (objtype, build_pointer_type (funtype),
					this, "object", "func");
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);
	  d_keep (ctype);
	}
    }

  return ctype;
}

type *
TypeClass::toCtype (void)
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
	  tree rec_type;
	  Array base_class_decls;
	  bool inherited = sym->baseClass != 0;
	  tree obj_rec_type;
	  tree vfield;

	  /* Need to set ctype right away in case of self-references to
	     the type during this call. */
	  rec_type = make_node (RECORD_TYPE);
	  ctype = build_reference_type (rec_type);
	  d_keep (ctype); // because BINFO moved out to toDebug

	  obj_rec_type = TREE_TYPE (build_object_type()->toCtype());

	  // Note that this is set on the reference type, not the record type.
	  TYPE_LANG_SPECIFIC (ctype) = build_d_type_lang_specific (this);

	  AggLayout agg_layout (sym, rec_type);

	  // Most of this silly code is just to produce correct debugging information.

	  /* gdb apparently expects the vtable field to be named
	     "_vptr$...." (stabsread.c) Otherwise, the debugger gives
	     lots of annoying error messages.  C++ appends the class
	     name of the first base witht that field after the '$'. */
	  /* update: annoying messages might not appear anymore after making
	     other changes */
	  // Add the virtual table pointer
	  tree decl = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
				  get_identifier ("_vptr$"), d_vtbl_ptr_type_node);
	  agg_layout.addField (decl, 0); // %% target stuff..

	  if (inherited)
	    {
	      vfield = copy_node (decl);
	      DECL_ARTIFICIAL (decl) = DECL_IGNORED_P (decl) = 1;
	    }
	  else
	    {
	      vfield = decl;
	    }
	  DECL_VIRTUAL_P (vfield) = 1;
	  TYPE_VFIELD (rec_type) = vfield; // This only seems to affect debug info
	  TREE_ADDRESSABLE (rec_type) = 1;

	  if (!sym->isInterfaceDeclaration())
	    {
	      DECL_FCONTEXT (vfield) = obj_rec_type;

	      // Add the monitor
	      // %% target type
	      decl = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
				 get_identifier ("_monitor"), ptr_type_node);
	      DECL_FCONTEXT (decl) = obj_rec_type;
	      DECL_ARTIFICIAL (decl) = DECL_IGNORED_P (decl) = inherited;
	      agg_layout.addField (decl, Target::ptrsize);

	      // Add the fields of each base class
	      agg_layout.go();
	    }
	  else
	    {
	      ClassDeclaration *p = sym;
	      while (p->baseclasses->dim)
	        p = (p->baseclasses->tdata()[0])->base;

	      DECL_FCONTEXT (vfield) = TREE_TYPE (p->type->toCtype());
	    }

	  agg_layout.finish (sym->userAttributes);

	  build_type_decl (rec_type, sym);
	  TYPE_CONTEXT (rec_type) = d_decl_context (sym);
	}
    }

  return ctype;
}


// These are not used for code generation in glue.

Symbol *
Type::toSymbol (void)
{
  return NULL;
}

unsigned
Type::totym (void)
{
  return 0;
}

unsigned
TypeFunction::totym (void)
{
  return 0;
}

