#define ASSEMBLY 1

#include <stdint.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"

/*
 */
void udelay(int usecs) 
{
	int i;
	for(i = 0; i < usecs; i++)
		outb(i&0xff, 0x80);
}

#include "lib/delay.c"
#include "cpu/x86/lapic/boot_cpu.c"
#include "debug.c"

static void main(void)
{
	/*	init_timer();*/
	outb(5, 0x80);
	
	uart_init();
	console_init();
	
	//print_pci_devices();
	//dump_pci_devices();
}
