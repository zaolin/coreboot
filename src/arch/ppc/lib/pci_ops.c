#include <console/console.h>
#include <arch/pciconf.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>

static const struct pci_ops *conf;
struct pci_ops {
	uint8_t (*read8) (uint8_t bus, int devfn, int where);
	uint16_t (*read16) (uint8_t bus, int devfn, int where);
	uint32_t (*read32) (uint8_t bus, int devfn, int where);
	int (*write8) (uint8_t bus, int devfn, int where, uint8_t val);
	int (*write16) (uint8_t bus, int devfn, int where, uint16_t val);
	int (*write32) (uint8_t bus, int devfn, int where, uint32_t val);
};

struct pci_ops pci_direct_ppc;

/*
 * Before we decide to use direct hardware access mechanisms, we try to do some
 * trivial checks to ensure it at least _seems_ to be working -- we just test
 * whether bus 00 contains a host bridge (this is similar to checking
 * techniques used in XFree86, but ours should be more reliable since we
 * attempt to make use of direct access hints provided by the PCI BIOS).
 *
 * This should be close to trivial, but it isn't, because there are buggy
 * chipsets (yes, you guessed it, by Intel and Compaq) that have no class ID.
 */
static int pci_sanity_check(const struct pci_ops *o)
{
	uint16_t class, vendor;
	uint8_t bus;
	int devfn;
#define PCI_CLASS_BRIDGE_HOST           0x0600
#define PCI_CLASS_DISPLAY_VGA           0x0300
#define PCI_VENDOR_ID_COMPAQ            0x0e11
#define PCI_VENDOR_ID_INTEL             0x8086
#define PCI_VENDOR_ID_MOTOROLA          0x1057
#define PCI_VENDOR_ID_IBM	        0x1014

	for (bus = 0, devfn = 0; devfn < 0x100; devfn++) {
		class = o->read16(bus, devfn, PCI_CLASS_DEVICE);
		vendor = o->read16(bus, devfn, PCI_VENDOR_ID);
		if (((class == PCI_CLASS_BRIDGE_HOST) || 
		     (class == PCI_CLASS_DISPLAY_VGA)) ||
			((vendor == PCI_VENDOR_ID_INTEL) || 
			(vendor == PCI_VENDOR_ID_COMPAQ) ||
			(vendor == PCI_VENDOR_ID_MOTOROLA) ||
			(vendor == PCI_VENDOR_ID_IBM))) {
			return 1;
		}
	}

	printk_err("PCI: Sanity check failed\n");
	return 0;
}

uint8_t pci_read_config8(device_t dev, unsigned where)
{
	return conf->read8(dev->bus->secondary, dev->path.u.pci.devfn, where);
}

uint16_t pci_read_config16(struct device *dev, unsigned where)
{
	return conf->read16(dev->bus->secondary, dev->path.u.pci.devfn, where);
}

uint32_t pci_read_config32(struct device *dev, unsigned where)
{
	return conf->read32(dev->bus->secondary, dev->path.u.pci.devfn, where);
}

void pci_write_config8(struct device *dev, unsigned where, uint8_t val)
{
	conf->write8(dev->bus->secondary, dev->path.u.pci.devfn, where, val);
}

void pci_write_config16(struct device *dev, unsigned where, uint16_t val)
{
	conf->write16(dev->bus->secondary, dev->path.u.pci.devfn, where, val);
}

void pci_write_config32(struct device *dev, unsigned where, uint32_t val)
{
	conf->write32(dev->bus->secondary, dev->path.u.pci.devfn, where, val);
}

/** Set the method to be used for PCI
 */
void pci_set_method(void)
{
	conf = &pci_direct_ppc;
	pci_sanity_check(conf);
}

struct pci_ops pci_direct_ppc =
{
    pci_ppc_read_config8,
    pci_ppc_read_config16,
    pci_ppc_read_config32,
    pci_ppc_write_config8,
    pci_ppc_write_config16,
    pci_ppc_write_config32     
};

