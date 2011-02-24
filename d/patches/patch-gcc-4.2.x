diff -cr gcc-orig/cgraph.c gcc/cgraph.c
*** gcc-orig/cgraph.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/cgraph.c	2010-08-22 20:09:26.026318962 -0400
***************
*** 198,203 ****
--- 198,204 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 219,230 ****
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
--- 220,235 ----
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
diff -cr gcc-orig/config/arm/arm.c gcc/config/arm/arm.c
*** gcc-orig/config/arm/arm.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/config/arm/arm.c	2010-08-22 20:09:26.030319960 -0400
***************
*** 15485,15490 ****
--- 15485,15499 ----
  	  asm_fprintf (asm_out_file, "\t.movsp %r, #%d\n",
  		       REGNO (e0), (int)INTVAL(XEXP (e1, 1)));
  	}
+       else if (GET_CODE (e1) == PLUS
+ 	      && GET_CODE (XEXP (e1, 0)) == REG
+ 	      && REGNO (XEXP (e1, 0)) == SP_REGNUM
+ 	      && GET_CODE (XEXP (e1, 1)) == CONST_INT)
+ 	{
+ 	  /* Set reg to offset from sp.  */
+ 	  asm_fprintf (asm_out_file, "\t.movsp %r, #%d\n",
+ 		       REGNO (e0), (int)INTVAL(XEXP (e1, 1)));
+ 	}
        else
  	abort ();
        break;
diff -cr gcc-orig/config/darwin.h gcc/config/darwin.h
*** gcc-orig/config/darwin.h	2007-09-01 11:28:30.000000000 -0400
--- gcc/config/darwin.h	2010-08-22 20:09:26.034318095 -0400
***************
*** 759,765 ****
  /* Macros defining the various PIC cases.  */
  
  #define MACHO_DYNAMIC_NO_PIC_P	(TARGET_DYNAMIC_NO_PIC)
! #define MACHOPIC_INDIRECT	(flag_pic || MACHO_DYNAMIC_NO_PIC_P)
  #define MACHOPIC_JUST_INDIRECT	(MACHO_DYNAMIC_NO_PIC_P)
  #define MACHOPIC_PURE		(flag_pic && ! MACHO_DYNAMIC_NO_PIC_P)
  
--- 759,765 ----
  /* Macros defining the various PIC cases.  */
  
  #define MACHO_DYNAMIC_NO_PIC_P	(TARGET_DYNAMIC_NO_PIC)
! #define MACHOPIC_INDIRECT	(flag_pic == 1 || MACHO_DYNAMIC_NO_PIC_P)
  #define MACHOPIC_JUST_INDIRECT	(MACHO_DYNAMIC_NO_PIC_P)
  #define MACHOPIC_PURE		(flag_pic && ! MACHO_DYNAMIC_NO_PIC_P)
  
diff -cr gcc-orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc-orig/config/i386/i386.c	2007-12-13 04:25:46.000000000 -0500
--- gcc/config/i386/i386.c	2010-08-22 20:09:26.046320530 -0400
***************
*** 5217,5222 ****
--- 5217,5227 ----
      frame->red_zone_size = 0;
    frame->to_allocate -= frame->red_zone_size;
    frame->stack_pointer_offset -= frame->red_zone_size;
+ 
+   if (cfun->naked)
+       /* As above, skip return address */
+       frame->stack_pointer_offset = UNITS_PER_WORD;
+ 
  #if 0
    fprintf (stderr, "nregs: %i\n", frame->nregs);
    fprintf (stderr, "size: %i\n", size);
***************
*** 17732,17738 ****
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 17737,17743 ----
  	  output_set_got (tmp, NULL_RTX);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc-orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc-orig/config/rs6000/rs6000.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/config/rs6000/rs6000.c	2010-08-22 20:09:26.082318825 -0400
***************
*** 15482,15488 ****
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
--- 15482,15489 ----
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
  	 a number, so for now use 9.  */
!       if (! strcmp (language_string, "GNU C") ||
!       	! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
diff -cr gcc-orig/dwarf2out.c gcc/dwarf2out.c
*** gcc-orig/dwarf2out.c	2007-10-10 05:29:13.000000000 -0400
--- gcc/dwarf2out.c	2010-08-22 20:09:26.094317488 -0400
***************
*** 5406,5412 ****
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
! 	  || lang == DW_LANG_C99
  	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
  }
  
--- 5406,5412 ----
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
! 	  || lang == DW_LANG_C99 || lang == DW_LANG_D
  	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
  }
  
***************
*** 12381,12386 ****
--- 12381,12388 ----
      language = DW_LANG_ObjC;
    else if (strcmp (language_string, "GNU Objective-C++") == 0)
      language = DW_LANG_ObjC_plus_plus;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
***************
*** 13475,13481 ****
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (DECL_CONTEXT (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
--- 13477,13483 ----
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (decl_function_context (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
diff -cr gcc-orig/except.c gcc/except.c
*** gcc-orig/except.c	2007-09-01 16:28:30.000000000 +0100
--- gcc/except.c	2010-09-01 17:58:16.098347399 +0100
***************
*** 1816,1821 ****
--- 1816,1833 ----
  
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
*** gcc-orig/expr.c	2008-02-04 17:03:09.000000000 -0500
--- gcc/expr.c	2010-08-22 20:09:26.098318137 -0400
***************
*** 8740,8745 ****
--- 8740,8750 ----
        /* Lowered by gimplify.c.  */
        gcc_unreachable ();
  
+     case STATIC_CHAIN_EXPR:
+     case STATIC_CHAIN_DECL:
+       /* Lowered by tree-nested.c */
+       gcc_unreachable ();
+ 
      case EXC_PTR_EXPR:
        return get_exception_pointer (cfun);
  
diff -cr gcc-orig/function.c gcc/function.c
*** gcc-orig/function.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/function.c	2010-08-22 20:09:26.102317179 -0400
***************
*** 3022,3028 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 3022,3029 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
***************
*** 3037,3043 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
--- 3038,3045 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
***************
*** 3167,3172 ****
--- 3169,3177 ----
    struct assign_parm_data_all all;
    tree fnargs, parm, stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4148,4158 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4153,4167 ----
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       if (! cfun->custom_static_chain)
!         {
! 	    set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
! 	    emit_move_insn (local, static_chain_incoming_rtx);
! 	}
!       /* else, the static chain will be set in the main body */
      }
  
    /* If the function receives a non-local goto, then store the
***************
*** 5065,5070 ****
--- 5074,5082 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+       return;
+ 
  #ifdef HAVE_prologue
    if (HAVE_prologue)
      {
diff -cr gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	2007-09-01 11:28:30.000000000 -0400
--- gcc/function.h	2010-08-22 20:09:26.102317179 -0400
***************
*** 462,467 ****
--- 462,475 ----
    /* Number of units of floating point registers that need saving in stdarg
       function.  */
    unsigned int va_list_fpr_size : 8;
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
*** gcc-orig/gcc.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/gcc.c	2010-08-22 20:09:26.106318526 -0400
***************
*** 126,131 ****
--- 126,134 ----
  /* Flag set by cppspec.c to 1.  */
  int is_cpp_driver;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 354,359 ****
--- 357,365 ----
  static const char *replace_outfile_spec_function (int, const char **);
  static const char *version_compare_spec_function (int, const char **);
  static const char *include_spec_function (int, const char **);
+ 
+ extern const char *d_all_sources_spec_function (int, const char **);
+ extern const char *d_output_prefix_spec_function (int, const char **);
  
  /* The Specs Language
  
***************
*** 461,466 ****
--- 467,473 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 1595,1600 ****
--- 1604,1613 ----
    { "replace-outfile",		replace_outfile_spec_function },
    { "version-compare",		version_compare_spec_function },
    { "include",			include_spec_function },
+ #ifdef D_USE_EXTRA_SPEC_FUNCTIONS
+   { "d-all-sources",		d_all_sources_spec_function },
+   { "d-output-prefix",		d_output_prefix_spec_function },
+ #endif
  #ifdef EXTRA_SPEC_FUNCTIONS
    EXTRA_SPEC_FUNCTIONS
  #endif
***************
*** 3927,3932 ****
--- 3940,3948 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if (save_temps_flag && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4265,4270 ****
--- 4281,4298 ----
  	}
      }
  
+   if (need_pthreads)
+     {
+ 	switches[n_switches].part1 = "pthread";
+ 	switches[n_switches].args = 0;
+ 	switches[n_switches].live_cond = SWITCH_OK;
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
*** 5197,5202 ****
--- 5225,5241 ----
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
*** gcc-orig/gcc.h	2007-09-01 11:28:30.000000000 -0400
--- gcc/gcc.h	2010-08-22 20:09:26.106318526 -0400
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
  
diff -cr gcc-orig/predict.c gcc/predict.c
*** gcc-orig/predict.c	2008-01-24 10:59:18.000000000 -0500
--- gcc/predict.c	2010-08-22 20:09:26.110318756 -0400
***************
*** 1318,1323 ****
--- 1318,1324 ----
  	     care for error returns and other cases are often used for
  	     fast paths through function.  */
  	  if (e->dest == EXIT_BLOCK_PTR
+ 	      && last_stmt (bb) == NULL_TREE
  	      && (tmp = last_stmt (bb))
  	      && TREE_CODE (tmp) == RETURN_EXPR
  	      && !single_pred_p (bb))
diff -cr gcc-orig/tree.def gcc/tree.def
*** gcc-orig/tree.def	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree.def	2010-08-22 20:09:26.110318756 -0400
***************
*** 527,532 ****
--- 527,539 ----
     Operand 2 is the static chain argument, or NULL.  */
  DEFTREECODE (CALL_EXPR, "call_expr", tcc_expression, 3)
  
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
*** gcc-orig/tree-gimple.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree-gimple.c	2010-08-22 20:09:26.110318756 -0400
***************
*** 71,76 ****
--- 71,78 ----
      case VECTOR_CST:
      case OBJ_TYPE_REF:
      case ASSERT_EXPR:
+     case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+     case STATIC_CHAIN_DECL:
        return true;
  
      default:
***************
*** 144,150 ****
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
--- 146,155 ----
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF
!           /* This is an lvalue because it will be replaced with the real
! 	     static chain decl. */
! 	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
diff -cr gcc-orig/tree-inline.c gcc/tree-inline.c
*** gcc-orig/tree-inline.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree-inline.c	2010-08-22 20:09:26.110318756 -0400
***************
*** 554,563 ****
       knows not to copy VAR_DECLs, etc., so this is safe.  */
    else
      {
        /* Here we handle trees that are not completely rewritten.
  	 First we detect some inlining-induced bogosities for
  	 discarding.  */
!       if (TREE_CODE (*tp) == MODIFY_EXPR
  	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
  	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
  	      (TREE_OPERAND (*tp, 0), fn)))
--- 554,574 ----
       knows not to copy VAR_DECLs, etc., so this is safe.  */
    else
      {
+       if (id->transform_call_graph_edges == CB_CGE_MOVE &&
+ 	  TREE_CODE (*tp) == MODIFY_EXPR &&
+ 	  TREE_OPERAND (*tp, 0) ==
+ 	  DECL_STRUCT_FUNCTION (fn)->static_chain_decl)
+ 	{
+ 	  /* Don't use special methods to initialize the static chain
+ 	     if expanding inline.  If this code could somehow be
+ 	     expanded in expand_start_function, it would not be
+ 	     necessary to deal with it here. */
+ 	  *tp = build_empty_stmt ();
+ 	}
        /* Here we handle trees that are not completely rewritten.
  	 First we detect some inlining-induced bogosities for
  	 discarding.  */
!       else if (TREE_CODE (*tp) == MODIFY_EXPR
  	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
  	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
  	      (TREE_OPERAND (*tp, 0), fn)))
diff -cr gcc-orig/tree-nested.c gcc/tree-nested.c
*** gcc-orig/tree-nested.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree-nested.c	2010-08-22 20:09:26.114318287 -0400
***************
*** 327,332 ****
--- 327,333 ----
    if (!decl)
      {
        tree type;
+       enum tree_code code;
  
        type = get_frame_type (info->outer);
        type = build_pointer_type (type);
***************
*** 799,804 ****
--- 800,807 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
***************
*** 1621,1626 ****
--- 1624,1633 ----
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
*** 1691,1696 ****
--- 1698,1711 ----
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
      case MODIFY_EXPR:
      case WITH_SIZE_EXPR:
***************
*** 1826,1831 ****
--- 1841,1864 ----
        tree x = build3 (COMPONENT_REF, TREE_TYPE (root->chain_field),
  		       root->frame_decl, root->chain_field, NULL_TREE);
        x = build2 (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
+       if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
+       {
+         tree_stmt_iterator i =
+           tsi_start ( BIND_EXPR_BODY (DECL_SAVED_TREE (context)));
+         while (!tsi_end_p (i))
+           {
+             tree t = tsi_stmt (i);
+             if (TREE_CODE (t) == MODIFY_EXPR &&
+                 TREE_OPERAND (t, 0) == root->chain_decl)
+               {
+                 tsi_link_after(& i, x, TSI_SAME_STMT);
+                 x = NULL_TREE;
+                 break;
+               }
+           }
+         gcc_assert(x == NULL_TREE);
+       }
+       else
        append_to_statement_list (x, &stmt_list);
      }
  
diff -cr gcc-orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc-orig/tree-pretty-print.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree-pretty-print.c	2010-08-22 20:09:26.114318287 -0400
***************
*** 1155,1160 ****
--- 1155,1170 ----
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
*** gcc-orig/tree-sra.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree-sra.c	2010-08-22 20:09:26.114318287 -0400
***************
*** 248,253 ****
--- 248,255 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
***************
*** 257,262 ****
--- 259,269 ----
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
diff -cr gcc-orig/tree-vect-analyze.c gcc/tree-vect-analyze.c
*** gcc-orig/tree-vect-analyze.c	2007-09-01 11:28:30.000000000 -0400
--- gcc/tree-vect-analyze.c	2010-08-22 20:09:26.118318097 -0400
***************
*** 593,598 ****
--- 593,601 ----
           
    if (DDR_ARE_DEPENDENT (ddr) == chrec_known)
      return false;
+     
+   if ((DR_IS_READ (dra) && DR_IS_READ (drb)) || (dra == drb))
+     return false;
    
    if (DDR_ARE_DEPENDENT (ddr) == chrec_dont_know)
      {
***************
*** 603,608 ****
--- 606,617 ----
            print_generic_expr (vect_dump, DR_REF (dra), TDF_SLIM);
            fprintf (vect_dump, " and ");
            print_generic_expr (vect_dump, DR_REF (drb), TDF_SLIM);
+           fprintf (vect_dump, "\n");
+           dump_data_reference (vect_dump, dra);
+           fprintf (vect_dump, " ---- ");
+           dump_data_reference (vect_dump, drb);
+           fprintf (vect_dump, "\n");
+           fprintf (vect_dump, "Eq test %i\n", DR_REF (dra) == DR_REF (drb));
          }
        return true;
      }
