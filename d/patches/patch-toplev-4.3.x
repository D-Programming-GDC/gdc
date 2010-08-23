diff -c gcc-4.3.5-orig/configure gcc-4.3.5/configure
*** gcc-4.3.5-orig/configure	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/configure	2010-08-22 20:16:50.074317683 -0400
***************
*** 1875,1881 ****
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 1875,1882 ----
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada \
! 		target-libphobos"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
diff -c gcc-4.3.5-orig/configure.ac gcc-4.3.5/configure.ac
*** gcc-4.3.5-orig/configure.ac	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/configure.ac	2010-08-22 20:17:03.518316972 -0400
***************
*** 185,191 ****
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 185,192 ----
  		target-boehm-gc \
  		${libgcj} \
  		target-libobjc \
! 		target-libada \
! 		target-libphobos"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
diff -c gcc-4.3.5-orig/Makefile.def gcc-4.3.5/Makefile.def
*** gcc-4.3.5-orig/Makefile.def	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/Makefile.def	2010-08-22 20:17:15.398318133 -0400
***************
*** 143,148 ****
--- 143,149 ----
  target_modules = { module= rda; };
  target_modules = { module= libada; };
  target_modules = { module= libgomp; lib_path=.libs; };
+ target_modules = { module= libphobos; };
  
  // These are (some of) the make targets to be done in each subdirectory.
  // Not all; these are the ones which don't have special options.
diff -c gcc-4.3.5-orig/Makefile.in gcc-4.3.5/Makefile.in
*** gcc-4.3.5-orig/Makefile.in	2009-04-25 01:55:24.000000000 -0400
--- gcc-4.3.5/Makefile.in	2010-08-22 20:17:58.000000000 -0400
***************
*** 723,729 ****
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
      maybe-configure-target-libada \
!     maybe-configure-target-libgomp
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
--- 723,730 ----
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
      maybe-configure-target-libada \
!     maybe-configure-target-libgomp \
!     maybe-configure-target-libphobos
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
***************
*** 877,882 ****
--- 878,884 ----
  all-target: maybe-all-target-rda
  all-target: maybe-all-target-libada
  all-target: maybe-all-target-libgomp
+ all-target: maybe-all-target-libphobos
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
***************
*** 988,993 ****
--- 990,996 ----
  info-target: maybe-info-target-rda
  info-target: maybe-info-target-libada
  info-target: maybe-info-target-libgomp
+ info-target: maybe-info-target-libphobos
  
  .PHONY: do-dvi
  do-dvi:
***************
*** 1094,1099 ****
--- 1097,1103 ----
  dvi-target: maybe-dvi-target-rda
  dvi-target: maybe-dvi-target-libada
  dvi-target: maybe-dvi-target-libgomp
+ dvi-target: maybe-dvi-target-libphobos
  
  .PHONY: do-pdf
  do-pdf:
***************
*** 1200,1205 ****
--- 1204,1210 ----
  pdf-target: maybe-pdf-target-rda
  pdf-target: maybe-pdf-target-libada
  pdf-target: maybe-pdf-target-libgomp
+ pdf-target: maybe-pdf-target-libphobos
  
  .PHONY: do-html
  do-html:
***************
*** 1306,1311 ****
--- 1311,1317 ----
  html-target: maybe-html-target-rda
  html-target: maybe-html-target-libada
  html-target: maybe-html-target-libgomp
+ html-target: maybe-html-target-libphobos
  
  .PHONY: do-TAGS
  do-TAGS:
***************
*** 1412,1417 ****
--- 1418,1424 ----
  TAGS-target: maybe-TAGS-target-rda
  TAGS-target: maybe-TAGS-target-libada
  TAGS-target: maybe-TAGS-target-libgomp
+ TAGS-target: maybe-TAGS-target-libphobos
  
  .PHONY: do-install-info
  do-install-info:
***************
*** 1518,1523 ****
--- 1525,1531 ----
  install-info-target: maybe-install-info-target-rda
  install-info-target: maybe-install-info-target-libada
  install-info-target: maybe-install-info-target-libgomp
+ install-info-target: maybe-install-info-target-libphobos
  
  .PHONY: do-install-pdf
  do-install-pdf:
***************
*** 1624,1629 ****
--- 1632,1638 ----
  install-pdf-target: maybe-install-pdf-target-rda
  install-pdf-target: maybe-install-pdf-target-libada
  install-pdf-target: maybe-install-pdf-target-libgomp
+ install-pdf-target: maybe-install-pdf-target-libphobos
  
  .PHONY: do-install-html
  do-install-html:
***************
*** 1730,1735 ****
--- 1739,1745 ----
  install-html-target: maybe-install-html-target-rda
  install-html-target: maybe-install-html-target-libada
  install-html-target: maybe-install-html-target-libgomp
+ install-html-target: maybe-install-html-target-libphobos
  
  .PHONY: do-installcheck
  do-installcheck:
***************
*** 1836,1841 ****
--- 1846,1852 ----
  installcheck-target: maybe-installcheck-target-rda
  installcheck-target: maybe-installcheck-target-libada
  installcheck-target: maybe-installcheck-target-libgomp
+ installcheck-target: maybe-installcheck-target-libphobos
  
  .PHONY: do-mostlyclean
  do-mostlyclean:
***************
*** 1942,1947 ****
--- 1953,1959 ----
  mostlyclean-target: maybe-mostlyclean-target-rda
  mostlyclean-target: maybe-mostlyclean-target-libada
  mostlyclean-target: maybe-mostlyclean-target-libgomp
+ mostlyclean-target: maybe-mostlyclean-target-libphobos
  
  .PHONY: do-clean
  do-clean:
***************
*** 2048,2053 ****
--- 2060,2066 ----
  clean-target: maybe-clean-target-rda
  clean-target: maybe-clean-target-libada
  clean-target: maybe-clean-target-libgomp
+ clean-target: maybe-clean-target-libphobos
  
  .PHONY: do-distclean
  do-distclean:
***************
*** 2154,2159 ****
--- 2167,2173 ----
  distclean-target: maybe-distclean-target-rda
  distclean-target: maybe-distclean-target-libada
  distclean-target: maybe-distclean-target-libgomp
+ distclean-target: maybe-distclean-target-libphobos
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean:
***************
*** 2260,2265 ****
--- 2274,2280 ----
  maintainer-clean-target: maybe-maintainer-clean-target-rda
  maintainer-clean-target: maybe-maintainer-clean-target-libada
  maintainer-clean-target: maybe-maintainer-clean-target-libgomp
+ maintainer-clean-target: maybe-maintainer-clean-target-libphobos
  
  
  # Here are the targets which correspond to the do-X targets.
***************
*** 2419,2425 ****
      maybe-check-target-qthreads \
      maybe-check-target-rda \
      maybe-check-target-libada \
!     maybe-check-target-libgomp
  
  do-check:
  	@: $(MAKE); $(unstage)
--- 2434,2441 ----
      maybe-check-target-qthreads \
      maybe-check-target-rda \
      maybe-check-target-libada \
!     maybe-check-target-libgomp \
!     maybe-check-target-libphobos
  
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
--- 2640,2647 ----
      maybe-install-target-qthreads \
      maybe-install-target-rda \
      maybe-install-target-libada \
!     maybe-install-target-libgomp \
!     maybe-install-target-libphobos
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
***************
*** 49104,49109 ****
--- 49121,49567 ----
  
  
  
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
  # ----------
  # GCC module
  # ----------
***************
*** 51217,51222 ****
--- 51675,51681 ----
  configure-target-rda: stage_last
  configure-target-libada: stage_last
  configure-target-libgomp: stage_last
+ configure-target-libphobos: stage_last
  @endif gcc-bootstrap
  
  @if gcc-no-bootstrap
***************
*** 51241,51246 ****
--- 51700,51706 ----
  configure-target-rda: maybe-all-gcc
  configure-target-libada: maybe-all-gcc
  configure-target-libgomp: maybe-all-gcc
+ configure-target-libphobos: maybe-all-gcc
  @endif gcc-no-bootstrap
  
  
***************
*** 51932,51937 ****
--- 52392,52398 ----
  configure-target-rda: maybe-all-target-libgcc
  configure-target-libada: maybe-all-target-libgcc
  configure-target-libgomp: maybe-all-target-libgcc
+ configure-target-libphobos: maybe-all-target-libgcc
  @endif gcc-no-bootstrap
  
  
***************
*** 51975,51980 ****
--- 52436,52443 ----
  
  configure-target-libgomp: maybe-all-target-newlib maybe-all-target-libgloss
  
+ configure-target-libphobos: maybe-all-target-newlib maybe-all-target-libgloss
+ 
  
  CONFIGURE_GDB_TK = @CONFIGURE_GDB_TK@
  GDB_TK = @GDB_TK@
