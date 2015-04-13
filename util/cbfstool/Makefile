obj ?= $(shell pwd)

HOSTCC ?= $(CC)
CFLAGS ?= -g
CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wwrite-strings -Wredundant-decls -Wno-trigraphs
CFLAGS += -Wstrict-aliasing -Wshadow -Werror

CBFSTOOL_BINARY:=$(obj)/cbfstool

CBFSTOOL_COMMON:=cbfstool.o common.o cbfs_image.o compress.o fit.o
CBFSTOOL_COMMON+=elfheaders.o cbfs-mkstage.o cbfs-mkpayload.o xdr.o
# LZMA
CBFSTOOL_COMMON+=lzma/lzma.o
CBFSTOOL_COMMON+=lzma/C/LzFind.o  lzma/C/LzmaDec.o  lzma/C/LzmaEnc.o

CBFSTOOL_COMMON+=linux_trampoline.o cbfs-payload-linux.o
# Crypto
CBFSTOOL_COMMON+=crypto.o crypto-utils.o

CBFSTOOL_COMMON:=$(addprefix $(obj)/,$(CBFSTOOL_COMMON))

CBKEYTOOL_BINARY:=$(obj)/cbkeytool
CBKEYTOOL_COMMON:=crypto.o common.o cbfs_image.o compress.o xdr.o
CBKEYTOOL_COMMON+=lzma/C/LzFind.o  lzma/C/LzmaDec.o  lzma/C/LzmaEnc.o
CBKEYTOOL_COMMON+=lzma/lzma.o crypto-utils.o cbkeytool.o

CBKEYTOOL_COMMON:=$(addprefix $(obj)/,$(CBKEYTOOL_COMMON))

RMODTOOL_BINARY:=$(obj)/rmodtool
RMODTOOL_COMMON:=rmodtool.o rmodule.o common.o elfheaders.o xdr.o

RMODTOOL_COMMON:=$(addprefix $(obj)/,$(RMODTOOL_COMMON))

all: dep $(CBFSTOOL_BINARY) $(RMODTOOL_BINARY) $(CBKEYTOOL_BINARY)

$(obj)/%.o: %.c
	$(HOSTCC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CBFSTOOL_COMMON) $(CBFSTOOL_BINARY)
	rm -f $(RMODTOOL_COMMON) $(RMODTOOL_BINARY)
	rm -f $(CBKEYTOOL_COMMON) $(CBKEYTOOL_BINARY)

tags:
	ctags *.[ch]

$(obj)/cbfstool:$(CBFSTOOL_COMMON)
	$(HOSTCC) $(CFLAGS) -o $@ $^

$(obj)/rmodtool:$(RMODTOOL_COMMON)
	$(HOSTCC) $(CFLAGS) -o $@ $^

$(obj)/cbkeytool:$(CBKEYTOOL_COMMON)
	$(HOSTCC) $(CFLAGS) -o $@ $^

dep:
	@$(HOSTCC) $(CFLAGS) -MM *.c > .dependencies
	@$(HOSTCC) $(CFLAGS) -MM lzma/*.c >> .dependencies
	@$(HOSTCC) $(CFLAGS) -MM lzma/C/*.c >> .dependencies

-include .dependencies
