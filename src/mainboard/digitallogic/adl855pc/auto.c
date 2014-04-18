#define ASSEMBLY 1

#include <stdint.h>
#include <device/pci_def.h>
#include <cpu/p6/apic.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"
#include "northbridge/via/vt8601/raminit.h"
#include "cpu/p6/earlymtrr.c"

/*
 */
void udelay(int usecs) 
{
	int i;
	for(i = 0; i < usecs; i++)
		outb(i&0xff, 0x80);
}

#include "lib/delay.c"
#include "cpu/p6/boot_cpu.c"
#include "debug.c"

#include "southbridge/via/vt8231/vt8231_early_serial.c"

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
        dev += 0x100; /* ICKY */

	pci_write_config8(dev, 0x42, 0);
}

static void enable_shadow_ram(void) 
{
	device_t dev = 0; /* no need to look up 0:0.0 */
	unsigned char shadowreg;
	/* dev 0 for southbridge */
	shadowreg = pci_read_config8(dev, 0x63);
	/* 0xf0000-0xfffff */
	shadowreg |= 0x30;
	pci_write_config8(dev, 0x63, shadowreg);
}

static void main(void)
{
	unsigned long x;
	/*	init_timer();*/
	outb(5, 0x80);
	
	enable_vt8231_serial();

	uart_init();
	console_init();
	
	enable_mainboard_devices();
	enable_smbus();
	enable_shadow_ram();
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
	for(i = 0; i < sizeof(check_addrs)/sizeof(check_addrs[0]); i++) {
		ram_check(check_addrs[i].lo, check_addrs[i].hi);
	}
#endif
	early_mtrr_init();
}
