/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Rudolf Marek <r.marek@assembler.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2 as published by
 * the Free Software Foundation.
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
#include <device/pciexp.h>
#include <device/pci_ids.h>

static void peg_init(struct device *dev)
{
	u8 reg;

	printk_debug("Configuring PCIe PEG\n");
	dump_south(dev);

	/* Disable link. */
	reg = pci_read_config8(dev, 0x50);
	pci_write_config8(dev, 0x50, reg | 0x10);

	/* Award has 0xb, VIA recomends 0x4. */
	pci_write_config8(dev, 0xe1, 0xb);

	/*
	 * pci_write_config8(dev, 0xe2, 0x0);
	 * pci_write_config8(dev, 0xe3, 0x92);
	 */
	/* Disable scrambling bit 6 to 1. */
	pci_write_config8(dev, 0xc0, 0x43);

	/* Set replay timer limit. */
	pci_write_config8(dev, 0xb1, 0xf0);

	/* Bit0 = 1 SDP (Start DLLP) always at Lane0. */
	reg = pci_read_config8(dev, 0xb8);
	pci_write_config8(dev, 0xb8, reg | 0x1);

	/*
	 * Downstream wait and Upstream Checking Malformed TLP through
	 * "Byte Enable Rule" And "Over 4K Boundary Rule".
	 */
	reg = pci_read_config8(dev, 0xa4);
	pci_write_config8(dev, 0xa4, reg | 0x30);

	/* Enable link. */
	reg = pci_read_config8(dev, 0x50);
	pci_write_config8(dev, 0x50, reg & ~0x10);

	/* Retrain link. */
	reg = pci_read_config8(dev, 0x50);
	pci_write_config8(dev, 0x50, reg | 0x20);

	reg = pci_read_config8(dev, 0x3e);
	reg |= 0x40;		/* Bus reset. */
	pci_write_config8(dev, 0x3e, reg);

	reg = pci_read_config8(dev, 0x3e);
	reg &= ~0x40;		/* Clear reset. */
	pci_write_config8(dev, 0x3e, reg);

	dump_south(dev);
}

static void pcie_init(struct device *dev)
{
	u8 reg;

	printk_debug("Configuring PCIe PEXs\n");
	dump_south(dev);

	/* Disable link. */
	reg = pci_read_config8(dev, 0x50);
	pci_write_config8(dev, 0x50, reg | 0x10);

	/* Award has 0xb, VIA recommends 0x4. */
	pci_write_config8(dev, 0xe1, 0xb);
	/* Set replay timer limit. */
	pci_write_config8(dev, 0xb1, 0xf0);

	/* Enable link. */
	reg = pci_read_config8(dev, 0x50);
	pci_write_config8(dev, 0x50, reg & ~0x10);

	/* Retrain. */
	reg = pci_read_config8(dev, 0x50);
	pci_write_config8(dev, 0x50, reg | 0x20);

	reg = pci_read_config8(dev, 0x3e);
	reg |= 0x40;		/* Bus reset. */
	pci_write_config8(dev, 0x3e, reg);

	reg = pci_read_config8(dev, 0x3e);
	reg &= ~0x40;		/* Clear reset. */
	pci_write_config8(dev, 0x3e, reg);

	dump_south(dev);
}

static const struct device_operations peg_ops = {
	.read_resources		= pci_bus_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.enable			= peg_init,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.ops_pci		= 0,
};

static const struct device_operations pcie_ops = {
	.read_resources		= pci_bus_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.enable			= pcie_init,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.ops_pci		= 0,
};

static const struct pci_driver northbridge_driver __pci_driver = {
	.ops	= &peg_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_K8T890CE_PEG,
};

static const struct pci_driver pcie_drvd3f0 __pci_driver = {
	.ops	= &pcie_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_K8T890CE_PEX0,
};

static const struct pci_driver pcie_drvd3f1 __pci_driver = {
	.ops	= &pcie_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_K8T890CE_PEX1,
};

static const struct pci_driver pcie_drvd3f2 __pci_driver = {
	.ops	= &pcie_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_K8T890CE_PEX2,
};

static const struct pci_driver pcie_drvd3f3 __pci_driver = {
	.ops	= &pcie_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_K8T890CE_PEX3,
};
