/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Uwe Hermann <uwe@hermann-uwe.de>
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

void *smp_write_config_table(void *v)
{
	static const char sig[4] = "PCMP";
	static const char oem[8] = "COREBOOT";
	static const char productid[12] = "ASUS P2B-DS ";
	struct mp_config_table *mc;

	mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);
	memset(mc, 0, sizeof(*mc));

	memcpy(mc->mpc_signature, sig, sizeof(sig));
	mc->mpc_length = sizeof(*mc);	/* initially just the header */
	mc->mpc_spec = 0x04;
	mc->mpc_checksum = 0;		/* not yet computed */
	memcpy(mc->mpc_oem, oem, sizeof(oem));
	memcpy(mc->mpc_productid, productid, sizeof(productid));
	mc->mpc_oemptr = 0;
	mc->mpc_oemsize = 0;
	mc->mpc_entry_count = 0;	/* No entries yet... */
	mc->mpc_lapic = LAPIC_ADDR;
	mc->mpe_length = 0;
	mc->mpe_checksum = 0;
	mc->reserved = 0;

	smp_write_processors(mc);

	/* Bus:		Bus ID	Type */
	smp_write_bus(mc, 0, "PCI   ");
	smp_write_bus(mc, 1, "ISA   ");

	/* I/O APICs:	APIC ID	Version	State		Address */
	smp_write_ioapic(mc, 2, 0x20, 0xfec00000);
	{
		device_t dev;
		struct resource *res;

		dev = dev_find_slot(1, PCI_DEVFN(0x1e, 0));
		if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res)
				smp_write_ioapic(mc, 3, 0x20, res->base);
		}
		dev = dev_find_slot(1, PCI_DEVFN(0x1c, 0));
		if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res)
				smp_write_ioapic(mc, 4, 0x20, res->base);
		}
		dev = dev_find_slot(4, PCI_DEVFN(0x1e, 0));
		if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res)
				smp_write_ioapic(mc, 5, 0x20, res->base);
		}
		dev = dev_find_slot(4, PCI_DEVFN(0x1c, 0));
		if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res)
				smp_write_ioapic(mc, 8, 0x20, res->base);
		}
	}

	/* I/O Ints: Type  Polarity  Trigger  Bus ID  IRQ  APIC ID  PIN# */
	smp_write_intsrc(mc, mp_ExtINT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x0, 0x2, 0x0);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x1, 0x2, 0x1);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x0, 0x2, 0x2);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x3, 0x2, 0x3);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x4, 0x2, 0x4);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x5, 0x2, 0x5);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x6, 0x2, 0x6);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x7, 0x2, 0x7);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x8, 0x2, 0x8);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0x9, 0x2, 0x9);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0xc, 0x2, 0xc);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0xe, 0x2, 0xe);
	smp_write_intsrc(mc, mp_INT,
			 MP_IRQ_TRIGGER_DEFAULT | MP_IRQ_POLARITY_DEFAULT, 0x1,
			 0xf, 0x2, 0xf);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0x0, 0x13, 0x2, 0x13);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0x0, 0x18, 0x2, 0x13);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0x0, 0x30, 0x2, 0x10);

	/* Local Ints: Type  Polarity  Trigger  Bus ID  IRQ  APIC ID  PIN# */
	smp_write_intsrc(mc, mp_ExtINT,
			 MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH, 0x1, 0x0,
			 MP_APIC_ALL, 0x0);
	smp_write_intsrc(mc, mp_NMI, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 0x1, 0x0, MP_APIC_ALL, 0x1);

	/* There is no extension information... */

	/* Compute the checksums */
	mc->mpe_checksum =
	    smp_compute_checksum(smp_next_mpc_entry(mc), mc->mpe_length);
	mc->mpc_checksum = smp_compute_checksum(mc, mc->mpc_length);
	printk_debug("Wrote the mp table end at: %p - %p\n",
		     mc, smp_next_mpe_entry(mc));
	return smp_next_mpe_entry(mc);
}

unsigned long write_smp_table(unsigned long addr)
{
	void *v;
	v = smp_write_floating_table(addr);
	return (unsigned long)smp_write_config_table(v);
}
