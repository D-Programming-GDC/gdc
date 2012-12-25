--- gcc/config/i386/i386.h
+++ gcc/config/i386/i386.h
@@ -583,6 +583,14 @@ extern const char *host_detect_local_cpu (int argc, const char **argv);
 /* Target CPU builtins.  */
 #define TARGET_CPU_CPP_BUILTINS() ix86_target_macros ()
 
+#define TARGET_CPU_D_BUILTINS()                 \
+do {                                            \
+  if (TARGET_64BIT)                             \
+    builtin_define("X86_64");                   \
+  else                                          \
+    builtin_define("X86");                      \
+} while (0)
+
 /* Target Pragmas.  */
 #define REGISTER_TARGET_PRAGMAS() ix86_register_pragmas ()
 
--- gcc/config/mips/mips.h
+++ gcc/config/mips/mips.h
@@ -554,6 +554,51 @@ struct mips_cpu_info {
     }									\
   while (0)
 
+
+#define TARGET_CPU_D_BUILTINS()                 \
+  do                                            \
+    {                                           \
+      builtin_define("MIPS");                   \
+      if (TARGET_64BIT)                         \
+        builtin_define("MIPS64");               \
+      else                                      \
+        builtin_define("MIPS32");               \
+                                                \
+      switch (mips_abi)                         \
+        {                                       \
+        case ABI_32:                            \
+          builtin_define("MIPS_O32");           \
+          break;                                \
+                                                \
+        case ABI_O64:                           \
+          builtin_define("MIPS_O64");           \
+          break;                                \
+                                                \
+        case ABI_N32:                           \
+          builtin_define("MIPS_N32");           \
+          break;                                \
+                                                \
+        case ABI_64:                            \
+          builtin_define("MIPS_N64");           \
+          break;                                \
+                                                \
+        case ABI_EABI:                          \
+          builtin_define("MIPS_EABI");          \
+          break;                                \
+                                                \
+        default:                                \
+          gcc_unreachable();                    \
+        }                                       \
+                                                \
+      if (TARGET_NO_FLOAT)                      \
+        builtin_define("MIPS_NoFloat");         \
+      else if (TARGET_HARD_FLOAT_ABI)           \
+        builtin_define("MIPS_HardFloat");       \
+      else                                      \
+        builtin_define("MIPS_SoftFloat");       \
+    }                                           \
+  while (0)
+
 /* Default target_flags if no switches are specified  */
 
 #ifndef TARGET_DEFAULT
--- gcc/config/rs6000/rs6000.c	2012-10-07 03:07:42.000000000 +0100
+++ gcc/config/rs6000/rs6000.c	2012-10-14 10:25:56.025866841 +0100
@@ -21418,7 +21418,8 @@ rs6000_output_function_epilogue (FILE *f
 	 either, so for now use 0.  */
       if (! strcmp (language_string, "GNU C")
 	  || ! strcmp (language_string, "GNU GIMPLE")
-	  || ! strcmp (language_string, "GNU Go"))
+	  || ! strcmp (language_string, "GNU Go")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || ! strcmp (language_string, "GNU Fortran"))
--- gcc/doc/frontends.texi	2011-01-03 20:52:22.000000000 +0000
+++ gcc/doc/frontends.texi	2012-10-14 10:25:56.029866841 +0100
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
--- gcc/doc/install.texi	2012-09-26 23:47:22.000000000 +0100
+++ gcc/doc/install.texi	2012-10-14 10:25:56.033866841 +0100
@@ -1345,12 +1345,12 @@ their runtime libraries should be built.
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
--- gcc/doc/invoke.texi	2012-10-06 15:06:04.000000000 +0100
+++ gcc/doc/invoke.texi	2012-10-14 10:25:56.045866839 +0100
@@ -1149,6 +1149,15 @@ called @dfn{specs}.
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
@@ -1184,6 +1193,7 @@ objective-c  objective-c-header  objecti
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
--- gcc/doc/sourcebuild.texi	2012-09-24 16:15:14.000000000 +0100
+++ gcc/doc/sourcebuild.texi	2012-10-14 10:25:56.049866840 +0100
@@ -114,6 +114,9 @@ The Objective-C and Objective-C++ runtim
 @item libquadmath
 The runtime support library for quad-precision math operations.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
--- gcc/doc/standards.texi	2011-12-21 17:53:58.000000000 +0000
+++ gcc/doc/standards.texi	2012-10-14 10:25:56.049866840 +0100
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
--- gcc/dwarf2out.c	2012-10-04 12:23:18.000000000 +0100
+++ gcc/dwarf2out.c	2012-10-14 10:25:56.061866840 +0100
@@ -18242,6 +18242,8 @@ gen_compile_unit_die (const char *filena
   language = DW_LANG_C89;
   if (strcmp (language_string, "GNU C++") == 0)
     language = DW_LANG_C_plus_plus;
+  else if (strcmp (language_string, "GNU D") == 0)
+    language = DW_LANG_D;
   else if (strcmp (language_string, "GNU F77") == 0)
     language = DW_LANG_Fortran77;
   else if (strcmp (language_string, "GNU Pascal") == 0)
--- gcc/gcc.c	2012-10-04 18:01:31.000000000 +0100
+++ gcc/gcc.c	2012-10-14 10:25:56.065866839 +0100
@@ -935,6 +935,7 @@ static const struct compiler default_com
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
