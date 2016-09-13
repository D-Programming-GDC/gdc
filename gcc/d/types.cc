/* types.cc -- Lower D frontend types to GCC trees.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/attrib.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/enum.h"
#include "dfrontend/mtype.h"
#include "dfrontend/target.h"
#include "dfrontend/visitor.h"

#include "d-system.h"
#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"


/* Implements the visitor interface to build the GCC trees of all
   Type AST classes emitted from the D Front-end, where CTYPE holds
   the cached backend representation to be returned.  */

class TypeVisitor : public Visitor
{
public:
  TypeVisitor () {}

  /* This should be overridden by each type class.  */

  void visit (Type *)
  {
    gcc_unreachable ();
  }

  /* Type assigned to erroneous expressions or constructs that
     failed during the semantic stage.  */

  void visit (TypeError *t)
  {
    t->ctype = error_mark_node;
  }

  /* Type assigned to generic nullable types.  */

  void visit (TypeNull *t)
  {
    t->ctype = ptr_type_node;
  }


  /* Basic Data Types.  */

  void visit (TypeBasic *t)
  {
    /* [type/basic-data-types]

       void	no type.
       bool	8 bit boolean value.
       byte	8 bit signed value.
       ubyte	8 bit unsigned value.
       short	16 bit signed value.
       ushort	16 bit unsigned value.
       int	32 bit signed value.
       uint	32 bit unsigned value.
       long	64 bit signed value.
       ulong	64 bit unsigned value.
       cent	128 bit signed value.
       ucent	128 bit unsigned value.
       float	32 bit IEEE 754 floating point value.
       double	64 bit IEEE 754 floating point value.
       real	largest FP size implemented in hardware.
       ifloat	imaginary float.
       idouble	imaginary double.
       ireal	imaginary real.
       cfloat	complex float.
       cdouble	complex double.
       creal	complex real.
       char	UTF-8 code unit.
       wchar	UTF-16 code unit.
       dchar	UTF-32 code unit.  */

    switch (t->ty)
      {
      case Tvoid:
	t->ctype = void_type_node;
	break;

      case Tint8:
	t->ctype = byte_type_node;
	break;

      case Tuns8:
	t->ctype = ubyte_type_node;
	break;

      case Tint16:
	t->ctype = short_type_node;
	break;

      case Tuns16:
	t->ctype = ushort_type_node;
	break;

      case Tint32:
	t->ctype = int_type_node;
	break;

      case Tuns32:
	t->ctype = uint_type_node;
	break;

      case Tint64:
	t->ctype = long_type_node;
	break;

      case Tuns64:
	t->ctype = ulong_type_node;
	break;

      case Tint128:
	t->ctype = cent_type_node;
	break;

      case Tuns128:
	t->ctype = ucent_type_node;
	break;

      case Tfloat32:
	t->ctype = float_type_node;
	break;

      case Tfloat64:
	t->ctype = double_type_node;
	break;

      case Tfloat80:
	t->ctype = long_double_type_node;
	break;

      case Tcomplex32:
	t->ctype = complex_float_type_node;
	break;

      case Tcomplex64:
	t->ctype = complex_double_type_node;
	break;

      case Tcomplex80:
	t->ctype = complex_long_double_type_node;
	break;

      case Tbool:
	t->ctype = bool_type_node;
	break;

      case Tchar:
	t->ctype = char8_type_node;
	break;

      case Twchar:
	t->ctype = char16_type_node;
	break;

      case Tdchar:
	t->ctype = char32_type_node;
	break;

      case Timaginary32:
	t->ctype = ifloat_type_node;
	break;

      case Timaginary64:
	t->ctype = idouble_type_node;
	break;

      case Timaginary80:
	t->ctype = ireal_type_node;
	break;

      default:
	gcc_unreachable ();
      }
  }


  /* Derived Data Types.  */

  /* Build a simple pointer to data type, analogous to C pointers.  */

  void visit (TypePointer *t)
  {
    t->ctype = build_pointer_type (build_ctype (t->next));
  }

  /* Build a dynamic array type, consisting of a length and a pointer
     to the array data.  */

  void visit (TypeDArray *t)
  {
    /* In [abi/arrays], dynamic array layout is:
        .length	array dimension.
        .ptr	pointer to array data.  */

    t->ctype = build_two_field_type (build_ctype (Type::tsize_t),
				     build_pointer_type (build_ctype (t->next)),
				     t, "length", "ptr");
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);
    d_keep (t->ctype);
  }

  /* Build a static array type, distinguished from dynamic arrays by
     having a length fixed at compile time, analogous to C arrays.  */

  void visit (TypeSArray *t)
  {
    if (t->dim->isConst () && t->dim->type->isintegral ())
      {
	uinteger_t size = t->dim->toUInteger ();
	/* In [arrays/void-arrays], void arrays can also be static,
	   the length is specified in bytes.  */
	if (t->next->toBasetype ()->ty == Tvoid)
	  t->ctype = d_array_type (Type::tuns8, size);
	else
	  t->ctype = d_array_type (t->next, size);
      }
    else
      {
	::error ("invalid expression for static array dimension: %s",
		 t->dim->toChars ());
	gcc_unreachable ();
      }
  }

  /* Build a vector type, a fixed array of floating or integer types.  */

  void visit (TypeVector *t)
  {
    int nunits = ((TypeSArray *) t->basetype)->dim->toUInteger ();
    tree inner = build_ctype (t->elementType ());

    /* Same rationale as void static arrays.  */
    if (inner == void_type_node)
      inner = build_ctype (Type::tuns8);

    t->ctype = build_vector_type (inner, nunits);
    layout_type (t->ctype);
  }

  /* Build an associative array type, distinguished from arrays by having an
     index that's not necessarily an integer, and can be sparsely populated.  */

  void visit (TypeAArray *t)
  {
    /* In [abi/associative-arrays], associative arrays are a struct that only
       consist of a pointer to an opaque, implementation defined type.  */
    t->ctype = make_node (RECORD_TYPE);
    tree ptr = build_decl (BUILTINS_LOCATION, FIELD_DECL,
			   get_identifier ("ptr"), ptr_type_node);

    DECL_FIELD_CONTEXT (ptr) = t->ctype;
    TYPE_FIELDS (t->ctype) = ptr;
    TYPE_NAME (t->ctype) = get_identifier (t->toChars ());
    layout_type (t->ctype);

    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);
    d_keep (t->ctype);
  }

  /* Build type for a function declaration, which consists of a return type,
     and a list of parameter types, and a linkage attribute.  */

  void visit (TypeFunction *t)
  {
    tree fnparams = NULL_TREE;
    tree fntype;

    /* [function/variadic]

       Variadic functions with D linkage have an additional hidden argument
       with the name _arguments passed to the function.  */
    if (t->varargs == 1 && t->linkage == LINKd)
      {
	tree type = build_ctype (Type::typeinfotypelist->type);
	fnparams = chainon (fnparams, build_tree_list (0, type));
      }

    if (t->parameters)
      {
	size_t n_args = Parameter::dim (t->parameters);

	for (size_t i = 0; i < n_args; i++)
	  {
	    tree type = type_passed_as (Parameter::getNth (t->parameters, i));
	    fnparams = chainon (fnparams, build_tree_list (0, type));
	  }
      }

    /* When the last parameter is void_list_node, that indicates a fixed length
       parameter list, otherwise function is treated as variadic.  */
    if (t->varargs != 1)
      fnparams = chainon (fnparams, void_list_node);

    if (t->next != NULL)
      {
	fntype = build_ctype (t->next);
	if (t->isref)
	  fntype = build_reference_type (fntype);
      }
    else
      fntype = void_type_node;

    /* Could the function type be self referenced by parameters?  */
    t->ctype = build_function_type (fntype, fnparams);
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);
    d_keep (t->ctype);

    /* Handle any special support for calling conventions.  */
    switch (t->linkage)
      {
      case LINKpascal:
      case LINKwindows:
	/* [attribute/linkage]

	   The Windows convention is distinct from the C convention only
	   on Win32, where it is equivalent to the stdcall convention.  */
	if (!global.params.is64bit)
	  t->ctype = insert_type_attribute (t->ctype, "stdcall");
	break;

      case LINKc:
      case LINKcpp:
      case LINKd:
	/* [abi/function-calling-conventions]

	  The extern (C) and extern (D) calling convention matches
	  the C calling convention used by the supported C compiler
	  on the host system.  */
	break;

      default:
	gcc_unreachable ();
      }
  }

  /* Build a delegate type, an aggregate of two pieces of data, an object
     reference and a pointer to a non-static member function, or a pointer
     to a closure and a pointer to a nested function.  */

  void visit (TypeDelegate *t)
  {
    tree fntype = build_ctype (t->next);
    tree dgtype = build_method_type (void_type_node, fntype);

    TYPE_ATTRIBUTES (dgtype) = TYPE_ATTRIBUTES (fntype);
    TYPE_LANG_SPECIFIC (dgtype) = TYPE_LANG_SPECIFIC (fntype);

    /* In [abi/delegates], delegate layout is:
        .ptr	    context pointer.
        .funcptr    pointer to function.  */
    t->ctype = build_two_field_type (build_ctype (Type::tvoidptr),
				     build_pointer_type (dgtype),
				     t, "ptr", "funcptr");
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);
    d_keep (t->ctype);
  }


  /* User Defined Types.  */

  /* Build a named enum type, a distinct value whose values are restrict to
     a group of constants of the same underlying base type.  */

  void visit (TypeEnum *t)
  {
    tree basetype = build_ctype (t->sym->memtype);

    if (!t->sym->memtype->isintegral () || t->sym->memtype->ty == Tbool)
      {
	/* Enums in D2 can have a base type that is not necessarily integral.
	   For these, we simplify this a little by using the base type directly
	   instead of building an ENUMERAL_TYPE.  */
	t->ctype = build_variant_type_copy (basetype);
      }
    else
      {
	t->ctype = make_node (ENUMERAL_TYPE);
	ENUM_IS_SCOPED (t->ctype) = 1;
	TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);
	d_keep (t->ctype);

	if (flag_short_enums)
	  TYPE_PACKED (t->ctype) = 1;

	TYPE_PRECISION (t->ctype) = t->size (t->sym->loc) * 8;
	TYPE_SIZE (t->ctype) = 0;

	TYPE_MIN_VALUE (t->ctype) = TYPE_MIN_VALUE (basetype);
	TYPE_MAX_VALUE (t->ctype) = TYPE_MAX_VALUE (basetype);
	layout_type (t->ctype);

	tree values = NULL_TREE;
	if (t->sym->members)
	  {
	    for (size_t i = 0; i < t->sym->members->dim; i++)
	      {
		EnumMember *member = (*t->sym->members)[i]->isEnumMember ();
		/* Templated functions can seep through to the backend
		   just ignore for now.  */
		if (member == NULL)
		  continue;

		tree ident = get_identifier (member->ident->string);
		tree value = build_integer_cst (member->value->toInteger (),
						basetype);

		/* Build a identifier for the enumeration constant.  */
		tree decl = build_decl (UNKNOWN_LOCATION, CONST_DECL,
					ident, basetype);
		set_decl_location (decl, member->loc);
		DECL_CONTEXT (decl) = t->ctype;
		TREE_CONSTANT (decl) = 1;
		TREE_READONLY (decl) = 1;
		DECL_INITIAL (decl) = value;

		/* Add this enumeration constant to the list for this type.  */
		values = chainon (values, build_tree_list (ident, decl));
	      }
	  }

	TYPE_VALUES (t->ctype) = values;
	TYPE_UNSIGNED (t->ctype) = TYPE_UNSIGNED (basetype);
	build_type_decl (t->ctype, t->sym);
      }

    if (t->sym->userAttribDecl)
      {
	Expressions *attrs = t->sym->userAttribDecl->getAttributes ();
	decl_attributes (&t->ctype, build_attributes (attrs),
			 ATTR_FLAG_TYPE_IN_PLACE);
      }
  }

  /* Build a struct or union type.  Layout should be exactly represented
     as an equivalent C struct, except for non-POD or nested structs.  */

  void visit (TypeStruct *t)
  {
    /* Need to set this right away in case of self-references.  */
    t->ctype = make_node (t->sym->isUnionDeclaration ()
			  ? UNION_TYPE : RECORD_TYPE);
    d_keep (t->ctype);

    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);

    /* Must set up the overall size and alignment before determining
       the context or laying out fields as those types may make
       recursive references to this type.  */
    unsigned structsize = t->sym->structsize;
    unsigned alignsize = t->sym->alignsize;

    TYPE_SIZE (t->ctype) = bitsize_int (structsize * BITS_PER_UNIT);
    TYPE_SIZE_UNIT (t->ctype) = size_int (structsize);
    TYPE_ALIGN (t->ctype) = alignsize * BITS_PER_UNIT;
    TYPE_PACKED (t->ctype) = (alignsize == 1);
    compute_record_mode (t->ctype);

    /* Put out all fields.  */
    layout_aggregate_type (t->sym, t->ctype, t->sym);
    finish_aggregate_type (structsize, alignsize, t->ctype,
			   t->sym->userAttribDecl);

    TYPE_CONTEXT (t->ctype) = d_decl_context (t->sym);
    build_type_decl (t->ctype, t->sym);

    /* For structs with a user defined postblit or a destructor,
       also set TREE_ADDRESSABLE on the type and all variants.
       This will make the struct be passed around by reference.  */
    if (t->sym->postblit || t->sym->dtor)
      {
	for (tree tv = t->ctype; tv != NULL_TREE; tv = TYPE_NEXT_VARIANT (tv))
	  TREE_ADDRESSABLE (tv) = 1;
      }
  }

  /* Build a class type.  Whereas structs are value types, classes are
     reference types, with all the object-orientated features.  */

  void visit (TypeClass *t)
  {
    /* Need to set ctype right away in case of self-references to
       the type during this call.  */
    tree basetype = make_node (RECORD_TYPE);
    t->ctype = build_pointer_type (basetype);
    d_keep (t->ctype);

    /* Note that lang_specific data is assigned to both the reference
       and the underlying record type.  */
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type (t);
    TYPE_LANG_SPECIFIC (basetype) = TYPE_LANG_SPECIFIC (t->ctype);
    CLASS_TYPE_P (basetype) = 1;

    /* Put out all fields, including from each base class.  */
    layout_aggregate_type (t->sym, basetype, t->sym);
    finish_aggregate_type (t->sym->structsize, t->sym->alignsize,
			   basetype, t->sym->userAttribDecl);

    /* Classes only live in memory, so always set the TREE_ADDRESSABLE bit.  */
    for (tree tv = basetype; tv != NULL_TREE; tv = TYPE_NEXT_VARIANT (tv))
      TREE_ADDRESSABLE (tv) = 1;

    /* Type is final, there are no derivations.  */
    if (t->sym->storage_class & STCfinal)
      TYPE_FINAL_P (basetype) = 1;

    /* Create BINFO even if debugging is off.  This is needed to keep
       references to inherited types.  */
    if (!t->sym->isInterfaceDeclaration ())
      TYPE_BINFO (basetype) = build_class_binfo (NULL_TREE, t->sym);
    else
      {
	unsigned offset = 0;

	TYPE_BINFO (basetype) = build_interface_binfo (NULL_TREE, t->sym,
						       offset);
      }

    /* Associate all virtual methods with the class too.  */
    for (size_t i = 0; i < t->sym->vtbl.dim; i++)
      {
	FuncDeclaration *fd = t->sym->vtbl[i]->isFuncDeclaration ();
	tree method = fd ? fd->toSymbol ()->Stree : NULL_TREE;

	if (method && DECL_CONTEXT (method) == basetype)
	  {
	    DECL_CHAIN (method) = TYPE_METHODS (basetype);
	    TYPE_METHODS (basetype) = method;
	  }
      }

    TYPE_CONTEXT (basetype) = d_decl_context (t->sym);
    build_type_decl (basetype, t->sym);
  }
};


/* Build a tree from a frontend Type.  */

tree
build_ctype (Type *t)
{
  if (!t->ctype)
    {
      TypeVisitor v;

      /* Strip const modifiers from type before building.  This is done
	 to ensure that backend treats i.e: const(T) as a variant of T,
	 and not as two distinct types.  */
      if (t->isNaked ())
	t->accept (&v);
      else
	{
	  Type *tb = t->castMod (0);
	  if (!tb->ctype)
	    tb->accept (&v);
	  t->ctype = insert_type_modifiers (tb->ctype, t->mod);
	}
    }

  return t->ctype;
}

