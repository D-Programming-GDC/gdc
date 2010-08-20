/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, (C) 2009, 2010
    Iain Buclaw, (C) 2010
   
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

#if ! V2
#include "async.h"
#include "json.h"
#endif

#include <assert.h>
#include "d-gcc-includes.h"
#include "options.h"
#include "d-lang.h"
#include "d-codegen.h"
#include "d-gcc-real.h"
#include "d-confdefs.h"

static char lang_name[6] = "GNU D";

#undef LANG_HOOKS_NAME
#define LANG_HOOKS_NAME lang_name

#undef LANG_HOOKS_INIT
#define LANG_HOOKS_INIT d_init

#undef LANG_HOOKS_INIT_OPTIONS
#define LANG_HOOKS_INIT_OPTIONS d_init_options

#undef LANG_HOOKS_HANDLE_OPTION
#define LANG_HOOKS_HANDLE_OPTION d_handle_option
#undef LANG_HOOKS_POST_OPTIONS
#define LANG_HOOKS_POST_OPTIONS d_post_options

#undef LANG_HOOKS_PARSE_FILE
#define LANG_HOOKS_PARSE_FILE d_parse_file

#if D_GCC_VER < 41
#undef LANG_HOOKS_TRUTHVALUE_CONVERSION
#define LANG_HOOKS_TRUTHVALUE_CONVERSION d_truthvalue_conversion
#endif

#undef LANG_HOOKS_MARK_ADDRESSABLE
#define LANG_HOOKS_MARK_ADDRESSABLE d_mark_addressable

#undef LANG_HOOKS_TYPE_FOR_MODE
#define LANG_HOOKS_TYPE_FOR_MODE d_type_for_mode

#undef LANG_HOOKS_TYPE_FOR_SIZE
#define LANG_HOOKS_TYPE_FOR_SIZE d_type_for_size

#undef LANG_HOOKS_BUILTIN_FUNCTION
#if D_GCC_VER >= 43
# define LANG_HOOKS_BUILTIN_FUNCTION d_builtin_function43
#else
# define LANG_HOOKS_BUILTIN_FUNCTION d_builtin_function
#endif

#if D_GCC_VER < 40
#undef LANG_HOOKS_EXPAND_EXPR
#define LANG_HOOKS_EXPAND_EXPR d_expand_expr
#endif

#define LANG_HOOKS_UNSIGNED_TYPE d_unsigned_type
#define LANG_HOOKS_SIGNED_TYPE d_signed_type
#define LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE d_signed_or_unsigned_type
#define LANG_HOOKS_TYPE_FOR_SIZE d_type_for_size
#define LANG_HOOKS_TYPE_FOR_MODE d_type_for_mode

#undef  LANG_HOOKS_TYPE_PROMOTES_TO
#define LANG_HOOKS_TYPE_PROMOTES_TO d_type_promotes_to

#undef LANG_HOOKS_WRITE_GLOBALS
#define LANG_HOOKS_WRITE_GLOBALS d_write_global_declarations

#undef LANG_HOOKS_GET_ALIAS_SET
#define LANG_HOOKS_GET_ALIAS_SET d_hook_get_alias_set

// Needed for try/finally -- statements cannont be re-evaluated
#if D_GCC_VER < 40
#undef LANG_HOOKS_UNSAFE_FOR_REEVAL
#define LANG_HOOKS_UNSAFE_FOR_REEVAL d_unsafe_for_reeval
#endif

#undef LANG_HOOKS_COMMON_ATTRIBUTE_TABLE
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE d_common_attribute_table
#undef LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE
#define LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE d_common_format_attribute_table

#if D_GCC_VER >= 40 && D_GCC_VER < 43
#undef LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION
#define LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION d_expand_function
#endif

#if D_GCC_VER >= 40
#undef LANG_HOOKS_TYPES_COMPATIBLE_P
#define LANG_HOOKS_TYPES_COMPATIBLE_P d_types_compatible_p

#if V2
#undef LANG_HOOKS_TREE_INLINING_CONVERT_PARM_FOR_INLINING
#define LANG_HOOKS_TREE_INLINING_CONVERT_PARM_FOR_INLINING d_convert_parm_for_inlining
#endif

#endif


////static tree d_type_for_size PARAMS ((unsigned, int));
static tree d_signed_or_unsigned_type(int, tree);
////tree d_unsigned_type(tree);
////tree d_signed_type(tree);

/*
class DGCCMain {
    bool stdInc;

    initOptions() { }
};
*/

static const char * iprefix;
static bool std_inc; // %%FIX: find a place for this
static const char * fonly_arg;
static const char * multilib_dir;
// Because of PR16888, on x86 platforms, GCC clears unused reg names.
// As this doesn't affect us, need a way to restore them.
static const char *saved_reg_names[FIRST_PSEUDO_REGISTER];

static unsigned int
d_init_options (unsigned int, const char ** argv)
{
    // Set default values
    global.params.argv0 = (char *) argv[0]; //progname
    global.params.link = 1;
    global.params.useAssert = 1;
    global.params.useInvariants = 1;
    global.params.useIn = 1;
    global.params.useOut = 1;
    global.params.useArrayBounds = 1;
    flag_bounds_check = global.params.useArrayBounds; // keep in synch with existing -fbounds-check flag
    global.params.useSwitchError = 1;
    global.params.useInline = 0;
    global.params.warnings = 0;
    global.params.obj = 1;
    global.params.Dversion = 2;
    global.params.quiet = 1;

    global.params.linkswitches = new Array();
    global.params.libfiles = new Array();
    global.params.objfiles = new Array();
    global.params.ddocfiles = new Array();

    global.params.imppath = new Array();
    global.params.fileImppath = new Array();

    // GCC options
    flag_exceptions = 1;

    // extra D-specific options
    gen.splitDynArrayVarArgs = true;
    gen.emitTemplates = TEauto;
    gen.useBuiltins = true;
    std_inc = true;

    return CL_D;
}

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
	NULL };
    if (!strcmp(cygwin_d_os_versym,"cygwin") && env && *env == '1') {
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

static char *
prefixed_path(const char * path)
{
    // based on c-incpath.c
    size_t len = cpp_GCC_INCLUDE_DIR_len;    
    if (iprefix && len != 0 && ! strncmp(path, cpp_GCC_INCLUDE_DIR, len))
	return concat(iprefix, path + len, NULL);
    else
	return xstrdup(path);
}

static int
d_cpp_forall_callback(cpp_reader *, cpp_hashnode * hn, void *)
{
    const char * str = (const char *) hn->ident.str;
    unsigned int len = hn->ident.len;
    char * d_ident = new char[4 + len + 1];
    if (len > 4 && str[0] == '_' && str[1] == '_' &&
	str[len - 1] == '_' && str[len - 2] == '_')
    {
	str += 2;
	len -= 4;
    }
    strcpy(d_ident, "GNU_");
    strncpy(d_ident + 4, str, len);
    d_ident[len + 4] = '\0';
    VersionCondition::addPredefinedGlobalIdent(d_ident);
    return 1;
}

/* Supports CPP builtins. */
cpp_reader * parse_in;
extern "C" void
builtin_define_std (const char *macro)
{
    // Do nothing.  No need for these (yet).
}

static bool
d_init ()
{
    const char * cpu_versym = NULL;

#if D_GCC_VER >= 40
    /* Restore register names if any were cleared during backend init */
    if (memcmp (reg_names, saved_reg_names, sizeof reg_names))
	memcpy (reg_names, saved_reg_names, sizeof reg_names);
#endif
    
    /* Currently, isX86_64 indicates a 64-bit target in general and is not
       Intel-specific. */
#ifdef TARGET_64BIT
    global.params.isX86_64 = TARGET_64BIT ? 1 : 0;
#else
    /* TARGET_64BIT is only defined on biarched archs defaulted to 64-bit
     * (as amd64 or s390x) so for full 64-bit archs (as ia64 or alpha) we
     * need to test it more. */
#  ifdef D_CPU_VERSYM64
	/* We are "defaulting" to 32-bit, which mean that if both D_CPU_VERSYM
	 * and D_CPU_VERSYM64 are defined, and not TARGET_64BIT, we will use
	 * 32 bits. This will be overidden for full 64-bit archs */
	global.params.isX86_64 = 0;
#    ifndef D_CPU_VERSYM
	/* So this is typically for alpha and ia64 */
	global.params.isX86_64 = 1;
#    endif
#  else
#    ifdef D_CPU_VERSYM /* D_CPU_VERSYM is defined and D_CPU_VERSYM64 is not. */
	global.params.isX86_64 = 0;
#    else
	/* If none of D_CPU_VERSYM and D_CPU_VERSYM64 defined check size_t
	 * length instead. */
	switch (sizeof(size_t)) {
	    case 4:
	    	global.params.isX86_64 = 0;
		break;
	    case 8:
	        global.params.isX86_64 = 1;
		break;
	}
#    endif
#  endif
#endif

#ifdef D_USE_MAPPED_LOCATION
    /* input_location is initially set to BUILTINS_LOCATION (2).  This 
       will cause a segfault if errors are reported before any line maps 
       are created. Setting input_location to zero stops those segfaults,
       but then decls are created with DECL_SOURCE_LOCATION set to zero
       which causes segfaults in dwarf2out. */
    linemap_add(line_table, LC_RENAME, 0, "<builtin>", 1);
    input_location = linemap_line_start(line_table, 1, 1);
#endif
    
    Type::init();
    Id::initialize();
    Module::init();
    initPrecedence();
    gcc_d_backend_init();
    real_t::init();

    maybe_fixup_cygwin();

    VersionCondition::addPredefinedGlobalIdent("GNU");
#ifdef D_CPU_VERSYM64
    if (global.params.isX86_64 == 1)
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
	 {
		 VersionCondition::addPredefinedGlobalIdent("Windows");
		 is_target_win32 = true;
	 }
    if (strcmp(D_OS_VERSYM, "freebsd") == 0)
    VersionCondition::addPredefinedGlobalIdent("FreeBSD");
#endif
#ifdef D_OS_VERSYM2
    VersionCondition::addPredefinedGlobalIdent(D_OS_VERSYM2);
    if (strcmp(D_OS_VERSYM2, "Win32") == 0)
	is_target_win32 = true;
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

    if (d_using_sjlj_exceptions()) {
	VersionCondition::addPredefinedGlobalIdent("GNU_SjLj_Exceptions");
    }
#ifdef TARGET_LONG_DOUBLE_128
    if (TARGET_LONG_DOUBLE_128)
	VersionCondition::addPredefinedGlobalIdent("GNU_LongDouble128");
#endif

    if (d_have_inline_asm())
    {
	VersionCondition::addPredefinedGlobalIdent("D_InlineAsm");

	if (TARGET_80387)
	    VersionCondition::addPredefinedGlobalIdent("D_InlineAsm_X86");
	// TODO: D_InlineAsm_X86_64
	
	/* Should define this anyway to set us apart from the competition. */
	VersionCondition::addPredefinedGlobalIdent("GNU_InlineAsm");
    }

    /* Setting global.params.cov forces module info generation which is
       not needed for thee GCC coverage implementation.  Instead, just
       test flag_test_coverage while leaving global.params.cov unset. */
    //if (global.params.cov)
    if (flag_test_coverage)
	VersionCondition::addPredefinedGlobalIdent("D_Coverage");
    if (global.params.useUnitTests)
	VersionCondition::addPredefinedGlobalIdent("unittest");

    {
	line_maps lm;
	cpp_reader * pfile; // Target macros below expect this identifier.
	int flag_iso = 0;   // ditto

	linemap_init(& lm);
#if D_GCC_VER >= 43
	lm.reallocator = NULL;
#endif
	parse_in = pfile = cpp_create_reader(CLK_STDC89, NULL
#if D_GCC_VER >= 40
	    , & lm
#endif
					     );
	cpp_change_file(pfile, LC_ENTER, "<built-in>");
	
	// from c-cppbuiltin.c
#ifndef TARGET_OS_CPP_BUILTINS
# define TARGET_OS_CPP_BUILTINS()
#endif
#ifndef TARGET_OBJFMT_CPP_BUILTINS
# define TARGET_OBJFMT_CPP_BUILTINS()
#endif

# define preprocessing_asm_p() (0)
# define preprocessing_trad_p() (0)
# define c_dialect_cxx() (0)
# define c_dialect_objc() (0)
# define builtin_define(TXT) (cpp_define (pfile, TXT))
# define builtin_define_with_value(m,e,s)
# define builtin_define_with_int_value(m,i)
# define builtin_define_std(TXT) (cpp_define (pfile, TXT))
# define builtin_assert(TXT) (cpp_assert (pfile, TXT))
	TARGET_CPU_CPP_BUILTINS ();
	TARGET_OS_CPP_BUILTINS ();
	TARGET_OBJFMT_CPP_BUILTINS ();
	
	cpp_forall_identifiers(pfile, & d_cpp_forall_callback, NULL);

	cpp_destroy(pfile);
	linemap_free(& lm);
    }

    VersionCondition::addPredefinedGlobalIdent("all");


    // %%TODO: front or back?
    if (std_inc)
    {
	char * target_dir = prefixed_path(D_PHOBOS_TARGET_DIR);
	if (multilib_dir)
	    target_dir = concat(target_dir, "/", multilib_dir, NULL);
	
	global.params.imppath->insert(0, prefixed_path(D_PHOBOS_DIR));
	global.params.imppath->insert(0, target_dir);
    }

    if (global.params.imppath)
    {
	for (unsigned i = 0; i < global.params.imppath->dim; i++)
	{
	    char *path = (char *)global.params.imppath->data[i];
	    // We would do this for D_INCLUDE_PATH env var, but not for '-I'
	    // command line args.
	    //Array *a = FileName::splitPath(path);

	    if (path)
	    {
		if (!global.path)
		    global.path = new Array();
		//global.path->append(a);
		global.path->push(path);
	    }
	}
    }

    if (global.params.fileImppath)
    {
	for (unsigned i = 0; i < global.params.fileImppath->dim; i++)
	{
	    char *path = (char *)global.params.fileImppath->data[i];
	    if (path)
	    {
		if (!global.filePath)
		    global.filePath = new Array();
		global.filePath->push(path);
	    }
	}
    }

    {
	char * path = FileName::searchPath(global.path, "phobos-ver-syms", 1);
	if (path) {
	    FILE * f = fopen(path, "r");
	    char buf[256];
	    char *p, *q;
	    if (f) {
		while ( ! feof(f) && fgets(buf, 256, f) ) {
		    p = buf;
		    while (*p && ISSPACE(*p))
			p++;
		    q = p;
		    while (*q && ! ISSPACE(*q))
			q++;
		    *q = 0;
		    if (p != q) {
			/* Needs to be predefined because we define
			   Unix/Windows this way. */
			VersionCondition::addPredefinedGlobalIdent(xstrdup(p));
		    }
		}
		fclose(f);
	    } else {
		//printf("failed\n");
	    }
	} else {
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

static int
d_handle_option (size_t scode, const char *arg, int value)
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
	  flag_bounds_check = global.params.useArrayBounds = ! value;
	  global.params.useSwitchError = ! value;
	  break;
      case OPT_funittest:
	  global.params.useUnitTests = value;
	  break;
      case OPT_fversion_:
	  if (ISDIGIT(arg[0])) {
	      if (! parse_int(arg, & level))
		  goto Lerror_v;
	      VersionCondition::setGlobalLevel(level);
	  } else if (Lexer::isValidIdentifier((char*) arg))
	      VersionCondition::addGlobalIdent(xstrdup(arg));
	  else {
	  Lerror_v:
	      error("bad argument for -fversion");
	  }
	  break;
      case OPT_fdebug:
	  global.params.debuglevel = value ? 1 : 0;
	  break;
      case OPT_fdebug_:
	  if (ISDIGIT(arg[0])) {
	      if (! parse_int(arg, & level))
		  goto Lerror_d;
	      DebugCondition::setGlobalLevel(level);
	  } else if (Lexer::isValidIdentifier((char*) arg))
	      DebugCondition::addGlobalIdent(xstrdup(arg));
	  else {
	  Lerror_d:
	      error("bad argument for -fdebug");
	  }
	  break;
      case OPT_fdebug_c:
	  strcpy(lang_name, value ? "GNU C" : "GNU D");
	  break;
#if ! V2
      case OPT_fdeps_:
	  global.params.moduleDepsFile = (char*)arg;
	  if (!global.params.moduleDepsFile[0])
	      error("bad argument for -fdeps");
	  global.params.moduleDeps = new OutBuffer;
	  break;
#endif
      case OPT_fignore_unknown_pragmas:
	  global.params.ignoreUnsupportedPragmas = value;
	  break;
#ifdef _DH
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
#endif
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
      case OPT_fd_version_1:
	  global.params.Dversion = 1;
	  break;
      case OPT_femit_templates:
	  gen.emitTemplates = value ? TEauto : TEnone;
	  break;
      case OPT_femit_templates_:
	  if (! arg || ! *arg) {
	      gen.emitTemplates = value ? TEauto : TEnone;
	  } else if (! strcmp(arg, "normal")) {
	      gen.emitTemplates = TEnormal;
	  } else if (! strcmp(arg, "all")) {
	      gen.emitTemplates = TEall;
	  } else if (! strcmp(arg, "private")) {
	      gen.emitTemplates = TEprivate;
	  } else if (! strcmp(arg, "none")) {
	      gen.emitTemplates = TEnone;
	  } else if (! strcmp(arg, "auto")) {
	      gen.emitTemplates = TEauto;
	  } else {
	      error("bad argument for -femit-templates");
	  }
	  break;
      case OPT_fonly_:
	  fonly_arg = xstrdup(arg);
	  break;
      case OPT_iprefix:
	  iprefix = xstrdup(arg);
	  break;
      case OPT_fmultilib_dir_:
	  multilib_dir = xstrdup(arg);
	  break;
      case OPT_nostdinc:
	  std_inc = false;
	  break;
      case OPT_fdump_source:
          global.params.dump_source = value;
	  break;
      case OPT_fbuiltin:
	  gen.useBuiltins = value;
	  break;
      case OPT_fsigned_char:
      case OPT_funsigned_char:
	  // ignored
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
#if V1
	  global.params.doXGeneration = 1;
	  global.params.xfilename = (char*)arg;
#endif
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
#if D_GCC_VER >= 40
    // Save register names for restoring later.
    memcpy (saved_reg_names, reg_names, sizeof reg_names);

    // Inline option code copied from c-opts.c
    flag_inline_trees = 1;

    /* Use tree inlining.  */
    if (!flag_no_inline)
	flag_no_inline = 1;
    if (flag_inline_functions)
	flag_inline_trees = 2;

    /* If we are given more than one input file, we must use
       unit-at-a-time mode.  */
    if (num_in_fnames > 1)
	flag_unit_at_a_time = 1;
#endif
    return false;
}

/* wrapup_global_declaration needs to be called or functions will not
   be emitted. */
Array globalFunctions; // Array of tree (for easy passing to wrapup_global_declarations)

void
d_add_global_function(tree decl)
{
    globalFunctions.push(decl);
}

static void
d_write_global_declarations()
{
    tree * vec = (tree *) globalFunctions.data;
    wrapup_global_declarations(vec, globalFunctions.dim);
    check_global_declarations(vec, globalFunctions.dim);
#if D_GCC_VER >= 40

#if D_GCC_VER >= 41
    cgraph_optimize();
#endif
    

    for (unsigned i = 0; i < globalFunctions.dim; i++)
	debug_hooks->global_decl(vec[i]);

#if D_GCC_VER == 40
    /* For 4.0.x, if cgraph_optimize is called before the loop over
       globalFunctions, dwarf2out can generate a DIE tree that has a
       nested class as the parent of an outer class (this causes an
       ICE.) Not sure if this is due to a bug in the D front end, but calling
       debug_hooks->global_decl ensures the outer elements are
       processed first.

       For later versions, calling cgraph_optimize later causes more
       problems.
    */
    cgraph_optimize();
#endif
    
#endif
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

#if D_GCC_VER < 40
// taken from c_common_unsafe_for_reeval
int
d_unsafe_for_reeval (tree exp)
{
  /* Statement expressions may not be reevaluated.  */
  if (TREE_CODE (exp) == (enum tree_code) D_STMT_EXPR)
      return 2;

  /* Walk all other expressions.  */
  return -1;
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
    tree decl = build_decl(TYPE_DECL, ident, type);
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
d_parse_file (int /*set_yydebug*/)
{
    Identifier * id;
    //Module * m;
    char * p, * e;
    char * name;
    unsigned i;
    
    if (global.params.verbose)
    {   printf("binary    %s\n", global.params.argv0);
        printf("version   %s\n", global.version);
    }

    if (global.params.verbose && asm_out_file == stdout)
    {
	// Really, driver should see the option and turn off -pipe
	error("Cannot use -fd-verbose with -pipe");
	return;
    }

    if (global.params.useUnitTests)
	global.params.useAssert = 1;
    global.params.useArrayBounds = flag_bounds_check;
    if (gen.emitTemplates == TEauto) {
	gen.emitTemplates = (supports_one_only()) ? TEnormal : TEprivate;
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

    /* hack...
     */
#if ! V2
    Type::tbit->toCtype();
#endif
    Type::tbool->toCtype();
    Type::tchar->toCtype();
    Type::twchar->toCtype();
    Type::tdchar->toCtype();
    
    for (TY ty = (TY) 0; ty < TMAX; ty = (TY)(ty + 1)) {
	if (Type::basic[ty] && ty != Terror)
	    nametype(Type::basic[ty]);
    }

    /*
    p = FileName::name(input_filename);
    e = FileName::ext(p);
    if (e) {
	e--;
	assert( *e == '.' );
	name = (char *) xmalloc((e - p) + 1);
	memcpy(name, p, e - p);
	name[e - p] = 0;
    } else
	name = p;
    */
    an_output_module = NULL;
    Array modules; // vs. outmodules... = [an_output_module] or modules
    modules.reserve(num_in_fnames);
#if ! V2
    AsyncRead *aw;
#endif

    // %% FIX
    if ( ! main_input_filename ) {
	::error("input file name required; cannot use stdin");
	goto had_errors;
    }

    if (fonly_arg) {
	/* In this mode, the first file name is supposed to be
	   a duplicate of one of the input file. */
	if (strcmp(fonly_arg, main_input_filename))
	    ::error("-fonly= argument is different from main input file name");
	if (strcmp(fonly_arg, in_fnames[0]))
	    ::error("-fonly= argument is different from first input file name");
    }

    //fprintf (stderr, "***** %d files  main=%s\n", num_in_fnames, input_filename);
    
    for (i = 0; i < num_in_fnames; i++) {
	if (fonly_arg) {
	    if (i == 0)
		continue;
	    /* %% Do the other modules really need to be processed? 
	    else if (an_output_module)
		break;
	    */
	}
	
	//fprintf(stderr, "fn %d = %s\n", i, in_fnames[i]);

	char * the_fname = (char*) in_fnames[i];
	
	p = FileName::name(the_fname);
	e = FileName::ext(p);
	if (e) {
	    e--;
	    assert( *e == '.' );
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
	{   name = p;
	    if (!*name)
		goto Linvalid;
	}
	
	id = new Identifier(name, 0);
	Module * m = new Module(the_fname, id, global.params.doDocComments, global.params.doHdrGeneration);
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

    assert(an_output_module);
    
    Module * m;

    //global.params.verbose = 1;
    
    // Read files
#if ! V2
    aw = AsyncRead::create(modules.dim);
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
	aw->addFile(m->srcfile);
    }
    aw->start();
    for (i = 0; i < modules.dim; i++)
    {
	if (aw->read(i))
	{
	    error("cannot read file %s", m->srcfile->name->toChars());
	    goto had_errors;
	}
    }
    AsyncRead::dispose(aw);
#else
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
	m->read(0);
    }
#endif
    // Parse files
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
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

#ifdef _DH
     if (global.params.doHdrGeneration)
     {
	 /* Generate 'header' import files.
	  * Since 'header' import files must be independent of command
	  * line switches and what else is imported, they are generated
	  * before any semantic analysis.
	  */
	 for (i = 0; i < modules.dim; i++)
	 {
	     m = (Module *)modules.data[i];
	     if (fonly_arg && m != an_output_module)
		 continue;
	     if (global.params.verbose)
		 printf("import    %s\n", m->toChars());
	     m->genhdrfile();
	 }
     }
     if (global.errors)
	 fatal();
#endif

#if ! V2
    // load all unconditional imports for better symbol resolving
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
	if (global.params.verbose)
	    printf("importall %s\n", m->toChars());
	m->importAll(0);
    }
    if (global.errors)
	goto had_errors;
#endif

    // Do semantic analysis
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
	if (global.params.verbose)
	    printf("semantic  %s\n", m->toChars());
	m->semantic();
    }
    if (global.errors)
	goto had_errors;
	
#if V1
    Module::dprogress = 1;
    Module::runDeferredSemantic();
#endif

    // Do pass 2 semantic analysis
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
	if (global.params.verbose)
	    printf("semantic2 %s\n", m->toChars());
	m->semantic2();
    }
    if (global.errors)
	goto had_errors;

    // Do pass 3 semantic analysis
    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
	if (global.params.verbose)
	    printf("semantic3 %s\n", m->toChars());
	m->semantic3();
    }
    if (global.errors)
	goto had_errors;

#if ! V2
    if (global.params.moduleDeps != NULL)
    {
	assert(global.params.moduleDepsFile != NULL);

	File deps(global.params.moduleDepsFile);
	OutBuffer* ob = global.params.moduleDeps;
	deps.setbuffer((void*)ob->data, ob->offset);
	deps.writev();
    }
#endif

    // Scan for functions to inline
    if (global.params.useInline)
    {
	/* The problem with useArrayBounds and useAssert is that the
	 * module being linked to may not have generated them, so if
	 * we inline functions from those modules, the symbols for them will
	 * not be found at link time.
	 */
	if (!global.params.useArrayBounds && !global.params.useAssert)
	{
	    // Do pass 3 semantic analysis on all imported modules,
	    // since otherwise functions in them cannot be inlined
	    for (i = 0; i < Module::amodules.dim; i++)
	    {
		m = (Module *)Module::amodules.data[i];
		if (global.params.verbose)
		    printf("semantic3 %s\n", m->toChars());
		m->semantic3();
	    }
	    if (global.errors)
		goto had_errors;
	}

	for (i = 0; i < modules.dim; i++)
	{
	    m = (Module *)modules.data[i];
	    if (global.params.verbose)
		printf("inline scan %s\n", m->toChars());
	    m->inlineScan();
	}
    }
    
    // Do not attempt to generate output files if errors or warnings occurred
    if (global.errors
#if V1
	|| global.warnings
#endif
       )
    {
	fatal();
    }

    g.ofile = new ObjectFile();
    if (fonly_arg)
	g.ofile->modules.push(an_output_module);
    else
	g.ofile->modules.append(& modules);
    g.irs = & gen; // needed for FuncDeclaration::toObjFile shouldDefer check
    
    // Generate output files
    
#if V1
    if (global.params.doXGeneration)
	json_generate(&modules);
#endif

    for (i = 0; i < modules.dim; i++)
    {
	m = (Module *)modules.data[i];
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
    errorcount += global.errors;

    g.ofile->finish();

#if D_GCC_VER >= 40
    cgraph_finalize_compilation_unit();
#endif

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
#if D_GCC_VER < 40
	  if ( ! TREE_STATIC(x) ) // %% C doesn't do this check
	      put_var_into_stack(x, 1);
#endif
	  // drop through
      case FUNCTION_DECL:
	TREE_ADDRESSABLE (x) = 1;
	/* drops through */
      default:
	return true;
    }
  
    return 1;
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

#if D_GCC_VER < 40
  switch (mode)
    {
    case V16QImode:
      return unsignedp ? unsigned_V16QI_type_node : V16QI_type_node;
    case V8HImode:
      return unsignedp ? unsigned_V8HI_type_node : V8HI_type_node;
    case V4SImode:
      return unsignedp ? unsigned_V4SI_type_node : V4SI_type_node;
    case V2DImode:
      return unsignedp ? unsigned_V2DI_type_node : V2DI_type_node;
    case V2SImode:
      return unsignedp ? unsigned_V2SI_type_node : V2SI_type_node;
    case V2HImode:
      return unsignedp ? unsigned_V2HI_type_node : V2HI_type_node;
    case V4HImode:
      return unsignedp ? unsigned_V4HI_type_node : V4HI_type_node;
    case V8QImode:
      return unsignedp ? unsigned_V8QI_type_node : V8QI_type_node;
    case V1DImode:
      return unsignedp ? unsigned_V1DI_type_node : V1DI_type_node;
    case V16SFmode:
      return V16SF_type_node;
    case V4SFmode:
      return V4SF_type_node;
    case V2SFmode:
      return V2SF_type_node;
    case V2DFmode:
      return V2DF_type_node;
    default:
      break;
    }
#else
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
#endif

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


extern "C" void pushlevel PARAMS ((int));
extern "C" tree poplevel PARAMS ((int, int, int));
extern "C" int global_bindings_p PARAMS ((void));
extern "C" void insert_block PARAMS ((tree));
extern "C" void set_block PARAMS ((tree));
extern "C" tree getdecls PARAMS ((void));


struct binding_level * current_binding_level;
struct binding_level * global_binding_level;


static binding_level *
alloc_binding_level()
{
    return (struct binding_level *) ggc_alloc_cleared (sizeof (struct binding_level));
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

    if ( level->this_block )
	block = level->this_block;
    else if (keep || routinebody)
	block = make_node(BLOCK);
    else
	block = NULL_TREE;
    
    if (block) {
	BLOCK_VARS( block ) = routinebody ? NULL_TREE : decls;
	BLOCK_SUBBLOCKS( block ) = level->blocks;
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
	TREE_USED (block) = 1;
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
    if ( DECL_CONTEXT( decl ) == NULL_TREE )
	DECL_CONTEXT( decl ) = current_function_decl; // could be NULL_TREE (top level) .. hmm. // hm.m.
    
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
    assert(current_binding_level);
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

#if D_GCC_VER < 40
const char
tree_code_type[] = {
#include "tree.def"
 'x',
#include "d/d-tree.def"
};

#else

const enum tree_code_class
tree_code_type[] = {
#include "tree.def"
 tcc_exceptional,
#include "d/d-tree.def"
};
#endif

#undef DEFTREECODE

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

#if D_GCC_VER >= 40 && D_GCC_VER < 43

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

#if D_GCC_VER >= 40
static int
d_types_compatible_p (tree t1, tree t2)
{
    tree d_va_list = NULL;

    /* Is compatible if types are equivalent */
    if (lhd_types_compatible_p (t1, t2))
	return 1;

    if (d_gcc_builtin_va_list_d_type)
	d_va_list = d_gcc_builtin_va_list_d_type->ctype;

    /* Is compatible if we are dealing with C <-> D va_list nodes */
    if ((t1 == d_va_list && t2 == va_list_type_node)
	|| (t2 == d_va_list && t1 == va_list_type_node))
	return 1;

    /* Is compatible if aggregates are same type and share the same
       attributes. The frontend should have already ensured that types
       aren't wildly different anyway... */
    if (AGGREGATE_TYPE_P (t1) && AGGREGATE_TYPE_P (t2)
	&& TREE_CODE (t1) == TREE_CODE (t2)
	&& TYPE_ATTRIBUTES (t1) == TYPE_ATTRIBUTES (t2))
	return 1;

    return 0;
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
    if (TREE_TYPE(parm) != TREE_TYPE(value))
	return build1(NOP_EXPR, TREE_TYPE(parm), value);
    else
	return value;
}
#endif
    
#endif


struct lang_type *
build_d_type_lang_specific(Type * t)
{
    struct lang_type * l = (struct lang_type *) ggc_alloc_cleared( sizeof(struct lang_type) );
    l->d_type = t;
    return l;
}

tree d_keep_list = NULL_TREE;

void
dkeep(tree t)
{
    d_keep_list = tree_cons(NULL_TREE, t, d_keep_list);
}


const struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;
