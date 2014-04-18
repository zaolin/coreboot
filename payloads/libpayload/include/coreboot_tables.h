/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _COREBOOT_TABLES_H_
#define _COREBOOT_TABLES_H_

#include <arch/types.h>

struct cbuint64 {
	uint32_t lo;
	uint32_t hi;
};

struct cb_header {
	uint8_t signature[4];
	uint32_t header_bytes;
	uint32_t header_checksum;
	uint32_t table_bytes;
	uint32_t table_checksum;
	uint32_t table_entries;
};

struct cb_record {
	uint32_t tag;
	uint32_t size;
};

#define CB_TAG_UNUSED     0x0000
#define CB_TAG_MEMORY     0x0001

struct cb_memory_range {
	struct cbuint64 start;
	struct cbuint64 size;
	uint32_t type;
};

#define CB_MEM_RAM      1
#define CB_MEM_RESERVED 2
#define CB_MEM_TABLE    16

struct cb_memory {
	uint32_t tag;
	uint32_t size;
	struct cb_memory_range map[0];
};

#define CB_TAG_HWRPB      0x0002

struct cb_hwrpb {
	uint32_t tag;
	uint32_t size;
	uint64_t hwrpb;
};

#define CB_TAG_MAINBOARD  0x0003

struct cb_mainboard {
	uint32_t tag;
	uint32_t size;
	uint8_t vendor_idx;
	uint8_t part_number_idx;
	uint8_t strings[0];
};

#define CB_TAG_VERSION        0x0004
#define CB_TAG_EXTRA_VERSION  0x0005
#define CB_TAG_BUILD          0x0006
#define CB_TAG_COMPILE_TIME   0x0007
#define CB_TAG_COMPILE_BY     0x0008
#define CB_TAG_COMPILE_HOST   0x0009
#define CB_TAG_COMPILE_DOMAIN 0x000a
#define CB_TAG_COMPILER       0x000b
#define CB_TAG_LINKER         0x000c
#define CB_TAG_ASSEMBLER      0x000d

struct cb_string {
	uint32_t tag;
	uint32_t size;
	uint8_t string[0];
};

#define CB_TAG_SERIAL         0x000f

struct cb_serial {
	uint32_t tag;
	uint32_t size;
	uint16_t ioport;
};

#define CB_TAG_CONSOLE       0x00010

struct cb_console {
	uint32_t tag;
	uint32_t size;
	uint16_t type;
};

#define CB_TAG_CONSOLE_SERIAL8250 0
#define CB_TAG_CONSOLE_VGA        1
#define CB_TAG_CONSOLE_BTEXT      2 
#define CB_TAG_CONSOLE_LOGBUF     3
#define CB_TAG_CONSOLE_SROM       4
#define CB_TAG_CONSOLE_EHCI       5

/* Still to come: CMOS information */

/* Helpful macros */

#define MEM_RANGE_COUNT(_rec) \
  (((_rec)->size - sizeof(*(_rec))) / sizeof((_rec)->map[0]))

#define MEM_RANGE_PTR(_rec, _idx) \
  (((uint8_t *) (_rec)) + sizeof(*(_rec)) \
  + (sizeof((_rec)->map[0]) * (_idx)))

#define MB_VENDOR_STRING(_mb) \
	(((unsigned char *) ((_mb)->strings)) + (_mb)->vendor_idx)

#define MB_PART_STRING(_mb) \
	(((unsigned char *) ((_mb)->strings)) + (_mb)->part_number_idx)

#define UNPACK_CB64(_in) \
	( (((uint64_t) _in.hi) << 32) | _in.lo )

#endif
