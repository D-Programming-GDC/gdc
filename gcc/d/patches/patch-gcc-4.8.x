--- gcc-4.8-20120812/gcc/config/rs6000/rs6000.c	2012-07-31 23:14:44.000000000 +0100
+++ gcc-4.8/gcc/config/rs6000/rs6000.c	2012-08-17 18:30:37.121021742 +0100
@@ -21410,7 +21410,8 @@ rs6000_output_function_epilogue (FILE *f
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc-4.8-20120812/gcc/doc/frontends.texi	2011-01-03 20:52:22.000000000 +0000
+++ gcc-4.8/gcc/doc/frontends.texi	2012-08-17 18:30:37.133021743 +0100
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
--- gcc-4.8-20120812/gcc/doc/install.texi	2012-07-16 20:14:18.000000000 +0100
+++ gcc-4.8/gcc/doc/install.texi	2012-08-17 18:30:37.141021743 +0100
@@ -1355,12 +1355,12 @@ their runtime libraries should be built.
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
--- gcc-4.8-20120812/gcc/doc/invoke.texi	2012-08-10 15:19:09.000000000 +0100
+++ gcc-4.8/gcc/doc/invoke.texi	2012-08-17 18:30:37.209021746 +0100
@@ -1142,6 +1142,15 @@ called @dfn{specs}.
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
@@ -1177,6 +1186,7 @@ objective-c  objective-c-header  objecti
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
--- gcc-4.8-20120812/gcc/doc/sourcebuild.texi	2012-08-10 13:11:29.000000000 +0100
+++ gcc-4.8/gcc/doc/sourcebuild.texi	2012-08-17 18:30:37.249021748 +0100
@@ -114,6 +114,9 @@ The Objective-C and Objective-C++ runtim
 @item libquadmath
 The runtime support library for quad-precision math operations.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
--- gcc-4.8-20120812/gcc/doc/standards.texi	2011-12-21 17:53:58.000000000 +0000
+++ gcc-4.8/gcc/doc/standards.texi	2012-08-17 18:30:37.277021749 +0100
@@ -289,6 +289,16 @@ a specific version.  In general GCC trac
 closely, and any given release will support the language as of the
 date that the release was frozen.
 
+@section D language
+
+The D language continues to evolve as of this writing; see the
+@uref{http://golang.org/@/doc/@/go_spec.html, current language
+specifications}.  At present there are no specific versions of Go, and
+there is no way to describe the language supported by GCC in terms of
+a specific version.  In general GCC tracks the evolving specification
+closely, and any given release will support the language as of the
+date that the release was frozen.
+
 @section References for other languages
 
 @xref{Top, GNAT Reference Manual, About This Guide, gnat_rm,
--- gcc-4.8-20120812/gcc/dwarf2out.c	2012-07-24 18:31:01.000000000 +0100
+++ gcc-4.8/gcc/dwarf2out.c	2012-08-17 18:30:37.345021752 +0100
@@ -18199,6 +18199,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
--- gcc-4.8-20120812/gcc/gcc.c	2012-07-27 19:17:00.000000000 +0100
+++ gcc-4.8/gcc/gcc.c	2012-08-17 18:30:37.377021753 +0100
@@ -935,6 +935,7 @@ static const struct compiler default_com
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
