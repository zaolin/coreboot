/*
 * Island Aruma ACPI support
 * written by Stefan Reinauer <stepan@openbios.org>
 *  (C) 2005 Stefan Reinauer
 *
 *
 *  Copyright 2005 AMD
 *  2005.9 yhlu modify that to more dynamic for AMD Opteron Based MB
 */

#include <console/console.h>
#include <string.h>
#include <arch/acpi.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/amdk8_sysconf.h>

#include "mb_sysconf.h"

#define DUMP_ACPI_TABLES 0

#if DUMP_ACPI_TABLES == 1
static void dump_mem(unsigned start, unsigned end)
{
        
	unsigned i;
        print_debug("dump_mem:");
        for(i=start;i<end;i++) {
                if((i & 0xf)==0) {
                        printk_debug("\n%08x:", i);
                }
                printk_debug(" %02x", (unsigned char)*((unsigned char *)i));
        }
        print_debug("\n");
 }
#endif

extern unsigned char AmlCode[];
extern unsigned char AmlCode_ssdt[];

#if ACPI_SSDTX_NUM >= 1
extern unsigned char AmlCode_ssdt2[];
extern unsigned char AmlCode_ssdt3[];
extern unsigned char AmlCode_ssdt4[];
extern unsigned char AmlCode_ssdt5[];
#endif

#define IO_APIC_ADDR	0xfec00000UL

unsigned long acpi_fill_madt(unsigned long current)
{
	unsigned int gsi_base=0x18;

        struct mb_sysconf_t *m;

        m = sysconf.mb;
 
	/* create all subtables for processors */
	current = acpi_create_madt_lapics(current);
	
	/* Write 8111 IOAPIC */
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, m->apicid_8111,
			IO_APIC_ADDR, 0);

        /* Write all 8131 IOAPICs */
        {
                device_t dev;
                struct resource *res;
                dev = dev_find_slot(m->bus_8132_0, PCI_DEVFN((sysconf.hcdn[0]&0xff), 1));
                if (dev) {
                        res = find_resource(dev, PCI_BASE_ADDRESS_0);
                        if (res) {
			        current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, m->apicid_8132_1,
		                        res->base, gsi_base );
				gsi_base+=4;

                        }
                }
                dev = dev_find_slot(m->bus_8132_0, PCI_DEVFN((sysconf.hcdn[0] & 0xff)+1, 1));
                if (dev) {
                        res = find_resource(dev, PCI_BASE_ADDRESS_0);
                        if (res) {
                                current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, m->apicid_8132_2,
                                        res->base, gsi_base );
                                gsi_base+=4;
                        }
                }

                int i;
                int j = 0;

                for(i=1; i< sysconf.hc_possible_num; i++) {
			unsigned d;
                        if(!(sysconf.pci1234[i] & 0x1) ) continue;
                        // 8131 need to use +4
			
                        switch (sysconf.hcid[i]) {
                        case 1:
				d = 7;
				break;
			case 3:
				d = 4;
				break;
			}
                        switch (sysconf.hcid[i]) {
                        case 1:
			case 3:
                                dev = dev_find_slot(m->bus_8132a[j][0], PCI_DEVFN(m->sbdn3a[j], 1));
                                if (dev) {
                                        res = find_resource(dev, PCI_BASE_ADDRESS_0);
                                        if (res) {
                                                current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, m->apicid_8132a[j][0],
                                                        res->base, gsi_base );
                                                gsi_base+=d;
                                        }
                                }
                                dev = dev_find_slot(m->bus_8132a[j][0], PCI_DEVFN(m->sbdn3a[j]+1, 1));
                                if (dev) {
                                        res = find_resource(dev, PCI_BASE_ADDRESS_0);
                                        if (res) {
                                                current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *)current, m->apicid_8132a[j][1],
                                                        res->base, gsi_base );
                                                gsi_base+=d;

                                        }
                                }
                                break;
                        }

                        j++;
                }

        }

	current += acpi_create_madt_irqoverride( (acpi_madt_irqoverride_t *)
			current, 0, 0, 2, 5 );
		/* 0: mean bus 0--->ISA */
		/* 0: PIC 0 */
		/* 2: APIC 2 */ 
		/* 5 mean: 0101 --> Edige-triggered, Active high*/


	        /* create all subtables for processors */
        current = acpi_create_madt_lapic_nmis(current, 5, 1);
		/* 1: LINT1 connect to NMI */


	return current;
}

extern void get_bus_conf(void);

extern void update_ssdt(void *ssdt);

void update_ssdtx(void *ssdtx, int i)
{
        uint8_t *PCI;
        uint8_t *HCIN;
        uint8_t *UID;

        PCI = ssdtx + 0x32;
        HCIN = ssdtx + 0x39;
        UID = ssdtx + 0x40;

        if(i<7) {
                *PCI  = (uint8_t) ('4' + i - 1);
        }
        else {
                *PCI  = (uint8_t) ('A' + i - 1 - 6);
        }
        *HCIN = (uint8_t) i;
        *UID  = (uint8_t) (i+3);

        /* FIXME: need to update the GSI id in the ssdtx too */

}

unsigned long write_acpi_tables(unsigned long start)
{
	unsigned long current;
	acpi_rsdp_t *rsdp;
	acpi_rsdt_t *rsdt;
	acpi_hpet_t *hpet;
	acpi_madt_t *madt;
	acpi_srat_t *srat;
	acpi_slit_t *slit;
	acpi_fadt_t *fadt;
	acpi_facs_t *facs;
	acpi_header_t *dsdt;
	acpi_header_t *ssdt;
	acpi_header_t *ssdtx;
	unsigned char *p;

	unsigned char *AmlCode_ssdtx[HC_POSSIBLE_NUM];

	int i;

	get_bus_conf(); //it will get sblk, pci1234, hcdn, and sbdn

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
	printk_debug("ACPI:    * HPET\n");
	hpet = (acpi_hpet_t *) current;
	current += sizeof(acpi_hpet_t);
	acpi_create_hpet(hpet);
	acpi_add_table(rsdt,hpet);

	/* If we want to use HPET Timers Linux wants an MADT */
	printk_debug("ACPI:    * MADT\n");
	madt = (acpi_madt_t *) current;
	acpi_create_madt(madt);
	current+=madt->header.length;
	acpi_add_table(rsdt,madt);


	/* SRAT */
        printk_debug("ACPI:    * SRAT\n");
        srat = (acpi_srat_t *) current;
        acpi_create_srat(srat);
        current+=srat->header.length;
        acpi_add_table(rsdt,srat);

	/* SLIT */
        printk_debug("ACPI:    * SLIT\n");
        slit = (acpi_slit_t *) current;
        acpi_create_slit(slit);
        current+=slit->header.length;
        acpi_add_table(rsdt,slit);

	/* SSDT */
	printk_debug("ACPI:    * SSDT\n");
	ssdt = (acpi_header_t *)current;
	current += ((acpi_header_t *)AmlCode_ssdt)->length;
	memcpy((void *)ssdt, (void *)AmlCode_ssdt, ((acpi_header_t *)AmlCode_ssdt)->length);
	//Here you need to set value in pci1234, sblk and sbdn in get_bus_conf.c
	update_ssdt((void*)ssdt);
        /* recalculate checksum */
        ssdt->checksum = 0;
        ssdt->checksum = acpi_checksum((unsigned char *)ssdt,ssdt->length);
	acpi_add_table(rsdt,ssdt);

#if ACPI_SSDTX_NUM >= 1

        //same htio, but different position? We may have to copy, change HCIN, and recalculate the checknum and add_table

        for(i=1;i<sysconf.hc_possible_num;i++) {  // 0: is hc sblink
                if((sysconf.pci1234[i] & 1) != 1 ) continue;
                uint8_t c;
                if(i<7) {
                        c  = (uint8_t) ('4' + i - 1);
                }
                else {
                        c  = (uint8_t) ('A' + i - 1 - 6);
                }
                printk_debug("ACPI:    * SSDT for PCI%c Aka hcid = %d\n", c, sysconf.hcid[i]); //pci0 and pci1 are in dsdt
                current   = ( current + 0x07) & -0x08;
                ssdtx = (acpi_header_t *)current;
                switch(sysconf.hcid[i]) {
                case 1: //8132
                        p = AmlCode_ssdt2;
                        break;
                case 2: //8151
                        p = AmlCode_ssdt3;
                        break;
		case 3: //8131
                        p = AmlCode_ssdt4;
                        break;
                default:
			//HTX no io apic
                        p = AmlCode_ssdt5;
			break;
                }
                current += ((acpi_header_t *)p)->length;
                memcpy((void *)ssdtx, (void *)p, ((acpi_header_t *)p)->length);
                update_ssdtx((void *)ssdtx, i);
                ssdtx->checksum = 0;
                ssdtx->checksum = acpi_checksum((unsigned char *)ssdtx,ssdtx->length);
                acpi_add_table(rsdt,ssdtx);
        }
#endif

	/* FACS */
	printk_debug("ACPI:    * FACS\n");
	facs = (acpi_facs_t *) current;
	current += sizeof(acpi_facs_t);
	acpi_create_facs(facs);

	/* DSDT */
	printk_debug("ACPI:    * DSDT\n");
	dsdt = (acpi_header_t *)current;
	current += ((acpi_header_t *)AmlCode)->length;
	memcpy((void *)dsdt,(void *)AmlCode, \
			((acpi_header_t *)AmlCode)->length);
	printk_debug("ACPI:    * DSDT @ %08x Length %x\n",dsdt,dsdt->length);

	/* FDAT */
	printk_debug("ACPI:    * FADT\n");
	fadt = (acpi_fadt_t *) current;
	current += sizeof(acpi_fadt_t);

	acpi_create_fadt(fadt,facs,dsdt);
	acpi_add_table(rsdt,fadt);

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

