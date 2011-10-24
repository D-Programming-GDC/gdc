--- gcc.orig/cgraph.c	2010-08-20 18:07:17.979440836 +0100
+++ gcc/cgraph.c	2010-08-20 18:08:05.728441136 +0100
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
--- gcc.orig/config/i386/i386.c	2010-08-20 18:07:18.079444473 +0100
+++ gcc/config/i386/i386.c	2011-07-24 14:27:02.959674225 +0100
@@ -3149,6 +3149,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("fastcall and regparm attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("fastcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine stdcall with fastcall (redundant), regparm and
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
@@ -3391,6 +3420,12 @@ ix86_return_pops_args (tree fundecl, tre
           || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
 	rtd = 1;
 
+      /* Optlink functions will pop the stack if returning float and
+         if not variable args..  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+          && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+	rtd = 1;
+
       if (rtd && ! stdarg_p (funtype))
 	return size;
     }
@@ -3529,6 +3564,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
 	    }
 	  else
 	    cum->nregs = ix86_function_regparm (fntype, fndecl);
+
+	  /* For optlink, last parameter is passed in eax rather than
+	     being pushed on the stack.  */
+	  if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (fntype)))
+	    cum->optlink = 1;
 	}
 
       /* Set up the number of SSE registers used for passing SFmode
@@ -5677,6 +5717,9 @@ ix86_can_use_return_insn_p (void)
   if (! reload_completed || frame_pointer_needed)
     return 0;
 
+  if (cfun->naked)
+    return 0;
+
   /* Don't allow more than 32 pop, since that's all we can do
      with one instruction.  */
   if (current_function_pops_args
@@ -5715,6 +5758,10 @@ ix86_frame_pointer_required (void)
   if (current_function_profile)
     return 1;
 
+  /* Optlink mandates the setting up of ebp, unless 'naked' is used.  */
+  if (current_function_args_info.optlink && !cfun->naked)
+    return 1;
+
   return 0;
 }
 
@@ -6151,6 +6198,11 @@ ix86_compute_frame_layout (struct ix86_f
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
@@ -22924,7 +22976,7 @@ x86_output_mi_thunk (FILE *file ATTRIBUT
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -25240,6 +25292,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2009-11-13 19:51:52.000000000 +0000
+++ gcc/config/i386/i386.h	2011-07-24 13:05:11.883321506 +0100
@@ -1672,6 +1672,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_sse;			/* True when we want to warn about SSE ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2010-08-20 18:07:18.231439846 +0100
+++ gcc/config/rs6000/rs6000.c	2010-08-20 18:08:06.464443120 +0100
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
--- gcc.orig/dojump.c	2009-05-07 16:53:11.000000000 +0100
+++ gcc/dojump.c	2011-07-24 13:10:30.688902370 +0100
@@ -70,7 +70,8 @@ void
 clear_pending_stack_adjust (void)
 {
   if (optimize > 0
-      && (! flag_omit_frame_pointer || current_function_calls_alloca)
+      && ((! flag_omit_frame_pointer && ! cfun->naked)
+          || current_function_calls_alloca)
       && EXIT_IGNORE_STACK
       && ! (DECL_INLINE (current_function_decl) && ! flag_no_inline))
     discard_pending_stack_adjust ();
--- gcc.orig/dwarf2out.c	2010-08-20 18:07:18.360467489 +0100
+++ gcc/dwarf2out.c	2010-08-20 18:08:06.483443929 +0100
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
--- gcc.orig/except.c	2010-08-07 23:20:32.623306514 +0100
+++ gcc/except.c	2010-09-01 18:03:06.763377179 +0100
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
--- gcc.orig/function.c	2010-08-20 18:07:18.479439027 +0100
+++ gcc/function.c	2011-07-24 12:54:27.280125106 +0100
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
 
@@ -5179,6 +5184,9 @@ thread_prologue_and_epilogue_insns (void
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+      return;
+
 #ifdef HAVE_prologue
   if (HAVE_prologue)
     {
--- gcc.orig/function.h	2010-08-20 18:07:18.499442756 +0100
+++ gcc/function.h	2011-07-24 12:54:35.248164604 +0100
@@ -463,6 +463,10 @@ struct function GTY(())
 
   /* Nonzero if pass_tree_profile was run on this function.  */
   unsigned int after_tree_profile : 1;
+
+  /* Nonzero if no code should be generated for prologues, copying
+     parameters, etc. */
+  unsigned int naked : 1;
 };
 
 /* If va_list_[gf]pr_size is set to this, it means we don't know how
--- gcc.orig/gcc.c	2010-08-20 18:07:18.515441647 +0100
+++ gcc/gcc.c	2011-07-24 13:07:13.707925615 +0100
@@ -129,6 +129,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -365,6 +368,9 @@ static const char *replace_outfile_spec_
 static const char *version_compare_spec_function (int, const char **);
 static const char *include_spec_function (int, const char **);
 static const char *print_asm_header_spec_function (int, const char **);
+
+extern const char *d_all_sources_spec_function (int, const char **);
+extern const char *d_output_prefix_spec_function (int, const char **);
 
 /* The Specs Language
 
@@ -3974,6 +3981,9 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4280,6 +4290,18 @@ warranty; not even for MERCHANTABILITY o
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
--- gcc.orig/gcc.h	2010-08-20 18:07:18.548356656 +0100
+++ gcc/gcc.h	2010-08-20 18:08:06.540492593 +0100
@@ -37,7 +37,7 @@ struct spec_function
    || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
    || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
    || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
-   || (CHAR) == 'B' || (CHAR) == 'b')
+   || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
 
 /* This defines which multi-letter switches take arguments.  */
 
--- gcc.orig/reload1.c	2007-10-22 20:28:23.000000000 +0100
+++ gcc/reload1.c	2011-07-24 13:15:09.350284190 +0100
@@ -3716,7 +3716,7 @@ init_elim_table (void)
 
   /* Does this function require a frame pointer?  */
 
-  frame_pointer_needed = (! flag_omit_frame_pointer
+  frame_pointer_needed = ((! flag_omit_frame_pointer && ! cfun->naked)
 			  /* ?? If EXIT_IGNORE_STACK is set, we will not save
 			     and restore sp for alloca.  So we can't eliminate
 			     the frame pointer in that case.  At some point,
--- gcc.orig/tree-nested.c	2010-08-20 18:07:18.615442350 +0100
+++ gcc/tree-nested.c	2011-07-24 12:57:20.604984574 +0100
@@ -1640,6 +1640,10 @@ convert_tramp_reference (tree *tp, int *
       if (DECL_NO_STATIC_CHAIN (decl))
 	break;
 
+      /* Don't use a trampoline for a static reference. */
+      if (TREE_STATIC (t))
+	break;
+
       /* Lookup the immediate parent of the callee, as that's where
 	 we need to insert the trampoline.  */
       for (i = info; i->context != target_context; i = i->outer)
--- gcc.orig/tree-sra.c	2010-08-20 18:07:18.663445099 +0100
+++ gcc/tree-sra.c	2010-08-20 18:08:06.575454930 +0100
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
