// d-todt.cc -- D frontend for GCC.
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

#include "enum.h"
#include "id.h"
#include "init.h"
#include "scope.h"
#include "ctfe.h"

#include "dfrontend/target.h"


// Append VAL to constructor PDT.  Create a new constructor
// of generic type if PDT is not already pointing to one.

dt_t **
dt_cons(dt_t **pdt, tree val)
{
  if (*pdt == NULL_TREE)
    *pdt = build_constructor(d_unknown_type_node, NULL);

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
	  tree field = build_decl(UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE, TREE_TYPE(value));
	  tree size = TYPE_SIZE_UNIT(TREE_TYPE(value));

	  DECL_CONTEXT(field) = aggtype;
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

      TREE_TYPE(dt) = type->toCtype();
      TREE_CONSTANT(dt) = 1;
      TREE_STATIC(dt) = 1;

      return dt_cons(pdt, dt);
    }
  else if (tb->ty == Tstruct)
    {
      dt = dt_container2(dt);
      TREE_TYPE(dt) = type->toCtype();
      return dt_cons(pdt, dt);
    }
  else if (tb->ty == Tclass)
    {
      dt = dt_container2(dt);
      TREE_TYPE(dt) = TREE_TYPE(type->toCtype());
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
  Symbol *s = cd->toVtblSymbol();
  dt_cons(pdt, build_address(s->Stree));

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
  dt_cons (&dt, build_constructor (type->toCtype(), NULL));
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
  Type *tb = type->toBasetype();
  if (tb->ty == Tvector)
    tb = ((TypeVector *) tb)->basetype;

  Type *tn = tb->nextOf()->toBasetype();

  Dts dts;
  dts.setDim (dim);
  dts.zero();

  size_t length = 0;
  for (size_t i = 0; i < index.dim; i++)
    {
      Expression *idx = index[i];
      if (idx)
	length = idx->toInteger();

      gcc_assert (length < dim);
      Initializer *val = value[i];
      tree dt = val->toDt();

      if (dts[length])
	error (loc, "duplicate initializations for index %d", length);
      dts[length] = dt;
      length++;
    }

  tree sadefault = NULL_TREE;
  if (tn->ty == Tsarray)
    tn->toDt (&sadefault);
  else
    {
      Expression *edefault = tb->nextOf()->defaultInit();
      edefault->toDt (&sadefault);
    }

  tree dt = NULL_TREE;
  for (size_t i = 0; i < dim; i++)
    dt_chainon (&dt, dts[i] ? dts[i] : sadefault);

  if (tb->ty == Tsarray)
    {
      TypeSArray *ta = (TypeSArray *) tb;
      size_t tadim = ta->dim->toInteger();

      if (dim < tadim)
	{
	  // Pad out the rest of the array.
	  for (size_t i = dim; i < tadim; i++)
	    dt_chainon (&dt, sadefault);
	}
      else if (dim > tadim)
	error (loc, "too many initializers, %d, for array[%d]", dim, tadim);
    }
  else
    {
      gcc_assert (tb->ty == Tarray || tb->ty == Tpointer);

      // Create symbol, and then refer to it
      Symbol *s = new Symbol();
      s->Sdt = dt;
      d_finish_symbol (s);
      dt = NULL_TREE;

      if (tb->ty == Tarray)
	dt_cons (&dt, size_int (dim));

      dt_cons (&dt, build_address (s->Stree));
    }

  tree cdt = NULL_TREE;
  dt_container (&cdt, type, dt);
  return cdt;
}

dt_t *
ExpInitializer::toDt()
{
  tree dt = NULL_TREE;
  exp = exp->optimize (WANTvalue);
  exp->toDt (&dt);
  return dt;
}

/* ================================================================ */

// Build constructors for front-end Expressions to be written to data segment.

dt_t **
Expression::toDt (dt_t **pdt)
{
  error ("non-constant expression %s", toChars());
  return pdt;
}

dt_t **
IntegerExp::toDt (dt_t **pdt)
{
  tree dt = toElem (NULL);
  return dt_cons (pdt, dt);
}

dt_t **
RealExp::toDt (dt_t **pdt)
{
  tree dt = toElem (NULL);
  return dt_cons (pdt, dt);
}

dt_t **
ComplexExp::toDt (dt_t **pdt)
{
  tree dt = toElem (NULL);
  return dt_cons (pdt, dt);
}

dt_t **
NullExp::toDt (dt_t **pdt)
{
  gcc_assert (type);

  tree dt = build_constructor (type->toCtype(), NULL);
  return dt_cons (pdt, dt);
}

dt_t **
StringExp::toDt (dt_t **pdt)
{
  tree dt = toElem (NULL);
  return dt_cons (pdt, dt);
}

dt_t **
ArrayLiteralExp::toDt (dt_t **pdt)
{
  tree dt = NULL_TREE;

  for (size_t i = 0; i < elements->dim; i++)
    {
      Expression *e = (*elements)[i];
      e->toDt (&dt);
    }

  Type *tb = type->toBasetype();

  if (tb->ty != Tsarray)
    {
      gcc_assert (tb->ty == Tarray || tb->ty == Tpointer);

      // Create symbol, and then refer to it
      Symbol *s = new Symbol();
      s->Sdt = dt;
      d_finish_symbol (s);
      dt = NULL_TREE;

      if (tb->ty == Tarray)
	dt_cons (&dt, size_int (elements->dim));

      dt_cons (&dt, build_address (s->Stree));
    }

  dt_container (pdt, type, dt);
  return pdt;
}

dt_t **
StructLiteralExp::toDt (dt_t **pdt)
{
  tree sdt = NULL_TREE;
  size_t offset = 0;

  gcc_assert (sd->fields.dim - sd->isNested() <= elements->dim);

  for (size_t i = 0; i < elements->dim; i++)
    {
      Expression *e = (*elements)[i];
      if (!e)
	continue;

      VarDeclaration *vd = NULL;
      size_t k = 0;

      for (size_t j = i; j < elements->dim; j++)
	{
	  VarDeclaration *vd2 = sd->fields[j];
	  if (vd2->offset < offset || (*elements)[j] == NULL)
	    continue;

	  // Find the nearest field
	  if (!vd)
	    {
	      vd = vd2;
	      k = j;
	    }
	  else if (vd2->offset < vd->offset)
	    {
	      // Each elements should have no overlapping
	      gcc_assert (!(vd->offset < vd2->offset + vd2->type->size()
			    && vd2->offset < vd->offset + vd->type->size()));
	      vd = vd2;
	      k = j;
	    }
	}

      if (vd != NULL)
	{
	  if (offset < vd->offset)
	    dt_zeropad (&sdt, vd->offset - offset);

	  e = (*elements)[k];

	  Type *tb = vd->type->toBasetype();
	  if (tb->ty == Tsarray)
	    ((TypeSArray *)tb)->toDtElem (&sdt, e);
	  else
	    e->toDt (&sdt);

	  offset = vd->offset + vd->type->size();
	}
    }

  if (offset < sd->structsize)
    dt_zeropad (&sdt, sd->structsize - offset);

  dt_container (pdt, type, sdt);
  return pdt;
}

dt_t **
SymOffExp::toDt (dt_t **pdt)
{
  gcc_assert (var);

  if (!(var->isDataseg() || var->isCodeseg())
      || var->needThis() || var->isThreadlocal())
    {
      error ("non-constant expression %s", toChars());
      return pdt;
    }

  Symbol *s = var->toSymbol();
  tree dt = build_offset (build_address (s->Stree), size_int (offset));
  return dt_cons (pdt, dt);
}

dt_t **
VarExp::toDt (dt_t **pdt)
{
  VarDeclaration *v = var->isVarDeclaration();
  SymbolDeclaration *sd = var->isSymbolDeclaration();

  if (v && (v->isConst() || v->isImmutable())
      && type->toBasetype()->ty != Tsarray && v->init)
    {
      if (v->inuse)
	{
	  error ("recursive reference %s", toChars());
	  return pdt;
	}
      v->inuse++;
      dt_chainon (pdt, v->init->toDt());
      v->inuse--;
    }
  else if (sd && sd->dsym)
    sd->dsym->toDt (pdt);
  else
    error ("non-constant expression %s", toChars());

  return pdt;
}

dt_t **
FuncExp::toDt (dt_t **pdt)
{
  if (fd->tok == TOKreserved && type->ty == Tpointer)
    {
      // Change to non-nested.
      fd->tok = TOKfunction;
      fd->vthis = NULL;
    }

  if (fd->isNested())
    {
      error ("non-constant nested delegate literal expression %s", toChars());
      return pdt;
    }

  tree dt = build_address (fd->toSymbol()->Stree);
  fd->toObjFile (0);

  return dt_cons (pdt, dt);
}

dt_t **
VectorExp::toDt (dt_t **pdt)
{
  tree dt = NULL_TREE;

  for (unsigned i = 0; i < dim; i++)
    {
      Expression *elem;
      if (e1->op == TOKarrayliteral)
	{
	  ArrayLiteralExp *ea = (ArrayLiteralExp *) e1;
	  elem = (*ea->elements)[i];
	}
      else
	elem = e1;

      elem->toDt (&dt);
    }

  dt_container (pdt, type, dt);
  return pdt;
}

dt_t **
CastExp::toDt (dt_t **pdt)
{
  if (e1->type->ty == Tclass && type->ty == Tclass)
    {
      TypeClass *tc = (TypeClass *) type;
      if (tc->sym->isInterfaceDeclaration())
	{
	  // Casting from class to interface.
	  gcc_assert (e1->op == TOKclassreference);

	  ClassReferenceExp *exp = (ClassReferenceExp *) e1;
	  ClassDeclaration *from = exp->originalClass();
	  InterfaceDeclaration *to = (InterfaceDeclaration *) tc->sym;
	  int off = 0;
	  int isbase = to->isBaseOf (from, &off);
	  gcc_assert (isbase);

	  return exp->toDtI (pdt, off);
	}
      else
	{
	  // Casting from class to class.
	  return e1->toDt (pdt);
	}
    }

  return UnaExp::toDt (pdt);
}

dt_t **
AddrExp::toDt (dt_t **pdt)
{
  if (e1->op == TOKstructliteral)
    {
      StructLiteralExp *sl = (StructLiteralExp *) e1;
      tree dt = build_address (sl->toSymbol()->Stree);
      return dt_cons (pdt, dt);
    }

  return UnaExp::toDt (pdt);
}

dt_t **
ClassReferenceExp::toDt(dt_t **pdt)
{
  InterfaceDeclaration *to = ((TypeClass *) type)->sym->isInterfaceDeclaration();

  if (to != NULL)
    {
      // Static typeof this literal is an interface.
      // We must add offset to symbol.
      ClassDeclaration *from = originalClass();
      int off = 0;
      int isbase = to->isBaseOf(from, &off);
      gcc_assert(isbase);

      return toDtI(pdt, off);
    }

  return toDtI(pdt, 0);
}

dt_t **
ClassReferenceExp::toDtI(dt_t **pdt, int off)
{
  tree dt = build_address(toSymbol()->Stree);

  if (off != 0)
    dt = build_offset(dt, size_int(off));

  return dt_cons(pdt, dt);
}

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
      e->toDt(&dt);
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
	    v->type->toDt(&dt);

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
		      else
			v->type->toDt(&fdt);
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
	      tree dt = build_address(cd2->toSymbol()->Stree);
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
	v->type->toDt(&dt);

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
	      tree dt = build_address(cd2->toSymbol()->Stree);
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
  sle->toDt(pdt);
}

/* ================================================================ */

// Generate the data for the default initialiser of the type.

dt_t **
Type::toDt (dt_t **pdt)
{
  Expression *e = defaultInit();
  return e->toDt (pdt);
}

dt_t **
TypeVector::toDt (dt_t **pdt)
{
  gcc_assert (basetype->ty == Tsarray);
  return ((TypeSArray *) basetype)->toDtElem (pdt, NULL);
}

dt_t **
TypeSArray::toDt (dt_t **pdt)
{
  return toDtElem (pdt, NULL);
}

dt_t **
TypeSArray::toDtElem (dt_t **pdt, Expression *e)
{
  dinteger_t len = dim->toInteger();

  if (len)
    {
      tree dt = NULL_TREE;
      Type *tnext = next;
      Type *tbn = tnext->toBasetype();
      if (tbn->ty == Tvector)
	tbn = ((TypeVector *) tbn)->basetype;

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
	    e->toDt (&dt);

	  // Single initialiser already constructed, just chain onto pdt.
	  if (len == 1)
	    return dt_chainon (pdt, dt);
	}
      else
	{
	  // If not already supplied use default initialiser.
	  if (!e)
	    e = tnext->defaultInit();

	  for (size_t i = 0; i < len; i++)
	    {
	      if (tbn->ty == Tsarray)
		((TypeSArray *) tbn)->toDtElem (&dt, e);
	      else
    		e->toDt (&dt);
	    }
	}

      return dt_container (pdt, this, dt);
    }

  return pdt;
}

dt_t **
TypeStruct::toDt (dt_t **pdt)
{
  sym->toDt (pdt);
  return pdt;
}

/* ================================================================ */

// Verify the runtime TypeInfo sizes.

static void
verify_structsize (ClassDeclaration *typeclass, size_t expected)
{
  if (typeclass->structsize != expected)
    {
       error (typeclass->loc, "mismatch between compiler and object.d or object.di found.");
       gcc_unreachable();
    }
}

// Generate the TypeInfo data layouts.

void
TypeInfoDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::dtypeinfo, 2 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   */
  build_vptr_monitor (pdt, Type::dtypeinfo);
}

void
TypeInfoConstDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoconst, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->mutableOf();
  tm = tm->merge();
  tm->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Const
  build_vptr_monitor (pdt, Type::typeinfoconst);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}

void
TypeInfoInvariantDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoinvariant, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->mutableOf();
  tm = tm->merge();
  tm->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Invariant
  build_vptr_monitor (pdt, Type::typeinfoinvariant);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}

void
TypeInfoSharedDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoshared, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->unSharedOf();
  tm = tm->merge();
  tm->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Shared
  build_vptr_monitor (pdt, Type::typeinfoshared);

  // TypeInfo for unshared type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}

void
TypeInfoWildDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfowild, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->mutableOf();
  tm = tm->merge();
  tm->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Wild
  build_vptr_monitor (pdt, Type::typeinfowild);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}


void
TypeInfoEnumDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoenum, 7 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo base;
   *  char[] name;
   *  void[] m_init;
   */
  gcc_assert (tinfo->ty == Tenum);

  TypeEnum *tc = (TypeEnum *) tinfo;
  EnumDeclaration *sd = tc->sym;

  // vtbl and monitor for TypeInfo_Enum
  build_vptr_monitor (pdt, Type::typeinfoenum);

  // TypeInfo for enum members.
  if (sd->memtype)
    {
      sd->memtype->genTypeInfo(NULL);
      dt_cons (pdt, build_address (sd->memtype->vtinfo->toSymbol()->Stree));
    }
  else
    dt_cons (pdt, null_pointer_node);

  // Name of the enum declaration.
  dt_cons (pdt, d_array_string (sd->toPrettyChars()));

  // Default initialiser for enum.
  tree tarray = Type::tvoid->arrayOf()->toCtype();
  if (!sd->members || tinfo->isZeroInit())
    {
      // zero initialiser, or the same as the base type.
      dt_cons (pdt, d_array_value (tarray, size_int (0), null_pointer_node));
    }
  else
    {
      tree dt = build_address (sd->toInitializer()->Stree);
      dt_cons (pdt, d_array_value (tarray, size_int (sd->type->size()), dt));
    }
}

void
TypeInfoPointerDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfopointer, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo m_next;
   */
  gcc_assert (tinfo->ty == Tpointer);

  TypePointer *tc = (TypePointer *) tinfo;
  tc->next->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Pointer
  build_vptr_monitor (pdt, Type::typeinfopointer);

  // TypeInfo for pointer-to type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));
}

void
TypeInfoArrayDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoarray, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo value;
   */
  gcc_assert (tinfo->ty == Tarray);

  TypeDArray *tc = (TypeDArray *) tinfo;
  tc->next->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Array
  build_vptr_monitor (pdt, Type::typeinfoarray);

  // TypeInfo for array of type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));
}

void
TypeInfoStaticArrayDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfostaticarray, 4 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo value;
   *  size_t len;
   */
  gcc_assert (tinfo->ty == Tsarray);

  TypeSArray *tc = (TypeSArray *) tinfo;
  tc->next->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_StaticArray
  build_vptr_monitor (pdt, Type::typeinfostaticarray);

  // TypeInfo for array of type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));

  // Static array length.
  dt_cons (pdt, size_int (tc->dim->toInteger()));
}

void
TypeInfoVectorDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfovector, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo base;
   */
  gcc_assert (tinfo->ty == Tvector);

  TypeVector *tc = (TypeVector *) tinfo;
  tc->basetype->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Vector
  build_vptr_monitor (pdt, Type::typeinfovector);

  // TypeInfo for equivalent static array.
  dt_cons (pdt, build_address (tc->basetype->vtinfo->toSymbol()->Stree));
}

void
TypeInfoAssociativeArrayDeclaration::toDt (dt_t  **pdt)
{
  verify_structsize (Type::typeinfoassociativearray, 4 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo value;
   *  TypeInfo key;
   *  TypeInfo impl;
   */
  gcc_assert (tinfo->ty == Taarray);

  TypeAArray *tc = (TypeAArray *) tinfo;
  tc->next->genTypeInfo(NULL);
  tc->index->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_AssociativeArray
  build_vptr_monitor (pdt, Type::typeinfoassociativearray);

  // TypeInfo for value of type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));

  // TypeInfo for index of type.
  dt_cons (pdt, build_address (tc->index->vtinfo->toSymbol()->Stree));
}

void
TypeInfoFunctionDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfofunction, 5 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   *  string deco;
   */
  gcc_assert (tinfo->ty == Tfunction);
  gcc_assert (tinfo->deco);

  TypeFunction *tc = (TypeFunction *) tinfo;
  tc->next->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Function
  build_vptr_monitor (pdt, Type::typeinfofunction);

  // TypeInfo for function return value.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));

  // Mangled name of function declaration.
  dt_cons (pdt, d_array_string (tinfo->deco));
}

void
TypeInfoDelegateDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfodelegate, 5 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   *  string deco;
   */
  gcc_assert (tinfo->ty == Tdelegate);
  gcc_assert (tinfo->deco);

  TypeDelegate *tc = (TypeDelegate *) tinfo;
  tc->next->nextOf()->genTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Delegate
  build_vptr_monitor (pdt, Type::typeinfodelegate);

  // TypeInfo for delegate return value.
  dt_cons (pdt, build_address (tc->next->nextOf()->vtinfo->toSymbol()->Stree));

  // Mangled name of delegate declaration.
  dt_cons (pdt, d_array_string (tinfo->deco));
}

void
TypeInfoStructDeclaration::toDt (dt_t **pdt)
{
  if (global.params.is64bit)
    verify_structsize (Type::typeinfostruct, 17 * Target::ptrsize);
  else
    verify_structsize (Type::typeinfostruct, 15 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  char[] name;
   *  void[] init;
   *  hash_t function(in void*) xtoHash;
   *  bool function(in void*, in void*) xopEquals;
   *  int function(in void*, in void*) xopCmp;
   *  string function(const(void)*) xtoString;
   *  StructFlags m_flags;
   *  xdtor;
   *  xpostblit;
   *  uint m_align;
   *  version (X86_64)
   *      TypeInfo m_arg1;
   *      TypeInfo m_arg2;
   *  xgetRTInfo
   */
  gcc_assert (tinfo->ty == Tstruct);

  TypeStruct *tc = (TypeStruct *) tinfo;
  StructDeclaration *sd = tc->sym;

  // vtbl and monitor for TypeInfo_Struct
  build_vptr_monitor (pdt, Type::typeinfostruct);

  if (!sd->members)
    return;

  // Name of the struct declaration.
  dt_cons (pdt, d_array_string (sd->toPrettyChars()));

  // Default initialiser for struct.
  dt_cons (pdt, size_int (sd->structsize));
  if (sd->zeroInit)
    dt_cons (pdt, null_pointer_node);
  else
    dt_cons (pdt, build_address (sd->toInitializer()->Stree));

  // hash_t function(in void*) xtoHash;
  FuncDeclaration *fdx = sd->xhash;
  if (fdx)
    {
      TypeFunction *tf = (TypeFunction *) fdx->type;
      gcc_assert(tf->ty == Tfunction);

      dt_cons (pdt, build_address (fdx->toSymbol()->Stree));

      if (!tf->isnothrow || tf->trust == TRUSTsystem)
	warning (fdx->loc, "toHash() must be declared as extern (D) size_t toHash() const nothrow @safe, not %s", tf->toChars());
    }
  else
    dt_cons (pdt, null_pointer_node);

  // bool function(in void*, in void*) xopEquals;
  if (sd->xeq)
    dt_cons (pdt, build_address (sd->xeq->toSymbol()->Stree));
  else
    dt_cons (pdt, null_pointer_node);

  // int function(in void*, in void*) xopCmp;
  if (sd->xcmp)
    dt_cons (pdt, build_address (sd->xcmp->toSymbol()->Stree));
  else
    dt_cons (pdt, null_pointer_node);

  // string function(const(void)*) xtoString;
  fdx = search_toString(sd);
  if (fdx)
    dt_cons (pdt, build_address (fdx->toSymbol()->Stree));
  else
    dt_cons (pdt, null_pointer_node);

  // uint m_flags;
  // StructFlags::Type m_flags;
  StructFlags::Type m_flags = 0;

  if (tc->hasPointers())
    m_flags |= StructFlags::hasPointers;

  dt_cons (pdt, size_int (m_flags));

  // xdtor
  if (sd->dtor)
    dt_cons (pdt, build_address (sd->dtor->toSymbol()->Stree));
  else
    dt_cons (pdt, null_pointer_node);

  // xpostblit
  if (sd->postblit && !(sd->postblit->storage_class & STCdisable))
    dt_cons (pdt, build_address (sd->postblit->toSymbol()->Stree));
  else
    dt_cons (pdt, null_pointer_node);

  // uint m_align;
  dt_cons (pdt, size_int (tc->alignsize()));

  if (global.params.is64bit)
    {
      // TypeInfo m_arg1;
      if (sd->arg1type)
	{
	  sd->arg1type->genTypeInfo(NULL);
	  dt_cons (pdt, build_address (sd->arg1type->vtinfo->toSymbol()->Stree));
	}
      else
	dt_cons (pdt, null_pointer_node);

      // TypeInfo m_arg2;
      if (sd->arg2type)
	{
	  sd->arg2type->genTypeInfo(NULL);
	  dt_cons (pdt, build_address (sd->arg2type->vtinfo->toSymbol()->Stree));
	}
      else
	dt_cons (pdt, null_pointer_node);
    }

  // xgetRTInfo
  if (sd->getRTInfo)
    sd->getRTInfo->toDt (pdt);
  else
    {
      // If struct has pointers.
      if (m_flags & StructFlags::hasPointers)
	dt_cons (pdt, size_int (1));
      else
	dt_cons (pdt, size_int (0));
    }
}

void
TypeInfoClassDeclaration::toDt (dt_t **)
{
  internal_error ("TypeInfoClassDeclaration::toDt called.");
  gcc_unreachable();
}

void
TypeInfoInterfaceDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfointerface, 3 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  ClassInfo info;
   */
  gcc_assert (tinfo->ty == Tclass);

  // vtbl and monitor for TypeInfo_Interface
  build_vptr_monitor (pdt, Type::typeinfointerface);

  TypeClass *tc = (TypeClass *) tinfo;
  if (!tc->sym->vclassinfo)
    tc->sym->vclassinfo = new TypeInfoClassDeclaration (tc);

  Symbol *s = tc->sym->vclassinfo->toSymbol();
  dt_cons (pdt, build_address (s->Stree));
}

void
TypeInfoTupleDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfotypelist, 4 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo[] elements;
   */
  gcc_assert (tinfo->ty == Ttuple);

  // vtbl and monitor for TypeInfo_Tuple
  build_vptr_monitor (pdt, Type::typeinfotypelist);

  TypeTuple *tu = (TypeTuple *) tinfo;
  tree dt = NULL_TREE;

  for (size_t i = 0; i < tu->arguments->dim; i++)
    {
      Parameter *arg = (*tu->arguments)[i];
      Expression *e = arg->type->getTypeInfo(NULL);
      e = e->optimize (WANTvalue);
      e->toDt (&dt);
    }

  Symbol *s = new Symbol();
  s->Sdt = dt;
  d_finish_symbol (s);

  // TypeInfo[] elements;
  dt_cons (pdt, size_int (tu->arguments->dim));
  dt_cons (pdt, build_address (s->Stree));
}

