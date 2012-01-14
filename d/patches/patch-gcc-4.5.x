--- gcc~/config/i386/i386.c	2011-02-17 21:22:02.000000000 +0000
+++ gcc/config/i386/i386.c	2012-01-14 19:15:48.215348464 +0000
@@ -4718,6 +4718,17 @@ ix86_return_pops_args (tree fundecl, tre
 
   return 0;
 }
+
+static int
+ix86_function_naked (const_tree fndecl)
+{
+  gcc_assert(fndecl != NULL_TREE);
+
+  if (lookup_attribute ("naked", DECL_ATTRIBUTES (fndecl)))
+    return 1;
+
+  return 0;
+}
 
 /* Argument support functions.  */
 
@@ -4903,6 +4914,10 @@ init_cumulative_args (CUMULATIVE_ARGS *c
    cum->call_abi = ix86_function_abi (fndecl);
   else
    cum->call_abi = ix86_function_type_abi (fntype);
+
+  if (fndecl && ix86_function_naked (fndecl))
+    cfun->machine->naked = true;
+
   /* Set up the number of registers to use for passing arguments.  */
 
   if (cum->call_abi == MS_ABI && !ACCUMULATE_OUTGOING_ARGS)
@@ -7529,6 +7544,9 @@ ix86_can_use_return_insn_p (void)
   if (! reload_completed || frame_pointer_needed)
     return 0;
 
+  if (ix86_current_function_naked)
+    return 0;
+
   /* Don't allow more than 32 pop, since that's all we can do
      with one instruction.  */
   if (crtl->args.pops_args
@@ -7547,6 +7565,9 @@ ix86_can_use_return_insn_p (void)
 static bool
 ix86_frame_pointer_required (void)
 {
+  if (ix86_current_function_naked)
+    return false;
+
   /* If we accessed previous frames, then the generated code expects
      to be able to access the saved ebp value in our frame.  */
   if (cfun->machine->accesses_prev_frame)
@@ -8131,6 +8152,12 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (ix86_current_function_naked)
+    {
+      /* As above, skip return address */
+      frame->stack_pointer_offset = UNITS_PER_WORD;
+    }
 }
 
 /* Emit code to save registers in the prologue.  */
@@ -8475,6 +8502,9 @@ ix86_expand_prologue (void)
   HOST_WIDE_INT allocate;
   int gen_frame_pointer = frame_pointer_needed;
 
+  if (ix86_current_function_naked)
+    return;
+
   ix86_finalize_stack_realign_flags ();
 
   /* DRAP should not coexist with stack_realign_fp */
@@ -9006,6 +9036,9 @@ ix86_expand_epilogue (int style)
   struct machine_cfa_state cfa_state_save = *ix86_cfa_state;
   bool using_drap;
 
+  if (ix86_current_function_naked)
+    return;
+
   ix86_finalize_stack_realign_flags ();
 
  /* When stack is realigned, SP must be valid.  */
@@ -29058,6 +29091,8 @@ static const struct attribute_spec ix86_
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
+  /* naked attribute says this function has no prologue or epilogue.  */
+  { "naked",     0, 0, true, false, false, ix86_handle_fndecl_attribute },
 #if TARGET_DLLIMPORT_DECL_ATTRIBUTES
   { "dllimport", 0, 0, false, false, false, handle_dll_attribute },
   { "dllexport", 0, 0, false, false, false, handle_dll_attribute },
--- gcc~/config/i386/i386.h	2011-03-30 11:48:07.000000000 +0100
+++ gcc/config/i386/i386.h	2012-01-14 19:06:51.103368063 +0000
@@ -2379,6 +2379,9 @@ struct GTY(()) machine_function {
   /* Nonzero if the function requires a CLD in the prologue.  */
   BOOL_BITFIELD needs_cld : 1;
 
+  /* Nonzero if current function is a naked function.  */
+  BOOL_BITFIELD naked : 1;
+
   /* Set by ix86_compute_frame_layout and used by prologue/epilogue
      expander to determine the style used.  */
   BOOL_BITFIELD use_fast_prologue_epilogue : 1;
@@ -2405,6 +2408,7 @@ struct GTY(()) machine_function {
 #define ix86_varargs_fpr_size (cfun->machine->varargs_fpr_size)
 #define ix86_optimize_mode_switching (cfun->machine->optimize_mode_switching)
 #define ix86_current_function_needs_cld (cfun->machine->needs_cld)
+#define ix86_current_function_naked (cfun->machine->naked)
 #define ix86_tls_descriptor_calls_expanded_in_cfun \
   (cfun->machine->tls_descriptor_call_expanded_p)
 /* Since tls_descriptor_call_expanded is not cleared, even if all TLS
--- gcc~/config/rs6000/rs6000.c	2011-02-04 16:31:48.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2012-01-14 19:04:06.423374071 +0000
@@ -20422,6 +20422,7 @@ rs6000_output_function_epilogue (FILE *f
 	 a number, so for now use 9.  LTO isn't assigned a number either,
 	 so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D")
 	  || ! strcmp (language_string, "GNU GIMPLE"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
--- gcc~/dwarf2out.c	2011-04-18 18:38:13.000000000 +0100
+++ gcc/dwarf2out.c	2012-01-14 19:04:06.471374070 +0000
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
--- gcc~/gcc.c	2010-04-18 18:46:08.000000000 +0100
+++ gcc/gcc.c	2012-01-14 19:04:06.515374068 +0000
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
