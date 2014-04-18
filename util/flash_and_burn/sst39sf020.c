/*
 * sst39sf020.c: driver for SST28SF040C flash models.
 *
 *
 * Copyright 2000 Silicon Integrated System Corporation
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Reference:
 *	4 MEgabit (512K x 8) SuperFlash EEPROM, SST28SF040 data sheet
 *
 * ToDo: Consilidated to standard JEDEC code.
 *
 * $Id$
 */

#include <stdio.h>
#include "flash.h"
#include "jedec.h"
#include "sst39sf020.h"

#define AUTO_PG_ERASE1		0x20
#define AUTO_PG_ERASE2		0xD0
#define AUTO_PGRM		0x10
#define CHIP_ERASE		0x30
#define RESET			0xFF
#define READ_ID			0x90

static __inline__ int erase_sector_39sf020(volatile char *bios,
					   unsigned long address)
{
	*bios = AUTO_PG_ERASE1;
	*(bios + address) = AUTO_PG_ERASE2;

	/* wait for Toggle bit ready         */
	toggle_ready_jedec(bios);

	return (0);
}

int write_39sf020(struct flashchip *flash, unsigned char *buf)
{
	int i;
	int total_size = flash->total_size * 1024, page_size =
	    flash->page_size;
	volatile char *bios = flash->virt_addr;

	erase_chip_jedec(flash);

	printf("Programming Page: ");
	for (i = 0; i < total_size / page_size; i++) {
		/* erase the page before programming */
		//erase_sector_39sf020(bios, i * page_size);

		/* write to the sector */
		printf("%04d at address: 0x%08x", i, i * page_size);
		write_sector_jedec(bios, buf + i * page_size,
				   bios + i * page_size, page_size);
		printf
		    ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}
	printf("\n");

	return (0);
}
