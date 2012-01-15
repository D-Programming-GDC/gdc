--- gcc~/config/i386/i386.c	2011-09-22 18:41:25.000000000 +0100
+++ gcc/config/i386/i386.c	2012-01-14 18:08:10.823496512 +0000
@@ -5690,6 +5690,17 @@ ix86_return_pops_args (tree fundecl, tre
 
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
 
@@ -5949,6 +5960,9 @@ init_cumulative_args (CUMULATIVE_ARGS *c
 	}
     }
 
+  if (fndecl && ix86_function_naked (fndecl))
+    cfun->machine->naked = true;
+
   cum->caller = caller;
 
   /* Set up the number of registers to use for passing arguments.  */
@@ -8714,6 +8728,9 @@ ix86_can_use_return_insn_p (void)
   if (! reload_completed || frame_pointer_needed)
     return 0;
 
+  if (ix86_current_function_naked)
+    return 0;
+
   /* Don't allow more than 32k pop, since that's all we can do
      with one instruction.  */
   if (crtl->args.pops_args && crtl->args.size >= 32768)
@@ -8731,6 +8748,9 @@ ix86_can_use_return_insn_p (void)
 static bool
 ix86_frame_pointer_required (void)
 {
+  if (ix86_current_function_naked)
+    return false;
+
   /* If we accessed previous frames, then the generated code expects
      to be able to access the saved ebp value in our frame.  */
   if (cfun->machine->accesses_prev_frame)
@@ -9397,6 +9417,12 @@ ix86_compute_frame_layout (struct ix86_f
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
@@ -10348,6 +10374,9 @@ ix86_expand_prologue (void)
   HOST_WIDE_INT allocate;
   bool int_registers_saved;
 
+  if (ix86_current_function_naked)
+    return;
+
   ix86_finalize_stack_realign_flags ();
 
   /* DRAP should not coexist with stack_realign_fp */
@@ -10963,6 +10992,9 @@ ix86_expand_epilogue (int style)
   bool restore_regs_via_mov;
   bool using_drap;
 
+  if (ix86_current_function_naked)
+    return;
+
   ix86_finalize_stack_realign_flags ();
   ix86_compute_frame_layout (&frame);
 
@@ -32754,6 +32786,8 @@ static const struct attribute_spec ix86_
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
+  /* naked attribute says this function has no prologue or epilogue.  */
+  { "naked",     0, 0, true, false, false, ix86_handle_fndecl_attribute },
 #if TARGET_DLLIMPORT_DECL_ATTRIBUTES
   { "dllimport", 0, 0, false, false, false, handle_dll_attribute },
   { "dllexport", 0, 0, false, false, false, handle_dll_attribute },
--- gcc~/config/i386/i386.h	2011-06-29 21:15:32.000000000 +0100
+++ gcc/config/i386/i386.h	2012-01-14 17:24:44.435591614 +0000
@@ -2282,6 +2282,9 @@ struct GTY(()) machine_function {
   /* Nonzero if the function requires a CLD in the prologue.  */
   BOOL_BITFIELD needs_cld : 1;
 
+  /* Nonzero if current function is a naked function.  */
+  BOOL_BITFIELD naked : 1;
+
   /* Set by ix86_compute_frame_layout and used by prologue/epilogue
      expander to determine the style used.  */
   BOOL_BITFIELD use_fast_prologue_epilogue : 1;
@@ -2330,6 +2333,7 @@ struct GTY(()) machine_function {
 #define ix86_varargs_fpr_size (cfun->machine->varargs_fpr_size)
 #define ix86_optimize_mode_switching (cfun->machine->optimize_mode_switching)
 #define ix86_current_function_needs_cld (cfun->machine->needs_cld)
+#define ix86_current_function_naked (cfun->machine->naked)
 #define ix86_tls_descriptor_calls_expanded_in_cfun \
   (cfun->machine->tls_descriptor_call_expanded_p)
 /* Since tls_descriptor_call_expanded is not cleared, even if all TLS
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
