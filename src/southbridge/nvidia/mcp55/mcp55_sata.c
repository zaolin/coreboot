/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2004 Tyan Computer
 * Written by Yinghai Lu <yhlu@tyan.com> for Tyan Computer.
 * Copyright (C) 2006,2007 AMD
 * Written by Yinghai Lu <yinghai.lu@amd.com> for AMD.
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
#include <delay.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "mcp55.h"

static void sata_init(struct device *dev)
{
	uint32_t dword;

	struct southbridge_nvidia_mcp55_config *conf;
	conf = dev->chip_info;

	dword = pci_read_config32(dev, 0x50);
	/* Ensure prefetch is disabled */
	dword &= ~((1 << 15) | (1 << 13));
	if(conf) {
		if (conf->sata1_enable) {
			/* Enable secondary SATA interface */
			dword |= (1<<0);
			printk_debug("SATA S \t");
		}
		if (conf->sata0_enable) {
			/* Enable primary SATA interface */
			dword |= (1<<1);
			printk_debug("SATA P \n");
		}
	} else {
		dword |= (1<<1) | (1<<0);
		printk_debug("SATA P and S \n");
	}


#if 1
	dword &= ~(0x1f<<24);
	dword |= (0x15<<24);
#endif
	pci_write_config32(dev, 0x50, dword);

	dword = pci_read_config32(dev, 0xf8);
	dword |= 2;
	pci_write_config32(dev, 0xf8, dword);


}

static void lpci_set_subsystem(device_t dev, unsigned vendor, unsigned device)
{
	pci_write_config32(dev, 0x40,
		((device & 0xffff) << 16) | (vendor & 0xffff));
}
static struct pci_operations lops_pci = {
	.set_subsystem	= lpci_set_subsystem,
};

static struct device_operations sata_ops = {
	.read_resources	= pci_dev_read_resources,
	.set_resources	= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
//	.enable		= mcp55_enable,
	.init		= sata_init,
	.scan_bus	= 0,
	.ops_pci	= &lops_pci,
};

static const struct pci_driver sata0_driver __pci_driver = {
	.ops	= &sata_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_SATA0,
};

static const struct pci_driver sata1_driver __pci_driver = {
	.ops	= &sata_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_SATA1,
};
