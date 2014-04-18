/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2004 Tyan Computer
 * Written by Yinghai Lu <yhlu@tyan.com> for Tyan Computer.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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
#include "ck804.h"

static void ide_init(struct device *dev)
{
	struct southbridge_nvidia_ck804_config *conf;
	uint32_t dword;
	uint16_t word;
	uint8_t byte;

	conf = dev->chip_info;

	word = pci_read_config16(dev, 0x50);
	/* Ensure prefetch is disabled. */
	word &= ~((1 << 15) | (1 << 13));
	if (conf->ide1_enable) {
		/* Enable secondary IDE interface. */
		word |= (1 << 0);
		printk(BIOS_DEBUG, "IDE1 \t");
	}
	if (conf->ide0_enable) {
		/* Enable primary IDE interface. */
		word |= (1 << 1);
		printk(BIOS_DEBUG, "IDE0\n");
	}

	word |= (1 << 12);
	word |= (1 << 14);

	pci_write_config16(dev, 0x50, word);

	byte = 0x20;		/* Latency: 64 --> 32 */
	pci_write_config8(dev, 0xd, byte);

	dword = pci_read_config32(dev, 0xf8);
	dword |= 12;
	pci_write_config32(dev, 0xf8, dword);

#if CONFIG_PCI_ROM_RUN == 1
	pci_dev_init(dev);
#endif
}

static void lpci_set_subsystem(device_t dev, unsigned vendor, unsigned device)
{
	pci_write_config32(dev, 0x40,
			   ((device & 0xffff) << 16) | (vendor & 0xffff));
}

static struct pci_operations lops_pci = {
	.set_subsystem = lpci_set_subsystem,
};

static struct device_operations ide_ops = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = ide_init,
	.scan_bus         = 0,
	// .enable        = ck804_enable,
	.ops_pci          = &lops_pci,
};

static const struct pci_driver ide_driver __pci_driver = {
	.ops    = &ide_ops,
	.vendor = PCI_VENDOR_ID_NVIDIA,
	.device = PCI_DEVICE_ID_NVIDIA_CK804_IDE,
};
