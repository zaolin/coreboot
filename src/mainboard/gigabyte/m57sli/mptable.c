/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 AMD
 * Written by Yinghai Lu <yinghailu@amd.com> for AMD.
 * Copyright (C) 2009 Harald Gutmann <harald.gutmann@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
#include <arch/smp/mpspec.h>
#include <device/pci.h>
#include <string.h>
#include <stdint.h>

#include <cpu/amd/amdk8_sysconf.h>
extern unsigned char bus_isa;
extern unsigned char bus_mcp55[8]; //1

extern unsigned apicid_mcp55;

extern unsigned bus_type[256]; 



static void *smp_write_config_table(void *v)
{
        static const char sig[4] = "PCMP";
        static const char oem[8] = "COREBOOT";
        static const char productid[12] = "M57SLI      ";
        struct mp_config_table *mc;
	unsigned sbdn;

	int i,j,k;

        mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);
        memset(mc, 0, sizeof(*mc));

        memcpy(mc->mpc_signature, sig, sizeof(sig));
        mc->mpc_length = sizeof(*mc); /* initially just the header */
        mc->mpc_spec = 0x04;
        mc->mpc_checksum = 0; /* not yet computed */
        memcpy(mc->mpc_oem, oem, sizeof(oem));
        memcpy(mc->mpc_productid, productid, sizeof(productid));
        mc->mpc_oemptr = 0;
        mc->mpc_oemsize = 0;
        mc->mpc_entry_count = 0; /* No entries yet... */
        mc->mpc_lapic = LAPIC_ADDR;
        mc->mpe_length = 0;
        mc->mpe_checksum = 0;
        mc->reserved = 0;

        smp_write_processors(mc);

	get_bus_conf();
	sbdn = sysconf.sbdn;

/*Bus:		Bus ID	Type*/
       /* define bus and isa numbers */
        for(j= 0; j < 256 ; j++) {
		if(bus_type[j])
			 smp_write_bus(mc, j, "PCI   ");
        }
        smp_write_bus(mc, bus_isa, "ISA   ");

/*I/O APICs:	APIC ID	Version	State		Address*/
        {
                device_t dev;
		struct resource *res;

                dev = dev_find_slot(bus_mcp55[0], PCI_DEVFN(sbdn+ 0x1,0));
                if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_1);
			if (res) {
				smp_write_ioapic(mc, apicid_mcp55, 0x11, res->base);
			}
			/* set up the interrupt registers of mcp55 */
	        	pci_write_config32(dev, 0x7c, 0xc643c643);
		        pci_write_config32(dev, 0x80, 0x8da01009);
		        pci_write_config32(dev, 0x84, 0x200018d2);
                }
	}

	/*I/O Ints:	     Type	Trigger    Polarity	                  Bus ID   IRQ	APIC ID	      PIN# */	
	smp_write_intsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, apicid_mcp55, 0x0);

/* ISA ints are edge-triggered, and usually originate from the ISA bus,
 * or its remainings.
 */
#define ISA_INT(intr, pin)\
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, (intr), apicid_mcp55, (pin))

	ISA_INT(1,1);
	ISA_INT(0,2);
	ISA_INT(3,3);
	ISA_INT(4,4);
	ISA_INT(6,6);
	ISA_INT(7,7);
	ISA_INT(8,8);
	ISA_INT(12,12);
	ISA_INT(13,13);
	ISA_INT(14,14);
	ISA_INT(15,15);

/* PCI interrupts are level triggered, and are
 * associated with a specific bus/device/function tuple.
 */
#define PCI_INT(bus, dev, fn, pin)					\
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW,\
			 bus_mcp55[bus], (((dev)<<2)|(fn)), apicid_mcp55, (pin))

	PCI_INT(0,sbdn+1,1, 10); /* SMBus */ 
	PCI_INT(0,sbdn+2,0, 22); /* USB */
	PCI_INT(0,sbdn+2,1, 23); /* USB */
	PCI_INT(0,sbdn+4,0, 21); /* IDE */
	PCI_INT(0,sbdn+5,0, 20); /* SATA */
	PCI_INT(0,sbdn+5,1, 21); /* SATA */
	PCI_INT(0,sbdn+5,2, 22); /* SATA */
	PCI_INT(0,sbdn+6,1, 23); /* HD Audio */
	PCI_INT(0,sbdn+8,0, 20); /* GBit Ethernet */

	/* The PCIe slots, each on its own bus */
        k = 1;
        for(i=0; i<4; i++){
                for(j=7; j>1; j--){
                        if(k>3) k=0;
                        PCI_INT(j,0,i, 16+k);
                        k++;
                }
                k--;
        }

	/* On bus 1: the PCI bus slots...
	   pyhsical PCI slots are j = 7,8 
	   FireWire is j = 10		
	*/
        k=2;
        for(i=0; i<4; i++){
                for(j=6; j<11; j++){
                        if(k>3) k=0;
                        PCI_INT(1,j,i, 16+k);
                        k++;
                }
        }

/*Local Ints:	Type	Polarity    Trigger	Bus ID	 IRQ	APIC ID	PIN#*/
	smp_write_intsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, MP_APIC_ALL, 0x0);
	smp_write_intsrc(mc, mp_NMI, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, MP_APIC_ALL, 0x1);
	/* There is no extension information... */

	/* Compute the checksums */
	mc->mpe_checksum = smp_compute_checksum(smp_next_mpc_entry(mc), mc->mpe_length);
	mc->mpc_checksum = smp_compute_checksum(mc, mc->mpc_length);
	printk(BIOS_DEBUG, "Wrote the mp table end at: %p - %p\n",
		mc, smp_next_mpe_entry(mc));
	return smp_next_mpe_entry(mc);
}

unsigned long write_smp_table(unsigned long addr)
{
	void *v;
	v = smp_write_floating_table(addr);
	return (unsigned long)smp_write_config_table(v);
}
