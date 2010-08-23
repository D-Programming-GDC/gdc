diff -cr gcc-orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc-orig/config/i386/i386.c	2005-08-03 10:15:28.000000000 -0400
--- gcc/config/i386/i386.c	2010-08-22 19:43:07.250318000 -0400
***************
*** 15684,15690 ****
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 15684,15690 ----
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
diff -cr gcc-orig/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
*** gcc-orig/config/rs6000/rs6000.c	2006-02-28 20:04:29.000000000 -0500
--- gcc/config/rs6000/rs6000.c	2010-08-22 19:43:07.226317000 -0400
***************
*** 12987,12993 ****
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77"))
  	i = 1;
--- 12987,12994 ----
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C") ||
! 	  ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77"))
  	i = 1;
diff -cr gcc-orig/dwarf2.h gcc/dwarf2.h
*** gcc-orig/dwarf2.h	2004-01-08 02:50:46.000000000 -0500
--- gcc/dwarf2.h	2010-08-22 19:43:07.230320000 -0400
***************
*** 591,596 ****
--- 591,597 ----
      DW_LANG_C99 = 0x000c,
      DW_LANG_Ada95 = 0x000d,
      DW_LANG_Fortran95 = 0x000e,
+     DW_LANG_D = 0x0013,
      /* MIPS.  */
      DW_LANG_Mips_Assembler = 0x8001
    };
diff -cr gcc-orig/dwarf2out.c gcc/dwarf2out.c
*** gcc-orig/dwarf2out.c	2005-05-10 21:51:52.000000000 -0400
--- gcc/dwarf2out.c	2010-08-22 19:43:07.242318000 -0400
***************
*** 4980,4986 ****
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus);
  }
  
  /* Return TRUE if the language is C++.  */
--- 4980,4987 ----
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus
! 	  || lang == DW_LANG_D);
  }
  
  /* Return TRUE if the language is C++.  */
***************
*** 11433,11438 ****
--- 11434,11441 ----
      language = DW_LANG_Pascal83;
    else if (strcmp (language_string, "GNU Java") == 0)
      language = DW_LANG_Java;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
***************
*** 13091,13097 ****
  	    add_child_die (comp_unit_die, die);
  	  else if (node->created_for
  		   && ((DECL_P (node->created_for)
! 			&& (context = DECL_CONTEXT (node->created_for)))
  		       || (TYPE_P (node->created_for)
  			   && (context = TYPE_CONTEXT (node->created_for))))
  		   && TREE_CODE (context) == FUNCTION_DECL)
--- 13094,13100 ----
  	    add_child_die (comp_unit_die, die);
  	  else if (node->created_for
  		   && ((DECL_P (node->created_for)
! 			&& (context = decl_function_context (node->created_for)))
  		       || (TYPE_P (node->created_for)
  			   && (context = TYPE_CONTEXT (node->created_for))))
  		   && TREE_CODE (context) == FUNCTION_DECL)
diff -cr gcc-orig/function.c gcc/function.c
*** gcc-orig/function.c	2005-05-11 17:19:54.000000000 -0400
--- gcc/function.c	2010-08-22 19:43:07.230320000 -0400
***************
*** 6670,6680 ****
      {
        last_ptr = assign_stack_local (Pmode, GET_MODE_SIZE (Pmode), 0);
  
!       /* Delay copying static chain if it is not a register to avoid
! 	 conflicts with regs used for parameters.  */
!       if (! SMALL_REGISTER_CLASSES
! 	  || GET_CODE (static_chain_incoming_rtx) == REG)
! 	emit_move_insn (last_ptr, static_chain_incoming_rtx);
      }
  
    /* If the parameters of this function need cleaning up, get a label
--- 6670,6683 ----
      {
        last_ptr = assign_stack_local (Pmode, GET_MODE_SIZE (Pmode), 0);
  
!       if (cfun->static_chain_expr == NULL_TREE)
!         {
! 	  /* Delay copying static chain if it is not a register to avoid
! 	     conflicts with regs used for parameters.  */
! 	  if (! SMALL_REGISTER_CLASSES
! 	      || GET_CODE (static_chain_incoming_rtx) == REG)
! 	    emit_move_insn (last_ptr, static_chain_incoming_rtx);
! 	}
      }
  
    /* If the parameters of this function need cleaning up, get a label
***************
*** 6762,6771 ****
  
    /* Copy the static chain now if it wasn't a register.  The delay is to
       avoid conflicts with the parameter passing registers.  */
! 
!   if (SMALL_REGISTER_CLASSES && current_function_needs_context)
!     if (GET_CODE (static_chain_incoming_rtx) != REG)
!       emit_move_insn (last_ptr, static_chain_incoming_rtx);
  
    /* The following was moved from init_function_start.
       The move is supposed to make sdb output more accurate.  */
--- 6765,6776 ----
  
    /* Copy the static chain now if it wasn't a register.  The delay is to
       avoid conflicts with the parameter passing registers.  */
!   if (cfun->static_chain_expr == NULL_TREE)
!     {
!       if (SMALL_REGISTER_CLASSES && current_function_needs_context)
! 	if (GET_CODE (static_chain_incoming_rtx) != REG)
! 	  emit_move_insn (last_ptr, static_chain_incoming_rtx);
!     }
  
    /* The following was moved from init_function_start.
       The move is supposed to make sdb output more accurate.  */
***************
*** 6790,6799 ****
  	  /* If the static chain originally came in a register, put it back
  	     there, then move it out in the next insn.  The reason for
  	     this peculiar code is to satisfy function integration.  */
! 	  if (SMALL_REGISTER_CLASSES
! 	      && GET_CODE (static_chain_incoming_rtx) == REG)
! 	    emit_move_insn (static_chain_incoming_rtx, last_ptr);
! 	  last_ptr = copy_to_reg (static_chain_incoming_rtx);
  	}
  
        while (tem)
--- 6795,6815 ----
  	  /* If the static chain originally came in a register, put it back
  	     there, then move it out in the next insn.  The reason for
  	     this peculiar code is to satisfy function integration.  */
! 	  if (cfun->static_chain_expr == NULL_TREE)
! 	    {
! 	      if (SMALL_REGISTER_CLASSES
! 		  && GET_CODE (static_chain_incoming_rtx) == REG)
! 		emit_move_insn (static_chain_incoming_rtx, last_ptr);
! 	      last_ptr = copy_to_reg (static_chain_incoming_rtx);
! 	    }
! 	  else
! 	    {
! 		rtx r = expand_expr_real (cfun->static_chain_expr, NULL_RTX,
! 		    VOIDmode, EXPAND_NORMAL, NULL);
! 		r = copy_to_reg (r);
! 		emit_move_insn (last_ptr, r);
! 	        last_ptr = r;
! 	    }
  	}
  
        while (tem)
diff -cr gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	2004-05-05 19:24:30.000000000 -0400
--- gcc/function.h	2010-08-22 19:43:07.230320000 -0400
***************
*** 517,522 ****
--- 517,526 ----
  
    /* Nonzero if the rtl inliner has saved the function for inlining.  */
    unsigned int saved_for_inline : 1;
+ 
+   /* Expression to be evaluated to get the static chain.  If NULL,
+      static_chain_incoming_rtx is used. */
+   tree static_chain_expr;
  };
  
  /* The function currently being compiled.  */
diff -cr gcc-orig/gcc.c gcc/gcc.c
*** gcc-orig/gcc.c	2006-01-21 13:52:11.000000000 -0500
--- gcc/gcc.c	2010-08-22 19:43:07.214319000 -0400
***************
*** 133,138 ****
--- 133,141 ----
  /* Flag set by cppspec.c to 1.  */
  int is_cpp_driver;
  
+ /* Flag set by drivers needing Pthreads. */
+ int need_pthreads;
+ 
  /* Flag saying to pass the greatest exit code returned by a sub-process
     to the calling program.  */
  static int pass_exit_codes;
***************
*** 464,469 ****
--- 467,473 ----
  	assembler has done its job.
   %D	Dump out a -L option for each directory in startfile_prefixes.
  	If multilib_dir is set, extra entries are generated with it affixed.
+  %N     Output the currently selected multilib directory name.
   %l     process LINK_SPEC as a spec.
   %L     process LIB_SPEC as a spec.
   %G     process LIBGCC_SPEC as a spec.
***************
*** 3738,3743 ****
--- 3742,3750 ----
  
    combine_inputs = (have_c && have_o && lang_n_infiles > 1);
  
+   if (need_pthreads)
+       n_switches++;
+ 
    if ((save_temps_flag || report_times) && use_pipes)
      {
        /* -save-temps overrides -pipe, so that temp files are produced */
***************
*** 4080,4085 ****
--- 4087,4104 ----
  	}
      }
  
+   if (need_pthreads)
+     {
+ 	switches[n_switches].part1 = "pthread";
+ 	switches[n_switches].args = 0;
+ 	switches[n_switches].live_cond = SWITCH_OK;
+ 	/* Do not print an error if there is not expansion for -pthread. */
+ 	switches[n_switches].validated = 1;
+ 	switches[n_switches].ordering = 0;
+ 
+ 	n_switches++;
+     }
+ 
    switches[n_switches].part1 = 0;
    infiles[n_infiles].name = 0;
  }
***************
*** 5041,5046 ****
--- 5060,5076 ----
  	      return value;
  	    break;
  
+ 	  case 'N':
+ 	    if (multilib_dir)
+ 	      {
+ 		arg_going = 1;
+ 		obstack_grow (&obstack, "-fmultilib-dir=",
+ 			      strlen ("-fmultilib-dir="));
+ 	        obstack_grow (&obstack, multilib_dir,
+ 			      strlen (multilib_dir));
+ 	      }
+ 	    break;
+ 
  	    /* Here we define characters other than letters and digits.  */
  
  	  case '{':
diff -cr gcc-orig/gcc.h gcc/gcc.h
*** gcc-orig/gcc.h	2003-07-06 02:15:36.000000000 -0400
--- gcc/gcc.h	2010-08-22 19:43:07.214319000 -0400
***************
*** 38,44 ****
     || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
     || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
!    || (CHAR) == 'B' || (CHAR) == 'b')
  
  /* This defines which multi-letter switches take arguments.  */
  
--- 38,44 ----
     || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
     || (CHAR) == 'I' || (CHAR) == 'm' || (CHAR) == 'x' \
     || (CHAR) == 'L' || (CHAR) == 'A' || (CHAR) == 'V' \
!    || (CHAR) == 'B' || (CHAR) == 'b' || (CHAR) == 'J')
  
  /* This defines which multi-letter switches take arguments.  */
  
diff -cr gcc-orig/libgcc2.c gcc/libgcc2.c
*** gcc-orig/libgcc2.c	2004-12-15 07:34:40.000000000 -0500
--- gcc/libgcc2.c	2010-08-22 19:43:07.214319000 -0400
***************
*** 29,34 ****
--- 29,45 ----
  Software Foundation, 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.  */
  
+ #ifdef L_eprintf
+ #ifdef __APPLE__
+ /* Hack for MacOS 10.4: gcc 3.4.x uses -mlong-double-128 to build
+    libgcc.  On 10.4, this causes *printf to be defined as
+    *printf$LDBLStub and requires linking with libSystemStubs.  Prevent
+    this from happening by making it seem as though double is the same
+    as long double. */
+ #undef  __LDBL_MANT_DIG__
+ #define __LDBL_MANT_DIG__ __DBL_MANT_DIG__
+ #endif
+ #endif
  
  /* We include auto-host.h here to get HAVE_GAS_HIDDEN.  This is
     supposedly valid even though this is a "target" file.  */
diff -cr gcc-orig/real.h gcc/real.h
*** gcc-orig/real.h	2003-10-10 16:33:07.000000000 -0400
--- gcc/real.h	2010-08-22 19:43:07.214319000 -0400
***************
*** 40,48 ****
  #define SIGSZ			(SIGNIFICAND_BITS / HOST_BITS_PER_LONG)
  #define SIG_MSB			((unsigned long)1 << (HOST_BITS_PER_LONG - 1))
  
  struct real_value GTY(())
  {
!   ENUM_BITFIELD (real_value_class) class : 2;
    unsigned int sign : 1;
    unsigned int signalling : 1;
    unsigned int canonical : 1;
--- 40,56 ----
  #define SIGSZ			(SIGNIFICAND_BITS / HOST_BITS_PER_LONG)
  #define SIG_MSB			((unsigned long)1 << (HOST_BITS_PER_LONG - 1))
  
+ /* Can't have "class" in C++, but gentype gets confused on #ifdefs
+    within the struct. */
+ #ifndef __cplusplus
+ # define REAL_CLASSY class
+ #else
+ # define REAL_CLASSY cl
+ #endif
+ 
  struct real_value GTY(())
  {
!   ENUM_BITFIELD (real_value_class) REAL_CLASSY : 2;
    unsigned int sign : 1;
    unsigned int signalling : 1;
    unsigned int canonical : 1;
***************
*** 50,55 ****
--- 58,65 ----
    unsigned long sig[SIGSZ];
  };
  
+ #undef REAL_CLASSY
+ 
  /* Various headers condition prototypes on #ifdef REAL_VALUE_TYPE, so it
     needs to be a macro.  We do need to continue to have a structure tag
     so that other headers can forward declare it.  */
diff -cr gcc-orig/rtl.h gcc/rtl.h
*** gcc-orig/rtl.h	2005-12-30 19:39:42.000000000 -0500
--- gcc/rtl.h	2010-08-22 19:43:07.214319000 -0400
***************
*** 121,128 ****
--- 121,133 ----
    int rtint;
    unsigned int rtuint;
    const char *rtstr;
+ #ifndef __cplusplus
    rtx rtx;
    rtvec rtvec;
+ #else
+   rtx rt_rtx;
+   rtvec rt_vec;
+ #endif
    enum machine_mode rttype;
    addr_diff_vec_flags rt_addr_diff_vec_flags;
    struct cselib_val_struct *rt_cselib;
