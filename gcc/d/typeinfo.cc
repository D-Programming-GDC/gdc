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

  /* Find the field at OFFSET and add its VALUE to the constructor.  */

  void set_field (size_t offset, tree value)
  {
    tree field = find_aggregate_field (this->type_, NULL_TREE,
				       size_int (offset));
    if (field != NULL_TREE)
      CONSTRUCTOR_APPEND_ELT (this->init_, field, value);
  }

  /* Write out the __vptr field of class CD.  */

  void layout_vtable (ClassDeclaration *cd)
  {
    if (cd != NULL)
      this->set_field ("__vptr", build_address (get_vtable_decl (cd)));
  }

  /* Write out the interfaces field of class CD.  */

  void layout_interfaces (ClassDeclaration *cd)
  {
    Type *tinterface = Type::typeinterface->type;
    tree type = build_ctype (tinterface);
    size_t offset = Type::typeinfoclass->structsize;
    tree csym = build_address (get_classinfo_decl (cd));

    /* Put out the offset to where vtblInterfaces are written.  */
    tree value = d_array_value (build_ctype (tinterface->arrayOf ()),
				size_int (cd->vtblInterfaces->dim),
				build_offset (csym, size_int (offset)));
    this->set_field ("interfaces", value);

    /* Layout of Interface is:
       TypeInfo_Class classinfo;
       void*[] vtbl;
       size_t offset;  */
    vec<constructor_elt, va_gc> *elms = NULL;;

    for (size_t i = 0; i < cd->vtblInterfaces->dim; i++)
      {
	BaseClass *b = (*cd->vtblInterfaces)[i];
	ClassDeclaration *id = b->sym;
	vec<constructor_elt, va_gc> *v = NULL;
	tree field;

	/* Fill in the vtbl[].  */
	b->fillVtbl (cd, &b->vtbl, 1);

	/* ClassInfo for the interface.  */
	field = find_aggregate_field (type, get_identifier ("classinfo"));
	if (field)
	  {
	    value = build_address (get_classinfo_decl (id));
	    CONSTRUCTOR_APPEND_ELT (v, field, value);
	  }

	if (!cd->isInterfaceDeclaration ())
	  {
	    /* The vtable of the interface.  */
	    field = find_aggregate_field (type, get_identifier ("vtbl"));
	    if (field)
	      {
		size_t voffset = base_vtable_offset (cd, b);
		value = d_array_value (build_ctype (Type::tvoidptr->arrayOf ()),
				       size_int (id->vtbl.dim),
				       build_offset (csym, size_int (voffset)));
		CONSTRUCTOR_APPEND_ELT (v, field, value);
	      }
	  }

	/* The 'this' offset.  */
	field = find_aggregate_field (type, get_identifier ("offset"));
	if (field)
	  {
	    value = size_int (b->offset);
	    CONSTRUCTOR_APPEND_ELT (v, field, value);
	  }

	CONSTRUCTOR_APPEND_ELT(elms, size_int (i), build_constructor (type, v));
      }

    /* Put out array of Interfaces.  */
    size_t dim = cd->vtblInterfaces->dim;
    value = build_constructor (make_array_type (tinterface, dim), elms);
    this->set_field (offset, value);
  }

  /* Write out the interfacing vtable[] of base class BCD that will be accessed
     from the overriding class CD.  If both are the same class, then this will
     be it's own vtable.  INDEX is the offset in the interfaces array of the
     base class where the Interface reference can be found.
     This must be mirrored with base_vtable_offset().  */

  void layout_base_vtable (ClassDeclaration *cd, ClassDeclaration *bcd,
			   size_t index)
  {
    BaseClass *bs = (*bcd->vtblInterfaces)[index];
    ClassDeclaration *id = bs->sym;
    size_t voffset = base_vtable_offset (cd, bs);
    vec<constructor_elt, va_gc> *elms = NULL;
    FuncDeclarations bvtbl;

    if (id->vtbl.dim == 0 || voffset == ~0u)
      return;

    /* Fill bvtbl with the functions we want to put out.  */
    if (cd != bcd && !bs->fillVtbl (cd, &bvtbl, 0))
      return;

    /* First entry is struct Interface reference.  */
    if (id->vtblOffset () && Type::typeinterface)
      {
	size_t offset = Type::typeinfoclass->structsize;
	offset += (index * Type::typeinterface->structsize);
	tree value = build_offset (build_address (get_classinfo_decl (bcd)),
				   size_int (offset));
	CONSTRUCTOR_APPEND_ELT (elms, size_zero_node, value);
      }

    for (size_t i = id->vtblOffset() ? 1 : 0; i < id->vtbl.dim; i++)
      {
	FuncDeclaration *fd = (cd == bcd) ? bs->vtbl[i] : bvtbl[i];
	if (fd != NULL)
	  {
	    tree value = build_address (make_thunk (fd, bs->offset));
	    CONSTRUCTOR_APPEND_ELT (elms, size_int (i), value);
	  }
      }

    tree value = build_constructor (make_array_type (Type::tvoidptr, id->vtbl.dim),
				    elms);
    this->set_field (voffset, value);
  }


public:
  TypeInfoVisitor (tree type)
  {
    this->type_ = type;
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
	tree ptr = build_address (enum_initializer_decl (ed));
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

  /* Layout of ClassInfo/TypeInfo_Class is:
	void **__vptr;
	void *__monitor;
	byte[] m_init;
	string name;
	void*[] vtbl;
	Interface[] interfaces;
	TypeInfo_Class base;
	void *destructor;
	void function(Object) classInvariant;
	ClassFlags m_flags;
	void *deallocator;
	OffsetTypeInfo[] m_offTi;
	void function(Object) defaultConstructor;
	immutable(void)* m_RTInfo;

     Information relating to interfaces, and their vtables are layed out
     immediately after the named fields, if there is anything to write.  */

  void visit (TypeInfoClassDeclaration *d)
  {
    gcc_assert (d->tinfo->ty == Tclass);
    TypeClass *ti = (TypeClass *) d->tinfo;
    ClassDeclaration *cd = ti->sym;

    /* The vtable for ClassInfo.  */
    this->layout_vtable (Type::typeinfoclass);

    if (!cd->members)
      return;

    if (!cd->isInterfaceDeclaration ())
      {
	/* Default initialiser for class.  */
	tree value = d_array_value (build_ctype (Type::tint8->arrayOf ()),
				    size_int (cd->structsize),
				    build_address (aggregate_initializer_decl (cd)));
	this->set_field ("m_init", value);

	/* Name of the class declaration.  */
	const char *name = cd->ident->toChars ();
	if (!(strlen (name) > 9 && memcmp (name, "TypeInfo_", 9) == 0))
	  name = cd->toPrettyChars ();
	this->set_field ("name", d_array_string (name));

	/* The vtable of the class declaration.  */
	value = d_array_value (build_ctype (Type::tvoidptr->arrayOf ()),
			       size_int (cd->vtbl.dim),
			       build_address (get_vtable_decl (cd)));
	this->set_field ("vtbl", value);

	/* Array of base interfaces that have their own vtable.  */
	if (cd->vtblInterfaces->dim && Type::typeinterface)
	  this->layout_interfaces (cd);

	/* TypeInfo_Class base;  */
	if (cd->baseClass)
	  {
	    tree base = get_classinfo_decl (cd->baseClass);
	    this->set_field ("base", build_address (base));
	  }

	/* void *destructor;  */
	if (cd->dtor)
	  {
	    tree dtor = get_symbol_decl (cd->dtor);
	    this->set_field ("destructor", build_address (dtor));
	  }

	/* void function(Object) classInvariant;  */
	if (cd->inv)
	  {
	    tree inv = get_symbol_decl (cd->inv);
	    this->set_field ("classInvariant", build_address (inv));
	  }

	/* ClassFlags m_flags;  */
	ClassFlags::Type flags = ClassFlags::hasOffTi;
	if (cd->isCOMclass ())
	  flags |= ClassFlags::isCOMclass;

	if (cd->isCPPclass ())
	  flags |= ClassFlags::isCPPclass;

	flags |= ClassFlags::hasGetMembers;
	flags |= ClassFlags::hasTypeInfo;

	if (cd->ctor)
	  flags |= ClassFlags::hasCtor;

	for (ClassDeclaration *bcd = cd; bcd; bcd = bcd->baseClass)
	  {
	    if (bcd->dtor)
	      {
		flags |= ClassFlags::hasDtor;
		break;
	      }
	  }

	if (cd->isabstract)
	  flags |= ClassFlags::isAbstract;

	for (ClassDeclaration *bcd = cd; bcd; bcd = bcd->baseClass)
	  {
	    if (!bcd->members)
	      continue;

	    for (size_t i = 0; i < cd->members->dim; i++)
	      {
		Dsymbol *sm = (*cd->members)[i];
		if (sm->hasPointers ())
		  goto Lhaspointers;
	      }
	  }

	flags |= ClassFlags::noPointers;

    Lhaspointers:
	this->set_field ("m_flags", size_int (flags));

	/* void *deallocator;  */
	if (cd->aggDelete)
	  {
	    tree ddtor = get_symbol_decl (cd->aggDelete);
	    this->set_field ("deallocator", build_address (ddtor));
	  }

	/* void function(Object) defaultConstructor;  */
	if (cd->defaultCtor && !(cd->defaultCtor->storage_class & STCdisable))
	  {
	    tree dctor = get_symbol_decl (cd->defaultCtor);
	    this->set_field ("defaultConstructor", build_address (dctor));
	  }

	/* immutable(void)* m_RTInfo;  */
	if (cd->getRTInfo)
	  this->set_field ("m_RTInfo", build_expr (cd->getRTInfo, true));
	else if (!(flags & ClassFlags::noPointers))
	  this->set_field ("m_RTInfo", size_one_node);
      }
    else
      {
	/* Name of the interface declaration.  */
	this->set_field ("name", d_array_string (cd->toPrettyChars ()));

	/* Array of base interfaces that have their own vtable.  */
	if (cd->vtblInterfaces->dim && Type::typeinterface)
	  this->layout_interfaces (cd);

	/* ClassFlags m_flags;  */
	ClassFlags::Type flags = ClassFlags::hasOffTi;
	flags |= ClassFlags::hasTypeInfo;
	if (cd->isCOMinterface ())
	  flags |= ClassFlags::isCOMclass;

	this->set_field ("m_flags", size_int (flags));

	/* immutable(void)* m_RTInfo;  */
	if (cd->getRTInfo)
	  this->set_field ("m_RTInfo", build_expr (cd->getRTInfo, true));
      }

    /* Put out this class' interface vtables[].  */
    for (size_t i = 0; i < cd->vtblInterfaces->dim; i++)
      this->layout_base_vtable (cd, cd, i);

    /* Put out the overriding interface vtables[].  */
    for (ClassDeclaration *bcd = cd->baseClass; bcd; bcd = bcd->baseClass)
      {
	for (size_t i = 0; i < bcd->vtblInterfaces->dim; i++)
	  this->layout_base_vtable (cd, bcd, i);
      }
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
	  build_decl_tree (sd->xeq);

	if (sd->xcmp && sd->xcmp != StructDeclaration::xerrcmp)
	  build_decl_tree (sd->xcmp);

	if (FuncDeclaration *ftostr = search_toString (sd))
	  build_decl_tree (ftostr);

	if (sd->xhash)
	  build_decl_tree (sd->xhash);

	if (sd->postblit)
	  build_decl_tree (sd->postblit);

	if (sd->dtor)
	  build_decl_tree (sd->dtor);
      }

    /* Name of the struct declaration.  */
    this->set_field ("name", d_array_string (sd->toPrettyChars ()));

    /* Default initialiser for struct.  */
    tree type = build_ctype (Type::tvoid->arrayOf ());
    tree length = size_int (sd->structsize);
    tree ptr = (sd->zeroInit) ? null_pointer_node :
      build_address (aggregate_initializer_decl (sd));
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
  tree type = build_ctype (d->type);
  gcc_assert (POINTER_TYPE_P (type));

  TypeInfoVisitor v = TypeInfoVisitor (TREE_TYPE (type));
  d->accept (&v);
  return v.result ();
}

/* Like layout_typeinfo, but generates the TypeInfo_Class for
   the class or interface declaration CD.  */

tree
layout_classinfo (ClassDeclaration *cd)
{
  tree type = TREE_TYPE (get_classinfo_decl (cd));
  /* The classinfo decl initialized here to accomodate both class and interfaces
     is thrown away immediately after exiting.  So the use of placement new is
     deliberate to avoid making more heap allocations than necessary.  */
  char buf[sizeof (TypeInfoClassDeclaration)];
  TypeInfoClassDeclaration *d = new (buf) TypeInfoClassDeclaration (cd->type);

  TypeInfoVisitor v = TypeInfoVisitor (type);
  d->accept (&v);
  return v.result ();
}

void
layout_cpp_typeinfo (ClassDeclaration *cd)
{
  gcc_assert (cd->isCPPclass ());

  tree decl = get_cpp_typeinfo_decl (cd);
  tree type = TREE_TYPE (decl);

  vec<constructor_elt, va_gc> *init = NULL;
  tree f_vptr = TYPE_FIELDS (type);
  tree f_typeinfo = DECL_CHAIN (DECL_CHAIN (f_vptr));

  /* Use the vtable of __cpp_type_info_ptr, the EH personality routine
     expects this, as it uses .classinfo identity comparison to test for
     C++ catch handlers.  */
  tree vptr = get_vtable_decl (ClassDeclaration::cpp_type_info_ptr);
  CONSTRUCTOR_APPEND_ELT (init, f_vptr, build_address (vptr));

  /* Let C++ do the RTTI generation, and just reference the symbol as
     extern, the knowing the underlying type is not required.  */
  const char *ident = cppTypeInfoMangle (cd);
  tree typeinfo = build_decl (BUILTINS_LOCATION, VAR_DECL,
			      get_identifier (ident), unknown_type_node);
  DECL_EXTERNAL (typeinfo) = 1;
  DECL_ARTIFICIAL (typeinfo) = 1;
  TREE_READONLY (typeinfo) = 1;
  CONSTRUCTOR_APPEND_ELT (init, f_typeinfo, build_address (typeinfo));

  /* Build the initializer and emit.  */
  DECL_INITIAL (decl) = build_constructor (type, init);
  d_pushdecl (decl);
  rest_of_decl_compilation (decl, 1, 0);
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
        t->vtinfo = TypeInfoDeclaration::create (type);

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
	      Module *m = sc->_module->importedFrom;
	      m->members->push (t->vtinfo);
	    }
	  else
	    build_decl_tree (t->vtinfo);
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
