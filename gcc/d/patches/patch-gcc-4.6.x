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
