#define ASSEMBLY 1
#include <stdint.h>
#include <device/pci_def.h>
#include "arch/romcc_io.h"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"
#include "northbridge/amd/amdk8/early_ht.c"
#include "southbridge/amd/amd8111/amd8111_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "sdram/generic_sdram.c"

static int boot_cpu(void)
{
	volatile unsigned long *local_apic;
	unsigned long apic_id;
	int bsp;
	msr_t msr;
	msr = rdmsr(0x1b);
	bsp = !!(msr.lo & (1 << 8));
	if (bsp) {
		print_debug("Bootstrap cpu\r\n");
	}

	return bsp;
}

static int cpu_init_detected(void)
{
	unsigned long dcl;
	int cpu_init;

	unsigned long htic;

	htic = pci_read_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL);
#if 0
	print_debug("htic: ");
	print_debug_hex32(htic);
	print_debug("\r\n");

	if (!(htic & HTIC_ColdR_Detect)) {
		print_debug("Cold Reset.\r\n");
	}
	if ((htic & HTIC_ColdR_Detect) && !(htic & HTIC_BIOSR_Detect)) {
		print_debug("BIOS generated Reset.\r\n");
	}
	if (htic & HTIC_INIT_Detect) {
		print_debug("Init event.\r\n");
	}
#endif
	cpu_init = (htic & HTIC_INIT_Detect);
	if (cpu_init) {
		print_debug("CPU INIT Detected.\r\n");
	}
	return cpu_init;
}


static void print_pci_devices(void)
{
	device_t dev;
	for(dev = PCI_DEV(0, 0, 0); 
		dev <= PCI_DEV(0, 0x1f, 0x7); 
		dev += PCI_DEV(0,0,1)) {
		uint32_t id;
		id = pci_read_config32(dev, PCI_VENDOR_ID);
		if (((id & 0xffff) == 0x0000) || ((id & 0xffff) == 0xffff) ||
			(((id >> 16) & 0xffff) == 0xffff) ||
			(((id >> 16) & 0xffff) == 0x0000)) {
			continue;
		}
		print_debug("PCI: 00:");
		print_debug_hex8(dev >> 11);
		print_debug_char('.');
		print_debug_hex8((dev >> 8) & 7);
		print_debug("\r\n");
	}
}


static void dump_spd_registers(void)
{
	unsigned device;
	device = SMBUS_MEM_DEVICE_START;
	print_debug("\r\n");
	while(device <= SMBUS_MEM_DEVICE_END) {
		int i;
		print_debug("dimm: "); 
		print_debug_hex8(device); 
		for(i = 0; i < 256; i++) {
			int status;
			unsigned char byte;
			if ((i & 0xf) == 0) {
				print_debug("\r\n");
				print_debug_hex8(i);
				print_debug(": ");
			}
			status = smbus_read_byte(device, i);
			if (status < 0) {
				print_debug("bad device\r\n");
				break;
			}
			byte = status & 0xff;
			print_debug_hex8(byte);
			print_debug_char(' ');
		}
		device += SMBUS_MEM_DEVICE_INC;
		print_debug("\r\n");
	}
}




static void main(void)
{
	uart_init();
	console_init();
	if (boot_cpu() && !cpu_init_detected()) {
		setup_coherent_ht_domain();
		enumerate_ht_chain();
		print_pci_devices();
		enable_smbus();
		sdram_initialize();

		dump_spd_registers();
#if 0
		ram_fill(  0x00100000, 0x00180000);
		ram_verify(0x00100000, 0x00180000);
#endif
#ifdef MEMORY_1024MB
		ram_fill(  0x00000000, 0x00001000);
		ram_verify(0x00000000, 0x00001000);
#endif
#ifdef MEMROY_512MB
		ram_fill(  0x00000000, 0x01ffffff);
		ram_verify(0x00000000, 0x01ffffff);
#endif
	}
}
