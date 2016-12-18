/* typeinfo.cc -- D runtime type identification.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"
#include "dfrontend/enum.h"
#include "dfrontend/module.h"
#include "dfrontend/mtype.h"
#include "dfrontend/scope.h"
#include "dfrontend/template.h"
#include "dfrontend/target.h"

#include "tree.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "stringpool.h"
#include "toplev.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "d-dmd-gcc.h"


/* Implements the visitor interface to build the TypeInfo layout of all
   TypeInfoDeclaration AST classes emitted from the D Front-end.
   All visit methods accept one parameter D, which holds the frontend AST
   of the TypeInfo class.  They also don't return any value, instead the
   generated symbol is cached internally and returned from the caller.  */

class TypeInfoVisitor : public Visitor
{
  tree type_;
  vec<constructor_elt, va_gc> *init_;

  /* Find the field identified by NAME and add its VALUE to the constructor.  */
  void set_field (const char *name, tree value)
  {
    tree field = find_aggregate_field (this->type_, get_identifier (name));

    if (field != NULL_TREE)
      CONSTRUCTOR_APPEND_ELT (this->init_, field, value);
  }

  /* Write out the __vptr field of class CD.  */
  void layout_vtable (ClassDeclaration *cd)
  {
    if (cd != NULL)
      this->set_field ("__vptr", build_address (get_vtable_decl (cd)));
  }

public:
  TypeInfoVisitor (tree type)
  {
    gcc_assert (POINTER_TYPE_P (type));
    this->type_ = TREE_TYPE (type);
    this->init_ = NULL;
  }

  /* Return the completed constructor for the TypeInfo record.  */

  tree result (void)
  {
    return build_struct_literal (this->type_, this->init_);
  }

  /* Layout of TypeInfo is:
	void **__vptr;
	void *__monitor;  */

  void visit (TypeInfoDeclaration *)
  {
    /* The vtable for TypeInfo.  */
    this->layout_vtable (Type::dtypeinfo);
  }

  /* Layout of TypeInfo_Const is:
	void **__vptr;
	void *__monitor;
	TypeInfo base;  */

  void visit (TypeInfoConstDeclaration *d)
  {
    Type *tm = d->tinfo->mutableOf ();
    tm = tm->merge ();

    /* The vtable for TypeInfo_Const.  */
    this->layout_vtable (Type::typeinfoconst);

    /* TypeInfo for the mutable type.  */
    this->set_field ("base", build_typeinfo (tm));
  }

  /* Layout of TypeInfo_Immutable is:
	void **__vptr;
	void *__monitor;
	TypeInfo base;  */

  void visit (TypeInfoInvariantDeclaration *d)
  {
    Type *tm = d->tinfo->mutableOf ();
    tm = tm->merge ();

    /* The vtable for TypeInfo_Invariant.  */
    this->layout_vtable (Type::typeinfoinvariant);

    /* TypeInfo for the mutable type.  */
    this->set_field ("base", build_typeinfo (tm));
  }

  /* Layout of TypeInfo_Shared is:
	void **__vptr;
	void *__monitor;
	TypeInfo base;  */

  void visit (TypeInfoSharedDeclaration *d)
  {
    Type *tm = d->tinfo->unSharedOf ();
    tm = tm->merge ();

    /* The vtable for TypeInfo_Shared.  */
    this->layout_vtable (Type::typeinfoshared);

    /* TypeInfo for the unshared type.  */
    this->set_field ("base", build_typeinfo (tm));
  }

  /* Layout of TypeInfo_Inout is:
	void **__vptr;
	void *__monitor;
	TypeInfo base;  */

  void visit (TypeInfoWildDeclaration *d)
  {
    Type *tm = d->tinfo->mutableOf ();
    tm = tm->merge ();

    /* The vtable for TypeInfo_Inout.  */
    this->layout_vtable (Type::typeinfowild);

    /* TypeInfo for the mutable type.  */
    this->set_field ("base", build_typeinfo (tm));
  }

  /* Layout of TypeInfo_Enum is:
	void **__vptr;
	void *__monitor;
	TypeInfo base;
	string name;
	void[] m_init;  */

  void visit (TypeInfoEnumDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tenum);
    TypeEnum *ti = (TypeEnum *) d->tinfo;
    EnumDeclaration *ed = ti->sym;

    /* The vtable for TypeInfo_Enum.  */
    this->layout_vtable (Type::typeinfoenum);

    /* TypeInfo for enum members.  */
    if (ed->memtype)
      this->set_field ("base", build_typeinfo (ed->memtype));

    /* Name of the enum declaration.  */
    this->set_field ("name", d_array_string (ed->toPrettyChars ()));

    /* Default initialiser for enum.  */
    if (ed->members && !d->tinfo->isZeroInit ())
      {
	tree type = build_ctype (Type::tvoid->arrayOf ());
	tree length = size_int (ed->type->size ());
	tree ptr = build_address (enum_initializer (ed));
	this->set_field ("m_init", d_array_value (type, length, ptr));
      }
  }

  /* Layout of TypeInfo_Pointer is:
	void **__vptr;
	void *__monitor;
	TypeInfo m_next;  */

  void visit (TypeInfoPointerDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tpointer);
    TypePointer *ti = (TypePointer *) d->tinfo;

    /* The vtable for TypeInfo_Pointer.  */
    this->layout_vtable (Type::typeinfopointer);

    /* TypeInfo for pointer-to type.  */
    this->set_field ("m_next", build_typeinfo (ti->next));
  }

  /* Layout of TypeInfo_Array is:
	void **__vptr;
	void *__monitor;
	TypeInfo value;  */

  void visit (TypeInfoArrayDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tarray);
    TypeDArray *ti = (TypeDArray *) d->tinfo;

    /* The vtable for TypeInfo_Array.  */
    this->layout_vtable (Type::typeinfoarray);

    /* TypeInfo for array of type.  */
    this->set_field ("value", build_typeinfo (ti->next));
  }

  /* Layout of TypeInfo_StaticArray is:
	void **__vptr;
	void *__monitor;
	TypeInfo value;
	size_t len;  */

  void visit (TypeInfoStaticArrayDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tsarray);
    TypeSArray *ti = (TypeSArray *) d->tinfo;

    /* The vtable for TypeInfo_StaticArray.  */
    this->layout_vtable (Type::typeinfostaticarray);

    /* TypeInfo for array of type.  */
    this->set_field ("value", build_typeinfo (ti->next));

    /* Static array length.  */
    this->set_field ("len", size_int (ti->dim->toInteger ()));
  }

  /* Layout of TypeInfo_AssociativeArray is:
	void **__vptr;
	void *__monitor;
	TypeInfo value;
	TypeInfo key;  */

  void visit (TypeInfoAssociativeArrayDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Taarray);
    TypeAArray *ti = (TypeAArray *) d->tinfo;

    /* The vtable for TypeInfo_AssociativeArray.  */
    this->layout_vtable (Type::typeinfoassociativearray);

    /* TypeInfo for value of type.  */
    this->set_field ("value", build_typeinfo (ti->next));

    /* TypeInfo for index of type.  */
    this->set_field ("key", build_typeinfo (ti->index));
  }

  /* Layout of TypeInfo_Vector is:
	void **__vptr;
	void *__monitor;
	TypeInfo base;  */

  void visit (TypeInfoVectorDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tvector);
    TypeVector *ti = (TypeVector *) d->tinfo;

    /* The vtable for TypeInfo_Vector.  */
    this->layout_vtable (Type::typeinfovector);

    /* TypeInfo for equivalent static array.  */
    this->set_field ("base", build_typeinfo (ti->basetype));
  }

  /* Layout of TypeInfo_Function is:
	void **__vptr;
	void *__monitor;
	TypeInfo next;
	string deco;  */

  void visit (TypeInfoFunctionDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tfunction && d->tinfo->deco != NULL);
    TypeFunction *ti = (TypeFunction *) d->tinfo;

    /* The vtable for TypeInfo_Function.  */
    this->layout_vtable (Type::typeinfofunction);

    /* TypeInfo for function return value.  */
    this->set_field ("next", build_typeinfo (ti->next));

    /* Mangled name of function declaration.  */
    this->set_field ("deco", d_array_string (d->tinfo->deco));
  }

  /* Layout of TypeInfo_Delegate is:
	void **__vptr;
	void *__monitor;
	TypeInfo next;
	string deco;  */

  void visit (TypeInfoDelegateDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tdelegate && d->tinfo->deco != NULL);
    TypeDelegate *ti = (TypeDelegate *) d->tinfo;

    /* The vtable for TypeInfo_Delegate.  */
    this->layout_vtable (Type::typeinfodelegate);

    /* TypeInfo for delegate return value.  */
    this->set_field ("next", build_typeinfo (ti->next));

    /* Mangled name of delegate declaration.  */
    this->set_field ("deco", d_array_string (d->tinfo->deco));
  }

  /* Generation of ClassInfo is done in ClassDeclaration::toObjFile.  */

  void visit (TypeInfoClassDeclaration *)
  {
    gcc_unreachable ();
  }

  /* Layout of TypeInfo_Interface is:
	void **__vptr;
	void *__monitor;
	TypeInfo_Class info;  */

  void visit (TypeInfoInterfaceDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tclass);
    TypeClass *ti = (TypeClass *) d->tinfo;

    if (!ti->sym->vclassinfo)
      ti->sym->vclassinfo = TypeInfoClassDeclaration::create (ti);

    /* The vtable for TypeInfo_Interface.  */
    this->layout_vtable (Type::typeinfointerface);

    /* TypeInfo for class inheriting the interface.  */
    tree tidecl = get_typeinfo_decl (ti->sym->vclassinfo);
    this->set_field ("info", build_address (tidecl));
  }

  /* Layout of TypeInfo_Struct is:
	void **__vptr;
	void *__monitor;
	string name;
	void[] m_init;
	hash_t function(in void*) xtoHash;
	bool function(in void*, in void*) xopEquals;
	int function(in void*, in void*) xopCmp;
	string function(const(void)*) xtoString;
	StructFlags m_flags;
	void function(void*) xdtor;
	void function(void*) xpostblit;
	uint m_align;
	version (X86_64)
	    TypeInfo m_arg1;
	    TypeInfo m_arg2;
	immutable(void)* xgetRTInfo;  */

  void visit (TypeInfoStructDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tstruct);
    TypeStruct *ti = (TypeStruct *) d->tinfo;
    StructDeclaration *sd = ti->sym;

    /* The vtable for TypeInfo_Struct.  */
    this->layout_vtable (Type::typeinfostruct);

    if (!sd->members)
      return;

    /* Send member functions to backend here.  */
    TemplateInstance *tinst = sd->isInstantiated ();
    if (tinst && !tinst->needsCodegen ())
      {
	gcc_assert (tinst->minst || sd->requestTypeInfo);

	if (sd->xeq && sd->xeq != StructDeclaration::xerreq)
	  sd->xeq->toObjFile ();

	if (sd->xcmp && sd->xcmp != StructDeclaration::xerrcmp)
	  sd->xcmp->toObjFile ();

	if (FuncDeclaration *ftostr = search_toString (sd))
	  ftostr->toObjFile ();

	if (sd->xhash)
	  sd->xhash->toObjFile ();

	if (sd->postblit)
	  sd->postblit->toObjFile ();

	if (sd->dtor)
	  sd->dtor->toObjFile ();
      }

    /* Name of the struct declaration.  */
    this->set_field ("name", d_array_string (sd->toPrettyChars ()));

    /* Default initialiser for struct.  */
    tree type = build_ctype (Type::tvoid->arrayOf ());
    tree length = size_int (sd->structsize);
    tree ptr = (sd->zeroInit) ? null_pointer_node :
      build_address (aggregate_initializer (sd));
    this->set_field ("m_init", d_array_value (type, length, ptr));

    /* hash_t function (in void*) xtoHash;  */
    FuncDeclaration *fdx = sd->xhash;
    if (fdx)
      {
	TypeFunction *tf = (TypeFunction *) fdx->type;
	gcc_assert (tf->ty == Tfunction);

	this->set_field ("xtoHash", build_address (get_symbol_decl (fdx)));

	if (!tf->isnothrow || tf->trust == TRUSTsystem)
	  warning (fdx->loc, "toHash() must be declared as extern (D) size_t "
		   "toHash() const nothrow @safe, not %s", tf->toChars ());
      }

    /* bool function(in void*, in void*) xopEquals;  */
    if (sd->xeq)
      this->set_field ("xopEquals", build_address (get_symbol_decl (sd->xeq)));

    /* int function(in void*, in void*) xopCmp;  */
    if (sd->xcmp)
      this->set_field ("xopCmp", build_address (get_symbol_decl (sd->xcmp)));

    /* string function(const(void)*) xtoString;  */
    fdx = search_toString (sd);
    if (fdx)
      this->set_field ("xtoString", build_address (get_symbol_decl (fdx)));

    /* StructFlags m_flags;  */
    StructFlags::Type m_flags = 0;
    if (ti->hasPointers ())
      m_flags |= StructFlags::hasPointers;
    this->set_field ("m_flags", size_int (m_flags));

    /* void function(void*) xdtor;  */
    if (sd->dtor)
      this->set_field ("xdtor", build_address (get_symbol_decl (sd->dtor)));

    /* void function(void*) xpostblit;  */
    if (sd->postblit && !(sd->postblit->storage_class & STCdisable))
      this->set_field ("xpostblit", build_address (get_symbol_decl (sd->postblit)));

    /* uint m_align;  */
    this->set_field ("m_align", size_int (ti->alignsize ()));

    if (global.params.is64bit)
      {
	/* TypeInfo m_arg1;  */
	if (sd->arg1type)
	  this->set_field ("m_arg1", build_typeinfo (sd->arg1type));

	/* TypeInfo m_arg2;  */
	if (sd->arg2type)
	  this->set_field ("m_arg2", build_typeinfo (sd->arg2type));
      }

    /* immutable(void)* xgetRTInfo;  */
    if (sd->getRTInfo)
      this->set_field ("xgetRTInfo", build_expr (sd->getRTInfo, true));
    else if (m_flags & StructFlags::hasPointers)
      this->set_field ("xgetRTInfo", size_one_node);
  }

  /* Layout of TypeInfo_Tuple is:
	void **__vptr;
	void *__monitor;
	TypeInfo[] elements;  */

  void visit (TypeInfoTupleDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Ttuple);
    TypeTuple *ti = (TypeTuple *) d->tinfo;

    /* The vtable for TypeInfo_Tuple.  */
    this->layout_vtable (Type::typeinfotypelist);

    /* TypeInfo[] elements;  */
    Type *satype = Type::tvoidptr->sarrayOf (ti->arguments->dim);
    vec<constructor_elt, va_gc> *elms = NULL;
    for (size_t i = 0; i < ti->arguments->dim; i++)
      {
	Parameter *arg = (*ti->arguments)[i];
	CONSTRUCTOR_APPEND_ELT (elms, size_int (i),
				build_typeinfo (arg->type));
      }
    tree ctor = build_constructor (build_ctype (satype), elms);
    tree decl = build_artificial_decl (TREE_TYPE (ctor), ctor);

    tree type = build_ctype (Type::tvoid->arrayOf ());
    tree length = size_int (ti->arguments->dim);
    tree ptr = build_address (decl);
    this->set_field ("elements", d_array_value (type, length, ptr));

    d_pushdecl (decl);
    rest_of_decl_compilation (decl, 1, 0);
  }
};


/* Main entry point for TypeInfoVisitor interface to generate
   TypeInfo constructor for the TypeInfoDeclaration AST class D.  */

tree
layout_typeinfo (TypeInfoDeclaration *d)
{
  TypeInfoVisitor v = TypeInfoVisitor (build_ctype (d->type));
  d->accept (&v);
  return v.result ();
}

/* Returns typeinfo reference for TYPE.  */

tree
build_typeinfo (Type *type)
{
  gcc_assert (type->ty != Terror);
  genTypeInfo (type, NULL);
  return build_address (get_typeinfo_decl (type->vtinfo));
}


/* Get the exact TypeInfo for TYPE.  */

void
genTypeInfo (Type *type, Scope *sc)
{
  if (!Type::dtypeinfo)
    {
      type->error (Loc (), "TypeInfo not found. "
		   "object.d may be incorrectly installed or corrupt");
      fatal ();
    }

  /* Do this since not all Type's are merge'd.  */
  Type *t = type->merge2 ();
  if (!t->vtinfo)
    {
      /* Does both 'shared' and 'shared const'.  */
      if (t->isShared ())
	t->vtinfo = TypeInfoSharedDeclaration::create (t);
      else if (t->isConst ())
	t->vtinfo = TypeInfoConstDeclaration::create (t);
      else if (t->isImmutable ())
	t->vtinfo = TypeInfoInvariantDeclaration::create (t);
      else if (t->isWild ())
	t->vtinfo = TypeInfoWildDeclaration::create (t);
      else if (t->ty == Tpointer)
	t->vtinfo = TypeInfoPointerDeclaration::create (type);
      else if (t->ty == Tarray)
	t->vtinfo = TypeInfoArrayDeclaration::create (type);
      else if (t->ty == Tsarray)
	t->vtinfo = TypeInfoStaticArrayDeclaration::create (type);
      else if (t->ty == Taarray)
	t->vtinfo = TypeInfoAssociativeArrayDeclaration::create (type);
      else if (t->ty == Tstruct)
	t->vtinfo = TypeInfoStructDeclaration::create (type);
      else if (t->ty == Tvector)
	t->vtinfo = TypeInfoVectorDeclaration::create (type);
      else if (t->ty == Tenum)
	t->vtinfo = TypeInfoEnumDeclaration::create (type);
      else if (t->ty == Tfunction)
	t->vtinfo = TypeInfoFunctionDeclaration::create (type);
      else if (t->ty == Tdelegate)
	t->vtinfo = TypeInfoDelegateDeclaration::create (type);
      else if (t->ty == Ttuple)
	t->vtinfo = TypeInfoTupleDeclaration::create (type);
      else if (t->ty == Tclass)
	{
	  if (((TypeClass *) type)->sym->isInterfaceDeclaration ())
	    t->vtinfo = TypeInfoInterfaceDeclaration::create (type);
	  else
	    t->vtinfo = TypeInfoClassDeclaration::create (type);
	}
      else
        t->vtinfo = TypeInfoDeclaration::create (type, 0);

      gcc_assert (t->vtinfo);

      /* If this has a custom implementation in rt/typeinfo, then
	 do not generate a COMDAT for it.  */
      bool builtinTypeInfo = false;
      if (t->isTypeBasic () || t->ty == Tclass)
	builtinTypeInfo = !t->mod;
      else if (t->ty == Tarray)
	{
	  Type *next = t->nextOf ();
	  /* Strings are so common, make them builtin.  */
	  builtinTypeInfo = !t->mod
	    && ((next->isTypeBasic () != NULL && !next->mod)
		|| (next->ty == Tchar && next->mod == MODimmutable)
		|| (next->ty == Tchar && next->mod == MODconst));
	}

      if (!builtinTypeInfo)
	{
	  if (sc)
	    {
	      /* Find module that will go all the way to an object file.  */
	      Module *m = sc->module->importedFrom;
	      m->members->push (t->vtinfo);
	    }
	  else
	    t->vtinfo->toObjFile ();
	}
    }
  /* Types aren't merged, but we can share the vtinfo's.  */
  if (!type->vtinfo)
    type->vtinfo = t->vtinfo;

  gcc_assert (type->vtinfo != NULL);
}

Type *
getTypeInfoType (Type *type, Scope *sc)
{
  gcc_assert (type->ty != Terror);
  genTypeInfo (type, sc);
  return type->vtinfo->type;
}

/* Bugzilla 14425: TypeInfo_Struct would refer the members of struct
   (e.g. opEquals via xopEquals field), so if it's instantiated in
   speculative context, TypeInfo creation should also be stopped to
   avoid 'unresolved symbol' linker errors.  */

bool
isSpeculativeType (Type *t)
{
  class SpeculativeTypeVisitor : public Visitor
  {
  public:
    bool result;

    SpeculativeTypeVisitor (void) : result(false) {}

    void visit (Type *t)
    {
      Type *tb = t->toBasetype ();
      if (tb != t)
	tb->accept (this);
    }

    void visit (TypeNext *t)
    {
      if (t->next)
	t->next->accept (this);
    }

    void visit (TypeBasic *) { }

    void visit (TypeVector *t)
    {
      t->basetype->accept (this);
    }

    void visit (TypeAArray *t)
    {
      t->index->accept (this);
      visit ((TypeNext *)t);
    }

    void visit (TypeFunction *t)
    {
      visit ((TypeNext *)t);
      /* Currently TypeInfo_Function doesn't store parameter types.  */
    }

    void visit (TypeStruct *t)
    {
      StructDeclaration *sd = t->sym;
      if (TemplateInstance *ti = sd->isInstantiated ())
	{
	  if (!ti->needsCodegen ())
	    {
	      if (ti->minst || sd->requestTypeInfo)
		return;

	      result |= true;
	      return;
	    }
	}
    }

    void visit (TypeClass *) { }

    void visit (TypeTuple *t)
    {
      if (t->arguments)
	{
	  for (size_t i = 0; i < t->arguments->dim; i++)
	    {
	      Type *tprm = (*t->arguments)[i]->type;
	      if (tprm)
		tprm->accept (this);
	      if (result)
		return;
	    }
	}
    }
  };

  SpeculativeTypeVisitor v;
  t->accept (&v);
  return v.result;
}
