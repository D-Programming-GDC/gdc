diff -pur gcc~/config/i386/i386.c gcc/config/i386/i386.c
--- gcc~/config/i386/i386.c	2011-09-22 18:41:25.000000000 +0100
+++ gcc/config/i386/i386.c	2012-01-10 22:10:15.107390593 +0000
@@ -9397,6 +9397,10 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->stack_pointer_offset -= frame->red_zone_size;
 
+  if (cfun->naked)
+    /* As above, skip return address.  */
+    frame->stack_pointer_offset = UNITS_PER_WORD;
+
   /* The SEH frame pointer location is near the bottom of the frame.
      This is enforced by the fact that the difference between the
      stack pointer and the frame pointer is limited to 240 bytes in
diff -pur gcc~/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
--- gcc~/config/rs6000/rs6000.c	2011-09-18 23:01:56.000000000 +0100
+++ gcc/config/rs6000/rs6000.c	2012-01-10 21:58:14.499416886 +0000
@@ -22062,6 +22062,7 @@ rs6000_output_function_epilogue (FILE *f
 	 a number, so for now use 9.  LTO isn't assigned a number either,
 	 so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D")
 	  || ! strcmp (language_string, "GNU GIMPLE"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
diff -pur gcc~/dojump.c gcc/dojump.c
--- gcc~/dojump.c	2010-05-19 21:09:57.000000000 +0100
+++ gcc/dojump.c	2012-01-10 21:58:14.503416886 +0000
@@ -80,7 +80,8 @@ void
 clear_pending_stack_adjust (void)
 {
   if (optimize > 0
-      && (! flag_omit_frame_pointer || cfun->calls_alloca)
+      && ((! flag_omit_frame_pointer && ! cfun->naked)
+	  || cfun->calls_alloca)
       && EXIT_IGNORE_STACK)
     discard_pending_stack_adjust ();
 }
diff -pur gcc~/dwarf2out.c gcc/dwarf2out.c
--- gcc~/dwarf2out.c	2011-10-24 19:09:21.000000000 +0100
+++ gcc/dwarf2out.c	2012-01-10 21:58:14.519416886 +0000
@@ -20078,6 +20078,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
@@ -21473,7 +21475,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
diff -pur gcc~/function.c gcc/function.c
--- gcc~/function.c	2011-03-09 20:49:00.000000000 +0000
+++ gcc/function.c	2012-01-10 21:58:14.523416886 +0000
@@ -3409,7 +3409,8 @@ assign_parms (tree fndecl)
       targetm.calls.function_arg_advance (&all.args_so_far, data.promoted_mode,
 					  data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3426,7 +3427,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.first_conversion_insn);
+  if (!cfun->naked)
+    emit_insn (all.first_conversion_insn);
 
   /* Estimate reload stack alignment from scalar return mode.  */
   if (SUPPORTS_STACK_ALIGNMENT)
@@ -3590,6 +3592,9 @@ gimplify_parameters (void)
   VEC(tree, heap) *fnargs;
   unsigned i;
 
+  if (cfun->naked)
+    return NULL;
+
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -5287,6 +5292,9 @@ thread_prologue_and_epilogue_insns (void
   edge e;
   edge_iterator ei;
 
+  if (cfun->naked)
+    return;
+
   rtl_profile_for_bb (ENTRY_BLOCK_PTR);
 
   inserted = false;
diff -pur gcc~/function.h gcc/function.h
--- gcc~/function.h	2011-01-03 20:52:22.000000000 +0000
+++ gcc/function.h	2012-01-10 21:58:14.527416885 +0000
@@ -636,6 +636,10 @@ struct GTY(()) function {
      adjusts one of its arguments and forwards to another
      function.  */
   unsigned int is_thunk : 1;
+
+  /* Nonzero if no code should be generated for prologues, copying
+     parameters, etc. */
+  unsigned int naked : 1;
 };
 
 /* Add the decl D to the local_decls list of FUN.  */
diff -pur gcc~/gcc.c gcc/gcc.c
--- gcc~/gcc.c	2011-02-23 02:04:43.000000000 +0000
+++ gcc/gcc.c	2012-01-10 21:58:14.531416885 +0000
@@ -83,6 +83,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Definition of string containing the arguments given to configure.  */
 #include "configargs.h"
 
@@ -3925,6 +3928,18 @@ process_command (unsigned int decoded_op
       add_infile ("help-dummy", "c");
     }
 
+  if (need_pthreads)
+    {
+      alloc_switch ();
+      switches[n_switches].part1 = "pthread";
+      switches[n_switches].args = 0;
+      switches[n_switches].live_cond = 0;
+      /* Do not print an error if there is not expansion for -pthread. */
+      switches[n_switches].validated = 1;
+      switches[n_switches].ordering = 0;
+      n_switches++;
+    }
+
   alloc_switch ();
   switches[n_switches].part1 = 0;
   alloc_infile ();
diff -pur gcc~/ira.c gcc/ira.c
--- gcc~/ira.c	2011-03-08 15:51:12.000000000 +0000
+++ gcc/ira.c	2012-01-10 21:58:14.531416885 +0000
@@ -1341,7 +1341,7 @@ ira_setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        /* We need the frame pointer to catch stack overflow exceptions
 	  if the stack pointer is moving.  */
