/*
 * (C) Copyright 2004 Nick Barker <nick.barker9@btinternet.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

 /* vt1211 routines and defines*/


#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include <uart8250.h>

#include "vt1211.h"
#include "chip.h"


static unsigned char vt1211hwmonitorinits[]={
 0x10,0x3, 0x11,0x10, 0x12,0xd, 0x13,0x7f,
 0x14,0x21, 0x15,0x81, 0x16,0xbd, 0x17,0x8a,
 0x18,0x0, 0x19,0x0, 0x1a,0x0, 0x1b,0x0,
 0x1d,0xff, 0x1e,0x0, 0x1f,0x73, 0x20,0x67,
 0x21,0xc1, 0x22,0xca, 0x23,0x74, 0x24,0xc2,
 0x25,0xc7, 0x26,0xc9, 0x27,0x7f, 0x29,0x0,
 0x2a,0x0, 0x2b,0xff, 0x2c,0x0, 0x2d,0xff,
 0x2e,0x0, 0x2f,0xff, 0x30,0x0, 0x31,0xff,
 0x32,0x0, 0x33,0xff, 0x34,0x0, 0x39,0xff,
 0x3a,0x0, 0x3b,0xff, 0x3c,0xff, 0x3d,0xff,
 0x3e,0x0, 0x3f,0xb0, 0x43,0xff, 0x44,0xff,
 0x46,0xff, 0x47,0x50, 0x4a,0x3, 0x4b,0xc0,
 0x4c,0x0, 0x4d,0x0, 0x4e,0xf, 0x5d,0x77,
 0x5c,0x0, 0x5f,0x33, 0x40,0x1};

static void pnp_enter_ext_func_mode(device_t dev) 
{
	outb(0x87, dev->path.u.pnp.port);
	outb(0x87, dev->path.u.pnp.port);
}

static void pnp_exit_ext_func_mode(device_t dev) 
{
	outb(0xaa, dev->path.u.pnp.port);
}

static void init_hwm(unsigned long base)
{
	int i;

 	// initialize vt1211 hardware monitor registers, which are at 0xECXX
	for(i = 0; i < sizeof(vt1211hwmonitorinits); i += 2) {
 			outb(vt1211hwmonitorinits[i + 1],
					base + vt1211hwmonitorinits[i]);
	}
}

static void vt1211_init(struct device *dev)
{
	struct superio_via_vt1211_config *conf = dev->chip_info;
	struct resource *res0;

	if (!dev->enabled) {
		return;
	}

	switch (dev->path.u.pnp.device) {
	case VT1211_SP1:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;
	case VT1211_SP2:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;
	case VT1211_HWM:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_hwm(res0->base);
		break;
	default:
		printk_info("vt1211 asked to initialise unknown device!\n");
	}
	
	/* activate com2
	start_conf_pnp(3);
	write_pnp(0x60,0xbe);
	write_pnp(0x70,0x3);
	write_pnp(0xf0,0x02);
	write_pnp(0x30,0x01);
	end_conf_pnp();

 	// Activate the vt1211 hardware monitor
	start_conf_pnp(0x0b);
	write_pnp(0x60,0xec); 
	write_pnp(0x30,1);
	end_conf_pnp(); */

}

void vt1211_pnp_enable_resources(device_t dev)
{
	pnp_enter_ext_func_mode(dev);
	pnp_enable_resources(dev);
	pnp_exit_ext_func_mode(dev);
}

void vt1211_pnp_set_resources(struct device *dev)
{
	pnp_enter_ext_func_mode(dev);
	pnp_set_resources(dev);
	pnp_exit_ext_func_mode(dev);
}

void vt1211_pnp_enable(device_t dev)
{
	if (!dev->enabled) {
		pnp_enter_ext_func_mode(dev);
		pnp_set_logical_device(dev);
		pnp_set_enable(dev, 0);
		pnp_exit_ext_func_mode(dev);
	}
}

struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = vt1211_pnp_set_resources,
	.enable_resources = vt1211_pnp_enable_resources,
	.enable           = vt1211_pnp_enable,
	.init             = vt1211_init,
};

static struct pnp_info pnp_dev_info[] = {
	{ &ops, VT1211_FDC, PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
	{ &ops, VT1211_PP,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
	{ &ops, VT1211_SP1, PNP_IO0 | PNP_IRQ0,            { 0x07f8, 0}, },
	{ &ops, VT1211_SP2, PNP_IO0 | PNP_IRQ0,            { 0x07f8, 0}, },
	{ &ops, VT1211_HWM, PNP_IO0 , { 0xfff8, 0 }, },
};

static void enable_dev(struct device *dev)
{
	printk_debug("vt1211 enabling PNP devices.\n");
	pnp_enable_devices(dev,
			&ops,
			sizeof(pnp_dev_info) / sizeof(pnp_dev_info[0]),
			pnp_dev_info);
}

struct chip_operations superio_via_vt1211_ops = {
	CHIP_NAME("VIA vt1211")
	.enable_dev = enable_dev,
};
