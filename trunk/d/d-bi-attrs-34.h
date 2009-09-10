static tree handle_packed_attribute (tree *, tree, tree, int, bool *);
static tree handle_nocommon_attribute (tree *, tree, tree, int, bool *);
static tree handle_common_attribute (tree *, tree, tree, int, bool *);
static tree handle_noreturn_attribute (tree *, tree, tree, int, bool *);
static tree handle_noinline_attribute (tree *, tree, tree, int, bool *);
static tree handle_always_inline_attribute (tree *, tree, tree, int,
					    bool *);
static tree handle_used_attribute (tree *, tree, tree, int, bool *);
static tree handle_unused_attribute (tree *, tree, tree, int, bool *);
static tree handle_const_attribute (tree *, tree, tree, int, bool *);
static tree handle_transparent_union_attribute (tree *, tree, tree,
						int, bool *);
static tree handle_constructor_attribute (tree *, tree, tree, int, bool *);
static tree handle_destructor_attribute (tree *, tree, tree, int, bool *);
static tree handle_mode_attribute (tree *, tree, tree, int, bool *);
static tree handle_section_attribute (tree *, tree, tree, int, bool *);
static tree handle_aligned_attribute (tree *, tree, tree, int, bool *);
static tree handle_weak_attribute (tree *, tree, tree, int, bool *) ;
static tree handle_alias_attribute (tree *, tree, tree, int, bool *);
static tree handle_visibility_attribute (tree *, tree, tree, int,
					 bool *);
static tree handle_tls_model_attribute (tree *, tree, tree, int,
					bool *);
static tree handle_no_instrument_function_attribute (tree *, tree,
						     tree, int, bool *);
static tree handle_malloc_attribute (tree *, tree, tree, int, bool *);
static tree handle_no_limit_stack_attribute (tree *, tree, tree, int,
					     bool *);
static tree handle_pure_attribute (tree *, tree, tree, int, bool *);
static tree handle_deprecated_attribute (tree *, tree, tree, int,
					 bool *);
static tree handle_vector_size_attribute (tree *, tree, tree, int,
					  bool *);
static tree handle_nonnull_attribute (tree *, tree, tree, int, bool *);
static tree handle_nothrow_attribute (tree *, tree, tree, int, bool *);
/*not in gdc//static tree handle_cleanup_attribute (tree *, tree, tree, int, bool *);*/
static tree handle_warn_unused_result_attribute (tree *, tree, tree, int,
						 bool *);

/*not in gdc//static void check_function_nonnull (tree, tree);*/
static void check_nonnull_arg (void *, tree, unsigned HOST_WIDE_INT);
static bool nonnull_check_p (tree, unsigned HOST_WIDE_INT);
static bool get_nonnull_operand (tree, unsigned HOST_WIDE_INT *);

/* extra for gdc copy: */
static void check_function_arguments_recurse (void (*)
					      (void *, tree,
					       unsigned HOST_WIDE_INT),
					      void *, tree,
					      unsigned HOST_WIDE_INT);
static tree
handle_format_arg_attribute (tree *node ATTRIBUTE_UNUSED, tree name ATTRIBUTE_UNUSED,
			     tree args ATTRIBUTE_UNUSED, int flags ATTRIBUTE_UNUSED, bool *no_add_attrs ATTRIBUTE_UNUSED)
{
    return NULL_TREE;
}
static tree
handle_format_attribute (tree *node ATTRIBUTE_UNUSED, tree name ATTRIBUTE_UNUSED, tree args ATTRIBUTE_UNUSED,
			 int flags ATTRIBUTE_UNUSED, bool *no_add_attrs ATTRIBUTE_UNUSED)
{
    return NULL_TREE;
}

/* -- end extra */

/* Table of machine-independent attributes common to all C-like languages.  */
const struct attribute_spec d_common_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  { "packed",                 0, 0, false, false, false,
			      handle_packed_attribute },
  { "nocommon",               0, 0, true,  false, false,
			      handle_nocommon_attribute },
  { "common",                 0, 0, true,  false, false,
			      handle_common_attribute },
  /* FIXME: logically, noreturn attributes should be listed as
     "false, true, true" and apply to function types.  But implementing this
     would require all the places in the compiler that use TREE_THIS_VOLATILE
     on a decl to identify non-returning functions to be located and fixed
     to check the function type instead.  */
  { "noreturn",               0, 0, true,  false, false,
			      handle_noreturn_attribute },
  { "volatile",               0, 0, true,  false, false,
			      handle_noreturn_attribute },
  { "noinline",               0, 0, true,  false, false,
			      handle_noinline_attribute },
  { "always_inline",          0, 0, true,  false, false,
			      handle_always_inline_attribute },
  { "used",                   0, 0, true,  false, false,
			      handle_used_attribute },
  { "unused",                 0, 0, false, false, false,
			      handle_unused_attribute },
  /* The same comments as for noreturn attributes apply to const ones.  */
  { "const",                  0, 0, true,  false, false,
			      handle_const_attribute },
  { "transparent_union",      0, 0, false, false, false,
			      handle_transparent_union_attribute },
  { "constructor",            0, 0, true,  false, false,
			      handle_constructor_attribute },
  { "destructor",             0, 0, true,  false, false,
			      handle_destructor_attribute },
  { "mode",                   1, 1, false,  true, false,
			      handle_mode_attribute },
  { "section",                1, 1, true,  false, false,
			      handle_section_attribute },
  { "aligned",                0, 1, false, false, false,
			      handle_aligned_attribute },
  { "weak",                   0, 0, true,  false, false,
			      handle_weak_attribute },
  { "alias",                  1, 1, true,  false, false,
			      handle_alias_attribute },
  { "no_instrument_function", 0, 0, true,  false, false,
			      handle_no_instrument_function_attribute },
  { "malloc",                 0, 0, true,  false, false,
			      handle_malloc_attribute },
  { "no_stack_limit",         0, 0, true,  false, false,
			      handle_no_limit_stack_attribute },
  { "pure",                   0, 0, true,  false, false,
			      handle_pure_attribute },
  { "deprecated",             0, 0, false, false, false,
			      handle_deprecated_attribute },
  { "vector_size",	      1, 1, false, true, false,
			      handle_vector_size_attribute },
  { "visibility",	      1, 1, true,  false, false,
			      handle_visibility_attribute },
  { "tls_model",	      1, 1, true,  false, false,
			      handle_tls_model_attribute },
  { "nonnull",                0, -1, false, true, true,
			      handle_nonnull_attribute },
  { "nothrow",                0, 0, true,  false, false,
			      handle_nothrow_attribute },
  { "may_alias",	      0, 0, false, true, false, NULL },
  /* not in gdc//  { "cleanup",		      1, 1, true, false, false,
     handle_cleanup_attribute },*/
  { "warn_unused_result",     0, 0, false, true, true,
			      handle_warn_unused_result_attribute },
  { NULL,                     0, 0, false, false, false, NULL }
};

/* Give the specifications for the format attributes, used by C and all
   descendants.  */

const struct attribute_spec d_common_format_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  { "format",                 3, 3, false, true,  true,
			      handle_format_attribute },
  { "format_arg",             1, 1, false, true,  true,
			      handle_format_arg_attribute },
  { NULL,                     0, 0, false, false, false, NULL }
};

/* Attribute handlers common to C front ends.  */

/* Handle a "packed" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_packed_attribute (tree *node, tree name, tree args  ATTRIBUTE_UNUSED,
			 int flags, bool *no_add_attrs)
{
  if (TYPE_P (*node))
    {
      if (!(flags & (int) ATTR_FLAG_TYPE_IN_PLACE))
	*node = build_type_copy (*node);
      TYPE_PACKED (*node) = 1;
      if (TYPE_MAIN_VARIANT (*node) == *node)
	{
	  /* If it is the main variant, then pack the other variants
   	     too. This happens in,
	     
	     struct Foo {
	       struct Foo const *ptr; // creates a variant w/o packed flag
	       } __ attribute__((packed)); // packs it now.
	  */
	  tree probe;
	  
	  for (probe = *node; probe; probe = TYPE_NEXT_VARIANT (probe))
	    TYPE_PACKED (probe) = 1;
	}
      
    }
  else if (TREE_CODE (*node) == FIELD_DECL)
    DECL_PACKED (*node) = 1;
  /* We can't set DECL_PACKED for a VAR_DECL, because the bit is
     used for DECL_REGISTER.  It wouldn't mean anything anyway.
     We can't set DECL_PACKED on the type of a TYPE_DECL, because
     that changes what the typedef is typing.  */
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "nocommon" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_nocommon_attribute (tree *node, tree name,
			   tree args ATTRIBUTE_UNUSED,
			   int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == VAR_DECL)
    DECL_COMMON (*node) = 0;
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "common" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_common_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			 int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == VAR_DECL)
    DECL_COMMON (*node) = 1;
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "noreturn" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noreturn_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			   int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree type = TREE_TYPE (*node);

  /* See FIXME comment in c_common_attribute_table.  */
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_THIS_VOLATILE (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(build_type_variant (TREE_TYPE (type),
			     TREE_READONLY (TREE_TYPE (type)), 1));
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "noinline" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noinline_attribute (tree *node, tree name,
			   tree args ATTRIBUTE_UNUSED,
			   int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_UNINLINABLE (*node) = 1;
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "always_inline" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_always_inline_attribute (tree *node, tree name,
				tree args ATTRIBUTE_UNUSED,
				int flags ATTRIBUTE_UNUSED,
				bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    {
      /* Do nothing else, just set the attribute.  We'll get at
	 it later with lookup_attribute.  */
    }
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "used" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_used_attribute (tree *pnode, tree name, tree args ATTRIBUTE_UNUSED,
		       int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree node = *pnode;

  if (TREE_CODE (node) == FUNCTION_DECL
      || (TREE_CODE (node) == VAR_DECL && TREE_STATIC (node)))
    {
      TREE_USED (node) = 1;
    }
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "unused" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_unused_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			 int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (DECL_P (*node))
    {
      tree decl = *node;

      if (TREE_CODE (decl) == PARM_DECL
	  || TREE_CODE (decl) == VAR_DECL
	  || TREE_CODE (decl) == FUNCTION_DECL
	  || TREE_CODE (decl) == LABEL_DECL
	  || TREE_CODE (decl) == TYPE_DECL)
	TREE_USED (decl) = 1;
      else
	{
	  warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
	  *no_add_attrs = true;
	}
    }
  else
    {
      if (!(flags & (int) ATTR_FLAG_TYPE_IN_PLACE))
	*node = build_type_copy (*node);
      TREE_USED (*node) = 1;
    }

  return NULL_TREE;
}

/* Handle a "const" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_const_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree type = TREE_TYPE (*node);

  /* See FIXME comment on noreturn in c_common_attribute_table.  */
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_READONLY (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(build_type_variant (TREE_TYPE (type), 1,
			     TREE_THIS_VOLATILE (TREE_TYPE (type))));
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "transparent_union" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_transparent_union_attribute (tree *node, tree name,
				    tree args ATTRIBUTE_UNUSED, int flags,
				    bool *no_add_attrs)
{
  tree decl = NULL_TREE;
  tree *type = NULL;
  int is_type = 0;

  if (DECL_P (*node))
    {
      decl = *node;
      type = &TREE_TYPE (decl);
      is_type = TREE_CODE (*node) == TYPE_DECL;
    }
  else if (TYPE_P (*node))
    type = node, is_type = 1;

  if (is_type
      && TREE_CODE (*type) == UNION_TYPE
      && (decl == 0
	  || (TYPE_FIELDS (*type) != 0
	      && TYPE_MODE (*type) == DECL_MODE (TYPE_FIELDS (*type)))))
    {
      if (!(flags & (int) ATTR_FLAG_TYPE_IN_PLACE))
	*type = build_type_copy (*type);
      TYPE_TRANSPARENT_UNION (*type) = 1;
    }
  else if (decl != 0 && TREE_CODE (decl) == PARM_DECL
	   && TREE_CODE (*type) == UNION_TYPE
	   && TYPE_MODE (*type) == DECL_MODE (TYPE_FIELDS (*type)))
    DECL_TRANSPARENT_UNION (decl) = 1;
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "constructor" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_constructor_attribute (tree *node, tree name,
			      tree args ATTRIBUTE_UNUSED,
			      int flags ATTRIBUTE_UNUSED,
			      bool *no_add_attrs)
{
  tree decl = *node;
  tree type = TREE_TYPE (decl);

  if (TREE_CODE (decl) == FUNCTION_DECL
      && TREE_CODE (type) == FUNCTION_TYPE
      && decl_function_context (decl) == 0)
    {
      DECL_STATIC_CONSTRUCTOR (decl) = 1;
      TREE_USED (decl) = 1;
    }
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "destructor" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_destructor_attribute (tree *node, tree name,
			     tree args ATTRIBUTE_UNUSED,
			     int flags ATTRIBUTE_UNUSED,
			     bool *no_add_attrs)
{
  tree decl = *node;
  tree type = TREE_TYPE (decl);

  if (TREE_CODE (decl) == FUNCTION_DECL
      && TREE_CODE (type) == FUNCTION_TYPE
      && decl_function_context (decl) == 0)
    {
      DECL_STATIC_DESTRUCTOR (decl) = 1;
      TREE_USED (decl) = 1;
    }
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "mode" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_mode_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
		       int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree type = *node;

  *no_add_attrs = true;

  if (TREE_CODE (TREE_VALUE (args)) != IDENTIFIER_NODE)
    warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
  else
    {
      int j;
      const char *p = IDENTIFIER_POINTER (TREE_VALUE (args));
      int len = strlen (p);
      enum machine_mode mode = VOIDmode;
      tree typefm;
      tree ptr_type;

      if (len > 4 && p[0] == '_' && p[1] == '_'
	  && p[len - 1] == '_' && p[len - 2] == '_')
	{
	  char *newp = alloca (len - 1);

	  strcpy (newp, &p[2]);
	  newp[len - 4] = '\0';
	  p = newp;
	}

      /* Change this type to have a type with the specified mode.
	 First check for the special modes.  */
      if (! strcmp (p, "byte"))
	mode = byte_mode;
      else if (!strcmp (p, "word"))
	mode = word_mode;
      else if (! strcmp (p, "pointer"))
	mode = ptr_mode;
      else
	for (j = 0; j < NUM_MACHINE_MODES; j++)
	  if (!strcmp (p, GET_MODE_NAME (j)))
	    {
	      mode = (enum machine_mode) j;
	      break;
	    }

      if (mode == VOIDmode)
	error ("unknown machine mode `%s'", p);
      else if (0 == (typefm = (*lang_hooks.types.type_for_mode)
		     (mode, TREE_UNSIGNED (type))))
	error ("no data type for mode `%s'", p);
      else if ((TREE_CODE (type) == POINTER_TYPE
		|| TREE_CODE (type) == REFERENCE_TYPE)
	       && !(*targetm.valid_pointer_mode) (mode))
	error ("invalid pointer mode `%s'", p);
      else
	{
	  /* If this is a vector, make sure we either have hardware
	     support, or we can emulate it.  */
	  if (VECTOR_MODE_P (mode) && !vector_mode_valid_p (mode))
	    {
	      error ("unable to emulate '%s'", GET_MODE_NAME (mode));
	      return NULL_TREE;
	    }

	  if (TREE_CODE (type) == POINTER_TYPE)
	    {
	      ptr_type = build_pointer_type_for_mode (TREE_TYPE (type),
						      mode);
	      *node = ptr_type;
	    }
	  else if (TREE_CODE (type) == REFERENCE_TYPE)
	    {
	      ptr_type = build_reference_type_for_mode (TREE_TYPE (type),
							mode);
	      *node = ptr_type;
	    }
	  else if (VECTOR_MODE_P (mode)
		   ? TREE_CODE (type) != TREE_CODE (TREE_TYPE (typefm))
		   : TREE_CODE (type) != TREE_CODE (typefm))
		   
	    {
	      error ("mode `%s' applied to inappropriate type", p);
	      return NULL_TREE;
	    }
	  else
	    *node = typefm;

	  /* No need to layout the type here.  The caller should do this.  */
	}
    }

  return NULL_TREE;
}

/* Handle a "section" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_section_attribute (tree *node, tree name ATTRIBUTE_UNUSED, tree args,
			  int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;

  if (targetm.have_named_sections)
    {
      if ((TREE_CODE (decl) == FUNCTION_DECL
	   || TREE_CODE (decl) == VAR_DECL)
	  && TREE_CODE (TREE_VALUE (args)) == STRING_CST)
	{
	  if (TREE_CODE (decl) == VAR_DECL
	      && current_function_decl != NULL_TREE
	      && ! (TREE_STATIC (decl) || DECL_EXTERNAL (decl)))
	    {
	      error ("%Jsection attribute cannot be specified for "
                     "local variables", decl);
	      *no_add_attrs = true;
	    }

	  /* The decl may have already been given a section attribute
	     from a previous declaration.  Ensure they match.  */
	  else if (DECL_SECTION_NAME (decl) != NULL_TREE
		   && strcmp (TREE_STRING_POINTER (DECL_SECTION_NAME (decl)),
			      TREE_STRING_POINTER (TREE_VALUE (args))) != 0)
	    {
	      error ("%Jsection of '%D' conflicts with previous declaration",
                     *node, *node);
	      *no_add_attrs = true;
	    }
	  else
	    DECL_SECTION_NAME (decl) = TREE_VALUE (args);
	}
      else
	{
	  error ("%Jsection attribute not allowed for '%D'", *node, *node);
	  *no_add_attrs = true;
	}
    }
  else
    {
      error ("%Jsection attributes are not supported for this target", *node);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "aligned" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_aligned_attribute (tree *node, tree name ATTRIBUTE_UNUSED, tree args,
			  int flags, bool *no_add_attrs)
{
  tree decl = NULL_TREE;
  tree *type = NULL;
  int is_type = 0;
  tree align_expr = (args ? TREE_VALUE (args)
		     : size_int (BIGGEST_ALIGNMENT / BITS_PER_UNIT));
  int i;

  if (DECL_P (*node))
    {
      decl = *node;
      type = &TREE_TYPE (decl);
      is_type = TREE_CODE (*node) == TYPE_DECL;
    }
  else if (TYPE_P (*node))
    type = node, is_type = 1;

  /* Strip any NOPs of any kind.  */
  while (TREE_CODE (align_expr) == NOP_EXPR
	 || TREE_CODE (align_expr) == CONVERT_EXPR
	 || TREE_CODE (align_expr) == NON_LVALUE_EXPR)
    align_expr = TREE_OPERAND (align_expr, 0);

  if (TREE_CODE (align_expr) != INTEGER_CST)
    {
      error ("requested alignment is not a constant");
      *no_add_attrs = true;
    }
  else if ((i = tree_log2 (align_expr)) == -1)
    {
      error ("requested alignment is not a power of 2");
      *no_add_attrs = true;
    }
  else if (i > HOST_BITS_PER_INT - 2)
    {
      error ("requested alignment is too large");
      *no_add_attrs = true;
    }
  else if (is_type)
    {
      /* If we have a TYPE_DECL, then copy the type, so that we
	 don't accidentally modify a builtin type.  See pushdecl.  */
      if (decl && TREE_TYPE (decl) != error_mark_node
	  && DECL_ORIGINAL_TYPE (decl) == NULL_TREE)
	{
	  tree tt = TREE_TYPE (decl);
	  *type = build_type_copy (*type);
	  DECL_ORIGINAL_TYPE (decl) = tt;
	  TYPE_NAME (*type) = decl;
	  TREE_USED (*type) = TREE_USED (decl);
	  TREE_TYPE (decl) = *type;
	}
      else if (!(flags & (int) ATTR_FLAG_TYPE_IN_PLACE))
	*type = build_type_copy (*type);

      TYPE_ALIGN (*type) = (1 << i) * BITS_PER_UNIT;
      TYPE_USER_ALIGN (*type) = 1;
    }
  else if (TREE_CODE (decl) != VAR_DECL
	   && TREE_CODE (decl) != FIELD_DECL)
    {
      error ("%Jalignment may not be specified for '%D'", decl, decl);
      *no_add_attrs = true;
    }
  else
    {
      DECL_ALIGN (decl) = (1 << i) * BITS_PER_UNIT;
      DECL_USER_ALIGN (decl) = 1;
    }

  return NULL_TREE;
}

/* Handle a "weak" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_weak_attribute (tree *node, tree name ATTRIBUTE_UNUSED,
		       tree args ATTRIBUTE_UNUSED,
		       int flags ATTRIBUTE_UNUSED,
		       bool *no_add_attrs ATTRIBUTE_UNUSED)
{
  declare_weak (*node);

  return NULL_TREE;
}

/* Handle an "alias" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_alias_attribute (tree *node, tree name, tree args,
			int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;

  if ((TREE_CODE (decl) == FUNCTION_DECL && DECL_INITIAL (decl))
      || (TREE_CODE (decl) != FUNCTION_DECL && ! DECL_EXTERNAL (decl)))
    {
      error ("%J'%D' defined both normally and as an alias", decl, decl);
      *no_add_attrs = true;
    }
  else if (decl_function_context (decl) == 0)
    {
      tree id;

      id = TREE_VALUE (args);
      if (TREE_CODE (id) != STRING_CST)
	{
	  error ("alias arg not a string");
	  *no_add_attrs = true;
	  return NULL_TREE;
	}
      id = get_identifier (TREE_STRING_POINTER (id));
      /* This counts as a use of the object pointed to.  */
      TREE_USED (id) = 1;

      if (TREE_CODE (decl) == FUNCTION_DECL)
	DECL_INITIAL (decl) = error_mark_node;
      else
	DECL_EXTERNAL (decl) = 0;
    }
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle an "visibility" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_visibility_attribute (tree *node, tree name, tree args,
			     int flags ATTRIBUTE_UNUSED,
			     bool *no_add_attrs)
{
  tree decl = *node;
  tree id = TREE_VALUE (args);

  *no_add_attrs = true;

  if (decl_function_context (decl) != 0 || ! TREE_PUBLIC (decl))
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      return NULL_TREE;
    }

  if (TREE_CODE (id) != STRING_CST)
    {
      error ("visibility arg not a string");
      return NULL_TREE;
    }

  if (strcmp (TREE_STRING_POINTER (id), "default") == 0)
    DECL_VISIBILITY (decl) = VISIBILITY_DEFAULT;
  else if (strcmp (TREE_STRING_POINTER (id), "internal") == 0)
    DECL_VISIBILITY (decl) = VISIBILITY_INTERNAL;
  else if (strcmp (TREE_STRING_POINTER (id), "hidden") == 0)
    DECL_VISIBILITY (decl) = VISIBILITY_HIDDEN;  
  else if (strcmp (TREE_STRING_POINTER (id), "protected") == 0)
    DECL_VISIBILITY (decl) = VISIBILITY_PROTECTED;
  else
    error ("visibility arg must be one of \"default\", \"hidden\", \"protected\" or \"internal\"");

  return NULL_TREE;
}

/* Handle an "tls_model" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_tls_model_attribute (tree *node, tree name, tree args,
			    int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;

  if (! DECL_THREAD_LOCAL (decl))
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }
  else
    {
      tree id;

      id = TREE_VALUE (args);
      if (TREE_CODE (id) != STRING_CST)
	{
	  error ("tls_model arg not a string");
	  *no_add_attrs = true;
	  return NULL_TREE;
	}
      if (strcmp (TREE_STRING_POINTER (id), "local-exec")
	  && strcmp (TREE_STRING_POINTER (id), "initial-exec")
	  && strcmp (TREE_STRING_POINTER (id), "local-dynamic")
	  && strcmp (TREE_STRING_POINTER (id), "global-dynamic"))
	{
	  error ("tls_model arg must be one of \"local-exec\", \"initial-exec\", \"local-dynamic\" or \"global-dynamic\"");
	  *no_add_attrs = true;
	  return NULL_TREE;
	}
    }

  return NULL_TREE;
}

/* Handle a "no_instrument_function" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_no_instrument_function_attribute (tree *node, tree name,
					 tree args ATTRIBUTE_UNUSED,
					 int flags ATTRIBUTE_UNUSED,
					 bool *no_add_attrs)
{
  tree decl = *node;

  if (TREE_CODE (decl) != FUNCTION_DECL)
    {
      error ("%J'%E' attribute applies only to functions", decl, name);
      *no_add_attrs = true;
    }
  else if (DECL_INITIAL (decl))
    {
      error ("%Jcan't set '%E' attribute after definition", decl, name);
      *no_add_attrs = true;
    }
  else
    DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT (decl) = 1;

  return NULL_TREE;
}

/* Handle a "malloc" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_malloc_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			 int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_IS_MALLOC (*node) = 1;
  /* ??? TODO: Support types.  */
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "no_limit_stack" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_no_limit_stack_attribute (tree *node, tree name,
				 tree args ATTRIBUTE_UNUSED,
				 int flags ATTRIBUTE_UNUSED,
				 bool *no_add_attrs)
{
  tree decl = *node;

  if (TREE_CODE (decl) != FUNCTION_DECL)
    {
      error ("%J'%E' attribute applies only to functions", decl, name);
      *no_add_attrs = true;
    }
  else if (DECL_INITIAL (decl))
    {
      error ("%Jcan't set '%E' attribute after definition", decl, name);
      *no_add_attrs = true;
    }
  else
    DECL_NO_LIMIT_STACK (decl) = 1;

  return NULL_TREE;
}

/* Handle a "pure" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_pure_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
		       int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_IS_PURE (*node) = 1;
  /* ??? TODO: Support types.  */
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle a "deprecated" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_deprecated_attribute (tree *node, tree name,
			     tree args ATTRIBUTE_UNUSED, int flags,
			     bool *no_add_attrs)
{
  tree type = NULL_TREE;
  int warn = 0;
  const char *what = NULL;

  if (DECL_P (*node))
    {
      tree decl = *node;
      type = TREE_TYPE (decl);

      if (TREE_CODE (decl) == TYPE_DECL
	  || TREE_CODE (decl) == PARM_DECL
	  || TREE_CODE (decl) == VAR_DECL
	  || TREE_CODE (decl) == FUNCTION_DECL
	  || TREE_CODE (decl) == FIELD_DECL)
	TREE_DEPRECATED (decl) = 1;
      else
	warn = 1;
    }
  else if (TYPE_P (*node))
    {
      if (!(flags & (int) ATTR_FLAG_TYPE_IN_PLACE))
	*node = build_type_copy (*node);
      TREE_DEPRECATED (*node) = 1;
      type = *node;
    }
  else
    warn = 1;

  if (warn)
    {
      *no_add_attrs = true;
      if (type && TYPE_NAME (type))
	{
	  if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
	    what = IDENTIFIER_POINTER (TYPE_NAME (*node));
	  else if (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
		   && DECL_NAME (TYPE_NAME (type)))
	    what = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (type)));
	}
      if (what)
	warning ("`%s' attribute ignored for `%s'",
		  IDENTIFIER_POINTER (name), what);
      else
	warning ("`%s' attribute ignored",
		      IDENTIFIER_POINTER (name));
    }

  return NULL_TREE;
}

/* Keep a list of vector type nodes we created in handle_vector_size_attribute,
   to prevent us from duplicating type nodes unnecessarily.
   The normal mechanism to prevent duplicates is to use type_hash_canon, but
   since we want to distinguish types that are essentially identical (except
   for their debug representation), we use a local list here.  */
static GTY(()) tree vector_type_node_list = 0;

/* Handle a "vector_size" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_vector_size_attribute (tree *node, tree name, tree args,
			      int flags ATTRIBUTE_UNUSED,
			      bool *no_add_attrs)
{
  unsigned HOST_WIDE_INT vecsize, nunits;
  enum machine_mode mode, orig_mode, new_mode;
  tree type = *node, new_type = NULL_TREE;
  tree type_list_node;

  *no_add_attrs = true;

  if (! host_integerp (TREE_VALUE (args), 1))
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      return NULL_TREE;
    }

  /* Get the vector size (in bytes).  */
  vecsize = tree_low_cst (TREE_VALUE (args), 1);

  /* We need to provide for vector pointers, vector arrays, and
     functions returning vectors.  For example:

       __attribute__((vector_size(16))) short *foo;

     In this case, the mode is SI, but the type being modified is
     HI, so we need to look further.  */

  while (POINTER_TYPE_P (type)
	 || TREE_CODE (type) == FUNCTION_TYPE
	 || TREE_CODE (type) == METHOD_TYPE
	 || TREE_CODE (type) == ARRAY_TYPE)
    type = TREE_TYPE (type);

  /* Get the mode of the type being modified.  */
  orig_mode = TYPE_MODE (type);

  if (TREE_CODE (type) == RECORD_TYPE
      || (GET_MODE_CLASS (orig_mode) != MODE_FLOAT
	  && GET_MODE_CLASS (orig_mode) != MODE_INT)
      || ! host_integerp (TYPE_SIZE_UNIT (type), 1))
    {
      error ("invalid vector type for attribute `%s'",
	     IDENTIFIER_POINTER (name));
      return NULL_TREE;
    }

  /* Calculate how many units fit in the vector.  */
  nunits = vecsize / tree_low_cst (TYPE_SIZE_UNIT (type), 1);

  /* Find a suitably sized vector.  */
  new_mode = VOIDmode;
  for (mode = GET_CLASS_NARROWEST_MODE (GET_MODE_CLASS (orig_mode) == MODE_INT
					? MODE_VECTOR_INT
					: MODE_VECTOR_FLOAT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    if (vecsize == GET_MODE_SIZE (mode)
	&& nunits == (unsigned HOST_WIDE_INT) GET_MODE_NUNITS (mode))
      {
	new_mode = mode;
	break;
      }

    if (new_mode == VOIDmode)
    {
      error ("no vector mode with the size and type specified could be found");
      return NULL_TREE;
    }

  for (type_list_node = vector_type_node_list; type_list_node;
       type_list_node = TREE_CHAIN (type_list_node))
    {
      tree other_type = TREE_VALUE (type_list_node);
      tree record = TYPE_DEBUG_REPRESENTATION_TYPE (other_type);
      tree fields = TYPE_FIELDS (record);
      tree field_type = TREE_TYPE (fields);
      tree array_type = TREE_TYPE (field_type);
      if (TREE_CODE (fields) != FIELD_DECL
	  || TREE_CODE (field_type) != ARRAY_TYPE)
	abort ();

      if (TYPE_MODE (other_type) == mode && type == array_type)
	{
	  new_type = other_type;
	  break;
	}
    }

  if (new_type == NULL_TREE)
    {
      tree index, array, rt, list_node;

      new_type = (*lang_hooks.types.type_for_mode) (new_mode,
						    TREE_UNSIGNED (type));

      if (!new_type)
	{
	  error ("no vector mode with the size and type specified could be found");
	  return NULL_TREE;
	}

      new_type = build_type_copy (new_type);

      /* If this is a vector, make sure we either have hardware
         support, or we can emulate it.  */
      if ((GET_MODE_CLASS (mode) == MODE_VECTOR_INT
	   || GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT)
	  && !vector_mode_valid_p (mode))
	{
	  error ("unable to emulate '%s'", GET_MODE_NAME (mode));
	  return NULL_TREE;
	}

      /* Set the debug information here, because this is the only
	 place where we know the underlying type for a vector made
	 with vector_size.  For debugging purposes we pretend a vector
	 is an array within a structure.  */
      index = build_int_2 (TYPE_VECTOR_SUBPARTS (new_type) - 1, 0);
      array = build_array_type (type, build_index_type (index));
      rt = make_node (RECORD_TYPE);

      TYPE_FIELDS (rt) = build_decl (FIELD_DECL, get_identifier ("f"), array);
      DECL_CONTEXT (TYPE_FIELDS (rt)) = rt;
      layout_type (rt);
      TYPE_DEBUG_REPRESENTATION_TYPE (new_type) = rt;

      list_node = build_tree_list (NULL, new_type);
      TREE_CHAIN (list_node) = vector_type_node_list;
      vector_type_node_list = list_node;
    }

  /* Build back pointers if needed.  */
  *node = reconstruct_complex_type (*node, new_type);

  return NULL_TREE;
}

/* Handle the "nonnull" attribute.  */
static tree
handle_nonnull_attribute (tree *node, tree name ATTRIBUTE_UNUSED,
			  tree args, int flags ATTRIBUTE_UNUSED,
			  bool *no_add_attrs)
{
  tree type = *node;
  unsigned HOST_WIDE_INT attr_arg_num;

  /* If no arguments are specified, all pointer arguments should be
     non-null.  Verify a full prototype is given so that the arguments
     will have the correct types when we actually check them later.  */
  if (! args)
    {
      if (! TYPE_ARG_TYPES (type))
	{
	  error ("nonnull attribute without arguments on a non-prototype");
          *no_add_attrs = true;
	}
      return NULL_TREE;
    }

  /* Argument list specified.  Verify that each argument number references
     a pointer argument.  */
  for (attr_arg_num = 1; args; args = TREE_CHAIN (args))
    {
      tree argument;
      unsigned HOST_WIDE_INT arg_num, ck_num;

      if (! get_nonnull_operand (TREE_VALUE (args), &arg_num))
	{
	  error ("nonnull argument has invalid operand number (arg %lu)",
		 (unsigned long) attr_arg_num);
	  *no_add_attrs = true;
	  return NULL_TREE;
	}

      argument = TYPE_ARG_TYPES (type);
      if (argument)
	{
	  for (ck_num = 1; ; ck_num++)
	    {
	      if (! argument || ck_num == arg_num)
		break;
	      argument = TREE_CHAIN (argument);
	    }

          if (! argument
	      || TREE_CODE (TREE_VALUE (argument)) == VOID_TYPE)
	    {
	      error ("nonnull argument with out-of-range operand number (arg %lu, operand %lu)",
		     (unsigned long) attr_arg_num, (unsigned long) arg_num);
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }

          if (TREE_CODE (TREE_VALUE (argument)) != POINTER_TYPE)
	    {
	      error ("nonnull argument references non-pointer operand (arg %lu, operand %lu)",
		   (unsigned long) attr_arg_num, (unsigned long) arg_num);
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }
	}
    }

  return NULL_TREE;
}

#if 0
/* not in gdc */
/* Check the argument list of a function call for null in argument slots
   that are marked as requiring a non-null pointer argument.  */

static void
check_function_nonnull (tree attrs, tree params)
{
  tree a, args, param;
  int param_num;

  for (a = attrs; a; a = TREE_CHAIN (a))
    {
      if (is_attribute_p ("nonnull", TREE_PURPOSE (a)))
	{
          args = TREE_VALUE (a);

          /* Walk the argument list.  If we encounter an argument number we
             should check for non-null, do it.  If the attribute has no args,
             then every pointer argument is checked (in which case the check
	     for pointer type is done in check_nonnull_arg).  */
          for (param = params, param_num = 1; ;
               param_num++, param = TREE_CHAIN (param))
            {
              if (! param)
	break;
              if (! args || nonnull_check_p (args, param_num))
	check_function_arguments_recurse (check_nonnull_arg, NULL,
					  TREE_VALUE (param),
					  param_num);
            }
	}
    }
}
#endif

/* Helper for check_function_nonnull; given a list of operands which
   must be non-null in ARGS, determine if operand PARAM_NUM should be
   checked.  */

static bool
nonnull_check_p (tree args, unsigned HOST_WIDE_INT param_num)
{
  unsigned HOST_WIDE_INT arg_num;

  for (; args; args = TREE_CHAIN (args))
    {
      if (! get_nonnull_operand (TREE_VALUE (args), &arg_num))
        abort ();

      if (arg_num == param_num)
	return true;
    }
  return false;
}

/* Check that the function argument PARAM (which is operand number
   PARAM_NUM) is non-null.  This is called by check_function_nonnull
   via check_function_arguments_recurse.  */

static void
check_nonnull_arg (void *ctx ATTRIBUTE_UNUSED, tree param,
		   unsigned HOST_WIDE_INT param_num)
{
  /* Just skip checking the argument if it's not a pointer.  This can
     happen if the "nonnull" attribute was given without an operand
     list (which means to check every pointer argument).  */

  if (TREE_CODE (TREE_TYPE (param)) != POINTER_TYPE)
    return;

  if (integer_zerop (param))
    warning ("null argument where non-null required (arg %lu)",
             (unsigned long) param_num);
}

/* Helper for nonnull attribute handling; fetch the operand number
   from the attribute argument list.  */

static bool
get_nonnull_operand (tree arg_num_expr, unsigned HOST_WIDE_INT *valp)
{
  /* Strip any conversions from the arg number and verify they
     are constants.  */
  while (TREE_CODE (arg_num_expr) == NOP_EXPR
	 || TREE_CODE (arg_num_expr) == CONVERT_EXPR
	 || TREE_CODE (arg_num_expr) == NON_LVALUE_EXPR)
    arg_num_expr = TREE_OPERAND (arg_num_expr, 0);

  if (TREE_CODE (arg_num_expr) != INTEGER_CST
      || TREE_INT_CST_HIGH (arg_num_expr) != 0)
    return false;

  *valp = TREE_INT_CST_LOW (arg_num_expr);
  return true;
}

/* Handle a "nothrow" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_nothrow_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			  int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_NOTHROW (*node) = 1;
  /* ??? TODO: Support types.  */
  else
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

#if 0
/* not in gdc */
/* Handle a "cleanup" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_cleanup_attribute (tree *node, tree name, tree args,
			  int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;
  tree cleanup_id, cleanup_decl;

  /* ??? Could perhaps support cleanups on TREE_STATIC, much like we do
     for global destructors in C++.  This requires infrastructure that
     we don't have generically at the moment.  It's also not a feature
     we'd be missing too much, since we do have attribute constructor.  */
  if (TREE_CODE (decl) != VAR_DECL || TREE_STATIC (decl))
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
      return NULL_TREE;
    }

  /* Verify that the argument is a function in scope.  */
  /* ??? We could support pointers to functions here as well, if
     that was considered desirable.  */
  cleanup_id = TREE_VALUE (args);
  if (TREE_CODE (cleanup_id) != IDENTIFIER_NODE)
    {
      error ("cleanup arg not an identifier");
      *no_add_attrs = true;
      return NULL_TREE;
    }
  cleanup_decl = lookup_name (cleanup_id);
  if (!cleanup_decl || TREE_CODE (cleanup_decl) != FUNCTION_DECL)
    {
      error ("cleanup arg not a function");
      *no_add_attrs = true;
      return NULL_TREE;
    }

  /* That the function has proper type is checked with the
     eventual call to build_function_call.  */

  return NULL_TREE;
}
#endif

/* Handle a "warn_unused_result" attribute.  No special handling.  */

static tree
handle_warn_unused_result_attribute (tree *node, tree name,
			       tree args ATTRIBUTE_UNUSED,
			       int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  /* Ignore the attribute for functions not returning any value.  */
  if (VOID_TYPE_P (TREE_TYPE (*node)))
    {
      warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

#if 0
/* not in gdc */
/* Check for valid arguments being passed to a function.  */
static void
check_function_arguments (tree attrs, tree params)
{
  /* Check for null being passed in a pointer argument that must be
     non-null.  We also need to do this if format checking is enabled.  */

  if (warn_nonnull)
    check_function_nonnull (attrs, params);

  /* Check for errors in format strings.  */

  if (warn_format)
    check_function_format (NULL, attrs, params);
}
#endif

/* Generic argument checking recursion routine.  PARAM is the argument to
   be checked.  PARAM_NUM is the number of the argument.  CALLBACK is invoked
   once the argument is resolved.  CTX is context for the callback.  */
void
check_function_arguments_recurse (void (*callback)
				  (void *, tree, unsigned HOST_WIDE_INT),
				  void *ctx, tree param,
				  unsigned HOST_WIDE_INT param_num)
{
  if (TREE_CODE (param) == NOP_EXPR)
    {
      /* Strip coercion.  */
      check_function_arguments_recurse (callback, ctx,
				        TREE_OPERAND (param, 0), param_num);
      return;
    }

  if (TREE_CODE (param) == CALL_EXPR)
    {
      tree type = TREE_TYPE (TREE_TYPE (TREE_OPERAND (param, 0)));
      tree attrs;
      bool found_format_arg = false;

      /* See if this is a call to a known internationalization function
	 that modifies a format arg.  Such a function may have multiple
	 format_arg attributes (for example, ngettext).  */

      for (attrs = TYPE_ATTRIBUTES (type);
	   attrs;
	   attrs = TREE_CHAIN (attrs))
	if (is_attribute_p ("format_arg", TREE_PURPOSE (attrs)))
	  {
	    tree inner_args;
	    tree format_num_expr;
	    int format_num;
	    int i;

	    /* Extract the argument number, which was previously checked
	       to be valid.  */
	    format_num_expr = TREE_VALUE (TREE_VALUE (attrs));
	    while (TREE_CODE (format_num_expr) == NOP_EXPR
		   || TREE_CODE (format_num_expr) == CONVERT_EXPR
		   || TREE_CODE (format_num_expr) == NON_LVALUE_EXPR)
	      format_num_expr = TREE_OPERAND (format_num_expr, 0);

	    if (TREE_CODE (format_num_expr) != INTEGER_CST
		|| TREE_INT_CST_HIGH (format_num_expr) != 0)
	      abort ();

	    format_num = TREE_INT_CST_LOW (format_num_expr);

	    for (inner_args = TREE_OPERAND (param, 1), i = 1;
		 inner_args != 0;
		 inner_args = TREE_CHAIN (inner_args), i++)
	      if (i == format_num)
		{
		  check_function_arguments_recurse (callback, ctx,
						    TREE_VALUE (inner_args),
						    param_num);
		  found_format_arg = true;
		  break;
		}
	  }

      /* If we found a format_arg attribute and did a recursive check,
	 we are done with checking this argument.  Otherwise, we continue
	 and this will be considered a non-literal.  */
      if (found_format_arg)
	return;
    }

  if (TREE_CODE (param) == COND_EXPR)
    {
      /* Check both halves of the conditional expression.  */
      check_function_arguments_recurse (callback, ctx,
				        TREE_OPERAND (param, 1), param_num);
      check_function_arguments_recurse (callback, ctx,
				        TREE_OPERAND (param, 2), param_num);
      return;
    }

  (*callback) (ctx, param, param_num);
}
