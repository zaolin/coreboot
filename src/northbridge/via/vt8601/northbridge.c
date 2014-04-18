#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/hypertransport.h>
#include <cpu/cpu.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include "chip.h"
#include "northbridge.h"

/*
 * This fixup is based on capturing values from an Award bios.  Without
 * this fixup the DMA write performance is awful (i.e. hdparm -t /dev/hda is 20x
 * slower than normal, ethernet drops packets).
 * Apparently these registers govern some sort of bus master behavior.
 */
static void northbridge_init(device_t dev) 
{
	printk_spew("VT8601 random fixup ...\n");
	pci_write_config8(dev, 0x70, 0xc0);
	pci_write_config8(dev, 0x71, 0x88);
	pci_write_config8(dev, 0x72, 0xec);
	pci_write_config8(dev, 0x73, 0x0c);
	pci_write_config8(dev, 0x74, 0x0e);
	pci_write_config8(dev, 0x75, 0x81);
	pci_write_config8(dev, 0x76, 0x52);
}

static struct device_operations northbridge_operations = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = northbridge_init,
	.enable           = 0,
	.ops_pci          = 0,
};

static const struct pci_driver northbridge_driver __pci_driver = {
	.ops = &northbridge_operations,
	.vendor = PCI_VENDOR_ID_VIA,
	.device = 0x0601, /* 0x8601 is the AGP bridge? */
};

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

#if CONFIG_WRITE_HIGH_TABLES==1
/* maximum size of high tables in KB */
#define HIGH_TABLES_SIZE 64
extern uint64_t high_tables_base, high_tables_size;
#endif

static void pci_domain_set_resources(device_t dev)
{
	static const uint8_t ramregs[] = {
		0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x56, 0x57
	};
	device_t mc_dev;
        uint32_t pci_tolm;

        pci_tolm = find_pci_tolm(&dev->link[0]);
	mc_dev = dev->link[0].children;
	if (mc_dev) {
		unsigned long tomk, tolmk;
		unsigned char rambits;
		int i, idx;

		for(rambits = 0, i = 0; i < ARRAY_SIZE(ramregs); i++) {
			unsigned char reg;
			reg = pci_read_config8(mc_dev, ramregs[i]);
			/* these are ENDING addresses, not sizes. 
			 * if there is memory in this slot, then reg will be > rambits.
			 * So we just take the max, that gives us total. 
			 * We take the highest one to cover for once and future coreboot
			 * bugs. We warn about bugs.
			 */
			if (reg > rambits)
				rambits = reg;
			if (reg < rambits)
				printk_err("ERROR! register 0x%x is not set!\n", 
					ramregs[i]);
		}
		printk_debug("I would set ram size to 0x%x Kbytes\n", (rambits)*8*1024);
		tomk = rambits*8*1024;
		/* Compute the top of Low memory */
		tolmk = pci_tolm >> 10;
		if (tolmk >= tomk) {
			/* The PCI hole does does not overlap the memory.
			 */
			tolmk = tomk;
		}

#if CONFIG_WRITE_HIGH_TABLES == 1
		high_tables_base = (tolmk - HIGH_TABLES_SIZE) * 1024;
		high_tables_size = HIGH_TABLES_SIZE* 1024;
		printk_debug("tom: %lx, high_tables_base: %llx, high_tables_size: %llx\n", tomk*1024, high_tables_base, high_tables_size);
#endif

		/* Report the memory regions */
		idx = 10;
		ram_resource(dev, idx++, 0, tolmk);
	}
	assign_resources(&dev->link[0]);
}

static struct device_operations pci_domain_ops = {
        .read_resources   = pci_domain_read_resources,
        .set_resources    = pci_domain_set_resources,
        .enable_resources = enable_childrens_resources,
        .init             = 0,
        .scan_bus         = pci_domain_scan_bus,
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
		pci_set_method(dev);
        }
        else if (dev->path.type == DEVICE_PATH_APIC_CLUSTER) {
                dev->ops = &cpu_bus_ops;
        }
}

struct chip_operations northbridge_via_vt8601_ops = {
	CHIP_NAME("VIA VT8601 Northbridge")
	.enable_dev = enable_dev, 
};
