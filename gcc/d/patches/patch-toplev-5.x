This implements building of libphobos library in GCC.
---

--- a/Makefile.def
+++ b/Makefile.def
@@ -150,6 +150,7 @@ target_modules = { module= libquadmath; };
 target_modules = { module= libgfortran; };
 target_modules = { module= libobjc; };
 target_modules = { module= libgo; };
+target_modules = { module= libphobos; };
 target_modules = { module= libtermcap; no_check=true;
                    missing=mostlyclean;
                    missing=clean;
@@ -279,6 +280,7 @@ flags_to_pass = { flag= GCJ_FOR_TARGET ; };
 flags_to_pass = { flag= GFORTRAN_FOR_TARGET ; };
 flags_to_pass = { flag= GOC_FOR_TARGET ; };
 flags_to_pass = { flag= GOCFLAGS_FOR_TARGET ; };
+flags_to_pass = { flag= GDC_FOR_TARGET ; };
 flags_to_pass = { flag= LD_FOR_TARGET ; };
 flags_to_pass = { flag= LIPO_FOR_TARGET ; };
 flags_to_pass = { flag= LDFLAGS_FOR_TARGET ; };
@@ -541,6 +543,8 @@ dependencies = { module=configure-target-libgo; on=all-target-libstdc++-v3; };
 dependencies = { module=all-target-libgo; on=all-target-libbacktrace; };
 dependencies = { module=all-target-libgo; on=all-target-libffi; };
 dependencies = { module=all-target-libgo; on=all-target-libatomic; };
+dependencies = { module=configure-target-libphobos; on=configure-target-zlib; };
+dependencies = { module=all-target-libphobos; on=all-target-zlib; };
 dependencies = { module=configure-target-libjava; on=configure-target-zlib; };
 dependencies = { module=configure-target-libjava; on=configure-target-boehm-gc; };
 dependencies = { module=configure-target-libjava; on=configure-target-libffi; };
@@ -604,6 +608,8 @@ languages = { language=objc;	gcc-check-target=check-objc;
 languages = { language=obj-c++;	gcc-check-target=check-obj-c++; };
 languages = { language=go;	gcc-check-target=check-go;
 				lib-check-target=check-target-libgo; };
+languages = { language=d;	gcc-check-target=check-d;
+				lib-check-target=check-target-libphobos; };
 
 // Toplevel bootstrap
 bootstrap_stage = { id=1 ; };
--- a/Makefile.in
+++ b/Makefile.in
@@ -157,6 +157,7 @@ BUILD_EXPORTS = \
 	GFORTRAN="$(GFORTRAN_FOR_BUILD)"; export GFORTRAN; \
 	GOC="$(GOC_FOR_BUILD)"; export GOC; \
 	GOCFLAGS="$(GOCFLAGS_FOR_BUILD)"; export GOCFLAGS; \
+	GDC="$(GDC_FOR_BUILD)"; export GDC; \
 	DLLTOOL="$(DLLTOOL_FOR_BUILD)"; export DLLTOOL; \
 	LD="$(LD_FOR_BUILD)"; export LD; \
 	LDFLAGS="$(LDFLAGS_FOR_BUILD)"; export LDFLAGS; \
@@ -194,6 +195,7 @@ HOST_EXPORTS = \
 	GCJ="$(GCJ)"; export GCJ; \
 	GFORTRAN="$(GFORTRAN)"; export GFORTRAN; \
 	GOC="$(GOC)"; export GOC; \
+	GDC="$(GDC)"; export GDC; \
 	AR="$(AR)"; export AR; \
 	AS="$(AS)"; export AS; \
 	CC_FOR_BUILD="$(CC_FOR_BUILD)"; export CC_FOR_BUILD; \
@@ -280,6 +282,7 @@ BASE_TARGET_EXPORTS = \
 	GCJ="$(GCJ_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GCJ; \
 	GFORTRAN="$(GFORTRAN_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GFORTRAN; \
 	GOC="$(GOC_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GOC; \
+	GDC="$(GDC_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GDC; \
 	DLLTOOL="$(DLLTOOL_FOR_TARGET)"; export DLLTOOL; \
 	LD="$(COMPILER_LD_FOR_TARGET)"; export LD; \
 	LDFLAGS="$(LDFLAGS_FOR_TARGET)"; export LDFLAGS; \
@@ -347,6 +350,7 @@ DLLTOOL_FOR_BUILD = @DLLTOOL_FOR_BUILD@
 GCJ_FOR_BUILD = @GCJ_FOR_BUILD@
 GFORTRAN_FOR_BUILD = @GFORTRAN_FOR_BUILD@
 GOC_FOR_BUILD = @GOC_FOR_BUILD@
+GDC_FOR_BUILD = @GDC_FOR_BUILD@
 LDFLAGS_FOR_BUILD = @LDFLAGS_FOR_BUILD@
 LD_FOR_BUILD = @LD_FOR_BUILD@
 NM_FOR_BUILD = @NM_FOR_BUILD@
@@ -529,6 +533,7 @@ RAW_CXX_FOR_TARGET=$(STAGE_CC_WRAPPER) @RAW_CXX_FOR_TARGET@
 GCJ_FOR_TARGET=$(STAGE_CC_WRAPPER) @GCJ_FOR_TARGET@
 GFORTRAN_FOR_TARGET=$(STAGE_CC_WRAPPER) @GFORTRAN_FOR_TARGET@
 GOC_FOR_TARGET=$(STAGE_CC_WRAPPER) @GOC_FOR_TARGET@
+GDC_FOR_TARGET=$(STAGE_CC_WRAPPER) @GDC_FOR_TARGET@
 DLLTOOL_FOR_TARGET=@DLLTOOL_FOR_TARGET@
 LD_FOR_TARGET=@LD_FOR_TARGET@
 
@@ -751,6 +756,7 @@ BASE_FLAGS_TO_PASS = \
 	"GFORTRAN_FOR_TARGET=$(GFORTRAN_FOR_TARGET)" \
 	"GOC_FOR_TARGET=$(GOC_FOR_TARGET)" \
 	"GOCFLAGS_FOR_TARGET=$(GOCFLAGS_FOR_TARGET)" \
+	"GDC_FOR_TARGET=$(GDC_FOR_TARGET)" \
 	"LD_FOR_TARGET=$(LD_FOR_TARGET)" \
 	"LIPO_FOR_TARGET=$(LIPO_FOR_TARGET)" \
 	"LDFLAGS_FOR_TARGET=$(LDFLAGS_FOR_TARGET)" \
@@ -804,6 +810,7 @@ EXTRA_HOST_FLAGS = \
 	'GCJ=$(GCJ)' \
 	'GFORTRAN=$(GFORTRAN)' \
 	'GOC=$(GOC)' \
+	'GDC=$(GDC)' \
 	'LD=$(LD)' \
 	'LIPO=$(LIPO)' \
 	'NM=$(NM)' \
@@ -852,6 +859,7 @@ EXTRA_TARGET_FLAGS = \
 	'GFORTRAN=$$(GFORTRAN_FOR_TARGET) $$(XGCC_FLAGS_FOR_TARGET) $$(TFLAGS)' \
 	'GOC=$$(GOC_FOR_TARGET) $$(XGCC_FLAGS_FOR_TARGET) $$(TFLAGS)' \
 	'GOCFLAGS=$$(GOCFLAGS_FOR_TARGET)' \
+	'GDC=$$(GDC_FOR_TARGET) $$(XGCC_FLAGS_FOR_TARGET) $$(TFLAGS)' \
 	'LD=$(COMPILER_LD_FOR_TARGET)' \
 	'LDFLAGS=$$(LDFLAGS_FOR_TARGET)' \
 	'LIBCFLAGS=$$(LIBCFLAGS_FOR_TARGET)' \
@@ -954,6 +962,7 @@ configure-target:  \
     maybe-configure-target-libgfortran \
     maybe-configure-target-libobjc \
     maybe-configure-target-libgo \
+    maybe-configure-target-libphobos \
     maybe-configure-target-libtermcap \
     maybe-configure-target-winsup \
     maybe-configure-target-libgloss \
@@ -1119,6 +1128,7 @@ all-target: maybe-all-target-libquadmath
 all-target: maybe-all-target-libgfortran
 all-target: maybe-all-target-libobjc
 all-target: maybe-all-target-libgo
+all-target: maybe-all-target-libphobos
 all-target: maybe-all-target-libtermcap
 all-target: maybe-all-target-winsup
 all-target: maybe-all-target-libgloss
@@ -1213,6 +1223,7 @@ info-target: maybe-info-target-libquadmath
 info-target: maybe-info-target-libgfortran
 info-target: maybe-info-target-libobjc
 info-target: maybe-info-target-libgo
+info-target: maybe-info-target-libphobos
 info-target: maybe-info-target-libtermcap
 info-target: maybe-info-target-winsup
 info-target: maybe-info-target-libgloss
@@ -1300,6 +1311,7 @@ dvi-target: maybe-dvi-target-libquadmath
 dvi-target: maybe-dvi-target-libgfortran
 dvi-target: maybe-dvi-target-libobjc
 dvi-target: maybe-dvi-target-libgo
+dvi-target: maybe-dvi-target-libphobos
 dvi-target: maybe-dvi-target-libtermcap
 dvi-target: maybe-dvi-target-winsup
 dvi-target: maybe-dvi-target-libgloss
@@ -1387,6 +1399,7 @@ pdf-target: maybe-pdf-target-libquadmath
 pdf-target: maybe-pdf-target-libgfortran
 pdf-target: maybe-pdf-target-libobjc
 pdf-target: maybe-pdf-target-libgo
+pdf-target: maybe-pdf-target-libphobos
 pdf-target: maybe-pdf-target-libtermcap
 pdf-target: maybe-pdf-target-winsup
 pdf-target: maybe-pdf-target-libgloss
@@ -1474,6 +1487,7 @@ html-target: maybe-html-target-libquadmath
 html-target: maybe-html-target-libgfortran
 html-target: maybe-html-target-libobjc
 html-target: maybe-html-target-libgo
+html-target: maybe-html-target-libphobos
 html-target: maybe-html-target-libtermcap
 html-target: maybe-html-target-winsup
 html-target: maybe-html-target-libgloss
@@ -1561,6 +1575,7 @@ TAGS-target: maybe-TAGS-target-libquadmath
 TAGS-target: maybe-TAGS-target-libgfortran
 TAGS-target: maybe-TAGS-target-libobjc
 TAGS-target: maybe-TAGS-target-libgo
+TAGS-target: maybe-TAGS-target-libphobos
 TAGS-target: maybe-TAGS-target-libtermcap
 TAGS-target: maybe-TAGS-target-winsup
 TAGS-target: maybe-TAGS-target-libgloss
@@ -1648,6 +1663,7 @@ install-info-target: maybe-install-info-target-libquadmath
 install-info-target: maybe-install-info-target-libgfortran
 install-info-target: maybe-install-info-target-libobjc
 install-info-target: maybe-install-info-target-libgo
+install-info-target: maybe-install-info-target-libphobos
 install-info-target: maybe-install-info-target-libtermcap
 install-info-target: maybe-install-info-target-winsup
 install-info-target: maybe-install-info-target-libgloss
@@ -1735,6 +1751,7 @@ install-pdf-target: maybe-install-pdf-target-libquadmath
 install-pdf-target: maybe-install-pdf-target-libgfortran
 install-pdf-target: maybe-install-pdf-target-libobjc
 install-pdf-target: maybe-install-pdf-target-libgo
+install-pdf-target: maybe-install-pdf-target-libphobos
 install-pdf-target: maybe-install-pdf-target-libtermcap
 install-pdf-target: maybe-install-pdf-target-winsup
 install-pdf-target: maybe-install-pdf-target-libgloss
@@ -1822,6 +1839,7 @@ install-html-target: maybe-install-html-target-libquadmath
 install-html-target: maybe-install-html-target-libgfortran
 install-html-target: maybe-install-html-target-libobjc
 install-html-target: maybe-install-html-target-libgo
+install-html-target: maybe-install-html-target-libphobos
 install-html-target: maybe-install-html-target-libtermcap
 install-html-target: maybe-install-html-target-winsup
 install-html-target: maybe-install-html-target-libgloss
@@ -1909,6 +1927,7 @@ installcheck-target: maybe-installcheck-target-libquadmath
 installcheck-target: maybe-installcheck-target-libgfortran
 installcheck-target: maybe-installcheck-target-libobjc
 installcheck-target: maybe-installcheck-target-libgo
+installcheck-target: maybe-installcheck-target-libphobos
 installcheck-target: maybe-installcheck-target-libtermcap
 installcheck-target: maybe-installcheck-target-winsup
 installcheck-target: maybe-installcheck-target-libgloss
@@ -1996,6 +2015,7 @@ mostlyclean-target: maybe-mostlyclean-target-libquadmath
 mostlyclean-target: maybe-mostlyclean-target-libgfortran
 mostlyclean-target: maybe-mostlyclean-target-libobjc
 mostlyclean-target: maybe-mostlyclean-target-libgo
+mostlyclean-target: maybe-mostlyclean-target-libphobos
 mostlyclean-target: maybe-mostlyclean-target-libtermcap
 mostlyclean-target: maybe-mostlyclean-target-winsup
 mostlyclean-target: maybe-mostlyclean-target-libgloss
@@ -2083,6 +2103,7 @@ clean-target: maybe-clean-target-libquadmath
 clean-target: maybe-clean-target-libgfortran
 clean-target: maybe-clean-target-libobjc
 clean-target: maybe-clean-target-libgo
+clean-target: maybe-clean-target-libphobos
 clean-target: maybe-clean-target-libtermcap
 clean-target: maybe-clean-target-winsup
 clean-target: maybe-clean-target-libgloss
@@ -2170,6 +2191,7 @@ distclean-target: maybe-distclean-target-libquadmath
 distclean-target: maybe-distclean-target-libgfortran
 distclean-target: maybe-distclean-target-libobjc
 distclean-target: maybe-distclean-target-libgo
+distclean-target: maybe-distclean-target-libphobos
 distclean-target: maybe-distclean-target-libtermcap
 distclean-target: maybe-distclean-target-winsup
 distclean-target: maybe-distclean-target-libgloss
@@ -2257,6 +2279,7 @@ maintainer-clean-target: maybe-maintainer-clean-target-libquadmath
 maintainer-clean-target: maybe-maintainer-clean-target-libgfortran
 maintainer-clean-target: maybe-maintainer-clean-target-libobjc
 maintainer-clean-target: maybe-maintainer-clean-target-libgo
+maintainer-clean-target: maybe-maintainer-clean-target-libphobos
 maintainer-clean-target: maybe-maintainer-clean-target-libtermcap
 maintainer-clean-target: maybe-maintainer-clean-target-winsup
 maintainer-clean-target: maybe-maintainer-clean-target-libgloss
@@ -2399,6 +2422,7 @@ check-target:  \
     maybe-check-target-libgfortran \
     maybe-check-target-libobjc \
     maybe-check-target-libgo \
+    maybe-check-target-libphobos \
     maybe-check-target-libtermcap \
     maybe-check-target-winsup \
     maybe-check-target-libgloss \
@@ -2573,6 +2597,7 @@ install-target:  \
     maybe-install-target-libgfortran \
     maybe-install-target-libobjc \
     maybe-install-target-libgo \
+    maybe-install-target-libphobos \
     maybe-install-target-libtermcap \
     maybe-install-target-winsup \
     maybe-install-target-libgloss \
@@ -2680,6 +2705,7 @@ install-strip-target:  \
     maybe-install-strip-target-libgfortran \
     maybe-install-strip-target-libobjc \
     maybe-install-strip-target-libgo \
+    maybe-install-strip-target-libphobos \
     maybe-install-strip-target-libtermcap \
     maybe-install-strip-target-winsup \
     maybe-install-strip-target-libgloss \
@@ -41326,6 +41352,464 @@ maintainer-clean-target-libgo:
 
 
 
+.PHONY: configure-target-libphobos maybe-configure-target-libphobos
+maybe-configure-target-libphobos:
+@if gcc-bootstrap
+configure-target-libphobos: stage_current
+@endif gcc-bootstrap
+@if target-libphobos
+maybe-configure-target-libphobos: configure-target-libphobos
+configure-target-libphobos: 
+	@: $(MAKE); $(unstage)
+	@r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	echo "Checking multilib configuration for libphobos..."; \
+	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+	$(CC_FOR_TARGET) --print-multi-lib > $(TARGET_SUBDIR)/libphobos/multilib.tmp 2> /dev/null ; \
+	if test -r $(TARGET_SUBDIR)/libphobos/multilib.out; then \
+	  if cmp -s $(TARGET_SUBDIR)/libphobos/multilib.tmp $(TARGET_SUBDIR)/libphobos/multilib.out; then \
+	    rm -f $(TARGET_SUBDIR)/libphobos/multilib.tmp; \
+	  else \
+	    rm -f $(TARGET_SUBDIR)/libphobos/Makefile; \
+	    mv $(TARGET_SUBDIR)/libphobos/multilib.tmp $(TARGET_SUBDIR)/libphobos/multilib.out; \
+	  fi; \
+	else \
+	  mv $(TARGET_SUBDIR)/libphobos/multilib.tmp $(TARGET_SUBDIR)/libphobos/multilib.out; \
+	fi; \
+	test ! -f $(TARGET_SUBDIR)/libphobos/Makefile || exit 0; \
+	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+	$(NORMAL_TARGET_EXPORTS)  \
+	echo Configuring in $(TARGET_SUBDIR)/libphobos; \
+	cd "$(TARGET_SUBDIR)/libphobos" || exit 1; \
+	case $(srcdir) in \
+	  /* | [A-Za-z]:[\\/]*) topdir=$(srcdir) ;; \
+	  *) topdir=`echo $(TARGET_SUBDIR)/libphobos/ | \
+		sed -e 's,\./,,g' -e 's,[^/]*/,../,g' `$(srcdir) ;; \
+	esac; \
+	module_srcdir=libphobos; \
+	rm -f no-such-file || : ; \
+	CONFIG_SITE=no-such-file $(SHELL) \
+	  $$s/$$module_srcdir/configure \
+	  --srcdir=$${topdir}/$$module_srcdir \
+	  $(TARGET_CONFIGARGS) --build=${build_alias} --host=${target_alias} \
+	  --target=${target_alias}  \
+	  || exit 1
+@endif target-libphobos
+
+
+
+
+
+.PHONY: all-target-libphobos maybe-all-target-libphobos
+maybe-all-target-libphobos:
+@if gcc-bootstrap
+all-target-libphobos: stage_current
+@endif gcc-bootstrap
+@if target-libphobos
+TARGET-target-libphobos=all
+maybe-all-target-libphobos: all-target-libphobos
+all-target-libphobos: configure-target-libphobos
+	@: $(MAKE); $(unstage)
+	@r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS)  \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) $(EXTRA_TARGET_FLAGS)   \
+		$(TARGET-target-libphobos))
+@endif target-libphobos
+
+
+
+
+
+.PHONY: check-target-libphobos maybe-check-target-libphobos
+maybe-check-target-libphobos:
+@if target-libphobos
+maybe-check-target-libphobos: check-target-libphobos
+
+check-target-libphobos:
+	@: $(MAKE); $(unstage)
+	@r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(TARGET_FLAGS_TO_PASS)   check)
+
+@endif target-libphobos
+
+.PHONY: install-target-libphobos maybe-install-target-libphobos
+maybe-install-target-libphobos:
+@if target-libphobos
+maybe-install-target-libphobos: install-target-libphobos
+
+install-target-libphobos: installdirs
+	@: $(MAKE); $(unstage)
+	@r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  install)
+
+@endif target-libphobos
+
+.PHONY: install-strip-target-libphobos maybe-install-strip-target-libphobos
+maybe-install-strip-target-libphobos:
+@if target-libphobos
+maybe-install-strip-target-libphobos: install-strip-target-libphobos
+
+install-strip-target-libphobos: installdirs
+	@: $(MAKE); $(unstage)
+	@r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  install-strip)
+
+@endif target-libphobos
+
+# Other targets (info, dvi, pdf, etc.)
+
+.PHONY: maybe-info-target-libphobos info-target-libphobos
+maybe-info-target-libphobos:
+@if target-libphobos
+maybe-info-target-libphobos: info-target-libphobos
+
+info-target-libphobos: \
+    configure-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing info in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           info) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-dvi-target-libphobos dvi-target-libphobos
+maybe-dvi-target-libphobos:
+@if target-libphobos
+maybe-dvi-target-libphobos: dvi-target-libphobos
+
+dvi-target-libphobos: \
+    configure-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing dvi in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           dvi) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-pdf-target-libphobos pdf-target-libphobos
+maybe-pdf-target-libphobos:
+@if target-libphobos
+maybe-pdf-target-libphobos: pdf-target-libphobos
+
+pdf-target-libphobos: \
+    configure-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing pdf in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           pdf) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-html-target-libphobos html-target-libphobos
+maybe-html-target-libphobos:
+@if target-libphobos
+maybe-html-target-libphobos: html-target-libphobos
+
+html-target-libphobos: \
+    configure-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing html in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           html) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-TAGS-target-libphobos TAGS-target-libphobos
+maybe-TAGS-target-libphobos:
+@if target-libphobos
+maybe-TAGS-target-libphobos: TAGS-target-libphobos
+
+TAGS-target-libphobos: \
+    configure-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing TAGS in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           TAGS) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-install-info-target-libphobos install-info-target-libphobos
+maybe-install-info-target-libphobos:
+@if target-libphobos
+maybe-install-info-target-libphobos: install-info-target-libphobos
+
+install-info-target-libphobos: \
+    configure-target-libphobos \
+    info-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing install-info in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           install-info) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-install-pdf-target-libphobos install-pdf-target-libphobos
+maybe-install-pdf-target-libphobos:
+@if target-libphobos
+maybe-install-pdf-target-libphobos: install-pdf-target-libphobos
+
+install-pdf-target-libphobos: \
+    configure-target-libphobos \
+    pdf-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing install-pdf in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           install-pdf) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-install-html-target-libphobos install-html-target-libphobos
+maybe-install-html-target-libphobos:
+@if target-libphobos
+maybe-install-html-target-libphobos: install-html-target-libphobos
+
+install-html-target-libphobos: \
+    configure-target-libphobos \
+    html-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing install-html in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           install-html) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-installcheck-target-libphobos installcheck-target-libphobos
+maybe-installcheck-target-libphobos:
+@if target-libphobos
+maybe-installcheck-target-libphobos: installcheck-target-libphobos
+
+installcheck-target-libphobos: \
+    configure-target-libphobos 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing installcheck in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           installcheck) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-mostlyclean-target-libphobos mostlyclean-target-libphobos
+maybe-mostlyclean-target-libphobos:
+@if target-libphobos
+maybe-mostlyclean-target-libphobos: mostlyclean-target-libphobos
+
+mostlyclean-target-libphobos: 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing mostlyclean in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           mostlyclean) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-clean-target-libphobos clean-target-libphobos
+maybe-clean-target-libphobos:
+@if target-libphobos
+maybe-clean-target-libphobos: clean-target-libphobos
+
+clean-target-libphobos: 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing clean in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           clean) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-distclean-target-libphobos distclean-target-libphobos
+maybe-distclean-target-libphobos:
+@if target-libphobos
+maybe-distclean-target-libphobos: distclean-target-libphobos
+
+distclean-target-libphobos: 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing distclean in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           distclean) \
+	  || exit 1
+
+@endif target-libphobos
+
+.PHONY: maybe-maintainer-clean-target-libphobos maintainer-clean-target-libphobos
+maybe-maintainer-clean-target-libphobos:
+@if target-libphobos
+maybe-maintainer-clean-target-libphobos: maintainer-clean-target-libphobos
+
+maintainer-clean-target-libphobos: 
+	@: $(MAKE); $(unstage)
+	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(NORMAL_TARGET_EXPORTS) \
+	echo "Doing maintainer-clean in $(TARGET_SUBDIR)/libphobos" ; \
+	for flag in $(EXTRA_TARGET_FLAGS); do \
+	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+	done; \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+	          "RANLIB=$${RANLIB}" \
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+	           maintainer-clean) \
+	  || exit 1
+
+@endif target-libphobos
+
+
+
+
+
 .PHONY: configure-target-libtermcap maybe-configure-target-libtermcap
 maybe-configure-target-libtermcap:
 @if gcc-bootstrap
@@ -47356,6 +47840,14 @@ check-gcc-go:
 	(cd gcc && $(MAKE) $(GCC_FLAGS_TO_PASS) check-go);
 check-go: check-gcc-go check-target-libgo
 
+.PHONY: check-gcc-d check-d
+check-gcc-d:
+	r=`${PWD_COMMAND}`; export r; \
+	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+	$(HOST_EXPORTS) \
+	(cd gcc && $(MAKE) $(GCC_FLAGS_TO_PASS) check-d);
+check-d: check-gcc-d check-target-libphobos
+
 
 # The gcc part of install-no-fixedincludes, which relies on an intimate
 # knowledge of how a number of gcc internal targets (inter)operate.  Delegate.
@@ -49492,6 +49984,7 @@ configure-target-libquadmath: stage_last
 configure-target-libgfortran: stage_last
 configure-target-libobjc: stage_last
 configure-target-libgo: stage_last
+configure-target-libphobos: stage_last
 configure-target-libtermcap: stage_last
 configure-target-winsup: stage_last
 configure-target-libgloss: stage_last
@@ -49526,6 +50019,7 @@ configure-target-libquadmath: maybe-all-gcc
 configure-target-libgfortran: maybe-all-gcc
 configure-target-libobjc: maybe-all-gcc
 configure-target-libgo: maybe-all-gcc
+configure-target-libphobos: maybe-all-gcc
 configure-target-libtermcap: maybe-all-gcc
 configure-target-winsup: maybe-all-gcc
 configure-target-libgloss: maybe-all-gcc
@@ -50319,6 +50813,8 @@ configure-target-libgo: maybe-all-target-libstdc++-v3
 all-target-libgo: maybe-all-target-libbacktrace
 all-target-libgo: maybe-all-target-libffi
 all-target-libgo: maybe-all-target-libatomic
+configure-target-libphobos: maybe-configure-target-zlib
+all-target-libphobos: maybe-all-target-zlib
 configure-target-libjava: maybe-configure-target-zlib
 configure-target-libjava: maybe-configure-target-boehm-gc
 configure-target-libjava: maybe-configure-target-libffi
@@ -50436,6 +50932,7 @@ configure-target-libquadmath: maybe-all-target-libgcc
 configure-target-libgfortran: maybe-all-target-libgcc
 configure-target-libobjc: maybe-all-target-libgcc
 configure-target-libgo: maybe-all-target-libgcc
+configure-target-libphobos: maybe-all-target-libgcc
 configure-target-libtermcap: maybe-all-target-libgcc
 configure-target-winsup: maybe-all-target-libgcc
 configure-target-libgloss: maybe-all-target-libgcc
@@ -50479,6 +50976,8 @@ configure-target-libobjc: maybe-all-target-newlib maybe-all-target-libgloss
 
 configure-target-libgo: maybe-all-target-newlib maybe-all-target-libgloss
 
+configure-target-libphobos: maybe-all-target-newlib maybe-all-target-libgloss
+
 configure-target-libtermcap: maybe-all-target-newlib maybe-all-target-libgloss
 
 configure-target-winsup: maybe-all-target-newlib maybe-all-target-libgloss
--- a/Makefile.tpl
+++ b/Makefile.tpl
@@ -160,6 +160,7 @@ BUILD_EXPORTS = \
 	GFORTRAN="$(GFORTRAN_FOR_BUILD)"; export GFORTRAN; \
 	GOC="$(GOC_FOR_BUILD)"; export GOC; \
 	GOCFLAGS="$(GOCFLAGS_FOR_BUILD)"; export GOCFLAGS; \
+	GDC="$(GDC_FOR_BUILD)"; export GDC; \
 	DLLTOOL="$(DLLTOOL_FOR_BUILD)"; export DLLTOOL; \
 	LD="$(LD_FOR_BUILD)"; export LD; \
 	LDFLAGS="$(LDFLAGS_FOR_BUILD)"; export LDFLAGS; \
@@ -197,6 +198,7 @@ HOST_EXPORTS = \
 	GCJ="$(GCJ)"; export GCJ; \
 	GFORTRAN="$(GFORTRAN)"; export GFORTRAN; \
 	GOC="$(GOC)"; export GOC; \
+	GDC="$(GDC)"; export GDC; \
 	AR="$(AR)"; export AR; \
 	AS="$(AS)"; export AS; \
 	CC_FOR_BUILD="$(CC_FOR_BUILD)"; export CC_FOR_BUILD; \
@@ -283,6 +285,7 @@ BASE_TARGET_EXPORTS = \
 	GCJ="$(GCJ_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GCJ; \
 	GFORTRAN="$(GFORTRAN_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GFORTRAN; \
 	GOC="$(GOC_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GOC; \
+	GDC="$(GDC_FOR_TARGET) $(XGCC_FLAGS_FOR_TARGET) $$TFLAGS"; export GDC; \
 	DLLTOOL="$(DLLTOOL_FOR_TARGET)"; export DLLTOOL; \
 	LD="$(COMPILER_LD_FOR_TARGET)"; export LD; \
 	LDFLAGS="$(LDFLAGS_FOR_TARGET)"; export LDFLAGS; \
@@ -350,6 +353,7 @@ DLLTOOL_FOR_BUILD = @DLLTOOL_FOR_BUILD@
 GCJ_FOR_BUILD = @GCJ_FOR_BUILD@
 GFORTRAN_FOR_BUILD = @GFORTRAN_FOR_BUILD@
 GOC_FOR_BUILD = @GOC_FOR_BUILD@
+GDC_FOR_BUILD = @GDC_FOR_BUILD@
 LDFLAGS_FOR_BUILD = @LDFLAGS_FOR_BUILD@
 LD_FOR_BUILD = @LD_FOR_BUILD@
 NM_FOR_BUILD = @NM_FOR_BUILD@
@@ -482,6 +486,7 @@ RAW_CXX_FOR_TARGET=$(STAGE_CC_WRAPPER) @RAW_CXX_FOR_TARGET@
 GCJ_FOR_TARGET=$(STAGE_CC_WRAPPER) @GCJ_FOR_TARGET@
 GFORTRAN_FOR_TARGET=$(STAGE_CC_WRAPPER) @GFORTRAN_FOR_TARGET@
 GOC_FOR_TARGET=$(STAGE_CC_WRAPPER) @GOC_FOR_TARGET@
+GDC_FOR_TARGET=$(STAGE_CC_WRAPPER) @GDC_FOR_TARGET@
 DLLTOOL_FOR_TARGET=@DLLTOOL_FOR_TARGET@
 LD_FOR_TARGET=@LD_FOR_TARGET@
 
@@ -605,6 +610,7 @@ EXTRA_HOST_FLAGS = \
 	'GCJ=$(GCJ)' \
 	'GFORTRAN=$(GFORTRAN)' \
 	'GOC=$(GOC)' \
+	'GDC=$(GDC)' \
 	'LD=$(LD)' \
 	'LIPO=$(LIPO)' \
 	'NM=$(NM)' \
@@ -653,6 +659,7 @@ EXTRA_TARGET_FLAGS = \
 	'GFORTRAN=$$(GFORTRAN_FOR_TARGET) $$(XGCC_FLAGS_FOR_TARGET) $$(TFLAGS)' \
 	'GOC=$$(GOC_FOR_TARGET) $$(XGCC_FLAGS_FOR_TARGET) $$(TFLAGS)' \
 	'GOCFLAGS=$$(GOCFLAGS_FOR_TARGET)' \
+	'GDC=$$(GDC_FOR_TARGET) $$(XGCC_FLAGS_FOR_TARGET) $$(TFLAGS)' \
 	'LD=$(COMPILER_LD_FOR_TARGET)' \
 	'LDFLAGS=$$(LDFLAGS_FOR_TARGET)' \
 	'LIBCFLAGS=$$(LIBCFLAGS_FOR_TARGET)' \
--- a/config-ml.in
+++ b/config-ml.in
@@ -513,6 +513,7 @@ multi-do:
 				exec_prefix="$(exec_prefix)" \
 				GCJFLAGS="$(GCJFLAGS) $${flags}" \
 				GOCFLAGS="$(GOCFLAGS) $${flags}" \
+				GDCFLAGS="$(GDCFLAGS) $${flags}" \
 				CXXFLAGS="$(CXXFLAGS) $${flags}" \
 				LIBCFLAGS="$(LIBCFLAGS) $${flags}" \
 				LIBCXXFLAGS="$(LIBCXXFLAGS) $${flags}" \
@@ -746,7 +747,7 @@ if [ -n "${multidirs}" ] && [ -z "${ml_norecursion}" ]; then
         break
       fi
     done
-    ml_config_env='CC="${CC_}$flags" CXX="${CXX_}$flags" F77="${F77_}$flags" GCJ="${GCJ_}$flags" GFORTRAN="${GFORTRAN_}$flags" GOC="${GOC_}$flags"'
+    ml_config_env='CC="${CC_}$flags" CXX="${CXX_}$flags" F77="${F77_}$flags" GCJ="${GCJ_}$flags" GFORTRAN="${GFORTRAN_}$flags" GOC="${GOC_}$flags" GDC="${GDC_}$flags"'
 
     if [ "${with_target_subdir}" = "." ]; then
 	CC_=$CC' '
@@ -755,6 +756,7 @@ if [ -n "${multidirs}" ] && [ -z "${ml_norecursion}" ]; then
 	GCJ_=$GCJ' '
 	GFORTRAN_=$GFORTRAN' '
 	GOC_=$GOC' '
+	GDC_=$GDC' '
     else
 	# Create a regular expression that matches any string as long
 	# as ML_POPDIR.
@@ -831,6 +833,18 @@ if [ -n "${multidirs}" ] && [ -z "${ml_norecursion}" ]; then
 	  esac
 	done
 
+	GDC_=
+	for arg in ${GDC}; do
+	  case $arg in
+	  -[BIL]"${ML_POPDIR}"/*)
+	    GDC_="${GDC_}"`echo "X${arg}" | sed -n "s/X\\(-[BIL]${popdir_rx}\\).*/\\1/p"`/${ml_dir}`echo "X${arg}" | sed -n "s/X-[BIL]${popdir_rx}\\(.*\\)/\\1/p"`' ' ;;
+	  "${ML_POPDIR}"/*)
+	    GDC_="${GDC_}"`echo "X${arg}" | sed -n "s/X\\(${popdir_rx}\\).*/\\1/p"`/${ml_dir}`echo "X${arg}" | sed -n "s/X${popdir_rx}\\(.*\\)/\\1/p"`' ' ;;
+	  *)
+	    GDC_="${GDC_}${arg} " ;;
+	  esac
+	done
+
 	if test "x${LD_LIBRARY_PATH+set}" = xset; then
 	  LD_LIBRARY_PATH_=
 	  for arg in `echo "$LD_LIBRARY_PATH" | tr ':' ' '`; do
--- a/config/multi.m4
+++ b/config/multi.m4
@@ -65,4 +65,6 @@ CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
 CC="$CC"
 CXX="$CXX"
 GFORTRAN="$GFORTRAN"
-GCJ="$GCJ"])])dnl
+GCJ="$GCJ"
+GDC="$GDC"])])dnl
+
--- a/configure
+++ b/configure
@@ -581,6 +581,7 @@ LD_FOR_TARGET
 DLLTOOL_FOR_TARGET
 AS_FOR_TARGET
 AR_FOR_TARGET
+GDC_FOR_TARGET
 GOC_FOR_TARGET
 GFORTRAN_FOR_TARGET
 GCJ_FOR_TARGET
@@ -614,6 +615,7 @@ RANLIB_FOR_BUILD
 NM_FOR_BUILD
 LD_FOR_BUILD
 LDFLAGS_FOR_BUILD
+GDC_FOR_BUILD
 GOC_FOR_BUILD
 GFORTRAN_FOR_BUILD
 GCJ_FOR_BUILD
@@ -828,6 +830,7 @@ GCC_FOR_TARGET
 GCJ_FOR_TARGET
 GFORTRAN_FOR_TARGET
 GOC_FOR_TARGET
+GDC_FOR_TARGET
 AR_FOR_TARGET
 AS_FOR_TARGET
 DLLTOOL_FOR_TARGET
@@ -1601,6 +1604,8 @@ Some influential environment variables:
               GFORTRAN for the target
   GOC_FOR_TARGET
               GOC for the target
+  GDC_FOR_TARGET
+              GDC for the target
   AR_FOR_TARGET
               AR for the target
   AS_FOR_TARGET
@@ -2759,7 +2764,8 @@ target_libraries="target-libgcc \
 		${libgcj} \
 		target-libobjc \
 		target-libada \
-		target-libgo"
+		target-libgo \
+		target-libphobos"
 
 # these tools are built using the target libraries, and are intended to
 # run only in the target environment
@@ -4015,6 +4021,7 @@ else
   GCJ_FOR_BUILD="\$(GCJ)"
   GFORTRAN_FOR_BUILD="\$(GFORTRAN)"
   GOC_FOR_BUILD="\$(GOC)"
+  GDC_FOR_BUILD="\$(GDC)"
   DLLTOOL_FOR_BUILD="\$(DLLTOOL)"
   LD_FOR_BUILD="\$(LD)"
   NM_FOR_BUILD="\$(NM)"
@@ -7624,6 +7631,7 @@ done
 
 
 
+
 # Generate default definitions for YACC, M4, LEX and other programs that run
 # on the build machine.  These are used if the Makefile can't locate these
 # programs in objdir.
@@ -10839,6 +10847,167 @@ fi
 
 
 
+if test -n "$GDC_FOR_TARGET"; then
+  ac_cv_prog_GDC_FOR_TARGET=$GDC_FOR_TARGET
+elif test -n "$ac_cv_prog_GDC_FOR_TARGET"; then
+  GDC_FOR_TARGET=$ac_cv_prog_GDC_FOR_TARGET
+fi
+
+if test -n "$ac_cv_prog_GDC_FOR_TARGET"; then
+  for ncn_progname in gdc; do
+    # Extract the first word of "${ncn_progname}", so it can be a program name with args.
+set dummy ${ncn_progname}; ac_word=$2
+{ $as_echo "$as_me:${as_lineno-$LINENO}: checking for $ac_word" >&5
+$as_echo_n "checking for $ac_word... " >&6; }
+if test "${ac_cv_prog_GDC_FOR_TARGET+set}" = set; then :
+  $as_echo_n "(cached) " >&6
+else
+  if test -n "$GDC_FOR_TARGET"; then
+  ac_cv_prog_GDC_FOR_TARGET="$GDC_FOR_TARGET" # Let the user override the test.
+else
+as_save_IFS=$IFS; IFS=$PATH_SEPARATOR
+for as_dir in $PATH
+do
+  IFS=$as_save_IFS
+  test -z "$as_dir" && as_dir=.
+    for ac_exec_ext in '' $ac_executable_extensions; do
+  if { test -f "$as_dir/$ac_word$ac_exec_ext" && $as_test_x "$as_dir/$ac_word$ac_exec_ext"; }; then
+    ac_cv_prog_GDC_FOR_TARGET="${ncn_progname}"
+    $as_echo "$as_me:${as_lineno-$LINENO}: found $as_dir/$ac_word$ac_exec_ext" >&5
+    break 2
+  fi
+done
+  done
+IFS=$as_save_IFS
+
+fi
+fi
+GDC_FOR_TARGET=$ac_cv_prog_GDC_FOR_TARGET
+if test -n "$GDC_FOR_TARGET"; then
+  { $as_echo "$as_me:${as_lineno-$LINENO}: result: $GDC_FOR_TARGET" >&5
+$as_echo "$GDC_FOR_TARGET" >&6; }
+else
+  { $as_echo "$as_me:${as_lineno-$LINENO}: result: no" >&5
+$as_echo "no" >&6; }
+fi
+
+
+  done
+fi
+
+if test -z "$ac_cv_prog_GDC_FOR_TARGET" && test -n "$with_build_time_tools"; then
+  for ncn_progname in gdc; do
+    { $as_echo "$as_me:${as_lineno-$LINENO}: checking for ${ncn_progname} in $with_build_time_tools" >&5
+$as_echo_n "checking for ${ncn_progname} in $with_build_time_tools... " >&6; }
+    if test -x $with_build_time_tools/${ncn_progname}; then
+      ac_cv_prog_GDC_FOR_TARGET=$with_build_time_tools/${ncn_progname}
+      { $as_echo "$as_me:${as_lineno-$LINENO}: result: yes" >&5
+$as_echo "yes" >&6; }
+      break
+    else
+      { $as_echo "$as_me:${as_lineno-$LINENO}: result: no" >&5
+$as_echo "no" >&6; }
+    fi
+  done
+fi
+
+if test -z "$ac_cv_prog_GDC_FOR_TARGET"; then
+  for ncn_progname in gdc; do
+    if test -n "$ncn_target_tool_prefix"; then
+      # Extract the first word of "${ncn_target_tool_prefix}${ncn_progname}", so it can be a program name with args.
+set dummy ${ncn_target_tool_prefix}${ncn_progname}; ac_word=$2
+{ $as_echo "$as_me:${as_lineno-$LINENO}: checking for $ac_word" >&5
+$as_echo_n "checking for $ac_word... " >&6; }
+if test "${ac_cv_prog_GDC_FOR_TARGET+set}" = set; then :
+  $as_echo_n "(cached) " >&6
+else
+  if test -n "$GDC_FOR_TARGET"; then
+  ac_cv_prog_GDC_FOR_TARGET="$GDC_FOR_TARGET" # Let the user override the test.
+else
+as_save_IFS=$IFS; IFS=$PATH_SEPARATOR
+for as_dir in $PATH
+do
+  IFS=$as_save_IFS
+  test -z "$as_dir" && as_dir=.
+    for ac_exec_ext in '' $ac_executable_extensions; do
+  if { test -f "$as_dir/$ac_word$ac_exec_ext" && $as_test_x "$as_dir/$ac_word$ac_exec_ext"; }; then
+    ac_cv_prog_GDC_FOR_TARGET="${ncn_target_tool_prefix}${ncn_progname}"
+    $as_echo "$as_me:${as_lineno-$LINENO}: found $as_dir/$ac_word$ac_exec_ext" >&5
+    break 2
+  fi
+done
+  done
+IFS=$as_save_IFS
+
+fi
+fi
+GDC_FOR_TARGET=$ac_cv_prog_GDC_FOR_TARGET
+if test -n "$GDC_FOR_TARGET"; then
+  { $as_echo "$as_me:${as_lineno-$LINENO}: result: $GDC_FOR_TARGET" >&5
+$as_echo "$GDC_FOR_TARGET" >&6; }
+else
+  { $as_echo "$as_me:${as_lineno-$LINENO}: result: no" >&5
+$as_echo "no" >&6; }
+fi
+
+
+    fi
+    if test -z "$ac_cv_prog_GDC_FOR_TARGET" && test $build = $target ; then
+      # Extract the first word of "${ncn_progname}", so it can be a program name with args.
+set dummy ${ncn_progname}; ac_word=$2
+{ $as_echo "$as_me:${as_lineno-$LINENO}: checking for $ac_word" >&5
+$as_echo_n "checking for $ac_word... " >&6; }
+if test "${ac_cv_prog_GDC_FOR_TARGET+set}" = set; then :
+  $as_echo_n "(cached) " >&6
+else
+  if test -n "$GDC_FOR_TARGET"; then
+  ac_cv_prog_GDC_FOR_TARGET="$GDC_FOR_TARGET" # Let the user override the test.
+else
+as_save_IFS=$IFS; IFS=$PATH_SEPARATOR
+for as_dir in $PATH
+do
+  IFS=$as_save_IFS
+  test -z "$as_dir" && as_dir=.
+    for ac_exec_ext in '' $ac_executable_extensions; do
+  if { test -f "$as_dir/$ac_word$ac_exec_ext" && $as_test_x "$as_dir/$ac_word$ac_exec_ext"; }; then
+    ac_cv_prog_GDC_FOR_TARGET="${ncn_progname}"
+    $as_echo "$as_me:${as_lineno-$LINENO}: found $as_dir/$ac_word$ac_exec_ext" >&5
+    break 2
+  fi
+done
+  done
+IFS=$as_save_IFS
+
+fi
+fi
+GDC_FOR_TARGET=$ac_cv_prog_GDC_FOR_TARGET
+if test -n "$GDC_FOR_TARGET"; then
+  { $as_echo "$as_me:${as_lineno-$LINENO}: result: $GDC_FOR_TARGET" >&5
+$as_echo "$GDC_FOR_TARGET" >&6; }
+else
+  { $as_echo "$as_me:${as_lineno-$LINENO}: result: no" >&5
+$as_echo "no" >&6; }
+fi
+
+
+    fi
+    test -n "$ac_cv_prog_GDC_FOR_TARGET" && break
+  done
+fi
+
+if test -z "$ac_cv_prog_GDC_FOR_TARGET" ; then
+  set dummy gdc
+  if test $build = $target ; then
+    GDC_FOR_TARGET="$2"
+  else
+    GDC_FOR_TARGET="${ncn_target_tool_prefix}$2"
+  fi
+else
+  GDC_FOR_TARGET="$ac_cv_prog_GDC_FOR_TARGET"
+fi
+
+
+
 cat > conftest.c << \EOF
 #ifdef __GNUC__
   gcc_yay;
@@ -14280,6 +14449,51 @@ $as_echo "pre-installed" >&6; }
   fi
 fi
 
+{ $as_echo "$as_me:${as_lineno-$LINENO}: checking where to find the target gdc" >&5
+$as_echo_n "checking where to find the target gdc... " >&6; }
+if test "x${build}" != "x${host}" ; then
+  if expr "x$GDC_FOR_TARGET" : "x/" > /dev/null; then
+    # We already found the complete path
+    ac_dir=`dirname $GDC_FOR_TARGET`
+    { $as_echo "$as_me:${as_lineno-$LINENO}: result: pre-installed in $ac_dir" >&5
+$as_echo "pre-installed in $ac_dir" >&6; }
+  else
+    # Canadian cross, just use what we found
+    { $as_echo "$as_me:${as_lineno-$LINENO}: result: pre-installed" >&5
+$as_echo "pre-installed" >&6; }
+  fi
+else
+  ok=yes
+  case " ${configdirs} " in
+    *" gcc "*) ;;
+    *) ok=no ;;
+  esac
+  case ,${enable_languages}, in
+    *,d,*) ;;
+    *) ok=no ;;
+  esac
+  if test $ok = yes; then
+    # An in-tree tool is available and we can use it
+    GDC_FOR_TARGET='$$r/$(HOST_SUBDIR)/gcc/gdc -B$$r/$(HOST_SUBDIR)/gcc/'
+    { $as_echo "$as_me:${as_lineno-$LINENO}: result: just compiled" >&5
+$as_echo "just compiled" >&6; }
+  elif expr "x$GDC_FOR_TARGET" : "x/" > /dev/null; then
+    # We already found the complete path
+    ac_dir=`dirname $GDC_FOR_TARGET`
+    { $as_echo "$as_me:${as_lineno-$LINENO}: result: pre-installed in $ac_dir" >&5
+$as_echo "pre-installed in $ac_dir" >&6; }
+  elif test "x$target" = "x$host"; then
+    # We can use an host tool
+    GDC_FOR_TARGET='$(GDC)'
+    { $as_echo "$as_me:${as_lineno-$LINENO}: result: host tool" >&5
+$as_echo "host tool" >&6; }
+  else
+    # We need a cross tool
+    { $as_echo "$as_me:${as_lineno-$LINENO}: result: pre-installed" >&5
+$as_echo "pre-installed" >&6; }
+  fi
+fi
+
 { $as_echo "$as_me:${as_lineno-$LINENO}: checking where to find the target ld" >&5
 $as_echo_n "checking where to find the target ld... " >&6; }
 if test "x${build}" != "x${host}" ; then
--- a/configure.ac
+++ b/configure.ac
@@ -170,7 +170,8 @@ target_libraries="target-libgcc \
 		${libgcj} \
 		target-libobjc \
 		target-libada \
-		target-libgo"
+		target-libgo \
+		target-libphobos"
 
 # these tools are built using the target libraries, and are intended to
 # run only in the target environment
@@ -1343,6 +1344,7 @@ else
   GCJ_FOR_BUILD="\$(GCJ)"
   GFORTRAN_FOR_BUILD="\$(GFORTRAN)"
   GOC_FOR_BUILD="\$(GOC)"
+  GDC_FOR_BUILD="\$(GDC)"
   DLLTOOL_FOR_BUILD="\$(DLLTOOL)"
   LD_FOR_BUILD="\$(LD)"
   NM_FOR_BUILD="\$(NM)"
@@ -3257,6 +3259,7 @@ AC_SUBST(DLLTOOL_FOR_BUILD)
 AC_SUBST(GCJ_FOR_BUILD)
 AC_SUBST(GFORTRAN_FOR_BUILD)
 AC_SUBST(GOC_FOR_BUILD)
+AC_SUBST(GDC_FOR_BUILD)
 AC_SUBST(LDFLAGS_FOR_BUILD)
 AC_SUBST(LD_FOR_BUILD)
 AC_SUBST(NM_FOR_BUILD)
@@ -3367,6 +3370,7 @@ NCN_STRICT_CHECK_TARGET_TOOLS(GCC_FOR_TARGET, gcc, ${CC_FOR_TARGET})
 NCN_STRICT_CHECK_TARGET_TOOLS(GCJ_FOR_TARGET, gcj)
 NCN_STRICT_CHECK_TARGET_TOOLS(GFORTRAN_FOR_TARGET, gfortran)
 NCN_STRICT_CHECK_TARGET_TOOLS(GOC_FOR_TARGET, gccgo)
+NCN_STRICT_CHECK_TARGET_TOOLS(GDC_FOR_TARGET, gdc)
 
 ACX_CHECK_INSTALLED_TARGET_TOOL(AR_FOR_TARGET, ar)
 ACX_CHECK_INSTALLED_TARGET_TOOL(AS_FOR_TARGET, as)
@@ -3402,6 +3406,8 @@ GCC_TARGET_TOOL(gfortran, GFORTRAN_FOR_TARGET, GFORTRAN,
 		[gcc/gfortran -B$$r/$(HOST_SUBDIR)/gcc/], fortran)
 GCC_TARGET_TOOL(gccgo, GOC_FOR_TARGET, GOC,
 		[gcc/gccgo -B$$r/$(HOST_SUBDIR)/gcc/], go)
+GCC_TARGET_TOOL(gdc, GDC_FOR_TARGET, GDC,
+		[gcc/gdc -B$$r/$(HOST_SUBDIR)/gcc/], d)
 GCC_TARGET_TOOL(ld, LD_FOR_TARGET, LD, [ld/ld-new])
 GCC_TARGET_TOOL(lipo, LIPO_FOR_TARGET, LIPO)
 GCC_TARGET_TOOL(nm, NM_FOR_TARGET, NM, [binutils/nm-new])
