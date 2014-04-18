/*
 * Initialisation of the PCI bridge .
 */

#include <arch/io.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <console/console.h>

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

static void pci_domain_set_resources(device_t dev)
{
	int idx = 3; /* who knows? */

	ram_resource(dev, idx, 0, CONFIG_EMBEDDED_RAM_SIZE>>10);
	assign_resources(&dev->link[0]);
}

struct device_operations pci_domain_ops  = {
        .read_resources   = pci_domain_read_resources,
        .set_resources    = pci_domain_set_resources,
        .enable_resources = enable_childrens_resources,
        .init             = 0,
        .scan_bus         = pci_domain_scan_bus,
	.ops_pci_bus	  = &pci_ppc_conf1
};

static void enable_dev(struct device *dev)
{
        /* Set the operations if it is a special bus type */
        if (dev->path.type == DEVICE_PATH_PCI_DOMAIN) {
                dev->ops = &pci_domain_ops;
        }
}

struct chip_operations cpu_ppc_ppc4xx_ops = {
        CHIP_NAME("PPC 4XX CPU")
        .enable_dev = enable_dev,
};
