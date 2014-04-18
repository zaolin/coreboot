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
#include <arch/smp/mpspec.h>
#include <device/pci.h>
#include <arch/io.h>
#include <string.h>
#include <stdint.h>

#include <cpu/amd/amdk8_sysconf.h>

extern u8 bus_isa;
extern u8 bus_rs690[8];
extern u8 bus_sb600[2];

extern unsigned long apicid_sb600;

extern unsigned long bus_type[256];
extern unsigned long sbdn_rs690;
extern unsigned long sbdn_sb600;

extern void get_bus_conf(void);

void *smp_write_config_table(void *v)
{
	static const char sig[4] = "PCMP";
	static const char oem[8] = "ATI     ";
	static const char productid[12] = "DBM690T     ";
	struct mp_config_table *mc;
	int i, j;

	mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);
	memset(mc, 0, sizeof(*mc));

	memcpy(mc->mpc_signature, sig, sizeof(sig));
	mc->mpc_length = sizeof(*mc);	/* initially just the header */
	mc->mpc_spec = 0x04;
	mc->mpc_checksum = 0;	/* not yet computed */
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

	get_bus_conf();

	/* Bus:         Bus ID  Type */
	/* define bus and isa numbers */
	for (j = 0; j < bus_isa; j++) {
		smp_write_bus(mc, j, (char *)"PCI   ");
	}
	smp_write_bus(mc, bus_isa, (char *)"ISA   ");

	/* I/O APICs:   APIC ID Version State   Address */
	{
		device_t dev;
		u32 dword;
		u8 byte;

		dev =
		    dev_find_slot(bus_sb600[0],
				  PCI_DEVFN(sbdn_sb600 + 0x14, 0));
		if (dev) {
			dword = pci_read_config32(dev, 0x74) & 0xfffffff0;
			smp_write_ioapic(mc, apicid_sb600, 0x11, dword);

			/* Initialize interrupt mapping */
			/* aza */
			byte = pci_read_config8(dev, 0x63);
			byte &= 0xf8;
			byte |= 0;	/* 0: INTA, ...., 7: INTH */
			pci_write_config8(dev, 0x63, byte);

			/* SATA */
			dword = pci_read_config32(dev, 0xac);
			dword &= ~(7 << 26);
			dword |= 6 << 26;	/* 0: INTA, ...., 7: INTH */
			/* dword |= 1<<22; PIC and APIC co exists */
			pci_write_config32(dev, 0xac, dword);

			/* 
			 * 00:12.0: PROG SATA : INT F
			 * 00:13.0: INTA USB_0
			 * 00:13.1: INTB USB_1
			 * 00:13.2: INTC USB_2
			 * 00:13.3: INTD USB_3
			 * 00:13.4: INTC USB_4
			 * 00:13.5: INTD USB2
			 * 00:14.1: INTA IDE
			 * 00:14.2: Prog HDA : INT E
			 * 00:14.5: INTB ACI
			 * 00:14.6: INTB MCI
			 */
		}
	}

	/* I/O Ints:    Type    Polarity    Trigger     Bus ID   IRQ    APIC ID PIN# */
	smp_write_intsrc(mc, mp_ExtINT,
			 MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH, bus_isa,
			 0x0, apicid_sb600, 0x0);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x1, apicid_sb600, 0x1);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x0, apicid_sb600, 0x2);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x3, apicid_sb600, 0x3);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x4, apicid_sb600, 0x4);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x6, apicid_sb600, 0x6);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x7, apicid_sb600, 0x7);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0xc, apicid_sb600, 0xc);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0xd, apicid_sb600, 0xd);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0xe, apicid_sb600, 0xe);

	/* usb */
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0, 19 << 2 | 0, apicid_sb600, 0x10);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0, 19 << 2 | 1, apicid_sb600, 0x11);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0, 19 << 2 | 2, apicid_sb600, 0x12);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0, 19 << 2 | 3, apicid_sb600, 0x13);

	/* sata */
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0, 18 << 2 | 0, apicid_sb600, 22);

	/* HD Audio: b0:d20:f1:reg63 should be 0. */
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 0, 20 << 2 | 0, apicid_sb600, 16);

	/* on board NIC & Slot PCIE.  */
	i = 2;
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[1], 0x5 << 2 | 0, apicid_sb600, 18);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[1], 0x5 << 2 | 1, apicid_sb600, 19);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[2], 0x0 << 2 | 0, apicid_sb600, 18);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[3], 0x0 << 2 | 0, apicid_sb600, 19);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[4], 0x0 << 2 | 0, apicid_sb600, 16);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[5], 0x0 << 2 | 0, apicid_sb600, 17);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[6], 0x0 << 2 | 0, apicid_sb600, 18);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_rs690[7], 0x0 << 2 | 0, apicid_sb600, 19);

	/* PCI slots */
	i += 6;
	j = 5;
	/* PCI_SLOT 0. */
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 5 << 2 | 0, apicid_sb600, 20);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 5 << 2 | 1, apicid_sb600, 21);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 5 << 2 | 2, apicid_sb600, 22);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 5 << 2 | 3, apicid_sb600, 23);

	/* PCI_SLOT 1. */
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 6 << 2 | 0, apicid_sb600, 21);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 6 << 2 | 1, apicid_sb600, 22);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 6 << 2 | 2, apicid_sb600, 23);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 6 << 2 | 3, apicid_sb600, 20);

	/* PCI_SLOT 2. */
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 7 << 2 | 0, apicid_sb600, 22);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 7 << 2 | 1, apicid_sb600, 23);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 7 << 2 | 2, apicid_sb600, 20);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW,
			 bus_sb600[1], 7 << 2 | 3, apicid_sb600, 21);

	/*Local Ints:   Type    Polarity    Trigger     Bus ID   IRQ    APIC ID PIN# */
	smp_write_intsrc(mc, mp_ExtINT,
			 MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH, bus_isa,
			 0x0, MP_APIC_ALL, 0x0);
	smp_write_intsrc(mc, mp_NMI, MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH,
			 bus_isa, 0x0, MP_APIC_ALL, 0x1);
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
