--- gcc.orig/cgraph.c	2011-07-12 22:58:59.004152166 +0100
+++ gcc/cgraph.c	2011-07-12 23:19:00.918112133 +0100
@@ -451,6 +451,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -472,12 +473,16 @@ cgraph_node (tree decl)
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
+	{
+	  node->origin = cgraph_node (context);
+	  node->next_nested = node->origin->nested;
+	  node->origin->nested = node;
+	  node->master_clone = node;
+	}
     }
   if (assembler_name_hash)
     {
--- gcc.orig/config/i386/i386.c	2011-07-12 22:58:59.136152820 +0100
+++ gcc/config/i386/i386.c	2011-07-24 14:30:14.440623725 +0100
@@ -4268,6 +4268,10 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -4286,6 +4290,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -4299,6 +4307,27 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -4535,6 +4564,12 @@ ix86_return_pops_args (tree fundecl, tre
           || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
 	rtd = 1;
 
+      /* Optlink functions will pop the stack if returning float and
+         if not variable args.  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+          && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+	rtd = 1;
+
       if (rtd && ! stdarg_p (funtype))
 	return size;
     }
@@ -4780,6 +4815,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
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
@@ -7368,6 +7408,9 @@ ix86_can_use_return_insn_p (void)
   if (! reload_completed || frame_pointer_needed)
     return 0;
 
+  if (cfun->naked)
+    return 0;
+
   /* Don't allow more than 32 pop, since that's all we can do
      with one instruction.  */
   if (crtl->args.pops_args
@@ -7407,6 +7450,10 @@ ix86_frame_pointer_required (void)
   if (crtl->profile)
     return 1;
 
+  /* Optlink mandates the setting up of ebp, unless 'naked' is used.  */
+  if (crtl->args.info.optlink && !cfun->naked)
+    return true;
+
   return 0;
 }
 
@@ -7907,6 +7954,11 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+    /* As above, skip return address */
+    frame->stack_pointer_offset = UNITS_PER_WORD;
+
 #if 0
   fprintf (stderr, "\n");
   fprintf (stderr, "size: %ld\n", (long)size);
@@ -26874,7 +26926,7 @@ x86_output_mi_thunk (FILE *file ATTRIBUT
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -29630,6 +29682,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2011-07-12 22:58:59.148152876 +0100
+++ gcc/config/i386/i386.h	2011-07-12 23:19:00.994112506 +0100
@@ -1572,6 +1572,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_avx;			/* True when we want to warn about AVX ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2011-07-12 22:58:59.244153352 +0100
+++ gcc/config/rs6000/rs6000.c	2011-07-12 23:19:01.042112746 +0100
@@ -17664,7 +17664,8 @@ rs6000_output_function_epilogue (FILE *f
 	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
 	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
 	 a number, so for now use 9.  */
-      if (! strcmp (language_string, "GNU C"))
+      if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc.orig/dojump.c	2011-07-12 22:58:59.264153455 +0100
+++ gcc/dojump.c	2011-07-12 23:19:01.050112794 +0100
@@ -80,7 +80,8 @@ void
 clear_pending_stack_adjust (void)
 {
   if (optimize > 0
-      && (! flag_omit_frame_pointer || cfun->calls_alloca)
+      && ((! flag_omit_frame_pointer && ! cfun->naked)
+          || cfun->calls_alloca)
       && EXIT_IGNORE_STACK)
     discard_pending_stack_adjust ();
 }
--- gcc.orig/dwarf2out.c	2011-07-12 22:58:59.304153654 +0100
+++ gcc/dwarf2out.c	2011-07-12 23:19:01.086112965 +0100
@@ -6398,7 +6398,8 @@ is_c_family (void)
 
   return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
 	  || lang == DW_LANG_C99
-	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
+	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus
+	  || lang == DW_LANG_D);
 }
 
 /* Return TRUE if the language is C++.  */
@@ -14506,6 +14507,8 @@ gen_compile_unit_die (const char *filena
     language = DW_LANG_ObjC;
   else if (strcmp (language_string, "GNU Objective-C++") == 0)
     language = DW_LANG_ObjC_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else
     language = DW_LANG_C89;
 
@@ -15688,7 +15691,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/except.c	2011-07-12 22:58:59.312153690 +0100
+++ gcc/except.c	2011-07-12 23:19:01.094113002 +0100
@@ -200,7 +200,7 @@ struct eh_region GTY(())
 
 typedef struct eh_region *eh_region;
 
-struct call_site_record GTY(())
+struct call_site_record_d GTY(())
 {
   rtx landing_pad;
   int action;
@@ -1777,6 +1777,18 @@ sjlj_mark_call_sites (struct sjlj_lp_inf
 
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
@@ -3198,7 +3210,7 @@ add_call_site (rtx landing_pad, int acti
 {
   call_site_record record;
   
-  record = GGC_NEW (struct call_site_record);
+  record = GGC_NEW (struct call_site_record_d);
   record->landing_pad = landing_pad;
   record->action = action;
 
@@ -3399,7 +3411,7 @@ dw2_size_of_call_site_table (void)
 
   for (i = 0; i < n; ++i)
     {
-      struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
+      struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
       size += size_of_uleb128 (cs->action);
     }
 
@@ -3415,7 +3427,7 @@ sjlj_size_of_call_site_table (void)
 
   for (i = 0; i < n; ++i)
     {
-      struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
+      struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
       size += size_of_uleb128 (INTVAL (cs->landing_pad));
       size += size_of_uleb128 (cs->action);
     }
@@ -3432,7 +3444,7 @@ dw2_output_call_site_table (void)
 
   for (i = 0; i < n; ++i)
     {
-      struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
+      struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
       char reg_start_lab[32];
       char reg_end_lab[32];
       char landing_pad_lab[32];
@@ -3486,7 +3498,7 @@ sjlj_output_call_site_table (void)
 
   for (i = 0; i < n; ++i)
     {
-      struct call_site_record *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
+      struct call_site_record_d *cs = VEC_index (call_site_record, crtl->eh.call_site_record, i);
 
       dw2_asm_output_data_uleb128 (INTVAL (cs->landing_pad),
 				   "region %d landing pad", i);
--- gcc.orig/function.c	2011-07-12 22:58:59.368153965 +0100
+++ gcc/function.c	2011-07-12 23:19:01.114113110 +0100
@@ -3190,7 +3190,8 @@ assign_parms (tree fndecl)
       FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
 			    data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3205,7 +3206,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.first_conversion_insn);
+  if (!cfun->naked)
+    emit_insn (all.first_conversion_insn);
 
   /* Estimate reload stack alignment from scalar return mode.  */
   if (SUPPORTS_STACK_ALIGNMENT)
@@ -3357,6 +3359,9 @@ gimplify_parameters (void)
   tree fnargs, parm;
   gimple_seq stmts = NULL;
 
+  if (cfun->naked)
+    return NULL;
+
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -4985,6 +4990,9 @@ thread_prologue_and_epilogue_insns (void
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+    return;
+
   rtl_profile_for_bb (ENTRY_BLOCK_PTR);
 #ifdef HAVE_prologue
   if (HAVE_prologue)
--- gcc.orig/function.h	2011-07-12 22:58:59.372153990 +0100
+++ gcc/function.h	2011-07-12 23:19:14.170177845 +0100
@@ -137,7 +137,7 @@ struct expr_status GTY(())
   rtx x_forced_labels;
 };
 
-typedef struct call_site_record *call_site_record;
+typedef struct call_site_record_d *call_site_record;
 DEF_VEC_P(call_site_record);
 DEF_VEC_ALLOC_P(call_site_record, gc);
 
@@ -175,12 +175,12 @@ struct rtl_eh GTY(())
 struct gimple_df;
 struct temp_slot;
 typedef struct temp_slot *temp_slot_p;
-struct call_site_record;
+struct call_site_record_d;
 
 DEF_VEC_P(temp_slot_p);
 DEF_VEC_ALLOC_P(temp_slot_p,gc);
-struct ipa_opt_pass;
-typedef struct ipa_opt_pass *ipa_opt_pass;
+struct ipa_opt_pass_d;
+typedef struct ipa_opt_pass_d *ipa_opt_pass;
 
 DEF_VEC_P(ipa_opt_pass);
 DEF_VEC_ALLOC_P(ipa_opt_pass,heap);
@@ -600,6 +600,10 @@ struct function GTY(())
      adjusts one of its arguments and forwards to another
      function.  */
   unsigned int is_thunk : 1;
+
+  /* Nonzero if no code should be generated for prologues, copying
+     parameters, etc. */
+  unsigned int naked : 1;
 };
 
 /* If va_list_[gf]pr_size is set to this, it means we don't know how
--- gcc.orig/gcc.c	2011-07-12 22:58:59.392154086 +0100
+++ gcc/gcc.c	2011-07-12 23:19:01.130113189 +0100
@@ -126,6 +126,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -4022,6 +4026,9 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+      n_switches++;
+ 
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4332,6 +4339,18 @@ warranty; not even for MERCHANTABILITY o
       infiles[0].name   = "help-dummy";
     }
 
+  if (need_pthreads)
+    {
+      switches[n_switches].part1 = "pthread";
+      switches[n_switches].args = 0;
+      switches[n_switches].live_cond = 0;
+      /* Do not print an error if there is not expansion for -pthread. */
+      switches[n_switches].validated = 1;
+      switches[n_switches].ordering = 0;
+
+      n_switches++;
+    }
+
   switches[n_switches].part1 = 0;
   infiles[n_infiles].name = 0;
 }
--- gcc.orig/gimple.h	2011-07-12 22:58:59.412154189 +0100
+++ gcc/gimple.h	2011-07-12 23:19:01.138113222 +0100
@@ -65,7 +65,7 @@ extern void gimple_check_failed (const_g
     const_gimple __gs = (GS);						\
     if (gimple_code (__gs) != (CODE))					\
       gimple_check_failed (__gs, __FILE__, __LINE__, __FUNCTION__,	\
-	  		   (CODE), 0);					\
+	  		   (CODE), ERROR_MARK);				\
   } while (0)
 #else  /* not ENABLE_GIMPLE_CHECKING  */
 #define GIMPLE_CHECK(GS, CODE)			(void)0
@@ -2216,7 +2216,7 @@ static inline enum tree_code
 gimple_cond_code (const_gimple gs)
 {
   GIMPLE_CHECK (gs, GIMPLE_COND);
-  return gs->gsbase.subcode;
+  return (enum tree_code) gs->gsbase.subcode;
 }
 
 
--- gcc.orig/ipa-cp.c	2011-07-12 22:58:59.416154202 +0100
+++ gcc/ipa-cp.c	2011-07-12 23:19:01.142113249 +0100
@@ -1393,7 +1393,7 @@ cgraph_gate_cp (void)
   return flag_ipa_cp;
 }
 
-struct ipa_opt_pass pass_ipa_cp = 
+struct ipa_opt_pass_d pass_ipa_cp = 
 {
  {
   IPA_PASS,
--- gcc.orig/ipa-inline.c	2011-07-12 22:58:59.424154248 +0100
+++ gcc/ipa-inline.c	2011-07-12 23:19:01.146113262 +0100
@@ -1753,7 +1753,7 @@ inline_transform (struct cgraph_node *no
   return todo | execute_fixup_cfg ();
 }
 
-struct ipa_opt_pass pass_ipa_inline = 
+struct ipa_opt_pass_d pass_ipa_inline = 
 {
  {
   IPA_PASS,
--- gcc.orig/ipa-pure-const.c	2011-07-12 22:58:59.428154263 +0100
+++ gcc/ipa-pure-const.c	2011-07-12 23:19:01.150113278 +0100
@@ -910,7 +910,7 @@ gate_pure_const (void)
 	  && !(errorcount || sorrycount));
 }
 
-struct ipa_opt_pass pass_ipa_pure_const =
+struct ipa_opt_pass_d pass_ipa_pure_const =
 {
  {
   IPA_PASS,
--- gcc.orig/ipa-reference.c	2011-07-12 22:58:59.432154288 +0100
+++ gcc/ipa-reference.c	2011-07-12 23:19:01.154113299 +0100
@@ -1257,7 +1257,7 @@ gate_reference (void)
 	  && !(errorcount || sorrycount));
 }
 
-struct ipa_opt_pass pass_ipa_reference =
+struct ipa_opt_pass_d pass_ipa_reference =
 {
  {
   IPA_PASS,
--- gcc.orig/ira.c	2011-07-12 22:58:59.448154363 +0100
+++ gcc/ira.c	2011-07-12 23:19:01.162113344 +0100
@@ -1404,7 +1404,7 @@ setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        || crtl->accesses_prior_frames
        || crtl->stack_realign_needed
--- gcc.orig/passes.c	2011-07-12 22:58:59.460154424 +0100
+++ gcc/passes.c	2011-07-12 23:19:01.166113360 +0100
@@ -1146,14 +1146,14 @@ update_properties_after_pass (void *data
 static void
 add_ipa_transform_pass (void *data)
 {
-  struct ipa_opt_pass *ipa_pass = (struct ipa_opt_pass *) data;
+  struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *) data;
   VEC_safe_push (ipa_opt_pass, heap, cfun->ipa_transforms_to_apply, ipa_pass);
 }
 
 /* Execute summary generation for all of the passes in IPA_PASS.  */
 
 static void
-execute_ipa_summary_passes (struct ipa_opt_pass *ipa_pass)
+execute_ipa_summary_passes (struct ipa_opt_pass_d *ipa_pass)
 {
   while (ipa_pass)
     {
@@ -1167,7 +1167,7 @@ execute_ipa_summary_passes (struct ipa_o
 	  ipa_pass->generate_summary ();
 	  pass_fini_dump_file (pass);
 	}
-      ipa_pass = (struct ipa_opt_pass *)ipa_pass->pass.next;
+      ipa_pass = (struct ipa_opt_pass_d *)ipa_pass->pass.next;
     }
 }
 
@@ -1175,7 +1175,7 @@ execute_ipa_summary_passes (struct ipa_o
 
 static void
 execute_one_ipa_transform_pass (struct cgraph_node *node,
-				struct ipa_opt_pass *ipa_pass)
+				struct ipa_opt_pass_d *ipa_pass)
 {
   struct opt_pass *pass = &ipa_pass->pass;
   unsigned int todo_after = 0;
@@ -1347,7 +1347,7 @@ execute_ipa_pass_list (struct opt_pass *
 	    {
 	      if (!quiet_flag && !cfun)
 		fprintf (stderr, " <summary generate>");
-	      execute_ipa_summary_passes ((struct ipa_opt_pass *) pass);
+	      execute_ipa_summary_passes ((struct ipa_opt_pass_d *) pass);
 	    }
 	  summaries_generated = true;
 	}
--- gcc.orig/system.h	2011-07-12 22:58:59.464154447 +0100
+++ gcc/system.h	2011-07-12 23:19:01.170113382 +0100
@@ -786,6 +786,9 @@ extern void fancy_abort (const char *, i
    change after the fact).  Beyond these uses, most other cases of
    using this macro should be viewed with extreme caution.  */
 
+#ifdef __cplusplus
+#define CONST_CAST2(TOTYPE,FROMTYPE,X) (const_cast<TOTYPE> (X))
+#else
 #if defined(__GNUC__) && GCC_VERSION > 4000
 /* GCC 4.0.x has a bug where it may ICE on this expression,
    so does GCC 3.4.x (PR17436).  */
@@ -793,6 +796,7 @@ extern void fancy_abort (const char *, i
 #else
 #define CONST_CAST2(TOTYPE,FROMTYPE,X) ((TOTYPE)(FROMTYPE)(X))
 #endif
+#endif
 #define CONST_CAST(TYPE,X) CONST_CAST2(TYPE, const TYPE, (X))
 #define CONST_CAST_TREE(X) CONST_CAST(union tree_node *, (X))
 #define CONST_CAST_RTX(X) CONST_CAST(struct rtx_def *, (X))
--- gcc.orig/tree-pass.h	2011-07-12 22:58:59.556154900 +0100
+++ gcc/tree-pass.h	2011-07-12 23:19:01.182113439 +0100
@@ -157,7 +157,7 @@ struct cgraph_node;
 
 /* Description of IPA pass with generate summary, write, execute, read and
    transform stages.  */
-struct ipa_opt_pass
+struct ipa_opt_pass_d
 {
   struct opt_pass pass;
 
@@ -390,10 +390,10 @@ extern struct gimple_opt_pass pass_build
 extern struct gimple_opt_pass pass_reset_cc_flags;
 
 /* IPA Passes */
-extern struct ipa_opt_pass pass_ipa_inline;
-extern struct ipa_opt_pass pass_ipa_cp;
-extern struct ipa_opt_pass pass_ipa_reference;
-extern struct ipa_opt_pass pass_ipa_pure_const;
+extern struct ipa_opt_pass_d pass_ipa_inline;
+extern struct ipa_opt_pass_d pass_ipa_cp;
+extern struct ipa_opt_pass_d pass_ipa_reference;
+extern struct ipa_opt_pass_d pass_ipa_pure_const;
 
 extern struct simple_ipa_opt_pass pass_ipa_matrix_reorg;
 extern struct simple_ipa_opt_pass pass_ipa_early_inline;
--- gcc.orig/tree-sra.c	2011-07-12 22:58:59.580155022 +0100
+++ gcc/tree-sra.c	2011-07-12 23:19:01.186113457 +0100
@@ -263,6 +263,8 @@ sra_type_can_be_decomposed_p (tree type)
     case RECORD_TYPE:
       {
 	bool saw_one_field = false;
+	tree last_offset = size_zero_node;
+	tree cmp;
 
 	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
 	  if (TREE_CODE (t) == FIELD_DECL)
@@ -274,6 +276,12 @@ sra_type_can_be_decomposed_p (tree type)
 		      != TYPE_PRECISION (TREE_TYPE (t))))
 		goto fail;
 
+	      /* Reject aliased fields created by GDC for anonymous unions. */
+	      cmp = fold_binary_to_constant (LE_EXPR, boolean_type_node,
+					     DECL_FIELD_OFFSET (t), last_offset);
+	      if (cmp == NULL_TREE || tree_expr_nonzero_p (cmp))
+		goto fail;
+
 	      saw_one_field = true;
 	    }
 
--- gcc.orig/tree-ssa.c	2011-07-12 22:58:59.612155175 +0100
+++ gcc/tree-ssa.c	2011-07-12 23:19:01.190113486 +0100
@@ -1094,8 +1094,10 @@ useless_type_conversion_p_1 (tree outer_
       && TYPE_CANONICAL (inner_type) == TYPE_CANONICAL (outer_type))
     return true;
 
-  /* Changes in machine mode are never useless conversions.  */
-  if (TYPE_MODE (inner_type) != TYPE_MODE (outer_type))
+  /* Changes in machine mode are never useless conversions unless we
+     deal with aggregate types in which case we defer to later checks.  */
+  if (TYPE_MODE (inner_type) != TYPE_MODE (outer_type)
+      && !AGGREGATE_TYPE_P (inner_type))
     return false;
 
   /* If both the inner and outer types are integral types, then the
