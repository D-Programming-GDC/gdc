diff -c gcc-4.3.5-orig/configure gcc-4.3.5/configure
*** gcc-4.3.5-orig/configure	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/configure	2010-08-19 19:15:55.424815746 -0400
***************
*** 1875,1881 ****
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 1875,1883 ----
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada \
! 		target-libphobos \
! 		target-libdruntime"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
diff -c gcc-4.3.5-orig/configure.ac gcc-4.3.5/configure.ac
*** gcc-4.3.5-orig/configure.ac	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/configure.ac	2010-08-19 19:16:09.112815882 -0400
***************
*** 185,191 ****
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 185,193 ----
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada \
! 		target-libphobos \
! 		target-libdruntime"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
diff -c gcc-4.3.5-orig/Makefile.def gcc-4.3.5/Makefile.def
*** gcc-4.3.5-orig/Makefile.def	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/Makefile.def	2010-08-19 19:15:25.668816084 -0400
***************
*** 143,148 ****
--- 143,150 ----
  target_modules = { module= rda; };
  target_modules = { module= libada; };
  target_modules = { module= libgomp; lib_path=.libs; };
+ target_modules = { module= libphobos; };
+ target_modules = { module= libdruntime; };
  
  // These are (some of) the make targets to be done in each subdirectory.
  // Not all; these are the ones which don't have special options.
diff -c gcc-4.3.5-orig/Makefile.in gcc-4.3.5/Makefile.in
*** gcc-4.3.5-orig/Makefile.in	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/Makefile.in	2010-08-19 19:16:16.000000000 -0400
***************
*** 723,729 ****
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
      maybe-configure-target-libada \
!     maybe-configure-target-libgomp
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
--- 723,731 ----
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
      maybe-configure-target-libada \
!     maybe-configure-target-libgomp \
!     maybe-configure-target-libphobos \
!     maybe-configure-target-libdruntime
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
***************
*** 877,882 ****
--- 879,886 ----
  all-target: maybe-all-target-rda
  all-target: maybe-all-target-libada
  all-target: maybe-all-target-libgomp
+ all-target: maybe-all-target-libphobos
+ all-target: maybe-all-target-libdruntime
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
***************
*** 988,993 ****
--- 992,999 ----
  info-target: maybe-info-target-rda
  info-target: maybe-info-target-libada
  info-target: maybe-info-target-libgomp
+ info-target: maybe-info-target-libphobos
+ info-target: maybe-info-target-libdruntime
  
  .PHONY: do-dvi
  do-dvi:
***************
*** 1094,1099 ****
--- 1100,1107 ----
  dvi-target: maybe-dvi-target-rda
  dvi-target: maybe-dvi-target-libada
  dvi-target: maybe-dvi-target-libgomp
+ dvi-target: maybe-dvi-target-libphobos
+ dvi-target: maybe-dvi-target-libdruntime
  
  .PHONY: do-pdf
  do-pdf:
***************
*** 1200,1205 ****
--- 1208,1215 ----
  pdf-target: maybe-pdf-target-rda
  pdf-target: maybe-pdf-target-libada
  pdf-target: maybe-pdf-target-libgomp
+ pdf-target: maybe-pdf-target-libphobos
+ pdf-target: maybe-pdf-target-libdruntime
  
  .PHONY: do-html
  do-html:
***************
*** 1306,1311 ****
--- 1316,1323 ----
  html-target: maybe-html-target-rda
  html-target: maybe-html-target-libada
  html-target: maybe-html-target-libgomp
+ html-target: maybe-html-target-libphobos
+ html-target: maybe-html-target-libdruntime
  
  .PHONY: do-TAGS
  do-TAGS:
***************
*** 1412,1417 ****
--- 1424,1431 ----
  TAGS-target: maybe-TAGS-target-rda
  TAGS-target: maybe-TAGS-target-libada
  TAGS-target: maybe-TAGS-target-libgomp
+ TAGS-target: maybe-TAGS-target-libphobos
+ TAGS-target: maybe-TAGS-target-libdruntime
  
  .PHONY: do-install-info
  do-install-info:
***************
*** 1518,1523 ****
--- 1532,1539 ----
  install-info-target: maybe-install-info-target-rda
  install-info-target: maybe-install-info-target-libada
  install-info-target: maybe-install-info-target-libgomp
+ install-info-target: maybe-install-info-target-libphobos
+ install-info-target: maybe-install-info-target-libdruntime
  
  .PHONY: do-install-pdf
  do-install-pdf:
***************
*** 1624,1629 ****
--- 1640,1647 ----
  install-pdf-target: maybe-install-pdf-target-rda
  install-pdf-target: maybe-install-pdf-target-libada
  install-pdf-target: maybe-install-pdf-target-libgomp
+ install-pdf-target: maybe-install-pdf-target-libphobos
+ install-pdf-target: maybe-install-pdf-target-libdruntime
  
  .PHONY: do-install-html
  do-install-html:
***************
*** 1730,1735 ****
--- 1748,1755 ----
  install-html-target: maybe-install-html-target-rda
  install-html-target: maybe-install-html-target-libada
  install-html-target: maybe-install-html-target-libgomp
+ install-html-target: maybe-install-html-target-libphobos
+ install-html-target: maybe-install-html-target-libdruntime
  
  .PHONY: do-installcheck
  do-installcheck:
***************
*** 1836,1841 ****
--- 1856,1863 ----
  installcheck-target: maybe-installcheck-target-rda
  installcheck-target: maybe-installcheck-target-libada
  installcheck-target: maybe-installcheck-target-libgomp
+ installcheck-target: maybe-installcheck-target-libphobos
+ installcheck-target: maybe-installcheck-target-libdruntime
  
  .PHONY: do-mostlyclean
  do-mostlyclean:
***************
*** 1942,1947 ****
--- 1964,1971 ----
  mostlyclean-target: maybe-mostlyclean-target-rda
  mostlyclean-target: maybe-mostlyclean-target-libada
  mostlyclean-target: maybe-mostlyclean-target-libgomp
+ mostlyclean-target: maybe-mostlyclean-target-libphobos
+ mostlyclean-target: maybe-mostlyclean-target-libdruntime
  
  .PHONY: do-clean
  do-clean:
***************
*** 2048,2053 ****
--- 2072,2079 ----
  clean-target: maybe-clean-target-rda
  clean-target: maybe-clean-target-libada
  clean-target: maybe-clean-target-libgomp
+ clean-target: maybe-clean-target-libphobos
+ clean-target: maybe-clean-target-libdruntime
  
  .PHONY: do-distclean
  do-distclean:
***************
*** 2154,2159 ****
--- 2180,2187 ----
  distclean-target: maybe-distclean-target-rda
  distclean-target: maybe-distclean-target-libada
  distclean-target: maybe-distclean-target-libgomp
+ distclean-target: maybe-distclean-target-libphobos
+ distclean-target: maybe-distclean-target-libdruntime
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean:
***************
*** 2260,2265 ****
--- 2288,2295 ----
  maintainer-clean-target: maybe-maintainer-clean-target-rda
  maintainer-clean-target: maybe-maintainer-clean-target-libada
  maintainer-clean-target: maybe-maintainer-clean-target-libgomp
+ maintainer-clean-target: maybe-maintainer-clean-target-libphobos
+ maintainer-clean-target: maybe-maintainer-clean-target-libdruntime
  
  
  # Here are the targets which correspond to the do-X targets.
***************
*** 2419,2425 ****
      maybe-check-target-qthreads \
      maybe-check-target-rda \
      maybe-check-target-libada \
!     maybe-check-target-libgomp
  
  do-check:
  	@: $(MAKE); $(unstage)
--- 2449,2457 ----
      maybe-check-target-qthreads \
      maybe-check-target-rda \
      maybe-check-target-libada \
!     maybe-check-target-libgomp \
!     maybe-check-target-libphobos \
!     maybe-check-target-libdruntime
  
  do-check:
  	@: $(MAKE); $(unstage)
***************
*** 2624,2630 ****
      maybe-install-target-qthreads \
      maybe-install-target-rda \
      maybe-install-target-libada \
!     maybe-install-target-libgomp
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
--- 2656,2664 ----
      maybe-install-target-qthreads \
      maybe-install-target-rda \
      maybe-install-target-libada \
!     maybe-install-target-libgomp \
!     maybe-install-target-libphobos \
!     maybe-install-target-libdruntime
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
***************
*** 49104,49109 ****
--- 49138,50025 ----
  
  
  
+ 
+ 
+ .PHONY: configure-target-libphobos maybe-configure-target-libphobos
+ maybe-configure-target-libphobos:
+ @if gcc-bootstrap
+ configure-target-libphobos: stage_current
+ @endif gcc-bootstrap
+ @if target-libphobos
+ maybe-configure-target-libphobos: configure-target-libphobos
+ configure-target-libphobos: 
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	echo "Checking multilib configuration for libphobos..."; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
+ 	$(CC_FOR_TARGET) --print-multi-lib > $(TARGET_SUBDIR)/libphobos/multilib.tmp 2> /dev/null ; \
+ 	if test -r $(TARGET_SUBDIR)/libphobos/multilib.out; then \
+ 	  if cmp -s $(TARGET_SUBDIR)/libphobos/multilib.tmp $(TARGET_SUBDIR)/libphobos/multilib.out; then \
+ 	    rm -f $(TARGET_SUBDIR)/libphobos/multilib.tmp; \
+ 	  else \
+ 	    rm -f $(TARGET_SUBDIR)/libphobos/Makefile; \
+ 	    mv $(TARGET_SUBDIR)/libphobos/multilib.tmp $(TARGET_SUBDIR)/libphobos/multilib.out; \
+ 	  fi; \
+ 	else \
+ 	  mv $(TARGET_SUBDIR)/libphobos/multilib.tmp $(TARGET_SUBDIR)/libphobos/multilib.out; \
+ 	fi; \
+ 	test ! -f $(TARGET_SUBDIR)/libphobos/Makefile || exit 0; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libphobos ; \
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
+ 	  $(TARGET_CONFIGARGS) --build=${build_alias} --host=${target_alias} \
+ 	  --target=${target_alias} $${srcdiroption}  \
+ 	  || exit 1
+ @endif target-libphobos
+ 
+ 
+ 
+ 
+ 
+ .PHONY: all-target-libphobos maybe-all-target-libphobos
+ maybe-all-target-libphobos:
+ @if gcc-bootstrap
+ all-target-libphobos: stage_current
+ @endif gcc-bootstrap
+ @if target-libphobos
+ TARGET-target-libphobos=all
+ maybe-all-target-libphobos: all-target-libphobos
+ all-target-libphobos: configure-target-libphobos
+ 	@: $(MAKE); $(unstage)
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
+ # Other targets (info, dvi, pdf, etc.)
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           dvi) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-pdf-target-libphobos pdf-target-libphobos
+ maybe-pdf-target-libphobos:
+ @if target-libphobos
+ maybe-pdf-target-libphobos: pdf-target-libphobos
+ 
+ pdf-target-libphobos: \
+     configure-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing pdf in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           pdf) \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           install-info) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-install-pdf-target-libphobos install-pdf-target-libphobos
+ maybe-install-pdf-target-libphobos:
+ @if target-libphobos
+ maybe-install-pdf-target-libphobos: install-pdf-target-libphobos
+ 
+ install-pdf-target-libphobos: \
+     configure-target-libphobos \
+     pdf-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-pdf in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           install-pdf) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ .PHONY: maybe-install-html-target-libphobos install-html-target-libphobos
+ maybe-install-html-target-libphobos:
+ @if target-libphobos
+ maybe-install-html-target-libphobos: install-html-target-libphobos
+ 
+ install-html-target-libphobos: \
+     configure-target-libphobos \
+     html-target-libphobos 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-html in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           install-html) \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
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
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           maintainer-clean) \
+ 	  || exit 1
+ 
+ @endif target-libphobos
+ 
+ 
+ 
+ 
+ 
+ .PHONY: configure-target-libdruntime maybe-configure-target-libdruntime
+ maybe-configure-target-libdruntime:
+ @if gcc-bootstrap
+ configure-target-libdruntime: stage_current
+ @endif gcc-bootstrap
+ @if target-libdruntime
+ maybe-configure-target-libdruntime: configure-target-libdruntime
+ configure-target-libdruntime: 
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	echo "Checking multilib configuration for libdruntime..."; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libdruntime ; \
+ 	$(CC_FOR_TARGET) --print-multi-lib > $(TARGET_SUBDIR)/libdruntime/multilib.tmp 2> /dev/null ; \
+ 	if test -r $(TARGET_SUBDIR)/libdruntime/multilib.out; then \
+ 	  if cmp -s $(TARGET_SUBDIR)/libdruntime/multilib.tmp $(TARGET_SUBDIR)/libdruntime/multilib.out; then \
+ 	    rm -f $(TARGET_SUBDIR)/libdruntime/multilib.tmp; \
+ 	  else \
+ 	    rm -f $(TARGET_SUBDIR)/libdruntime/Makefile; \
+ 	    mv $(TARGET_SUBDIR)/libdruntime/multilib.tmp $(TARGET_SUBDIR)/libdruntime/multilib.out; \
+ 	  fi; \
+ 	else \
+ 	  mv $(TARGET_SUBDIR)/libdruntime/multilib.tmp $(TARGET_SUBDIR)/libdruntime/multilib.out; \
+ 	fi; \
+ 	test ! -f $(TARGET_SUBDIR)/libdruntime/Makefile || exit 0; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libdruntime ; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo Configuring in $(TARGET_SUBDIR)/libdruntime; \
+ 	cd "$(TARGET_SUBDIR)/libdruntime" || exit 1; \
+ 	case $(srcdir) in \
+ 	  /* | [A-Za-z]:[\\/]*) topdir=$(srcdir) ;; \
+ 	  *) topdir=`echo $(TARGET_SUBDIR)/libdruntime/ | \
+ 		sed -e 's,\./,,g' -e 's,[^/]*/,../,g' `$(srcdir) ;; \
+ 	esac; \
+ 	srcdiroption="--srcdir=$${topdir}/libdruntime"; \
+ 	libsrcdir="$$s/libdruntime"; \
+ 	rm -f no-such-file || : ; \
+ 	CONFIG_SITE=no-such-file $(SHELL) $${libsrcdir}/configure \
+ 	  $(TARGET_CONFIGARGS) --build=${build_alias} --host=${target_alias} \
+ 	  --target=${target_alias} $${srcdiroption}  \
+ 	  || exit 1
+ @endif target-libdruntime
+ 
+ 
+ 
+ 
+ 
+ .PHONY: all-target-libdruntime maybe-all-target-libdruntime
+ maybe-all-target-libdruntime:
+ @if gcc-bootstrap
+ all-target-libdruntime: stage_current
+ @endif gcc-bootstrap
+ @if target-libdruntime
+ TARGET-target-libdruntime=all
+ maybe-all-target-libdruntime: all-target-libdruntime
+ all-target-libdruntime: configure-target-libdruntime
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  $(TARGET-target-libdruntime))
+ @endif target-libdruntime
+ 
+ 
+ 
+ 
+ 
+ .PHONY: check-target-libdruntime maybe-check-target-libdruntime
+ maybe-check-target-libdruntime:
+ @if target-libdruntime
+ maybe-check-target-libdruntime: check-target-libdruntime
+ 
+ check-target-libdruntime:
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)   check)
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: install-target-libdruntime maybe-install-target-libdruntime
+ maybe-install-target-libdruntime:
+ @if target-libdruntime
+ maybe-install-target-libdruntime: install-target-libdruntime
+ 
+ install-target-libdruntime: installdirs
+ 	@: $(MAKE); $(unstage)
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  install)
+ 
+ @endif target-libdruntime
+ 
+ # Other targets (info, dvi, pdf, etc.)
+ 
+ .PHONY: maybe-info-target-libdruntime info-target-libdruntime
+ maybe-info-target-libdruntime:
+ @if target-libdruntime
+ maybe-info-target-libdruntime: info-target-libdruntime
+ 
+ info-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing info in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           info) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-dvi-target-libdruntime dvi-target-libdruntime
+ maybe-dvi-target-libdruntime:
+ @if target-libdruntime
+ maybe-dvi-target-libdruntime: dvi-target-libdruntime
+ 
+ dvi-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing dvi in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           dvi) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-pdf-target-libdruntime pdf-target-libdruntime
+ maybe-pdf-target-libdruntime:
+ @if target-libdruntime
+ maybe-pdf-target-libdruntime: pdf-target-libdruntime
+ 
+ pdf-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing pdf in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           pdf) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-html-target-libdruntime html-target-libdruntime
+ maybe-html-target-libdruntime:
+ @if target-libdruntime
+ maybe-html-target-libdruntime: html-target-libdruntime
+ 
+ html-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing html in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           html) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-TAGS-target-libdruntime TAGS-target-libdruntime
+ maybe-TAGS-target-libdruntime:
+ @if target-libdruntime
+ maybe-TAGS-target-libdruntime: TAGS-target-libdruntime
+ 
+ TAGS-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing TAGS in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           TAGS) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-install-info-target-libdruntime install-info-target-libdruntime
+ maybe-install-info-target-libdruntime:
+ @if target-libdruntime
+ maybe-install-info-target-libdruntime: install-info-target-libdruntime
+ 
+ install-info-target-libdruntime: \
+     configure-target-libdruntime \
+     info-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-info in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           install-info) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-install-pdf-target-libdruntime install-pdf-target-libdruntime
+ maybe-install-pdf-target-libdruntime:
+ @if target-libdruntime
+ maybe-install-pdf-target-libdruntime: install-pdf-target-libdruntime
+ 
+ install-pdf-target-libdruntime: \
+     configure-target-libdruntime \
+     pdf-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-pdf in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           install-pdf) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-install-html-target-libdruntime install-html-target-libdruntime
+ maybe-install-html-target-libdruntime:
+ @if target-libdruntime
+ maybe-install-html-target-libdruntime: install-html-target-libdruntime
+ 
+ install-html-target-libdruntime: \
+     configure-target-libdruntime \
+     html-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-html in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           install-html) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-installcheck-target-libdruntime installcheck-target-libdruntime
+ maybe-installcheck-target-libdruntime:
+ @if target-libdruntime
+ maybe-installcheck-target-libdruntime: installcheck-target-libdruntime
+ 
+ installcheck-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing installcheck in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           installcheck) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-mostlyclean-target-libdruntime mostlyclean-target-libdruntime
+ maybe-mostlyclean-target-libdruntime:
+ @if target-libdruntime
+ maybe-mostlyclean-target-libdruntime: mostlyclean-target-libdruntime
+ 
+ mostlyclean-target-libdruntime: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing mostlyclean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           mostlyclean) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-clean-target-libdruntime clean-target-libdruntime
+ maybe-clean-target-libdruntime:
+ @if target-libdruntime
+ maybe-clean-target-libdruntime: clean-target-libdruntime
+ 
+ clean-target-libdruntime: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing clean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           clean) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-distclean-target-libdruntime distclean-target-libdruntime
+ maybe-distclean-target-libdruntime:
+ @if target-libdruntime
+ maybe-distclean-target-libdruntime: distclean-target-libdruntime
+ 
+ distclean-target-libdruntime: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing distclean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           distclean) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ .PHONY: maybe-maintainer-clean-target-libdruntime maintainer-clean-target-libdruntime
+ maybe-maintainer-clean-target-libdruntime:
+ @if target-libdruntime
+ maybe-maintainer-clean-target-libdruntime: maintainer-clean-target-libdruntime
+ 
+ maintainer-clean-target-libdruntime: 
+ 	@: $(MAKE); $(unstage)
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing maintainer-clean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" "WINDMC=$${WINDMC}" \
+ 	           maintainer-clean) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ 
+ 
  # ----------
  # GCC module
  # ----------
***************
*** 51217,51222 ****
--- 52133,52140 ----
  configure-target-rda: stage_last
  configure-target-libada: stage_last
  configure-target-libgomp: stage_last
+ configure-target-libphobos: stage_last
+ configure-target-libdruntime: stage_last
  @endif gcc-bootstrap
  
  @if gcc-no-bootstrap
***************
*** 51241,51246 ****
--- 52159,52166 ----
  configure-target-rda: maybe-all-gcc
  configure-target-libada: maybe-all-gcc
  configure-target-libgomp: maybe-all-gcc
+ configure-target-libphobos: maybe-all-gcc
+ configure-target-libdruntime: maybe-all-gcc
  @endif gcc-no-bootstrap
  
  
***************
*** 51932,51937 ****
--- 52852,52859 ----
  configure-target-rda: maybe-all-target-libgcc
  configure-target-libada: maybe-all-target-libgcc
  configure-target-libgomp: maybe-all-target-libgcc
+ configure-target-libphobos: maybe-all-target-libgcc
+ configure-target-libdruntime: maybe-all-target-libgcc
  @endif gcc-no-bootstrap
  
  
***************
*** 51975,51980 ****
--- 52897,52906 ----
  
  configure-target-libgomp: maybe-all-target-newlib maybe-all-target-libgloss
  
+ configure-target-libphobos: maybe-all-target-newlib maybe-all-target-libgloss
+ 
+ configure-target-libdruntime: maybe-all-target-newlib maybe-all-target-libgloss
+ 
  
  CONFIGURE_GDB_TK = @CONFIGURE_GDB_TK@
  GDB_TK = @GDB_TK@
