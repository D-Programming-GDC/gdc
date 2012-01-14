--- gcc~/config/i386/i386.c	2011-02-17 21:22:02.000000000 +0000
+++ gcc/config/i386/i386.c	2012-01-11 01:05:02.707007920 +0000
@@ -7529,6 +7529,9 @@ ix86_can_use_return_insn_p (void)
   if (! reload_completed || frame_pointer_needed)
     return 0;
 
+  if (cfun->naked)
+    return 0;
+
   /* Don't allow more than 32 pop, since that's all we can do
      with one instruction.  */
   if (crtl->args.pops_args
@@ -8131,6 +8134,10 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+    /* As above, skip return address */
+    frame->stack_pointer_offset = UNITS_PER_WORD;
 }
 
 /* Emit code to save registers in the prologue.  */
--- gcc~/config/rs6000/rs6000.c	2011-02-04 16:31:48.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2012-01-11 01:05:02.723007919 +0000
@@ -20422,6 +20422,7 @@ rs6000_output_function_epilogue (FILE *f
 	 a number, so for now use 9.  LTO isn't assigned a number either,
 	 so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D")
 	  || ! strcmp (language_string, "GNU GIMPLE"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
--- gcc~/dojump.c	2010-02-19 18:19:06.000000000 +0000
+++ gcc/dojump.c	2012-01-11 01:05:02.723007919 +0000
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
--- gcc~/dwarf2out.c	2011-04-18 18:38:13.000000000 +0100
+++ gcc/dwarf2out.c	2012-01-11 01:05:02.735007919 +0000
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
--- gcc~/function.c	2010-08-16 21:18:08.000000000 +0100
+++ gcc/function.c	2012-01-11 01:05:02.739007919 +0000
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
--- gcc~/function.h	2009-11-25 10:55:54.000000000 +0000
+++ gcc/function.h	2012-01-11 01:05:02.739007919 +0000
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
--- gcc~/gcc.c	2010-04-18 18:46:08.000000000 +0100
+++ gcc/gcc.c	2012-01-11 01:05:02.743007919 +0000
@@ -139,6 +139,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -4282,6 +4285,9 @@ process_command (int argc, const char **
       save_temps_prefix = NULL;
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4646,6 +4652,18 @@ process_command (int argc, const char **
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
--- gcc~/ira.c	2010-09-09 14:55:35.000000000 +0100
+++ gcc/ira.c	2012-01-11 01:05:02.747007919 +0000
@@ -1440,7 +1440,7 @@ ira_setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        /* We need the frame pointer to catch stack overflow exceptions
 	  if the stack pointer is moving.  */
