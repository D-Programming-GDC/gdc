This implements D language support in the GCC back end, and adds
relevant documentation about the GDC front end.
---

--- gcc.orig/config/rs6000/rs6000.c	2013-03-27 12:47:24.187363448 +0100
+++ gcc/config/rs6000/rs6000.c	2013-03-27 12:38:27.072545859 +0100
@@ -21065,7 +21065,8 @@ rs6000_output_function_epilogue (FILE *f
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
diff -urp gcc.orig/doc/frontends.texi gcc/doc/frontends.texi
--- gcc.orig/doc/frontends.texi	2013-03-27 12:47:24.133362461 +0100
+++ gcc/doc/frontends.texi	2013-03-27 12:38:27.073545878 +0100
@@ -10,6 +10,7 @@
 @cindex GNU Compiler Collection
 @cindex GNU C Compiler
 @cindex Ada
+@cindex D
 @cindex Fortran
 @cindex Go
 @cindex Java
@@ -18,7 +19,7 @@
 GCC stands for ``GNU Compiler Collection''.  GCC is an integrated
 distribution of compilers for several major programming languages.  These
 languages currently include C, C++, Objective-C, Objective-C++, Java,
-Fortran, Ada, and Go.
+Fortran, Ada, D and Go.
 
 The abbreviation @dfn{GCC} has multiple meanings in common use.  The
 current official meaning is ``GNU Compiler Collection'', which refers
diff -urp gcc.orig/doc/install.texi gcc/doc/install.texi
--- gcc.orig/doc/install.texi	2013-03-27 12:47:24.141362608 +0100
+++ gcc/doc/install.texi	2013-03-27 12:38:27.075545915 +0100
@@ -1360,12 +1360,12 @@ their runtime libraries should be built.
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
diff -urp gcc.orig/doc/invoke.texi gcc/doc/invoke.texi
--- gcc.orig/doc/invoke.texi	2013-03-27 12:47:24.137362535 +0100
+++ gcc/doc/invoke.texi	2013-03-27 12:38:27.112546594 +0100
@@ -1134,6 +1134,15 @@ called @dfn{specs}.
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
@@ -1169,6 +1178,7 @@ objective-c  objective-c-header  objecti
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
diff -urp gcc.orig/doc/sourcebuild.texi gcc/doc/sourcebuild.texi
--- gcc.orig/doc/sourcebuild.texi	2013-03-27 12:47:24.137362535 +0100
+++ gcc/doc/sourcebuild.texi	2013-03-27 12:38:27.115546649 +0100
@@ -104,6 +104,9 @@ dereferencing operations.
 @item libobjc
 The Objective-C and Objective-C++ runtime library.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
diff -urp gcc.orig/doc/standards.texi gcc/doc/standards.texi
--- gcc.orig/doc/standards.texi	2013-03-27 12:47:24.141362608 +0100
+++ gcc/doc/standards.texi	2013-03-27 12:41:50.429264859 +0100
@@ -289,6 +289,16 @@ a specific version.  In general GCC trac
 closely, and any given release will support the language as of the
 date that the release was frozen.
 
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
diff -urp gcc.orig/dwarf2out.c gcc/dwarf2out.c
--- gcc.orig/dwarf2out.c	2013-03-27 12:47:24.231364253 +0100
+++ gcc/dwarf2out.c	2013-03-27 12:38:27.124546814 +0100
@@ -18521,6 +18521,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
diff -urp gcc.orig/gcc.c gcc/gcc.c
--- gcc.orig/gcc.c	2013-03-27 12:47:24.143362644 +0100
+++ gcc/gcc.c	2013-03-27 12:38:27.129546906 +0100
@@ -934,6 +934,7 @@ static const struct compiler default_com
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
