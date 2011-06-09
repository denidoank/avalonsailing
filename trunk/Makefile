#
# Top level makefile
#

# modules are all directories that have a Makefile
SUBDIRS = $(shell find . -mindepth 2 -name Makefile | xargs -n 1 dirname)
DATE = $(shell date --iso)
PROJECT = avalonsailing
OUTDIR = ..

default: all

all clean test test.run install installconf:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done

tarball: clean
	tar zcpf $(OUTDIR)/$(PROJECT)-$(DATE).tar.gz .

image: clean
	$(MAKE) -C buildroot $@
