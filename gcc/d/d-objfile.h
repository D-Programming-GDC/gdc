// d-objfile.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_OBFILE_H
#define GCC_DCMPLR_OBFILE_H

#include "config.h"
#include "coretypes.h"

#include "root.h"
#include "mtype.h"

// These must match the values in object_.d
enum ModuleInfoFlags
{
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

struct FuncFrameInfo;
struct Thunk;
typedef tree_node dt_t;

struct Symbol
{
  Symbol();

  const char *Sident;
  const char *prettyIdent;
  int Salignment;
  bool Sreadonly;

  dt_t *Sdt;

  // Our GNU backend tree for the symbol.
  tree Stree;

  // FIELD_DECL in frame struct that this variable is allocated in.
  tree SframeField;

  // RESULT_DECL in a function that returns by nrvo.
  tree SnamedResult;

  // For FuncDeclarations:
  auto_vec<Thunk *> thunks;
  FuncFrameInfo *frameInfo;
};

struct Thunk
{
  Thunk()
  { offset = 0; symbol = NULL; }

  int offset;
  Symbol *symbol;
};

extern dt_t **dt_cons (dt_t **pdt, tree val);
extern dt_t **dt_chainon (dt_t **pdt, dt_t *val);
extern dt_t **dt_container (dt_t **pdt, Type *type, dt_t *dt);
extern dt_t **build_vptr_monitor (dt_t **pdt, ClassDeclaration *cd);

extern tree dtvector_to_tree (dt_t *dt);

extern void build_moduleinfo (Symbol *sym);


struct ModuleInfo
{
  auto_vec<ClassDeclaration *> classes;
  auto_vec<FuncDeclaration *> ctors;
  auto_vec<FuncDeclaration *> dtors;
  auto_vec<VarDeclaration *> ctorgates;

  auto_vec<FuncDeclaration *> sharedctors;
  auto_vec<FuncDeclaration *> shareddtors;
  auto_vec<VarDeclaration *> sharedctorgates;

  auto_vec<FuncDeclaration *> unitTests;
};

extern ModuleInfo *current_module_info;

enum TemplateEmission
{
  TEnone,
  TEnormal,
  TEallinst,
};

extern location_t get_linemap (const Loc loc);
extern void set_input_location (const Loc& loc);
extern void set_input_location (Dsymbol *decl);

extern void set_decl_location (tree t, const Loc& loc);
extern void set_decl_location (tree t, Dsymbol *decl);
extern void set_function_end_locus (const Loc& loc);

extern void get_unique_name (tree decl, const char *prefix = NULL);

extern void setup_symbol_storage (Dsymbol *dsym, tree decl, bool is_public);
extern void d_comdat_linkage (tree decl);

extern void d_finish_symbol (Symbol *sym);
extern void d_finish_function (FuncDeclaration *f);
extern void d_finish_module();
extern void d_finish_compilation (tree *vec, int len);

extern void build_type_decl (tree t, Dsymbol *dsym);

extern Modules output_modules;
extern bool output_module_p (Module *mod);

extern void write_deferred_thunks();
extern void use_thunk (tree thunk_decl, tree target_decl, int offset);
extern void finish_thunk (tree thunk_decl, tree target_decl, int offset);

#endif

