#include <console/console.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <string.h>
#include <stdint.h>
#if CONFIG_LOGICAL_CPUS==1
#include <cpu/amd/dualcore.h>
#endif

#include <cpu/amd/amdk8_sysconf.h>


// Global variables for MB layouts and these will be shared by irqtable mptable and acpi_tables
//busnum is default
        unsigned char bus_isa;
        unsigned char bus_ck804_0; //1
        unsigned char bus_ck804_1; //2
        unsigned char bus_ck804_2; //3
        unsigned char bus_ck804_3; //4
        unsigned char bus_ck804_4; //5
        unsigned char bus_ck804_5; //6
        unsigned char bus_8131_0;  //7
        unsigned char bus_8131_1;  //8
        unsigned char bus_8131_2;  //9
        unsigned char bus_ck804b_0;//a
        unsigned char bus_ck804b_1;//b
        unsigned char bus_ck804b_2;//c
        unsigned char bus_ck804b_3;//d
        unsigned char bus_ck804b_4;//e
        unsigned char bus_ck804b_5;//f
        unsigned apicid_ck804;
        unsigned apicid_8131_1;
        unsigned apicid_8131_2;
        unsigned apicid_ck804b;

unsigned pci1234x[] = 
{        //Here you only need to set value in pci1234 for HT-IO that could be installed or not
	 //You may need to preset pci1234 for HTIO board, please refer to src/northbridge/amd/amdk8/get_sblk_pci1234.c for detail
        0x0000ff0,
        0x0000ff0,
        0x0000ff0,
//        0x0000ff0,
//        0x0000ff0,
//        0x0000ff0,
//        0x0000ff0,
//        0x0000ff0
};
unsigned hcdnx[] = 
{ //HT Chain device num, actually it is unit id base of every ht device in chain, assume every chain only have 4 ht device at most
	0x20202020,
	0x20202020,
        0x20202020,
//        0x20202020,
//        0x20202020,
//        0x20202020,
//        0x20202020,
//        0x20202020,
};
unsigned sbdn3;
unsigned sbdnb;

extern void get_sblk_pci1234(void);

static unsigned get_bus_conf_done = 0;

void get_bus_conf(void)
{

	unsigned apicid_base;
	unsigned sbdn;

        device_t dev;
        int i;

        if(get_bus_conf_done==1) return; //do it only once

        get_bus_conf_done = 1;

        sysconf.hc_possible_num = sizeof(pci1234x)/sizeof(pci1234x[0]);
        for(i=0;i<sysconf.hc_possible_num; i++) {
                sysconf.pci1234[i] = pci1234x[i];
                sysconf.hcdn[i] = hcdnx[i];
        }

        get_sblk_pci1234();

	sysconf.sbdn = (sysconf.hcdn[0] & 0xff); // first byte of first chain
	sbdn = sysconf.sbdn;

	sbdn3 = (sysconf.hcdn[1] & 0xff);

	sbdnb = (sysconf.hcdn[2] & 0xff); // first byte of second chain

	bus_ck804_0 = (sysconf.pci1234[0] >> 16) & 0xff;

                /* CK804 */
                dev = dev_find_slot(bus_ck804_0, PCI_DEVFN(sbdn + 0x09,0));
                if (dev) {
                        bus_ck804_1 = pci_read_config8(dev, PCI_SECONDARY_BUS);
#if 0
                        bus_ck804_2 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804_2++;
#else
                        bus_ck804_5 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804_5++;
#endif
                }
                else {
                        printk_debug("ERROR - could not find PCI 1:%02x.0, using defaults\n", sbdn + 0x09);

                        bus_ck804_1 = 2;
#if 0
                        bus_ck804_2 = 3;
#else
                        bus_ck804_5 = 3;
#endif

                }
#if 0
                dev = dev_find_slot(bus_ck804_0, PCI_DEVFN(sbdn + 0x0b,0));
                if (dev) {
                        bus_ck804_2 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804_3 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804_3++;
                }
                else {
                        printk_debug("ERROR - could not find PCI 1:%02x.0, using defaults\n", sbdn + 0x0b);

                        bus_ck804_3 = bus_ck804_2+1;
                }

                dev = dev_find_slot(bus_ck804_0, PCI_DEVFN(sbdn + 0x0c,0));
                if (dev) {
                        bus_ck804_3 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804_4 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804_4++;
                }
                else {
                        printk_debug("ERROR - could not find PCI 1:%02x.0, using defaults\n", sbdn + 0x0c);

                        bus_ck804_4 = bus_ck804_3+1;
                }


                dev = dev_find_slot(bus_ck804_0, PCI_DEVFN(sbdn + 0x0d,0));
                if (dev) {
                        bus_ck804_4 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804_5 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804_5++;
                }
                else {
                        printk_debug("ERROR - could not find PCI 1:%02x.0, using defaults\n",sbdn + 0x0d);

                        bus_ck804_5 = bus_ck804_4+1;
                }
#endif

                dev = dev_find_slot(bus_ck804_0, PCI_DEVFN(sbdn+ 0x0e,0));
                if (dev) {
                        bus_ck804_5 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                }
                else {
                        printk_debug("ERROR - could not find PCI 1:%02x.0, using defaults\n", sbdn+ 0x0e);
                }

		bus_8131_0 = (sysconf.pci1234[1] >> 16) & 0xff;
                /* 8131-1 */
                dev = dev_find_slot(bus_8131_0, PCI_DEVFN(sbdn3,0));
                if (dev) {
                        bus_8131_1 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_8131_2 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_8131_2++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:01.0, using defaults\n", bus_8131_0);

                        bus_8131_1 = bus_8131_0+1;
                        bus_8131_2 = bus_8131_0+2;
                }
                /* 8131-2 */
                dev = dev_find_slot(bus_8131_0, PCI_DEVFN(sbdn3,0));
                if (dev) {
                        bus_8131_2 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_isa = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_isa++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:02.0, using defaults\n", bus_8131_0);

                        bus_8131_2 = bus_8131_1+1;
                }

                /* CK804b */

	if(sysconf.pci1234[2] & 0x0f) { //if the second cpu is installed
		bus_ck804b_0 = (sysconf.pci1234[2]>>16) & 0xff;
#if 0
                dev = dev_find_slot(bus_ck804b_0, PCI_DEVFN(sbdnb + 0x09,0));
                if (dev) {
                        bus_ck804b_1 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804b_2 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804b_2++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:%02x.0, using defaults\n", bus_ck804b_0,sbdnb+0x09);

                        bus_ck804b_1 = bus_ck804b_0+1;
                        bus_ck804b_2 = bus_ck804b_0+2;
                }

                dev = dev_find_slot(bus_ck804b_0, PCI_DEVFN(sbdnb + 0x0b,0));
                if (dev) {
                        bus_ck804b_2 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804b_3 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804b_3++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:%02x.0, using defaults\n", bus_ck804b_0,sbdnb+0x0b);

                        bus_ck804b_2 = bus_ck804b_0+1;
                        bus_ck804b_3 = bus_ck804b_0+2;
                }

                dev = dev_find_slot(bus_ck804b_0, PCI_DEVFN(sbdnb + 0x0c,0));
                if (dev) {
                        bus_ck804b_3 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804b_4 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804b_4++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:%02x.0, using defaults\n", bus_ck804b_0,sbdnb+0x0c);

                        bus_ck804b_4 = bus_ck804b_3+1;
                }
                dev = dev_find_slot(bus_ck804b_0, PCI_DEVFN(sbdnb + 0x0d,0));
                if (dev) {
                        bus_ck804b_4 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_ck804b_5 = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_ck804b_5++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:%02x.0, using defaults\n", bus_ck804b_0,sbdnb+0x0d);

                        bus_ck804b_5 = bus_ck804b_4+1;
                }
#endif

                dev = dev_find_slot(bus_ck804b_0, PCI_DEVFN(sbdnb + 0x0e,0));
                if (dev) {
                        bus_ck804b_5 = pci_read_config8(dev, PCI_SECONDARY_BUS);
                        bus_isa = pci_read_config8(dev, PCI_SUBORDINATE_BUS);
                        bus_isa++;
                }
                else {
                        printk_debug("ERROR - could not find PCI %02x:%02x.0, using defaults\n", bus_ck804b_0,sbdnb+0x0e);
#if 1
                        bus_ck804b_5 = bus_ck804b_4+1;
#endif

                        bus_isa = bus_ck804b_5+1;
                }
	}


/*I/O APICs:	APIC ID	Version	State		Address*/
#if CONFIG_LOGICAL_CPUS==1
	apicid_base = get_apicid_base(4);
#else 
	apicid_base = CONFIG_MAX_PHYSICAL_CPUS; 
#endif
	apicid_ck804 = apicid_base+0;
        apicid_8131_1 = apicid_base+1;
        apicid_8131_2 = apicid_base+2;
        apicid_ck804b = apicid_base+3;

}
