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

#include "enum.h"
#include "id.h"
#include "init.h"
#include "scope.h"
#include "dfrontend/target.h"

typedef ArrayBase<dt_t> Dts;


// Append VAL to constructor PDT.  Create a new constructor
// of generic type if PDT is not already pointing to one.

dt_t **
dt_cons (dt_t **pdt, tree val)
{
  if (*pdt == NULL_TREE)
    *pdt = build_constructor (d_unknown_type_node, NULL);

  CONSTRUCTOR_APPEND_ELT (CONSTRUCTOR_ELTS (*pdt), 0, val);
  return pdt;
}

// Concatenate two constructors of dt_t nodes by appending all
// values of DT to PDT.

dt_t **
dt_chainon (dt_t **pdt, dt_t *dt)
{
  vec<constructor_elt, va_gc> *elts = CONSTRUCTOR_ELTS (dt);
  tree value;
  size_t i;

  gcc_assert (*pdt != dt);

  FOR_EACH_CONSTRUCTOR_VALUE (elts, i, value)
    dt_cons (pdt, value);

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
// ad-hoc type from dtvector_to_tree is fine.  It must still
// be a CONSTRUCTOR, or the CCP pass may use it incorrectly.

static tree
dt_container2 (dt_t *dt)
{
  // Generate type on the fly
  vec<constructor_elt, va_gc> *elts = NULL;
  tree fields = NULL_TREE;

  tree aggtype = make_node (RECORD_TYPE);
  tree offset = size_zero_node;

  if (dt != NULL_TREE)
    {
      tree value;
      size_t i;

      FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (dt), i, value)
	{
	  tree field = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE, TREE_TYPE (value));
	  tree size = TYPE_SIZE_UNIT (TREE_TYPE (value));

	  DECL_CONTEXT (field) = aggtype;
	  DECL_FIELD_OFFSET (field) = offset;
	  DECL_FIELD_BIT_OFFSET (field) = bitsize_zero_node;
	  SET_DECL_OFFSET_ALIGN (field, TYPE_ALIGN (TREE_TYPE (value)));
	  DECL_ARTIFICIAL (field) = 1;
	  DECL_IGNORED_P (field) = 1;

	  layout_decl (field, 0);
	  fields = chainon (fields, field);
	  CONSTRUCTOR_APPEND_ELT (elts, field, value);
	  offset = size_binop (PLUS_EXPR, offset, size);
	}
    }
  else
    dt = build_constructor (aggtype, NULL);

  TYPE_FIELDS (aggtype) = fields;
  TYPE_SIZE (aggtype) = size_binop (MULT_EXPR, offset, size_int (BITS_PER_UNIT));
  TYPE_SIZE_UNIT (aggtype) = offset;
  compute_record_mode (aggtype);

  TREE_TYPE (dt) = aggtype;
  CONSTRUCTOR_ELTS (dt) = elts;
  TREE_READONLY (dt) = 1;
  TREE_STATIC (dt) = 1;
  TREE_CONSTANT (dt) = 1;

  return dt;
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
      vec<constructor_elt, va_gc> *elts = NULL;
      tree value;
      size_t i;

      gcc_assert (CONSTRUCTOR_NELTS (dt) == tsa->dim->toInteger());

      FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (dt), i, value)
	CONSTRUCTOR_APPEND_ELT (elts, size_int (i), value);

      TREE_TYPE (dt) = type->toCtype();
      CONSTRUCTOR_ELTS (dt) = elts;
      TREE_CONSTANT (dt) = 1;
      TREE_READONLY (dt) = 1;
      TREE_STATIC (dt) = 1;

      return dt_cons (pdt, dt);
    }
  else if (tb->ty == Tstruct)
    {
      dt = dt_container2 (dt);
      TREE_TYPE (dt) = type->toCtype();
      return dt_cons (pdt, dt);
    }

  return dt_cons (pdt, dtvector_to_tree (dt));
}

// Return a new CONSTRUCTOR whose values are in a dt_t
// list pointed to by DT.

tree
dtvector_to_tree (dt_t *dt)
{
  if (dt && CONSTRUCTOR_NELTS (dt) == 1)
    return CONSTRUCTOR_ELT (dt, 0)->value;

  return dt_container2 (dt);
}

// Put out __vptr and __monitor of class CD into PDT.

dt_t **
build_vptr_monitor (dt_t **pdt, ClassDeclaration *cd)
{
  gcc_assert (cd != NULL);
  Symbol *s = cd->toVtblSymbol();
  dt_cons (pdt, build_address (s->Stree));
  dt_cons (pdt, size_int (0));
  return pdt;
}

/* ================================================================ */

// Build constructors for front-end Initialisers to be written to data segment.

dt_t *
Initializer::toDt (void)
{
  gcc_unreachable();
  return NULL_TREE;
}

dt_t *
VoidInitializer::toDt (void)
{
  // void initialisers are set to 0, just because we need something
  // to set them to in the static data segment.
  tree dt = NULL_TREE;
  dt_cons (&dt, build_constructor (type->toCtype(), NULL));
  return dt;
}

dt_t *
StructInitializer::toDt (void)
{
  Dts dts;
  dts.setDim (ad->fields.dim);
  dts.zero();

  for (size_t i = 0; i < vars.dim; i++)
    {
      VarDeclaration *v = vars[i];
      Initializer *val = value[i];

      for (size_t j = 0; true; j++)
	{
	  gcc_assert (j < dts.dim);

	  if (ad->fields[j] == v)
	    {
	      if (dts[j])
		error (loc, "field %s of %s already initialized", v->toChars(), ad->toChars());
	      dts[j] = val->toDt();
	      break;
	    }
	}
    }

  size_t offset = 0;
  tree sdt = NULL_TREE;

  for (size_t i = 0; i < dts.dim; i++)
    {
      VarDeclaration *v = ad->fields[i];
      tree fdt = dts[i];

      if (fdt == NULL_TREE)
	{
	  // An instance specific initialiser was not provided.
	  // Look to see if there's a default initialiser from the
	  // struct definition
	  if (v->init)
	    {
	      if (!v->init->isVoidInitializer())
		fdt = v->init->toDt();
	    }
	  else if (v->offset >= offset)
	    {
	      size_t offset2 = v->offset + v->type->size();

	      // Make sure this field does not overlap any explicitly
	      // initialized field.
	      for (size_t j = i + 1; true; j++)
		{
		  // Didn't find any overlap.
		  if (j == dts.dim)
		    {
		      v->type->toDt (&fdt);
		      break;
		    }

		  VarDeclaration *v2 = ad->fields[j];

		  // Overlap.
		  if (v2->offset < offset2 && dts[j])
		    break;
		}
	    }
	}

      if (fdt != NULL_TREE)
	{
	  if (v->offset < offset)
	    error (loc, "duplicate union initialization for %s", v->toChars());
	  else
	    {
	      size_t sz = int_size_in_bytes (TREE_TYPE (CONSTRUCTOR_ELT (fdt, 0)->value));
	      size_t vsz = v->type->size();
	      size_t voffset = v->offset;
	      size_t dim = 1;

	      if (sz > vsz)
		{
		  gcc_assert (v->type->ty == Tsarray && vsz == 0);
		  error (loc, "zero length array %s has non-zero length initializer", v->toChars());
		}

	      for (Type *vt = v->type->toBasetype();
		   vt->ty == Tsarray; vt = vt->nextOf()->toBasetype())
		{
		  TypeSArray *tsa = (TypeSArray *) vt;
		  dim *= tsa->dim->toInteger();
		}

	      gcc_assert (sz == vsz || sz * dim <= vsz);

	      for (size_t i = 0; i < dim; i++)
		{
		  if (offset < voffset)
		    dt_zeropad (&sdt, voffset - offset);

		  if (fdt == NULL_TREE)
		    {
		      if (v->init)
			fdt = v->init->toDt();
		      else
			v->type->toDt (&fdt);
		    }

		  dt_chainon (&sdt, fdt);
		  fdt = NULL_TREE;

		  offset = voffset + sz;
		  voffset += vsz / dim;
		  if (sz == vsz)
		    break;
		}
	    }
	}
    }

  if (offset < ad->structsize)
    dt_zeropad (&sdt, ad->structsize - offset);

  tree cdt = NULL_TREE;
  dt_container (&cdt, ad->type, sdt);
  return cdt;
}

dt_t *
ArrayInitializer::toDt (void)
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
	error(loc, "duplicate initializations for index %d", length);
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
ExpInitializer::toDt (void)
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
  // For elements[], construct a corresponding array dts[] the elements
  // of which are the initializers.
  // Nulls in elements[] become nulls in dts[].
  Dts dts;
  dts.setDim (sd->fields.dim);
  dts.zero();

  gcc_assert (elements->dim <= sd->fields.dim);

  for (size_t i = 0; i < elements->dim; i++)
    {
      Expression *e = (*elements)[i];
      if (!e)
	continue;
      tree dt = NULL_TREE;
      e->toDt (&dt);
      dts[i] = dt;
    }

  size_t offset = 0;
  tree sdt = NULL_TREE;

  for (size_t i = 0; i < dts.dim; i++)
    {
      VarDeclaration *v = sd->fields[i];
      tree fdt = dts[i];

      if (fdt == NULL_TREE)
	{
	  // An instance specific initialiser was not provided.
	  // If there is no overlap with any explicit initialiser in dts[],
	  // supply a default initialiser.
	  if (v->offset >= offset)
	    {
	      size_t offset2 = v->offset + v->type->size();

	      for (size_t j = i + 1; 1; j++)
		{
		  // Didn't find any overlap
		  if (j == dts.dim)
		    {
		      // Set fdt to be the default initialiser
		      if (v->init)
			fdt = v->init->toDt();
		      else
			v->type->toDt (&fdt);
		      break;
		    }

		  VarDeclaration *v2 = sd->fields[j];

		  // Overlap
		  if (v2->offset < offset2 && dts[j])
		    break;
		}
	    }
	}

      if (fdt != NULL_TREE)
	{
	  if (v->offset < offset)
	    error ("duplicate union initialization for %s", v->toChars());
	  else
	    {
	      size_t sz = int_size_in_bytes (TREE_TYPE (CONSTRUCTOR_ELT (fdt, 0)->value));
	      size_t vsz = v->type->size();
	      size_t voffset = v->offset;
	      size_t dim = 1;

	      if (sz > vsz)
		{
		  gcc_assert (v->type->ty == Tsarray && vsz == 0);
		  error ("zero length array %s has non-zero length initializer", v->toChars());
		}

	      for (Type *vt = v->type->toBasetype();
		   vt->ty == Tsarray; vt = vt->nextOf()->toBasetype())
		{
		  TypeSArray *tsa = (TypeSArray *) vt;
		  dim *= tsa->dim->toInteger();
		}

	      gcc_assert (sz == vsz || sz * dim <= vsz);

	      for (size_t i = 0; i < dim; i++)
		{
		  if (offset < voffset)
		    dt_zeropad (&sdt, voffset - offset);

		  if (fdt == NULL_TREE)
		    {
		      if (v->init)
			fdt = v->init->toDt();
		      else
			v->type->toDt (&fdt);
		    }

		  dt_chainon (&sdt, fdt);
		  fdt = NULL_TREE;

		  offset = voffset + sz;
		  voffset += vsz / dim;
		  if (sz == vsz)
		    break;
		}
	    }
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

/* ================================================================ */

// Generate the data for the static initialiser.

void
ClassDeclaration::toDt (dt_t **pdt)
{
  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   */
  build_vptr_monitor (pdt, this);

  // Put out rest of class fields.
  toDt2 (pdt, this);
}

void
ClassDeclaration::toDt2 (dt_t **pdt, ClassDeclaration *cd)
{
  size_t offset;

  if (baseClass)
    {
      baseClass->toDt2 (pdt, cd);
      offset = baseClass->structsize;
    }
  else
    offset = Target::ptrsize * 2;

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
		((TypeSArray *) tb)->toDtElem (&dt, ei->exp);
	      else
		dt = init->toDt();
	    }
	}
      else if (v->offset >= offset)
	v->type->toDt (&dt);

      if (dt != NULL_TREE)
	{
	  if (v->offset < offset)
	    error ("duplicated union initialization for %s", v->toChars());
	  else
	    {
	      if (offset < v->offset)
		dt_zeropad (pdt, v->offset - offset);
	      dt_chainon (pdt, dt);
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
	  gcc_assert (cd2);
	  unsigned csymoffset = cd2->baseVtblOffset (b);
	  if (csymoffset != (unsigned) ~0)
	    {
	      tree dt = build_address (cd2->toSymbol()->Stree);
	      if (offset < (size_t) b->offset)
		dt_zeropad (pdt, b->offset - offset);
	      dt_cons (pdt, build_offset (dt, size_int (csymoffset)));
	      break;
	    }
	}

      offset = b->offset + Target::ptrsize;
    }

  if (offset < structsize)
    dt_zeropad (pdt, structsize - offset);
}

void
StructDeclaration::toDt (dt_t **pdt)
{
  size_t offset = 0;
  tree sdt = NULL_TREE;

  // Note equivalence of this loop to class's
  for (size_t i = 0; i < fields.dim; i++)
    {
      size_t vsize;
      VarDeclaration *v = fields[i];
      tree dt = NULL_TREE;

      if (v->storage_class & STCref)
	{
	  vsize = Target::ptrsize;
	  if (v->offset >= offset)
	    dt_zeropad (&dt, vsize);
	}
      else
	{
	  vsize = v->type->size();
	  Initializer *init = v->init;
	  if (init)
	    {
	      ExpInitializer *ei = init->isExpInitializer();
	      Type *tb = v->type->toBasetype();
	      if (!init->isVoidInitializer())
		{
		  if (ei && tb->ty == Tsarray)
		    ((TypeSArray *) tb)->toDtElem (&dt, ei->exp);
		  else
		    dt = init->toDt();
		}
	    }
	  else if (v->offset >= offset)
	    v->type->toDt (&dt);
	}

      if (dt != NULL_TREE)
	{
	  if (v->offset < offset)
	    error("overlapping initialization for struct %s.%s", toChars(), v->toChars());
	  else
	    {
	      if (offset < v->offset)
		dt_zeropad (&sdt, v->offset - offset);
	      dt_chainon (&sdt, dt);
	      offset = v->offset + vsize;
	    }
	}
    }

  if (offset < structsize)
    dt_zeropad (&sdt, structsize - offset);

  dt_container (pdt, type, sdt);
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
      Type *tnext = next;
      Type *tbn = tnext->toBasetype();
      tree dt = NULL_TREE;

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

dt_t **
TypeTypedef::toDt (dt_t **pdt)
{
  if (sym->init)
    dt_chainon (pdt, sym->init->toDt());
  else
    sym->basetype->toDt (pdt);

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
  verify_structsize (Type::typeinfo, 2 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   */
  build_vptr_monitor (pdt, Type::typeinfo);
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
  tm->getTypeInfo (NULL);

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
  tm->getTypeInfo (NULL);

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
  tm->getTypeInfo (NULL);

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
  tm->getTypeInfo (NULL);

  // vtbl and monitor for TypeInfo_Wild
  build_vptr_monitor (pdt, Type::typeinfowild);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}


void
TypeInfoTypedefDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfotypedef, 7 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo base;
   *  char[] name;
   *  void[] m_init;
   */
  gcc_assert (tinfo->ty == Ttypedef);

  TypedefDeclaration *sd = ((TypeTypedef *) tinfo)->sym;
  sd->basetype = sd->basetype->merge();
  // Generate vtinfo.
  sd->basetype->getTypeInfo (NULL);
  gcc_assert (sd->basetype->vtinfo);

  // vtbl and monitor for TypeInfo_Typedef
  build_vptr_monitor (pdt, Type::typeinfotypedef);

  // Typeinfo for basetype.
  dt_cons (pdt, build_address (sd->basetype->vtinfo->toSymbol()->Stree));

  // Name of the typedef declaration.
  dt_cons (pdt, d_array_string (sd->toPrettyChars()));

  // Default initialiser for typedef.
  tree tarray = Type::tvoid->arrayOf()->toCtype();
  if (tinfo->isZeroInit() || !sd->init)
    dt_cons (pdt, d_array_value (tarray, size_int (0), d_null_pointer));
  else
    {
      tree sinit = build_address (sd->toInitializer()->Stree);
      dt_cons (pdt, d_array_value (tarray, size_int (sd->type->size()), sinit));
    }
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
      sd->memtype->getTypeInfo(NULL);
      dt_cons (pdt, build_address (sd->memtype->vtinfo->toSymbol()->Stree));
    }
  else
    dt_cons (pdt, d_null_pointer);

  // Name of the enum declaration.
  dt_cons (pdt, d_array_string (sd->toPrettyChars()));

  // Default initialiser for enum.
  tree tarray = Type::tvoid->arrayOf()->toCtype();
  if (!sd->defaultval || tinfo->isZeroInit())
    {
      // zero initialiser, or the same as the base type.
      dt_cons (pdt, d_array_value (tarray, size_int (0), d_null_pointer));
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
  tc->next->getTypeInfo(NULL);

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
  tc->next->getTypeInfo(NULL);

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
  tc->next->getTypeInfo(NULL);

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
  tc->basetype->getTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Vector
  build_vptr_monitor (pdt, Type::typeinfovector);

  // TypeInfo for equivalent static array.
  dt_cons (pdt, build_address (tc->basetype->vtinfo->toSymbol()->Stree));
}

void
TypeInfoAssociativeArrayDeclaration::toDt (dt_t  **pdt)
{
  verify_structsize (Type::typeinfoassociativearray, 5 * Target::ptrsize);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo value;
   *  TypeInfo key;
   *  TypeInfo impl;
   */
  gcc_assert (tinfo->ty == Taarray);

  TypeAArray *tc = (TypeAArray *) tinfo;
  tc->next->getTypeInfo(NULL);
  tc->index->getTypeInfo(NULL);
  tc->getImpl()->type->getTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_AssociativeArray
  build_vptr_monitor (pdt, Type::typeinfoassociativearray);

  // TypeInfo for value of type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));

  // TypeInfo for index of type.
  dt_cons (pdt, build_address (tc->index->vtinfo->toSymbol()->Stree));

  // TypeInfo impl;
  dt_cons (pdt, build_address (tc->getImpl()->type->vtinfo->toSymbol()->Stree));
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
  tc->next->getTypeInfo(NULL);

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
  tc->next->nextOf()->getTypeInfo(NULL);

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
   *  uint m_flags;
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

  // Name of the struct declaration.
  dt_cons (pdt, d_array_string (sd->toPrettyChars()));

  // Default initialiser for struct.
  dt_cons (pdt, size_int (sd->structsize));
  if (sd->zeroInit)
    dt_cons (pdt, d_null_pointer);
  else
    dt_cons (pdt, build_address (sd->toInitializer()->Stree));

  // hash_t function(in void*) xtoHash;
  Dsymbol *s = search_function(sd, Id::tohash);
  FuncDeclaration *fdx = s ? s->isFuncDeclaration() : NULL;
  if (fdx)
    {
      static TypeFunction *tftohash;
      if (!tftohash)
	{
	  Scope sc;
	  // const hash_t toHash();
	  tftohash = new TypeFunction (NULL, Type::thash_t, 0, LINKd);
	  tftohash->mod = MODconst;
	  tftohash = (TypeFunction *) tftohash->semantic (0, &sc);
	}

      FuncDeclaration *fd = fdx->overloadExactMatch(tftohash);
      if (fd)
	{
	  dt_cons (pdt, build_address (fd->toSymbol()->Stree));
	  TypeFunction *tf = (TypeFunction *) fd->type;
	  gcc_assert(tf->ty == Tfunction);

	  if (!tf->isnothrow || tf->trust == TRUSTsystem)
	    warning(fd->loc, "toHash() must be declared as extern (D) size_t toHash() const nothrow @safe, not %s", tf->toChars());
	}
      else
	dt_cons (pdt, d_null_pointer);
    }
  else
    dt_cons (pdt, d_null_pointer);

  // bool function(in void*, in void*) xopEquals;
  if (sd->xeq)
    dt_cons (pdt, build_address (sd->xeq->toSymbol()->Stree));
  else
    dt_cons (pdt, d_null_pointer);

  // int function(in void*, in void*) xopCmp;
  s = search_function(sd, Id::cmp);
  fdx = s ? s->isFuncDeclaration() : NULL;
  if (fdx)
    {
      Scope sc;

      // const int opCmp(ref const KeyType s);
      Parameters *arguments = new Parameters;

      // arg type is ref const T
      Parameter *arg = new Parameter(STCref, tc->constOf(), NULL, NULL);
      arguments->push(arg);

      TypeFunction *tfcmpptr = new TypeFunction(arguments, Type::tint32, 0, LINKd);
      tfcmpptr->mod = MODconst;
      tfcmpptr = (TypeFunction *) tfcmpptr->semantic(0, &sc);

      FuncDeclaration *fd = fdx->overloadExactMatch(tfcmpptr);
      if (fd)
	dt_cons (pdt, build_address (fd->toSymbol()->Stree));
      else
	dt_cons (pdt, d_null_pointer);
    }
  else
    dt_cons (pdt, d_null_pointer);

  // string function(const(void)*) xtoString;
  s = search_function(sd, Id::tostring);
  fdx = s ? s->isFuncDeclaration() : NULL;
  if (fdx)
    {
      static TypeFunction *tftostring;
      if (!tftostring)
	{
	  Scope sc;
	  // string toString()
	  tftostring = new TypeFunction (NULL, Type::tchar->invariantOf()->arrayOf(), 0, LINKd);
	  tftostring = (TypeFunction *) tftostring->semantic (0, &sc);
	}

      FuncDeclaration *fd = fdx->overloadExactMatch(tftostring);
      if (fd)
	dt_cons (pdt, build_address (fd->toSymbol()->Stree));
      else
	dt_cons (pdt, d_null_pointer);
    }
  else
    dt_cons (pdt, d_null_pointer);

  // uint m_flags;
  dt_cons (pdt, size_int (tc->hasPointers()));

  // xdtor
  if (sd->dtor)
    dt_cons (pdt, build_address (sd->dtor->toSymbol()->Stree));
  else
    dt_cons (pdt, d_null_pointer);

  // xpostblit
  if (sd->postblit && !(sd->postblit->storage_class & STCdisable))
    dt_cons (pdt, build_address (sd->postblit->toSymbol()->Stree));
  else
    dt_cons (pdt, d_null_pointer);

  // uint m_align;
  dt_cons (pdt, size_int (tc->alignsize()));

  if (global.params.is64bit)
    {
      // TypeInfo m_arg1;
      if (sd->arg1type)
	{
	  sd->arg1type->getTypeInfo (NULL);
	  dt_cons (pdt, build_address (sd->arg1type->vtinfo->toSymbol()->Stree));
	}
      else
	dt_cons (pdt, d_null_pointer);

      // TypeInfo m_arg2;
      if (sd->arg2type)
	{
	  sd->arg2type->getTypeInfo (NULL);
	  dt_cons (pdt, build_address (sd->arg2type->vtinfo->toSymbol()->Stree));
	}
      else
	dt_cons (pdt, d_null_pointer);
    }

  // xgetRTInfo
  if (sd->getRTInfo)
    sd->getRTInfo->toDt (pdt);
  else
    {
      // If struct has pointers.
      if (tc->hasPointers())
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
      Expression *e = arg->type->getTypeInfo (NULL);
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

