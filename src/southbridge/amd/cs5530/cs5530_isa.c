/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Uwe Hermann <uwe@hermann-uwe.de>
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
#include <arch/io.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include "cs5530.h"

static void isa_init(struct device *dev)
{
	uint8_t reg8;

	// TODO: Test if needed, otherwise drop.

	/* Set positive decode on ROM. */
	reg8 = pci_read_config8(dev, DECODE_CONTROL_REG2);
	reg8 |= BIOS_ROM_POSITIVE_DECODE;
	pci_write_config8(dev, DECODE_CONTROL_REG2, reg8);
}

static void cs5530_pci_dev_enable_resources(device_t dev)
{
	// TODO: Needed?
	pci_dev_enable_resources(dev);
	enable_childrens_resources(dev);
}

static struct device_operations isa_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= cs5530_pci_dev_enable_resources,
	.init			= isa_init,
	.enable			= 0,
	.scan_bus		= scan_static_bus,
};

static const struct pci_driver isa_driver __pci_driver = {
	.ops	= &isa_ops,
	.vendor	= PCI_VENDOR_ID_CYRIX,
	.device	= PCI_DEVICE_ID_CYRIX_5530_LEGACY,
};
