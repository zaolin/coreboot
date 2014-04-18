/*
 * Ported to Intel XE7501DEVKIT from Island Aruma
 * written by Stefan Reinauer <stepan@openbios.org>
 *  (C) 2005 Stefan Reinauer
 *  (C) 2005 Digital Design Corporation
 */

#include <console/console.h>
#include <string.h>
#include <arch/acpi.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <assert.h>
#include "bus.h"
#include "ioapic.h"

unsigned long acpi_dump_apics(unsigned long current)
{
	unsigned int irq_start = 0;
	device_t dev = 0;
    struct resource* res = NULL;

 
	// SJM: Hard-code CPU LAPIC entries for now
	//		Use SourcePoint numbering of processors
	current += acpi_create_madt_lapic((acpi_madt_lapic_t *)current, 0, 6);
	current += acpi_create_madt_lapic((acpi_madt_lapic_t *)current, 1, 7);
	current += acpi_create_madt_lapic((acpi_madt_lapic_t *)current, 2, 0);
	current += acpi_create_madt_lapic((acpi_madt_lapic_t *)current, 3, 1);
	

	// Southbridge IOAPIC
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, IOAPIC_ICH3, 0xfec00000, irq_start);
	irq_start += INTEL_IOAPIC_NUM_INTERRUPTS;

	// P64H2#2 Bus A IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_B, PCI_DEVFN(30, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, IOAPIC_P64H2_2_BUS_A, res->base, irq_start);
	irq_start += INTEL_IOAPIC_NUM_INTERRUPTS;

	// P64H2#2 Bus B IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_B, PCI_DEVFN(28, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, IOAPIC_P64H2_2_BUS_B, res->base, irq_start);
	irq_start += INTEL_IOAPIC_NUM_INTERRUPTS;


	// P64H2#1 Bus A IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_D, PCI_DEVFN(30, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, IOAPIC_P64H2_1_BUS_A, res->base, irq_start);
	irq_start += INTEL_IOAPIC_NUM_INTERRUPTS;

	// P64H2#1 Bus B IOAPIC
	dev = dev_find_slot(PCI_BUS_E7501_HI_D, PCI_DEVFN(28, 0));	
	if (!dev)
		BUG();		// Config.lb error?
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, IOAPIC_P64H2_1_BUS_B, res->base, irq_start);
	irq_start += INTEL_IOAPIC_NUM_INTERRUPTS;

	// Map ISA IRQ 0 to IRQ 2
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)current, 1, 0, 2, 0);

	// IRQ9 differs from ISA standard - ours is active high, level-triggered
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)current, 0, 9, 9, 0xD);

	return current;
}


unsigned long write_acpi_tables(unsigned long start)
{
	unsigned long current;
	acpi_rsdp_t *rsdp;
	acpi_rsdt_t *rsdt;
	acpi_madt_t *madt;

	/* Align ACPI tables to 16byte */
	start   = ( start + 0x0f ) & -0x10;
	current = start;
	
	printk_info("ACPI: Writing ACPI tables at %lx...\n", start);

	/* We need at least an RSDP and an RSDT Table */
	rsdp = (acpi_rsdp_t *) current;
	current += sizeof(acpi_rsdp_t);
	rsdt = (acpi_rsdt_t *) current;
	current += sizeof(acpi_rsdt_t);

	/* clear all table memory */
	memset((void *)start, 0, current - start);
	
	acpi_write_rsdp(rsdp, rsdt);
	acpi_write_rsdt(rsdt);
	
	/*
	 * We explicitly add these tables later on:
	 */
	/* QNX wants an MADT */
	printk_debug("ACPI:    * MADT\n");

	madt = (acpi_madt_t *) current;
	acpi_create_madt(madt);
	current+=madt->header.length;
	acpi_add_table(rsdt,madt);

	printk_info("ACPI: done.\n");
	return current;
}

