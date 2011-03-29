--- gcc.orig/cgraph.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/cgraph.c	2011-03-21 18:37:14.047955433 +0000
@@ -198,6 +198,7 @@ struct cgraph_node *
 cgraph_node (tree decl)
 {
   struct cgraph_node key, *node, **slot;
+  tree context;
 
   gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);
 
@@ -219,12 +220,16 @@ cgraph_node (tree decl)
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
--- gcc.orig/config/arm/arm.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/config/arm/arm.c	2011-03-21 18:37:14.095955665 +0000
@@ -15485,6 +15485,15 @@ arm_unwind_emit_set (FILE * asm_out_file
 	  asm_fprintf (asm_out_file, "\t.movsp %r, #%d\n",
 		       REGNO (e0), (int)INTVAL(XEXP (e1, 1)));
 	}
+      else if (GET_CODE (e1) == PLUS
+	      && GET_CODE (XEXP (e1, 0)) == REG
+	      && REGNO (XEXP (e1, 0)) == SP_REGNUM
+	      && GET_CODE (XEXP (e1, 1)) == CONST_INT)
+	{
+	  /* Set reg to offset from sp.  */
+	  asm_fprintf (asm_out_file, "\t.movsp %r, #%d\n",
+		       REGNO (e0), (int)INTVAL(XEXP (e1, 1)));
+	}
       else
 	abort ();
       break;
--- gcc.orig/config/darwin.h	2007-09-01 16:28:30.000000000 +0100
+++ gcc/config/darwin.h	2011-03-21 18:37:14.155955946 +0000
@@ -759,7 +759,7 @@ enum machopic_addr_class {
 /* Macros defining the various PIC cases.  */
 
 #define MACHO_DYNAMIC_NO_PIC_P	(TARGET_DYNAMIC_NO_PIC)
-#define MACHOPIC_INDIRECT	(flag_pic || MACHO_DYNAMIC_NO_PIC_P)
+#define MACHOPIC_INDIRECT	(flag_pic == 1 || MACHO_DYNAMIC_NO_PIC_P)
 #define MACHOPIC_JUST_INDIRECT	(MACHO_DYNAMIC_NO_PIC_P)
 #define MACHOPIC_PURE		(flag_pic && ! MACHO_DYNAMIC_NO_PIC_P)
 
--- gcc.orig/config/i386/i386.c	2007-12-13 09:25:46.000000000 +0000
+++ gcc/config/i386/i386.c	2011-03-27 19:41:33.001632869 +0100
@@ -2219,6 +2219,8 @@ const struct attribute_spec ix86_attribu
   /* Sseregparm attribute says we are using x86_64 calling conventions
      for FP arguments.  */
   { "sseregparm", 0, 0, false, true, true, ix86_handle_cconv_attribute },
+  /* Optlink attribute says we are using D calling convention */
+  { "optlink",    0, 0, false, true, true, ix86_handle_cconv_attribute },
   /* force_align_arg_pointer says this function realigns the stack at entry.  */
   { (const char *)&ix86_force_align_arg_pointer_string, 0, 0,
     false, true,  true, ix86_handle_cconv_attribute },
@@ -2395,6 +2397,10 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -2413,6 +2419,10 @@ ix86_handle_cconv_attribute (tree *node,
         {
 	  error ("stdcall and fastcall attributes are not compatible");
 	}
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (*node)))
+        {
+	  error ("stdcall and optlink attributes are not compatible");
+	}
     }
 
   /* Can combine cdecl with regparm and sseregparm.  */
@@ -2426,6 +2436,27 @@ ix86_handle_cconv_attribute (tree *node,
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
@@ -2630,6 +2661,12 @@ ix86_return_pops_args (tree fundecl, tre
         || lookup_attribute ("fastcall", TYPE_ATTRIBUTES (funtype)))
       rtd = 1;
 
+    /* Optlink functions will pop the stack if returning float and
+       if not variable args.  */
+    if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (funtype))
+        && FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (funtype))))
+      rtd = 1;
+
     if (rtd
         && (TYPE_ARG_TYPES (funtype) == NULL_TREE
 	    || (TREE_VALUE (tree_last (TYPE_ARG_TYPES (funtype)))
@@ -2756,6 +2793,11 @@ init_cumulative_args (CUMULATIVE_ARGS *c
 	}
       else
 	cum->nregs = ix86_function_regparm (fntype, fndecl);
+
+      /* For optlink, last parameter is passed in eax rather than
+         being pushed on the stack.  */
+      if (lookup_attribute ("optlink", TYPE_ATTRIBUTES (fntype)))
+	cum->optlink = 1;
     }
 
   /* Set up the number of SSE registers used for passing SFmode
@@ -5217,6 +5259,11 @@ ix86_compute_frame_layout (struct ix86_f
     frame->red_zone_size = 0;
   frame->to_allocate -= frame->red_zone_size;
   frame->stack_pointer_offset -= frame->red_zone_size;
+
+  if (cfun->naked)
+      /* As above, skip return address */
+      frame->stack_pointer_offset = UNITS_PER_WORD;
+
 #if 0
   fprintf (stderr, "nregs: %i\n", frame->nregs);
   fprintf (stderr, "size: %i\n", size);
@@ -17732,7 +17779,7 @@ x86_output_mi_thunk (FILE *file ATTRIBUT
 	  output_set_got (tmp, NULL_RTX);
 
 	  xops[1] = tmp;
-	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
+	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
 	  output_asm_insn ("jmp\t{*}%1", xops);
 	}
     }
--- gcc.orig/config/i386/i386.h	2007-09-01 16:28:30.000000000 +0100
+++ gcc/config/i386/i386.h	2011-03-24 09:11:49.110507094 +0000
@@ -1428,6 +1428,7 @@ typedef struct ix86_args {
   int nregs;			/* # registers available for passing */
   int regno;			/* next available register number */
   int fastcall;			/* fastcall calling convention is used */
+  int optlink;			/* optlink calling convention is used */
   int sse_words;		/* # sse words passed so far */
   int sse_nregs;		/* # sse registers available for passing */
   int warn_sse;			/* True when we want to warn about SSE ABI.  */
--- gcc.orig/config/rs6000/rs6000.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/config/rs6000/rs6000.c	2011-03-21 18:37:14.283956590 +0000
@@ -15482,7 +15482,8 @@ rs6000_output_function_epilogue (FILE *f
 	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
 	 Java is 13.  Objective-C is 14.  Objective-C++ isn't assigned
 	 a number, so for now use 9.  */
-      if (! strcmp (language_string, "GNU C"))
+      if (! strcmp (language_string, "GNU C") ||
+      	! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU F95"))
--- gcc.orig/dwarf2out.c	2007-10-10 10:29:13.000000000 +0100
+++ gcc/dwarf2out.c	2011-03-21 18:37:14.339956873 +0000
@@ -5406,7 +5406,7 @@ is_c_family (void)
   unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
 
   return (lang == DW_LANG_C || lang == DW_LANG_C89 || lang == DW_LANG_ObjC
-	  || lang == DW_LANG_C99
+	  || lang == DW_LANG_C99 || lang == DW_LANG_D
 	  || lang == DW_LANG_C_plus_plus || lang == DW_LANG_ObjC_plus_plus);
 }
 
@@ -12381,6 +12381,8 @@ gen_compile_unit_die (const char *filena
     language = DW_LANG_ObjC;
   else if (strcmp (language_string, "GNU Objective-C++") == 0)
     language = DW_LANG_ObjC_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else
     language = DW_LANG_C89;
 
@@ -13475,7 +13477,7 @@ dwarf2out_decl (tree decl)
 
       /* For local statics lookup proper context die.  */
       if (TREE_STATIC (decl) && decl_function_context (decl))
-	context_die = lookup_decl_die (DECL_CONTEXT (decl));
+	context_die = lookup_decl_die (decl_function_context (decl));
 
       /* If we are in terse mode, don't generate any DIEs to represent any
 	 variable declarations or definitions.  */
--- gcc.orig/except.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/except.c	2011-03-21 18:37:14.347956905 +0000
@@ -1816,6 +1816,18 @@ sjlj_mark_call_sites (struct sjlj_lp_inf
 
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
--- gcc.orig/expr.c	2008-02-04 22:03:09.000000000 +0000
+++ gcc/expr.c	2011-03-21 18:37:14.367957002 +0000
@@ -8740,6 +8740,11 @@ expand_expr_real_1 (tree exp, rtx target
       /* Lowered by gimplify.c.  */
       gcc_unreachable ();
 
+    case STATIC_CHAIN_EXPR:
+    case STATIC_CHAIN_DECL:
+      /* Lowered by tree-nested.c */
+      gcc_unreachable ();
+
     case EXC_PTR_EXPR:
       return get_exception_pointer (cfun);
 
--- gcc.orig/function.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/function.c	2011-03-21 18:37:14.383957086 +0000
@@ -3022,7 +3022,8 @@ assign_parms (tree fndecl)
       FUNCTION_ARG_ADVANCE (all.args_so_far, data.promoted_mode,
 			    data.passed_type, data.named_arg);
 
-      assign_parm_adjust_stack_rtl (&data);
+      if (!cfun->naked)
+	assign_parm_adjust_stack_rtl (&data);
 
       if (assign_parm_setup_block_p (&data))
 	assign_parm_setup_block (&all, parm, &data);
@@ -3037,7 +3038,8 @@ assign_parms (tree fndecl)
 
   /* Output all parameter conversion instructions (possibly including calls)
      now that all parameters have been copied out of hard registers.  */
-  emit_insn (all.conversion_insns);
+  if (!cfun->naked)
+    emit_insn (all.conversion_insns);
 
   /* If we are receiving a struct value address as the first argument, set up
      the RTL for the function result. As this might require code to convert
@@ -3167,6 +3169,9 @@ gimplify_parameters (void)
   struct assign_parm_data_all all;
   tree fnargs, parm, stmts = NULL;
 
+  if (cfun->naked)
+    return NULL;
+
   assign_parms_initialize_all (&all);
   fnargs = assign_parms_augmented_arg_list (&all);
 
@@ -4148,11 +4153,15 @@ expand_function_start (tree subr)
       tree parm = cfun->static_chain_decl;
       rtx local = gen_reg_rtx (Pmode);
 
-      set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
       SET_DECL_RTL (parm, local);
       mark_reg_pointer (local, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (parm))));
 
-      emit_move_insn (local, static_chain_incoming_rtx);
+      if (! cfun->custom_static_chain)
+        {
+	    set_decl_incoming_rtl (parm, static_chain_incoming_rtx);
+	    emit_move_insn (local, static_chain_incoming_rtx);
+	}
+      /* else, the static chain will be set in the main body */
     }
 
   /* If the function receives a non-local goto, then store the
@@ -5065,6 +5074,9 @@ thread_prologue_and_epilogue_insns (rtx
 #endif
   edge_iterator ei;
 
+  if (cfun->naked)
+      return;
+
 #ifdef HAVE_prologue
   if (HAVE_prologue)
     {
--- gcc.orig/function.h	2007-09-01 16:28:30.000000000 +0100
+++ gcc/function.h	2011-03-21 18:37:14.387957113 +0000
@@ -462,6 +462,14 @@ struct function GTY(())
   /* Number of units of floating point registers that need saving in stdarg
      function.  */
   unsigned int va_list_fpr_size : 8;
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
--- gcc.orig/gcc.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/gcc.c	2011-03-21 18:37:14.403957191 +0000
@@ -126,6 +126,9 @@ static const char dir_separator_str[] =
 /* Flag set by cppspec.c to 1.  */
 int is_cpp_driver;
 
+/* Flag set by drivers needing Pthreads. */
+int need_pthreads;
+
 /* Flag saying to pass the greatest exit code returned by a sub-process
    to the calling program.  */
 static int pass_exit_codes;
@@ -461,6 +464,7 @@ or with constant text in a single argume
 	assembler has done its job.
  %D	Dump out a -L option for each directory in startfile_prefixes.
 	If multilib_dir is set, extra entries are generated with it affixed.
+ %N     Output the currently selected multilib directory name.
  %l     process LINK_SPEC as a spec.
  %L     process LIB_SPEC as a spec.
  %G     process LIBGCC_SPEC as a spec.
@@ -3927,6 +3931,9 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+      n_switches++;
+
   if (save_temps_flag && use_pipes)
     {
       /* -save-temps overrides -pipe, so that temp files are produced */
@@ -4265,6 +4272,18 @@ warranty; not even for MERCHANTABILITY o
 	}
     }
 
+  if (need_pthreads)
+    {
+	switches[n_switches].part1 = "pthread";
+	switches[n_switches].args = 0;
+	switches[n_switches].live_cond = SWITCH_OK;
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
@@ -5197,6 +5216,17 @@ do_spec_1 (const char *spec, int inswitc
 	      return value;
 	    break;
 
+	  case 'N':
+	    if (multilib_dir)
+	      {
+		arg_going = 1;
+		obstack_grow (&obstack, "-fmultilib-dir=",
+			      strlen ("-fmultilib-dir="));
+	        obstack_grow (&obstack, multilib_dir,
+			      strlen (multilib_dir));
+	      }
+	    break;
+
 	    /* Here we define characters other than letters and digits.  */
 
 	  case '{':
--- gcc.orig/gcc.h	2007-09-01 16:28:30.000000000 +0100
+++ gcc/gcc.h	2011-03-21 18:37:14.407957209 +0000
@@ -37,7 +37,7 @@ struct spec_function
    || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
    || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
    || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
-   || (CHAR) == 'B' || (CHAR) == 'b')
+   || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
 
 /* This defines which multi-letter switches take arguments.  */
 
--- gcc.orig/predict.c	2008-01-24 15:59:18.000000000 +0000
+++ gcc/predict.c	2011-03-21 18:37:14.407957209 +0000
@@ -1318,6 +1318,7 @@ tree_estimate_probability (void)
 	     care for error returns and other cases are often used for
 	     fast paths through function.  */
 	  if (e->dest == EXIT_BLOCK_PTR
+	      && last_stmt (bb) == NULL_TREE
 	      && (tmp = last_stmt (bb))
 	      && TREE_CODE (tmp) == RETURN_EXPR
 	      && !single_pred_p (bb))
--- gcc.orig/tree.def	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree.def	2011-03-21 18:37:14.415957245 +0000
@@ -527,6 +527,13 @@ DEFTREECODE (BIND_EXPR, "bind_expr", tcc
    Operand 2 is the static chain argument, or NULL.  */
 DEFTREECODE (CALL_EXPR, "call_expr", tcc_expression, 3)
 
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
--- gcc.orig/tree-gimple.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree-gimple.c	2011-03-21 18:37:14.419957266 +0000
@@ -71,6 +71,8 @@ is_gimple_formal_tmp_rhs (tree t)
     case VECTOR_CST:
     case OBJ_TYPE_REF:
     case ASSERT_EXPR:
+    case STATIC_CHAIN_EXPR: /* not sure if this is right...*/
+    case STATIC_CHAIN_DECL:
       return true;
 
     default:
@@ -144,7 +146,10 @@ is_gimple_lvalue (tree t)
 	  || TREE_CODE (t) == WITH_SIZE_EXPR
 	  /* These are complex lvalues, but don't have addresses, so they
 	     go here.  */
-	  || TREE_CODE (t) == BIT_FIELD_REF);
+	  || TREE_CODE (t) == BIT_FIELD_REF
+          /* This is an lvalue because it will be replaced with the real
+	     static chain decl. */
+	  || TREE_CODE (t) == STATIC_CHAIN_DECL);
 }
 
 /*  Return true if T is a GIMPLE condition.  */
--- gcc.orig/tree-inline.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree-inline.c	2011-03-27 19:35:28.411824960 +0100
@@ -554,10 +554,21 @@ copy_body_r (tree *tp, int *walk_subtree
      knows not to copy VAR_DECLs, etc., so this is safe.  */
   else
     {
+      if (id->transform_call_graph_edges == CB_CGE_MOVE &&
+	  TREE_CODE (*tp) == MODIFY_EXPR &&
+	  TREE_OPERAND (*tp, 0) ==
+	  DECL_STRUCT_FUNCTION (fn)->static_chain_decl)
+	{
+	  /* Don't use special methods to initialize the static chain
+	     if expanding inline.  If this code could somehow be
+	     expanded in expand_start_function, it would not be
+	     necessary to deal with it here. */
+	  *tp = build_empty_stmt ();
+	}
       /* Here we handle trees that are not completely rewritten.
 	 First we detect some inlining-induced bogosities for
 	 discarding.  */
-      if (TREE_CODE (*tp) == MODIFY_EXPR
+      else if (TREE_CODE (*tp) == MODIFY_EXPR
 	  && TREE_OPERAND (*tp, 0) == TREE_OPERAND (*tp, 1)
 	  && (lang_hooks.tree_inlining.auto_var_in_fn_p
 	      (TREE_OPERAND (*tp, 0), fn)))
--- gcc.orig/tree-nested.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree-nested.c	2011-03-27 19:27:04.065324056 +0100
@@ -327,6 +327,7 @@ get_chain_decl (struct nesting_info *inf
   if (!decl)
     {
       tree type;
+      enum tree_code code;
 
       type = get_frame_type (info->outer);
       type = build_pointer_type (type);
@@ -799,6 +800,8 @@ get_static_chain (struct nesting_info *i
 
   if (info->context == target_context)
     {
+      /* might be doing something wrong to need the following line.. */
+      get_frame_type (info);
       x = build_addr (info->frame_decl, target_context);
     }
   else
@@ -1621,6 +1624,10 @@ convert_tramp_reference (tree *tp, int *
       if (DECL_NO_STATIC_CHAIN (decl))
 	break;
 
+      /* Don't use a trampoline for a static reference. */
+      if (TREE_STATIC (t))
+	break;
+
       /* Lookup the immediate parent of the callee, as that's where
 	 we need to insert the trampoline.  */
       for (i = info; i->context != target_context; i = i->outer)
@@ -1691,6 +1698,14 @@ convert_call_expr (tree *tp, int *walk_s
 	}
       break;
 
+    case STATIC_CHAIN_EXPR:
+      *tp = get_static_chain (info, TREE_OPERAND (t, 0), &wi->tsi);
+      break;
+
+    case STATIC_CHAIN_DECL:
+      *tp = get_chain_decl (info);
+      break;
+ 
     case RETURN_EXPR:
     case MODIFY_EXPR:
     case WITH_SIZE_EXPR:
@@ -1826,6 +1841,24 @@ finalize_nesting_tree_1 (struct nesting_
       tree x = build3 (COMPONENT_REF, TREE_TYPE (root->chain_field),
 		       root->frame_decl, root->chain_field, NULL_TREE);
       x = build2 (MODIFY_EXPR, TREE_TYPE (x), x, get_chain_decl (root));
+      if (DECL_STRUCT_FUNCTION (root->context)->custom_static_chain)
+      {
+        tree_stmt_iterator i =
+          tsi_start ( BIND_EXPR_BODY (DECL_SAVED_TREE (context)));
+        while (!tsi_end_p (i))
+          {
+            tree t = tsi_stmt (i);
+            if (TREE_CODE (t) == MODIFY_EXPR &&
+                TREE_OPERAND (t, 0) == root->chain_decl)
+              {
+                tsi_link_after(& i, x, TSI_SAME_STMT);
+                x = NULL_TREE;
+                break;
+              }
+          }
+        gcc_assert(x == NULL_TREE);
+      }
+      else
       append_to_statement_list (x, &stmt_list);
     }
 
--- gcc.orig/tree-pretty-print.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree-pretty-print.c	2011-03-21 18:37:14.455957448 +0000
@@ -1155,6 +1155,16 @@ dump_generic_node (pretty_printer *buffe
 	pp_string (buffer, " [tail call]");
       break;
 
+    case STATIC_CHAIN_EXPR:
+	pp_string (buffer, "<<static chain of ");
+	dump_generic_node (buffer, TREE_OPERAND (node, 0), spc, flags, false);
+	pp_string (buffer, ">>");
+      break;
+
+    case STATIC_CHAIN_DECL:
+       pp_string (buffer, "<<static chain decl>>");
+       break;
+	
     case WITH_CLEANUP_EXPR:
       NIY;
       break;
--- gcc.orig/tree-sra.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree-sra.c	2011-03-21 18:37:14.487957606 +0000
@@ -248,6 +248,8 @@ sra_type_can_be_decomposed_p (tree type)
     case RECORD_TYPE:
       {
 	bool saw_one_field = false;
+	tree last_offset = size_zero_node;
+	tree cmp;
 
 	for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
 	  if (TREE_CODE (t) == FIELD_DECL)
@@ -257,6 +259,11 @@ sra_type_can_be_decomposed_p (tree type)
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
--- gcc.orig/tree-vect-analyze.c	2007-09-01 16:28:30.000000000 +0100
+++ gcc/tree-vect-analyze.c	2011-03-21 18:37:14.495957642 +0000
@@ -593,6 +593,9 @@ vect_analyze_data_ref_dependence (struct
          
   if (DDR_ARE_DEPENDENT (ddr) == chrec_known)
     return false;
+    
+  if ((DR_IS_READ (dra) && DR_IS_READ (drb)) || (dra == drb))
+    return false;
   
   if (DDR_ARE_DEPENDENT (ddr) == chrec_dont_know)
     {
@@ -603,6 +606,12 @@ vect_analyze_data_ref_dependence (struct
           print_generic_expr (vect_dump, DR_REF (dra), TDF_SLIM);
           fprintf (vect_dump, " and ");
           print_generic_expr (vect_dump, DR_REF (drb), TDF_SLIM);
+          fprintf (vect_dump, "\n");
+          dump_data_reference (vect_dump, dra);
+          fprintf (vect_dump, " ---- ");
+          dump_data_reference (vect_dump, drb);
+          fprintf (vect_dump, "\n");
+          fprintf (vect_dump, "Eq test %i\n", DR_REF (dra) == DR_REF (drb));
         }
       return true;
     }
