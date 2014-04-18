/*
 * (C) 2003 Linux Networx, SuSE Linux AG
 */
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>


struct ioapicreg {
	unsigned int reg;
	unsigned int value_low, value_high;
};

static struct ioapicreg ioapicregvalues[] = {
#define ALL		(0xff << 24)
#define NONE		(0)
#define DISABLED	(1 << 16)
#define ENABLED		(0 << 16)
#define TRIGGER_EDGE	(0 << 15)
#define TRIGGER_LEVEL	(1 << 15)
#define POLARITY_HIGH	(0 << 13)
#define POLARITY_LOW	(1 << 13)
#define PHYSICAL_DEST	(0 << 11)
#define LOGICAL_DEST	(1 << 11)
#define ExtINT		(7 << 8)
#define NMI		(4 << 8)
#define SMI		(2 << 8)
#define INT		(1 << 8)
	/* IO-APIC virtual wire mode configuration */
	/* mask, trigger, polarity, destination, delivery, vector */
	{   0, ENABLED | TRIGGER_EDGE | POLARITY_HIGH | PHYSICAL_DEST | ExtINT, NONE},
	{   1, DISABLED, NONE},
	{   2, DISABLED, NONE},
	{   3, DISABLED, NONE},
	{   4, DISABLED, NONE},
	{   5, DISABLED, NONE},
	{   6, DISABLED, NONE},
	{   7, DISABLED, NONE},
	{   8, DISABLED, NONE},
	{   9, DISABLED, NONE},
	{  10, DISABLED, NONE},
	{  11, DISABLED, NONE},
	{  12, DISABLED, NONE},
	{  13, DISABLED, NONE},
	{  14, DISABLED, NONE},
	{  15, DISABLED, NONE},
	{  16, DISABLED, NONE},
	{  17, DISABLED, NONE},
	{  18, DISABLED, NONE},
	{  19, DISABLED, NONE},
	{  20, DISABLED, NONE},
	{  21, DISABLED, NONE},
	{  22, DISABLED, NONE},
	{  23, DISABLED, NONE},
	/* Be careful and don't write past the end... */
};

static void setup_ioapic(void)
{
	int i;
	unsigned long value_low, value_high;
	unsigned long ioapic_base = 0xfec00000;
	volatile unsigned long *l;
	struct ioapicreg *a = ioapicregvalues;

	l = (unsigned long *) ioapic_base;

	for (i = 0; i < sizeof(ioapicregvalues) / sizeof(ioapicregvalues[0]);
	     i++, a++) {
		l[0] = (a->reg * 2) + 0x10;
		l[4] = a->value_low;
		value_low = l[4];
		l[0] = (a->reg *2) + 0x11;
		l[4] = a->value_high;
		value_high = l[4];
		if ((i==0) && (value_low == 0xffffffff)) {
			printk_warning("IO APIC not responding.\n");
			return;
		}
		printk_spew("for IRQ, reg 0x%08x value 0x%08x 0x%08x\n", 
			a->reg, a->value_low, a->value_high);
	}
}

static void lpc_init(struct device *dev)
{
	uint8_t byte;
	uint16_t word;
	int pwr_on=-1;

	printk_debug("lpc_init\n");

	/* IO APIC initialization */
	byte = pci_read_config8(dev, 0x4B);
	byte |= 1;
	pci_write_config8(dev, 0x4B, byte);
	setup_ioapic();

	/* posted memory write enable */
	byte = pci_read_config8(dev, 0x46);
	pci_write_config8(dev, 0x46, byte | (1<<0));

//BY LYH
        /* Disable AC97 and Ethernet */
        word = pci_read_config16(dev, 0x48);
        pci_write_config16(dev, 0x48, word & ~((1<<5)|(1<<6)|(1<<9)));
//BY LYH END
 

	/* power after power fail */
	byte = pci_read_config8(dev, 0x43);
	if (pwr_on) { 
		byte &= ~(1<<6);
	} else {
		byte |= (1<<6);
	}
	pci_write_config8(dev, 0x43, byte);


}

static struct device_operations lpc_ops  = {
	.read_resources = pci_dev_read_resources,
	.set_resources  = pci_dev_set_resources,
	.init = lpc_init,
	.scan_bus = 0,
};

static struct pci_driver lpc_driver __pci_driver = {
	.ops    = &lpc_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = PCI_DEVICE_ID_AMD_8111_ISA,
};
