diff -cr gcc-orig/cgraph.c gcc/cgraph.c
*** gcc-orig/cgraph.c	2008-11-14 08:26:59.000000000 -0500
--- gcc/cgraph.c	2010-08-22 20:21:33.226320227 -0400
***************
*** 181,186 ****
--- 181,187 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 202,213 ****
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
      {
!       node->origin = cgraph_node (DECL_CONTEXT (decl));
!       node->next_nested = node->origin->nested;
!       node->origin->nested = node;
!       node->master_clone = node;
      }
    return node;
  }
--- 203,218 ----
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (!DECL_NO_STATIC_CHAIN (decl))
      {
!       context = decl_function_context (decl);
!       if (context)
!         {
! 	  node->origin = cgraph_node (context);
! 	  node->next_nested = node->origin->nested;
! 	  node->origin->nested = node;
! 	  node->master_clone = node;
!         }
      }
    return node;
  }
diff -cr gcc-orig/cgraphunit.c gcc/cgraphunit.c
*** gcc-orig/cgraphunit.c	2008-11-14 08:26:59.000000000 -0500
--- gcc/cgraphunit.c	2010-08-22 20:21:33.226320227 -0400
***************
*** 1150,1155 ****
--- 1150,1159 ----
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
*** 1159,1169 ****
--- 1163,1183 ----
      announce_function (decl);
  
    gcc_assert (node->lowered);
+   
+   if (!inited)
+   {
+       inited = 1;
+       orig_omit_frame_pointer = flag_omit_frame_pointer;
+   }
+   flag_omit_frame_pointer = orig_omit_frame_pointer ||
+     DECL_STRUCT_FUNCTION (decl)->naked;
  
    /* Generate RTL for the body of DECL.  */
    if (lang_hooks.callgraph.emit_associated_thunks)
      lang_hooks.callgraph.emit_associated_thunks (decl);
    tree_rest_of_compilation (decl);
+   
+   flag_omit_frame_pointer = save_flag_omit_frame_pointer;
  
    /* Make sure that BE didn't give up on compiling.  */
    /* ??? Can happen with nested function of extern inline.  */
diff -cr gcc-orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc-orig/config/i386/i386.c	2010-03-31 16:14:10.000000000 -0400
--- gcc/config/i386/i386.c	2010-08-22 20:21:33.242319467 -0400
***************
*** 6151,6156 ****
--- 6151,6161 ----
      frame->red_zone_size = 0;
    frame->to_allocate -= frame->red_zone_size;
    frame->stack_pointer_offset -= frame->red_zone_size;
+ 
+   if (cfun->naked)
+       /* As above, skip return address */
+       frame->stack_pointer_offset = UNITS_PER_WORD;
+ 
  #if 0
    fprintf (stderr, "\n");
    fprintf (stderr, "nregs: %ld\n", (long)frame->nregs);
***************
*** 22924,22930 ****
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 22929,22935 ----
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc-orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc-orig/config/rs6000/rs6000.c	2009-09-23 18:30:05.000000000 -0400
--- gcc/config/rs6000/rs6000.c	2010-08-22 20:21:33.258318288 -0400
***************
*** 16943,16949 ****
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
--- 16943,16950 ----
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C")
! 	  || ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
diff -cr gcc-orig/dwarf2out.c gcc/dwarf2out.c
*** gcc-orig/dwarf2out.c	2009-06-18 16:06:04.000000000 -0400
--- gcc/dwarf2out.c	2010-08-22 20:21:33.266319165 -0400
***************
*** 5743,5749 ****
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
  	  || lang == DW_LANG_C99
! 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
  }
  
  /* Return TRUE if the language is C++.  */
--- 5743,5750 ----
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
  	  || lang == DW_LANG_C99
! 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus
! 	  || lang == DW_LANG_D);
  }
  
  /* Return TRUE if the language is C++.  */
***************
*** 13267,13272 ****
--- 13268,13275 ----
      language = DW_LANG_ObjC;
    else if (strcmp (language_string, "GNU Objective-C++") == 0)
      language = DW_LANG_ObjC_plus_plus;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
***************
*** 14408,14414 ****
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (DECL_CONTEXT (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
--- 14411,14417 ----
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (decl_function_context (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
diff -cr gcc-orig/except.c gcc/except.c
*** gcc-orig/except.c	2010-08-07 23:20:32.623306514 +0100
--- gcc/except.c	2010-09-01 18:03:06.763377179 +0100
***************
*** 1821,1826 ****
--- 1821,1838 ----
  
  	  region = VEC_index (eh_region, cfun->eh->region_array, INTVAL (XEXP (note, 0)));
  	  this_call_site = lp_info[region->region_number].call_site_index;
+ 	  if (region->type == ERT_CATCH)
+ 	  {
+ 	    /* Use previous region information */
+ 	    region = region->outer;
+ 	    if (!region)
+ 	    {
+ 	      /* No previous region, must change function contexts. */
+ 	      this_call_site = -1;
+ 	    }
+ 	    else
+ 	    this_call_site = lp_info[region->region_number].call_site_index;        
+ 	  }
  	}
  
        if (this_call_site == last_call_site)
diff -cr gcc-orig/expr.c gcc/expr.c
*** gcc-orig/expr.c	2009-01-06 11:17:41.000000000 -0500
--- gcc/expr.c	2010-08-22 20:21:33.270321419 -0400
***************
*** 9227,9232 ****
--- 9227,9237 ----
  	 represent.  */
        return const0_rtx;
  
+     case STATIC_CHAIN_EXPR:
+     case STATIC_CHAIN_DECL:
+       /* Lowered by tree-nested.c */
+       gcc_unreachable ();
+ 
      case EXC_PTR_EXPR:
        return get_exception_pointer (cfun);
  
diff -cr gcc-orig/function.c gcc/function.c
*** gcc-orig/function.c	2009-06-19 17:44:24.000000000 -0400
--- gcc/function.c	2010-08-22 20:21:33.270321419 -0400
***************
*** 3062,3068 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 3062,3069 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
***************
*** 3077,3083 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.first_conversion_insn);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
--- 3078,3085 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.first_conversion_insn);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
***************
*** 3207,3212 ****
--- 3209,3217 ----
    struct assign_parm_data_all all;
    tree fnargs, parm, stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+   
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4280,4290 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4285,4299 ----
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       if (! cfun->custom_static_chain)
!         {
! 	    set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
! 	    emit_move_insn (local, static_chain_incoming_rtx);
! 	}
!       /* else, the static chain will be set in the main body */
      }
  
    /* If the function receives a non-local goto, then store the
***************
*** 5179,5184 ****
--- 5188,5196 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+       return;
+ 
  #ifdef HAVE_prologue
    if (HAVE_prologue)
      {
diff -cr gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	2008-01-26 12:18:35.000000000 -0500
--- gcc/function.h	2010-08-22 20:21:33.270321419 -0400
***************
*** 463,468 ****
--- 463,476 ----
  
    /* Nonzero if pass_tree_profile was run on this function.  */
    unsigned int after_tree_profile : 1;
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
diff -cr gcc-orig/gcc.c gcc/gcc.c
*** gcc-orig/gcc.c	2008-03-02 17:55:19.000000000 -0500
--- gcc/gcc.c	2010-08-22 20:21:33.274319973 -0400
***************
*** 129,134 ****
--- 129,137 ----
  /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
  static bool at_file_supplied;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 365,370 ****
--- 368,376 ----
  static const char *version_compare_spec_function (int, const char **);
  static const char *include_spec_function (int, const char **);
  static const char *print_asm_header_spec_function (int, const char **);
+ 
+ extern const char *d_all_sources_spec_function (int, const char **);
+ extern const char *d_output_prefix_spec_function (int, const char **);
  
  /* The Specs Language
  
***************
*** 472,477 ****
--- 478,484 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 1635,1640 ****
--- 1644,1653 ----
    { "version-compare",		version_compare_spec_function },
    { "include",			include_spec_function },
    { "print-asm-header",		print_asm_header_spec_function },
+ #ifdef D_USE_EXTRA_SPEC_FUNCTIONS
+   { "d-all-sources",            d_all_sources_spec_function },
+   { "d-output-prefix",          d_output_prefix_spec_function },
+ #endif
  #ifdef EXTRA_SPEC_FUNCTIONS
    EXTRA_SPEC_FUNCTIONS
  #endif
***************
*** 3974,3979 ****
--- 3987,3995 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if (save_temps_flag && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4280,4285 ****
--- 4296,4313 ----
        infiles[0].name   = "help-dummy";
      }
  
+   if (need_pthreads)
+     {
+ 	switches[n_switches].part1 = "pthread";
+ 	switches[n_switches].args = 0;
+ 	switches[n_switches].live_cond = 0;
+ 	/* Do not print an error if there is not expansion for -pthread. */
+ 	switches[n_switches].validated = 1;
+ 	switches[n_switches].ordering = 0;
+ 
+ 	n_switches++;
+     }
+ 
    switches[n_switches].part1 = 0;
    infiles[n_infiles].name = 0;
  }
***************
*** 5240,5245 ****
--- 5268,5284 ----
  	      return value;
  	    break;
  
+ 	  case 'N':
+ 	    if (multilib_dir)
+ 	      {
+ 		arg_going = 1;
+ 		obstack_grow (&obstack, "-fmultilib-dir=",
+ 			      strlen ("-fmultilib-dir="));
+ 	        obstack_grow (&obstack, multilib_dir,
+ 			      strlen (multilib_dir));
+ 	      }
+ 	    break;
+ 
  	    /* Here we define characters other than letters and digits.  */
  
  	  case '{':
diff -cr gcc-orig/gcc.h gcc/gcc.h
*** gcc-orig/gcc.h	2007-07-26 04:37:01.000000000 -0400
--- gcc/gcc.h	2010-08-22 20:21:33.274319973 -0400
***************
*** 37,43 ****
     || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
     || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
!    || (CHAR) == 'B' || (CHAR) == 'b')
  
  /* This defines which multi-letter switches take arguments.  */
  
--- 37,43 ----
     || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
     || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
!    || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
  
  /* This defines which multi-letter switches take arguments.  */
  
diff -cr gcc-orig/tree.def gcc/tree.def
*** gcc-orig/tree.def	2007-10-29 07:05:04.000000000 -0400
--- gcc/tree.def	2010-08-22 20:21:33.282318755 -0400
***************
*** 539,544 ****
--- 539,551 ----
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
diff -cr gcc-orig/tree-gimple.c gcc/tree-gimple.c
*** gcc-orig/tree-gimple.c	2007-12-13 16:49:09.000000000 -0500
--- gcc/tree-gimple.c	2010-08-22 20:21:33.278319643 -0400
***************
*** 74,79 ****
--- 74,81 ----
      case VECTOR_CST:
      case OBJ_TYPE_REF:
      case ASSERT_EXPR:
+     case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+     case STATIC_CHAIN_DECL:
        return true;
  
      default:
***************
*** 147,153 ****
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
--- 149,158 ----
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF
!           /* This is an lvalue because it will be replaced with the real
! 	     static chain decl. */
! 	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
diff -cr gcc-orig/tree-nested.c gcc/tree-nested.c
*** gcc-orig/tree-nested.c	2008-05-29 07:35:05.000000000 -0400
--- gcc/tree-nested.c	2010-08-22 20:22:52.414316742 -0400
***************
*** 815,820 ****
--- 815,822 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
***************
*** 1640,1645 ****
--- 1642,1651 ----
        if (DECL_NO_STATIC_CHAIN (decl))
  	break;
  
+       /* Don't use a trampoline for a static reference. */
+       if (TREE_STATIC (t))
+ 	break;
+ 
        /* Lookup the immediate parent of the callee, as that's where
  	 we need to insert the trampoline.  */
        for (i = info; i->context != target_context; i = i->outer)
***************
*** 1714,1719 ****
--- 1720,1733 ----
  	}
        break;
  
+     case STATIC_CHAIN_EXPR:
+       *tp = get_static_chain (info, TREE_OPERAND (t, 0), &wi->tsi);
+       break;
+ 
+     case STATIC_CHAIN_DECL:
+       *tp = get_chain_decl (info);
+       break;
+  
      case RETURN_EXPR:
      case GIMPLE_MODIFY_STMT:
      case WITH_SIZE_EXPR:
***************
*** 1889,1897 ****
      {
        annotate_all_with_locus (&stmt_list,
  			       DECL_SOURCE_LOCATION (context));
!       append_to_statement_list (BIND_EXPR_BODY (DECL_SAVED_TREE (context)),
! 				&stmt_list);
!       BIND_EXPR_BODY (DECL_SAVED_TREE (context)) = stmt_list;
      }
  
    /* If a chain_decl was created, then it needs to be registered with
--- 1903,1936 ----
      {
        annotate_all_with_locus (&stmt_list,
  			       DECL_SOURCE_LOCATION (context));
!       /* If the function has a custom static chain, chain_field must
! 	 be set after the static chain. */
!       if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
! 	{
! 	  /* Should use walk_function instead. */
! 	  tree_stmt_iterator i =
! 	      tsi_start ( BIND_EXPR_BODY (DECL_SAVED_TREE (context)));
! 	  int found = 0;
! 	  while (!tsi_end_p (i))
! 	    {
! 	      tree t = tsi_stmt (i);
! 	      if (TREE_CODE (t) == GIMPLE_MODIFY_STMT &&
! 		  GIMPLE_STMT_OPERAND (t, 0) == root->chain_decl)
! 		{
! 		  tsi_link_after (& i, stmt_list, TSI_SAME_STMT);
! 		  found = 1;
! 		  break;
! 		}
! 	      tsi_next (& i);
! 	    }
! 	  gcc_assert (found);
! 	}
!       else
!         {
! 	  append_to_statement_list (BIND_EXPR_BODY (DECL_SAVED_TREE (context)),
! 				    &stmt_list);
! 	  BIND_EXPR_BODY (DECL_SAVED_TREE (context)) = stmt_list;
! 	}
      }
  
    /* If a chain_decl was created, then it needs to be registered with
diff -cr gcc-orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc-orig/tree-pretty-print.c	2008-01-27 11:48:54.000000000 -0500
--- gcc/tree-pretty-print.c	2010-08-22 20:21:33.282318755 -0400
***************
*** 1251,1256 ****
--- 1251,1266 ----
  	pp_string (buffer, " [tail call]");
        break;
  
+     case STATIC_CHAIN_EXPR:
+ 	pp_string (buffer, "<<static chain of ");
+ 	dump_generic_node (buffer, TREE_OPERAND (node, 0), spc, flags, false);
+ 	pp_string (buffer, ">>");
+       break;
+ 
+     case STATIC_CHAIN_DECL:
+        pp_string (buffer, "<<static chain decl>>");
+        break;
+ 	
      case WITH_CLEANUP_EXPR:
        NIY;
        break;
diff -cr gcc-orig/tree-sra.c gcc/tree-sra.c
*** gcc-orig/tree-sra.c	2010-04-18 11:56:56.000000000 -0400
--- gcc/tree-sra.c	2010-08-22 20:21:33.282318755 -0400
***************
*** 262,267 ****
--- 262,269 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
***************
*** 271,276 ****
--- 273,283 ----
  		  && (tree_low_cst (DECL_SIZE (t), 1)
  		      != TYPE_PRECISION (TREE_TYPE (t))))
  		goto fail;
+ 	      /* Reject aliased fields created by GDC for anonymous unions. */
+ 	      cmp = fold_binary_to_constant (LE_EXPR, boolean_type_node,
+ 		DECL_FIELD_OFFSET (t), last_offset);
+ 	      if (cmp == NULL_TREE || tree_expr_nonzero_p (cmp))
+ 		goto fail;
  
  	      saw_one_field = true;
  	    }
