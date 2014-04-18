/* This should be done by Eric
	2004.12 yhlu add dual core support
	2005.01 yhlu add support move apic before pci_domain in MB Config.lb
	2005.02 yhlu add e0 memory hole support
*/

#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/hypertransport.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include <cpu/cpu.h>

#include <cpu/x86/lapic.h>

#if CONFIG_LOGICAL_CPUS==1
#include <cpu/amd/dualcore.h>
#include <pc80/mc146818rtc.h>
#endif

#include "chip.h"
#include "root_complex/chip.h"
#include "northbridge.h"
#include "amdk8.h"

#if K8_E0_MEM_HOLE_SIZEK != 0
#include "./cpu_rev.c"
#endif

#define FX_DEVS 8
static device_t __f0_dev[FX_DEVS];
static device_t __f1_dev[FX_DEVS];

#if 0
static void debug_fx_devs(void)
{
	int i;
	for (i = 0; i < FX_DEVS; i++) {
		device_t dev;
		dev = __f0_dev[i];
		if (dev) {
			printk_debug("__f0_dev[%d]: %s bus: %p\n",
				i, dev_path(dev), dev->bus);
		}
		dev = __f1_dev[i];
		if (dev) {
			printk_debug("__f1_dev[%d]: %s bus: %p\n",
				i, dev_path(dev), dev->bus);
		}
	}
}
#endif

static void get_fx_devs(void)
{
	int i;
	if (__f1_dev[0]) {
		return;
	}
	for (i = 0; i < FX_DEVS; i++) {
		__f0_dev[i] = dev_find_slot(0, PCI_DEVFN(0x18 + i, 0));
		__f1_dev[i] = dev_find_slot(0, PCI_DEVFN(0x18 + i, 1));
	}
	if (!__f1_dev[0]) {
		die("Cannot find 0:0x18.1\n");
	}
}

static uint32_t f1_read_config32(unsigned reg)
{
	get_fx_devs();
	return pci_read_config32(__f1_dev[0], reg);
}

static void f1_write_config32(unsigned reg, uint32_t value)
{
	int i;
	get_fx_devs();
	for (i = 0; i < FX_DEVS; i++) {
		device_t dev;
		dev = __f1_dev[i];
		if (dev && dev->enabled) {
			pci_write_config32(dev, reg, value);
		}
	}
}

static unsigned int amdk8_nodeid(device_t dev)
{
	return (dev->path.u.pci.devfn >> 3) - 0x18;
}

static unsigned int amdk8_scan_chains(device_t dev, unsigned int max)
{
	unsigned nodeid;
	unsigned link;
	nodeid = amdk8_nodeid(dev);
#if 0
	printk_debug("%s amdk8_scan_chains max: %d starting...\n", 
		     dev_path(dev), max);
#endif
	for (link = 0; link < dev->links; link++) {
		uint32_t link_type;
		uint32_t busses, config_busses;
		unsigned free_reg, config_reg;
		dev->link[link].cap = 0x80 + (link *0x20);
		do {
			link_type = pci_read_config32(dev, dev->link[link].cap + 0x18);
		} while(link_type & ConnectionPending);
		if (!(link_type & LinkConnected)) {
			continue;
		}
		do {
			link_type = pci_read_config32(dev, dev->link[link].cap + 0x18);
		} while(!(link_type & InitComplete));
		if (!(link_type & NonCoherent)) {
			continue;
		}
		/* See if there is an available configuration space mapping
		 * register in function 1. */
		free_reg = 0;
		for (config_reg = 0xe0; config_reg <= 0xec; config_reg += 4) {
			uint32_t config;
			config = f1_read_config32(config_reg);
			if (!free_reg && ((config & 3) == 0)) {
				free_reg = config_reg;
				continue;
			}
			if (((config & 3) == 3) && 
			    (((config >> 4) & 7) == nodeid) &&
			    (((config >> 8) & 3) == link)) {
				break;
			}
		}
		if (free_reg && (config_reg > 0xec)) {
			config_reg = free_reg;
		}
		/* If we can't find an available configuration space mapping
		 * register skip this bus */
		if (config_reg > 0xec) {
			continue;
		}

		/* Set up the primary, secondary and subordinate bus numbers.
		 * We have no idea how many busses are behind this bridge yet,
		 * so we set the subordinate bus number to 0xff for the moment.
		 */
		dev->link[link].secondary = ++max;
		dev->link[link].subordinate = 0xff;

		/* Read the existing primary/secondary/subordinate bus
		 * number configuration.
		 */
		busses = pci_read_config32(dev, dev->link[link].cap + 0x14);
		config_busses = f1_read_config32(config_reg);

		/* Configure the bus numbers for this bridge: the configuration
		 * transactions will not be propagates by the bridge if it is
		 * not correctly configured
		 */
		busses &= 0xff000000;
		busses |= (((unsigned int)(dev->bus->secondary) << 0) |
			   ((unsigned int)(dev->link[link].secondary) << 8) |
			   ((unsigned int)(dev->link[link].subordinate) << 16));
		pci_write_config32(dev, dev->link[link].cap + 0x14, busses);

		config_busses &= 0x000fc88;
		config_busses |= 
			(3 << 0) |  /* rw enable, no device compare */
			(( nodeid & 7) << 4) | 
			(( link & 3 ) << 8) |  
			((dev->link[link].secondary) << 16) |
			((dev->link[link].subordinate) << 24);
		f1_write_config32(config_reg, config_busses);

#if 0
		printk_debug("%s Hyper transport scan link: %d max: %d\n", 
			dev_path(dev), link, max);
#endif
		/* Now we can scan all of the subordinate busses i.e. the
		 * chain on the hypertranport link */
		max = hypertransport_scan_chain(&dev->link[link], max);

#if 0
		printk_debug("%s Hyper transport scan link: %d new max: %d\n",
			dev_path(dev), link, max);
#endif

		/* We know the number of busses behind this bridge.  Set the
		 * subordinate bus number to it's real value
		 */
		dev->link[link].subordinate = max;
		busses = (busses & 0xff00ffff) |
			((unsigned int) (dev->link[link].subordinate) << 16);
		pci_write_config32(dev, dev->link[link].cap + 0x14, busses);

		config_busses = (config_busses & 0x00ffffff) |
			(dev->link[link].subordinate << 24);
		f1_write_config32(config_reg, config_busses);
#if 0
		printk_debug("%s Hypertransport scan link: %d done\n",
			dev_path(dev), link);
#endif
	}
#if 0
	printk_debug("%s amdk8_scan_chains max: %d done\n", 
		dev_path(dev), max);
#endif
	return max;
}

static int reg_useable(unsigned reg, device_t goal_dev, unsigned goal_nodeid,
		       unsigned goal_link)
{
	struct resource *res;
	unsigned nodeid, link;
	int result;
	res = 0;
	for (nodeid = 0; !res && (nodeid < 8); nodeid++) {
		device_t dev;
		dev = __f0_dev[nodeid];
		for (link = 0; !res && (link < 3); link++) {
			res = probe_resource(dev, 0x100 + (reg | link));
		}
	}
	result = 2;
	if (res) {
		result = 0;
		if ((goal_link == (link - 1)) && 
		    (goal_nodeid == (nodeid - 1)) &&
		    (res->flags <= 1)) {
			result = 1;
		}
	}
#if 0
	printk_debug("reg: %02x result: %d gnodeid: %u glink: %u nodeid: %u link: %u\n",
		     reg, result, goal_nodeid, goal_link, nodeid, link);
#endif
	return result;
}

static struct resource *amdk8_find_iopair(device_t dev, unsigned nodeid, unsigned link)
{
	struct resource *resource;
	unsigned free_reg, reg;
	resource = 0;
	free_reg = 0;
	for (reg = 0xc0; reg <= 0xd8; reg += 0x8) {
		int result;
		result = reg_useable(reg, dev, nodeid, link);
		if (result == 1) {
			/* I have been allocated this one */
			break;
		}
		else if (result > 1) {
			/* I have a free register pair */
			free_reg = reg;
		}
	}
	if (reg > 0xd8) {
		reg = free_reg;
	}
	if (reg > 0) {
		resource = new_resource(dev, 0x100 + (reg | link));
	}
	return resource;
}

static struct resource *amdk8_find_mempair(device_t dev, unsigned nodeid, unsigned link)
{
	struct resource *resource;
	unsigned free_reg, reg;
	resource = 0;
	free_reg = 0;
	for (reg = 0x80; reg <= 0xb8; reg += 0x8) {
		int result;
		result = reg_useable(reg, dev, nodeid, link);
		if (result == 1) {
			/* I have been allocated this one */
			break;
		}
		else if (result > 1) {
			/* I have a free register pair */
			free_reg = reg;
		}
	}
	if (reg > 0xb8) {
		reg = free_reg;
	}
	if (reg > 0) {
		resource = new_resource(dev, 0x100 + (reg | link));
	}
	return resource;
}

static void amdk8_link_read_bases(device_t dev, unsigned nodeid, unsigned link)
{
	struct resource *resource;
	
	/* Initialize the io space constraints on the current bus */
	resource =  amdk8_find_iopair(dev, nodeid, link);
	if (resource) {
		resource->base  = 0;
		resource->size  = 0;
		resource->align = log2(HT_IO_HOST_ALIGN);
		resource->gran  = log2(HT_IO_HOST_ALIGN);
		resource->limit = 0xffffUL;
		resource->flags = IORESOURCE_IO;
		compute_allocate_resource(&dev->link[link], resource, 
			IORESOURCE_IO, IORESOURCE_IO);
	}

	/* Initialize the prefetchable memory constraints on the current bus */
	resource = amdk8_find_mempair(dev, nodeid, link);
	if (resource) {
		resource->base  = 0;
		resource->size  = 0;
		resource->align = log2(HT_MEM_HOST_ALIGN);
		resource->gran  = log2(HT_MEM_HOST_ALIGN);
		resource->limit = 0xffffffffffULL;
		resource->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;
		compute_allocate_resource(&dev->link[link], resource, 
			IORESOURCE_MEM | IORESOURCE_PREFETCH, 
			IORESOURCE_MEM | IORESOURCE_PREFETCH);
	}

	/* Initialize the memory constraints on the current bus */
	resource = amdk8_find_mempair(dev, nodeid, link);
	if (resource) {
		resource->base  = 0;
		resource->size  = 0;
		resource->align = log2(HT_MEM_HOST_ALIGN);
		resource->gran  = log2(HT_MEM_HOST_ALIGN);
		resource->limit = 0xffffffffffULL;
		resource->flags = IORESOURCE_MEM;
		compute_allocate_resource(&dev->link[link], resource, 
			IORESOURCE_MEM | IORESOURCE_PREFETCH, 
			IORESOURCE_MEM);
	}
}

static void amdk8_read_resources(device_t dev)
{
	unsigned nodeid, link;
	nodeid = amdk8_nodeid(dev);
	for (link = 0; link < dev->links; link++) {
		if (dev->link[link].children) {
			amdk8_link_read_bases(dev, nodeid, link);
		}
	}
}

static void amdk8_set_resource(device_t dev, struct resource *resource, unsigned nodeid)
{
	resource_t rbase, rend;
	unsigned reg, link;
	char buf[50];

	/* Make certain the resource has actually been set */
	if (!(resource->flags & IORESOURCE_ASSIGNED)) {
		return;
	}

	/* If I have already stored this resource don't worry about it */
	if (resource->flags & IORESOURCE_STORED) {
		return;
	}
	
	/* Only handle PCI memory and IO resources */
	if (!(resource->flags & (IORESOURCE_MEM | IORESOURCE_IO)))
		return;

	/* Ensure I am actually looking at a resource of function 1 */
	if (resource->index < 0x100) {
		return;
	}
	/* Get the base address */
	rbase = resource->base;
	
	/* Get the limit (rounded up) */
	rend  = resource_end(resource);

	/* Get the register and link */
	reg  = resource->index & 0xfc;
	link = resource->index & 3;

	if (resource->flags & IORESOURCE_IO) {
		uint32_t base, limit;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_IO, IORESOURCE_IO);
		base  = f1_read_config32(reg);
		limit = f1_read_config32(reg + 0x4);
		base  &= 0xfe000fcc;
		base  |= rbase  & 0x01fff000;
		base  |= 3;
		limit &= 0xfe000fc8;
		limit |= rend & 0x01fff000;
		limit |= (link & 3) << 4;
		limit |= (nodeid & 7);

		if (dev->link[link].bridge_ctrl & PCI_BRIDGE_CTL_VGA) {
                        printk_spew("%s, enabling legacy VGA IO forwarding for %s link %s\n",
                                    __func__, dev_path(dev), link);		
			base |= PCI_IO_BASE_VGA_EN;
		}
		if (dev->link[link].bridge_ctrl & PCI_BRIDGE_CTL_NO_ISA) {
			base |= PCI_IO_BASE_NO_ISA;
		}
		
		f1_write_config32(reg + 0x4, limit);
		f1_write_config32(reg, base);
	}
	else if (resource->flags & IORESOURCE_MEM) {
		uint32_t base, limit;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH,
			resource->flags & (IORESOURCE_MEM | IORESOURCE_PREFETCH));
		base  = f1_read_config32(reg);
		limit = f1_read_config32(reg + 0x4);
		base  &= 0x000000f0;
		base  |= (rbase >> 8) & 0xffffff00;
		base  |= 3;
		limit &= 0x00000048;
		limit |= (rend >> 8) & 0xffffff00;
		limit |= (link & 3) << 4;
		limit |= (nodeid & 7);
		f1_write_config32(reg + 0x4, limit);
		f1_write_config32(reg, base);
	}
	resource->flags |= IORESOURCE_STORED;
	sprintf(buf, " <node %d link %d>",
		nodeid, link);
	report_resource_stored(dev, resource, buf);
}

/**
 *
 * I tried to reuse the resource allocation code in amdk8_set_resource()
 * but it is too diffcult to deal with the resource allocation magic.
 */
static void amdk8_create_vga_resource(device_t dev, unsigned nodeid)
{
	struct resource *resource;
	unsigned link;
	uint32_t base, limit;
	unsigned reg;

	/* find out which link the VGA card is connected,
	 * we only deal with the 'first' vga card */
	for (link = 0; link < dev->links; link++) {
		if (dev->link[link].bridge_ctrl & PCI_BRIDGE_CTL_VGA) {
			break;
		}
	}
	
	printk_spew("%s: link %d has VGA device\n", __func__, link);

	/* no VGA card installed */
	if (link == dev->links)
		return;

	/* allocate a temp resrouce for legacy VGA buffer */
	resource = amdk8_find_mempair(dev, nodeid, link);
	resource->base = 0xa0000;
	resource->size = 0x20000;

	/* write the resource to the hardware */
	reg  = resource->index & 0xfc;
	base  = f1_read_config32(reg);
	limit = f1_read_config32(reg + 0x4);
	base  &= 0x000000f0;
	base  |= (resource->base >> 8) & 0xffffff00;
	base  |= 3;
	limit &= 0x00000048;
	limit |= ((resource->base + resource->size) >> 8) & 0xffffff00;
	limit |= (resource->index & 3) << 4;
	limit |= (nodeid & 7);
	f1_write_config32(reg + 0x4, limit);
	f1_write_config32(reg, base);

	/* release the temp resource */
	resource->flags = 0;

}

static void amdk8_set_resources(device_t dev)
{
	unsigned nodeid, link;
	int i;

	/* Find the nodeid */
	nodeid = amdk8_nodeid(dev);

	amdk8_create_vga_resource(dev, nodeid);
	
	/* Set each resource we have found */
	for (i = 0; i < dev->resources; i++) {
		amdk8_set_resource(dev, &dev->resource[i], nodeid);
	}

	for (link = 0; link < dev->links; link++) {
		struct bus *bus;
		bus = &dev->link[link];
		if (bus->children) {
			assign_resources(bus);
		}
	}
}

static void amdk8_enable_resources(device_t dev)
{
	pci_dev_enable_resources(dev);
	enable_childrens_resources(dev);
}

static void mcf0_control_init(struct device *dev)
{
	uint32_t cmd;

#if 0	
	printk_debug("NB: Function 0 Misc Control.. ");
#endif
#if 1
	/* improve latency and bandwith on HT */
	cmd = pci_read_config32(dev, 0x68);
	cmd &= 0xffff80ff;
	cmd |= 0x00004800;
	pci_write_config32(dev, 0x68, cmd );
#endif

#if 0	
	/* over drive the ht port to 1000 Mhz */
	cmd = pci_read_config32(dev, 0xa8);
	cmd &= 0xfffff0ff;
	cmd |= 0x00000600;
	pci_write_config32(dev, 0xdc, cmd );
#endif	
#if 0
	printk_debug("done.\n");
#endif
}

static struct device_operations northbridge_operations = {
	.read_resources   = amdk8_read_resources,
	.set_resources    = amdk8_set_resources,
	.enable_resources = amdk8_enable_resources,
	.init             = mcf0_control_init,
	.scan_bus         = amdk8_scan_chains,
	.enable           = 0,
	.ops_pci          = 0,
};


static struct pci_driver mcf0_driver __pci_driver = {
	.ops    = &northbridge_operations,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1100,
};

#if CONFIG_CHIP_NAME == 1

struct chip_operations northbridge_amd_amdk8_ops = {
	CHIP_NAME("AMD K8 Northbridge")
	.enable_dev = 0,
};

#endif

static void pci_domain_read_resources(device_t dev)
{
	struct resource *resource;
	unsigned reg;

	/* Find the already assigned resource pairs */
	get_fx_devs();
	for (reg = 0x80; reg <= 0xd8; reg+= 0x08) {
		uint32_t base, limit;
		base  = f1_read_config32(reg);
		limit = f1_read_config32(reg + 0x04);
		/* Is this register allocated? */
		if ((base & 3) != 0) {
			unsigned nodeid, link;
			device_t dev;
			nodeid = limit & 7;
			link   = (limit >> 4) & 3;
			dev = __f0_dev[nodeid];
			if (dev) {
				/* Reserve the resource  */
				struct resource *resource;
				resource = new_resource(dev, 0x100 + (reg | link));
				if (resource) {
					resource->flags = 1;
				}
			}
		}
	}

	/* Initialize the system wide io space constraints */
	resource = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	resource->base  = 0x400;
	resource->limit = 0xffffUL;
	resource->flags = IORESOURCE_IO | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;

	/* Initialize the system wide memory resources constraints */
	resource = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
	resource->limit = 0xfcffffffffULL;
	resource->flags = IORESOURCE_MEM | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;
}

static void ram_resource(device_t dev, unsigned long index,
			 unsigned long basek, unsigned long sizek)
{
	struct resource *resource;

	if (!sizek) {
		return;
	}
	resource = new_resource(dev, index);
	resource->base  = ((resource_t)basek) << 10;
	resource->size  = ((resource_t)sizek) << 10;
	resource->flags =  IORESOURCE_MEM | IORESOURCE_CACHEABLE | \
		IORESOURCE_FIXED | IORESOURCE_STORED | IORESOURCE_ASSIGNED;
}

static void tolm_test(void *gp, struct device *dev, struct resource *new)
{
	struct resource **best_p = gp;
	struct resource *best;
	best = *best_p;
	if (!best || (best->base > new->base)) {
		best = new;
	}
	*best_p = best;
}

static uint32_t find_pci_tolm(struct bus *bus)
{
	struct resource *min;
	uint32_t tolm;
	min = 0;
	search_bus_resources(bus, IORESOURCE_MEM, IORESOURCE_MEM, tolm_test, &min);
	tolm = 0xffffffffUL;
	if (min && tolm > min->base) {
		tolm = min->base;
	}
	return tolm;
}

static void pci_domain_set_resources(device_t dev)
{
	unsigned long mmio_basek;
	uint32_t pci_tolm;
	int i, idx;

	pci_tolm = find_pci_tolm(&dev->link[0]);

#warning "FIXME handle interleaved nodes"
	mmio_basek = pci_tolm >> 10;
	/* Round mmio_basek to something the processor can support */
	mmio_basek &= ~((1 << 6) -1);

#if 1
#warning "FIXME improve mtrr.c so we don't use up all of the mtrrs with a 64M MMIO hole"
	/* Round the mmio hold to 64M */
	mmio_basek &= ~((64*1024) - 1);
#endif

#if K8_E0_MEM_HOLE_SIZEK != 0
	if (!is_cpu_pre_e0())
        for (i = 0; i < 8; i++) {
                uint32_t base;
                base  = f1_read_config32(0x40 + (i << 3));
                if ((base & ((1<<1)|(1<<0))) != ((1<<1)|(1<<0))) {
                        continue;
                }
		
		base = pci_read_config32(__f1_dev[i], 0xf0);
		if((base & 1)==0) continue;
		base &= 0xff<<24;
		base >>= 10;
	        if (mmio_basek > base) {
        	        mmio_basek = base;
        	}
		break; // only one hole	
	}
#endif

	idx = 10;
	for (i = 0; i < 8; i++) {
		uint32_t base, limit;
		unsigned basek, limitk, sizek;
		base  = f1_read_config32(0x40 + (i << 3));
		limit = f1_read_config32(0x44 + (i << 3));
		if ((base & ((1<<1)|(1<<0))) != ((1<<1)|(1<<0))) {
			continue;
		}
		basek = (base & 0xffff0000) >> 2;
		limitk = ((limit + 0x00010000) & 0xffff0000) >> 2;
		sizek = limitk - basek;

		/* see if we need a hole from 0xa0000 to 0xbffff */
		if ((basek < ((8*64)+(8*16))) && (sizek > ((8*64)+(16*16)))) {
			ram_resource(dev, idx++, basek, ((8*64)+(8*16)) - basek);
			basek = (8*64)+(16*16);
			sizek = limitk - ((8*64)+(16*16));
			
		}

		
		/* See if I need to split the region to accomodate pci memory space */
		if ((basek < mmio_basek) && (limitk > mmio_basek)) {
			if (basek < mmio_basek) {
				unsigned pre_sizek;
				pre_sizek = mmio_basek - basek;
				ram_resource(dev, idx++, basek, pre_sizek);
				sizek -= pre_sizek;
				basek = mmio_basek;
			}
			if ((basek + sizek) <= 4*1024*1024) {
				sizek = 0;
			}
			else {
				basek = 4*1024*1024;
				sizek -= (4*1024*1024 - mmio_basek);
			}
		}
		ram_resource(dev, idx++, basek, sizek);
	}
	assign_resources(&dev->link[0]);
}

static unsigned int pci_domain_scan_bus(device_t dev, unsigned int max)
{
	unsigned reg;
	/* Unmap all of the HT chains */
	for (reg = 0xe0; reg <= 0xec; reg += 4) {
		f1_write_config32(reg, 0);
	}
	max = pci_scan_bus(&dev->link[0], PCI_DEVFN(0x18, 0), 0xff, max);
	return max;
}

static struct device_operations pci_domain_ops = {
	.read_resources   = pci_domain_read_resources,
	.set_resources    = pci_domain_set_resources,
	.enable_resources = enable_childrens_resources,
	.init             = 0,
	.scan_bus         = pci_domain_scan_bus,
	.ops_pci_bus      = &pci_cf8_conf1,
};

#define APIC_ID_OFFSET 0x10
static unsigned int cpu_bus_scan(device_t dev, unsigned int max)
{
	struct bus *cpu_bus;
	device_t dev_mc;
	int i,j;
	unsigned nb_cfg_54 = 0;
	unsigned siblings = 0;
	int enable_apic_ext_id = 0;
	int bsp_apic_id = lapicid(); // bsp apicid
	int apic_id_offset = bsp_apic_id;

#if CONFIG_LOGICAL_CPUS==1
	int e0_later_single_core;
	int disable_siblings = !CONFIG_LOGICAL_CPUS;
	get_option(&disable_siblings, "dual_core");

	// for pre_e0, nb_cfg_54 can not be set, ( even set, when you read it still be 0)
	// How can I get the nb_cfg_54 of every node' nb_cfg_54 in bsp??? and differ d0 and e0 single core
 
	nb_cfg_54 = read_nb_cfg_54();
#endif

	dev_mc = dev_find_slot(0, PCI_DEVFN(0x18, 0));
	if(pci_read_config32(dev_mc, 0x68) & ( HTTC_APIC_EXT_ID | HTTC_APIC_EXT_BRD_CST)) {
		enable_apic_ext_id = 1;
		if(apic_id_offset==0) { //bsp apic id is not changed
			apic_id_offset = APIC_ID_OFFSET;
		}
	}


	/* Find which cpus are present */
	cpu_bus = &dev->link[0];
	for (i = 0; i < 8; i++) {
		device_t dev, cpu;
		struct device_path cpu_path;

		/* Find the cpu's memory controller */
		dev = dev_find_slot(0, PCI_DEVFN(0x18 + i, 3));
		if(!dev) { // in case we move apic cluser before pci_domain and not set that for second CPU
			for(j=0; j<4; j++) {
	                        struct device dummy;
				uint32_t id;
	                        dummy.bus              = dev_mc->bus;
	                        dummy.path.type        = DEVICE_PATH_PCI;
	                        dummy.path.u.pci.devfn = PCI_DEVFN(0x18 + i, j);
	                        id = pci_read_config32(&dummy, PCI_VENDOR_ID);
	                        if (id != 0xffffffff && id != 0x00000000 &&
        	                        id != 0x0000ffff && id != 0xffff0000) {
                	        	//create that for it
	                	        dev = alloc_dev(dev_mc->bus, &dummy.path);
				}
			}
		}

#if CONFIG_LOGICAL_CPUS==1
		e0_later_single_core = 0;
		if((!disable_siblings) && dev && dev->enabled) {
			j = (pci_read_config32(dev, 0xe8) >> 12) & 3;  //dev is func 3

			printk_debug("  %s siblings=%d\r\n", dev_path(dev), j);

			if(nb_cfg_54) {
				// For e0 single core if nb_cfg_54 is set, apicid will be 0, 2, 4.... 
				//  ----> you can mixed single core e0 and dual core e0 at any sequence
				// That is the typical case

		                if(j == 0 ){
                 		       e0_later_single_core = is_e0_later_in_bsp(i);  // single core 
		                } else {
		                       e0_later_single_core = 0;
               			}
				if(e0_later_single_core) { 
					printk_debug("\tFound e0 single core\r\n");
					j=1; 
				}
	
				if(siblings > j ) {
					//actually we can't be here, because d0 nb_cfg_54 can not be set
					//even worse is_e0_later_in_bsp() can not find out if it is d0 or e0

					die("When NB_CFG_54 is set, if you want to mix e0 (single core and dual core) and single core(pre e0) CPUs, you need to put all the single core (pre e0) CPUs before all the (e0 single or dual core) CPUs\r\n");
				}
				else {
					siblings = j;
				}
			} else {
				siblings = j;
			}
		}
#endif

#if CONFIG_LOGICAL_CPUS==1
                for (j = 0; j <= (e0_later_single_core?0:siblings); j++ ) {
#else 
		for (j = 0; j <= siblings; j++ ) {
#endif
                        /* Build the cpu device path */
                        cpu_path.type = DEVICE_PATH_APIC;
                        cpu_path.u.apic.apic_id = i * (nb_cfg_54?(siblings+1):1) + j * (nb_cfg_54?1:8);
  
                        /* See if I can find the cpu */
                        cpu = find_dev_path(cpu_bus, &cpu_path);
  
                        /* Enable the cpu if I have the processor */
                        if (dev && dev->enabled) {
                                if (!cpu) {
                                        cpu = alloc_dev(cpu_bus, &cpu_path);
                                }
                                if (cpu) {
                                        cpu->enabled = 1; 
                                }
                        }

                        /* Disable the cpu if I don't have the processor */
                        if (cpu && (!dev || !dev->enabled)) {
                                cpu->enabled = 0;
                        }

                        /* Report what I have done */
                        if (cpu) {
				if(enable_apic_ext_id) {
			                if(cpu->path.u.apic.apic_id<apic_id_offset) { //all add offset except bsp core0
						if( (cpu->path.u.apic.apic_id > siblings) || (bsp_apic_id!=0) )
	        		                        cpu->path.u.apic.apic_id += apic_id_offset;
                			}
				}
                                printk_debug("CPU: %s %s\n",
                                        dev_path(cpu), cpu->enabled?"enabled":"disabled");
                        }
                } //j
	}

	return max;
}

static void cpu_bus_init(device_t dev)
{
	initialize_cpus(&dev->link[0]);
}

static void cpu_bus_noop(device_t dev) 
{
}

static struct device_operations cpu_bus_ops = {
	.read_resources   = cpu_bus_noop,
	.set_resources    = cpu_bus_noop,
	.enable_resources = cpu_bus_noop,
	.init             = cpu_bus_init,
	.scan_bus         = cpu_bus_scan,
};

static void root_complex_enable_dev(struct device *dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_PCI_DOMAIN) {
		dev->ops = &pci_domain_ops;
	}
	else if (dev->path.type == DEVICE_PATH_APIC_CLUSTER) {
		dev->ops = &cpu_bus_ops;
	}
}

struct chip_operations northbridge_amd_amdk8_root_complex_ops = {
	CHIP_NAME("AMD K8 Root Complex")
	.enable_dev = root_complex_enable_dev,
};
