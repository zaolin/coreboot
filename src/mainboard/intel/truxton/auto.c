/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Arastra, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#define ASSEMBLY 1
#define __PRE_RAM__
#include <stdint.h>
#include <stdlib.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "pc80/udelay_io.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "southbridge/intel/i3100/i3100_early_smbus.c"
#include "southbridge/intel/i3100/i3100_early_lpc.c"
#include "northbridge/intel/i3100/raminit_ep80579.h"
#include "superio/intel/i3100/i3100.h"
#include "cpu/x86/lapic/boot_cpu.c"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "superio/intel/i3100/i3100_early_serial.c"
#include "cpu/x86/bist.h"
#include "spd.h"

#define SIO_GPIO_BASE 0x680
#define SIO_XBUS_BASE 0x4880

#define DEVPRES_CONFIG  (DEVPRES_D1F0 | DEVPRES_D2F0 | DEVPRES_D3F0 | DEVPRES_D4F0)

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}
static inline int spd_read_byte(u16 device, u8 address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/intel/i3100/raminit_ep80579.c"
#include "lib/generic_sdram.c"
#include "../../intel/jarrell/debug.c"

/* #define TRUXTON_DEBUG */

static void main(unsigned long bist)
{
	msr_t msr;
	u16 perf;
	static const struct mem_controller mch[] = {
		{
			.node_id = 0,
			.f0 = PCI_DEV(0, 0x00, 0),
			.channel0 = { (0xa<<3)|2, (0xa<<3)|3 },
		}
	};

	if (bist == 0) {
		/* Skip this if there was a built in self test failure */
		early_mtrr_init();
		if (memory_initialized()) {
			asm volatile ("jmp __cpu_reset");
		}
	}

	/* Set up the console */
	i3100_enable_superio();
	i3100_enable_serial(I3100_SUPERIO_CONFIG_PORT, I3100_SP1, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Prevent the TCO timer from rebooting us */
	i3100_halt_tco_timer();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

#ifdef TRUXTON_DEBUG
	print_pci_devices();
#endif
	enable_smbus();
	dump_spd_registers();

	sdram_initialize(ARRAY_SIZE(mch), mch);
	dump_pci_devices();
	dump_pci_device(PCI_DEV(0, 0x00, 0));
#ifdef TRUXTON_DEBUG
	dump_bar14(PCI_DEV(0, 0x00, 0));
#endif

#ifdef TRUXTON_DEBUG
	ram_fill(0x00000000, 0x02000000);
	ram_verify(0x00000000, 0x02000000);
#endif
}
