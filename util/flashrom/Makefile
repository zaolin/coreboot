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
CFLAGS  = -Os -Wall -Werror
OS_ARCH	= $(shell uname)
ifeq ($(OS_ARCH), SunOS)
LDFLAGS = -lpci -lz
else
LDFLAGS = -lpci -lz
STRIP_ARGS = -s
endif
ifeq ($(OS_ARCH), FreeBSD)
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif

OBJS = chipset_enable.o board_enable.o udelay.o jedec.o stm50flw0x0x.o \
	sst28sf040.o am29f040b.o mx29f002.o sst39sf020.o m29f400bt.o \
	w49f002u.o 82802ab.o pm49fl00x.o sst49lf040.o en29f002a.o \
	sst49lfxxxc.o sst_fwhub.o layout.o cbtable.o flashchips.o \
	flashrom.o w39v080fa.o sharplhf00l04.o w29ee011.o spi.o it87spi.o \
	ichspi.o w39v040c.o sb600spi.o

all: pciutils dep $(PROGRAM)

# Set the flashrom version string from the highest revision number
# of the checked out flashrom files.
SVNDEF := -D'FLASHROM_VERSION="$(shell svnversion -cn . \
          | sed -e "s/.*://" -e "s/\([0-9]*\).*/\1/")"'

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)

flashrom.o: flashrom.c
	$(CC) -c $(CFLAGS) $(SVNDEF) $(CPPFLAGS) $< -o $@

clean:
	rm -f $(PROGRAM) *.o

distclean: clean
	rm -f .dependencies

dep:
	@$(CC) -MM *.c > .dependencies

pciutils:
	@echo; printf "Checking for pciutils and zlib... "
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
	$(INSTALL) $(PROGRAM) $(PREFIX)/sbin
	mkdir -p $(PREFIX)/share/man/man8
	$(INSTALL) $(PROGRAM).8 $(PREFIX)/share/man/man8

.PHONY: all clean distclean dep pciutils

-include .dependencies
