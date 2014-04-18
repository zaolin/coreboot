/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2000 AG Electronics Ltd.
 * Copyright (C) 2003-2004 Linux Networx
 * Copyright (C) 2004 Tyan 
 * Copyright (C) 2005 Digital Design Corporation
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

/* RAM driver for SMSC LPC47B272 Super I/O chip. */

#include <arch/io.h>
#include <device/device.h>
#include <device/pnp.h>
#include <console/console.h>
#include <device/smbus.h>
#include <string.h>
#include <bitops.h>
#include <uart8250.h>
#include <pc80/keyboard.h>
#include <stdlib.h>
#include "chip.h"
#include "lpc47b272.h"

// Forward declarations
static void enable_dev(device_t dev);
void lpc47b272_pnp_set_resources(device_t dev);
void lpc47b272_pnp_set_resources(device_t dev);
void lpc47b272_pnp_enable_resources(device_t dev);
void lpc47b272_pnp_enable(device_t dev);
static void lpc47b272_init(device_t dev);

static void pnp_enter_conf_state(device_t dev);
static void pnp_exit_conf_state(device_t dev);
static void dump_pnp_device(device_t dev);


struct chip_operations superio_smsc_lpc47b272_ops = {
	CHIP_NAME("SMSC LPC47B272 Super I/O")
	.enable_dev = enable_dev
};

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = lpc47b272_pnp_set_resources,
	.enable_resources = lpc47b272_pnp_enable_resources,
	.enable           = lpc47b272_pnp_enable,
	.init             = lpc47b272_init,
};

static struct pnp_info pnp_dev_info[] = {
        { &ops, LPC47B272_FDC,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
        { &ops, LPC47B272_PP,   PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
        { &ops, LPC47B272_SP1,  PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        { &ops, LPC47B272_SP2,  PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        { &ops, LPC47B272_KBC,  PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_IRQ1, { 0x7ff, 0 }, { 0x7ff, 0x4}, },
        { &ops, LPC47B272_RT,   PNP_IO0, { 0x780, 0 }, },
};

/**********************************************************************************/
/*								PUBLIC INTERFACE								  */
/**********************************************************************************/

//----------------------------------------------------------------------------------
// Function:    	enable_dev
// Parameters:  	dev - pointer to structure describing a Super I/O device 
// Return Value:	None
// Description: 	Create device structures and allocate resources to devices 
//					specified in the pnp_dev_info array (above).
//
static void enable_dev(device_t dev)
{
	pnp_enable_devices(dev, &pnp_ops, 
					   ARRAY_SIZE(pnp_dev_info), 
					   pnp_dev_info);
}

//----------------------------------------------------------------------------------
// Function:    	lpc47b272_pnp_set_resources
// Parameters:  	dev - pointer to structure describing a Super I/O device 
// Return Value:	None
// Description: 	Configure the specified Super I/O device with the resources
//					(I/O space, etc.) that have been allocated for it.
//
void lpc47b272_pnp_set_resources(device_t dev)
{
	pnp_enter_conf_state(dev);  
	pnp_set_resources(dev);
    pnp_exit_conf_state(dev);  
}       

void lpc47b272_pnp_enable_resources(device_t dev)
{       
	pnp_enter_conf_state(dev);
    pnp_enable_resources(dev);
    pnp_exit_conf_state(dev);
}

void lpc47b272_pnp_enable(device_t dev)
{
	pnp_enter_conf_state(dev);   
	pnp_set_logical_device(dev);

	if(dev->enabled) {
		pnp_set_enable(dev, 1);
	}
	else {
		pnp_set_enable(dev, 0);
	}
	pnp_exit_conf_state(dev);  
}

//----------------------------------------------------------------------------------
// Function:    	lpc47b272_init
// Parameters:  	dev - pointer to structure describing a Super I/O device 
// Return Value:	None
// Description: 	Initialize the specified Super I/O device.
//					Devices other than COM ports and the keyboard controller are 
//					ignored. For COM ports, we configure the baud rate. 
//
static void lpc47b272_init(device_t dev)
{
	struct superio_smsc_lpc47b272_config *conf = dev->chip_info;
	struct resource *res0, *res1;

	if (!dev->enabled)
		return;
	
	switch(dev->path.u.pnp.device) {
	case LPC47B272_SP1: 
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;
		
	case LPC47B272_SP2:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;
		
	case LPC47B272_KBC:
		res0 = find_resource(dev, PNP_IDX_IO0);
		res1 = find_resource(dev, PNP_IDX_IO1);
		init_pc_keyboard(res0->base, res1->base, &conf->keyboard);
		break;
	}
}

/**********************************************************************************/
/*								PRIVATE FUNCTIONS							      */
/**********************************************************************************/

//----------------------------------------------------------------------------------
// Function:    	pnp_enter_conf_state
// Parameters:  	dev - pointer to structure describing a Super I/O device 
// Return Value:	None
// Description: 	Enable access to the LPC47B272's configuration registers.
//
static void pnp_enter_conf_state(device_t dev) 
{
	outb(0x55, dev->path.u.pnp.port);
}

//----------------------------------------------------------------------------------
// Function:    	pnp_exit_conf_state
// Parameters:  	dev - pointer to structure describing a Super I/O device 
// Return Value:	None
// Description: 	Disable access to the LPC47B272's configuration registers.
//
static void pnp_exit_conf_state(device_t dev) 
{
    outb(0xaa, dev->path.u.pnp.port);
}

#if 0
//----------------------------------------------------------------------------------
// Function:    	dump_pnp_device
// Parameters:  	dev - pointer to structure describing a Super I/O device 
// Return Value:	None
// Description: 	Print the values of all of the LPC47B272's configuration registers.
//					NOTE: The LPC47B272 must be in configuration mode when this
//						  function is called.
//
static void dump_pnp_device(device_t dev)
{
    int register_index;
    print_debug("\r\n");

    for(register_index = 0; register_index <= LPC47B272_MAX_CONFIG_REGISTER; register_index++) {
        uint8_t register_value;

        if ((register_index & 0x0f) == 0) {
                print_debug_hex8(register_index);
                print_debug_char(':');
        }

		// Skip over 'register' that would cause exit from configuration mode
	    if (register_index == 0xaa)
			register_value = 0xaa;
		else
        	register_value = pnp_read_config(dev, register_index);

        print_debug_char(' ');
        print_debug_hex8(register_value);
        if ((register_index & 0x0f) == 0x0f) {
        	print_debug("\r\n");
        }
    }

   	print_debug("\r\n");
}
#endif
