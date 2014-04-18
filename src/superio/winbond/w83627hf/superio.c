/* Copyright 2000  AG Electronics Ltd. */
/* Copyright 2003-2004 Linux Networx */
/* Copyright 2004 Tyan 
   By LYH change from PC87360 */
/* This code is distributed without warranty under the GPL v2 (see COPYING) */

#include <arch/io.h>
#include <device/device.h>
#include <device/pnp.h>
#include <device/chip.h>
#include <console/console.h>
#include <string.h>
#include <bitops.h>
#include <uart8250.h>
#include <pc80/keyboard.h>
#include "chip.h"
#include "w83627hf.h"

static void init(device_t dev)
{
	struct superio_winbond_w83627hf_config *conf;
	struct resource *res0, *res1;
	/* Wishlist handle well known programming interfaces more
	 * generically.
	 */
	if (!dev->enable) {
		return;
	}
	conf = dev->chip->chip_info;
	switch(dev->path.u.pnp.device) {
	case W83627HF_SP1: 
		res0 = get_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;
	case W83627HF_SP2:
		res0 = get_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;
	case W83627HF_KBC:
		res0 = get_resource(dev, PNP_IDX_IO0);
		res1 = get_resource(dev, PNP_IDX_IO1);
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

static void enumerate(struct chip *chip)
{
	pnp_enumerate(chip, sizeof(pnp_dev_info)/sizeof(pnp_dev_info[0]), 
		&pnp_ops, pnp_dev_info);
}
static void sio_enable(struct chip *chip, enum chip_pass pass)
{

        struct superio_winbond_w83627hf_config *conf = (struct superio_winbond_w83627hf_config *)chip->chip_info;

        switch (pass) {
        case CONF_PASS_PRE_CONSOLE:
                init_pc_keyboard(0x60, 0x64, &conf->keyboard);
                break;
        default:
                /* nothing yet */
                break;
        }
}


struct chip_control superio_winbond_w83627hf_control = {
	.enable	   = sio_enable,
	.enumerate = enumerate,
	.name      = "Winbond w83627hf"
};
