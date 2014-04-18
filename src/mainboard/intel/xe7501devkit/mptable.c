#include <console/console.h>
#include <arch/smp/mpspec.h>
#include <device/pci.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "bus.h"
#include "ioapic.h"

// Generate MP-table IRQ numbers for PCI devices.
#define INT_A	0
#define INT_B	1
#define INT_C	2
#define INT_D	3
#define PCI_IRQ(dev, intLine)	(((dev)<<2) | intLine)


void xe7501devkit_register_buses(struct mp_config_table *mc)
{
	// Bus ID, Bus Type
	smp_write_bus(mc, PCI_BUS_CHIPSET,		BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_E7501_HI_B,	BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_P64H2_2_B,	BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_P64H2_2_A,	BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_E7501_HI_D,	BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_P64H2_1_B,	BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_P64H2_1_A,	BUSTYPE_PCI);
	smp_write_bus(mc, PCI_BUS_ICH3,			BUSTYPE_PCI);
	smp_write_bus(mc, SUPERIO_BUS,			BUSTYPE_ISA);
}

void xe7501devkit_register_ioapics(struct mp_config_table *mc)
{
	device_t dev;
    struct resource *res;

	// TODO: Gack. This is REALLY ugly.

	// Southbridge IOAPIC
	smp_write_ioapic(mc, IOAPIC_ICH3, 0x20, 0xfec00000);	// APIC ID, Version, Address

	// P64H2#2 Bus A IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_B, PCI_DEVFN(30, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	smp_write_ioapic(mc, IOAPIC_P64H2_2_BUS_A, P64H2_IOAPIC_VERSION, res->base);

	// P64H2#2 Bus B IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_B, PCI_DEVFN(28, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	smp_write_ioapic(mc, IOAPIC_P64H2_2_BUS_B, P64H2_IOAPIC_VERSION, res->base);


	// P64H2#1 Bus A IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_D, PCI_DEVFN(30, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	smp_write_ioapic(mc, IOAPIC_P64H2_1_BUS_A, P64H2_IOAPIC_VERSION, res->base);

	// P64H2#1 Bus B IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_D, PCI_DEVFN(28, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	smp_write_ioapic(mc, IOAPIC_P64H2_1_BUS_B, P64H2_IOAPIC_VERSION, res->base);
}

void xe7501devkit_register_interrupts(struct mp_config_table *mc)
{
	// Chipset PCI bus
	//					 Type		Trigger | Polarity							Bus ID				IRQ					APIC ID					PIN#
	smp_write_intsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	PCI_BUS_CHIPSET,	0,					MP_APIC_ALL,			 0);
	smp_write_intsrc(mc, mp_NMI,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	PCI_BUS_CHIPSET,	0,					MP_APIC_ALL,			 1);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_CHIPSET, 	PCI_IRQ(29, INT_A), IOAPIC_ICH3, 			16);	// USB 1.1 Controller #1
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_CHIPSET,	PCI_IRQ(31, INT_B),	IOAPIC_ICH3,			17);	// SMBus
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_CHIPSET, 	PCI_IRQ(29, INT_C), IOAPIC_ICH3, 			18);	// USB 1.1 Controller #3
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_CHIPSET, 	PCI_IRQ(31, INT_C), IOAPIC_ICH3, 			18);	// IDE
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_CHIPSET, 	PCI_IRQ(29, INT_D), IOAPIC_ICH3, 			19);	// USB 1.1 Controller #2

	// P64H2#2 Bus B
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(1, INT_A), 	IOAPIC_P64H2_2_BUS_B,    0);	// Slot 2A (J23)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(1, INT_B), 	IOAPIC_P64H2_2_BUS_B,    1);	// Slot 2A (J23)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(1, INT_C), 	IOAPIC_P64H2_2_BUS_B,    2);	// Slot 2A (J23)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(1, INT_D), 	IOAPIC_P64H2_2_BUS_B,    3);	// Slot 2A (J23)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(2, INT_A), 	IOAPIC_P64H2_2_BUS_B,    4);	// Slot 2B (J24)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(2, INT_B), 	IOAPIC_P64H2_2_BUS_B,    5);	// Slot 2B (J24)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(2, INT_C), 	IOAPIC_P64H2_2_BUS_B,    6);	// Slot 2B (J24)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(2, INT_D), 	IOAPIC_P64H2_2_BUS_B,    7);	// Slot 2B (J24)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(3, INT_A), 	IOAPIC_P64H2_2_BUS_B,    8);	// Slot 2C (J25)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(3, INT_B), 	IOAPIC_P64H2_2_BUS_B,    9);	// Slot 2C (J25)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(3, INT_C), 	IOAPIC_P64H2_2_BUS_B,   10);	// Slot 2C (J25)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(3, INT_D), 	IOAPIC_P64H2_2_BUS_B,   11);	// Slot 2C (J25)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(4, INT_A), 	IOAPIC_P64H2_2_BUS_B,   12);	// Slot 2D (J12)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(4, INT_B), 	IOAPIC_P64H2_2_BUS_B,   13);	// Slot 2D (J12)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(4, INT_C), 	IOAPIC_P64H2_2_BUS_B,   14);	// Slot 2D (J12)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_2_B, 	PCI_IRQ(4, INT_D), 	IOAPIC_P64H2_2_BUS_B,   15);	// Slot 2D (J12)
	
	// P64H2#2 Bus A
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_P64H2_2_A, 	PCI_IRQ(1, INT_A),	IOAPIC_P64H2_2_BUS_A,	 0);	// SCSI
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_P64H2_2_A, 	PCI_IRQ(1, INT_B),	IOAPIC_P64H2_2_BUS_A,	 1);	// SCSI
	
	// P64H2#1 Bus B
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_B, 	PCI_IRQ(1, INT_A), 	IOAPIC_P64H2_1_BUS_B,    0);	// GB Ethernet
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_B, 	PCI_IRQ(2, INT_A), 	IOAPIC_P64H2_1_BUS_B,    4);	// Slot 1B (J21)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_B, 	PCI_IRQ(2, INT_B), 	IOAPIC_P64H2_1_BUS_B,    5);	// Slot 1B (J21)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_B, 	PCI_IRQ(2, INT_C), 	IOAPIC_P64H2_1_BUS_B,    6);	// Slot 1B (J21)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_B, 	PCI_IRQ(2, INT_D), 	IOAPIC_P64H2_1_BUS_B,    7);	// Slot 1B (J21)

	// P64H2#1 Bus A
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_A, 	PCI_IRQ(1, INT_A), 	IOAPIC_P64H2_1_BUS_A,    0);	// Slot 1A (J20)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_A, 	PCI_IRQ(1, INT_B), 	IOAPIC_P64H2_1_BUS_A,    1);	// Slot 1A (J20)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_A, 	PCI_IRQ(1, INT_C), 	IOAPIC_P64H2_1_BUS_A,    2);	// Slot 1A (J20)
	smp_write_intsrc(mc, mp_INT, 	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 	PCI_BUS_P64H2_1_A, 	PCI_IRQ(1, INT_D), 	IOAPIC_P64H2_1_BUS_A,    3);	// Slot 1A (J20)

	// ICH-3
	
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_ICH3,		PCI_IRQ(0, INT_A),	IOAPIC_ICH3,			16);	// Video
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_ICH3,		PCI_IRQ(2, INT_A),	IOAPIC_ICH3,			18);	// Debug slot (J11)
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_ICH3,		PCI_IRQ(2, INT_B),	IOAPIC_ICH3,			19);	// Debug slot (J11)
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_ICH3,		PCI_IRQ(2, INT_C),	IOAPIC_ICH3,			16);	// Debug slot (J11)
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,	PCI_BUS_ICH3,		PCI_IRQ(2, INT_D),	IOAPIC_ICH3,			17);	// Debug slot (J11)
	
	// TODO: Not sure how to handle BT_INTR# signals from the P64H2s. Do we even need to, in APIC mode?

	// Super I/O (ISA interrupts)
	smp_write_intsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 0,					IOAPIC_ICH3,			 0);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 1,					IOAPIC_ICH3,			 1);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 0,					IOAPIC_ICH3,			 2);

	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 3,					IOAPIC_ICH3,			 3);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 4,					IOAPIC_ICH3,			 4);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 6,					IOAPIC_ICH3,			 6);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 8,					IOAPIC_ICH3,			 8);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		 9,					IOAPIC_ICH3,			 9);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		12,					IOAPIC_ICH3,			12);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		13,					IOAPIC_ICH3,			13);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		14,					IOAPIC_ICH3,			14);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_EDGE |MP_IRQ_POLARITY_HIGH,	SUPERIO_BUS,		15,					IOAPIC_ICH3,			15);
}

void* smp_write_config_table(void* v)
{
	static const char sig[4] = MPC_SIGNATURE;
    static const char oem[8] = "INTEL   ";
    static const char productid[12] = "XE7501DEVKIT";
    struct mp_config_table *mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);
	memset(mc, 0, sizeof(*mc));

    memcpy(mc->mpc_signature, sig, sizeof(sig));
    memcpy(mc->mpc_oem, oem, sizeof(oem));
    memcpy(mc->mpc_productid, productid, sizeof(productid));

    mc->mpc_length = sizeof(*mc);					// initially just the header
    mc->mpc_spec = 0x04;							// Multiprocessing Spec V1.4
    mc->mpc_lapic = LAPIC_ADDR;

	smp_write_processors(mc);

	xe7501devkit_register_buses(mc);
	xe7501devkit_register_ioapics(mc);
	xe7501devkit_register_interrupts(mc);

	/* Compute the checksums */
	mc->mpe_checksum = smp_compute_checksum(smp_next_mpc_entry(mc), mc->mpe_length);
	mc->mpc_checksum = smp_compute_checksum(mc, mc->mpc_length);
	printk_debug("Wrote the mp table end at: %p - %p\n", mc, smp_next_mpe_entry(mc));

	return smp_next_mpe_entry(mc);
}

unsigned long write_smp_table(unsigned long addr)
{
	void *v;
	v = smp_write_floating_table(addr);
	return (unsigned long)smp_write_config_table(v);
}
