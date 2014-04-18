/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Corey Osgood <corey.osgood@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "i82801xx.h"

static void usb_init(struct device *dev)
{
/* TODO: Any init needed? Some ports have it, others don't */
}

static struct device_operations usb_ops  = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = usb_init,
	.scan_bus         = 0,
	.enable           = i82801xx_enable,
};

/* i82801aa */
static struct pci_driver i82801aa_usb_1 __pci_driver = {
	.ops    = &usb_ops,
	.vendor = PCI_VENDOR_ID_INTEL,
	.device = 0x2412,
};

/* i82801ab */
static struct pci_driver i82801ab_usb_1 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x2422,
};

/* i82801ba */
static struct pci_driver i82801ba_usb_1 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x2442,
};

static struct pci_driver i82801ba_usb_2 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x2444,
};

/* i82801ca */
static struct pci_driver i82801ca_usb_1 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x2482,
};

static struct pci_driver i82801ca_usb_2 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x2484,
};

static struct pci_driver i82801ca_usb_3 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x2487,
};

/* i82801db and i82801dbm */
static struct pci_driver i82801db_usb_1 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24c2,
};

static struct pci_driver i82801db_usb_2 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24c4,
};

static struct pci_driver i82801db_usb_3 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24c7,
};

/* i82801eb and i82801er */
static struct pci_driver i82801ex_usb_1 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24d2,
};

static struct pci_driver i82801ex_usb_2 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24d4,
};

static struct pci_driver i82801ex_usb_3 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24d7,
};

static struct pci_driver i82801ex_usb_4 __pci_driver = {
        .ops    = &usb_ops,
        .vendor = PCI_VENDOR_ID_INTEL,
        .device = 0x24de,
};
