## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = foreign dejagnu

# Setup the testing framework, if you have one
EXPECT = $(shell if test -f $(top_builddir)/../expect/expect; then \
	   echo $(top_builddir)/../expect/expect; else echo expect; fi)

_RUNTEST = $(shell if test -f $(top_srcdir)/../dejagnu/runtest; then \
	     echo $(top_srcdir)/../dejagnu/runtest; else echo runtest; fi)
RUNTEST = "$(_RUNTEST) $(AM_RUNTESTFLAGS)"

AM_MAKEFLAGS = "EXEEXT=$(EXEEXT)"

CLEANFILES = *.log *.sum
