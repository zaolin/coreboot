/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Uwe Hermann <uwe@hermann-uwe.de>
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

#define ASSEMBLY 1

#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"
#include "southbridge/intel/i82371eb/i82371eb_early_smbus.c"
#include "northbridge/intel/i440bx/raminit.h"
#include "mainboard/bitworks/ims/debug.c"  // FIXME
#include "pc80/udelay_io.c"
#include "lib/delay.c"
#include "superio/nsc/pc87309/pc87309_early_serial.c"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "cpu/x86/bist.h"

#define SERIAL_DEV PNP_DEV(0x2e, PC87309_SP1)

static inline int spd_read_byte(unsigned int device, unsigned int address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/intel/i440bx/raminit.c"
#include "northbridge/intel/i440bx/debug.c"
#include "sdram/generic_sdram.c"

static void main(unsigned long bist)
{
	static const struct mem_controller memctrl[] = {
		{
		 .d0 = PCI_DEV(0, 0, 0),
		 .channel0 = {
			      (0xa << 3) | 0,
			      (0xa << 3) | 1,
			      (0xa << 3) | 2,
			      (0xa << 3) | 3,
			      },
		 },
	};

	/* Skip this if there was a built in self test failure. */
	if (bist == 0) {
		early_mtrr_init();
	}

	pc87309_enable_serial(SERIAL_DEV, TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure. */
	report_bist_failure(bist);

	enable_smbus();

	dump_spd_registers(&memctrl[0]);

	sdram_initialize(sizeof(memctrl) / sizeof(memctrl[0]), memctrl);

	/* Check 64 MB of RAM. */
	ram_check(0x00000000, 0x04000000);
}
