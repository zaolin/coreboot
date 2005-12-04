/* This file was generated by getpir.c, do not modify! 
   (but if you do, please run checkpir on it to verify)
   Contains the IRQ Routing Table dumped directly from your memory , wich BIOS sets up

   Documentation at : http://www.microsoft.com/hwdev/busbios/PCIIRQ.HTM
*/
#include <console/console.h>
#include <device/pci.h>
#include <string.h>
#include <stdint.h>
#include <arch/pirq_routing.h>

const struct irq_routing_table intel_irq_routing_table = {
	PIRQ_SIGNATURE, /* u32 signature */
	PIRQ_VERSION,   /* u16 version   */
	32+16*11,        /* there can be total 11 devices on the bus */
	3,           /* Where the interrupt router lies (bus) */
	(4<<3)|3,           /* Where the interrupt router lies (dev) */
	0,         /* IRQs devoted exclusively to PCI usage */
	0x1022,         /* Vendor */
	0x746b,         /* Device */
	0,         /* Crap (miniport) */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* u8 rfu[11] */
	0x42,         /*  u8 checksum , this hase to set to some value that would give 0 after the sum of all bytes for this structure (including checksum) */
	{
		{3,(4<<3)|0, {{0x1, 0xdef8}, {0x2, 0xdef8}, {0x3, 0xdef8}, {0x4, 0xdef8}}, 0, 0},
		{0x6,0, {{0, 0}, {0, 0}, {0, 0}, {0x4, 0xdef8}}, 0, 0},
		{0x1,0, {{0x1, 0xdef8}, {0x2, 0xdef8}, {0, 0}, {0, 0}}, 0x0, 0},
		{0x5,(3<<3)|0, {{0x1, 0xdef8}, {0x2, 0xdef8}, {0x3, 0xdef8}, {0x4, 0xdef8}}, 0x1, 0},
		{0x5,(6<<3)|0, {{0x2, 0xdef8}, {0x3, 0xdef8}, {0x4, 0xdef8}, {0x1, 0xdef8}}, 0x2, 0},
		{0x4,(8<<3)|0, {{0x4, 0xdef8}, {0x1, 0xdef8}, {0x2, 0xdef8}, {0x3, 0xdef8}}, 0x3, 0},
		{0x4,(7<<3)|0, {{0x3, 0xdef8}, {0x4, 0xdef8}, {0x1, 0xdef8}, {0x2, 0xdef8}}, 0x4, 0},
		{0x6,(0x0a<<3)|0, {{0x1, 0xdef8}, {0x2, 0xdef8}, {0x3, 0xdef8}, {0x4, 0xdef8}}, 0x5, 0},
		{0x4,(9<<3)|0, {{0x1, 0xdef8}, {2, 0xdef8}, {0, 0}, {0, 0}}, 0, 0},
		{0x6,(0x0b<<3)|0, {{0x2, 0xdef8}, {0, 0}, {0, 0}, {0, 0}}, 0, 0},
		{0x6,(0x0c<<3)|0, {{0x4, 0xdef8}, {0, 0}, {0, 0}, {0, 0}}, 0, 0},
	}
};

static void write_pirq_info(struct irq_info *pirq_info, uint8_t bus, uint8_t devfn, uint8_t link0, uint16_t bitmap0, 
		uint8_t link1, uint16_t bitmap1, uint8_t link2, uint16_t bitmap2,uint8_t link3, uint16_t bitmap3,
		uint8_t slot, uint8_t rfu)
{
        pirq_info->bus = bus; 
        pirq_info->devfn = devfn;
                pirq_info->irq[0].link = link0;
                pirq_info->irq[0].bitmap = bitmap0;
                pirq_info->irq[1].link = link1;
                pirq_info->irq[1].bitmap = bitmap1;
                pirq_info->irq[2].link = link2;
                pirq_info->irq[2].bitmap = bitmap2;
                pirq_info->irq[3].link = link3;
                pirq_info->irq[3].bitmap = bitmap3;
        pirq_info->slot = slot;
        pirq_info->rfu = rfu;
}

extern  unsigned char bus_8132_0;
extern  unsigned char bus_8132_1;
extern  unsigned char bus_8132_2;
extern  unsigned char bus_8111_0;
extern  unsigned char bus_8111_1;
extern  unsigned char bus_8151_0;
extern  unsigned char bus_8151_1;

extern  unsigned sbdn;
extern  unsigned hcdn[];

extern void get_bus_conf(void);

unsigned long write_pirq_routing_table(unsigned long addr)
{

	struct irq_routing_table *pirq;
	struct irq_info *pirq_info;
	unsigned slot_num;
	uint8_t *v;

        uint8_t sum=0;
        int i;

	get_bus_conf(); // it will find out all bus num and apic that share with mptable.c and mptable.c and acpi_tables.c

        /* Align the table to be 16 byte aligned. */
        addr += 15;
        addr &= ~15;

        /* This table must be betweeen 0xf0000 & 0x100000 */
        printk_info("Writing IRQ routing tables to 0x%x...", addr);

	pirq = (void *)(addr);
	v = (uint8_t *)(addr);
	
	pirq->signature = PIRQ_SIGNATURE;
	pirq->version  = PIRQ_VERSION;
	
	pirq->rtr_bus = bus_8111_0;
	pirq->rtr_devfn = ((sbdn+1)<<3)|0;

	pirq->exclusive_irqs = 0;
	
	pirq->rtr_vendor = 0x1022;
	pirq->rtr_device = 0x746b;

	pirq->miniport_data = 0;

	memset(pirq->rfu, 0, sizeof(pirq->rfu));
	
	pirq_info = (void *) ( &pirq->checksum + 1);
	slot_num = 0;
//pci bridge
	write_pirq_info(pirq_info, bus_8111_0, ((sbdn+1)<<3)|0, 0x1, 0xdef8, 0x2, 0xdef8, 0x3, 0xdef8, 0x4, 0xdef8, 0, 0);
	pirq_info++; slot_num++;
//pcix bridge
//        write_pirq_info(pirq_info, bus_8132_0, (sbd3<<3)|0, 0x1, 0xdef8, 0x2, 0xdef8, 0x3, 0xdef8, 0x4, 0xdef8, 0, 0);
//        pirq_info++; slot_num++;
//agp bridge
        write_pirq_info(pirq_info, bus_8151_0, ((hcdn[1] & 0xff)<<3)|0, 0x1, 0xdef8, 0x2, 0xdef8, 0x3, 0xdef8, 0x4, 0xdef8, 0, 0); 

        pirq_info++; slot_num++;
             
	pirq->size = 32 + 16 * slot_num; 

        for (i = 0; i < pirq->size; i++)
                sum += v[i];	

	sum = pirq->checksum - sum;

        if (sum != pirq->checksum) {
                pirq->checksum = sum;
        }

	printk_info("done.\n");

	return	(unsigned long) pirq_info;

}
