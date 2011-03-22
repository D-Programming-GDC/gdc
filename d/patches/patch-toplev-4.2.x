--- gcc-4.2.4-orig/configure	2007-09-15 01:42:24.000000000 +0100
+++ gcc-4.2.4/configure	2011-03-22 00:16:27.040880461 +0000
@@ -940,7 +940,8 @@ target_libraries="target-libiberty \
 		${libgcj} \
 		target-libobjc \
 		target-libada \
-		target-libgomp"
+		target-libgomp \
+		target-libphobos"
 
 # these tools are built using the target libraries, and are intended to
 # run only in the target environment
--- gcc-4.2.4-orig/configure.in	2007-09-15 01:42:24.000000000 +0100
+++ gcc-4.2.4/configure.in	2011-03-22 00:16:27.200881259 +0000
@@ -152,7 +152,8 @@ target_libraries="target-libiberty \
 		${libgcj} \
 		target-libobjc \
 		target-libada \
-		target-libgomp"
+		target-libgomp \
+		target-libphobos"
 
 # these tools are built using the target libraries, and are intended to
 # run only in the target environment
--- gcc-4.2.4-orig/Makefile.def	2006-12-29 17:47:06.000000000 +0000
+++ gcc-4.2.4/Makefile.def	2011-03-22 00:16:28.992890135 +0000
@@ -139,6 +139,7 @@ target_modules = { module= qthreads; };
 target_modules = { module= rda; };
 target_modules = { module= libada; };
 target_modules = { module= libgomp; lib_path=.libs; };
+target_modules = { module= libphobos; };
 
 // These are (some of) the make targets to be done in each subdirectory.
 // Not all; these are the ones which don't have special options.
--- gcc-4.2.4-orig/Makefile.in	2006-12-29 17:47:06.000000000 +0000
+++ gcc-4.2.4/Makefile.in	2011-03-22 00:16:29.652893410 +0000
@@ -661,7 +661,8 @@ configure-target:  \
     maybe-configure-target-qthreads \
     maybe-configure-target-rda \
     maybe-configure-target-libada \
-    maybe-configure-target-libgomp
+    maybe-configure-target-libgomp \
+    maybe-configure-target-libphobos
 
 # The target built for a native non-bootstrap build.
 .PHONY: all
@@ -806,6 +807,7 @@ all-target: maybe-all-target-qthreads
 all-target: maybe-all-target-rda
 all-target: maybe-all-target-libada
 all-target: maybe-all-target-libgomp
+all-target: maybe-all-target-libphobos
 
 # Do a target for all the subdirectories.  A ``make do-X'' will do a
 # ``make X'' in all subdirectories (because, in general, there is a
@@ -914,6 +916,7 @@ info-target: maybe-info-target-qthreads
 info-target: maybe-info-target-rda
 info-target: maybe-info-target-libada
 info-target: maybe-info-target-libgomp
+info-target: maybe-info-target-libphobos
 
 .PHONY: do-dvi
 do-dvi:
@@ -1017,6 +1020,7 @@ dvi-target: maybe-dvi-target-qthreads
 dvi-target: maybe-dvi-target-rda
 dvi-target: maybe-dvi-target-libada
 dvi-target: maybe-dvi-target-libgomp
+dvi-target: maybe-dvi-target-libphobos
 
 .PHONY: do-pdf
 do-pdf:
@@ -1120,6 +1124,7 @@ pdf-target: maybe-pdf-target-qthreads
 pdf-target: maybe-pdf-target-rda
 pdf-target: maybe-pdf-target-libada
 pdf-target: maybe-pdf-target-libgomp
+pdf-target: maybe-pdf-target-libphobos
 
 .PHONY: do-html
 do-html:
@@ -1223,6 +1228,7 @@ html-target: maybe-html-target-qthreads
 html-target: maybe-html-target-rda
 html-target: maybe-html-target-libada
 html-target: maybe-html-target-libgomp
+html-target: maybe-html-target-libphobos
 
 .PHONY: do-TAGS
 do-TAGS:
@@ -1326,6 +1332,7 @@ TAGS-target: maybe-TAGS-target-qthreads
 TAGS-target: maybe-TAGS-target-rda
 TAGS-target: maybe-TAGS-target-libada
 TAGS-target: maybe-TAGS-target-libgomp
+TAGS-target: maybe-TAGS-target-libphobos
 
 .PHONY: do-install-info
 do-install-info:
@@ -1429,6 +1436,7 @@ install-info-target: maybe-install-info-
 install-info-target: maybe-install-info-target-rda
 install-info-target: maybe-install-info-target-libada
 install-info-target: maybe-install-info-target-libgomp
+install-info-target: maybe-install-info-target-libphobos
 
 .PHONY: do-install-html
 do-install-html:
@@ -1532,6 +1540,7 @@ install-html-target: maybe-install-html-
 install-html-target: maybe-install-html-target-rda
 install-html-target: maybe-install-html-target-libada
 install-html-target: maybe-install-html-target-libgomp
+install-html-target: maybe-install-html-target-libphobos
 
 .PHONY: do-installcheck
 do-installcheck:
@@ -1635,6 +1644,7 @@ installcheck-target: maybe-installcheck-
 installcheck-target: maybe-installcheck-target-rda
 installcheck-target: maybe-installcheck-target-libada
 installcheck-target: maybe-installcheck-target-libgomp
+installcheck-target: maybe-installcheck-target-libphobos
 
 .PHONY: do-mostlyclean
 do-mostlyclean:
@@ -1738,6 +1748,7 @@ mostlyclean-target: maybe-mostlyclean-ta
 mostlyclean-target: maybe-mostlyclean-target-rda
 mostlyclean-target: maybe-mostlyclean-target-libada
 mostlyclean-target: maybe-mostlyclean-target-libgomp
+mostlyclean-target: maybe-mostlyclean-target-libphobos
 
 .PHONY: do-clean
 do-clean:
@@ -1841,6 +1852,7 @@ clean-target: maybe-clean-target-qthread
 clean-target: maybe-clean-target-rda
 clean-target: maybe-clean-target-libada
 clean-target: maybe-clean-target-libgomp
+clean-target: maybe-clean-target-libphobos
 
 .PHONY: do-distclean
 do-distclean:
@@ -1944,6 +1956,7 @@ distclean-target: maybe-distclean-target
 distclean-target: maybe-distclean-target-rda
 distclean-target: maybe-distclean-target-libada
 distclean-target: maybe-distclean-target-libgomp
+distclean-target: maybe-distclean-target-libphobos
 
 .PHONY: do-maintainer-clean
 do-maintainer-clean:
@@ -2047,6 +2060,7 @@ maintainer-clean-target: maybe-maintaine
 maintainer-clean-target: maybe-maintainer-clean-target-rda
 maintainer-clean-target: maybe-maintainer-clean-target-libada
 maintainer-clean-target: maybe-maintainer-clean-target-libgomp
+maintainer-clean-target: maybe-maintainer-clean-target-libphobos
 
 
 # Here are the targets which correspond to the do-X targets.
@@ -2205,7 +2219,8 @@ check-target:  \
     maybe-check-target-qthreads \
     maybe-check-target-rda \
     maybe-check-target-libada \
-    maybe-check-target-libgomp
+    maybe-check-target-libgomp \
+    maybe-check-target-libphobos
 
 do-check:
 	@: $(MAKE); $(unstage)
@@ -2405,7 +2420,8 @@ install-target:  \
     maybe-install-target-qthreads \
     maybe-install-target-rda \
     maybe-install-target-libada \
-    maybe-install-target-libgomp
+    maybe-install-target-libgomp \
+    maybe-install-target-libphobos
 
 uninstall:
 	@echo "the uninstall target is not supported in this tree"
@@ -41705,6 +41721,419 @@ maintainer-clean-target-libgomp:
 
 
 
+
+
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
+	$(NORMAL_TARGET_EXPORTS) \
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
+	  $(TARGET_CONFIGARGS) $${srcdiroption}  \
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
+	$(NORMAL_TARGET_EXPORTS) \
+	(cd $(TARGET_SUBDIR)/libphobos && \
+	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  $(TARGET-target-libphobos))
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+	           install-info) \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+	           maintainer-clean) \
+	  || exit 1
+
+@endif target-libphobos
+
+
+
 # ----------
 # GCC module
 # ----------
@@ -43190,6 +43619,7 @@ configure-target-qthreads: stage_last
 configure-target-rda: stage_last
 configure-target-libada: stage_last
 configure-target-libgomp: stage_last
+configure-target-libphobos: stage_last
 @endif gcc-bootstrap
 
 @if gcc-no-bootstrap
@@ -43213,6 +43643,7 @@ configure-target-qthreads: maybe-all-gcc
 configure-target-rda: maybe-all-gcc
 configure-target-libada: maybe-all-gcc
 configure-target-libgomp: maybe-all-gcc
+configure-target-libphobos: maybe-all-gcc
 @endif gcc-no-bootstrap
 
 
