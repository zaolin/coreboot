#include <console/console.h>
#include <device/device.h>
#include <device/path.h>
#include <device/pci.h>
#include <string.h>

/**
 * @brief See if a device structure exists for path
 *
 * @param bus The bus to find the device on
 * @param path The relative path from the bus to the appropriate device
 * @return pointer to a device structure for the device on bus at path
 *         or 0/NULL if no device is found
 */
device_t find_dev_path(struct bus *parent, struct device_path *path)
{
	device_t child;
	for(child = parent->children; child; child = child->sibling) {
		if (path_eq(path, &child->path)) {
			break;
		}
	}
	return child;
}

/**
 * @brief See if a device structure already exists and if not allocate it
 *
 * @param bus The bus to find the device on
 * @param path The relative path from the bus to the appropriate device
 * @return pointer to a device structure for the device on bus at path
 */
device_t alloc_find_dev(struct bus *parent, struct device_path *path)
{
	device_t child;
	child = find_dev_path(parent, path);
	if (!child) {
		child = alloc_dev(parent, path);
	}
	return child;
}

/**
 * Given a bus and a devfn number, find the device structure
 * @param bus The bus number
 * @param devfn a device/function number
 * @return pointer to the device structure
 */
struct device *dev_find_slot(unsigned int bus, unsigned int devfn)
{
	struct device *dev, *result;

	result = 0;
	for (dev = all_devices; dev; dev = dev->next) {
		if ((dev->path.type == DEVICE_PATH_PCI) &&
			(dev->bus->secondary == bus) && 
			(dev->path.u.pci.devfn == devfn)) {
			result = dev;
			break;
		}
	}
	return result;
}

/** Find a device of a given vendor and type
 * @param vendor Vendor ID (e.g. 0x8086 for Intel)
 * @param device Device ID
 * @param from Pointer to the device structure, used as a starting point
 *        in the linked list of all_devices, which can be 0 to start at the 
 *        head of the list (i.e. all_devices)
 * @return Pointer to the device struct 
 */
struct device *dev_find_device(unsigned int vendor, unsigned int device, struct device *from)
{
	if (!from)
		from = all_devices;
	else
		from = from->next;
	while (from && (from->vendor != vendor || from->device != device)) {
		from = from->next;
	}
	return from;
}

/** Find a device of a given class
 * @param class Class of the device
 * @param from Pointer to the device structure, used as a starting point
 *        in the linked list of all_devices, which can be 0 to start at the 
 *        head of the list (i.e. all_devices)
 * @return Pointer to the device struct 
 */
struct device *dev_find_class(unsigned int class, struct device *from)
{
	if (!from)
		from = all_devices;
	else
		from = from->next;
	while (from && (from->class & 0xffffff00) != class)
		from = from->next;
	return from;
}


const char *dev_path(device_t dev)
{
	static char buffer[DEVICE_PATH_MAX];
	buffer[0] = '\0';
	if (!dev) {
		memcpy(buffer, "<null>", 7);
	}
	else {
		switch(dev->path.type) {
		case DEVICE_PATH_ROOT:
			memcpy(buffer, "Root Device", 12);
			break;
		case DEVICE_PATH_DEFAULT_CPU:
			memcpy(buffer, "Default CPU", 12);
			break;
		case DEVICE_PATH_PCI:
			sprintf(buffer, "PCI: %02x:%02x.%01x",
				dev->bus->secondary, 
				PCI_SLOT(dev->path.u.pci.devfn), PCI_FUNC(dev->path.u.pci.devfn));
			break;
		case DEVICE_PATH_PNP:
			sprintf(buffer, "PNP: %04x.%01x",
				dev->path.u.pnp.port, dev->path.u.pnp.device);
			break;
		case DEVICE_PATH_I2C:
			sprintf(buffer, "I2C: %02x",
				dev->path.u.i2c.device);
			break;
		case DEVICE_PATH_APIC:
			sprintf(buffer, "APIC: %02x",
				dev->path.u.apic.apic_id);
			break;
		case DEVICE_PATH_PCI_DOMAIN:
			sprintf(buffer, "PCI_DOMAIN: %04x",
				dev->path.u.pci_domain.domain);
			break;
		case DEVICE_PATH_APIC_CLUSTER:
			sprintf(buffer, "APIC_CLUSTER: %01x",
				dev->path.u.apic_cluster.cluster);
			break;
		default:
			printk_err("Unknown device path type: %d\n", dev->path.type);
			break;
		}
	}
	return buffer;
}

int path_eq(struct device_path *path1, struct device_path *path2)
{
	int equal = 0;
	if (path1->type == path2->type) {
		switch(path1->type) {
		case DEVICE_PATH_NONE:
			break;
		case DEVICE_PATH_ROOT:
			equal = 1;
			break;
		case DEVICE_PATH_DEFAULT_CPU:
			equal = 1;
			break;
		case DEVICE_PATH_PCI:
			equal = (path1->u.pci.devfn == path2->u.pci.devfn);
			break;
		case DEVICE_PATH_PNP:
			equal = (path1->u.pnp.port == path2->u.pnp.port) &&
				(path1->u.pnp.device == path2->u.pnp.device);
			break;
		case DEVICE_PATH_I2C:
			equal = (path1->u.i2c.device == path2->u.i2c.device);
			break;
		case DEVICE_PATH_APIC:
			equal = (path1->u.apic.apic_id == path2->u.apic.apic_id);
			break;
		case DEVICE_PATH_PCI_DOMAIN:
			equal = (path1->u.pci_domain.domain == path2->u.pci_domain.domain);
			break;
		case DEVICE_PATH_APIC_CLUSTER:
			equal = (path1->u.apic_cluster.cluster == path2->u.apic_cluster.cluster);
			break;
		default:
			printk_err("Uknown device type: %d\n", path1->type);
			break;
		}
	}
	return equal;
}

/**
 * See if we have unused but allocated resource structures.
 * If so remove the allocation.
 * @param dev The device to find the resource on
 */
void compact_resources(device_t dev)
{
	struct resource *resource;
	int i;
	/* Move all of the free resources to the end */
	for(i = 0; i < dev->resources;) {
		resource = &dev->resource[i];
		if (!resource->flags) {
			memmove(resource, resource + 1, dev->resources - i);
			dev->resources -= 1;
			memset(&dev->resource[dev->resources], 0, sizeof(*resource));
		} else {
			i++;
		}
	}
}


/**
 * See if a resource structure already exists for a given index
 * @param dev The device to find the resource on
 * @param index  The index of the resource on the device.
 * @return the resource if it already exists
 */
struct resource *probe_resource(device_t dev, unsigned index)
{
	struct resource *resource;
	int i;
	/* See if there is a resource with the appropriate index */
	resource = 0;
	for(i = 0; i < dev->resources; i++) {
		if (dev->resource[i].index == index) {
			resource = &dev->resource[i];
			break;
		}
	}
	return resource;
}

/**
 * See if a resource structure already exists for a given index and if
 * not allocate one.  Then initialize the initialize the resource
 * to default values.
 * @param dev The device to find the resource on
 * @param index  The index of the resource on the device.
 */
struct resource *new_resource(device_t dev, unsigned index)
{
	struct resource *resource;

	/* First move all of the free resources to the end */
	compact_resources(dev);

	/* See if there is a resource with the appropriate index */
	resource = probe_resource(dev, index);
	if (!resource) {
		if (dev->resources == MAX_RESOURCES) {
			die("MAX_RESOURCES exceeded.");
		}
		resource = &dev->resource[dev->resources];
		memset(resource, 0, sizeof(*resource));
		dev->resources++;
	}
	/* Initialize the resource values */
	if (!(resource->flags & IORESOURCE_FIXED)) {
		resource->flags = 0;
		resource->base = 0;
	}
	resource->size  = 0;
	resource->limit = 0;
	resource->index = index;
	resource->align = 0;
	resource->gran  = 0;

	return resource;
}

/**
 * Return an existing resource structure for a given index.
 * @param dev The device to find the resource on
 * @param index  The index of the resource on the device.
 */
struct resource *find_resource(device_t dev, unsigned index)
{
	struct resource *resource;

	/* See if there is a resource with the appropriate index */
	resource = probe_resource(dev, index);
	if (!resource) {
		printk_emerg("%s missing resource: %02x\n",
			dev_path(dev), index);
		die("");
	}
	return resource;
}


/**
 * @brief round a number up to the next multiple of gran
 * @param val the starting value
 * @param gran granularity we are aligning the number to.
 * @returns aligned value
 */
static resource_t align_up(resource_t val, unsigned long gran)
{
	resource_t mask;
	mask = (1ULL << gran) - 1ULL;
	val += mask;
	val &= ~mask;
	return val;
}

/**
 * @brief round a number up to the previous multiple of gran
 * @param val the starting value
 * @param gran granularity we are aligning the number to.
 * @returns aligned value
 */
static resource_t align_down(resource_t val, unsigned long gran)
{
	resource_t mask;
	mask = (1ULL << gran) - 1ULL;
	val &= ~mask;
	return val;
}

/**
 * @brief Compute the maximum address that is part of a resource
 * @param resource the resource whose limit is desired
 * @returns the end
 */
resource_t resource_end(struct resource *resource)
{
	resource_t base, end;
	/* get the base address */
	base = resource->base;

	/* For a non bridge resource granularity and alignment are the same.
	 * For a bridge resource align is the largest needed alignment below
	 * the bridge.  While the granularity is simply how many low bits of the
	 * address cannot be set.
	 */
	
	/* Get the end (rounded up) */
	end = base + align_up(resource->size, resource->gran) - 1;

	return end;
}

/**
 * @brief Compute the maximum legal value for resource->base
 * @param resource the resource whose maximum is desired
 * @returns the maximum
 */
resource_t resource_max(struct resource *resource)
{
	resource_t max;

	max = align_down(resource->limit - resource->size + 1, resource->align);

	return max;
}

/**
 * @brief print the resource that was just stored.
 * @param dev the device the stored resorce lives on
 * @param resource the resource that was just stored.
 */
void report_resource_stored(device_t dev, struct resource *resource, const char *comment)
{
	if (resource->flags & IORESOURCE_STORED) {
		unsigned char buf[10];
		unsigned long long base, end;
		base = resource->base;
		end = resource_end(resource);
		buf[0] = '\0';
		if (resource->flags & IORESOURCE_PCI_BRIDGE) {
			sprintf(buf, "bus %d ", dev->link[0].secondary);
		}
		printk_debug(
			"%s %02x <- [0x%010Lx - 0x%010Lx] %s%s%s%s\n",
			dev_path(dev),
			resource->index,
			base, end,
			buf,
			(resource->flags & IORESOURCE_PREFETCH) ? "pref" : "",
			(resource->flags & IORESOURCE_IO)? "io":
			(resource->flags & IORESOURCE_DRQ)? "drq":
			(resource->flags & IORESOURCE_IRQ)? "irq":
			(resource->flags & IORESOURCE_MEM)? "mem": 
			"????",
			comment);
	}
}
