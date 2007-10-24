/*
 * Copyright  2005 AMD
 *  by yinghai.lu@amd.com
 */

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>

static void pcie_init(struct device *dev)
{

	/* Enable pci error detecting */
	uint32_t dword;

	/* System error enable */
	dword = pci_read_config32(dev, 0x04);
	dword |= (1<<8); /* System error enable */
	dword |= (1<<30); /* Clear possible errors */
	pci_write_config32(dev, 0x04, dword);

}

static struct pci_operations lops_pci = {
        .set_subsystem = 0,
};

static struct device_operations pcie_ops  = {
        .read_resources   = pci_bus_read_resources,
        .set_resources    = pci_dev_set_resources,
        .enable_resources = pci_bus_enable_resources,
        .init             = pcie_init,
        .scan_bus         = pci_scan_bridge,
        .reset_bus        = pci_bus_reset,
        .ops_pci          = &lops_pci,

};

static const struct pci_driver pcie_driver __pci_driver = {
	.ops    = &pcie_ops,
	.vendor = PCI_VENDOR_ID_SERVERWORKS,
	.device = PCI_DEVICE_ID_SERVERWORKS_BCM5780_PCIE,
};

