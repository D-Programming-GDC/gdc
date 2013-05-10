// d-typinf.cc -- D frontend for GCC.
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

#include "module.h"
#include "mtype.h"
#include "scope.h"
#include "declaration.h"
#include "aggregate.h"


/*******************************************
 * Get a canonicalized form of the TypeInfo for use with the internal
 * runtime library routines. Canonicalized in that static arrays are
 * represented as dynamic arrays, enums are represented by their
 * underlying type, etc. This reduces the number of TypeInfo's needed,
 * so we can use the custom internal ones more.
 */

Expression *
Type::getInternalTypeInfo (Scope *sc)
{
  TypeInfoDeclaration *tid;
  Expression *e;
  Type *t;
  static TypeInfoDeclaration *internalTI[TMAX];

  t = toBasetype();
  switch (t->ty)
    {
    case Tsarray:
      break;

    case Tclass:
      if (((TypeClass *) t)->sym->isInterfaceDeclaration())
	break;
      goto Linternal;

    case Tarray:
      // convert to corresponding dynamic array type
      t = t->nextOf()->mutableOf()->arrayOf();
      if (t->nextOf()->ty != Tclass)
	break;
      goto Linternal;

    case Tfunction:
    case Tdelegate:
    case Tpointer:
    Linternal:
      tid = internalTI[t->ty];
      if (!tid)
	{
	  tid = new TypeInfoDeclaration (t, 1);
	  internalTI[t->ty] = tid;
	}
      e = new VarExp (0, tid);
      e = e->addressOf (sc);
      // do this so we don't get redundant dereference
      e->type = tid->type;
      return e;

    default:
      break;
    }
  return t->getTypeInfo (sc);
}


/****************************************************
 * Get the exact TypeInfo.
 */

Expression *
Type::getTypeInfo (Scope *sc)
{
  if (!Type::typeinfo)
    {
      error (0, "TypeInfo not found. object.d may be incorrectly installed or corrupt, compile with -v switch");
      fatal();
    }

  // do this since not all Type's are merge'd
  Type *t = merge2();
  if (!t->vtinfo)
    {
      // does both 'shared' and 'shared const'
      if (t->isShared())
	t->vtinfo = new TypeInfoSharedDeclaration (t);
      else if (t->isConst())
	t->vtinfo = new TypeInfoConstDeclaration (t);
      else if (t->isImmutable())
	t->vtinfo = new TypeInfoInvariantDeclaration (t);
      else if (t->isWild())
	t->vtinfo = new TypeInfoWildDeclaration (t);
      else
	t->vtinfo = t->getTypeInfoDeclaration();
      gcc_assert (t->vtinfo);
      vtinfo = t->vtinfo;

      /* If this has a custom implementation in std/typeinfo, then
       * do not generate a COMDAT for it.
       */
      if (!t->builtinTypeInfo())
	{
	  if (sc)
	    {
	      // Find module that will go all the way to an object file
	      Module *m = sc->module->importedFrom;
	      m->members->push (t->vtinfo);
	    }
	  else
	    t->vtinfo->toObjFile (0);
	}
    }
  // Types aren't merged, but we can share the vtinfo's
  if (!vtinfo)
    vtinfo = t->vtinfo;

  Expression *e = new VarExp (0, t->vtinfo);
  e = e->addressOf (sc);
  // do this so we don't get redundant dereference
  e->type = t->vtinfo->type;
  return e;
}

TypeInfoDeclaration *
Type::getTypeInfoDeclaration (void)
{
  return new TypeInfoDeclaration (this, 0);
}

TypeInfoDeclaration *
TypeTypedef::getTypeInfoDeclaration (void)
{
  return new TypeInfoTypedefDeclaration (this);
}

TypeInfoDeclaration *
TypePointer::getTypeInfoDeclaration (void)
{
  return new TypeInfoPointerDeclaration (this);
}

TypeInfoDeclaration *
TypeDArray::getTypeInfoDeclaration (void)
{
  return new TypeInfoArrayDeclaration (this);
}

TypeInfoDeclaration *
TypeSArray::getTypeInfoDeclaration (void)
{
  return new TypeInfoStaticArrayDeclaration (this);
}

TypeInfoDeclaration *
TypeAArray::getTypeInfoDeclaration (void)
{
  return new TypeInfoAssociativeArrayDeclaration (this);
}

TypeInfoDeclaration *
TypeStruct::getTypeInfoDeclaration (void)
{
  return new TypeInfoStructDeclaration (this);
}

TypeInfoDeclaration *
TypeClass::getTypeInfoDeclaration (void)
{
  if (sym->isInterfaceDeclaration())
    return new TypeInfoInterfaceDeclaration (this);
  else
    return new TypeInfoClassDeclaration (this);
}

TypeInfoDeclaration *
TypeVector::getTypeInfoDeclaration (void)
{
  return new TypeInfoVectorDeclaration (this);
}

TypeInfoDeclaration *
TypeEnum::getTypeInfoDeclaration (void)
{
  return new TypeInfoEnumDeclaration (this);
}

TypeInfoDeclaration *
TypeFunction::getTypeInfoDeclaration (void)
{
  return new TypeInfoFunctionDeclaration (this);
}

TypeInfoDeclaration *
TypeDelegate::getTypeInfoDeclaration (void)
{
  return new TypeInfoDelegateDeclaration (this);
}

TypeInfoDeclaration *
TypeTuple::getTypeInfoDeclaration (void)
{
  return new TypeInfoTupleDeclaration (this);
}

/* ========================================================================= */

/* These decide if there's an instance for them already in std.typeinfo,
 * because then the compiler doesn't need to build one.
 */

int
Type::builtinTypeInfo (void)
{
  return 0;
}

int
TypeBasic::builtinTypeInfo (void)
{
  return mod ? 0 : 1;
}

int
TypeDArray::builtinTypeInfo (void)
{
  return !mod && ((next->isTypeBasic() != NULL && !next->mod)
		  // strings are so common, make them builtin
		  || (next->ty == Tchar && next->mod == MODimmutable));
}

int
TypeClass::builtinTypeInfo (void)
{
  /* This is statically put out with the ClassInfo, so
   * claim it is built in so it isn't regenerated by each module.
   */
  return mod ? 0 : 1;
}

/* ========================================================================= */

/***************************************
 * Create a static array of TypeInfo references
 * corresponding to an array of Expression's.
 * Used to supply hidden _arguments[] value for variadic D functions.
 */

Expression *
createTypeInfoArray (Scope *sc, Expression *exps[], size_t dim)
{
  /*
   * Pass a reference to the TypeInfo_Tuple corresponding to the types of the
   * arguments. Source compatibility is maintained by computing _arguments[]
   * at the start of the called function by offseting into the TypeInfo_Tuple
   * reference.
   */
  Parameters *args = new Parameters;
  args->setDim (dim);
  for (size_t i = 0; i < dim; i++)
    {
      Parameter *arg = new Parameter (STCin, exps[i]->type, NULL, NULL);
      (*args)[i] = arg;
    }
  TypeTuple *tup = new TypeTuple (args);
  Expression *e = tup->getTypeInfo (sc);
  e = e->optimize (WANTvalue);
  gcc_assert (e->op == TOKsymoff);

  return e;
}

