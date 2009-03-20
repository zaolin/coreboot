/*
 * This file is part of the coreboot project.
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

#define K8_ALLOCATE_IO_RANGE 1
// #define K8_SCAN_PCI_BUS 1

#define QRANK_DIMM_SUPPORT 1

#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#endif

// used by init_cpus and fidvid
#define K8_SET_FIDVID 1
//if we want to wait for core1 done before DQS training, set it to 0
#define K8_SET_FIDVID_CORE0_ONLY 1

#if K8_REV_F_SUPPORT == 1
#define K8_REV_F_SUPPORT_F0_F1_WORKAROUND 0
#endif

#include <stdint.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"

// for enable the FAN
#include "southbridge/nvidia/mcp55/mcp55_early_smbus.c"

#if USE_FAILOVER_IMAGE==0
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"

#include <cpu/amd/model_fxx_rev.h>

// #include "southbridge/nvidia/mcp55/mcp55_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#endif

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"
#include "superio/winbond/w83627hf/w83627hf_early_init.c"

#if USE_FAILOVER_IMAGE==0

#include "cpu/x86/bist.h"

#if CONFIG_USE_INIT == 0
#include "lib/memcpy.c"
#endif

#include "northbridge/amd/amdk8/debug.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

#include "southbridge/nvidia/mcp55/mcp55_early_ctrl.c"

static void memreset_setup(void)
{
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

static int smbus_send_byte_one(unsigned device, unsigned char val)
{
	return do_smbus_send_byte(SMBUS1_IO_BASE, device, val);
}

static void dump_smbus_registers(void)
{
	u32 device;

	print_debug("\r\n");
	for (device = 1; device < 0x80; device++) {
		int j;
		if (smbus_read_byte(device, 0) < 0)
			continue;
		printk_debug("smbus: %02x", device);
		for (j = 0; j < 256; j++) {
			int status;
			unsigned char byte;
			status = smbus_read_byte(device, j);
			if (status < 0) {
				break;
			}
			if ((j & 0xf) == 0) {
				printk_debug("\r\n%02x: ", j);
			}
			byte = status & 0xff;
			printk_debug("%02x ", byte);
		}
		print_debug("\r\n");
	}
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
/* We don't do any switching yet.
#define SMBUS_SWITCH1 0x48
#define SMBUS_SWITCH2 0x49
	unsigned device=(ctrl->channel0[0])>>8;
	smbus_send_byte(SMBUS_SWITCH1, device);
	smbus_send_byte(SMBUS_SWITCH2, (device >> 4) & 0x0f);
*/
	/* nothing to do */
}

/*
static inline void change_i2c_mux(unsigned device)
{
#define SMBUS_SWITCH1 0x48
#define SMBUS_SWITHC2 0x49
	smbus_send_byte(SMBUS_SWITCH1, device & 0x0f);
	smbus_send_byte_one(SMBUS_SWITCH2, (device >> 4) & 0x0f);
	int ret;
        print_debug("change_i2c_mux i="); print_debug_hex8(device); print_debug("\r\n");
	dump_smbus_registers();
        ret = smbus_send_byte(SMBUS_SWITCH1, device);
        print_debug("change_i2c_mux ret="); print_debug_hex32(ret); print_debug("\r\n");
	dump_smbus_registers();
        ret = smbus_send_byte_one(SMBUS_SWITCH2, device);
        print_debug("change_i2c_mux ret="); print_debug_hex32(ret); print_debug("\r\n");
	dump_smbus_registers();
}
*/

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/amdk8_f.h"
#include "northbridge/amd/amdk8/coherent_ht.c"

#include "northbridge/amd/amdk8/incoherent_ht.c"

#include "northbridge/amd/amdk8/raminit_f.c"

#include "sdram/generic_sdram.c"

#include "resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

#define MCP55_NUM 1
#define MCP55_USE_NIC 1
#define MCP55_USE_AZA 1

#define MCP55_PCI_E_X_0 4

#include "southbridge/nvidia/mcp55/mcp55_early_setup_ss.h"
#include "southbridge/nvidia/mcp55/mcp55_early_setup_car.c"

#include "cpu/amd/car/copy_and_run.c"

#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "cpu/amd/model_fxx/fidvid.c"

#endif

#if ((HAVE_FAILOVER_BOOT==1) && (USE_FAILOVER_IMAGE == 1)) || ((HAVE_FAILOVER_BOOT==0) && (USE_FALLBACK_IMAGE == 1))

#include "southbridge/nvidia/mcp55/mcp55_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"

static void sio_setup(void)
{

	u32 value;
	uint32_t dword;
	uint8_t byte;

	enable_smbus();
//      smbusx_write_byte(1, (0x58>>1), 0, 0x80); /* select bank0 */
	smbusx_write_byte(1, (0x58 >> 1), 0xb1, 0xff);	/* set FAN ctrl to DC mode */

	byte = pci_read_config8(PCI_DEV(0, MCP55_DEVN_BASE + 1, 0), 0x7b);
	byte |= 0x20;
	pci_write_config8(PCI_DEV(0, MCP55_DEVN_BASE + 1, 0), 0x7b, byte);

	dword = pci_read_config32(PCI_DEV(0, MCP55_DEVN_BASE + 1, 0), 0xa0);
	dword |= (1 << 0);
	pci_write_config32(PCI_DEV(0, MCP55_DEVN_BASE + 1, 0), 0xa0, dword);

	dword = pci_read_config32(PCI_DEV(0, MCP55_DEVN_BASE + 1, 0), 0xa4);
	dword |= (1 << 16);
	pci_write_config32(PCI_DEV(0, MCP55_DEVN_BASE + 1, 0), 0xa4, dword);

}

void failover_process(unsigned long bist, unsigned long cpu_init_detectedx)
{
	u32 last_boot_normal_x = last_boot_normal();

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
	} else {
		goto fallback_image;
	}
normal_image:
	__asm__ volatile ("jmp __normal_image":	/* outputs */
			  :"a" (bist), "b"(cpu_init_detectedx)	/* inputs */
	    );

      fallback_image:
#if HAVE_FAILOVER_BOOT==1
	__asm__ volatile ("jmp __fallback_image":	/* outputs */
			  :"a" (bist), "b"(cpu_init_detectedx)	/* inputs */
	    )
#endif
	;
}
#endif
void real_main(unsigned long bist, unsigned long cpu_init_detectedx);

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
#if HAVE_FAILOVER_BOOT==1
#if USE_FAILOVER_IMAGE==1
	failover_process(bist, cpu_init_detectedx);
#else
	real_main(bist, cpu_init_detectedx);
#endif
#else
#if USE_FALLBACK_IMAGE == 1
	failover_process(bist, cpu_init_detectedx);
#endif
	real_main(bist, cpu_init_detectedx);
#endif
}

/* We have no idea where the SMBUS switch is. This doesn't do anything ATM. */
#define RC0 (2<<8)
#define RC1 (1<<8)

#if USE_FAILOVER_IMAGE==0

void real_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
/* The SPD is being read from the CPU1 (marked CPU2 on the board) and we
   don't know how to switch the SMBus to decode the CPU0 SPDs. So, The
   memory on each CPU must be an exact match.
 */
	static const uint16_t spd_addr[] = {
		RC0 | (0xa << 3) | 0, RC0 | (0xa << 3) | 2,
		    RC0 | (0xa << 3) | 4, RC0 | (0xa << 3) | 6,
		RC0 | (0xa << 3) | 1, RC0 | (0xa << 3) | 3,
		    RC0 | (0xa << 3) | 5, RC0 | (0xa << 3) | 7,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
		RC1 | (0xa << 3) | 0, RC1 | (0xa << 3) | 2,
		    RC1 | (0xa << 3) | 4, RC1 | (0xa << 3) | 6,
		RC1 | (0xa << 3) | 1, RC1 | (0xa << 3) | 3,
		    RC1 | (0xa << 3) | 5, RC1 | (0xa << 3) | 7,
#endif
	};

	struct sys_info *sysinfo =
	    (DCACHE_RAM_BASE + DCACHE_RAM_SIZE - DCACHE_RAM_GLOBAL_VAR_SIZE);

	int needs_reset = 0;
	unsigned bsp_apicid = 0;

	if (bist == 0) {
		bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
	}

	pnp_enter_ext_func_mode(SERIAL_DEV);
	pnp_write_config(SERIAL_DEV, 0x24, 0x84 | (1 << 6));
	w83627hf_enable_dev(SERIAL_DEV, TTYS0_BASE);
	pnp_exit_ext_func_mode(SERIAL_DEV);

	uart_init();
	console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

	print_debug("*sysinfo range: [");
	print_debug_hex32(sysinfo);
	print_debug(",");
	print_debug_hex32((unsigned long)sysinfo + sizeof(struct sys_info));
	print_debug(")\r\n");

	setup_mb_resource_map();

	print_debug("bsp_apicid=");
	print_debug_hex8(bsp_apicid);
	print_debug("\r\n");

#if MEM_TRAIN_SEQ == 1
	set_sysinfo_in_ram(0);	// in BSP so could hold all ap until sysinfo is in ram
#endif
/*	dump_smbus_registers(); */
	setup_coherent_ht_domain();	// routing table and start other core0

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
	ht_setup_chains_x(sysinfo);	// it will init sblnk and sbbusn, nodes, sbdn

#if K8_SET_FIDVID == 1

	{
		msr_t msr;
		msr = rdmsr(0xc0010042);
		print_debug("begin msr fid, vid ");
		print_debug_hex32(msr.hi);
		print_debug_hex32(msr.lo);
		print_debug("\r\n");

	}

	enable_fid_change();

	enable_fid_change_on_sb(sysinfo->sbbusn, sysinfo->sbdn);

	init_fidvid_bsp(bsp_apicid);

	// show final fid and vid
	{
		msr_t msr;
		msr = rdmsr(0xc0010042);
		print_debug("end   msr fid, vid ");
		print_debug_hex32(msr.hi);
		print_debug_hex32(msr.lo);
		print_debug("\r\n");

	}
#endif

#if 1
	needs_reset |= optimize_link_coherent_ht();
	needs_reset |= optimize_link_incoherent_ht(sysinfo);
	needs_reset |= mcp55_early_setup_x();

	// fidvid change will issue one LDTSTOP and the HT change will be effective too
	if (needs_reset) {
		print_info("ht reset -\r\n");
		soft_reset();
	}
#endif
	allow_all_aps_stop(bsp_apicid);

	//It's the time to set ctrl in sysinfo now;
	fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr);

	enable_smbus();		/* enable in sio_setup */

	memreset_setup();

	//do we need apci timer, tsc...., only debug need it for better output
	/* all ap stopped? */
//        init_timer(); // Need to use TMICT to synconize FID/VID

	sdram_initialize(sysinfo->nodes, sysinfo->ctrl, sysinfo);

	post_cache_as_ram();	// bsp swtich stack to ram and copy sysinfo ram now

}

#endif
