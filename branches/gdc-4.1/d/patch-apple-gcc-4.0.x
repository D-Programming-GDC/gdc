diff -cr gcc-orig/cgraph.c gcc/cgraph.c
*** gcc-orig/cgraph.c	Sat Apr  9 02:13:35 2005
--- gcc/cgraph.c	Thu Jun 22 19:49:34 2006
***************
*** 172,177 ****
--- 172,178 ----
  cgraph_node (tree decl)
  {
    struct cgraph_node key, *node, **slot;
+   tree context;
  
    gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
  
***************
*** 188,198 ****
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
--- 189,203 ----
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
*** gcc-orig/config/i386/i386.c	Fri Apr 28 16:42:39 2006
--- gcc/config/i386/i386.c	Thu Jun 22 19:49:34 2006
***************
*** 5049,5054 ****
--- 5049,5059 ----
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
*** 17396,17402 ****
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 17396,17402 ----
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc-orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc-orig/config/rs6000/rs6000.c	Thu Apr 27 21:06:13 2006
--- gcc/config/rs6000/rs6000.c	Thu Jun 22 19:49:35 2006
***************
*** 17445,17451 ****
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
--- 17445,17452 ----
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C") ||
!  	  ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77")
  	       || ! strcmp (language_string, "GNU F95"))
diff -cr gcc-orig/convert.c gcc/convert.c
*** gcc-orig/convert.c	Wed Sep 21 19:20:36 2005
--- gcc/convert.c	Thu Jun 22 20:08:13 2006
***************
*** 30,39 ****
--- 30,41 ----
  #include "tree.h"
  #include "flags.h"
  #include "convert.h"
+ #if 0
  /* APPLE LOCAL begin AltiVec */
  #include "c-tree.h"
  #include "c-common.h"
  /* APPLE LOCAL end AltiVec */
+ #endif
  #include "toplev.h"
  #include "langhooks.h"
  #include "real.h"
***************
*** 703,708 ****
--- 705,711 ----
      }
  }
  
+ #if 0
  /* APPLE LOCAL begin AltiVec */
  /* Build a COMPOUND_LITERAL_EXPR.  TYPE is the type given in the compound
     literal.  INIT is a CONSTRUCTOR that initializes the compound literal.  */
***************
*** 727,732 ****
--- 730,736 ----
    return complit;
  }
  /* APPLE LOCAL end AltiVec */
+ #endif
  
  /* Convert EXPR to the vector type TYPE in the usual ways.  */
  
***************
*** 742,747 ****
--- 746,752 ----
  	  error ("can't convert between vector values of different size");
  	  return error_mark_node;
  	}
+ #if 0
        /* APPLE LOCAL begin AltiVec */
        if (TREE_CODE (type) == VECTOR_TYPE  
  	  && TREE_CODE (TREE_TYPE (expr)) == VECTOR_TYPE
***************
*** 749,754 ****
--- 754,760 ----
  	  /* converting a constant vector to new vector type with Motorola Syntax. */
  	  return convert (type, build_compound_literal_vector (TREE_TYPE (expr), expr));
        /* APPLE LOCAL end AltiVec */
+ #endif
  
        return build1 (NOP_EXPR, type, expr);
  
diff -cr gcc-orig/dwarf2out.c gcc/dwarf2out.c
*** gcc-orig/dwarf2out.c	Wed Apr 26 15:06:48 2006
--- gcc/dwarf2out.c	Thu Jun 22 20:01:30 2006
***************
*** 5350,5356 ****
  /* APPLE LOCAL begin mainline 2006-03-24 4485597 */
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
  	  || lang == DW_LANG_C99
! 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
  /* APPLE LOCAL end mainline 2006-03-24 4485597 */
  }
  
--- 5350,5357 ----
  /* APPLE LOCAL begin mainline 2006-03-24 4485597 */
    return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
  	  || lang == DW_LANG_C99
! 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus
! 	  || lang == DW_LANG_D);
  /* APPLE LOCAL end mainline 2006-03-24 4485597 */
  }
  
***************
*** 12020,12025 ****
--- 12021,12028 ----
    else if (strcmp (language_string, "GNU Objective-C++") == 0)
      language = DW_LANG_ObjC_plus_plus;
  /* APPLE LOCAL end mainline 2006-03-24 4485597 */
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
diff -cr gcc-orig/expr.c gcc/expr.c
*** gcc-orig/expr.c	Fri Oct 28 14:47:00 2005
--- gcc/expr.c	Thu Jun 22 19:49:35 2006
***************
*** 8264,8269 ****
--- 8264,8274 ----
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
*** gcc-orig/function.c	Wed Apr 26 14:07:46 2006
--- gcc/function.c	Thu Jun 22 19:49:35 2006
***************
*** 3130,3136 ****
            FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			        data.passed_type, data.named_arg);
  
!           assign_parm_adjust_stack_rtl (&data);
  
            if (assign_parm_setup_block_p (&data))
  	    assign_parm_setup_block (&all, parm, &data);
--- 3130,3137 ----
            FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
  			        data.passed_type, data.named_arg);
  
! 	  if (!cfun->naked)
! 	    assign_parm_adjust_stack_rtl (&data);
  
            if (assign_parm_setup_block_p (&data))
  	    assign_parm_setup_block (&all, parm, &data);
***************
*** 3147,3153 ****
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
--- 3148,3155 ----
  
    /* Output all parameter conversion instructions (possibly including calls)
       now that all parameters have been copied out of hard registers.  */
!   if (!cfun->naked)
!     emit_insn (all.conversion_insns);
  
    /* If we are receiving a struct value address as the first argument, set up
       the RTL for the function result. As this might require code to convert
***************
*** 3287,3292 ****
--- 3289,3297 ----
    struct assign_parm_data_all all;
    tree fnargs, parm, stmts = NULL;
  
+   if (cfun->naked)
+     return NULL;
+ 
    assign_parms_initialize_all (&all);
    fnargs = assign_parms_augmented_arg_list (&all);
  
***************
*** 4219,4229 ****
        tree parm = cfun->static_chain_decl;
        rtx local = gen_reg_rtx (Pmode);
  
-       set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
        SET_DECL_RTL (parm, local);
        mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
  
!       emit_move_insn (local, static_chain_incoming_rtx);
      }
  
    /* If the function receives a non-local goto, then store the
--- 4224,4238 ----
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
*** 5132,5137 ****
--- 5141,5149 ----
  #endif
    edge_iterator ei;
  
+   if (cfun->naked)
+       return;
+ 
  #ifdef HAVE_prologue
    if (HAVE_prologue)
      {
diff -cr gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	Wed Apr 26 14:07:46 2006
--- gcc/function.h	Thu Jun 22 19:51:26 2006
***************
*** 452,457 ****
--- 452,465 ----
    /* APPLE LOCAL begin 3837835  */
    unsigned int uses_vector : 1;
    /* APPLE LOCAL end 3837835  */
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
--- gcc/gcc.c	Thu Mar  1 10:27:25 2007
***************
*** 134,139 ****
--- 134,142 ----
  /* Flag set by cppspec.c to 1.  */
  int is_cpp_driver;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 483,488 ****
--- 486,492 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 900,905 ****
--- 904,911 ----
  #endif
  #endif
  
+ #define GCC_SPEC_FORMAT_4 1
+ 
  /* Record the mapping from file suffixes for compilation specs.  */
  
  struct compiler
***************
*** 3965,3970 ****
--- 3971,3979 ----
  	}
      }
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if ((save_temps_flag || report_times) && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4349,4354 ****
--- 4358,4375 ----
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
*** 5438,5443 ****
--- 5459,5475 ----
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
*** gcc-orig/gcc.h  Fri Mar  4 15:17:11 2005
--- gcc/gcc.h  Sun Mar  4 13:36:10 2007
***************
*** 40,46 ****
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
     /* APPLE LOCAL frameworks */ \
     || (CHAR) == 'F' \
!    || (CHAR) == 'B' || (CHAR) == 'b')
  
  /* This defines which multi-letter switches take arguments.  */
  
--- 40,46 ----
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
     /* APPLE LOCAL frameworks */ \
     || (CHAR) == 'F' \
!    || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
  
  /* This defines which multi-letter switches take arguments.  */
  
diff -cr gcc-orig/gimplify.c gcc/gimplify.c
*** gcc-orig/gimplify.c	Thu Aug 11 19:55:22 2005
--- gcc/gimplify.c	Thu Jun 22 19:49:35 2006
***************
*** 3876,3881 ****
--- 3876,3887 ----
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
  
diff -cr gcc-orig/real.c gcc/real.c
*** gcc-orig/real.c	Tue Jan 25 15:21:59 2005
--- gcc/real.c	Thu Jun 22 19:49:35 2006
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
diff -cr gcc-orig/tree-dump.c gcc/tree-dump.c
*** gcc-orig/tree-dump.c	Tue Feb 15 10:53:52 2005
--- gcc/tree-dump.c	Thu Jun 22 19:49:35 2006
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
*** gcc-orig/tree-gimple.c	Sat Jul  9 18:14:08 2005
--- gcc/tree-gimple.c	Thu Jun 22 19:49:35 2006
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
*** gcc-orig/tree-inline.c	Wed Apr 26 14:07:46 2006
--- gcc/tree-inline.c	Thu Jun 22 19:49:35 2006
***************
*** 552,558 ****
      {
        tree old_node = *tp;
  
!       if (TREE_CODE (*tp) == MODIFY_EXPR
  	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
  	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
  	      (TREE_OPERAND (*tp, 0), fn)))
--- 552,569 ----
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
*** gcc-orig/tree-nested.c	Mon Aug  8 15:02:59 2005
--- gcc/tree-nested.c	Thu Jun 22 19:49:35 2006
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
*** 717,722 ****
--- 723,730 ----
  
    if (info->context == target_context)
      {
+       /* might be doing something wrong to need the following line.. */
+       get_frame_type (info);
        x = build_addr (info->frame_decl);
      }
    else
***************
*** 1195,1200 ****
--- 1203,1212 ----
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
*** 1259,1264 ****
--- 1271,1284 ----
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
*** 1360,1366 ****
        tree x = build (COMPONENT_REF, TREE_TYPE (root->chain_field),
  		      root->frame_decl, root->chain_field, NULL_TREE);
        x = build (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
!       append_to_statement_list (x, &stmt_list);
      }
  
    /* If trampolines were created, then we need to initialize them.  */
--- 1380,1407 ----
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
*** gcc-orig/tree-pretty-print.c	Thu Dec  9 05:54:50 2004
--- gcc/tree-pretty-print.c	Thu Jun 22 19:49:35 2006
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
diff -cr gcc-orig/tree.def gcc/tree.def
*** gcc-orig/tree.def	Mon Oct  3 16:31:24 2005
--- gcc/tree.def	Thu Jun 22 21:09:39 2006
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
***************
*** 901,907 ****
  
  /* APPLE LOCAL begin AV vmul_uch --haifa  */
  /* Used during vectorization to represent computation idioms.  */
! DEFTREECODE (MULT_UCH_EXPR, "mult_uch", '2', 2)
  /* APPLE LOCAL end AV vmul_uch --haifa  */
  
  /* Value handles.  Artificial nodes to represent expressions in
--- 908,914 ----
  
  /* APPLE LOCAL begin AV vmul_uch --haifa  */
  /* Used during vectorization to represent computation idioms.  */
! DEFTREECODE (MULT_UCH_EXPR, "mult_uch", tcc_binary, 2)
  /* APPLE LOCAL end AV vmul_uch --haifa  */
  
  /* Value handles.  Artificial nodes to represent expressions in
diff -cr gcc-orig/config/darwin.h gcc/config/darwin.h
*** gcc-orig/config/darwin.h    Thu Apr 27 21:06:13 2006
--- gcc/config/darwin.h Thu Dec  7 23:53:11 2006
***************
*** 376,381 ****
--- 376,388 ----
      %{!nostdlib:%{!nodefaultlibs:%G %L}} \
      %{!A:%{!nostdlib:%{!nostartfiles:%E}}} %{T*} %{F*} }}}}}}}}"
  
+ #ifdef TARGET_SYSTEM_ROOT
+ #define LINK_SYSROOT_SPEC \
+   "%{isysroot*:-syslibroot %*;:-syslibroot " TARGET_SYSTEM_ROOT "}"
+ #else
+ #define LINK_SYSROOT_SPEC "%{isysroot*:-syslibroot %*}"
+ #endif
+ 
  /* Please keep the random linker options in alphabetical order (modulo
     'Z' and 'no' prefixes).  Options that can only go to one of libtool
     or ld must be listed twice, under both !Zdynamiclib and
***************
*** 450,456 ****
     %{Zfn_seg_addr_table_filename*:-seg_addr_table_filename %*} \
     %{sub_library*} %{sub_umbrella*} \
     "/* APPLE LOCAL mainline 4.1 2005-06-03 */" \
!    %{isysroot*:-syslibroot %*} \
     %{twolevel_namespace} %{twolevel_namespace_hints} \
     %{umbrella*} \
     %{undefined*} \
--- 457,463 ----
     %{Zfn_seg_addr_table_filename*:-seg_addr_table_filename %*} \
     %{sub_library*} %{sub_umbrella*} \
     "/* APPLE LOCAL mainline 4.1 2005-06-03 */" \
!    " LINK_SYSROOT_SPEC " \
     %{twolevel_namespace} %{twolevel_namespace_hints} \
     %{umbrella*} \
     %{undefined*} \
