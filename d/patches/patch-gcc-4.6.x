--- gcc.orig/cgraph.c	2011-03-04 18:49:23.000000000 +0000
+++ gcc/cgraph.c	2011-07-09 20:25:16.517533109 +0100
@@ -491,6 +491,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -512,11 +513,15 @@ cgraph_node (tree decl)
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
--- gcc.orig/config/i386/i386.c	2011-03-04 17:56:39.000000000 +0000
+++ gcc/config/i386/i386.c	2011-07-24 12:47:54.466177239 +0100
@@ -5358,6 +5358,10 @@ ix86_handle_cconv_attribute (tree *node,
 	{
 	  error ("fastcall and thiscall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("fastcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine stdcall with fastcall (redundant), regparm and
@@ -5376,6 +5380,10 @@ ix86_handle_cconv_attribute (tree *node,
 	{
 	  error ("stdcall and thiscall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -5393,6 +5401,10 @@ ix86_handle_cconv_attribute (tree *node,
 	{
 	  error ("cdecl and thiscall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("cdecl and optlink attributes are not compatible");
+	}
     }
   else if (is_attribute_p ("thiscall", name))
     {
@@ -5411,6 +5423,31 @@ ix86_handle_cconv_attribute (tree *node,
 	{
 	  error ("cdecl and thiscall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("optlink and thiscall attributes are not compatible");
+	}
+    }
+
+  /* Can combine optlink with regparm and sseregparm.  */
+  else if (is_attribute_p ("optlink", name))
+    {
+      if (lookup_attribute ("cdecl", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("optlink and cdecl attributes are not compatible");
+	}
+      if (lookup_attribute ("fastcall", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("optlink and fastcall attributes are not compatible");
+	}
+      if (lookup_attribute ("stdcall", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("optlink and stdcall attributes are not compatible");
+	}
+      if (lookup_attribute ("thiscall", TYPE_ATTRIBUTES (*node)))
+	{
+	  error ("optlink and thiscall attributes are not compatible");
+	}
     }
 
   /* Can combine sseregparm with all attributes.  */
@@ -5644,6 +5681,12 @@ ix86_return_pops_args (tree fundecl, tre
           || lookup_attribute ("thiscall", TYPE_ATTRIBUTES (funtype)))
 	rtd = 1;
 
+      /* Optlink functions will pop the stack if floating-point return
+         and if not variable args.  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+	  && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+	rtd = 1;
+
       if (rtd && ! stdarg_p (funtype))
 	return size;
     }
@@ -5991,6 +6034,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
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
@@ -8721,6 +8769,10 @@ ix86_frame_pointer_required (void)
   if (crtl->profile && !flag_fentry)
     return true;
 
+  /* Optlink mandates the setting up of ebp, unless 'naked' is used.  */
+  if (crtl->args.info.optlink && !cfun->naked)
+    return true;
+
   return false;
 }
 
@@ -9366,6 +9418,10 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->stack_pointer_offset -= frame->red_zone_size;
 
+  if (cfun->naked)
+    /* As above, skip return address.  */
+    frame->stack_pointer_offset = UNITS_PER_WORD;
+
   /* The SEH frame pointer location is near the bottom of the frame.
      This is enforced by the fact that the difference between the
      stack pointer and the frame pointer is limited to 240 bytes in
@@ -29731,7 +29787,7 @@ x86_output_mi_thunk (FILE *file,
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
@@ -32553,6 +32609,8 @@ static const struct attribute_spec ix86_
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
--- gcc.orig/config/i386/i386.h	2011-01-14 21:03:22.000000000 +0000
+++ gcc/config/i386/i386.h	2011-07-09 20:25:16.661533818 +0100
@@ -1498,6 +1498,7 @@ typedef struct ix86_args {
   int regno;			/* next available register number */
   int fastcall;			/* fastcall or thiscall calling convention
 				   is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_avx;			/* True when we want to warn about AVX ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2011-03-15 12:57:37.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2011-07-09 20:25:16.721534120 +0100
@@ -22039,6 +22039,7 @@ rs6000_output_function_epilogue (FILE *f
 	 a number, so for now use 9.  LTO isn't assigned a number either,
 	 so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
+	  || ! strcmp (language_string, "GNU D")
 	  || ! strcmp (language_string, "GNU GIMPLE"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
--- gcc.orig/dojump.c	2010-05-19 21:09:57.000000000 +0100
+++ gcc/dojump.c	2011-07-12 23:03:55.909624421 +0100
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
--- gcc.orig/dwarf2out.c	2011-03-18 16:22:01.000000000 +0000
+++ gcc/dwarf2out.c	2011-07-09 20:25:16.781534412 +0100
@@ -20069,6 +20069,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
@@ -21464,7 +21466,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/function.c	2011-03-09 20:49:00.000000000 +0000
+++ gcc/function.c	2011-07-10 18:54:33.562977424 +0100
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
--- gcc.orig/function.h	2011-01-03 20:52:22.000000000 +0000
+++ gcc/function.h	2011-07-12 23:04:20.197744890 +0100
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
--- gcc.orig/gcc.c	2011-02-23 02:04:43.000000000 +0000
+++ gcc/gcc.c	2011-08-01 20:19:35.899861308 +0100
@@ -83,6 +83,9 @@ int is_cpp_driver;
 /* Flag set to nonzero if an @file argument has been supplied to gcc.  */
 static bool at_file_supplied;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Definition of string containing the arguments given to configure.  */
 #include "configargs.h"
 
@@ -3925,6 +3929,18 @@ process_command (unsigned int decoded_op
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
--- gcc.orig/ira.c	2011-03-08 15:51:12.000000000 +0000
+++ gcc/ira.c	2011-07-12 23:04:12.433706377 +0100
@@ -1341,7 +1341,7 @@ ira_setup_eliminable_regset (void)
      case.  At some point, we should improve this by emitting the
      sp-adjusting insns for this case.  */
   int need_fp
-    = (! flag_omit_frame_pointer
+    = ((! flag_omit_frame_pointer && ! cfun->naked)
        || (cfun->calls_alloca && EXIT_IGNORE_STACK)
        /* We need the frame pointer to catch stack overflow exceptions
 	  if the stack pointer is moving.  */
--- gcc.orig/tree-sra.c	2011-02-17 16:18:24.000000000 +0000
+++ gcc/tree-sra.c	2011-07-09 20:25:16.941535211 +0100
@@ -1533,6 +1533,8 @@ is_va_list_type (tree type)
 /* The very first phase of intraprocedural SRA.  It marks in candidate_bitmap
    those with type which is suitable for scalarization.  */
 
+/* FIXME: Should we do something here for GDC? */
+
 static bool
 find_var_candidates (void)
 {
