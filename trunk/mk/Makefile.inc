# Generic Gnu Make file for C and C++ subsystems.
# vi:set ft=make:
#
# Almost zero config make system. in your project, 
# create a Makefile containing optionally
#     DEPS+=path/to/dir (relative to top)
# lines and ending with
#     include ../mk/Makefile.inc
# this file, make sure you include enough ../'s.  
#
# main programs should have a file name ending in _main.c or _main.cc
# test programs should have a file name ending in _test.c or _test.cc
# everything else goes into lib.a
#
# if a dependency lib.a, this will recurse into that modules directory and run make,
# but it will not rebuilt if it's out of date.   For that, run 'make all' in the top
# level directory.

# C++ Source files
BIN_CCSRCS = $(wildcard *_main.cc) 
TST_CCSRCS = $(wildcard *_test.cc)
LIB_CCSRCS = $(filter-out $(BIN_CCSRCS) $(TST_CCSRCS), $(wildcard *.cc))  

# C Source files
BIN_CSRCS = $(wildcard *_main.c)
TST_CSRCS = $(wildcard *_test.c)
LIB_CSRCS = $(filter-out $(BIN_CSRCS) $(TST_CSRCS), $(wildcard *.c))  

# TOP is this makefile's directory's parent
TOP = $(realpath $(dir $(lastword $(MAKEFILE_LIST)))..)

# make includes relative to the top work
CFLAGS+=-I$(TOP)
CXXFLAGS+=-I$(TOP)

# default targets
all: test bin

clean:
	rm -f *~ *.o lib.a Makefile.bak $(TST_CSRCS:.c=) $(TST_CCSRCS:.cc=) $(BIN_CSRCS:_main.c=) $(BIN_CCSRCS:_main.cc=)

depend:
	makedepend -Y -I$(TOP) -- $(LIB_CSRCS) $(BIN_CSRCS) $(TST_CSRCS) $(LIB_CCSRCS) $(BIN_CCSRCS) $(TST_CCSRCS)

test: $(TST_CCSRCS:.cc=) $(TST_CSRCS:.c=)

%_test.run: %_test
	-@echo -n "Running $^: "; ./$<

bin: $(BIN_CCSRCS:_main.cc=) $(BIN_CSRCS:_main.c=)

$(TST_CCSRCS:.cc=) $(BIN_CCSRCS:_main.cc=):lib.a
$(TST_CSRCS:.cc=) $(BIN_CSRCS:_main.c=):lib.a

ifneq ($(strip $(LIB_CCSRCS:.cc=.o) $(LIB_CSRCS:.c=.o)),)
LIBA = lib.a
endif

lib.a: $(LIB_CCSRCS:.cc=.o) $(LIB_CSRCS:.c=.o)
	@rm -f $@
	$(AR) -v -q $@ $^

# always use the C++ linker
LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)  

%: %_main.o $(LIBA) $(foreach dir, $(DEPS), $(TOP)/$(dir)/lib.a)
	$(LINK.o) $^ -o $@

$(BIN_CCSRCS:_main.cc=) $(BIN_CSRCS:_main.c=): $(foreach dir, $(DEPS), $(TOP)/$(dir)/lib.a)

$(TOP)/%/lib.a:
	$(MAKE) -C $(TOP)/$* lib.a
