/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DMD_SYMBOL_H
#define DMD_SYMBOL_H

#include "root.h"

#include "d-gcc-tree.h"

#include "mtype.h"

enum mangle_t
{
    mTY_INVALID,
    mTYman_c,
    mTYman_cpp,
    mTYman_std,
    mTYman_pas,
    mTYman_d
};

enum TypeType
{
    // no clue about which can be used together..
    TYnptr =    0x001,
    TYnfunc =   0x010,
    TYjfunc =   0x020,
    TYstruct =  0x040,
    TYarray  =  0x080,
    TYbit    =  0x100,
    TYint    =  0x200,
    mTYconst    = 0x1000,
    mTYvolatile = 0x2000
};

enum TypeFlag
{
    TFsizeunknown = 0x01,
    TFforward = 0x02,
    TFprototype = 0x04,
    TFfixed = 0x08
};

enum SymbolStorageClass
{
    SC_INVALID,
    SCextern,
    SCstatic,
    SCauto,
    SCglobal,
    SCstruct,
    SCcomdat
};

typedef enum SymbolStorageClass enum_SC;

enum SymbolFL
{
    FL_INVALID,
    FLextern = 0x01,
    FLauto   = 0x02,
    FLdata   = 0x04,
};

enum SymbolFlag
{
    SFLimplem  = 0x01,
    SFLnodebug = 0x02,
    STRglobal  = 0x04,
    SFLweak    = 0x08
};

enum SymbolSegment
{
    INVALID,
    DATA,
    CDATA,
    UDATA
};

// not sure if this needs to inherit object..
// union tree_node; typedef union tree_node dt_t;
struct dt_t;

enum OutputStage
{
    NotStarted,
    InProgress,
    Finished
};

struct FuncFrameInfo;
typedef ArrayBase<struct Thunk> Thunks;

struct Symbol : Object
{
    Symbol();

    const char *Sident;
    const char *prettyIdent;
    //unused in GCC//TYPE *Stype; // maybe type/TYPE ?
    SymbolStorageClass Sclass;
    SymbolFL           Sfl;
    SymbolSegment      Sseg;
    int                Sflags;

    dt_t * Sdt;

    // Specific to GNU backend
    tree     Stree;
    tree     ScontextDecl; // The DECL_CONTEXT to use for child declarations, but see IRState::declContext
    tree     SframeField;  // FIELD_DECL in frame struct that this variable is allocated in -- Eventually move back into Stree once everything works right

    // For FuncDeclarations:
    Thunks * thunks;
    FuncDeclarations * otherNestedFuncs;
    OutputStage outputStage;
    FuncFrameInfo *frameInfo;
};

struct Thunk
{
    int offset;
    Symbol * symbol;
    Thunk();
};

extern Symbol * symbol_calloc(const char * string);
extern Symbol * symbol_name(const char * id, int sclass, TYPE * t);
extern Symbol * struct_calloc();
extern Symbol * symbol_generate(SymbolStorageClass sc, TYPE * type);
extern void     symbol_func(Symbol * sym);
extern tree     check_static_sym(Symbol * sym);
extern void     outdata(Symbol * sym);
inline void     obj_export(Symbol *, int) { }
extern void     obj_moduleinfo(Symbol *sym);
extern void     obj_tlssections();

extern Symbol * symbol_tree(tree);
extern Symbol * static_sym();

extern void     slist_add(Symbol *);
extern void     slist_reset();
#endif
