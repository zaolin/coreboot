#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <cpu/cpu.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include "chip.h"

static void pci_domain_read_resources(device_t dev)
{
        struct resource *resource;
        unsigned reg;

        /* Initialize the system wide io space constraints */
        resource = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	resource->base = 0x400; //yhlu
        resource->limit = 0xffffUL;
        resource->flags = IORESOURCE_IO | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;

        /* Initialize the system wide memory resources constraints */
        resource = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
        resource->limit = 0xffffffffULL;
        resource->flags = IORESOURCE_MEM | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;
}

static void ram_resource(device_t dev, unsigned long index,
        unsigned long basek, unsigned long sizek)
{
        struct resource *resource;

        if (!sizek) {
                return;
        }
        resource = new_resource(dev, index);
        resource->base  = ((resource_t)basek) << 10;
        resource->size  = ((resource_t)sizek) << 10;
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

static void pci_domain_set_resources(device_t dev)
{
	device_t mc_dev;
        uint32_t pci_tolm;

        pci_tolm = find_pci_tolm(&dev->link[0]);
	mc_dev = dev->link[0].children;
	if (mc_dev) {
		/* Figure out which areas are/should be occupied by RAM.
		 * This is all computed in kilobytes and converted to/from
		 * the memory controller right at the edges.
		 * Having different variables in different units is
		 * too confusing to get right.  Kilobytes are good up to
		 * 4 Terabytes of RAM...
		 */
		uint16_t tolm_r, remapbase_r, remaplimit_r;
		unsigned long tomk, tolmk;
		unsigned long remapbasek, remaplimitk;
		int idx;

		/* Get the value of the highest DRB. This tells the end of
		 * the physical memory.  The units are ticks of 64MB
		 * i.e. 1 means 64MB.
		 */
		tomk = ((unsigned long)pci_read_config8(mc_dev, 0x67)) << 16;
		/* Compute the top of Low memory */
		tolmk = pci_tolm >> 10;
		if (tolmk >= tomk) {
			/* The PCI hole does not overlap memory
			 * we won't use the remap window.
			 */
			tolmk = tomk;
			remapbasek   = 0x3ff << 16;
			remaplimitk  = 0 << 16;
		}
		else {
			/* The PCI memory hole overlaps memory
			 * setup the remap window.
			 */
			/* Find the bottom of the remap window
			 * is it above 4G?
			 */
			remapbasek = 4*1024*1024;
			if (tomk > remapbasek) {
				remapbasek = tomk;
			}
			/* Find the limit of the remap window */
			remaplimitk = (remapbasek + (4*1024*1024 - tolmk) - (1 << 16));
		}
		/* Write the ram configuration registers,
		 * preserving the reserved bits.
		 */
		tolm_r = pci_read_config16(mc_dev, 0xc4);
		tolm_r = ((tolmk >> 17) << 11) | (tolm_r & 0x7ff);
		pci_write_config16(mc_dev, 0xc4, tolm_r);

		remapbase_r = pci_read_config16(mc_dev, 0xc6);
		remapbase_r = (remapbasek >> 16) | (remapbase_r & 0xfc00);
		pci_write_config16(mc_dev, 0xc6, remapbase_r);
		
		remaplimit_r = pci_read_config16(mc_dev, 0xc8);
		remaplimit_r = (remaplimitk >> 16) | (remaplimit_r & 0xfc00);
		pci_write_config16(mc_dev, 0xc8, remaplimit_r);
		
		/* Report the memory regions */
		idx = 10;
		ram_resource(dev, idx++, 0, 640);
		ram_resource(dev, idx++, 768, tolmk - 768);
		if (tomk > 4*1024*1024) {
			ram_resource(dev, idx++, 4096*1024, tomk - 4*1024*1024);
		}
		if (remaplimitk >= remapbasek) {
			ram_resource(dev, idx++, remapbasek,
				(remaplimitk + 64*1024) - remapbasek);
		}
	}
	assign_resources(&dev->link[0]);
}

static unsigned int pci_domain_scan_bus(device_t dev, unsigned int max)
{
        max = pci_scan_bus(&dev->link[0], PCI_DEVFN(0, 0), 0xff, max);
        return max;
}

static struct device_operations pci_domain_ops = {
        .read_resources   = pci_domain_read_resources,
        .set_resources    = pci_domain_set_resources,
        .enable_resources = enable_childrens_resources,
        .init             = 0,
        .scan_bus         = pci_domain_scan_bus,
	.ops_pci_bus      = &pci_cf8_conf1,
};  

static void cpu_bus_init(device_t dev)
{
        initialize_cpus(&dev->link[0]);
}

static void cpu_bus_noop(device_t dev)
{
}

static struct device_operations cpu_bus_ops = {
        .read_resources   = cpu_bus_noop,
        .set_resources    = cpu_bus_noop,
        .enable_resources = cpu_bus_noop,
        .init             = cpu_bus_init,
        .scan_bus         = 0,
};

static void enable_dev(struct device *dev)
{
        /* Set the operations if it is a special bus type */
        if (dev->path.type == DEVICE_PATH_PCI_DOMAIN) {
                dev->ops = &pci_domain_ops;
        }
        else if (dev->path.type == DEVICE_PATH_APIC_CLUSTER) {
                dev->ops = &cpu_bus_ops;
        }
}

struct chip_operations northbridge_intel_e7501_ops = {
	CHIP_NAME("Intel E7501 Northbridge")
	.enable_dev = enable_dev,
};
