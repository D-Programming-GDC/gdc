// d-objfile.h -- D frontend for GCC.
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

#ifndef GCC_DCMPLR_OBFILE_H
#define GCC_DCMPLR_OBFILE_H

#include "config.h"
#include "coretypes.h"

#include "root.h"
#include "mtype.h"

// These must match the values in object_.d
enum ModuleInfoFlags
{
  MIstandalone	    = 4,
  MItlsctor	    = 8,
  MItlsdtor	    = 0x10,
  MIctor	    = 0x20,
  MIdtor	    = 0x40,
  MIxgetMembers	    = 0x80,
  MIictor	    = 0x100,
  MIunitTest	    = 0x200,
  MIimportedModules = 0x400,
  MIlocalClasses    = 0x800,
  MInew		    = 0x80000000
};

enum OutputStage
{
  NotStarted,
  InProgress,
  Finished
};

struct FuncFrameInfo;
typedef ArrayBase<struct Thunk> Thunks;
typedef union tree_node dt_t;

struct Symbol : Object
{
  Symbol (void);

  const char *Sident;
  const char *prettyIdent;
  int Salignment;
  bool Sreadonly;

  dt_t *Sdt;

  // Our GNU backend tree for the symbol.
  tree Stree;

  // The DECL_CONTEXT to use for child declarations.
  tree ScontextDecl;

  // FIELD_DECL in frame struct that this variable is allocated in.
  tree SframeField;

  // For FuncDeclarations:
  Thunks thunks;
  OutputStage outputStage;
  FuncFrameInfo *frameInfo;
};

struct Thunk
{
  Thunk (void)
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
extern void build_tlssections (void);
extern void d_finish_symbol (Symbol *sym);


struct ModuleInfo
{
  ClassDeclarations classes;
  FuncDeclarations ctors;
  FuncDeclarations dtors;
  VarDeclarations ctorgates;

  FuncDeclarations sharedctors;
  FuncDeclarations shareddtors;
  VarDeclarations sharedctorgates;

  FuncDeclarations unitTests;
};

enum TemplateEmission
{
  TEnone,
  TEnormal,
  TEprivate
};

struct DeferredThunk
{
  tree decl;
  tree target;
  int offset;
};

typedef ArrayBase<DeferredThunk> DeferredThunks;


/* Nearly everything is static for effeciency since there is
   only one object per run of the backend */
struct ObjectFile
{
 public:
  ObjectFile (void) { };

  static void beginModule (Module *m);
  static void endModule (void);

  static void finish (void);

  static void doLineNote (const Loc& loc);
  static void setLoc (const Loc& loc);

  // ** Declaration maninpulation
  static void setDeclLoc (tree t, const Loc& loc);

  // Some D Declarations don't have the loc set, this searches decl's parents
  // until a valid loc is found.
  static void setDeclLoc (tree t, Dsymbol *decl);
  static void setCfunEndLoc (const Loc& loc);
  static void giveDeclUniqueName (tree decl, const char *prefix = NULL);

  // Set a DECL's STATIC and EXTERN based on the decl's storage class
  // and if it is to be emitted in this module.
  static void setupSymbolStorage (Dsymbol *decl, tree decl_tree, bool force_static_public = false);

  // Definitely in static data, but not neccessarily this module.
  // Assumed to be public data.
  static void setupStaticStorage (Dsymbol *dsym, tree decl_tree);
  static void makeDeclOneOnly (tree decl_tree);

  static void outputStaticSymbol (Symbol *s);
  static void outputFunction (FuncDeclaration *f);

  static void addAggMethod (tree rec_type, FuncDeclaration *fd);

  static void initTypeDecl (tree t, Dsymbol *d_sym);

  static void declareType (tree t, Type *d_type);
  static void declareType (tree t, Dsymbol *d_sym);

  // Hack for systems without linkonce support
  static bool shouldEmit (Declaration *d_sym);
  static bool shouldEmit (Symbol *sym);

  static void doThunk (tree thunk_decl, tree target_decl, int offset);

  // Can't use VAR_DECLs for the DECL_INITIAL of static varibles or in CONSTRUCTORSs
  static tree stripVarDecl (tree value);

  static FuncDeclaration *doSimpleFunction (const char *name, tree expr, bool static_ctor);
  static FuncDeclaration *doFunctionToCallFunctions (const char *name, FuncDeclarations *functions, bool force_and_public = false);
  static FuncDeclaration *doCtorFunction (const char *name, FuncDeclarations *functions, VarDeclarations *gates);
  static FuncDeclaration *doDtorFunction (const char *name, FuncDeclarations *functions);
  static FuncDeclaration *doUnittestFunction (const char *name, FuncDeclarations *functions);
  
  // ** Module info.  Assuming only one module per run of the compiler.

  static ModuleInfo *moduleInfo; // of ModuleInfo *

  // ** static constructors (not D static constructors)
  static FuncDeclarations staticCtorList; // usually only one.
  static FuncDeclarations staticDtorList; // only if __attribute__(destructor) is used.

  /* support for multiple modules per object file */
  static bool hasModule (Module *m);
  static Modules modules;

  // Template emission behaviour.
  static TemplateEmission emitTemplates;

 protected:
  static void outputThunk (tree thunk_decl, tree target_decl, int offset);

  static void initTypeDecl (tree t, tree decl);
  static void declareType (tree decl);

  // Can't output thunks while a function is being compiled.
  static DeferredThunks deferredThunks;

 private:
  static unsigned moduleSearchIndex;

};

#endif

