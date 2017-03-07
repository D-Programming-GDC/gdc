// d-todt.cc -- D frontend for GCC.
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

#include "dfrontend/init.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/expression.h"
#include "dfrontend/declaration.h"
#include "dfrontend/template.h"
#include "dfrontend/ctfe.h"
#include "dfrontend/target.h"

#include "d-system.h"
#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "d-dmd-gcc.h"
#include "id.h"

// Append VAL to constructor PDT.  Create a new constructor
// of generic type if PDT is not already pointing to one.

dt_t **
dt_cons(dt_t **pdt, tree val)
{
  if (*pdt == NULL_TREE)
    *pdt = build_constructor(unknown_type_node, NULL);

  CONSTRUCTOR_APPEND_ELT(CONSTRUCTOR_ELTS(*pdt), 0, val);
  return pdt;
}

// It is necessary to give static array data its original
// type.  Otherwise, the SRA pass will not find the array
// elements.

// SRA accesses struct elements by field offset, so the
// ad-hoc type from dtvector_to_tree is fine.  It must still
// be a CONSTRUCTOR, or the CCP pass may use it incorrectly.

static tree
dt_container(dt_t *dt)
{
  // Generate type on the fly
  vec<constructor_elt, va_gc> *elts = NULL;
  tree fields = NULL_TREE;

  tree aggtype = make_node(RECORD_TYPE);
  tree offset = size_zero_node;

  if (dt != NULL_TREE)
    {
      tree value;
      size_t i;

      FOR_EACH_CONSTRUCTOR_VALUE(CONSTRUCTOR_ELTS(dt), i, value)
	{
	  if (value == error_mark_node)
	    return error_mark_node;

	  tree field = create_field_decl(TREE_TYPE(value), NULL, 1, 1);
	  tree size = TYPE_SIZE_UNIT(TREE_TYPE(value));

	  DECL_FIELD_CONTEXT(field) = aggtype;
	  DECL_FIELD_OFFSET(field) = offset;
	  DECL_FIELD_BIT_OFFSET(field) = bitsize_zero_node;
	  SET_DECL_OFFSET_ALIGN(field, TYPE_ALIGN(TREE_TYPE(value)));

	  layout_decl(field, 0);
	  fields = chainon(fields, field);
	  CONSTRUCTOR_APPEND_ELT(elts, field, value);
	  offset = size_binop(PLUS_EXPR, offset, size);
	}
    }
  else
    dt = build_constructor(aggtype, NULL);

  TYPE_FIELDS(aggtype) = fields;
  TYPE_SIZE(aggtype) = size_binop(MULT_EXPR, offset, size_int(BITS_PER_UNIT));
  TYPE_SIZE_UNIT(aggtype) = offset;
  compute_record_mode(aggtype);

  TREE_TYPE(dt) = aggtype;
  CONSTRUCTOR_ELTS(dt) = elts;
  TREE_STATIC(dt) = 1;
  TREE_CONSTANT(dt) = 1;

  return dt;
}

// Return a new CONSTRUCTOR whose values are in a dt_t
// list pointed to by DT.

tree
dtvector_to_tree(dt_t *dt)
{
  if (dt && CONSTRUCTOR_NELTS(dt) == 1)
    return CONSTRUCTOR_ELT(dt, 0)->value;

  return dt_container(dt);
}

/* ================================================================ */

// Build constructors for front-end Initialisers to be written to data segment.

dt_t *
Initializer::toDt()
{
  gcc_unreachable();
  return NULL_TREE;
}

dt_t *
VoidInitializer::toDt()
{
  // void initialisers are set to 0, just because we need something
  // to set them to in the static data segment.
  tree dt = NULL_TREE;
  dt_cons (&dt, build_constructor (build_ctype(type), NULL));
  return dt;
}

dt_t *
StructInitializer::toDt()
{
  ::error ("StructInitializer::toDt: we shouldn't emit this (%s)", toChars());
  gcc_unreachable();
}

dt_t *
ArrayInitializer::toDt()
{
  tree dt = NULL_TREE;
  dt_cons(&dt, build_expr(this->toExpression(), true));
  return dt;
}

dt_t *
ExpInitializer::toDt()
{
  tree dt = NULL_TREE;
  exp = exp->ctfeInterpret();
  dt_cons(&dt, build_expr(exp, true));
  return dt;
}

/* ================================================================ */

// Generate the data for the static initialiser.

void
ClassDeclaration::toDt(dt_t **pdt)
{
  NewExp *ne = new NewExp(this->loc, NULL, NULL, this->type, NULL);
  ne->type = this->type;

  Expression *e = ne->ctfeInterpret();
  gcc_assert (e->op == TOKclassreference);

  dt_cons(pdt, build_class_instance((ClassReferenceExp *) e));
}

void
StructDeclaration::toDt(dt_t **pdt)
{
  StructLiteralExp *sle = StructLiteralExp::create (loc, this, NULL);

  if (!fill(loc, sle->elements, true))
    gcc_unreachable();

  sle->type = type;
  dt_cons(pdt, build_expr(sle, true));
}
