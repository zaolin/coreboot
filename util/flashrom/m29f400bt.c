/*
 * m29f400bt.c: driver for programming JEDEC standard flash parts
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
 *
 * $Id$
 */

#include "flash.h"
#include "m29f400bt.h"

int probe_m29f400bt(struct flashchip *flash)
{
	volatile char *bios = flash->virt_addr;
	unsigned char id1, id2;

	*(volatile char *) (bios + 0xAAA) = 0xAA;
	*(volatile char *) (bios + 0x555) = 0x55;
	*(volatile char *) (bios + 0xAAA) = 0x90;

	myusec_delay(10);

	id1 = *(volatile unsigned char *) bios;
	id2 = *(volatile unsigned char *) (bios + 0x02);

	*(volatile char *) (bios + 0xAAA) = 0xAA;
	*(volatile char *) (bios + 0x555) = 0x55;
	*(volatile char *) (bios + 0xAAA) = 0xF0;

	myusec_delay(10);

	printf("%s: id1 0x%x, id2 0x%x\n", __FUNCTION__, id1, id2);


	if (id1 == flash->manufacture_id && id2 == flash->model_id)
		return 1;

	return 0;
}

int erase_m29f400bt(struct flashchip *flash)
{
	volatile char *bios = flash->virt_addr;

	*(volatile char *) (bios + 0xAAA) = 0xAA;
	*(volatile char *) (bios + 0x555) = 0x55;
	*(volatile char *) (bios + 0xAAA) = 0x80;

	*(volatile char *) (bios + 0xAAA) = 0xAA;
	*(volatile char *) (bios + 0x555) = 0x55;
	*(volatile char *) (bios + 0xAAA) = 0x10;

	myusec_delay(10);
	toggle_ready_m29f400bt(bios);

	return (0);
}

int block_erase_m29f400bt(volatile char *bios, volatile char *dst)
{

	*(volatile char *) (bios + 0xAAA) = 0xAA;
	*(volatile char *) (bios + 0x555) = 0x55;
	*(volatile char *) (bios + 0xAAA) = 0x80;

	*(volatile char *) (bios + 0xAAA) = 0xAA;
	*(volatile char *) (bios + 0x555) = 0x55;
	//*(volatile char *) (bios + 0xAAA) = 0x10;
	*dst = 0x30;

	myusec_delay(10);
	toggle_ready_m29f400bt(bios);

	return (0);
}

int write_m29f400bt(struct flashchip *flash, unsigned char *buf)
{
	int i;
	int total_size = flash->total_size * 1024, page_size =
	    flash->page_size;
	volatile char *bios = flash->virt_addr;

	//erase_m29f400bt (flash);
	printf("Programming Page:\n ");
	/*********************************
	*Pages for M29F400BT:
	* 16	0x7c000		0x7ffff		TOP
	*  8 	0x7a000		0x7bfff
	*  8 	0x78000		0x79fff
	* 32	0x70000		0x77fff
	* 64	0x60000		0x6ffff
	* 64	0x50000		0x5ffff
	* 64	0x40000		0x4ffff
	*---------------------------------
	* 64	0x30000		0x3ffff
	* 64	0x20000		0x2ffff
	* 64	0x10000		0x1ffff
	* 64	0x00000		0x0ffff		BOTTOM
	*********************************/
	printf("total_size/page_size = %d\n", total_size / page_size);
	for (i = 0; i < (total_size / page_size) - 1; i++) {
		printf("%04d at address: 0x%08x\n", i, i * page_size);
		block_erase_m29f400bt(bios, bios + i * page_size);
		write_page_m29f400bt(bios, buf + i * page_size,
				     bios + i * page_size, page_size);
		printf
		    ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}

	printf("%04d at address: 0x%08x\n", 7, 0x70000);
	block_erase_m29f400bt(bios, bios + 0x70000);
	write_page_m29f400bt(bios, buf + 0x70000, bios + 0x70000,
			     32 * 1024);

	printf("%04d at address: 0x%08x\n", 8, 0x78000);
	block_erase_m29f400bt(bios, bios + 0x78000);
	write_page_m29f400bt(bios, buf + 0x78000, bios + 0x78000,
			     8 * 1024);

	printf("%04d at address: 0x%08x\n", 9, 0x7a000);
	block_erase_m29f400bt(bios, bios + 0x7a000);
	write_page_m29f400bt(bios, buf + 0x7a000, bios + 0x7a000,
			     8 * 1024);

	printf("%04d at address: 0x%08x\n", 10, 0x7c000);
	block_erase_m29f400bt(bios, bios + 0x7c000);
	write_page_m29f400bt(bios, buf + 0x7c000, bios + 0x7c000,
			     16 * 1024);

	printf("\n");
	//protect_m29f400bt (bios);

	return (0);
}

int write_linuxbios_m29f400bt(struct flashchip *flash, unsigned char *buf)
{
	volatile char *bios = flash->virt_addr;

	printf("Programming Page:\n ");
	/*********************************
	*Pages for M29F400BT:
	* 16	0x7c000		0x7ffff		TOP
	*  8 	0x7a000		0x7bfff
	*  8 	0x78000		0x79fff
	* 32	0x70000		0x77fff
	* 64	0x60000		0x6ffff
	* 64	0x50000		0x5ffff
	* 64	0x40000		0x4ffff
	*---------------------------------
	* 64	0x30000		0x3ffff
	* 64	0x20000		0x2ffff
	* 64	0x10000		0x1ffff
	* 64	0x00000		0x0ffff		BOTTOM
	*********************************/
	printf("%04d at address: 0x%08x\n", 7, 0x00000);
	block_erase_m29f400bt(bios, bios + 0x00000);
	write_page_m29f400bt(bios, buf + 0x00000, bios + 0x00000,
			     64 * 1024);

	printf("%04d at address: 0x%08x\n", 7, 0x10000);
	block_erase_m29f400bt(bios, bios + 0x10000);
	write_page_m29f400bt(bios, buf + 0x10000, bios + 0x10000,
			     64 * 1024);

	printf("%04d at address: 0x%08x\n", 7, 0x20000);
	block_erase_m29f400bt(bios, bios + 0x20000);
	write_page_m29f400bt(bios, buf + 0x20000, bios + 0x20000,
			     64 * 1024);

	printf("%04d at address: 0x%08x\n", 7, 0x30000);
	block_erase_m29f400bt(bios, bios + 0x30000);
	write_page_m29f400bt(bios, buf + 0x30000, bios + 0x30000,
			     64 * 1024);

	printf("\n");
	//protect_m29f400bt (bios);

	return (0);
}
