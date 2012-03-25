/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Iain Buclaw, (C) 2010, 2011

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

#include "symbol.h"
#include "rmem.h"

Symbol::Symbol()
{
    Sident = 0;
    prettyIdent = 0;
    //Stype = 0;
    Sclass = SC_INVALID;
    Sfl = FL_INVALID;
    Sseg = INVALID;
    Sflags = 0;

    //Ssymnum = 0;
    Sdt = 0;

    //Sstruct = 0;
    //Sstructalign = 0;

    Stree = 0; // %% make it NULL-TREE, include d-gcc-include
    ScontextDecl = 0;
    SframeField = 0;

    thunks = NULL;
    otherNestedFuncs = NULL;
    outputStage = NotStarted;
    frameInfo = NULL;
}

Symbol *
symbol_calloc(const char * string)
{
    // Need to dup the string because sometimes the string is alloca()'d
    Symbol * s = new Symbol;
    s->Sident = mem.strdup(string);
    return s;
}

Symbol *
symbol_name(const char * id, int , TYPE *)
{
    // %% Nothing special, just do the same as symbol_calloc
    // we don't even bother using sclass and t
    return symbol_calloc(id);
}

Symbol *
struct_calloc()
{
    return new Symbol;
}

Symbol *
symbol_generate(SymbolStorageClass, TYPE *)
{
    return 0;
}

Thunk::Thunk()
{
    offset = 0;
    symbol = 0;
}

void
symbol_func(Symbol *)
{
}

Symbol *
symbol_tree(tree t)
{
    Symbol * s = new Symbol;
    s->Stree = t;
    return s;
}

void slist_add(Symbol *)
{
}

void slist_reset()
{
}

