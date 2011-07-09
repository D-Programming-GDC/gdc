--- gcc.orig/cgraph.c	2008-11-16 22:31:58.000000000 +0000
+++ gcc/cgraph.c	2011-03-21 18:37:28.108025146 +0000
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
--- gcc.orig/config/i386/i386.c	2010-08-06 08:52:04.000000000 +0100
+++ gcc/config/i386/i386.c	2011-03-27 19:42:34.441937542 +0100
@@ -4270,6 +4270,10 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -4288,6 +4292,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -4301,6 +4309,27 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -4537,6 +4566,12 @@ ix86_return_pops_args (tree fundecl, tre
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
@@ -4782,6 +4817,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
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
@@ -7879,6 +7919,11 @@ ix86_compute_frame_layout (struct ix86_f
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
@@ -26795,7 +26840,7 @@ x86_output_mi_thunk (FILE *file ATTRIBUT
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -29551,6 +29596,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2009-11-13 19:13:16.000000000 +0000
+++ gcc/config/i386/i386.h	2011-03-24 09:11:22.018372742 +0000
@@ -1572,6 +1572,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_avx;			/* True when we want to warn about AVX ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2010-06-04 05:57:21.000000000 +0100
+++ gcc/config/rs6000/rs6000.c	2011-03-21 18:37:28.292026049 +0000
@@ -17665,7 +17665,8 @@ rs6000_output_function_epilogue (FILE *f
 	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
 	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
 	 a number, so for now use 9.  */
-      if (! strcmp (language_string, "GNU C"))
+      if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc.orig/dojump.c	2010-03-08 11:46:28.000000000 +0000
+++ gcc/dojump.c	2011-03-21 18:37:28.304026114 +0000
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
--- gcc.orig/dwarf2out.c	2010-02-10 15:09:06.000000000 +0000
+++ gcc/dwarf2out.c	2011-03-21 18:37:28.356026367 +0000
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
--- gcc.orig/except.c	2010-03-08 11:46:28.000000000 +0000
+++ gcc/except.c	2011-03-21 18:37:28.368026429 +0000
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
--- gcc.orig/expr.c	2010-09-23 08:41:30.000000000 +0100
+++ gcc/expr.c	2011-03-21 18:37:28.396026575 +0000
@@ -9351,6 +9351,11 @@ expand_expr_real_1 (tree exp, rtx target
 	 represent.  */
       return const0_rtx;
 
+    case STATIC_CHAIN_EXPR:
+    case STATIC_CHAIN_DECL:
+      /* Lowered by tree-nested.c */
+      gcc_unreachable ();
+
     case EXC_PTR_EXPR:
       return get_exception_pointer ();
 
--- gcc.orig/function.c	2010-08-16 21:24:54.000000000 +0100
+++ gcc/function.c	2011-03-21 18:37:28.412026653 +0000
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
 
@@ -4460,11 +4465,15 @@ expand_function_start (tree subr)
       tree parm = cfun->static_chain_decl;
       rtx local = gen_reg_rtx (Pmode);
 
-      set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
       SET_DECL_RTL (parm, local);
       mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
 
-      emit_move_insn (local, static_chain_incoming_rtx);
+      if (!cfun->custom_static_chain)
+	{
+	  set_decl_incoming_rtl (parm, static_chain_incoming_rtx, false);
+	  emit_move_insn (local, static_chain_incoming_rtx);
+	}
+      /* else, the static chain will be set in the main body */
     }
 
   /* If the function receives a non-local goto, then store the
@@ -4985,6 +4994,9 @@ thread_prologue_and_epilogue_insns (void
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+    return;
+
   rtl_profile_for_bb (ENTRY_BLOCK_PTR);
 #ifdef HAVE_prologue
   if (HAVE_prologue)
--- gcc.orig/function.h	2009-09-23 15:58:58.000000000 +0100
+++ gcc/function.h	2011-03-21 18:37:28.420026685 +0000
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
@@ -600,6 +600,14 @@ struct function GTY(())
      adjusts one of its arguments and forwards to another
      function.  */
   unsigned int is_thunk : 1;
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
--- gcc.orig/gcc.c	2010-01-09 00:05:06.000000000 +0000
+++ gcc/gcc.c	2011-03-21 18:37:28.448026828 +0000
@@ -126,6 +126,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -480,6 +483,7 @@ or with constant text in a single argume
 	assembler has done its job.
  %D	Dump out a -L option for each directory in startfile_prefixes.
 	If multilib_dir is set, extra entries are generated with it affixed.
+ %N     Output the currently selected multilib directory name
  %l     process LINK_SPEC as a spec.
  %L     process LIB_SPEC as a spec.
  %G     process LIBGCC_SPEC as a spec.
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
@@ -5337,6 +5356,17 @@ do_spec_1 (const char *spec, int inswitc
 	      return value;
 	    break;
 
+	  case 'N':
+	    if (multilib_dir)
+	      {
+		arg_going = 1;
+		obstack_grow (&obstack, "-fmultilib-dir=",
+			      strlen ("-fmultilib-dir="));
+		obstack_grow (&obstack, multilib_dir,
+			      strlen (multilib_dir));
+	      }
+	    break;
+
 	    /* Here we define characters other than letters and digits.  */
 
 	  case '{':
--- gcc.orig/gimple.c	2008-12-07 23:27:14.000000000 +0000
+++ gcc/gimple.c	2011-03-21 18:37:28.480026985 +0000
@@ -2535,6 +2535,9 @@ get_gimple_rhs_num_ops (enum tree_code c
       || (SYM) == POLYNOMIAL_CHREC					    \
       || (SYM) == DOT_PROD_EXPR						    \
       || (SYM) == VEC_COND_EXPR						    \
+      || (SYM) == STATIC_CHAIN_DECL					    \
+      || (SYM) == STATIC_CHAIN_EXPR /* not sure if this is right...*/	    \
+      || (SYM) == VEC_COND_EXPR						    \
       || (SYM) == REALIGN_LOAD_EXPR) ? GIMPLE_SINGLE_RHS		    \
    : GIMPLE_INVALID_RHS),
 #define END_OF_BASE_TREE_CODES (unsigned char) GIMPLE_INVALID_RHS,
@@ -2615,7 +2618,10 @@ is_gimple_lvalue (tree t)
 	  || TREE_CODE (t) == WITH_SIZE_EXPR
 	  /* These are complex lvalues, but don't have addresses, so they
 	     go here.  */
-	  || TREE_CODE (t) == BIT_FIELD_REF);
+	  || TREE_CODE (t) == BIT_FIELD_REF
+	  /* This is an lvalue because it will be replaced with the real
+	     static chain decl. */
+	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
 }
 
 /*  Return true if T is a GIMPLE condition.  */
@@ -2851,6 +2857,7 @@ is_gimple_variable (tree t)
   return (TREE_CODE (t) == VAR_DECL
 	  || TREE_CODE (t) == PARM_DECL
 	  || TREE_CODE (t) == RESULT_DECL
+	  || TREE_CODE (t) == STATIC_CHAIN_DECL
 	  || TREE_CODE (t) == SSA_NAME);
 }
 
--- gcc.orig/gimple.h	2009-05-18 11:13:43.000000000 +0100
+++ gcc/gimple.h	2011-03-21 18:37:28.508027129 +0000
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
 
 
--- gcc.orig/ipa-cp.c	2009-12-27 22:39:58.000000000 +0000
+++ gcc/ipa-cp.c	2011-03-21 18:37:28.528027223 +0000
@@ -1393,7 +1393,7 @@ cgraph_gate_cp (void)
   return flag_ipa_cp;
 }
 
-struct ipa_opt_pass pass_ipa_cp = 
+struct ipa_opt_pass_d pass_ipa_cp = 
 {
  {
   IPA_PASS,
--- gcc.orig/ipa-inline.c	2010-02-08 14:10:15.000000000 +0000
+++ gcc/ipa-inline.c	2011-03-21 18:37:28.556027367 +0000
@@ -1753,7 +1753,7 @@ inline_transform (struct cgraph_node *no
   return todo | execute_fixup_cfg ();
 }
 
-struct ipa_opt_pass pass_ipa_inline = 
+struct ipa_opt_pass_d pass_ipa_inline = 
 {
  {
   IPA_PASS,
--- gcc.orig/ipa-pure-const.c	2008-09-18 18:28:40.000000000 +0100
+++ gcc/ipa-pure-const.c	2011-03-21 18:37:28.556027367 +0000
@@ -910,7 +910,7 @@ gate_pure_const (void)
 	  && !(errorcount || sorrycount));
 }
 
-struct ipa_opt_pass pass_ipa_pure_const =
+struct ipa_opt_pass_d pass_ipa_pure_const =
 {
  {
   IPA_PASS,
--- gcc.orig/ipa-reference.c	2008-09-18 18:28:40.000000000 +0100
+++ gcc/ipa-reference.c	2011-03-21 18:37:28.560027381 +0000
@@ -1257,7 +1257,7 @@ gate_reference (void)
 	  && !(errorcount || sorrycount));
 }
 
-struct ipa_opt_pass pass_ipa_reference =
+struct ipa_opt_pass_d pass_ipa_reference =
 {
  {
   IPA_PASS,
--- gcc.orig/ira.c	2010-09-09 14:58:24.000000000 +0100
+++ gcc/ira.c	2011-03-21 18:37:28.572027448 +0000
@@ -1404,7 +1404,7 @@ setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        || crtl->accesses_prior_frames
        || crtl->stack_realign_needed
--- gcc.orig/passes.c	2009-02-20 15:20:38.000000000 +0000
+++ gcc/passes.c	2011-03-21 18:37:28.588027527 +0000
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
--- gcc.orig/system.h	2008-07-28 15:33:56.000000000 +0100
+++ gcc/system.h	2011-03-21 18:37:28.596027557 +0000
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
--- gcc.orig/tree.def	2009-02-20 15:20:38.000000000 +0000
+++ gcc/tree.def	2011-03-21 18:37:28.624027699 +0000
@@ -554,6 +554,13 @@ DEFTREECODE (BIND_EXPR, "bind_expr", tcc
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
--- gcc.orig/tree-inline.c	2010-09-18 18:23:20.000000000 +0100
+++ gcc/tree-inline.c	2011-03-21 18:37:28.648027815 +0000
@@ -1832,6 +1832,7 @@ initialize_cfun (tree new_fndecl, tree c
 
   /* Copy items we preserve during clonning.  */
   cfun->static_chain_decl = src_cfun->static_chain_decl;
+  cfun->custom_static_chain = src_cfun->custom_static_chain;
   cfun->nonlocal_goto_save_area = src_cfun->nonlocal_goto_save_area;
   cfun->function_end_locus = src_cfun->function_end_locus;
   cfun->curr_properties = src_cfun->curr_properties;
@@ -2206,7 +2207,8 @@ initialize_inlined_parameters (copy_body
   /* Initialize the static chain.  */
   p = DECL_STRUCT_FUNCTION (fn)->static_chain_decl;
   gcc_assert (fn != current_function_decl);
-  if (p)
+  /* Custom static chain has already been dealt with.  */
+  if (p && ! DECL_STRUCT_FUNCTION(fn)->custom_static_chain)
     {
       /* No static chain?  Seems like a bug in tree-nested.c.  */
       gcc_assert (static_chain);
--- gcc.orig/tree-nested.c	2010-04-20 09:37:12.000000000 +0100
+++ gcc/tree-nested.c	2011-07-09 18:27:52.654604473 +0100
@@ -714,6 +714,8 @@ get_static_chain (struct nesting_info *i
 
   if (info->context == target_context)
     {
+      /* might be doing something wrong to need the following line.. */
+      get_frame_type (info);
       x = build_addr (info->frame_decl, target_context);
     }
   else
@@ -1908,6 +1910,25 @@ convert_gimple_call (gimple_stmt_iterato
       walk_body (convert_gimple_call, NULL, info, gimple_omp_body (stmt));
       break;
 
+    case GIMPLE_ASSIGN:
+      /* Deal with static_chain_decl */
+      decl = gimple_op(stmt, 0);
+      if (TREE_CODE (decl) == STATIC_CHAIN_DECL)
+	{
+	  tree chain = get_chain_decl (info);
+	  gimple_assign_set_lhs (stmt, chain);
+	  break;
+	}
+      /* Deal with static_chain_expr */
+      decl = gimple_op(stmt, 1);
+      if (TREE_CODE (decl) == STATIC_CHAIN_EXPR)
+	{
+	  tree chain = get_static_chain (info, TREE_OPERAND (decl, 0), &wi->gsi);
+	  gimple_assign_set_rhs1 (stmt, chain);
+	  break;
+	}
+      /* FALLTHRU */
+
     default:
       /* Keep looking for other operands.  */
       *handled_ops_p = false;
@@ -2054,8 +2075,38 @@ finalize_nesting_tree_1 (struct nesting_
       gimple bind;
       annotate_all_with_location (stmt_list, DECL_SOURCE_LOCATION (context));
       bind = gimple_seq_first_stmt (gimple_body (context));
-      gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
-      gimple_bind_set_body (bind, stmt_list);
+
+      if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
+	{
+	  /* This chicanery is needed to make sure the GIMPLE initialization
+	     statements are in the right order
+
+	     CHAIN.3 = D.1614;
+	     FRAME.0.__chain = CHAIN.3;
+	     FRAME.0.m = m;
+	   */
+	  gimple_stmt_iterator gsi =
+	      gsi_start (gimple_bind_body (bind));
+	  int found = 0;
+	  while (!gsi_end_p (gsi))
+	    {
+	      gimple g = gsi_stmt (gsi);
+	      if (gimple_code (g) == GIMPLE_ASSIGN &&
+		  gimple_op (g, 0) == root->chain_decl)
+		{
+		  gsi_insert_seq_after (& gsi, stmt_list, GSI_SAME_STMT);
+		  found = 1;
+		  break;
+		}
+	      gsi_next (& gsi);
+	    }
+	  gcc_assert (found);
+	}
+      else
+	{
+	  gimple_seq_add_seq (&stmt_list, gimple_bind_body (bind));
+	  gimple_bind_set_body (bind, stmt_list);
+	}
     }
 
   /* If a chain_decl was created, then it needs to be registered with
--- gcc.orig/tree-pass.h	2009-02-20 15:20:38.000000000 +0000
+++ gcc/tree-pass.h	2011-03-21 18:37:28.660027875 +0000
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
--- gcc.orig/tree-pretty-print.c	2009-02-18 21:03:05.000000000 +0000
+++ gcc/tree-pretty-print.c	2011-03-21 18:37:28.680027975 +0000
@@ -1216,6 +1216,16 @@ dump_generic_node (pretty_printer *buffe
 	pp_string (buffer, " [tail call]");
       break;
 
+    case STATIC_CHAIN_EXPR:
+      pp_string (buffer, "<<static chain of ");
+      dump_generic_node (buffer, TREE_OPERAND (node, 0), spc, flags, false);
+      pp_string (buffer, ">>");
+      break;
+
+    case STATIC_CHAIN_DECL:
+      pp_string (buffer, "<<static chain decl>>");
+      break;
+
     case WITH_CLEANUP_EXPR:
       NIY;
       break;
--- gcc.orig/tree-sra.c	2010-04-18 16:56:32.000000000 +0100
+++ gcc/tree-sra.c	2011-03-21 18:37:28.692028033 +0000
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
 
--- gcc.orig/tree-ssa.c	2009-08-03 20:27:32.000000000 +0100
+++ gcc/tree-ssa.c	2011-03-21 18:37:28.700028081 +0000
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
