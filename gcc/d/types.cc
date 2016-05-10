// types.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "dfrontend/attrib.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/enum.h"
#include "dfrontend/mtype.h"
#include "dfrontend/target.h"
#include "dfrontend/visitor.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "tm.h"
#include "function.h"
#include "toplev.h"
#include "target.h"
#include "stringpool.h"
#include "stor-layout.h"
#include "attribs.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"

// Implements the visitor interface to build the GCC trees of all Type
// AST classes emitted from the D Front-end, where CTYPE holds the
// cached backend representation to be returned.

class TypeVisitor : public Visitor
{
public:
  TypeVisitor() {}

  // This should be overridden by each type class
  void visit(Type *)
  {
    gcc_unreachable();
  }

  //
  void visit(TypeError *t)
  {
    t->ctype = error_mark_node;
  }

  //
  void visit(TypeBasic *t)
  {
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
	gcc_unreachable();
      }
  }

  // Valid case for Ttuple is in CommaExp codegen, in instances when
  // a tuple has been expanded as a large chain of comma expressions.
  void visit(TypeTuple *t)
  {
    t->ctype = void_type_node;
  }

  //
  void visit(TypeNull *t)
  {
    t->ctype = ptr_type_node;
  }

  //
  void visit(TypeEnum *t)
  {
    tree cmemtype = build_ctype(t->sym->memtype);

    // Enums in D2 can have a base type that is not necessarily integral.
    // So don't bother trying to make an ENUMERAL_TYPE using them.
    if (!t->sym->memtype->isintegral() || t->sym->memtype->ty == Tbool)
      {
	t->ctype = cmemtype;
	return;
      }

    t->ctype = make_node(ENUMERAL_TYPE);
    ENUM_IS_SCOPED (t->ctype) = 1;
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);
    d_keep(t->ctype);

    if (flag_short_enums)
      TYPE_PACKED (t->ctype) = 1;

    TYPE_PRECISION (t->ctype) = t->size(t->sym->loc) * 8;
    TYPE_SIZE (t->ctype) = 0;

    TYPE_MIN_VALUE (t->ctype) = TYPE_MIN_VALUE (cmemtype);
    TYPE_MAX_VALUE (t->ctype) = TYPE_MAX_VALUE (cmemtype);
    layout_type(t->ctype);

    tree enum_values = NULL_TREE;
    if (t->sym->members)
      {
	for (size_t i = 0; i < t->sym->members->dim; i++)
	  {
	    EnumMember *member = (*t->sym->members)[i]->isEnumMember();
	    // Templated functions can seep through to the backend - just ignore for now.
	    if (member == NULL)
	      continue;

	    tree ident = get_identifier(member->ident->string);
	    tree value = build_integer_cst(member->value->toInteger(), cmemtype);

	    // Build a identifier for the enumeration constant.
	    tree decl = build_decl(UNKNOWN_LOCATION, CONST_DECL, ident, cmemtype);
	    set_decl_location(decl, member->loc);
	    DECL_CONTEXT (decl) = t->ctype;
	    TREE_CONSTANT (decl) = 1;
	    TREE_READONLY (decl) = 1;
	    DECL_INITIAL (decl) = value;

	    // Add this enumeration constant to the list for this type.
	    enum_values = chainon(enum_values, build_tree_list(ident, decl));
	  }
      }

    TYPE_VALUES (t->ctype) = enum_values;
    TYPE_UNSIGNED (t->ctype) = TYPE_UNSIGNED (cmemtype);

    if (t->sym->userAttribDecl)
      {
	Expressions *attrs = t->sym->userAttribDecl->getAttributes();
	decl_attributes(&t->ctype, build_attributes(attrs),
			ATTR_FLAG_TYPE_IN_PLACE);
      }

    TYPE_CONTEXT (t->ctype) = d_decl_context(t->sym);
    build_type_decl(t->ctype, t->sym);
  }

  //
  void visit(TypeStruct *t)
  {
    // Need to set this right away in case of self-references.
    t->ctype = make_node(t->sym->isUnionDeclaration() ? UNION_TYPE : RECORD_TYPE);
    d_keep(t->ctype);

    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);

    // Must set up the overall size, etc. before determining the context or
    // laying out fields as those types may make references to this type.
    unsigned structsize = t->sym->structsize;
    unsigned alignsize = t->sym->alignsize;

    TYPE_SIZE (t->ctype) = bitsize_int(structsize * BITS_PER_UNIT);
    TYPE_SIZE_UNIT (t->ctype) = size_int(structsize);
    TYPE_ALIGN (t->ctype) = alignsize * BITS_PER_UNIT;
    TYPE_PACKED (t->ctype) = (alignsize == 1);
    compute_record_mode(t->ctype);

    layout_aggregate_type(t->sym, t->ctype, t->sym);
    finish_aggregate_type(structsize, alignsize, t->ctype, t->sym->userAttribDecl);

    TYPE_CONTEXT (t->ctype) = d_decl_context(t->sym);
    build_type_decl(t->ctype, t->sym);
  }

  //
  void visit(TypeFunction *t)
  {
    tree type_list = NULL_TREE;
    tree ret_type;

    if (t->varargs == 1 && t->linkage == LINKd)
      {
	// hidden _arguments parameter
	type_list = chainon(type_list, build_tree_list(0, build_ctype(Type::typeinfotypelist->type)));
      }

    if (t->parameters)
      {
	size_t n_args = Parameter::dim(t->parameters);

	for (size_t i = 0; i < n_args; i++)
	  {
	    Parameter *arg = Parameter::getNth(t->parameters, i);
	    type_list = chainon(type_list, build_tree_list(0, type_passed_as(arg)));
	  }
      }

    /* Last parm if void indicates fixed length list (as opposed to
       printf style va_* list). */
    if (t->varargs != 1)
      type_list = chainon(type_list, void_list_node);

    if (t->next != NULL)
      {
	ret_type = build_ctype(t->next);
	if (t->isref)
	  ret_type = build_reference_type(ret_type);
      }
    else
      ret_type = void_type_node;

    // Function type could be referenced by parameters, so set ctype earlier?
    t->ctype = build_function_type(ret_type, type_list);
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);
    d_keep(t->ctype);

    if (t->next && !t->isref)
      {
	Type *tn = t->next->baseElemOf();
	if (tn->ty == Tstruct)
	  {
	    // Non-POD structs must return in memory.
	    TypeStruct *ts = (TypeStruct *) tn->toBasetype();
	    if (!ts->sym->isPOD())
	      TREE_ADDRESSABLE (t->ctype) = 1;
	  }

	// Aggregate types that don't return in registers are eligable for
	// returning via slot optimisation.
	if (AGGREGATE_TYPE_P (TREE_TYPE (t->ctype))
	    && aggregate_value_p(TREE_TYPE (t->ctype), t->ctype))
	  TREE_ADDRESSABLE (t->ctype) = 1;
      }

    switch (t->linkage)
      {
      case LINKpascal:
      case LINKwindows:
	if (!global.params.is64bit)
	  t->ctype = insert_type_attribute(t->ctype, "stdcall");
	break;

      case LINKc:
      case LINKcpp:
      case LINKd:
	break;

      default:
	gcc_unreachable();
      }
  }

  //
  void visit(TypeVector *t)
  {
    int nunits = ((TypeSArray *) t->basetype)->dim->toUInteger();
    tree inner = build_ctype(t->elementType());

    if (inner == void_type_node)
      inner = build_ctype(Type::tuns8);

    t->ctype = build_vector_type(inner, nunits);
    layout_type(t->ctype);

    /* Give a graceful error if the backend does not support the vector type
       we are creating.  If backend has support for the inner type mode,
       then it can safely emulate the vector.  */
    if (!targetm.vector_mode_supported_p(TYPE_MODE (t->ctype))
	&& !targetm.scalar_mode_supported_p(TYPE_MODE (inner)))
      ::error("vector type %s is not supported on this architechture", t->toChars());
  }

  //
  void visit(TypeSArray *t)
  {
    if (t->dim->isConst() && t->dim->type->isintegral())
      {
	uinteger_t size = t->dim->toUInteger();
	if (t->next->toBasetype()->ty == Tvoid)
	  t->ctype = d_array_type(Type::tuns8, size);
	else
	  t->ctype = d_array_type(t->next, size);
      }
    else
      {
	::error("invalid expressions for static array dimension: %s", t->dim->toChars());
	gcc_unreachable();
      }
  }

  //
  void visit(TypeDArray *t)
  {
    tree lentype = build_ctype(Type::tsize_t);
    tree ptrtype = build_ctype(t->next);

    t->ctype = build_two_field_type(lentype, build_pointer_type(ptrtype),
				    t, "length", "ptr");
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);
    d_keep(t->ctype);
  }

  //
  void visit(TypeAArray *t)
  {
    /* Library functions expect a struct-of-pointer which could be passed
       differently from a pointer. */
    t->ctype = make_node(RECORD_TYPE);
    tree ptr = build_decl(BUILTINS_LOCATION, FIELD_DECL,
			  get_identifier("ptr"), ptr_type_node);
    DECL_FIELD_CONTEXT (ptr) = t->ctype;
    TYPE_FIELDS (t->ctype) = ptr;
    TYPE_NAME (t->ctype) = get_identifier(t->toChars());
    TYPE_TRANSPARENT_AGGR (t->ctype) = 1;
    layout_type(t->ctype);

    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);
    d_keep(t->ctype);
  }

  //
  void visit(TypePointer *t)
  {
    t->ctype = build_pointer_type(build_ctype(t->next));
  }

  //
  void visit(TypeDelegate *t)
  {
    gcc_assert(t->next->toBasetype()->ty == Tfunction);
    tree nexttype = build_ctype(t->next);
    tree objtype = build_ctype(Type::tvoidptr);
    // Delegate function types are like method types, in that
    // they pass around a hidden internal state.
    // Unlike method types, the hidden state is a generic pointer.
    tree funtype = build_vthis_type(void_type_node, nexttype);

    TYPE_ATTRIBUTES (funtype) = TYPE_ATTRIBUTES (nexttype);
    TYPE_LANG_SPECIFIC (funtype) = TYPE_LANG_SPECIFIC (nexttype);
    TREE_ADDRESSABLE (funtype) = TREE_ADDRESSABLE (nexttype);

    t->ctype = build_two_field_type(objtype, build_pointer_type(funtype),
				    t, "object", "func");
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);
    d_keep(t->ctype);
  }

  //
  void visit(TypeClass *t)
  {
    // Need to set t->ctype right away in case of self-references to
    // the type during this call.
    tree basetype = make_node(RECORD_TYPE);
    t->ctype = build_pointer_type(basetype);
    d_keep(t->ctype);

    // Note that this is set on both the reference type and record type.
    TYPE_LANG_SPECIFIC (t->ctype) = build_lang_type(t);
    TYPE_LANG_SPECIFIC (basetype) = TYPE_LANG_SPECIFIC (t->ctype);
    CLASS_TYPE_P (basetype) = 1;

    // Add the fields of each base class
    layout_aggregate_type(t->sym, basetype, t->sym);
    finish_aggregate_type(t->sym->structsize, t->sym->alignsize, basetype, t->sym->userAttribDecl);

    // Type is final, there are no derivations.
    if (t->sym->storage_class & STCfinal)
      TYPE_FINAL_P (basetype) = 1;

    // Create BINFO even if debugging is off.  This is needed to keep
    // references to inherited types.
    if (!t->sym->isInterfaceDeclaration())
      TYPE_BINFO (basetype) = build_class_binfo(NULL_TREE, t->sym);
    else
      {
	unsigned offset = 0;
	TYPE_BINFO (basetype) = build_interface_binfo(NULL_TREE, t->sym, offset);
      }

    // Same for virtual methods too.
    for (size_t i = 0; i < t->sym->vtbl.dim; i++)
      {
	FuncDeclaration *fd = t->sym->vtbl[i]->isFuncDeclaration();
	tree method = fd ? fd->toSymbol()->Stree : NULL_TREE;

	if (method && DECL_CONTEXT (method) == basetype)
	  {
	    DECL_CHAIN (method) = TYPE_METHODS (basetype);
	    TYPE_METHODS (basetype) = method;
	  }
      }

    TYPE_CONTEXT (basetype) = d_decl_context(t->sym);
    build_type_decl(basetype, t->sym);
  }
};

//
tree
build_ctype(Type *t)
{
  if (!t->ctype)
    {
      TypeVisitor v;
      if (t->isNaked())
	t->accept(&v);
      else
	{
	  Type *tb = t->castMod(0);
	  if (!tb->ctype)
	    tb->accept(&v);
	  t->ctype = insert_type_modifiers(tb->ctype, t->mod);
	}
    }

  return t->ctype;
}

