diff -c gcc-4.0.4-orig/configure gcc-4.0.4/configure
*** gcc-4.0.4-orig/configure	2006-11-21 13:02:38.000000000 -0500
--- gcc-4.0.4/configure	2010-08-19 17:21:02.332716934 -0400
***************
*** 939,945 ****
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
! 		target-libada"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 939,947 ----
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
! 		target-libada \
! 		target-libphobos \
! 		target-libdruntime"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
***************
*** 1781,1786 ****
--- 1783,1789 ----
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    CC_FOR_TARGET=${CC_FOR_TARGET-${target_alias}-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    CXX_FOR_TARGET=${CXX_FOR_TARGET-${target_alias}-c++}
    GCJ_FOR_TARGET=${GCJ_FOR_TARGET-${target_alias}-gcj}
    GCC_FOR_TARGET=${GCC_FOR_TARGET-${CC_FOR_TARGET-${target_alias}-gcc}}
***************
*** 1797,1802 ****
--- 1800,1806 ----
    # This is all going to change when we autoconfiscate...
  
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    GCC_FOR_TARGET="\$(USUAL_GCC_FOR_TARGET)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
***************
*** 5149,5154 ****
--- 5153,5159 ----
  s%@target_configargs@%$target_configargs%g
  s%@target_configdirs@%$target_configdirs%g
  s%@CC_FOR_BUILD@%$CC_FOR_BUILD%g
+ s%@CXX_FOR_BUILD@%$CXX_FOR_BUILD%g
  s%@config_shell@%$config_shell%g
  s%@AR@%$AR%g
  s%@ncn_cv_AR@%$ncn_cv_AR%g
diff -c gcc-4.0.4-orig/configure.in gcc-4.0.4/configure.in
*** gcc-4.0.4-orig/configure.in	2006-11-21 13:02:38.000000000 -0500
--- gcc-4.0.4/configure.in	2010-08-19 17:22:17.368717032 -0400
***************
*** 173,179 ****
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
! 		target-libada"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 173,182 ----
  		target-libgfortran \
  		${libgcj} \
  		target-libobjc \
! 		target-libada \
! 		target-libphobos \
! 		target-libdruntime"
! 		
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
***************
*** 990,995 ****
--- 993,999 ----
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    CC_FOR_TARGET=${CC_FOR_TARGET-${target_alias}-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    CXX_FOR_TARGET=${CXX_FOR_TARGET-${target_alias}-c++}
    GCJ_FOR_TARGET=${GCJ_FOR_TARGET-${target_alias}-gcj}
    GCC_FOR_TARGET=${GCC_FOR_TARGET-${CC_FOR_TARGET-${target_alias}-gcc}}
***************
*** 1006,1011 ****
--- 1010,1016 ----
    # This is all going to change when we autoconfiscate...
  
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    GCC_FOR_TARGET="\$(USUAL_GCC_FOR_TARGET)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
***************
*** 2178,2183 ****
--- 2183,2189 ----
  
  # Build tools.
  AC_SUBST(CC_FOR_BUILD)
+ AC_SUBST(CXX_FOR_BUILD)
  AC_SUBST(config_shell)
  
  # Host tools.
diff -c gcc-4.0.4-orig/Makefile.def gcc-4.0.4/Makefile.def
*** gcc-4.0.4-orig/Makefile.def	2005-10-20 07:55:29.000000000 -0400
--- gcc-4.0.4/Makefile.def	2010-08-19 17:19:29.040716886 -0400
***************
*** 134,139 ****
--- 134,141 ----
  target_modules = { module= qthreads; };
  target_modules = { module= rda; };
  target_modules = { module= libada; };
+ target_modules = { module= libphobos; };
+ target_modules = { module= libdruntime; };
  
  // These are (some of) the make targets to be done in each subdirectory.
  // Not all; these are the ones which don't have special options.
diff -c gcc-4.0.4-orig/Makefile.in gcc-4.0.4/Makefile.in
*** gcc-4.0.4-orig/Makefile.in	2006-04-04 17:04:37.000000000 -0400
--- gcc-4.0.4/Makefile.in	2010-08-19 17:26:17.000000000 -0400
***************
*** 735,741 ****
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
!     maybe-configure-target-libada
  
  # The target built for a native non-bootstrap build.
  .PHONY: all
--- 735,743 ----
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
      maybe-configure-target-rda \
!     maybe-configure-target-libada \
!     maybe-configure-target-libphobos \
!     maybe-configure-target-libdruntime
  
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
--- 839,847 ----
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
      maybe-all-target-rda \
!     maybe-all-target-libada \
!     maybe-all-target-libphobos \
!     maybe-all-target-libdruntime
  
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
--- 939,947 ----
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
      maybe-info-target-rda \
!     maybe-info-target-libada \
!     maybe-info-target-libphobos \
!     maybe-info-target-libdruntime
  
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
--- 1034,1042 ----
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
      maybe-dvi-target-rda \
!     maybe-dvi-target-libada \
!     maybe-dvi-target-libphobos \
!     maybe-dvi-target-libdruntime
  
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
--- 1129,1137 ----
      maybe-html-target-boehm-gc \
      maybe-html-target-qthreads \
      maybe-html-target-rda \
!     maybe-html-target-libada \
!     maybe-html-target-libphobos \
!     maybe-html-target-libdruntime
  
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
--- 1224,1232 ----
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
      maybe-TAGS-target-rda \
!     maybe-TAGS-target-libada \
!     maybe-TAGS-target-libphobos \
!     maybe-TAGS-target-libdruntime
  
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
--- 1319,1327 ----
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
      maybe-install-info-target-rda \
!     maybe-install-info-target-libada \
!     maybe-install-info-target-libphobos \
!     maybe-install-info-target-libdruntime
  
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
--- 1414,1422 ----
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
      maybe-installcheck-target-rda \
!     maybe-installcheck-target-libada \
!     maybe-installcheck-target-libphobos \
!     maybe-installcheck-target-libdruntime
  
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
--- 1509,1517 ----
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
      maybe-mostlyclean-target-rda \
!     maybe-mostlyclean-target-libada \
!     maybe-mostlyclean-target-libphobos \
!     maybe-mostlyclean-target-libdruntime
  
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
--- 1604,1612 ----
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
      maybe-clean-target-rda \
!     maybe-clean-target-libada \
!     maybe-clean-target-libphobos \
!     maybe-clean-target-libdruntime
  
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
--- 1699,1707 ----
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
      maybe-distclean-target-rda \
!     maybe-distclean-target-libada \
!     maybe-distclean-target-libphobos \
!     maybe-distclean-target-libdruntime
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean: unstage maintainer-clean-host maintainer-clean-target stage
***************
*** 1772,1778 ****
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
      maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libada
  
  
  # Here are the targets which correspond to the do-X targets.
--- 1794,1802 ----
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
      maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libada \
!     maybe-maintainer-clean-target-libphobos \
!     maybe-maintainer-clean-target-libdruntime
  
  
  # Here are the targets which correspond to the do-X targets.
***************
*** 1921,1927 ****
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
      maybe-check-target-rda \
!     maybe-check-target-libada stage
  
  # Automated reporting of test results.
  
--- 1945,1953 ----
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
      maybe-check-target-rda \
!     maybe-check-target-libada \
!     maybe-check-target-libphobos \
!     maybe-check-target-libdruntime stage
  
  # Automated reporting of test results.
  
***************
*** 2105,2111 ****
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
      maybe-install-target-rda \
!     maybe-install-target-libada
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
--- 2131,2139 ----
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
      maybe-install-target-rda \
!     maybe-install-target-libada \
!     maybe-install-target-libphobos \
!     maybe-install-target-libdruntime
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
***************
*** 30361,30366 ****
--- 30389,31078 ----
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
+ .PHONY: configure-target-libdruntime maybe-configure-target-libdruntime
+ maybe-configure-target-libdruntime:
+ @if target-libdruntime
+ maybe-configure-target-libdruntime: configure-target-libdruntime
+ 
+ # There's only one multilib.out.  Cleverer subdirs shouldn't need it copied.
+ $(TARGET_SUBDIR)/libdruntime/multilib.out: multilib.out
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libdruntime ; \
+ 	rm -f $(TARGET_SUBDIR)/libdruntime/Makefile || : ; \
+ 	cp multilib.out $(TARGET_SUBDIR)/libdruntime/multilib.out
+ 
+ configure-target-libdruntime: $(TARGET_SUBDIR)/libdruntime/multilib.out
+ 	@test ! -f $(TARGET_SUBDIR)/libdruntime/Makefile || exit 0; \
+ 	$(SHELL) $(srcdir)/mkinstalldirs $(TARGET_SUBDIR)/libdruntime ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo Configuring in $(TARGET_SUBDIR)/libdruntime; \
+ 	cd "$(TARGET_SUBDIR)/libdruntime" || exit 1; \
+ 	case $(srcdir) in \
+ 	  /* | [A-Za-z]:[\\/]*) \
+ 	    topdir=$(srcdir) ;; \
+ 	  *) \
+ 	    case "$(TARGET_SUBDIR)" in \
+ 	      .) topdir="../$(srcdir)" ;; \
+ 	      *) topdir="../../$(srcdir)" ;; \
+ 	    esac ;; \
+ 	esac; \
+ 	  srcdiroption="--srcdir=$${topdir}/libdruntime"; \
+ 	  libsrcdir="$$s/libdruntime"; \
+ 	rm -f no-such-file || : ; \
+ 	CONFIG_SITE=no-such-file $(SHELL) $${libsrcdir}/configure \
+ 	  $(TARGET_CONFIGARGS) $${srcdiroption} \
+ 	  --with-target-subdir="$(TARGET_SUBDIR)"  \
+ 	  || exit 1
+ @endif target-libdruntime
+ 
+ .PHONY: all-target-libdruntime maybe-all-target-libdruntime
+ maybe-all-target-libdruntime:
+ @if target-libdruntime
+ TARGET-target-libdruntime=all
+ maybe-all-target-libdruntime: all-target-libdruntime
+ all-target-libdruntime: configure-target-libdruntime
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)   $(TARGET-target-libdruntime))
+ @endif target-libdruntime
+ 
+ .PHONY: check-target-libdruntime maybe-check-target-libdruntime
+ maybe-check-target-libdruntime:
+ @if target-libdruntime
+ maybe-check-target-libdruntime: check-target-libdruntime
+ 
+ check-target-libdruntime:
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
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
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  install)
+ 
+ @endif target-libdruntime
+ 
+ # Other targets (info, dvi, etc.)
+ 
+ .PHONY: maybe-info-target-libdruntime info-target-libdruntime
+ maybe-info-target-libdruntime:
+ @if target-libdruntime
+ maybe-info-target-libdruntime: info-target-libdruntime
+ 
+ info-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing info in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing dvi in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           dvi) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing html in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing TAGS in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing install-info in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           install-info) \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing installcheck in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing mostlyclean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing clean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing distclean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
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
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	$(NORMAL_TARGET_EXPORTS) \
+ 	echo "Doing maintainer-clean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	           maintainer-clean) \
+ 	  || exit 1
+ 
+ @endif target-libdruntime
+ 
+ 
  
  # ----------
  # GCC module
***************
*** 34964,34969 ****
--- 35676,35685 ----
  
  configure-target-libada: maybe-all-gcc
  
+ configure-target-libphobos: maybe-all-gcc
+ 
+ configure-target-libdruntime: maybe-all-gcc
+ 
  
  
  configure-target-boehm-gc: maybe-all-target-newlib maybe-all-target-libgloss
