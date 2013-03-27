This implements building of libphobos library in GCC.
---

--- gcc/configure	2013-02-15 17:45:54.000000000 +0000
+++ gcc/configure	2013-03-20 16:26:18.442235719 +0000
@@ -2781,7 +2781,8 @@ target_libraries="target-libgcc \
 		${libgcj} \
 		target-libobjc \
 		target-libada \
-		target-libgo"
+		target-libgo \
+		target-libphobos"
 
 # these tools are built using the target libraries, and are intended to
 # run only in the target environment
--- gcc/configure.ac	2013-02-15 17:45:54.000000000 +0000
+++ gcc/configure.ac	2013-03-20 16:26:18.462235720 +0000
@@ -168,7 +168,8 @@ target_libraries="target-libgcc \
 		${libgcj} \
 		target-libobjc \
 		target-libada \
-		target-libgo"
+		target-libgo \
+		target-libphobos"
 
 # these tools are built using the target libraries, and are intended to
 # run only in the target environment
--- gcc/Makefile.def	2013-01-14 16:15:21.000000000 +0000
+++ gcc/Makefile.def	2013-03-20 16:26:18.466235720 +0000
@@ -131,6 +131,7 @@ target_modules = { module= libquadmath;
 target_modules = { module= libgfortran; };
 target_modules = { module= libobjc; };
 target_modules = { module= libgo; };
+target_modules = { module= libphobos; };
 target_modules = { module= libtermcap; no_check=true;
                    missing=mostlyclean;
                    missing=clean;
@@ -505,6 +506,8 @@ dependencies = { module=configure-target
 dependencies = { module=all-target-libgo; on=all-target-libbacktrace; };
 dependencies = { module=all-target-libgo; on=all-target-libffi; };
 dependencies = { module=all-target-libgo; on=all-target-libatomic; };
+dependencies = { module=configure-target-libphobos; on=configure-target-zlib; };
+dependencies = { module=all-target-libphobos; on=all-target-zlib; };
 dependencies = { module=configure-target-libjava; on=configure-target-zlib; };
 dependencies = { module=configure-target-libjava; on=configure-target-boehm-gc; };
 dependencies = { module=configure-target-libjava; on=configure-target-libffi; };
@@ -560,6 +563,8 @@ languages = { language=objc;	gcc-check-t
 languages = { language=obj-c++;	gcc-check-target=check-obj-c++; };
 languages = { language=go;	gcc-check-target=check-go;
 				lib-check-target=check-target-libgo; };
+languages = { language=d;	gcc-check-target=check-d;
+				lib-check-target=check-target-libphobos; };
 
 // Toplevel bootstrap
 bootstrap_stage = { id=1 ; };
--- gcc/Makefile.in	2013-01-14 16:15:21.000000000 +0000
+++ gcc/Makefile.in	2013-03-20 16:26:18.514235720 +0000
@@ -933,6 +933,7 @@ configure-target:  \
     maybe-configure-target-libgfortran \
     maybe-configure-target-libobjc \
     maybe-configure-target-libgo \
+    maybe-configure-target-libphobos \
     maybe-configure-target-libtermcap \
     maybe-configure-target-winsup \
     maybe-configure-target-libgloss \
@@ -1086,6 +1087,7 @@ all-target: maybe-all-target-libquadmath
 all-target: maybe-all-target-libgfortran
 all-target: maybe-all-target-libobjc
 all-target: maybe-all-target-libgo
+all-target: maybe-all-target-libphobos
 all-target: maybe-all-target-libtermcap
 all-target: maybe-all-target-winsup
 all-target: maybe-all-target-libgloss
@@ -1175,6 +1177,7 @@ info-target: maybe-info-target-libquadma
 info-target: maybe-info-target-libgfortran
 info-target: maybe-info-target-libobjc
 info-target: maybe-info-target-libgo
+info-target: maybe-info-target-libphobos
 info-target: maybe-info-target-libtermcap
 info-target: maybe-info-target-winsup
 info-target: maybe-info-target-libgloss
@@ -1257,6 +1260,7 @@ dvi-target: maybe-dvi-target-libquadmath
 dvi-target: maybe-dvi-target-libgfortran
 dvi-target: maybe-dvi-target-libobjc
 dvi-target: maybe-dvi-target-libgo
+dvi-target: maybe-dvi-target-libphobos
 dvi-target: maybe-dvi-target-libtermcap
 dvi-target: maybe-dvi-target-winsup
 dvi-target: maybe-dvi-target-libgloss
@@ -1339,6 +1343,7 @@ pdf-target: maybe-pdf-target-libquadmath
 pdf-target: maybe-pdf-target-libgfortran
 pdf-target: maybe-pdf-target-libobjc
 pdf-target: maybe-pdf-target-libgo
+pdf-target: maybe-pdf-target-libphobos
 pdf-target: maybe-pdf-target-libtermcap
 pdf-target: maybe-pdf-target-winsup
 pdf-target: maybe-pdf-target-libgloss
@@ -1421,6 +1426,7 @@ html-target: maybe-html-target-libquadma
 html-target: maybe-html-target-libgfortran
 html-target: maybe-html-target-libobjc
 html-target: maybe-html-target-libgo
+html-target: maybe-html-target-libphobos
 html-target: maybe-html-target-libtermcap
 html-target: maybe-html-target-winsup
 html-target: maybe-html-target-libgloss
@@ -1503,6 +1509,7 @@ TAGS-target: maybe-TAGS-target-libquadma
 TAGS-target: maybe-TAGS-target-libgfortran
 TAGS-target: maybe-TAGS-target-libobjc
 TAGS-target: maybe-TAGS-target-libgo
+TAGS-target: maybe-TAGS-target-libphobos
 TAGS-target: maybe-TAGS-target-libtermcap
 TAGS-target: maybe-TAGS-target-winsup
 TAGS-target: maybe-TAGS-target-libgloss
@@ -1585,6 +1592,7 @@ install-info-target: maybe-install-info-
 install-info-target: maybe-install-info-target-libgfortran
 install-info-target: maybe-install-info-target-libobjc
 install-info-target: maybe-install-info-target-libgo
+install-info-target: maybe-install-info-target-libphobos
 install-info-target: maybe-install-info-target-libtermcap
 install-info-target: maybe-install-info-target-winsup
 install-info-target: maybe-install-info-target-libgloss
@@ -1667,6 +1675,7 @@ install-pdf-target: maybe-install-pdf-ta
 install-pdf-target: maybe-install-pdf-target-libgfortran
 install-pdf-target: maybe-install-pdf-target-libobjc
 install-pdf-target: maybe-install-pdf-target-libgo
+install-pdf-target: maybe-install-pdf-target-libphobos
 install-pdf-target: maybe-install-pdf-target-libtermcap
 install-pdf-target: maybe-install-pdf-target-winsup
 install-pdf-target: maybe-install-pdf-target-libgloss
@@ -1749,6 +1758,7 @@ install-html-target: maybe-install-html-
 install-html-target: maybe-install-html-target-libgfortran
 install-html-target: maybe-install-html-target-libobjc
 install-html-target: maybe-install-html-target-libgo
+install-html-target: maybe-install-html-target-libphobos
 install-html-target: maybe-install-html-target-libtermcap
 install-html-target: maybe-install-html-target-winsup
 install-html-target: maybe-install-html-target-libgloss
@@ -1831,6 +1841,7 @@ installcheck-target: maybe-installcheck-
 installcheck-target: maybe-installcheck-target-libgfortran
 installcheck-target: maybe-installcheck-target-libobjc
 installcheck-target: maybe-installcheck-target-libgo
+installcheck-target: maybe-installcheck-target-libphobos
 installcheck-target: maybe-installcheck-target-libtermcap
 installcheck-target: maybe-installcheck-target-winsup
 installcheck-target: maybe-installcheck-target-libgloss
@@ -1913,6 +1924,7 @@ mostlyclean-target: maybe-mostlyclean-ta
 mostlyclean-target: maybe-mostlyclean-target-libgfortran
 mostlyclean-target: maybe-mostlyclean-target-libobjc
 mostlyclean-target: maybe-mostlyclean-target-libgo
+mostlyclean-target: maybe-mostlyclean-target-libphobos
 mostlyclean-target: maybe-mostlyclean-target-libtermcap
 mostlyclean-target: maybe-mostlyclean-target-winsup
 mostlyclean-target: maybe-mostlyclean-target-libgloss
@@ -1995,6 +2007,7 @@ clean-target: maybe-clean-target-libquad
 clean-target: maybe-clean-target-libgfortran
 clean-target: maybe-clean-target-libobjc
 clean-target: maybe-clean-target-libgo
+clean-target: maybe-clean-target-libphobos
 clean-target: maybe-clean-target-libtermcap
 clean-target: maybe-clean-target-winsup
 clean-target: maybe-clean-target-libgloss
@@ -2077,6 +2090,7 @@ distclean-target: maybe-distclean-target
 distclean-target: maybe-distclean-target-libgfortran
 distclean-target: maybe-distclean-target-libobjc
 distclean-target: maybe-distclean-target-libgo
+distclean-target: maybe-distclean-target-libphobos
 distclean-target: maybe-distclean-target-libtermcap
 distclean-target: maybe-distclean-target-winsup
 distclean-target: maybe-distclean-target-libgloss
@@ -2159,6 +2173,7 @@ maintainer-clean-target: maybe-maintaine
 maintainer-clean-target: maybe-maintainer-clean-target-libgfortran
 maintainer-clean-target: maybe-maintainer-clean-target-libobjc
 maintainer-clean-target: maybe-maintainer-clean-target-libgo
+maintainer-clean-target: maybe-maintainer-clean-target-libphobos
 maintainer-clean-target: maybe-maintainer-clean-target-libtermcap
 maintainer-clean-target: maybe-maintainer-clean-target-winsup
 maintainer-clean-target: maybe-maintainer-clean-target-libgloss
@@ -2296,6 +2311,7 @@ check-target:  \
     maybe-check-target-libgfortran \
     maybe-check-target-libobjc \
     maybe-check-target-libgo \
+    maybe-check-target-libphobos \
     maybe-check-target-libtermcap \
     maybe-check-target-winsup \
     maybe-check-target-libgloss \
@@ -2451,6 +2467,7 @@ install-target:  \
     maybe-install-target-libgfortran \
     maybe-install-target-libobjc \
     maybe-install-target-libgo \
+    maybe-install-target-libphobos \
     maybe-install-target-libtermcap \
     maybe-install-target-winsup \
     maybe-install-target-libgloss \
@@ -2553,6 +2570,7 @@ install-strip-target:  \
     maybe-install-strip-target-libgfortran \
     maybe-install-strip-target-libobjc \
     maybe-install-strip-target-libgo \
+    maybe-install-strip-target-libphobos \
     maybe-install-strip-target-libtermcap \
     maybe-install-strip-target-winsup \
     maybe-install-strip-target-libgloss \
@@ -37320,6 +37338,463 @@ maintainer-clean-target-libgo:
 
 
 
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
+	srcdiroption="--srcdir=$${topdir}/libphobos"; \
+	libsrcdir="$$s/libphobos"; \
+	rm -f no-such-file || : ; \
+	CONFIG_SITE=no-such-file $(SHELL) $${libsrcdir}/configure \
+	  $(TARGET_CONFIGARGS) --build=${build_alias} --host=${target_alias} \
+	  --target=${target_alias} $${srcdiroption}  \
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
+	  $(MAKE) $(BASE_FLAGS_TO_PASS) $(EXTRA_TARGET_FLAGS)  \
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
@@ -43354,6 +43829,14 @@ check-gcc-go:
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
@@ -45407,6 +45890,7 @@ configure-target-libquadmath: stage_last
 configure-target-libgfortran: stage_last
 configure-target-libobjc: stage_last
 configure-target-libgo: stage_last
+configure-target-libphobos: stage_last
 configure-target-libtermcap: stage_last
 configure-target-winsup: stage_last
 configure-target-libgloss: stage_last
@@ -45438,6 +45922,7 @@ configure-target-libquadmath: maybe-all-
 configure-target-libgfortran: maybe-all-gcc
 configure-target-libobjc: maybe-all-gcc
 configure-target-libgo: maybe-all-gcc
+configure-target-libphobos: maybe-all-gcc
 configure-target-libtermcap: maybe-all-gcc
 configure-target-winsup: maybe-all-gcc
 configure-target-libgloss: maybe-all-gcc
@@ -46180,6 +46665,8 @@ configure-target-libgo: maybe-all-target
 all-target-libgo: maybe-all-target-libbacktrace
 all-target-libgo: maybe-all-target-libffi
 all-target-libgo: maybe-all-target-libatomic
+configure-target-libphobos: maybe-configure-target-zlib
+all-target-libphobos: maybe-all-target-zlib
 configure-target-libjava: maybe-configure-target-zlib
 configure-target-libjava: maybe-configure-target-boehm-gc
 configure-target-libjava: maybe-configure-target-libffi
@@ -46266,6 +46753,7 @@ configure-target-libquadmath: maybe-all-
 configure-target-libgfortran: maybe-all-target-libgcc
 configure-target-libobjc: maybe-all-target-libgcc
 configure-target-libgo: maybe-all-target-libgcc
+configure-target-libphobos: maybe-all-target-libgcc
 configure-target-libtermcap: maybe-all-target-libgcc
 configure-target-winsup: maybe-all-target-libgcc
 configure-target-libgloss: maybe-all-target-libgcc
@@ -46301,6 +46789,8 @@ configure-target-libobjc: maybe-all-targ
 
 configure-target-libgo: maybe-all-target-newlib maybe-all-target-libgloss
 
+configure-target-libphobos: maybe-all-target-newlib maybe-all-target-libgloss
+
 configure-target-libtermcap: maybe-all-target-newlib maybe-all-target-libgloss
 
 configure-target-winsup: maybe-all-target-newlib maybe-all-target-libgloss
