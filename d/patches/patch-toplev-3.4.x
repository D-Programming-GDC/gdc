diff -c gcc-3.4.6-orig/configure gcc-3.4.6/configure
*** gcc-3.4.6-orig/configure	2005-03-08 12:31:40.000000000 -0500
--- gcc-3.4.6/configure	2010-08-19 17:09:28.756716760 -0400
***************
*** 898,904 ****
  		target-libstdc++-v3 \
  		target-libf2c \
  		${libgcj} \
! 		target-libobjc"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 898,906 ----
  		target-libstdc++-v3 \
  		target-libf2c \
  		${libgcj} \
! 		target-libobjc \
! 		target-libphobos \
! 		target-libdruntime"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
***************
*** 2130,2135 ****
--- 2132,2138 ----
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    CC_FOR_TARGET=${CC_FOR_TARGET-${target_alias}-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    CXX_FOR_TARGET=${CXX_FOR_TARGET-${target_alias}-c++}
    GCJ_FOR_TARGET=${GCJ_FOR_TARGET-${target_alias}-gcj}
    GCC_FOR_TARGET=${GCC_FOR_TARGET-${CC_FOR_TARGET-${target_alias}-gcc}}
***************
*** 2188,2193 ****
--- 2191,2197 ----
  
    BISON="\$(USUAL_BISON)"
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    GCC_FOR_TARGET="\$(USUAL_GCC_FOR_TARGET)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
***************
*** 4293,4298 ****
--- 4297,4303 ----
  s%@target_configdirs@%$target_configdirs%g
  s%@BISON@%$BISON%g
  s%@CC_FOR_BUILD@%$CC_FOR_BUILD%g
+ s%@CXX_FOR_BUILD@%$CXX_FOR_BUILD%g
  s%@LEX@%$LEX%g
  s%@MAKEINFO@%$MAKEINFO%g
  s%@YACC@%$YACC%g
diff -c gcc-3.4.6-orig/configure.in gcc-3.4.6/configure.in
*** gcc-3.4.6-orig/configure.in	2005-03-08 12:31:40.000000000 -0500
--- gcc-3.4.6/configure.in	2010-08-19 17:10:45.808716807 -0400
***************
*** 158,164 ****
  		target-libstdc++-v3 \
  		target-libf2c \
  		${libgcj} \
! 		target-libobjc"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
--- 158,166 ----
  		target-libstdc++-v3 \
  		target-libf2c \
  		${libgcj} \
! 		target-libobjc \
! 		target-libphobos \
! 		target-libdruntime"
  
  # these tools are built using the target libraries, and are intended to
  # run only in the target environment
***************
*** 1369,1374 ****
--- 1371,1377 ----
    CXXFLAGS=${CXXFLAGS-"-g -O2"}
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    CC_FOR_TARGET=${CC_FOR_TARGET-${target_alias}-gcc}
+   CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
    CXX_FOR_TARGET=${CXX_FOR_TARGET-${target_alias}-c++}
    GCJ_FOR_TARGET=${GCJ_FOR_TARGET-${target_alias}-gcj}
    GCC_FOR_TARGET=${GCC_FOR_TARGET-${CC_FOR_TARGET-${target_alias}-gcc}}
***************
*** 1427,1432 ****
--- 1430,1436 ----
  
    BISON="\$(USUAL_BISON)"
    CC_FOR_BUILD="\$(CC)"
+   CXX_FOR_BUILD="\$(CXX)"
    GCC_FOR_TARGET="\$(USUAL_GCC_FOR_TARGET)"
    BUILD_PREFIX=
    BUILD_PREFIX_1=loser-
***************
*** 2085,2090 ****
--- 2089,2095 ----
  # Build tools.
  AC_SUBST(BISON)
  AC_SUBST(CC_FOR_BUILD)
+ AC_SUBST(CXX_FOR_BUILD)
  AC_SUBST(LEX)
  AC_SUBST(MAKEINFO)
  AC_SUBST(YACC)
diff -c gcc-3.4.6-orig/Makefile.def gcc-3.4.6/Makefile.def
*** gcc-3.4.6-orig/Makefile.def	2004-01-14 15:09:37.000000000 -0500
--- gcc-3.4.6/Makefile.def	2010-08-19 17:06:52.160728630 -0400
***************
*** 116,121 ****
--- 116,123 ----
  target_modules = { module= boehm-gc; };
  target_modules = { module= qthreads; };
  target_modules = { module= rda; };
+ target_modules = { module= libphobos; };
+ target_modules = { module= libdruntime; };
  
  // These are (some of) the make targets to be done in each subdirectory.
  // Not all; these are the ones which don't have special options.
diff -c gcc-3.4.6-orig/Makefile.in gcc-3.4.6/Makefile.in
*** gcc-3.4.6-orig/Makefile.in	2004-09-23 20:43:53.000000000 -0400
--- gcc-3.4.6/Makefile.in	2010-08-19 17:12:53.000000000 -0400
***************
*** 612,618 ****
      maybe-configure-target-zlib \
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
!     maybe-configure-target-rda
  
  # The target built for a native build.
  .PHONY: all.normal
--- 612,620 ----
      maybe-configure-target-zlib \
      maybe-configure-target-boehm-gc \
      maybe-configure-target-qthreads \
!     maybe-configure-target-rda \
!     maybe-configure-target-libphobos \
!     maybe-configure-target-libdruntime
  
  # The target built for a native build.
  .PHONY: all.normal
***************
*** 701,707 ****
      maybe-all-target-zlib \
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
!     maybe-all-target-rda
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
--- 703,711 ----
      maybe-all-target-zlib \
      maybe-all-target-boehm-gc \
      maybe-all-target-qthreads \
!     maybe-all-target-rda \
!     maybe-all-target-libphobos \
!     maybe-all-target-libdruntime
  
  # Do a target for all the subdirectories.  A ``make do-X'' will do a
  # ``make X'' in all subdirectories (because, in general, there is a
***************
*** 795,801 ****
      maybe-info-target-zlib \
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
!     maybe-info-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-info-gcc info-gcc
--- 799,807 ----
      maybe-info-target-zlib \
      maybe-info-target-boehm-gc \
      maybe-info-target-qthreads \
!     maybe-info-target-rda \
!     maybe-info-target-libphobos \
!     maybe-info-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-info-gcc info-gcc
***************
*** 2583,2588 ****
--- 2589,2638 ----
  	  || exit 1
  
  
+ .PHONY: maybe-info-target-libphobos info-target-libphobos
+ maybe-info-target-libphobos:
+ 
+ info-target-libphobos: \
+     configure-target-libphobos 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing info in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          info) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-info-target-libdruntime info-target-libdruntime
+ maybe-info-target-libdruntime:
+ 
+ info-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing info in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          info) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-dvi
  do-dvi: dvi-host dvi-target
***************
*** 2671,2677 ****
      maybe-dvi-target-zlib \
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
!     maybe-dvi-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-dvi-gcc dvi-gcc
--- 2721,2729 ----
      maybe-dvi-target-zlib \
      maybe-dvi-target-boehm-gc \
      maybe-dvi-target-qthreads \
!     maybe-dvi-target-rda \
!     maybe-dvi-target-libphobos \
!     maybe-dvi-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-dvi-gcc dvi-gcc
***************
*** 4459,4464 ****
--- 4511,4560 ----
  	  || exit 1
  
  
+ .PHONY: maybe-dvi-target-libphobos dvi-target-libphobos
+ maybe-dvi-target-libphobos:
+ 
+ dvi-target-libphobos: \
+     configure-target-libphobos 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing dvi in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          dvi) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-dvi-target-libdruntime dvi-target-libdruntime
+ maybe-dvi-target-libdruntime:
+ 
+ dvi-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing dvi in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          dvi) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-TAGS
  do-TAGS: TAGS-host TAGS-target
***************
*** 4547,4553 ****
      maybe-TAGS-target-zlib \
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
!     maybe-TAGS-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-TAGS-gcc TAGS-gcc
--- 4643,4651 ----
      maybe-TAGS-target-zlib \
      maybe-TAGS-target-boehm-gc \
      maybe-TAGS-target-qthreads \
!     maybe-TAGS-target-rda \
!     maybe-TAGS-target-libphobos \
!     maybe-TAGS-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-TAGS-gcc TAGS-gcc
***************
*** 6335,6340 ****
--- 6433,6482 ----
  	  || exit 1
  
  
+ .PHONY: maybe-TAGS-target-libphobos TAGS-target-libphobos
+ maybe-TAGS-target-libphobos:
+ 
+ TAGS-target-libphobos: \
+     configure-target-libphobos 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing TAGS in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          TAGS) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-TAGS-target-libdruntime TAGS-target-libdruntime
+ maybe-TAGS-target-libdruntime:
+ 
+ TAGS-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing TAGS in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          TAGS) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-install-info
  do-install-info: install-info-host install-info-target
***************
*** 6423,6429 ****
      maybe-install-info-target-zlib \
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
!     maybe-install-info-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-install-info-gcc install-info-gcc
--- 6565,6573 ----
      maybe-install-info-target-zlib \
      maybe-install-info-target-boehm-gc \
      maybe-install-info-target-qthreads \
!     maybe-install-info-target-rda \
!     maybe-install-info-target-libphobos \
!     maybe-install-info-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-install-info-gcc install-info-gcc
***************
*** 8292,8297 ****
--- 8436,8487 ----
  	  || exit 1
  
  
+ .PHONY: maybe-install-info-target-libphobos install-info-target-libphobos
+ maybe-install-info-target-libphobos:
+ 
+ install-info-target-libphobos: \
+     configure-target-libphobos \
+     info-target-libphobos 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing install-info in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          install-info) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-install-info-target-libdruntime install-info-target-libdruntime
+ maybe-install-info-target-libdruntime:
+ 
+ install-info-target-libdruntime: \
+     configure-target-libdruntime \
+     info-target-libdruntime 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing install-info in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          install-info) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-installcheck
  do-installcheck: installcheck-host installcheck-target
***************
*** 8380,8386 ****
      maybe-installcheck-target-zlib \
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
!     maybe-installcheck-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-installcheck-gcc installcheck-gcc
--- 8570,8578 ----
      maybe-installcheck-target-zlib \
      maybe-installcheck-target-boehm-gc \
      maybe-installcheck-target-qthreads \
!     maybe-installcheck-target-rda \
!     maybe-installcheck-target-libphobos \
!     maybe-installcheck-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-installcheck-gcc installcheck-gcc
***************
*** 10168,10173 ****
--- 10360,10409 ----
  	  || exit 1
  
  
+ .PHONY: maybe-installcheck-target-libphobos installcheck-target-libphobos
+ maybe-installcheck-target-libphobos:
+ 
+ installcheck-target-libphobos: \
+     configure-target-libphobos 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing installcheck in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          installcheck) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-installcheck-target-libdruntime installcheck-target-libdruntime
+ maybe-installcheck-target-libdruntime:
+ 
+ installcheck-target-libdruntime: \
+     configure-target-libdruntime 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing installcheck in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          installcheck) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-mostlyclean
  do-mostlyclean: mostlyclean-host mostlyclean-target
***************
*** 10256,10262 ****
      maybe-mostlyclean-target-zlib \
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
!     maybe-mostlyclean-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-mostlyclean-gcc mostlyclean-gcc
--- 10492,10500 ----
      maybe-mostlyclean-target-zlib \
      maybe-mostlyclean-target-boehm-gc \
      maybe-mostlyclean-target-qthreads \
!     maybe-mostlyclean-target-rda \
!     maybe-mostlyclean-target-libphobos \
!     maybe-mostlyclean-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-mostlyclean-gcc mostlyclean-gcc
***************
*** 11921,11926 ****
--- 12159,12206 ----
  	  || exit 1
  
  
+ .PHONY: maybe-mostlyclean-target-libphobos mostlyclean-target-libphobos
+ maybe-mostlyclean-target-libphobos:
+ 
+ mostlyclean-target-libphobos: 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing mostlyclean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          mostlyclean) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-mostlyclean-target-libdruntime mostlyclean-target-libdruntime
+ maybe-mostlyclean-target-libdruntime:
+ 
+ mostlyclean-target-libdruntime: 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing mostlyclean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          mostlyclean) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-clean
  do-clean: clean-host clean-target
***************
*** 12009,12015 ****
      maybe-clean-target-zlib \
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
!     maybe-clean-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-clean-gcc clean-gcc
--- 12289,12297 ----
      maybe-clean-target-zlib \
      maybe-clean-target-boehm-gc \
      maybe-clean-target-qthreads \
!     maybe-clean-target-rda \
!     maybe-clean-target-libphobos \
!     maybe-clean-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-clean-gcc clean-gcc
***************
*** 13688,13693 ****
--- 13970,14017 ----
  	  || exit 1
  
  
+ .PHONY: maybe-clean-target-libphobos clean-target-libphobos
+ maybe-clean-target-libphobos:
+ 
+ clean-target-libphobos: 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing clean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          clean) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-clean-target-libdruntime clean-target-libdruntime
+ maybe-clean-target-libdruntime:
+ 
+ clean-target-libdruntime: 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing clean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          clean) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-distclean
  do-distclean: distclean-host distclean-target
***************
*** 13776,13782 ****
      maybe-distclean-target-zlib \
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
!     maybe-distclean-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-distclean-gcc distclean-gcc
--- 14100,14108 ----
      maybe-distclean-target-zlib \
      maybe-distclean-target-boehm-gc \
      maybe-distclean-target-qthreads \
!     maybe-distclean-target-rda \
!     maybe-distclean-target-libphobos \
!     maybe-distclean-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-distclean-gcc distclean-gcc
***************
*** 15455,15460 ****
--- 15781,15828 ----
  	  || exit 1
  
  
+ .PHONY: maybe-distclean-target-libphobos distclean-target-libphobos
+ maybe-distclean-target-libphobos:
+ 
+ distclean-target-libphobos: 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing distclean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          distclean) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-distclean-target-libdruntime distclean-target-libdruntime
+ maybe-distclean-target-libdruntime:
+ 
+ distclean-target-libdruntime: 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing distclean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          distclean) \
+ 	  || exit 1
+ 
+ 
  
  .PHONY: do-maintainer-clean
  do-maintainer-clean: maintainer-clean-host maintainer-clean-target
***************
*** 15543,15549 ****
      maybe-maintainer-clean-target-zlib \
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
!     maybe-maintainer-clean-target-rda
  
  # GCC, the eternal special case
  .PHONY: maybe-maintainer-clean-gcc maintainer-clean-gcc
--- 15911,15919 ----
      maybe-maintainer-clean-target-zlib \
      maybe-maintainer-clean-target-boehm-gc \
      maybe-maintainer-clean-target-qthreads \
!     maybe-maintainer-clean-target-rda \
!     maybe-maintainer-clean-target-libphobos \
!     maybe-maintainer-clean-target-libdruntime
  
  # GCC, the eternal special case
  .PHONY: maybe-maintainer-clean-gcc maintainer-clean-gcc
***************
*** 17222,17227 ****
--- 17592,17639 ----
  	  || exit 1
  
  
+ .PHONY: maybe-maintainer-clean-target-libphobos maintainer-clean-target-libphobos
+ maybe-maintainer-clean-target-libphobos:
+ 
+ maintainer-clean-target-libphobos: 
+ 	@[ -f $(TARGET_SUBDIR)/libphobos/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing maintainer-clean in $(TARGET_SUBDIR)/libphobos" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          maintainer-clean) \
+ 	  || exit 1
+ 
+ 
+ .PHONY: maybe-maintainer-clean-target-libdruntime maintainer-clean-target-libdruntime
+ maybe-maintainer-clean-target-libdruntime:
+ 
+ maintainer-clean-target-libdruntime: 
+ 	@[ -f $(TARGET_SUBDIR)/libdruntime/Makefile ] || exit 0 ; \
+ 	r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	echo "Doing maintainer-clean in $(TARGET_SUBDIR)/libdruntime" ; \
+ 	for flag in $(EXTRA_TARGET_FLAGS); do \
+ 	  eval `echo "$$flag" | sed -e "s|^\([^=]*\)=\(.*\)|\1='\2'; export \1|"`; \
+ 	done; \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(BASE_FLAGS_TO_PASS) "AR=$${AR}" "AS=$${AS}" \
+ 	          "CC=$${CC}" "CXX=$${CXX}" "LD=$${LD}" "NM=$${NM}" \
+ 	          "RANLIB=$${RANLIB}" \
+ 	          "DLLTOOL=$${DLLTOOL}" "WINDRES=$${WINDRES}" \
+ 	          maintainer-clean) \
+ 	  || exit 1
+ 
+ 
  
  
  # Here are the targets which correspond to the do-X targets.
***************
*** 17364,17370 ****
      maybe-check-target-zlib \
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
!     maybe-check-target-rda
  
  # Automated reporting of test results.
  
--- 17776,17784 ----
      maybe-check-target-zlib \
      maybe-check-target-boehm-gc \
      maybe-check-target-qthreads \
!     maybe-check-target-rda \
!     maybe-check-target-libphobos \
!     maybe-check-target-libdruntime
  
  # Automated reporting of test results.
  
***************
*** 17543,17549 ****
      maybe-install-target-zlib \
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
!     maybe-install-target-rda
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
--- 17957,17965 ----
      maybe-install-target-zlib \
      maybe-install-target-boehm-gc \
      maybe-install-target-qthreads \
!     maybe-install-target-rda \
!     maybe-install-target-libphobos \
!     maybe-install-target-libdruntime
  
  uninstall:
  	@echo "the uninstall target is not supported in this tree"
***************
*** 23309,23314 ****
--- 23725,23894 ----
  	  $(MAKE) $(TARGET_FLAGS_TO_PASS) install)
  
  
+ .PHONY: configure-target-libphobos maybe-configure-target-libphobos
+ maybe-configure-target-libphobos:
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
+ 	AR="$(AR_FOR_TARGET)"; export AR; \
+ 	AS="$(AS_FOR_TARGET)"; export AS; \
+ 	CC="$(CC_FOR_TARGET)"; export CC; \
+ 	CFLAGS="$(CFLAGS_FOR_TARGET)"; export CFLAGS; \
+ 	CONFIG_SHELL="$(SHELL)"; export CONFIG_SHELL; \
+ 	CPPFLAGS="$(CFLAGS_FOR_TARGET)"; export CPPFLAGS; \
+ 	CXX="$(CXX_FOR_TARGET)"; export CXX; \
+ 	CXXFLAGS="$(CXXFLAGS_FOR_TARGET)"; export CXXFLAGS; \
+ 	GCJ="$(GCJ_FOR_TARGET)"; export GCJ; \
+ 	DLLTOOL="$(DLLTOOL_FOR_TARGET)"; export DLLTOOL; \
+ 	LD="$(LD_FOR_TARGET)"; export LD; \
+ 	LDFLAGS="$(LDFLAGS_FOR_TARGET)"; export LDFLAGS; \
+ 	NM="$(NM_FOR_TARGET)"; export NM; \
+ 	RANLIB="$(RANLIB_FOR_TARGET)"; export RANLIB; \
+ 	WINDRES="$(WINDRES_FOR_TARGET)"; export WINDRES; \
+ 	SET_GCC_LIB_PATH_CMD="@SET_GCC_LIB_PATH@"; export SET_GCC_LIB_PATH_CMD; \
+ 	@SET_GCC_LIB_PATH@ \
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
+ 	  --with-target-subdir="$(TARGET_SUBDIR)" \
+ 	  || exit 1
+ 
+ .PHONY: all-target-libphobos maybe-all-target-libphobos
+ maybe-all-target-libphobos:
+ all-target-libphobos: configure-target-libphobos
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  all)
+ 
+ .PHONY: check-target-libphobos maybe-check-target-libphobos
+ maybe-check-target-libphobos:
+ 
+ check-target-libphobos:
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  check)
+ 
+ 
+ .PHONY: install-target-libphobos maybe-install-target-libphobos
+ maybe-install-target-libphobos:
+ 
+ install-target-libphobos: installdirs
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	(cd $(TARGET_SUBDIR)/libphobos && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS) install)
+ 
+ 
+ .PHONY: configure-target-libdruntime maybe-configure-target-libdruntime
+ maybe-configure-target-libdruntime:
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
+ 	AR="$(AR_FOR_TARGET)"; export AR; \
+ 	AS="$(AS_FOR_TARGET)"; export AS; \
+ 	CC="$(CC_FOR_TARGET)"; export CC; \
+ 	CFLAGS="$(CFLAGS_FOR_TARGET)"; export CFLAGS; \
+ 	CONFIG_SHELL="$(SHELL)"; export CONFIG_SHELL; \
+ 	CPPFLAGS="$(CFLAGS_FOR_TARGET)"; export CPPFLAGS; \
+ 	CXX="$(CXX_FOR_TARGET)"; export CXX; \
+ 	CXXFLAGS="$(CXXFLAGS_FOR_TARGET)"; export CXXFLAGS; \
+ 	GCJ="$(GCJ_FOR_TARGET)"; export GCJ; \
+ 	DLLTOOL="$(DLLTOOL_FOR_TARGET)"; export DLLTOOL; \
+ 	LD="$(LD_FOR_TARGET)"; export LD; \
+ 	LDFLAGS="$(LDFLAGS_FOR_TARGET)"; export LDFLAGS; \
+ 	NM="$(NM_FOR_TARGET)"; export NM; \
+ 	RANLIB="$(RANLIB_FOR_TARGET)"; export RANLIB; \
+ 	WINDRES="$(WINDRES_FOR_TARGET)"; export WINDRES; \
+ 	SET_GCC_LIB_PATH_CMD="@SET_GCC_LIB_PATH@"; export SET_GCC_LIB_PATH_CMD; \
+ 	@SET_GCC_LIB_PATH@ \
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
+ 	  --with-target-subdir="$(TARGET_SUBDIR)" \
+ 	  || exit 1
+ 
+ .PHONY: all-target-libdruntime maybe-all-target-libdruntime
+ maybe-all-target-libdruntime:
+ all-target-libdruntime: configure-target-libdruntime
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  all)
+ 
+ .PHONY: check-target-libdruntime maybe-check-target-libdruntime
+ maybe-check-target-libdruntime:
+ 
+ check-target-libdruntime:
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS)  check)
+ 
+ 
+ .PHONY: install-target-libdruntime maybe-install-target-libdruntime
+ maybe-install-target-libdruntime:
+ 
+ install-target-libdruntime: installdirs
+ 	@r=`${PWD_COMMAND}`; export r; \
+ 	s=`cd $(srcdir); ${PWD_COMMAND}`; export s; \
+ 	$(SET_LIB_PATH) \
+ 	(cd $(TARGET_SUBDIR)/libdruntime && \
+ 	  $(MAKE) $(TARGET_FLAGS_TO_PASS) install)
+ 
+ 
  
  # ----------
  # GCC module
