/* d-target.h -- Data structure definitions for target-specific D behavior.
   Copyright (C) 2017-2018 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_D_TARGET_H
#define GCC_D_TARGET_H

/* Used by target as parameter for d_vaarg_ti_types */
enum d_tinfo_kind
{
  D_TK_TYPEINFO_TYPE,		/* object.TypeInfo  */
  D_TK_CLASSINFO_TYPE,		/* object.TypeInfo_Class  */
  D_TK_INTERFACE_TYPE,		/* object.TypeInfo_Interface  */
  D_TK_STRUCT_TYPE,		/* object.TypeInfo_Struct  */
  D_TK_POINTER_TYPE,		/* object.TypeInfo_Pointer  */
  D_TK_ARRAY_TYPE,		/* object.TypeInfo_Array  */
  D_TK_STATICARRAY_TYPE,		/* object.TypeInfo_StaticArray  */
  D_TK_ASSOCIATIVEARRAY_TYPE,	/* object.TypeInfo_AssociativeArray  */
  D_TK_VECTOR_TYPE,		/* object.TypeInfo_Vector  */
  D_TK_ENUMERAL_TYPE,		/* object.TypeInfo_Enum  */
  D_TK_FUNCTION_TYPE,		/* object.TypeInfo_Function  */
  D_TK_DELEGATE_TYPE,		/* object.TypeInfo_Delegate  */
  D_TK_TYPELIST_TYPE,		/* object.TypeInfo_Tuple  */
  D_TK_CONST_TYPE,		/* object.TypeInfo_Const  */
  D_TK_IMMUTABLE_TYPE,		/* object.TypeInfo_Invariant  */
  D_TK_SHARED_TYPE,		/* object.TypeInfo_Shared  */
  D_TK_INOUT_TYPE,		/* object.TypeInfo_Inout  */
  D_TK_CPPTI_TYPE,		/* object.__cpp_type_info_ptr  */
  D_TK_END,
};

#define DEFHOOKPOD(NAME, DOC, TYPE, INIT) TYPE NAME;
#define DEFHOOK(NAME, DOC, TYPE, PARAMS, INIT) TYPE (* NAME) PARAMS;
#define DEFHOOK_UNDOC DEFHOOK
#define HOOKSTRUCT(FRAGMENT) FRAGMENT

#include "d-target.def"

/* Each target can provide their own.  */
extern struct gcc_targetdm targetdm;

/* Used by target to add predefined version idenditiers.  */
extern void d_add_builtin_version (const char *);

/* default implementation for d_vaarg_ti_types */
extern tree d_default_vaarg_ti_types (d_tinfo_kind);

/* default implementation for d_vaarg_ti_values */
extern tree d_default_vaarg_ti_values (d_tinfo_kind, tree);

#endif /* GCC_D_TARGET_H */
