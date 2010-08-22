diff -cr gcc.orig/cgraph.c gcc/cgraph.c
*** gcc.orig/cgraph.c	2008-11-16 22:31:58.000000000 +0000
--- gcc/cgraph.c	2010-08-14 11:48:10.482538287 +0100
***************
*** 451,456 ****
--- 451,457 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 472,483 ****
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
    if (assembler_name_hash)
      {
--- 473,488 ----
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (!DECL_NO_STATIC_CHAIN (decl))
      {
!       context = decl_function_context (decl);
!       if (context)
! 	{
! 	  node->origin = cgraph_node (context);
! 	  node->next_nested = node->origin->nested;
! 	  node->origin->nested = node;
! 	  node->master_clone = node;
! 	}
      }
    if (assembler_name_hash)
      {
diff -cr gcc.orig/cgraphunit.c gcc/cgraphunit.c
*** gcc.orig/cgraphunit.c	2010-03-31 14:23:17.000000000 +0100
--- gcc/cgraphunit.c	2010-08-14 11:48:10.486540332 +0100
***************
*** 1032,1037 ****
--- 1032,1041 ----
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
*** 1041,1051 ****
--- 1045,1065 ----
  
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
    if (lang_hooks.callgraph.emit_associated_thunks)
      lang_hooks.callgraph.emit_associated_thunks (decl);
    tree_rest_of_compilation (decl);
  
+   flag_omit_frame_pointer = save_flag_omit_frame_pointer;
+ 
    /* Make sure that BE didn't give up on compiling.  */
    gcc_assert (TREE_ASM_WRITTEN (decl));
    current_function_decl = NULL;
diff -cr gcc.orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc.orig/config/i386/i386.c	2010-04-08 16:09:17.000000000 +0100
--- gcc/config/i386/i386.c	2010-08-14 11:48:10.525941850 +0100
***************
*** 7876,7881 ****
--- 7876,7886 ----
      frame->red_zone_size = 0;
    frame->to_allocate -= frame->red_zone_size;
    frame->stack_pointer_offset -= frame->red_zone_size;
+ 
+   if (cfun->naked)
+     /* As above, skip return address */
+     frame->stack_pointer_offset = UNITS_PER_WORD;
+ 
  #if 0
    fprintf (stderr, "\n");
    fprintf (stderr, "size: %ld\n", (long)size);
***************
*** 26792,26798 ****
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 26797,26803 ----
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc.orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc.orig/config/rs6000/rs6000.c	2010-03-27 18:56:08.000000000 +0000
--- gcc/config/rs6000/rs6000.c	2010-08-14 11:48:10.557542026 +0100
***************
*** 17630,17636 ****
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU Fortran"))
--- 17630,17637 ----
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C")
! 	  || ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU Fortran"))
Only in gcc: d
diff -cr gcc.orig/dwarf2out.c gcc/dwarf2out.c
*** gcc.orig/dwarf2out.c	2010-02-10 15:09:06.000000000 +0000
--- gcc/dwarf2out.c	2010-08-14 11:48:10.590539796 +0100
***************
*** 6398,6404 ****
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
  	  || lang == DW_LANG_C99
! 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
  }
  
  /* Return TRUE if the language is C++.  */
--- 6398,6405 ----
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
  	  || lang == DW_LANG_C99
! 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus
! 	  || lang == DW_LANG_D);
  }
  
  /* Return TRUE if the language is C++.  */
***************
*** 14506,14511 ****
--- 14507,14514 ----
      language = DW_LANG_ObjC;
    else if (strcmp (language_string, "GNU Objective-C++") == 0)
      language = DW_LANG_ObjC_plus_plus;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
***************
*** 15688,15694 ****
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (DECL_CONTEXT (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
--- 15691,15697 ----
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (decl_function_context (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
diff -cr gcc.orig/except.c gcc/except.c
*** gcc.orig/except.c	2010-03-08 11:46:28.000000000 +0000
--- gcc/except.c	2010-08-14 11:48:10.602564369 +0100
***************
*** 200,206 ****
  
  typedef struct eh_region *eh_region;
  
! struct call_site_record GTY(())
  {
    rtx landing_pad;
    int action;
--- 200,206 ----
  
  typedef struct eh_region *eh_region;
  
! struct call_site_record_d GTY(())
  {
    rtx landing_pad;
    int action;
***************
*** 3198,3204 ****
  {
    call_site_record record;
    
!   record = GGC_NEW (struct call_site_record);
    record->landing_pad = landing_pad;
    record->action = action;
  
--- 3198,3204 ----
  {
    call_site_record record;
    
!   record = GGC_NEW (struct call_site_record_d);
    record->landing_pad = landing_pad;
    record->action = action;
  
***************
*** 3399,3405 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (cs->action);
      }
  
--- 3399,3405 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (cs->action);
      }
  
***************
*** 3415,3421 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (INTVAL (cs->landing_pad));
        size += size_of_uleb128 (cs->action);
      }
--- 3415,3421 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (INTVAL (cs->landing_pad));
        size += size_of_uleb128 (cs->action);
      }
***************
*** 3432,3438 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        char reg_start_lab[32];
        char reg_end_lab[32];
        char landing_pad_lab[32];
--- 3432,3438 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        char reg_start_lab[32];
        char reg_end_lab[32];
        char landing_pad_lab[32];
***************
*** 3486,3492 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
  
        dw2_asm_output_data_uleb128 (INTVAL (cs->landing_pad),
  				   "region %d landing pad", i);
--- 3486,3492 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
  
        dw2_asm_output_data_uleb128 (INTVAL (cs->landing_pad),
  				   "region %d landing pad", i);
diff -cr gcc.orig/expr.c gcc/expr.c
*** gcc.orig/expr.c	2010-03-08 11:46:28.000000000 +0000
--- gcc/expr.c	2010-08-14 11:48:10.613540625 +0100
***************
*** 9321,9326 ****
--- 9321,9331 ----
  	 represent.  */
        return const0_rtx;
  
+     case STATIC_CHAIN_EXPR:
+     case STATIC_CHAIN_DECL:
+       /* Lowered by tree-nested.c */
+       gcc_unreachable ();
+ 
      case EXC_PTR_EXPR:
        return get_exception_pointer ();
  
diff -cr gcc.orig/function.c gcc/function.c
*** gcc.orig/function.c	2009-11-13 19:57:51.000000000 +0000
--- gcc/function.c	2010-08-14 11:48:10.621550234 +0100
***************
*** 3184,3190 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 3184,3191 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
***************
*** 3199,3205 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.first_conversion_insn);
  
    /* Estimate reload stack alignment from scalar return mode.  */
    if (SUPPORTS_STACK_ALIGNMENT)
--- 3200,3207 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.first_conversion_insn);
  
    /* Estimate reload stack alignment from scalar return mode.  */
    if (SUPPORTS_STACK_ALIGNMENT)
***************
*** 3351,3356 ****
--- 3353,3361 ----
    tree fnargs, parm;
    gimple_seq stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4456,4466 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4461,4475 ----
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       if (!cfun->custom_static_chain)
! 	{
! 	  set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
! 	  emit_move_insn (local, static_chain_incoming_rtx);
! 	}
!       /* else, the static chain will be set in the main body */
      }
  
    /* If the function receives a non-local goto, then store the
***************
*** 4981,4986 ****
--- 4990,4998 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+     return;
+ 
    rtl_profile_for_bb (ENTRY_BLOCK_PTR);
  #ifdef HAVE_prologue
    if (HAVE_prologue)
diff -cr gcc.orig/function.h gcc/function.h
*** gcc.orig/function.h	2009-09-23 15:58:58.000000000 +0100
--- gcc/function.h	2010-08-14 11:48:10.625562336 +0100
***************
*** 137,143 ****
    rtx x_forced_labels;
  };
  
! typedef struct call_site_record *call_site_record;
  DEF_VEC_P(call_site_record);
  DEF_VEC_ALLOC_P(call_site_record, gc);
  
--- 137,143 ----
    rtx x_forced_labels;
  };
  
! typedef struct call_site_record_d *call_site_record;
  DEF_VEC_P(call_site_record);
  DEF_VEC_ALLOC_P(call_site_record, gc);
  
***************
*** 175,186 ****
  struct gimple_df;
  struct temp_slot;
  typedef struct temp_slot *temp_slot_p;
! struct call_site_record;
  
  DEF_VEC_P(temp_slot_p);
  DEF_VEC_ALLOC_P(temp_slot_p,gc);
! struct ipa_opt_pass;
! typedef struct ipa_opt_pass *ipa_opt_pass;
  
  DEF_VEC_P(ipa_opt_pass);
  DEF_VEC_ALLOC_P(ipa_opt_pass,heap);
--- 175,186 ----
  struct gimple_df;
  struct temp_slot;
  typedef struct temp_slot *temp_slot_p;
! struct call_site_record_d;
  
  DEF_VEC_P(temp_slot_p);
  DEF_VEC_ALLOC_P(temp_slot_p,gc);
! struct ipa_opt_pass_d;
! typedef struct ipa_opt_pass_d *ipa_opt_pass;
  
  DEF_VEC_P(ipa_opt_pass);
  DEF_VEC_ALLOC_P(ipa_opt_pass,heap);
***************
*** 600,605 ****
--- 600,613 ----
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
*** gcc.orig/gcc.c	2010-01-09 00:05:06.000000000 +0000
--- gcc/gcc.c	2010-08-14 11:48:10.633539259 +0100
***************
*** 126,131 ****
--- 126,134 ----
  /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
  static bool at_file_supplied;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 373,378 ****
--- 376,384 ----
  static const char *version_compare_spec_function (int, const char **);
  static const char *include_spec_function (int, const char **);
  static const char *print_asm_header_spec_function (int, const char **);
+ 
+ extern const char *d_all_sources_spec_function (int, const char **);
+ extern const char *d_output_prefix_spec_function (int, const char **);
  
  /* The Specs Language
  
***************
*** 480,485 ****
--- 486,492 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 925,930 ****
--- 932,939 ----
  #endif
  #endif
  
+ #define GCC_SPEC_FORMAT_4 1
+ 
  /* Record the mapping from file suffixes for compilation specs.  */
  
  struct compiler
***************
*** 1647,1652 ****
--- 1656,1665 ----
    { "version-compare",		version_compare_spec_function },
    { "include",			include_spec_function },
    { "print-asm-header",		print_asm_header_spec_function },
+ #ifdef D_USE_EXTRA_SPEC_FUNCTIONS
+   { "d-all-sources",		d_all_sources_spec_function },
+   { "d-output-prefix",		d_output_prefix_spec_function },
+ #endif
  #ifdef EXTRA_SPEC_FUNCTIONS
    EXTRA_SPEC_FUNCTIONS
  #endif
***************
*** 4332,4337 ****
--- 4345,4362 ----
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
*** 5337,5342 ****
--- 5362,5378 ----
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
*** gcc.orig/gimple.c	2008-12-07 23:27:14.000000000 +0000
--- gcc/gimple.c	2010-08-14 11:48:10.641547051 +0100
***************
*** 2535,2540 ****
--- 2535,2543 ----
        || (SYM) == POLYNOMIAL_CHREC					    \
        || (SYM) == DOT_PROD_EXPR						    \
        || (SYM) == VEC_COND_EXPR						    \
+       || (SYM) == STATIC_CHAIN_DECL					    \
+       || (SYM) == STATIC_CHAIN_EXPR /* not sure if this is right...*/	    \
+       || (SYM) == VEC_COND_EXPR						    \
        || (SYM) == REALIGN_LOAD_EXPR) ? GIMPLE_SINGLE_RHS		    \
     : GIMPLE_INVALID_RHS),
  #define END_OF_BASE_TREE_CODES (unsigned char) GIMPLE_INVALID_RHS,
***************
*** 2615,2621 ****
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
--- 2618,2627 ----
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
*** 2851,2856 ****
--- 2857,2863 ----
    return (TREE_CODE (t) == VAR_DECL
  	  || TREE_CODE (t) == PARM_DECL
  	  || TREE_CODE (t) == RESULT_DECL
+ 	  || TREE_CODE (t) == STATIC_CHAIN_DECL
  	  || TREE_CODE (t) == SSA_NAME);
  }
  
diff -cr gcc.orig/gimple.h gcc/gimple.h
*** gcc.orig/gimple.h	2009-05-18 11:13:43.000000000 +0100
--- gcc/gimple.h	2010-08-14 11:48:10.649549536 +0100
***************
*** 65,71 ****
      const_gimple __gs = (GS);						\
      if (gimple_code (__gs) != (CODE))					\
        gimple_check_failed (__gs, __FILE__, __LINE__, __FUNCTION__,	\
! 	  		   (CODE), 0);					\
    } while (0)
  #else  /* not ENABLE_GIMPLE_CHECKING  */
  #define GIMPLE_CHECK(GS, CODE)			(void)0
--- 65,71 ----
      const_gimple __gs = (GS);						\
      if (gimple_code (__gs) != (CODE))					\
        gimple_check_failed (__gs, __FILE__, __LINE__, __FUNCTION__,	\
! 	  		   (CODE), ERROR_MARK);				\
    } while (0)
  #else  /* not ENABLE_GIMPLE_CHECKING  */
  #define GIMPLE_CHECK(GS, CODE)			(void)0
***************
*** 2216,2222 ****
  gimple_cond_code (const_gimple gs)
  {
    GIMPLE_CHECK (gs, GIMPLE_COND);
!   return gs->gsbase.subcode;
  }
  
  
--- 2216,2222 ----
  gimple_cond_code (const_gimple gs)
  {
    GIMPLE_CHECK (gs, GIMPLE_COND);
!   return (enum tree_code) gs->gsbase.subcode;
  }
  
  
diff -cr gcc.orig/gimplify.c gcc/gimplify.c
*** gcc.orig/gimplify.c	2010-04-08 12:47:13.000000000 +0100
--- gcc/gimplify.c	2010-08-14 11:48:10.657561937 +0100
***************
*** 6447,6452 ****
--- 6447,6458 ----
  	    }
  	  break;
  
+ 	case STATIC_CHAIN_EXPR:
+ 	  /* The argument is used as information only.  No need to gimplify */
+ 	case STATIC_CHAIN_DECL:
+ 	  ret = GS_ALL_DONE;
+ 	  break;
+ 
  	case TREE_LIST:
  	  gcc_unreachable ();
  
diff -cr gcc.orig/ipa-cp.c gcc/ipa-cp.c
*** gcc.orig/ipa-cp.c	2009-12-27 22:39:58.000000000 +0000
--- gcc/ipa-cp.c	2010-08-14 11:48:10.665539977 +0100
***************
*** 1393,1399 ****
    return flag_ipa_cp;
  }
  
! struct ipa_opt_pass pass_ipa_cp = 
  {
   {
    IPA_PASS,
--- 1393,1399 ----
    return flag_ipa_cp;
  }
  
! struct ipa_opt_pass_d pass_ipa_cp = 
  {
   {
    IPA_PASS,
diff -cr gcc.orig/ipa-inline.c gcc/ipa-inline.c
*** gcc.orig/ipa-inline.c	2010-02-08 14:10:15.000000000 +0000
--- gcc/ipa-inline.c	2010-08-14 11:48:10.669558086 +0100
***************
*** 1753,1759 ****
    return todo | execute_fixup_cfg ();
  }
  
! struct ipa_opt_pass pass_ipa_inline = 
  {
   {
    IPA_PASS,
--- 1753,1759 ----
    return todo | execute_fixup_cfg ();
  }
  
! struct ipa_opt_pass_d pass_ipa_inline = 
  {
   {
    IPA_PASS,
diff -cr gcc.orig/ipa-pure-const.c gcc/ipa-pure-const.c
*** gcc.orig/ipa-pure-const.c	2008-09-18 18:28:40.000000000 +0100
--- gcc/ipa-pure-const.c	2010-08-14 11:48:10.673553217 +0100
***************
*** 910,916 ****
  	  && !(errorcount || sorrycount));
  }
  
! struct ipa_opt_pass pass_ipa_pure_const =
  {
   {
    IPA_PASS,
--- 910,916 ----
  	  && !(errorcount || sorrycount));
  }
  
! struct ipa_opt_pass_d pass_ipa_pure_const =
  {
   {
    IPA_PASS,
diff -cr gcc.orig/ipa-reference.c gcc/ipa-reference.c
*** gcc.orig/ipa-reference.c	2008-09-18 18:28:40.000000000 +0100
--- gcc/ipa-reference.c	2010-08-14 11:48:10.673553217 +0100
***************
*** 1257,1263 ****
  	  && !(errorcount || sorrycount));
  }
  
! struct ipa_opt_pass pass_ipa_reference =
  {
   {
    IPA_PASS,
--- 1257,1263 ----
  	  && !(errorcount || sorrycount));
  }
  
! struct ipa_opt_pass_d pass_ipa_reference =
  {
   {
    IPA_PASS,
diff -cr gcc.orig/passes.c gcc/passes.c
*** gcc.orig/passes.c	2009-02-20 15:20:38.000000000 +0000
--- gcc/passes.c	2010-08-14 11:48:10.677535776 +0100
***************
*** 1146,1159 ****
  static void
  add_ipa_transform_pass (void *data)
  {
!   struct ipa_opt_pass *ipa_pass = (struct ipa_opt_pass *) data;
    VEC_safe_push (ipa_opt_pass, heap, cfun->ipa_transforms_to_apply, ipa_pass);
  }
  
  /* Execute summary generation for all of the passes in IPA_PASS.  */
  
  static void
! execute_ipa_summary_passes (struct ipa_opt_pass *ipa_pass)
  {
    while (ipa_pass)
      {
--- 1146,1159 ----
  static void
  add_ipa_transform_pass (void *data)
  {
!   struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *) data;
    VEC_safe_push (ipa_opt_pass, heap, cfun->ipa_transforms_to_apply, ipa_pass);
  }
  
  /* Execute summary generation for all of the passes in IPA_PASS.  */
  
  static void
! execute_ipa_summary_passes (struct ipa_opt_pass_d *ipa_pass)
  {
    while (ipa_pass)
      {
***************
*** 1167,1173 ****
  	  ipa_pass->generate_summary ();
  	  pass_fini_dump_file (pass);
  	}
!       ipa_pass = (struct ipa_opt_pass *)ipa_pass->pass.next;
      }
  }
  
--- 1167,1173 ----
  	  ipa_pass->generate_summary ();
  	  pass_fini_dump_file (pass);
  	}
!       ipa_pass = (struct ipa_opt_pass_d *)ipa_pass->pass.next;
      }
  }
  
***************
*** 1175,1181 ****
  
  static void
  execute_one_ipa_transform_pass (struct cgraph_node *node,
! 				struct ipa_opt_pass *ipa_pass)
  {
    struct opt_pass *pass = &ipa_pass->pass;
    unsigned int todo_after = 0;
--- 1175,1181 ----
  
  static void
  execute_one_ipa_transform_pass (struct cgraph_node *node,
! 				struct ipa_opt_pass_d *ipa_pass)
  {
    struct opt_pass *pass = &ipa_pass->pass;
    unsigned int todo_after = 0;
***************
*** 1347,1353 ****
  	    {
  	      if (!quiet_flag && !cfun)
  		fprintf (stderr, " <summary generate>");
! 	      execute_ipa_summary_passes ((struct ipa_opt_pass *) pass);
  	    }
  	  summaries_generated = true;
  	}
--- 1347,1353 ----
  	    {
  	      if (!quiet_flag && !cfun)
  		fprintf (stderr, " <summary generate>");
! 	      execute_ipa_summary_passes ((struct ipa_opt_pass_d *) pass);
  	    }
  	  summaries_generated = true;
  	}
diff -cr gcc.orig/system.h gcc/system.h
*** gcc.orig/system.h	2008-07-28 15:33:56.000000000 +0100
--- gcc/system.h	2010-08-14 11:48:10.681554164 +0100
***************
*** 786,791 ****
--- 786,794 ----
     change after the fact).  Beyond these uses, most other cases of
     using this macro should be viewed with extreme caution.  */
  
+ #ifdef __cplusplus
+ #define CONST_CAST2(TOTYPE,FROMTYPE,X) (const_cast<TOTYPE> (X))
+ #else
  #if defined(__GNUC__) && GCC_VERSION > 4000
  /* GCC 4.0.x has a bug where it may ICE on this expression,
     so does GCC 3.4.x (PR17436).  */
***************
*** 793,798 ****
--- 796,802 ----
  #else
  #define CONST_CAST2(TOTYPE,FROMTYPE,X) ((TOTYPE)(FROMTYPE)(X))
  #endif
+ #endif
  #define CONST_CAST(TYPE,X) CONST_CAST2(TYPE, const TYPE, (X))
  #define CONST_CAST_TREE(X) CONST_CAST(union tree_node *, (X))
  #define CONST_CAST_RTX(X) CONST_CAST(struct rtx_def *, (X))
diff -cr gcc.orig/tree.def gcc/tree.def
*** gcc.orig/tree.def	2009-02-20 15:20:38.000000000 +0000
--- gcc/tree.def	2010-08-14 11:48:10.681554164 +0100
***************
*** 554,559 ****
--- 554,566 ----
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
*** gcc.orig/tree-dump.c	2008-08-06 16:57:09.000000000 +0100
--- gcc/tree-dump.c	2010-08-14 11:48:10.685609848 +0100
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
diff -cr gcc.orig/tree-nested.c gcc/tree-nested.c
*** gcc.orig/tree-nested.c	2010-04-20 09:37:12.000000000 +0100
--- gcc/tree-nested.c	2010-08-17 13:34:21.612469750 +0100
***************
*** 714,719 ****
--- 714,721 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
***************
*** 1761,1766 ****
--- 1763,1772 ----
        if (DECL_NO_STATIC_CHAIN (decl))
  	break;
  
+       /* Don't use a trampoline for a static reference. */
+       if (TREE_STATIC (t))
+ 	break;
+ 
        /* If we don't want a trampoline, then don't build one.  */
        if (TREE_NO_TRAMPOLINE (t))
  	break;
***************
*** 1908,1913 ****
--- 1914,1938 ----
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
*** 2054,2061 ****
        gimple bind;
        annotate_all_with_location (stmt_list, DECL_SOURCE_LOCATION (context));
        bind = gimple_seq_first_stmt (gimple_body (context));
!       gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
!       gimple_bind_set_body (bind, stmt_list);
      }
  
    /* If a chain_decl was created, then it needs to be registered with
--- 2079,2116 ----
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
diff -cr gcc.orig/tree-pass.h gcc/tree-pass.h
*** gcc.orig/tree-pass.h	2009-02-20 15:20:38.000000000 +0000
--- gcc/tree-pass.h	2010-08-14 11:48:10.693548008 +0100
***************
*** 157,163 ****
  
  /* Description of IPA pass with generate summary, write, execute, read and
     transform stages.  */
! struct ipa_opt_pass
  {
    struct opt_pass pass;
  
--- 157,163 ----
  
  /* Description of IPA pass with generate summary, write, execute, read and
     transform stages.  */
! struct ipa_opt_pass_d
  {
    struct opt_pass pass;
  
***************
*** 390,399 ****
  extern struct gimple_opt_pass pass_reset_cc_flags;
  
  /* IPA Passes */
! extern struct ipa_opt_pass pass_ipa_inline;
! extern struct ipa_opt_pass pass_ipa_cp;
! extern struct ipa_opt_pass pass_ipa_reference;
! extern struct ipa_opt_pass pass_ipa_pure_const;
  
  extern struct simple_ipa_opt_pass pass_ipa_matrix_reorg;
  extern struct simple_ipa_opt_pass pass_ipa_early_inline;
--- 390,399 ----
  extern struct gimple_opt_pass pass_reset_cc_flags;
  
  /* IPA Passes */
! extern struct ipa_opt_pass_d pass_ipa_inline;
! extern struct ipa_opt_pass_d pass_ipa_cp;
! extern struct ipa_opt_pass_d pass_ipa_reference;
! extern struct ipa_opt_pass_d pass_ipa_pure_const;
  
  extern struct simple_ipa_opt_pass pass_ipa_matrix_reorg;
  extern struct simple_ipa_opt_pass pass_ipa_early_inline;
diff -cr gcc.orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc.orig/tree-pretty-print.c	2009-02-18 21:03:05.000000000 +0000
--- gcc/tree-pretty-print.c	2010-08-14 11:48:10.697541253 +0100
***************
*** 1216,1221 ****
--- 1216,1231 ----
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
*** gcc.orig/tree-sra.c	2010-04-18 16:56:32.000000000 +0100
--- gcc/tree-sra.c	2010-08-14 11:48:10.705548556 +0100
***************
*** 263,268 ****
--- 263,270 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
***************
*** 274,279 ****
--- 276,287 ----
  		      != TYPE_PRECISION (TREE_TYPE (t))))
  		goto fail;
  
+ 	      /* Reject aliased fields created by GDC for anonymous unions. */
+ 	      cmp = fold_binary_to_constant (LE_EXPR, boolean_type_node,
+ 					     DECL_FIELD_OFFSET (t), last_offset);
+ 	      if (cmp == NULL_TREE || tree_expr_nonzero_p (cmp))
+ 		goto fail;
+ 
  	      saw_one_field = true;
  	    }
  
diff -cr gcc.orig/tree-ssa.c gcc/tree-ssa.c
*** gcc.orig/tree-ssa.c	2009-08-03 20:27:32.000000000 +0100
--- gcc/tree-ssa.c	2010-08-15 09:49:44.658847183 +0100
***************
*** 1094,1101 ****
        && TYPE_CANONICAL (inner_type) == TYPE_CANONICAL (outer_type))
      return true;
  
!   /* Changes in machine mode are never useless conversions.  */
!   if (TYPE_MODE (inner_type) != TYPE_MODE (outer_type))
      return false;
  
    /* If both the inner and outer types are integral types, then the
--- 1094,1103 ----
        && TYPE_CANONICAL (inner_type) == TYPE_CANONICAL (outer_type))
      return true;
  
!   /* Changes in machine mode are never useless conversions unless we
!      deal with aggregate types in which case we defer to later checks.  */
!   if (TYPE_MODE (inner_type) != TYPE_MODE (outer_type)
!       && !AGGREGATE_TYPE_P (inner_type))
      return false;
  
    /* If both the inner and outer types are integral types, then the
