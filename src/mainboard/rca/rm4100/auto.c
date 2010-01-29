/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Joseph Smith <joe@settoplinux.org>
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
#define __PRE_RAM__

#include <stdint.h>
#include <stdlib.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "pc80/udelay_io.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "superio/smsc/smscsuperio/smscsuperio_early_serial.c"
#include "northbridge/intel/i82830/raminit.h"
#include "northbridge/intel/i82830/memory_initialized.c"
#include "southbridge/intel/i82801xx/i82801xx.h"
#include "southbridge/intel/i82801xx/i82801xx_reset.c"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "cpu/x86/bist.h"
#include "spd_table.h"
#include "gpio.c"

#define SERIAL_DEV PNP_DEV(0x2e, SMSCSUPERIO_SP1)

#include "southbridge/intel/i82801xx/i82801xx_early_smbus.c"

/**
 * The onboard 64MB PC133 memory does not have a SPD EEPROM so the
 * values have to be set manually, the SO-DIMM socket is located in
 * socket0 (0x50), and the onboard memory is located in socket1 (0x51).
 */
static inline int spd_read_byte(unsigned device, unsigned address)
{
	int i;

	if (device == 0x50) {
		return smbus_read_byte(device, address);
	} else if (device == 0x51) {
		for (i = 0; i < ARRAY_SIZE(spd_table); i++) {
			if (spd_table[i].address == address)
				return spd_table[i].data;
		}
		return 0xFF; /* Return 0xFF when address is not found. */
	} else {
		return 0xFF; /* Return 0xFF on any failures. */
	}
}

#include "northbridge/intel/i82830/raminit.c"
#include "lib/generic_sdram.c"

/**
 * Setup mainboard specific registers pre raminit.
 */
static void mb_early_setup(void)
{
	/* - Hub Interface to PCI Bridge Registers - */
	/* 12-Clock Retry Enable */
	pci_write_config16(PCI_DEV(0, 0x1e, 0), 0x50, 0x1402);
	/* Master Latency Timer Count */
	pci_write_config8(PCI_DEV(0, 0x1e, 0), 0x1b, 0x20);
	/* I/O Address Base */
	pci_write_config8(PCI_DEV(0, 0x1e, 0), 0x1c, 0xf0);

	/* - LPC Interface Bridge Registers - */
	/* Delayed Transaction Enable */
	pci_write_config32(PCI_DEV(0, 0x1f, 0), 0xd0, 0x00000002);
	/* Disable the TCO Timer system reboot feature */
	pci_write_config8(PCI_DEV(0, 0x1f, 0), 0xd4, 0x02);
	/* CPU Frequency Strap */
	pci_write_config8(PCI_DEV(0, 0x1f, 0), 0xd5, 0x02);
	/* ACPI base address and enable Resource Indicator */
	pci_write_config32(PCI_DEV(0, 0x1f, 0), PMBASE, (PMBASE_ADDR | 1)); 
	/* Enable the SMBUS */
	enable_smbus();
	/* ACPI base address and disable Resource Indicator */
	pci_write_config32(PCI_DEV(0, 0x1f, 0), PMBASE, (PMBASE_ADDR)); 
	/*  ACPI Enable */
	pci_write_config8(PCI_DEV(0, 0x1f, 0), ACPI_CNTL, 0x10);
}

static void main(unsigned long bist)
{
	static const struct mem_controller memctrl[] = {
		{
			.d0 = PCI_DEV(0, 0, 0),
			.channel0 = {0x50, 0x51},
		}
	};

	if (bist == 0)
		early_mtrr_init();
		if (memory_initialized()) {
			hard_reset();
		}

	/* Set southbridge and superio gpios */
	mb_gpio_init();

	smscsuperio_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure. */
	report_bist_failure(bist);

	/* Setup mainboard specific registers */
	mb_early_setup();

	/* SDRAM init */
	sdram_set_registers(memctrl);
	sdram_set_spd_registers(memctrl);
	sdram_enable(0, memctrl);

	/* Check RAM. */
	/* ram_check(0, 640 * 1024); */
	/* ram_check(64512 * 1024, 65536 * 1024); */
}