/*
 * This file is part of the LinuxBIOS project.
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

/* Based on other VIA SB code. */

#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>
#include <console/console.h>
#include "vt8237r.h"
#include "chip.h"

/**
 * No native mode. Interrupts from unconnected HDDs might occur if
 * IRQ14/15 is used for PCI. Therefore no native mode support.
 */
static void ide_init(struct device *dev)
{
	struct southbridge_via_vt8237r_config *sb =
	    (struct southbridge_via_vt8237r_config *)dev->chip_info;

	u8 enables;
	u32 cablesel;

	printk_info("%s IDE interface %s\n", "Primary",
		    sb->ide0_enable ? "enabled" : "disabled");
	printk_info("%s IDE interface %s\n", "Secondary",
		    sb->ide1_enable ? "enabled" : "disabled");
	enables = pci_read_config8(dev, IDE_CS) & ~0x3;
	enables |= (sb->ide0_enable << 1) | sb->ide1_enable;
	pci_write_config8(dev, IDE_CS, enables);
	enables = pci_read_config8(dev, IDE_CS);
	printk_debug("Enables in reg 0x40 read back as 0x%x\n", enables);

	/* Enable only compatibility mode. */
	enables = pci_read_config8(dev, IDE_CONF_II);
	enables &= ~0xc0;
	pci_write_config8(dev, IDE_CONF_II, enables);
	enables = pci_read_config8(dev, IDE_CONF_II);
	printk_debug("Enables in reg 0x42 read back as 0x%x\n", enables);

	/* Enable prefetch buffers. */
	enables = pci_read_config8(dev, IDE_CONF_I);
	enables |= 0xf0;
	pci_write_config8(dev, IDE_CONF_I, enables);

	/* Flush FIFOs at half. */
	enables = pci_read_config8(dev, IDE_CONF_FIFO);
	enables &= 0xf0;
	enables |= (1 << 2) | (1 << 0);
	pci_write_config8(dev, IDE_CONF_FIFO, enables);

	/* PIO read prefetch counter, Bus Master IDE Status Reg. Read Retry. */
	enables = pci_read_config8(dev, IDE_MISC_I);
	enables &= 0xe2;
	enables |= (1 << 4) | (1 << 3);
	pci_write_config8(dev, IDE_MISC_I, enables);

	/* Use memory read multiple, Memory-Write-and-Invalidate. */
	enables = pci_read_config8(dev, IDE_MISC_II);
	enables |= (1 << 2) | (1 << 3);
	pci_write_config8(dev, IDE_MISC_II, enables);

	/* Force interrupts to use compat mode. */
	pci_write_config8(dev, PCI_INTERRUPT_PIN, 0x0);
	pci_write_config8(dev, PCI_INTERRUPT_LINE, 0xff);

	/* Cable guy... */
	cablesel = pci_read_config32(dev, IDE_UDMA);
	cablesel &= ~((1 << 28) | (1 << 20) | (1 << 12) | (1 << 4));
	cablesel |= (sb->ide0_80pin_cable << 28) |
		    (sb->ide0_80pin_cable << 20) |
		    (sb->ide1_80pin_cable << 12) |
		    (sb->ide1_80pin_cable << 4);
	pci_write_config32(dev, IDE_UDMA, cablesel);
}

static const struct device_operations ide_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= ide_init,
	.enable			= 0,
	.ops_pci		= 0,
};

static const struct pci_driver northbridge_driver __pci_driver = {
	.ops	= &ide_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_82C586_1,
};
