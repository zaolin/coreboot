#include <console/console.h>
#include <arch/smp/mpspec.h>
#include <device/pci.h>
#include <string.h>
#include <stdint.h>

void *smp_write_config_table(void *v)
{
	static const char sig[4] = "PCMP";
	static const char oem[8] = "AMD     ";
	static const char productid[12] = "Serenade   ";
	struct mp_config_table *mc;

	unsigned char bus_num;
	unsigned char bus_isa;
	unsigned char bus_8111_0;
	unsigned char bus_8111_1;
	unsigned char bus_8131_1;
	unsigned char bus_8131_2;

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

	{
		device_t dev;

		/* 8111 */
		dev = dev_find_slot(1, PCI_DEVFN(0x03,0));
		if (dev) {
                	bus_8111_0 = pci_read_config8(dev, PCI_PRIMARY_BUS);
			bus_8111_1 = pci_read_config8(dev, PCI_SECONDARY_BUS);
			bus_isa	   = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
			bus_isa++;
		} else {
			printk_debug("ERROR - could not find PCI 1:03.0, using defaults\n");
			bus_8111_1 = 4;
			bus_isa = 5;
		}

		/* 8131-1 */
		dev = dev_find_slot(1, PCI_DEVFN(0x01,0));
		if (dev) {
			bus_8131_1 = pci_read_config8(dev, PCI_SECONDARY_BUS);

		} else {
			printk_debug("ERROR - could not find PCI 1:01.0, using defaults\n");
			bus_8131_1 = 2;
		}

		/* 8131-2 */
		dev = dev_find_slot(1, PCI_DEVFN(0x02,0));
		if (dev) {
			bus_8131_2 = pci_read_config8(dev, PCI_SECONDARY_BUS);
		} else {
			printk_debug("ERROR - could not find PCI 1:02.0, using defaults\n");
			bus_8131_2 = 3;
		}
	}

	/* define bus and isa numbers */
	for (bus_num = 0; bus_num < bus_isa; bus_num++) {
		smp_write_bus(mc, bus_num, "PCI	  ");
	}
	smp_write_bus(mc, bus_isa, "ISA	  ");

	/* Legacy IOAPIC #2 */
	smp_write_ioapic(mc, 2, 0x11, 0xfec00000);
	{
		device_t dev;
		struct resource *res;
		/* 8131-1 apic #3 */
		dev = dev_find_slot(1, PCI_DEVFN(0x01,1));
		if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res) {
				smp_write_ioapic(mc, 0x03, 0x11, res->base);
			}
		}
		/* 8131-2 apic #4 */
		dev = dev_find_slot(1, PCI_DEVFN(0x02,1));
 		if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res) {
				smp_write_ioapic(mc, 0x04, 0x11, res->base);
			}
		}
	}

	/* ISA backward compatibility interrupts  */
	smp_write_intsrc(mc, mp_ExtINT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x00, 0x02, 0x00);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x01, 0x02, 0x01);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x00, 0x02, 0x02);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x03, 0x02, 0x03);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x04, 0x02, 0x04);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x05, 0x02, 0x05);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x06, 0x02, 0x06);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x07, 0x02, 0x07);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x08, 0x02, 0x08);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x09, 0x02, 0x09);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x0a, 0x02, 0x0a);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x0b, 0x02, 0x0b);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x0c, 0x02, 0x0c);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x0d, 0x02, 0x0d);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x0e, 0x02, 0x0e);
	smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x0f, 0x02, 0x0f);

	/* PCI Ints:	     Type	Polarity    	    Trigger		  	Bus ID      PCIDEVNUM|IRQ  APIC ID PIN# */
	/* Integrated SMBus 2.0 */
        smp_write_intsrc(mc, mp_INT,	MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8111_0, (0x04<<2)|3, 0x2, 0x13);
	/* Integrated AMD AC97 Audio */
        smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8111_0, (0x04<<2)|1, 0x2, 0x11);

	/* Integrated AMD USB */
        smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8111_1, (0x00<<2)|3, 0x2, 0x13);

	/* On board ATI Rage XL */
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8111_1, (0x05<<2)|0, 0x2, 0x10);

	/* On board Broadcom nics */
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_1, (0x01<<2)|0, 0x3, 0x00);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_1, (0x01<<2)|1, 0x3, 0x01);

	/* On board LSI SCSI */
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_1, (0x02<<2)|0, 0x3, 0x02);

	/* PCI Slot 1 PCIX */
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x03<<2)|0, 0x2, 0x10);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x03<<2)|1, 0x2, 0x11);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x03<<2)|2, 0x2, 0x12);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x03<<2)|3, 0x2, 0x13);

	/* PCI Slot 2 PCIX */
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x04<<2)|0, 0x2, 0x11);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x04<<2)|1, 0x2, 0x12);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x04<<2)|2, 0x2, 0x13);
	smp_write_intsrc(mc, mp_INT,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_8131_2, (0x04<<2)|3, 0x2, 0x10);

	/* Standard local interrupt assignments:
	 * 		      Type	 Polarity		Trigger			 Bus ID   IRQ	APIC ID	     PIN# */
	smp_write_lintsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x00, MP_APIC_ALL, 0x00);
	smp_write_lintsrc(mc, mp_NMI,    MP_IRQ_TRIGGER_DEFAULT|MP_IRQ_POLARITY_DEFAULT, bus_isa, 0x00, MP_APIC_ALL, 0x01);

	/* There is no extension information... */

	/* Compute the checksums */
	mc->mpe_checksum = smp_compute_checksum(smp_next_mpc_entry(mc), mc->mpe_length);
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

