#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>
#include <pc80/keyboard.h>
#include "vt8235.h"
#include "chip.h"

/*
 * Base VT8235.
 */
static int enabled = 0;

void hard_reset(void) 
{
	printk_err("NO HARD RESET ON VT8235! FIX ME!\n");
}

static void keyboard_on(struct device *dev)
{
	unsigned char regval;

	regval = pci_read_config8(dev, 0x51);
	regval |= 0x05; 
	regval &= 0xfd;
	pci_write_config8(dev, 0x51, regval);

	init_pc_keyboard(0x60, 0x64, 0);
}

void dump_south(device_t dev0)
{
	int i,j;
	
	for(i = 0; i < 256; i += 16) {
		printk_debug("0x%x: ", i);
		for(j = 0; j < 16; j++) {
			printk_debug("%02x ", pci_read_config8(dev0, i+j));
		}
		printk_debug("\n");
	}
}

void set_led()
{
	// set power led to steady now that lxbios has virtually done its job
	device_t dev;
	dev = dev_find_device(PCI_VENDOR_ID_VIA, PCI_DEVICE_ID_VIA_8235, 0);
	pci_write_config8(dev, 0x94, 0xb0);
}


static void vt8235_enable(struct device *dev)
{
	struct southbridge_via_vt8235_config *conf = dev->chip_info;
	unsigned char regval;
	unsigned short vendor,model;


	vendor = pci_read_config16(dev,0);
	model = pci_read_config16(dev,0x2);

	printk_debug("In vt8235_enable %04x %04x.\n",vendor,model);
	
	/* if this is not the southbridge itself just return */
	/* this is necessary because USB devices are slot 10, whereas this device is slot 11 
	  therefore usb devices get called first during the bus scan */

	if( (vendor != PCI_VENDOR_ID_VIA) || (model != PCI_DEVICE_ID_VIA_8235))
		return;

	printk_debug("Initialising Devices\n");


	setup_i8259();   // make sure interupt controller is configured before keyboard init 

	/* enable RTC and ethernet */
	regval = pci_read_config8(dev, 0x51);
	regval |= 0x18; 
	pci_write_config8(dev, 0x51, regval);

	/* turn on keyboard */
	keyboard_on(dev);

	/* enable USB 1.1 & USB 2.0 -redundant really since we've already been there - see note above*/
   	regval = pci_read_config8(dev, 0x50);
	regval &= ~(0x36);
	pci_write_config8(dev, 0x50, regval);


}

struct chip_operations southbridge_via_vt8235_ops = {
	CHIP_NAME("VIA vt8235")
	.enable_dev = vt8235_enable,
};
