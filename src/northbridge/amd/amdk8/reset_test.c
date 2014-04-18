#include <stdint.h>
#include <cpu/x86/lapic.h>
#define NODE_ID		0x60
#define	HT_INIT_CONTROL 0x6c

#define HTIC_ColdR_Detect  (1<<4)
#define HTIC_BIOSR_Detect  (1<<5)
#define HTIC_INIT_Detect   (1<<6)

static int cpu_init_detected(unsigned nodeid)
{
	unsigned long htic;
	device_t dev;

	dev = PCI_DEV(0, 0x18 + nodeid, 0);
	htic = pci_read_config32(dev, HT_INIT_CONTROL);

	return !!(htic & HTIC_INIT_Detect);
}

static int bios_reset_detected(void)
{
	unsigned long htic;
	htic = pci_read_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL);

	return (htic & HTIC_ColdR_Detect) && !(htic & HTIC_BIOSR_Detect);
}

static int cold_reset_detected(void)
{
	unsigned long htic;
	htic = pci_read_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL);

	return !(htic & HTIC_ColdR_Detect);
}

static void distinguish_cpu_resets(unsigned nodeid)
{
	uint32_t htic;
	device_t device;
	device = PCI_DEV(0, 0x18 + nodeid, 0);
	htic = pci_read_config32(device, HT_INIT_CONTROL);
	htic |= HTIC_ColdR_Detect | HTIC_BIOSR_Detect | HTIC_INIT_Detect;
	pci_write_config32(device, HT_INIT_CONTROL, htic);
}

static void set_bios_reset(void)
{
	unsigned long htic;
	htic = pci_read_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL);
	htic &= ~HTIC_BIOSR_Detect;
	pci_write_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL, htic);
}

static unsigned node_link_to_bus(unsigned node, unsigned link)
{
	unsigned reg;

	for(reg = 0xE0; reg < 0xF0; reg += 0x04) {
		unsigned config_map;
		config_map = pci_read_config32(PCI_DEV(0, 0x18, 1), reg);
		if ((config_map & 3) != 3) {
			continue;
		}
		if ((((config_map >> 4) & 7) == node) &&
			(((config_map >> 8) & 3) == link))
		{
			return (config_map >> 16) & 0xff;
		}
	}
	return 0;
}

static unsigned get_sblk(void)
{
	uint32_t reg;
	/* read PCI_DEV(0,0x18,0) 0x64 bit [8:9] to find out SbLink m */
	reg = pci_read_config32(PCI_DEV(0, 0x18, 0), 0x64);
	return ((reg>>8) & 3) ;
}

static unsigned get_sbbusn(unsigned sblk)
{
	return node_link_to_bus(0, sblk);
}


