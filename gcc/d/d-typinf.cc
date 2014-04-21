// d-typinf.cc -- D frontend for GCC.
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

#include "module.h"
#include "mtype.h"
#include "scope.h"
#include "declaration.h"
#include "aggregate.h"

extern FuncDeclaration *search_toHash(StructDeclaration *sd);
extern FuncDeclaration *search_toString(StructDeclaration *sd);


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
	  tid = TypeInfoDeclaration::create (t, 1);
	  internalTI[t->ty] = tid;
	}
      e = VarExp::create (Loc(), tid);
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
  if (!Type::dtypeinfo)
    {
      error (Loc(), "TypeInfo not found. object.d may be incorrectly installed or corrupt");
      fatal();
    }

  // do this since not all Type's are merge'd
  Type *t = merge2();
  if (!t->vtinfo)
    {
      // does both 'shared' and 'shared const'
      if (t->isShared())
	t->vtinfo = TypeInfoSharedDeclaration::create (t);
      else if (t->isConst())
	t->vtinfo = TypeInfoConstDeclaration::create (t);
      else if (t->isImmutable())
	t->vtinfo = TypeInfoInvariantDeclaration::create (t);
      else if (t->isWild())
	t->vtinfo = TypeInfoWildDeclaration::create (t);
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

	      if (ty == Tstruct)
		{
		  StructDeclaration *sd = ((TypeStruct *) this)->sym;

		  if (((sd->xeq && sd->xeq != sd->xerreq)
		       || (sd->xcmp && sd->xcmp != sd->xerrcmp)
		       || search_toHash (sd) || search_toString (sd))
		      && sd->inNonRoot())
		    Module::addDeferredSemantic3 (sd);
		}
	    }
	  else
	    t->vtinfo->toObjFile (0);
	}
    }
  // Types aren't merged, but we can share the vtinfo's
  if (!vtinfo)
    vtinfo = t->vtinfo;

  Expression *e = VarExp::create (Loc(), t->vtinfo);
  e = e->addressOf (sc);
  // do this so we don't get redundant dereference
  e->type = t->vtinfo->type;
  return e;
}

TypeInfoDeclaration *
Type::getTypeInfoDeclaration (void)
{
  return TypeInfoDeclaration::create (this, 0);
}

TypeInfoDeclaration *
TypeTypedef::getTypeInfoDeclaration (void)
{
  return TypeInfoTypedefDeclaration::create (this);
}

TypeInfoDeclaration *
TypePointer::getTypeInfoDeclaration (void)
{
  return TypeInfoPointerDeclaration::create (this);
}

TypeInfoDeclaration *
TypeDArray::getTypeInfoDeclaration (void)
{
  return TypeInfoArrayDeclaration::create (this);
}

TypeInfoDeclaration *
TypeSArray::getTypeInfoDeclaration (void)
{
  return TypeInfoStaticArrayDeclaration::create (this);
}

TypeInfoDeclaration *
TypeAArray::getTypeInfoDeclaration (void)
{
  return TypeInfoAssociativeArrayDeclaration::create (this);
}

TypeInfoDeclaration *
TypeStruct::getTypeInfoDeclaration (void)
{
  return TypeInfoStructDeclaration::create (this);
}

TypeInfoDeclaration *
TypeClass::getTypeInfoDeclaration (void)
{
  if (sym->isInterfaceDeclaration())
    return TypeInfoInterfaceDeclaration::create (this);
  else
    return TypeInfoClassDeclaration::create (this);
}

TypeInfoDeclaration *
TypeVector::getTypeInfoDeclaration (void)
{
  return TypeInfoVectorDeclaration::create (this);
}

TypeInfoDeclaration *
TypeEnum::getTypeInfoDeclaration (void)
{
  return TypeInfoEnumDeclaration::create (this);
}

TypeInfoDeclaration *
TypeFunction::getTypeInfoDeclaration (void)
{
  return TypeInfoFunctionDeclaration::create (this);
}

TypeInfoDeclaration *
TypeDelegate::getTypeInfoDeclaration (void)
{
  return TypeInfoDelegateDeclaration::create (this);
}

TypeInfoDeclaration *
TypeTuple::getTypeInfoDeclaration (void)
{
  return TypeInfoTupleDeclaration::create (this);
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
  // Strings are so common, make them builtin.
  return !mod
    && ((next->isTypeBasic() != NULL && !next->mod)
	|| (next->ty == Tchar && next->mod == MODimmutable)
	|| (next->ty == Tchar && next->mod == MODconst));
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
      Parameter *arg = Parameter::create (STCin, exps[i]->type, NULL, NULL);
      (*args)[i] = arg;
    }
  TypeTuple *tup = TypeTuple::create (args);
  Expression *e = tup->getTypeInfo (sc);
  e = e->optimize (WANTvalue);
  gcc_assert (e->op == TOKsymoff);

  return e;
}

