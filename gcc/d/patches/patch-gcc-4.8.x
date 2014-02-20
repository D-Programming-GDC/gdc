This implements D language support in the GCC back end, and adds
relevant documentation about the GDC front end.
---

--- gcc/config/rs6000/rs6000.c	2013-09-23 16:19:31.000000000 +0200
+++ gcc/config/rs6000/rs6000.c	2014-02-20 11:31:16.690232916 +0100
@@ -21584,7 +21584,8 @@
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc/doc/frontends.texi	2013-01-10 21:38:27.000000000 +0100
+++ gcc/doc/frontends.texi	2014-02-20 11:31:16.690232916 +0100
@@ -9,6 +9,7 @@
 @cindex GNU Compiler Collection
 @cindex GNU C Compiler
 @cindex Ada
+@cindex D
 @cindex Fortran
 @cindex Go
 @cindex Java
@@ -17,7 +18,7 @@
 GCC stands for ``GNU Compiler Collection''.  GCC is an integrated
 distribution of compilers for several major programming languages.  These
 languages currently include C, C++, Objective-C, Objective-C++, Java,
-Fortran, Ada, and Go.
+Fortran, Ada, D and Go.
 
 The abbreviation @dfn{GCC} has multiple meanings in common use.  The
 current official meaning is ``GNU Compiler Collection'', which refers
--- gcc/doc/install.texi	2013-02-05 10:26:06.000000000 +0100
+++ gcc/doc/install.texi	2014-02-20 11:31:16.690232916 +0100
@@ -1350,12 +1350,12 @@
 grep language= */config-lang.in
 @end smallexample
 Currently, you can use any of the following:
-@code{all}, @code{ada}, @code{c}, @code{c++}, @code{fortran},
+@code{all}, @code{ada}, @code{c}, @code{c++}, @code{d}, @code{fortran},
 @code{go}, @code{java}, @code{objc}, @code{obj-c++}.
 Building the Ada compiler has special requirements, see below.
 If you do not pass this flag, or specify the option @code{all}, then all
 default languages available in the @file{gcc} sub-tree will be configured.
-Ada, Go and Objective-C++ are not default languages; the rest are.
+Ada, D, Go and Objective-C++ are not default languages; the rest are.
 
 @item --enable-stage1-languages=@var{lang1},@var{lang2},@dots{}
 Specify that a particular subset of compilers and their runtime
--- gcc/doc/invoke.texi	2013-06-19 21:55:50.000000000 +0200
+++ gcc/doc/invoke.texi	2014-02-20 11:31:16.693566250 +0100
@@ -1163,6 +1163,15 @@
 Ada source code file containing a library unit body (a subprogram or
 package body).  Such files are also called @dfn{bodies}.
 
+@item @var{file}.d
+D source code file.
+
+@item @var{file}.di
+D interface code file.
+
+@item @var{file}.dd
+D documentation code file.
+
 @c GCC also knows about some suffixes for languages not yet included:
 @c Pascal:
 @c @var{file}.p
@@ -1198,6 +1207,7 @@
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
--- gcc/doc/sourcebuild.texi	2013-01-10 21:38:27.000000000 +0100
+++ gcc/doc/sourcebuild.texi	2014-02-20 11:31:16.693566250 +0100
@@ -113,6 +113,9 @@
 @item libquadmath
 The runtime support library for quad-precision math operations.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
--- gcc/doc/standards.texi	2013-04-05 06:26:20.000000000 +0200
+++ gcc/doc/standards.texi	2014-02-20 11:31:16.693566250 +0100
@@ -283,6 +283,16 @@
 As of the GCC 4.7.1 release, GCC supports the Go 1 language standard,
 described at @uref{http://golang.org/doc/go1.html}.
 
+@section D language
+
+The D language is under development as of this writing; see the
+@uref{http://dlang.org/@/language-reference.html, current language
+reference}.  At present the current major version of D is 2.0, and
+there is no way to describe the language supported by GCC in terms of
+a specific minor version.  In general GCC follows the D frontend
+releases closely, and any given GCC release will support the current
+language as of the date that the release was frozen.
+
 @section References for other languages
 
 @xref{Top, GNAT Reference Manual, About This Guide, gnat_rm,
--- gcc/dwarf2out.c	2013-03-21 22:24:47.000000000 +0100
+++ gcc/dwarf2out.c	2014-02-20 11:37:15.040240785 +0100
@@ -4557,6 +4557,15 @@
   return lang == DW_LANG_Ada95 || lang == DW_LANG_Ada83;
 }
 
+/* Return TRUE if the language is D.  */
+
+static inline bool
+is_dlang (void)
+{
+  unsigned int lang = get_AT_unsigned (comp_unit_die (), DW_AT_language);
+  return lang == DW_LANG_D;
+}
+
 /* Remove the specified attribute if present.  */
 
 static void
@@ -18909,6 +18918,8 @@
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
@@ -19854,7 +19865,7 @@
     {
       /* Output a real namespace or module.  */
       context_die = setup_namespace_context (decl, comp_unit_die ());
-      namespace_die = new_die (is_fortran ()
+      namespace_die = new_die (is_fortran () || is_dlang ()
 			       ? DW_TAG_module : DW_TAG_namespace,
 			       context_die, decl);
       /* For Fortran modules defined in different CU don't add src coords.  */
--- gcc/gcc.c	2013-05-17 14:24:26.000000000 +0200
+++ gcc/gcc.c	2014-02-20 11:31:16.696899583 +0100
@@ -1003,6 +1003,7 @@
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
