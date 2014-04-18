/*
 * LinuxBIOS ACPI Table support
 * written by Stefan Reinauer <stepan@openbios.org>
 * ACPI FADT, FACS, and DSDT table support added by 
 * Nick Barker <nick.barker9@btinternet.com>, and those portions
 * (C) Copyright 2004 Nick Barker
 * (C) Copyright 2005 Stefan Reinauer
 */

#include <console/console.h>
#include <string.h>
#include <arch/acpi.h>

extern unsigned char AmlCode[];

unsigned long acpi_dump_apics(unsigned long current)
{
	/* Nothing to do */
}

unsigned long write_acpi_tables(unsigned long start)
{
	unsigned long current;
	acpi_rsdp_t *rsdp;
	acpi_rsdt_t *rsdt;
	acpi_hpet_t *hpet;
	acpi_madt_t *madt;
	acpi_fadt_t *fadt;
	acpi_facs_t *facs;
	acpi_header_t *dsdt;
	
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
	printk_debug("ACPI:     * FACS\n");
	facs = (acpi_facs_t *) current;
	current += sizeof(acpi_facs_t);
	acpi_create_facs(facs);

	dsdt = (acpi_header_t *)current;
	current += ((acpi_header_t *)AmlCode)->length;
	memcpy((void *)dsdt,(void *)AmlCode, ((acpi_header_t *)AmlCode)->length);
	dsdt->checksum = 0; // don't trust intel iasl compiler to get this right
	dsdt->checksum = acpi_checksum(dsdt,dsdt->length);
	printk_debug("ACPI:     * DSDT @ %08x Length %x\n",dsdt,dsdt->length);
	printk_debug("ACPI:     * FADT\n");

	fadt = (acpi_fadt_t *) current;
	current += sizeof(acpi_fadt_t);

	acpi_create_fadt(fadt,facs,dsdt);
	acpi_add_table(rsdt,fadt);

	printk_info("ACPI: done.\n");
	return current;
}

