2018-09-18  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-lang.cc (deps_write): Use toChars accessor to get module path.
	* decl.cc (get_symbol_decl): Use get_identifier_with_length to get
	mangle override identifier node.

2018-09-10  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-lang.cc (d_init_options): Set-up global.params.argv0 as D array.
	(d_parse_file): Use global.params.argv0 pointer field as format value.
	* intrinsics.cc (maybe_expand_intrinsic): Handle INTRINSIC_EXP.
	* intrinsics.def (EXP): Add CTFE intrinsic.

2018-09-07  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-lang.cc: Include errors.h, mars.h.
	* decl.cc: Include errors.h.
	* typeinfo.cc: Include globals.h, errors.h.
	* verstr.h: Update to 2.082.0

2018-09-04  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-frontend.h: Remove file, and all sources that include it.
	* d-lang.cc: Include dmd/doc.h and dmd/mangle.h.
	* d-target.cc: Include dmd/mangle.h.
	* decl.cc: Include dmd/mangle.h.

2018-08-25  Iain Buclaw  <ibuclaw@gdcproject.org>

	* Make-lang.in (D_FRONTEND_OBJS): Add iasm.o, iasmgcc.o
	(d.tags): Scan dmd/root/*.h
	* d-builtins.cc (build_frontend_type): Update callers for new
	front-end signatures.
	(d_init_versions): Add D_ModuleInfo, D_Exceptions, D_TypeInfo.
	* d-diagnostic.cc (vwarning): Increment gagged warnings error if
	gagging turned on.
	(vdeprecation): Likewise.
	* d-frontend.cc (asmSemantic): Remove function.
	* d-lang.cc (d_handle_option): Remove case for OPT_fproperty.
	* d-target.cc (Target::_init): Remove int64Mangle and uint64Mangle.
	* lang.opt (fproperty): Remove option.
	* toir.cc (IRVisitor::visit(ExtAsmStatement)): Rename override to
	GccAsmStatement.
	* typeinfo.cc (TypeInfoVisitor::visit(TypeInfoClassDeclaration)): Use
	int for collecting ClassFlags.
	* (TypeInfoVisitor::visit(TypeInfoClassDeclaration)): Use int for
	collecting StructFlags.

2018-07-13  Iain Buclaw  <ibuclaw@gdcproject.org>

	* expr.cc (ExprVisitor::visit(CmpExp)): Remove lowering of static and
	dynamic array comparisons.
	* runtime.def (ADCMP2): Remove define.
	(SWITCH_STRING): Likewise.
	(SWITCH_USTRING): Likewise.
	(SWITCH_DSTRING): Likewise.
	(SWITCH_ERROR): Likewise.
	* toir.cc (IRVisitor::visit(SwitchStatement)): Remove lowering of
	string switch statements.
	(IRVisitor::visit(SwitchErrorStatement)): Remove lowering of throwing
	SwitchErrors.

2018-07-12  Iain Buclaw  <ibuclaw@gdcproject.org>

	* Make-lang.in (CHECKING_DFLAGS): New variable.
	(ALL_DFLAGS): Add -frelease when front-end tree checking is disabled.

2018-07-08  Iain Buclaw  <ibuclaw@gdcproject.org>

	* verstr.h: Update to 2.081.1

2018-07-08  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-lang.cc: Include id.h.

2018-07-04  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-lang.cc (d_handle_option): Handle options -ftransition=dip1008 and
	-ftransition=intpromote.
	* lang.opt (ftransition=dip1008): New option.
	(ftransition=intpromote): New options.

2018-07-04  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-builtins.cc (d_init_versions): Update condition for enabling
	version assert.
	* d-lang.cc (d_init_options): Remove setting of flags that are default
	initialized statically.
	(d_init_options_struct): Update condition for setting bounds check.
	(d_handle_option): Update condition for setting flags for enabling
	asserts and switch errors.
	(d_post_options): Likewise.
	* expr.cc (ExprVisitor::visit(AssertExp)): Update condition for
	generating assert code.

2018-07-01  Iain Buclaw  <ibuclaw@gdcproject.org>

	* verstr.h: Update to 2.081.0-rc.1

2018-07-01  Iain Buclaw  <ibuclaw@gdcproject.org>

	* decl.cc (walk_pragma_cdtor): New function.
	(DeclVisitor::visit(PragmaDeclaration)): Handle pragma crt_constructor
	and crt_destructor.

2018-07-01  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-target.cc (Target::_init): Set int64Mangle, uint64Mangle,
	twoDtorInVariable.
	* typeinfo.cc (TypeInfoVisitor::visit(TypeInfoClassDeclaration)): Use
	tidtor symbol for destructor.
	(TypeInfoVisitor::visit(TypeInfoClassDeclaration)): Likewise.

2018-06-29  Iain Buclaw  <ibuclaw@gdcproject.org>

	* Make-lang.in (WARN_DFLAGS): New variable.
	(ALL_DFLAGS): Use coverage and warn flags.

2018-06-27  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-builtins.cc (d_init_versions): Add version D_BetterC.
	* d-codegen.cc (find_aggregate_field): Move to decl.cc
	(build_class_instance): Move to decl.cc, make static.
	* d-frontend.cc (getTypeInfoType): Issue warning when -fno-rtti.
	* d-lang.cc (d_init): Check for global useExceptions.
	(d_handle_option): Handle OPT_fdruntime, OPT_fexceptions, OPT_frtti.
	(d_post_options): Set flags if -fno-druntime was given.
	* d-tree.h (build_class_instance): Remove declaration.
	(have_typeinfo_p): Add declaration.
	(build_typeinfo): Update signature.
	* decl.cc (DeclVisitor::finish_vtable): New function.
	(DeclVisitor::visit(StructDeclaration)): Generate typeinfo only if
	found in library.
	(DeclVisitor::visit(EnumDeclaration)): Likewise.
	(DeclVisitor::visit(InterfaceDeclaration)): Likewise.
	(DeclVisitor::visit(ClassDeclaration)): Likewise.
	Exit early if semantic error occurred during final semantic.
	* decl.cc: Update all callers of build_typeinfo.
	* lang.opt (fdruntime): New option.
	(fmoduleinfo): Add flag for option.
	(frtti): New option.
	* modules.cc (build_module_tree): Check for global useModuleInfo.
	* toir.cc (IRVisitor::visit(ThrowStatement)): Check for global
	useExceptions.
	* typeinfo.cc: Include options.h.
	(make_frontend_typeinfo): Set members and storage class fields on
	compiler-generated typeinfo.
	(have_typeinfo_p): New function.
	(TypeInfoVisitor::layout_base): Add reference to vtable only if
	typeinfo found in library.
	(TypeInfoVisitor::visit): Update all callers of build_typeinfo.
	(TypeInfoVisitor::visit(TypeInfoClassDeclaration)): Always set RTInfo
	field, even if null.
	(build_typeinfo): Add error if -fno-rtti passed on commandline.

2018-06-24  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-tree.h (lang_identifier): Add daggregate field.
	(IDENTIFIER_DAGGREGATE): New macro.
	(mangle_decl): Declare.
	* decl.cc (mangle_decl): Remove static linkage.
	* types.cc (TypeVisitor::visit(TypeStruct)): Handle duplicate
	declarations of type symbol.
	(TypeVisitor::visit(TypeClass)): Likewise.

2018-06-22  Iain Buclaw  <ibuclaw@gdcproject.org>

	* verstr.h: Update to 2.081.0-beta.2

2018-06-22  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-builtins.cc (d_init_versions): Replace BOUNDSCHECK enum values
	with CHECKENABLE.
	* d-codegen.cc (array_bounds_check): Likewise.
	* d-frontend.cc (Global::init_): Don't set params initialized by the
	frontend.
	* d-lang.cc (d_init_options): Update initialization of global struct.
	(d_handle_option): Replace BOUNDSCHECK enum values with CHECKENABLE.
	Update handling of debug and version identifiers.
	(d_post_options): Replace BOUNDSCHECK enum values with CHECKENABLE.
	Handle debug and version identifiers given on the command line.
	(d_parse_file): Use global versionids to get full list of predefined
	identifiers.

2018-06-22  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-frontend.cc (Global::startGagging): Remove function.
	(Global::endGagging): Remove function.
	(Global::increaseErrorCount): Remove function.
	(Loc::equals): Remove function.
	(retStyle): Remove function.
	(getTypeInfoType): Update signature.
	* d-target.cc (Target::isVectorOpSupported): Don't handle unordered
	expressions.
	(Target::prefixName): Remove function.
	(Target::cppParameterType): New function.
	(Target::isReturnOnStack): New function.

2018-06-22  Iain Buclaw  <ibuclaw@gdcproject.org>

	* decl.cc (DeclVisitor::visit(ClassDeclaration)): Use
	ClassDeclaration::vtblSymbol to access vtable symbol.
	(get_vtable_decl): Likewise.

2018-06-22  Iain Buclaw  <ibuclaw@gdcproject.org>

	* d-builtins.cc (build_frontend_type): Update call to
	TypeVector::create.  Use Type::merge2 to complete type.
	(maybe_set_builtin_1): Update call to AttribDeclaration::include.
	* d-codegen.cc (declaration_type): Use Type::merge2 to complete type.
	(type_passed_as): Likewise.
	* d-convert.cc (convert_expr): Use ClassDeclaration::isCPPclass.
	* d-frontend.cc (genCmain): Use new semantic entrypoints.
	* d-lang.cc (d_parse_file): Likewise.
	(d_build_eh_runtime_type): Use ClassDeclaration::isCPPclass.
	* decl.cc (DeclVisitor::visit(AttribDeclaration)): Update call to
	AttribDeclaration::include.
	(get_symbol_decl): Replace PROT enum values with Prot.
	* expr.cc (ExprVisitor::visit): Merge AndAndExp and OrOrExp into
	LogicalExp visitor method.
	* modules.cc (get_internal_fn): Replace PROT enum value with Prot.
	* toir.cc (IRVisitor::visit): Use ClassDecalration::isCPPclass.
	* typeinfo.cc (make_frontend_typeinfo): Use new semantic entrypoints.
	(TypeInfoVisitor::visit): Use Type::merge2 to complete type.
	* types.cc (layout_aggregate_members): Update call to
	AttribDeclaration::include.
	(layout_aggregate_type): Use ClassDeclaration::isCPPclass.

2018-06-22  Iain Buclaw  <ibuclaw@gdcproject.org>

	* Makefile.in (d.mostlyclean): Remove cleanup of verstr.h.

2018-06-20  Iain Buclaw  <ibuclaw@gdcproject.org>

	* Makefile.in (D_FRONTEND_OBJS): Add compiler.o, ctorflow.o,
	dsymbolsem.o, lambdacomp.o, longdouble.o, parsetimevisitor.o,
	permissivevisitor.o, port.o, semantic2.o, semantic3.o,
	templateparamsem.o, transitivevisitor.o
	(D_INCLUDES): Rename ddmd to dmd.
	(d/%.o): Likewise.

2018-06-16  Iain Buclaw  <ibuclaw@gdcproject.org>

	* Makefile.in (DMD_WARN_CXXFLAGS, DMD_COMPILE)
	(DMDGEN_COMPILE): Remove variables.
	(ALL_DFLAGS, DCOMPILE.base, DCOMPILE, DPOSTCOMPILE, DLINKER)
	(DLLINKER): New variables.
	(D_FRONTEND_OBJS): Add new frontend objects.
	(D_GENERATED_SRCS, D_GENERATED_OBJS): Remove variables.
	(D_ALL_OBJS): Remove D_GENERATED_OBJS.
	(cc1d): Use DLLINKER command to produce compiler.
	(d.mostlyclean): Remove generated sources.
	(CFLAGS-d/id.o, CFLAGS-d/impcnvtab.o): Remove recipes.
	(d/%.o): Use DCOMPILE and DPOSTCOMPILE to build frontend.
	(d/idgen, d/impcvgen, d/id.c, d/id.h, d/impcnvtab.c)
	(d/verstr.h): Remove recipes.
	* config-lang.in (boot_language): New variable.
	* d-frontend.cc (inlineCopy): Remove function.
	(global): Remove variable.
	* d-diagnostics.cc (error, errorSupplemental): Remove functions.
	(warning, warningSupplemental): Likewise.
	(deprecation, deprecationSupplemental): Likewise.
	* d-lang.cc (d_init_options): Initialize D runtime.
	* d-longdouble.cc (CTFloat::zero, CTFloat::one, CTFloat::minusone)
	(CTFloat::half): Remove variables.
	* d-target.cc (Target::ptrsize, Target::c_longsize, Target::realsize)
	(Target::realpad, Target::realalignsize, Target::reverseCppOverloads)
	(Target::cppExceptions, Target::classinfosize)
	(Target::maxStaticDataSize): Remove variables.
	* verstr.h: New file.

Copyright (C) 2018 Free Software Foundation, Inc.

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.
