/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
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

#include <types.h>
#include <lib.h>
#include <console.h>
#include <device/pci.h>
#include <msr.h>
#include <legacy.h>
#include <device/pci_ids.h>
#include <statictree.h>
#include <config.h>
#include "sb600.h"

static void lpc_init(struct device * dev)
{
	u8 byte;
	u32 dword;
	struct device * sm_dev;

	/* Enable the LPC Controller */
	sm_dev = dev_find_slot(0, PCI_DEVFN(0x14, 0));
	dword = pci_read_config32(sm_dev, 0x64);
	dword |= 1 << 20;
	pci_write_config32(sm_dev, 0x64, dword);

	/* Initialize isa dma */
	isa_dma_init();

	/* RPR 7.2 Enable DMA transaction on the LPC bus */
	byte = pci_read_config8(dev, 0x40);
	byte |= (1 << 2);
	pci_write_config8(dev, 0x40, byte);

	/* RPR 7.3 Disable the timeout mechanism on LPC */
	byte = pci_read_config8(dev, 0x48);
	byte &= ~(1 << 7);
	pci_write_config8(dev, 0x48, byte);

	/* RPR 7.5 Disable LPC MSI Capability */
	byte = pci_read_config8(dev, 0x78);
	byte &= ~(1 << 1);
	pci_write_config8(dev, 0x78, byte);

}

static void sb600_lpc_read_resources(struct device * dev)
{
	struct resource *res;

	/* Get the normal pci resources of this device */
	pci_dev_read_resources(dev);	/* We got one for APIC, or one more for TRAP */

	pci_get_resource(dev, 0xA0); /* SPI ROM base address */

	/* Add an extra subtractive resource for both memory and I/O */
	res = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	res->flags =
	    IORESOURCE_IO | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;

	res = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
	res->flags =
	    IORESOURCE_MEM | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;

	compact_resources(dev);
}

/**     
 * @brief Enable resources for children devices
 *      
 * @param dev the device whos children's resources are to be enabled
 *      
 * This function is call by the global enable_resources() indirectly via the
 * device_operation::enable_resources() method of devices.
 *      
 * Indirect mutual recursion:
 *      enable_childrens_resources() -> enable_resources()
 *      enable_resources() -> device_operation::enable_resources()
 *      device_operation::enable_resources() -> enable_children_resources()
 */
static void sb600_lpc_enable_childrens_resources(struct device * dev)
{
	u32 link;
	u32 reg, reg_x;
	int i;
	int var_num = 0;
	u16 reg_var[3];

	reg = pci_read_config32(dev, 0x44);
	reg_x = pci_read_config32(dev, 0x48);

	for (link = 0; link < dev->links; link++) {
		struct device * child;
		for (child = dev->link[link].children; child;
		     child = child->sibling) {
			enable_resources(child);
			if (child->have_resources
			    && (child->path.type == DEVICE_PATH_PNP)) {
				for (i = 0; i < child->resources; i++) {
					struct resource *res;
					unsigned long base, end;	/*  don't need long long */
					res = &child->resource[i];
					if (!(res->flags & IORESOURCE_IO))
						continue;
					base = res->base;
					end = resource_end(res);
					printk_debug
					    ("sb600 lpc decode:%s, base=0x%08x, end=0x%08x\n",
					     dev_path(child), base, end);
					switch (base) {
					case 0x60:	/*  KB */
					case 0x64:	/*  MS */
						reg |= (1 << 29);
						break;
					case 0x3f8:	/*  COM1 */
						reg |= (1 << 6);
						break;
					case 0x2f8:	/*  COM2 */
						reg |= (1 << 7);
						break;
					case 0x378:	/*  Parallal 1 */
						reg |= (1 << 0);
						break;
					case 0x3f0:	/*  FD0 */
						reg |= (1 << 26);
						break;
					case 0x220:	/*  Aduio 0 */
						reg |= (1 << 8);
						break;
					case 0x300:	/*  Midi 0 */
						reg |= (1 << 18);
						break;
					case 0x400:
						reg_x |= (1 << 16);
						break;
					case 0x480:
						reg_x |= (1 << 17);
						break;
					case 0x500:
						reg_x |= (1 << 18);
						break;
					case 0x580:
						reg_x |= (1 << 19);
						break;
					case 0x4700:
						reg_x |= (1 << 22);
						break;
					case 0xfd60:
						reg_x |= (1 << 23);
						break;
					      default:
						if (var_num >= 3)
							continue;	/* only 3 var ; compact them ? */
						switch (var_num) {
						case 0:
							reg_x |= (1 << 2);
							break;
						case 1:
							reg_x |= (1 << 24);
							break;
						case 2:
							reg_x |= (1 << 25);
							break;
						}
						reg_var[var_num++] =
						    base & 0xffff;
					}
				}
			}
		}
	}
	pci_write_config32(dev, 0x44, reg);
	pci_write_config32(dev, 0x48, reg_x);
	switch (var_num) {
	case 2:
		pci_write_config16(dev, 0x90, reg_var[2]);
	case 1:
		pci_write_config16(dev, 0x66, reg_var[1]);
	case 0:
		pci_write_config16(dev, 0x64, reg_var[0]);
		break;
	}
}

static void sb600_lpc_enable_resources(struct device * dev)
{
	pci_dev_enable_resources(dev);
	sb600_lpc_enable_childrens_resources(dev);
}

static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

struct device_operations sb600_lpc = {
	.id = {.type = DEVICE_ID_PCI,
		{.pci = {.vendor = PCI_VENDOR_ID_AMD,
			      .device = PCI_DEVICE_ID_AMD_8111_IDE}}},
	.constructor		 = default_device_constructor,
	.phase3_scan_bus	= scan_status_bus,
	.phase4_read_resources	 = sb600_lpc_read_resources,
	.phase4_set_resources	 = pci_dev_set_resources,
	.phase5_enable_resources = sb600_lpc_enable_resources,
	.phase6_init		 = lpc_init,
	.ops_pci          = &lops_pci
};
