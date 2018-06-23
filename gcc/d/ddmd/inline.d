/**
 * Compiler implementation of the
 * $(LINK2 http://www.dlang.org, D programming language).
 *
 * Copyright:   Copyright (c) 1999-2017 by Digital Mars, All Rights Reserved
 * Authors:     $(LINK2 http://www.digitalmars.com, Walter Bright)
 * License:     $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:    $(LINK2 https://github.com/dlang/dmd/blob/master/src/ddmd/inline.d, _inline.d)
 */

module ddmd.inline;

// Online documentation: https://dlang.org/phobos/ddmd_inline.html

import ddmd.dscope;
import ddmd.expression;

/***********************************************************
 * Perform the "inline copying" of a default argument for a function parameter.
 */

public Expression inlineCopy (Expression e, Scope* sc)
{
    return e.copy();
}
