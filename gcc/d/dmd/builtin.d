/* builtin.d -- Handlers for evaluating builtins during CTFE.
 * Copyright (C) 2018 Free Software Foundation, Inc.
 *
 * GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC; see the file COPYING3.  If not see
 * <http://www.gnu.org/licenses/>.
 */

module dmd.builtin;

import core.stdc.math;
import core.stdc.string;
import dmd.arraytypes;
import dmd.expression;
import dmd.func;
import dmd.globals;

/**********************************
 * Determine if function is a builtin one that we can
 * evaluate at compile time.
 */
public extern (C++) BUILTIN isBuiltin(FuncDeclaration fd);

/**************************************
 * Evaluate builtin function.
 * Return result; NULL if cannot evaluate it.
 */
public extern (C++) Expression eval_builtin(Loc loc, FuncDeclaration fd, Expressions* arguments);
