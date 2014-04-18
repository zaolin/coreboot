/*
 * This file is part of the coreboot project.
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
#include <stdlib.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"
#include "northbridge/intel/i82810/raminit.h"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "cpu/x86/bist.h"
#include "southbridge/intel/i82801ax/i82801ax_early_smbus.c"
#include "pc80/udelay_io.c"
#include "lib/debug.c"
#include "northbridge/intel/i82810/raminit.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

static void main(unsigned long bist)
{
	if (bist == 0)
		early_mtrr_init();

	/* FIXME */
	outb(0x87, 0x2e);
	outb(0x87, 0x2e);
	pnp_write_config(SERIAL_DEV, 0x24, 0x84 | (1 << 6));
	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	outb(0x87, 0xaa);

	uart_init();
	console_init();

	enable_smbus();

	report_bist_failure(bist);

	/* dump_spd_registers(); */
	sdram_set_registers();
	sdram_set_spd_registers();
	sdram_enable();
	/* ram_check(0, 640 * 1024); */
}
