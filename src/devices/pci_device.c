/*
 *      PCI Bus Services, see include/linux/pci.h for further explanation.
 *
 *      Copyright 1993 -- 1997 Drew Eckhardt, Frederic Potter,
 *      David Mosberger-Tang
 *
 *      Copyright 1997 -- 1999 Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 *	
 *	Copyright 2003 -- Eric Biederman <ebiederman@lnxi.com>
 */

#include <console/console.h>
#include <stdlib.h>
#include <stdint.h>
#include <bitops.h>
#include <string.h>
#include <arch/io.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <part/hard_reset.h>
#include <part/fallback_boot.h>
#include <delay.h>

static uint8_t pci_moving_config8(struct device *dev, unsigned reg)
{
	uint8_t value, ones, zeroes;
	value = pci_read_config8(dev, reg);
	
	pci_write_config8(dev, reg, 0xff);
	ones = pci_read_config8(dev, reg);

	pci_write_config8(dev, reg, 0x00);
	zeroes = pci_read_config8(dev, reg);

	pci_write_config8(dev, reg, value);

	return ones ^ zeroes;
}
static uint16_t pci_moving_config16(struct device *dev, unsigned reg)
{
	uint16_t value, ones, zeroes;
	value = pci_read_config16(dev, reg);
	
	pci_write_config16(dev, reg, 0xffff);
	ones = pci_read_config16(dev, reg);

	pci_write_config16(dev, reg, 0x0000);
	zeroes = pci_read_config16(dev, reg);

	pci_write_config16(dev, reg, value);

	return ones ^ zeroes;
}
static uint32_t pci_moving_config32(struct device *dev, unsigned reg)
{
	uint32_t value, ones, zeroes;
	value = pci_read_config32(dev, reg);
	
	pci_write_config32(dev, reg, 0xffffffff);
	ones = pci_read_config32(dev, reg);

	pci_write_config32(dev, reg, 0x00000000);
	zeroes = pci_read_config32(dev, reg);

	pci_write_config32(dev, reg, value);

	return ones ^ zeroes;
}

unsigned pci_find_capability(device_t dev, unsigned cap)
{
	unsigned pos;
	pos = 0;
	switch(dev->hdr_type & 0x7f) {
	case PCI_HEADER_TYPE_NORMAL:
	case PCI_HEADER_TYPE_BRIDGE:
		pos = PCI_CAPABILITY_LIST;
		break;
	}
	if (pos > PCI_CAP_LIST_NEXT) {
		pos = pci_read_config8(dev, pos);
	}
	while(pos != 0) {   /* loop through the linked list */
		int this_cap;
		this_cap = pci_read_config8(dev, pos + PCI_CAP_LIST_ID);
		if (this_cap == cap) {
			return pos;
		}
	}
	return 0;
}


/** Given a device and register, read the size of the BAR for that register. 
 * @param dev       Pointer to the device structure
 * @param resource  Pointer to the resource structure
 * @param index     Address of the pci configuration register
 */
struct resource *pci_get_resource(struct device *dev, unsigned long index)
{
	struct resource *resource;
	unsigned long value, attr;
	resource_t  moving, limit;

	/* Initialize the resources to nothing */
	resource = new_resource(dev, index);

	/* Get the initial value */
	value = pci_read_config32(dev, index);

	/* See which bits move */
	moving = pci_moving_config32(dev, index);
	
	/* Initialize attr to the bits that do not move */
	attr = value & ~moving;

	/* If it is a 64bit resource look at the high half as well */
	if (((attr & PCI_BASE_ADDRESS_SPACE_IO) == 0) &&
		((attr & PCI_BASE_ADDRESS_MEM_LIMIT_MASK) == PCI_BASE_ADDRESS_MEM_LIMIT_64))
	{
		/* Find the high bits that move */
		moving |= ((resource_t)pci_moving_config32(dev, index + 4)) << 32;
	}
	/* Find the resource constraints. 
	 *
	 * Start by finding the bits that move. From there:
	 * - Size is the least significant bit of the bits that move.
	 * - Limit is all of the bits that move plus all of the lower bits.
	 * See PCI Spec 6.2.5.1 ...
	 */
	limit = 0;
	if (moving) {
		resource->size = 1;
		resource->align = resource->gran = 0;
		while(!(moving & resource->size)) {
			resource->size <<= 1;
			resource->align += 1;
			resource->gran  += 1;
		}
		resource->limit = limit = moving | (resource->size - 1);
	}
	/*
	 * some broken hardware has read-only registers that do not 
	 * really size correctly.
	 * Example: the acer m7229 has BARs 1-4 normally read-only. 
	 * so BAR1 at offset 0x10 reads 0x1f1. If you size that register
	 * by writing 0xffffffff to it, it will read back as 0x1f1 -- a 
	 * violation of the spec. 
	 * We catch this case and ignore it by observing which bits move,
	 * This also catches the common case unimplemented registers
	 * that always read back as 0.
	 */
	if (moving == 0) {
		if (value != 0) {
			printk_debug(
				"%s register %02x(%08x), read-only ignoring it\n",
				dev_path(dev), index, value);
		}
		resource->flags = 0;
	}
	else if (attr & PCI_BASE_ADDRESS_SPACE_IO) {
		/* An I/O mapped base address */
		attr &= PCI_BASE_ADDRESS_IO_ATTR_MASK;
		resource->flags |= IORESOURCE_IO;
		/* I don't want to deal with 32bit I/O resources */
		resource->limit = 0xffff;
	} 
	else {
		/* A Memory mapped base address */
		attr &= PCI_BASE_ADDRESS_MEM_ATTR_MASK;
		resource->flags |= IORESOURCE_MEM;
		if (attr & PCI_BASE_ADDRESS_MEM_PREFETCH) {
			resource->flags |= IORESOURCE_PREFETCH;
		}
		attr &= PCI_BASE_ADDRESS_MEM_LIMIT_MASK;
		if (attr == PCI_BASE_ADDRESS_MEM_LIMIT_32) {
			/* 32bit limit */
			resource->limit = 0xffffffffUL;
		}
		else if (attr == PCI_BASE_ADDRESS_MEM_LIMIT_1M) {
			/* 1MB limit */
			resource->limit = 0x000fffffUL;
		}
		else if (attr == PCI_BASE_ADDRESS_MEM_LIMIT_64) {
			/* 64bit limit */
			resource->limit = 0xffffffffffffffffULL;
			resource->flags |= IORESOURCE_PCI64;
		}
		else {
			/* Invalid value */
			resource->flags = 0;
		}
	}
	/* Don't let the limit exceed which bits can move */
	if (resource->limit > limit) {
		resource->limit = limit;
	}
#if 0
	if (resource->flags) {
		printk_debug("%s %02x ->",
			dev_path(dev), resource->index);
		printk_debug(" value: 0x%08Lx zeroes: 0x%08Lx ones: 0x%08Lx attr: %08lx\n",
			value, zeroes, ones, attr);
		printk_debug(
			"%s %02x -> size: 0x%08Lx max: 0x%08Lx %s%s\n ",
			dev_path(dev),
			resource->index,
			resource->size,	resource->limit,
			(resource->flags == 0) ? "unused":
			(resource->flags & IORESOURCE_IO)? "io":
			(resource->flags & IORESOURCE_PREFETCH)? "prefmem": "mem",
			(resource->flags & IORESOURCE_PCI64)?"64":"");
	}
#endif

	return resource;
}

/** Read the base address registers for a given device. 
 * @param dev Pointer to the dev structure
 * @param howmany How many registers to read (6 for device, 2 for bridge)
 */
static void pci_read_bases(struct device *dev, unsigned int howmany)
{
	unsigned long index;

	for(index = PCI_BASE_ADDRESS_0; (index < PCI_BASE_ADDRESS_0 + (howmany << 2)); ) {
		struct resource *resource;
		resource = pci_get_resource(dev, index);
		index += (resource->flags & IORESOURCE_PCI64)?8:4;
	}
	compact_resources(dev);
}

static void pci_set_resource(struct device *dev, struct resource *resource);

static void pci_record_bridge_resource(
	struct device *dev, resource_t moving,
	unsigned index, unsigned long mask, unsigned long type)
{
	/* Initiliaze the constraints on the current bus */
	struct resource *resource;
	resource = 0;
	if (moving) {
		unsigned long gran;
		resource_t step;
		resource = new_resource(dev, index);
		resource->size = 0;
		gran = 0;
		step = 1;
		while((moving & step) == 0) {
			gran += 1;
			step <<= 1;
		}
		resource->gran = gran;
		resource->align = gran;
		resource->limit = moving | (step - 1);
		resource->flags = type | IORESOURCE_PCI_BRIDGE;
		compute_allocate_resource(&dev->link[0], resource, mask, type);
		/* If there is nothing behind the resource,
		 * clear it and forget it.
		 */
		if (resource->size == 0) {
			resource->base = moving;
			resource->flags |= IORESOURCE_ASSIGNED;
			resource->flags &= ~IORESOURCE_STORED;
			pci_set_resource(dev, resource);
			resource->flags = 0;
		}
	}
	return;
}


static void pci_bridge_read_bases(struct device *dev)
{
	resource_t moving_base, moving_limit, moving;


	/* See if the bridge I/O resources are implemented */
	moving_base = ((uint32_t)pci_moving_config8(dev, PCI_IO_BASE)) << 8;
	moving_base |= ((uint32_t)pci_moving_config16(dev, PCI_IO_BASE_UPPER16)) << 16;

	moving_limit = ((uint32_t)pci_moving_config8(dev, PCI_IO_LIMIT)) << 8;
	moving_limit |= ((uint32_t)pci_moving_config16(dev, PCI_IO_LIMIT_UPPER16)) << 16;

	moving = moving_base & moving_limit;

	/* Initialize the io space constraints on the current bus */
	pci_record_bridge_resource(
		dev, moving, PCI_IO_BASE, 
		IORESOURCE_IO, IORESOURCE_IO);


	/* See if the bridge prefmem resources are implemented */
	moving_base =  ((resource_t)pci_moving_config16(dev, PCI_PREF_MEMORY_BASE)) << 16;
	moving_base |= ((resource_t)pci_moving_config32(dev, PCI_PREF_BASE_UPPER32)) << 32;

	moving_limit =  ((resource_t)pci_moving_config16(dev, PCI_PREF_MEMORY_LIMIT)) << 16;
	moving_limit |= ((resource_t)pci_moving_config32(dev, PCI_PREF_LIMIT_UPPER32)) << 32;
	
	moving = moving_base & moving_limit;
	/* Initiliaze the prefetchable memory constraints on the current bus */
	pci_record_bridge_resource(
		dev, moving, PCI_PREF_MEMORY_BASE, 
		IORESOURCE_MEM | IORESOURCE_PREFETCH,
		IORESOURCE_MEM | IORESOURCE_PREFETCH);
	

	/* See if the bridge mem resources are implemented */
	moving_base = ((uint32_t)pci_moving_config16(dev, PCI_MEMORY_BASE)) << 16;
	moving_limit = ((uint32_t)pci_moving_config16(dev, PCI_MEMORY_LIMIT)) << 16;

	moving = moving_base & moving_limit;

	/* Initialize the memory resources on the current bus */
	pci_record_bridge_resource(
		dev, moving, PCI_MEMORY_BASE, 
		IORESOURCE_MEM | IORESOURCE_PREFETCH,
		IORESOURCE_MEM);

	compact_resources(dev);
}

void pci_dev_read_resources(struct device *dev)
{
	uint32_t addr;

	pci_read_bases(dev, 6);

	addr = pci_read_config32(dev, PCI_ROM_ADDRESS);
	dev->rom_address = (addr == 0xffffffff)? 0 : addr;
}

void pci_bus_read_resources(struct device *dev)
{
	uint32_t addr;

	pci_bridge_read_bases(dev);
	pci_read_bases(dev, 2);
	
	addr = pci_read_config32(dev, PCI_ROM_ADDRESS1);
	dev->rom_address = (addr == 0xffffffff)? 0 : addr;
}

static void pci_set_resource(struct device *dev, struct resource *resource)
{
	resource_t base, end;

	/* Make certain the resource has actually been set */
	if (!(resource->flags & IORESOURCE_ASSIGNED)) {
		printk_err("ERROR: %s %02x not allocated\n",
			dev_path(dev), resource->index);
		return;
	}

	/* If I have already stored this resource don't worry about it */
	if (resource->flags & IORESOURCE_STORED) {
		return;
	}

	/* If the resources is substractive don't worry about it */
	if (resource->flags & IORESOURCE_SUBTRACTIVE) {
		return;
	}

	/* Only handle PCI memory and IO resources for now */
	if (!(resource->flags & (IORESOURCE_MEM |IORESOURCE_IO)))
		return;

	/* Enable the resources in the command register */
	if (resource->size) {
		if (resource->flags & IORESOURCE_MEM) {
			dev->command |= PCI_COMMAND_MEMORY;
		}
		if (resource->flags & IORESOURCE_IO) {
			dev->command |= PCI_COMMAND_IO;
		}
		if (resource->flags & IORESOURCE_PCI_BRIDGE) {
			dev->command |= PCI_COMMAND_MASTER;
		}
	}
	/* Get the base address */
	base = resource->base;

	/* Get the end */
	end = resource_end(resource);
	
	/* Now store the resource */
	resource->flags |= IORESOURCE_STORED;
	if (!(resource->flags & IORESOURCE_PCI_BRIDGE)) {
		unsigned long base_lo, base_hi;
		/*
		 * some chipsets allow us to set/clear the IO bit. 
		 * (e.g. VIA 82c686a.) So set it to be safe)
		 */
		base_lo = base & 0xffffffff;
		base_hi = (base >> 32) & 0xffffffff;
		if (resource->flags & IORESOURCE_IO) {
			base_lo |= PCI_BASE_ADDRESS_SPACE_IO;
		}
		pci_write_config32(dev, resource->index, base_lo);
		if (resource->flags & IORESOURCE_PCI64) {
			pci_write_config32(dev, resource->index + 4, base_hi);
		}
	}
	else if (resource->index == PCI_IO_BASE) {
		/* set the IO ranges */
		compute_allocate_resource(&dev->link[0], resource, 
			IORESOURCE_IO, IORESOURCE_IO);
		pci_write_config8(dev,  PCI_IO_BASE, base >> 8);
		pci_write_config16(dev, PCI_IO_BASE_UPPER16, base >> 16);
		pci_write_config8(dev,  PCI_IO_LIMIT, end >> 8);
		pci_write_config16(dev, PCI_IO_LIMIT_UPPER16, end >> 16);
	}
	else if (resource->index == PCI_MEMORY_BASE) {
		/* set the memory range	 */
		compute_allocate_resource(&dev->link[0], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH, 
			IORESOURCE_MEM);
		pci_write_config16(dev, PCI_MEMORY_BASE, base >> 16);
		pci_write_config16(dev, PCI_MEMORY_LIMIT, end >> 16);
	}
	else if (resource->index == PCI_PREF_MEMORY_BASE) {
		/* set the prefetchable memory range */
		compute_allocate_resource(&dev->link[0], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH, 
			IORESOURCE_MEM | IORESOURCE_PREFETCH);
		pci_write_config16(dev, PCI_PREF_MEMORY_BASE, base >> 16);
		pci_write_config32(dev, PCI_PREF_BASE_UPPER32, base >> 32);
		pci_write_config16(dev, PCI_PREF_MEMORY_LIMIT, end >> 16);
		pci_write_config32(dev, PCI_PREF_LIMIT_UPPER32, end >> 32);
	}
	else {
		/* Don't let me think I stored the resource */
		resource->flags &= ~IORESOURCE_STORED;
		printk_err("ERROR: invalid resource->index %x\n",
			resource->index);
	}
	report_resource_stored(dev, resource, "");
	return;
}

void pci_dev_set_resources(struct device *dev)
{
	struct resource *resource, *last;
	unsigned link;
	uint8_t line;

	last = &dev->resource[dev->resources];

	for(resource = &dev->resource[0]; resource < last; resource++) {
		pci_set_resource(dev, resource);
	}
	for(link = 0; link < dev->links; link++) {
		struct bus *bus;
		bus = &dev->link[link];
		if (bus->children) {
			assign_resources(bus);
		}
	}

	/* set a default latency timer */
	pci_write_config8(dev, PCI_LATENCY_TIMER, 0x40);

	/* set a default secondary latency timer */
	if ((dev->hdr_type & 0x7f) == PCI_HEADER_TYPE_BRIDGE) {
		pci_write_config8(dev, PCI_SEC_LATENCY_TIMER, 0x40);
	}

	/* zero the irq settings */
	line = pci_read_config8(dev, PCI_INTERRUPT_PIN);
	if (line) {
		pci_write_config8(dev, PCI_INTERRUPT_LINE, 0);
	}
	/* set the cache line size, so far 64 bytes is good for everyone */
	pci_write_config8(dev, PCI_CACHE_LINE_SIZE, 64 >> 2);
}

void pci_dev_enable_resources(struct device *dev)
{
	const struct pci_operations *ops;
	uint16_t command;

	/* Set the subsystem vendor and device id for mainboard devices */
	ops = ops_pci(dev);
	if (dev->on_mainboard && ops && ops->set_subsystem) {
		printk_debug("%s subsystem <- %02x/%02x\n",
			dev_path(dev), 
			MAINBOARD_PCI_SUBSYSTEM_VENDOR_ID,
			MAINBOARD_PCI_SUBSYSTEM_DEVICE_ID);
		ops->set_subsystem(dev, 
			MAINBOARD_PCI_SUBSYSTEM_VENDOR_ID,
			MAINBOARD_PCI_SUBSYSTEM_DEVICE_ID);
	}
	command = pci_read_config16(dev, PCI_COMMAND);
	command |= dev->command;
	command |= (PCI_COMMAND_PARITY + PCI_COMMAND_SERR); /* error check */
	printk_debug("%s cmd <- %02x\n", dev_path(dev), command);
	pci_write_config16(dev, PCI_COMMAND, command);
}

void pci_bus_enable_resources(struct device *dev)
{
	uint16_t ctrl;
	ctrl = pci_read_config16(dev, PCI_BRIDGE_CONTROL);
	ctrl |= dev->link[0].bridge_ctrl;
	ctrl |= (PCI_BRIDGE_CTL_PARITY + PCI_BRIDGE_CTL_SERR); /* error check */
	printk_debug("%s bridge ctrl <- %04x\n", dev_path(dev), ctrl);
	pci_write_config16(dev, PCI_BRIDGE_CONTROL, ctrl);

	pci_dev_enable_resources(dev);

	enable_childrens_resources(dev);
}


void pci_dev_set_subsystem(device_t dev, unsigned vendor, unsigned device)
{
	pci_write_config32(dev, PCI_SUBSYSTEM_VENDOR_ID, 
		((device & 0xffff) << 16) | (vendor & 0xffff));
}

/** Default device operation for PCI devices */
static struct pci_operations pci_dev_ops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

struct device_operations default_pci_ops_dev = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init		  = 0,
	.scan_bus	  = 0,
	.enable           = 0,
	.ops_pci          = &pci_dev_ops_pci,
};

/** Default device operations for PCI bridges */
static struct pci_operations pci_bus_ops_pci = {
	.set_subsystem = 0,
};
struct device_operations default_pci_ops_bus = {
	.read_resources   = pci_bus_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_bus_enable_resources,
	.init		  = 0,
	.scan_bus	  = pci_scan_bridge,
	.enable           = 0,
	.ops_pci          = &pci_bus_ops_pci,
};

/**
 * @brief Set up PCI device operation
 *
 *
 * @param dev 
 *
 * @see pci_drivers
 */
static void set_pci_ops(struct device *dev)
{
	struct pci_driver *driver;
	if (dev->ops) {
		return;
	}

	/* Look through the list of setup drivers and find one for
	 * this pci device 
	 */
	for(driver = &pci_drivers[0]; driver != &epci_drivers[0]; driver++) {
		if ((driver->vendor == dev->vendor) &&
			(driver->device == dev->device)) 
		{
			dev->ops = driver->ops;
			printk_debug("%s [%04x/%04x] %sops\n", 
				dev_path(dev),
				driver->vendor, driver->device,
				(driver->ops->scan_bus?"bus ":""));
			return;
		}
	}

	/* If I don't have a specific driver use the default operations */
	switch(dev->hdr_type & 0x7f) {	/* header type */
	case PCI_HEADER_TYPE_NORMAL:	/* standard header */
		if ((dev->class >> 8) == PCI_CLASS_BRIDGE_PCI)
			goto bad;
		dev->ops = &default_pci_ops_dev;
		break;
	case PCI_HEADER_TYPE_BRIDGE:
		if ((dev->class >> 8) != PCI_CLASS_BRIDGE_PCI)
			goto bad;
		dev->ops = &default_pci_ops_bus;
		break;
	default:
	bad:
		if (dev->enabled) {
			printk_err("%s [%04x/%04x/%06x] has unknown header "
				"type %02x, ignoring.\n",
				dev_path(dev),
				dev->vendor, dev->device, 
				dev->class >> 8, dev->hdr_type);
		}
	}
	return;
}

/**
 * @brief See if we have already allocated a device structure for a given devfn.
 *
 * Given a linked list of PCI device structures and a devfn number, find the
 * device structure correspond to the devfn, if present.
 *
 * @param list the device structure list
 * @param devfn a device/function number
 *
 * @return pointer to the device structure found or null of we have not allocated
 *         a device for this devfn yet.
 */
static struct device *pci_scan_get_dev(struct device **list, unsigned int devfn)
{
	struct device *dev;
	dev = 0;
	for(; *list; list = &(*list)->sibling) {
		if ((*list)->path.type != DEVICE_PATH_PCI) {
			printk_err("child %s not a pci device\n",
				dev_path(*list));
			continue;
		}
		if ((*list)->path.u.pci.devfn == devfn) {
			/* Unlink from the list */
			dev = *list;
			*list = (*list)->sibling;
			dev->sibling = 0;
			break;
		}
	}
	/* Just like alloc_dev add the device to the 
	 * list of device on the bus.  When the list of devices was formed
	 * we removed all of the parents children, and now we are interleaving
	 * static and dynamic devices in order on the bus.
	 */
	if (dev) {
		device_t child;
		/* Find the last child of our parent */
		for(child = dev->bus->children; child && child->sibling; ) {
			child = child->sibling;
		}
		/* Place the device on the list of children of it's parent. */
		if (child) {
			child->sibling = dev;
		} else {
			dev->bus->children = dev;
		}
	}

	return dev;
}

/** 
 * @brief Scan a PCI bus.
 *
 * Determine the existence of devices and bridges on a PCI bus. If there are
 * bridges on the bus, recursively scan the buses behind the bridges.
 *
 * This function is the default scan_bus() method for the root device
 * 'dev_root'.
 *
 * @param bus pointer to the bus structure
 * @param min_devfn minimum devfn to look at in the scan usually 0x00
 * @param max_devfn maximum devfn to look at in the scan usually 0xff
 * @param max current bus number
 *
 * @return The maximum bus number found, after scanning all subordinate busses
 */
unsigned int pci_scan_bus(struct bus *bus,
	unsigned min_devfn, unsigned max_devfn,
	unsigned int max)
{
	unsigned int devfn;
	device_t dev;
	device_t old_devices;
	device_t child;

	printk_debug("PCI: pci_scan_bus for bus %d\n", bus->secondary);

	old_devices = bus->children;
	bus->children = 0;

	post_code(0x24);

	/* probe all devices/functions on this bus with some optimization for
	 * non-existence and single funcion devices
	 */
	for (devfn = min_devfn; devfn <= max_devfn; devfn++) {
		uint32_t id, class;
		uint8_t hdr_type;

		/* First thing setup the device structure */
		dev = pci_scan_get_dev(&old_devices, devfn);
	
		/* Detect if a device is present */
		if (!dev) {
			struct device dummy;
			dummy.bus              = bus;
			dummy.path.type        = DEVICE_PATH_PCI;
			dummy.path.u.pci.devfn = devfn;
			id = pci_read_config32(&dummy, PCI_VENDOR_ID);
			/* some broken boards return 0 if a slot is empty: */
			if (	(id == 0xffffffff) || (id == 0x00000000) || 
				(id == 0x0000ffff) || (id == 0xffff0000))
			{
				printk_spew("PCI: devfn 0x%x, bad id 0x%x\n", devfn, id);
				if (PCI_FUNC(devfn) == 0x00) {
					/* if this is a function 0 device and 
					 * it is not present,
					 * skip to next device 
					 */
					devfn += 0x07;
				}
				/* This function in a multi function device is
				 * not present, skip to the next function.
				 */
				continue;
			}
			dev = alloc_dev(bus, &dummy.path);
		}
		else {
			/* Enable/disable the device.  Once we have
			 * found the device specific operations this
			 * operations we will disable the device with
			 * those as well.
			 * 
			 * This is geared toward devices that have subfunctions
			 * that do not show up by default.
			 * 
			 * If a device is a stuff option on the motherboard
			 * it may be absent and enable_dev must cope.
			 * 
			 */
			if (dev->chip_ops && dev->chip_ops->enable_dev) 
			{
				dev->chip_ops->enable_dev(dev);
			}
			/* Now read the vendor and device id */
			id = pci_read_config32(dev, PCI_VENDOR_ID);
			
			/* If the device does not have a pci id disable it.
			 * Possibly this is because we have already disabled
			 * the device.  But this also handles optional devices
			 * that may not always show up.
			 */
			if (id == 0xffffffff || id == 0x00000000 ||
				id == 0x0000ffff || id == 0xffff0000) 
			{
				if (dev->enabled) {
					printk_info("Disabling static device: %s\n",
						dev_path(dev));
					dev->enabled = 0;
				}
			}
		}
		/* Read the rest of the pci configuration information */
		hdr_type = pci_read_config8(dev, PCI_HEADER_TYPE);
		class = pci_read_config32(dev, PCI_CLASS_REVISION);

		/* Store the interesting information in the device structure */
		dev->vendor = id & 0xffff;
		dev->device = (id >> 16) & 0xffff;
		dev->hdr_type = hdr_type;
		/* class code, the upper 3 bytes of PCI_CLASS_REVISION */
		dev->class = class >> 8;

		/* Architectural/System devices always need to
		 * be bus masters.
		 */
		if ((dev->class >> 16) == PCI_BASE_CLASS_SYSTEM) {
			dev->command |= PCI_COMMAND_MASTER;
		}

		/* Look at the vendor and device id, or at least the 
		 * header type and class and figure out which set of
		 * configuration methods to use.  Unless we already
		 * have some pci ops.
		 */
		set_pci_ops(dev);
		/* Error if we don't have some pci operations for it */
		if (!dev->ops) {
			printk_err("%s No device operations\n",
				dev_path(dev));
			continue;
		}

		/* Now run the magic enable/disable sequence for the device */
		if (dev->ops && dev->ops->enable) {
			dev->ops->enable(dev);
		}

		printk_debug("%s [%04x/%04x] %s\n", 
			dev_path(dev),
			dev->vendor, dev->device, 
			dev->enabled?"enabled": "disabled");

		if (PCI_FUNC(devfn) == 0x00 && (hdr_type & 0x80) != 0x80) {
			/* if this is not a multi function device, 
			 * don't waste time probing another function. 
			 * Skip to next device. 
			 */
			devfn += 0x07;
		}
	}
	post_code(0x25);

	/* For all children that implement scan_bus (i.e. bridges)
	 * scan the bus behind that child.
	 */
	for(child = bus->children; child; child = child->sibling) {
		if (!child->enabled ||
			!child->ops || 
			!child->ops->scan_bus) 
		{
			continue;
		}
		max = child->ops->scan_bus(child, max);
	}

	/*
	 * We've scanned the bus and so we know all about what's on
	 * the other side of any bridges that may be on this bus plus
	 * any devices.
	 *
	 * Return how far we've got finding sub-buses.
	 */
	printk_debug("PCI: pci_scan_bus returning with max=%02x\n", max);
	post_code(0x55);
	return max;
}

/**
 * @brief Scan a PCI bridge and the buses behind the bridge.
 *
 * Determine the existence of buses behind the bridge. Set up the bridge
 * according to the result of the scan.
 *
 * This function is the default scan_bus() method for PCI bridge devices.
 *
 * @param dev pointer to the bridge device
 * @param max the highest bus number assgined up to now
 *
 * @return The maximum bus number found, after scanning all subordinate busses
 */
unsigned int pci_scan_bridge(struct device *dev, unsigned int max)
{
	struct bus *bus;
	uint32_t buses;
	uint16_t cr;

	bus = &dev->link[0];
	bus->dev = dev;
	dev->links = 1;

	/* Set up the primary, secondary and subordinate bus numbers. We have
	 * no idea how many buses are behind this bridge yet, so we set the
	 * subordinate bus number to 0xff for the moment. 
	 */
	bus->secondary = ++max;
	bus->subordinate = 0xff;

	/* Clear all status bits and turn off memory, I/O and master enables. */
	cr = pci_read_config16(dev, PCI_COMMAND);
	pci_write_config16(dev, PCI_COMMAND, 0x0000);
	pci_write_config16(dev, PCI_STATUS, 0xffff);

	/*
	 * Read the existing primary/secondary/subordinate bus
	 * number configuration.
	 */
	buses = pci_read_config32(dev, PCI_PRIMARY_BUS);

	/* Configure the bus numbers for this bridge: the configuration
	 * transactions will not be propagated by the bridge if it is not
	 * correctly configured.
	 */
	buses &= 0xff000000;
	buses |= (((unsigned int) (dev->bus->secondary) << 0) |
		((unsigned int) (bus->secondary) << 8) |
		((unsigned int) (bus->subordinate) << 16));
	pci_write_config32(dev, PCI_PRIMARY_BUS, buses);
	
	/* Now we can scan all subordinate buses 
	 * i.e. the bus behind the bridge.
	 */
	max = pci_scan_bus(bus, 0x00, 0xff, max);
	
	/* We know the number of buses behind this bridge. Set the subordinate
	 * bus number to its real value.
	 */
	bus->subordinate = max;
	buses = (buses & 0xff00ffff) |
		((unsigned int) (bus->subordinate) << 16);
	pci_write_config32(dev, PCI_PRIMARY_BUS, buses);
	pci_write_config16(dev, PCI_COMMAND, cr);
		
	printk_spew("%s returns max %d\n", __func__, max);
	return max;
}

/*
    Tell the EISA int controller this int must be level triggered
    THIS IS A KLUDGE -- sorry, this needs to get cleaned up.
*/
static void pci_level_irq(unsigned char intNum)
{
	unsigned short intBits = inb(0x4d0) | (((unsigned) inb(0x4d1)) << 8);

	printk_spew("%s: current ints are 0x%x\n", __func__, intBits);
	intBits |= (1 << intNum);

	printk_spew("%s: try to set ints 0x%x\n", __func__, intBits);

	// Write new values
	outb((unsigned char) intBits, 0x4d0);
	outb((unsigned char) (intBits >> 8), 0x4d1);

	/* this seems like an error but is not ... */
#if 1
	if (inb(0x4d0) != (intBits & 0xf)) {
	  printk_err("%s: lower order bits are wrong: want 0x%x, got 0x%x\n",
		     __func__, intBits &0xf, inb(0x4d0));
	}
	if (inb(0x4d1) != ((intBits >> 8) & 0xf)) {
	  printk_err("%s: lower order bits are wrong: want 0x%x, got 0x%x\n",
		     __func__, (intBits>>8) &0xf, inb(0x4d1));
	}
#endif
}

/*
    This function assigns IRQs for all functions contained within
    the indicated device address.  If the device does not exist or does
    not require interrupts then this function has no effect.

    This function should be called for each PCI slot in your system.  

    pIntAtoD is an array of IRQ #s that are assigned to PINTA through PINTD of
    this slot.  
    The particular irq #s that are passed in depend on the routing inside
    your southbridge and on your motherboard.

    -kevinh@ispiri.com
*/
void pci_assign_irqs(unsigned bus, unsigned slot,
	const unsigned char pIntAtoD[4])
{
	unsigned functNum;
	device_t pdev;
	unsigned char line;
	unsigned char irq;
	unsigned char readback;

	/* Each slot may contain up to eight functions */
	for (functNum = 0; functNum < 8; functNum++) {
		pdev = dev_find_slot(bus, (slot << 3) + functNum);

		if (pdev) {
		  line = pci_read_config8(pdev, PCI_INTERRUPT_PIN);

			// PCI spec says all other values are reserved 
			if ((line >= 1) && (line <= 4)) {
				irq = pIntAtoD[line - 1];

				printk_debug("Assigning IRQ %d to %d:%x.%d\n", \
					irq, bus, slot, functNum);

				pci_write_config8(pdev, PCI_INTERRUPT_LINE,\
					pIntAtoD[line - 1]);

				readback = pci_read_config8(pdev, PCI_INTERRUPT_LINE);
				printk_debug("  Readback = %d\n", readback);

				// Change to level triggered
				pci_level_irq(pIntAtoD[line - 1]);
			}
		}
	}
}
