// d-lang.cc -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

/*
  d-lang.cc: implementation of back-end callbacks and data structures
*/

#include "d-system.h"
#include "options.h"
#include "cppdefault.h"
#include "debug.h"

#include "d-lang.h"
#include "d-codegen.h"
#include "d-confdefs.h"

#include "mars.h"
#include "mtype.h"
#include "cond.h"
#include "id.h"
#include "json.h"
#include "module.h"
#include "root.h"
#include "async.h"
#include "dfrontend/target.h"

static tree d_handle_noinline_attribute (tree *, tree, tree, int, bool *);
static tree d_handle_forceinline_attribute (tree *, tree, tree, int, bool *);
static tree d_handle_flatten_attribute (tree *, tree, tree, int, bool *);
static tree d_handle_target_attribute (tree *, tree, tree, int, bool *);


static char lang_name[6] = "GNU D";

const struct attribute_spec d_attribute_table[] = 
{
    { "noinline",               0, 0, true,  false, false,
				d_handle_noinline_attribute, false },
    { "forceinline",            0, 0, true,  false, false,
				d_handle_forceinline_attribute, false },
    { "flatten",                0, 0, true,  false, false,
				d_handle_flatten_attribute, false },
    { "target",                 1, -1, true, false, false,
				d_handle_target_attribute, false },
    { NULL,                     0, 0, false, false, false, NULL, false }
};

/* Lang Hooks */
#undef LANG_HOOKS_NAME
#undef LANG_HOOKS_INIT
#undef LANG_HOOKS_INIT_TS
#undef LANG_HOOKS_INIT_OPTIONS
#undef LANG_HOOKS_INIT_OPTIONS_STRUCT
#undef LANG_HOOKS_INITIALIZE_DIAGNOSTICS
#undef LANG_HOOKS_OPTION_LANG_MASK
#undef LANG_HOOKS_HANDLE_OPTION
#undef LANG_HOOKS_POST_OPTIONS
#undef LANG_HOOKS_PARSE_FILE
#undef LANG_HOOKS_COMMON_ATTRIBUTE_TABLE
#undef LANG_HOOKS_ATTRIBUTE_TABLE
#undef LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE
#undef LANG_HOOKS_TYPES_COMPATIBLE_P
#undef LANG_HOOKS_BUILTIN_FUNCTION
#undef LANG_HOOKS_BUILTIN_FUNCTION_EXT_SCOPE
#undef LANG_HOOKS_REGISTER_BUILTIN_TYPE
#undef LANG_HOOKS_FINISH_INCOMPLETE_DECL
#undef LANG_HOOKS_GIMPLIFY_EXPR
#undef LANG_HOOKS_EH_PERSONALITY
#undef LANG_HOOKS_EH_RUNTIME_TYPE

#define LANG_HOOKS_NAME				lang_name
#define LANG_HOOKS_INIT				d_init
#define LANG_HOOKS_INIT_TS			d_init_ts
#define LANG_HOOKS_INIT_OPTIONS			d_init_options
#define LANG_HOOKS_INIT_OPTIONS_STRUCT		d_init_options_struct
#define LANG_HOOKS_INITIALIZE_DIAGNOSTICS	d_initialize_diagnostics
#define LANG_HOOKS_OPTION_LANG_MASK		d_option_lang_mask
#define LANG_HOOKS_HANDLE_OPTION		d_handle_option
#define LANG_HOOKS_POST_OPTIONS			d_post_options
#define LANG_HOOKS_PARSE_FILE			d_parse_file
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE       d_builtins_attribute_table
#define LANG_HOOKS_ATTRIBUTE_TABLE              d_attribute_table
#define LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE	d_format_attribute_table
#define LANG_HOOKS_TYPES_COMPATIBLE_P		d_types_compatible_p
#define LANG_HOOKS_BUILTIN_FUNCTION		d_builtin_function
#define LANG_HOOKS_BUILTIN_FUNCTION_EXT_SCOPE	d_builtin_function
#define LANG_HOOKS_REGISTER_BUILTIN_TYPE	d_register_builtin_type
#define LANG_HOOKS_FINISH_INCOMPLETE_DECL	d_finish_incomplete_decl
#define LANG_HOOKS_GIMPLIFY_EXPR		d_gimplify_expr
#define LANG_HOOKS_EH_PERSONALITY		d_eh_personality
#define LANG_HOOKS_EH_RUNTIME_TYPE		d_build_eh_type_type

/* Lang Hooks for decls */
#undef LANG_HOOKS_WRITE_GLOBALS
#define LANG_HOOKS_WRITE_GLOBALS		d_write_global_declarations

/* Lang Hooks for types */
#undef LANG_HOOKS_TYPE_FOR_MODE
#undef LANG_HOOKS_TYPE_FOR_SIZE
#undef LANG_HOOKS_TYPE_PROMOTES_TO

#define LANG_HOOKS_TYPE_FOR_MODE		d_type_for_mode
#define LANG_HOOKS_TYPE_FOR_SIZE		d_type_for_size
#define LANG_HOOKS_TYPE_PROMOTES_TO		d_type_promotes_to


static const char *fonly_arg;

/* List of modules being compiled.  */
Modules output_modules;

/* Zero disables all standard directories for headers.  */
static bool std_inc = true;

/* Common initialization before calling option handlers.  */
static void
d_init_options (unsigned int, struct cl_decoded_option *decoded_options)
{
  // Set default values
  global.params.argv0 = xstrdup (decoded_options[0].arg);
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
  global.params.useDeprecated = 2;
  global.params.betterC = 0;

  global.params.linkswitches = new Strings();
  global.params.libfiles = new Strings();
  global.params.objfiles = new Strings();
  global.params.ddocfiles = new Strings();

  global.params.imppath = new Strings();
  global.params.fileImppath = new Strings();

  // extra D-specific options
  flag_emit_templates = TEnormal;
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
  opts->x_flag_bounds_check = global.params.useArrayBounds;

  // Honour left to right code evaluation.
  opts->x_flag_evaluation_order = 1;

  // Default to using strict aliasing.
  opts->x_flag_strict_aliasing = 1;
}

static void
d_initialize_diagnostics (diagnostic_context *context)
{
  // We don't need any of these in error messages.
  context->show_caret = false;
  context->show_option_requested = false;
  context->show_column = false;
}

/* Return language mask for option parsing.  */
static unsigned int
d_option_lang_mask (void)
{
  return CL_D;
}

static void
d_add_builtin_version(const char* ident)
{
  if (strcmp (ident, "linux") == 0)
    global.params.isLinux = 1;
  else if (strcmp (ident, "OSX") == 0)
    global.params.isOSX = 1;
  else if (strcmp (ident, "Windows") == 0)
    global.params.isWindows = 1;
  else if (strcmp (ident, "FreeBSD") == 0)
    global.params.isFreeBSD = 1;
  else if (strcmp (ident, "OpenBSD") == 0)
    global.params.isOpenBSD = 1;
  else if (strcmp (ident, "Solaris") == 0)
    global.params.isSolaris = 1;
  else if (strcmp (ident, "X86_64") == 0)
    global.params.is64bit = 1;

  VersionCondition::addPredefinedGlobalIdent (ident);
}

static bool
d_init (void)
{
  if(POINTER_SIZE == 64)
    global.params.isLP64 = 1;

  Type::init();
  Id::initialize();
  Module::init();
  initPrecedence();

  d_backend_init();

  longdouble::init();
  Target::init();
  Port::init();

#ifndef TARGET_CPU_D_BUILTINS
# define TARGET_CPU_D_BUILTINS()
#endif

#ifndef TARGET_OS_D_BUILTINS
# define TARGET_OS_D_BUILTINS()
#endif

# define builtin_define(TXT) d_add_builtin_version(TXT)

  TARGET_CPU_D_BUILTINS();
  TARGET_OS_D_BUILTINS();

  VersionCondition::addPredefinedGlobalIdent ("GNU");
  VersionCondition::addPredefinedGlobalIdent ("D_Version2");

  if (BYTES_BIG_ENDIAN)
    VersionCondition::addPredefinedGlobalIdent ("BigEndian");
  else
    VersionCondition::addPredefinedGlobalIdent ("LittleEndian");

  if (targetm_common.except_unwind_info (&global_options) == UI_SJLJ)
    VersionCondition::addPredefinedGlobalIdent ("GNU_SjLj_Exceptions");

#ifdef STACK_GROWS_DOWNWARD
  VersionCondition::addPredefinedGlobalIdent ("GNU_StackGrowsDown");
#endif

  /* Should define this anyway to set us apart from the competition. */
  VersionCondition::addPredefinedGlobalIdent ("GNU_InlineAsm");

  /* LP64 only means 64bit pointers in D. */
  if (global.params.isLP64)
    VersionCondition::addPredefinedGlobalIdent ("D_LP64");

  /* Setting global.params.cov forces module info generation which is
     not needed for thee GCC coverage implementation.  Instead, just
     test flag_test_coverage while leaving global.params.cov unset. */
  //if (global.params.cov)
  if (flag_test_coverage)
    VersionCondition::addPredefinedGlobalIdent ("D_Coverage");
  if (flag_pic)
    VersionCondition::addPredefinedGlobalIdent ("D_PIC");
  if (global.params.doDocComments)
    VersionCondition::addPredefinedGlobalIdent ("D_Ddoc");
  if (global.params.useUnitTests)
    VersionCondition::addPredefinedGlobalIdent ("unittest");
  if (global.params.useAssert)
    VersionCondition::addPredefinedGlobalIdent("assert");
  if (global.params.noboundscheck)
    VersionCondition::addPredefinedGlobalIdent("D_NoBoundsChecks");

  VersionCondition::addPredefinedGlobalIdent ("all");

  /* Insert all library-configured identifiers and import paths.  */
  add_import_paths(std_inc);
  add_phobos_versyms();

  return 1;
}

void
d_init_ts (void)
{
  MARK_TS_TYPED (IASM_EXPR);
  MARK_TS_TYPED (FLOAT_MOD_EXPR);
  MARK_TS_TYPED (UNSIGNED_RSHIFT_EXPR);
}


static bool
parse_int (const char *arg, int *value_ret)
{
  /* Common case of a single digit.  */
  if (arg[1] == '\0')
    *value_ret = arg[0] - '0';
  else
    {
      HOST_WIDE_INT v = 0;
      unsigned int base = 10, c = 0;
      int overflow = 0;
      for (const char *p = arg; *p != '\0'; p++)
	{
	  c = *p;

	  if (ISDIGIT (c))
	    c = hex_value (c);
	  else
	    return false;

	  v = v * base + c;
	  overflow |= (v > INT_MAX);
	}

      if (overflow)
	return false;

      *value_ret = v;
    }

  return true;
}

static bool
d_handle_option (size_t scode, const char *arg, int value,
		 int kind ATTRIBUTE_UNUSED,
		 location_t loc ATTRIBUTE_UNUSED,
		 const struct cl_option_handlers *handlers ATTRIBUTE_UNUSED)
{
  enum opt_code code = (enum opt_code) scode;
  bool result = true;
  int level;

  switch (code)
    {
    case OPT_fassert:
      global.params.useAssert = value;
      break;

    case OPT_fbounds_check:
      global.params.noboundscheck = !value;
      break;

    case OPT_fdebug:
      global.params.debuglevel = value ? 1 : 0;
      break;

    case OPT_fdebug_:
      if (ISDIGIT (arg[0]))
	{
	  if (!parse_int (arg, &level))
	    goto Lerror_d;
	  DebugCondition::setGlobalLevel (level);
	}
      else if (Lexer::isValidIdentifier (CONST_CAST (char *, arg)))
	DebugCondition::addGlobalIdent (xstrdup (arg));
      else
	{
    Lerror_d:
	  error ("bad argument for -fdebug");
	}
      break;

    case OPT_fdeps_:
      global.params.moduleDepsFile = xstrdup (arg);
      if (!global.params.moduleDepsFile[0])
	error ("bad argument for -fdeps");
      global.params.moduleDeps = new OutBuffer;
      break;

    case OPT_fdoc:
      global.params.doDocComments = value;
      break;

    case OPT_fdoc_dir_:
      global.params.doDocComments = 1;
      global.params.docdir = xstrdup (arg);
      break;

    case OPT_fdoc_file_:
      global.params.doDocComments = 1;
      global.params.docname = xstrdup (arg);
      break;

    case OPT_fdoc_inc_:
      global.params.ddocfiles->push (xstrdup (arg));
      break;

    case OPT_fd_verbose:
      global.params.verbose = value;
      break;

    case OPT_fd_vtls:
      global.params.vtls = value;
      break;

    case OPT_femit_templates:
      flag_emit_templates = value ? TEprivate : TEnone;
      break;

    case OPT_femit_moduleinfo:
      global.params.betterC = !value;
      break;

    case OPT_fignore_unknown_pragmas:
      global.params.ignoreUnsupportedPragmas = value;
      break;

    case OPT_fin:
      global.params.useIn = value;
      break;

    case OPT_fintfc:
      global.params.doHdrGeneration = value;
      break;

    case OPT_fintfc_dir_:
      global.params.doHdrGeneration = 1;
      global.params.hdrdir = xstrdup (arg);
      break;

    case OPT_fintfc_file_:
      global.params.doHdrGeneration = 1;
      global.params.hdrname = xstrdup (arg);
      break;

    case OPT_finvariants:
      global.params.useInvariants = value;
      break;

    case OPT_fmake_deps_:
      global.params.makeDeps = new OutBuffer;
      global.params.makeDepsStyle = 1;
      global.params.makeDepsFile = xstrdup (arg);
      if (!global.params.makeDepsFile[0])
	error ("bad argument for -fmake-deps");
      break;

    case OPT_fmake_mdeps_:
      global.params.makeDeps = new OutBuffer;
      global.params.makeDepsStyle = 2;
      global.params.makeDepsFile = xstrdup (arg);
      if (!global.params.makeDepsFile[0])
	error ("bad argument for -fmake-deps");
      break;

    case OPT_fonly_:
      fonly_arg = xstrdup (arg);
      break;

    case OPT_fout:
      global.params.useOut = value;
      break;

    case OPT_fproperty:
      global.params.enforcePropertySyntax = value;
      break;

    case OPT_frelease:
      global.params.useInvariants = !value;
      global.params.useIn = !value;
      global.params.useOut = !value;
      global.params.useAssert = !value;
      // release mode doesn't turn off bounds checking for safe functions.
      global.params.useArrayBounds = !value ? 2 : 1;
      flag_bounds_check = !value;
      global.params.useSwitchError = !value;
      break;

    case OPT_funittest:
      global.params.useUnitTests = value;
      break;

    case OPT_fversion_:
      if (ISDIGIT (arg[0]))
	{
	  if (!parse_int (arg, &level))
	    goto Lerror_v;
	  VersionCondition::setGlobalLevel (level);
	}
      else if (Lexer::isValidIdentifier (CONST_CAST (char *, arg)))
	VersionCondition::addGlobalIdent (xstrdup (arg));
      else
	{
    Lerror_v:
	  error ("bad argument for -fversion");
	}
      break;

    case OPT_fXf_:
      global.params.doXGeneration = 1;
      global.params.xfilename = xstrdup (arg);
      break;

    case OPT_imultilib:
      multilib_dir = xstrdup (arg);
      break;

    case OPT_iprefix:
      iprefix = xstrdup (arg);
      break;

    case OPT_I:
      global.params.imppath->push (xstrdup (arg)); // %% not sure if we can keep the arg or not
      break;

    case OPT_J:
      global.params.fileImppath->push (xstrdup (arg));
      break;

    case OPT_nostdinc:
      std_inc = false;
      break;

    case OPT_Wall:
      if (value)
	global.params.warnings = 2;
      break;

    case OPT_Wdeprecated:
      global.params.useDeprecated = value ? 2 : 1;
      break;

    case OPT_Werror:
      if (value)
	global.params.warnings = 1;
      break;

    default:
      break;
    }

  return result;
}

bool
d_post_options (const char ** fn)
{
  // Canonicalize the input filename.
  if (in_fnames == NULL)
    {
      in_fnames = XNEWVEC (const char *, 1);
      in_fnames[0] = "";
    }
  else if (strcmp (in_fnames[0], "-") == 0)
    in_fnames[0] = "";

  // The front end considers the first input file to be the main one.
  if (num_in_fnames)
    *fn = in_fnames[0];

  /* If we are given more than one input file, we must use
     unit-at-a-time mode.  */
  if (num_in_fnames > 1)
    flag_unit_at_a_time = 1;

  /* Array bounds checking. */
  if (global.params.noboundscheck)
    flag_bounds_check = global.params.useArrayBounds = 0;

  /* Error about use of deprecated features. */
  if (global.params.useDeprecated == 2 && global.params.warnings == 1)
    global.params.useDeprecated = 0;

  if (flag_excess_precision_cmdline == EXCESS_PRECISION_DEFAULT)
    flag_excess_precision_cmdline = EXCESS_PRECISION_STANDARD;

  return false;
}

/* wrapup_global_declaration needs to be called or functions will not
   be emitted. */
static Array globalDeclarations;

void
d_add_global_declaration (tree decl)
{
  globalDeclarations.push (decl);
}

static void
d_write_global_declarations (void)
{
  tree *vec = (tree *) globalDeclarations.data;

  /* Complete all generated thunks. */
  cgraph_process_same_body_aliases();

  /* Process all file scopes in this compilation, and the external_scope,
     through wrapup_global_declarations.  */
  wrapup_global_declarations (vec, globalDeclarations.dim);

  /* We're done parsing; proceed to optimize and emit assembly. */
  if (!global.errors && !errorcount)
    finalize_compilation_unit();

  /* Now, issue warnings about static, but not defined, functions.  */
  check_global_declarations (vec, globalDeclarations.dim);

  /* After cgraph has had a chance to emit everything that's going to
     be emitted, output debug information for globals.  */
  emit_debug_global_declarations (vec, globalDeclarations.dim);
}


/* Gimplification of expression trees.  */
int
d_gimplify_expr (tree *expr_p, gimple_seq *pre_p ATTRIBUTE_UNUSED,
		 gimple_seq *post_p ATTRIBUTE_UNUSED)
{
  enum tree_code code = TREE_CODE (*expr_p);
  switch (code)
    {
    case INIT_EXPR:
    case MODIFY_EXPR:
	{
	  // If the back end isn't clever enough to know that the lhs and rhs
	  // types are the same, add an explicit conversion.
	  tree op0 = TREE_OPERAND (*expr_p, 0);
	  tree op1 = TREE_OPERAND (*expr_p, 1);

	  if (!error_mark_p (op0) && !error_mark_p (op1)
	      && (AGGREGATE_TYPE_P (TREE_TYPE (op0))
		  || AGGREGATE_TYPE_P (TREE_TYPE (op1)))
	      && !useless_type_conversion_p (TREE_TYPE (op1), TREE_TYPE (op0)))
	    {
	      TREE_OPERAND (*expr_p, 1) = build1 (VIEW_CONVERT_EXPR,
						  TREE_TYPE (op0), op1);
	    }
	  return GS_OK;
	}

    case UNSIGNED_RSHIFT_EXPR:
	{
	  // Convert op0 to an unsigned type.
	  tree op0 = TREE_OPERAND (*expr_p, 0);
	  tree op1 = TREE_OPERAND (*expr_p, 1);

	  tree unstype = d_unsigned_type (TREE_TYPE (op0));

	  *expr_p = convert (TREE_TYPE (*expr_p),
			     build2 (RSHIFT_EXPR, unstype,
	 			     convert (unstype, op0), op1));
	  return GS_UNHANDLED;
	}

    case FLOAT_MOD_EXPR:
    case IASM_EXPR:
      gcc_unreachable();

    default:
      return GS_UNHANDLED;
    }
}


static Module *output_module = NULL;

Module *
d_gcc_get_output_module (void)
{
  return output_module;
}

static void
nametype (tree type, const char *name)
{
  tree ident = get_identifier (name);
  tree decl = build_decl (UNKNOWN_LOCATION, TYPE_DECL, ident, type);
  TYPE_NAME (type) = decl;
  rest_of_decl_compilation (decl, 1, 0);
}

static void
nametype (Type *t)
{
  nametype (t->toCtype(), t->toChars());
}

static void
deps_write (Module *m)
{
  OutBuffer *ob = global.params.makeDeps;
  size_t size, column = 0, colmax = 72;
  FileName *fn;

  // Write out object name.
  fn = m->objfile->name;
  size = fn->len();
  ob->writestring (fn->str);
  column = size;

  ob->writestring (": ");
  column += 2;

  // First dependency is source file for module.
  fn = m->srcfile->name;
  size = fn->len();
  ob->writestring (fn->str);
  column += size;

  // Write out file dependencies.
  for (size_t i = 0; i < m->aimports.dim; i++)
    {
      Module *mi = m->aimports[i];

      // Ignore self references.
      if (mi == m)
	continue;

      if (global.params.makeDepsStyle == 2)
	{
	  // Don't emit system modules. This includes core.*, std.*, gcc.* and object.
	  ModuleDeclaration *md = mi->md;

	  if (md && md->packages)
	    {
	      if (strcmp ((md->packages->tdata()[0])->string, "core") == 0)
		continue;
	      if (strcmp ((md->packages->tdata()[0])->string, "std") == 0)
		continue;
	      if (strcmp ((md->packages->tdata()[0])->string, "gcc") == 0)
		continue;
	    }
	  else if (md && md->id)
	    {
	      if (strcmp (md->id->string, "object") == 0 && md->packages == NULL)
		continue;
	    }
	}

      // All checks done, write out file path/name.
      fn = mi->srcfile->name;
      size = fn->len();
      column += size;
      if (column > colmax)
	{
	  ob->writestring (" \\\n ");
	  column = 1 + size;
	}
      else
	{
	  ob->writestring (" ");
	  column++;
	}
      ob->writestring (fn->str);
    }
  ob->writestring ("\n");
}


// Binary search for P in TAB between the range 0 to HIGH.

int binary(const char *p , const char **tab, int high)
{
    int low = 0;
    do
    {
        int pos = (low + high) / 2;
        int cmp = strcmp(p, tab[pos]);
        if (! cmp)
            return pos;
        else if (cmp < 0)
            high = pos;
        else
            low = pos + 1;
    } while (low != high);

    return -1;
}

void
d_parse_file (void)
{
  if (global.params.verbose)
    {
      fprintf (stdmsg, "binary    %s\n", global.params.argv0);
      fprintf (stdmsg, "version   %s\n", global.version);
    }

  if (global.params.useUnitTests)
    global.params.useAssert = 1;

  global.params.symdebug = write_symbols != NO_DEBUG;
  //global.params.useInline = flag_inline_functions;
  global.params.obj = !flag_syntax_only;
  global.params.pic = flag_pic != 0; // Has no effect yet.

  // better to use input_location.xxx ?
  (*debug_hooks->start_source_file) (input_line, main_input_filename);

  for (TY ty = (TY) 0; ty < TMAX; ty = (TY) (ty + 1))
    {
      if (Type::basic[ty] && ty != Terror)
	nametype (Type::basic[ty]);
    }

  // Create Modules
  Modules modules;
  modules.reserve (num_in_fnames);
  AsyncRead *aw = NULL;
  Module *m = NULL;

  if (!main_input_filename || !main_input_filename[0])
    {
      ::error ("input file name required; cannot use stdin");
      goto had_errors;
    }

  if (fonly_arg)
    {
      /* In this mode, the first file name is supposed to be
	 a duplicate of one of the input file. */
      if (strcmp (fonly_arg, main_input_filename))
	::error ("-fonly= argument is different from main input file name");
      if (strcmp (fonly_arg, in_fnames[0]))
	::error ("-fonly= argument is different from first input file name");
    }

  for (size_t i = 0; i < num_in_fnames; i++)
    {
      //fprintf (stderr, "fn %d = %s\n", i, in_fnames[i]);
      char *fname = xstrdup (in_fnames[i]);

      // Strip path
      char *p = CONST_CAST (char *, FileName::name (fname));
      const char *ext = FileName::ext (p);
      char *name;

      if (ext)
	{
	  // Skip onto '.'
	  ext--;
	  gcc_assert (*ext == '.');
	  name = (char *) xmalloc ((ext - p) + 1);
	  memcpy (name, p, ext - p);
	  // Strip extension
	  name[ext - p] = 0;

	  if (name[0] == 0
	      || strcmp (name, "..") == 0
	      || strcmp (name, ".") == 0)
	    {
	Linvalid:
	      ::error ("invalid file name '%s'", fname);
	      goto had_errors;
	    }
	}
      else
	{
	  name = p;
	  if (!*name)
	    goto Linvalid;
	}

      // At this point, name is the D source file name stripped of
      // its path and extension.
      Identifier *id = Lexer::idPool (name);
      m = new Module (fname, id, global.params.doDocComments, global.params.doHdrGeneration);
      modules.push (m);

      if (!strcmp (in_fnames[i], main_input_filename))
	output_module = m;
    }

  // Current_module shouldn't have any implications before genobjfile...
  // but it does.  We need to know what module in which to insert
  // TemplateInstances during the semantic pass.  In order for
  // -femit-templates to work, template instances must be emitted
  // in every translation unit.  To do this, the TemplateInstaceS have to
  // have toObjFile called in the module being compiled.
  // TemplateInstance puts itself somwhere during ::semantic, thus it has
  // to know the current module.

  gcc_assert (output_module);

  // Read files
  aw = AsyncRead::create (modules.dim);
  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      aw->addFile (m->srcfile);
    }
  aw->start();

  // Parse files
  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      if (global.params.verbose)
	fprintf (stdmsg, "parse     %s\n", m->toChars());
      if (!Module::rootModule)
	Module::rootModule = m;
      m->importedFrom = m;
      if (aw->read (i))
	{
	  error ("cannot read file %s", m->srcfile->name->toChars());
	  goto had_errors;
	}
      m->parse();
      d_gcc_magic_module (m);
      if (m->isDocFile)
	{
	  m->gendocfile();
	  // Remove m from list of modules
	  modules.remove (i);
	  i--;
	}
    }
  AsyncRead::dispose (aw);

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
	  m = modules[i];
	  if (fonly_arg && m != output_module)
	    continue;
	  if (global.params.verbose)
	    fprintf (stdmsg, "import    %s\n", m->toChars());
	  m->genhdrfile();
	}
    }

  if (global.errors)
    goto had_errors;

  // load all unconditional imports for better symbol resolving
  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      if (global.params.verbose)
	fprintf (stdmsg, "importall %s\n", m->toChars());
      m->importAll (0);
    }

  if (global.errors)
    goto had_errors;

  // Do semantic analysis
  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      if (global.params.verbose)
	fprintf (stdmsg, "semantic  %s\n", m->toChars());
      m->semantic();
    }

  if (global.errors)
    goto had_errors;

  Module::dprogress = 1;
  Module::runDeferredSemantic();

  // Do pass 2 semantic analysis
  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      if (global.params.verbose)
	fprintf (stdmsg, "semantic2 %s\n", m->toChars());
      m->semantic2();
    }

  if (global.errors)
    goto had_errors;

  // Do pass 3 semantic analysis
  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      if (global.params.verbose)
	fprintf (stdmsg, "semantic3 %s\n", m->toChars());
      m->semantic3();
    }

  if (global.errors)
    goto had_errors;

  if (global.params.moduleDeps != NULL)
    {
      gcc_assert (global.params.moduleDepsFile != NULL);

      File deps (global.params.moduleDepsFile);
      OutBuffer *ob = global.params.moduleDeps;
      deps.setbuffer ((void *) ob->data, ob->offset);
      deps.writev();
    }

  if (global.params.makeDeps != NULL)
    {
      for (size_t i = 0; i < modules.dim; i++)
	{
	  m = modules[i];
	  deps_write (m);
	}


      OutBuffer *ob = global.params.makeDeps;
      if (global.params.makeDepsFile == NULL)
	printf ("%s", (char *) ob->data);
      else
	{
	  File deps (global.params.makeDepsFile);
	  deps.setbuffer ((void *) ob->data, ob->offset);
	  deps.writev();
	}
    }

  // Do not attempt to generate output files if errors or warnings occurred
  if (global.errors || global.warnings)
    goto had_errors;

  if (fonly_arg)
    output_modules.push (output_module);
  else
    output_modules.append (&modules);

  cirstate = new IRState();

  // Generate output files
  if (global.params.doXGeneration)
    {
      OutBuffer buf;
      json_generate(&buf, &modules);

      // Write buf to file
      const char *name = global.params.xfilename;

      if (name && name[0] == '-' && name[1] == 0)
	{
	  size_t n = fwrite (buf.data, 1, buf.offset, stdmsg);
	  gcc_assert (n == buf.offset);
	}
      else
	{
	  const char *jsonfilename;

	  if (name && *name)
	    jsonfilename = FileName::defaultExt(name, global.json_ext);
	  else
	    {
	      // Generate json file name from first obj name
	      const char *n = (*global.params.objfiles)[0];
	      n = FileName::name(n);
	      jsonfilename = FileName::forceExt(n, global.json_ext);
	    }

	  FileName::ensurePathToNameExists(jsonfilename);
	  File *jsonfile = new File(jsonfilename);

	  jsonfile->setbuffer(buf.data, buf.offset);
	  jsonfile->ref = 1;
	  jsonfile->writev();
	}
    }

  for (size_t i = 0; i < modules.dim; i++)
    {
      m = modules[i];
      if (fonly_arg && m != output_module)
	continue;
      if (global.params.verbose)
	fprintf (stdmsg, "code      %s\n", m->toChars());
      if (!flag_syntax_only)
	m->genobjfile (false);
      if (!global.errors && !errorcount)
	{
	  if (global.params.doDocComments)
	    m->gendocfile();
	}
    }

  // better to use input_location.xxx ?
  (*debug_hooks->end_source_file) (input_line);
 had_errors:
  // Add D frontend error count to GCC error count to to exit with error status
  errorcount += (global.errors + global.warnings);

  d_finish_module();
  d_backend_term();

  output_module = NULL;
}

static tree
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

static tree
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

static tree
d_signed_or_unsigned_type (int unsignedp, tree type)
{
  if (!INTEGRAL_TYPE_P (type)
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

/* Type promotion for variable arguments.  */
static tree
d_type_promotes_to (tree type)
{
  /* Almost the same as c_type_promotes_to.  This is needed varargs to work on
     certain targets. */
  if (TYPE_MAIN_VARIANT (type) == float_type_node)
    return double_type_node;

  // not quite the same as... if (c_promoting_integer_type_p (type))
  if (INTEGRAL_TYPE_P (type)
      && (TYPE_PRECISION (type) < TYPE_PRECISION (integer_type_node)))
    {
      /* Preserve unsignedness if not really getting any wider.  */
      if (TYPE_UNSIGNED (type)
	  && (TYPE_PRECISION (type) == TYPE_PRECISION (integer_type_node)))
	return unsigned_type_node;
      return integer_type_node;
    }

  return type;
}


struct binding_level *current_binding_level;
struct binding_level *global_binding_level;


static binding_level *
alloc_binding_level (void)
{
  return ggc_alloc_cleared_binding_level();
}

/* The D front-end does not use the 'binding level' system for a symbol table,
   It is only needed to get debugging information for local variables and
   otherwise support the backend. */

void
pushlevel (int)
{
  binding_level *new_level = alloc_binding_level();
  new_level->level_chain = current_binding_level;
  current_binding_level = new_level;
}

tree
poplevel (int keep, int reverse, int routinebody)
{
  binding_level *level = current_binding_level;
  tree block, decls;

  current_binding_level = level->level_chain;
  decls = level->names;
  if (reverse)
    decls = nreverse (decls);

  if (level->this_block)
    block = level->this_block;
  else if (keep || routinebody)
    block = make_node (BLOCK);
  else
    block = NULL_TREE;

  if (block)
    {
      BLOCK_VARS (block) = routinebody ? NULL_TREE : decls;
      BLOCK_SUBBLOCKS (block) = level->blocks;
      // %% need this for when insert_block is called by backend... or make
      // insert_block do it's work elsewere
      // %% pascal does: in each subblock, record that this is the superiod..
    }
  /* In each subblock, record that this is its superior. */
  for (tree t = level->blocks; t; t = BLOCK_CHAIN (t))
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
      for (tree t = nreverse (vars); t != NULL_TREE; t = TREE_CHAIN (t))
	{
	  if (TREE_CODE (t) == VAR_DECL
	      && (!TREE_USED (t) /*|| !DECL_READ_P (t)*/) // %% TODO
	      && !TREE_NO_WARNING (t)
	      && DECL_NAME (t)
	      && !DECL_ARTIFICIAL (t))
	    {
	      if (!TREE_USED (t))
		warning_at (DECL_SOURCE_LOCATION (t),
			    OPT_Wunused_variable, "unused variable %q+D", t);
	      else if (DECL_CONTEXT (t) == current_function_decl)
		warning_at (DECL_SOURCE_LOCATION (t),
			    OPT_Wunused_but_set_variable, "variable %qD set but not used", t);
	    }
	}
    }
  return block;
}


// This is called by the backend before parsing.  Need to make this do
// something or lang_hooks.clear_binding_stack (lhd_clear_binding_stack)
// loops forever.
bool
global_bindings_p (void)
{
  return current_binding_level == global_binding_level || !global_binding_level;
}

void
init_global_binding_level (void)
{
  current_binding_level = global_binding_level = alloc_binding_level();
}


void
insert_block (tree block)
{
  TREE_USED (block) = 1;
  current_binding_level->blocks = block_chainon (current_binding_level->blocks, block);
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
  if (DECL_CONTEXT (decl) == NULL_TREE)
    DECL_CONTEXT (decl) = current_function_decl; // could be NULL_TREE (top level) .. hmm. // hm.m.

  /* Put decls on list in reverse order. We will reverse them later if necessary. */
  TREE_CHAIN (decl) = current_binding_level->names;
  current_binding_level->names = decl;
  if (!TREE_CHAIN (decl))
    current_binding_level->names_end = decl;
  return decl;
}

void
set_decl_binding_chain (tree decl_chain)
{
  gcc_assert (current_binding_level);
  current_binding_level->names = decl_chain;
}


// Supports dbx and stabs
tree
getdecls (void)
{
  if (current_binding_level)
    return current_binding_level->names;
  else
    return NULL_TREE;
}


static int
d_types_compatible_p (tree t1, tree t2)
{
  /* Is compatible if types are equivalent */
  if (TYPE_MAIN_VARIANT (t1) == TYPE_MAIN_VARIANT (t2))
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
    {
      /* D allows zero-length declarations.  Such a declaration ends up with
	 DECL_SIZE (t) == NULL_TREE which is what the backend function
	 assembler_variable checks.  This could change in later versions...

	 Maybe all of these variables should be aliased to one symbol... */
      if (DECL_SIZE (decl) == 0)
	{
	  DECL_SIZE (decl) = bitsize_zero_node;
	  DECL_SIZE_UNIT (decl) = size_zero_node;
	}
    }
}


struct lang_type *
build_d_type_lang_specific (Type *t)
{
  unsigned sz = sizeof (struct lang_type);
  struct lang_type *lt = ggc_alloc_cleared_lang_type (sz);
  lt->d_type = t;
  return lt;
}

struct lang_decl *
build_d_decl_lang_specific (Declaration *d)
{
  unsigned sz = sizeof (struct lang_decl);
  struct lang_decl *ld = ggc_alloc_cleared_lang_decl (sz);
  ld->d_decl = d;
  return ld;
}


// This preserves tree we create from the garbage collector.
tree d_keep_list = NULL_TREE;

void
d_keep (tree t)
{
  d_keep_list = tree_cons (NULL_TREE, t, d_keep_list);
}

tree d_eh_personality_decl;

/* Return the GDC personality function decl.  */
static tree
d_eh_personality (void)
{
  if (!d_eh_personality_decl)
    {
      d_eh_personality_decl
	= build_personality_function ("gdc");
    }
  return d_eh_personality_decl;
}

static tree
d_build_eh_type_type (tree type)
{
  Type *dtype = build_dtype (type);
  Symbol *sym;

  if (dtype)
    dtype = dtype->toBasetype();

  gcc_assert (dtype && dtype->ty == Tclass);
  sym = ((TypeClass *) dtype)->sym->toSymbol();

  return convert (ptr_type_node, build_address (sym->Stree));
}

void
d_init_exceptions (void)
{
  using_eh_for_cleanups();
}


/* Handle a "noinline" attribute.  */

static tree
d_handle_noinline_attribute (tree *node, tree name,
			     tree ARG_UNUSED (args),
			     int ARG_UNUSED (flags), bool *no_add_attrs)
{
  Type *t = build_dtype (TREE_TYPE (*node));

  if (t->ty == Tfunction)
    DECL_UNINLINABLE (*node) = 1;
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "forceinline" attribute.  */

static tree
d_handle_forceinline_attribute (tree *node, tree name,
				tree ARG_UNUSED (args),
				int ARG_UNUSED (flags),
				bool *no_add_attrs)
{
  Type *t = build_dtype (TREE_TYPE (*node));

  if (t->ty == Tfunction)
    {
      tree attributes = DECL_ATTRIBUTES (*node);

      // Push attribute always_inline.
      if (! lookup_attribute ("always_inline", attributes))
	DECL_ATTRIBUTES (*node) = tree_cons (get_identifier ("always_inline"),
					     NULL_TREE, attributes);

      DECL_DECLARED_INLINE_P (*node) = 1;
      DECL_NO_INLINE_WARNING_P (*node) = 1;
      DECL_DISREGARD_INLINE_LIMITS (*node) = 1;
    }
  else
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "flatten" attribute.  */

static tree
d_handle_flatten_attribute (tree *node, tree name,
			    tree args ATTRIBUTE_UNUSED,
			    int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  Type *t = build_dtype (TREE_TYPE (*node));

  if (t->ty != Tfunction)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "target" attribute.  */

static tree
d_handle_target_attribute (tree *node, tree name, tree args, int flags,
			   bool *no_add_attrs)
{
  Type *t = build_dtype (TREE_TYPE (*node));

  /* Ensure we have a function type.  */
  if (t->ty != Tfunction)
    {
      warning (OPT_Wattributes, "%qE attribute ignored", name);
      *no_add_attrs = true;
    }
  else if (! targetm.target_option.valid_attribute_p (*node, name, args, flags))
    *no_add_attrs = true;

  return NULL_TREE;
}

struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;
