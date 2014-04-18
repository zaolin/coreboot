#
# Makefile for flashrom utility
# 
# redone by Stefan Reinauer <stepan@openbios.org>
#

PROGRAM = flashrom

CC      = gcc
STRIP	= strip
INSTALL = /usr/bin/install
PREFIX  = /usr/local
#CFLAGS  = -O2 -g -Wall -Werror
CFLAGS  = -Os -Wall -Werror -DDISABLE_DOC # -DTS5300
OS_ARCH	= $(shell uname)
ifeq ($(OS_ARCH), SunOS)
LDFLAGS = -lpci -lz
else
LDFLAGS = -lpci -lz -static 
STRIP_ARGS = -s
endif

OBJS = chipset_enable.o board_enable.o udelay.o jedec.o sst28sf040.o \
	am29f040b.o mx29f002.o sst39sf020.o m29f400bt.o w49f002u.o \
	82802ab.o msys_doc.o pm49fl004.o sst49lf040.o sst49lfxxxc.o \
	sst_fwhub.o layout.o lbtable.o flashchips.o flashrom.o \
	sharplhf00l04.o w29ee011.o spi.o

all: pciutils dep $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)

clean:
	rm -f *.o *~

distclean: clean
	rm -f $(PROGRAM) .dependencies
	
dep:
	@$(CC) -MM *.c > .dependencies

pciutils:
	@echo; echo -n "Checking for pciutils and zlib... "
	@$(shell ( echo "#include <pci/pci.h>";		   \
		   echo "struct pci_access *pacc;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ pacc = pci_alloc(); return 0; }"; ) > .test.c )
	@$(CC) $(CFLAGS) .test.c -o .test $(LDFLAGS) &>/dev/null &&	\
		echo "found." || ( echo "not found."; echo;		\
		echo "Please install pciutils-devel and zlib-devel.";	\
		echo "See README for more information."; echo;		\
		rm -f .test.c .test; exit 1)
	@rm -f .test.c .test

install: $(PROGRAM)
	$(INSTALL) flashrom $(PREFIX)/bin

.PHONY: all clean distclean dep pciutils

-include .dependencies

