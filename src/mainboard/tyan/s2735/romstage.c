 
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

#include "southbridge/intel/i82801ex/i82801ex_early_smbus.c"
#include "northbridge/intel/e7501/raminit.h"

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/intel/e7501/debug.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"

#include "cpu/x86/mtrr/earlymtrr.c"
#include "cpu/x86/bist.h"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

// FIXME: There's another hard_reset() in reset.c. Why?
static void hard_reset(void)
{
        /* full reset */
	outb(0x0a, 0x0cf9);
        outb(0x0e, 0x0cf9);
}

static void soft_reset(void)
{
#if 1
        /* link reset */
	outb(0x02, 0x0cf9);
        outb(0x06, 0x0cf9);
#endif
}

static void memreset_setup(void)
{
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/intel/e7501/raminit.c"
#include "northbridge/intel/e7501/reset_test.c"
#include "lib/generic_sdram.c"

void main(unsigned long bist)
{
	static const struct mem_controller memctrl[] = {
                {
                        .d0 = PCI_DEV(0, 0, 0),
                        .d0f1 = PCI_DEV(0, 0, 1),
                        .channel0 = { (0xa<<3)|0, (0xa<<3)|1, (0xa<<3)|2, 0 },
                        .channel1 = { (0xa<<3)|4, (0xa<<3)|5, (0xa<<3)|6, 0 },
                },
	};
	
	unsigned cpu_reset = 0;

       if (bist == 0) 
	{
//		early_mtrr_init();
                enable_lapic();

        }

//	post_code(0x32);
	
 	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
        uart_init();
        console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

//        setup_s2735_resource_map();

	if(bios_reset_detected()) {
		hard_reset();
	}

	enable_smbus();
#if 0
	dump_spd_registers(&memctrl[0]);
#endif
#if 0
	dump_smbus_registers();
#endif

	memreset_setup();
	sdram_initialize(1, memctrl);

#if 0
	dump_pci_devices();
#endif

#if 1
        dump_pci_device(PCI_DEV(0, 0, 0));
#endif
}

