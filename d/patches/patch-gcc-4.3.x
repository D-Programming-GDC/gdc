diff -pcr gcc.orig/cgraph.c gcc/cgraph.c
*** gcc.orig/cgraph.c	2008-11-14 13:26:59.000000000 +0000
--- gcc/cgraph.c	2011-03-21 18:37:21.139990586 +0000
*************** struct cgraph_node *
*** 181,186 ****
--- 181,187 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
*************** cgraph_node (tree decl)
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
diff -pcr gcc.orig/cgraphunit.c gcc/cgraphunit.c
*** gcc.orig/cgraphunit.c	2008-11-14 13:26:59.000000000 +0000
--- gcc/cgraphunit.c	2011-03-21 18:37:21.143990613 +0000
*************** cgraph_mark_functions_to_output (void)
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
*************** cgraph_expand_function (struct cgraph_no
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
diff -pcr gcc.orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc.orig/config/i386/i386.c	2010-03-31 21:14:10.000000000 +0100
--- gcc/config/i386/i386.c	2011-03-21 19:42:27.571361547 +0000
*************** ix86_handle_cconv_attribute (tree *node,
*** 3145,3150 ****
--- 3145,3154 ----
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
*** 3163,3168 ****
--- 3167,3176 ----
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
*** 3176,3181 ****
--- 3184,3210 ----
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
*** 3391,3396 ****
--- 3420,3431 ----
            || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
  	rtd = 1;
  
+       /* Optlink functions will pop the stack if returning float and
+          if not variable args.  */
+       else if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+           && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+ 	rtd = 1;
+ 
        if (rtd && ! stdarg_p (funtype))
  	return size;
      }
*************** ix86_compute_frame_layout (struct ix86_f
*** 6151,6156 ****
--- 6186,6196 ----
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
*************** x86_output_mi_thunk (FILE *file ATTRIBUT
*** 22924,22930 ****
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 22964,22970 ----
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
*************** static const struct attribute_spec ix86_
*** 25240,25245 ****
--- 25280,25287 ----
    /* Sseregparm attribute says we are using x86_64 calling conventions
       for FP arguments.  */
    { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+   /* Optlink attribute says we are using D calling convention */
+   { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
    /* force_align_arg_pointer says this function realigns the stack at entry.  */
    { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
      false, true,  true, ix86_handle_cconv_attribute },
diff -pcr gcc.orig/config/i386/i386.h gcc/config/i386/i386.h
*** gcc.orig/config/i386/i386.h	2009-11-13 19:51:52.000000000 +0000
--- gcc/config/i386/i386.h	2011-03-21 19:39:45.910559903 +0000
*************** typedef struct ix86_args {
*** 1672,1677 ****
--- 1672,1678 ----
    int nregs;			/* # registers available for passing */
    int regno;			/* next available register number */
    int fastcall;			/* fastcall calling convention is used */
+   int optlink;			/* optlink calling convention is used */
    int sse_words;		/* # sse words passed so far */
    int sse_nregs;		/* # sse registers available for passing */
    int warn_sse;			/* True when we want to warn about SSE ABI.  */
diff -pcr gcc.orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc.orig/config/rs6000/rs6000.c	2009-09-23 23:30:05.000000000 +0100
--- gcc/config/rs6000/rs6000.c	2011-03-21 18:37:21.267991218 +0000
*************** rs6000_output_function_epilogue (FILE *f
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
diff -pcr gcc.orig/dwarf2out.c gcc/dwarf2out.c
*** gcc.orig/dwarf2out.c	2009-06-18 21:06:04.000000000 +0100
--- gcc/dwarf2out.c	2011-03-21 18:37:21.327991520 +0000
*************** is_c_family (void)
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
*************** gen_compile_unit_die (const char *filena
*** 13267,13272 ****
--- 13268,13275 ----
      language = DW_LANG_ObjC;
    else if (strcmp (language_string, "GNU Objective-C++") == 0)
      language = DW_LANG_ObjC_plus_plus;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
*************** dwarf2out_decl (tree decl)
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
diff -pcr gcc.orig/except.c gcc/except.c
*** gcc.orig/except.c	2008-06-28 11:43:12.000000000 +0100
--- gcc/except.c	2011-03-21 18:37:21.347991617 +0000
*************** sjlj_mark_call_sites (struct sjlj_lp_inf
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
diff -pcr gcc.orig/expr.c gcc/expr.c
*** gcc.orig/expr.c	2009-01-06 16:17:41.000000000 +0000
--- gcc/expr.c	2011-03-21 18:37:21.383991797 +0000
*************** expand_expr_real_1 (tree exp, rtx target
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
  
diff -pcr gcc.orig/function.c gcc/function.c
*** gcc.orig/function.c	2009-06-19 22:44:24.000000000 +0100
--- gcc/function.c	2011-03-21 18:37:21.407991915 +0000
*************** assign_parms (tree fndecl)
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
*************** assign_parms (tree fndecl)
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
*************** gimplify_parameters (void)
*** 3207,3212 ****
--- 3209,3217 ----
    struct assign_parm_data_all all;
    tree fnargs, parm, stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+   
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
*************** expand_function_start (tree subr)
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
*************** thread_prologue_and_epilogue_insns (void
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
diff -pcr gcc.orig/function.h gcc/function.h
*** gcc.orig/function.h	2008-01-26 17:18:35.000000000 +0000
--- gcc/function.h	2011-03-21 18:37:21.419991989 +0000
*************** struct function GTY(())
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
diff -pcr gcc.orig/gcc.c gcc/gcc.c
*** gcc.orig/gcc.c	2008-03-02 22:55:19.000000000 +0000
--- gcc/gcc.c	2011-03-21 18:37:21.455992151 +0000
*************** int is_cpp_driver;
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
*************** or with constant text in a single argume
*** 472,477 ****
--- 475,481 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
*************** warranty; not even for MERCHANTABILITY o
*** 3974,3979 ****
--- 3978,3986 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if (save_temps_flag && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
*************** warranty; not even for MERCHANTABILITY o
*** 4280,4285 ****
--- 4287,4304 ----
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
*************** do_spec_1 (const char *spec, int inswitc
*** 5240,5245 ****
--- 5259,5275 ----
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
diff -pcr gcc.orig/gcc.h gcc/gcc.h
*** gcc.orig/gcc.h	2007-07-26 09:37:01.000000000 +0100
--- gcc/gcc.h	2011-03-21 18:37:21.455992151 +0000
*************** struct spec_function
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
  
diff -pcr gcc.orig/tree.def gcc/tree.def
*** gcc.orig/tree.def	2007-10-29 11:05:04.000000000 +0000
--- gcc/tree.def	2011-03-21 18:37:21.463992196 +0000
*************** DEFTREECODE (BIND_EXPR, "bind_expr", tcc
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
diff -pcr gcc.orig/tree-gimple.c gcc/tree-gimple.c
*** gcc.orig/tree-gimple.c	2007-12-13 21:49:09.000000000 +0000
--- gcc/tree-gimple.c	2011-03-21 18:37:21.475992250 +0000
*************** is_gimple_formal_tmp_rhs (tree t)
*** 74,79 ****
--- 74,81 ----
      case VECTOR_CST:
      case OBJ_TYPE_REF:
      case ASSERT_EXPR:
+     case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+     case STATIC_CHAIN_DECL:
        return true;
  
      default:
*************** is_gimple_lvalue (tree t)
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
diff -pcr gcc.orig/tree-nested.c gcc/tree-nested.c
*** gcc.orig/tree-nested.c	2008-05-29 12:35:05.000000000 +0100
--- gcc/tree-nested.c	2011-03-21 18:37:21.483992295 +0000
*************** get_static_chain (struct nesting_info *i
*** 815,820 ****
--- 815,822 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
*************** convert_tramp_reference (tree *tp, int *
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
*************** convert_call_expr (tree *tp, int *walk_s
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
*************** finalize_nesting_tree_1 (struct nesting_
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
diff -pcr gcc.orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc.orig/tree-pretty-print.c	2008-01-27 16:48:54.000000000 +0000
--- gcc/tree-pretty-print.c	2011-03-21 18:37:21.491992325 +0000
*************** dump_generic_node (pretty_printer *buffe
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
diff -pcr gcc.orig/tree-sra.c gcc/tree-sra.c
*** gcc.orig/tree-sra.c	2010-04-18 16:56:56.000000000 +0100
--- gcc/tree-sra.c	2011-03-21 18:37:21.499992373 +0000
*************** sra_type_can_be_decomposed_p (tree type)
*** 262,267 ****
--- 262,269 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
*************** sra_type_can_be_decomposed_p (tree type)
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
