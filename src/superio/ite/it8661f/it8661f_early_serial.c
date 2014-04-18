/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2006 Uwe Hermann <uwe@hermann-uwe.de>
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

#include <arch/romcc_io.h>
#include "it8661f.h"

/* The base address is 0x3f0, 0x3bd, or 0x370, depending on config bytes. */
#define SIO_BASE                   0x3f0
#define SIO_INDEX                  SIO_BASE
#define SIO_DATA                   SIO_BASE+1

/* Global configuration registers. */
#define IT8661F_CONFIG_REG_CC      0x02   /* Configure Control (write-only). */
#define IT8661F_CONFIG_REG_LDN     0x07   /* Logical Device Number. */
#define IT8661F_CONFIG_REG_LDE     0x23   /* PnP Logical Device Enable. */
#define IT8661F_CONFIG_REG_SWSUSP  0x24   /* Software Suspend + Clock Select. */

#define IT8661F_CONFIGURATION_PORT 0x0279 /* Write-only. */

/* Special values used for entering MB PnP mode. The first four bytes of
   each line determine the address port, the last four are data. */
static const uint8_t init_values[] = {
	0x6a, 0xb5, 0xda, 0xed, /**/ 0xf6, 0xfb, 0x7d, 0xbe,
	0xdf, 0x6f, 0x37, 0x1b, /**/ 0x0d, 0x86, 0xc3, 0x61,
	0xb0, 0x58, 0x2c, 0x16, /**/ 0x8b, 0x45, 0xa2, 0xd1,
	0xe8, 0x74, 0x3a, 0x9d, /**/ 0xce, 0xe7, 0x73, 0x39,
};

/* The content of IT8661F_CONFIG_REG_LDN (index 0x07) must be set to the
   LDN the register belongs to, before you can access the register. */
static void it8661f_sio_write(uint8_t ldn, uint8_t index, uint8_t value)
{
	outb(IT8661F_CONFIG_REG_LDN, SIO_BASE);
	outb(ldn, SIO_DATA);
	outb(index, SIO_BASE);
	outb(value, SIO_DATA);
}

/* Enable the peripheral devices on the IT8661F Super I/O chip. */
static void it8661f_enable_serial(device_t dev, unsigned iobase)
{
	uint8_t i;

	/* (1) Enter the configuration state (MB PnP mode). */

	/* Perform MB PnP setup to put the SIO chip at 0x3f0. */
	/* Base address 0x3f0: 0x86 0x80 0x55 0x55. */
	/* Base address 0x3bd: 0x86 0x80 0x55 0xaa. */
	/* Base address 0x370: 0x86 0x80 0xaa 0x55. */
	outb(0x86, IT8661F_CONFIGURATION_PORT);
	outb(0x80, IT8661F_CONFIGURATION_PORT);
	outb(0x55, IT8661F_CONFIGURATION_PORT);
	outb(0x55, IT8661F_CONFIGURATION_PORT);

	/* Sequentially write the 32 special values. */
	for (i = 0; i < 32; i++) {
		outb(init_values[i], SIO_BASE);
	}

	/* (2) Modify the data of configuration registers. */

	/* Allow all devices to be enabled. Bits: FDC (0), Com1 (1), Com2 (2),
	   PP (3), IR (4). Bits 5-7 are reserved. */
	it8661f_sio_write(0x00, IT8661F_CONFIG_REG_LDE, 0x1f);

	/* Enable all devices. */
	it8661f_sio_write(IT8661F_FDC,  0x30, 0x1); /* Floppy */
	it8661f_sio_write(IT8661F_SP1,  0x30, 0x1); /* Serial port 1 */
	it8661f_sio_write(IT8661F_SP2,  0x30, 0x1); /* Serial port 2 */
	it8661f_sio_write(IT8661F_PP,   0x30, 0x1); /* Parallel port */
	it8661f_sio_write(IT8661F_IR,   0x30, 0x1); /* IR */

	/* Select 24MHz CLKIN (clear bit 1) and clear software suspend mode
	   (clear bit 0). */
	it8661f_sio_write(0x00, IT8661F_CONFIG_REG_SWSUSP, 0x00);

	/* (3) Exit the configuration state (MB PnP mode). */
	it8661f_sio_write(0x00, IT8661F_CONFIG_REG_CC, 0x02);
}

