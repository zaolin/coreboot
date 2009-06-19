/*
 * This file is part of the coreboot project.
 *
 * Written by Stefan Reinauer <stepan@openbios.org>.
 * ACPI FADT, FACS, and DSDT table support added by 
 *
 * Copyright (C) 2004 Stefan Reinauer <stepan@openbios.org>
 * Copyright (C) 2005 Nick Barker <nick.barker9@btinternet.com>
 * Copyright (C) 2007, 2008 Rudolf Marek <r.marek@assembler.cz>
 * Copyright (C) 2009 Harald Gutmann <harald.gutmann@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2 as published by
 * the Free Software Foundation.
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

#include <console/console.h>
#include <string.h>
#include <arch/acpi.h>
#include <arch/smp/mpspec.h>
#include <device/device.h>
#include <device/pci_ids.h>
#include <../../../northbridge/amd/amdk8/amdk8_acpi.h>
#include <cpu/amd/model_fxx_powernow.h>
#include <device/pci.h>
#include <cpu/amd/amdk8_sysconf.h>

extern unsigned char AmlCode[];

unsigned long acpi_fill_mcfg(unsigned long current)
{
	/* Not implemented */
	return current;
}

unsigned long acpi_fill_madt(unsigned long current)
{
	unsigned int gsi_base = 0x18;
	extern unsigned char bus_mcp55[8];
	extern unsigned apicid_mcp55;
	extern void get_bus_conf(void);
	unsigned sbdn;
	struct resource *res;
	device_t dev;

	get_bus_conf();
	sbdn = sysconf.sbdn;
	
	/* Create all subtables for processors. */
	current = acpi_create_madt_lapics(current);

	/* Write SB IOAPIC. */
	dev = dev_find_slot(bus_mcp55[0], PCI_DEVFN(sbdn+ 0x1,0));
	if (dev) {
		res = find_resource(dev, PCI_BASE_ADDRESS_1);
		if (res) {
			current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *) current,
				apicid_mcp55, res->base,  0);
		}
	}

	/* Write NB IOAPIC. */
	dev = dev_find_slot(bus_mcp55[0], PCI_DEVFN(sbdn+ 0x12,1));
	if (dev) {
		res = find_resource(dev, PCI_BASE_ADDRESS_1);
		if (res) {
			current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *) current,
				apicid_mcp55++, res->base,  gsi_base);
		}
	}

	/* IRQ9 ACPI active low. */
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)
		current, 0, 9, 9, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW);

	/* IRQ0 -> APIC IRQ2. */
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)
		current, 0, 0, 2, 0x0);	

	/* Create all subtables for processors. */
	current = acpi_create_madt_lapic_nmis(current,
		MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH, 1);

	return current;
}

unsigned long acpi_fill_ssdt_generator(unsigned long current, char *oem_table_id) 
{
	k8acpi_write_vars();
	amd_model_fxx_generate_powernow(0, 0, 0);
	return (unsigned long) (acpigen_get_current());
}

unsigned long write_acpi_tables(unsigned long start)
{
	unsigned long current;
	acpi_rsdp_t *rsdp;
	acpi_srat_t *srat;
	acpi_rsdt_t *rsdt;
	acpi_mcfg_t *mcfg;
	acpi_hpet_t *hpet;
	acpi_madt_t *madt;
	acpi_fadt_t *fadt;
	acpi_facs_t *facs;
	acpi_slit_t *slit;
	acpi_header_t *ssdt;
	acpi_header_t *dsdt;

	/* Align ACPI tables to 16 byte. */
	start = (start + 0x0f) & -0x10;
	current = start;

	printk_info("ACPI: Writing ACPI tables at %lx...\n", start);

	/* We need at least an RSDP and an RSDT table. */
	rsdp = (acpi_rsdp_t *) current;
	current += sizeof(acpi_rsdp_t);
	rsdt = (acpi_rsdt_t *) current;
	current += sizeof(acpi_rsdt_t);

	/* Clear all table memory. */
	memset((void *) start, 0, current - start);

	acpi_write_rsdp(rsdp, rsdt);
	acpi_write_rsdt(rsdt);

	/* We explicitly add these tables later on: */
	printk_debug("ACPI:     * FACS\n");

	/* we should align FACS to 64B as per ACPI specs */
	current = ALIGN(current, 64);
	facs = (acpi_facs_t *) current;
	current += sizeof(acpi_facs_t);
	acpi_create_facs(facs);

	dsdt = (acpi_header_t *) current;
	current += ((acpi_header_t *) AmlCode)->length;
	memcpy((void *) dsdt, (void *) AmlCode,
	       ((acpi_header_t *) AmlCode)->length);
	dsdt->checksum = 0;	/* Don't trust iasl to get this right. */
	dsdt->checksum = acpi_checksum(dsdt, dsdt->length);
	printk_debug("ACPI:     * DSDT @ %08x Length %x\n", dsdt,
		     dsdt->length);
	printk_debug("ACPI:     * FADT\n");

	fadt = (acpi_fadt_t *) current;
	current += sizeof(acpi_fadt_t);

	acpi_create_fadt(fadt, facs, dsdt);
	acpi_add_table(rsdt, fadt);

	printk_debug("ACPI:    * HPET\n");
	hpet = (acpi_hpet_t *) current;
	current += sizeof(acpi_hpet_t);
	acpi_create_hpet(hpet);
	acpi_add_table(rsdt, hpet);

	/* If we want to use HPET timers Linux wants an MADT. */
	printk_debug("ACPI:    * MADT\n");
	madt = (acpi_madt_t *) current;
	acpi_create_madt(madt);
	current += madt->header.length;
	acpi_add_table(rsdt, madt);

	printk_debug("ACPI:    * MCFG\n");
	mcfg = (acpi_mcfg_t *) current;
	acpi_create_mcfg(mcfg);
	current += mcfg->header.length;
	acpi_add_table(rsdt, mcfg);

	printk_debug("ACPI:    * SRAT\n");
	srat = (acpi_srat_t *) current;
	acpi_create_srat(srat);
	current += srat->header.length;
	acpi_add_table(rsdt, srat);

	/* SLIT */
	printk_debug("ACPI:    * SLIT\n");
	slit = (acpi_slit_t *) current;
	acpi_create_slit(slit);
	current+=slit->header.length;
	acpi_add_table(rsdt,slit);

	/* SSDT */
	printk_debug("ACPI:    * SSDT\n");
	ssdt = (acpi_header_t *)current;

	acpi_create_ssdt_generator(ssdt, "DYNADATA");
	current += ssdt->length;
	acpi_add_table(rsdt, ssdt);

	printk_info("ACPI: done.\n");
	return current;
}
