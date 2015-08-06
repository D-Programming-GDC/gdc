// d-irstate.cc -- D frontend for GCC.
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

#include "dfrontend/init.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/declaration.h"
#include "dfrontend/statement.h"

#include "tree.h"
#include "tree-iterator.h"
#include "fold-const.h"
#include "diagnostic.h"
#include "tm.h"

#include "d-tree.h"
#include "d-objfile.h"
#include "d-irstate.h"
#include "d-codegen.h"

Flow *
IRState::beginFlow (Statement *stmt)
{
  Flow *flow = new Flow (stmt);
  this->loops_.safe_push (flow);
  return flow;
}

void
IRState::endFlow()
{
  gcc_assert (!this->loops_.is_empty());

  Flow *flow = this->loops_.pop();

  if (flow->exitLabel)
    this->doLabel (flow->exitLabel);
}

void
IRState::doLabel (tree label)
{
  /* Don't write out label unless it is marked as used by the frontend.
     This makes auto-vectorization possible in conditional loops.
     The only excemption to this is in LabelStatement::toIR, in which
     all computed labels are marked regardless.  */
  if (TREE_USED (label))
    add_stmt(fold_build1 (LABEL_EXPR, void_type_node, label));
}

