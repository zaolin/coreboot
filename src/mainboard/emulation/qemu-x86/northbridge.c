#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include "chip.h"
#include <delay.h>

static void ram_resource(device_t dev, unsigned long index,
	unsigned long basek, unsigned long sizek)
{
	struct resource *resource;

	if (!sizek) {
		return;
	}
	resource = new_resource(dev, index);
	resource->base	= ((resource_t)basek) << 10;
	resource->size	= ((resource_t)sizek) << 10;
	resource->flags =  IORESOURCE_MEM | IORESOURCE_CACHEABLE | \
		IORESOURCE_FIXED | IORESOURCE_STORED | IORESOURCE_ASSIGNED;
}

static void tolm_test(void *gp, struct device *dev, struct resource *new)
{
	struct resource **best_p = gp;
	struct resource *best;
	best = *best_p;
	if (!best || (best->base > new->base)) {
		best = new;
	}
	*best_p = best;
}

static uint32_t find_pci_tolm(struct bus *bus)
{
	struct resource *min;
	uint32_t tolm;
	min = 0;
	search_bus_resources(bus, IORESOURCE_MEM, IORESOURCE_MEM, tolm_test, &min);
	tolm = 0xffffffffUL;
	if (min && tolm > min->base) {
		tolm = min->base;
	}
	return tolm;
}

#if CONFIG_WRITE_HIGH_TABLES==1
#define HIGH_TABLES_SIZE 64	// maximum size of high tables in KB
extern uint64_t high_tables_base, high_tables_size;
#endif

#define CMOS_ADDR_PORT 0x70
#define CMOS_DATA_PORT 0x71
#define HIGH_RAM_ADDR 0x35
#define LOW_RAM_ADDR 0x34

static void cpu_pci_domain_set_resources(device_t dev)
{
	u32 pci_tolm = find_pci_tolm(&dev->link[0]);
	unsigned long tomk = 0, tolmk;
	int idx;

	outb (HIGH_RAM_ADDR, CMOS_ADDR_PORT);
	tomk = ((unsigned long) inb(CMOS_DATA_PORT)) << 14;
	outb (LOW_RAM_ADDR, CMOS_ADDR_PORT);
	tomk |= ((unsigned long) inb(CMOS_DATA_PORT)) << 6;
	tomk += 16 * 1024;

	printk(BIOS_DEBUG, "Detected %lu Kbytes (%lu MiB) RAM.\n",
	       tomk, tomk / 1024);

	/* Compute the top of Low memory */
	tolmk = pci_tolm >> 10;
	if (tolmk >= tomk) {
		/* The PCI hole does not overlap the memory. */
		tolmk = tomk;
	}

	/* Report the memory regions. */
	idx = 10;
	ram_resource(dev, idx++, 0, 640);
	ram_resource(dev, idx++, 768, tolmk - 768);

#if CONFIG_WRITE_HIGH_TABLES==1
	/* Leave some space for ACPI, PIRQ and MP tables */
	high_tables_base = (tomk - HIGH_TABLES_SIZE) * 1024;
	high_tables_size = HIGH_TABLES_SIZE * 1024;
#endif

	assign_resources(&dev->link[0]);
}

static void cpu_pci_domain_read_resources(struct device *dev)
{
	struct resource *res;

	pci_domain_read_resources(dev);

	/* Reserve space for the IOAPIC.  This should be in the Southbridge,
	 * but I couldn't tell which device to put it in. */
	res = new_resource(dev, 2);
	res->base = 0xfec00000UL;
	res->size = 0x100000UL;
	res->limit = 0xffffffffUL;
	res->flags = IORESOURCE_MEM | IORESOURCE_FIXED | IORESOURCE_STORED |
		     IORESOURCE_ASSIGNED;

	/* Reserve space for the LAPIC.  There's one in every processor, but
	 * the space only needs to be reserved once, so we do it here. */
	res = new_resource(dev, 3);
	res->base = 0xfee00000UL;
	res->size = 0x10000UL;
	res->limit = 0xffffffffUL;
	res->flags = IORESOURCE_MEM | IORESOURCE_FIXED | IORESOURCE_STORED |
		     IORESOURCE_ASSIGNED;
}

static struct device_operations pci_domain_ops = {
	.read_resources		= cpu_pci_domain_read_resources,
	.set_resources		= cpu_pci_domain_set_resources,
	.enable_resources	= enable_childrens_resources,
	.init			= 0,
	.scan_bus		= pci_domain_scan_bus,
};

static void enable_dev(struct device *dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_PCI_DOMAIN) {
		dev->ops = &pci_domain_ops;
		pci_set_method(dev);
	}
}

struct chip_operations mainboard_emulation_qemu_x86_ops = {
	CHIP_NAME("QEMU Northbridge")
	.enable_dev = enable_dev,
};
