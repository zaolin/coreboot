#define ASSEMBLY 1
#define __ROMCC__

#define RAMINIT_SYSINFO 1
#define CACHE_AS_RAM_ADDRESS_DEBUG 0

#define SET_NB_CFG_54 1 

//used by raminit
#define QRANK_DIMM_SUPPORT 1

//used by incoherent_ht
//#define K8_SCAN_PCI_BUS 1
//#define K8_ALLOCATE_IO_RANGE 1


//used by init_cpus and fidvid
#define K8_SET_FIDVID 0
//if we want to wait for core1 done before DQS training, set it to 0
#define K8_SET_FIDVID_CORE0_ONLY 1

#if CONFIG_K8_REV_F_SUPPORT == 1
#define K8_REV_F_SUPPORT_F0_F1_WORKAROUND 0
#endif

#include <stdint.h>
#include <string.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"

#if CONFIG_USE_FAILOVER_IMAGE==0
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include <cpu/amd/model_fxx_rev.h>
#include "southbridge/amd/amd8111/amd8111_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#endif



#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"

#if CONFIG_USE_FAILOVER_IMAGE==0
#include "cpu/x86/bist.h"

#include "lib/delay.c"

#include "northbridge/amd/amdk8/debug.c"
#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

#include "southbridge/amd/amd8111/amd8111_early_ctrl.c"

/*
 * GPIO28 of 8111 will control H0_MEMRESET_L
 * GPIO29 of 8111 will control H1_MEMRESET_L
 */
static void memreset_setup(void)
{
	if (is_cpu_pre_c0()) {
		/* Set the memreset low */
		outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(0<<0), SMBUS_IO_BASE + 0xc0 + 28);
		/* Ensure the BIOS has control of the memory lines */
		outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(0<<0), SMBUS_IO_BASE + 0xc0 + 29);
	} else {
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
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
        return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/amdk8.h"
#include "northbridge/amd/amdk8/coherent_ht.c"

#include "northbridge/amd/amdk8/incoherent_ht.c"

#include "northbridge/amd/amdk8/raminit.c"

#include "sdram/generic_sdram.c"
#include "ram/ramtest.c"

 /* tyan does not want the default */
#include "northbridge/amd/amdk8/resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

#define DIMM0 0x50
#define DIMM1 0x51
#define DIMM2 0x52
#define DIMM3 0x53
#define DIMM4 0x54
#define DIMM5 0x55
#define DIMM6 0x56
#define DIMM7 0x57


#include "cpu/amd/car/copy_and_run.c"
#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "cpu/amd/model_fxx/fidvid.c"
#endif

#if ((CONFIG_HAVE_FAILOVER_BOOT==1) && (CONFIG_USE_FAILOVER_IMAGE == 1)) || ((CONFIG_HAVE_FAILOVER_BOOT==0) && (CONFIG_USE_FALLBACK_IMAGE == 1))

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

        /* Setup the rom access for 4M */
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
#if CONFIG_HAVE_FAILOVER_BOOT==1
        __asm__ volatile ("jmp __fallback_image"
                : /* outputs */
                : "a" (bist), "b" (cpu_init_detectedx) /* inputs */
                )
#endif
	;
}
#endif

void real_main(unsigned long bist, unsigned long cpu_init_detectedx);

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
#if CONFIG_HAVE_FAILOVER_BOOT==1 
    #if CONFIG_USE_FAILOVER_IMAGE==1
	failover_process(bist, cpu_init_detectedx);	
    #else
	real_main(bist, cpu_init_detectedx);
    #endif
#else
    #if CONFIG_USE_FALLBACK_IMAGE == 1
	failover_process(bist, cpu_init_detectedx);	
    #endif
	real_main(bist, cpu_init_detectedx);
#endif
}

#if CONFIG_USE_FAILOVER_IMAGE==0

void real_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const uint16_t spd_addr[] = {
			//first node
                        DIMM0, DIMM2, 0, 0,
                        DIMM1, DIMM3, 0, 0,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
			//second node
                        DIMM4, DIMM6, 0, 0,
                        DIMM5, DIMM7, 0, 0,
#endif

	};

	struct sys_info *sysinfo = (CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE - CONFIG_DCACHE_RAM_GLOBAL_VAR_SIZE);

        int needs_reset; int i;
        unsigned bsp_apicid = 0;

        if (bist == 0) {
		bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
        }

 	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
        uart_init();
        console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

        print_debug("*sysinfo range: ["); print_debug_hex32(sysinfo); print_debug(",");  print_debug_hex32((unsigned long)sysinfo+sizeof(struct sys_info)); print_debug(")\r\n");

        setup_default_resource_map();

	print_debug("bsp_apicid="); print_debug_hex8(bsp_apicid); print_debug("\r\n");

#if CONFIG_MEM_TRAIN_SEQ == 1
        set_sysinfo_in_ram(0); // in BSP so could hold all ap until sysinfo is in ram 
#endif
	setup_coherent_ht_domain(); // routing table and start other core0

	wait_all_core0_started();
#if CONFIG_LOGICAL_CPUS==1
        // It is said that we should start core1 after all core0 launched
	/* becase optimize_link_coherent_ht is moved out from setup_coherent_ht_domain, 
	 * So here need to make sure last core0 is started, esp for two way system,
	 * (there may be apic id conflicts in that case) 
	 */
        start_other_cores();
	wait_all_other_cores_started(bsp_apicid);
#endif
	
	/* it will set up chains and store link pair for optimization later */
        ht_setup_chains_x(sysinfo); // it will init sblnk and sbbusn, nodes, sbdn


#if K8_SET_FIDVID == 1

        {
                msr_t msr;
	        msr=rdmsr(0xc0010042);
                print_debug("begin msr fid, vid "); print_debug_hex32( msr.hi ); print_debug_hex32(msr.lo); print_debug("\r\n");

        }

	enable_fid_change();

	enable_fid_change_on_sb(sysinfo->sbbusn, sysinfo->sbdn);

        init_fidvid_bsp(bsp_apicid);

        // show final fid and vid
        {
                msr_t msr;
               	msr=rdmsr(0xc0010042);
               	print_debug("end   msr fid, vid "); print_debug_hex32( msr.hi ); print_debug_hex32(msr.lo); print_debug("\r\n"); 

        }
#endif

	needs_reset = optimize_link_coherent_ht();
	needs_reset |= optimize_link_incoherent_ht(sysinfo);

        // fidvid change will issue one LDTSTOP and the HT change will be effective too
        if (needs_reset) {
                print_info("ht reset -\r\n");
                soft_reset_x(sysinfo->sbbusn, sysinfo->sbdn);
        }

	allow_all_aps_stop(bsp_apicid);

        //It's the time to set ctrl in sysinfo now;
	fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr);

	enable_smbus();

#if 0
	dump_smbus_registers();
#endif

	memreset_setup();

	//do we need apci timer, tsc...., only debug need it for better output
        /* all ap stopped? */
        init_timer(); // Need to use TMICT to synconize FID/VID
	sdram_initialize(sysinfo->nodes, sysinfo->ctrl, sysinfo);


#if 0
        dump_pci_devices();
#endif

        post_cache_as_ram(); // bsp swtich stack to ram and copy sysinfo ram now

}
#endif
