/* Copyright 2000  AG Electronics Ltd. */
/* This code is distributed without warranty under the GPL v2 (see COPYING) */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include "chip.h"
#include "pc97317.h"

static void init(device_t dev)
{
	struct superio_nsc_pc97317_config *conf;
	struct resource *res0, *res1;

	if (!dev->enabled) {
		return;
	}
	conf = dev->chip_info;
	switch(dev->path.u.pnp.device) {
	case PC97317_SP1:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;

	case PC97317_SP2:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;

	case PC97317_KBCK:
		/* Enable keyboard */
		pnp_set_logical_device(dev);
		pnp_set_enable(dev, 0); /* Disable keyboard */
		pnp_write_config(dev, 0xf0, 0x40); /* Set KBC clock to 8 Mhz */
		pnp_set_enable(dev, 1); /* Enable keyboard */

		res0 = find_resource(dev, PNP_IDX_IO0);
		res1 = find_resource(dev, PNP_IDX_IO1);
		init_pc_keyboard(res0->base, res1->base, &conf->keyboard);
		break;

#if 0
	case PC97317_FDC:
	{
		unsigned reg;
		/* Set up floppy in PS/2 mode */
		outb(0x09, SIO_CONFIG_RA);
		reg = inb(SIO_CONFIG_RD);
		reg = (reg & 0x3F) | 0x40;
		outb(reg, SIO_CONFIG_RD);
		outb(reg, SIO_CONFIG_RD);       /* Have to write twice to change! */
		break;
	}
#endif
	default:
		break;
	}
}

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = pnp_set_resources,
	.enable_resources = pnp_enable_resources,
	.enable           = pnp_enable,
	.init             = init,
};

static struct pnp_info pnp_dev_info[] = {
 { &ops, PC97317_KBCK, PNP_IO0 | PNP_IO1 | PNP_IRQ0, { 0xffb, 0 }, { 0xffb, 0x4}, },
 { &ops, PC97317_KBCM, PNP_IRQ0 },
 { &ops, PC97317_RTC,  PNP_IO0 | PNP_IRQ0, { 0xfffe, 0}, },
 { &ops, PC97317_FDC,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0xfffa, 0}, },
 { &ops, PC97317_PP,   PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x03fc, 0}, },
 { &ops, PC97317_SP2,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0 | PNP_DRQ1, { 0xfff8, 0 }, },
 { &ops, PC97317_SP1,  PNP_IO0 | PNP_IRQ0, { 0xfff8, 0 }, },
 { &ops, PC97317_GPIO, PNP_IO0, { 0xfff8, 0 } },
 { &ops, PC97317_PM,   PNP_IO0, { 0xfffe, 0 } },
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops,
		sizeof(pnp_dev_info)/sizeof(pnp_dev_info[0]), pnp_dev_info);
}

struct chip_operations superio_nsc_pc97317_ops = {
	CHIP_NAME("NSC 97317")
	.enable_dev = enable_dev,
};
