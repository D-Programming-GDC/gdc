// d-objfile.cc -- D frontend for GCC.
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
#include "dfrontend/enum.h"
#include "dfrontend/import.h"
#include "dfrontend/init.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"
#include "dfrontend/module.h"
#include "dfrontend/statement.h"
#include "dfrontend/staticassert.h"
#include "dfrontend/template.h"
#include "dfrontend/nspace.h"
#include "dfrontend/target.h"
#include "dfrontend/hdrgen.h"

#include "d-system.h"
#include "debug.h"
#include "pointer-set.h"

#include "d-tree.h"
#include "d-objfile.h"
#include "d-codegen.h"
#include "d-dmd-gcc.h"
#include "id.h"

static FuncDeclaration *build_call_function (const char *, vec<FuncDeclaration *>, bool);
static Symbol *build_emutls_function (vec<VarDeclaration *> tlsVars);
static Symbol *build_ctor_function (const char *, vec<FuncDeclaration *>, vec<VarDeclaration *>);
static Symbol *build_dtor_function (const char *, vec<FuncDeclaration *>);
static Symbol *build_unittest_function (const char *, vec<FuncDeclaration *>);

// Module info.  Assuming only one module per run of the compiler.
ModuleInfo *current_module_info;

// static constructors (not D static constructors)
static vec<FuncDeclaration *> static_ctor_list;
static vec<FuncDeclaration *> static_dtor_list;

// Construct a new Symbol.

Symbol::Symbol()
{
  this->Sident = NULL;
  this->prettyIdent = NULL;

  this->Sdt = NULL_TREE;
  this->Salignment = 0;
  this->Sreadonly = false;

  this->Stree = NULL_TREE;
  this->SframeField = NULL_TREE;
  this->SnamedResult = NULL_TREE;

  this->frameInfo = NULL;
}

// Returns true if DSYM is from the gcc.attribute module.

static bool
gcc_attribute_p(Dsymbol *dsym)
{
  ModuleDeclaration *md = dsym->getModule()->md;

  if (md && md->packages && md->packages->dim == 1)
    {
      if (!strcmp((*md->packages)[0]->string, "gcc")
	  && !strcmp(md->id->string, "attribute"))
	return true;
    }

  return false;
}

void
Dsymbol::toObjFile()
{
  // Emit the imported symbol to debug.
  Import *imp = this->isImport();

  if (imp != NULL)
    {
      // Implements import declarations by telling the debug backend we are
      // importing the NAMESPACE_DECL of the module or IMPORTED_DECL of the
      // declaration into the current lexical scope CONTEXT.  NAME is set if
      // this is a renamed import.

      if (imp->isstatic)
	return;

      // Get the context of this import, this should never be null.
      tree context;
      if (cfun != NULL)
	context = current_function_decl;
      else
	context = build_import_decl(current_module_decl);

      if (imp->ident == NULL)
	{
	  // Importing declaration list.
	  for (size_t i = 0; i < imp->names.dim; i++)
	    {
	      AliasDeclaration *aliasdecl = imp->aliasdecls[i];
	      tree decl = build_import_decl(aliasdecl);

              // Skip over unhandled imports.
	      if (decl == NULL_TREE)
		continue;

	      set_decl_location(decl, imp);

	      Identifier *alias = imp->aliases[i];
	      tree name = (alias != NULL)
		? get_identifier(alias->string) : NULL_TREE;

	      (*debug_hooks->imported_module_or_decl)(decl, name, context, false);
	    }
	}
      else
	{
	  // Importing the entire module.
	  tree decl = build_import_decl(imp->mod);
	  set_input_location(imp);

	  tree name = (imp->aliasId != NULL)
	    ? get_identifier(imp->aliasId->string) : NULL_TREE;

	  (*debug_hooks->imported_module_or_decl)(decl, name, context, false);
	}

      return;
    }

  // Emit local variables for tuples.
  TupleDeclaration *td = this->isTupleDeclaration();
  if (td == NULL)
    return;

  for (size_t i = 0; i < td->objects->dim; i++)
    {
      RootObject *o = (*td->objects)[i];
      if ((o->dyncast() == DYNCAST_EXPRESSION) && ((Expression *) o)->op == TOKdsymbol)
	{
	  Declaration *d = ((DsymbolExp *) o)->s->isDeclaration();
	  if (d)
	    d->toObjFile();
	}
    }
}

void
AttribDeclaration::toObjFile()
{
  Dsymbols *d = include (NULL, NULL);

  if (!d)
    return;

  for (size_t i = 0; i < d->dim; i++)
    {
      Dsymbol *s = (*d)[i];
      s->toObjFile();
    }
}

void
PragmaDeclaration::toObjFile()
{
  if (!global.params.ignoreUnsupportedPragmas)
    {
      if (ident == Id::lib)
	warning (loc, "pragma(lib) not implemented");
       else if (ident == Id::startaddress)
	 warning (loc, "pragma(startaddress) not implemented");
    }

  AttribDeclaration::toObjFile();
}

void
Nspace::toObjFile()
{
  if (isError(this) || !members)
    return;

  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *s = (*members)[i];
      s->toObjFile();
    }
}

void
StaticAssert::toObjFile()
{
}

void
StructDeclaration::toObjFile()
{
  if (type->ty == Terror)
    {
      error ("had semantic errors when compiling");
      return;
    }

  // Anonymous structs/unions only exist as part of others,
  // do not output forward referenced structs's
  if (isAnonymous() || !members)
    return;

  // Don't emit any symbols from gcc.attribute module.
  if (gcc_attribute_p(this))
    return;

  // Generate TypeInfo
  genTypeInfo(type, NULL);

  // Generate static initialiser
  toInitializer();
  toDt (&sinit->Sdt);

  sinit->Sreadonly = true;
  d_finish_symbol (sinit);

  // Put out the members
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *member = (*members)[i];
      // There might be static ctors in the members, and they cannot
      // be put in separate object files.
      member->toObjFile();
    }

  // Put out xopEquals, xopCmp and xopHash
  if (xeq && xeq != xerreq)
    xeq->toObjFile();

  if (xcmp && xcmp != xerrcmp)
    xcmp->toObjFile();

  if (xhash)
    xhash->toObjFile();
}

void
ClassDeclaration::toObjFile()
{
  if (type->ty == Terror)
    {
      error ("had semantic errors when compiling");
      return;
    }

  if (!members)
    return;

  // Put out the members
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *member = (*members)[i];
      member->toObjFile();
    }

  // Generate C symbols
  toSymbol();
  toVtblSymbol();
  sinit = toInitializer();

  // Generate static initialiser
  toDt (&sinit->Sdt);
  sinit->Sreadonly = true;
  d_finish_symbol (sinit);

  // Put out the TypeInfo
  genTypeInfo(type, NULL);

  // must be ClassInfo.size
  size_t offset = Target::classinfosize;
  if (Type::typeinfoclass->structsize != offset)
    {
      error ("mismatch between compiler and object.d or object.di found. Check installation and import paths.");
      gcc_unreachable();
    }

  /* Put out the ClassInfo.
   * The layout is:
   *  void **vptr;
   *  monitor_t monitor;
   *  byte[] initializer;         // static initialisation data
   *  char[] name;                // class name
   *  void *[] vtbl;
   *  Interface[] interfaces;
   *  Object *base;               // base class
   *  void *destructor;
   *  void *invariant;            // class invariant
   *  ClassFlags flags;
   *  void *deallocator;
   *  OffsetTypeInfo[] offTi;
   *  void *defaultConstructor;
   *  void* xgetRTInfo;
   */
  tree dt = NULL_TREE;

  build_vptr_monitor (&dt, Type::typeinfoclass);

  // initializer[]
  gcc_assert (structsize >= 8 || (cpp && structsize >= 4));
  dt_cons (&dt, d_array_value (build_ctype(Type::tint8->arrayOf()),
			       size_int (structsize),
			       build_address (sinit->Stree)));
  // name[]
  const char *name = ident->toChars();
  if (!(strlen (name) > 9 && memcmp (name, "TypeInfo_", 9) == 0))
    name = toPrettyChars();
  dt_cons (&dt, d_array_string (name));

  // vtbl[]
  dt_cons (&dt, d_array_value (build_ctype(Type::tvoidptr->arrayOf()),
			       size_int (vtbl.dim),
			       build_address (vtblsym->Stree)));
  // (*vtblInterfaces)[]
  dt_cons (&dt, size_int (vtblInterfaces->dim));

  if (vtblInterfaces->dim)
    {
      // Put out offset to where (*vtblInterfaces)[] is put out
      // after the other data members.
      dt_cons (&dt, build_offset (build_address (csym->Stree),
				  size_int (offset)));
    }
  else
    dt_cons (&dt, null_pointer_node);

  // base*
  if (baseClass)
    dt_cons (&dt, build_address (baseClass->toSymbol()->Stree));
  else
    dt_cons (&dt, null_pointer_node);

  // dtor*
  if (dtor)
    dt_cons (&dt, build_address (dtor->toSymbol()->Stree));
  else
    dt_cons (&dt, null_pointer_node);

  // invariant*
  if (inv)
    dt_cons (&dt, build_address (inv->toSymbol()->Stree));
  else
    dt_cons (&dt, null_pointer_node);

  // flags
  ClassFlags::Type flags = ClassFlags::hasOffTi;

  if (isCOMclass())
    flags |= ClassFlags::isCOMclass;

  if (isCPPclass())
    flags |= ClassFlags::isCPPclass;

  flags |= ClassFlags::hasGetMembers;
  flags |= ClassFlags::hasTypeInfo;

  if (ctor)
    flags |= ClassFlags::hasCtor;

  for (ClassDeclaration *cd = this; cd; cd = cd->baseClass)
    {
      if (cd->dtor)
	{
	  flags |= ClassFlags::hasDtor;
	  break;
	}
    }

  if (isabstract)
    flags |= ClassFlags::isAbstract;

  for (ClassDeclaration *cd = this; cd; cd = cd->baseClass)
    {
      if (!cd->members)
	continue;

      for (size_t i = 0; i < cd->members->dim; i++)
	{
	  Dsymbol *sm = (*cd->members)[i];
	  if (sm->hasPointers())
	    goto Lhaspointers;
	}
    }

  flags |= ClassFlags::noPointers;

Lhaspointers:
  dt_cons (&dt, size_int (flags));

  // deallocator*
  if (aggDelete)
    dt_cons (&dt, build_address (aggDelete->toSymbol()->Stree));
  else
    dt_cons (&dt, null_pointer_node);

  // offTi[]
  dt_cons (&dt, d_array_value (build_ctype(Type::tuns8->arrayOf()),
			       size_int (0), null_pointer_node));

  // defaultConstructor*
  if (defaultCtor && !(defaultCtor->storage_class & STCdisable))
    dt_cons (&dt, build_address (defaultCtor->toSymbol()->Stree));
  else
    dt_cons (&dt, null_pointer_node);

  // xgetRTInfo*
  if (getRTInfo)
    dt_cons (&dt, build_expr(getRTInfo, true));
  else
    {
      // If class has no pointers.
      if (flags & ClassFlags::noPointers)
	dt_cons (&dt, size_int (0));
      else
	dt_cons (&dt, size_int (1));
    }

  /* Put out (*vtblInterfaces)[]. Must immediately follow csym.
   * The layout is:
   *  TypeInfo_Class typeinfo;
   *  void*[] vtbl;
   *  size_t offset;
   */
  offset += vtblInterfaces->dim * (4 * Target::ptrsize);
  for (size_t i = 0; i < vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*vtblInterfaces)[i];
      ClassDeclaration *id = b->base;

      // Fill in vtbl[]
      b->fillVtbl(this, &b->vtbl, 1);

      // ClassInfo
      dt_cons (&dt, build_address (id->toSymbol()->Stree));

      // vtbl[]
      dt_cons (&dt, size_int (id->vtbl.dim));
      dt_cons (&dt, build_offset (build_address (csym->Stree),
				     size_int (offset)));
      // 'this' offset.
      dt_cons (&dt, size_int (b->offset));

      offset += id->vtbl.dim * Target::ptrsize;
    }

  // Put out the (*vtblInterfaces)[].vtbl[]
  // This must be mirrored with ClassDeclaration::baseVtblOffset()
  for (size_t i = 0; i < vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*vtblInterfaces)[i];
      ClassDeclaration *id = b->base;

      // First entry is struct Interface reference.
      if (id->vtblOffset())
	{
	  tree size = size_int (Target::classinfosize + i * (4 * Target::ptrsize));
	  dt_cons (&dt, build_offset (build_address (csym->Stree), size));
	}

      gcc_assert (id->vtbl.dim == b->vtbl.dim);
      for (size_t j = id->vtblOffset() ? 1 : 0; j < id->vtbl.dim; j++)
	{
	  gcc_assert (j < b->vtbl.dim);
	  FuncDeclaration *fd = b->vtbl[j];
	  if (fd)
	    dt_cons (&dt, build_address (fd->toThunkSymbol (b->offset)->Stree));
	  else
	    dt_cons (&dt, null_pointer_node);
	}
    }

  // Put out the overriding interface vtbl[]s.
  // This must be mirrored with ClassDeclaration::baseVtblOffset()
  ClassDeclaration *cd;

  for (cd = this->baseClass; cd; cd = cd->baseClass)
    {
      for (size_t i = 0; i < cd->vtblInterfaces->dim; i++)
	{
	  BaseClass *bs = (*cd->vtblInterfaces)[i];
	  FuncDeclarations bvtbl;

	  if (bs->fillVtbl (this, &bvtbl, 0))
	    {
	      ClassDeclaration *id = bs->base;
	      // First entry is struct Interface reference.
	      if (id->vtblOffset())
		{
		  tree size = size_int (Target::classinfosize + i * (4 * Target::ptrsize));
		  dt_cons (&dt, build_offset (build_address (cd->toSymbol()->Stree), size));
		}

	      for (size_t j = id->vtblOffset() ? 1 : 0; j < id->vtbl.dim; j++)
		{
		  gcc_assert (j < bvtbl.dim);
		  FuncDeclaration *fd = bvtbl[j];
		  if (fd)
		    dt_cons (&dt, build_address (fd->toThunkSymbol (bs->offset)->Stree));
		  else
		    dt_cons (&dt, null_pointer_node);
		}
	    }
	}
    }

  csym->Sdt = dt;
  d_finish_symbol (csym);

  // Put out the vtbl[]
  dt = NULL_TREE;

  // first entry is ClassInfo reference
  if (vtblOffset())
    dt_cons (&dt, build_address (csym->Stree));

  for (size_t i = vtblOffset(); i < vtbl.dim; i++)
    {
      FuncDeclaration *fd = vtbl[i]->isFuncDeclaration();

      if (fd && (fd->fbody || !isAbstract()))
	{
	  fd->functionSemantic();
	  Symbol *s = fd->toSymbol();

	  if (!isFuncHidden (fd))
	    goto Lcontinue;

	  // If fd overlaps with any function in the vtbl[], then
	  // issue 'hidden' error.
	  for (size_t j = 1; j < vtbl.dim; j++)
	    {
	      if (j == i)
		continue;

	      FuncDeclaration *fd2 = vtbl[j]->isFuncDeclaration();
	      if (!fd2->ident->equals (fd->ident))
		continue;

	      if (fd->leastAsSpecialized (fd2) || fd2->leastAsSpecialized (fd))
		{
		  TypeFunction *tf = (TypeFunction *) fd->type;
		  if (tf->ty == Tfunction)
		    {
		      error("use of %s%s hidden by %s is deprecated. "
			    "Use 'alias %s = %s.%s;' to introduce base class overload set.",
			    fd->toPrettyChars(), parametersTypeToChars(tf->parameters, tf->varargs), toChars(),
			    fd->toChars(), fd->parent->toChars(), fd->toChars());
		    }
		  else
		    error("use of %s hidden by %s is deprecated", fd->toPrettyChars(), toChars());
		  break;
		}
	    }

	Lcontinue:
	  dt_cons (&dt, build_address (s->Stree));
	}
      else
	dt_cons (&dt, null_pointer_node);
    }

  vtblsym->Sdt = dt;
  vtblsym->Sreadonly = true;
  d_finish_symbol (vtblsym);
}

// Get offset of base class's vtbl[] initialiser from start of csym.

unsigned
ClassDeclaration::baseVtblOffset (BaseClass *bc)
{
  unsigned csymoffset = Target::classinfosize;
  csymoffset += vtblInterfaces->dim * (4 * Target::ptrsize);

  for (size_t i = 0; i < vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*vtblInterfaces)[i];
      if (b == bc)
	return csymoffset;
      csymoffset += b->base->vtbl.dim * Target::ptrsize;
    }

  // Put out the overriding interface vtbl[]s.
  for (ClassDeclaration *cd = this->baseClass; cd; cd = cd->baseClass)
    {
      for (size_t k = 0; k < cd->vtblInterfaces->dim; k++)
	{
	  BaseClass *bs = (*cd->vtblInterfaces)[k];
	  if (bs->fillVtbl(this, NULL, 0))
	    {
	      if (bc == bs)
		return csymoffset;
	      csymoffset += bs->base->vtbl.dim * Target::ptrsize;
	    }
	}
    }

  return ~0;
}

void
InterfaceDeclaration::toObjFile()
{
  if (type->ty == Terror)
    {
      error ("had semantic errors when compiling");
      return;
    }

  if (!members)
    return;

  // Put out the members
  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *member = (*members)[i];
      member->toObjFile();
    }

  // Generate C symbols
  toSymbol();

  // Put out the TypeInfo
  genTypeInfo(type, NULL);
  type->vtinfo->toObjFile();

  /* Put out the ClassInfo.
   * The layout is:
   *  void **vptr;
   *  monitor_t monitor;
   *  byte[] initializer;         // static initialisation data
   *  char[] name;                // class name
   *  void *[] vtbl;
   *  Interface[] interfaces;
   *  Object *base;               // base class
   *  void *destructor;
   *  void *invariant;            // class invariant
   *  uint flags;
   *  void *deallocator;
   *  OffsetTypeInfo[] offTi;
   *  void *defaultConstructor;
   *  void* xgetRTInfo;
   */
  tree dt = NULL_TREE;

  build_vptr_monitor (&dt, Type::typeinfoclass);

  // initializer[]
  dt_cons (&dt, d_array_value (build_ctype(Type::tint8->arrayOf()),
			       size_int (0), null_pointer_node));
  // name[]
  dt_cons (&dt, d_array_string (toPrettyChars()));

  // vtbl[]
  dt_cons (&dt, d_array_value (build_ctype(Type::tvoidptr->arrayOf()),
			       size_int (0), null_pointer_node));
  // (*vtblInterfaces)[]
  dt_cons (&dt, size_int (vtblInterfaces->dim));

  if (vtblInterfaces->dim)
    {
      // must be ClassInfo.size
      size_t offset = Target::classinfosize;
      if (Type::typeinfoclass->structsize != offset)
	{
	  error ("mismatch between compiler and object.d or object.di found. Check installation and import paths.");
	  gcc_unreachable();
	}
      // Put out offset to where (*vtblInterfaces)[] is put out
      // after the other data members.
      dt_cons (&dt, build_offset (build_address (csym->Stree),
				  size_int (offset)));
    }
  else
    dt_cons (&dt, null_pointer_node);

  // base*, dtor*, invariant*
  gcc_assert (!baseClass);
  dt_cons (&dt, null_pointer_node);
  dt_cons (&dt, null_pointer_node);
  dt_cons (&dt, null_pointer_node);

  // flags
  ClassFlags::Type flags = ClassFlags::hasOffTi;
  flags |= ClassFlags::hasTypeInfo;

  if (isCOMinterface())
    flags |= ClassFlags::isCOMclass;

  dt_cons (&dt, size_int (flags));

  // deallocator*
  dt_cons (&dt, null_pointer_node);

  // offTi[]
  dt_cons (&dt, d_array_value (build_ctype(Type::tuns8->arrayOf()),
			       size_int (0), null_pointer_node));

  // defaultConstructor*
  dt_cons (&dt, null_pointer_node);

  // xgetRTInfo*
  if (getRTInfo)
    dt_cons (&dt, build_expr(getRTInfo, true));
  else
    dt_cons (&dt, size_int (0));

  /* Put out (*vtblInterfaces)[]. Must immediately follow csym.
   * The layout is:
   *  TypeInfo_Class typeinfo;
   *  void*[] vtbl;
   *  size_t offset;
   */
  for (size_t i = 0; i < vtblInterfaces->dim; i++)
    {
      BaseClass *b = (*vtblInterfaces)[i];
      ClassDeclaration *id = b->base;

      // ClassInfo
      dt_cons (&dt, build_address (id->toSymbol()->Stree));

      // vtbl[]
      dt_cons (&dt, d_array_value (build_ctype(Type::tvoidptr->arrayOf()),
				   size_int (0), null_pointer_node));
      // 'this' offset.
      dt_cons (&dt, size_int (b->offset));
    }

  csym->Sdt = dt;
  csym->Sreadonly = true;
  d_finish_symbol (csym);
}

void
EnumDeclaration::toObjFile()
{
  if (semanticRun >= PASSobj)
    return;

  if (errors || type->ty == Terror)
    {
      error ("had semantic errors when compiling");
      return;
    }

  if (isAnonymous())
    return;

  // Generate TypeInfo
  genTypeInfo(type, NULL);

  TypeEnum *tc = (TypeEnum *) type;
  if (tc->sym->members && !type->isZeroInit())
    {
      // Generate static initialiser
      toInitializer();
      DECL_INITIAL (sinit->Stree) = build_expr(tc->sym->defaultval, true);
      d_finish_symbol (sinit);
    }

  semanticRun = PASSobj;
}

void
VarDeclaration::toObjFile()
{
  if (type->ty == Terror)
    {
      error ("had semantic errors when compiling");
      return;
    }

  if (aliassym)
    {
      toAlias()->toObjFile();
      return;
    }

  // Do not store variables we cannot take the address of,
  // but keep the values for purposes of debugging.
  if (!canTakeAddressOf())
    {
      // Don't know if there is a good way to handle instantiations.
      if (isInstantiated())
	return;

      // CONST_DECL was initially intended for enumerals and may
      // be used for scalars in general but not for aggregates.
      if (!type->isscalar())
	return;

      tree decl = toSymbol()->Stree;
      gcc_assert (init && !init->isVoidInitializer());

      Expression *ie = init->toExpression();
      DECL_INITIAL (decl) = build_expr(ie, true);

      d_pushdecl (decl);
      rest_of_decl_compilation (decl, 1, 0);
    }
  else if (isDataseg() && !(storage_class & STCextern))
    {
      Symbol *s = toSymbol();

      // Duplicated VarDeclarations map to the same symbol. Check if this
      // is the one declaration which will be emitted.
      tree ident = get_identifier(s->Sident);
      if (IDENTIFIER_DSYMBOL (ident) && IDENTIFIER_DSYMBOL (ident) != this)
	return;

      if (isThreadlocal())
	{
	  ModuleInfo *mi = current_module_info;
	  mi->tlsVars.safe_push (this);
	}

      size_t sz = type->size();

      if (init)
	{
	  // Look for static array that is block initialised.
	  Type *tb = type->toBasetype();
	  ExpInitializer *ie = init->isExpInitializer();

	  if (ie && tb->ty == Tsarray
	      && !tb->nextOf()->equals(ie->exp->type->toBasetype()->nextOf())
	      && ie->exp->implicitConvTo(tb->nextOf()))
	    {
	      TypeSArray *tsa = (TypeSArray *) tb;
	      tsa->toDtElem (&s->Sdt, ie->exp);
	    }
	  else if (!init->isVoidInitializer())
	    s->Sdt = init->toDt();
	}
      else
	type->toDt(&s->Sdt);

      // Frontend should have already caught this.
      gcc_assert (sz || type->toBasetype()->ty == Tsarray);
      d_finish_symbol (s);
    }
  else
    {
      // This is needed for VarDeclarations in mixins that are to be local
      // variables of a function.  Otherwise, it would be enough to make
      // a check for isVarDeclaration() in DeclarationExp codegen.
      if (!isDataseg() && !isMember())
	{
	  build_local_var (this);

	  if (init)
	    {
	      if (!init->isVoidInitializer())
		{
		  ExpInitializer *vinit = init->isExpInitializer();
		  Expression *ie = vinit->toExpression();
		  tree exp = build_expr(ie);
		  add_stmt(exp);
		}
	      else if (size (loc) != 0)
		{
		  // Zero-length arrays do not have an initializer.
		  warning (OPT_Wuninitialized, "uninitialized variable '%s'",
			   ident ? ident->string : "(no name)");
		}
	    }
	}
    }
}

void
TemplateInstance::toObjFile()
{
  if (isError (this)|| !members)
    return;

  if (!needsCodegen())
    return;

  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *s = (*members)[i];
      s->toObjFile();
    }
}

void
TemplateMixin::toObjFile()
{
  if (isError (this)|| !members)
    return;

  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *s = (*members)[i];
      s->toObjFile();
    }
}

void
TypeInfoDeclaration::toObjFile()
{
  if (isSpeculativeType(this->tinfo))
    return;

  Symbol *s = toSymbol();
  DECL_INITIAL (s->Stree) = layout_typeinfo(this);
  d_finish_symbol(s);
}


// Put out instance of ModuleInfo for this Module

void
Module::genmoduleinfo()
{
  Symbol *msym = toSymbol();
  tree dt = NULL_TREE;
  ClassDeclarations aclasses;
  FuncDeclaration *sgetmembers;

  if (Module::moduleinfo == NULL)
    ObjectNotFound (Id::ModuleInfo);

  for (size_t i = 0; i < members->dim; i++)
    {
      Dsymbol *member = (*members)[i];
      member->addLocalClass (&aclasses);
    }

  size_t aimports_dim = aimports.dim;
  for (size_t i = 0; i < aimports.dim; i++)
    {
      Module *m = aimports[i];
      if (!m->needmoduleinfo)
	aimports_dim--;
    }

  sgetmembers = findGetMembers();

  size_t flags = 0;
  if (sctor)
    flags |= MItlsctor;
  if (sdtor)
    flags |= MItlsdtor;
  if (ssharedctor)
    flags |= MIctor;
  if (sshareddtor)
    flags |= MIdtor;
  if (sgetmembers)
    flags |= MIxgetMembers;
  if (sictor)
    flags |= MIictor;
  if (stest)
    flags |= MIunitTest;
  if (aimports_dim)
    flags |= MIimportedModules;
  if (aclasses.dim)
    flags |= MIlocalClasses;
  if (!needmoduleinfo)
    flags |= MIstandalone;

  flags |= MIname;

  /* Put out:
   *  uint flags;
   *  uint index;
   */
  dt_cons (&dt, build_integer_cst (flags, build_ctype(Type::tuns32)));
  dt_cons (&dt, build_integer_cst (0, build_ctype(Type::tuns32)));

  /*
   * emutls scan function
   */
  if (!targetm.have_tls)
    {
      if (current_module_info->tlsVars.is_empty())
	{
	  dt_cons (&dt, null_pointer_node);
	}
      else
	{
	  Symbol *emutls = build_emutls_function (current_module_info->tlsVars);
	  dt_cons (&dt, build_address (emutls->Stree));
	}
    }

  /* Order of appearance, depending on flags
   *  void function() tlsctor;
   *  void function() tlsdtor;
   *  void* function() xgetMembers;
   *  void function() ctor;
   *  void function() dtor;
   *  void function() ictor;
   *  void function() unitTest;
   *  ModuleInfo*[] importedModules;
   *  TypeInfo_Class[] localClasses;
   *  char[N] name;
   */
  if (flags & MItlsctor)
    dt_cons (&dt, build_address (sctor->Stree));

  if (flags & MItlsdtor)
    dt_cons (&dt, build_address (sdtor->Stree));

  if (flags & MIctor)
    dt_cons (&dt, build_address (ssharedctor->Stree));

  if (flags & MIdtor)
    dt_cons (&dt, build_address (sshareddtor->Stree));

  if (flags & MIxgetMembers)
    dt_cons (&dt, build_address (sgetmembers->toSymbol()->Stree));

  if (flags & MIictor)
    dt_cons (&dt, build_address (sictor->Stree));

  if (flags & MIunitTest)
    dt_cons (&dt, build_address (stest->Stree));

  if (flags & MIimportedModules)
    {
      dt_cons (&dt, size_int (aimports_dim));
      for (size_t i = 0; i < aimports.dim; i++)
	{
	  Module *m = aimports[i];
	  if (m->needmoduleinfo)
	    dt_cons (&dt, build_address (m->toSymbol()->Stree));
	}
    }

  if (flags & MIlocalClasses)
    {
      dt_cons (&dt, size_int (aclasses.dim));
      for (size_t i = 0; i < aclasses.dim; i++)
	{
	  ClassDeclaration *cd = aclasses[i];
	  dt_cons (&dt, build_address (cd->toSymbol()->Stree));
	}
    }

  if (flags & MIname)
    {
      // Put out module name as a 0-terminated C-string, to save bytes
      const char *name = toPrettyChars();
      size_t namelen = strlen (name) + 1;
      tree strtree = build_string (namelen, name);
      TREE_TYPE (strtree) = d_array_type (Type::tchar, namelen);
      dt_cons (&dt, strtree);
    }

  csym->Sdt = dt;
  d_finish_symbol (csym);

  build_moduleinfo (msym);
}

// Finish up a function declaration and compile it all the way
// down to assembler language output.

void
FuncDeclaration::toObjFile()
{
  // Already generated the function.
  if (this->semanticRun >= PASSobj)
    return;

  // Don't emit any symbols from gcc.attribute module.
  if (gcc_attribute_p(this))
    return;

  // Not emitting unittest functions.
  if (!global.params.useUnitTests && this->isUnitTestDeclaration())
    return;

  // Check if any errors occurred when running semantic.
  if (this->type->ty == Tfunction)
    {
      TypeFunction *tf = (TypeFunction *) this->type;
      if (tf->next == NULL || tf->next->ty == Terror)
	return;
    }

  if (this->semantic3Errors)
    return;

  if (this->isNested())
    {
      FuncDeclaration *fdp = this;
      while (fdp && fdp->isNested())
	{
	  fdp = fdp->toParent2()->isFuncDeclaration();

	  if (fdp == NULL)
	    break;

	  // Parent failed to compile, but errors were gagged.
	  if (fdp->semantic3Errors)
	    return;

	  if (UnitTestDeclaration *udp = fdp->isUnitTestDeclaration())
	    {
	      // Defer until outer unittest has been emitted.
	      if (udp->semanticRun < PASSobj)
		{
		  udp->deferredNested.push(this);
		  return;
		}
	    }
	}
    }

  // Ensure all semantic passes have ran.
  if (semanticRun < PASSsemantic3)
    {
      functionSemantic3();
      Module::runDeferredSemantic3();
    }

  if (global.errors)
    return;

  // Duplicated FuncDeclarations map to the same symbol. Check if this
  // is the one declaration which will be emitted.
  if (toSymbol()->Sident)
    {
      tree ident = get_identifier(toSymbol()->Sident);
      if (IDENTIFIER_DSYMBOL (ident) && IDENTIFIER_DSYMBOL (ident) != this)
	return;
    }

  tree fndecl = toSymbol()->Stree;

  // For nested functions in particular, unnest fndecl in the cgraph, as
  // all static chain passing is handled by the front-end.  Do this even
  // if we are not emitting the body.
  struct cgraph_node *node = cgraph_get_create_node(fndecl);
  if (node->origin)
    cgraph_unnest_node(node);

  if (!fbody)
    {
      rest_of_decl_compilation (fndecl, 1, 0);
      return;
    }

  // Start generating code for this function.
  gcc_assert(this->semanticRun == PASSsemantic3done);
  this->semanticRun = PASSobj;

  // Nested functions may not have its toObjFile called before the outer
  // function is finished.  GCC requires that nested functions be finished
  // first so we need to arrange for toObjFile to be called earlier.
  FuncDeclaration *fdp = this->toParent2()->isFuncDeclaration();
  if (fdp && fdp->semanticRun < PASSobj)
    fdp->toObjFile();

  if (global.params.verbose)
    fprintf (global.stdmsg, "function  %s\n", this->toPrettyChars());

  tree old_current_function_decl = current_function_decl;
  function *old_cfun = cfun;
  current_function_decl = fndecl;

  tree return_type = TREE_TYPE (TREE_TYPE (fndecl));
  tree result_decl = build_decl (UNKNOWN_LOCATION, RESULT_DECL, NULL_TREE, return_type);

  set_decl_location (result_decl, this);
  DECL_RESULT (fndecl) = result_decl;
  DECL_CONTEXT (result_decl) = fndecl;
  DECL_ARTIFICIAL (result_decl) = 1;
  DECL_IGNORED_P (result_decl) = 1;

  allocate_struct_function (fndecl, false);
  set_function_end_locus (endloc);

  start_function(this);

  tree parm_decl = NULL_TREE;
  tree param_list = NULL_TREE;

  // Special arguments...

  // 'this' parameter
  // For nested functions, D still generates a vthis, but it
  // should not be referenced in any expression.
  if (vthis)
    {
      parm_decl = vthis->toSymbol()->Stree;
      DECL_ARTIFICIAL (parm_decl) = 1;
      TREE_READONLY (parm_decl) = 1;

      if (vthis->type == Type::tvoidptr)
	{
	  // Replace generic pointer with backend closure type (this wins for gdb).
	  tree frame_type = get_frameinfo (this)->closure_rec;
	  gcc_assert (frame_type != NULL_TREE);
	  TREE_TYPE (parm_decl) = build_pointer_type (frame_type);
	}

      set_decl_location (parm_decl, vthis);
      param_list = chainon (param_list, parm_decl);
      cfun->language->static_chain = parm_decl;
    }

  // _arguments parameter.
  if (v_arguments)
    {
      parm_decl = v_arguments->toSymbol()->Stree;
      set_decl_location (parm_decl, v_arguments);
      param_list = chainon (param_list, parm_decl);
    }

  // formal function parameters.
  size_t n_parameters = parameters ? parameters->dim : 0;

  for (size_t i = 0; i < n_parameters; i++)
    {
      VarDeclaration *param = (*parameters)[i];
      parm_decl = param->toSymbol()->Stree;
      set_decl_location (parm_decl, (Dsymbol *) param);
      // chain them in the correct order
      param_list = chainon (param_list, parm_decl);
    }

  DECL_ARGUMENTS (fndecl) = param_list;
  rest_of_decl_compilation (fndecl, 1, 0);
  DECL_INITIAL (fndecl) = error_mark_node;

  push_stmt_list();
  push_binding_level(level_function);
  set_input_location (loc);

  // If this is a member function that nested (possibly indirectly) in another
  // function, construct an expession for this member function's static chain
  // by going through parent link of nested classes.
  if (isThis())
    {
      AggregateDeclaration *ad = isThis();
      tree this_tree = vthis->toSymbol()->Stree;

      while (ad->isNested())
	{
	  Dsymbol *d = ad->toParent2();
	  tree vthis_field = ad->vthis->toSymbol()->Stree;
	  this_tree = component_ref (build_deref (this_tree), vthis_field);

	  ad = d->isAggregateDeclaration();
	  if (ad == NULL)
	    {
	      cfun->language->static_chain = this_tree;
	      break;
	    }
	}
    }

  // May change cfun->static_chain
  build_closure(this);

  if (vresult)
    build_local_var (vresult);

  if (v_argptr)
    push_stmt_list();

  // The fabled D named return value optimisation.
  // Implemented by overriding all the RETURN_EXPRs and replacing all
  // occurrences of VAR with the RESULT_DECL for the function.
  // This is only worth doing for functions that can return in memory.
  if (nrvo_can)
    {
      if (!AGGREGATE_TYPE_P (return_type))
	nrvo_can = 0;
      else
	nrvo_can = aggregate_value_p (return_type, fndecl);
    }

  if (nrvo_can)
    {
      TREE_TYPE (result_decl) = build_reference_type(TREE_TYPE (result_decl));
      DECL_BY_REFERENCE (result_decl) = 1;
      TREE_ADDRESSABLE (result_decl) = 0;
      relayout_decl(result_decl);

      if (nrvo_var)
	{
	  Symbol *nrvsym = nrvo_var->toSymbol();
	  tree var = nrvsym->Stree;

	  // Copy name from VAR to RESULT.
	  DECL_NAME (result_decl) = DECL_NAME (var);
	  // Don't forget that we take it's address.
	  TREE_ADDRESSABLE (var) = 1;

	  result_decl = build_deref(result_decl);

	  SET_DECL_VALUE_EXPR (var, result_decl);
	  DECL_HAS_VALUE_EXPR_P (var) = 1;

	  nrvsym->SnamedResult = result_decl;
	}
    }

  build_ir (this);

  if (v_argptr)
    {
      tree body = pop_stmt_list();
      tree var = get_decl_tree (v_argptr);
      var = build_address (var);

      tree init_exp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_VA_START), 2, var, parm_decl);
      build_local_var (v_argptr);
      add_stmt(init_exp);

      tree cleanup = d_build_call_nary (builtin_decl_explicit (BUILT_IN_VA_END), 1, var);
      add_stmt(build2 (TRY_FINALLY_EXPR, void_type_node, body, cleanup));
    }

  // Backend expects a statement list to come from somewhere, however
  // popStatementList returns expressions when there is a single statement.
  // So here we create a statement list unconditionally.
  tree block = pop_binding_level();
  tree body = pop_stmt_list();
  tree bind = build3(BIND_EXPR, void_type_node,
		     BLOCK_VARS (block), body, block);

  if (TREE_CODE (body) != STATEMENT_LIST)
    {
      tree stmtlist = alloc_stmt_list();
      append_to_statement_list_force (body, &stmtlist);
      BIND_EXPR_BODY (bind) = stmtlist;
    }
  else if (!STATEMENT_LIST_HEAD (body))
    {
      /* For empty functions: Without this, there is a segfault when inlined.
	 Seen on build=ppc-linux but not others (why?).  */
      tree ret = return_expr (NULL_TREE);
      append_to_statement_list_force (ret, &body);
    }

  DECL_SAVED_TREE (fndecl) = bind;

  if (!errorcount && !global.errors)
    {
      // Dump the D-specific tree IR.
      int local_dump_flags;
      FILE *dump_file = dump_begin (TDI_original, &local_dump_flags);
      if (dump_file)
	{
	  fprintf (dump_file, "\n;; Function %s",
		   lang_hooks.decl_printable_name (fndecl, 2));
	  fprintf (dump_file, " (%s)\n",
		   (!DECL_ASSEMBLER_NAME_SET_P (fndecl) ? "null"
		    : IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl))));
	  fprintf (dump_file, ";; enabled by -%s\n", dump_flag_name (TDI_original));
	  fprintf (dump_file, "\n");

	  if (local_dump_flags & TDF_RAW)
	    dump_node (DECL_SAVED_TREE (fndecl),
		       TDF_SLIM | local_dump_flags, dump_file);
	  else
	    print_generic_expr (dump_file, DECL_SAVED_TREE (fndecl), local_dump_flags);
	  fprintf (dump_file, "\n");

	  dump_end (TDI_original, dump_file);
	}
    }

  if (!errorcount && !global.errors)
    d_finish_function (this);

  // Process all deferred nested functions.
  for (size_t i = 0; i < cfun->language->deferred_fns.length(); ++i)
    {
      FuncDeclaration *fd = cfun->language->deferred_fns[i];
      fd->toObjFile();
    }

  if (UnitTestDeclaration *ud = this->isUnitTestDeclaration())
    {
      for (size_t i = 0; i < ud->deferredNested.dim; ++i)
	{
	  FuncDeclaration *fd = ud->deferredNested[i];
	  fd->toObjFile();
	}
    }

  end_function();

  current_function_decl = old_current_function_decl;
  set_cfun (old_cfun);
}

//

void
Module::genobjfile(bool)
{
  // Normally would create an ObjFile here, but gcc is limited to one object
  // file per pass and there may be more than one module per object file.
  current_module_info = new ModuleInfo;
  current_module_decl = this;

  if (members)
    {
      for (size_t i = 0; i < members->dim; i++)
	{
	  Dsymbol *dsym = (*members)[i];
	  dsym->toObjFile();
	}
    }

  // Default behaviour is to always generate module info because of templates.
  // Can be switched off for not compiling against runtime library.
  if (!global.params.betterC && ident != Id::entrypoint)
    {
      ModuleInfo *mi = current_module_info;

      if (!mi->ctors.is_empty() || !mi->ctorgates.is_empty())
	sctor = build_ctor_function ("*__modctor", mi->ctors, mi->ctorgates);

      if (!mi->dtors.is_empty())
	sdtor = build_dtor_function ("*__moddtor", mi->dtors);

      if (!mi->sharedctors.is_empty() || !mi->sharedctorgates.is_empty())
	ssharedctor = build_ctor_function ("*__modsharedctor", mi->sharedctors, mi->sharedctorgates);

      if (!mi->shareddtors.is_empty())
	sshareddtor = build_dtor_function ("*__modshareddtor", mi->shareddtors);

      if (!mi->unitTests.is_empty())
	stest = build_unittest_function ("*__modtest", mi->unitTests);

      genmoduleinfo();
    }

  // Finish off any thunks deferred during compilation.
  write_deferred_thunks();

  current_module_info = NULL;
  current_module_decl = NULL;
}

// Support for multiple modules per object file.
// Returns TRUE if module M is being compiled.

bool
output_module_p (Module *m)
{
  static unsigned search_index = 0;

  if (!m || !output_modules.dim)
    return false;

  if (output_modules[search_index] == m)
    return true;

  for (size_t i = 0; i < output_modules.dim; i++)
    {
      if (output_modules[i] == m)
       {
         search_index = i;
         return true;
       }
    }

  return false;
}

void
d_finish_module()
{
  /* If the target does not directly support static constructors,
     static_ctor_list contains a list of all static constructors defined
     so far.  This routine will create a function to call all of those
     and is picked up by collect2. */
  const char *ident;

  if (!static_ctor_list.is_empty())
    {
      ident = IDENTIFIER_POINTER (get_file_function_name ("I"));
      build_call_function (ident, static_ctor_list, true);
    }

  if (!static_dtor_list.is_empty())
    {
      ident = IDENTIFIER_POINTER (get_file_function_name ("D"));
      build_call_function (ident, static_dtor_list, true);
    }
}

location_t
get_linemap (const Loc& loc)
{
  location_t gcc_location;

  linemap_add (line_table, LC_ENTER, 0, loc.filename, loc.linnum);
  linemap_line_start (line_table, loc.linnum, 0);
  gcc_location = linemap_position_for_column (line_table, loc.charnum);
  linemap_add (line_table, LC_LEAVE, 0, NULL, 0);

  return gcc_location;
}

// Update input_location to LOC.

void
set_input_location (const Loc& loc)
{
  if (loc.filename)
    input_location = get_linemap (loc);
}

// Some D Declarations don't have the loc set, this searches DECL's parents
// until a valid loc is found.

void
set_input_location (Dsymbol *decl)
{
  Dsymbol *dsym = decl;
  while (dsym)
    {
      if (dsym->loc.filename)
	{
	  set_input_location (dsym->loc);
	  return;
	}
      dsym = dsym->toParent();
    }

  // Fallback; backend sometimes crashes if not set
  Module *mod = decl->getModule();
  Loc loc;

  if (mod && mod->srcfile && mod->srcfile->name)
    loc.filename = mod->srcfile->name->str;
  else
    // Empty string can mess up debug info
    loc.filename = "<no_file>";

  loc.linnum = 1;
  set_input_location (loc);
}

// Like set_input_location, but sets the location on decl T.

void
set_decl_location (tree t, const Loc& loc)
{
  // DWARF2 will often crash if the DECL_SOURCE_FILE is not set.
  // It's easier the error here.
  gcc_assert (loc.filename);
  DECL_SOURCE_LOCATION (t) = get_linemap (loc);
}

void
set_decl_location (tree t, Dsymbol *decl)
{
  Dsymbol *dsym = decl;
  while (dsym)
    {
      if (dsym->loc.filename)
	{
	  set_decl_location (t, dsym->loc);
	  return;
	}
      dsym = dsym->toParent();
    }

  // Fallback; backend sometimes crashes if not set
  Module *mod = decl->getModule();
  Loc loc;

  if (mod && mod->srcfile && mod->srcfile->name)
    loc.filename = mod->srcfile->name->str;
  else
    // Empty string can mess up debug info
    loc.filename = "<no_file>";

  loc.linnum = 1;
  set_decl_location (t, loc);
}

void
set_function_end_locus (const Loc& loc)
{
  if (loc.filename)
    cfun->function_end_locus = get_linemap (loc);
  else
    cfun->function_end_locus = DECL_SOURCE_LOCATION (cfun->decl);
}

// Return the COMDAT group into which DECL should be placed.

static tree
d_comdat_group(tree decl)
{
  // If already part of a comdat group, use that.
  if (DECL_COMDAT_GROUP (decl))
    return DECL_COMDAT_GROUP (decl);

  return DECL_ASSEMBLER_NAME (decl);
}

// Set DECL up to have the closest approximation of "initialized common"
// linkage available.

void
d_comdat_linkage(tree decl)
{
  // Weak definitions have to be public.
  if (!TREE_PUBLIC (decl))
    return;

  // Necessary to allow DECL_ONE_ONLY or DECL_WEAK functions to be inlined
  if (TREE_CODE (decl) == FUNCTION_DECL)
    DECL_DECLARED_INLINE_P (decl) = 1;

  // The following makes assumptions about the behavior of make_decl_one_only.
  if (SUPPORTS_ONE_ONLY)
    make_decl_one_only(decl, d_comdat_group(decl));
  else if (SUPPORTS_WEAK)
    {
      tree decl_init = DECL_INITIAL (decl);
      DECL_INITIAL (decl) = integer_zero_node;
      make_decl_one_only(decl, d_comdat_group(decl));
      DECL_INITIAL (decl) = decl_init;
    }
  else if (TREE_CODE (decl) == FUNCTION_DECL
	   || (VAR_P (decl) && DECL_ARTIFICIAL (decl)))
    // We can just emit function and compiler-generated variables
    // statically; having multiple copies is (for the most part) only
    // a waste of space.
    TREE_PUBLIC (decl) = 0;
  else if (DECL_INITIAL (decl) == NULL_TREE
	   || DECL_INITIAL (decl) == error_mark_node)
    // Fallback, cannot have multiple copies.
    DECL_COMMON (decl) = 1;

  DECL_COMDAT (decl) = 1;
}


// Check if dsym is a template and whether it will be emitted (local_p)
// or if it's external.

void
get_template_storage_info (Dsymbol *dsym, bool *local_p, bool *template_p)
{
  *local_p = output_module_p(dsym->getModule());
  *template_p = false;
  Dsymbol *sym = dsym->toParent();

  while (sym)
    {
      TemplateInstance *ti = sym->isTemplateInstance();
      if (ti)
	{
	  *local_p = output_module_p(ti->minst);
	  *template_p = true;
	  break;
	}
      sym = sym->toParent();
    }
}

// Set a DECL's STATIC and EXTERN based on the decl's storage class
// and if it is to be emitted in this module.

void
setup_symbol_storage(Dsymbol *dsym, tree decl, bool public_p)
{
  Declaration *rd = dsym->isDeclaration();

  if (public_p
      || (VAR_P (decl) && (rd && rd->isDataseg()))
      || (TREE_CODE (decl) == FUNCTION_DECL))
    {
      bool local_p, template_p;
      get_template_storage_info(dsym, &local_p, &template_p);

      if (template_p)
	{
	  D_DECL_ONE_ONLY (decl) = 1;
	  D_DECL_IS_TEMPLATE (decl) = 1;
	  DECL_ABSTRACT (decl) = !flag_emit_templates;
	}

      VarDeclaration *vd = rd ? rd->isVarDeclaration() : NULL;
      FuncDeclaration *fd = rd ? rd->isFuncDeclaration() : NULL;
      if (!local_p || (vd && vd->storage_class & STCextern) || (fd && !fd->fbody))
	{
	  DECL_EXTERNAL (decl) = 1;
	  TREE_STATIC (decl) = 0;
	}
      else
	{
	  DECL_EXTERNAL (decl) = 0;
	  TREE_STATIC (decl) = 1;
	}

      // Tell backend this is a thread local decl.
      if (vd && vd->isDataseg() && vd->isThreadlocal())
	DECL_TLS_MODEL (decl) = decl_default_tls_model (decl);

      // Do this by default, but allow private templates to override
      if (public_p || !fd || !fd->isNested())
	TREE_PUBLIC (decl) = 1;

      // Used by debugger.
      if (rd && rd->protection == PROTprivate)
	TREE_PRIVATE (decl) = 1;
      else if (rd && rd->protection == PROTprotected)
	TREE_PROTECTED (decl) = 1;

      if (D_DECL_ONE_ONLY (decl))
	d_comdat_linkage(decl);
    }
  else
    {
      TREE_STATIC (decl) = 0;
      DECL_EXTERNAL (decl) = 0;
      TREE_PUBLIC (decl) = 0;
    }

  if (rd && rd->userAttribDecl)
    {
      Expressions *attrs = rd->userAttribDecl->getAttributes();
      decl_attributes(&decl, build_attributes(attrs), 0);
    }
  else if (DECL_ATTRIBUTES (decl) != NULL)
    decl_attributes(&decl, DECL_ATTRIBUTES (decl), 0);
}

// Mark DECL, which is a VAR_DECL or FUNCTION_DECL as a symbol that
// must be emitted in this, output module.

static void
mark_needed (tree decl)
{
  TREE_USED (decl) = 1;

  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      struct cgraph_node *node = cgraph_get_create_node (decl);
      node->forced_by_abi = true;
    }
  else if (VAR_P (decl))
    {
      struct varpool_node *node = varpool_node_for_decl (decl);
      node->forced_by_abi = true;
    }
}

// Finish up a symbol declaration and compile it all the way to
// the assembler language output.

void
d_finish_symbol (Symbol *sym)
{
  if (!sym->Stree)
    {
      tree init = dtvector_to_tree (sym->Sdt);
      sym->Stree = build_artificial_decl(TREE_TYPE (init), init);
      gcc_assert (!sym->Sident);
    }

  tree decl = sym->Stree;

  if (sym->Sdt)
    {
      if (DECL_INITIAL (decl) == NULL_TREE)
	{
	  tree sinit = dtvector_to_tree (sym->Sdt);
	  if (TREE_TYPE (decl) == unknown_type_node)
	    {
	      TREE_TYPE (decl) = TREE_TYPE (sinit);
	      TYPE_NAME (TREE_TYPE (decl)) = get_identifier (sym->Sident);
	    }

	  // No gain setting DECL_INITIAL if the initialiser is all zeros.
	  // Let the backend put the symbol in bss instead, if supported.
	  if (!initializer_zerop (sinit))
	    DECL_INITIAL (decl) = sinit;
	}
      gcc_assert (COMPLETE_TYPE_P (TREE_TYPE (decl)));
    }

  gcc_assert (!error_operand_p (decl));

  // If the symbol was marked as readonly in the frontend, set TREE_READONLY.
  if (sym->Sreadonly)
    TREE_READONLY (decl) = 1;

  // We are sending this symbol to object file, can't be extern.
  TREE_STATIC (decl) = 1;
  DECL_EXTERNAL (decl) = 0;
  relayout_decl (decl);

#ifdef ENABLE_TREE_CHECKING
  if (DECL_INITIAL (decl) != NULL_TREE)
    {
      // Initialiser must never be bigger than symbol size.
      dinteger_t tsize = int_size_in_bytes (TREE_TYPE (decl));
      dinteger_t dtsize = int_size_in_bytes (TREE_TYPE (DECL_INITIAL (decl)));

      if (tsize < dtsize)
	internal_error ("Mismatch between declaration '%s' size (%wd) and it's initializer size (%wd).",
			sym->prettyIdent ? sym->prettyIdent : sym->Sident, tsize, dtsize);
    }
#endif

  // User declared alignment.
  if (sym->Salignment > 0)
    {
      DECL_ALIGN (decl) = sym->Salignment * BITS_PER_UNIT;
      DECL_USER_ALIGN (decl) = 1;
    }

  d_add_global_declaration (decl);

  // %% FIXME: DECL_COMMON so the symbol goes in .tcommon
  if (DECL_THREAD_LOCAL_P (decl)
      && DECL_ASSEMBLER_NAME (decl) == get_identifier ("_tlsend"))
    {
      DECL_INITIAL (decl) = NULL_TREE;
      DECL_COMMON (decl) = 1;
    }

  rest_of_decl_compilation (decl, 1, 0);
}

void
d_finish_function(FuncDeclaration *fd)
{
  Symbol *s = fd->toSymbol();
  tree decl = s->Stree;

  gcc_assert(TREE_CODE (decl) == FUNCTION_DECL);

  // If function is not needed, don't send it to backend.
  if (DECL_ABSTRACT (decl))
    return;

  // If we generated the function, but it's really extern.
  // Such as external inlinable functions or thunk aliases.
  bool extern_p = false;
  for (FuncDeclaration *fdp = fd; fdp != NULL;)
    {
      if (fdp->inNonRoot())
	{
	  extern_p = true;
	  break;
	}

      if (!fdp->isNested())
	break;

      fdp = fdp->toParent2()->isFuncDeclaration();
    }

  if (extern_p)
    {
      TREE_STATIC (decl) = 0;
      DECL_EXTERNAL (decl) = 1;
    }
  else if (DECL_SAVED_TREE (decl) != NULL_TREE)
    {
      TREE_STATIC (decl) = 1;
      DECL_EXTERNAL (decl) = 0;
    }

  if (!targetm.have_ctors_dtors)
    {
      if (DECL_STATIC_CONSTRUCTOR (decl))
	static_ctor_list.safe_push(fd);
      if (DECL_STATIC_DESTRUCTOR (decl))
	static_dtor_list.safe_push(fd);
    }

  d_add_global_declaration(decl);
  cgraph_finalize_function(decl, true);
}

// Wrapup all global declarations and start the final compilation.

void
d_finish_compilation(tree *vec, int len)
{
  // Complete all generated thunks.
  cgraph_process_same_body_aliases();

  // Process all file scopes in this compilation, and the external_scope,
  // through wrapup_global_declarations.
  for (int i = 0; i < len; i++)
    {
      tree decl = vec[i];
      wrapup_global_declarations(&decl, 1);

      // We want the static symbol to be written.
      if ((VAR_P (decl) && TREE_STATIC (decl))
	  || TREE_CODE (decl) == FUNCTION_DECL)
	mark_needed(decl);
      else if (TREE_CODE (decl) == TYPE_DECL)
	{
	  bool toplevel = !DECL_CONTEXT (decl);
	  rest_of_decl_compilation(decl, toplevel, 0);
	}
    }

  // We're done parsing; proceed to optimize and emit assembly.
  if (!global.errors && !errorcount)
    finalize_compilation_unit();

  // Now, issue warnings about static, but not defined, functions.
  check_global_declarations (vec, len);

  // After cgraph has had a chance to emit everything that's going to
  // be emitted, output debug information for globals.
  emit_debug_global_declarations (vec, len);
}

// Return an anonymous static variable of type TYPE, initialized with
// INIT, and optionally prefixing the name with PREFIX.
// At some point the INIT constant should be hashed to remove duplicates.

tree
build_artificial_decl(tree type, tree init, const char *prefix)
{
  tree decl = build_decl(UNKNOWN_LOCATION, VAR_DECL, NULL_TREE, type);
  const char *name = prefix ? prefix : "___s";
  char *label;

  ASM_FORMAT_PRIVATE_NAME (label, name, DECL_UID (decl));
  SET_DECL_ASSEMBLER_NAME (decl, get_identifier(label));
  DECL_NAME (decl) = DECL_ASSEMBLER_NAME (decl);

  TREE_PUBLIC (decl) = 0;
  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;
  DECL_IGNORED_P (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;

  DECL_INITIAL (decl) = init;
  d_add_global_declaration(decl);

  return decl;
}

// Build TYPE_DECL for the declaration DSYM.

void
build_type_decl (tree type, Dsymbol *dsym)
{
  if (TYPE_STUB_DECL (type))
    return;

  gcc_assert(!POINTER_TYPE_P (type));

  tree decl = build_decl(UNKNOWN_LOCATION, TYPE_DECL,
			 get_identifier(dsym->ident->string), type);

  DECL_CONTEXT (decl) = d_decl_context(dsym);
  set_decl_location(decl, dsym);

  TYPE_CONTEXT (type) = DECL_CONTEXT (decl);
  TYPE_NAME (type) = decl;

  if (TREE_CODE (type) == ENUMERAL_TYPE || TREE_CODE (type) == RECORD_TYPE
      || TREE_CODE (type) == UNION_TYPE)
    {
      /* Not sure if there is a need for separate TYPE_DECLs in
	 TYPE_NAME and TYPE_STUB_DECL. */
      TYPE_STUB_DECL (type) = decl;
      DECL_ARTIFICIAL (decl) = 1;
    }

  d_add_global_declaration(decl);
}

// Can't output thunks while a function is being compiled.

struct DeferredThunk
{
  tree decl;
  tree target;
  int offset;
};

static vec<DeferredThunk *> deferred_thunks;

// Process all deferred thunks in list DEFERRED_THUNKS.

void
write_deferred_thunks()
{
  for (size_t i = 0; i < deferred_thunks.length(); i++)
    {
      DeferredThunk *t = deferred_thunks[i];
      finish_thunk (t->decl, t->target, t->offset);
    }

  deferred_thunks.truncate (0);
}

// Emit the definition of a D vtable thunk.  If a function
// is still being compiled, defer emitting.

void
use_thunk (tree thunk_decl, tree target_decl, int offset)
{
  if (current_function_decl)
    {
      DeferredThunk *t = new DeferredThunk;
      t->decl = thunk_decl;
      t->target = target_decl;
      t->offset = offset;
      deferred_thunks.safe_push (t);
    }
  else
    finish_thunk (thunk_decl, target_decl, offset);
}

/* Thunk code is based on g++ */

static int thunk_labelno;

/* Create a static alias to function.  */

static tree
make_alias_for_thunk (tree function)
{
  tree alias;
  char buf[256];

  // Thunks may reference extern functions which cannot be aliased.
  if (DECL_EXTERNAL (function))
    return function;

  targetm.asm_out.generate_internal_label (buf, "LTHUNK", thunk_labelno);
  thunk_labelno++;

  alias = build_decl (DECL_SOURCE_LOCATION (function), FUNCTION_DECL,
		      get_identifier (buf), TREE_TYPE (function));
  DECL_LANG_SPECIFIC (alias) = DECL_LANG_SPECIFIC (function);
  DECL_CONTEXT (alias) = NULL_TREE;
  TREE_READONLY (alias) = TREE_READONLY (function);
  TREE_THIS_VOLATILE (alias) = TREE_THIS_VOLATILE (function);
  TREE_PUBLIC (alias) = 0;

  DECL_EXTERNAL (alias) = 0;
  DECL_ARTIFICIAL (alias) = 1;

  DECL_DECLARED_INLINE_P (alias) = 0;
  DECL_INITIAL (alias) = error_mark_node;
  DECL_ARGUMENTS (alias) = copy_list (DECL_ARGUMENTS (function));

  TREE_ADDRESSABLE (alias) = 1;
  TREE_USED (alias) = 1;
  SET_DECL_ASSEMBLER_NAME (alias, DECL_NAME (alias));

  if (!flag_syntax_only)
    {
      cgraph_node *aliasn;
      aliasn = cgraph_same_body_alias (cgraph_get_create_node (function),
				       alias, function);
      DECL_ASSEMBLER_NAME (function);
      gcc_assert (aliasn != NULL);
    }
  return alias;
}

void
finish_thunk (tree thunk_decl, tree target_decl, int offset)
{
  /* Setup how D thunks are outputted.  */
  int fixed_offset = -offset;
  bool this_adjusting = true;
  int virtual_value = 0;
  tree alias;

  if (TARGET_USE_LOCAL_THUNK_ALIAS_P (target_decl))
    alias = make_alias_for_thunk (target_decl);
  else
    alias = target_decl;

  TREE_ADDRESSABLE (target_decl) = 1;
  TREE_USED (target_decl) = 1;

  if (flag_syntax_only)
    {
      TREE_ASM_WRITTEN (thunk_decl) = 1;
      return;
    }

  if (TARGET_USE_LOCAL_THUNK_ALIAS_P (target_decl)
      && targetm_common.have_named_sections)
    {
      tree fn = target_decl;
      struct symtab_node *symbol = symtab_get_node (target_decl);

      if (symbol != NULL && symbol->alias)
	{
	  if (symbol->analyzed)
	    fn = symtab_alias_ultimate_target (symtab_get_node (target_decl))->decl;
	  else
	    fn = symtab_get_node (target_decl)->alias_target;
	}
      resolve_unique_section (fn, 0, flag_function_sections);

      if (DECL_SECTION_NAME (fn) != NULL && DECL_ONE_ONLY (fn))
	{
	  resolve_unique_section (thunk_decl, 0, flag_function_sections);

	  /* Output the thunk into the same section as function.  */
	  DECL_SECTION_NAME (thunk_decl) = DECL_SECTION_NAME (fn);
	}
    }

  /* Set up cloned argument trees for the thunk.  */
  tree t = NULL_TREE;
  for (tree a = DECL_ARGUMENTS (target_decl); a; a = DECL_CHAIN (a))
    {
      tree x = copy_node (a);
      DECL_CHAIN (x) = t;
      DECL_CONTEXT (x) = thunk_decl;
      SET_DECL_RTL (x, NULL);
      DECL_HAS_VALUE_EXPR_P (x) = 0;
      TREE_ADDRESSABLE (x) = 0;
      t = x;
    }
  DECL_ARGUMENTS (thunk_decl) = nreverse (t);
  TREE_ASM_WRITTEN (thunk_decl) = 1;

  cgraph_node *funcn, *thunk_node;

  funcn = cgraph_get_create_node (target_decl);
  gcc_assert (funcn);
  thunk_node = cgraph_add_thunk (funcn, thunk_decl, thunk_decl,
				 this_adjusting, fixed_offset,
				 virtual_value, 0, alias);

  if (DECL_ONE_ONLY (target_decl))
    symtab_add_to_same_comdat_group (thunk_node, funcn);

  if (!targetm.asm_out.can_output_mi_thunk (thunk_decl, fixed_offset,
					    virtual_value, alias))
    {
      /* if varargs... */
      sorry ("backend for this target machine does not support thunks");
    }
}

// Build and emit a function named NAME, whose function body is in EXPR.

static FuncDeclaration *
build_simple_function (const char *name, tree expr, bool static_ctor)
{
  Module *mod = current_module_decl;

  if (!mod)
    mod = d_gcc_get_output_module();

  if (name[0] == '*')
    {
      Symbol *s = mod->toSymbolX (name + 1, 0, 0, "FZv");
      name = s->Sident;
    }

  TypeFunction *func_type = new TypeFunction (0, Type::tvoid, 0, LINKc);
  FuncDeclaration *func = new FuncDeclaration (mod->loc, mod->loc,
					       Identifier::idPool (name), STCstatic, func_type);
  func->loc = Loc(mod->srcfile->toChars(), 1, 0);
  func->linkage = func_type->linkage;
  func->parent = mod;
  func->protection = PROTprivate;
  func->semanticRun = PASSsemantic3done;

  tree func_decl = func->toSymbol()->Stree;

  if (static_ctor)
    DECL_STATIC_CONSTRUCTOR (func_decl) = 1;

  // D static ctors, dtors, unittests, and the ModuleInfo chain function
  // are always private (see setup_symbol_storage, default case)
  TREE_PUBLIC (func_decl) = 0;
  TREE_USED (func_decl) = 1;

  // %% Maybe remove the identifier
  WrappedExp *body = new WrappedExp (mod->loc, expr, Type::tvoid);
  func->fbody = new ExpStatement (mod->loc, body);
  func->toObjFile();

  return func;
}

// Build and emit a function identified by NAME that calls (in order)
// the list of functions in FUNCTIONS.  If FORCE_P, create a new function
// even if there is only one function to call in the list.

static FuncDeclaration *
build_call_function (const char *name, vec<FuncDeclaration *> functions, bool force_p)
{
  tree expr_list = NULL_TREE;

  // If there is only one function, just return that
  if (functions.length() == 1 && !force_p)
    return functions[0];

  Module *mod = current_module_decl;
  if (!mod)
    mod = d_gcc_get_output_module();
  set_input_location(Loc(mod->srcfile->toChars(), 1, 0));

  // Shouldn't front end build these?
  for (size_t i = 0; i < functions.length(); i++)
    {
      tree fndecl = (functions[i])->toSymbol()->Stree;
      tree call_expr = d_build_call_list (void_type_node, build_address (fndecl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    return build_simple_function (name, expr_list, false);

  return NULL;
}

// Build and emit a function that takes a scope
// delegate parameter and calls it once for every TLS variable in the
// module.

static Symbol *
build_emutls_function (vec<VarDeclaration *> tlsVars)
{
  Module *mod = current_module_decl;

  if (!mod)
    mod = d_gcc_get_output_module();

  const char *name = "__modtlsscan";

  // void __modtlsscan(scope void delegate(void*, void*) nothrow dg) nothrow
  // {
  //     dg(&$tlsVars[0]$, &$tlsVars[0]$ + $tlsVars[0]$.sizeof);
  //     dg(&$tlsVars[1]$, &$tlsVars[1]$ + $tlsVars[0]$.sizeof);
  //     ...
  // }

  Parameters *del_args = new Parameters();
  del_args->push (new Parameter (0, Type::tvoidptr, NULL, NULL));
  del_args->push (new Parameter (0, Type::tvoidptr, NULL, NULL));

  TypeFunction *del_func_type = new TypeFunction (del_args, Type::tvoid, 0, LINKd, STCnothrow);
  Parameters *args = new Parameters();
  Parameter *dg_arg = new Parameter (STCscope, new TypeDelegate (del_func_type),
				     Identifier::idPool ("dg"), NULL);
  args->push (dg_arg);
  TypeFunction *func_type = new TypeFunction (args, Type::tvoid, 0, LINKd, STCnothrow);
  FuncDeclaration *func = new FuncDeclaration (mod->loc, mod->loc,
					       Identifier::idPool (name), STCstatic, func_type);
  func->loc = Loc(mod->srcfile->toChars(), 1, 0);
  func->linkage = func_type->linkage;
  func->parent = mod;
  func->protection = PROTprivate;

  func->semantic (mod->scope);
  Statements *body = new Statements();
  for (size_t i = 0; i < tlsVars.length(); i++)
    {
      VarDeclaration *var = tlsVars[i];
      Expression *addr = (new VarExp (mod->loc, var))->addressOf();
      Expression *addr2 = new SymOffExp (mod->loc, var, var->type->size());
      Expressions* addrs = new Expressions();
      addrs->push (addr);
      addrs->push (addr2);

      Expression *call = CallExp::create (mod->loc, new IdentifierExp (Loc(), dg_arg->ident), addrs);
      body->push (new ExpStatement (mod->loc, call));
    }
  func->fbody = new CompoundStatement (mod->loc, body);
  func->semantic3 (mod->scope);
  func->toObjFile();

  return func->toSymbol();
}

// Same as build_call_function, but includes a gate to
// protect static ctors in templates getting called multiple times.

static Symbol *
build_ctor_function (const char *name, vec<FuncDeclaration *> functions, vec<VarDeclaration *> gates)
{
  tree expr_list = NULL_TREE;

  // If there is only one function, just return that
  if (functions.length() == 1 && gates.is_empty())
    return (functions[0])->toSymbol();

  // Increment gates first.
  for (size_t i = 0; i < gates.length(); i++)
    {
      tree var_decl = (gates[i])->toSymbol()->Stree;
      tree value = build2 (PLUS_EXPR, TREE_TYPE (var_decl), var_decl, integer_one_node);
      tree var_expr = modify_expr (var_decl, value);
      expr_list = compound_expr (expr_list, var_expr);
    }

  // Call Ctor Functions
  for (size_t i = 0; i < functions.length(); i++)
    {
      tree fndecl = (functions[i])->toSymbol()->Stree;
      tree call_expr = d_build_call_list (void_type_node, build_address (fndecl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    {
      FuncDeclaration *fd = build_simple_function (name, expr_list, false);
      return fd->toSymbol();
    }

  return NULL;
}

// Same as build_call_function, but calls all functions in
// the reverse order that the constructors were called in.

static Symbol *
build_dtor_function (const char *name, vec<FuncDeclaration *> functions)
{
  tree expr_list = NULL_TREE;

  // If there is only one function, just return that
  if (functions.length() == 1)
    return (functions[0])->toSymbol();

  for (int i = functions.length() - 1; i >= 0; i--)
    {
      tree fndecl = (functions[i])->toSymbol()->Stree;
      tree call_expr = d_build_call_list (void_type_node, build_address (fndecl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    {
      FuncDeclaration *fd = build_simple_function (name, expr_list, false);
      return fd->toSymbol();
    }

  return NULL;
}

// Same as build_call_function, but returns the Symbol to
// the function generated.

static Symbol *
build_unittest_function (const char *name, vec<FuncDeclaration *> functions)
{
  FuncDeclaration *fd = build_call_function (name, functions, false);
  return fd->toSymbol();
}

// Generate our module reference and append to _Dmodule_ref.

void
build_moduleinfo (Symbol *sym)
{
  // Generate:
  //  struct ModuleReference
  //  {
  //    void *next;
  //    ModuleReference m;
  //  }

  // struct ModuleReference in moduleinit.d
  Type *type = build_object_type();
  tree tmodref = build_two_field_type (ptr_type_node, build_ctype(type),
				       NULL, "next", "mod");
  tree nextfield = TYPE_FIELDS (tmodref);
  tree modfield = TREE_CHAIN (nextfield);

  // extern (C) ModuleReference *_Dmodule_ref;
  tree dmodule_ref = build_decl (BUILTINS_LOCATION, VAR_DECL,
				 get_identifier ("_Dmodule_ref"),
				 build_pointer_type (tmodref));
  d_keep (dmodule_ref);
  DECL_EXTERNAL (dmodule_ref) = 1;
  TREE_PUBLIC (dmodule_ref) = 1;

  // private ModuleReference modref = { next: null, mod: _ModuleInfo_xxx };
  tree modref = build_artificial_decl (tmodref, NULL_TREE, "__mod_ref");
  set_decl_location (modref, current_module_decl);
  d_keep (modref);

  vec<constructor_elt, va_gc> *ce = NULL;
  CONSTRUCTOR_APPEND_ELT (ce, nextfield, null_pointer_node);
  CONSTRUCTOR_APPEND_ELT (ce, modfield, build_address (sym->Stree));

  DECL_INITIAL (modref) = build_constructor (tmodref, ce);
  TREE_STATIC (DECL_INITIAL (modref)) = 1;
  rest_of_decl_compilation (modref, 1, 0);

  // Generate:
  //  void ___modinit()  // a static constructor
  //  {
  //    modref.next = _Dmodule_ref;
  //    _Dmodule_ref = &modref;
  //  }
  tree m1 = modify_expr (component_ref (modref, nextfield), dmodule_ref);
  tree m2 = modify_expr (dmodule_ref, build_address (modref));

  build_simple_function ("*__modinit", compound_expr (m1, m2), true);
}

