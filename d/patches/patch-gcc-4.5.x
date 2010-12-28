diff -cr gcc.orig/cgraph.c gcc/cgraph.c
*** gcc.orig/cgraph.c	2010-07-01 12:03:31.000000000 +0100
--- gcc/cgraph.c	2010-12-17 18:25:03.443968543 +0000
***************
*** 464,469 ****
--- 464,470 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 485,495 ****
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
      {
!       node->origin = cgraph_node (DECL_CONTEXT (decl));
!       node->next_nested = node->origin->nested;
!       node->origin->nested = node;
      }
    if (assembler_name_hash)
      {
--- 486,500 ----
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (DECL_STATIC_CHAIN (decl))
      {
!       context = decl_function_context (decl);
!       if (context)
! 	{
! 	  node->origin = cgraph_node (context);
! 	  node->next_nested = node->origin->nested;
! 	  node->origin->nested = node;
! 	}
      }
    if (assembler_name_hash)
      {
diff -cr gcc.orig/cgraphunit.c gcc/cgraphunit.c
*** gcc.orig/cgraphunit.c	2010-06-30 14:17:35.000000000 +0100
--- gcc/cgraphunit.c	2010-12-17 18:25:03.447968543 +0000
***************
*** 1560,1565 ****
--- 1560,1569 ----
  static void
  cgraph_expand_function (struct cgraph_node *node)
  {
+   int save_flag_omit_frame_pointer = flag_omit_frame_pointer;
+   static int inited = 0;
+   static int orig_omit_frame_pointer;
+ 
    tree decl = node->decl;
  
    /* We ought to not compile any inline clones.  */
***************
*** 1570,1578 ****
--- 1574,1592 ----
  
    gcc_assert (node->lowered);
  
+   if (!inited)
+     {
+       inited = 1;
+       orig_omit_frame_pointer = flag_omit_frame_pointer;
+     }
+   flag_omit_frame_pointer = orig_omit_frame_pointer ||
+       DECL_STRUCT_FUNCTION (decl)->naked;
+ 
    /* Generate RTL for the body of DECL.  */
    tree_rest_of_compilation (decl);
  
+   flag_omit_frame_pointer = save_flag_omit_frame_pointer;
+ 
    /* Make sure that BE didn't give up on compiling.  */
    gcc_assert (TREE_ASM_WRITTEN (decl));
    current_function_decl = NULL;
diff -cr gcc.orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc.orig/config/i386/i386.c	2010-09-30 21:24:54.000000000 +0100
--- gcc/config/i386/i386.c	2010-12-17 18:25:03.479968543 +0000
***************
*** 8131,8136 ****
--- 8131,8140 ----
      frame->red_zone_size = 0;
    frame->to_allocate -= frame->red_zone_size;
    frame->stack_pointer_offset -= frame->red_zone_size;
+ 
+   if (cfun->naked)
+     /* As above, skip return address */
+     frame->stack_pointer_offset = UNITS_PER_WORD;
  }
  
  /* Emit code to save registers in the prologue.  */
***************
*** 26366,26372 ****
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 26370,26376 ----
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc.orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc.orig/config/rs6000/rs6000.c	2010-11-17 06:09:53.000000000 +0000
--- gcc/config/rs6000/rs6000.c	2010-12-17 18:25:03.507968543 +0000
***************
*** 20365,20370 ****
--- 20365,20371 ----
  	 a number, so for now use 9.  LTO isn't assigned a number either,
  	 so for now use 0.  */
        if (! strcmp (language_string, "GNU C")
+ 	  || ! strcmp (language_string, "GNU D")
  	  || ! strcmp (language_string, "GNU GIMPLE"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
diff -cr gcc.orig/dwarf2out.c gcc/dwarf2out.c
*** gcc.orig/dwarf2out.c	2010-12-07 15:12:45.000000000 +0000
--- gcc/dwarf2out.c	2010-12-17 18:25:03.527968543 +0000
***************
*** 18729,18734 ****
--- 18729,18736 ----
    language = DW_LANG_C89;
    if (strcmp (language_string, "GNU C++") == 0)
      language = DW_LANG_C_plus_plus;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else if (strcmp (language_string, "GNU F77") == 0)
      language = DW_LANG_Fortran77;
    else if (strcmp (language_string, "GNU Pascal") == 0)
***************
*** 19967,19973 ****
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (DECL_CONTEXT (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
--- 19969,19975 ----
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (decl_function_context (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
diff -cr gcc.orig/expr.c gcc/expr.c
*** gcc.orig/expr.c	2010-09-21 15:18:34.000000000 +0100
--- gcc/expr.c	2010-12-17 18:25:03.539968543 +0000
***************
*** 9625,9630 ****
--- 9625,9635 ----
        /* Lowered by gimplify.c.  */
        gcc_unreachable ();
  
+     case STATIC_CHAIN_EXPR:
+     case STATIC_CHAIN_DECL:
+       /* Lowered by tree-nested.c */
+       gcc_unreachable ();
+ 
      case FDESC_EXPR:
        /* Function descriptors are not valid except for as
  	 initialization constants, and should not be expanded.  */
diff -cr gcc.orig/function.c gcc/function.c
*** gcc.orig/function.c	2010-08-16 21:18:08.000000000 +0100
--- gcc/function.c	2010-12-17 18:25:03.551968543 +0000
***************
*** 3202,3208 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 3202,3209 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
***************
*** 3219,3225 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.first_conversion_insn);
  
    /* Estimate reload stack alignment from scalar return mode.  */
    if (SUPPORTS_STACK_ALIGNMENT)
--- 3220,3227 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.first_conversion_insn);
  
    /* Estimate reload stack alignment from scalar return mode.  */
    if (SUPPORTS_STACK_ALIGNMENT)
***************
*** 3373,3378 ****
--- 3375,3383 ----
    VEC(tree, heap) *fnargs;
    unsigned i;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4472,4482 ****
        local = gen_reg_rtx (Pmode);
        chain = targetm.calls.static_chain (current_function_decl, true);
  
-       set_decl_incoming_rtl (parm, chain, false);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       insn = emit_move_insn (local, chain);
  
        /* Mark the register as eliminable, similar to parameters.  */
        if (MEM_P (chain)
--- 4477,4491 ----
        local = gen_reg_rtx (Pmode);
        chain = targetm.calls.static_chain (current_function_decl, true);
  
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       if (!cfun->custom_static_chain)
! 	{
! 	  set_decl_incoming_rtl (parm, chain, false);
! 	  insn = emit_move_insn (local, chain);
! 	}
!       /* else, the static chain will be set in the main body */
  
        /* Mark the register as eliminable, similar to parameters.  */
        if (MEM_P (chain)
***************
*** 5015,5020 ****
--- 5024,5032 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+     return;
+ 
    rtl_profile_for_bb (ENTRY_BLOCK_PTR);
  #ifdef HAVE_prologue
    if (HAVE_prologue)
diff -cr gcc.orig/function.h gcc/function.h
*** gcc.orig/function.h	2009-11-25 10:55:54.000000000 +0000
--- gcc/function.h	2010-12-17 18:25:03.551968543 +0000
***************
*** 596,601 ****
--- 596,609 ----
       adjusts one of its arguments and forwards to another
       function.  */
    unsigned int is_thunk : 1;
+ 
+   /* Nonzero if static chain is initialized by something other than
+      static_chain_incoming_rtx. */
+   unsigned int custom_static_chain : 1;
+ 
+   /* Nonzero if no code should be generated for prologues, copying
+      parameters, etc. */
+   unsigned int naked : 1;
  };
  
  /* If va_list_[gf]pr_size is set to this, it means we don't know how
diff -cr gcc.orig/gcc.c gcc/gcc.c
*** gcc.orig/gcc.c	2010-04-18 18:46:08.000000000 +0100
--- gcc/gcc.c	2010-12-17 18:25:03.559968543 +0000
***************
*** 139,144 ****
--- 139,147 ----
  /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
  static bool at_file_supplied;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 407,412 ****
--- 410,418 ----
  static const char *compare_debug_dump_opt_spec_function (int, const char **);
  static const char *compare_debug_self_opt_spec_function (int, const char **);
  static const char *compare_debug_auxbase_opt_spec_function (int, const char **);
+ 
+ extern const char *d_all_sources_spec_function (int, const char **);
+ extern const char *d_output_prefix_spec_function (int, const char **);
  
  /* The Specs Language
  
***************
*** 515,520 ****
--- 521,527 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N	Output the currently selected multilib directory name
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 1001,1006 ****
--- 1008,1015 ----
  #endif
  #endif
  
+ #define GCC_SPEC_FORMAT_4 1
+ 
  /* Record the mapping from file suffixes for compilation specs.  */
  
  struct compiler
***************
*** 1730,1735 ****
--- 1739,1748 ----
    { "compare-debug-dump-opt",	compare_debug_dump_opt_spec_function },
    { "compare-debug-self-opt",	compare_debug_self_opt_spec_function },
    { "compare-debug-auxbase-opt", compare_debug_auxbase_opt_spec_function },
+ #ifdef D_USE_EXTRA_SPEC_FUNCTIONS
+   { "d-all-sources",		d_all_sources_spec_function },
+   { "d-output-prefix",		d_output_prefix_spec_function },
+ #endif
  #ifdef EXTRA_SPEC_FUNCTIONS
    EXTRA_SPEC_FUNCTIONS
  #endif
***************
*** 4282,4287 ****
--- 4295,4303 ----
        save_temps_prefix = NULL;
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if (save_temps_flag && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4646,4651 ****
--- 4662,4679 ----
        infiles[0].name   = "help-dummy";
      }
  
+   if (need_pthreads)
+     {
+       switches[n_switches].part1 = "pthread";
+       switches[n_switches].args = 0;
+       switches[n_switches].live_cond = 0;
+       /* Do not print an error if there is not expansion for -pthread. */
+       switches[n_switches].validated = 1;
+       switches[n_switches].ordering = 0;
+ 
+       n_switches++;
+     }
+ 
    switches[n_switches].part1 = 0;
    infiles[n_infiles].name = 0;
  }
***************
*** 5809,5814 ****
--- 5837,5853 ----
  	      return value;
  	    break;
  
+ 	  case 'N':
+ 	    if (multilib_dir)
+ 	      {
+ 		arg_going = 1;
+ 		obstack_grow (&obstack, "-fmultilib-dir=",
+ 			      strlen ("-fmultilib-dir="));
+ 		obstack_grow (&obstack, multilib_dir,
+ 			      strlen (multilib_dir));
+ 	      }
+ 	    break;
+ 
  	    /* Here we define characters other than letters and digits.  */
  
  	  case '{':
diff -cr gcc.orig/gimple.c gcc/gimple.c
*** gcc.orig/gimple.c	2010-06-22 19:23:11.000000000 +0100
--- gcc/gimple.c	2010-12-17 18:25:03.567968543 +0000
***************
*** 2404,2409 ****
--- 2404,2411 ----
        || (SYM) == POLYNOMIAL_CHREC					    \
        || (SYM) == DOT_PROD_EXPR						    \
        || (SYM) == VEC_COND_EXPR						    \
+       || (SYM) == STATIC_CHAIN_DECL					    \
+       || (SYM) == STATIC_CHAIN_EXPR /* not sure if this is right...*/	    \
        || (SYM) == REALIGN_LOAD_EXPR) ? GIMPLE_SINGLE_RHS		    \
     : GIMPLE_INVALID_RHS),
  #define END_OF_BASE_TREE_CODES (unsigned char) GIMPLE_INVALID_RHS,
***************
*** 2460,2466 ****
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
--- 2462,2471 ----
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF
! 	  /* This is an lvalue because it will be replaced with the real
! 	     static chain decl. */
! 	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
***************
*** 2694,2699 ****
--- 2699,2705 ----
    return (TREE_CODE (t) == VAR_DECL
  	  || TREE_CODE (t) == PARM_DECL
  	  || TREE_CODE (t) == RESULT_DECL
+ 	  || TREE_CODE (t) == STATIC_CHAIN_DECL
  	  || TREE_CODE (t) == SSA_NAME);
  }
  
diff -cr gcc.orig/tree.def gcc/tree.def
*** gcc.orig/tree.def	2010-04-02 20:54:46.000000000 +0100
--- gcc/tree.def	2010-12-17 18:25:03.575968543 +0000
***************
*** 543,548 ****
--- 543,555 ----
     arguments to the call.  */
  DEFTREECODE (CALL_EXPR, "call_expr", tcc_vl_exp, 3)
  
+ /* Operand 0 is the FUNC_DECL of the outer function for
+    which the static chain is to be computed. */
+ DEFTREECODE (STATIC_CHAIN_EXPR, "static_chain_expr", tcc_expression, 1)
+ 
+ /* Represents a function's static chain.  It can be used as an lvalue. */
+ DEFTREECODE (STATIC_CHAIN_DECL, "static_chain_decl", tcc_expression, 0)
+ 
  /* Specify a value to compute along with its corresponding cleanup.
     Operand 0 is the cleanup expression.
     The cleanup is executed by the first enclosing CLEANUP_POINT_EXPR,
diff -cr gcc.orig/tree-dump.c gcc/tree-dump.c
*** gcc.orig/tree-dump.c	2009-12-31 10:52:56.000000000 +0000
--- gcc/tree-dump.c	2010-12-17 18:25:03.579968543 +0000
***************
*** 639,644 ****
--- 639,648 ----
        }
        break;
  
+     case STATIC_CHAIN_EXPR:
+       dump_child ("func", TREE_OPERAND (t, 0));
+       break;
+ 
      case CONSTRUCTOR:
        {
  	unsigned HOST_WIDE_INT cnt;
diff -cr gcc.orig/tree-inline.c gcc/tree-inline.c
*** gcc.orig/tree-inline.c	2010-09-25 22:38:56.000000000 +0100
--- gcc/tree-inline.c	2010-12-17 18:25:03.587968543 +0000
***************
*** 2607,2613 ****
    /* Initialize the static chain.  */
    p = DECL_STRUCT_FUNCTION (fn)->static_chain_decl;
    gcc_assert (fn != current_function_decl);
!   if (p)
      {
        /* No static chain?  Seems like a bug in tree-nested.c.  */
        gcc_assert (static_chain);
--- 2607,2614 ----
    /* Initialize the static chain.  */
    p = DECL_STRUCT_FUNCTION (fn)->static_chain_decl;
    gcc_assert (fn != current_function_decl);
!   /* Custom static chain has already been dealt with.  */
!   if (p && !DECL_STRUCT_FUNCTION (fn)->custom_static_chain)
      {
        /* No static chain?  Seems like a bug in tree-nested.c.  */
        gcc_assert (static_chain);
diff -cr gcc.orig/tree-nested.c gcc/tree-nested.c
*** gcc.orig/tree-nested.c	2010-08-31 22:08:15.000000000 +0100
--- gcc/tree-nested.c	2010-12-17 18:25:03.591968543 +0000
***************
*** 750,755 ****
--- 750,757 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
***************
*** 1888,1893 ****
--- 1890,1899 ----
        if (!DECL_STATIC_CHAIN (decl))
  	break;
  
+       /* Don't use a trampoline for a static reference. */
+       if (TREE_STATIC (t))
+ 	break;
+ 
        /* If we don't want a trampoline, then don't build one.  */
        if (TREE_NO_TRAMPOLINE (t))
  	break;
***************
*** 2038,2043 ****
--- 2044,2068 ----
        walk_body (convert_gimple_call, NULL, info, gimple_omp_body (stmt));
        break;
  
+     case GIMPLE_ASSIGN:
+       /* Deal with static_chain_decl */
+       decl = gimple_op(stmt, 0);
+       if (TREE_CODE (decl) == STATIC_CHAIN_DECL)
+ 	{
+ 	  tree chain = get_chain_decl (info);
+ 	  gimple_assign_set_lhs (stmt, chain);
+ 	  break;
+ 	}
+       /* Deal with static_chain_expr */
+       decl = gimple_op(stmt, 1);
+       if (TREE_CODE (decl) == STATIC_CHAIN_EXPR)
+ 	{
+ 	  tree chain = get_static_chain (info, TREE_OPERAND (decl, 0), &wi->gsi);
+ 	  gimple_assign_set_rhs1 (stmt, chain);
+ 	  break;
+ 	}
+       /* FALLTHRU */
+ 
      default:
        /* Keep looking for other operands.  */
        *handled_ops_p = false;
***************
*** 2366,2373 ****
        gimple bind;
        annotate_all_with_location (stmt_list, DECL_SOURCE_LOCATION (context));
        bind = gimple_seq_first_stmt (gimple_body (context));
!       gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
!       gimple_bind_set_body (bind, stmt_list);
      }
  
    /* If a chain_decl was created, then it needs to be registered with
--- 2391,2428 ----
        gimple bind;
        annotate_all_with_location (stmt_list, DECL_SOURCE_LOCATION (context));
        bind = gimple_seq_first_stmt (gimple_body (context));
! 
!       if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
! 	{
! 	  /* This chicanery is needed to make sure the GIMPLE initialization
! 	     statements are in the right order
! 	     
! 	     CHAIN.3 = D.1614;
! 	     FRAME.0.__chain = CHAIN.3;
! 	     FRAME.0.m = m;
! 	   */
! 	  gimple_stmt_iterator gsi =
! 	      gsi_start (gimple_bind_body (bind));
! 	  int found = 0;
! 	  while (!gsi_end_p (gsi))
! 	    {
! 	      gimple g = gsi_stmt (gsi);
! 	      if (gimple_code (g) == GIMPLE_ASSIGN &&
! 		  gimple_op (g, 0) == root->chain_decl)
! 		{
! 		  gsi_insert_seq_after (& gsi, stmt_list, GSI_SAME_STMT);
! 		  found = 1;
! 		  break;
! 		}
! 	      gsi_next (& gsi);
! 	    }
! 	  gcc_assert (found);
! 	}
!       else
! 	{
! 	  gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
! 	  gimple_bind_set_body (bind, stmt_list);
! 	}
      }
  
    /* If a chain_decl was created, then it needs to be registered with
diff -cr gcc.orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc.orig/tree-pretty-print.c	2009-11-30 10:36:54.000000000 +0000
--- gcc/tree-pretty-print.c	2010-12-17 18:25:03.595968543 +0000
***************
*** 1410,1415 ****
--- 1410,1425 ----
  	pp_string (buffer, " [tail call]");
        break;
  
+     case STATIC_CHAIN_EXPR:
+       pp_string (buffer, "<<static chain of ");
+       dump_generic_node (buffer, TREE_OPERAND (node, 0), spc, flags, false);
+       pp_string (buffer, ">>");
+       break;
+ 
+     case STATIC_CHAIN_DECL:
+       pp_string (buffer, "<<static chain decl>>");
+       break;
+ 
      case WITH_CLEANUP_EXPR:
        NIY;
        break;
diff -cr gcc.orig/tree-sra.c gcc/tree-sra.c
*** gcc.orig/tree-sra.c	2010-08-03 10:52:46.000000000 +0100
--- gcc/tree-sra.c	2010-12-17 18:25:03.599968543 +0000
***************
*** 1505,1510 ****
--- 1505,1512 ----
  /* The very first phase of intraprocedural SRA.  It marks in candidate_bitmap
     those with type which is suitable for scalarization.  */
  
+ /* FIXME: Should we do something here for GDC? */
+ 
  static bool
  find_var_candidates (void)
  {
diff -cr gcc.orig/tree-ssa-operands.c gcc/tree-ssa-operands.c
*** gcc.orig/tree-ssa-operands.c	2010-04-02 20:54:46.000000000 +0100
--- gcc/tree-ssa-operands.c	2010-12-17 18:25:03.603968543 +0000
***************
*** 1001,1006 ****
--- 1001,1012 ----
          return;
        }
  
+     case STATIC_CHAIN_EXPR:
+       {
+ 	  get_expr_operands (stmt, &TREE_OPERAND (expr, 0), flags);
+ 	  return;
+       }
+ 
      case FUNCTION_DECL:
      case LABEL_DECL:
      case CONST_DECL:
