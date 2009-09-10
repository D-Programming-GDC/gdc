	diff -c gcc-4.1.1-orig/Makefile.def gcc-4.1.1/Makefile.def
*** gcc-4.1.1-orig/Makefile.def	Fri Nov 11 16:47:07 2005
--- gcc-4.1.1/Makefile.def	Tue Jan 23 19:39:14 2007
***************
*** 136,141 ****
--- 136,142 ----
  target_modules = { module= qthreads; };
  target_modules = { module= rda; };
  target_modules = { module= libada; };
+ target_modules = { module= libphobos; };
  
  // These are (some of) the make targets to be done in each subdirectory.
  // Not all; these are the ones which don't have special options.
diff -c gcc-4.1.1-orig/Makefile.in gcc-4.1.1/Makefile.in
*** gcc-4.1.1-orig/Makefile.in	Tue Apr  4 17:03:05 2006
--- gcc-4.1.1/Makefile.in	Tue Jan 23 19:39:14 2007
***************
*** 626,632 ****
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
!     maybe-configure-target-libada
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
--- 626,633 ----
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
!     maybe-configure-target-libada \
!     maybe-configure-target-libphobos
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
***************
*** 745,751 ****
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
      maybe-all-target-rda \
!     maybe-all-target-libada
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
--- 746,753 ----
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
      maybe-all-target-rda \
!     maybe-all-target-libada \
!     maybe-all-target-libphobos
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
***************
*** 852,858 ****
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
      maybe-info-target-rda \
!     maybe-info-target-libada
  
  .PHONY: do-dvi
  do-dvi:
--- 854,861 ----
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
      maybe-info-target-rda \
!     maybe-info-target-libada \
!     maybe-info-target-libphobos
  
  .PHONY: do-dvi
  do-dvi:
***************
*** 954,960 ****
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
      maybe-dvi-target-rda \
!     maybe-dvi-target-libada
  
  .PHONY: do-html
  do-html:
--- 957,964 ----
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
      maybe-dvi-target-rda \
!     maybe-dvi-target-libada \
!     maybe-dvi-target-libphobos
  
  .PHONY: do-html
  do-html:
***************
*** 1056,1062 ****
      maybe-html-target-boehm-gc \
      maybe-html-target-qthreads \
      maybe-html-target-rda \
!     maybe-html-target-libada
  
  .PHONY: do-TAGS
  do-TAGS:
--- 1060,1067 ----
      maybe-html-target-boehm-gc \
      maybe-html-target-qthreads \
      maybe-html-target-rda \
!     maybe-html-target-libada \
!     maybe-html-target-libphobos
  
  .PHONY: do-TAGS
  do-TAGS:
***************
*** 1158,1164 ****
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
      maybe-TAGS-target-rda \
!     maybe-TAGS-target-libada
  
  .PHONY: do-install-info
  do-install-info:
--- 1163,1170 ----
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
      maybe-TAGS-target-rda \
!     maybe-TAGS-target-libada \
!     maybe-TAGS-target-libphobos
  
  .PHONY: do-install-info
  do-install-info:
***************
*** 1260,1266 ****
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
      maybe-install-info-target-rda \
!     maybe-install-info-target-libada
  
  .PHONY: do-installcheck
  do-installcheck:
--- 1266,1273 ----
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
      maybe-install-info-target-rda \
!     maybe-install-info-target-libada \
!     maybe-install-info-target-libphobos
  
  .PHONY: do-installcheck
  do-installcheck:
***************
*** 1362,1368 ****
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
      maybe-installcheck-target-rda \
!     maybe-installcheck-target-libada
  
  .PHONY: do-mostlyclean
  do-mostlyclean:
--- 1369,1376 ----
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
      maybe-installcheck-target-rda \
!     maybe-installcheck-target-libada \
!     maybe-installcheck-target-libphobos
  
  .PHONY: do-mostlyclean
  do-mostlyclean:
***************
*** 1464,1470 ****
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
      maybe-mostlyclean-target-rda \
!     maybe-mostlyclean-target-libada
  
  .PHONY: do-clean
  do-clean:
--- 1472,1479 ----
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
      maybe-mostlyclean-target-rda \
!     maybe-mostlyclean-target-libada \
!     maybe-mostlyclean-target-libphobos
  
  .PHONY: do-clean
  do-clean:
***************
*** 1566,1572 ****
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
      maybe-clean-target-rda \
!     maybe-clean-target-libada
  
  .PHONY: do-distclean
  do-distclean:
--- 1575,1582 ----
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
      maybe-clean-target-rda \
!     maybe-clean-target-libada \
!     maybe-clean-target-libphobos
  
  .PHONY: do-distclean
  do-distclean:
***************
*** 1668,1674 ****
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
      maybe-distclean-target-rda \
!     maybe-distclean-target-libada
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean:
--- 1678,1685 ----
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
      maybe-distclean-target-rda \
!     maybe-distclean-target-libada \
!     maybe-distclean-target-libphobos
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean:
***************
*** 1770,1776 ****
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
      maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libada
  
  
  # Here are the targets which correspond to the do-X targets.
--- 1781,1788 ----
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
      maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libada \
!     maybe-maintainer-clean-target-libphobos
  
  
  # Here are the targets which correspond to the do-X targets.
***************
*** 1928,1934 ****
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
      maybe-check-target-rda \
!     maybe-check-target-libada
  
  do-check:
  	@: $(MAKE); $(unstage)
--- 1940,1947 ----
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
      maybe-check-target-rda \
!     maybe-check-target-libada \
!     maybe-check-target-libphobos
  
  do-check:
  	@: $(MAKE); $(unstage)
***************
*** 2127,2133 ****
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
      maybe-install-target-rda \
!     maybe-install-target-libada
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
--- 2140,2147 ----
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
      maybe-install-target-rda \
!     maybe-install-target-libada \
!     maybe-install-target-libphobos
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
***************
*** 34743,34748 ****
--- 34757,35111 ----
  
  
  
+ # There's only one multilib.out.  Cleverer subdirs shouldn't need it copied.
+ @if target-libphobos
+ $(TARGET_SUBDIR)/libphobos/multilib.out: multilib.out
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+ 	rm -f $(TARGET_SUBDIR)/libphobos/Makefile || : ; \
+ 	cp multilib.out $(TARGET_SUBDIR)/libphobos/multilib.out
+ @endif target-libphobos
+ 
+ 
+ 
+ .PHONY: configure-target-libphobos maybe-configure-target-libphobos
+ maybe-configure-target-libphobos:
+ @if target-libphobos
+ maybe-configure-target-libphobos: configure-target-libphobos
+ configure-target-libphobos: $(TARGET_SUBDIR)/libphobos/multilib.out
+ 	@$(unstage)
+ 	@test ! -f $(TARGET_SUBDIR)/libphobos/Makefile || exit 0; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo Configuring in $(TARGET_SUBDIR)/libphobos; \
+ 	cd "$(TARGET_SUBDIR)/libphobos" || exit 1; \
+ 	case $(srcdir) in \
+ 	  /* | [A-Za-z]:[\\/]*) topdir=$(srcdir) ;; \
+ 	  *) topdir=`echo $(TARGET_SUBDIR)/libphobos/ | \
+ 		sed -e 's,\./,,g' -e 's,[^/]*/,../,g' `$(srcdir) ;; \
+ 	esac; \
+ 	srcdiroption="--srcdir=$${topdir}/libphobos"; \
+ 	libsrcdir="$$s/libphobos"; \
+ 	rm -f no-such-file || : ; \
+ 	CONFIG_SITE=no-such-file $(SHELL) $${libsrcdir}/configure \
+ 	  $(TARGET_CONFIGARGS) $${srcdiroption}  \
+ 	  || exit 1
+ @endif target-libphobos
+ 
+ 
+ 
+ 
+ 
+ .PHONY: all-target-libphobos maybe-all-target-libphobos
+ maybe-all-target-libphobos:
+ @if target-libphobos
+ TARGET-target-libphobos=all
+ maybe-all-target-libphobos: all-target-libphobos
+ all-target-libphobos: configure-target-libphobos
+ 	@$(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  $(TARGET-target-libphobos))
+ @endif target-libphobos
+ 
+ 
+ 
+ 
+ 
+ .PHONY: check-target-libphobos maybe-check-target-libphobos
+ maybe-check-target-libphobos:
+ @if target-libphobos
+ maybe-check-target-libphobos: check-target-libphobos
+ 
+ check-target-libphobos:
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)   check)
+ 
+ @endif target-libphobos
+ 
+ .PHONY: install-target-libphobos maybe-install-target-libphobos
+ maybe-install-target-libphobos:
+ @if target-libphobos
+ maybe-install-target-libphobos: install-target-libphobos
+ 
+ install-target-libphobos: installdirs
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  install)
+ 
+ @endif target-libphobos
+ 
+ # Other targets (info, dvi, etc.)
+ 
+ .PHONY: maybe-info-target-libphobos info-target-libphobos
+ maybe-info-target-libphobos:
+ @if target-libphobos
+ maybe-info-target-libphobos: info-target-libphobos
+ 
+ info-target-libphobos: \
+     configure-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing info in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           info) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-dvi-target-libphobos dvi-target-libphobos
+ maybe-dvi-target-libphobos:
+ @if target-libphobos
+ maybe-dvi-target-libphobos: dvi-target-libphobos
+ 
+ dvi-target-libphobos: \
+     configure-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing dvi in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           dvi) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-html-target-libphobos html-target-libphobos
+ maybe-html-target-libphobos:
+ @if target-libphobos
+ maybe-html-target-libphobos: html-target-libphobos
+ 
+ html-target-libphobos: \
+     configure-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing html in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           html) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-TAGS-target-libphobos TAGS-target-libphobos
+ maybe-TAGS-target-libphobos:
+ @if target-libphobos
+ maybe-TAGS-target-libphobos: TAGS-target-libphobos
+ 
+ TAGS-target-libphobos: \
+     configure-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing TAGS in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           TAGS) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-install-info-target-libphobos install-info-target-libphobos
+ maybe-install-info-target-libphobos:
+ @if target-libphobos
+ maybe-install-info-target-libphobos: install-info-target-libphobos
+ 
+ install-info-target-libphobos: \
+     configure-target-libphobos \
+     info-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-info in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           install-info) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-installcheck-target-libphobos installcheck-target-libphobos
+ maybe-installcheck-target-libphobos:
+ @if target-libphobos
+ maybe-installcheck-target-libphobos: installcheck-target-libphobos
+ 
+ installcheck-target-libphobos: \
+     configure-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing installcheck in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           installcheck) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-mostlyclean-target-libphobos mostlyclean-target-libphobos
+ maybe-mostlyclean-target-libphobos:
+ @if target-libphobos
+ maybe-mostlyclean-target-libphobos: mostlyclean-target-libphobos
+ 
+ mostlyclean-target-libphobos: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing mostlyclean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           mostlyclean) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-clean-target-libphobos clean-target-libphobos
+ maybe-clean-target-libphobos:
+ @if target-libphobos
+ maybe-clean-target-libphobos: clean-target-libphobos
+ 
+ clean-target-libphobos: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing clean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           clean) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-distclean-target-libphobos distclean-target-libphobos
+ maybe-distclean-target-libphobos:
+ @if target-libphobos
+ maybe-distclean-target-libphobos: distclean-target-libphobos
+ 
+ distclean-target-libphobos: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing distclean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           distclean) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-maintainer-clean-target-libphobos maintainer-clean-target-libphobos
+ maybe-maintainer-clean-target-libphobos:
+ @if target-libphobos
+ maybe-maintainer-clean-target-libphobos: maintainer-clean-target-libphobos
+ 
+ maintainer-clean-target-libphobos: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing maintainer-clean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           maintainer-clean) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ 
+ 
  # ----------
  # GCC module
  # ----------
***************
*** 36182,36187 ****
--- 36545,36552 ----
  
  configure-target-libada: maybe-all-gcc
  
+ configure-target-libphobos: maybe-all-gcc
+ 
  
  
  configure-target-boehm-gc: maybe-all-target-newlib maybe-all-target-libgloss
Common subdirectories: gcc-4.1.1-orig/config and gcc-4.1.1/config
diff -c gcc-4.1.1-orig/configure gcc-4.1.1/configure
*** gcc-4.1.1-orig/configure	Fri Dec 16 07:57:40 2005
--- gcc-4.1.1/configure	Tue Jan 23 19:39:14 2007
***************
*** 909,914 ****
--- 909,915 ----
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
+   		target-libphobos \
  		target-libada"
  
  # these tools are built using the target libraries, and are intended to
***************
*** 1802,1807 ****
--- 1803,1809 ----
    CXX=${CXX-${host_alias}-c++}
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    BUILD_PREFIX=${build_alias}-
    BUILD_PREFIX_1=${build_alias}-
  
***************
*** 1815,1820 ****
--- 1817,1823 ----
    # This is all going to change when we autoconfiscate...
  
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
  
***************
*** 6311,6316 ****
--- 6314,6320 ----
  s%@target_configargs@%$target_configargs%g
  s%@target_configdirs@%$target_configdirs%g
  s%@CC_FOR_BUILD@%$CC_FOR_BUILD%g
+ s%@CXX_FOR_BUILD@%$CXX_FOR_BUILD%g
  s%@config_shell@%$config_shell%g
  s%@YACC@%$YACC%g
  s%@BISON@%$BISON%g
diff -c gcc-4.1.1-orig/configure.in gcc-4.1.1/configure.in
*** gcc-4.1.1-orig/configure.in	Fri Dec 16 07:57:40 2005
--- gcc-4.1.1/configure.in	Tue Jan 23 19:39:14 2007
***************
*** 151,156 ****
--- 151,157 ----
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
+   		target-libphobos \
  		target-libada"
  
  # these tools are built using the target libraries, and are intended to
***************
*** 1010,1015 ****
--- 1011,1017 ----
    CXX=${CXX-${host_alias}-c++}
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    BUILD_PREFIX=${build_alias}-
    BUILD_PREFIX_1=${build_alias}-
  
***************
*** 1023,1028 ****
--- 1025,1031 ----
    # This is all going to change when we autoconfiscate...
  
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
  
***************
*** 2086,2091 ****
--- 2089,2095 ----
  
  # Build tools.
  AC_SUBST(CC_FOR_BUILD)
+ AC_SUBST(CXX_FOR_BUILD)
  AC_SUBST(config_shell)
  
  # Generate default definitions for YACC, M4, LEX and other programs that run
