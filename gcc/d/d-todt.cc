// d-todt.cc -- D frontend for GCC.
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
#include "dt.h"


// Return a pointer to the last empty node in PDT, which is a chain
// of dt_t nodes (chained through TREE_CHAIN).

dt_t **
dt_last (dt_t **pdt)
{
  if (*pdt)
    {
      tree chain = *pdt;
      tree next;
      while ((next = TREE_CHAIN (chain)))
	chain = next;
      pdt = &TREE_CHAIN (chain);
    }
  return pdt;
}

// Append dt_t node VAL on the end of chain of dt_t
// nodes (chained through TREE_CHAIN) in PDT.

dt_t **
dt_cons (dt_t **pdt, dt_t *val)
{
  tree *pdtend = dt_last (pdt);
  gcc_assert (TREE_CODE (val) != TREE_LIST);
  *pdtend = tree_cons (NULL_TREE, val, NULL_TREE);
  return pdt;
}

// Concatenate two chains of dt_t nodes (chained through TREE_CHAIN)
// by modifying the last chain of PDT to point to VAL.

dt_t **
dt_chainon (dt_t **pdt, dt_t *val)
{
  tree *pdtend = dt_last (pdt);
  gcc_assert (TREE_CODE (val) == TREE_LIST);
  *pdtend = copy_node (val);
  return pdt;
}

// Add zero padding of size SIZE onto end of PDT.

dt_t **
dt_zeropad (dt_t **pdt, size_t size)
{
  tree type = d_array_type (Type::tuns8, size);
  gcc_assert (size != 0);
  return dt_cons (pdt, build_constructor (type, NULL));
}

// It is necessary to give static array data its original
// type.  Otherwise, the SRA pass will not find the array
// elements.

// SRA accesses struct elements by field offset, so the
// ad-hoc type from dtlist_to_tree is fine.  It must still
// be a CONSTRUCTOR, or the CCP pass may use it incorrectly.

static tree
dt_container2 (dt_t *dt)
{
  // Generate type on the fly
  CtorEltMaker elts;
  ListMaker fields;
  tree ctor;

  tree aggtype = make_node (RECORD_TYPE);
  tree offset = size_zero_node;

  while (dt)
    {
      tree value = TREE_VALUE (dt);
      tree field = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE, TREE_TYPE (value));
      tree size = TYPE_SIZE_UNIT (TREE_TYPE (value));

      DECL_CONTEXT (field) = aggtype;
      DECL_FIELD_OFFSET (field) = offset;
      DECL_FIELD_BIT_OFFSET (field) = bitsize_zero_node;
      SET_DECL_OFFSET_ALIGN (field, TYPE_ALIGN (TREE_TYPE (value)));
      DECL_ARTIFICIAL (field) = 1;
      DECL_IGNORED_P (field) = 1;

      layout_decl (field, 0);
      fields.chain (field);
      elts.cons (field, value);

      offset = size_binop (PLUS_EXPR, offset, size);
      dt = TREE_CHAIN (dt);
    }

  TYPE_FIELDS (aggtype) = fields.head;
  TYPE_SIZE (aggtype) = size_binop (MULT_EXPR, offset, size_int (BITS_PER_UNIT));
  TYPE_SIZE_UNIT (aggtype) = offset;
  compute_record_mode (aggtype);

  ctor = build_constructor (aggtype, elts.head);
  TREE_READONLY (ctor) = 1;
  TREE_STATIC (ctor) = 1;
  TREE_CONSTANT (ctor) = 1;

  return ctor;
}

// Build a new CONSTRUCTOR of type TYPE around the values
// DT and append to the dt_t node list PDT.

dt_t **
dt_container (dt_t **pdt, Type *type, dt_t *dt)
{
  Type *tb = type->toBasetype();

  if (tb->ty == Tsarray)
    {
      // Generate static array constructor.
      TypeSArray *tsa = (TypeSArray *) tb;
      CtorEltMaker elts;
      elts.reserve (tsa->dim->toInteger());
      size_t i = 0;

      while (dt)
	{
	  elts.cons (size_int (i++), TREE_VALUE (dt));
	  dt = TREE_CHAIN (dt);
	}

      tree ctor = build_constructor (type->toCtype(), elts.head);
      TREE_CONSTANT (ctor) = 1;
      TREE_READONLY (ctor) = 1;
      TREE_STATIC (ctor) = 1;

      return dt_cons (pdt, ctor);
    }
  else if (tb->ty == Tstruct)
    {
      tree ctor = dt_container2 (dt);
      TREE_TYPE (ctor) = type->toCtype();
      return dt_cons (pdt, ctor);
    }

  return dt_cons (pdt, dtlist_to_tree (dt));
}

// Return a new CONSTRUCTOR whose values are in a dt_t
// list pointed to by DT.

tree
dtlist_to_tree (dt_t *dt)
{
  if (dt && !TREE_CHAIN (dt))
    return TREE_VALUE (dt);

  return dt_container2 (dt);
}

// Put out vptr and monitor of class CD into PDT.

dt_t **
build_dt_vtable (dt_t **pdt, ClassDeclaration *cd)
{
  gcc_assert (cd != NULL);
  Symbol *s = cd->toVtblSymbol();
  dt_cons (pdt, build_address (s->Stree));
  dt_cons (pdt, size_int (0));
  return pdt;
}

/* ================================================================ */

// Create constructor for ComplexExp to be written to data segment.

dt_t **
ComplexExp::toDt (dt_t **pdt)
{
  return dttree (pdt, toElem (&gen));
}

// Create constructor for IntegerExp to be written to data segment.

dt_t **
IntegerExp::toDt (dt_t **pdt)
{
  return dttree (pdt, toElem (&gen));
}

// Create constructor for RealExp to be written to data segment.

dt_t **
RealExp::toDt (dt_t **pdt)
{
  return dttree (pdt, toElem (&gen));
}
