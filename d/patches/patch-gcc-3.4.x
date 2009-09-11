diff -c gcc-orig/gcc.c gcc/gcc.c
*** gcc-orig/gcc.c	Sat Jan 21 13:52:11 2006
--- gcc/gcc.c	Thu Mar  1 10:48:46 2007
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
diff -cr gcc-orig/gcc.c gcc/gcc.h
*** gcc-orig/gcc.h Fri Jun 24 22:02:01 2005
--- gcc/gcc.h       Sun Mar  4 13:44:05 2007
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
  
diff -c gcc-gcc-3.4.0-orig/real.h gcc-gcc-3.4.0/real.h
*** gcc-gcc-3.4.0-orig/real.h	Fri Oct 10 20:33:05 2003
--- gcc-gcc-3.4.0/real.h	Mon Sep 27 14:06:40 2004
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
diff -c gcc-gcc-3.4.0-orig/rtl.h gcc-gcc-3.4.0/rtl.h
*** gcc-gcc-3.4.0-orig/rtl.h	Thu Mar 25 16:44:43 2004
--- gcc-gcc-3.4.0/rtl.h	Mon Sep 27 14:06:40 2004
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
*** gcc-orig/libgcc2.c	Sun Sep 26 16:47:14 2004
--- gcc/libgcc2.c	Sat May  7 11:47:55 2005
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
*** gcc-orig/config/rs6000/rs6000.c	Fri Oct 22 15:19:35 2004
--- gcc/config/rs6000/rs6000.c	Sat May 21 16:08:37 2005
***************
*** 12973,12979 ****
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77"))
  	i = 1;
--- 12973,12980 ----
  	 use language_string.
  	 C is 0.  Fortran is 1.  Pascal is 2.  Ada is 3.  C++ is 9.
  	 Java is 13.  Objective-C is 14.  */
!       if (! strcmp (language_string, "GNU C") ||
! 	  ! strcmp (language_string, "GNU D"))
  	i = 0;
        else if (! strcmp (language_string, "GNU F77"))
  	i = 1;
diff -c gcc-orig/function.c gcc/function.c
*** gcc-orig/function.c	Wed May 11 17:19:49 2005
--- gcc/function.c	Thu Jun  9 19:53:27 2005
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
diff -c gcc-orig/function.h gcc/function.h
*** gcc-orig/function.h	Wed May  5 19:24:30 2004
--- gcc/function.h	Wed Jun  8 18:46:24 2005
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
diff -cr gcc-orig/dwarf2.h gcc/dwarf2.h
*** gcc-orig/dwarf2.h	Thu Jan  8 07:50:36 2004
--- gcc/dwarf2.h	Thu Sep 22 13:21:47 2005
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
*** gcc-orig/dwarf2out.c	Mon Oct 25 21:46:45 2004
--- gcc/dwarf2out.c	Thu Sep 22 13:23:12 2005
***************
*** 4979,4985 ****
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus);
  }
  
  /* Return TRUE if the language is C++.  */
--- 4979,4986 ----
    unsigned int lang = get_AT_unsigned (comp_unit_die, DW_AT_language);
  
    return (lang == DW_LANG_C || lang == DW_LANG_C89
! 	  || lang == DW_LANG_C_plus_plus
! 	  || lang == DW_LANG_D);
  }
  
  /* Return TRUE if the language is C++.  */
***************
*** 11403,11408 ****
--- 11404,11411 ----
      language = DW_LANG_Pascal83;
    else if (strcmp (language_string, "GNU Java") == 0)
      language = DW_LANG_Java;
+   else if (strcmp (language_string, "GNU D") == 0)
+     language = DW_LANG_D;
    else
      language = DW_LANG_C89;
  
***************
*** 13061,13067 ****
  	    add_child_die (comp_unit_die, die);
  	  else if (node->created_for
  		   && ((DECL_P (node->created_for)
! 			&& (context = DECL_CONTEXT (node->created_for)))
  		       || (TYPE_P (node->created_for)
  			   && (context = TYPE_CONTEXT (node->created_for))))
  		   && TREE_CODE (context) == FUNCTION_DECL)
--- 13064,13070 ----
  	    add_child_die (comp_unit_die, die);
  	  else if (node->created_for
  		   && ((DECL_P (node->created_for)
! 			&& (context = decl_function_context (node->created_for)))
  		       || (TYPE_P (node->created_for)
  			   && (context = TYPE_CONTEXT (node->created_for))))
  		   && TREE_CODE (context) == FUNCTION_DECL)
diff -cr gcc-orig/config/i386/i386.c gcc/config/i386/i386.c
*** gcc-orig/config/i386/i386.c	Wed Mar 16 10:23:40 2005
--- gcc/config/i386/i386.c	Sun Oct 30 10:10:39 2005
***************
*** 15683,15689 ****
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%0@GOT(%1), %1|%1, %0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
--- 15683,15689 ----
  	  output_set_got (tmp);
  
  	  xops[1] = tmp;
! 	  output_asm_insn ("mov{l}\t{%a0@GOT(%1), %1|%1, %a0@GOT[%1]}", xops);
  	  output_asm_insn ("jmp\t{*}%1", xops);
  	}
      }
