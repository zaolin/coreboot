/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 LiPPERT Embedded Computers GmbH
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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

/* Based on romstage.c from AMD's DB800 and DBM690T mainboards. */

#include <stdlib.h>
#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
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

#include "southbridge/amd/cs5536/cs5536_early_smbus.c"
#include "southbridge/amd/cs5536/cs5536_early_setup.c"
#include "superio/ite/it8712f/it8712f_early_serial.c"

#define ManualConf 1		/* No automatic strapped PLL config */
#define PLLMSRhi 0x0000049C	/* Manual settings for the PLL */
#define PLLMSRlo 0x00DE6001
#define DIMM0 0xA0
#define DIMM1 0xA2

static inline int spd_read_byte(unsigned int device, unsigned int address)
{
	if (device != DIMM0)
		return 0xFF;	/* No DIMM1, don't even try. */

	return smbus_read_byte(device, address);
}

#include "northbridge/amd/lx/raminit.h"
#include "northbridge/amd/lx/pll_reset.c"
#include "northbridge/amd/lx/raminit.c"
#include "lib/generic_sdram.c"
#include "cpu/amd/model_lx/cpureginit.c"
#include "cpu/amd/model_lx/syspreinit.c"

static void msr_init(void)
{
	msr_t msr;

	/* Setup access to the cache for under 1MB. */
	msr.hi = 0x24fffc02;
	msr.lo = 0x1000A000;	/* 0-A0000 write back */
	wrmsr(CPU_RCONF_DEFAULT, msr);

	msr.hi = 0x0;		/* Write back */
	msr.lo = 0x0;
	wrmsr(CPU_RCONF_A0_BF, msr);
	wrmsr(CPU_RCONF_C0_DF, msr);
	wrmsr(CPU_RCONF_E0_FF, msr);

	/* Setup access to the cache for under 640K. Note MC not setup yet. */
	msr.hi = 0x20000000;
	msr.lo = 0xfff80;
	wrmsr(MSR_GLIU0 + 0x20, msr);

	msr.hi = 0x20000000;
	msr.lo = 0x80fffe0;
	wrmsr(MSR_GLIU0 + 0x21, msr);

	msr.hi = 0x20000000;
	msr.lo = 0xfff80;
	wrmsr(MSR_GLIU1 + 0x20, msr);

	msr.hi = 0x20000000;
	msr.lo = 0x80fffe0;
	wrmsr(MSR_GLIU1 + 0x21, msr);
}

static const u16 sio_init_table[] = {	// hi=data, lo=index
	0x0707,		// select LDN 7 (GPIO, SPI, watchdog, ...)
	0x1E2C,		// disable ATXPowerGood - will cause a reboot!
	0x0423,		// don't delay POWerOK1/2
	0x9072,		// watchdog triggers POWOK, counts seconds
#if !CONFIG_USE_WATCHDOG_ON_BOOT
	0x0073, 0x0074,	// disable watchdog by setting timeout to 0
#endif
	0xBF25, 0x372A, 0xF326, // select GPIO function for most pins
	0xBF27, 0xFF28, 0x2529, // (GP36=FAN_CTL3, GP13=PWROK1)
	0x1E2C,		// VIN6=enabled?, FAN4/5 enabled, VIN7=internal, VIN3=enabled
	0x46B8, 0x0CB9,	// enable pullups
	0x36C0,		// enable Simple-I/O for GP15,14,12,11= LIVE_LED, WD_ACTIVE, RS485_EN2,1
	0xFFC3,		// enable Simple-I/O for GP47-40 (GPIOs on Supervisory Connector)
	0x26C8,		// config GP15,12,11 as output; GP14 as input
	0x2DF5,		// map Hw Monitor Thermal Output to GP55
	0x0DF8,		// map GP LED Blinking 1 to GP15=LIVE_LED (deactivate Simple-I/O to use)
};

/* Early mainboard specific GPIO setup. */
static void mb_gpio_init(void)
{
	int i;

	/* Init Super I/O WDT, GPIOs. Done early, WDT init may trigger reset! */
	it8712f_enter_conf();
	for (i = 0; i < ARRAY_SIZE(sio_init_table); i++) {
		u16 val = sio_init_table[i];
		outb((u8)val, SIO_INDEX);
		outb(val >> 8, SIO_DATA);
	}
	it8712f_exit_conf();
}

void cache_as_ram_main(void)
{
	POST_CODE(0x01);

	static const struct mem_controller memctrl[] = {
		{.channel0 = {(0xa << 3) | 0, (0xa << 3) | 1}}
	};

	SystemPreInit();
	msr_init();

	cs5536_early_setup();

	/*
	 * Note: must do this AFTER the early_setup! It is counting on some
	 * early MSR setup for CS5536.
	 */
	it8712f_enable_serial(0, CONFIG_TTYS0_BASE); // Does not use its 1st parameter
	mb_gpio_init();
	uart_init();
	console_init();

	pll_reset(ManualConf);

	cpuRegInit();

	sdram_initialize(1, memctrl);

	/* Check memory. */
	/* ram_check(0x00000000, 640 * 1024); */

	/* Memory is setup. Return to cache_as_ram.inc and continue to boot. */
	return;
}

