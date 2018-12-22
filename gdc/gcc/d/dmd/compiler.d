/* compiler.d -- Compiler interface for the D front end.
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

module dmd.compiler;

import dmd.dmodule;
import dmd.dscope;
import dmd.expression;
import dmd.mtype;

/**
 * A data structure that describes a back-end compiler and implements
 * compiler-specific actions.
 */
struct Compiler
{
    /**
     * Generate C main() in response to seeing D main().
     *
     * This function will generate a module called `__entrypoint`,
     * and set the globals `entrypoint` and `rootHasMain`.
     *
     * This used to be in druntime, but contained a reference to _Dmain
     * which didn't work when druntime was made into a dll and was linked
     * to a program, such as a C++ program, that didn't have a _Dmain.
     *
     * Params:
     *   sc = Scope which triggered the generation of the C main,
     *        used to get the module where the D main is.
     */
    extern (C++) static void genCmain(Scope* sc);

    /******************************
     * Encode the given expression, which is assumed to be an rvalue literal
     * as another type for use in CTFE.
     * This corresponds roughly to the idiom *(Type *)&e.
     */
    extern (C++) static Expression paintAsType(Expression e, Type type);

    /******************************
     * For the given module, perform any post parsing analysis.
     * Certain compiler backends (ie: GDC) have special placeholder
     * modules whose source are empty, but code gets injected
     * immediately after loading.
     */
    extern (C++) static void loadModule(Module m);

    /**
     * A callback function that is called once an imported module is
     * parsed. If the callback returns true, then it tells the
     * frontend that the driver intends on compiling the import.
     */
    extern(C++) static bool onImport(Module m);
}
