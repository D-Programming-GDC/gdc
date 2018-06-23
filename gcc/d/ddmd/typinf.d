/**
 * Compiler implementation of the
 * $(LINK2 http://www.dlang.org, D programming language).
 *
 * Copyright:   Copyright (c) 1999-2017 by Digital Mars, All Rights Reserved
 * Authors:     $(LINK2 http://www.digitalmars.com, Walter Bright)
 * License:     $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:      $(LINK2 https://github.com/dlang/dmd/blob/master/src/ddmd/typeinf.d, _typeinf.d)
 */

module ddmd.typinf;

// Online documentation: https://dlang.org/phobos/ddmd_typinf.html

import ddmd.declaration;
import ddmd.dscope;
import ddmd.mtype;

/****************************************************
 * Get the exact TypeInfo.
 */
extern (C++) Type getTypeInfoType(Type t, Scope* sc);
extern (C++) TypeInfoDeclaration getTypeInfoDeclaration(Type t);
extern (C++) bool isSpeculativeType(Type t);
extern (C++) static bool builtinTypeInfo(Type t);
