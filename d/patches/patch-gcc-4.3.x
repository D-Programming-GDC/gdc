--- gcc.orig/cgraph.c	2008-11-14 13:26:59.000000000 +0000
+++ gcc/cgraph.c	2011-03-21 18:37:21.139990586 +0000
@@ -181,6 +181,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -202,12 +203,16 @@ cgraph_node (tree decl)
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
--- gcc.orig/cgraphunit.c	2008-11-14 13:26:59.000000000 +0000
+++ gcc/cgraphunit.c	2011-03-21 18:37:21.143990613 +0000
@@ -1150,6 +1150,10 @@ cgraph_mark_functions_to_output (void)
 static void
 cgraph_expand_function (struct cgraph_node *node)
 {
+  int save_flag_omit_frame_pointer = flag_omit_frame_pointer;
+  static int inited = 0;
+  static int orig_omit_frame_pointer;
+  
   tree decl = node->decl;
 
   /* We ought to not compile any inline clones.  */
@@ -1159,11 +1163,21 @@ cgraph_expand_function (struct cgraph_no
     announce_function (decl);
 
   gcc_assert (node->lowered);
+  
+  if (!inited)
+  {
+      inited = 1;
+      orig_omit_frame_pointer = flag_omit_frame_pointer;
+  }
+  flag_omit_frame_pointer = orig_omit_frame_pointer ||
+    DECL_STRUCT_FUNCTION (decl)->naked;
 
   /* Generate RTL for the body of DECL.  */
   if (lang_hooks.callgraph.emit_associated_thunks)
     lang_hooks.callgraph.emit_associated_thunks (decl);
   tree_rest_of_compilation (decl);
+  
+  flag_omit_frame_pointer = save_flag_omit_frame_pointer;
 
   /* Make sure that BE didn't give up on compiling.  */
   /* ??? Can happen with nested function of extern inline.  */
--- gcc.orig/config/i386/i386.c	2010-03-31 21:14:10.000000000 +0100
+++ gcc/config/i386/i386.c	2011-03-21 19:42:27.571361547 +0000
@@ -3145,6 +3145,10 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -3163,6 +3167,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -3176,6 +3184,27 @@ ix86_handle_cconv_attribute (tree *node,
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
+      if (lookup_attribute ("stdcall", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and stdcall attributes are not compatible");
+	}
+      if (lookup_attribute ("fastcall", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and fastcall attributes are not compatible");
+	}
+      if (lookup_attribute ("cdecl", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("optlink and cdecl attributes are not compatible");
+	}
     }
 
   /* Can combine sseregparm with all attributes.  */
@@ -3391,6 +3420,12 @@ ix86_return_pops_args (tree fundecl, tre
           || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
 	rtd = 1;
 
+      /* Optlink functions will pop the stack if returning float and
+         if not variable args.  */
+      else if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+          && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+	rtd = 1;
+
       if (rtd && ! stdarg_p (funtype))
 	return size;
     }
@@ -6151,6 +6186,11 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+      /* As above, skip return address */
+      frame->stack_pointer_offset = UNITS_PER_WORD;
+
 #if 0
   fprintf (stderr, "\n");
   fprintf (stderr, "nregs: %ld\n", (long)frame->nregs);
@@ -22924,7 +22964,7 @@ x86_output_mi_thunk (FILE *file ATTRIBUT
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -25240,6 +25280,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2009-11-13 19:51:52.000000000 +0000
+++ gcc/config/i386/i386.h	2011-03-21 19:39:45.910559903 +0000
@@ -1672,6 +1672,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_sse;			/* True when we want to warn about SSE ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2009-09-23 23:30:05.000000000 +0100
+++ gcc/config/rs6000/rs6000.c	2011-03-21 18:37:21.267991218 +0000
@@ -16943,7 +16943,8 @@ rs6000_output_function_epilogue (FILE *f
 	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
 	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
 	 a number, so for now use 9.  */
-      if (! strcmp (language_string, "GNU C"))
+      if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU F95"))
--- gcc.orig/dwarf2out.c	2009-06-18 21:06:04.000000000 +0100
+++ gcc/dwarf2out.c	2011-03-21 18:37:21.327991520 +0000
@@ -5743,7 +5743,8 @@ is_c_family (void)
 
   return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
 	  || lang == DW_LANG_C99
-	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
+	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus
+	  || lang == DW_LANG_D);
 }
 
 /* Return TRUE if the language is C++.  */
@@ -13267,6 +13268,8 @@ gen_compile_unit_die (const char *filena
     language = DW_LANG_ObjC;
   else if (strcmp (language_string, "GNU Objective-C++") == 0)
     language = DW_LANG_ObjC_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else
     language = DW_LANG_C89;
 
@@ -14408,7 +14411,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/except.c	2008-06-28 11:43:12.000000000 +0100
+++ gcc/except.c	2011-03-21 18:37:21.347991617 +0000
@@ -1821,6 +1821,18 @@ sjlj_mark_call_sites (struct sjlj_lp_inf
 
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
--- gcc.orig/expr.c	2009-01-06 16:17:41.000000000 +0000
+++ gcc/expr.c	2011-03-21 18:37:21.383991797 +0000
@@ -9227,6 +9227,11 @@ expand_expr_real_1 (tree exp, rtx target
 	 represent.  */
       return const0_rtx;
 
+    case STATIC_CHAIN_EXPR:
+    case STATIC_CHAIN_DECL:
+      /* Lowered by tree-nested.c */
+      gcc_unreachable ();
+
     case EXC_PTR_EXPR:
       return get_exception_pointer (cfun);
 
--- gcc.orig/function.c	2009-06-19 22:44:24.000000000 +0100
+++ gcc/function.c	2011-03-21 18:37:21.407991915 +0000
@@ -3062,7 +3062,8 @@ assign_parms (tree fndecl)
       FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
 			    data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3077,7 +3078,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.first_conversion_insn);
+  if (!cfun->naked)
+    emit_insn (all.first_conversion_insn);
 
   /* If we are receiving a struct value address as the first argument, set up
      the RTL for the function result. As this might require code to convert
@@ -3207,6 +3209,9 @@ gimplify_parameters (void)
   struct assign_parm_data_all all;
   tree fnargs, parm, stmts = NULL;
 
+  if (cfun->naked)
+    return NULL;
+  
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -4280,11 +4285,15 @@ expand_function_start (tree subr)
       tree parm = cfun->static_chain_decl;
       rtx local = gen_reg_rtx (Pmode);
 
-      set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
       SET_DECL_RTL (parm, local);
       mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
 
-      emit_move_insn (local, static_chain_incoming_rtx);
+      if (! cfun->custom_static_chain)
+        {
+	    set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
+	    emit_move_insn (local, static_chain_incoming_rtx);
+	}
+      /* else, the static chain will be set in the main body */
     }
 
   /* If the function receives a non-local goto, then store the
@@ -5179,6 +5188,9 @@ thread_prologue_and_epilogue_insns (void
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+      return;
+
 #ifdef HAVE_prologue
   if (HAVE_prologue)
     {
--- gcc.orig/function.h	2008-01-26 17:18:35.000000000 +0000
+++ gcc/function.h	2011-03-21 18:37:21.419991989 +0000
@@ -463,6 +463,14 @@ struct function GTY(())
 
   /* Nonzero if pass_tree_profile was run on this function.  */
   unsigned int after_tree_profile : 1;
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
--- gcc.orig/gcc.c	2008-03-02 22:55:19.000000000 +0000
+++ gcc/gcc.c	2011-03-21 18:37:21.455992151 +0000
@@ -129,6 +129,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -472,6 +475,7 @@ or with constant text in a single argume
 	assembler has done its job.
  %D	Dump out a -L option for each directory in startfile_prefixes.
 	If multilib_dir is set, extra entries are generated with it affixed.
+ %N     Output the currently selected multilib directory name.
  %l     process LINK_SPEC as a spec.
  %L     process LIB_SPEC as a spec.
  %G     process LIBGCC_SPEC as a spec.
@@ -3974,6 +3978,9 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4280,6 +4287,18 @@ warranty; not even for MERCHANTABILITY o
       infiles[0].name   = "help-dummy";
     }
 
+  if (need_pthreads)
+    {
+	switches[n_switches].part1 = "pthread";
+	switches[n_switches].args = 0;
+	switches[n_switches].live_cond = 0;
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
@@ -5240,6 +5259,17 @@ do_spec_1 (const char *spec, int inswitc
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
--- gcc.orig/gcc.h	2007-07-26 09:37:01.000000000 +0100
+++ gcc/gcc.h	2011-03-21 18:37:21.455992151 +0000
@@ -37,7 +37,7 @@ struct spec_function
    || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
    || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
    || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
-   || (CHAR) == 'B' || (CHAR) == 'b')
+   || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
 
 /* This defines which multi-letter switches take arguments.  */
 
--- gcc.orig/tree.def	2007-10-29 11:05:04.000000000 +0000
+++ gcc/tree.def	2011-03-21 18:37:21.463992196 +0000
@@ -539,6 +539,13 @@ DEFTREECODE (BIND_EXPR, "bind_expr", tcc
    arguments to the call.  */
 DEFTREECODE (CALL_EXPR, "call_expr", tcc_vl_exp, 3)
 
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
--- gcc.orig/tree-gimple.c	2007-12-13 21:49:09.000000000 +0000
+++ gcc/tree-gimple.c	2011-03-21 18:37:21.475992250 +0000
@@ -74,6 +74,8 @@ is_gimple_formal_tmp_rhs (tree t)
     case VECTOR_CST:
     case OBJ_TYPE_REF:
     case ASSERT_EXPR:
+    case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+    case STATIC_CHAIN_DECL:
       return true;
 
     default:
@@ -147,7 +149,10 @@ is_gimple_lvalue (tree t)
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
--- gcc.orig/tree-nested.c	2008-05-29 12:35:05.000000000 +0100
+++ gcc/tree-nested.c	2011-03-21 18:37:21.483992295 +0000
@@ -815,6 +815,8 @@ get_static_chain (struct nesting_info *i
 
   if (info->context == target_context)
     {
+      /* might be doing something wrong to need the following line.. */
+      get_frame_type (info);
       x = build_addr (info->frame_decl, target_context);
     }
   else
@@ -1640,6 +1642,10 @@ convert_tramp_reference (tree *tp, int *
       if (DECL_NO_STATIC_CHAIN (decl))
 	break;
 
+      /* Don't use a trampoline for a static reference. */
+      if (TREE_STATIC (t))
+	break;
+
       /* Lookup the immediate parent of the callee, as that's where
 	 we need to insert the trampoline.  */
       for (i = info; i->context != target_context; i = i->outer)
@@ -1714,6 +1720,14 @@ convert_call_expr (tree *tp, int *walk_s
 	}
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
     case GIMPLE_MODIFY_STMT:
     case WITH_SIZE_EXPR:
@@ -1889,9 +1903,34 @@ finalize_nesting_tree_1 (struct nesting_
     {
       annotate_all_with_locus (&stmt_list,
 			       DECL_SOURCE_LOCATION (context));
-      append_to_statement_list (BIND_EXPR_BODY (DECL_SAVED_TREE (context)),
-				&stmt_list);
-      BIND_EXPR_BODY (DECL_SAVED_TREE (context)) = stmt_list;
+      /* If the function has a custom static chain, chain_field must
+	 be set after the static chain. */
+      if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
+	{
+	  /* Should use walk_function instead. */
+	  tree_stmt_iterator i =
+	      tsi_start ( BIND_EXPR_BODY (DECL_SAVED_TREE (context)));
+	  int found = 0;
+	  while (!tsi_end_p (i))
+	    {
+	      tree t = tsi_stmt (i);
+	      if (TREE_CODE (t) == GIMPLE_MODIFY_STMT &&
+		  GIMPLE_STMT_OPERAND (t, 0) == root->chain_decl)
+		{
+		  tsi_link_after (& i, stmt_list, TSI_SAME_STMT);
+		  found = 1;
+		  break;
+		}
+	      tsi_next (& i);
+	    }
+	  gcc_assert (found);
+	}
+      else
+        {
+	  append_to_statement_list (BIND_EXPR_BODY (DECL_SAVED_TREE (context)),
+				    &stmt_list);
+	  BIND_EXPR_BODY (DECL_SAVED_TREE (context)) = stmt_list;
+	}
     }
 
   /* If a chain_decl was created, then it needs to be registered with
--- gcc.orig/tree-pretty-print.c	2008-01-27 16:48:54.000000000 +0000
+++ gcc/tree-pretty-print.c	2011-03-21 18:37:21.491992325 +0000
@@ -1251,6 +1251,16 @@ dump_generic_node (pretty_printer *buffe
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
--- gcc.orig/tree-sra.c	2010-04-18 16:56:56.000000000 +0100
+++ gcc/tree-sra.c	2011-03-21 18:37:21.499992373 +0000
@@ -262,6 +262,8 @@ sra_type_can_be_decomposed_p (tree type)
     case RECORD_TYPE:
       {
 	bool saw_one_field = false;
+	tree last_offset = size_zero_node;
+	tree cmp;
 
 	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
 	  if (TREE_CODE (t) == FIELD_DECL)
@@ -271,6 +273,11 @@ sra_type_can_be_decomposed_p (tree type)
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
