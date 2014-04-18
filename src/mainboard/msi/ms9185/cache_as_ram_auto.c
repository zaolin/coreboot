/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 Tyan
 * Copyright (C) 2006 AMD
 * Written by Yinghai Lu <yinghailu@gmail.com> for Tyan and AMD.
 *
 * Copyright (C) 2006 MSI
 * Written by bxshi <bingxunshi@gmail.com> for MSI.
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
#define K8_SET_FIDVID 1
//if we want to wait for core1 done before DQS training, set it to 0
#define K8_SET_FIDVID_CORE0_ONLY 1

#define DEBUG_SMBUS 1

#include <stdint.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"

#if 0
static void post_code(uint8_t value) {
#if 1
        int i;
        for(i=0;i<0x80000;i++) {
                outb(value, 0x80);
        }
#endif
}
#endif

#include <cpu/amd/model_fxx_rev.h>
#include "southbridge/broadcom/bcm5785/bcm5785_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#if CONFIG_USE_INIT == 0
       #include "lib/memcpy.c"
#endif


#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "superio/nsc/pc87417/pc87417_early_serial.c"
#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, PC87417_SP1)
#define RTC_DEV PNP_DEV(0x2e, PC87417_RTC)
#include "southbridge/broadcom/bcm5785/bcm5785_early_setup.c"
static void memreset_setup(void)
{
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
#define SMBUS_SWITCH1 0x70
#define SMBUS_SWITCH2 0x72
        unsigned device = (ctrl->channel0[0]) >> 8;
        smbus_send_byte(SMBUS_SWITCH1, device & 0x0f);
        smbus_send_byte(SMBUS_SWITCH2, (device >> 4) & 0x0f );
}

#if 0
static inline void change_i2c_mux(unsigned device)
{
#define SMBUS_SWITCH1 0x70
#define SMBUS_SWITCH2 0x72
        smbus_send_byte(SMBUS_SWITCH1, device & 0x0f);
        smbus_send_byte(SMBUS_SWITCH2, (device >> 4) & 0x0f );
}
#endif



static inline int spd_read_byte(unsigned device, unsigned address)
{
        return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/amdk8_f.h"
#include "northbridge/amd/amdk8/coherent_ht.c"

#include "northbridge/amd/amdk8/incoherent_ht.c"

#include "northbridge/amd/amdk8/raminit_f.c"

#include "sdram/generic_sdram.c"

 /* msi does not want the default */
#include "resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

#define RC0 (0x10<<8)
#define RC1 (0x01<<8)

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

#if USE_FALLBACK_IMAGE == 1

#include "northbridge/amd/amdk8/early_ht.c"

void failover_process(unsigned long bist, unsigned long cpu_init_detectedx)
{

        /* Is this a cpu only reset? Is this a secondary cpu? */
        if ((cpu_init_detectedx) || (!boot_cpu())) {
                if (last_boot_normal()) { // RTC already inited
                        goto normal_image;
                } else {
                        goto fallback_image;
                }
        }
        /* Nothing special needs to be done to find bus 0 */
        /* Allow the HT devices to be found */

        enumerate_ht_chain();

        bcm5785_enable_rom();

        bcm5785_enable_lpc();

        //enable RTC
        pc87417_enable_dev(RTC_DEV);

        /* Is this a deliberate reset by the bios */
//        post_code(0x22);
        if (bios_reset_detected() && last_boot_normal()) {
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
//        post_code(0x23);
        __asm__ volatile ("jmp __normal_image"
                : /* outputs */
                : "a" (bist), "b" (cpu_init_detectedx) /* inputs */
                );

 fallback_image:
//        post_code(0x25);
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
       static const uint16_t spd_addr[] = {
                       //first node
                        RC0|DIMM0, RC0|DIMM2, RC0|DIMM4, RC0|DIMM6,
                        RC0|DIMM1, RC0|DIMM3, RC0|DIMM5, RC0|DIMM7,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
                       //second node
                       RC1|DIMM0, RC1|DIMM2, RC1|DIMM4, RC1|DIMM6,
                       RC1|DIMM1, RC1|DIMM3, RC1|DIMM5, RC1|DIMM7,
#endif

       };

       struct sys_info *sysinfo = (DCACHE_RAM_BASE + DCACHE_RAM_SIZE - DCACHE_RAM_GLOBAL_VAR_SIZE);

        int needs_reset;
        unsigned bsp_apicid = 0;

        if (bist == 0) {
               bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
        }

//     post_code(0x32);

       pc87417_enable_serial(SERIAL_DEV, TTYS0_BASE);
        uart_init();
        console_init();

//     dump_mem(DCACHE_RAM_BASE+DCACHE_RAM_SIZE-0x200, DCACHE_RAM_BASE+DCACHE_RAM_SIZE);

       /* Halt if there was a built in self test failure */
       report_bist_failure(bist);

       print_debug("*sysinfo range: ["); print_debug_hex32(sysinfo); print_debug(",");  print_debug_hex32((unsigned long)sysinfo+sizeof(struct sys_info)); print_debug(")\r\n");

       setup_ms9185_resource_map();
#if 0
        dump_pci_device(PCI_DEV(0, 0x18, 0));
       dump_pci_device(PCI_DEV(0, 0x19, 0));
#endif

       print_debug("bsp_apicid="); print_debug_hex8(bsp_apicid); print_debug("\r\n");

       setup_coherent_ht_domain();

       wait_all_core0_started();
#if CONFIG_LOGICAL_CPUS==1
        // It is said that we should start core1 after all core0 launched
       /* becase optimize_link_coherent_ht is moved out from setup_coherent_ht_domain,
        * So here need to make sure last core0 is started, esp for two way system,
        * (there may be apic id conflicts in that case)
        */
        start_other_cores();
//bx_a010-     wait_all_other_cores_started(bsp_apicid);
#endif

       /* it will set up chains and store link pair for optimization later */
        ht_setup_chains_x(sysinfo); // it will init sblnk and sbbusn, nodes, sbdn

       bcm5785_early_setup();


#if 0
       //it your CPU min fid is 1G, you can change HT to 1G and FID to max one time.
        needs_reset = optimize_link_coherent_ht();
        needs_reset |= optimize_link_incoherent_ht(sysinfo);
#endif

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

#if 1
       needs_reset = optimize_link_coherent_ht();
       needs_reset |= optimize_link_incoherent_ht(sysinfo);

        // fidvid change will issue one LDTSTOP and the HT change will be effective too
        if (needs_reset) {
                print_info("ht reset -\r\n");
                soft_reset();
        }
#endif
       allow_all_aps_stop(bsp_apicid);

        //It's the time to set ctrl in sysinfo now;
       fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr);

       enable_smbus();

#if 0
       int i;
       for(i=0;i<2;i++) {
               activate_spd_rom(sysinfo->ctrl+i);
               dump_smbus_registers();
       }
#endif

#if 0
       int i;
        for(i=1;i<256;i<<=1) {
                change_i2c_mux(i);
                dump_smbus_registers();
        }
#endif

       memreset_setup();

       //do we need apci timer, tsc...., only debug need it for better output
        /* all ap stopped? */
//        init_timer(); // Need to use TMICT to synconize FID/VID

       sdram_initialize(sysinfo->nodes, sysinfo->ctrl, sysinfo);

#if 0
        print_pci_devices();
#endif

#if 0
//        dump_pci_devices();
        dump_pci_device_index_wait(PCI_DEV(0, 0x18, 2), 0x98);
       dump_pci_device_index_wait(PCI_DEV(0, 0x19, 2), 0x98);
#endif

       post_cache_as_ram();


}
