#include <console/console.h>
#include <arch/pirq_routing.h>
#include <string.h>
#include <device/pci.h>

#if (DEBUG==1 && HAVE_PIRQ_TABLE==1)
static void check_pirq_routing_table(struct irq_routing_table *rt)
{
	uint8_t *addr = (uint8_t *)rt;
	uint8_t sum=0;
	int i;

	printk_info("Checking IRQ routing table consistency...\n");

#if defined(IRQ_SLOT_COUNT)
	if (sizeof(struct irq_routing_table) != rt->size) {
		printk_warning("Inconsistent IRQ routing table size (0x%x/0x%x)\n",
			       sizeof(struct irq_routing_table),
			       rt->size
			);
		rt->size=sizeof(struct irq_routing_table);
	}
#endif

	for (i = 0; i < rt->size; i++)
		sum += addr[i];

	printk_debug("%s() - irq_routing_table located at: 0x%p\n",
		     __FUNCTION__, addr);

	
	sum = rt->checksum - sum;

	if (sum != rt->checksum) {
		printk_warning("%s:%6d:%s() - "
			       "checksum is: 0x%02x but should be: 0x%02x\n",
			       __FILE__, __LINE__, __FUNCTION__, rt->checksum, sum);
		rt->checksum = sum;
	}

	if (rt->signature != PIRQ_SIGNATURE || rt->version != PIRQ_VERSION ||
	    rt->size % 16 ) {
		printk_warning("%s:%6d:%s() - "
			       "Interrupt Routing Table not valid\n",
			       __FILE__, __LINE__, __FUNCTION__);
		return;
	}

	sum = 0;
	for (i=0; i<rt->size; i++)
		sum += addr[i];

	if (sum) {
		printk_warning("%s:%6d:%s() - "
			       "checksum error in irq routing table\n",
			       __FILE__, __LINE__, __FUNCTION__);
	}

	printk_info("done.\n");
}

static int verify_copy_pirq_routing_table(unsigned long addr)
{
	int i;
	uint8_t *rt_orig, *rt_curr;

	rt_curr = (uint8_t*)addr;
	rt_orig = (uint8_t*)&intel_irq_routing_table;
	printk_info("Verifing copy of IRQ routing tables at 0x%x...", addr);
	for (i = 0; i < intel_irq_routing_table.size; i++) {
		if (*(rt_curr + i) != *(rt_orig + i)) {
			printk_info("failed\n");
			return -1;
		}
	}
	printk_info("done\n");
	
	check_pirq_routing_table((struct irq_routing_table *)addr);
	
	return 0;
}
#else
#define verify_copy_pirq_routing_table(addr)
#endif

#if HAVE_PIRQ_TABLE==1
unsigned long copy_pirq_routing_table(unsigned long addr)
{
	/* Align the table to be 16 byte aligned. */
	addr += 15;
	addr &= ~15;

	/* This table must be betweeen 0xf0000 & 0x100000 */
	printk_info("Copying IRQ routing tables to 0x%x...", addr);
	memcpy((void *)addr, &intel_irq_routing_table, intel_irq_routing_table.size);
	printk_info("done.\n");
	verify_copy_pirq_routing_table(addr);
	pirq_routing_irqs(addr);
	return addr + intel_irq_routing_table.size;
}
#endif

#if (PIRQ_ROUTE==1 && HAVE_PIRQ_TABLE==1)
void pirq_routing_irqs(unsigned long addr)
{
	int i, j, k, num_entries;
	unsigned char irq_slot[4];
	unsigned char pirq[4] = {0, 0, 0, 0};
	struct irq_routing_table *pirq_tbl;
	device_t pdev;

	pirq_tbl = (struct irq_routing_table *)(addr);
	num_entries = (pirq_tbl->size - 32) / 16;

	/* Set PCI IRQs. */
	for (i = 0; i < num_entries; i++) {

		printk_debug("PIR Entry %d Dev/Fn: %X Slot: %d\n", i,
			pirq_tbl->slots[i].devfn >> 3, pirq_tbl->slots[i].slot);

		for (j = 0; j < 4; j++) {

			int link = pirq_tbl->slots[i].irq[j].link;
			int bitmap = pirq_tbl->slots[i].irq[j].bitmap & pirq_tbl->exclusive_irqs;
			int irq = 0;

			printk_debug("INT: %c link: %x bitmap: %x  ",
				'A' + j, link, bitmap);

			if (!bitmap|| !link || link > 4) {

				printk_debug("not routed\n");
				irq_slot[j] = irq;
				continue;
			}

			/* yet not routed */
			if (!pirq[link - 1]) {

				for (k = 2; k < 15; k++) {

					if (!((bitmap >> k) & 1))
						continue;

					irq = k;

					/* yet not routed */
					if (pirq[0] != irq && pirq[1] != irq && pirq[2] != irq && pirq[3] != irq)
						break;
				}

				if (irq)
					pirq[link - 1] = irq;
			}
			else
				irq = pirq[link - 1];

			printk_debug("IRQ: %d\n", irq);
			irq_slot[j] = irq;
		}

		/* Bus, device, slots IRQs for {A,B,C,D}. */
		pci_assign_irqs(pirq_tbl->slots[i].bus,
			pirq_tbl->slots[i].devfn >> 3, irq_slot);
	}

	printk_debug("PIRQ1: %d\n", pirq[0]);
	printk_debug("PIRQ2: %d\n", pirq[1]);
	printk_debug("PIRQ3: %d\n", pirq[2]);
	printk_debug("PIRQ4: %d\n", pirq[3]);

	pirq_assign_irqs(pirq);
}
#endif
