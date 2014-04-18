/* Copyright 2000  AG Electronics Ltd. */
/* Copyright 2003-2004 Linux Networx */
/* Copyright 2004 Tyan 
   By LYH change from PC87360 */
/* This code is distributed without warranty under the GPL v2 (see COPYING) */

#include <arch/io.h>
#include <device/device.h>
#include <device/pnp.h>
#include <console/console.h>
#include <string.h>
#include <bitops.h>
#include <uart8250.h>
#include <pc80/keyboard.h>
#include "chip.h"
#include "w83627thf.h"

static void init(device_t dev)
{
	struct superio_winbond_w83627thf_config *conf;
	struct resource *res0, *res1;
	/* Wishlist handle well known programming interfaces more
	 * generically.
	 */
	if (!dev->enable) {
		return;
	}
	conf = dev->chip_info;
	switch(dev->path.u.pnp.device) {
	case W83627HF_SP1: 
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;
	case W83627HF_SP2:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;
	case W83627HF_KBC:
		res0 = find_resource(dev, PNP_IDX_IO0);
		res1 = find_resource(dev, PNP_IDX_IO1);
		init_pc_keyboard(res0->base, res1->base, &conf->keyboard);
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
        { &ops, W83627HF_FDC,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
        { &ops, W83627HF_PP,   PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
        { &ops, W83627HF_SP1,  PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        { &ops, W83627HF_SP2,  PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        // No 4 { 0,},
        { &ops, W83627HF_KBC,  PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_IRQ1, { 0x7ff, 0 }, { 0x7ff, 0x4}, },
        { &ops, W83627HF_CIR, PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        { &ops, W83627HF_GAME_MIDI_GPIO1, PNP_IO0 | PNP_IO1 | PNP_IRQ0, { 0x7ff, 0 }, {0x7fe, 4} },
        // { W83627HF_GPIO2,},
        // { W83627HF_GPIO3,},
        { &ops, W83627HF_ACPI, PNP_IRQ0,  },
        { &ops, W83627HF_HWM,  PNP_IO0 | PNP_IRQ0, { 0xff8, 0 } },
};

static void enable_dev(device_t dev)
{
	pnp_enable_device(dev, &pnp_ops,
		sizeof(pnp_dev_info)/sizeof(pnp_dev_info[0]), pnp_dev_info);
}

struct chip_operations superio_winbond_w83627thf_control = {
	.enable_dev = enable_dev,
	.name       = "Winbond w83627thf"
};
