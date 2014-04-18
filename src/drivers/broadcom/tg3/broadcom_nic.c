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

static void broadcom_nic_init(struct device *dev)
{
	uint16_t word;

        word = pci_read_config16(dev, 0x4);
        word |= ((1 << 2) |(1<<4)); // Command: 3--> 17
        pci_write_config16(dev, 0x4, word);

	printk_debug("BROADCOM_NIC_FIXUP:  done  \n");
	
	
}

static struct device_operations broadcom_nic_ops  = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = broadcom_nic_init,
	.scan_bus         = 0,
};

static struct pci_driver broadcom_nic_driver __pci_driver = {
	.ops    = &broadcom_nic_ops,
	.vendor = PCI_VENDOR_ID_BROADCOM,
	.device = PCI_DEVICE_ID_TIGON3_5703X,
};
static struct pci_driver broadcom_nic2_driver __pci_driver = {
        .ops    = &broadcom_nic_ops,
        .vendor = PCI_VENDOR_ID_BROADCOM,
        .device = 0x1653,
};
static struct pci_driver broadcom_nic3_driver __pci_driver = {
        .ops    = &broadcom_nic_ops,
        .vendor = PCI_VENDOR_ID_BROADCOM,
        .device = 0x4401,
};

 
