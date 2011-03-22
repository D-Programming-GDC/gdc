--- gcc.orig/cgraph.c	2010-07-01 12:03:31.000000000 +0100
+++ gcc/cgraph.c	2011-03-21 18:37:34.308055890 +0000
@@ -464,6 +464,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -485,11 +486,15 @@ cgraph_node (tree decl)
   node = cgraph_create_node ();
   node->decl = decl;
   *slot = node;
-  if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
+  if (DECL_STATIC_CHAIN (decl))
     {
-      node->origin = cgraph_node (DECL_CONTEXT (decl));
-      node->next_nested = node->origin->nested;
-      node->origin->nested = node;
+      context = decl_function_context (decl);
+      if (context)
+	{
+	  node->origin = cgraph_node (context);
+	  node->next_nested = node->origin->nested;
+	  node->origin->nested = node;
+	}
     }
   if (assembler_name_hash)
     {
--- gcc.orig/config/i386/i386.c	2010-09-30 21:24:54.000000000 +0100
+++ gcc/config/i386/i386.c	2011-03-21 19:13:59.450891425 +0000
@@ -4466,6 +4466,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("fastcall and stdcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("fastcall and optlink attributes are not compatible");
+	}
       if (lookup_attribute ("regparm", TYPE_ATTRIBUTES (*node)))
         {
 	  error ("fastcall and regparm attributes are not compatible");
@@ -4484,6 +4488,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -4497,6 +4505,27 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("fastcall and cdecl attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and cdecl attributes are not compatible");
+	}
+    }
+
+  /* Can combine optlink with regparm and sseregparm.  */
+  else if (is_attribute_p ("optlink", name))
+    {
+      if (lookup_attribute ("cdecl", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and cdecl attributes are not compatible");
+	}
+      if (lookup_attribute ("fastcall", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and fastcall attributes are not compatible");
+	}
+      if (lookup_attribute ("stdcall", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and stdcall attributes are not compatible");
+	}
     }
 
   /* Can combine sseregparm with all attributes.  */
@@ -4703,6 +4732,12 @@ ix86_return_pops_args (tree fundecl, tre
           || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
 	rtd = 1;
 
+      /* Optlink functions will pop the stack if floating-point return
+         and if not variable args.  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+          && X87_FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+	rtd = 1;
+
       if (rtd && ! stdarg_p (funtype))
 	return size;
     }
@@ -8131,6 +8166,10 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+    /* As above, skip return address */
+    frame->stack_pointer_offset = UNITS_PER_WORD;
 }
 
 /* Emit code to save registers in the prologue.  */
@@ -26366,7 +26405,7 @@ x86_output_mi_thunk (FILE *file,
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -28996,6 +29035,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2010-04-27 21:14:19.000000000 +0100
+++ gcc/config/i386/i386.h	2011-03-21 18:40:34.116947508 +0000
@@ -1581,6 +1581,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_avx;			/* True when we want to warn about AVX ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2010-11-17 06:09:53.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2011-03-21 18:37:34.684057757 +0000
@@ -20365,6 +20365,7 @@ rs6000_output_function_epilogue (FILE *f
 	 a number, so for now use 9.  LTO isn't assigned a number either,
 	 so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D")
 	  || ! strcmp (language_string, "GNU GIMPLE"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
--- gcc.orig/dojump.c	2010-02-19 18:19:06.000000000 +0000
+++ gcc/dojump.c	2011-03-21 18:37:34.852058580 +0000
@@ -80,7 +80,8 @@ void
 clear_pending_stack_adjust (void)
 {
   if (optimize > 0
-      && (! flag_omit_frame_pointer || cfun->calls_alloca)
+      && ((! flag_omit_frame_pointer && ! cfun->naked)
+          || cfun->calls_alloca)
       && EXIT_IGNORE_STACK)
     discard_pending_stack_adjust ();
 }
--- gcc.orig/dwarf2out.c	2010-12-07 15:12:45.000000000 +0000
+++ gcc/dwarf2out.c	2011-03-21 18:37:35.032059476 +0000
@@ -18729,6 +18729,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
@@ -19967,7 +19969,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/expr.c	2010-09-21 15:18:34.000000000 +0100
+++ gcc/expr.c	2011-03-21 18:37:35.080059715 +0000
@@ -9625,6 +9625,11 @@ expand_expr_real_1 (tree exp, rtx target
       /* Lowered by gimplify.c.  */
       gcc_unreachable ();
 
+    case STATIC_CHAIN_EXPR:
+    case STATIC_CHAIN_DECL:
+      /* Lowered by tree-nested.c */
+      gcc_unreachable ();
+
     case FDESC_EXPR:
       /* Function descriptors are not valid except for as
 	 initialization constants, and should not be expanded.  */
--- gcc.orig/function.c	2010-08-16 21:18:08.000000000 +0100
+++ gcc/function.c	2011-03-21 18:37:35.096059805 +0000
@@ -3202,7 +3202,8 @@ assign_parms (tree fndecl)
       FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
 			    data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3219,7 +3220,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.first_conversion_insn);
+  if (!cfun->naked)
+    emit_insn (all.first_conversion_insn);
 
   /* Estimate reload stack alignment from scalar return mode.  */
   if (SUPPORTS_STACK_ALIGNMENT)
@@ -3373,6 +3375,9 @@ gimplify_parameters (void)
   VEC(tree, heap) *fnargs;
   unsigned i;
 
+  if (cfun->naked)
+    return NULL;
+
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -4472,11 +4477,15 @@ expand_function_start (tree subr)
       local = gen_reg_rtx (Pmode);
       chain = targetm.calls.static_chain (current_function_decl, true);
 
-      set_decl_incoming_rtl (parm, chain, false);
       SET_DECL_RTL (parm, local);
       mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
 
-      insn = emit_move_insn (local, chain);
+      if (!cfun->custom_static_chain)
+	{
+	  set_decl_incoming_rtl (parm, chain, false);
+	  insn = emit_move_insn (local, chain);
+	}
+      /* else, the static chain will be set in the main body */
 
       /* Mark the register as eliminable, similar to parameters.  */
       if (MEM_P (chain)
@@ -5015,6 +5024,9 @@ thread_prologue_and_epilogue_insns (void
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+    return;
+
   rtl_profile_for_bb (ENTRY_BLOCK_PTR);
 #ifdef HAVE_prologue
   if (HAVE_prologue)
--- gcc.orig/function.h	2009-11-25 10:55:54.000000000 +0000
+++ gcc/function.h	2011-03-21 18:37:35.104059824 +0000
@@ -596,6 +596,14 @@ struct GTY(()) function {
      adjusts one of its arguments and forwards to another
      function.  */
   unsigned int is_thunk : 1;
+
+  /* Nonzero if static chain is initialized by something other than
+     static_chain_incoming_rtx. */
+  unsigned int custom_static_chain : 1;
+
+  /* Nonzero if no code should be generated for prologues, copying
+     parameters, etc. */
+  unsigned int naked : 1;
 };
 
 /* If va_list_[gf]pr_size is set to this, it means we don't know how
--- gcc.orig/gcc.c	2010-04-18 18:46:08.000000000 +0100
+++ gcc/gcc.c	2011-03-21 18:37:35.116059904 +0000
@@ -139,6 +139,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -515,6 +518,7 @@ or with constant text in a single argume
 	assembler has done its job.
  %D	Dump out a -L option for each directory in startfile_prefixes.
 	If multilib_dir is set, extra entries are generated with it affixed.
+ %N	Output the currently selected multilib directory name
  %l     process LINK_SPEC as a spec.
  %L     process LIB_SPEC as a spec.
  %G     process LIBGCC_SPEC as a spec.
@@ -4282,6 +4286,9 @@ process_command (int argc, const char **
       save_temps_prefix = NULL;
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4646,6 +4653,18 @@ process_command (int argc, const char **
       infiles[0].name   = "help-dummy";
     }
 
+  if (need_pthreads)
+    {
+      switches[n_switches].part1 = "pthread";
+      switches[n_switches].args = 0;
+      switches[n_switches].live_cond = 0;
+      /* Do not print an error if there is not expansion for -pthread. */
+      switches[n_switches].validated = 1;
+      switches[n_switches].ordering = 0;
+
+      n_switches++;
+    }
+
   switches[n_switches].part1 = 0;
   infiles[n_infiles].name = 0;
 }
@@ -5809,6 +5828,17 @@ do_spec_1 (const char *spec, int inswitc
 	      return value;
 	    break;
 
+	  case 'N':
+	    if (multilib_dir)
+	      {
+		arg_going = 1;
+		obstack_grow (&obstack, "-fmultilib-dir=",
+			      strlen ("-fmultilib-dir="));
+		obstack_grow (&obstack, multilib_dir,
+			      strlen (multilib_dir));
+	      }
+	    break;
+
 	    /* Here we define characters other than letters and digits.  */
 
 	  case '{':
--- gcc.orig/gimple.c	2010-06-22 19:23:11.000000000 +0100
+++ gcc/gimple.c	2011-03-21 18:37:35.124059923 +0000
@@ -2404,6 +2404,8 @@ get_gimple_rhs_num_ops (enum tree_code c
       || (SYM) == POLYNOMIAL_CHREC					    \
       || (SYM) == DOT_PROD_EXPR						    \
       || (SYM) == VEC_COND_EXPR						    \
+      || (SYM) == STATIC_CHAIN_DECL					    \
+      || (SYM) == STATIC_CHAIN_EXPR /* not sure if this is right...*/	    \
       || (SYM) == REALIGN_LOAD_EXPR) ? GIMPLE_SINGLE_RHS		    \
    : GIMPLE_INVALID_RHS),
 #define END_OF_BASE_TREE_CODES (unsigned char) GIMPLE_INVALID_RHS,
@@ -2460,7 +2462,10 @@ is_gimple_lvalue (tree t)
 	  || TREE_CODE (t) == WITH_SIZE_EXPR
 	  /* These are complex lvalues, but don't have addresses, so they
 	     go here.  */
-	  || TREE_CODE (t) == BIT_FIELD_REF);
+	  || TREE_CODE (t) == BIT_FIELD_REF
+	  /* This is an lvalue because it will be replaced with the real
+	     static chain decl. */
+	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
 }
 
 /*  Return true if T is a GIMPLE condition.  */
@@ -2694,6 +2699,7 @@ is_gimple_variable (tree t)
   return (TREE_CODE (t) == VAR_DECL
 	  || TREE_CODE (t) == PARM_DECL
 	  || TREE_CODE (t) == RESULT_DECL
+	  || TREE_CODE (t) == STATIC_CHAIN_DECL
 	  || TREE_CODE (t) == SSA_NAME);
 }
 
--- gcc.orig/ira.c	2010-09-09 14:55:35.000000000 +0100
+++ gcc/ira.c	2011-03-21 18:37:35.136060003 +0000
@@ -1440,7 +1440,7 @@ ira_setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        /* We need the frame pointer to catch stack overflow exceptions
 	  if the stack pointer is moving.  */
--- gcc.orig/tree.def	2010-04-02 20:54:46.000000000 +0100
+++ gcc/tree.def	2011-03-21 18:37:35.140060012 +0000
@@ -543,6 +543,13 @@ DEFTREECODE (BIND_EXPR, "bind_expr", tcc
    arguments to the call.  */
 DEFTREECODE (CALL_EXPR, "call_expr", tcc_vl_exp, 3)
 
+/* Operand 0 is the FUNC_DECL of the outer function for
+   which the static chain is to be computed. */
+DEFTREECODE (STATIC_CHAIN_EXPR, "static_chain_expr", tcc_expression, 1)
+
+/* Represents a function's static chain.  It can be used as an lvalue. */
+DEFTREECODE (STATIC_CHAIN_DECL, "static_chain_decl", tcc_expression, 0)
+
 /* Specify a value to compute along with its corresponding cleanup.
    Operand 0 is the cleanup expression.
    The cleanup is executed by the first enclosing CLEANUP_POINT_EXPR,
--- gcc.orig/tree-inline.c	2010-09-25 22:38:56.000000000 +0100
+++ gcc/tree-inline.c	2011-03-21 18:37:35.148060041 +0000
@@ -2023,6 +2023,7 @@ initialize_cfun (tree new_fndecl, tree c
 
   /* Copy items we preserve during clonning.  */
   cfun->static_chain_decl = src_cfun->static_chain_decl;
+  cfun->custom_static_chain = src_cfun->custom_static_chain;
   cfun->nonlocal_goto_save_area = src_cfun->nonlocal_goto_save_area;
   cfun->function_end_locus = src_cfun->function_end_locus;
   cfun->curr_properties = src_cfun->curr_properties;
@@ -2607,7 +2608,8 @@ initialize_inlined_parameters (copy_body
   /* Initialize the static chain.  */
   p = DECL_STRUCT_FUNCTION (fn)->static_chain_decl;
   gcc_assert (fn != current_function_decl);
-  if (p)
+  /* Custom static chain has already been dealt with.  */
+  if (p && ! DECL_STRUCT_FUNCTION (fn)->custom_static_chain)
     {
       /* No static chain?  Seems like a bug in tree-nested.c.  */
       gcc_assert (static_chain);
--- gcc.orig/tree-nested.c	2010-08-31 22:08:15.000000000 +0100
+++ gcc/tree-nested.c	2011-03-21 18:37:35.152060066 +0000
@@ -750,6 +750,8 @@ get_static_chain (struct nesting_info *i
 
   if (info->context == target_context)
     {
+      /* might be doing something wrong to need the following line.. */
+      get_frame_type (info);
       x = build_addr (info->frame_decl, target_context);
     }
   else
@@ -1888,6 +1890,10 @@ convert_tramp_reference_op (tree *tp, in
       if (!DECL_STATIC_CHAIN (decl))
 	break;
 
+      /* Don't use a trampoline for a static reference. */
+      if (TREE_STATIC (t))
+	break;
+
       /* If we don't want a trampoline, then don't build one.  */
       if (TREE_NO_TRAMPOLINE (t))
 	break;
@@ -2038,6 +2044,25 @@ convert_gimple_call (gimple_stmt_iterato
       walk_body (convert_gimple_call, NULL, info, gimple_omp_body (stmt));
       break;
 
+    case GIMPLE_ASSIGN:
+      /* Deal with static_chain_decl */
+      decl = gimple_op(stmt, 0);
+      if (TREE_CODE (decl) == STATIC_CHAIN_DECL)
+	{
+	  tree chain = get_chain_decl (info);
+	  gimple_assign_set_lhs (stmt, chain);
+	  break;
+	}
+      /* Deal with static_chain_expr */
+      decl = gimple_op(stmt, 1);
+      if (TREE_CODE (decl) == STATIC_CHAIN_EXPR)
+	{
+	  tree chain = get_static_chain (info, TREE_OPERAND (decl, 0), &wi->gsi);
+	  gimple_assign_set_rhs1 (stmt, chain);
+	  break;
+	}
+      /* FALLTHRU */
+
     default:
       /* Keep looking for other operands.  */
       *handled_ops_p = false;
@@ -2366,8 +2391,38 @@ finalize_nesting_tree_1 (struct nesting_
       gimple bind;
       annotate_all_with_location (stmt_list, DECL_SOURCE_LOCATION (context));
       bind = gimple_seq_first_stmt (gimple_body (context));
-      gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
-      gimple_bind_set_body (bind, stmt_list);
+
+      if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
+	{
+	  /* This chicanery is needed to make sure the GIMPLE initialization
+	     statements are in the right order
+	     
+	     CHAIN.3 = D.1614;
+	     FRAME.0.__chain = CHAIN.3;
+	     FRAME.0.m = m;
+	   */
+	  gimple_stmt_iterator gsi =
+	      gsi_start (gimple_bind_body (bind));
+	  int found = 0;
+	  while (!gsi_end_p (gsi))
+	    {
+	      gimple g = gsi_stmt (gsi);
+	      if (gimple_code (g) == GIMPLE_ASSIGN &&
+		  gimple_op (g, 0) == root->chain_decl)
+		{
+		  gsi_insert_seq_after (& gsi, stmt_list, GSI_SAME_STMT);
+		  found = 1;
+		  break;
+		}
+	      gsi_next (& gsi);
+	    }
+	  gcc_assert (found);
+	}
+      else
+	{
+	  gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
+	  gimple_bind_set_body (bind, stmt_list);
+	}
     }
 
   /* If a chain_decl was created, then it needs to be registered with
--- gcc.orig/tree-pretty-print.c	2009-11-30 10:36:54.000000000 +0000
+++ gcc/tree-pretty-print.c	2011-03-21 18:37:35.164060121 +0000
@@ -1410,6 +1410,16 @@ dump_generic_node (pretty_printer *buffe
 	pp_string (buffer, " [tail call]");
       break;
 
+    case STATIC_CHAIN_EXPR:
+      pp_string (buffer, "<<static chain of ");
+      dump_generic_node (buffer, TREE_OPERAND (node, 0), spc, flags, false);
+      pp_string (buffer, ">>");
+      break;
+
+    case STATIC_CHAIN_DECL:
+      pp_string (buffer, "<<static chain decl>>");
+      break;
+
     case WITH_CLEANUP_EXPR:
       NIY;
       break;
--- gcc.orig/tree-sra.c	2010-08-03 10:52:46.000000000 +0100
+++ gcc/tree-sra.c	2011-03-21 18:37:35.172060172 +0000
@@ -1505,6 +1505,8 @@ is_va_list_type (tree type)
 /* The very first phase of intraprocedural SRA.  It marks in candidate_bitmap
    those with type which is suitable for scalarization.  */
 
+/* FIXME: Should we do something here for GDC? */
+
 static bool
 find_var_candidates (void)
 {
--- gcc.orig/tree-ssa-operands.c	2010-04-02 20:54:46.000000000 +0100
+++ gcc/tree-ssa-operands.c	2011-03-21 18:37:35.192060263 +0000
@@ -1001,6 +1001,12 @@ get_expr_operands (gimple stmt, tree *ex
         return;
       }
 
+    case STATIC_CHAIN_EXPR:
+      {
+	  get_expr_operands (stmt, &TREE_OPERAND (expr, 0), flags);
+	  return;
+      }
+
     case FUNCTION_DECL:
     case LABEL_DECL:
     case CONST_DECL:
