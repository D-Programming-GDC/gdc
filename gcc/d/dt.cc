// dt.cc -- D frontend for GCC.
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

#include "d-gcc-includes.h"
#include "d-lang.h"
#include "d-codegen.h"
#include "dt.h"

static tree dt2tree_list_of_elems (dt_t * dt);
static tree dt2node (dt_t * dt);


dt_t**
dtval (dt_t** pdt, DT t, dinteger_t i, const void * p)
{
  dt_t * d = new dt_t;
  d->dt = t;
  d->DTnext = 0;
  d->DTint = i;
  d->DTpointer = p;
  return dtcat (pdt, d);
}

dt_t**
dtcat (dt_t** pdt, dt_t * d)
{
  gcc_assert (d);
  // wasted time and mem touching... shortcut DTend field?
  while (*pdt)
    pdt = & (*pdt)->DTnext;
  *pdt = d;
  return & d->DTnext;
}

typedef unsigned bitunit_t;

dt_t**
dtnbits (dt_t** pdt, size_t count, char * pbytes, unsigned unit_size)
{
  gcc_assert (unit_size == sizeof (bitunit_t));
  gcc_assert (count % unit_size == 0);

  bitunit_t * p_unit = (bitunit_t *) pbytes;
  bitunit_t * p_unit_end = (bitunit_t *) (pbytes + count);
  char * pbits = new char[count];
  char * p_out = pbits;
  unsigned b = 0;
  char outv = 0;

  while (p_unit < p_unit_end)
    {
      bitunit_t inv = *p_unit++;

      for (size_t i = 0; i < sizeof (bitunit_t)*8; i++)
	{
	  outv |= ((inv >> i) & 1) << b;
	  if (++b == 8)
	    {
	      *p_out++ = outv;
	      b = 0;
	      outv = 0;
	    }
	}
    }
  gcc_assert ((unsigned) (p_out - pbits) == count);

  return dtnbytes (pdt, count, pbits);
}

dt_t**
dtnwords (dt_t** pdt, size_t word_count, void * pwords, unsigned word_size)
{
  return dtnbytes (pdt, word_count * word_size,
       		   gen.hostToTargetString ((char*) pwords, word_count, word_size));
}

dt_t**
dtawords (dt_t** pdt, size_t word_count, void * pwords, unsigned word_size)
{
  return dtabytes (pdt, TYnptr, 0, word_count * word_size,
       		   gen.hostToTargetString ((char*) pwords, word_count, word_size));
}

/* Add a 32-bit value to a dt.  If pad_to_word is true, adds any
   necessary padding so that the next value is aligned to PTRSIZE. */
dt_t**
dti32 (dt_t** pdt, unsigned val, int pad_to_word)
{
  dt_t** result = dttree (pdt, gen.integerConstant (val, Type::tuns32));
  if (! pad_to_word || PTRSIZE == 4)
    return result;
  else if (PTRSIZE == 8)
    return dttree (result, gen.integerConstant (0, Type::tuns32));
  else
    gcc_unreachable ();
}

dt_t**
dtcontainer (dt_t** pdt, Type * type, dt_t* values)
{
  dt_t * d = new dt_t;
  d->dt = DT_container;
  d->DTnext = 0;
  d->DTtype = type;
  d->DTvalues = values;
  return dtcat (pdt, d);
}


size_t
dt_size (dt_t * dt)
{
  size_t size = 0;

  while (dt)
    {
      switch (dt->dt)
	{
	case DT_azeros:
	case DT_common:
	case DT_nbytes:
	  size += dt->DTint;
	  break;

	case DT_abytes:
	case DT_ibytes:
	case DT_xoff:
	  size += PTRSIZE;
	  break;

	case DT_tree:
	  if (! gen.isErrorMark (dt->DTtree))
	    {
	      tree t_size = TYPE_SIZE_UNIT (TREE_TYPE (dt->DTtree));
	      size += gen.getTargetSizeConst (t_size);
	    }
	  break;

	case DT_container:
	  size += dt_size (dt->DTvalues);
	  break;

	default:
	  gcc_unreachable ();
	}
      dt = dt->DTnext;
    }

  return size;
}

tree
dt2tree (dt_t * dt)
{
  if (dt && /*dt->dt == DT_container || */dt->DTnext == NULL)
    return dt2node (dt);
  else
    return dt2tree_list_of_elems (dt);
}

static tree
dt2node (dt_t * dt)
{
  switch (dt->dt)
    {
    case DT_azeros:
    case DT_common:
	{
	  tree type = gen.arrayType (Type::tuns8, dt->DTint);
	  tree a = build_constructor (type, 0);
	  TREE_READONLY (a) = 1;
	  TREE_CONSTANT (a) = 1;
	  return a;
	}
    case DT_nbytes:
	{
	  tree s = build_string (dt->DTint, (char *) dt->DTpointer);
	  TREE_TYPE (s) = gen.arrayType (Type::tuns8, dt->DTint);
	  return s;
	}
    case DT_abytes:
	{
	  tree s = build_string (dt->DTint, (char *) dt->DTpointer);
	  TREE_TYPE (s) = gen.arrayType (Type::tuns8, dt->DTint);
	  TREE_STATIC (s) = 1;
	  return gen.addressOf (s);
	}
    case DT_ibytes:
	{
	  // %% make sure this is the target word type
	  return gen.integerConstant (dt->DTint, Type::tsize_t);
	}
    case DT_xoff:
	{
	  return gen.pointerOffset (
				    gen.addressOf (check_static_sym (dt->DTsym)),
				    gen.integerConstant (dt->DTint, Type::tsize_t));
	}
    case DT_tree:
	{
	  return dt->DTtree;
	}
    case DT_container:
	{
	  /* It is necessary to give static array data its original
	     type.  Otherwise, the SRA pass will not find the array
	     elements.

	     SRA accesses struct elements by field offset, so the ad
	     hoc type from dt2tree is fine.  It must still be a
	     CONSTRUCTOR, or the CCP pass may use it incorrectly.
	    */
	  Type *tb = NULL;
	  if (dt->DTtype)
	    tb = dt->DTtype->toBasetype ();
	  if (tb && tb->ty == Tsarray)
	    {
	      TypeSArray * tsa = (TypeSArray *) tb;
	      CtorEltMaker ctor_elts;
	      dt_t * dte = dt->DTvalues;
	      size_t i = 0;
	      ctor_elts.reserve (tsa->dim->toInteger ());
	      while (dte)
		{
		  ctor_elts.cons (gen.integerConstant (i++, size_type_node),
      				  dt2node (dte));
		  dte = dte->DTnext;
		}
	      tree ctor = build_constructor (dt->DTtype->toCtype (), ctor_elts.head);
	      // DT data should always be constant.  If the decl is not TREE_CONSTANT, fine.
	      TREE_CONSTANT (ctor) = 1;
	      TREE_READONLY (ctor) = 1;
	      TREE_STATIC (ctor) = 1;
	      return ctor;
	    }
	  else if (tb && tb->ty == Tstruct)
	    return dt2tree_list_of_elems (dt->DTvalues);
	  else
	    return dt2tree (dt->DTvalues);
	}
    default:
      gcc_unreachable ();
    }
  return NULL;
}

static tree
dt2tree_list_of_elems (dt_t * dt)
{
  // Generate type on the fly
  CtorEltMaker elts;
  ListMaker fields;
  tree offset = size_zero_node;

  tree aggtype = make_node (RECORD_TYPE);

  while (dt)
    {
      tree value = dt2node (dt);
      tree field = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE, TREE_TYPE (value));
      tree size = TYPE_SIZE_UNIT (TREE_TYPE (value));
      DECL_CONTEXT (field) = aggtype;
      DECL_FIELD_OFFSET (field) = offset;
      DECL_FIELD_BIT_OFFSET (field) = bitsize_zero_node;
      SET_DECL_OFFSET_ALIGN (field, TYPE_ALIGN (TREE_TYPE (field)));
      DECL_ARTIFICIAL (field) = 1;
      DECL_IGNORED_P (field) = 1;
      layout_decl (field, 0);

      fields.chain (field);
      elts.cons (field, value);

      offset = size_binop (PLUS_EXPR, offset, size);
      dt = dt->DTnext;
    }

  TYPE_FIELDS (aggtype) = fields.head; // or finish_laout
  TYPE_SIZE (aggtype) = convert (bitsizetype,
			 	 size_binop (MULT_EXPR, offset, size_int (BITS_PER_UNIT)));
  TYPE_SIZE_UNIT (aggtype) = offset;
  // okay no alignment -- decl (which has the correct type) should take care of it..
  // align=bits per word?
  compute_record_mode (aggtype);

  // DT data should always be constant.  If the decl is not TREE_CONSTANT, fine.
  tree ctor = build_constructor (aggtype, elts.head);
  TREE_READONLY (ctor) = 1;
  // dt created data is always static
  TREE_STATIC (ctor) = 1;
  // should always be constant
  TREE_CONSTANT (ctor) = 1;

  return ctor;
}

