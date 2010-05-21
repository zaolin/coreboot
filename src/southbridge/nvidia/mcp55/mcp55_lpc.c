/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2003 Linux Networx
 * Copyright (C) 2003 SuSE Linux AG
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
#include <device/pci.h>
#include <device/pnp.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <pc80/mc146818rtc.h>
#include <pc80/isa-dma.h>
#include <bitops.h>
#include <arch/io.h>
#include <arch/ioapic.h>
#include <cpu/x86/lapic.h>
#include <stdlib.h>
#include "mcp55.h"

#define NMI_OFF	0

// 0x7a or e3
#define PREVIOUS_POWER_STATE	0x7A

#define MAINBOARD_POWER_OFF	0
#define MAINBOARD_POWER_ON	1
#define SLOW_CPU_OFF		0
#define SLOW_CPU__ON		1

#ifndef CONFIG_MAINBOARD_POWER_ON_AFTER_POWER_FAIL
#define CONFIG_MAINBOARD_POWER_ON_AFTER_POWER_FAIL MAINBOARD_POWER_ON
#endif

static void lpc_common_init(device_t dev, int master)
{
	uint8_t byte;
	uint32_t ioapic_base;

	/* IO APIC initialization */
	byte = pci_read_config8(dev, 0x74);
	byte |= (1<<0); // enable APIC
	pci_write_config8(dev, 0x74, byte);
	ioapic_base = pci_read_config32(dev, PCI_BASE_ADDRESS_1); // 0x14

	if (master)
		setup_ioapic(ioapic_base, 0);
	else
		clear_ioapic(ioapic_base);
}

static void lpc_slave_init(device_t dev)
{
	lpc_common_init(dev, 0);
}

static void enable_hpet(struct device *dev)
{
	unsigned long hpet_address;

	pci_write_config32(dev,0x44, 0xfed00001);
	hpet_address=pci_read_config32(dev,0x44)& 0xfffffffe;
	printk(BIOS_DEBUG, "enabling HPET @0x%lx\n", hpet_address);
}

static void lpc_init(device_t dev)
{
	uint8_t byte;
	uint8_t byte_old;
	int on;
	int nmi_option;

	lpc_common_init(dev, 1);

#if 0
	/* posted memory write enable */
	byte = pci_read_config8(dev, 0x46);
	pci_write_config8(dev, 0x46, byte | (1<<0));
#endif
	/* power after power fail */

#if 1
	on = CONFIG_MAINBOARD_POWER_ON_AFTER_POWER_FAIL;
	get_option(&on, "power_on_after_fail");
	byte = pci_read_config8(dev, PREVIOUS_POWER_STATE);
	byte &= ~0x40;
	if (!on) {
		byte |= 0x40;
	}
	pci_write_config8(dev, PREVIOUS_POWER_STATE, byte);
	printk(BIOS_INFO, "set power %s after power fail\n", on?"on":"off");
#endif
	/* Throttle the CPU speed down for testing */
	on = SLOW_CPU_OFF;
	get_option(&on, "slow_cpu");
	if(on) {
		uint16_t pm10_bar;
		uint32_t dword;
		pm10_bar = (pci_read_config16(dev, 0x60)&0xff00);
		outl(((on<<1)+0x10)  ,(pm10_bar + 0x10));
		dword = inl(pm10_bar + 0x10);
		on = 8-on;
		printk(BIOS_DEBUG, "Throttling CPU %2d.%1.1d percent.\n",
			     (on*12)+(on>>1),(on&1)*5);
	}

#if 0
// default is enabled
	/* Enable Port 92 fast reset */
	byte = pci_read_config8(dev, 0xe8);
	byte |= ~(1 << 3);
	pci_write_config8(dev, 0xe8, byte);
#endif

	/* Enable Error reporting */
	/* Set up sync flood detected */
	byte = pci_read_config8(dev, 0x47);
	byte |= (1 << 1);
	pci_write_config8(dev, 0x47, byte);

	/* Set up NMI on errors */
	byte = inb(0x70); // RTC70
	byte_old = byte;
	nmi_option = NMI_OFF;
	get_option(&nmi_option, "nmi");
	if (nmi_option) {
		byte &= ~(1 << 7); /* set NMI */
	} else {
		byte |= ( 1 << 7); // Can not mask NMI from PCI-E and NMI_NOW
	}
	if( byte != byte_old) {
		outb(byte, 0x70);
	}

	/* Initialize the real time clock */
	rtc_init(0);

	/* Initialize isa dma */
	isa_dma_init();

	/* Initialize the High Precision Event Timers */
	enable_hpet(dev);

}

static void mcp55_lpc_read_resources(device_t dev)
{
	struct resource *res;

	/* Get the normal PCI resources of this device. */
	/* We got one for APIC, or one more for TRAP. */
	pci_dev_read_resources(dev);

	/* Add an extra subtractive resource for both memory and I/O. */
	res = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	res->base = 0;
	res->size = 0x1000;
	res->flags = IORESOURCE_IO | IORESOURCE_SUBTRACTIVE |
		     IORESOURCE_ASSIGNED | IORESOURCE_FIXED;

	res = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
	res->base = 0xff800000;
	res->size = 0x00800000; /* 8 MB for flash */
	res->flags = IORESOURCE_MEM | IORESOURCE_SUBTRACTIVE |
		     IORESOURCE_ASSIGNED | IORESOURCE_FIXED;

	res = new_resource(dev, 3); /* IOAPIC */
	res->base = 0xfec00000;
	res->size = 0x00001000;
	res->flags = IORESOURCE_MEM | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;
}

/**
 * @brief Enable resources for children devices
 *
 * @param dev the device whos children's resources are to be enabled
 *
 * This function is called by the global enable_resources() indirectly via the
 * device_operation::enable_resources() method of devices.
 *
 * Indirect mutual recursion:
 *      enable_childrens_resources() -> enable_resources()
 *      enable_resources() -> device_operation::enable_resources()
 *      device_operation::enable_resources() -> enable_children_resources()
 */
static void mcp55_lpc_enable_childrens_resources(device_t dev)
{
	unsigned link;
	uint32_t reg, reg_var[4];
	int i;
	int var_num = 0;

	reg = pci_read_config32(dev, 0xa0);

	for (link = 0; link < dev->links; link++) {
		device_t child;
		for (child = dev->link[link].children; child; child = child->sibling) {
			enable_resources(child);
			if(child->enabled && (child->path.type == DEVICE_PATH_PNP)) {
				struct resource *res;
				for(res = child->resource_list; res; res = res->next) {
					unsigned long base, end; // don't need long long
					if(!(res->flags & IORESOURCE_IO)) continue;
					base = res->base;
					end = resource_end(res);
					printk(BIOS_DEBUG, "mcp55 lpc decode:%s, base=0x%08lx, end=0x%08lx\n",dev_path(child),base, end);
					switch(base) {
					case 0x3f8: // COM1
						reg |= (1<<0);	break;
					case 0x2f8: // COM2
						reg |= (1<<1);  break;
					case 0x378: // Parallal 1
						reg |= (1<<24); break;
					case 0x3f0: // FD0
						reg |= (1<<20); break;
					case 0x220:  // Aduio 0
						reg |= (1<<8);	break;
					case 0x300:  // Midi 0
						reg |= (1<<12);	break;
					}
					if( (base == 0x290) || (base >= 0x400)) {
						if(var_num>=4) continue; // only 4 var ; compact them ?
						reg |= (1<<(28+var_num));
						reg_var[var_num++] = (base & 0xffff)|((end & 0xffff)<<16);
					}
				}
			}
		}
	}
	pci_write_config32(dev, 0xa0, reg);
	for(i=0;i<var_num;i++) {
		pci_write_config32(dev, 0xa8 + i*4, reg_var[i]);
	}


}

static void mcp55_lpc_enable_resources(device_t dev)
{
	pci_dev_enable_resources(dev);
	mcp55_lpc_enable_childrens_resources(dev);
}

static void lpci_set_subsystem(device_t dev, unsigned vendor, unsigned device)
{
	pci_write_config32(dev, 0x40,
		((device & 0xffff) << 16) | (vendor & 0xffff));
}

static struct pci_operations lops_pci = {
	.set_subsystem	= lpci_set_subsystem,
};

static struct device_operations lpc_ops  = {
	.read_resources	= mcp55_lpc_read_resources,
	.set_resources	= pci_dev_set_resources,
	.enable_resources	= mcp55_lpc_enable_resources,
	.init		= lpc_init,
	.scan_bus	= scan_static_bus,
//	.enable		= mcp55_enable,
	.ops_pci	= &lops_pci,
};
static const struct pci_driver lpc_driver __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_LPC,
};

static const struct pci_driver lpc_driver_pro __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_PRO,
};

static const struct pci_driver lpc_driver_lpc2 __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_LPC_2,
};
static const struct pci_driver lpc_driver_lpc3 __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_LPC_3,
};
static const struct pci_driver lpc_driver_lpc4 __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_LPC_4,
};
static const struct pci_driver lpc_driver_lpc5 __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_LPC_5,
};
static const struct pci_driver lpc_driver_lpc6 __pci_driver = {
	.ops	= &lpc_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_LPC_6,
};

static struct device_operations lpc_slave_ops  = {
	.read_resources	= mcp55_lpc_read_resources,
	.set_resources	= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init		= lpc_slave_init,
//	.enable		= mcp55_enable,
	.ops_pci	= &lops_pci,
};

static const struct pci_driver lpc_driver_slave __pci_driver = {
	.ops	= &lpc_slave_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_SLAVE,
};
