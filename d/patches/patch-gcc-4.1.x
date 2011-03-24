--- gcc.orig/cgraph.c	2007-01-05 19:44:10.000000000 +0000
+++ gcc/cgraph.c	2011-03-21 18:36:45.631814511 +0000
@@ -182,6 +182,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -203,12 +204,16 @@ cgraph_node (tree decl)
   node = cgraph_create_node ();
   node->decl = decl;
   *slot = node;
-  if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
+  if (!DECL_NO_STATIC_CHAIN (decl))
     {
-      node->origin = cgraph_node (DECL_CONTEXT (decl));
-      node->next_nested = node->origin->nested;
-      node->origin->nested = node;
-      node->master_clone = node;
+      context = decl_function_context (decl);
+      if (context)
+        {
+	  node->origin = cgraph_node (context);
+	  node->next_nested = node->origin->nested;
+	  node->origin->nested = node;
+	  node->master_clone = node;
+        }
     }
   return node;
 }
--- gcc.orig/config/arm/arm.c	2006-10-17 02:04:38.000000000 +0100
+++ gcc/config/arm/arm.c	2011-03-21 18:36:45.675814743 +0000
@@ -15371,6 +15371,15 @@ arm_unwind_emit_set (FILE * asm_out_file
 	  /* Move from sp to reg.  */
 	  asm_fprintf (asm_out_file, "\t.movsp %r\n", REGNO (e0));
 	}
+      else if (GET_CODE (e1) == PLUS
+	      && GET_CODE (XEXP (e1, 0)) == REG
+	      && REGNO (XEXP (e1, 0)) == SP_REGNUM
+	      && GET_CODE (XEXP (e1, 1)) == CONST_INT)
+	{
+	  /* Set reg to offset from sp.  */
+	  asm_fprintf (asm_out_file, "\t.movsp %r, #%d\n",
+		       REGNO (e0), (int)INTVAL(XEXP (e1, 1)));
+	}
       else
 	abort ();
       break;
--- gcc.orig/config/darwin.h	2005-11-15 04:55:12.000000000 +0000
+++ gcc/config/darwin.h	2011-03-21 18:36:46.047816566 +0000
@@ -926,8 +926,8 @@ enum machopic_addr_class {
 
 #define MACHO_DYNAMIC_NO_PIC_P	(TARGET_DYNAMIC_NO_PIC)
 #define MACHOPIC_INDIRECT	(flag_pic || MACHO_DYNAMIC_NO_PIC_P)
-#define MACHOPIC_JUST_INDIRECT	(flag_pic == 1 || MACHO_DYNAMIC_NO_PIC_P)
-#define MACHOPIC_PURE		(flag_pic == 2 && ! MACHO_DYNAMIC_NO_PIC_P)
+#define MACHOPIC_JUST_INDIRECT	(MACHO_DYNAMIC_NO_PIC_P)
+#define MACHOPIC_PURE		(flag_pic && ! MACHO_DYNAMIC_NO_PIC_P)
 
 #undef TARGET_ENCODE_SECTION_INFO
 #define TARGET_ENCODE_SECTION_INFO  darwin_encode_section_info
--- gcc.orig/config/i386/i386.c	2006-11-17 07:01:22.000000000 +0000
+++ gcc/config/i386/i386.c	2011-03-24 09:17:12.680111593 +0000
@@ -1914,6 +1914,8 @@ const struct attribute_spec ix86_attribu
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
 #if TARGET_DLLIMPORT_DECL_ATTRIBUTES
   { "dllimport", 0, 0, false, false, false, handle_dll_attribute },
   { "dllexport", 0, 0, false, false, false, handle_dll_attribute },
@@ -2078,6 +2080,10 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -2096,6 +2102,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -2109,6 +2119,27 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -2169,7 +2200,7 @@ ix86_function_regparm (tree type, tree d
 	  user_convention = true;
 	}
 
-      if (lookup_attribute ("fastcall", TYPE_ATTRIBUTES (type)))
+      if (lookup_attribute ("fastcall", type_attributes (type)))
 	{
 	  regparm = 2;
 	  user_convention = true;
@@ -2301,6 +2332,12 @@ ix86_return_pops_args (tree fundecl, tre
         || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
       rtd = 1;
 
+    /* Optlink functions will pop the stack if returning float and
+       if not variable args.  */
+    if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+        && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+      rtd = 1;
+
     if (rtd
         && (TYPE_ARG_TYPES (funtype) == NULL_TREE
 	    || (TREE_VALUE (tree_last (TYPE_ARG_TYPES (funtype)))
@@ -2413,6 +2450,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
 	}
       else
 	cum->nregs = ix86_function_regparm (fntype, fndecl);
+
+      /* For optlink, last parameter is passed in eax rather than
+         being pushed on the stack.  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (fntype)))
+	cum->optlink = 1;
     }
 
   /* Set up the number of SSE registers used for passing SFmode
@@ -4754,6 +4796,11 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+      /* As above, skip return address */
+      frame->stack_pointer_offset = UNITS_PER_WORD;
+
 #if 0
   fprintf (stderr, "nregs: %i\n", frame->nregs);
   fprintf (stderr, "size: %i\n", size);
@@ -16979,7 +17026,7 @@ x86_output_mi_thunk (FILE *file ATTRIBUT
 	  output_set_got (tmp);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
--- gcc.orig/config/i386/i386.h	2006-12-16 19:24:56.000000000 +0000
+++ gcc/config/i386/i386.h	2011-03-24 09:12:00.922565654 +0000
@@ -1476,6 +1476,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_sse;			/* True when we want to warn about SSE ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2006-12-16 19:24:56.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2011-03-21 18:36:47.467823619 +0000
@@ -15305,7 +15305,8 @@ rs6000_output_function_epilogue (FILE *f
 	 use language_string.
 	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
 	 Java is 13.  Objective-C is 14.  */
-      if (! strcmp (language_string, "GNU C"))
+      if (! strcmp (language_string, "GNU C") ||
+ 	  ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU F95"))
--- gcc.orig/dwarf2.h	2005-06-25 03:02:01.000000000 +0100
+++ gcc/dwarf2.h	2011-03-21 18:36:47.907825798 +0000
@@ -731,6 +731,7 @@ enum dwarf_source_language
     DW_LANG_C99 = 0x000c,
     DW_LANG_Ada95 = 0x000d,
     DW_LANG_Fortran95 = 0x000e,
+    DW_LANG_D = 0x0013,
     /* MIPS.  */
     DW_LANG_Mips_Assembler = 0x8001,
     /* UPC.  */
--- gcc.orig/dwarf2out.c	2006-12-27 22:23:55.000000000 +0000
+++ gcc/dwarf2out.c	2011-03-21 18:36:48.383828154 +0000
@@ -5322,7 +5322,8 @@ is_c_family (void)
   unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
 
   return (lang == DW_LANG_C || lang == DW_LANG_C89
-	  || lang == DW_LANG_C_plus_plus);
+	  || lang == DW_LANG_C_plus_plus
+	  || lang == DW_LANG_D);
 }
 
 /* Return TRUE if the language is C++.  */
@@ -12210,6 +12211,8 @@ gen_compile_unit_die (const char *filena
     language = DW_LANG_Pascal83;
   else if (strcmp (language_string, "GNU Java") == 0)
     language = DW_LANG_Java;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else
     language = DW_LANG_C89;
 
@@ -13346,7 +13349,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/except.c	2007-02-09 02:52:53.000000000 +0000
+++ gcc/except.c	2011-03-21 18:36:49.635834363 +0000
@@ -1725,6 +1725,18 @@ sjlj_mark_call_sites (struct sjlj_lp_inf
 
 	  region = VEC_index (eh_region, cfun->eh->region_array, INTVAL (XEXP (note, 0)));
 	  this_call_site = lp_info[region->region_number].call_site_index;
+	  if (region->type == ERT_CATCH)
+	  {
+	    /* Use previous region information */
+	    region = region->outer;
+	    if (!region)
+	    {
+	      /* No previous region, must change function contexts. */
+	      this_call_site = -1;
+	    }
+	    else
+	    this_call_site = lp_info[region->region_number].call_site_index;        
+	  }
 	}
 
       if (this_call_site == last_call_site)
--- gcc.orig/expr.c	2006-11-02 17:18:52.000000000 +0000
+++ gcc/expr.c	2011-03-21 18:36:51.095841612 +0000
@@ -8477,6 +8477,11 @@ expand_expr_real_1 (tree exp, rtx target
       /* Lowered by gimplify.c.  */
       gcc_unreachable ();
 
+    case STATIC_CHAIN_EXPR:
+    case STATIC_CHAIN_DECL:
+      /* Lowered by tree-nested.c */
+      gcc_unreachable ();
+
     case EXC_PTR_EXPR:
       return get_exception_pointer (cfun);
 
--- gcc.orig/function.c	2006-11-28 12:01:45.000000000 +0000
+++ gcc/function.c	2011-03-21 18:36:52.067846426 +0000
@@ -2997,7 +2997,8 @@ assign_parms (tree fndecl)
       FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
 			    data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3012,7 +3013,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.conversion_insns);
+  if (!cfun->naked)
+    emit_insn (all.conversion_insns);
 
   /* If we are receiving a struct value address as the first argument, set up
      the RTL for the function result. As this might require code to convert
@@ -3142,6 +3144,9 @@ gimplify_parameters (void)
   struct assign_parm_data_all all;
   tree fnargs, parm, stmts = NULL;
 
+  if (cfun->naked)
+    return NULL;
+
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -4176,11 +4181,15 @@ expand_function_start (tree subr)
       tree parm = cfun->static_chain_decl;
       rtx local = gen_reg_rtx (Pmode);
 
-      set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
       SET_DECL_RTL (parm, local);
       mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
 
-      emit_move_insn (local, static_chain_incoming_rtx);
+      if (! cfun->custom_static_chain)
+        {
+	    set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
+	    emit_move_insn (local, static_chain_incoming_rtx);
+	}
+      /* else, the static chain will be set in the main body */
     }
 
   /* If the function receives a non-local goto, then store the
@@ -5093,6 +5102,9 @@ thread_prologue_and_epilogue_insns (rtx
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+      return;
+
 #ifdef HAVE_prologue
   if (HAVE_prologue)
     {
--- gcc.orig/function.h	2005-08-19 22:16:20.000000000 +0100
+++ gcc/function.h	2011-03-21 18:36:52.187847028 +0000
@@ -461,6 +461,14 @@ struct function GTY(())
   /* Number of units of floating point registers that need saving in stdarg
      function.  */
   unsigned int va_list_fpr_size : 8;
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
--- gcc.orig/gcc.c	2006-11-07 14:26:21.000000000 +0000
+++ gcc/gcc.c	2011-03-21 18:36:52.459848384 +0000
@@ -126,6 +126,9 @@ static const char dir_separator_str[] =
 /* Flag set by cppspec.c to 1.  */
 int is_cpp_driver;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -458,6 +461,7 @@ or with constant text in a single argume
 	assembler has done its job.
  %D	Dump out a -L option for each directory in startfile_prefixes.
 	If multilib_dir is set, extra entries are generated with it affixed.
+ %N     Output the currently selected multilib directory name.
  %l     process LINK_SPEC as a spec.
  %L     process LIB_SPEC as a spec.
  %G     process LIBGCC_SPEC as a spec.
@@ -3800,6 +3804,9 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4138,6 +4145,18 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+    {
+	switches[n_switches].part1 = "pthread";
+	switches[n_switches].args = 0;
+	switches[n_switches].live_cond = SWITCH_OK;
+	/* Do not print an error if there is not expansion for -pthread. */
+	switches[n_switches].validated = 1;
+	switches[n_switches].ordering = 0;
+
+	n_switches++;
+    }
+
   switches[n_switches].part1 = 0;
   infiles[n_infiles].name = 0;
 }
@@ -5109,6 +5128,17 @@ do_spec_1 (const char *spec, int inswitc
 	      return value;
 	    break;
 
+	  case 'N':
+	    if (multilib_dir)
+	      {
+		arg_going = 1;
+		obstack_grow (&obstack, "-fmultilib-dir=",
+			      strlen ("-fmultilib-dir="));
+	        obstack_grow (&obstack, multilib_dir,
+			      strlen (multilib_dir));
+	      }
+	    break;
+
 	    /* Here we define characters other than letters and digits.  */
 
 	  case '{':
--- gcc.orig/gcc.h	2005-06-25 03:02:01.000000000 +0100
+++ gcc/gcc.h	2011-03-21 18:36:52.923850671 +0000
@@ -38,7 +38,7 @@ struct spec_function
    || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
    || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
    || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
-   || (CHAR) == 'B' || (CHAR) == 'b')
+   || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
 
 /* This defines which multi-letter switches take arguments.  */
 
--- gcc.orig/gimplify.c	2006-11-19 16:15:47.000000000 +0000
+++ gcc/gimplify.c	2011-03-23 20:32:47.058620783 +0000
@@ -1845,6 +1845,7 @@ gimplify_call_expr (tree *expr_p, tree *
   tree decl;
   tree arglist;
   enum gimplify_status ret;
+  int reverse_args;
 
   gcc_assert (TREE_CODE (*expr_p) == CALL_EXPR);
 
@@ -1907,7 +1908,9 @@ gimplify_call_expr (tree *expr_p, tree *
   ret = gimplify_expr (&TREE_OPERAND (*expr_p, 0), pre_p, NULL,
 		       is_gimple_call_addr, fb_rvalue);
 
-  if (PUSH_ARGS_REVERSED)
+  /* Evaluate args left to right if evaluation order matters. */
+  reverse_args = flag_evaluation_order ? 0 : PUSH_ARGS_REVERSED;
+  if (reverse_args)
     TREE_OPERAND (*expr_p, 1) = nreverse (TREE_OPERAND (*expr_p, 1));
   for (arglist = TREE_OPERAND (*expr_p, 1); arglist;
        arglist = TREE_CHAIN (arglist))
@@ -1919,7 +1922,7 @@ gimplify_call_expr (tree *expr_p, tree *
       if (t == GS_ERROR)
 	ret = GS_ERROR;
     }
-  if (PUSH_ARGS_REVERSED)
+  if (reverse_args)
     TREE_OPERAND (*expr_p, 1) = nreverse (TREE_OPERAND (*expr_p, 1));
 
   /* Try this again in case gimplification exposed something.  */
--- gcc.orig/predict.c	2005-11-05 00:55:23.000000000 +0000
+++ gcc/predict.c	2011-03-21 18:36:53.043851268 +0000
@@ -1339,6 +1339,7 @@ tree_estimate_probability (void)
 	     care for error returns and other cases are often used for
 	     fast paths trought function.  */
 	  if (e->dest == EXIT_BLOCK_PTR
+	      && last_stmt (bb) == NULL_TREE
 	      && TREE_CODE (last_stmt (bb)) == RETURN_EXPR
 	      && !single_pred_p (bb))
 	    {
--- gcc.orig/real.c	2005-09-19 18:01:40.000000000 +0100
+++ gcc/real.c	2011-03-21 18:36:53.167851880 +0000
@@ -2212,6 +2212,8 @@ real_maxval (REAL_VALUE_TYPE *r, int sig
   np2 = SIGNIFICAND_BITS - fmt->p * fmt->log2_b;
   memset (r->sig, -1, SIGSZ * sizeof (unsigned long));
   clear_significand_below (r, np2);
+  if (REAL_MODE_FORMAT_COMPOSITE_P (mode))
+      clear_significand_bit (r, SIGNIFICAND_BITS - fmt->pnan - 1);
 }
 
 /* Fills R with 2**N.  */
--- gcc.orig/tree.def	2006-02-10 17:32:10.000000000 +0000
+++ gcc/tree.def	2011-03-21 18:36:53.287852477 +0000
@@ -526,6 +526,13 @@ DEFTREECODE (BIND_EXPR, "bind_expr", tcc
    Operand 2 is the static chain argument, or NULL.  */
 DEFTREECODE (CALL_EXPR, "call_expr", tcc_expression, 3)
 
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
--- gcc.orig/tree-gimple.c	2005-11-20 19:05:43.000000000 +0000
+++ gcc/tree-gimple.c	2011-03-21 18:36:53.555853805 +0000
@@ -72,6 +72,8 @@ is_gimple_formal_tmp_rhs (tree t)
     case VECTOR_CST:
     case OBJ_TYPE_REF:
     case ASSERT_EXPR:
+    case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+    case STATIC_CHAIN_DECL:
       return true;
 
     default:
@@ -143,7 +145,10 @@ is_gimple_lvalue (tree t)
 	  || TREE_CODE (t) == WITH_SIZE_EXPR
 	  /* These are complex lvalues, but don't have addresses, so they
 	     go here.  */
-	  || TREE_CODE (t) == BIT_FIELD_REF);
+	  || TREE_CODE (t) == BIT_FIELD_REF
+          /* This is an lvalue because it will be replaced with the real
+	     static chain decl. */
+	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
 }
 
 /*  Return true if T is a GIMPLE condition.  */
--- gcc.orig/tree-inline.c	2007-01-05 13:53:45.000000000 +0000
+++ gcc/tree-inline.c	2011-03-21 18:36:53.911855567 +0000
@@ -602,10 +602,21 @@ copy_body_r (tree *tp, int *walk_subtree
      knows not to copy VAR_DECLs, etc., so this is safe.  */
   else
     {
+      if (! id->cloning_p && ! id->saving_p &&
+	  TREE_CODE (*tp) == MODIFY_EXPR &&
+	  TREE_OPERAND (*tp, 0) ==
+	  DECL_STRUCT_FUNCTION (fn)->static_chain_decl)
+	{
+	  /* Don't use special methods to initialize the static chain
+	     if expanding inline.  If this code could somehow be
+	     expanded in expand_start_function, it would not be
+	     necessary to deal with it here. */
+	  *tp = build_empty_stmt ();
+	}
       /* Here we handle trees that are not completely rewritten.
 	 First we detect some inlining-induced bogosities for
 	 discarding.  */
-      if (TREE_CODE (*tp) == MODIFY_EXPR
+      else if (TREE_CODE (*tp) == MODIFY_EXPR
 	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
 	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
 	      (TREE_OPERAND (*tp, 0), fn)))
--- gcc.orig/tree-nested.c	2006-07-20 16:43:44.000000000 +0100
+++ gcc/tree-nested.c	2011-03-21 18:36:54.239857194 +0000
@@ -323,6 +323,7 @@ get_chain_decl (struct nesting_info *inf
   if (!decl)
     {
       tree type;
+      enum tree_code code;
 
       type = get_frame_type (info->outer);
       type = build_pointer_type (type);
@@ -333,12 +334,17 @@ get_chain_decl (struct nesting_info *inf
 	 Note also that it's represented as a parameter.  This is more
 	 close to the truth, since the initial value does come from 
 	 the caller.  */
-      decl = build_decl (PARM_DECL, create_tmp_var_name ("CHAIN"), type);
+      /* If the function has a custom static chain, a VAR_DECL is more
+	 appropriate. */
+      code = DECL_STRUCT_FUNCTION (info->context)->custom_static_chain ?
+	  VAR_DECL : PARM_DECL;
+      decl = build_decl (code, create_tmp_var_name ("CHAIN"), type);
       DECL_ARTIFICIAL (decl) = 1;
       DECL_IGNORED_P (decl) = 1;
       TREE_USED (decl) = 1;
       DECL_CONTEXT (decl) = info->context;
-      DECL_ARG_TYPE (decl) = type;
+      if (TREE_CODE (decl) == PARM_DECL)
+	  DECL_ARG_TYPE (decl) = type;
 
       /* Tell tree-inline.c that we never write to this variable, so
 	 it can copy-prop the replacement value immediately.  */
@@ -741,6 +747,8 @@ get_static_chain (struct nesting_info *i
 
   if (info->context == target_context)
     {
+      /* might be doing something wrong to need the following line.. */
+      get_frame_type (info);
       x = build_addr (info->frame_decl, target_context);
     }
   else
@@ -1224,6 +1232,10 @@ convert_tramp_reference (tree *tp, int *
       if (DECL_NO_STATIC_CHAIN (decl))
 	break;
 
+      /* Don't use a trampoline for a static reference. */
+      if (TREE_STATIC (t))
+	break;
+
       /* Lookup the immediate parent of the callee, as that's where
 	 we need to insert the trampoline.  */
       for (i = info; i->context != target_context; i = i->outer)
@@ -1288,6 +1300,14 @@ convert_call_expr (tree *tp, int *walk_s
 	  = get_static_chain (info, target_context, &wi->tsi);
       break;
 
+    case STATIC_CHAIN_EXPR:
+      *tp = get_static_chain (info, TREE_OPERAND (t, 0), &wi->tsi);
+      break;
+
+    case STATIC_CHAIN_DECL:
+      *tp = get_chain_decl (info);
+      break;
+ 
     case RETURN_EXPR:
     case MODIFY_EXPR:
     case WITH_SIZE_EXPR:
@@ -1387,7 +1407,29 @@ finalize_nesting_tree_1 (struct nesting_
       tree x = build (COMPONENT_REF, TREE_TYPE (root->chain_field),
 		      root->frame_decl, root->chain_field, NULL_TREE);
       x = build (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
-      append_to_statement_list (x, &stmt_list);
+      /* If the function has a custom static chain, chain_field must
+	 be set after the static chain. */
+      if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
+	{
+	  /* Should use walk_function instead. */
+	  tree_stmt_iterator i =
+	      tsi_start ( BIND_EXPR_BODY (DECL_SAVED_TREE (context)));
+	  while (!tsi_end_p (i))
+	    {
+	      tree t = tsi_stmt (i);
+	      if (TREE_CODE (t) == MODIFY_EXPR &&
+		  TREE_OPERAND (t, 0) == root->chain_decl)
+		{
+		  tsi_link_after(& i, x, TSI_SAME_STMT);
+		  x = NULL_TREE;
+		  break;
+		}
+	      tsi_next (& i);
+	    }
+	  gcc_assert(x == NULL_TREE);
+	}
+      else
+	append_to_statement_list (x, &stmt_list);
     }
 
   /* If trampolines were created, then we need to initialize them.  */
--- gcc.orig/tree-pretty-print.c	2005-07-31 21:55:41.000000000 +0100
+++ gcc/tree-pretty-print.c	2011-03-21 18:36:54.595858964 +0000
@@ -1004,6 +1004,16 @@ dump_generic_node (pretty_printer *buffe
 	pp_string (buffer, " [tail call]");
       break;
 
+    case STATIC_CHAIN_EXPR:
+	pp_string (buffer, "<<static chain of ");
+	dump_generic_node (buffer, TREE_OPERAND (node, 0), spc, flags, false);
+	pp_string (buffer, ">>");
+      break;
+
+    case STATIC_CHAIN_DECL:
+       pp_string (buffer, "<<static chain decl>>");
+       break;
+	
     case WITH_CLEANUP_EXPR:
       NIY;
       break;
--- gcc.orig/tree-sra.c	2005-11-21 00:55:57.000000000 +0000
+++ gcc/tree-sra.c	2011-03-21 18:36:57.019870974 +0000
@@ -198,6 +198,8 @@ sra_type_can_be_decomposed_p (tree type)
     case RECORD_TYPE:
       {
 	bool saw_one_field = false;
+	tree last_offset = size_zero_node;
+	tree cmp;
 
 	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
 	  if (TREE_CODE (t) == FIELD_DECL)
@@ -207,6 +209,11 @@ sra_type_can_be_decomposed_p (tree type)
 		  && (tree_low_cst (DECL_SIZE (t), 1)
 		      != TYPE_PRECISION (TREE_TYPE (t))))
 		goto fail;
+	      /* Reject aliased fields created by GDC for anonymous unions. */
+	      cmp = fold_binary_to_constant (LE_EXPR, boolean_type_node,
+		DECL_FIELD_OFFSET (t), last_offset);
+	      if (cmp == NULL_TREE || tree_expr_nonzero_p (cmp))
+		goto fail;
 
 	      saw_one_field = true;
 	    }
--- gcc.orig/varray.h	2005-06-25 03:02:01.000000000 +0100
+++ gcc/varray.h	2011-03-21 18:36:57.527873500 +0000
@@ -62,6 +62,17 @@ enum varray_data_enum {
   NUM_VARRAY_DATA
 };
 
+#ifndef __cplusplus 
+# define VARRAY_STRANGE_1 rtx
+# define VARRAY_STRANGE_2 rtvec
+# define VARRAY_STRANGE_3 tree
+#else
+# define VARRAY_STRANGE_1 rtx_
+# define VARRAY_STRANGE_2 rtvec_
+# define VARRAY_STRANGE_3 tree_
+#endif
+
+
 /* Union of various array types that are used.  */
 typedef union varray_data_tag GTY (()) {
   char			  GTY ((length ("%0.num_elements"),
@@ -91,11 +102,11 @@ typedef union varray_data_tag GTY (()) {
   char			 *GTY ((length ("%0.num_elements"),
 				tag ("VARRAY_DATA_CPTR")))	cptr[1];
   rtx			  GTY ((length ("%0.num_elements"),
-				tag ("VARRAY_DATA_RTX")))	rtx[1];
+				tag ("VARRAY_DATA_RTX")))	VARRAY_STRANGE_1[1];
   rtvec			  GTY ((length ("%0.num_elements"),
-				tag ("VARRAY_DATA_RTVEC")))	rtvec[1];
+				tag ("VARRAY_DATA_RTVEC")))	VARRAY_STRANGE_2[1];
   tree			  GTY ((length ("%0.num_elements"),
-				tag ("VARRAY_DATA_TREE")))	tree[1];
+				tag ("VARRAY_DATA_TREE")))	VARRAY_STRANGE_3[1];
   struct bitmap_head_def *GTY ((length ("%0.num_elements"),
 				tag ("VARRAY_DATA_BITMAP")))	bitmap[1];
   struct reg_info_def	 *GTY ((length ("%0.num_elements"), skip,
