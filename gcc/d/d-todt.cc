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

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "toplev.h"
#include "stor-layout.h"

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

// Concatenate two constructors of dt_t nodes by appending all
// values of DT to PDT.

dt_t **
dt_chainon(dt_t **pdt, dt_t *dt)
{
  vec<constructor_elt, va_gc> *elts = CONSTRUCTOR_ELTS(dt);
  tree value;
  size_t i;

  gcc_assert(*pdt != dt);

  FOR_EACH_CONSTRUCTOR_VALUE(elts, i, value)
    dt_cons(pdt, value);

  return pdt;
}

// Add zero padding of size SIZE onto end of PDT.

dt_t **
dt_zeropad(dt_t **pdt, size_t size)
{
  tree type = d_array_type(Type::tuns8, size);
  gcc_assert(size != 0);
  return dt_cons(pdt, build_constructor(type, NULL));
}

// It is necessary to give static array data its original
// type.  Otherwise, the SRA pass will not find the array
// elements.

// SRA accesses struct elements by field offset, so the
// ad-hoc type from dtvector_to_tree is fine.  It must still
// be a CONSTRUCTOR, or the CCP pass may use it incorrectly.

static tree
dt_container2(dt_t *dt)
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

	  tree field = build_decl(UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE, TREE_TYPE(value));
	  tree size = TYPE_SIZE_UNIT(TREE_TYPE(value));

	  DECL_FIELD_CONTEXT(field) = aggtype;
	  DECL_FIELD_OFFSET(field) = offset;
	  DECL_FIELD_BIT_OFFSET(field) = bitsize_zero_node;
	  SET_DECL_OFFSET_ALIGN(field, TYPE_ALIGN(TREE_TYPE(value)));
	  DECL_ARTIFICIAL(field) = 1;
	  DECL_IGNORED_P(field) = 1;

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

// Build a new CONSTRUCTOR of type TYPE around the values
// DT and append to the dt_t node list PDT.

dt_t **
dt_container(dt_t **pdt, Type *type, dt_t *dt)
{
  Type *tb = type->toBasetype();

  if (tb->ty == Tsarray)
    {
      // Generate static array constructor.
      TypeSArray *tsa = (TypeSArray *) tb;
      vec<constructor_elt, va_gc> *elts = NULL;
      tree value;
      size_t i;

      if (dt == NULL)
	dt = dt_container2(dt);
      else
	{
	  gcc_assert(CONSTRUCTOR_NELTS(dt) == tsa->dim->toInteger());

	  FOR_EACH_CONSTRUCTOR_VALUE(CONSTRUCTOR_ELTS(dt), i, value)
	    CONSTRUCTOR_APPEND_ELT(elts, size_int(i), value);

	  CONSTRUCTOR_ELTS(dt) = elts;
	}

      if (dt != error_mark_node)
	{
	  TREE_TYPE(dt) = build_ctype(type);
	  TREE_CONSTANT(dt) = 1;
	  TREE_STATIC(dt) = 1;
	}

      return dt_cons(pdt, dt);
    }
  else if (tb->ty == Tstruct)
    {
      dt = dt_container2(dt);
      if (dt != error_mark_node)
	TREE_TYPE(dt) = build_ctype(type);
      return dt_cons(pdt, dt);
    }
  else if (tb->ty == Tclass)
    {
      dt = dt_container2(dt);
      if (dt != error_mark_node)
	TREE_TYPE(dt) = TREE_TYPE(build_ctype(type));
      return dt_cons(pdt, dt);
    }

  return dt_cons(pdt, dtvector_to_tree(dt));
}

// Return a new CONSTRUCTOR whose values are in a dt_t
// list pointed to by DT.

tree
dtvector_to_tree(dt_t *dt)
{
  if (dt && CONSTRUCTOR_NELTS(dt) == 1)
    return CONSTRUCTOR_ELT(dt, 0)->value;

  return dt_container2(dt);
}

// Put out __vptr and __monitor of class CD into PDT.

dt_t **
build_vptr_monitor(dt_t **pdt, ClassDeclaration *cd)
{
  gcc_assert(cd != NULL);
  dt_cons(pdt, build_address(get_vtable_decl (cd)));

  if (!cd->cpp)
    dt_cons(pdt, size_int(0));

  return pdt;
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

// Build constructors for front-end Expressions to be written to data segment.

dt_t **
ClassReferenceExp::toInstanceDt(dt_t **pdt)
{
  ClassDeclaration *cd = originalClass();
  Dts dts;
  dts.setDim(value->elements->dim);
  dts.zero();

  for (size_t i = 0; i < value->elements->dim; i++)
    {
      Expression *e = (*value->elements)[i];
      if (!e)
	continue;
      tree dt = NULL_TREE;
      dt_cons(&dt, build_expr(e, true));
      dts[i] = dt;
    }

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   */
  tree cdt = NULL_TREE;
  build_vptr_monitor(&cdt, cd);

  // Put out rest of class fields.
  toDt2(&cdt, cd, &dts);

  return dt_container(pdt, cd->type, cdt);
}

// Generates the data for the static initializer of class variable.
// DTS is an array of dt fields, which values have been evaluated in compile time.
// CD - is a ClassDeclaration, for which initializing data is being built
// this function, being alike to ClassDeclaration::toDt2, recursively builds the dt for all base classes.

dt_t **
ClassReferenceExp::toDt2(dt_t **pdt, ClassDeclaration *cd, Dts *dts)
{
  // Note equivalence of this implementation to class's
  size_t offset;

  if (cd->baseClass)
    {
      toDt2(pdt, cd->baseClass, dts);
      offset = cd->baseClass->structsize;
    }
  else
    {
      // Allow room for __vptr and __monitor.
      if (cd->cpp)
	offset = Target::ptrsize;
      else
	offset = Target::ptrsize * 2;
    }

  for (size_t i = 0; i < cd->fields.dim; i++)
    {
      VarDeclaration *v = cd->fields[i];
      int index = findFieldIndexByName(v);
      gcc_assert(index != -1);

      tree fdt = (*dts)[index];

      if (fdt == NULL_TREE)
	{
	  tree dt = NULL_TREE;
	  Initializer *init = v->init;

	  if (init)
	    {
	      ExpInitializer *ei = init->isExpInitializer();
	      Type *tb = v->type->toBasetype();
	      if (!init->isVoidInitializer())
		{
		  if (ei && tb->ty == Tsarray)
		    ((TypeSArray *) tb)->toDtElem(&dt, ei->exp);
		  else
		    dt = init->toDt();
		}
	    }
	  else if (v->offset >= offset)
	    {
	      if (v->type->ty == Tstruct)
		((TypeStruct *) v->type)->sym->toDt(&dt);
	      else
		{
		  Expression *e = v->type->defaultInitLiteral(loc);
		  dt_cons(&dt, build_expr(e, true));
		}
	    }

	  if (dt != NULL_TREE)
	    {
	      if (v->offset < offset)
		error("duplicated union initialization for %s", v->toChars());
	      else
		{
		  if (offset < v->offset)
		    dt_zeropad(pdt, v->offset - offset);
		  dt_chainon(pdt, dt);
		  offset = v->offset + v->type->size();
		}
	    }
	}

      if (fdt != NULL_TREE)
	{
	  if (v->offset < offset)
	    error("duplicate union initialization for %s", v->toChars());
	  else
	    {
	      size_t sz = int_size_in_bytes(TREE_TYPE(CONSTRUCTOR_ELT(fdt, 0)->value));
	      size_t vsz = v->type->size();
	      size_t voffset = v->offset;
	      size_t dim = 1;

	      if (sz > vsz)
		{
		  gcc_assert(v->type->ty == Tsarray && vsz == 0);
		  error("zero length array %s has non-zero length initializer", v->toChars());
		}

	      for (Type *vt = v->type->toBasetype();
		   vt->ty == Tsarray; vt = vt->nextOf()->toBasetype())
		{
		  TypeSArray *tsa = (TypeSArray *) vt;
		  dim *= tsa->dim->toInteger();
		}

	      gcc_assert(sz == vsz || sz * dim <= vsz);

	      for (size_t i = 0; i < dim; i++)
		{
		  if (offset < voffset)
		    dt_zeropad(pdt, voffset - offset);

		  if (fdt == NULL_TREE)
		    {
		      if (v->init)
			fdt = v->init->toDt();
		      else if (v->type->ty == Tstruct)
			((TypeStruct *) v->type)->sym->toDt(&fdt);
		      else
			{
			  Expression *e = v->type->defaultInitLiteral(loc);
			  dt_cons(&fdt, build_expr(e, true));
			}
		    }

		  dt_chainon(pdt, fdt);
		  fdt = NULL_TREE;

		  offset = voffset + sz;
		  voffset += vsz / dim;
		  if (sz == vsz)
		    break;
		}
	    }
	}
    }

  // Interface vptr initializations
  cd->toSymbol();

  for (size_t i = 0; i < cd->vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*cd->vtblInterfaces)[i];

      for (ClassDeclaration *cd2 = originalClass(); 1; cd2 = cd2->baseClass)
	{
	  gcc_assert(cd2);
	  unsigned csymoffset = cd2->baseVtblOffset(b);
	  if (csymoffset != (unsigned) ~0)
	    {
	      tree dt = build_address(cd2->toSymbol());
	      if (offset < (size_t) b->offset)
		dt_zeropad(pdt, b->offset - offset);
	      dt_cons(pdt, build_offset(dt, size_int(csymoffset)));
	      break;
	    }
	}

      offset = b->offset + Target::ptrsize;
    }

  if (offset < cd->structsize)
    dt_zeropad(pdt, cd->structsize - offset);

  return pdt;
}

/* ================================================================ */

// Generate the data for the static initialiser.

void
ClassDeclaration::toDt(dt_t **pdt)
{
  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   */
  tree cdt = NULL_TREE;
  build_vptr_monitor(&cdt, this);

  // Put out rest of class fields.
  toDt2(&cdt, this);

  dt_container(pdt, type, cdt);
}

void
ClassDeclaration::toDt2(dt_t **pdt, ClassDeclaration *cd)
{
  size_t offset;

  if (baseClass)
    {
      baseClass->toDt2(pdt, cd);
      offset = baseClass->structsize;
    }
  else
    {
      // Allow room for __vptr and __monitor
      if (cd->cpp)
	offset = Target::ptrsize;
      else
	offset = Target::ptrsize * 2;
    }

  // Note equivalence of this loop to struct's
  for (size_t i = 0; i < fields.dim; i++)
    {
      VarDeclaration *v = fields[i];
      Initializer *init = v->init;
      tree dt = NULL_TREE;

      if (init)
	{
	  ExpInitializer *ei = init->isExpInitializer();
	  Type *tb = v->type->toBasetype();
	  if (!init->isVoidInitializer())
	    {
	      if (ei && tb->ty == Tsarray)
		((TypeSArray *) tb)->toDtElem(&dt, ei->exp);
	      else
		dt = init->toDt();
	    }
	}
      else if (v->offset >= offset)
	{
	  if (v->type->ty == Tstruct)
	    ((TypeStruct *) v->type)->sym->toDt(&dt);
	  else
	    {
	      Expression *e = v->type->defaultInitLiteral(loc);
	      dt_cons(&dt, build_expr(e, true));
	    }
	}


      if (dt != NULL_TREE)
	{
	  if (v->offset < offset)
	    error("duplicated union initialization for %s", v->toChars());
	  else
	    {
	      if (offset < v->offset)
		dt_zeropad(pdt, v->offset - offset);
	      dt_chainon(pdt, dt);
	      offset = v->offset + v->type->size();
	    }
	}
    }

  // Interface vptr initializations
  toSymbol();

  for (size_t i = 0; i < vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*vtblInterfaces)[i];

      for (ClassDeclaration *cd2 = cd; 1; cd2 = cd2->baseClass)
	{
	  gcc_assert(cd2);
	  unsigned csymoffset = cd2->baseVtblOffset(b);
	  if (csymoffset != (unsigned) ~0)
	    {
	      tree dt = build_address(cd2->toSymbol());
	      if (offset < (size_t) b->offset)
		dt_zeropad(pdt, b->offset - offset);
	      dt_cons(pdt, build_offset(dt, size_int(csymoffset)));
	      break;
	    }
	}

      offset = b->offset + Target::ptrsize;
    }

  if (offset < structsize)
    dt_zeropad(pdt, structsize - offset);
}

void
StructDeclaration::toDt(dt_t **pdt)
{
  StructLiteralExp *sle = new StructLiteralExp(loc, this, NULL);

  if (!fill(loc, sle->elements, true))
    gcc_unreachable();

  sle->type = type;
  dt_cons(pdt, build_expr(sle, true));
}

/* ================================================================ */

// Generate the data for the default initialiser of the type.

dt_t **
TypeSArray::toDtElem (dt_t **pdt, Expression *e)
{
  dinteger_t len = dim->toInteger();

  if (len)
    {
      tree dt = NULL_TREE;
      Type *tnext = next;
      Type *tbn = tnext->toBasetype();

      if (e && (e->op == TOKstring || e->op == TOKarrayliteral))
	{
	  while (tbn->ty == Tsarray && (!e || tbn != e->type->nextOf()))
	    {
	      TypeSArray *tsa = (TypeSArray *) tbn;
	      len *= tsa->dim->toInteger();
	      tnext = tbn->nextOf();
	      tbn = tnext->toBasetype();
	    }

	  if (e->op == TOKstring)
	    len /= ((StringExp *) e)->len;
	  else if (e->op == TOKarrayliteral)
	    len /= ((ArrayLiteralExp *) e)->elements->dim;

	  for (size_t i = 0; i < len; i++)
	    dt_cons(&dt, build_expr(e, true));

	  // Single initialiser already constructed, just chain onto pdt.
	  if (len == 1)
	    return dt_chainon (pdt, dt);
	}
      else
	{
	  // If not already supplied use default initialiser.
	  if (!e)
	    e = defaultInit(Loc());

	  for (size_t i = 0; i < len; i++)
	    {
	      if (tbn->ty == Tsarray)
		((TypeSArray *) tbn)->toDtElem (&dt, e);
	      else
		dt_cons(&dt, build_expr(e, true));
	    }
	}

      return dt_container (pdt, this, dt);
    }

  return pdt;
}
