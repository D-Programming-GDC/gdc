/* target.d -- Target interface for the D front end.
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

module dmd.target;

import dmd.argtypes;
import dmd.dclass;
import dmd.dsymbol;
import dmd.expression;
import dmd.globals;
import dmd.mtype;
import dmd.tokens : TOK;
import dmd.root.ctfloat;
import dmd.root.outbuffer;

/**
 * Describes a back-end target. At present it is incomplete, but in the future
 * it should grow to contain most or all target machine and target O/S specific
 * information.
 *
 * In many cases, calls to sizeof() can't be used directly for getting data type
 * sizes since cross compiling is supported and would end up using the host
 * sizes rather than the target sizes.
 */
struct Target
{
    extern (C++) __gshared
    {
        // D ABI
        uint ptrsize;             /// size of a pointer in bytes
        uint realsize;            /// size a real consumes in memory
        uint realpad;             /// padding added to the CPU real size to bring it up to realsize
        uint realalignsize;       /// alignment for reals
        uint classinfosize;       /// size of `ClassInfo`
        ulong maxStaticDataSize;  /// maximum size of static data

        // C ABI
        uint c_longsize;          /// size of a C `long` or `unsigned long` type
        uint c_long_doublesize;   /// size of a C `long double`

        // C++ ABI
        bool reverseCppOverloads; /// set if overloaded functions are grouped and in reverse order (such as in dmc and cl)
        bool cppExceptions;       /// set if catching C++ exceptions is supported
        bool twoDtorInVtable;     /// target C++ ABI puts deleting and non-deleting destructor into vtable
    }

    /**
     * Values representing all properties for floating point types
     */
    extern (C++) struct FPTypeProperties(T)
    {
        __gshared
        {
            real_t max;                         /// largest representable value that's not infinity
            real_t min_normal;                  /// smallest representable normalized value that's not 0
            real_t nan;                         /// NaN value
            real_t snan;                        /// signalling NaN value
            real_t infinity;                    /// infinity value
            real_t epsilon;                     /// smallest increment to the value 1

            d_int64 dig;                        /// number of decimal digits of precision
            d_int64 mant_dig;                   /// number of bits in mantissa
            d_int64 max_exp;                    /// maximum int value such that 2$(SUPERSCRIPT `max_exp-1`) is representable
            d_int64 min_exp;                    /// minimum int value such that 2$(SUPERSCRIPT `min_exp-1`) is representable as a normalized value
            d_int64 max_10_exp;                 /// maximum int value such that 10$(SUPERSCRIPT `max_10_exp` is representable)
            d_int64 min_10_exp;                 /// minimum int value such that 10$(SUPERSCRIPT `min_10_exp`) is representable as a normalized value
        }
    }

    ///
    alias FloatProperties = FPTypeProperties!float;
    ///
    alias DoubleProperties = FPTypeProperties!double;
    ///
    alias RealProperties = FPTypeProperties!real_t;

    /**
     * Initialize the Target
     */
    extern (C++) static void _init();

    /**
     * Requested target memory alignment size of the given type.
     * Params:
     *      type = type to inspect
     * Returns:
     *      alignment in bytes
     */
    extern (C++) static uint alignsize(Type type);

    /**
     * Requested target field alignment size of the given type.
     * Params:
     *      type = type to inspect
     * Returns:
     *      alignment in bytes
     */
    extern (C++) static uint fieldalign(Type type);

    /**
     * Size of the target OS critical section.
     * Returns:
     *      size in bytes
     */
    extern (C++) static uint critsecsize();

    /**
     * Type for the `va_list` type for the target.
     * NOTE: For Posix/x86_64 this returns the type which will really
     * be used for passing an argument of type va_list.
     * Returns:
     *      `Type` that represents `va_list`.
     */
    extern (C++) static Type va_listType();

    /**
     * Checks whether the target supports a vector type.
     * Params:
     *      sz   = vector type size in bytes
     *      type = vector element type
     * Returns:
     *      0   vector type is supported,
     *      1   vector type is not supported on the target at all
     *      2   vector element type is not supported
     *      3   vector size is not supported
     */
    extern (C++) static int isVectorTypeSupported(int sz, Type type);

    /**
     * Checks whether the target supports the given operation for vectors.
     * Params:
     *      type = target type of operation
     *      op   = the unary or binary op being done on the `type`
     *      t2   = type of second operand if `op` is a binary operation
     * Returns:
     *      true if the operation is supported or type is not a vector
     */
    extern (C++) static bool isVectorOpSupported(Type type, TOK op, Type t2 = null);

    /**
     * Mangle the given symbol for C++ ABI.
     * Params:
     *      s = declaration with C++ linkage
     * Returns:
     *      string mangling of symbol
     */
    extern (C++) static const(char)* toCppMangle(Dsymbol s);

    /**
     * Get RTTI mangling of the given class declaration for C++ ABI.
     * Params:
     *      cd = class with C++ linkage
     * Returns:
     *      string mangling of C++ typeinfo
     */
    extern (C++) static const(char)* cppTypeInfoMangle(ClassDeclaration cd);

    /**
     * Gets vendor-specific type mangling for C++ ABI.
     * Params:
     *      t = type to inspect
     * Returns:
     *      string if type is mangled specially on target
     *      null if unhandled
     */
    extern (C++) static const(char)* cppTypeMangle(Type t);

    /**
     * Get the type that will really be used for passing the given argument
     * to an `extern(C++)` function.
     * Params:
     *      p = parameter to be passed.
     * Returns:
     *      `Type` to use for parameter `p`.
     */
    extern (C++) static Type cppParameterType(Parameter p);

    /**
     * Default system linkage for the target.
     * Returns:
     *      `LINK` to use for `extern(System)`
     */
    extern (C++) static LINK systemLinkage();

    /**
     * Describes how an argument type is passed to a function on target.
     * Params:
     *      t = type to break down
     * Returns:
     *      tuple of types if type is passed in one or more registers
     *      empty tuple if type is always passed on the stack
     */
    extern (C++) static TypeTuple toArgTypes(Type t)
    {
        return .toArgTypes(t);
    }

    /**
     * Determine return style of function - whether in registers or
     * through a hidden pointer to the caller's stack.
     * Params:
     *   tf = function type to check
     *   needsThis = true if the function type is for a non-static member function
     * Returns:
     *   true if return value from function is on the stack
     */
    extern (C++) static bool isReturnOnStack(TypeFunction tf, bool needsThis);

    /***
     * Determine the size a value of type `t` will be when it
     * is passed on the function parameter stack.
     * Params:
     *  loc = location to use for error messages
     *  t = type of parameter
     * Returns:
     *  size used on parameter stack
     */
    extern (C++) static ulong parameterSize(const ref Loc loc, Type t);

    /**
     * Get targetInfo by key
     * Params:
     *  name = name of targetInfo to get
     *  loc = location to use for error messages
     * Returns:
     *  Expression for the requested targetInfo
     */
    extern (C++) static Expression getTargetInfo(const(char)* name, const ref Loc loc);
}
