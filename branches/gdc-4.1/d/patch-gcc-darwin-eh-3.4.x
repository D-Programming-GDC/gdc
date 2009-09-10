*** gcc-orig/config/darwin.h	Sat Sep 11 16:32:17 2004
--- gcc/config/darwin.h	Wed Jan 19 17:07:34 2005
***************
*** 644,650 ****
  		".section __DATA,__gcc_except_tab", 0)	\
  SECTION_FUNCTION (darwin_eh_frame_section,		\
  		in_darwin_eh_frame,			\
! 		".section __TEXT,__eh_frame", 0)	\
  							\
  static void					\
  objc_section_init (void)			\
--- 644,650 ----
  		".section __DATA,__gcc_except_tab", 0)	\
  SECTION_FUNCTION (darwin_eh_frame_section,		\
  		in_darwin_eh_frame,			\
! 		".section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms", 0)/*df*/	\
  							\
  static void					\
  objc_section_init (void)			\
*** gcc-orig/dwarf2out.c	Mon Oct 25 17:46:45 2004
--- gcc/dwarf2out.c	Sat May  7 21:19:29 2005
***************
*** 390,396 ****
--- 390,400 ----
  #define FUNC_END_LABEL		"LFE"
  #endif
  
+ #ifdef DARWIN_REGISTER_TARGET_PRAGMAS
+ #define FRAME_BEGIN_LABEL	"EH_frame"
+ #else
  #define FRAME_BEGIN_LABEL	"Lframe"
+ #endif
  #define CIE_AFTER_SIZE_LABEL	"LSCIE"
  #define CIE_END_LABEL		"LECIE"
  #define FDE_LABEL		"LSFDE"
