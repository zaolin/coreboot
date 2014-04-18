/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 AMD
 * Written by Yinghai Lu <yinghailu@amd.com> for AMD.
 *
 * Copyright (C) 2006 MSI
 * Written by Bingxun Shi <bingxunshi@gmail.com> for MSI.
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

#define RAMINIT_SYSINFO 1
#define CACHE_AS_RAM_ADDRESS_DEBUG 0

#define SET_NB_CFG_54 1

//used by raminit
#define QRANK_DIMM_SUPPORT 1

//used by init_cpus and fidvid
#define K8_SET_FIDVID 1
//if we want to wait for core1 done before DQS training, set it to 0
#define K8_SET_FIDVID_CORE0_ONLY 1

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
#include "arch/i386/lib/console.c"

#include <cpu/amd/model_fxx_rev.h>
#include "southbridge/nvidia/mcp55/mcp55_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "superio/winbond/w83627ehg/w83627ehg_early_serial.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627EHG_SP1)
#define RTC_DEV PNP_DEV(0x2e, W83627EHG_RTC)

#include <device/pci_ids.h>
#include "southbridge/nvidia/mcp55/mcp55_early_ctrl.c"
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
        unsigned device=(ctrl->channel0[0])>>8;
        smbus_send_byte(SMBUS_SWITCH1, device);
       smbus_send_byte(SMBUS_SWITCH2, (device >> 4) & 0x0f);
}

#if 0
static inline void change_i2c_mux(unsigned device)
{
#define SMBUS_SWITCH1 0x70
#define SMBUS_SWITHC2 0x72
        smbus_send_byte(SMBUS_SWITCH1, device & 0x0f);
       smbus_send_byte(SMBUS_SWITCH2, (device >> 4) & 0x0f);
}
#endif

static inline int spd_read_byte(unsigned device, unsigned address)
{
       return smbus_read_byte(device, address);
}

//#define K8_4RANK_DIMM_SUPPORT 1

#include "northbridge/amd/amdk8/amdk8_f.h"
#include "northbridge/amd/amdk8/raminit_f.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "lib/generic_sdram.c"

 /* msi does not want the default */
#include "resourcemap.c"
#include "cpu/amd/dualcore/dualcore.c"

#define MCP55_NUM 1
#include "southbridge/nvidia/mcp55/mcp55_early_setup_ss.h"
//set GPIO to input mode
#define MCP55_MB_SETUP \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+15, ~(0xff), ((0<<4)|(0<<2)|(0<<0)),/* M8,GPIO16, PCIXA_PRSNT2_L*/ \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+44, ~(0xff), ((0<<4)|(0<<2)|(0<<0)),/* P5,GPIO45, PCIXA_PRSNT1_L*/ \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+16, ~(0xff), ((0<<4)|(0<<2)|(0<<0)),/* K4,GPIO17, PCIXB_PRSNT1_L*/ \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+45, ~(0xff), ((0<<4)|(0<<2)|(0<<0)),/* P7,GPIO46, PCIXB_PRSNT2_L*/ \

#include "southbridge/nvidia/mcp55/mcp55_early_setup_car.c"

#include "cpu/amd/car/copy_and_run.c"

#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"
#include "cpu/amd/model_fxx/fidvid.c"

#if CONFIG_USE_FALLBACK_IMAGE == 1

#include "southbridge/nvidia/mcp55/mcp55_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"


static void sio_setup(void)
{

        unsigned value;
        uint32_t dword;
        uint8_t byte;

        byte = pci_read_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0x7b);
        byte |= 0x20;
        pci_write_config8(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0x7b, byte);

        dword = pci_read_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0xa0);
        dword |= (1<<0);
        pci_write_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0xa0, dword);


}
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

        sio_setup();

        /* Setup the mcp55 */
        mcp55_enable_rom();

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
                : "a" (bist), "b"(cpu_init_detectedx) /* inputs */
                );

 fallback_image:
       ;
}
#endif

void real_main(unsigned long bist, unsigned long cpu_init_detectedx);

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{

#if CONFIG_USE_FALLBACK_IMAGE == 1
        failover_process(bist, cpu_init_detectedx);
#endif
        real_main(bist, cpu_init_detectedx);

}

//CPU 1 mem is on SMBUS_HUB channel 2, and CPU 2 mem is on channel 1.
#define RC0 (2<<8)
#define RC1 (1<<8)

void real_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
       static const uint16_t spd_addr [] = {
                       RC0|(0xa<<3)|0, RC0|(0xa<<3)|2, RC0|(0xa<<3)|4, RC0|(0xa<<3)|6,
                       RC0|(0xa<<3)|1, RC0|(0xa<<3)|3, RC0|(0xa<<3)|5, RC0|(0xa<<3)|7,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
                       RC1|(0xa<<3)|0, RC1|(0xa<<3)|2, RC1|(0xa<<3)|4, RC1|(0xa<<3)|6,
                       RC1|(0xa<<3)|1, RC1|(0xa<<3)|3, RC1|(0xa<<3)|5, RC1|(0xa<<3)|7,
#endif
       };

       unsigned bsp_apicid = 0;
        int needs_reset;
       struct sys_info *sysinfo = (CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE - CONFIG_DCACHE_RAM_GLOBAL_VAR_SIZE);
       char *p ;

        if (bist == 0) {
               //init_cpus(cpu_init_detectedx);
               bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
        }

       w83627ehg_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
        uart_init();
        console_init();

       /* Halt if there was a built in self test failure */
       report_bist_failure(bist);

        setup_ms9282_resource_map();

       setup_coherent_ht_domain();

        wait_all_core0_started();

#if CONFIG_LOGICAL_CPUS==1
        // It is said that we should start core1 after all core0 launched
        start_other_cores();
        //wait_all_other_cores_started(bsp_apicid);
#endif
        ht_setup_chains_x(sysinfo); // it will init sblnk and sbbusn, nodes, sbdn

       needs_reset = optimize_link_coherent_ht();

       needs_reset |= optimize_link_incoherent_ht(sysinfo);

        needs_reset |= mcp55_early_setup_x();

               if (needs_reset) {
                       print_info("ht reset -\r\n");
                       soft_reset();
               }

        //It's the time to set ctrl now;
        fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr);

       enable_smbus();

#if 0
        int i;
        for(i=4;i<8;i++) {
                change_i2c_mux(i);
                dump_smbus_registers();
        }
#endif

       memreset_setup();

       sdram_initialize(sysinfo->nodes, sysinfo->ctrl, sysinfo);

       post_cache_as_ram();

}
