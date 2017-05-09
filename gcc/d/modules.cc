/* modules.cc -- D module initialization and termination.
   Copyright (C) 2017 Free Software Foundation, Inc.

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
#include "dfrontend/module.h"
#include "dfrontend/statement.h"

#include "tree.h"
#include "fold-const.h"
#include "tm.h"
#include "function.h"
#include "cgraph.h"
#include "stor-layout.h"
#include "toplev.h"
#include "target.h"
#include "common/common-target.h"
#include "stringpool.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "id.h"


/* D generates module information to inform the runtime library which modules
   needs some kind of special handling.  All `static this()', `static ~this()',
   and `unittest' functions for a given module are aggregated into a single
   function - one for each kind - and a pointer to that function is inserted
   into the ModuleInfo instance for that module.

   Module information for a particular module is indicated with an ABI defined
   structure derived from ModuleInfo.  ModuleInfo is a variably sized struct
   with two fixed base fields.  The first field `flags' determines what
   information is packed immediately after the record type.

   Like TypeInfo, the runtime library provides the definitions of the ModuleInfo
   structure, as well as accessors for the variadic fields.  So we only define
   layout compatible POD_structs for ModuleInfo.  */

/* The internally represented ModuleInfo type.  */
static tree moduleinfo_type_node;

/* Record information about module initialization, termination,
   unit testing, and thread local storage in the compilation.  */

struct module_info
{
  vec<tree, va_gc> *ctors;
  vec<tree, va_gc> *dtors;
  vec<tree, va_gc> *ctorgates;

  vec<tree, va_gc> *sharedctors;
  vec<tree, va_gc> *shareddtors;
  vec<tree, va_gc> *sharedctorgates;

  vec<tree, va_gc> *unitTests;
  vec<VarDeclaration *> tlsVars;
};

/* These must match the values in libdruntime/object_.d.  */

enum module_info_flags
{
  MIctorstart	    = 0x1,
  MIctordone	    = 0x2,
  MIstandalone	    = 0x4,
  MItlsctor	    = 0x8,
  MItlsdtor	    = 0x10,
  MIctor	    = 0x20,
  MIdtor	    = 0x40,
  MIxgetMembers	    = 0x80,
  MIictor	    = 0x100,
  MIunitTest	    = 0x200,
  MIimportedModules = 0x400,
  MIlocalClasses    = 0x800,
  MIname	    = 0x1000,
};

/* The ModuleInfo information structure for the module currently being compiled.
   Assuming that only ever process one at a time.  */

static module_info *current_moduleinfo;

/* The declaration of the current module being compiled.  */

static Module *current_module_decl;

/* Static constructors and destructors (not D `static this').  */

static vec<tree, va_gc> *static_ctor_list;
static vec<tree, va_gc> *static_dtor_list;

/* Returns an internal function identified by IDENT.  This is used
   by both module initialization and dso handlers.  */

static FuncDeclaration *
get_internal_fn (tree ident)
{
  Module *mod = current_module_decl;
  const char *name = IDENTIFIER_POINTER (ident);

  if (!mod)
    mod = Module::rootModule;

  if (name[0] == '*')
    {
      tree s = make_internal_name (mod, name + 1, "FZv");
      name = IDENTIFIER_POINTER (s);
    }

  TypeFunction *tf = TypeFunction::create (0, Type::tvoid, 0, LINKc);
  FuncDeclaration *fd = new FuncDeclaration (mod->loc, mod->loc,
					     Identifier::idPool (name),
					     STCstatic, tf);
  fd->loc = Loc (mod->srcfile->toChars (), 1, 0);
  fd->linkage = tf->linkage;
  fd->parent = mod;
  fd->protection = PROTprivate;
  fd->semanticRun = PASSsemantic3done;

  return fd;
}

/* Generate an internal function identified by IDENT.
   The function body to add is in EXPR.  */

static tree
build_internal_fn (tree ident, tree expr)
{
  FuncDeclaration *fd = get_internal_fn (ident);
  tree decl = get_symbol_decl (fd);

  tree old_context = start_function (fd);
  rest_of_decl_compilation (decl, 1, 0);
  add_stmt (expr);
  finish_function (old_context);

  /* D static ctors, static dtors, unittests, and the ModuleInfo
     chain function are always private.  */
  TREE_PUBLIC (decl) = 0;
  TREE_USED (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;

  return decl;
}

/* Build and emit a function identified by IDENT that increments (in order)
   all variables in GATES, then calls the list of functions in FUNCTIONS.  */

static tree
build_funcs_gates_fn (tree ident, vec<tree, va_gc> *functions,
		      vec<tree, va_gc> *gates)
{
  tree expr_list = NULL_TREE;

  /* Increment gates first.  */
  for (size_t i = 0; i < vec_safe_length (gates); i++)
    {
      tree decl = (*gates)[i];
      tree value = build2 (PLUS_EXPR, TREE_TYPE (decl),
			   decl, integer_one_node);
      tree var_expr = modify_expr (decl, value);
      expr_list = compound_expr (expr_list, var_expr);
    }

  /* Call Functions.  */
  for (size_t i = 0; i < vec_safe_length (functions); i++)
    {
      tree decl = (*functions)[i];
      tree call_expr = d_build_call_list (void_type_node,
					  build_address (decl), NULL_TREE);
      expr_list = compound_expr (expr_list, call_expr);
    }

  if (expr_list)
    return build_internal_fn (ident, expr_list);

  return NULL_TREE;
}

/* Build and emit a function that takes a scope delegate parameter and
   calls it once for every TLS variable in the module.  */

static tree
build_emutls_function (vec<VarDeclaration *> tlsVars)
{
  Module *mod = current_module_decl;

  if (!mod)
    mod = Module::rootModule;

  const char *name = "__modtlsscan";

  /* void __modtlsscan(scope void delegate(void*, void*) nothrow dg) nothrow
     {
	dg(&$tlsVars[0]$, &$tlsVars[0]$ + $tlsVars[0]$.sizeof);
	dg(&$tlsVars[1]$, &$tlsVars[1]$ + $tlsVars[0]$.sizeof);
	...
     }
   */
  Parameters *del_args = new Parameters ();
  del_args->push (Parameter::create (0, Type::tvoidptr, NULL, NULL));
  del_args->push (Parameter::create (0, Type::tvoidptr, NULL, NULL));

  TypeFunction *del_func_type = TypeFunction::create (del_args, Type::tvoid, 0,
						      LINKd, STCnothrow);
  Parameters *args = new Parameters ();
  Parameter *dg_arg = Parameter::create (STCscope,
					 new TypeDelegate (del_func_type),
					 Identifier::idPool ("dg"), NULL);
  args->push (dg_arg);
  TypeFunction *func_type = TypeFunction::create (args, Type::tvoid, 0, LINKd,
						  STCnothrow);
  FuncDeclaration *func = new FuncDeclaration (mod->loc, mod->loc,
					       Identifier::idPool (name),
					       STCstatic, func_type);
  func->loc = Loc (mod->srcfile->toChars (), 1, 0);
  func->linkage = func_type->linkage;
  func->parent = mod;
  func->protection = PROTprivate;

  func->semantic (mod->_scope);
  Statements *body = new Statements ();
  for (size_t i = 0; i < tlsVars.length (); i++)
    {
      VarDeclaration *var = tlsVars[i];
      Expression *addr = (VarExp::create (mod->loc, var))->addressOf ();
      Expression *addr2 = new SymOffExp (mod->loc, var, var->type->size ());
      Expressions* addrs = new Expressions ();
      addrs->push (addr);
      addrs->push (addr2);

      Expression *call = CallExp::create (mod->loc,
					  IdentifierExp::create (Loc (),
								 dg_arg->ident),
					  addrs);
      body->push (ExpStatement::create (mod->loc, call));
    }
  func->fbody = new CompoundStatement (mod->loc, body);
  func->semantic3 (mod->_scope);
  build_decl_tree (func);

  return get_symbol_decl (func);
}

/* Convenience function for layout_moduleinfo_fields.  Adds a field of TYPE to
   the moduleinfo record at OFFSET, incrementing the offset to the next field
   position.  No alignment is taken into account, all fields are packed.  */

static void
layout_moduleinfo_field (tree type, tree rec_type, HOST_WIDE_INT& offset)
{
  tree field = create_field_decl (type, NULL, 1, 1);
  insert_aggregate_field (rec_type, field, offset);
  offset += int_size_in_bytes (type);
}

/* Layout fields that immediately come after the moduleinfo TYPE for DECL.
   Data relating to the module is packed into the type on an as-needed
   basis, this is done to keep its size to a minimum.  */

static tree
layout_moduleinfo_fields (Module *decl, tree type)
{
  HOST_WIDE_INT offset = int_size_in_bytes (type);
  type = copy_aggregate_type (type);

  /* First fields added are all the function pointers.  */
  if (decl->sctor)
    layout_moduleinfo_field (ptr_type_node, type, offset);

  if (decl->sdtor)
    layout_moduleinfo_field (ptr_type_node, type, offset);

  if (decl->ssharedctor)
    layout_moduleinfo_field (ptr_type_node, type, offset);

  if (decl->sshareddtor)
    layout_moduleinfo_field (ptr_type_node, type, offset);

  if (decl->findGetMembers ())
    layout_moduleinfo_field (ptr_type_node, type, offset);

  if (decl->sictor)
    layout_moduleinfo_field (ptr_type_node, type, offset);

  if (decl->stest)
    layout_moduleinfo_field (ptr_type_node, type, offset);

  /* Array of module imports is layed out as a length field, followed by
     a static array of ModuleInfo pointers.  */
  size_t aimports_dim = decl->aimports.dim;
  for (size_t i = 0; i < decl->aimports.dim; i++)
    {
      Module *mi = decl->aimports[i];
      if (!mi->needmoduleinfo)
	aimports_dim--;
    }

  if (aimports_dim)
    {
      layout_moduleinfo_field (size_type_node, type, offset);
      layout_moduleinfo_field (make_array_type (Type::tvoidptr, aimports_dim),
			       type, offset);
    }

  /* Array of local ClassInfo decls are layed out in the same way.  */
  ClassDeclarations aclasses;
  for (size_t i = 0; i < decl->members->dim; i++)
    {
      Dsymbol *member = (*decl->members)[i];
      member->addLocalClass (&aclasses);
    }

  if (aclasses.dim)
    {
      layout_moduleinfo_field (size_type_node, type, offset);
      layout_moduleinfo_field (make_array_type (Type::tvoidptr, aclasses.dim),
			       type, offset);
    }

  /* Lastly, the name of the module is a static char array.  */
  size_t namelen = strlen (decl->toPrettyChars ()) + 1;
  layout_moduleinfo_field (make_array_type (Type::tchar, namelen),
			   type, offset);

  finish_aggregate_type (offset, 1, type, NULL);

  return type;
}

/* Return the type for ModuleInfo, create it if it doesn't already exist.  */

static tree
get_moduleinfo_type_node (void)
{
  if (moduleinfo_type_node)
    return moduleinfo_type_node;

  /* Create the ModuleInfo type, the first two fields are the only ones
     named in ModuleInfo.  */
  tree fields = create_field_decl (uint_type_node, NULL, 1, 1);
  DECL_CHAIN (fields) = create_field_decl (uint_type_node, NULL, 1, 1);

  /* An EmuTLS scan function is added for targets to support GC scanning
     of TLS storage, but don't support it natively.  */
  if (!targetm.have_tls)
    {
      tree field = create_field_decl (ptr_type_node, NULL, 1, 1);
      DECL_CHAIN (field) = fields;
      fields = field;
    }

  moduleinfo_type_node = make_node (RECORD_TYPE);
  finish_builtin_struct (moduleinfo_type_node, Id::ModuleInfo->toChars (),
			 fields, NULL_TREE);

  return moduleinfo_type_node;
}

/* Get the VAR_DECL of the ModuleInfo for DECL.  If this does not yet exist,
   create it.  The ModuleInfo decl is used to keep track of constructors,
   destructors, unittests, members, classes, and imports for the given module.
   This is used by the D runtime for module initialization and termination.  */

static tree
get_moduleinfo_decl (Module *decl)
{
  if (decl->csym)
    return decl->csym;

  tree ident = make_internal_name (decl, "__ModuleInfo", "Z");
  tree type = get_moduleinfo_type_node ();

  decl->csym = declare_extern_var (ident, type);
  DECL_LANG_SPECIFIC (decl->csym) = build_lang_decl (NULL);

  DECL_CONTEXT (decl->csym) = build_import_decl (decl);
  /* Not readonly, moduleinit depends on this.  */
  TREE_READONLY (decl->csym) = 0;

  return decl->csym;
}

/* Build the ModuleInfo symbol for Module M.  */

static tree
build_moduleinfo (Module *m)
{
  ClassDeclarations aclasses;
  FuncDeclaration *sgetmembers;

  for (size_t i = 0; i < m->members->dim; i++)
    {
      Dsymbol *member = (*m->members)[i];
      member->addLocalClass (&aclasses);
    }

  size_t aimports_dim = m->aimports.dim;
  for (size_t i = 0; i < m->aimports.dim; i++)
    {
      Module *mi = m->aimports[i];
      if (!mi->needmoduleinfo)
	aimports_dim--;
    }

  sgetmembers = m->findGetMembers ();

  size_t flags = 0;
  if (m->sctor)
    flags |= MItlsctor;
  if (m->sdtor)
    flags |= MItlsdtor;
  if (m->ssharedctor)
    flags |= MIctor;
  if (m->sshareddtor)
    flags |= MIdtor;
  if (sgetmembers)
    flags |= MIxgetMembers;
  if (m->sictor)
    flags |= MIictor;
  if (m->stest)
    flags |= MIunitTest;
  if (aimports_dim)
    flags |= MIimportedModules;
  if (aclasses.dim)
    flags |= MIlocalClasses;
  if (!m->needmoduleinfo)
    flags |= MIstandalone;

  flags |= MIname;

  tree decl = get_moduleinfo_decl (m);
  tree type = layout_moduleinfo_fields (m, TREE_TYPE (decl));
  tree field = TYPE_FIELDS (type);

  /* Put out the two named fields in a ModuleInfo decl:
	uint flags;
	uint index;  */
  vec<constructor_elt, va_gc> *minit = NULL;

  CONSTRUCTOR_APPEND_ELT (minit, field,
			  build_integer_cst (flags, TREE_TYPE (field)));
  field = TREE_CHAIN (field);

  CONSTRUCTOR_APPEND_ELT (minit, field,
			  build_integer_cst (0, TREE_TYPE (field)));
  field = TREE_CHAIN (field);

  /* version (GNU_EMUTLS):
	void function(delegate(void*, void*)) scanTLS;  */
  if (!targetm.have_tls)
    {
      if (current_moduleinfo->tlsVars.is_empty ())
	CONSTRUCTOR_APPEND_ELT (minit, field, null_pointer_node);
      else
	{
	  tree emutls = build_emutls_function (current_moduleinfo->tlsVars);
	  CONSTRUCTOR_APPEND_ELT (minit, field, build_address (emutls));
	}

      field = TREE_CHAIN (field);
    }

  /* Order of appearance, depending on flags:
	void function() tlsctor;
	void function() tlsdtor;
	void* function() xgetMembers;
	void function() ctor;
	void function() dtor;
	void function() ictor;
	void function() unitTest;
	ModuleInfo*[] importedModules;
	TypeInfo_Class[] localClasses;
	char[N] name;
   */
  if (flags & MItlsctor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sctor));
      field = TREE_CHAIN (field);
    }

  if (flags & MItlsdtor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sdtor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIctor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->ssharedctor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIdtor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sshareddtor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIxgetMembers)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field,
			      build_address (get_symbol_decl (sgetmembers)));
      field = TREE_CHAIN (field);
    }

  if (flags & MIictor)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->sictor));
      field = TREE_CHAIN (field);
    }

  if (flags & MIunitTest)
    {
      CONSTRUCTOR_APPEND_ELT (minit, field, build_address (m->stest));
      field = TREE_CHAIN (field);
    }

  if (flags & MIimportedModules)
    {
      vec<constructor_elt, va_gc> *elms = NULL;
      tree satype = make_array_type (Type::tvoidptr, aimports_dim);
      size_t idx = 0;

      for (size_t i = 0; i < m->aimports.dim; i++)
	{
	  Module *mi = m->aimports[i];
	  if (mi->needmoduleinfo)
	    {
	      CONSTRUCTOR_APPEND_ELT (elms, size_int (idx),
				      build_address (get_moduleinfo_decl (mi)));
	      idx++;
	    }
	}

      CONSTRUCTOR_APPEND_ELT (minit, field, size_int (aimports_dim));
      field = TREE_CHAIN (field);
      CONSTRUCTOR_APPEND_ELT (minit, field, build_constructor (satype, elms));
      field = TREE_CHAIN (field);
    }

  if (flags & MIlocalClasses)
    {
      vec<constructor_elt, va_gc> *elms = NULL;
      tree satype = make_array_type (Type::tvoidptr, aclasses.dim);

      for (size_t i = 0; i < aclasses.dim; i++)
	{
	  ClassDeclaration *cd = aclasses[i];
	  CONSTRUCTOR_APPEND_ELT (elms, size_int (i),
				  build_address (get_classinfo_decl (cd)));
	}

      CONSTRUCTOR_APPEND_ELT (minit, field, size_int (aclasses.dim));
      field = TREE_CHAIN (field);
      CONSTRUCTOR_APPEND_ELT (minit, field, build_constructor (satype, elms));
      field = TREE_CHAIN (field);
    }

  if (flags & MIname)
    {
      /* Put out module name as a 0-terminated C-string, to save bytes.  */
      const char *name = m->toPrettyChars ();
      size_t namelen = strlen (name) + 1;
      tree strtree = build_string (namelen, name);
      TREE_TYPE (strtree) = make_array_type (Type::tchar, namelen);
      CONSTRUCTOR_APPEND_ELT (minit, field, strtree);
      field = TREE_CHAIN (field);
    }

  gcc_assert (field == NULL_TREE);

  TREE_TYPE (decl) = type;
  DECL_INITIAL (decl) = build_struct_literal (type, minit);
  d_finish_decl (decl);

  return decl;
}

/* Build a variable used in the dso_registry code. The variable is always
   visibility = hidden and TREE_PUBLIC. Optionally sets an initializer, makes
   the variable external and/or comdat.  */

static tree
build_dso_registry_var (const char * name, tree type, tree init, bool comdat_p,
			bool external_p)
{
  tree var = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			 get_identifier (name), type);
  DECL_VISIBILITY (var) = VISIBILITY_HIDDEN;
  DECL_VISIBILITY_SPECIFIED (var) = 1;
  TREE_PUBLIC (var) = 1;
  DECL_ARTIFICIAL (var) = 1;

  if (external_p)
    DECL_EXTERNAL (var) = 1;
  else
    TREE_STATIC (var) = 1;

  if (init != NULL_TREE)
    DECL_INITIAL (var) = init;
  if (comdat_p)
    d_comdat_linkage (var);

  rest_of_decl_compilation (var, 1, 0);
  return var;
}

/* Build and emit a constructor or destructor calling dso_registry_func if
   dso_initialized is (false in a constructor or true in a destructor).  */

static void
emit_dso_registry_cdtor (Dsymbol *compiler_dso_type, Dsymbol *dso_registry_func,
  tree dso_initialized, tree dso_slot, bool ctor_p)
{
  tree ident = get_identifier (ctor_p ? "gdc_dso_ctor" : "gdc_dso_dtor");
  tree init_condition = (ctor_p) ? boolean_true_node : boolean_false_node;

  /* extern extern(C) void* __start_minfo @hidden;
     extern extern(C) void* __stop_minfo @hidden;  */
  tree start_minfo = build_dso_registry_var ("__start_minfo", ptr_type_node,
					     NULL_TREE, false, true);
  tree stop_minfo = build_dso_registry_var ("__stop_minfo", ptr_type_node,
					    NULL_TREE, false, true);

  /* extern(C) void gdc_dso_[c/d]tor() @hidden @weak @[con/de]structor.  */
  FuncDeclaration *fd = get_internal_fn (ident);
  tree decl = get_symbol_decl (fd);

  TREE_PUBLIC (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_VISIBILITY (decl) = VISIBILITY_HIDDEN;
  DECL_VISIBILITY_SPECIFIED (decl) = 1;

  if (ctor_p)
    vec_safe_push (static_ctor_list, decl);
  else
    vec_safe_push (static_dtor_list, decl);

  d_comdat_linkage (decl);

  tree old_context = start_function (fd);
  rest_of_decl_compilation (decl, 1, 0);

  /* dso = {1, &dsoSlot, &__start_minfo, &__stop_minfo};  */
  tree dso_type = build_ctype (compiler_dso_type->isStructDeclaration ()->type);
  vec<constructor_elt, va_gc> *ve = NULL;

  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field (dso_type,
						    get_identifier ("_version")),
			  build_int_cst (size_type_node, 1));
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field (dso_type,
						    get_identifier ("_slot")),
			  build_address (dso_slot));
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field (dso_type,
						    get_identifier ("_minfo_beg")),
			  build_address (start_minfo));
  CONSTRUCTOR_APPEND_ELT (ve, find_aggregate_field (dso_type,
						    get_identifier ("_minfo_end")),
			  build_address (stop_minfo));

  tree dso_data = build_local_temp (dso_type);

  tree fbody = modify_expr (dso_data, build_struct_literal (dso_type, ve));

  /* if (!(gdc_dso_initialized == %init_condition))
     {
	gdc_dso_initialized = %init_condition;
	CompilerDSOData dso = {1, &dsoSlot, &__start_minfo, &__stop_minfo};
	_d_dso_registry(&dso);
     }
   */
  tree condition = build_boolop (NE_EXPR, dso_initialized, init_condition);
  tree assign_expr = modify_expr (dso_initialized, init_condition);
  fbody = compound_expr (fbody, assign_expr);

  tree registry_func = get_symbol_decl (dso_registry_func->isFuncDeclaration ());
  tree call_expr = d_build_call_nary (registry_func, 1,
				      build_address (dso_data));
  fbody = compound_expr (fbody, call_expr);

  add_stmt (build_vcondition (condition, fbody, void_node));
  finish_function (old_context);
}

/* Build and emit the helper functions for the DSO registry code, including
   the gdc_dso_slot and gdc_dso_initialized variable and the gdc_dso_ctor
   and gdc_dso_dtor functions.  */

static void
emit_dso_registry_helpers (Dsymbol *compiler_dso_type,
			   Dsymbol *dso_registry_func)
{
  /* extern(C) void* gdc_dso_slot @hidden @weak = null;
     extern(C) bool gdc_dso_initialized @hidden @weak = false;  */
  tree dso_slot = build_dso_registry_var ("gdc_dso_slot", ptr_type_node,
    null_pointer_node, true, false);
  tree dso_initialized = build_dso_registry_var ("gdc_dso_initialized",
						 boolean_type_node,
						 boolean_false_node,
						 true, false);

  /* extern(C) void gdc_dso_ctor() @hidden @weak @ctor
     extern(C) void gdc_dso_dtor() @hidden @weak @dtor  */
  emit_dso_registry_cdtor (compiler_dso_type, dso_registry_func,
			   dso_initialized, dso_slot, true);
  emit_dso_registry_cdtor (compiler_dso_type, dso_registry_func,
			   dso_initialized, dso_slot, false);
}

/* Emit code to place sym into the minfo list. Also emit helpers to call
   _d_dso_registry if necessary.  */

static void
emit_dso_registry_hooks (tree sym, Dsymbol *compiler_dso_type,
			 Dsymbol *dso_registry_func)
{
  gcc_assert (targetm_common.have_named_sections);

  /* Step 1: Place the ModuleInfo into the minfo section.
     Do this once for every emitted Module
     @section("minfo") void* __mod_ref_%s = &ModuleInfo(module);  */
  const char *name = concat ("__mod_ref_",
			     IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (sym)),
			     NULL);
  tree decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			  get_identifier (name), ptr_type_node);
  d_keep (decl);
  TREE_PUBLIC (decl) = 1;
  TREE_STATIC (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_PRESERVE_P (decl) = 1;
  DECL_INITIAL (decl) = build_address (sym);
  TREE_STATIC (DECL_INITIAL (decl)) = 1;

  /* Do not start section with '.' to use the __start_ feature:
     https://sourceware.org/binutils/docs-2.26/ld/Orphan-Sections.html  */
  set_decl_section_name (decl, "minfo");
  rest_of_decl_compilation (decl, 1, 0);

  /* Step 2: Emit helper functions. These are only required once per
     shared library, so it's safe to emit them only one per object file.  */
  static bool emitted = false;
  if (!emitted)
    {
      emitted = true;
      emit_dso_registry_helpers (compiler_dso_type, dso_registry_func);
    }
}

/* Emit code to add sym to the _Dmodule_ref linked list.  */

static void
emit_modref_hooks (tree sym, Dsymbol *mref)
{
  /* Generate:
      struct ModuleReference
      {
	void *next;
	void* mod;
      }
    */

  /* struct ModuleReference in moduleinit.d.  */
  tree tmodref = make_two_field_type (ptr_type_node, ptr_type_node,
				      NULL, "next", "mod");
  tree nextfield = TYPE_FIELDS (tmodref);
  tree modfield = TREE_CHAIN (nextfield);

  /* extern (C) ModuleReference *_Dmodule_ref;  */
  tree dmodule_ref = get_symbol_decl (mref->isDeclaration ());

  /* private ModuleReference modref = { next: null, mod: _ModuleInfo_xxx };  */
  tree modref = build_artificial_decl (tmodref, NULL_TREE, "__mod_ref");
  d_keep (modref);

  vec<constructor_elt, va_gc> *ce = NULL;
  CONSTRUCTOR_APPEND_ELT (ce, nextfield, null_pointer_node);
  CONSTRUCTOR_APPEND_ELT (ce, modfield, build_address (sym));

  DECL_INITIAL (modref) = build_constructor (tmodref, ce);
  TREE_STATIC (DECL_INITIAL (modref)) = 1;
  d_pushdecl (modref);
  rest_of_decl_compilation (modref, 1, 0);

  /* Generate:
      void ___modinit()  // a static constructor
      {
	modref.next = _Dmodule_ref;
	_Dmodule_ref = &modref;
      }
   */
  tree m1 = modify_expr (component_ref (modref, nextfield), dmodule_ref);
  tree m2 = modify_expr (dmodule_ref, build_address (modref));

  tree decl = build_internal_fn (get_identifier ("*__modinit"),
				 compound_expr (m1, m2));
  vec_safe_push (static_ctor_list, decl);
}

/* Output the ModuleInfo for module M and register it with druntime.  */

static void
layout_moduleinfo (Module *m)
{
  /* Load the rt.sections module and retrieve the internal DSO/ModuleInfo
     types, ignoring any errors as a result of missing files.  */
  unsigned errors = global.startGagging ();
  Module *sections = Module::load (Loc (), NULL,
				   Identifier::idPool ("rt/sections"));
  global.endGagging (errors);

  if (sections == NULL)
    return;

  sections->importedFrom = sections;
  sections->importAll (NULL);
  sections->semantic (NULL);
  sections->semantic2 (NULL);
  sections->semantic3 (NULL);

  /* These symbols are normally private to the module they're declared in,
     but for internal compiler lookups, their visibility is discarded.  */
  int sflags = IgnoreErrors | IgnoreSymbolVisibility;

  /* Try to find the required types and functions in druntime.  */
  Dsymbol *mref = sections->search (Loc (),
				    Identifier::idPool ("_Dmodule_ref"),
				    sflags);
  Dsymbol *dso_type = sections->search (Loc (),
					Identifier::idPool ("CompilerDSOData"),
					sflags);
  Dsymbol *dso_func = sections->search (Loc (),
					Identifier::idPool ("_d_dso_registry"),
					sflags);

  tree sym = build_moduleinfo (m);

  /* Prefer _d_dso_registry model if available.  */
  if (dso_type != NULL && dso_func != NULL)
    emit_dso_registry_hooks (sym, dso_type, dso_func);
  else if (mref != NULL)
    emit_modref_hooks (sym, mref);
}

/* */

tree
d_module_context (void)
{
  if (cfun != NULL)
    return current_function_decl;

  gcc_assert (current_module_decl != NULL);
  return build_import_decl (current_module_decl);
}

/* */

void
build_module (Module *m)
{
  /* There may be more than one module per object file, but should only
     ever compile them one at a time.  */
  assert (!current_moduleinfo && !current_module_decl);

  module_info mi = module_info ();

  current_moduleinfo = &mi;
  current_module_decl = m;

  if (m->members)
    {
      for (size_t i = 0; i < m->members->dim; i++)
	{
	  Dsymbol *s = (*m->members)[i];
	  build_decl_tree (s);
	}
    }

  /* Default behaviour is to always generate module info because of templates.
     Can be switched off for not compiling against runtime library.  */
  if (!global.params.betterC && m->ident != Id::entrypoint)
    {
      if (mi.ctors || mi.ctorgates)
	m->sctor = build_funcs_gates_fn (get_identifier ("*__modctor"),
					 mi.ctors, mi.ctorgates);

      if (mi.dtors)
	m->sdtor = build_funcs_gates_fn (get_identifier ("*__moddtor"),
					 mi.dtors, NULL);

      if (mi.sharedctors || mi.sharedctorgates)
	m->ssharedctor
	  = build_funcs_gates_fn (get_identifier ("*__modsharedctor"),
				  mi.sharedctors, mi.sharedctorgates);

      if (mi.shareddtors)
	m->sshareddtor
	  = build_funcs_gates_fn (get_identifier ("*__modshareddtor"),
				  mi.shareddtors, NULL);

      if (mi.unitTests)
	m->stest = build_funcs_gates_fn (get_identifier ("*__modtest"),
					 mi.unitTests, NULL);

      layout_moduleinfo (m);
    }

  current_moduleinfo = NULL;
  current_module_decl = NULL;
}

/* */

void
register_module_decl (Declaration *d)
{
  VarDeclaration *vd = d->isVarDeclaration ();
  if (vd != NULL)
    {
      /* Register TLS variables against ModuleInfo.  */
      if (vd->isThreadlocal ())
	current_moduleinfo->tlsVars.safe_push (vd);
    }

  FuncDeclaration *fd = d->isFuncDeclaration ();
  if (fd != NULL)
    {
      tree decl = get_symbol_decl (fd);

      /* If a static constructor, push into the current ModuleInfo.
	 Checks for `shared' first because it derives from the non-shared
	 constructor type in the front-end.  */
      if (fd->isSharedStaticCtorDeclaration ())
	vec_safe_push (current_moduleinfo->sharedctors, decl);
      else if (fd->isStaticCtorDeclaration ())
	vec_safe_push (current_moduleinfo->ctors, decl);

      /* If a static destructor, do same as with constructors, but also
	 increment the destructor's vgate at construction time.  */
      if (fd->isSharedStaticDtorDeclaration ())
	{
	  VarDeclaration *vgate = ((SharedStaticDtorDeclaration *) fd)->vgate;
	  if (vgate != NULL)
	    {
	      tree gate = get_symbol_decl (vgate);
	      vec_safe_push (current_moduleinfo->sharedctorgates, gate);
	    }
	  vec_safe_insert (current_moduleinfo->shareddtors, 0, decl);
	}
      else if (fd->isStaticDtorDeclaration ())
	{
	  VarDeclaration *vgate = ((StaticDtorDeclaration *) fd)->vgate;
	  if (vgate != NULL)
	    {
	      tree gate = get_symbol_decl (vgate);
	      vec_safe_push (current_moduleinfo->ctorgates, gate);
	    }
	  vec_safe_insert (current_moduleinfo->dtors, 0, decl);
	}

      /* If a unittest function.  */
      if (fd->isUnitTestDeclaration ())
	vec_safe_push (current_moduleinfo->unitTests, decl);
    }
}

/* Wrapup all global declarations and start the final compilation.  */

void
d_finish_module (tree *vec, int len)
{
  /* Complete all generated thunks.  */
  symtab->process_same_body_aliases ();

  /* Process all file scopes in this compilation, and the external_scope,
     through wrapup_global_declarations.  */
  for (int i = 0; i < len; i++)
    {
      tree decl = vec[i];
      wrapup_global_declarations (&decl, 1);

      /* We want the static symbol to be written.  */
      if (TREE_PUBLIC (decl) && VAR_OR_FUNCTION_DECL_P (decl))
	{
	  if ((VAR_P (decl) && TREE_STATIC (decl)) || !DECL_ARTIFICIAL (decl))
	    mark_needed (decl);
	}
    }

  /* If the target does not directly support static constructors,
     static_ctor_list contains a list of all static constructors defined
     so far.  This routine will create a function to call all of those
     and is picked up by collect2. */
  if (static_ctor_list)
    {
      tree decl = build_funcs_gates_fn (get_file_function_name ("I"),
					static_ctor_list, NULL);
      DECL_STATIC_CONSTRUCTOR (decl) = 1;
      decl_init_priority_insert (decl, DEFAULT_INIT_PRIORITY);
    }

  if (static_dtor_list)
    {
      tree decl = build_funcs_gates_fn (get_file_function_name ("D"),
					static_dtor_list, NULL);
      DECL_STATIC_DESTRUCTOR (decl) = 1;
      decl_fini_priority_insert (decl, DEFAULT_INIT_PRIORITY);
    }
}
