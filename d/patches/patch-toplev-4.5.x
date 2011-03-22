--- gcc-4.5.2-orig/configure	2010-10-06 11:29:55.000000000 +0100
+++ gcc-4.5.2/configure	2011-03-22 00:17:38.965237120 +0000
@@ -2925,6 +2925,7 @@ target_libraries="target-libgcc \
 		target-boehm-gc \
 		${libgcj} \
 		target-libobjc \
+		target-libphobos \
 		target-libada"
 
 # these tools are built using the target libraries, and are intended to
--- gcc-4.5.2-orig/configure.ac	2010-10-06 11:29:55.000000000 +0100
+++ gcc-4.5.2/configure.ac	2011-03-22 00:17:38.973237151 +0000
@@ -198,6 +198,7 @@ target_libraries="target-libgcc \
 		target-boehm-gc \
 		${libgcj} \
 		target-libobjc \
+		target-libphobos \
 		target-libada"
 
 # these tools are built using the target libraries, and are intended to
--- gcc-4.5.2-orig/Makefile.def	2010-06-10 18:05:59.000000000 +0100
+++ gcc-4.5.2/Makefile.def	2011-03-22 00:17:38.973237151 +0000
@@ -175,6 +175,7 @@ target_modules = { module= qthreads; };
 target_modules = { module= rda; };
 target_modules = { module= libada; };
 target_modules = { module= libgomp; bootstrap= true; lib_path=.libs; };
+target_modules = { module= libphobos; };
 
 // These are (some of) the make targets to be done in each subdirectory.
 // Not all; these are the ones which don't have special options.
--- gcc-4.5.2-orig/Makefile.in	2010-06-10 18:05:59.000000000 +0100
+++ gcc-4.5.2/Makefile.in	2011-03-22 00:17:39.041237488 +0000
@@ -940,7 +940,8 @@ configure-target:  \
     maybe-configure-target-qthreads \
     maybe-configure-target-rda \
     maybe-configure-target-libada \
-    maybe-configure-target-libgomp
+    maybe-configure-target-libgomp \
+    maybe-configure-target-libphobos
 
 # The target built for a native non-bootstrap build.
 .PHONY: all
@@ -1125,6 +1126,7 @@ all-target: maybe-all-target-libada
 @if target-libgomp-no-bootstrap
 all-target: maybe-all-target-libgomp
 @endif target-libgomp-no-bootstrap
+all-target: maybe-all-target-libphobos
 
 # Do a target for all the subdirectories.  A ``make do-X'' will do a
 # ``make X'' in all subdirectories (because, in general, there is a
@@ -1244,6 +1246,7 @@ info-target: maybe-info-target-qthreads
 info-target: maybe-info-target-rda
 info-target: maybe-info-target-libada
 info-target: maybe-info-target-libgomp
+info-target: maybe-info-target-libphobos
 
 .PHONY: do-dvi
 do-dvi:
@@ -1358,6 +1361,7 @@ dvi-target: maybe-dvi-target-qthreads
 dvi-target: maybe-dvi-target-rda
 dvi-target: maybe-dvi-target-libada
 dvi-target: maybe-dvi-target-libgomp
+dvi-target: maybe-dvi-target-libphobos
 
 .PHONY: do-pdf
 do-pdf:
@@ -1472,6 +1476,7 @@ pdf-target: maybe-pdf-target-qthreads
 pdf-target: maybe-pdf-target-rda
 pdf-target: maybe-pdf-target-libada
 pdf-target: maybe-pdf-target-libgomp
+pdf-target: maybe-pdf-target-libphobos
 
 .PHONY: do-html
 do-html:
@@ -1586,6 +1591,7 @@ html-target: maybe-html-target-qthreads
 html-target: maybe-html-target-rda
 html-target: maybe-html-target-libada
 html-target: maybe-html-target-libgomp
+html-target: maybe-html-target-libphobos
 
 .PHONY: do-TAGS
 do-TAGS:
@@ -1700,6 +1706,7 @@ TAGS-target: maybe-TAGS-target-qthreads
 TAGS-target: maybe-TAGS-target-rda
 TAGS-target: maybe-TAGS-target-libada
 TAGS-target: maybe-TAGS-target-libgomp
+TAGS-target: maybe-TAGS-target-libphobos
 
 .PHONY: do-install-info
 do-install-info:
@@ -1814,6 +1821,7 @@ install-info-target: maybe-install-info-
 install-info-target: maybe-install-info-target-rda
 install-info-target: maybe-install-info-target-libada
 install-info-target: maybe-install-info-target-libgomp
+install-info-target: maybe-install-info-target-libphobos
 
 .PHONY: do-install-pdf
 do-install-pdf:
@@ -1928,6 +1936,7 @@ install-pdf-target: maybe-install-pdf-ta
 install-pdf-target: maybe-install-pdf-target-rda
 install-pdf-target: maybe-install-pdf-target-libada
 install-pdf-target: maybe-install-pdf-target-libgomp
+install-pdf-target: maybe-install-pdf-target-libphobos
 
 .PHONY: do-install-html
 do-install-html:
@@ -2042,6 +2051,7 @@ install-html-target: maybe-install-html-
 install-html-target: maybe-install-html-target-rda
 install-html-target: maybe-install-html-target-libada
 install-html-target: maybe-install-html-target-libgomp
+install-html-target: maybe-install-html-target-libphobos
 
 .PHONY: do-installcheck
 do-installcheck:
@@ -2156,6 +2166,7 @@ installcheck-target: maybe-installcheck-
 installcheck-target: maybe-installcheck-target-rda
 installcheck-target: maybe-installcheck-target-libada
 installcheck-target: maybe-installcheck-target-libgomp
+installcheck-target: maybe-installcheck-target-libphobos
 
 .PHONY: do-mostlyclean
 do-mostlyclean:
@@ -2270,6 +2281,7 @@ mostlyclean-target: maybe-mostlyclean-ta
 mostlyclean-target: maybe-mostlyclean-target-rda
 mostlyclean-target: maybe-mostlyclean-target-libada
 mostlyclean-target: maybe-mostlyclean-target-libgomp
+mostlyclean-target: maybe-mostlyclean-target-libphobos
 
 .PHONY: do-clean
 do-clean:
@@ -2384,6 +2396,7 @@ clean-target: maybe-clean-target-qthread
 clean-target: maybe-clean-target-rda
 clean-target: maybe-clean-target-libada
 clean-target: maybe-clean-target-libgomp
+clean-target: maybe-clean-target-libphobos
 
 .PHONY: do-distclean
 do-distclean:
@@ -2498,6 +2511,7 @@ distclean-target: maybe-distclean-target
 distclean-target: maybe-distclean-target-rda
 distclean-target: maybe-distclean-target-libada
 distclean-target: maybe-distclean-target-libgomp
+distclean-target: maybe-distclean-target-libphobos
 
 .PHONY: do-maintainer-clean
 do-maintainer-clean:
@@ -2612,6 +2626,7 @@ maintainer-clean-target: maybe-maintaine
 maintainer-clean-target: maybe-maintainer-clean-target-rda
 maintainer-clean-target: maybe-maintainer-clean-target-libada
 maintainer-clean-target: maybe-maintainer-clean-target-libgomp
+maintainer-clean-target: maybe-maintainer-clean-target-libphobos
 
 
 # Here are the targets which correspond to the do-X targets.
@@ -2780,7 +2795,8 @@ check-target:  \
     maybe-check-target-qthreads \
     maybe-check-target-rda \
     maybe-check-target-libada \
-    maybe-check-target-libgomp
+    maybe-check-target-libgomp \
+    maybe-check-target-libphobos
 
 do-check:
 	@: $(MAKE); $(unstage)
@@ -3001,7 +3017,8 @@ install-target:  \
     maybe-install-target-qthreads \
     maybe-install-target-rda \
     maybe-install-target-libada \
-    maybe-install-target-libgomp
+    maybe-install-target-libgomp \
+    maybe-install-target-libphobos
 
 uninstall:
 	@echo "the uninstall target is not supported in this tree"
@@ -55592,6 +55609,448 @@ maintainer-clean-target-libgomp:
 
 
 
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
 # ----------
 # GCC module
 # ----------
@@ -57670,6 +58129,7 @@ configure-stage3-target-libgomp: maybe-a
 configure-stage4-target-libgomp: maybe-all-stage4-gcc
 configure-stageprofile-target-libgomp: maybe-all-stageprofile-gcc
 configure-stagefeedback-target-libgomp: maybe-all-stagefeedback-gcc
+configure-target-libphobos: stage_last
 @endif gcc-bootstrap
 
 @if gcc-no-bootstrap
@@ -57694,6 +58154,7 @@ configure-target-qthreads: maybe-all-gcc
 configure-target-rda: maybe-all-gcc
 configure-target-libada: maybe-all-gcc
 configure-target-libgomp: maybe-all-gcc
+configure-target-libphobos: maybe-all-gcc
 @endif gcc-no-bootstrap
 
 
@@ -58475,6 +58936,7 @@ configure-target-qthreads: maybe-all-tar
 configure-target-rda: maybe-all-target-libgcc
 configure-target-libada: maybe-all-target-libgcc
 configure-target-libgomp: maybe-all-target-libgcc
+configure-target-libphobos: maybe-all-target-libgcc
 @endif gcc-no-bootstrap
 
 
@@ -58518,6 +58980,8 @@ configure-target-libada: maybe-all-targe
 
 configure-target-libgomp: maybe-all-target-newlib maybe-all-target-libgloss
 
+configure-target-libphobos: maybe-all-target-newlib maybe-all-target-libgloss
+
 
 CONFIGURE_GDB_TK = @CONFIGURE_GDB_TK@
 GDB_TK = @GDB_TK@
