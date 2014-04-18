#define ASSEMBLY 1
#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include <stdlib.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "southbridge/intel/i82801er/i82801er_early_smbus.c"
#include "northbridge/intel/e7520/raminit.h"
#include "superio/nsc/pc8374/pc8374_early_serial.c"
#include "cpu/x86/lapic/boot_cpu.c"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "debug.c"
#include "watchdog.c"
#include "reset.c"
#include "s1850_fixups.c"
#include "northbridge/intel/e7520/memory_initialized.c"
#include "cpu/x86/bist.h"


#define SIO_GPIO_BASE 0x680
#define SIO_XBUS_BASE 0x4880

#define CONSOLE_SERIAL_DEV PNP_DEV(0x2e, PC8374_SP1)

#define DEVPRES_CONFIG  ( \
	DEVPRES_D0F0 | \
	DEVPRES_D1F0 | \
	DEVPRES_D2F0 | \
	DEVPRES_D3F0 | \
	DEVPRES_D4F0 | \
	DEVPRES_D6F0 | \
	0 )
#define DEVPRES1_CONFIG (DEVPRES1_D0F1 | DEVPRES1_D8F0)

#define RECVENA_CONFIG  0x0808090a
#define RECVENB_CONFIG  0x0808090a

//void udelay(int usecs)
//{
//        int i;
//        for(i = 0; i < usecs; i++)
//                outb(i&0xff, 0x80);
//}

#if 0
static void hard_reset(void)
{
	/* enable cf9 */
	pci_write_config8(PCI_DEV(0, 0x04, 3), 0x41, 0xf1);
	/* reset */
	outb(0x0e, 0x0cf9);
}
#endif

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}
static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/intel/e7520/raminit.c"
#include "lib/generic_sdram.c"


/* IPMI garbage. This is all test stuff, if it really works we'll move it somewhere
 */

#define nftransport  0xc

#define OBF  0
#define IBF 1

#define ipmidata  0xca0
#define ipmicsr  0xca4


static inline void  ibfzero(void)
{
	while(inb(ipmicsr) &  (1<<IBF)) 
		;
}
static inline void  clearobf(void)
{
	(void) inb(ipmidata);
}

static inline void  waitobf(void)
{
	while((inb(ipmicsr) &  (1<<OBF)) == 0) 
		;
}
/* quite possibly the stupidest interface ever designed. */
static inline void  first_cmd_byte(unsigned char byte)
{
	ibfzero();
	clearobf();
	outb(0x61, ipmicsr);
	ibfzero();
	clearobf();
	outb(byte, ipmidata);
}

static inline void  next_cmd_byte(unsigned char byte)
{

	ibfzero();
	clearobf();
	outb(byte, ipmidata);
}

static inline void  last_cmd_byte(unsigned char byte)
{
	outb(0x62, ipmicsr);

	ibfzero();
	clearobf();
	outb(byte,  ipmidata);
}

static inline void read_response_byte(void)
{
	int val = -1;
	if ((inb(ipmicsr)>>6) != 1)
		return;

	ibfzero();
	waitobf();
	val = inb(ipmidata);
	outb(0x68, ipmidata);

	/* see if it is done */
	if ((inb(ipmicsr)>>6) != 1){
		/* wait for the dummy read. Which describes this protocol */
		waitobf();
		(void)inb(ipmidata);
	}
}

static inline void ipmidelay(void)
{
	int i;
	for(i = 0; i < 1000; i++) {
		inb(0x80);
	}
}

static inline void bmc_foad(void)
{
	unsigned char c;
	/* be safe; make sure it is really ready */
	while ((inb(ipmicsr)>>6)) {
		outb(0x60, ipmicsr);
		inb(ipmidata);
	}
	first_cmd_byte(nftransport << 2);
	ipmidelay();
	next_cmd_byte(0x12);
	ipmidelay();
	next_cmd_byte(2);
	ipmidelay();
	last_cmd_byte(3);
	ipmidelay();
}

/* end IPMI garbage */
static void main(unsigned long bist)
{
	/*
	 * 
	 * 
	 */
	static const struct mem_controller mch[] = {
		{
			.node_id = 0,
			.f0 = PCI_DEV(0, 0x00, 0),
			.f1 = PCI_DEV(0, 0x00, 1),
			.f2 = PCI_DEV(0, 0x00, 2),
			.f3 = PCI_DEV(0, 0x00, 3),
			.channel0 = {(0xa<<3)|3, (0xa<<3)|2, (0xa<<3)|1, (0xa<<3)|0, },
			.channel1 = {(0xa<<3)|7, (0xa<<3)|6, (0xa<<3)|5, (0xa<<3)|4, },
		}
	};

	if (bist == 0) {
		/* Skip this if there was a built in self test failure */
		early_mtrr_init();
		if (memory_initialized()) {
			asm volatile ("jmp __cpu_reset");
		}
	}
	/* Setup the console */
	mainboard_set_ich5();
	bmc_foad();
	pc8374_enable_serial(CONSOLE_SERIAL_DEV, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure */
//	report_bist_failure(bist);

	/* MOVE ME TO A BETTER LOCATION !!! */
	/* config LPC decode for flash memory access */
        device_t dev;
        dev = pci_locate_device(PCI_ID(0x8086, 0x24d0), 0);
        if (dev == PCI_DEV_INVALID) {
                die("Missing ich5?");
        }
        pci_write_config32(dev, 0xe8, 0x00000000);
        pci_write_config8(dev, 0xf0, 0x00);

#if 0
	display_cpuid_update_microcode();
#endif
#if 1
	print_pci_devices();
#endif
#if 1
	enable_smbus();
#endif
#if 1
//	dump_spd_registers(&cpu[0]);
	int i;
	for(i = 0; i < 1; i++) {
		dump_spd_registers();
	}
#endif
	disable_watchdogs();
//	dump_ipmi_registers();
	mainboard_set_e7520_leds();	
//	memreset_setup();
	sdram_initialize(ARRAY_SIZE(mch), mch);
#if 1
	dump_pci_devices();
#endif
#if 1
	dump_pci_device(PCI_DEV(0, 0x00, 0));
	dump_bar14(PCI_DEV(0, 0x00, 0));
#endif

#if 1 // temporarily disabled 
	/* Check the first 1M */
//	ram_check(0x00000000, 0x000100000);
//	ram_check(0x00000000, 0x000a0000);
//	ram_check(0x00100000, 0x01000000);
	ram_check(0x00100000, 0x00100100);
	/* check the first 1M in the 3rd Gig */
//	ram_check(0x30100000, 0x31000000);
#endif
#if 0
	ram_check(0x00000000, 0x02000000);
#endif
	
#if 0	
	while(1) {
		hlt();
	}
#endif
}
