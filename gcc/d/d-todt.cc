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

#include "scope.h"
#include "enum.h"
#include "id.h"
#include "init.h"

typedef ArrayBase<dt_t> Dts;


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
	      size_t sz = int_size_in_bytes (TREE_TYPE (TREE_VALUE (fdt)));
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
      d_finalize_symbol (s);
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
  verify_structsize (Type::typeinfo, 2 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   */
  build_dt_vtable (pdt, Type::typeinfo);
}

void
TypeInfoConstDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoconst, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->mutableOf();
  tm = tm->merge();
  tm->getTypeInfo (NULL);

  // vtbl and monitor for TypeInfo_Const
  build_dt_vtable (pdt, Type::typeinfoconst);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}

void
TypeInfoInvariantDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoinvariant, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->mutableOf();
  tm = tm->merge();
  tm->getTypeInfo (NULL);

  // vtbl and monitor for TypeInfo_Invariant
  build_dt_vtable (pdt, Type::typeinfoinvariant);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}

void
TypeInfoSharedDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoshared, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->unSharedOf();
  tm = tm->merge();
  tm->getTypeInfo (NULL);

  // vtbl and monitor for TypeInfo_Shared
  build_dt_vtable (pdt, Type::typeinfoshared);

  // TypeInfo for unshared type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}

void
TypeInfoWildDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfowild, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo next;
   */
  Type *tm = tinfo->mutableOf();
  tm = tm->merge();
  tm->getTypeInfo (NULL);

  // vtbl and monitor for TypeInfo_Wild
  build_dt_vtable (pdt, Type::typeinfowild);

  // TypeInfo for mutable type.
  dt_cons (pdt, build_address (tm->vtinfo->toSymbol()->Stree));
}


void
TypeInfoTypedefDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfotypedef, 7 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfotypedef);

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
  verify_structsize (Type::typeinfoenum, 7 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfoenum);

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
  verify_structsize (Type::typeinfopointer, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo m_next;
   */
  gcc_assert (tinfo->ty == Tpointer);

  TypePointer *tc = (TypePointer *) tinfo;
  tc->next->getTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Pointer
  build_dt_vtable (pdt, Type::typeinfopointer);

  // TypeInfo for pointer-to type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));
}

void
TypeInfoArrayDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfoarray, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo value;
   */
  gcc_assert (tinfo->ty == Tarray);

  TypeDArray *tc = (TypeDArray *) tinfo;
  tc->next->getTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Array
  build_dt_vtable (pdt, Type::typeinfoarray);

  // TypeInfo for array of type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));
}

void
TypeInfoStaticArrayDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfostaticarray, 4 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfostaticarray);

  // TypeInfo for array of type.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));

  // Static array length.
  dt_cons (pdt, size_int (tc->dim->toInteger()));
}

void
TypeInfoVectorDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfovector, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo base;
   */
  gcc_assert (tinfo->ty == Tvector);

  TypeVector *tc = (TypeVector *) tinfo;
  tc->basetype->getTypeInfo(NULL);

  // vtbl and monitor for TypeInfo_Vector
  build_dt_vtable (pdt, Type::typeinfovector);

  // TypeInfo for equivalent static array.
  dt_cons (pdt, build_address (tc->basetype->vtinfo->toSymbol()->Stree));
}

void
TypeInfoAssociativeArrayDeclaration::toDt (dt_t  **pdt)
{
  verify_structsize (Type::typeinfoassociativearray, 5 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfoassociativearray);

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
  verify_structsize (Type::typeinfofunction, 5 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfofunction);

  // TypeInfo for function return value.
  dt_cons (pdt, build_address (tc->next->vtinfo->toSymbol()->Stree));

  // Mangled name of function declaration.
  dt_cons (pdt, d_array_string (tinfo->deco));
}

void
TypeInfoDelegateDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfodelegate, 5 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfodelegate);

  // TypeInfo for delegate return value.
  dt_cons (pdt, build_address (tc->next->nextOf()->vtinfo->toSymbol()->Stree));

  // Mangled name of delegate declaration.
  dt_cons (pdt, d_array_string (tinfo->deco));
}

void
TypeInfoStructDeclaration::toDt (dt_t **pdt)
{
  if (global.params.is64bit)
    verify_structsize (Type::typeinfostruct, 17 * PTRSIZE);
  else
    verify_structsize (Type::typeinfostruct, 15 * PTRSIZE);

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
  build_dt_vtable (pdt, Type::typeinfostruct);

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

  // %% FIXME: is64bit does not mean X86_64.
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
  verify_structsize (Type::typeinfointerface, 3 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  ClassInfo info;
   */
  gcc_assert (tinfo->ty == Tclass);

  // vtbl and monitor for TypeInfo_Interface
  build_dt_vtable (pdt, Type::typeinfointerface);

  TypeClass *tc = (TypeClass *) tinfo;
  if (!tc->sym->vclassinfo)
    tc->sym->vclassinfo = new TypeInfoClassDeclaration (tc);

  Symbol *s = tc->sym->vclassinfo->toSymbol();
  dt_cons (pdt, build_address (s->Stree));
}

void
TypeInfoTupleDeclaration::toDt (dt_t **pdt)
{
  verify_structsize (Type::typeinfotypelist, 4 * PTRSIZE);

  /* Put out:
   *  void **vptr;
   *  monitor_t monitor;
   *  TypeInfo[] elements;
   */
  gcc_assert (tinfo->ty == Ttuple);

  // vtbl and monitor for TypeInfo_Tuple
  build_dt_vtable (pdt, Type::typeinfotypelist);

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
  d_finalize_symbol (s);

  // TypeInfo[] elements;
  dt_cons (pdt, size_int (tu->arguments->dim));
  dt_cons (pdt, build_address (s->Stree));
}

