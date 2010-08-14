#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "flags.h"
#include "convert.h"
#include "cpplib.h"

/* Various functions referenced by $(C_TARGET_OBJS), $(CXX_TARGET_OBJS).
   We need to include these objects to get target-specific version
   symbols.  The objects also contain functions not needed by GDC and
   link against function in the C front end.  These definitions
   satisfy the link requirements, but should never be executed. */

typedef void iasm_md_extra_info;

int iasm_in_operands;
int iasm_state;

enum { unused } c_language;
const char *constant_string_class_name = NULL;
int flag_next_runtime;
int flag_iasm_blocks;
int flag_weak;

int disable_typechecking_for_spec_flag;

add_cpp_dir_path (cpp_dir *p, int chain)
{
    /* nothing */
}

void
add_path (char *path, int chain, int cxx_aware, bool user_supplied_p)
{
    /* nothing */
}

void
builtin_define_with_value (const char *macro, const char *expansion, int is_str)
{
    /* nothing */
}

tree
default_conversion (tree exp)
{
    return exp;
}

tree
build_function_call (tree function, tree params)
{
  tree fntype, fundecl = 0;
  tree name = NULL_TREE, result;
  /* Strip NON_LVALUE_EXPRs, etc., since we aren't using as an lvalue.  */
  STRIP_TYPE_NOPS (function);
  /* Convert anything with function type to a pointer-to-function.  */
  gcc_assert(TREE_CODE (function) == FUNCTION_DECL);
  if (TREE_CODE (function) == FUNCTION_DECL)
    {
      name = DECL_NAME (function);

      /* Differs from default_conversion by not setting TREE_ADDRESSABLE
	 (because calling an inline function does not mean the function
	 needs to be separately compiled).  */
      fntype = build_type_variant (TREE_TYPE (function),
				   TREE_READONLY (function),
				   TREE_THIS_VOLATILE (function));
      fundecl = function;
      function = build1 (ADDR_EXPR, build_pointer_type (fntype), function);
    }
  else
      function = default_conversion (function);
  /* APPLE LOCAL begin mainline */
  /* For Objective-C, convert any calls via a cast to OBJC_TYPE_REF
     expressions, like those used for ObjC messenger dispatches.  */
  function = objc_rewrite_function_call (function, params);

  /* APPLE LOCAL end mainline */
  fntype = TREE_TYPE (function);

  if (TREE_CODE (fntype) == ERROR_MARK)
    return error_mark_node;

  if (!(TREE_CODE (fntype) == POINTER_TYPE
	&& TREE_CODE (TREE_TYPE (fntype)) == FUNCTION_TYPE))
    {
      error ("called object %qE is not a function", function);
      return error_mark_node;
    }
  /* fntype now gets the type of function pointed to.  */
  fntype = TREE_TYPE (fntype);
  
  result = build3 (CALL_EXPR, TREE_TYPE (fntype),
		   function, params, NULL_TREE);
  TREE_SIDE_EFFECTS (result) = 1;

  result = fold (result);
  return result;
}

tree
build_modify_expr (tree lhs, enum tree_code modifycode, tree rhs)
{
    gcc_assert(modifycode == NOP_EXPR);
    tree result = build2(MODIFY_EXPR, TREE_TYPE(lhs), lhs, rhs);
    TREE_SIDE_EFFECTS (result) = 1;
    // objc write barrier...
    return result;
}

tree
c_common_type_for_mode (enum machine_mode mode, int unsignedp)
{
    gcc_assert(0);
}

tree
pointer_int_sum (enum tree_code resultcode, tree ptrop, tree intop)
{
    gcc_assert(0);
}

tree
decl_constant_value (tree decl)
{
    gcc_assert(0);
}

tree
lookup_name_two (tree name, int ARG_UNUSED (prefer_type))
{
    return NULL_TREE;
}

enum cpp_ttype
c_lex (tree *value)
{
    gcc_assert(0);
}

tree
iasm_addr (tree e)
{
    gcc_assert(0);
}

static enum machine_mode
iasm_get_mode (tree type)
{
    gcc_assert(0);
}

tree
iasm_ptr_conv (tree type, tree exp)
{
    gcc_assert(0);
}

tree
iasm_build_bracket (tree v1, tree v2)
{
    gcc_assert(0);
}

static tree
iasm_default_function_conversion (tree exp)
{
    gcc_assert(0);
}

bool
iasm_is_pseudo (const char *opcode)
{
    gcc_assert(0);
}

static int
iasm_op_comp (const void *a, const void *b)
{
    gcc_assert(0);
}

static const char*
iasm_constraint_for (const char *opcode, unsigned argnum, unsigned ARG_UNUSED (num_args))
{
    gcc_assert(0);
}

static void
iasm_process_arg (const char *opcodename, int op_num,
		  tree *outputsp, tree *inputsp, tree *uses, unsigned num_args,
		  iasm_md_extra_info *e)
{
    gcc_assert(0);
}

static tree
iasm_identifier (tree expr)
{
    gcc_assert(0);
}

bool
iasm_is_prefix (tree ARG_UNUSED (id))
{
    gcc_assert(0);
}

static int
iasm_num_constraints_1 (tree io)
{
    gcc_assert(0);
}

static int
iasm_num_constraints (tree inputs, tree outputs)
{
    gcc_assert(0);
}

static void
iasm_set_constraints_1 (int num, tree io)
{
    gcc_assert(0);
}

static void
iasm_set_constraints (int num, tree inputs, tree outputs)
{
    gcc_assert(0);
}

static int
iasm_op_clobber_comp (const void *a, const void *b)
{
    gcc_assert(0);
}

static void
iasm_extra_clobbers (const char *opcode, tree *clobbersp)
{
    gcc_assert(0);
}

tree
iasm_stmt (tree expr, tree args, int lineno)
{
    gcc_assert(0);
}

static int
iasm_field_offset (tree arg)
{
    gcc_assert(0);
}

static bool
iasm_simple_expr (tree arg)
{
    gcc_assert(0);
}

static int
iasm_expr_val (tree arg)
{
    gcc_assert(0);
}

void
iasm_force_constraint (const char *c, iasm_md_extra_info *e)
{
    gcc_assert(0);
}

static void
iasm_maybe_force_mem (tree arg, char *buf, unsigned argnum, bool must_be_reg, iasm_md_extra_info *e)
{
    gcc_assert(0);
}

void
iasm_print_operand (char *buf, tree arg, unsigned argnum,
		    tree *uses,
		    bool must_be_reg, bool must_not_be_reg, iasm_md_extra_info *e)
{
    gcc_assert(0);
}

void
iasm_get_register_var (tree var, const char *modifier, char *buf, unsigned argnum,
		       bool must_be_reg, iasm_md_extra_info *e)
{
    gcc_assert(0);
}

tree
iasm_reg_name (tree id)
{
    gcc_assert(0);
}

tree
iasm_label (tree labid, int atsign)
{
    gcc_assert(0);
}

tree
iasm_get_identifier (tree id, const char *str)
{
    gcc_assert(0);
}

void
iasm_clear_labels (void)
{
    gcc_assert(0);
}

tree
iasm_do_id (tree id)
{
    gcc_assert(0);
}

static tree
iasm_get_label (tree labid)
{
    gcc_assert(0);
}

tree
iasm_build_register_offset (tree offset, tree regname)
{
    gcc_assert(0);
}

tree
iasm_entry (tree keyword, tree scspec, tree fn)
{
    gcc_assert(0);
}
