--- gcc.orig/cgraph.c	2010-07-01 12:03:31.000000000 +0100
+++ gcc/cgraph.c	2011-03-22 00:31:28.341349775 +0000
@@ -464,6 +464,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -485,11 +486,15 @@ cgraph_node (tree decl)
   node = cgraph_create_node ();
   node->decl = decl;
   *slot = node;
-  if (DECL_CONTEXT (decl) && TREE_CODE (DECL_CONTEXT (decl)) == FUNCTION_DECL)
+  if (DECL_STATIC_CHAIN (decl))
     {
-      node->origin = cgraph_node (DECL_CONTEXT (decl));
-      node->next_nested = node->origin->nested;
-      node->origin->nested = node;
+      context = decl_function_context (decl);
+      if (context)
+	{
+	  node->origin = cgraph_node (context);
+	  node->next_nested = node->origin->nested;
+	  node->origin->nested = node;
+	}
     }
   if (assembler_name_hash)
     {
--- gcc.orig/config/i386/i386.c	2010-09-30 21:24:54.000000000 +0100
+++ gcc/config/i386/i386.c	2011-07-24 12:50:17.370885866 +0100
@@ -4466,6 +4466,10 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -4484,6 +4488,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -4497,6 +4505,27 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -4703,6 +4732,12 @@ ix86_return_pops_args (tree fundecl, tre
           || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
 	rtd = 1;
 
+      /* Optlink functions will pop the stack if floating-point return
+         and if not variable args.  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+          && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+	rtd = 1;
+
       if (rtd && ! stdarg_p (funtype))
 	return size;
     }
@@ -4969,6 +5004,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
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
@@ -7529,6 +7569,9 @@ ix86_can_use_return_insn_p (void)
   if (! reload_completed || frame_pointer_needed)
     return 0;
 
+  if (cfun->naked)
+    return 0;
+
   /* Don't allow more than 32 pop, since that's all we can do
      with one instruction.  */
   if (crtl->args.pops_args
@@ -7568,6 +7611,10 @@ ix86_frame_pointer_required (void)
   if (crtl->profile)
     return true;
 
+  /* Optink mandates the setting up of ebp, unless 'naked' is used.  */
+  if (crtl->args.info.optlink && !cfun->naked)
+    return true;
+
   return false;
 }
 
@@ -8131,6 +8178,10 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+    /* As above, skip return address */
+    frame->stack_pointer_offset = UNITS_PER_WORD;
 }
 
 /* Emit code to save registers in the prologue.  */
@@ -26366,7 +26417,7 @@ x86_output_mi_thunk (FILE *file,
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -28996,6 +29047,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2010-04-27 21:14:19.000000000 +0100
+++ gcc/config/i386/i386.h	2011-04-25 00:46:47.553601354 +0100
@@ -1581,6 +1581,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_avx;			/* True when we want to warn about AVX ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2010-11-17 06:09:53.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2011-03-22 00:31:28.417350149 +0000
@@ -20365,6 +20365,7 @@ rs6000_output_function_epilogue (FILE *f
 	 a number, so for now use 9.  LTO isn't assigned a number either,
 	 so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D")
 	  || ! strcmp (language_string, "GNU GIMPLE"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
--- gcc.orig/dojump.c	2010-02-19 18:19:06.000000000 +0000
+++ gcc/dojump.c	2011-04-25 02:24:02.758536569 +0100
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
--- gcc.orig/dwarf2out.c	2010-12-07 15:12:45.000000000 +0000
+++ gcc/dwarf2out.c	2011-03-22 00:31:28.509350608 +0000
@@ -18729,6 +18729,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
@@ -19967,7 +19969,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/function.c	2010-08-16 21:18:08.000000000 +0100
+++ gcc/function.c	2011-07-24 12:29:15.576628946 +0100
@@ -3202,7 +3202,8 @@ assign_parms (tree fndecl)
       FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
 			    data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3219,7 +3220,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.first_conversion_insn);
+  if (!cfun->naked)
+    emit_insn (all.first_conversion_insn);
 
   /* Estimate reload stack alignment from scalar return mode.  */
   if (SUPPORTS_STACK_ALIGNMENT)
@@ -3373,6 +3375,9 @@ gimplify_parameters (void)
   VEC(tree, heap) *fnargs;
   unsigned i;
 
+  if (cfun->naked)
+    return NULL;
+
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -4475,7 +4480,7 @@ expand_function_start (tree subr)
       set_decl_incoming_rtl (parm, chain, false);
       SET_DECL_RTL (parm, local);
       mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
-
+      
       insn = emit_move_insn (local, chain);
 
       /* Mark the register as eliminable, similar to parameters.  */
@@ -5015,6 +5020,9 @@ thread_prologue_and_epilogue_insns (void
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+    return;
+
   rtl_profile_for_bb (ENTRY_BLOCK_PTR);
 #ifdef HAVE_prologue
   if (HAVE_prologue)
--- gcc.orig/function.h	2009-11-25 10:55:54.000000000 +0000
+++ gcc/function.h	2011-07-24 12:29:23.188666683 +0100
@@ -596,6 +596,10 @@ struct GTY(()) function {
      adjusts one of its arguments and forwards to another
      function.  */
   unsigned int is_thunk : 1;
+
+   /* Nonzero if no code should be generated for prologues, copying
+      parameters, etc. */
+  unsigned int naked : 1;
 };
 
 /* If va_list_[gf]pr_size is set to this, it means we don't know how
--- gcc.orig/gcc.c	2010-04-18 18:46:08.000000000 +0100
+++ gcc/gcc.c	2011-03-24 22:54:48.766216607 +0000
@@ -139,6 +139,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -4282,6 +4286,9 @@ process_command (int argc, const char **
       save_temps_prefix = NULL;
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4646,6 +4653,18 @@ process_command (int argc, const char **
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
--- gcc.orig/ira.c	2010-09-09 14:55:35.000000000 +0100
+++ gcc/ira.c	2011-04-25 02:26:12.763181215 +0100
@@ -1440,7 +1440,7 @@ ira_setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        /* We need the frame pointer to catch stack overflow exceptions
 	  if the stack pointer is moving.  */
--- gcc.orig/tree-sra.c	2010-08-03 10:52:46.000000000 +0100
+++ gcc/tree-sra.c	2011-03-22 00:31:28.597351039 +0000
@@ -1505,6 +1505,8 @@ is_va_list_type (tree type)
 /* The very first phase of intraprocedural SRA.  It marks in candidate_bitmap
    those with type which is suitable for scalarization.  */
 
+/* FIXME: Should we do something here for GDC? */
+
 static bool
 find_var_candidates (void)
 {
