/* GDC -- D front-end for GCC
   Copyright (C) 2011, 2012 Free Software Foundation, Inc.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.
*/


/**
  Declarations are automatically created by the compiler.  All
  declarations start with "__builtin_". Refer to _builtins.def in the
  GCC source for a list of functions.  Not all of the functions are
  supported.

  In addition to built-in functions, the following types are defined.

  $(TABLE
  $(TR $(TD ___builtin_va_list)      $(TD The target's va_list type ))
  $(TR $(TD ___builtin_clong  )      $(TD The D equivalent of the target's
                                           C "long" type ))
  $(TR $(TD ___builtin_culong )      $(TD The D equivalent of the target's
                                           C "unsigned long" type ))
  $(TR $(TD ___builtin_machine_int ) $(TD Signed word-sized integer ))
  $(TR $(TD ___builtin_machine_uint) $(TD Unsigned word-sized integer ))
  $(TR $(TD ___builtin_pointer_int ) $(TD Signed pointer-sized integer ))
  $(TR $(TD ___builtin_pointer_uint) $(TD Unsigned pointer-sized integer ))
  )
 */

module gcc.builtins;
