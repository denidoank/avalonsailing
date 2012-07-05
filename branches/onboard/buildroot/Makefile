#
# buildroot makefile
#

ifneq ($(strip $(IMAGE_DIR)),)
FLAGS = -d $(IMAGE_DIR)
endif

image:
	./build.sh $(FLAGS)

include ../mk/Makefile.inc
