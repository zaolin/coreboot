#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/smbus.h>
#include <pc80/mc146818rtc.h>
#include <bitops.h>
#include <arch/io.h>
#include "amd8111.h"
#include "amd8111_smbus.h"

#define PREVIOUS_POWER_STATE 0x43
#define MAINBOARD_POWER_OFF 0
#define MAINBOARD_POWER_ON 1
#define SLOW_CPU_OFF 0
#define SLOW_CPU__ON 1

#ifndef MAINBOARD_POWER_ON_AFTER_POWER_FAIL
#define MAINBOARD_POWER_ON_AFTER_POWER_FAIL MAINBOARD_POWER_ON
#endif


static int lsmbus_recv_byte(device_t dev)
{
	unsigned device;
	struct resource *res;

	device = dev->path.u.i2c.device;
	res = find_resource(get_pbus_smbus(dev)->dev, 0x58);
	
	return do_smbus_recv_byte(res->base, device);
}

static int lsmbus_send_byte(device_t dev, uint8_t val)
{
	unsigned device;
	struct resource *res;

	device = dev->path.u.i2c.device;
	res = find_resource(get_pbus_smbus(dev)->dev, 0x58);

	return do_smbus_send_byte(res->base, device, val);
}


static int lsmbus_read_byte(device_t dev, uint8_t address)
{
	unsigned device;
	struct resource *res;

	device = dev->path.u.i2c.device;
	res = find_resource(get_pbus_smbus(dev)->dev, 0x58);
	
	return do_smbus_read_byte(res->base, device, address);
}

static int lsmbus_write_byte(device_t dev, uint8_t address, uint8_t val)
{
	unsigned device;
	struct resource *res;

	device = dev->path.u.i2c.device;
	res = find_resource(get_pbus_smbus(dev)->dev, 0x58);
	
	return do_smbus_write_byte(res->base, device, address, val);
}

static void acpi_init(struct device *dev)
{
	uint8_t byte;
	uint16_t word;
	uint16_t pm10_bar;
	uint32_t dword;
	int on;

#if 0
	printk_debug("ACPI: disabling NMI watchdog.. ");
	byte = pci_read_config8(dev, 0x49);
	pci_write_config8(dev, 0x49, byte | (1<<2));


	byte = pci_read_config8(dev, 0x41);
	pci_write_config8(dev, 0x41, byte | (1<<6)|(1<<2));

	/* added from sourceforge */
	byte = pci_read_config8(dev, 0x48);
	pci_write_config8(dev, 0x48, byte | (1<<3));

	printk_debug("done.\n");


	printk_debug("ACPI: Routing IRQ 12 to PS2 port.. ");
	word = pci_read_config16(dev, 0x46);
	pci_write_config16(dev, 0x46, word | (1<<9));
	printk_debug("done.\n");

	
#endif
	/* power on after power fail */
	on = MAINBOARD_POWER_ON_AFTER_POWER_FAIL;
	get_option(&on, "power_on_after_fail");
	byte = pci_read_config8(dev, PREVIOUS_POWER_STATE);
	byte &= ~0x40;
	if (!on) {
		byte |= 0x40;
	}
	pci_write_config8(dev, PREVIOUS_POWER_STATE, byte);
	printk_info("set power %s after power fail\n", on?"on":"off");

	/* Throttle the CPU speed down for testing */
	on = SLOW_CPU_OFF;
	get_option(&on, "slow_cpu");
	if(on) {
		pm10_bar = (pci_read_config16(dev, 0x58)&0xff00);
		outl(((on<<1)+0x10)  ,(pm10_bar + 0x10));
		dword = inl(pm10_bar + 0x10);
		on = 8-on;
		printk_debug("Throttling CPU %2d.%1.1d percent.\n",
				(on*12)+(on>>1),(on&1)*5);
	}
}

static void acpi_read_resources(device_t dev)
{
	struct resource *resource;

	/* Handle the generic bars */
	pci_dev_read_resources(dev);

	/* Add the ACPI/SMBUS bar */
	resource = new_resource(dev, 0x58);
	resource->base  = 0;
	resource->size  = 256;
	resource->align = log2(256);
	resource->gran  = log2(256);
	resource->limit = 65536;
	resource->flags = IORESOURCE_IO;
	resource->index = 0x58;
}

static void acpi_enable_resources(device_t dev)
{
	uint8_t byte;
	/* Enable the generic pci resources */
	pci_dev_enable_resources(dev);

	/* Enable the ACPI/SMBUS Bar */
	byte = pci_read_config8(dev, 0x41);
	byte |= (1 << 7);
	pci_write_config8(dev, 0x41, byte);

	/* Set the class code */
	pci_write_config32(dev, 0x60, 0x06800000);
	
}

static void lpci_set_subsystem(device_t dev, unsigned vendor, unsigned device)
{
	pci_write_config32(dev, 0x7c, 
		((device & 0xffff) << 16) | (vendor & 0xffff));
}

static struct smbus_bus_operations lops_smbus_bus = {
	.recv_byte  = lsmbus_recv_byte,
	.send_byte  = lsmbus_send_byte,
	.read_byte  = lsmbus_read_byte,
	.write_byte = lsmbus_write_byte,
};

static struct pci_operations lops_pci = {
	.set_subsystem = lpci_set_subsystem,
};

static struct device_operations acpi_ops  = {
	.read_resources   = acpi_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = acpi_enable_resources,
	.init             = acpi_init,
	.scan_bus         = scan_static_bus,
	/*  We don't need amd8111_enable, chip ops takes care of it.
	 *  It could be useful if these devices were not 
	 *  enabled by default.
	 */
//	.enable           = amd8111_enable,
	.ops_pci          = &lops_pci,
	.ops_smbus_bus    = &lops_smbus_bus,
};

static struct pci_driver acpi_driver __pci_driver = {
	.ops    = &acpi_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = PCI_DEVICE_ID_AMD_8111_ACPI,
};

