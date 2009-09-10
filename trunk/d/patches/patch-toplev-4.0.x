diff -c gcc-4.0.3-orig/Makefile.def gcc-4.0.3/Makefile.def
*** gcc-4.0.3-orig/Makefile.def	Thu Oct 20 07:55:29 2005
--- gcc-4.0.3/Makefile.def	Fri Nov 10 15:28:57 2006
***************
*** 134,139 ****
--- 134,140 ----
  target_modules = { module= qthreads; };
  target_modules = { module= rda; };
  target_modules = { module= libada; };
+ target_modules = { module= libphobos; };
  
  // These are (some of) the make targets to be done in each subdirectory.
  // Not all; these are the ones which don't have special options.
diff -c gcc-4.0.3-orig/Makefile.in gcc-4.0.3/Makefile.in
*** gcc-4.0.3-orig/Makefile.in	Thu Oct 20 07:55:29 2005
--- gcc-4.0.3/Makefile.in	Fri Nov 10 15:34:15 2006
***************
*** 243,249 ****
  CC_FOR_BUILD = @CC_FOR_BUILD@
  CFLAGS_FOR_BUILD = @CFLAGS_FOR_BUILD@
  
! CXX_FOR_BUILD = $(CXX)
  
  # Special variables passed down in EXTRA_GCC_FLAGS.  They are defined
  # here so that they can be overridden by Makefile fragments.
--- 243,249 ----
  CC_FOR_BUILD = @CC_FOR_BUILD@
  CFLAGS_FOR_BUILD = @CFLAGS_FOR_BUILD@
  
! CXX_FOR_BUILD = @CXX_FOR_BUILD@
  
  # Special variables passed down in EXTRA_GCC_FLAGS.  They are defined
  # here so that they can be overridden by Makefile fragments.
***************
*** 735,741 ****
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
!     maybe-configure-target-libada
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
--- 735,742 ----
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
!     maybe-configure-target-libada \
!     maybe-configure-target-libphobos
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
***************
*** 837,843 ****
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
      maybe-all-target-rda \
!     maybe-all-target-libada
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
--- 838,845 ----
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
      maybe-all-target-rda \
!     maybe-all-target-libada \
!     maybe-all-target-libphobos
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
***************
*** 935,941 ****
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
      maybe-info-target-rda \
!     maybe-info-target-libada
  
  .PHONY: do-dvi
  do-dvi: unstage dvi-host dvi-target stage
--- 937,944 ----
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
      maybe-info-target-rda \
!     maybe-info-target-libada \
!     maybe-info-target-libphobos
  
  .PHONY: do-dvi
  do-dvi: unstage dvi-host dvi-target stage
***************
*** 1028,1034 ****
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
      maybe-dvi-target-rda \
!     maybe-dvi-target-libada
  
  .PHONY: do-html
  do-html: unstage html-host html-target stage
--- 1031,1038 ----
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
      maybe-dvi-target-rda \
!     maybe-dvi-target-libada \
!     maybe-dvi-target-libphobos
  
  .PHONY: do-html
  do-html: unstage html-host html-target stage
***************
*** 1121,1127 ****
      maybe-html-target-boehm-gc \
      maybe-html-target-qthreads \
      maybe-html-target-rda \
!     maybe-html-target-libada
  
  .PHONY: do-TAGS
  do-TAGS: unstage TAGS-host TAGS-target stage
--- 1125,1132 ----
      maybe-html-target-boehm-gc \
      maybe-html-target-qthreads \
      maybe-html-target-rda \
!     maybe-html-target-libada \
!     maybe-html-target-libphobos
  
  .PHONY: do-TAGS
  do-TAGS: unstage TAGS-host TAGS-target stage
***************
*** 1214,1220 ****
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
      maybe-TAGS-target-rda \
!     maybe-TAGS-target-libada
  
  .PHONY: do-install-info
  do-install-info: unstage install-info-host install-info-target stage
--- 1219,1226 ----
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
      maybe-TAGS-target-rda \
!     maybe-TAGS-target-libada \
!     maybe-TAGS-target-libphobos
  
  .PHONY: do-install-info
  do-install-info: unstage install-info-host install-info-target stage
***************
*** 1307,1313 ****
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
      maybe-install-info-target-rda \
!     maybe-install-info-target-libada
  
  .PHONY: do-installcheck
  do-installcheck: unstage installcheck-host installcheck-target stage
--- 1313,1320 ----
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
      maybe-install-info-target-rda \
!     maybe-install-info-target-libada \
!     maybe-install-info-target-libphobos
  
  .PHONY: do-installcheck
  do-installcheck: unstage installcheck-host installcheck-target stage
***************
*** 1400,1406 ****
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
      maybe-installcheck-target-rda \
!     maybe-installcheck-target-libada
  
  .PHONY: do-mostlyclean
  do-mostlyclean: unstage mostlyclean-host mostlyclean-target stage
--- 1407,1414 ----
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
      maybe-installcheck-target-rda \
!     maybe-installcheck-target-libada \
!     maybe-installcheck-target-libphobos
  
  .PHONY: do-mostlyclean
  do-mostlyclean: unstage mostlyclean-host mostlyclean-target stage
***************
*** 1493,1499 ****
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
      maybe-mostlyclean-target-rda \
!     maybe-mostlyclean-target-libada
  
  .PHONY: do-clean
  do-clean: unstage clean-host clean-target stage
--- 1501,1508 ----
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
      maybe-mostlyclean-target-rda \
!     maybe-mostlyclean-target-libada \
!     maybe-mostlyclean-target-libphobos
  
  .PHONY: do-clean
  do-clean: unstage clean-host clean-target stage
***************
*** 1586,1592 ****
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
      maybe-clean-target-rda \
!     maybe-clean-target-libada
  
  .PHONY: do-distclean
  do-distclean: unstage distclean-host distclean-target stage
--- 1595,1602 ----
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
      maybe-clean-target-rda \
!     maybe-clean-target-libada \
!     maybe-clean-target-libphobos
  
  .PHONY: do-distclean
  do-distclean: unstage distclean-host distclean-target stage
***************
*** 1679,1685 ****
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
      maybe-distclean-target-rda \
!     maybe-distclean-target-libada
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean: unstage maintainer-clean-host maintainer-clean-target stage
--- 1689,1696 ----
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
      maybe-distclean-target-rda \
!     maybe-distclean-target-libada \
!     maybe-distclean-target-libphobos
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean: unstage maintainer-clean-host maintainer-clean-target stage
***************
*** 1772,1778 ****
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
      maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libada
  
  
  # Here are the targets which correspond to the do-X targets.
--- 1783,1790 ----
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
      maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libada \
!     maybe-maintainer-clean-target-libphobos
  
  
  # Here are the targets which correspond to the do-X targets.
***************
*** 1921,1927 ****
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
      maybe-check-target-rda \
!     maybe-check-target-libada stage
  
  # Automated reporting of test results.
  
--- 1933,1940 ----
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
      maybe-check-target-rda \
!     maybe-check-target-libada \
!     maybe-check-target-libphobos stage
  
  # Automated reporting of test results.
  
***************
*** 2105,2111 ****
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
      maybe-install-target-rda \
!     maybe-install-target-libada
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
--- 2118,2125 ----
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
      maybe-install-target-rda \
!     maybe-install-target-libada \
!     maybe-install-target-libphobos
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
***************
*** 30361,30366 ****
--- 30375,30722 ----
  @endif target-libada
  
  
+ .PHONY: configure-target-libphobos maybe-configure-target-libphobos
+ maybe-configure-target-libphobos:
+ @if target-libphobos
+ maybe-configure-target-libphobos: configure-target-libphobos
+ 
+ # There's only one multilib.out.  Cleverer subdirs shouldn't need it copied.
+ $(TARGET_SUBDIR)/libphobos/multilib.out: multilib.out
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+ 	rm -f $(TARGET_SUBDIR)/libphobos/Makefile || : ; \
+ 	cp multilib.out $(TARGET_SUBDIR)/libphobos/multilib.out
+ 
+ configure-target-libphobos: $(TARGET_SUBDIR)/libphobos/multilib.out
+ 	@test ! -f $(TARGET_SUBDIR)/libphobos/Makefile || exit 0; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo Configuring in $(TARGET_SUBDIR)/libphobos; \
+ 	cd "$(TARGET_SUBDIR)/libphobos" || exit 1; \
+ 	case $(srcdir) in \
+ 	  /* | [A-Za-z]:[\\/]*) \
+ 	    topdir=$(srcdir) ;; \
+ 	  *) \
+ 	    case "$(TARGET_SUBDIR)" in \
+ 	      .) topdir="../$(srcdir)" ;; \
+ 	      *) topdir="../../$(srcdir)" ;; \
+ 	    esac ;; \
+ 	esac; \
+ 	  srcdiroption="--srcdir=$${topdir}/libphobos"; \
+ 	  libsrcdir="$$s/libphobos"; \
+ 	rm -f no-such-file || : ; \
+ 	CONFIG_SITE=no-such-file $(SHELL) $${libsrcdir}/configure \
+ 	  $(TARGET_CONFIGARGS) $${srcdiroption} \
+ 	  --with-target-subdir="$(TARGET_SUBDIR)"  \
+ 	  || exit 1
+ @endif target-libphobos
+ 
+ .PHONY: all-target-libphobos maybe-all-target-libphobos
+ maybe-all-target-libphobos:
+ @if target-libphobos
+ TARGET-target-libphobos=all
+ maybe-all-target-libphobos: all-target-libphobos
+ all-target-libphobos: configure-target-libphobos
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)   $(TARGET-target-libphobos))
+ @endif target-libphobos
+ 
+ .PHONY: check-target-libphobos maybe-check-target-libphobos
+ maybe-check-target-libphobos:
+ @if target-libphobos
+ maybe-check-target-libphobos: check-target-libphobos
+ 
+ check-target-libphobos:
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
  
  # ----------
  # GCC module
***************
*** 34964,34969 ****
--- 35320,35327 ----
  
  configure-target-libada: maybe-all-gcc
  
+ configure-target-libphobos: maybe-all-gcc
+ 
  
  
  configure-target-boehm-gc: maybe-all-target-newlib maybe-all-target-libgloss
diff -c gcc-4.0.3-orig/configure gcc-4.0.3/configure
*** gcc-4.0.3-orig/configure	Tue Sep 13 03:01:28 2005
--- gcc-4.0.3/configure	Fri Nov 10 15:31:52 2006
***************
*** 939,944 ****
--- 939,945 ----
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
+  		target-libphobos \
  		target-libada"
  
  # these tools are built using the target libraries, and are intended to
***************
*** 1781,1786 ****
--- 1782,1788 ----
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    CC_FOR_TARGET=${CC_FOR_TARGET-${target_alias}-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    CXX_FOR_TARGET=${CXX_FOR_TARGET-${target_alias}-c++}
    GCJ_FOR_TARGET=${GCJ_FOR_TARGET-${target_alias}-gcj}
    GCC_FOR_TARGET=${GCC_FOR_TARGET-${CC_FOR_TARGET-${target_alias}-gcc}}
***************
*** 1797,1802 ****
--- 1799,1805 ----
    # This is all going to change when we autoconfiscate...
  
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    GCC_FOR_TARGET="\$(USUAL_GCC_FOR_TARGET)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
***************
*** 5143,5148 ****
--- 5146,5152 ----
  s%@target_configargs@%$target_configargs%g
  s%@target_configdirs@%$target_configdirs%g
  s%@CC_FOR_BUILD@%$CC_FOR_BUILD%g
+ s%@CXX_FOR_BUILD@%$CXX_FOR_BUILD%g
  s%@config_shell@%$config_shell%g
  s%@AR@%$AR%g
  s%@ncn_cv_AR@%$ncn_cv_AR%g
diff -c gcc-4.0.3-orig/configure.in gcc-4.0.3/configure.in
*** gcc-4.0.3-orig/configure.in	Tue Sep 13 03:01:28 2005
--- gcc-4.0.3/configure.in	Fri Nov 10 15:33:46 2006
***************
*** 173,178 ****
--- 173,179 ----
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
+  		target-libphobos \
  		target-libada"
  
  # these tools are built using the target libraries, and are intended to
***************
*** 990,995 ****
--- 991,997 ----
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    CC_FOR_TARGET=${CC_FOR_TARGET-${target_alias}-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    CXX_FOR_TARGET=${CXX_FOR_TARGET-${target_alias}-c++}
    GCJ_FOR_TARGET=${GCJ_FOR_TARGET-${target_alias}-gcj}
    GCC_FOR_TARGET=${GCC_FOR_TARGET-${CC_FOR_TARGET-${target_alias}-gcc}}
***************
*** 1006,1011 ****
--- 1008,1014 ----
    # This is all going to change when we autoconfiscate...
  
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    GCC_FOR_TARGET="\$(USUAL_GCC_FOR_TARGET)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
***************
*** 2172,2177 ****
--- 2175,2181 ----
  
  # Build tools.
  AC_SUBST(CC_FOR_BUILD)
+ AC_SUBST(CXX_FOR_BUILD)
  AC_SUBST(config_shell)
  
  # Host tools.
