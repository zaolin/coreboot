/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 AMD
 * (Written by Yinghai Lu <yinghailu@amd.com> for AMD)
 * Copyright (C) 2007 Philipp Degler <pdegler@rumms.uni-mannheim.de>
 * (Thanks to LSRA University of Mannheim for their support)
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

/* Used by it8712f_enable_serial(). */
#define SERIAL_DEV PNP_DEV(0x2e, IT8712F_SP1)

/* Used by raminit. */
#define QRANK_DIMM_SUPPORT 1

#if CONFIG_LOGICAL_CPUS == 1
#define SET_NB_CFG_54 1
#endif

#include <stdint.h>
#include <string.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "superio/ite/it8712f/it8712f_early_serial.c"

/* Used by ck894_early_setup(). */
#define CK804_NUM 1

#include <cpu/amd/model_fxx_rev.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "southbridge/nvidia/ck804/ck804_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"
#include "northbridge/amd/amdk8/debug.c"
#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"
#include "northbridge/amd/amdk8/setup_resource_map.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "cpu/amd/dualcore/dualcore.c"

static void memreset_setup(void)
{
	/* Nothing to do. */
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
	/* Nothing to do. */
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* Nothing to do. */
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/raminit.c"
#include "lib/generic_sdram.c"
#include "southbridge/nvidia/ck804/ck804_early_setup_ss.h"
#include "southbridge/nvidia/ck804/ck804_early_setup.c"
#include "cpu/amd/car/copy_and_run.c"
#include "cpu/amd/car/post_cache_as_ram.c"
#include "cpu/amd/model_fxx/init_cpus.c"

#include "southbridge/nvidia/ck804/ck804_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"

static void sio_setup(void)
{
	unsigned value;
	uint32_t dword;
	uint8_t byte;

	/* Subject decoding */
	byte = pci_read_config8(PCI_DEV(0, CK804_DEVN_BASE + 1, 0), 0x7b);
	byte |= 0x20;
	pci_write_config8(PCI_DEV(0, CK804_DEVN_BASE + 1, 0), 0x7b, byte);

	/* LPC Positive Decode 0 */
	dword = pci_read_config32(PCI_DEV(0, CK804_DEVN_BASE + 1, 0), 0xa0);
	dword |= (1 << 0) | (1 << 1);	/* Serial 0, Serial 1 */
	pci_write_config32(PCI_DEV(0, CK804_DEVN_BASE + 1, 0), 0xa0, dword);
}

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const uint16_t spd_addr[] = {
		(0xa << 3) | 0, (0xa << 3) | 2, 0, 0,
		(0xa << 3) | 1, (0xa << 3) | 3, 0, 0,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
		(0xa << 3) | 4, (0xa << 3) | 6, 0, 0,
		(0xa << 3) | 5, (0xa << 3) | 7, 0, 0,
#endif
	};

	int needs_reset;
	unsigned nodes, bsp_apicid = 0;
	struct mem_controller ctrl[8];

	if (!cpu_init_detectedx && boot_cpu()) {
		/* Nothing special needs to be done to find bus 0 */
		/* Allow the HT devices to be found */
		enumerate_ht_chain();

		sio_setup();

		/* Setup the ck804 */
		ck804_enable_rom();
	}

	if (bist == 0)
		bsp_apicid = init_cpus(cpu_init_detectedx);

	it8712f_24mhz_clkin();
	it8712f_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

#if 0
	dump_pci_device(PCI_DEV(0, 0x18, 0));
#endif

	needs_reset = setup_coherent_ht_domain();

	wait_all_core0_started();
#if CONFIG_LOGICAL_CPUS==1
	/* It is said that we should start core1 after all core0 launched. */
	start_other_cores();
	wait_all_other_cores_started(bsp_apicid);
#endif

	needs_reset |= ht_setup_chains_x();
	needs_reset |= ck804_early_setup_x();

	if (needs_reset) {
		print_info("ht reset -\r\n");
		soft_reset();
	}

	allow_all_aps_stop(bsp_apicid);

	nodes = get_nodes();
	/* It's the time to set ctrl now. */
	fill_mem_ctrl(nodes, ctrl, spd_addr);

	enable_smbus();

#if 0
	dump_spd_registers(&ctrl[0]);
	dump_smbus_registers();
#endif

	memreset_setup();
	sdram_initialize(nodes, ctrl);

#if 0
	print_pci_devices();
	dump_pci_devices();
#endif

	post_cache_as_ram();
}
