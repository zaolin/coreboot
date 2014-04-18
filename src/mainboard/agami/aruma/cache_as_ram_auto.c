#define ASSEMBLY 1
#define __ROMCC__

#define RAMINIT_SYSINFO 0

#if CONFIG_LOGICAL_CPUS==1
        #define SET_NB_CFG_54 1
#endif

//use by raminit
#define QRANK_DIMM_SUPPORT 1

//used by incoherent_ht
//#define K8_SCAN_PCI_BUS 1
//#define K8_ALLOCATE_IO_RANGE 1

#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"


#include <cpu/amd/model_fxx_rev.h>

#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "southbridge/amd/amd8111/amd8111_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#if CONFIG_USE_INIT == 0
#include "lib/memcpy.c"
#endif

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

static void hard_reset(void)
{
        set_bios_reset();
        pci_write_config8(PCI_DEV(0, 0x04, 3), 0x41, 0xf1);
        //outb(0x0e, 0x0cf9);
	outb(0x06, 0x0cf9); /* this value will assert RESET_L and LDTRST_L */
}

static void soft_reset(void)
{
        set_bios_reset();
        pci_write_config8(PCI_DEV(0, 0x04, 0), 0x47, 1);
        //outb(0x0e, 0x0cf9);
	outb(0x06, 0x0cf9); /* this value will assert RESET_L and LDTRST_L */
}

/*
 * GPIO28 of 8111 will control H0_MEMRESET_L
 * GPIO29 of 8111 will control H1_MEMRESET_L
 */
static void memreset_setup(void)
{
	/* Ensure the CPU has controll of the memory lines */
	outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(1<<0),
			SMBUS_IO_BASE + 0xc0 + 29);
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
	if (is_cpu_pre_c0()) {
		udelay(800);
		/* Set memreset_high */
		outb((0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0), 
				SMBUS_IO_BASE + 0xc0 + 28);
		udelay(90);
	}
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
#define SMBUS_SWITCH1 0x71
#define SMBUS_SWITCH2 0x73
        /* Switch 1: pca 9545, Switch 2: pca 9543 */
        unsigned device = (ctrl->channel0[0]) >> 8;
        /* Disable all outputs on SMBus switch 1 */
        smbus_send_byte(SMBUS_SWITCH1, 0x0);
        /* Select SMBus switch 2 Channel 0/1 */
        smbus_send_byte(SMBUS_SWITCH2, device);

}

static inline int spd_read_byte(unsigned device, unsigned address)
{
        return smbus_read_byte(device, address);
}


#include "northbridge/amd/amdk8/raminit.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "sdram/generic_sdram.c"

 /* tyan does not want the default */
#include "resourcemap.c" 

#include "cpu/amd/dualcore/dualcore.c"

#define CHAN0 0x100
#define CHAN1 0x200

#include "cpu/amd/car/copy_and_run.c"
#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#if USE_FALLBACK_IMAGE == 1

#include "southbridge/amd/amd8111/amd8111_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"

void failover_process(unsigned long bist, unsigned long cpu_init_detectedx)
{

        unsigned last_boot_normal_x = last_boot_normal();

        /* Is this a cpu only reset? or Is this a secondary cpu? */
        if ((cpu_init_detectedx) || (!boot_cpu())) {
                if (last_boot_normal_x) {
                        goto normal_image;
                } else {
                        goto fallback_image;
                }
        }

        /* Nothing special needs to be done to find bus 0 */
        /* Allow the HT devices to be found */

        enumerate_ht_chain();

        /* Setup the flash access */
        amd8111_enable_rom();

        /* Is this a deliberate reset by the bios */
        if (bios_reset_detected() && last_boot_normal_x) {
                goto normal_image;
        }
        /* This is the primary cpu how should I boot? */
        else if (do_normal_boot()) {
                goto normal_image;
        }
        else {
                goto fallback_image;
        }
 normal_image:
        __asm__ volatile ("jmp __normal_image"
                : /* outputs */
                : "a" (bist), "b" (cpu_init_detectedx) /* inputs */
                );

 fallback_image:
	;
}
#endif

void real_main(unsigned long bist, unsigned long cpu_init_detectedx);

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{

#if USE_FALLBACK_IMAGE == 1
        failover_process(bist, cpu_init_detectedx);
#endif
        real_main(bist, cpu_init_detectedx);

}


void real_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const uint16_t spd_addr [] = {
		// node 0
                 (0xa0>>1)|CHAN0, (0xa4>>1)|CHAN0, 0, 0,
                 (0xa2>>1)|CHAN0, (0xa6>>1)|CHAN0, 0, 0,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
		// node 1
                 (0xa8>>1)|CHAN0, (0xac>>1)|CHAN0, 0, 0,
                 (0xaa>>1)|CHAN0, (0xae>>1)|CHAN0, 0, 0,
#endif
#if CONFIG_MAX_PHYSICAL_CPUS > 2
		// node 2
                 (0xa8>>1)|CHAN1, (0xac>>1)|CHAN1, 0, 0,
                 (0xaa>>1)|CHAN1, (0xae>>1)|CHAN1, 0, 0,
		// node 3
                 (0xa0>>1)|CHAN1, (0xa4>>1)|CHAN1, 0, 0,
                 (0xa2>>1)|CHAN1, (0xa6>>1)|CHAN1, 0, 0,
#endif
	};

        int needs_reset;
	unsigned bsp_apicid = 0;
	struct mem_controller ctrl[8];
	unsigned nodes;

        if (bist == 0) {
		bsp_apicid = init_cpus(cpu_init_detectedx);
        }

 	w83627hf_enable_serial(SERIAL_DEV, TTYS0_BASE);
        uart_init();
        console_init();

	
	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

	setup_aruma_resource_map();	

	needs_reset = setup_coherent_ht_domain();

#if CONFIG_LOGICAL_CPUS==1
         /* here need to make sure last core0 is started, esp for two way system,
         * (there may be apic id conflicts in that case)
         */
        wait_all_core0_started();
        start_other_cores();
#endif
	
	wait_all_aps_started(bsp_apicid);
		
        needs_reset |= ht_setup_chains_x();

       	if (needs_reset) {
               	print_info("ht reset -\r\n");
               	soft_reset();
       	}

	allow_all_aps_stop(bsp_apicid);

	nodes = get_nodes();
        //It's the time to set ctrl now;
        fill_mem_ctrl(nodes, ctrl, spd_addr);

	enable_smbus();

	memreset_setup();
	sdram_initialize(nodes, ctrl);

	post_cache_as_ram();

}
