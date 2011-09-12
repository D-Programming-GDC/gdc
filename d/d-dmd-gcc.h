/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

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

/* This file contains declarations used by the modified DMD front-end to
   interact with GCC-specific code. */

#ifndef GCC_DCMPLR_DMD_GCC_H
#define GCC_DCMPLR_DMD_GCC_H

#ifdef __cplusplus

/* used in module.c */
struct Module;
extern void d_gcc_magic_module(Module *m);
extern void d_gcc_dump_source(const char * filename, const char * ext, unsigned char * data, unsigned len);

/* used in func.c */
struct Type;
extern Type * d_gcc_builtin_va_list_d_type;

/* used in parse.c */
extern bool d_gcc_is_target_win32();

/* used in toobj.c */
struct VarDeclaration;
extern void d_gcc_emit_local_variable(VarDeclaration *);
extern bool d_gcc_supports_weak();

#if V2
struct Symbol;
enum RTLSYM
{
    RTLSYM_DHIDDENFUNC,
    N_RTLSYM
};
extern Symbol* rtlsym[N_RTLSYM];
#endif

/* used in template.c */
extern bool d_gcc_force_templates();
extern Module * d_gcc_get_output_module();

#if V2
/* used in interpret.c */
struct FuncDeclaration;
typedef ArrayBase<struct Expression> Expressions;
extern Expression * d_gcc_eval_builtin(FuncDeclaration *, Expressions *);
#endif

#endif

#endif
