/*
 * This file is part of the coreboot project.
 *
 * It was originally based on the Linux kernel (arch/i386/kernel/pci-pc.c).
 *
 * Modifications are:
 * Copyright (C) 2003 Eric Biederman <ebiederm@xmission.com>
 * Copyright (C) 2003-2004 Linux Networx
 * (Written by Eric Biederman <ebiederman@lnxi.com> for Linux Networx)
 * Copyright (C) 2003 Ronald G. Minnich <rminnich@gmail.com>
 * Copyright (C) 2004-2005 Li-Ta Lo <ollie@lanl.gov>
 * Copyright (C) 2005-2006 Tyan
 * (Written by Yinghai Lu <yhlu@tyan.com> for Tyan)
 * Copyright (C) 2005-2006 Stefan Reinauer <stepan@openbios.org>
 * Copyright (C) 2009 Myles Watson <mylesgw@gmail.com>
 */

/*
 *      (c) 1999--2000 Martin Mares <mj@suse.cz>
 */
/* lots of mods by ron minnich (rminnich@lanl.gov), with
 * the final architecture guidance from Tom Merritt (tjm@codegen.com)
 * In particular, we changed from the one-pass original version to
 * Tom's recommended multiple-pass version. I wasn't sure about doing
 * it with multiple passes, until I actually started doing it and saw
 * the wisdom of Tom's recommendations ...
 *
 * Lots of cleanups by Eric Biederman to handle bridges, and to
 * handle resource allocation for non-pci devices.
 */

#include <console/console.h>
#include <bitops.h>
#include <arch/io.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <stdlib.h>
#include <string.h>
#include <smp/spinlock.h>

/** Linked list of ALL devices */
struct device *all_devices = &dev_root;
/** Pointer to the last device */
extern struct device **last_dev_p;


/**
 * @brief Allocate a new device structure.
 *
 * Allocte a new device structure and attached it to the device tree as a
 * child of the parent bus.
 *
 * @param parent parent bus the newly created device attached to.
 * @param path path to the device to be created.
 *
 * @return pointer to the newly created device structure.
 *
 * @see device_path
 */
static spinlock_t dev_lock = SPIN_LOCK_UNLOCKED;
device_t alloc_dev(struct bus *parent, struct device_path *path)
{
	device_t dev, child;
	int link;

	spin_lock(&dev_lock);

	/* Find the last child of our parent. */
	for (child = parent->children; child && child->sibling; /* */ ) {
		child = child->sibling;
	}

	dev = malloc(sizeof(*dev));
	if (dev == 0)
		die("DEV: out of memory.\n");

	memset(dev, 0, sizeof(*dev));
	memcpy(&dev->path, path, sizeof(*path));

	/* Initialize the back pointers in the link fields. */
	for (link = 0; link < MAX_LINKS; link++) {
		dev->link[link].dev = dev;
		dev->link[link].link = link;
	}

	/* By default devices are enabled. */
	dev->enabled = 1;

	/* Add the new device to the list of children of the bus. */
	dev->bus = parent;
	if (child) {
		child->sibling = dev;
	} else {
		parent->children = dev;
	}

	/* Append a new device to the global device list.
	 * The list is used to find devices once everything is set up.
	 */
	*last_dev_p = dev;
	last_dev_p = &dev->next;

	spin_unlock(&dev_lock);
	return dev;
}

/**
 * @brief round a number up to an alignment.
 * @param val the starting value
 * @param roundup Alignment as a power of two
 * @returns rounded up number
 */
static resource_t round(resource_t val, unsigned long pow)
{
	resource_t mask;
	mask = (1ULL << pow) - 1ULL;
	val += mask;
	val &= ~mask;
	return val;
}

/** Read the resources on all devices of a given bus.
 * @param bus bus to read the resources on.
 */
static void read_resources(struct bus *bus)
{
	struct device *curdev;

	printk_spew("%s %s bus %x link: %d\n", dev_path(bus->dev), __func__,
		    bus->secondary, bus->link);

	/* Walk through all devices and find which resources they need. */
	for (curdev = bus->children; curdev; curdev = curdev->sibling) {
		int i;
		if (!curdev->enabled) {
			continue;
		}
		if (!curdev->ops || !curdev->ops->read_resources) {
			printk_err("%s missing read_resources\n",
				   dev_path(curdev));
			continue;
		}
		curdev->ops->read_resources(curdev);

		/* Read in the resources behind the current device's links. */
		for (i = 0; i < curdev->links; i++)
			read_resources(&curdev->link[i]);
	}
	printk_spew("%s read_resources bus %d link: %d done\n",
		    dev_path(bus->dev), bus->secondary, bus->link);
}

struct pick_largest_state {
	struct resource *last;
	struct device *result_dev;
	struct resource *result;
	int seen_last;
};

static void pick_largest_resource(void *gp, struct device *dev,
				  struct resource *resource)
{
	struct pick_largest_state *state = gp;
	struct resource *last;

	last = state->last;

	/* Be certain to pick the successor to last. */
	if (resource == last) {
		state->seen_last = 1;
		return;
	}
	if (resource->flags & IORESOURCE_FIXED)
		return;		// Skip it.
	if (last && ((last->align < resource->align) ||
		     ((last->align == resource->align) &&
		      (last->size < resource->size)) ||
		     ((last->align == resource->align) &&
		      (last->size == resource->size) && (!state->seen_last)))) {
		return;
	}
	if (!state->result ||
	    (state->result->align < resource->align) ||
	    ((state->result->align == resource->align) &&
	     (state->result->size < resource->size))) {
		state->result_dev = dev;
		state->result = resource;
	}
}

static struct device *largest_resource(struct bus *bus,
				       struct resource **result_res,
				       unsigned long type_mask,
				       unsigned long type)
{
	struct pick_largest_state state;

	state.last = *result_res;
	state.result_dev = NULL;
	state.result = NULL;
	state.seen_last = 0;

	search_bus_resources(bus, type_mask, type, pick_largest_resource,
			     &state);

	*result_res = state.result;
	return state.result_dev;
}

/* Compute allocate resources is the guts of the resource allocator.
 *
 * The problem.
 *  - Allocate resource locations for every device.
 *  - Don't overlap, and follow the rules of bridges.
 *  - Don't overlap with resources in fixed locations.
 *  - Be efficient so we don't have ugly strategies.
 *
 * The strategy.
 * - Devices that have fixed addresses are the minority so don't
 *   worry about them too much. Instead only use part of the address
 *   space for devices with programmable addresses. This easily handles
 *   everything except bridges.
 *
 * - PCI devices are required to have their sizes and their alignments
 *   equal. In this case an optimal solution to the packing problem
 *   exists. Allocate all devices from highest alignment to least
 *   alignment or vice versa. Use this.
 *
 * - So we can handle more than PCI run two allocation passes on bridges. The
 *   first to see how large the resources are behind the bridge, and what
 *   their alignment requirements are. The second to assign a safe address to
 *   the devices behind the bridge. This allows us to treat a bridge as just
 *   a device with a couple of resources, and not need to special case it in
 *   the allocator. Also this allows handling of other types of bridges.
 *
 */
void compute_resources(struct bus *bus, struct resource *bridge,
		       unsigned long type_mask, unsigned long type)
{
	struct device *dev;
	struct resource *resource;
	resource_t base;
	base = round(bridge->base, bridge->align);

	printk_spew( "%s %s_%s: base: %llx size: %llx align: %d gran: %d limit: %llx\n",
	       dev_path(bus->dev), __func__,
	       (type & IORESOURCE_IO) ? "io" : (type & IORESOURCE_PREFETCH) ?
	       "prefmem" : "mem",
	       base, bridge->size, bridge->align, bridge->gran, bridge->limit);

	/* For each child which is a bridge, compute_resource_needs. */
	for (dev = bus->children; dev; dev = dev->sibling) {
		unsigned i;
		struct resource *child_bridge;

		if (!dev->links)
			continue;

		/* Find the resources with matching type flags. */
		for (i = 0; i < dev->resources; i++) {
			unsigned link;
			child_bridge = &dev->resource[i];

			if (!(child_bridge->flags & IORESOURCE_BRIDGE) ||
			    (child_bridge->flags & type_mask) != type)
				continue;

			/* Split prefetchable memory if combined.  Many domains
			 * use the same address space for prefetchable memory
			 * and non-prefetchable memory.  Bridges below them
			 * need it separated.  Add the PREFETCH flag to the
			 * type_mask and type.
			 */
			link = IOINDEX_LINK(child_bridge->index);
			compute_resources(&dev->link[link], child_bridge,
					  type_mask | IORESOURCE_PREFETCH,
					  type | (child_bridge->flags &
						  IORESOURCE_PREFETCH));
		}
	}

	/* Remember we haven't found anything yet. */
	resource = NULL;

	/* Walk through all the resources on the current bus and compute the
	 * amount of address space taken by them.  Take granularity and
	 * alignment into account.
	 */
	while ((dev = largest_resource(bus, &resource, type_mask, type))) {

		/* Size 0 resources can be skipped. */
		if (!resource->size) {
			continue;
		}

		/* Propagate the resource alignment to the bridge resource. */
		if (resource->align > bridge->align) {
			bridge->align = resource->align;
		}

		/* Propagate the resource limit to the bridge register. */
		if (bridge->limit > resource->limit) {
			bridge->limit = resource->limit;
		}

		/* Warn if it looks like APICs aren't declared. */
		if ((resource->limit == 0xffffffff) &&
		    (resource->flags & IORESOURCE_ASSIGNED)) {
			printk_err("Resource limit looks wrong! (no APIC?)\n");
			printk_err("%s %02lx limit %08Lx\n", dev_path(dev),
			           resource->index, resource->limit);
		}

		if (resource->flags & IORESOURCE_IO) {
			/* Don't allow potential aliases over the legacy PCI
			 * expansion card addresses. The legacy PCI decodes
			 * only 10 bits, uses 0x100 - 0x3ff. Therefore, only
			 * 0x00 - 0xff can be used out of each 0x400 block of
			 * I/O space.
			 */
			if ((base & 0x300) != 0) {
				base = (base & ~0x3ff) + 0x400;
			}
			/* Don't allow allocations in the VGA I/O range.
			 * PCI has special cases for that.
			 */
			else if ((base >= 0x3b0) && (base <= 0x3df)) {
				base = 0x3e0;
			}
		}
		/* Base must be aligned. */
		base = round(base, resource->align);
		resource->base = base;
		base += resource->size;

		printk_spew("%s %02lx *  [0x%llx - 0x%llx] %s\n",
			    dev_path(dev), resource->index,
			    resource->base,
			    resource->base + resource->size - 1,
			    (resource->flags & IORESOURCE_IO) ? "io" :
			    (resource->flags & IORESOURCE_PREFETCH) ?
			     "prefmem" : "mem");
	}
	/* A pci bridge resource does not need to be a power
	 * of two size, but it does have a minimum granularity.
	 * Round the size up to that minimum granularity so we
	 * know not to place something else at an address postitively
	 * decoded by the bridge.
	 */
	bridge->size = round(base, bridge->gran) -
		       round(bridge->base, bridge->align);

	printk_spew("%s %s_%s: base: %llx size: %llx align: %d gran: %d limit: %llx done\n",
		    dev_path(bus->dev), __func__,
		    (bridge->flags & IORESOURCE_IO) ? "io" :
		     (bridge->flags & IORESOURCE_PREFETCH) ?  "prefmem" : "mem",
		    base, bridge->size, bridge->align, bridge->gran, bridge->limit);
}

/**
 * This function is the second part of the resource allocator.
 *
 * The problem.
 *  - Allocate resource locations for every device.
 *  - Don't overlap, and follow the rules of bridges.
 *  - Don't overlap with resources in fixed locations.
 *  - Be efficient so we don't have ugly strategies.
 *
 * The strategy.
 * - Devices that have fixed addresses are the minority so don't
 *   worry about them too much. Instead only use part of the address
 *   space for devices with programmable addresses. This easily handles
 *   everything except bridges.
 *
 * - PCI devices are required to have their sizes and their alignments
 *   equal. In this case an optimal solution to the packing problem
 *   exists. Allocate all devices from highest alignment to least
 *   alignment or vice versa. Use this.
 *
 * - So we can handle more than PCI run two allocation passes on bridges. The
 *   first to see how large the resources are behind the bridge, and what
 *   their alignment requirements are. The second to assign a safe address to
 *   the devices behind the bridge. This allows us to treat a bridge as just
 *   a device with a couple of resources, and not need to special case it in
 *   the allocator. Also this allows handling of other types of bridges.
 *
 * - This function assigns the resources a value.
 *
 * @param bus The bus we are traversing.
 * @param bridge The bridge resource which must contain the bus' resources.
 * @param type_mask This value gets anded with the resource type.
 * @param type This value must match the result of the and.
 */
void allocate_resources(struct bus *bus, struct resource *bridge,
			unsigned long type_mask, unsigned long type)
{
	struct device *dev;
	struct resource *resource;
	resource_t base;
	base = bridge->base;

	printk_spew("%s %s_%s: base:%llx size:%llx align:%d gran:%d limit:%llx\n",
	       dev_path(bus->dev), __func__,
	       (type & IORESOURCE_IO) ? "io" : (type & IORESOURCE_PREFETCH) ?
	       "prefmem" : "mem",
	       base, bridge->size, bridge->align, bridge->gran, bridge->limit);

	/* Remember we haven't found anything yet. */
	resource = NULL;

	/* Walk through all the resources on the current bus and allocate them
	 * address space.
	 */
	while ((dev = largest_resource(bus, &resource, type_mask, type))) {

		/* Propagate the bridge limit to the resource register. */
		if (resource->limit > bridge->limit) {
			resource->limit = bridge->limit;
		}

		/* Size 0 resources can be skipped. */
		if (!resource->size) {
			/* Set the base to limit so it doesn't confuse tolm. */
			resource->base = resource->limit;
			resource->flags |= IORESOURCE_ASSIGNED;
			continue;
		}

		if (resource->flags & IORESOURCE_IO) {
			/* Don't allow potential aliases over the legacy PCI
			 * expansion card addresses. The legacy PCI decodes
			 * only 10 bits, uses 0x100 - 0x3ff. Therefore, only
			 * 0x00 - 0xff can be used out of each 0x400 block of
			 * I/O space.
			 */
			if ((base & 0x300) != 0) {
				base = (base & ~0x3ff) + 0x400;
			}
			/* Don't allow allocations in the VGA I/O range.
			 * PCI has special cases for that.
			 */
			else if ((base >= 0x3b0) && (base <= 0x3df)) {
				base = 0x3e0;
			}
		}

		if ((round(base, resource->align) + resource->size - 1) <=
		    resource->limit) {
			/* Base must be aligned. */
			base = round(base, resource->align);
			resource->base = base;
			resource->flags |= IORESOURCE_ASSIGNED;
			resource->flags &= ~IORESOURCE_STORED;
			base += resource->size;
		} else {
			printk_err("!! Resource didn't fit !!\n");
			printk_err("   aligned base %llx size %llx limit %llx\n",
			       round(base, resource->align), resource->size,
			       resource->limit);
			printk_err("   %llx needs to be <= %llx (limit)\n",
			       (round(base, resource->align) +
				resource->size) - 1, resource->limit);
			printk_err("   %s%s %02lx *  [0x%llx - 0x%llx] %s\n",
			       (resource->
				flags & IORESOURCE_ASSIGNED) ? "Assigned: " :
			       "", dev_path(dev), resource->index,
			       resource->base,
			       resource->base + resource->size - 1,
			       (resource->
				flags & IORESOURCE_IO) ? "io" : (resource->
								 flags &
								 IORESOURCE_PREFETCH)
			       ? "prefmem" : "mem");
		}

		printk_spew("%s%s %02lx *  [0x%llx - 0x%llx] %s\n",
		       (resource->flags & IORESOURCE_ASSIGNED) ? "Assigned: "
		       : "",
		       dev_path(dev), resource->index, resource->base,
		       resource->size ? resource->base + resource->size - 1 :
		       resource->base,
		       (resource->flags & IORESOURCE_IO) ? "io" :
		       (resource->flags & IORESOURCE_PREFETCH) ? "prefmem" :
		       "mem");
	}
	/* A PCI bridge resource does not need to be a power of two size, but
	 * it does have a minimum granularity. Round the size up to that
	 * minimum granularity so we know not to place something else at an
	 * address positively decoded by the bridge.
	 */

	bridge->flags |= IORESOURCE_ASSIGNED;

	printk_spew("%s %s_%s: next_base: %llx size: %llx align: %d gran: %d done\n",
	       dev_path(bus->dev), __func__,
	       (type & IORESOURCE_IO) ? "io" : (type & IORESOURCE_PREFETCH) ?
	       "prefmem" : "mem",
	       base, bridge->size, bridge->align, bridge->gran);

	/* For each child which is a bridge, allocate_resources. */
	for (dev = bus->children; dev; dev = dev->sibling) {
		unsigned i;
		struct resource *child_bridge;

		if (!dev->links)
			continue;

		/* Find the resources with matching type flags. */
		for (i = 0; i < dev->resources; i++) {
			unsigned link;
			child_bridge = &dev->resource[i];

			if (!(child_bridge->flags & IORESOURCE_BRIDGE) ||
			    (child_bridge->flags & type_mask) != type)
				continue;

			/* Split prefetchable memory if combined.  Many domains
			 * use the same address space for prefetchable memory
			 * and non-prefetchable memory.  Bridges below them
			 * need it separated.  Add the PREFETCH flag to the
			 * type_mask and type.
			 */
			link = IOINDEX_LINK(child_bridge->index);
			allocate_resources(&dev->link[link], child_bridge,
					   type_mask | IORESOURCE_PREFETCH,
					   type | (child_bridge->flags &
						   IORESOURCE_PREFETCH));
		}
	}
}

#if CONFIG_PCI_64BIT_PREF_MEM == 1
	#define MEM_MASK (IORESOURCE_PREFETCH | IORESOURCE_MEM)
#else
	#define MEM_MASK (IORESOURCE_MEM)
#endif
#define IO_MASK (IORESOURCE_IO)
#define PREF_TYPE (IORESOURCE_PREFETCH | IORESOURCE_MEM)
#define MEM_TYPE (IORESOURCE_MEM)
#define IO_TYPE (IORESOURCE_IO)

struct constraints {
	struct resource pref, io, mem;
};

static void constrain_resources(struct device *dev, struct constraints* limits)
{
	struct device *child;
	struct resource *res;
	struct resource *lim;
	int i;

	printk_spew("%s: %s\n", __func__, dev_path(dev));

	/* Constrain limits based on the fixed resources of this device. */
	for (i = 0; i < dev->resources; i++) {
		res = &dev->resource[i];
		if (!(res->flags & IORESOURCE_FIXED))
			continue;
		if (!res->size) {
			/* It makes no sense to have 0-sized, fixed resources.*/
			printk_err("skipping %s@%lx fixed resource, size=0!\n",
				   dev_path(dev), res->index);
			continue;
		}

		/* PREFETCH, MEM, or I/O - skip any others. */
		if ((res->flags & MEM_MASK) == PREF_TYPE)
			lim = &limits->pref;
		else if ((res->flags & MEM_MASK) == MEM_TYPE)
			lim = &limits->mem;
		else if ((res->flags & IO_MASK) == IO_TYPE)
			lim = &limits->io;
		else
			continue;

		/* Is it already outside the limits? */
		if (res->size && (((res->base + res->size -1) < lim->base) ||
				  (res->base > lim->limit)))
			continue;

		/* Choose to be above or below fixed resources.  This
		 * check is signed so that "negative" amounts of space
		 * are handled correctly.
		 */
		if ((signed long long)(lim->limit - (res->base + res->size -1)) >
		    (signed long long)(res->base - lim->base))
			lim->base = res->base + res->size;
		else
			lim->limit = res->base -1;
	}

	/* Descend into every enabled child and look for fixed resources. */
	for (i = 0; i < dev->links; i++)
		for (child = dev->link[i].children; child;
		     child = child->sibling)
			if (child->enabled)
				constrain_resources(child, limits);
}

static void avoid_fixed_resources(struct device *dev)
{
	struct constraints limits;
	struct resource *res;
	int i;

	printk_spew("%s: %s\n", __func__, dev_path(dev));
	/* Initialize constraints to maximum size. */

	limits.pref.base = 0;
	limits.pref.limit = 0xffffffffffffffffULL;
	limits.io.base = 0;
	limits.io.limit = 0xffffffffffffffffULL;
	limits.mem.base = 0;
	limits.mem.limit = 0xffffffffffffffffULL;

	/* Constrain the limits to dev's initial resources. */
	for (i = 0; i < dev->resources; i++) {
		res = &dev->resource[i];
		if ((res->flags & IORESOURCE_FIXED))
			continue;
		printk_spew("%s:@%s %02lx limit %08Lx\n", __func__,
			     dev_path(dev), res->index, res->limit);
		if ((res->flags & MEM_MASK) == PREF_TYPE &&
		    (res->limit < limits.pref.limit))
			limits.pref.limit = res->limit;
		if ((res->flags & MEM_MASK) == MEM_TYPE &&
		    (res->limit < limits.mem.limit))
			limits.mem.limit = res->limit;
		if ((res->flags & IO_MASK) == IO_TYPE &&
		    (res->limit < limits.io.limit))
			limits.io.limit = res->limit;
	}

	/* Look through the tree for fixed resources and update the limits. */
	constrain_resources(dev, &limits);

	/* Update dev's resources with new limits. */
	for (i = 0; i < dev->resources; i++) {
		struct resource *lim;
		res = &dev->resource[i];

		if ((res->flags & IORESOURCE_FIXED))
			continue;

		/* PREFETCH, MEM, or I/O - skip any others. */
		if ((res->flags & MEM_MASK) == PREF_TYPE)
			lim = &limits.pref;
		else if ((res->flags & MEM_MASK) == MEM_TYPE)
			lim = &limits.mem;
		else if ((res->flags & IO_MASK) == IO_TYPE)
			lim = &limits.io;
		else
			continue;

		printk_spew("%s2: %s@%02lx limit %08Lx\n", __func__,
			     dev_path(dev), res->index, res->limit);
		printk_spew("\tlim->base %08Lx lim->limit %08Lx\n",
			     lim->base, lim->limit);

		/* Is the resource outside the limits? */
		if (lim->base > res->base)
			res->base = lim->base;
		if (res->limit > lim->limit)
			res->limit = lim->limit;
	}
}

#if CONFIG_CONSOLE_VGA == 1
device_t vga_pri = 0;
static void set_vga_bridge_bits(void)
{
#warning "FIXME modify set_vga_bridge so it is less pci centric!"
#warning "This function knows too much about PCI stuff, it should be just a iterator/visitor."

	/* FIXME: Handle the VGA palette snooping. */
	struct device *dev, *vga, *vga_onboard, *vga_first, *vga_last;
	struct bus *bus;
	bus = 0;
	vga = 0;
	vga_onboard = 0;
	vga_first = 0;
	vga_last = 0;
	for (dev = all_devices; dev; dev = dev->next) {
		if (!dev->enabled)
			continue;
		if (((dev->class >> 16) == PCI_BASE_CLASS_DISPLAY) &&
		    ((dev->class >> 8) != PCI_CLASS_DISPLAY_OTHER)) {
			if (!vga_first) {
				if (dev->on_mainboard) {
					vga_onboard = dev;
				} else {
					vga_first = dev;
				}
			} else {
				if (dev->on_mainboard) {
					vga_onboard = dev;
				} else {
					vga_last = dev;
				}
			}

			/* It isn't safe to enable other VGA cards. */
			dev->command &= ~(PCI_COMMAND_MEMORY | PCI_COMMAND_IO);
		}
	}

	vga = vga_last;

	if (!vga) {
		vga = vga_first;
	}
#if CONFIG_CONSOLE_VGA_ONBOARD_AT_FIRST == 1
	if (vga_onboard)	// Will use on board VGA as pri.
#else
	if (!vga)		// Will use last add on adapter as pri.
#endif
	{
		vga = vga_onboard;
	}

	if (vga) {
		/* VGA is first add on card or the only onboard VGA. */
		printk_debug("Setting up VGA for %s\n", dev_path(vga));
		/* All legacy VGA cards have MEM & I/O space registers. */
		vga->command |= (PCI_COMMAND_MEMORY | PCI_COMMAND_IO);
		vga_pri = vga;
		bus = vga->bus;
	}
	/* Now walk up the bridges setting the VGA enable. */
	while (bus) {
		printk_debug("Setting PCI_BRIDGE_CTL_VGA for bridge %s\n",
			     dev_path(bus->dev));
		bus->bridge_ctrl |= PCI_BRIDGE_CTL_VGA;
		bus = (bus == bus->dev->bus) ? 0 : bus->dev->bus;
	}
}

#endif

/**
 * @brief  Assign the computed resources to the devices on the bus.
 *
 * @param bus Pointer to the structure for this bus
 *
 * Use the device specific set_resources method to store the computed
 * resources to hardware. For bridge devices, the set_resources() method
 * has to recurse into every down stream buses.
 *
 * Mutual recursion:
 *	assign_resources() -> device_operation::set_resources()
 *	device_operation::set_resources() -> assign_resources()
 */
void assign_resources(struct bus *bus)
{
	struct device *curdev;

	printk_spew("%s assign_resources, bus %d link: %d\n",
		    dev_path(bus->dev), bus->secondary, bus->link);

	for (curdev = bus->children; curdev; curdev = curdev->sibling) {
		if (!curdev->enabled || !curdev->resources) {
			continue;
		}
		if (!curdev->ops || !curdev->ops->set_resources) {
			printk_err("%s missing set_resources\n",
				   dev_path(curdev));
			continue;
		}
		curdev->ops->set_resources(curdev);
	}
	printk_spew("%s assign_resources, bus %d link: %d\n",
		    dev_path(bus->dev), bus->secondary, bus->link);
}

/**
 * @brief Enable the resources for a specific device
 *
 * @param dev the device whose resources are to be enabled
 *
 * Enable resources of the device by calling the device specific
 * enable_resources() method.
 *
 * The parent's resources should be enabled first to avoid having enabling
 * order problem. This is done by calling the parent's enable_resources()
 * method and let that method to call it's children's enable_resoruces()
 * method via the (global) enable_childrens_resources().
 *
 * Indirect mutual recursion:
 *	enable_resources() -> device_operations::enable_resource()
 *	device_operations::enable_resource() -> enable_children_resources()
 *	enable_children_resources() -> enable_resources()
 */
void enable_resources(struct device *dev)
{
	if (!dev->enabled) {
		return;
	}
	if (!dev->ops || !dev->ops->enable_resources) {
		printk_err("%s missing enable_resources\n", dev_path(dev));
		return;
	}
	dev->ops->enable_resources(dev);
}

/**
 * @brief Reset all of the devices a bus
 *
 * Reset all of the devices on a bus and clear the bus's reset_needed flag.
 *
 * @param bus pointer to the bus structure
 *
 * @return 1 if the bus was successfully reset, 0 otherwise.
 *
 */
int reset_bus(struct bus *bus)
{
	if (bus && bus->dev && bus->dev->ops && bus->dev->ops->reset_bus) {
		bus->dev->ops->reset_bus(bus);
		bus->reset_needed = 0;
		return 1;
	}
	return 0;
}

/**
 * @brief Scan for devices on a bus.
 *
 * If there are bridges on the bus, recursively scan the buses behind the
 * bridges. If the setting up and tuning of the bus causes a reset to be
 * required, reset the bus and scan it again.
 *
 * @param busdev Pointer to the bus device.
 * @param max Current bus number.
 * @return The maximum bus number found, after scanning all subordinate buses.
 */
unsigned int scan_bus(struct device *busdev, unsigned int max)
{
	unsigned int new_max;
	int do_scan_bus;
	if (!busdev || !busdev->enabled || !busdev->ops ||
	    !busdev->ops->scan_bus) {
		return max;
	}

	do_scan_bus = 1;
	while (do_scan_bus) {
		int link;
		new_max = busdev->ops->scan_bus(busdev, max);
		do_scan_bus = 0;
		for (link = 0; link < busdev->links; link++) {
			if (busdev->link[link].reset_needed) {
				if (reset_bus(&busdev->link[link])) {
					do_scan_bus = 1;
				} else {
					busdev->bus->reset_needed = 1;
				}
			}
		}
	}
	return new_max;
}

/**
 * @brief Determine the existence of devices and extend the device tree.
 *
 * Most of the devices in the system are listed in the mainboard Config.lb
 * file. The device structures for these devices are generated at compile
 * time by the config tool and are organized into the device tree. This
 * function determines if the devices created at compile time actually exist
 * in the physical system.
 *
 * For devices in the physical system but not listed in the Config.lb file,
 * the device structures have to be created at run time and attached to the
 * device tree.
 *
 * This function starts from the root device 'dev_root', scan the buses in
 * the system recursively, modify the device tree according to the result of
 * the probe.
 *
 * This function has no idea how to scan and probe buses and devices at all.
 * It depends on the bus/device specific scan_bus() method to do it. The
 * scan_bus() method also has to create the device structure and attach
 * it to the device tree.
 */
void dev_enumerate(void)
{
	struct device *root;
	printk_info("Enumerating buses...\n");
	root = &dev_root;

	show_all_devs(BIOS_DEBUG, "Before Device Enumeration.");
	printk_debug("Compare with tree...\n");

	show_devs_tree(root, BIOS_DEBUG, 0, 0);

	if (root->chip_ops && root->chip_ops->enable_dev) {
		root->chip_ops->enable_dev(root);
	}
	if (!root->ops || !root->ops->scan_bus) {
		printk_err("dev_root missing scan_bus operation");
		return;
	}
	scan_bus(root, 0);
	printk_info("done\n");
}

/**
 * @brief Configure devices on the devices tree.
 *
 * Starting at the root of the device tree, travel it recursively in two
 * passes. In the first pass, we compute and allocate resources (ranges)
 * requried by each device. In the second pass, the resources ranges are
 * relocated to their final position and stored to the hardware.
 *
 * I/O resources grow upward. MEM resources grow downward.
 *
 * Since the assignment is hierarchical we set the values into the dev_root
 * struct.
 */
void dev_configure(void)
{
	struct resource *res;
	struct device *root;
	struct device *child;
	int i;

#if CONFIG_CONSOLE_VGA == 1
	set_vga_bridge_bits();
#endif

	printk_info("Allocating resources...\n");

	root = &dev_root;

	/* Each domain should create resources which contain the entire address
	 * space for IO, MEM, and PREFMEM resources in the domain. The
	 * allocation of device resources will be done from this address space.
	 */

	/* Read the resources for the entire tree. */

	printk_info("Reading resources...\n");
	read_resources(&root->link[0]);
	printk_info("Done reading resources.\n");

	print_resource_tree(root, BIOS_DEBUG, "After reading.");

	/* Compute resources for all domains. */
	for (child = root->link[0].children; child; child = child->sibling) {
		if (!(child->path.type == DEVICE_PATH_PCI_DOMAIN))
			continue;
		for (i = 0; i < child->resources; i++) {
			res = &child->resource[i];
			if (res->flags & IORESOURCE_FIXED)
				continue;
			if (res->flags & IORESOURCE_PREFETCH) {
				compute_resources(&child->link[0],
					       res, MEM_MASK, PREF_TYPE);
				continue;
			}
			if (res->flags & IORESOURCE_MEM) {
				compute_resources(&child->link[0],
					       res, MEM_MASK, MEM_TYPE);
				continue;
			}
			if (res->flags & IORESOURCE_IO) {
				compute_resources(&child->link[0],
					       res, IO_MASK, IO_TYPE);
				continue;
			}
		}
	}

	/* For all domains. */
	for (child = root->link[0].children; child; child=child->sibling)
		if (child->path.type == DEVICE_PATH_PCI_DOMAIN)
			avoid_fixed_resources(child);

	/* Now we need to adjust the resources. MEM resources need to start at
	 * the highest address managable.
	 */
	for (child = root->link[0].children; child; child = child->sibling) {
		if (child->path.type != DEVICE_PATH_PCI_DOMAIN)
			continue;
		for (i = 0; i < child->resources; i++) {
			res = &child->resource[i];
			if (!(res->flags & IORESOURCE_MEM) ||
			    res->flags & IORESOURCE_FIXED)
				continue;
			res->base = resource_max(res);
		}
	}

	/* Store the computed resource allocations into device registers ... */
	printk_info("Setting resources...\n");
	for (child = root->link[0].children; child; child = child->sibling) {
		if (!(child->path.type == DEVICE_PATH_PCI_DOMAIN))
			continue;
		for (i = 0; i < child->resources; i++) {
			res = &child->resource[i];
			if (res->flags & IORESOURCE_FIXED)
				continue;
			if (res->flags & IORESOURCE_PREFETCH) {
				allocate_resources(&child->link[0],
					       res, MEM_MASK, PREF_TYPE);
				continue;
			}
			if (res->flags & IORESOURCE_MEM) {
				allocate_resources(&child->link[0],
					       res, MEM_MASK, MEM_TYPE);
				continue;
			}
			if (res->flags & IORESOURCE_IO) {
				allocate_resources(&child->link[0],
					       res, IO_MASK, IO_TYPE);
				continue;
			}
		}
	}
	assign_resources(&root->link[0]);
	printk_info("Done setting resources.\n");
	print_resource_tree(root, BIOS_DEBUG, "After assigning values.");

	printk_info("Done allocating resources.\n");
}

/**
 * @brief Enable devices on the device tree.
 *
 * Starting at the root, walk the tree and enable all devices/bridges by
 * calling the device's enable_resources() method.
 */
void dev_enable(void)
{
	printk_info("Enabling resources...\n");

	/* now enable everything. */
	enable_resources(&dev_root);

	printk_info("done.\n");
}

/**
 * @brief Initialize all devices in the global device list.
 *
 * Starting at the first device on the global device link list,
 * walk the list and call the device's init() method to do deivce
 * specific setup.
 */
void dev_initialize(void)
{
	struct device *dev;

	printk_info("Initializing devices...\n");
	for (dev = all_devices; dev; dev = dev->next) {
		if (dev->enabled && !dev->initialized &&
		    dev->ops && dev->ops->init) {
			if (dev->path.type == DEVICE_PATH_I2C) {
				printk_debug("smbus: %s[%d]->",
					     dev_path(dev->bus->dev),
					     dev->bus->link);
			}
			printk_debug("%s init\n", dev_path(dev));
			dev->initialized = 1;
			dev->ops->init(dev);
		}
	}
	printk_info("Devices initialized\n");
	show_all_devs(BIOS_DEBUG, "After init.");
}
