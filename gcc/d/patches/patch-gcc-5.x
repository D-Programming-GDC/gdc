This implements D language support in the GCC back end, and adds
relevant documentation about the GDC front end.
---

--- a/gcc/config/rs6000/rs6000.c
+++ b/gcc/config/rs6000/rs6000.c
@@ -25738,7 +25738,8 @@ rs6000_output_function_epilogue (FILE *file,
       if (lang_GNU_C ()
 	  || ! strcmp (language_string, "GNU GIMPLE")
 	  || ! strcmp (language_string, "GNU Go")
-	  || ! strcmp (language_string, "libgccjit"))
+	  || ! strcmp (language_string, "libgccjit")
+	  || ! strcmp (language_string, "GNU D"))
 	i = 0;
       else if (! strcmp (language_string, "GNU F77")
 	       || lang_GNU_Fortran ())
--- a/gcc/doc/frontends.texi
+++ b/gcc/doc/frontends.texi
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
--- a/gcc/doc/install.texi
+++ b/gcc/doc/install.texi
@@ -1547,12 +1547,12 @@ their runtime libraries should be built.  For a list of valid values for
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
--- a/gcc/doc/invoke.texi
+++ b/gcc/doc/invoke.texi
@@ -1259,6 +1259,15 @@ called @dfn{specs}.
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
@@ -1294,6 +1303,7 @@ objective-c  objective-c-header  objective-c-cpp-output
 objective-c++ objective-c++-header objective-c++-cpp-output
 assembler  assembler-with-cpp
 ada
+d
 f77  f77-cpp-input f95  f95-cpp-input
 go
 java
--- a/gcc/doc/sourcebuild.texi
+++ b/gcc/doc/sourcebuild.texi
@@ -109,6 +109,9 @@ The Objective-C and Objective-C++ runtime library.
 @item libquadmath
 The runtime support library for quad-precision math operations.
 
+@item libphobos
+The D standard runtime library.
+
 @item libssp
 The Stack protector runtime library.
 
--- a/gcc/doc/standards.texi
+++ b/gcc/doc/standards.texi
@@ -280,6 +280,16 @@ available online, see @uref{http://gcc.gnu.org/readings.html}
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
 @section References for Other Languages
 
 @xref{Top, GNAT Reference Manual, About This Guide, gnat_rm,
--- a/gcc/dwarf2out.c
+++ b/gcc/dwarf2out.c
@@ -4756,6 +4756,15 @@ is_ada (void)
   return lang == DW_LANG_Ada95 || lang == DW_LANG_Ada83;
 }
 
+/* Return TRUE if the language is D.  */
+static inline bool
+is_dlang (void)
+{
+  unsigned int lang = get_AT_unsigned (comp_unit_die (), DW_AT_language);
+
+  return lang == DW_LANG_D;
+}
+
 /* Remove the specified attribute if present.  */
 
 static void
@@ -19844,6 +19853,8 @@ gen_compile_unit_die (const char *filename)
 	language = DW_LANG_ObjC;
       else if (strcmp (language_string, "GNU Objective-C++") == 0)
 	language = DW_LANG_ObjC_plus_plus;
+      else if (strcmp (language_string, "GNU D") == 0)
+	language = DW_LANG_D;
       else if (dwarf_version >= 5 || !dwarf_strict)
 	{
 	  if (strcmp (language_string, "GNU Go") == 0)
@@ -20813,7 +20824,7 @@ declare_in_namespace (tree thing, dw_die_ref context_die)
 
   if (ns_context != context_die)
     {
-      if (is_fortran ())
+      if (is_fortran () || is_dlang ())
 	return ns_context;
       if (DECL_P (thing))
 	gen_decl_die (thing, NULL, ns_context);
@@ -20836,7 +20847,7 @@ gen_namespace_die (tree decl, dw_die_ref context_die)
     {
       /* Output a real namespace or module.  */
       context_die = setup_namespace_context (decl, comp_unit_die ());
-      namespace_die = new_die (is_fortran ()
+      namespace_die = new_die (is_fortran () || is_dlang ()
 			       ? DW_TAG_module : DW_TAG_namespace,
 			       context_die, decl);
       /* For Fortran modules defined in different CU don't add src coords.  */
@@ -20899,7 +20910,7 @@ gen_decl_die (tree decl, tree origin, dw_die_ref context_die)
       break;
 
     case CONST_DECL:
-      if (!is_fortran () && !is_ada ())
+      if (!is_fortran () && !is_ada () && !is_dlang ())
 	{
 	  /* The individual enumerators of an enum type get output when we output
 	     the Dwarf representation of the relevant enum type itself.  */
@@ -21370,7 +21381,7 @@ dwarf2out_decl (tree decl)
     case CONST_DECL:
       if (debug_info_level <= DINFO_LEVEL_TERSE)
 	return;
-      if (!is_fortran () && !is_ada ())
+      if (!is_fortran () && !is_ada () && !is_dlang ())
 	return;
       if (TREE_STATIC (decl) && decl_function_context (decl))
 	context_die = lookup_decl_die (DECL_CONTEXT (decl));
--- a/gcc/gcc.c
+++ b/gcc/gcc.c
@@ -1102,6 +1102,7 @@ static const struct compiler default_compilers[] =
   {".java", "#Java", 0, 0, 0}, {".class", "#Java", 0, 0, 0},
   {".zip", "#Java", 0, 0, 0}, {".jar", "#Java", 0, 0, 0},
   {".go", "#Go", 0, 1, 0},
+  {".d", "#D", 0, 1, 0}, {".dd", "#D", 0, 1, 0}, {".di", "#D", 0, 1, 0},
   /* Next come the entries for C.  */
   {".c", "@c", 0, 0, 1},
   {"@c",
--- a/gcc/po/EXCLUDES
+++ b/gcc/po/EXCLUDES
@@ -53,3 +53,43 @@ genrecog.c
 gensupport.c
 gensupport.h
 read-md.c
+
+#   These files are part of the front end to D, and have no i18n support.
+d/dfrontend/arrayop.c
+d/dfrontend/attrib.c
+d/dfrontend/canthrow.c
+d/dfrontend/cond.c
+d/dfrontend/constfold.c
+d/dfrontend/cppmangle.c
+d/dfrontend/ctfeexpr.c
+d/dfrontend/dcast.c
+d/dfrontend/dclass.c
+d/dfrontend/declaration.c
+d/dfrontend/denum.c
+d/dfrontend/dimport.c
+d/dfrontend/dinterpret.c
+d/dfrontend/dmangle.c
+d/dfrontend/dmodule.c
+d/dfrontend/doc.c
+d/dfrontend/dscope.c
+d/dfrontend/dstruct.c
+d/dfrontend/dsymbol.c
+d/dfrontend/dtemplate.c
+d/dfrontend/dversion.c
+d/dfrontend/expression.c
+d/dfrontend/func.c
+d/dfrontend/init.c
+d/dfrontend/inline.c
+d/dfrontend/lexer.c
+d/dfrontend/mtype.c
+d/dfrontend/nogc.c
+d/dfrontend/nspace.c
+d/dfrontend/objc.c
+d/dfrontend/opover.c
+d/dfrontend/optimize.c
+d/dfrontend/parse.c
+d/dfrontend/sideeffect.c
+d/dfrontend/statement.c
+d/dfrontend/statementsem.c
+d/dfrontend/staticassert.c
+d/dfrontend/traits.c
