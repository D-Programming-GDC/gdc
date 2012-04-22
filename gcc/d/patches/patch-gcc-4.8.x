--- gcc~/config/i386/i386.c	2012-03-31 22:03:36.000000000 +0100
+++ gcc/config/i386/i386.c	2012-04-08 19:44:40.864738490 +0100
@@ -5429,6 +5429,17 @@ ix86_return_pops_args (tree fundecl, tre
 
   return 0;
 }
+
+static int
+ix86_function_naked (const_tree fndecl)
+{
+  gcc_assert (fndecl != NULL_TREE);
+
+  if (lookup_attribute ("naked", DECL_ATTRIBUTES (fndecl)))
+    return 1;
+
+  return 0;
+}
 
 /* Argument support functions.  */
 
@@ -5688,6 +5699,9 @@ init_cumulative_args (CUMULATIVE_ARGS *c
 	}
     }
 
+  if (fndecl && ix86_function_naked (fndecl))
+    cfun->machine->naked = true;
+
   cum->caller = caller;
 
   /* Set up the number of registers to use for passing arguments.  */
@@ -8509,6 +8523,9 @@ ix86_can_use_return_insn_p (void)
   if (crtl->args.pops_args && crtl->args.size >= 32768)
     return 0;
 
+  if (ix86_current_function_naked)
+    return 0;
+
   ix86_compute_frame_layout (&frame);
   return (frame.stack_pointer_offset == UNITS_PER_WORD
 	  && (frame.nregs + frame.nsseregs) == 0);
@@ -8521,6 +8538,9 @@ ix86_can_use_return_insn_p (void)
 static bool
 ix86_frame_pointer_required (void)
 {
+  if (ix86_current_function_naked)
+    return false;
+
   /* If we accessed previous frames, then the generated code expects
      to be able to access the saved ebp value in our frame.  */
   if (cfun->machine->accesses_prev_frame)
@@ -9110,6 +9130,12 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->stack_pointer_offset -= frame->red_zone_size;
 
+  if (ix86_current_function_naked)
+    {
+      /* As above, skip return address.  */
+      frame->stack_pointer_offset = UNITS_PER_WORD;
+    }
+
   /* The SEH frame pointer location is near the bottom of the frame.
      This is enforced by the fact that the difference between the
      stack pointer and the frame pointer is limited to 240 bytes in
@@ -10117,6 +10143,9 @@ ix86_expand_prologue (void)
   HOST_WIDE_INT allocate;
   bool int_registers_saved;
 
+  if (ix86_current_function_naked)
+    return;
+
   ix86_finalize_stack_realign_flags ();
 
   /* DRAP should not coexist with stack_realign_fp */
@@ -10757,6 +10786,9 @@ ix86_expand_epilogue (int style)
   bool restore_regs_via_mov;
   bool using_drap;
 
+  if (ix86_current_function_naked)
+    return;
+
   ix86_finalize_stack_realign_flags ();
   ix86_compute_frame_layout (&frame);
 
@@ -35587,6 +35619,8 @@ static const struct attribute_spec ix86_
     false },
   { "callee_pop_aggregate_return", 1, 1, false, true, true,
     ix86_handle_callee_pop_aggregate_return, true },
+  /* naked attribute says this function has no prologue or epilogue.  */
+  { "naked",     0, 0, true, false, false, ix86_handle_fndecl_attribute },
   /* End element.  */
   { NULL,        0, 0, false, false, false, NULL, false }
 };
--- gcc~/config/i386/i386.h	2012-03-28 23:42:28.000000000 +0100
+++ gcc/config/i386/i386.h	2012-04-08 19:44:40.872738490 +0100
@@ -2226,6 +2226,9 @@ struct GTY(()) machine_function {
   /* Nonzero if the function requires a CLD in the prologue.  */
   BOOL_BITFIELD needs_cld : 1;
 
+  /* Nonzero if current function is a naked function.  */
+  BOOL_BITFIELD naked : 1;
+
   /* Set by ix86_compute_frame_layout and used by prologue/epilogue
      expander to determine the style used.  */
   BOOL_BITFIELD use_fast_prologue_epilogue : 1;
@@ -2274,6 +2277,7 @@ struct GTY(()) machine_function {
 #define ix86_varargs_fpr_size (cfun->machine->varargs_fpr_size)
 #define ix86_optimize_mode_switching (cfun->machine->optimize_mode_switching)
 #define ix86_current_function_needs_cld (cfun->machine->needs_cld)
+#define ix86_current_function_naked (cfun->machine->naked)
 #define ix86_tls_descriptor_calls_expanded_in_cfun \
   (cfun->machine->tls_descriptor_call_expanded_p)
 /* Since tls_descriptor_call_expanded is not cleared, even if all TLS
--- gcc~/config/rs6000/rs6000.c	2012-03-21 20:13:50.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2012-04-08 19:44:40.888738490 +0100
@@ -21032,7 +21032,8 @@ rs6000_output_function_epilogue (FILE *f
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+          || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc~/dwarf2out.c	2012-03-30 19:00:03.000000000 +0100
+++ gcc/dwarf2out.c	2012-04-08 19:44:40.900738490 +0100
@@ -18475,6 +18475,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
