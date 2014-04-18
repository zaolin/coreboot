/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2005 Digital Design Corporation
 * Copyright (C) 2008-2009 coresystems GmbH
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

/* Pre-RAM driver for SMSC LPC47N227 Super I/O chip. */

#include <arch/romcc_io.h>
#include "lpc47n227.h"

//----------------------------------------------------------------------------------
// Function:            pnp_enter_conf_state
// Parameters:          dev - high 8 bits = Super I/O port
// Return Value:        None
// Description:         Enable access to the LPC47N227's configuration registers.
//
static inline void pnp_enter_conf_state(device_t dev)
{
	unsigned port = dev >> 8;
	outb(0x55, port);
}

//----------------------------------------------------------------------------------
// Function:            pnp_exit_conf_state
// Parameters:          dev - high 8 bits = Super I/O port
// Return Value:        None
// Description:         Disable access to the LPC47N227's configuration registers.
//
static void pnp_exit_conf_state(device_t dev)
{
	unsigned port = dev >> 8;
	outb(0xaa, port);
}

//----------------------------------------------------------------------------------
// Function:            lpc47n227_pnp_set_iobase
// Parameters:          dev - high 8 bits = Super I/O port, 
//                            low 8 bits = logical device number (per lpc47n227.h)
//                      iobase - base I/O port for the logical device
// Return Value:        None
// Description:         Program the base I/O port for the specified logical device.
//
void lpc47n227_pnp_set_iobase(device_t dev, unsigned iobase)
{
	// LPC47N227 requires base ports to be a multiple of 4
	ASSERT(!(iobase & 0x3));

	switch (dev & 0xFF) {
	case LPC47N227_PP:
		pnp_write_config(dev, 0x23, (iobase >> 2) & 0xff);
		break;

	case LPC47N227_SP1:
		pnp_write_config(dev, 0x24, (iobase >> 2) & 0xff);
		break;

	case LPC47N227_SP2:
		pnp_write_config(dev, 0x25, (iobase >> 2) & 0xff);
		break;

	default:
		break;
	}
}

//----------------------------------------------------------------------------------
// Function:            lpc47n227_pnp_set_enable
// Parameters:          dev - high 8 bits = Super I/O port, 
//                      low 8 bits = logical device number (per lpc47n227.h)
//                      enable - 0 to disable, anythig else to enable
// Return Value:        None
// Description:         Enable or disable the specified logical device.
//                      Technically, a full disable requires setting the device's base
//                      I/O port below 0x100. We don't do that here, because we don't
//                      have access to a data structure that specifies what the 'real'
//                      base port is (when asked to enable the device). Also the function
//                      is used only to disable the device while its true base port is
//                      programmed (see lpc47n227_enable_serial() below).
//
void lpc47n227_pnp_set_enable(device_t dev, int enable)
{
	uint8_t power_register = 0;
	uint8_t power_mask = 0;
	uint8_t current_power;
	uint8_t new_power;

	switch (dev & 0xFF) {
	case LPC47N227_PP:
		power_register = 0x01;
		power_mask = 0x04;
		break;

	case LPC47N227_SP1:
		power_register = 0x02;
		power_mask = 0x08;
		break;

	case LPC47N227_SP2:
		power_register = 0x02;
		power_mask = 0x80;
		break;

	default:
		return;
	}

	current_power = pnp_read_config(dev, power_register);
	new_power = current_power & ~power_mask;	// disable by default

	if (enable)
		new_power |= power_mask;	// Enable

	pnp_write_config(dev, power_register, new_power);
}

//----------------------------------------------------------------------------------
// Function:            lpc47n227_enable_serial
// Parameters:          dev - high 8 bits = Super I/O port, 
//                            low 8 bits = logical device number (per lpc47n227.h)
//                      iobase - processor I/O port address to assign to this serial device
// Return Value:        bool
// Description:         Configure the base I/O port of the specified serial device
//                      and enable the serial device.
//
static void lpc47n227_enable_serial(device_t dev, unsigned iobase)
{
	// NOTE: Cannot use pnp_set_XXX() here because they assume chip
	// support for logical devices, which the LPC47N227 doesn't have

	pnp_enter_conf_state(dev);
	lpc47n227_pnp_set_enable(dev, 0);
	lpc47n227_pnp_set_iobase(dev, iobase);
	lpc47n227_pnp_set_enable(dev, 1);
	pnp_exit_conf_state(dev);
}
