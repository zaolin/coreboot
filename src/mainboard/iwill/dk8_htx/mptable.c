#include <console/console.h>
#include <arch/smp/mpspec.h>
#include <device/pci.h>
#include <string.h>
#include <stdint.h>
#if CONFIG_LOGICAL_CPUS==1
#include <cpu/amd/multicore.h>
#endif

#include <cpu/amd/amdk8_sysconf.h>
#include "mb_sysconf.h"



static void *smp_write_config_table(void *v)
{
        static const char sig[4] = "PCMP";
        static const char oem[8] = "COREBOOT";
        static const char productid[12] = "DK8-HTX     ";
        struct mp_config_table *mc;

        unsigned char bus_num;
	int i, j;
	struct mb_sysconf_t *m;

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

	m = sysconf.mb;

/*Bus:		Bus ID	Type*/
       /* define bus and isa numbers */
        for(bus_num = 0; bus_num < m->bus_isa; bus_num++) {
                smp_write_bus(mc, bus_num, "PCI   ");
        }
        smp_write_bus(mc, m->bus_isa, "ISA   ");

/*I/O APICs:	APIC ID	Version	State		Address*/
	smp_write_ioapic(mc, m->apicid_8111, 0x11, 0xfec00000); //8111
        {
                device_t dev;
		struct resource *res;
                dev = dev_find_slot(m->bus_8132_0, PCI_DEVFN(m->sbdn3, 1));
                if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res) {
				smp_write_ioapic(mc, m->apicid_8132_1, 0x11, res->base);
			}
                }
                dev = dev_find_slot(m->bus_8132_0, PCI_DEVFN(m->sbdn3+1, 1));
                if (dev) {
			res = find_resource(dev, PCI_BASE_ADDRESS_0);
			if (res) {
				smp_write_ioapic(mc, m->apicid_8132_2, 0x11, res->base);
			}
                }

                j = 0;

                for(i=1; i< sysconf.hc_possible_num; i++) {
                        if(!(sysconf.pci1234[i] & 0x1) ) continue;

                        switch(sysconf.hcid[i]) {
                        case 1: // 8132
			case 3: // 8131
                                dev = dev_find_slot(m->bus_8132a[j][0], PCI_DEVFN(m->sbdn3a[j], 1));
                                if (dev) {
                                        res = find_resource(dev, PCI_BASE_ADDRESS_0);
                                        if (res) {
                                                smp_write_ioapic(mc, m->apicid_8132a[j][0], 0x11, res->base);
                                        }
                                }
                                dev = dev_find_slot(m->bus_8132a[j][0], PCI_DEVFN(m->sbdn3a[j]+1, 1));
                                if (dev) {
                                        res = find_resource(dev, PCI_BASE_ADDRESS_0);
                                        if (res) {
                                                smp_write_ioapic(mc, m->apicid_8132a[j][1], 0x11, res->base);
                                        }
                                }
                                break;
                        }
                        j++;
                }

	}
  
/*I/O Ints:	Type	Polarity    Trigger	Bus ID	 IRQ	APIC ID	PIN# */	
	smp_write_intsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, m->bus_isa, 0x0, m->apicid_8111, 0x0);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x1, m->apicid_8111, 0x1);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x0, m->apicid_8111, 0x2);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x3, m->apicid_8111, 0x3);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x4, m->apicid_8111, 0x4);
  	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x5, m->apicid_8111, 0x5);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x6, m->apicid_8111, 0x6);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x7, m->apicid_8111, 0x7);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x8, m->apicid_8111, 0x8);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0x9, m->apicid_8111, 0x9);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0xc, m->apicid_8111, 0xc);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0xd, m->apicid_8111, 0xd);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0xe, m->apicid_8111, 0xe);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  m->bus_isa, 0xf, m->apicid_8111, 0xf);
//??? What
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8111_0, ((sysconf.sbdn+1)<<2)|3, m->apicid_8111, 0x13);

// Onboard AMD USB
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8111_1, (0<<2)|3, m->apicid_8111, 0x13);

// Onboard VGA
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8111_1, (6<<2)|0, m->apicid_8111, 0x12);

//Slot 5 PCI 32
        for(i=0;i<4;i++) {
                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8111_1, (5<<2)|i, m->apicid_8111, 0x10 + (1+i)%4); //16
        }

//Slot 6 PCI 32
        for(i=0;i<4;i++) {
                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8111_1, (4<<2)|i, m->apicid_8111, 0x10 + (0+i)%4); //16
        }
//Slot 1: HTX

//Slot 2 PCI-X 133/100/66
        for(i=0;i<4;i++) {
                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132_2, (2<<2)|i, m->apicid_8132_2, (2+i)%4); //30
        }

//Slot 3 PCI-X 133/100/66
        for(i=0;i<4;i++) {
                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132_1, (1<<2)|i, m->apicid_8132_1, (1+i)%4); //25
        }

//Slot 4 PCI-X 133/100/66
        for(i=0;i<4;i++) {
                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132_1, (2<<2)|i, m->apicid_8132_1, (2+i)%4); //26 
        }

//Onboard NICS
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132_1, (3<<2)|0, m->apicid_8132_1, 3); //27
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132_1, (4<<2)|0, m->apicid_8132_1, 0); //24

//Onboard SATA 
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132_1, (5<<2)|0, m->apicid_8132_1, 1); //25

        j = 0;

        for(i=1; i< sysconf.hc_possible_num; i++) {
                if(!(sysconf.pci1234[i] & 0x1) ) continue;
                int ii;
                device_t dev;
                struct resource *res;
                switch(sysconf.hcid[i]) {
                case 1:
		case 3:
                        dev = dev_find_slot(m->bus_8132a[j][0], PCI_DEVFN(m->sbdn3a[j], 1));
                        if (dev) {
                                res = find_resource(dev, PCI_BASE_ADDRESS_0);
                                if (res) {
                                        //Slot 1 PCI-X 133/100/66
                                        for(ii=0;ii<4;ii++) {
                                                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132a[j][1], (0<<2)|ii, m->apicid_8132a[j][0], (0+ii)%4); //
                                        }
                                }
                        }

                        dev = dev_find_slot(m->bus_8132a[j][0], PCI_DEVFN(m->sbdn3a[j]+1, 1));
                        if (dev) {
                                res = find_resource(dev, PCI_BASE_ADDRESS_0);
                                if (res) {
                                        //Slot 2 PCI-X 133/100/66
                                        for(ii=0;ii<4;ii++) {
                                                smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8132a[j][2], (0<<2)|ii, m->apicid_8132a[j][1], (0+ii)%4); //25
                                        }
                                }
                        }

                        break;
                case 2:

                //  Slot AGP
                        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, m->bus_8151[j][1], 0x0, m->apicid_8111, 0x11);
                        break;
                }

                j++;
        }



/*Local Ints:	Type	Polarity    Trigger	Bus ID	 IRQ	APIC ID	PIN#*/
	smp_write_intsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, m->bus_isa, 0x0, MP_APIC_ALL, 0x0);
	smp_write_intsrc(mc, mp_NMI, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, m->bus_isa, 0x0, MP_APIC_ALL, 0x1);
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
