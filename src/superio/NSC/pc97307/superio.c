/* Copyright 2000  AG Electronics Ltd. */
/* This code is distributed without warranty under the GPL v2 (see COPYING) */

#include <arch/io.h>
#include <console/console.h>
#include "chip.h"
#include "pc97307.h"

static void init(device_t dev)
{
	struct superio_NSC_pc97307_config *conf;
	struct resource *res0, *res1;

	if (!dev->enabled) {
		return;
	}
	conf = dev->chip;
	switch(dev->path.u.pnp.device) {
	case PC97307_SP1:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;

	case PC97307_SP2:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;

	case PC97307_KBCK:
		/* Enable keyboard */
		pnp_set_logical_device(dev);
		pnp_set_enable(dev, 0); /* Disable keyboard */
		pnp_write_config(dev, 0xf0, 0x40); /* Set KBC clock to 8 Mhz */
		pnp_set_enable(dev, 1); /* Enable keyboard */

		res0 = find_resource(dev, PNP_IDX_IO0);
		res1 = find_resource(dev, PNP_IDX_IO1);
		init_pc_keyboard(res0->base, res1->base, &conf->keyboard);
		break;

	case PC97307_FDC:
		/* Set up floppy in PS/2 mode */
		outb(0x09, SIO_CONFIG_RA);
		reg = inb(SIO_CONFIG_RD);
		reg = (reg & 0x3F) | 0x40;
		outb(reg, SIO_CONFIG_RD);
		outb(reg, SIO_CONFIG_RD);       /* Have to write twice to change! */
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
 { &ops, PC97307_KBCK, PNP_IO0 | PNP_IO1 | PNP_IRQ0, { 0xffb, 0 }, { 0xffb, 0x4}, },
 { &ops, PC97307_KBCM, PNP_IRQ0 },
 { &ops, PC97307_RTC,  PNP_IO0 | PNP_IRQ0, { 0xfffe, 0}, },
 { &ops, PC97307_FDC,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0xfffa, 0}, },
 { &ops, PC97307_PP,   PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x03fc, 0}, },
 { &ops, PC97307_SP2,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0 | PNP_DRQ1, { 0xfff8, 0 }, },
 { &ops, PC97307_SP1,  PNP_IO0 | PNP_IRQ0, { 0xfff8, 0 }, },
 { &ops, PC97307_GPIO, PNP_IO0, { 0xfff8, 0 } },
 { &ops, PC97307_PM,   PNP_IO0, { 0xfffe, 0 } },
};

static void enable_dev(struct device *dev)
{
	pnp_enable_device(dev, &pnp_ops,
		sizeof(pnp_dev_info)/sizeof(pnp_dev_info[0]), pnp_dev_info);
}

struct chip_operations superio_NSC_pc97307_control = {
	.enable_dev = enable_dev,
	.name       =  "NSC 97307"
};
