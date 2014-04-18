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

#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>
#include <console/console.h>

/* This fine tunes the HT link settings, which were loaded by ROM strap. */
static void host_ctrl_enable(struct device *dev)
{
	dump_south(dev);

	/*
	 * Bit 4 is reserved but set by AW. Set PCI to HT outstanding
	 * requests to 3.
	 */
	pci_write_config8(dev, 0xa0, 0x13);

	/* Disable NVRAM and enable non-posted PCI writes. */
	pci_write_config8(dev, 0xa1, 0x8e);

	/*
	 * NVRAM I/O base 0xe00-0xeff, but it is disabled.
	 * Some bits are set and reserved.
	 */
	pci_write_config8(dev, 0xa2, 0x0e);
	/* Arbitration control, some bits are reserved. */
	pci_write_config8(dev, 0xa5, 0x3c);

	/* Arbitration control 2 */
	pci_write_config8(dev, 0xa6, 0x80);

	writeback(dev, 0xa0, 0x13);	/* Bit4 is reserved! */
	writeback(dev, 0xa1, 0x8e);	/* Some bits are reserved. */
	writeback(dev, 0xa2, 0x0e);	/* I/O NVRAM base 0xe00-0xeff disabled. */
	writeback(dev, 0xa3, 0x31);
	writeback(dev, 0xa4, 0x30);

	writeback(dev, 0xa5, 0x3c);	/* Some bits reserved. */
	writeback(dev, 0xa6, 0x80);	/* Some bits reserved. */
	writeback(dev, 0xa7, 0x86);	/* Some bits reserved. */
	writeback(dev, 0xa8, 0x7f);	/* Some bits reserved. */
	writeback(dev, 0xa9, 0xcf);	/* Some bits reserved. */
	writeback(dev, 0xaa, 0x44);
	writeback(dev, 0xab, 0x22);
	writeback(dev, 0xac, 0x35);	/* Maybe bit0 is read-only? */

	writeback(dev, 0xae, 0x22);
	writeback(dev, 0xaf, 0x40);
	/* b0 is missing. */
	writeback(dev, 0xb1, 0x13);
	writeback(dev, 0xb4, 0x02);	/* Some bits are reserved. */
	writeback(dev, 0xc0, 0x20);
	writeback(dev, 0xc1, 0xaa);
	writeback(dev, 0xc2, 0xaa);
	writeback(dev, 0xc3, 0x02);
	writeback(dev, 0xc4, 0x50);
	writeback(dev, 0xc5, 0x50);

	dump_south(dev);
}

static const struct device_operations host_ctrl_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.enable			= host_ctrl_enable,
	.ops_pci		= 0,
};

static const struct pci_driver northbridge_driver __pci_driver = {
	.ops	= &host_ctrl_ops,
	.vendor	= PCI_VENDOR_ID_VIA,
	.device	= PCI_DEVICE_ID_VIA_K8T890CE_2,
};
