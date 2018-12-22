/**
 * Compiler implementation of the
 * $(LINK2 http://www.dlang.org, D programming language).
 *
 * Copyright:   Copyright (C) 1999-2018 by The D Language Foundation, All Rights Reserved
 * Authors:     $(LINK2 http://www.digitalmars.com, Walter Bright)
 * License:     $(LINK2 http://www.boost.org/LICENSE_1_0.txt, Boost License 1.0)
 * Source:      $(LINK2 https://github.com/dlang/dmd/blob/master/src/dmd/errors.d, _errors.d)
 * Documentation:  https://dlang.org/phobos/dmd_errors.html
 * Coverage:    https://codecov.io/gh/dlang/dmd/src/master/src/dmd/errors.d
 */

module dmd.errors;

import core.stdc.stdarg;
import dmd.globals;

/**
 * Print an error message, increasing the global error count.
 * Params:
 *      loc    = location of error
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void error(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    verror(loc, format, ap);
    va_end(ap);
}

/**
 * Same as above, but allows Loc() literals to be passed.
 * Params:
 *      loc    = location of error
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (D) void error(Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    verror(loc, format, ap);
    va_end(ap);
}

/**
 * Same as above, but takes a filename and line information arguments as separate parameters.
 * Params:
 *      filename = source file of error
 *      linnum   = line in the source file
 *      charnum  = column number on the line
 *      format   = printf-style format specification
 *      ...      = printf-style variadic arguments
 */
extern (C++) void error(const(char)* filename, uint linnum, uint charnum, const(char)* format, ...)
{
    const loc = Loc(filename, linnum, charnum);
    va_list ap;
    va_start(ap, format);
    verror(loc, format, ap);
    va_end(ap);
}

/**
 * Print additional details about an error message.
 * Doesn't increase the error count or print an additional error prefix.
 * Params:
 *      loc    = location of error
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void errorSupplemental(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    verrorSupplemental(loc, format, ap);
    va_end(ap);
}

/**
 * Print a warning message, increasing the global warning count.
 * Params:
 *      loc    = location of warning
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void warning(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vwarning(loc, format, ap);
    va_end(ap);
}

/**
 * Print additional details about a warning message.
 * Doesn't increase the warning count or print an additional warning prefix.
 * Params:
 *      loc    = location of warning
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void warningSupplemental(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vwarningSupplemental(loc, format, ap);
    va_end(ap);
}

/**
 * Print a deprecation message, may increase the global warning or error count
 * depending on whether deprecations are ignored.
 * Params:
 *      loc    = location of deprecation
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void deprecation(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vdeprecation(loc, format, ap);
    va_end(ap);
}

/**
 * Print additional details about a deprecation message.
 * Doesn't increase the error count, or print an additional deprecation prefix.
 * Params:
 *      loc    = location of deprecation
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void deprecationSupplemental(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vdeprecationSupplemental(loc, format, ap);
    va_end(ap);
}

/**
 * Print a verbose message.
 * Doesn't prefix or highlight messages.
 * Params:
 *      loc    = location of message
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void message(const ref Loc loc, const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vmessage(loc, format, ap);
    va_end(ap);
}

/**
 * Same as above, but doesn't take a location argument.
 * Params:
 *      format = printf-style format specification
 *      ...    = printf-style variadic arguments
 */
extern (C++) void message(const(char)* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vmessage(Loc.initial, format, ap);
    va_end(ap);
}

/**
 * Same as $(D error), but takes a va_list parameter, and optionally additional message prefixes.
 * Params:
 *      loc    = location of error
 *      format = printf-style format specification
 *      ap     = printf-style variadic arguments
 *      p1     = additional message prefix
 *      p2     = additional message prefix
 *      header = title of error message
 */
extern (C++) void verror(const ref Loc loc, const(char)* format, va_list ap, const(char)* p1 = null, const(char)* p2 = null, const(char)* header = "Error: ");

/**
 * Same as $(D errorSupplemental), but takes a va_list parameter.
 * Params:
 *      loc    = location of error
 *      format = printf-style format specification
 *      ap     = printf-style variadic arguments
 */
extern (C++) void verrorSupplemental(const ref Loc loc, const(char)* format, va_list ap);

/**
 * Same as $(D warning), but takes a va_list parameter.
 * Params:
 *      loc    = location of warning
 *      format = printf-style format specification
 *      ap     = printf-style variadic arguments
 */
extern (C++) void vwarning(const ref Loc loc, const(char)* format, va_list ap);

/**
 * Same as $(D warningSupplemental), but takes a va_list parameter.
 * Params:
 *      loc    = location of warning
 *      format = printf-style format specification
 *      ap     = printf-style variadic arguments
 */
extern (C++) void vwarningSupplemental(const ref Loc loc, const(char)* format, va_list ap);

/**
 * Same as $(D deprecation), but takes a va_list parameter, and optionally additional message prefixes.
 * Params:
 *      loc    = location of deprecation
 *      format = printf-style format specification
 *      ap     = printf-style variadic arguments
 *      p1     = additional message prefix
 *      p2     = additional message prefix
 */
extern (C++) void vdeprecation(const ref Loc loc, const(char)* format, va_list ap, const(char)* p1 = null, const(char)* p2 = null);

/**
 * Same as $(D message), but takes a va_list parameter.
 * Params:
 *      loc       = location of message
 *      format    = printf-style format specification
 *      ap        = printf-style variadic arguments
 */
extern (C++) void vmessage(const ref Loc loc, const(char)* format, va_list ap);

/**
 * Same as $(D deprecationSupplemental), but takes a va_list parameter.
 * Params:
 *      loc    = location of deprecation
 *      format = printf-style format specification
 *      ap     = printf-style variadic arguments
 */
extern (C++) void vdeprecationSupplemental(const ref Loc loc, const(char)* format, va_list ap);

/**
 * Call this after printing out fatal error messages to clean up and exit
 * the compiler.
 */
extern (C++) void fatal();

/**
 * Try to stop forgetting to remove the breakpoints from
 * release builds.
 */
extern (C++) void halt();
