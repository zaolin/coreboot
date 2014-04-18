/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Luis Correia <luis.f.correia@gmail.com>
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

#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "console/console.c"
#include "lib/ramtest.c"
#include "superio/winbond/w83977tf/w83977tf_early_serial.c"
#include "southbridge/amd/cs5530/cs5530_enable_rom.c"
#include "cpu/x86/bist.h"

#define SERIAL_DEV PNP_DEV(0x3f0, W83977TF_SP1)

#include "northbridge/amd/gx1/raminit.c"

static void main(unsigned long bist)
{
	/* Initialize the serial console. */
	w83977tf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure. */
	report_bist_failure(bist);

	cs5530_enable_rom();

	/* Initialize RAM. */
	sdram_init();

	/* Check RAM. */
	/* ram_check(0x00000000, 640 * 1024); */
}

