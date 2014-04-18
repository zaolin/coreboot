#define ASSEMBLY 1

#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include <stdlib.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "northbridge/via/vt8601/raminit.h"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "cpu/x86/bist.h"

/*
 */
void udelay(unsigned usecs) 
{
	int i;
	for (i = 0; i < usecs; i++)
		outb(i&0xff, 0x80);
}

#include "lib/delay.c"
#include "cpu/x86/lapic/boot_cpu.c"
#include "lib/debug.c"

#include "southbridge/via/vt8231/vt8231_early_smbus.c"
#include "southbridge/via/vt8231/vt8231_early_serial.c"

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/via/vt8601/raminit.c"
/*
  #include "lib/generic_sdram.c"
*/

static void enable_mainboard_devices(void) 
{
	device_t dev;
	/* dev 0 for southbridge */
  
	dev = pci_locate_device(PCI_ID(0x1106,0x8231), 0);
  
	if (dev == PCI_DEV_INVALID) {
		die("Southbridge not found!!!\n");
	}

	pci_write_config8(dev, 0x50, 7);
	pci_write_config8(dev, 0x51, 0xff);
#if 0
	// This early setup switches IDE into compatibility mode before PCI gets 
	// // a chance to assign I/Os
	//         movl    $CONFIG_ADDR(0, 0x89, 0x42), %eax
	//         //      movb    $0x09, %dl
	//                 movb    $0x00, %dl
	//                         PCI_WRITE_CONFIG_BYTE
	//
#endif
	/* we do this here as in V2, we can not yet do raw operations 
	 * to pci!
	 */
	/* changed this to work correctly on later revisions of LB.
	* The original dev += 0x100; stopped working. It also appears
	* that if this is not set here, but in ide_init() only, the IDE
	* does not work at all. I assume it needs to be set before something else,
	* possibly before enabling the IDE peripheral, or it is a timing issue.
	* Ben Hewson 29 Apr 2007.
	*/

	dev = pci_locate_device(PCI_ID(0x1106,0x0571), 0);
	pci_write_config8(dev, 0x42, 0);
}

static void enable_shadow_ram(void) 
{
	device_t dev = 0;
	unsigned char shadowreg;

	shadowreg = pci_read_config8(dev, 0x63);
	/* 0xf0000-0xfffff */
	shadowreg |= 0x30;
	pci_write_config8(dev, 0x63, shadowreg);
}

static void main(unsigned long bist)
{
	unsigned long x;
	
	if (bist == 0) {
		early_mtrr_init();
	}
	enable_vt8231_serial();
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);
	
	enable_mainboard_devices();
	enable_smbus();
	enable_shadow_ram();

	/*
	  this is way more generic than we need.
	  sdram_initialize(ARRAY_SIZE(cpu), cpu);
	*/
	sdram_set_registers((const struct mem_controller *) 0);
	sdram_set_spd_registers((const struct mem_controller *) 0);
	sdram_enable(0, (const struct mem_controller *) 0);
	
	/* Check all of memory */
#if 0
	ram_check(0x00000000, msr.lo);
#endif
#if 0
	static const struct {
		unsigned long lo, hi;
	} check_addrs[] = {
		/* Check 16MB of memory @ 0*/
		{ 0x00000000, 0x01000000 },
#if TOTAL_CPUS > 1
		/* Check 16MB of memory @ 2GB */
		{ 0x80000000, 0x81000000 },
#endif
	};
	int i;
	for(i = 0; i < ARRAY_SIZE(check_addrs); i++) {
		ram_check(check_addrs[i].lo, check_addrs[i].hi);
	}
#endif
}
