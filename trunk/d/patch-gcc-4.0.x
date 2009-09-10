diff -cr gcc-orig/cgraph.c gcc/cgraph.c
*** gcc-orig/cgraph.c	Tue Mar 29 15:40:43 2005
--- gcc/cgraph.c	Fri Nov  4 13:51:39 2005
***************
*** 170,175 ****
--- 170,176 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 186,196 ****
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
      {
!       node->origin = cgraph_node (DECL_CONTEXT (decl));
!       node->next_nested = node->origin->nested;
!       node->origin->nested = node;
      }
    return node;
  }
--- 187,201 ----
    node = cgraph_create_node ();
    node->decl = decl;
    *slot = node;
!   if (!DECL_NO_STATIC_CHAIN (decl))
      {
!       context = decl_function_context (decl);
!       if (context)
!         {
!           node->origin = cgraph_node (context);
!           node->next_nested = node->origin->nested;
!           node->origin->nested = node;
!         }
      }
    return node;
  }
diff -cr gcc-orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc-orig/config/i386/i386.c	Tue Sep  6 20:03:18 2005
--- gcc/config/i386/i386.c	Fri Nov  4 13:51:39 2005
***************
*** 4292,4297 ****
--- 4292,4302 ----
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
*** 15975,15981 ****
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 15975,15981 ----
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc-orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc-orig/config/rs6000/rs6000.c	Mon Sep 12 14:45:00 2005
--- gcc/config/rs6000/rs6000.c	Fri Nov  4 13:51:39 2005
***************
*** 15283,15289 ****
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
--- 15283,15290 ----
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C") ||
!  	  ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
Only in gcc: cp
Only in gcc: d
diff -cr gcc-orig/dwarf2.h gcc/dwarf2.h
*** gcc-orig/dwarf2.h	Wed Oct  6 20:27:15 2004
--- gcc/dwarf2.h	Fri Nov  4 13:51:39 2005
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
*** gcc-orig/dwarf2out.c	Thu Sep  1 14:04:38 2005
--- gcc/dwarf2out.c	Fri Nov  4 13:51:39 2005
***************
*** 5166,5172 ****
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus);
  }
  
  /* Return TRUE if the language is C++.  */
--- 5166,5173 ----
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus
! 	  || lang == DW_LANG_D);
  }
  
  /* Return TRUE if the language is C++.  */
***************
*** 11880,11885 ****
--- 11881,11888 ----
      language = DW_LANG_Pascal83;
    else if (strcmp (language_string, "GNU Java") == 0)
      language = DW_LANG_Java;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
diff -cr gcc-orig/expr.c gcc/expr.c
*** gcc-orig/expr.c	Sat Sep 10 01:03:28 2005
--- gcc/expr.c	Fri Nov  4 13:51:39 2005
***************
*** 8217,8222 ****
--- 8217,8227 ----
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
*** gcc-orig/function.c	Thu Mar 10 15:11:04 2005
--- gcc/function.c	Fri Nov  4 13:51:42 2005
***************
*** 3099,3105 ****
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
--- 3099,3106 ----
        FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			    data.passed_type, data.named_arg);
  
!       if (!cfun->naked)
! 	assign_parm_adjust_stack_rtl (&data);
  
        if (assign_parm_setup_block_p (&data))
  	assign_parm_setup_block (&all, parm, &data);
***************
*** 3114,3120 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
--- 3115,3122 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
***************
*** 3250,3255 ****
--- 3252,3260 ----
    struct assign_parm_data_all all;
    tree fnargs, parm, stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4203,4213 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4208,4222 ----
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
*** 5116,5121 ****
--- 5125,5133 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+       return;
+ 
  #ifdef HAVE_prologue
    if (HAVE_prologue)
      {
diff -cr gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	Sun Feb 20 11:09:16 2005
--- gcc/function.h	Fri Nov  4 13:51:42 2005
***************
*** 430,435 ****
--- 430,443 ----
  
    /* Nonzero if code to initialize arg_pointer_save_area has been emitted.  */
    unsigned int arg_pointer_save_area_init : 1;
+ 
+   /* Nonzero if static chain is initialized by something other than
+      static_chain_incoming_rtx. */
+   unsigned int custom_static_chain : 1;
+ 
+   /* Nonzero if no code should be generated for prologues, copying
+      parameters, etc. */
+   unsigned int naked : 1;
  };
  
  /* The function currently being compiled.  */
diff -cr gcc-orig/gcc.c gcc/gcc.c
*** gcc-orig/gcc.c	Sat Jan 21 13:38:48 2006
--- gcc/gcc.c	Thu Mar  1 10:34:32 2007
***************
*** 132,137 ****
--- 132,140 ----
  /* Flag set by cppspec.c to 1.  */
  int is_cpp_driver;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 470,475 ****
--- 473,479 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 871,876 ****
--- 875,882 ----
  #endif
  #endif
  
+ #define GCC_SPEC_FORMAT_4 1
+ 
  /* Record the mapping from file suffixes for compilation specs.  */
  
  struct compiler
***************
*** 3777,3782 ****
--- 3783,3791 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if ((save_temps_flag || report_times) && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4119,4124 ****
--- 4128,4145 ----
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
*** 5087,5092 ****
--- 5108,5124 ----
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
diff -cr gcc-orig/gcc.c gcc/gcc.h
*** gcc-orig/gcc.h Fri Jun 24 22:02:01 2005
--- gcc/gcc.h       Sun Mar  4 13:44:05 2007
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
  
diff -cr gcc-orig/gimplify.c gcc/gimplify.c
*** gcc-orig/gimplify.c	Fri Sep  2 15:34:38 2005
--- gcc/gimplify.c	Fri Nov  4 13:51:43 2005
***************
*** 3844,3849 ****
--- 3844,3855 ----
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
  
diff -cr gcc-orig/tree-dump.c gcc/tree-dump.c
*** gcc-orig/tree-dump.c	Tue Aug 23 07:39:43 2005
--- gcc/tree-dump.c	Fri Nov  4 13:51:43 2005
***************
*** 577,582 ****
--- 577,586 ----
        dump_child ("args", TREE_OPERAND (t, 1));
        break;
  
+     case STATIC_CHAIN_EXPR:
+       dump_child ("func", TREE_OPERAND (t, 0));
+       break;
+ 
      case CONSTRUCTOR:
        dump_child ("elts", CONSTRUCTOR_ELTS (t));
        break;
diff -cr gcc-orig/tree-gimple.c gcc/tree-gimple.c
*** gcc-orig/tree-gimple.c	Sat May 21 16:58:33 2005
--- gcc/tree-gimple.c	Fri Nov  4 13:51:43 2005
***************
*** 73,78 ****
--- 73,80 ----
      case COMPLEX_CST:
      case VECTOR_CST:
      case OBJ_TYPE_REF:
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
*** gcc-orig/tree-inline.c	Fri Jul  8 23:38:56 2005
--- gcc/tree-inline.c	Fri Nov  4 15:08:11 2005
***************
*** 546,552 ****
      {
        tree old_node = *tp;
  
!       if (TREE_CODE (*tp) == MODIFY_EXPR
  	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
  	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
  	      (TREE_OPERAND (*tp, 0), fn)))
--- 546,563 ----
      {
        tree old_node = *tp;
  
!       if (! id->cloning_p && ! id->saving_p &&
! 	  TREE_CODE (*tp) == MODIFY_EXPR &&
! 	  TREE_OPERAND (*tp, 0) ==
! 	  DECL_STRUCT_FUNCTION (fn)->static_chain_decl)
! 	{
! 	  /* Don't use special methods to initialize the static chain
! 	     if expanding inline.  If this code could somehow be
! 	     expanded in expand_start_function, it would not be
! 	     necessary to deal with it here. */
! 	  *tp = build_empty_stmt ();
! 	}
!       else if (TREE_CODE (*tp) == MODIFY_EXPR
  	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
  	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
  	      (TREE_OPERAND (*tp, 0), fn)))
diff -cr gcc-orig/tree-nested.c gcc/tree-nested.c
*** gcc-orig/tree-nested.c	Mon Oct  3 18:01:55 2005
--- gcc/tree-nested.c	Thu Jun  8 22:26:01 2006
***************
*** 298,303 ****
--- 298,304 ----
    if (!decl)
      {
        tree type;
+       enum tree_code code;
  
        type = get_frame_type (info->outer);
        type = build_pointer_type (type);
***************
*** 308,319 ****
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
--- 309,325 ----
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
*** 716,721 ****
--- 722,729 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl);
      }
    else
***************
*** 1194,1199 ****
--- 1202,1211 ----
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
*** 1258,1263 ****
--- 1270,1283 ----
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
*** 1352,1358 ****
        tree x = build (COMPONENT_REF, TREE_TYPE (root->chain_field),
  		      root->frame_decl, root->chain_field, NULL_TREE);
        x = build (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
!       append_to_statement_list (x, &stmt_list);
      }
  
    /* If trampolines were created, then we need to initialize them.  */
--- 1372,1399 ----
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
! 	    }
! 	  gcc_assert(x == NULL_TREE);
! 	}
!       else
! 	append_to_statement_list (x, &stmt_list);
      }
  
    /* If trampolines were created, then we need to initialize them.  */
diff -cr gcc-orig/tree-pretty-print.c gcc/tree-pretty-print.c
*** gcc-orig/tree-pretty-print.c	Thu Dec  9 10:54:36 2004
--- gcc/tree-pretty-print.c	Fri Nov  4 13:51:43 2005
***************
*** 953,958 ****
--- 953,968 ----
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
*** gcc-orig/tree-sra.c Fri Aug  5 16:39:04 2005
--- gcc/tree-sra.c      Thu Jul 26 18:45:59 2007
***************
*** 196,201 ****
--- 196,203 ----
      case RECORD_TYPE:
        {
  	bool saw_one_field = false;
+ 	tree last_offset = size_zero_node;
+ 	tree cmp;
  
  	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
  	  if (TREE_CODE (t) == FIELD_DECL)
***************
*** 205,210 ****
--- 207,217 ----
  		  && (tree_low_cst (DECL_SIZE (t), 1)
  		      != TYPE_PRECISION (TREE_TYPE (t))))
  		goto fail;
+ 	      /* Reject aliased fields created by GDC for anonymous unions. */
+ 	      cmp = fold_binary_to_constant (LE_EXPR, boolean_type_node,
+ 		DECL_FIELD_OFFSET (t), last_offset);
+ 	      if (cmp == NULL_TREE || TREE_CODE (cmp) != INTEGER_CST || TREE_INT_CST_LOW (cmp))
+ 		goto fail;
  
  	      saw_one_field = true;
  	    }
diff -cr gcc-orig/tree.def gcc/tree.def
*** gcc-orig/tree.def	Sun Jan 23 15:05:34 2005
--- gcc/tree.def	Fri Nov  4 13:51:43 2005
***************
*** 528,533 ****
--- 528,540 ----
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
diff -cr gcc-orig/real.c gcc/real.c
*** gcc-orig/real.c	Mon Sep 19 12:56:24 2005
--- gcc/real.c		Tue Apr 25 21:18:32 2006
***************
*** 2205,2210 ****
--- 2205,2212 ----
    np2 = SIGNIFICAND_BITS - fmt->p * fmt->log2_b;
    memset (r->sig, -1, SIGSZ * sizeof (unsigned long));
    clear_significand_below (r, np2);
+   if (REAL_MODE_FORMAT_COMPOSITE_P (mode))
+       clear_significand_bit (r, SIGNIFICAND_BITS - fmt->pnan - 1);
  }
  
  /* Fills R with 2**N.  */
