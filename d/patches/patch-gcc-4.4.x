diff -pcr gcc.orig/cgraph.c gcc/cgraph.c
*** gcc.orig/cgraph.c	2008-11-16 22:31:58.000000000 +0000
--- gcc/cgraph.c	2011-03-21 18:37:28.108025146 +0000
*************** struct cgraph_node *
*** 451,456 ****
--- 451,457 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
*************** cgraph_node (tree decl)
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
diff -pcr gcc.orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc.orig/config/i386/i386.c	2010-08-06 08:52:04.000000000 +0100
--- gcc/config/i386/i386.c	2011-03-21 19:46:00.804418905 +0000
*************** ix86_handle_cconv_attribute (tree *node,
*** 4270,4275 ****
--- 4270,4279 ----
          {
  	  error ("fastcall and stdcall attributes are not compatible");
  	}
+       if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+         {
+ 	  error ("fastcall and optlink attributes are not compatible");
+ 	}
        if (lookup_attribute ("regparm", TYPE_ATTRIBUTES (*node)))
          {
  	  error ("fastcall and regparm attributes are not compatible");
*************** ix86_handle_cconv_attribute (tree *node,
*** 4288,4293 ****
--- 4292,4301 ----
          {
  	  error ("stdcall and fastcall attributes are not compatible");
  	}
+       if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+         {
+ 	  error ("stdcall and optlink attributes are not compatible");
+ 	}
      }
  
    /* Can combine cdecl with regparm and sseregparm.  */
*************** ix86_handle_cconv_attribute (tree *node,
*** 4301,4306 ****
--- 4309,4335 ----
          {
  	  error ("fastcall and cdecl attributes are not compatible");
  	}
+       if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+         {
+ 	  error ("optlink and cdecl attributes are not compatible");
+ 	}
+     }
+ 
+   /* Can combine optlink with regparm and sseregparm.  */
+   else if (is_attribute_p ("optlink", name))
+     {
+       if (lookup_attribute ("stdcall", TYPE_ATTRIBUTES (*node)))
+         {
+ 	  error ("optlink and stdcall attributes are not compatible");
+ 	}
+       if (lookup_attribute ("fastcall", TYPE_ATTRIBUTES (*node)))
+         {
+ 	  error ("optlink and fastcall attributes are not compatible");
+ 	}
+       if (lookup_attribute ("cdecl", TYPE_ATTRIBUTES (*node)))
+         {
+ 	  error ("optlink and cdecl attributes are not compatible");
+ 	}
      }
  
    /* Can combine sseregparm with all attributes.  */
*************** ix86_return_pops_args (tree fundecl, tre
*** 4537,4542 ****
--- 4566,4577 ----
            || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
  	rtd = 1;
  
+       /* Optlink functions will pop the stack if returning float and
+          if not variable args.  */
+       if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+           && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+ 	rtd = 1;
+ 
        if (rtd && ! stdarg_p (funtype))
  	return size;
      }
*************** ix86_compute_frame_layout (struct ix86_f
*** 7879,7884 ****
--- 7914,7924 ----
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
*************** x86_output_mi_thunk (FILE *file ATTRIBUT
*** 26795,26801 ****
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 26835,26841 ----
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
*************** static const struct attribute_spec ix86_
*** 29551,29556 ****
--- 29591,29598 ----
    /* Sseregparm attribute says we are using x86_64 calling conventions
       for FP arguments.  */
    { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+   /* Optlink attribute says we are using D calling convention */
+   { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
    /* force_align_arg_pointer says this function realigns the stack at entry.  */
    { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
      false, true,  true, ix86_handle_cconv_attribute },
diff -pcr gcc.orig/config/i386/i386.h gcc/config/i386/i386.h
*** gcc.orig/config/i386/i386.h	2009-11-13 19:13:16.000000000 +0000
--- gcc/config/i386/i386.h	2011-03-21 19:43:34.955695689 +0000
*************** typedef struct ix86_args {
*** 1572,1577 ****
--- 1572,1578 ----
    int nregs;			/* # registers available for passing */
    int regno;			/* next available register number */
    int fastcall;			/* fastcall calling convention is used */
+   int optlink;			/* optlink calling convention is used */
    int sse_words;		/* # sse words passed so far */
    int sse_nregs;		/* # sse registers available for passing */
    int warn_avx;			/* True when we want to warn about AVX ABI.  */
diff -pcr gcc.orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc.orig/config/rs6000/rs6000.c	2010-06-04 05:57:21.000000000 +0100
--- gcc/config/rs6000/rs6000.c	2011-03-21 18:37:28.292026049 +0000
*************** rs6000_output_function_epilogue (FILE *f
*** 17665,17671 ****
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU Fortran"))
--- 17665,17672 ----
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C")
! 	  || ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU Fortran"))
diff -pcr gcc.orig/dojump.c gcc/dojump.c
*** gcc.orig/dojump.c	2010-03-08 11:46:28.000000000 +0000
--- gcc/dojump.c	2011-03-21 18:37:28.304026114 +0000
*************** void
*** 80,86 ****
  clear_pending_stack_adjust (void)
  {
    if (optimize > 0
!       && (! flag_omit_frame_pointer || cfun->calls_alloca)
        && EXIT_IGNORE_STACK)
      discard_pending_stack_adjust ();
  }
--- 80,87 ----
  clear_pending_stack_adjust (void)
  {
    if (optimize > 0
!       && ((! flag_omit_frame_pointer && ! cfun->naked)
!           || cfun->calls_alloca)
        && EXIT_IGNORE_STACK)
      discard_pending_stack_adjust ();
  }
diff -pcr gcc.orig/dwarf2out.c gcc/dwarf2out.c
*** gcc.orig/dwarf2out.c	2010-02-10 15:09:06.000000000 +0000
--- gcc/dwarf2out.c	2011-03-21 18:37:28.356026367 +0000
*************** is_c_family (void)
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
*************** gen_compile_unit_die (const char *filena
*** 14506,14511 ****
--- 14507,14514 ----
      language = DW_LANG_ObjC;
    else if (strcmp (language_string, "GNU Objective-C++") == 0)
      language = DW_LANG_ObjC_plus_plus;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
*************** dwarf2out_decl (tree decl)
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
diff -pcr gcc.orig/except.c gcc/except.c
*** gcc.orig/except.c	2010-03-08 11:46:28.000000000 +0000
--- gcc/except.c	2011-03-21 18:37:28.368026429 +0000
*************** struct eh_region GTY(())
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
*************** sjlj_mark_call_sites (struct sjlj_lp_inf
*** 1777,1782 ****
--- 1777,1794 ----
  
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
*************** add_call_site (rtx landing_pad, int acti
*** 3198,3204 ****
  {
    call_site_record record;
    
!   record = GGC_NEW (struct call_site_record);
    record->landing_pad = landing_pad;
    record->action = action;
  
--- 3210,3216 ----
  {
    call_site_record record;
    
!   record = GGC_NEW (struct call_site_record_d);
    record->landing_pad = landing_pad;
    record->action = action;
  
*************** dw2_size_of_call_site_table (void)
*** 3399,3405 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (cs->action);
      }
  
--- 3411,3417 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (cs->action);
      }
  
*************** sjlj_size_of_call_site_table (void)
*** 3415,3421 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (INTVAL (cs->landing_pad));
        size += size_of_uleb128 (cs->action);
      }
--- 3427,3433 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        size += size_of_uleb128 (INTVAL (cs->landing_pad));
        size += size_of_uleb128 (cs->action);
      }
*************** dw2_output_call_site_table (void)
*** 3432,3438 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        char reg_start_lab[32];
        char reg_end_lab[32];
        char landing_pad_lab[32];
--- 3444,3450 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
        char reg_start_lab[32];
        char reg_end_lab[32];
        char landing_pad_lab[32];
*************** sjlj_output_call_site_table (void)
*** 3486,3492 ****
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
  
        dw2_asm_output_data_uleb128 (INTVAL (cs->landing_pad),
  				   "region %d landing pad", i);
--- 3498,3504 ----
  
    for (i = 0; i < n; ++i)
      {
!       struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
  
        dw2_asm_output_data_uleb128 (INTVAL (cs->landing_pad),
  				   "region %d landing pad", i);
diff -pcr gcc.orig/expr.c gcc/expr.c
*** gcc.orig/expr.c	2010-09-23 08:41:30.000000000 +0100
--- gcc/expr.c	2011-03-21 18:37:28.396026575 +0000
*************** expand_expr_real_1 (tree exp, rtx target
*** 9351,9356 ****
--- 9351,9361 ----
  	 represent.  */
        return const0_rtx;
  
+     case STATIC_CHAIN_EXPR:
+     case STATIC_CHAIN_DECL:
+       /* Lowered by tree-nested.c */
+       gcc_unreachable ();
+ 
      case EXC_PTR_EXPR:
        return get_exception_pointer ();
  
diff -pcr gcc.orig/function.c gcc/function.c
*** gcc.orig/function.c	2010-08-16 21:24:54.000000000 +0100
--- gcc/function.c	2011-03-21 18:37:28.412026653 +0000
*************** assign_parms (tree fndecl)
*** 3190,3196 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 3190,3197 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
*************** assign_parms (tree fndecl)
*** 3205,3211 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.first_conversion_insn);
  
    /* Estimate reload stack alignment from scalar return mode.  */
    if (SUPPORTS_STACK_ALIGNMENT)
--- 3206,3213 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.first_conversion_insn);
  
    /* Estimate reload stack alignment from scalar return mode.  */
    if (SUPPORTS_STACK_ALIGNMENT)
*************** gimplify_parameters (void)
*** 3357,3362 ****
--- 3359,3367 ----
    tree fnargs, parm;
    gimple_seq stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
*************** expand_function_start (tree subr)
*** 4460,4470 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4465,4479 ----
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
*************** thread_prologue_and_epilogue_insns (void
*** 4985,4990 ****
--- 4994,5002 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+     return;
+ 
    rtl_profile_for_bb (ENTRY_BLOCK_PTR);
  #ifdef HAVE_prologue
    if (HAVE_prologue)
diff -pcr gcc.orig/function.h gcc/function.h
*** gcc.orig/function.h	2009-09-23 15:58:58.000000000 +0100
--- gcc/function.h	2011-03-21 18:37:28.420026685 +0000
*************** struct expr_status GTY(())
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
  
*************** struct rtl_eh GTY(())
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
*************** struct function GTY(())
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
diff -pcr gcc.orig/gcc.c gcc/gcc.c
*** gcc.orig/gcc.c	2010-01-09 00:05:06.000000000 +0000
--- gcc/gcc.c	2011-03-21 18:37:28.448026828 +0000
*************** int is_cpp_driver;
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
*************** or with constant text in a single argume
*** 480,485 ****
--- 483,489 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
*************** warranty; not even for MERCHANTABILITY o
*** 4022,4027 ****
--- 4026,4034 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+  
    if (save_temps_flag && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
*************** warranty; not even for MERCHANTABILITY o
*** 4332,4337 ****
--- 4339,4356 ----
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
*************** do_spec_1 (const char *spec, int inswitc
*** 5337,5342 ****
--- 5356,5372 ----
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
diff -pcr gcc.orig/gimple.c gcc/gimple.c
*** gcc.orig/gimple.c	2008-12-07 23:27:14.000000000 +0000
--- gcc/gimple.c	2011-03-21 18:37:28.480026985 +0000
*************** get_gimple_rhs_num_ops (enum tree_code c
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
*************** is_gimple_lvalue (tree t)
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
*************** is_gimple_variable (tree t)
*** 2851,2856 ****
--- 2857,2863 ----
    return (TREE_CODE (t) == VAR_DECL
  	  || TREE_CODE (t) == PARM_DECL
  	  || TREE_CODE (t) == RESULT_DECL
+ 	  || TREE_CODE (t) == STATIC_CHAIN_DECL
  	  || TREE_CODE (t) == SSA_NAME);
  }
  
diff -pcr gcc.orig/gimple.h gcc/gimple.h
*** gcc.orig/gimple.h	2009-05-18 11:13:43.000000000 +0100
--- gcc/gimple.h	2011-03-21 18:37:28.508027129 +0000
*************** extern void gimple_check_failed (const_g
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
*************** static inline enum tree_code
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
  
  
diff -pcr gcc.orig/ipa-cp.c gcc/ipa-cp.c
*** gcc.orig/ipa-cp.c	2009-12-27 22:39:58.000000000 +0000
--- gcc/ipa-cp.c	2011-03-21 18:37:28.528027223 +0000
*************** cgraph_gate_cp (void)
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
diff -pcr gcc.orig/ipa-inline.c gcc/ipa-inline.c
*** gcc.orig/ipa-inline.c	2010-02-08 14:10:15.000000000 +0000
--- gcc/ipa-inline.c	2011-03-21 18:37:28.556027367 +0000
*************** inline_transform (struct cgraph_node *no
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
diff -pcr gcc.orig/ipa-pure-const.c gcc/ipa-pure-const.c
*** gcc.orig/ipa-pure-const.c	2008-09-18 18:28:40.000000000 +0100
--- gcc/ipa-pure-const.c	2011-03-21 18:37:28.556027367 +0000
*************** gate_pure_const (void)
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
diff -pcr gcc.orig/ipa-reference.c gcc/ipa-reference.c
*** gcc.orig/ipa-reference.c	2008-09-18 18:28:40.000000000 +0100
--- gcc/ipa-reference.c	2011-03-21 18:37:28.560027381 +0000
*************** gate_reference (void)
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
diff -pcr gcc.orig/ira.c gcc/ira.c
*** gcc.orig/ira.c	2010-09-09 14:58:24.000000000 +0100
--- gcc/ira.c	2011-03-21 18:37:28.572027448 +0000
*************** setup_eliminable_regset (void)
*** 1404,1410 ****
       case.  At some point, we should improve this by emitting the
       sp-adjusting insns for this case.  */
    int need_fp
!     = (! flag_omit_frame_pointer
         || (cfun->calls_alloca && EXIT_IGNORE_STACK)
         || crtl->accesses_prior_frames
         || crtl->stack_realign_needed
--- 1404,1410 ----
       case.  At some point, we should improve this by emitting the
       sp-adjusting insns for this case.  */
    int need_fp
!     = ((! flag_omit_frame_pointer && ! cfun->naked)
         || (cfun->calls_alloca && EXIT_IGNORE_STACK)
         || crtl->accesses_prior_frames
         || crtl->stack_realign_needed
diff -pcr gcc.orig/passes.c gcc/passes.c
*** gcc.orig/passes.c	2009-02-20 15:20:38.000000000 +0000
--- gcc/passes.c	2011-03-21 18:37:28.588027527 +0000
*************** update_properties_after_pass (void *data
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
*************** execute_ipa_summary_passes (struct ipa_o
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
  
*************** execute_ipa_summary_passes (struct ipa_o
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
*************** execute_ipa_pass_list (struct opt_pass *
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
diff -pcr gcc.orig/system.h gcc/system.h
*** gcc.orig/system.h	2008-07-28 15:33:56.000000000 +0100
--- gcc/system.h	2011-03-21 18:37:28.596027557 +0000
*************** extern void fancy_abort (const char *, i
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
*************** extern void fancy_abort (const char *, i
*** 793,798 ****
--- 796,802 ----
  #else
  #define CONST_CAST2(TOTYPE,FROMTYPE,X) ((TOTYPE)(FROMTYPE)(X))
  #endif
+ #endif
  #define CONST_CAST(TYPE,X) CONST_CAST2(TYPE, const TYPE, (X))
  #define CONST_CAST_TREE(X) CONST_CAST(union tree_node *, (X))
  #define CONST_CAST_RTX(X) CONST_CAST(struct rtx_def *, (X))
diff -pcr gcc.orig/tree.def gcc/tree.def
*** gcc.orig/tree.def	2009-02-20 15:20:38.000000000 +0000
--- gcc/tree.def	2011-03-21 18:37:28.624027699 +0000
*************** DEFTREECODE (BIND_EXPR, "bind_expr", tcc
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
diff -pcr gcc.orig/tree-inline.c gcc/tree-inline.c
*** gcc.orig/tree-inline.c	2010-09-18 18:23:20.000000000 +0100
--- gcc/tree-inline.c	2011-03-21 18:37:28.648027815 +0000
*************** initialize_cfun (tree new_fndecl, tree c
*** 1832,1837 ****
--- 1832,1838 ----
  
    /* Copy items we preserve during clonning.  */
    cfun->static_chain_decl = src_cfun->static_chain_decl;
+   cfun->custom_static_chain = src_cfun->custom_static_chain;
    cfun->nonlocal_goto_save_area = src_cfun->nonlocal_goto_save_area;
    cfun->function_end_locus = src_cfun->function_end_locus;
    cfun->curr_properties = src_cfun->curr_properties;
*************** initialize_inlined_parameters (copy_body
*** 2206,2212 ****
    /* Initialize the static chain.  */
    p = DECL_STRUCT_FUNCTION (fn)->static_chain_decl;
    gcc_assert (fn != current_function_decl);
!   if (p)
      {
        /* No static chain?  Seems like a bug in tree-nested.c.  */
        gcc_assert (static_chain);
--- 2207,2214 ----
    /* Initialize the static chain.  */
    p = DECL_STRUCT_FUNCTION (fn)->static_chain_decl;
    gcc_assert (fn != current_function_decl);
!   /* Custom static chain has already been dealt with.  */
!   if (p && ! DECL_STRUCT_FUNCTION(fn)->custom_static_chain)
      {
        /* No static chain?  Seems like a bug in tree-nested.c.  */
        gcc_assert (static_chain);
diff -pcr gcc.orig/tree-nested.c gcc/tree-nested.c
*** gcc.orig/tree-nested.c	2010-04-20 09:37:12.000000000 +0100
--- gcc/tree-nested.c	2011-03-21 18:37:28.656027857 +0000
*************** get_static_chain (struct nesting_info *i
*** 714,719 ****
--- 714,721 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
*************** convert_tramp_reference_op (tree *tp, in
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
*************** convert_gimple_call (gimple_stmt_iterato
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
*************** finalize_nesting_tree_1 (struct nesting_
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
diff -pcr gcc.orig/tree-pass.h gcc/tree-pass.h
*** gcc.orig/tree-pass.h	2009-02-20 15:20:38.000000000 +0000
--- gcc/tree-pass.h	2011-03-21 18:37:28.660027875 +0000
*************** struct cgraph_node;
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
  
*************** extern struct gimple_opt_pass pass_build
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
diff -pcr gcc.orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc.orig/tree-pretty-print.c	2009-02-18 21:03:05.000000000 +0000
--- gcc/tree-pretty-print.c	2011-03-21 18:37:28.680027975 +0000
*************** dump_generic_node (pretty_printer *buffe
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
diff -pcr gcc.orig/tree-sra.c gcc/tree-sra.c
*** gcc.orig/tree-sra.c	2010-04-18 16:56:32.000000000 +0100
--- gcc/tree-sra.c	2011-03-21 18:37:28.692028033 +0000
*************** sra_type_can_be_decomposed_p (tree type)
*** 263,268 ****
--- 263,270 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
*************** sra_type_can_be_decomposed_p (tree type)
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
  
diff -pcr gcc.orig/tree-ssa.c gcc/tree-ssa.c
*** gcc.orig/tree-ssa.c	2009-08-03 20:27:32.000000000 +0100
--- gcc/tree-ssa.c	2011-03-21 18:37:28.700028081 +0000
*************** useless_type_conversion_p_1 (tree outer_
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
