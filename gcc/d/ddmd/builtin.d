/**
 * Compiler implementation of the
 * $(LINK2 http://www.dlang.org, D programming language).
 *
 * Copyright:   Copyright (c) 1999-2017 by Digital Mars, All Rights Reserved
 * Authors:     $(LINK2 http://www.digitalmars.com, Walter Bright)
 * License:     $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:      $(LINK2 https://github.com/dlang/dmd/blob/master/src/ddmd/builtin.d, _builtin.d)
 */

module ddmd.builtin;

// Online documentation: https://dlang.org/phobos/ddmd_builtin.html

import core.stdc.math;
import core.stdc.string;
import ddmd.arraytypes;
import ddmd.dmangle;
import ddmd.errors;
import ddmd.expression;
import ddmd.func;
import ddmd.globals;
import ddmd.mtype;
import ddmd.root.ctfloat;
import ddmd.root.stringtable;
import ddmd.tokens;

/**********************************
 * Determine if function is a builtin one that we can
 * evaluate at compile time.
 */
extern (C++) BUILTIN isBuiltin(FuncDeclaration fd);

/**************************************
 * Evaluate builtin function.
 * Return result; NULL if cannot evaluate it.
 */
extern (C++) Expression eval_builtin(Loc loc, FuncDeclaration fd, Expressions* arguments);
