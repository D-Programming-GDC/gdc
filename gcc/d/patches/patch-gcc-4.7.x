--- gcc~/config/rs6000/rs6000.c	2012-01-30 03:28:10.991380885 +0000
+++ gcc/config/rs6000/rs6000.c	2012-01-30 03:28:21.407380698 +0000
@@ -21064,7 +21064,8 @@ rs6000_output_function_epilogue (FILE *f
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+          || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc~/dwarf2out.c	2012-01-30 03:27:17.023381855 +0000
+++ gcc/dwarf2out.c	2012-01-30 03:28:21.419380696 +0000
@@ -18455,6 +18455,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
