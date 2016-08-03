# set from top makefile
OS:=
MODEL:=
DMD:=
DRUNTIME:=
DRUNTIMESO:=../../src/.libs/libgphobos2.so
REAL_DRUNTIMESO:=../../libdruntime/.libs/libgdruntime.so
QUIET:=
LINKDL:= -ldl
LDL:=$(LINKDL)
CC:=
GCC_CFLAGS:=
GCC_DFLAGS:=
abs_srcdir:=
abs_top_srcdir:=
VPATH = $(abs_srcdir)

QUIET:=
SRC:=src
ROOT:=.
TESTS:=link load linkD linkDR loadDR host finalize
TESTS+=link_linkdep load_linkdep link_loaddep load_loaddep load_13414

ifneq (default,$(MODEL))
	MODEL_FLAG:=-m$(MODEL)
endif
D_EXTRA_DFLAGS=-nostdinc -I $(abs_top_srcdir)/libdruntime -I $(abs_top_srcdir)/src \
	-I ../../libdruntime -I $(abs_srcdir)/src
D_EXTRA_LFLAGS=-L../../src/.libs/ -L../../libdruntime/.libs/
LOCAL_DFLAGS=$(GCC_DFLAGS) $(D_EXTRA_DFLAGS) $(D_EXTRA_LFLAGS) -shared-libphobos
LOCAL_CFLAGS=$(GCC_CFLAGS) -Wl,-rpath-link ../../src/.libs/ -Wl,-rpath-link ../../libdruntime/.libs/
LD_LIB_PATH=../../src/.libs/:../../libdruntime/.libs/:.

.PHONY: all clean
all: $(addprefix $(ROOT)/,$(addsuffix .done,$(TESTS)))

$(ROOT)/loadDR.done: RUN_ARGS:=$(DRUNTIMESO)

$(ROOT)/%.done: $(ROOT)/%
	@echo Testing $*
	$(QUIET)LD_LIBRARY_PATH=$(LD_LIB_PATH) $(ROOT)/$* $(RUN_ARGS)
	@touch $@

$(ROOT)/link: $(SRC)/link.d $(ROOT)/lib.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(ROOT)/lib.so

$(ROOT)/link_linkdep: $(SRC)/link_linkdep.d $(ROOT)/lib.so $(ROOT)/liblinkdep.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(LINKFLAGS) $(ROOT)/liblinkdep.so $(ROOT)/lib.so

$(ROOT)/load_linkdep: $(SRC)/load_linkdep.d $(ROOT)/lib.so $(ROOT)/liblinkdep.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(LINKFLAGS) $(LINKDL)

$(ROOT)/link_loaddep: $(SRC)/link_loaddep.d $(ROOT)/lib.so $(ROOT)/libloaddep.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(LINKFLAGS) $(ROOT)/libloaddep.so

$(ROOT)/load_loaddep: $(SRC)/load_loaddep.d $(ROOT)/lib.so $(ROOT)/libloaddep.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(LINKFLAGS) $(LINKDL)

$(ROOT)/load $(ROOT)/finalize: $(ROOT)/%: $(SRC)/%.d $(ROOT)/lib.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(LINKDL)

$(ROOT)/load_13414: $(ROOT)/%: $(SRC)/%.d $(ROOT)/lib_13414.so $(DRUNTIMESO)
	$(QUIET)$(DMD) $(LOCAL_DFLAGS) -o $@ $< $(LINKDL)

$(ROOT)/linkD: $(SRC)/linkD.c $(ROOT)/lib.so $(DRUNTIMESO)
	$(QUIET)$(CC) $(LOCAL_CFLAGS) -o $@ $< $(ROOT)/lib.so $(LDL) -pthread

$(ROOT)/linkDR: $(SRC)/linkDR.c $(ROOT)/lib.so $(DRUNTIMESO)
	$(QUIET)$(CC) $(LOCAL_CFLAGS) -o $@ $< $(DRUNTIMESO) $(REAL_DRUNTIMESO) $(LDL) -pthread

$(ROOT)/loadDR: $(SRC)/loadDR.c $(ROOT)/lib.so $(DRUNTIMESO)
	$(QUIET)$(CC) $(LOCAL_CFLAGS) -o $@ $< $(LDL) -pthread

$(ROOT)/host: $(SRC)/host.c $(ROOT)/plugin1.so $(ROOT)/plugin2.so
	$(QUIET)$(CC) $(LOCAL_CFLAGS) -o $@ $< $(LDL) -pthread

$(ROOT)/liblinkdep.so: $(ROOT)/lib.so
$(ROOT)/liblinkdep.so: LOCAL_DFLAGS+=$(ROOT)/lib.so

$(ROOT)/plugin1.so $(ROOT)/plugin2.so: $(SRC)/plugin.d $(DRUNTIMESO)
	$(QUIET)$(DMD) -fPIC -shared $(LOCAL_DFLAGS) -o $@ $<

$(ROOT)/%.so: $(SRC)/%.d $(DRUNTIMESO)
	$(QUIET)$(DMD) -fPIC -shared $(LOCAL_DFLAGS) -o $@ $< $(LINKDL)

clean:
	rm -rf ./*
