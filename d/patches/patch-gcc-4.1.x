diff -cr gcc-orig/cgraph.c gcc/cgraph.c
*** gcc-orig/cgraph.c	2007-01-05 14:44:10.000000000 -0500
--- gcc/cgraph.c	2010-08-22 20:03:59.362319560 -0400
***************
*** 182,187 ****
--- 182,188 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 203,214 ****
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
--- 204,219 ----
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
*** gcc-orig/config/arm/arm.c	2006-10-16 21:04:38.000000000 -0400
--- gcc/config/arm/arm.c	2010-08-22 20:03:59.378318102 -0400
***************
*** 15371,15376 ****
--- 15371,15385 ----
  	  /* Move from sp to reg.  */
  	  asm_fprintf (asm_out_file, "\t.movsp %r\n", REGNO (e0));
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
*** gcc-orig/config/darwin.h	2005-11-14 23:55:12.000000000 -0500
--- gcc/config/darwin.h	2010-08-22 20:03:59.378318102 -0400
***************
*** 926,933 ****
  
  #define MACHO_DYNAMIC_NO_PIC_P	(TARGET_DYNAMIC_NO_PIC)
  #define MACHOPIC_INDIRECT	(flag_pic || MACHO_DYNAMIC_NO_PIC_P)
! #define MACHOPIC_JUST_INDIRECT	(flag_pic == 1 || MACHO_DYNAMIC_NO_PIC_P)
! #define MACHOPIC_PURE		(flag_pic == 2 && ! MACHO_DYNAMIC_NO_PIC_P)
  
  #undef TARGET_ENCODE_SECTION_INFO
  #define TARGET_ENCODE_SECTION_INFO  darwin_encode_section_info
--- 926,933 ----
  
  #define MACHO_DYNAMIC_NO_PIC_P	(TARGET_DYNAMIC_NO_PIC)
  #define MACHOPIC_INDIRECT	(flag_pic || MACHO_DYNAMIC_NO_PIC_P)
! #define MACHOPIC_JUST_INDIRECT	(MACHO_DYNAMIC_NO_PIC_P)
! #define MACHOPIC_PURE		(flag_pic && ! MACHO_DYNAMIC_NO_PIC_P)
  
  #undef TARGET_ENCODE_SECTION_INFO
  #define TARGET_ENCODE_SECTION_INFO  darwin_encode_section_info
diff -cr gcc-orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc-orig/config/i386/i386.c	2006-11-17 02:01:22.000000000 -0500
--- gcc/config/i386/i386.c	2010-08-22 20:03:59.390319978 -0400
***************
*** 4754,4759 ****
--- 4754,4764 ----
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
*** 16979,16985 ****
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 16984,16990 ----
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc-orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc-orig/config/rs6000/rs6000.c	2006-12-16 14:24:56.000000000 -0500
--- gcc/config/rs6000/rs6000.c	2010-08-22 20:03:59.398320995 -0400
***************
*** 15305,15311 ****
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
--- 15305,15312 ----
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C") ||
!  	  ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
diff -cr gcc-orig/dwarf2.h gcc/dwarf2.h
*** gcc-orig/dwarf2.h	2005-06-24 22:02:01.000000000 -0400
--- gcc/dwarf2.h	2010-08-22 20:03:59.398320995 -0400
***************
*** 731,736 ****
--- 731,737 ----
      DW_LANG_C99 = 0x000c,
      DW_LANG_Ada95 = 0x000d,
      DW_LANG_Fortran95 = 0x000e,
+     DW_LANG_D = 0x0013,
      /* MIPS.  */
      DW_LANG_Mips_Assembler = 0x8001,
      /* UPC.  */
diff -cr gcc-orig/dwarf2out.c gcc/dwarf2out.c
*** gcc-orig/dwarf2out.c	2006-12-27 17:23:55.000000000 -0500
--- gcc/dwarf2out.c	2010-08-22 20:03:59.402321504 -0400
***************
*** 5322,5328 ****
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus);
  }
  
  /* Return TRUE if the language is C++.  */
--- 5322,5329 ----
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus
! 	  || lang == DW_LANG_D);
  }
  
  /* Return TRUE if the language is C++.  */
***************
*** 12210,12215 ****
--- 12211,12218 ----
      language = DW_LANG_Pascal83;
    else if (strcmp (language_string, "GNU Java") == 0)
      language = DW_LANG_Java;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
***************
*** 13346,13352 ****
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (DECL_CONTEXT (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
--- 13349,13355 ----
  
        /* For local statics lookup proper context die.  */
        if (TREE_STATIC (decl) && decl_function_context (decl))
! 	context_die = lookup_decl_die (decl_function_context (decl));
  
        /* If we are in terse mode, don't generate any DIEs to represent any
  	 variable declarations or definitions.  */
diff -cr gcc-orig/except.c gcc/except.c
*** gcc-orig/except.c	2010-09-01 17:53:52.026346348 +0100
--- gcc/except.c	2010-09-01 17:54:14.191346857 +0100
***************
*** 1725,1730 ****
--- 1725,1742 ----
  
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
*** gcc-orig/expr.c	2006-11-02 12:18:52.000000000 -0500
--- gcc/expr.c	2010-08-22 20:03:59.406319848 -0400
***************
*** 8477,8482 ****
--- 8477,8487 ----
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
*** gcc-orig/function.c	2006-11-28 07:01:45.000000000 -0500
--- gcc/function.c	2010-08-22 20:03:59.410318611 -0400
***************
*** 2997,3003 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 2997,3004 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
***************
*** 3012,3018 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
--- 3013,3020 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
***************
*** 3142,3147 ****
--- 3144,3152 ----
    struct assign_parm_data_all all;
    tree fnargs, parm, stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4176,4186 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4181,4195 ----
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
*** 5093,5098 ****
--- 5102,5110 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+       return;
+ 
  #ifdef HAVE_prologue
    if (HAVE_prologue)
      {
diff -cr gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	2005-08-19 17:16:20.000000000 -0400
--- gcc/function.h	2010-08-22 20:03:59.414319678 -0400
***************
*** 461,466 ****
--- 461,474 ----
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
*** gcc-orig/gcc.c	2006-11-07 09:26:21.000000000 -0500
--- gcc/gcc.c	2010-08-22 20:03:59.422317972 -0400
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
*** 458,463 ****
--- 461,467 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 876,881 ****
--- 880,887 ----
  #endif
  #endif
  
+ #define GCC_SPEC_FORMAT_4 1
+ 
  /* Record the mapping from file suffixes for compilation specs.  */
  
  struct compiler
***************
*** 3800,3805 ****
--- 3806,3814 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if (save_temps_flag && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4138,4143 ****
--- 4147,4164 ----
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
*** 5109,5114 ****
--- 5130,5146 ----
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
*** gcc-orig/gcc.h	2005-06-24 22:02:01.000000000 -0400
--- gcc/gcc.h	2010-08-22 20:03:59.422317972 -0400
***************
*** 38,44 ****
     || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
     || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
!    || (CHAR) == 'B' || (CHAR) == 'b')
  
  /* This defines which multi-letter switches take arguments.  */
  
--- 38,44 ----
     || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
     || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
!    || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
  
  /* This defines which multi-letter switches take arguments.  */
  
diff -cr gcc-orig/predict.c gcc/predict.c
*** gcc-orig/predict.c	2005-11-04 19:55:23.000000000 -0500
--- gcc/predict.c	2010-08-22 20:03:59.426318760 -0400
***************
*** 1339,1344 ****
--- 1339,1345 ----
  	     care for error returns and other cases are often used for
  	     fast paths trought function.  */
  	  if (e->dest == EXIT_BLOCK_PTR
+ 	      && last_stmt (bb) == NULL_TREE
  	      && TREE_CODE (last_stmt (bb)) == RETURN_EXPR
  	      && !single_pred_p (bb))
  	    {
diff -cr gcc-orig/real.c gcc/real.c
*** gcc-orig/real.c	2005-09-19 13:01:40.000000000 -0400
--- gcc/real.c	2010-08-22 20:03:59.430318780 -0400
***************
*** 2212,2217 ****
--- 2212,2219 ----
    np2 = SIGNIFICAND_BITS - fmt->p * fmt->log2_b;
    memset (r->sig, -1, SIGSZ * sizeof (unsigned long));
    clear_significand_below (r, np2);
+   if (REAL_MODE_FORMAT_COMPOSITE_P (mode))
+       clear_significand_bit (r, SIGNIFICAND_BITS - fmt->pnan - 1);
  }
  
  /* Fills R with 2**N.  */
diff -cr gcc-orig/tree.def gcc/tree.def
*** gcc-orig/tree.def	2006-02-10 12:32:10.000000000 -0500
--- gcc/tree.def	2010-08-22 20:03:59.434318171 -0400
***************
*** 526,531 ****
--- 526,538 ----
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
diff -cr gcc-orig/tree-dump.c gcc/tree-dump.c
*** gcc-orig/tree-dump.c	2005-08-20 12:03:58.000000000 -0400
--- gcc/tree-dump.c	2010-08-22 20:03:59.430318780 -0400
***************
*** 582,587 ****
--- 582,591 ----
        dump_child ("args", TREE_OPERAND (t, 1));
        break;
  
+     case STATIC_CHAIN_EXPR:
+       dump_child ("func", TREE_OPERAND (t, 0));
+       break;
+ 
      case CONSTRUCTOR:
        {
  	unsigned HOST_WIDE_INT cnt;
diff -cr gcc-orig/tree-gimple.c gcc/tree-gimple.c
*** gcc-orig/tree-gimple.c	2005-11-20 14:05:43.000000000 -0500
--- gcc/tree-gimple.c	2010-08-22 20:03:59.430318780 -0400
***************
*** 72,77 ****
--- 72,79 ----
      case VECTOR_CST:
      case OBJ_TYPE_REF:
      case ASSERT_EXPR:
+     case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+     case STATIC_CHAIN_DECL:
        return true;
  
      default:
***************
*** 143,149 ****
  	  || TREE_CODE (t) == WITH_SIZE_EXPR
  	  /* These are complex lvalues, but don't have addresses, so they
  	     go here.  */
! 	  || TREE_CODE (t) == BIT_FIELD_REF);
  }
  
  /*  Return true if T is a GIMPLE condition.  */
--- 145,154 ----
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
*** gcc-orig/tree-inline.c	2007-01-05 08:53:45.000000000 -0500
--- gcc/tree-inline.c	2010-08-22 20:03:59.430318780 -0400
***************
*** 602,611 ****
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
--- 602,622 ----
       knows not to copy VAR_DECLs, etc., so this is safe.  */
    else
      {
+       if (! id->cloning_p && ! id->saving_p &&
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
*** gcc-orig/tree-nested.c	2006-07-20 11:43:44.000000000 -0400
--- gcc/tree-nested.c	2010-08-22 20:03:59.434318171 -0400
***************
*** 323,328 ****
--- 323,329 ----
    if (!decl)
      {
        tree type;
+       enum tree_code code;
  
        type = get_frame_type (info->outer);
        type = build_pointer_type (type);
***************
*** 333,344 ****
  	 Note also that it's represented as a parameter.  This is more
  	 close to the truth, since the initial value does come from 
  	 the caller.  */
!       decl = build_decl (PARM_DECL, create_tmp_var_name ("CHAIN"), type);
        DECL_ARTIFICIAL (decl) = 1;
        DECL_IGNORED_P (decl) = 1;
        TREE_USED (decl) = 1;
        DECL_CONTEXT (decl) = info->context;
!       DECL_ARG_TYPE (decl) = type;
  
        /* Tell tree-inline.c that we never write to this variable, so
  	 it can copy-prop the replacement value immediately.  */
--- 334,350 ----
  	 Note also that it's represented as a parameter.  This is more
  	 close to the truth, since the initial value does come from 
  	 the caller.  */
!       /* If the function has a custom static chain, a VAR_DECL is more
! 	 appropriate. */
!       code = DECL_STRUCT_FUNCTION (info->context)->custom_static_chain ?
! 	  VAR_DECL : PARM_DECL;
!       decl = build_decl (code, create_tmp_var_name ("CHAIN"), type);
        DECL_ARTIFICIAL (decl) = 1;
        DECL_IGNORED_P (decl) = 1;
        TREE_USED (decl) = 1;
        DECL_CONTEXT (decl) = info->context;
!       if (TREE_CODE (decl) == PARM_DECL)
! 	  DECL_ARG_TYPE (decl) = type;
  
        /* Tell tree-inline.c that we never write to this variable, so
  	 it can copy-prop the replacement value immediately.  */
***************
*** 741,746 ****
--- 747,754 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl, target_context);
      }
    else
***************
*** 1224,1229 ****
--- 1232,1241 ----
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
*** 1288,1293 ****
--- 1300,1313 ----
  	  = get_static_chain (info, target_context, &wi->tsi);
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
*** 1387,1393 ****
        tree x = build (COMPONENT_REF, TREE_TYPE (root->chain_field),
  		      root->frame_decl, root->chain_field, NULL_TREE);
        x = build (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
!       append_to_statement_list (x, &stmt_list);
      }
  
    /* If trampolines were created, then we need to initialize them.  */
--- 1407,1435 ----
        tree x = build (COMPONENT_REF, TREE_TYPE (root->chain_field),
  		      root->frame_decl, root->chain_field, NULL_TREE);
        x = build (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
!       /* If the function has a custom static chain, chain_field must
! 	 be set after the static chain. */
!       if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
! 	{
! 	  /* Should use walk_function instead. */
! 	  tree_stmt_iterator i =
! 	      tsi_start ( BIND_EXPR_BODY (DECL_SAVED_TREE (context)));
! 	  while (!tsi_end_p (i))
! 	    {
! 	      tree t = tsi_stmt (i);
! 	      if (TREE_CODE (t) == MODIFY_EXPR &&
! 		  TREE_OPERAND (t, 0) == root->chain_decl)
! 		{
! 		  tsi_link_after(& i, x, TSI_SAME_STMT);
! 		  x = NULL_TREE;
! 		  break;
! 		}
! 	      tsi_next (& i);
! 	    }
! 	  gcc_assert(x == NULL_TREE);
! 	}
!       else
! 	append_to_statement_list (x, &stmt_list);
      }
  
    /* If trampolines were created, then we need to initialize them.  */
diff -cr gcc-orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc-orig/tree-pretty-print.c	2005-07-31 16:55:41.000000000 -0400
--- gcc/tree-pretty-print.c	2010-08-22 20:03:59.434318171 -0400
***************
*** 1004,1009 ****
--- 1004,1019 ----
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
*** gcc-orig/tree-sra.c	2005-11-20 19:55:57.000000000 -0500
--- gcc/tree-sra.c	2010-08-22 20:03:59.434318171 -0400
***************
*** 198,203 ****
--- 198,205 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
***************
*** 207,212 ****
--- 209,219 ----
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
diff -cr gcc-orig/varray.h gcc/varray.h
*** gcc-orig/varray.h	2005-06-24 22:02:01.000000000 -0400
--- gcc/varray.h	2010-08-22 20:03:59.434318171 -0400
***************
*** 62,67 ****
--- 62,78 ----
    NUM_VARRAY_DATA
  };
  
+ #ifndef __cplusplus 
+ # define VARRAY_STRANGE_1 rtx
+ # define VARRAY_STRANGE_2 rtvec
+ # define VARRAY_STRANGE_3 tree
+ #else
+ # define VARRAY_STRANGE_1 rtx_
+ # define VARRAY_STRANGE_2 rtvec_
+ # define VARRAY_STRANGE_3 tree_
+ #endif
+ 
+ 
  /* Union of various array types that are used.  */
  typedef union varray_data_tag GTY (()) {
    char			  GTY ((length ("%0.num_elements"),
***************
*** 91,101 ****
    char			 *GTY ((length ("%0.num_elements"),
  				tag ("VARRAY_DATA_CPTR")))	cptr[1];
    rtx			  GTY ((length ("%0.num_elements"),
! 				tag ("VARRAY_DATA_RTX")))	rtx[1];
    rtvec			  GTY ((length ("%0.num_elements"),
! 				tag ("VARRAY_DATA_RTVEC")))	rtvec[1];
    tree			  GTY ((length ("%0.num_elements"),
! 				tag ("VARRAY_DATA_TREE")))	tree[1];
    struct bitmap_head_def *GTY ((length ("%0.num_elements"),
  				tag ("VARRAY_DATA_BITMAP")))	bitmap[1];
    struct reg_info_def	 *GTY ((length ("%0.num_elements"), skip,
--- 102,112 ----
    char			 *GTY ((length ("%0.num_elements"),
  				tag ("VARRAY_DATA_CPTR")))	cptr[1];
    rtx			  GTY ((length ("%0.num_elements"),
! 				tag ("VARRAY_DATA_RTX")))	VARRAY_STRANGE_1[1];
    rtvec			  GTY ((length ("%0.num_elements"),
! 				tag ("VARRAY_DATA_RTVEC")))	VARRAY_STRANGE_2[1];
    tree			  GTY ((length ("%0.num_elements"),
! 				tag ("VARRAY_DATA_TREE")))	VARRAY_STRANGE_3[1];
    struct bitmap_head_def *GTY ((length ("%0.num_elements"),
  				tag ("VARRAY_DATA_BITMAP")))	bitmap[1];
    struct reg_info_def	 *GTY ((length ("%0.num_elements"), skip,
