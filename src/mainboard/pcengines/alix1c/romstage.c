/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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
 */

#include <stdint.h>
#include <spd.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "console/console.c"
#include "lib/ramtest.c"
#include "cpu/x86/bist.h"
#include "cpu/x86/msr.h"
#include <cpu/amd/lxdef.h>
#include <cpu/amd/geode_post_code.h>
#include "southbridge/amd/cs5536/cs5536.h"

#define POST_CODE(x) outb(x, 0x80)
#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

/* The ALIX1.C has no SMBus; the setup is hard-wired. */
void cs5536_enable_smbus(void)
{
}

#include "southbridge/amd/cs5536/cs5536_early_setup.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"

/* The part is a Hynix hy5du121622ctp-d43.
 *
 * HY 5D U 12 16 2 2 C <blank> T <blank> P D43
 * Hynix
 * DDR SDRAM (5D)
 * VDD 2.5 VDDQ 2.5 (U)
 * 512M 8K REFRESH (12)
 * x16 (16)
 * 4banks (2)
 * SSTL_2 (2)
 * 4th GEN die (C)
 * Normal Power Consumption (<blank> )
 * TSOP (T)
 * Single Die (<blank>)
 * Lead Free (P)
 * DDR400 3-3-3 (D43)
 */
/* SPD array */
static const u8 spdbytes[] = {
	[SPD_ACCEPTABLE_CAS_LATENCIES] = 0x10,
	[SPD_BANK_DENSITY] = 0x40,
	[SPD_DEVICE_ATTRIBUTES_GENERAL] = 0xff,
	[SPD_MEMORY_TYPE] = 7,
	[SPD_MIN_CYCLE_TIME_AT_CAS_MAX] = 10, /* A guess for the tRAC value */
	[SPD_MODULE_ATTRIBUTES] = 0xff, /* FIXME later when we figure out. */
	[SPD_NUM_BANKS_PER_SDRAM] = 4,
	[SPD_PRIMARY_SDRAM_WIDTH] = 8,
	[SPD_NUM_DIMM_BANKS] = 1, /* ALIX1.C is 1 bank. */
	[SPD_NUM_COLUMNS] = 0xa,
	[SPD_NUM_ROWS] = 3,
	[SPD_REFRESH] = 0x3a,
	[SPD_SDRAM_CYCLE_TIME_2ND] = 60,
	[SPD_SDRAM_CYCLE_TIME_3RD] = 75,
	[SPD_tRAS] = 40,
	[SPD_tRCD] = 15,
	[SPD_tRFC] = 70,
	[SPD_tRP] = 15,
	[SPD_tRRD] = 10,
};

static u8 spd_read_byte(u8 device, u8 address)
{
	print_debug("spd_read_byte dev ");
	print_debug_hex8(device);

	if (device != (0x50 << 1)) {
		print_debug(" returns 0xff\n");
		return 0xff;
	}

	print_debug(" addr ");
	print_debug_hex8(address);
	print_debug(" returns ");
	print_debug_hex8(spdbytes[address]);
	print_debug("\n");

	return spdbytes[address];
}

#define ManualConf	0		/* Do automatic strapped PLL config */
#define PLLMSRhi	0x00001490	/* Manual settings for the PLL */
#define PLLMSRlo	0x02000030

#define DIMM0		0xa0
#define DIMM1		0xa2

#include "northbridge/amd/lx/raminit.h"
#include "northbridge/amd/lx/pll_reset.c"
#include "northbridge/amd/lx/raminit.c"
#include "lib/generic_sdram.c"
#include "cpu/amd/model_lx/cpureginit.c"
#include "cpu/amd/model_lx/syspreinit.c"

static void msr_init(void)
{
	msr_t msr;

	/* Setup access to the MC for under 1MB. Note MC not setup yet. */
	msr.hi = 0x24fffc02;
	msr.lo = 0x10010000;
	wrmsr(CPU_RCONF_DEFAULT, msr);

	msr.hi = 0x20000000;
	msr.lo = 0xfff00;
	wrmsr(MSR_GLIU0 + 0x20, msr);

	msr.hi = 0x20000000;
	msr.lo = 0xfff00;
	wrmsr(MSR_GLIU1 + 0x20, msr);
}

/** Early mainboard specific GPIO setup. */
static void mb_gpio_init(void)
{
}

void cache_as_ram_main(void)
{
	static const struct mem_controller memctrl[] = {
		{.channel0 = {0x50}},
	};

	extern void RestartCAR();

	POST_CODE(0x01);

	SystemPreInit();
	msr_init();

	cs5536_early_setup();

	/* NOTE: Must do this AFTER cs5536_early_setup()!
	 * It is counting on some early MSR setup for the CS5536.
	 */
	cs5536_disable_internal_uart();
	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	mb_gpio_init();
	uart_init();
	console_init();

	pll_reset(ManualConf);

	cpuRegInit();

	sdram_initialize(1, memctrl);

	/* Check memory */
	/* Enable this only if you are having questions. */
	/* ram_check(0, 640 * 1024); */

	/* Switch from Cache as RAM to real RAM.
	 *
	 * There are two ways we could think about this.
	 *
	 * 1. If we are using the romstage.inc ROMCC way, the stack is
	 * going to be re-setup in the code following this code.  Just
	 * wbinvd the stack to clear the cache tags.  We don't care
	 * where the stack used to be.
	 *
	 * 2. This file is built as a normal .c -> .o and linked in
	 * etc.  The stack might be used to return etc.  That means we
	 * care about what is in the stack.  If we are smart we set
	 * the CAR stack to the same location as the rest of
	 * coreboot. If that is the case we can just do a wbinvd.
	 * The stack will be written into real RAM that is now setup
	 * and we continue like nothing happened.  If the stack is
	 * located somewhere other than where LB would like it, you
	 * need to write some code to do a copy from cache to RAM
	 *
	 * We use method 1 on Norwich and on this board too.
	 */
	POST_CODE(0x02);
	print_err("POST 02\n");
	__asm__("wbinvd\n");
	print_err("Past wbinvd\n");

	/* We are finding the return does not work on this board. Explicitly
	 * call the label that is after the call to us. This is gross, but
	 * sometimes at this level it is the only way out.
	 */
	void done_cache_as_ram_main(void);
	done_cache_as_ram_main();
}

