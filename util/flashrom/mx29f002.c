/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdint.h>
#include "flash.h"

int probe_29f002(struct flashchip *flash)
{
	volatile uint8_t *bios = flash->virtual_memory;
	uint8_t id1, id2;

	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0x90, bios + 0x5555);

	id1 = chip_readb(bios);
	id2 = chip_readb(bios + 0x01);

	chip_writeb(0xF0, bios);

	myusec_delay(10);

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __FUNCTION__, id1, id2);
	if (id1 == flash->manufacture_id && id2 == flash->model_id)
		return 1;

	return 0;
}

int erase_29f002(struct flashchip *flash)
{
	volatile uint8_t *bios = flash->virtual_memory;

	chip_writeb(0xF0, bios + 0x555);
	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x80, bios + 0x555);
	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x10, bios + 0x555);

	myusec_delay(100);
	toggle_ready_jedec(bios);

	//   while ((*bios & 0x40) != 0x40)
	//;

#if 0
	toggle_ready_jedec(bios);
	chip_writeb(0x30, bios + 0x0ffff);
	chip_writeb(0x30, bios + 0x1ffff);
	chip_writeb(0x30, bios + 0x2ffff);
	chip_writeb(0x30, bios + 0x37fff);
	chip_writeb(0x30, bios + 0x39fff);
	chip_writeb(0x30, bios + 0x3bfff);
#endif

	return 0;
}

int write_29f002(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	volatile uint8_t *bios = flash->virtual_memory;
	volatile uint8_t *dst = bios;

	chip_writeb(0xF0, bios);
	myusec_delay(10);
	erase_29f002(flash);
	//*bios = 0xF0;
#if 1
	printf("Programming page: ");
	for (i = 0; i < total_size; i++) {
		/* write to the sector */
		if ((i & 0xfff) == 0)
			printf("address: 0x%08lx", (unsigned long)i);
		chip_writeb(0xAA, bios + 0x5555);
		chip_writeb(0x55, bios + 0x2AAA);
		chip_writeb(0xA0, bios + 0x5555);
		chip_writeb(*buf++, dst++);

		/* wait for Toggle bit ready */
		toggle_ready_jedec(dst);

		if ((i & 0xfff) == 0)
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
#endif
	printf("\n");

	return 0;
}
