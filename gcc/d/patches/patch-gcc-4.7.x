diff -ur gcc~/config/rs6000/rs6000.c gcc/config/rs6000/rs6000.c
--- gcc~/config/rs6000/rs6000.c	2012-12-09 11:33:43.594200576 +0100
+++ gcc/config/rs6000/rs6000.c	2012-12-09 11:41:33.073886990 +0100
@@ -21066,7 +21066,8 @@
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
diff -ur gcc~/doc/frontends.texi gcc/doc/frontends.texi
--- gcc~/doc/frontends.texi	2012-12-09 11:33:43.514199278 +0100
+++ gcc/doc/frontends.texi	2012-12-09 11:36:00.904434463 +0100
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
diff -ur gcc~/doc/install.texi gcc/doc/install.texi
--- gcc~/doc/install.texi	2012-12-09 11:33:43.523199424 +0100
+++ gcc/doc/install.texi	2012-12-09 11:36:59.238390013 +0100
@@ -1360,12 +1360,12 @@
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
diff -ur gcc~/doc/invoke.texi gcc/doc/invoke.texi
--- gcc~/doc/invoke.texi	2012-12-09 11:33:43.518199342 +0100
+++ gcc/doc/invoke.texi	2012-12-09 11:38:01.859417176 +0100
@@ -1133,6 +1133,15 @@
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
@@ -1168,6 +1177,7 @@
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
diff -ur gcc~/doc/sourcebuild.texi gcc/doc/sourcebuild.texi
--- gcc~/doc/sourcebuild.texi	2012-12-09 11:33:43.519199358 +0100
+++ gcc/doc/sourcebuild.texi	2012-12-09 11:39:12.633579298 +0100
@@ -104,6 +104,9 @@
 @item libobjc
 The Objective-C and Objective-C++ runtime library.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
diff -ur gcc~/doc/standards.texi gcc/doc/standards.texi
--- gcc~/doc/standards.texi	2012-12-09 11:33:43.523199424 +0100
+++ gcc/doc/standards.texi	2012-12-09 11:39:54.322264574 +0100
@@ -289,6 +289,16 @@
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
diff -ur gcc~/dwarf2out.c gcc/dwarf2out.c
--- gcc~/dwarf2out.c	2012-12-09 11:33:43.673201860 +0100
+++ gcc/dwarf2out.c	2012-12-09 11:34:43.033163998 +0100
@@ -18491,6 +18491,8 @@
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
diff -ur gcc~/gcc.c gcc/gcc.c
--- gcc~/gcc.c	2012-12-09 11:33:43.524199440 +0100
+++ gcc/gcc.c	2012-12-09 11:42:06.193431771 +0100
@@ -934,6 +934,7 @@
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
