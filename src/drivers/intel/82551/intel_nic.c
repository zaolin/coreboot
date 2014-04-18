/* Copyright 2003 Tyan */

/* Author: Yinghai Lu 
 *
 */


#include <delay.h>
#include <stdlib.h>
#include <string.h>
#include <arch/io.h>

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>


static void intel_nic_init(struct device *dev)
{
	uint16_t word;

        word = pci_read_config16(dev, 0x4);
        word |= ((1 << 2) |(1<<4)); // Command: 3--> 17
        pci_write_config16(dev, 0x4, word);

	printk_debug("INTEL_NIC_FIXUP:  done  \n");
	
	
}

static struct device_operations intel_nic_ops  = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = intel_nic_init,
	.scan_bus         = 0,
};

static struct pci_driver intel_nic_driver __pci_driver = {
	.ops    = &intel_nic_ops,
	.vendor = PCI_VENDOR_ID_INTEL,
	.device = 0x1010,
};
static struct pci_driver intel_nic2_driver __pci_driver = {
        .ops    = &intel_nic_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x100f,
};
 
