This implements D language support in the GCC back end, and adds
relevant documentation about the GDC front end.
---

--- gcc/config/rs6000/rs6000.c	2013-02-08 19:36:04.000000000 +0000
+++ gcc/config/rs6000/rs6000.c	2013-03-20 16:26:18.566235720 +0000
@@ -21578,7 +21578,8 @@ rs6000_output_function_epilogue (FILE *f
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc/doc/frontends.texi	2013-01-10 20:38:27.000000000 +0000
+++ gcc/doc/frontends.texi	2013-03-20 16:26:18.574235721 +0000
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
--- gcc/doc/install.texi	2013-02-05 09:26:06.000000000 +0000
+++ gcc/doc/install.texi	2013-03-20 16:26:18.586235720 +0000
@@ -1350,12 +1350,12 @@ their runtime libraries should be built.
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
--- gcc/doc/invoke.texi	2013-03-14 09:13:36.000000000 +0000
+++ gcc/doc/invoke.texi	2013-03-20 16:26:18.610235722 +0000
@@ -1164,6 +1164,15 @@ called @dfn{specs}.
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
@@ -1199,6 +1208,7 @@ objective-c  objective-c-header  objecti
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
--- gcc/doc/sourcebuild.texi	2013-01-10 20:38:27.000000000 +0000
+++ gcc/doc/sourcebuild.texi	2013-03-20 16:26:18.614235722 +0000
@@ -113,6 +113,9 @@ The Objective-C and Objective-C++ runtim
 @item libquadmath
 The runtime support library for quad-precision math operations.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
--- gcc/doc/standards.texi	2013-01-10 20:38:27.000000000 +0000
+++ gcc/doc/standards.texi	2013-03-20 16:26:18.662235721 +0000
@@ -288,6 +288,16 @@ a specific version.  In general GCC trac
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
--- gcc/dwarf2out.c	2013-03-05 23:08:45.000000000 +0000
+++ gcc/dwarf2out.c	2013-03-20 16:26:18.690235723 +0000
@@ -18907,6 +18907,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
--- gcc/gcc.c	2013-03-06 06:32:03.000000000 +0000
+++ gcc/gcc.c	2013-03-20 16:26:18.706235722 +0000
@@ -1002,6 +1002,7 @@ static const struct compiler default_com
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
