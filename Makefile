#
# Top level makefile
#

# modules are all directories that have a Makefile
SUBDIRS = $(shell find . -mindepth 2 -name Makefile | xargs -n 1 dirname)

default: all

all clean test test.run install:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done


