/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, (C) 2009, 2010
    Iain Buclaw, (C) 2010, 2011

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

/*
  dc-lang.cc: implementation of back-end callbacks and data structures
*/

#include "root.h"
#include "mtype.h"
#include "id.h"
#include "module.h"
#include "cond.h"
#include "mars.h"

#include "async.h"
#include "json.h"

#include "d-gcc-includes.h"
#include "options.h"
#include "cppdefault.h"

#include "d-lang.h"
#include "d-codegen.h"
#include "d-gcc-real.h"
#include "d-confdefs.h"

static char lang_name[6] = "GNU D";

/* Lang Hooks */
#undef LANG_HOOKS_NAME
#undef LANG_HOOKS_INIT
#undef LANG_HOOKS_INIT_OPTIONS
#undef LANG_HOOKS_HANDLE_OPTION
#undef LANG_HOOKS_POST_OPTIONS
#undef LANG_HOOKS_PARSE_FILE
#undef LANG_HOOKS_COMMON_ATTRIBUTE_TABLE
#undef LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE
#undef LANG_HOOKS_GET_ALIAS_SET
#undef LANG_HOOKS_MARK_ADDRESSABLE
#undef LANG_HOOKS_TYPES_COMPATIBLE_P
#undef LANG_HOOKS_BUILTIN_FUNCTION
#undef LANG_HOOKS_REGISTER_BUILTIN_TYPE
#undef LANG_HOOKS_FINISH_INCOMPLETE_DECL

#define LANG_HOOKS_NAME                     lang_name
#define LANG_HOOKS_INIT                     d_init
#define LANG_HOOKS_INIT_OPTIONS             d_init_options
#define LANG_HOOKS_HANDLE_OPTION            d_handle_option
#define LANG_HOOKS_POST_OPTIONS             d_post_options
#define LANG_HOOKS_PARSE_FILE               d_parse_file
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE   d_common_attribute_table
#define LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE   d_common_format_attribute_table
#define LANG_HOOKS_GET_ALIAS_SET            d_hook_get_alias_set
#define LANG_HOOKS_MARK_ADDRESSABLE         d_mark_addressable
#define LANG_HOOKS_TYPES_COMPATIBLE_P       d_types_compatible_p
#define LANG_HOOKS_REGISTER_BUILTIN_TYPE    d_register_builtin_type
#define LANG_HOOKS_FINISH_INCOMPLETE_DECL   d_finish_incomplete_decl

#if D_GCC_VER >= 43
#define LANG_HOOKS_BUILTIN_FUNCTION         d_builtin_function43
#else
#define LANG_HOOKS_BUILTIN_FUNCTION         d_builtin_function
#endif

#if D_GCC_VER >= 43
#undef LANG_HOOKS_GIMPLIFY_EXPR
#define LANG_HOOKS_GIMPLIFY_EXPR            d_gimplify_expr
#endif

#if D_GCC_VER >= 44
#undef LANG_HOOKS_BUILTIN_FUNCTION_EXT_SCOPE
#define LANG_HOOKS_BUILTIN_FUNCTION_EXT_SCOPE d_builtin_function43
#endif

#if D_GCC_VER >= 45
#undef LANG_HOOKS_EH_PERSONALITY
#undef LANG_HOOKS_EH_RUNTIME_TYPE

#define LANG_HOOKS_EH_PERSONALITY           d_eh_personality
#define LANG_HOOKS_EH_RUNTIME_TYPE          d_build_eh_type_type
#endif

#if D_GCC_VER >= 46
#undef LANG_HOOKS_OPTION_LANG_MASK
#undef LANG_HOOKS_INIT_OPTIONS_STRUCT
#define LANG_HOOKS_OPTION_LANG_MASK         d_option_lang_mask
#define LANG_HOOKS_INIT_OPTIONS_STRUCT      d_init_options_struct
#endif

/* Lang Hooks for decls */
#undef LANG_HOOKS_WRITE_GLOBALS
#define LANG_HOOKS_WRITE_GLOBALS            d_write_global_declarations

/* Lang Hooks for types */
#undef LANG_HOOKS_TYPE_FOR_MODE
#undef LANG_HOOKS_TYPE_FOR_SIZE
#undef LANG_HOOKS_TYPE_PROMOTES_TO
#undef LANG_HOOKS_UNSIGNED_TYPE
#undef LANG_HOOKS_SIGNED_TYPE
#undef LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE

#define LANG_HOOKS_TYPE_FOR_MODE            d_type_for_mode
#define LANG_HOOKS_TYPE_FOR_SIZE            d_type_for_size
#define LANG_HOOKS_TYPE_PROMOTES_TO         d_type_promotes_to
#define LANG_HOOKS_UNSIGNED_TYPE            d_unsigned_type
#define LANG_HOOKS_SIGNED_TYPE              d_signed_type
#define LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE  d_signed_or_unsigned_type

/* Lang Hooks for callgraph */
#if D_GCC_VER < 43
#undef LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION
#define LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION d_expand_function
#endif

/* Lang Hooks for tree inlining */
#if V2
#undef LANG_HOOKS_TREE_INLINING_CONVERT_PARM_FOR_INLINING
#define LANG_HOOKS_TREE_INLINING_CONVERT_PARM_FOR_INLINING  d_convert_parm_for_inlining
#endif


////static tree d_type_for_size PARAMS ((unsigned, int));
static tree d_signed_or_unsigned_type(int, tree);
////tree d_unsigned_type(tree);
////tree d_signed_type(tree);

static const char * fonly_arg;
// Because of PR16888, on x86 platforms, GCC clears unused reg names.
// As this doesn't affect us, need a way to restore them.
static const char *saved_reg_names[FIRST_PSEUDO_REGISTER];

#if D_GCC_VER >= 46
/* Common initialization before calling option handlers.  */
static void
d_init_options (unsigned int, struct cl_decoded_option *decoded_options)
{
    // Set default values
    global.params.argv0 = xstrdup(decoded_options[0].arg);
    global.params.link = 1;
    global.params.useAssert = 1;
    global.params.useInvariants = 1;
    global.params.useIn = 1;
    global.params.useOut = 1;
    global.params.useArrayBounds = 2;
    global.params.useSwitchError = 1;
    global.params.useInline = 0;
    global.params.warnings = 0;
    global.params.obj = 1;
    global.params.Dversion = 2;
    global.params.quiet = 1;

    global.params.linkswitches = new Strings();
    global.params.libfiles = new Strings();
    global.params.objfiles = new Strings();
    global.params.ddocfiles = new Strings();

    global.params.imppath = new Strings();
    global.params.fileImppath = new Strings();

    // extra D-specific options
    gen.splitDynArrayVarArgs = true;
    gen.emitTemplates = TEnormal;
    gen.useInlineAsm = true;
    gen.useBuiltins = true;
    std_inc = true;
}

/* Initialize options structure OPTS.  */
static void
d_init_options_struct (struct gcc_options *opts)
{
    // GCC options
    opts->x_flag_exceptions = 1;

    // Avoid range issues for complex multiply and divide.
    opts->x_flag_complex_method = 2;

    // Unlike C, there is no global 'errno' variable.
    opts->x_flag_errno_math = 0;
    opts->frontend_set_flag_errno_math = true;

    // Keep in synch with existing -fbounds-check flag.
    opts->x_flag_bounds_check = 1;

    // Honour left to right code evaluation
    opts->x_flag_evaluation_order = 1;
}

/* Return language mask for option parsing.  */
static unsigned int
d_option_lang_mask (void)
{
    return CL_D;
}

#else
static unsigned int
d_init_options (unsigned int, const char ** argv)
{
    // Set default values
    global.params.argv0 = xstrdup(argv[0]);
    global.params.link = 1;
    global.params.useAssert = 1;
    global.params.useInvariants = 1;
    global.params.useIn = 1;
    global.params.useOut = 1;
    global.params.useArrayBounds = 2;
    global.params.useSwitchError = 1;
    global.params.useInline = 0;
    global.params.warnings = 0;
    global.params.obj = 1;
    global.params.Dversion = 2;
    global.params.quiet = 1;

    global.params.linkswitches = new Strings();
    global.params.libfiles = new Strings();
    global.params.objfiles = new Strings();
    global.params.ddocfiles = new Strings();

    global.params.imppath = new Strings();
    global.params.fileImppath = new Strings();

    // GCC options
    flag_exceptions = 1;

    // Avoid range issues for complex multiply and divide.
    flag_complex_method = 2;

    // Unlike C, there is no global 'errno' variable.
    flag_errno_math = 0;

    // keep in synch with existing -fbounds-check flag
    flag_bounds_check = global.params.useArrayBounds;

    // Honour left to right code evaluation.
    flag_evaluation_order = 1;

    // extra D-specific options
    gen.splitDynArrayVarArgs = true;
    gen.emitTemplates = TEnormal;
    gen.useInlineAsm = true;
    gen.useBuiltins = true;
    std_inc = true;

    return CL_D;
}
#endif

// support for the -mno-cygwin switch
// copied from cygwin.h, cygwin2.c
#ifndef CYGWIN_MINGW_SUBDIR
#define CYGWIN_MINGW_SUBDIR "/mingw"
#endif
#define CYGWIN_MINGW_SUBDIR_LEN (sizeof (CYGWIN_MINGW_SUBDIR) - 1)

char cygwin_d_phobos_dir[sizeof(D_PHOBOS_DIR) + 1
        + (CYGWIN_MINGW_SUBDIR_LEN)] = D_PHOBOS_DIR;
#undef D_PHOBOS_DIR
#define D_PHOBOS_DIR (cygwin_d_phobos_dir)
char cygwin_d_target_dir[sizeof(D_PHOBOS_TARGET_DIR) + 1
        + (CYGWIN_MINGW_SUBDIR_LEN)] = D_PHOBOS_TARGET_DIR;
#undef D_PHOBOS_TARGET_DIR
#define D_PHOBOS_TARGET_DIR (cygwin_d_target_dir)

#ifdef D_OS_VERSYM
const char * cygwin_d_os_versym = D_OS_VERSYM;
#undef D_OS_VERSYM
#define D_OS_VERSYM cygwin_d_os_versym
#endif

void
maybe_fixup_cygwin()
{
#ifdef D_OS_VERSYM
    char * env = getenv("GCC_CYGWIN_MINGW");
    char * p;
    char ** av;

    static char *d_cvt_to_mingw[] = {
        cygwin_d_phobos_dir,
        cygwin_d_target_dir,
        NULL
    };
    if (!strcmp(cygwin_d_os_versym,"cygwin") && env && *env == '1')
    {
        cygwin_d_os_versym = "Win32";

        for (av = d_cvt_to_mingw; *av; av++)
        {
            int sawcygwin = 0;
            while ((p = strstr (*av, "-cygwin")))
            {
                char *over = p + sizeof ("-cygwin") - 1;
                memmove (over + 1, over, strlen (over));
                memcpy (p, "-mingw32", sizeof("-mingw32") - 1);
                p = ++over;
                while (ISALNUM (*p))
                    p++;
                strcpy (over, p);
                sawcygwin = 1;
            }
            if (!sawcygwin && !strstr (*av, "mingw"))
                strcat (*av, CYGWIN_MINGW_SUBDIR);
        }
    }
#endif
}

static bool is_target_win32 = false;

bool
d_gcc_is_target_win32()
{
    return is_target_win32;
}

static bool
d_init ()
{
    const char * cpu_versym = NULL;

    /* Restore register names if any were cleared during backend init */
    if (memcmp (reg_names, saved_reg_names, sizeof reg_names))
        memcpy (reg_names, saved_reg_names, sizeof reg_names);

    /* Currently, is64bit indicates a 64-bit target in general and is not
       Intel-specific. */
#ifdef TARGET_64BIT
    global.params.is64bit = TARGET_64BIT ? 1 : 0;
#else
    /* TARGET_64BIT is only defined on biarched archs defaulted to 64-bit
     * (as amd64 or s390x) so for full 64-bit archs (as ia64 or alpha) we
     * need to test it more. */
#  ifdef D_CPU_VERSYM64
    /* We are "defaulting" to 32-bit, which mean that if both D_CPU_VERSYM
     * and D_CPU_VERSYM64 are defined, and not TARGET_64BIT, we will use
     * 32 bits. This will be overidden for full 64-bit archs */
    global.params.is64bit = 0;
#    ifndef D_CPU_VERSYM
    /* So this is typically for alpha and ia64 */
    global.params.is64bit = 1;
#    endif
#  else
#    ifdef D_CPU_VERSYM /* D_CPU_VERSYM is defined and D_CPU_VERSYM64 is not. */
    global.params.is64bit = 0;
#    else
    /* If none of D_CPU_VERSYM and D_CPU_VERSYM64 defined check size_t
     * length instead. */
    switch (sizeof(size_t))
    {
        case 4:
            global.params.is64bit = 0;
            break;
        case 8:
            global.params.is64bit = 1;
            break;
    }
#    endif
#  endif
#endif

#ifdef D_USE_MAPPED_LOCATION
#if 0
    /* input_location is initially set to BUILTINS_LOCATION (2).  This
       will cause a segfault if errors are reported before any line maps
       are created. Setting input_location to zero stops those segfaults,
       but then decls are created with DECL_SOURCE_LOCATION set to zero
       which causes segfaults in dwarf2out. */
    linemap_add(line_table, LC_RENAME, 0, "<builtin>", 1);
    input_location = linemap_line_start(line_table, 1, 1);
#endif
#endif

    Type::init();
    Id::initialize();
    Module::init();
    initPrecedence();
    gcc_d_backend_init();
    real_t::init();

    maybe_fixup_cygwin();

    VersionCondition::addPredefinedGlobalIdent("GNU");
#if V2
    VersionCondition::addPredefinedGlobalIdent("D_Version2");
#endif
#ifdef D_CPU_VERSYM64
    if (global.params.is64bit == 1)
        cpu_versym = D_CPU_VERSYM64;
#  ifdef D_CPU_VERSYM
    else
        cpu_versym = D_CPU_VERSYM;
#  endif
#else
#  ifdef D_CPU_VERSYM
    cpu_versym = D_CPU_VERSYM;
#  endif
#endif
    if (cpu_versym)
        VersionCondition::addPredefinedGlobalIdent(cpu_versym);
#ifdef D_OS_VERSYM
    VersionCondition::addPredefinedGlobalIdent(D_OS_VERSYM);
    if (strcmp(D_OS_VERSYM, "darwin") == 0)
        VersionCondition::addPredefinedGlobalIdent("OSX");
    if (strcmp(D_OS_VERSYM, "Win32") == 0)
        is_target_win32 = true;
#endif
#ifdef D_OS_VERSYM2
    VersionCondition::addPredefinedGlobalIdent(D_OS_VERSYM2);
    if (strcmp(D_OS_VERSYM2, "Win32") == 0)
        is_target_win32 = true;
#endif
#ifdef D_VENDOR_VERSYM
    VersionCondition::addPredefinedGlobalIdent(D_VENDOR_VERSYM);
#endif

#ifdef TARGET_THUMB
    if (TARGET_THUMB)
        VersionCondition::addPredefinedGlobalIdent("Thumb");
    else
        VersionCondition::addPredefinedGlobalIdent("Arm");
#endif

    if (BYTES_BIG_ENDIAN)
        VersionCondition::addPredefinedGlobalIdent("BigEndian");
    else
        VersionCondition::addPredefinedGlobalIdent("LittleEndian");

#if D_GCC_VER >= 46
    if (targetm.except_unwind_info(&global_options) == UI_SJLJ)
        VersionCondition::addPredefinedGlobalIdent("GNU_SjLj_Exceptions");
#else
    if (USING_SJLJ_EXCEPTIONS)
        VersionCondition::addPredefinedGlobalIdent("GNU_SjLj_Exceptions");
#endif

#ifdef TARGET_LONG_DOUBLE_128
    if (TARGET_LONG_DOUBLE_128)
        VersionCondition::addPredefinedGlobalIdent("GNU_LongDouble128");
#endif

#ifdef STACK_GROWS_DOWNWARD
    VersionCondition::addPredefinedGlobalIdent("GNU_StackGrowsDown");
#endif

    if (d_have_inline_asm() && gen.useInlineAsm)
    {
        VersionCondition::addPredefinedGlobalIdent("D_InlineAsm");

        if (cpu_versym)
        {
            if (strcmp(cpu_versym, "X86") == 0)
                VersionCondition::addPredefinedGlobalIdent("D_InlineAsm_X86");
            else if (strcmp(cpu_versym, "X86_64") == 0)
                VersionCondition::addPredefinedGlobalIdent("D_InlineAsm_X86_64");
        }

        /* Should define this anyway to set us apart from the competition. */
        VersionCondition::addPredefinedGlobalIdent("GNU_InlineAsm");
    }

    /* Setting global.params.cov forces module info generation which is
       not needed for thee GCC coverage implementation.  Instead, just
       test flag_test_coverage while leaving global.params.cov unset. */
    //if (global.params.cov)
    if (flag_test_coverage)
        VersionCondition::addPredefinedGlobalIdent("D_Coverage");
    if (flag_pic)
        VersionCondition::addPredefinedGlobalIdent("D_PIC");
    if (global.params.doDocComments)
        VersionCondition::addPredefinedGlobalIdent("D_Ddoc");
    if (global.params.useUnitTests)
        VersionCondition::addPredefinedGlobalIdent("unittest");

    VersionCondition::addPredefinedGlobalIdent("all");

    register_import_chains();

    {
        char * path = FileName::searchPath(global.path, "phobos-ver-syms", 1);
        if (path)
        {
            FILE * f = fopen(path, "r");
            char buf[256];
            char *p, *q;
            if (f)
            {
                while ( ! feof(f) && fgets(buf, 256, f) )
                {
                    p = buf;
                    while (*p && ISSPACE(*p))
                        p++;
                    q = p;
                    while (*q && ! ISSPACE(*q))
                        q++;
                    *q = 0;
                    if (p != q)
                    {   /* Needs to be predefined because we define
                           Unix/Windows this way. */
                        VersionCondition::addPredefinedGlobalIdent(xstrdup(p));
                    }
                }
                fclose(f);
            }
            else
            {
                //printf("failed\n");
            }
        }
        else
        {
            //printf("no p-v-s found\n");
        }
    }

    return 1;
}

static bool
parse_int (const char * arg, int * value_ret)
{
    long v;
    char * err;
    errno = 0;
    v = strtol(arg, & err, 10);
    if (*err || errno || v > INT_MAX)
        return false;
    * value_ret = v;
    return true;
}

#if D_GCC_VER >= 46
static bool
d_handle_option (size_t scode, const char *arg, int value,
                 int kind, location_t loc,
                 const struct cl_option_handlers *handlers)
#else
static int
d_handle_option (size_t scode, const char *arg, int value)
#endif
{
  enum opt_code code = (enum opt_code) scode;
  int level;

  switch (code)
      {
      case OPT_I:
          global.params.imppath->push(xstrdup(arg)); // %% not sure if we can keep the arg or not
          break;
      case OPT_J:
          global.params.fileImppath->push(xstrdup(arg));
          break;
      case OPT_fdeprecated:
          global.params.useDeprecated = value;
          break;
      case OPT_fassert:
          global.params.useAssert = value;
          break;
      case OPT_frelease:
          global.params.useInvariants = ! value;
          global.params.useIn = ! value;
          global.params.useOut = ! value;
          global.params.useAssert = ! value;
#if V2
          // release mode doesn't turn off bounds checking for safe functions.
          global.params.useArrayBounds = ! value ? 2 : 1;
          flag_bounds_check = ! value;
#else
          flag_bounds_check = global.params.useArrayBounds = ! value;
#endif
          global.params.useSwitchError = ! value;
          break;
#if V2
      case OPT_fbounds_check:
          global.params.noboundscheck = ! value;
          break;
#endif
      case OPT_funittest:
          global.params.useUnitTests = value;
          break;
      case OPT_fversion_:
          if (ISDIGIT(arg[0]))
          {
              if (! parse_int(arg, & level))
                  goto Lerror_v;
              VersionCondition::setGlobalLevel(level);
          }
          else if (Lexer::isValidIdentifier(CONST_CAST(char*, arg)))
              VersionCondition::addGlobalIdent(xstrdup(arg));
          else
          {
        Lerror_v:
              error("bad argument for -fversion");
          }
          break;
      case OPT_fdebug:
          global.params.debuglevel = value ? 1 : 0;
          break;
      case OPT_fdebug_:
          if (ISDIGIT(arg[0]))
          {
              if (! parse_int(arg, & level))
                  goto Lerror_d;
              DebugCondition::setGlobalLevel(level);
          }
          else if (Lexer::isValidIdentifier(CONST_CAST(char*, arg)))
              DebugCondition::addGlobalIdent(xstrdup(arg));
          else
          {
        Lerror_d:
              error("bad argument for -fdebug");
          }
          break;
      case OPT_fdebug_c:
          strcpy(lang_name, value ? "GNU C" : "GNU D");
          break;
      case OPT_fdeps_:
          global.params.moduleDepsFile = xstrdup(arg);
          if (!global.params.moduleDepsFile[0])
              error("bad argument for -fdeps");
          global.params.moduleDeps = new OutBuffer;
          break;
      case OPT_fignore_unknown_pragmas:
          global.params.ignoreUnsupportedPragmas = value;
          break;
#if V2
      case OPT_fproperty:
          global.params.enforcePropertySyntax = 1;
#endif
      case OPT_fintfc:
          global.params.doHdrGeneration = value;
          break;
      case OPT_fintfc_dir_:
          global.params.doHdrGeneration = 1;
          global.params.hdrdir = xstrdup(arg);
          break;
      case OPT_fintfc_file_:
          global.params.doHdrGeneration = 1;
          global.params.hdrname = xstrdup(arg);
          break;
      case OPT_fdoc:
          global.params.doDocComments = value;
          break;
      case OPT_fdoc_dir_:
          global.params.doDocComments = 1;
          global.params.docdir = xstrdup(arg);
          break;
      case OPT_fdoc_file_:
          global.params.doDocComments = 1;
          global.params.docname = xstrdup(arg);
          break;
      case OPT_fdoc_inc_:
          global.params.ddocfiles->push(xstrdup(arg));
          break;
      case OPT_fd_verbose:
          global.params.verbose = 1;
          break;
      case OPT_fd_vtls:
          global.params.vtls = 1;
          break;
      case OPT_fd_version_1:
          global.params.Dversion = 1;
          break;
      case OPT_femit_templates:
          gen.emitTemplates = value ? TEauto : TEnone;
          break;
      case OPT_femit_templates_:
          if (! arg || ! *arg)
              gen.emitTemplates = value ? TEauto : TEnone;
          else if (! strcmp(arg, "normal"))
              gen.emitTemplates = TEnormal;
          else if (! strcmp(arg, "all"))
              gen.emitTemplates = TEall;
          else if (! strcmp(arg, "private"))
              gen.emitTemplates = TEprivate;
          else if (! strcmp(arg, "none"))
              gen.emitTemplates = TEnone;
          else if (! strcmp(arg, "auto"))
              gen.emitTemplates = TEauto;
          else
              error("bad argument for -femit-templates");
          break;
      case OPT_fonly_:
          fonly_arg = xstrdup(arg);
          break;
      case OPT_iprefix:
          iprefix = xstrdup(arg);
          break;
      case OPT_imultilib:
          multilib_dir = xstrdup(arg);
          break;
      case OPT_nostdinc:
          std_inc = false;
          break;
      case OPT_fdump_source:
          global.params.dump_source = value;
          break;
      case OPT_fasm:
          gen.useInlineAsm = value;
          break;
      case OPT_fbuiltin:
          gen.useBuiltins = value;
          break;
      case OPT_Wall:
          global.params.warnings = 2;
          gen.warnSignCompare = value;
          break;
      case OPT_Werror:
          global.params.warnings = 1;
          gen.warnSignCompare = value;
          break;
      case OPT_Wsign_compare:
          gen.warnSignCompare = value;
          break;
      case OPT_fXf_:
          global.params.doXGeneration = 1;
          global.params.xfilename = xstrdup(arg);
          break;
      default:
          break;
      }
  return 1;
}

bool d_post_options(const char ** fn)
{
    // The front end considers the first input file to be the main one.
    if (num_in_fnames)
        *fn = in_fnames[0];

    // Save register names for restoring later.
    memcpy (saved_reg_names, reg_names, sizeof reg_names);

#if D_GCC_VER == 43
    // Workaround for embedded functions, don't inline if debugging is on.
    // See Issue #38 for why.
    flag_inline_small_functions = !write_symbols;
#endif

#if D_GCC_VER < 44
    // Inline option code copied from c-opts.c
    flag_inline_trees = 1;

    /* Use tree inlining.  */
    if (!flag_no_inline)
        flag_no_inline = 1;
    if (flag_inline_functions)
        flag_inline_trees = 2;
#endif

    /* If we are given more than one input file, we must use
       unit-at-a-time mode.  */
    if (num_in_fnames > 1)
        flag_unit_at_a_time = 1;

#if V2
    /* array bounds checking */
    if (global.params.noboundscheck)
        flag_bounds_check = global.params.useArrayBounds = 0;
#endif

#if D_GCC_VER >= 45
    /* Excess precision other than "fast" requires front-end
       support that we don't offer. */
    if (flag_excess_precision_cmdline == EXCESS_PRECISION_DEFAULT)
       flag_excess_precision_cmdline = EXCESS_PRECISION_FAST;
#endif
    return false;
}

/* wrapup_global_declaration needs to be called or functions will not
   be emitted. */
Array globalDeclarations;

void
d_add_global_declaration(tree decl)
{
    globalDeclarations.push(decl);
}

static void
d_write_global_declarations()
{
    tree * vec = (tree *) globalDeclarations.data;

    /* Process all file scopes in this compilation, and the external_scope,
       through wrapup_global_declarations and check_global_declarations.  */
    wrapup_global_declarations(vec, globalDeclarations.dim);
    check_global_declarations(vec, globalDeclarations.dim);

#if D_GCC_VER >= 45
    /* We're done parsing; proceed to optimize and emit assembly. */
    cgraph_finalize_compilation_unit();
#else
    cgraph_finalize_compilation_unit();
    cgraph_optimize();
#endif

    /* After cgraph has had a chance to emit everything that's going to
       be emitted, output debug information for globals.  */
    emit_debug_global_declarations(vec, globalDeclarations.dim);
}

// Some phobos code (isnormal, etc.) breaks with strict aliasing...
// so I guess D doesn't have aliasing rules.  Would be nice to enable
// strict aliasing, but hooking or defaulting flag_strict_aliasing is
// not trivial
static
#if D_GCC_VER < 43
HOST_WIDE_INT
#else
alias_set_type
#endif
d_hook_get_alias_set(tree)
{
    return 0;
}

/* Gimplification of expression trees.  */
#if D_GCC_VER >= 44
int
d_gimplify_expr (tree *expr_p, gimple_seq *pre_p ATTRIBUTE_UNUSED,
                 gimple_seq *post_p ATTRIBUTE_UNUSED)
{
    enum tree_code code = TREE_CODE (*expr_p);
    switch (code)
    {
        case MODIFY_EXPR:
        {   /* If the back end isn't clever enough to know that the lhs and rhs
               types are the same, add an explicit conversion.  */
            tree op0 = TREE_OPERAND (*expr_p, 0);
            tree op1 = TREE_OPERAND (*expr_p, 1);

            if (!gen.isErrorMark(op0) && !gen.isErrorMark(op1)
                && (TYPE_STRUCTURAL_EQUALITY_P (TREE_TYPE (op0))
                    || TYPE_STRUCTURAL_EQUALITY_P (TREE_TYPE (op1)))
                && !useless_type_conversion_p (TREE_TYPE (op1), TREE_TYPE (op0)))
            {
                TREE_OPERAND (*expr_p, 1) = build1 (VIEW_CONVERT_EXPR,
                                                    TREE_TYPE (op0), op1);
            }
        }

        default:
            return GS_UNHANDLED;
    }
}
#elif D_GCC_VER >= 43
int
d_gimplify_expr (tree *expr_p, tree *pre_p ATTRIBUTE_UNUSED,
                 tree *post_p ATTRIBUTE_UNUSED)
{
    enum tree_code code = TREE_CODE (*expr_p);
    switch (code)
    {
        case MODIFY_EXPR:
        {   /* If the back end isn't clever enough to know that the lhs and rhs
               types are the same, add an explicit conversion.  */
            tree op0 = TREE_OPERAND (*expr_p, 0);
            tree op1 = TREE_OPERAND (*expr_p, 1);

            if (!gen.isErrorMark(op0) && !gen.isErrorMark(op1)
                && (TYPE_STRUCTURAL_EQUALITY_P (TREE_TYPE (op0))
                    || TYPE_STRUCTURAL_EQUALITY_P (TREE_TYPE (op1)))
                && !useless_type_conversion_p (TREE_TYPE (op1), TREE_TYPE (op0)))
            {
                TREE_OPERAND (*expr_p, 1) = build1 (VIEW_CONVERT_EXPR,
                                                    TREE_TYPE (op0), op1);
            }
        }

        default:
            return GS_UNHANDLED;
    }
}
#endif

static Module * an_output_module = 0;

Module *
d_gcc_get_output_module()
{
    return an_output_module;
}

static void
nametype(tree type, const char * name)
{
    tree ident = get_identifier(name);
    tree decl = d_build_decl(TYPE_DECL, ident, type);
    TYPE_NAME(type) = decl;
    ObjectFile::rodc(decl, 1);
}

static void
nametype(Type * t)
{
    nametype(t->toCtype(), t->toChars());
}

#if V2
Symbol* rtlsym[N_RTLSYM];
#endif

void
#if D_GCC_VER >= 46
d_parse_file (void)
#else
d_parse_file (int /*set_yydebug*/)
#endif
{
    if (global.params.verbose)
    {
        printf("binary    %s\n", global.params.argv0);
        printf("version   %s\n", global.version);
    }

    if (global.params.verbose && asm_out_file == stdout)
    {   // Really, driver should see the option and turn off -pipe
        error("Cannot use -fd-verbose with -pipe");
        return;
    }

    if (global.params.useUnitTests)
        global.params.useAssert = 1;
#if V1
    global.params.useArrayBounds = flag_bounds_check;
#endif
    if (gen.emitTemplates == TEauto)
    {
        gen.emitTemplates = (supports_one_only()) ? TEall : TEprivate;
    }
    global.params.symdebug = write_symbols != NO_DEBUG;
    //global.params.useInline = flag_inline_functions;
    global.params.obj = ! flag_syntax_only;
    global.params.pic = flag_pic != 0; // Has no effect yet.
    gen.originalOmitFramePointer = flag_omit_frame_pointer;

    // better to use input_location.xxx ?
    (*debug_hooks->start_source_file) (input_line, main_input_filename);

    /*
    printf("input_filename = '%s'\n", input_filename);
    printf("main_input_filename = '%s'\n", main_input_filename);
    */

    for (TY ty = (TY) 0; ty < TMAX; ty = (TY)(ty + 1))
    {
        if (Type::basic[ty] && ty != Terror)
            nametype(Type::basic[ty]);
    }

    an_output_module = NULL;
    Modules modules; // vs. outmodules... = [an_output_module] or modules
    modules.reserve(num_in_fnames);
    AsyncRead * aw = NULL;
    Module * m = NULL;

    // %% FIX
    if ( ! main_input_filename )
    {
        ::error("input file name required; cannot use stdin");
        goto had_errors;
    }

    if (fonly_arg)
    {   /* In this mode, the first file name is supposed to be
           a duplicate of one of the input file. */
        if (strcmp(fonly_arg, main_input_filename))
            ::error("-fonly= argument is different from main input file name");
        if (strcmp(fonly_arg, in_fnames[0]))
            ::error("-fonly= argument is different from first input file name");
    }
    //fprintf (stderr, "***** %d files  main=%s\n", num_in_fnames, input_filename);

    for (size_t i = 0; i < num_in_fnames; i++)
    {
        if (fonly_arg)
        {
            /* %% Do the other modules really need to be processed?
            if (an_output_module)
                break;
             */
        }
        //fprintf(stderr, "fn %d = %s\n", i, in_fnames[i]);
        char * the_fname = xstrdup(in_fnames[i]);
        char * p = FileName::name(the_fname);
        char * e = FileName::ext(p);
        char * name;
        if (e)
        {
            e--;
            gcc_assert(*e == '.');
            name = (char *) xmalloc((e - p) + 1);
            memcpy(name, p, e - p);
            name[e - p] = 0;

            if (name[0] == 0 ||
                    strcmp(name, "..") == 0 ||
                    strcmp(name, ".") == 0)
            {
            Linvalid:
                ::error("invalid file name '%s'", the_fname);
                goto had_errors;
            }
        }
        else
        {
            name = p;
            if (!*name)
                goto Linvalid;
        }

        Identifier * id = Lexer::idPool(name);
        m = new Module(the_fname, id, global.params.doDocComments, global.params.doHdrGeneration);
        if (! strcmp(in_fnames[i], main_input_filename))
            an_output_module = m;
        modules.push(m);
    }

#if V2
    // There is only one of these so far...
    rtlsym[RTLSYM_DHIDDENFUNC] =
        gen.getLibCallDecl(LIBCALL_HIDDEN_FUNC)->toSymbol();
#endif

    // current_module shouldn't have any implications before genobjfile..
    // ... but it does.  We need to know what module in which to insert
    // TemplateInstanceS during the semantic pass.  In order for
    // -femit-templates=private to work, template instances must be emitted
    // in every translation unit.  To do this, the TemplateInstaceS have to
    // have toObjFile called in the module being compiled.
    // TemplateInstance puts itself somwhere during ::semantic, thus it has
    // to know the current module...

    gcc_assert(an_output_module);

    //global.params.verbose = 1;

    // Read files
    aw = AsyncRead::create(modules.dim);
    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        aw->addFile(m->srcfile);
    }
    aw->start();
    for (size_t i = 0; i < modules.dim; i++)
    {
        if (aw->read(i))
        {
            error("cannot read file %s", m->srcfile->name->toChars());
            goto had_errors;
        }
    }
    AsyncRead::dispose(aw);

    // Parse files
    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        if (global.params.verbose)
            printf("parse     %s\n", m->toChars());
        if (!Module::rootModule)
            Module::rootModule = m;
        m->importedFrom = m;
        //m->deleteObjFile(); // %% driver does this
        m->parse(global.params.dump_source);
        d_gcc_magic_module(m);
        if (m->isDocFile)
        {
            m->gendocfile();
            // Remove m from list of modules
            modules.remove(i);
            i--;
        }
    }
    if (global.errors)
        goto had_errors;

     if (global.params.doHdrGeneration)
     {
         /* Generate 'header' import files.
          * Since 'header' import files must be independent of command
          * line switches and what else is imported, they are generated
          * before any semantic analysis.
          */
         for (size_t i = 0; i < modules.dim; i++)
         {
             m = modules.tdata()[i];
             if (fonly_arg && m != an_output_module)
                 continue;
             if (global.params.verbose)
                 printf("import    %s\n", m->toChars());
             m->genhdrfile();
         }
     }
     if (global.errors)
         goto had_errors;

    // load all unconditional imports for better symbol resolving
    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        if (global.params.verbose)
            printf("importall %s\n", m->toChars());
        m->importAll(0);
    }
    if (global.errors)
        goto had_errors;

    // Do semantic analysis
    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        if (global.params.verbose)
            printf("semantic  %s\n", m->toChars());
        m->semantic();
    }
    if (global.errors)
        goto had_errors;

    Module::dprogress = 1;
    Module::runDeferredSemantic();

    // Do pass 2 semantic analysis
    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        if (global.params.verbose)
            printf("semantic2 %s\n", m->toChars());
        m->semantic2();
    }
    if (global.errors)
        goto had_errors;

    // Do pass 3 semantic analysis
    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        if (global.params.verbose)
            printf("semantic3 %s\n", m->toChars());
        m->semantic3();
    }
    if (global.errors)
        goto had_errors;

    if (global.params.moduleDeps != NULL)
    {
        gcc_assert(global.params.moduleDepsFile != NULL);

        File deps(global.params.moduleDepsFile);
        OutBuffer* ob = global.params.moduleDeps;
        deps.setbuffer((void*)ob->data, ob->offset);
        deps.writev();
    }

    // Do not attempt to generate output files if errors or warnings occurred
    if (global.errors || global.warnings)
        goto had_errors;

    g.ofile = new ObjectFile();
    if (fonly_arg)
        g.ofile->modules.push(an_output_module);
    else
        g.ofile->modules.append(& modules);
    g.irs = & gen; // needed for FuncDeclaration::toObjFile shouldDefer check

    // Generate output files
    if (global.params.doXGeneration)
        json_generate(&modules);

    for (size_t i = 0; i < modules.dim; i++)
    {
        m = modules.tdata()[i];
        if (fonly_arg && m != an_output_module)
            continue;
        if (global.params.verbose)
            printf("code      %s\n", m->toChars());
        if (! flag_syntax_only)
            m->genobjfile(false);
        if (! global.errors && ! errorcount)
        {
            if (global.params.doDocComments)
                m->gendocfile();
        }
    }

    // better to use input_location.xxx ?
    (*debug_hooks->end_source_file) (input_line);
 had_errors:
    // Add DMD error count to GCC error count to to exit with error status
    errorcount += (global.errors + global.warnings);

    g.ofile->finish();
    an_output_module = 0;

    gcc_d_backend_term();
}

void
d_gcc_dump_source(const char * srcname, const char * ext, unsigned char * data, unsigned len)
{
    // Note: There is a dump_base_name variable, but as long as the all-sources hack is in
    // around, the base name has to be determined here.

    /* construct output name */
    char* base = (char*) alloca(strlen(srcname)+1);
    base = strcpy(base, srcname);
    base = basename(base);

    char* name = (char*) alloca(strlen(base)+strlen(ext)+2);
    name = strcpy(name, base);
    if(strlen(ext)>0){
            name = strcat(name, ".");
            name = strcat(name, ext);
    }

    /* output
     * ignores if the output file exists
     * ignores if the output fails
     */
    FILE* output = fopen(name, "w");
    if(output){
        fwrite(data, 1, len, output);
        fclose(output);
    }

    /* cleanup */
    errno=0;
}

#ifdef D_USE_MAPPED_LOCATION
/* Create a DECL_... node of code CODE, name NAME and data type TYPE.
   LOC is the location of the decl.  */

tree
d_build_decl_loc(location_t loc, tree_code code, tree name, tree type)
{
    tree t;
#if D_GCC_VER >= 45
    t = build_decl(loc, code, name, type);
#else
    t = build_decl(code, name, type);
    DECL_SOURCE_LOCATION(t) = loc;
#endif
    return t;
}
#endif

/* Same as d_build_decl_loc, except location of DECL is unknown.
   This is really for backwards compatibility with old code.  */

tree
d_build_decl(tree_code code, tree name, tree type)
{
    return d_build_decl_loc(UNKNOWN_LOCATION, code, name, type);
}

bool
d_mark_addressable (tree t)
{
  tree x = t;
  while (1)
    switch (TREE_CODE (x))
      {
      case ADDR_EXPR:
      case COMPONENT_REF:
          /* If D had bit fields, we would need to handle that here */
      case ARRAY_REF:
      case REALPART_EXPR:
      case IMAGPART_EXPR:
        x = TREE_OPERAND (x, 0);
        break;
        /* %% C++ prevents {& this} .... */
        /* %% TARGET_EXPR ... */
      case TRUTH_ANDIF_EXPR:
      case TRUTH_ORIF_EXPR:
      case COMPOUND_EXPR:
        x = TREE_OPERAND (x, 1);
        break;

      case COND_EXPR:
        return d_mark_addressable (TREE_OPERAND (x, 1))
          && d_mark_addressable (TREE_OPERAND (x, 2));

      case CONSTRUCTOR:
        TREE_ADDRESSABLE (x) = 1;
        return true;

      case INDIRECT_REF:
          /* %% this was in Java, not sure for D */
        /* We sometimes add a cast *(TYPE*)&FOO to handle type and mode
           incompatibility problems.  Handle this case by marking FOO.  */
        if (TREE_CODE (TREE_OPERAND (x, 0)) == NOP_EXPR
            && TREE_CODE (TREE_OPERAND (TREE_OPERAND (x, 0), 0)) == ADDR_EXPR)
          {
            x = TREE_OPERAND (TREE_OPERAND (x, 0), 0);
            break;
          }
        if (TREE_CODE (TREE_OPERAND (x, 0)) == ADDR_EXPR)
          {
            x = TREE_OPERAND (x, 0);
            break;
          }
        return true;

      case VAR_DECL:
      case CONST_DECL:
      case PARM_DECL:
      case RESULT_DECL:
      case FUNCTION_DECL:
        TREE_ADDRESSABLE (x) = 1;
        /* drops through */
      default:
        return true;
    }

    return 1;
}


/* Mark EXP as read, not just set, for set but not used -Wunused
   warning purposes.  */

void
d_mark_exp_read (tree exp)
{
    switch (TREE_CODE (exp))
    {
        case VAR_DECL:
        case PARM_DECL:
            D_DECL_READ (exp) = 1;
            break;

        case ARRAY_REF:
        case COMPONENT_REF:
        case MODIFY_EXPR:
        case REALPART_EXPR:
        case IMAGPART_EXPR:
        case NOP_EXPR:   
        case CONVERT_EXPR:
        case ADDR_EXPR:
            d_mark_exp_read (TREE_OPERAND (exp, 0));
            break;

        case COMPOUND_EXPR:
            d_mark_exp_read (TREE_OPERAND (exp, 1));
            break;

        default:
            break;
    }
}


tree
d_type_for_mode (enum machine_mode mode, int unsignedp)
{
    // taken from c-common.c
  if (mode == TYPE_MODE (integer_type_node))
    return unsignedp ? unsigned_type_node : integer_type_node;

  if (mode == TYPE_MODE (signed_char_type_node))
    return unsignedp ? unsigned_char_type_node : signed_char_type_node;

  if (mode == TYPE_MODE (short_integer_type_node))
    return unsignedp ? short_unsigned_type_node : short_integer_type_node;

  if (mode == TYPE_MODE (long_integer_type_node))
    return unsignedp ? long_unsigned_type_node : long_integer_type_node;

  if (mode == TYPE_MODE (long_long_integer_type_node))
    return unsignedp ? long_long_unsigned_type_node : long_long_integer_type_node;

  /*%% ?
  if (mode == TYPE_MODE (widest_integer_literal_type_node))
    return unsignedp ? widest_unsigned_literal_type_node
                     : widest_integer_literal_type_node;
  */
  if (mode == QImode)
    return unsignedp ? unsigned_intQI_type_node : intQI_type_node;

  if (mode == HImode)
    return unsignedp ? unsigned_intHI_type_node : intHI_type_node;

  if (mode == SImode)
    return unsignedp ? unsigned_intSI_type_node : intSI_type_node;

  if (mode == DImode)
    return unsignedp ? unsigned_intDI_type_node : intDI_type_node;

#if HOST_BITS_PER_WIDE_INT >= 64
  if (mode == TYPE_MODE (intTI_type_node))
    return unsignedp ? unsigned_intTI_type_node : intTI_type_node;
#endif

  if (mode == TYPE_MODE (float_type_node))
    return float_type_node;

  if (mode == TYPE_MODE (double_type_node))
    return double_type_node;

  if (mode == TYPE_MODE (long_double_type_node))
    return long_double_type_node;

  if (mode == TYPE_MODE (build_pointer_type (char_type_node)))
    return build_pointer_type (char_type_node);

  if (mode == TYPE_MODE (build_pointer_type (integer_type_node)))
    return build_pointer_type (integer_type_node);

  if (COMPLEX_MODE_P (mode))
    {
      enum machine_mode inner_mode;
      tree inner_type;

      if (mode == TYPE_MODE (complex_float_type_node))
        return complex_float_type_node;
      if (mode == TYPE_MODE (complex_double_type_node))
        return complex_double_type_node;
      if (mode == TYPE_MODE (complex_long_double_type_node))
        return complex_long_double_type_node;

      if (mode == TYPE_MODE (complex_integer_type_node) && !unsignedp)
        return complex_integer_type_node;

      inner_mode = (machine_mode) GET_MODE_INNER (mode);
      inner_type = d_type_for_mode (inner_mode, unsignedp);
      if (inner_type != NULL_TREE)
        return build_complex_type (inner_type);
    }
  else if (VECTOR_MODE_P (mode))
    {
      enum machine_mode inner_mode = (machine_mode) GET_MODE_INNER (mode);
      tree inner_type = d_type_for_mode (inner_mode, unsignedp);
      if (inner_type != NULL_TREE)
        return build_vector_type_for_mode (inner_type, mode);
    }

  return 0;
}

tree
d_type_for_size (unsigned bits, int unsignedp)
{
  if (bits == TYPE_PRECISION (integer_type_node))
    return unsignedp ? unsigned_type_node : integer_type_node;

  if (bits == TYPE_PRECISION (signed_char_type_node))
    return unsignedp ? unsigned_char_type_node : signed_char_type_node;

  if (bits == TYPE_PRECISION (short_integer_type_node))
    return unsignedp ? short_unsigned_type_node : short_integer_type_node;

  if (bits == TYPE_PRECISION (long_integer_type_node))
    return unsignedp ? long_unsigned_type_node : long_integer_type_node;

  if (bits == TYPE_PRECISION (long_long_integer_type_node))
    return (unsignedp ? long_long_unsigned_type_node
            : long_long_integer_type_node);
  /* %%?
  if (bits == TYPE_PRECISION (widest_integer_literal_type_node))
    return (unsignedp ? widest_unsigned_literal_type_node
            : widest_integer_literal_type_node);
  */
  if (bits <= TYPE_PRECISION (intQI_type_node))
    return unsignedp ? unsigned_intQI_type_node : intQI_type_node;

  if (bits <= TYPE_PRECISION (intHI_type_node))
    return unsignedp ? unsigned_intHI_type_node : intHI_type_node;

  if (bits <= TYPE_PRECISION (intSI_type_node))
    return unsignedp ? unsigned_intSI_type_node : intSI_type_node;

  if (bits <= TYPE_PRECISION (intDI_type_node))
    return unsignedp ? unsigned_intDI_type_node : intDI_type_node;

  return 0;
}

tree
d_unsigned_type (tree type)
{
  tree type1 = TYPE_MAIN_VARIANT (type);
  if (type1 == signed_char_type_node || type1 == char_type_node)
    return unsigned_char_type_node;
  if (type1 == integer_type_node)
    return unsigned_type_node;
  if (type1 == short_integer_type_node)
    return short_unsigned_type_node;
  if (type1 == long_integer_type_node)
    return long_unsigned_type_node;
  if (type1 == long_long_integer_type_node)
    return long_long_unsigned_type_node;
  /* %%?
  if (type1 == widest_integer_literal_type_node)
    return widest_unsigned_literal_type_node;
  */
#if HOST_BITS_PER_WIDE_INT >= 64
  if (type1 == intTI_type_node)
    return unsigned_intTI_type_node;
#endif
  if (type1 == intDI_type_node)
    return unsigned_intDI_type_node;
  if (type1 == intSI_type_node)
    return unsigned_intSI_type_node;
  if (type1 == intHI_type_node)
    return unsigned_intHI_type_node;
  if (type1 == intQI_type_node)
    return unsigned_intQI_type_node;

  return d_signed_or_unsigned_type (1, type);
}

tree
d_signed_type (tree type)
{
  tree type1 = TYPE_MAIN_VARIANT (type);
  if (type1 == unsigned_char_type_node || type1 == char_type_node)
    return signed_char_type_node;
  if (type1 == unsigned_type_node)
    return integer_type_node;
  if (type1 == short_unsigned_type_node)
    return short_integer_type_node;
  if (type1 == long_unsigned_type_node)
    return long_integer_type_node;
  if (type1 == long_long_unsigned_type_node)
    return long_long_integer_type_node;
  /*
  if (type1 == widest_unsigned_literal_type_node)
    return widest_integer_literal_type_node;
  */
#if HOST_BITS_PER_WIDE_INT >= 64
  if (type1 == unsigned_intTI_type_node)
    return intTI_type_node;
#endif
  if (type1 == unsigned_intDI_type_node)
    return intDI_type_node;
  if (type1 == unsigned_intSI_type_node)
    return intSI_type_node;
  if (type1 == unsigned_intHI_type_node)
    return intHI_type_node;
  if (type1 == unsigned_intQI_type_node)
    return intQI_type_node;

  return d_signed_or_unsigned_type (0, type);
}

tree
d_signed_or_unsigned_type (int unsignedp, tree type)
{
  if (! INTEGRAL_TYPE_P (type)
      || TYPE_UNSIGNED (type) == (unsigned) unsignedp)
    return type;

  if (TYPE_PRECISION (type) == TYPE_PRECISION (signed_char_type_node))
    return unsignedp ? unsigned_char_type_node : signed_char_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (integer_type_node))
    return unsignedp ? unsigned_type_node : integer_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (short_integer_type_node))
    return unsignedp ? short_unsigned_type_node : short_integer_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (long_integer_type_node))
    return unsignedp ? long_unsigned_type_node : long_integer_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (long_long_integer_type_node))
    return (unsignedp ? long_long_unsigned_type_node
            : long_long_integer_type_node);
  /* %%?
  if (TYPE_PRECISION (type) == TYPE_PRECISION (widest_integer_literal_type_node))
    return (unsignedp ? widest_unsigned_literal_type_node
            : widest_integer_literal_type_node);
  */
#if HOST_BITS_PER_WIDE_INT >= 64
  if (TYPE_PRECISION (type) == TYPE_PRECISION (intTI_type_node))
    return unsignedp ? unsigned_intTI_type_node : intTI_type_node;
#endif
  if (TYPE_PRECISION (type) == TYPE_PRECISION (intDI_type_node))
    return unsignedp ? unsigned_intDI_type_node : intDI_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (intSI_type_node))
    return unsignedp ? unsigned_intSI_type_node : intSI_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (intHI_type_node))
    return unsignedp ? unsigned_intHI_type_node : intHI_type_node;
  if (TYPE_PRECISION (type) == TYPE_PRECISION (intQI_type_node))
    return unsignedp ? unsigned_intQI_type_node : intQI_type_node;

  return type;
}

/* Type promotion for variable arguments.  */
tree
d_type_promotes_to (tree type)
{
  /* Almost the same as c_type_promotes_to.  This is needed varargs to work on
     certain targets. */
  if (TYPE_MAIN_VARIANT (type) == float_type_node)
    return double_type_node;

  // not quite the same as... if (c_promoting_integer_type_p (type))
  if (INTEGRAL_TYPE_P (type) &&
      (TYPE_PRECISION (type) < TYPE_PRECISION (integer_type_node)) )
    {
      /* Preserve unsignedness if not really getting any wider.  */
      if (TYPE_UNSIGNED (type)
          && (TYPE_PRECISION (type) == TYPE_PRECISION (integer_type_node)))
        return unsigned_type_node;
      return integer_type_node;
    }

  return type;
}


struct binding_level * current_binding_level;
struct binding_level * global_binding_level;


static binding_level *
alloc_binding_level()
{
    unsigned sz = sizeof (struct binding_level);
#if D_GCC_VER >= 46
    return (struct binding_level *) ggc_alloc_cleared_atomic (sz);
#else
    return (struct binding_level *) ggc_alloc_cleared (sz);
#endif
}

/* The D front-end does not use the 'binding level' system for a symbol table,
   It is only needed to get debugging information for local variables and
   otherwise support the backend. */

void
pushlevel (int /*arg*/)
{
    binding_level * new_level = alloc_binding_level();
    new_level->level_chain = current_binding_level;
    current_binding_level = new_level;
}

tree
poplevel (int keep, int reverse, int routinebody)
{
    binding_level * level = current_binding_level;
    tree block, decls;

    current_binding_level = level->level_chain;
    decls = level->names;
    if (reverse)
        decls = nreverse(decls);

    if (level->this_block)
        block = level->this_block;
    else if (keep || routinebody)
        block = make_node(BLOCK);
    else
        block = NULL_TREE;

    if (block)
    {
        BLOCK_VARS(block) = routinebody ? NULL_TREE : decls;
        BLOCK_SUBBLOCKS(block) = level->blocks;
        // %% need this for when insert_block is called by backend... or make
        // insert_block do it's work elsewere
        // BLOCK_SUBBLOCKS( block ) = level->blocks;
        // %% pascal does: in each subblock, record that this is the superiod..
    }
    /* In each subblock, record that this is its superior. */
    for (tree t = level->blocks; t; t = TREE_CHAIN (t))
        BLOCK_SUPERCONTEXT (t) = block;
    /* Dispose of the block that we just made inside some higher level. */
    if (routinebody)
        DECL_INITIAL (current_function_decl) = block;
    else if (block)
    {
        // Original logic was: If this block was created by this poplevel
        // call and not and earlier set_block, insert it into the parent's
        // list of blocks.  Blocks created with set_block have to be
        // inserted with insert_block.
        //
        // For D, currently always using set_block/insert_block
        if (!level->this_block)
            current_binding_level->blocks = chainon (current_binding_level->blocks, block);
    }
    /* If we did not make a block for the level just exited, any blocks made for inner
       levels (since they cannot be recorded as subblocks in that level) must be
       carried forward so they will later become subblocks of something else. */
    else if (level->blocks)
        current_binding_level->blocks = chainon (current_binding_level->blocks, level->blocks);

    if (block)
    {
        TREE_USED (block) = 1;
        tree vars = copy_list (BLOCK_VARS (block));

        /* Warnings for unused variables.  */
        for (tree t = nreverse(vars); t != NULL_TREE; t = TREE_CHAIN (t))
        {
            gcc_assert(TREE_CODE (t) == VAR_DECL);
            if ((!TREE_USED (t) /*|| !D_DECL_READ(t)*/) //%% TODO
                && !TREE_NO_WARNING (t)
                && DECL_NAME (t)
                && !DECL_ARTIFICIAL (t))
            {
                if (!TREE_USED (t))
                    warning (OPT_Wunused_variable, "unused variable %q+D", t);
                else if (DECL_CONTEXT (t) == current_function_decl)
                {
#if D_GCC_VER >= 46
                    warning_at (DECL_SOURCE_LOCATION (t),
                                OPT_Wunused_but_set_variable, "variable %qD set but not used", t);
#elif D_GCC_VER >= 44
                    warning_at (DECL_SOURCE_LOCATION (t),
                                OPT_Wunused_variable, "variable %qD set but not used", t);
#else
                    warning (OPT_Wunused_variable, "variable %qD set but not used", t);
#endif
                }
            }
        }
    }
    return block;
}

int
global_bindings_p (void)
{
    // This is called by the backend before parsing.  Need to make this do
    // something or lang_hooks.clear_binding_stack (lhd_clear_binding_stack)
    // loops forever.
    return current_binding_level == global_binding_level || ! global_binding_level;
}

void
init_global_binding_level()
{
    current_binding_level = global_binding_level = alloc_binding_level();
}


void
insert_block (tree block)
{
    TREE_USED (block) = 1;
    current_binding_level->blocks = chainon (current_binding_level->blocks, block);
}

void
set_block (tree block)
{
    current_binding_level->this_block = block;
}

tree
pushdecl (tree decl)
{
    // %% Pascal: if not a local external routine decl doesn't consitite nesting

    // %% probably  should be cur_irs->getDeclContext()
    // %% should only be for variables OR, should also use TRANSLATION_UNIT for toplevel..
    if (DECL_CONTEXT(decl) == NULL_TREE)
        DECL_CONTEXT(decl) = current_function_decl; // could be NULL_TREE (top level) .. hmm. // hm.m.

    /* Put decls on list in reverse order. We will reverse them later if necessary. */
    TREE_CHAIN (decl) = current_binding_level->names;
    current_binding_level->names = decl;
    if (!TREE_CHAIN (decl))
        current_binding_level->names_end = decl;
    return decl;
}

/* pushdecl_top_level is only for building with Apple GCC. */
extern "C" tree
pushdecl_top_level (tree x)
{
  tree t;
  struct binding_level *b = current_binding_level;

  current_binding_level = global_binding_level;
  t = pushdecl (x);
  current_binding_level = b;
  return t;
}


void
set_decl_binding_chain(tree decl_chain)
{
    gcc_assert(current_binding_level);
    current_binding_level->names = decl_chain;
}


// Supports dbx and stabs
tree
getdecls ()
{
    if (current_binding_level)
        return current_binding_level->names;
    else
        return NULL_TREE;
}


/* Tree code classes. */

#undef DEFTREECODE
#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) TYPE,

#if D_GCC_VER < 44

const enum tree_code_class
tree_code_type[] = {
#include "tree.def"
 tcc_exceptional,
#include "d/d-tree.def"
};
#endif

#undef DEFTREECODE

#if D_GCC_VER < 44

/* Table indexed by tree code giving number of expression
 operands beyond the fixed part of the node structure.
 Not used for types or decls. */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) LENGTH,

const unsigned char tree_code_length[] = {
#include "tree.def"
 0,
#include "d/d-tree.def"
};
#undef DEFTREECODE

/* Names of tree components.
 Used for printing out the tree and error messages. */
#define DEFTREECODE(SYM, NAME, TYPE, LEN) NAME,

const char *const tree_code_name[] = {
#include "tree.def"
 "@@dummy",
#include "d/d-tree.def"
};
#undef DEFTREECODE

#endif

#if D_GCC_VER < 43

static void
d_expand_function(tree fndecl)
{
  if (!DECL_INITIAL (fndecl)
      || DECL_INITIAL (fndecl) == error_mark_node)
    return;

  bool save_flag = flag_omit_frame_pointer;
  flag_omit_frame_pointer = gen.originalOmitFramePointer ||
      D_DECL_NO_FRAME_POINTER( fndecl );

  tree_rest_of_compilation (fndecl);

  flag_omit_frame_pointer = save_flag;

  if (DECL_STATIC_CONSTRUCTOR (fndecl)
      && targetm.have_ctors_dtors)
    targetm.asm_out.constructor (XEXP (DECL_RTL (fndecl), 0),
                                 DEFAULT_INIT_PRIORITY);
  if (DECL_STATIC_DESTRUCTOR (fndecl)
      && targetm.have_ctors_dtors)
    targetm.asm_out.destructor (XEXP (DECL_RTL (fndecl), 0),
                                DEFAULT_INIT_PRIORITY);
}

#endif

static int
d_types_compatible_p (tree t1, tree t2)
{
    /* Is compatible if types are equivalent */
    if (TYPE_MAIN_VARIANT (t1) == TYPE_MAIN_VARIANT (t2))
        return 1;

    /* Is compatible if we are dealing with C <-> D va_list nodes */
    if ((t1 == d_va_list_type_node && t2 == va_list_type_node)
        || (t2 == d_va_list_type_node && t1 == va_list_type_node))
        return 1;

    /* Is compatible if aggregates are same type or share the same
       attributes. The frontend should have already ensured that types
       aren't wildly different anyway... */
    if (AGGREGATE_TYPE_P (t1) && AGGREGATE_TYPE_P (t2)
        && TREE_CODE (t1) == TREE_CODE (t2))
    {
        if (TREE_CODE (t1) == ARRAY_TYPE)
            return (TREE_TYPE (t1) == TREE_TYPE (t2));

        return (TYPE_ATTRIBUTES (t1) == TYPE_ATTRIBUTES (t2));
    }
    /* else */
    return 0;
}

static void
d_finish_incomplete_decl (tree decl)
{
    if (TREE_CODE (decl) == VAR_DECL)
    {   /* D allows zero-length declarations.  Such a declaration ends up with
           DECL_SIZE(t) == NULL_TREE which is what the backend function
           assembler_variable checks.  This could change in later versions...

           Maybe all of these variables should be aliased to one symbol... */
        if (DECL_SIZE (decl) == 0)
        {
            DECL_SIZE (decl) = bitsize_zero_node;
            DECL_SIZE_UNIT (decl) = size_zero_node;
        }
    }
}

#if V2

/* DMD 2 makes a parameter delclaration's type 'const(T)' if the
   parameter is a simple STCin or STCconst.  The TypeFunction's
   Argument's type stays unqualified, however.

   This mismatch causes a problem with optimization and inlining.  For
   RECORD_TYPE arguments, failure will occur in (setup_one_parameter
   -> fold_convert).  d_types_compatible_p hacks lead to failures in
   the sra pass.

   Fortunately, the middle end provides a simple workaround by using
   this hook.
*/

tree
d_convert_parm_for_inlining  (tree parm, tree value, tree fndecl, int argnum)
{
    if (!value)
        return value;

    if (TYPE_ARG_TYPES(TREE_TYPE(fndecl))
            && (TYPE_MAIN_VARIANT(TREE_TYPE(parm))
                == TYPE_MAIN_VARIANT(TREE_TYPE(value))))
        return value;

    if (TREE_TYPE(parm) != TREE_TYPE(value))
        return build1(NOP_EXPR, TREE_TYPE(parm), value);

    return value;
}
#endif


struct lang_type *
build_d_type_lang_specific(Type * t)
{
    struct lang_type * l;
    unsigned sz = sizeof(struct lang_type);
#if D_GCC_VER >= 46
    l = (struct lang_type *) ggc_alloc_cleared_atomic(sz);
#else
    l = (struct lang_type *) ggc_alloc_cleared(sz);
#endif
    l->d_type = t;
    l->c_type = t->ctype;
    return l;
}

struct lang_decl *
build_d_decl_lang_specific(Declaration * d)
{
    struct lang_decl * l;
    unsigned sz = sizeof(struct lang_decl);
#if D_GCC_VER >= 46
    l = (struct lang_decl *) ggc_alloc_cleared_atomic(sz);
#else
    l = (struct lang_decl *) ggc_alloc_cleared(sz);
#endif
    l->d_decl = d;
    return l;
}


// This preserves tree we create from the garbage collector.
tree d_keep_list = NULL_TREE;

void
dkeep(tree t)
{
    d_keep_list = tree_cons(NULL_TREE, t, d_keep_list);
}

#if D_GCC_VER >= 45
tree d_eh_personality_decl;

/* Return the GDC personality function decl.  */
static tree
d_eh_personality (void)
{
    if (!d_eh_personality_decl)
    {
#if D_GCC_VER >= 46
        d_eh_personality_decl
            = build_personality_function ("gdc");
#else
        d_eh_personality_decl
            = build_personality_function (USING_SJLJ_EXCEPTIONS
                                          ? "__gdc_personality_sj0"
                                          : "__gdc_personality_v0");
#endif
    }
    return d_eh_personality_decl;
}
#endif

static tree
d_build_eh_type_type(tree type)
{
    if (TREE_CODE(type) == ADDR_EXPR)
        return type;    // %% why???

    TypeClass * d_type = (TypeClass *) gen.getDType(type);
    gcc_assert(d_type);
    d_type = (TypeClass *) d_type->toBasetype();
    gcc_assert(d_type->ty == Tclass);
    return gen.addressOf(d_type->sym->toSymbol()->Stree);
}

void
d_init_exceptions(void)
{
#if D_GCC_VER >= 45
    // Handled with langhooks eh_personality and eh_runtime_type
#else
    eh_personality_libfunc = init_one_libfunc(USING_SJLJ_EXCEPTIONS
            ? "__gdc_personality_sj0" : "__gdc_personality_v0");
    default_init_unwind_resume_libfunc ();
    lang_eh_runtime_type = d_build_eh_type_type;
#endif
    using_eh_for_cleanups ();
    // lang_protect_cleanup_actions = ...; // no need? ... probably needed for autos
}

#if D_GCC_VER < 45
const
#endif
struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;
