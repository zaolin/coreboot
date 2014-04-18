/*
 * This code is derived from the Tyan s2882 romstage.c
 * Adapted by Stefan Reinauer <stepan@coresystems.de>
 * Additional (C) 2007 coresystems GmbH 
 */

 
#include <stdint.h>
#include <string.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "console/console.c"
#include "lib/ramtest.c"

#include <cpu/amd/model_fxx_rev.h>

#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "southbridge/amd/amd8111/amd8111_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

#include "southbridge/amd/amd8111/amd8111_early_ctrl.c"

static void memreset_setup(void)
{
	if (is_cpu_pre_c0()) {
		/* Set the memreset low */
		outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(0<<0), SMBUS_IO_BASE + 0xc0 + 28);
		/* Ensure the BIOS has control of the memory lines */
		outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(0<<0), SMBUS_IO_BASE + 0xc0 + 29);
	}
	else {
		/* Ensure the CPU has controll of the memory lines */
		outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(1<<0), SMBUS_IO_BASE + 0xc0 + 29);
	}
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
	if (is_cpu_pre_c0()) {
		udelay(800);
		/* Set memreset_high */
		outb((0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0), SMBUS_IO_BASE + 0xc0 + 28);
		udelay(90);
	}
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#define QRANK_DIMM_SUPPORT 1

#include "northbridge/amd/amdk8/raminit.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "lib/generic_sdram.c"

 /* newisys khepri does not want the default */
#include "resourcemap.c" 

#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#endif
#include "cpu/amd/dualcore/dualcore.c"



#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "southbridge/amd/amd8111/amd8111_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const uint16_t spd_addr [] = {
			(0xa<<3)|0, (0xa<<3)|2, 0, 0,
			(0xa<<3)|1, (0xa<<3)|3, 0, 0,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
			(0xa<<3)|4, (0xa<<3)|6, 0, 0,
			(0xa<<3)|5, (0xa<<3)|7, 0, 0,
#endif
	};

        int needs_reset;
        unsigned bsp_apicid = 0;

        struct mem_controller ctrl[8];
        unsigned nodes;

        if (!cpu_init_detectedx && boot_cpu()) {
		/* Nothing special needs to be done to find bus 0 */
		/* Allow the HT devices to be found */

		enumerate_ht_chain();

		/* Setup the amd8111 */
		amd8111_enable_rom();
        }

        if (bist == 0) {
                bsp_apicid = init_cpus(cpu_init_detectedx);
        }

//	post_code(0x32);
	
 	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
        uart_init();
        console_init();

//	dump_mem(CONFIG_DCACHE_RAM_BASE+CONFIG_DCACHE_RAM_SIZE-0x200, CONFIG_DCACHE_RAM_BASE+CONFIG_DCACHE_RAM_SIZE);
	
	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

        setup_khepri_resource_map();
#if 0
        dump_pci_device(PCI_DEV(0, 0x18, 0));
	dump_pci_device(PCI_DEV(0, 0x19, 0));
#endif

	needs_reset = setup_coherent_ht_domain();

        wait_all_core0_started();
#if CONFIG_LOGICAL_CPUS==1
        // It is said that we should start core1 after all core0 launched
        start_other_cores();
        wait_all_other_cores_started(bsp_apicid);
#endif

        needs_reset |= ht_setup_chains_x();

       	if (needs_reset) {
               	print_info("ht reset -\n");
               	soft_reset();
       	}

        allow_all_aps_stop(bsp_apicid);

        nodes = get_nodes();
        //It's the time to set ctrl now;
        fill_mem_ctrl(nodes, ctrl, spd_addr);

        enable_smbus();

        memreset_setup();
        sdram_initialize(nodes, ctrl);

#if 0
	dump_pci_devices();
#endif

	post_cache_as_ram();

}

