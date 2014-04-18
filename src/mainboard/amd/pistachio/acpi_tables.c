/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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
#include <device/pci.h>
#include <device/pci_ids.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/amdk8_sysconf.h>
#include <../../../northbridge/amd/amdk8/amdk8_acpi.h>
#include <arch/cpu.h>
#include <cpu/amd/model_fxx_powernow.h>

extern u16 pm_base;

#define DUMP_ACPI_TABLES 0

/*
* Assume the max pstate number is 8
* 0x21(33 bytes) is one package length of _PSS package
*/

#define Maxpstate 8
#define Defpkglength 0x21

#if DUMP_ACPI_TABLES == 1
static void dump_mem(u32 start, u32 end)
{

	u32 i;
	print_debug("dump_mem:");
	for (i = start; i < end; i++) {
		if ((i & 0xf) == 0) {
			printk_debug("\n%08x:", i);
		}
		printk_debug(" %02x", (u8)*((u8 *)i));
	}
	print_debug("\n");
}
#endif

extern u8 AmlCode[];

#if CONFIG_ACPI_SSDTX_NUM >= 1
extern u8 AmlCode_ssdt2[];
extern u8 AmlCode_ssdt3[];
extern u8 AmlCode_ssdt4[];
extern u8 AmlCode_ssdt5[];
#endif

#define IO_APIC_ADDR	0xfec00000UL

unsigned long acpi_fill_mcfg(unsigned long current)
{
	/* Just a dummy */
	return current;
}

unsigned long acpi_fill_madt(unsigned long current)
{
	/* create all subtables for processors */
	current = acpi_create_madt_lapics(current);

	/* Write SB600 IOAPIC, only one */
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *) current, 2,
					   IO_APIC_ADDR, 0);

	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)
						current, 0, 0, 2, 0);
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)
						current, 0, 9, 9, 0xF);
	/* 0: mean bus 0--->ISA */
	/* 0: PIC 0 */
	/* 2: APIC 2 */
	/* 5 mean: 0101 --> Edige-triggered, Active high */

	/* create all subtables for processors */
	/* current = acpi_create_madt_lapic_nmis(current, 5, 1); */
	/* 1: LINT1 connect to NMI */

	return current;
}

extern void get_bus_conf(void);

static void update_ssdtx(void *ssdtx, int i)
{
	uint8_t *PCI;
	uint8_t *HCIN;
	uint8_t *UID;

	PCI = ssdtx + 0x32;
	HCIN = ssdtx + 0x39;
	UID = ssdtx + 0x40;

	if (i < 7) {
		*PCI = (uint8_t) ('4' + i - 1);
	} else {
		*PCI = (uint8_t) ('A' + i - 1 - 6);
	}
	*HCIN = (uint8_t) i;
	*UID = (uint8_t) (i + 3);

	/* FIXME: need to update the GSI id in the ssdtx too */

}

unsigned long acpi_fill_ssdt_generator(unsigned long current, const char *oem_table_id) {
	k8acpi_write_vars();
	amd_model_fxx_generate_powernow(pm_base + 8, 6, 1);
	return (unsigned long) (acpigen_get_current());
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
	acpi_header_t *ssdt;

	get_bus_conf();		/* it will get sblk, pci1234, hcdn, and sbdn */

	/* Align ACPI tables to 16byte */
	start = (start + 0x0f) & -0x10;
	current = start;

	printk_info("ACPI: Writing ACPI tables at %lx...\n", start);

	/* We need at least an RSDP and an RSDT Table */
	rsdp = (acpi_rsdp_t *) current;
	current += sizeof(acpi_rsdp_t);
	rsdt = (acpi_rsdt_t *) current;
	current += sizeof(acpi_rsdt_t);

	/* clear all table memory */
	memset((void *)start, 0, current - start);

	acpi_write_rsdp(rsdp, rsdt, NULL);
	acpi_write_rsdt(rsdt);

	/*
	 * We explicitly add these tables later on:
	 */
	/* If we want to use HPET Timers Linux wants an MADT */
	printk_debug("ACPI:    * HPET\n");
	hpet = (acpi_hpet_t *) current;
	current += sizeof(acpi_hpet_t);
	acpi_create_hpet(hpet);
	acpi_add_table(rsdp, hpet);

	printk_debug("ACPI:    * MADT\n");
	madt = (acpi_madt_t *) current;
	acpi_create_madt(madt);
	current += madt->header.length;
	acpi_add_table(rsdp, madt);

#if 0
	/* SRAT */
	printk_debug("ACPI:    * SRAT\n");
	srat = (acpi_srat_t *) current;
	acpi_create_srat(srat);
	current += srat->header.length;
	acpi_add_table(rsdp, srat);

	/* SLIT */
	printk_debug("ACPI:    * SLIT\n");
	slit = (acpi_slit_t *) current;
	acpi_create_slit(slit);
	current += slit->header.length;
	acpi_add_table(rsdp, slit);
#endif

	/* SSDT */
	printk_debug("ACPI:    * SSDT\n");
	ssdt = (acpi_header_t *)current;

	acpi_create_ssdt_generator(ssdt, "DYNADATA");
	current += ssdt->length;
	acpi_add_table(rsdp, ssdt);

#if CONFIG_ACPI_SSDTX_NUM >= 1

	/* same htio, but different position? We may have to copy, change HCIN, and recalculate the checknum and add_table */

	for (i = 1; i < sysconf.hc_possible_num; i++) {	/* 0: is hc sblink */
		if ((sysconf.pci1234[i] & 1) != 1)
			continue;
		uint8_t c;
		if (i < 7) {
			c = (uint8_t) ('4' + i - 1);
		} else {
			c = (uint8_t) ('A' + i - 1 - 6);
		}
		printk_debug("ACPI:    * SSDT for PCI%c Aka hcid = %d\n", c, sysconf.hcid[i]);	/* pci0 and pci1 are in dsdt */
		current = (current + 0x07) & -0x08;
		ssdtx = (acpi_header_t *) current;
		switch (sysconf.hcid[i]) {
		case 1:	/* 8132 */
			p = AmlCode_ssdt2;
			break;
		case 2:	/* 8151 */
			p = AmlCode_ssdt3;
			break;
		case 3:	/* 8131 */
			p = AmlCode_ssdt4;
			break;
		default:
			/* HTX no io apic */
			p = AmlCode_ssdt5;
			break;
		}
		current += ((acpi_header_t *) p)->length;
		memcpy((void *)ssdtx, (void *)p, ((acpi_header_t *) p)->length);
		update_ssdtx((void *)ssdtx, i);
		ssdtx->checksum = 0;
		ssdtx->checksum =
		    acpi_checksum((u8 *)ssdtx, ssdtx->length);
		acpi_add_table(rsdp, ssdtx);
	}
#endif

	/* FACS */
	printk_debug("ACPI:    * FACS\n");
	facs = (acpi_facs_t *) current;
	current += sizeof(acpi_facs_t);
	acpi_create_facs(facs);

	/* DSDT */
	printk_debug("ACPI:    * DSDT\n");
	dsdt = (acpi_header_t *) current;
	memcpy((void *)dsdt, (void *)AmlCode,
	       ((acpi_header_t *) AmlCode)->length);

	current += dsdt->length;
	printk_debug("ACPI:    * DSDT @ %p Length %x\n", dsdt, dsdt->length);
	/* FADT */
	printk_debug("ACPI:    * FADT\n");
	fadt = (acpi_fadt_t *) current;
	current += sizeof(acpi_fadt_t);

	acpi_create_fadt(fadt, facs, dsdt);
	acpi_add_table(rsdp, fadt);

#if DUMP_ACPI_TABLES == 1
	printk_debug("rsdp\n");
	dump_mem(rsdp, ((void *)rsdp) + sizeof(acpi_rsdp_t));

	printk_debug("rsdt\n");
	dump_mem(rsdt, ((void *)rsdt) + sizeof(acpi_rsdt_t));

	printk_debug("madt\n");
	dump_mem(madt, ((void *)madt) + madt->header.length);

	printk_debug("srat\n");
	dump_mem(srat, ((void *)srat) + srat->header.length);

	printk_debug("slit\n");
	dump_mem(slit, ((void *)slit) + slit->header.length);

	printk_debug("ssdt\n");
	dump_mem(ssdt, ((void *)ssdt) + ssdt->length);

	printk_debug("fadt\n");
	dump_mem(fadt, ((void *)fadt) + fadt->header.length);
#endif

	printk_info("ACPI: done.\n");
	return current;
}
