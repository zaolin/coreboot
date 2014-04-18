/* Copyright 2000  AG Electronics Ltd. */
/* Copyright 2003-2004 Linux Networx */
/* Copyright 2004 Tyan 
 */ 

/* This code is distributed without warranty under the GPL v2 (see COPYING) */

#include <arch/io.h>
#include <device/device.h>
#include <device/pnp.h>
#include <console/console.h>
#include <device/smbus.h>
#include <string.h>
#include <bitops.h>
#include <uart8250.h>
#include <pc80/keyboard.h>
#include "chip.h"
#include "lpc47b397.h"


static void pnp_enter_conf_state(device_t dev) {
        outb(0x55, dev->path.u.pnp.port);
}
static void pnp_exit_conf_state(device_t dev) {
        outb(0xaa, dev->path.u.pnp.port);
}

static void pnp_write_index(unsigned long port_base, uint8_t reg, uint8_t value)
{
        outb(reg, port_base);
        outb(value, port_base + 1);
}

static uint8_t pnp_read_index(unsigned long port_base, uint8_t reg)
{
        outb(reg, port_base);
        return inb(port_base + 1);
}

static void enable_hwm_smbus(device_t dev) {
	/* enable SensorBus register access */
        uint8_t reg, value;
        reg = 0xf0; 
        value = pnp_read_config(dev, reg);
        value |= 0x01;
        pnp_write_config(dev, reg, value);
} 


static void lpc47b397_init(device_t dev)
{
	struct superio_smsc_lpc47b397_config *conf;
	struct resource *res0, *res1;
	if (!dev->enabled) {
		return;
	}
	conf = dev->chip_info;
	switch(dev->path.u.pnp.device) {
	case LPC47B397_SP1: 
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com1);
		break;
	case LPC47B397_SP2:
		res0 = find_resource(dev, PNP_IDX_IO0);
		init_uart8250(res0->base, &conf->com2);
		break;
	case LPC47B397_KBC:
		res0 = find_resource(dev, PNP_IDX_IO0);
		res1 = find_resource(dev, PNP_IDX_IO1);
		init_pc_keyboard(res0->base, res1->base, &conf->keyboard);
		break;
	}
	
}

void lpc47b397_pnp_set_resources(device_t dev)
{

	pnp_enter_conf_state(dev);  

	pnp_set_resources(dev);

#if 0
        dump_pnp_device(dev);
#endif
                
        pnp_exit_conf_state(dev);  
        
}       
        
void lpc47b397_pnp_enable_resources(device_t dev)
{       

        pnp_enter_conf_state(dev);

        pnp_enable_resources(dev);

        switch(dev->path.u.pnp.device) {
        case LPC47B397_HWM:
                printk_debug("lpc47b397 SensorBus Register Access enabled\r\n");
		pnp_set_logical_device(dev);
                enable_hwm_smbus(dev);
                break;
        }

#if 0  
        dump_pnp_device(dev);
#endif

        pnp_exit_conf_state(dev);

}

void lpc47b397_pnp_enable(device_t dev)
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

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = lpc47b397_pnp_set_resources,
	.enable_resources = lpc47b397_pnp_enable_resources,
	.enable           = lpc47b397_pnp_enable,
	.init             = lpc47b397_init,
};


#define HWM_INDEX 0
#define HWM_DATA  1
#define SB_INDEX  0x0b
#define SB_DATA0  0x0c
#define SB_DATA1  0x0d
#define SB_DATA2  0x0e
#define SB_DATA3  0x0f

static int lsmbus_read_byte(device_t dev, uint8_t address)
{
        unsigned device;
        struct resource *res;
	int result;

        device = dev->path.u.i2c.device;

        res = find_resource(get_pbus_smbus(dev)->dev, PNP_IDX_IO0);

	pnp_write_index(res->base+HWM_INDEX, 0, device); // why 0?
	
	result = pnp_read_index(res->base+SB_INDEX, address); // we only read it one byte one time
        
        return result;
}

static int lsmbus_write_byte(device_t dev, uint8_t address, uint8_t val)
{       
        unsigned device;
        struct resource *res;
        
        device = dev->path.u.i2c.device;
	res = find_resource(get_pbus_smbus(dev)->dev, PNP_IDX_IO0);

        pnp_write_index(res->base+HWM_INDEX, 0, device); // why 0?

        pnp_write_index(res->base+SB_INDEX, address, val); // we only write it one byte one time
        
        return 0;
}

static struct smbus_bus_operations lops_smbus_bus = {
//        .recv_byte  = lsmbus_recv_byte,
//        .send_byte  = lsmbus_send_byte,
        .read_byte  = lsmbus_read_byte,
        .write_byte = lsmbus_write_byte,
};
static struct device_operations ops_hwm = {
        .read_resources   = pnp_read_resources,
        .set_resources    = lpc47b397_pnp_set_resources,
        .enable_resources = lpc47b397_pnp_enable_resources,
        .enable           = lpc47b397_pnp_enable,
        .init             = lpc47b397_init,
	.scan_bus         = scan_static_bus,
	.ops_smbus_bus    = &lops_smbus_bus,
};

static struct pnp_info pnp_dev_info[] = {
        { &ops, LPC47B397_FDC,  PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
        { &ops, LPC47B397_PP,   PNP_IO0 | PNP_IRQ0 | PNP_DRQ0, { 0x07f8, 0}, },
        { &ops, LPC47B397_SP1,  PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        { &ops, LPC47B397_SP2,  PNP_IO0 | PNP_IRQ0, { 0x7f8, 0 }, },
        { &ops, LPC47B397_KBC,  PNP_IO0 | PNP_IO1 | PNP_IRQ0 | PNP_IRQ1, { 0x7ff, 0 }, { 0x7ff, 0x4}, },
	{ &ops_hwm, LPC47B397_HWM,  PNP_IO0, { 0x7f0, 0 }, },
        { &ops, LPC47B397_RT,   PNP_IO0, { 0x780, 0 }, },
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &pnp_ops,
		sizeof(pnp_dev_info)/sizeof(pnp_dev_info[0]), pnp_dev_info);
}

struct chip_operations superio_smsc_lpc47b397_ops = {
	CHIP_NAME("SMSC LPC47B397 Super I/O")
	.enable_dev = enable_dev,
};

