#include <console/console.h>
#include <device/device.h>
#include <device/path.h>
#include <device/pci_ids.h>
#include <cpu/cpu.h>
#include <arch/smp/mpspec.h>
#include <string.h>
#include <arch/cpu.h>
#include <cpu/x86/lapic.h>

unsigned char smp_compute_checksum(void *v, int len)
{
	unsigned char *bytes;
	unsigned char checksum;
	int i;
	bytes = v;
	checksum = 0;
	for(i = 0; i < len; i++) {
		checksum -= bytes[i];
	}
	return checksum;
}

void *smp_write_floating_table(unsigned long addr)
{
	struct intel_mp_floating *mf;
	void *v;
	
	/* 16 byte align the table address */
	addr = (addr + 0xf) & (~0xf);
	v = (void *)addr;

	mf = v;
	mf->mpf_signature[0] = '_';
	mf->mpf_signature[1] = 'M';
	mf->mpf_signature[2] = 'P';
	mf->mpf_signature[3] = '_';
	mf->mpf_physptr = (unsigned long)(((char *)v) + SMP_FLOATING_TABLE_LEN);
	mf->mpf_length = 1;
	mf->mpf_specification = 4;
	mf->mpf_checksum = 0;
	mf->mpf_feature1 = 0;
	mf->mpf_feature2 = 0;
	mf->mpf_feature3 = 0;
	mf->mpf_feature4 = 0;
	mf->mpf_feature5 = 0;
	mf->mpf_checksum = smp_compute_checksum(mf, mf->mpf_length*16);
	return v;
}

void *smp_write_floating_table_physaddr(unsigned long addr, unsigned long mpf_physptr)
{
        struct intel_mp_floating *mf;
        void *v;
	
	v = (void *)addr;
        mf = v;
        mf->mpf_signature[0] = '_';
        mf->mpf_signature[1] = 'M';
        mf->mpf_signature[2] = 'P';
        mf->mpf_signature[3] = '_';
        mf->mpf_physptr = mpf_physptr;
        mf->mpf_length = 1;
        mf->mpf_specification = 4;
        mf->mpf_checksum = 0;
        mf->mpf_feature1 = 0;
        mf->mpf_feature2 = 0;
        mf->mpf_feature3 = 0;
        mf->mpf_feature4 = 0;
        mf->mpf_feature5 = 0;
        mf->mpf_checksum = smp_compute_checksum(mf, mf->mpf_length*16);
        return v;
}

void *smp_next_mpc_entry(struct mp_config_table *mc)
{
	void *v;
	v = (void *)(((char *)mc) + mc->mpc_length);
	return v;
}
static void smp_add_mpc_entry(struct mp_config_table *mc, unsigned length)
{
	mc->mpc_length += length;
	mc->mpc_entry_count++;
}

void *smp_next_mpe_entry(struct mp_config_table *mc)
{
	void *v;
	v = (void *)(((char *)mc) + mc->mpc_length + mc->mpe_length);
	return v;
}
static void smp_add_mpe_entry(struct mp_config_table *mc, mpe_t mpe)
{
	mc->mpe_length += mpe->mpe_length;
}

void smp_write_processor(struct mp_config_table *mc,
	unsigned char apicid, unsigned char apicver,
	unsigned char cpuflag, unsigned int cpufeature,
	unsigned int featureflag)
{
	struct mpc_config_processor *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_PROCESSOR;
	mpc->mpc_apicid = apicid;
	mpc->mpc_apicver = apicver;
	mpc->mpc_cpuflag = cpuflag;
	mpc->mpc_cpufeature = cpufeature;
	mpc->mpc_featureflag = featureflag;
	smp_add_mpc_entry(mc, sizeof(*mpc));
}

/* If we assume a symmetric processor configuration we can
 * get all of the information we need to write the processor
 * entry from the bootstrap processor.
 * Plus I don't think linux really even cares.
 * Having the proper apicid's in the table so the non-bootstrap
 *  processors can be woken up should be enough.
 */
void smp_write_processors(struct mp_config_table *mc)
{
	int boot_apic_id;
	unsigned apic_version;
	unsigned cpu_features;
	unsigned cpu_feature_flags;
	struct cpuid_result result;
	device_t cpu;
	
	boot_apic_id = lapicid();
	apic_version = lapic_read(LAPIC_LVR) & 0xff;
	result = cpuid(1);
	cpu_features = result.eax;
	cpu_feature_flags = result.edx;
	for(cpu = all_devices; cpu; cpu = cpu->next) {
		unsigned long cpu_flag;
		if ((cpu->path.type != DEVICE_PATH_APIC) || 
			(cpu->bus->dev->path.type != DEVICE_PATH_APIC_CLUSTER))
		{
			continue;
		}
		if (!cpu->enabled) {
			continue;
		}
		cpu_flag = MPC_CPU_ENABLED;
		if (boot_apic_id == cpu->path.u.apic.apic_id) {
			cpu_flag = MPC_CPU_ENABLED | MPC_CPU_BOOTPROCESSOR;
		}
		smp_write_processor(mc, 
			cpu->path.u.apic.apic_id, apic_version,
			cpu_flag, cpu_features, cpu_feature_flags
		);
	}
}

void smp_write_bus(struct mp_config_table *mc,
	unsigned char id, char *bustype)
{
	struct mpc_config_bus *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_BUS;
	mpc->mpc_busid = id;
	memcpy(mpc->mpc_bustype, bustype, sizeof(mpc->mpc_bustype));
	smp_add_mpc_entry(mc, sizeof(*mpc));
}

void smp_write_ioapic(struct mp_config_table *mc,
	unsigned char id, unsigned char ver, 
	unsigned long apicaddr)
{
	struct mpc_config_ioapic *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_IOAPIC;
	mpc->mpc_apicid = id;
	mpc->mpc_apicver = ver;
	mpc->mpc_flags = MPC_APIC_USABLE;
	mpc->mpc_apicaddr = apicaddr;
	smp_add_mpc_entry(mc, sizeof(*mpc));
}

void smp_write_intsrc(struct mp_config_table *mc,
	unsigned char irqtype, unsigned short irqflag,
	unsigned char srcbus, unsigned char srcbusirq,
	unsigned char dstapic, unsigned char dstirq)
{
	struct mpc_config_intsrc *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_INTSRC;
	mpc->mpc_irqtype = irqtype;
	mpc->mpc_irqflag = irqflag;
	mpc->mpc_srcbus = srcbus;
	mpc->mpc_srcbusirq = srcbusirq;
	mpc->mpc_dstapic = dstapic;
	mpc->mpc_dstirq = dstirq;
	smp_add_mpc_entry(mc, sizeof(*mpc));
#if CONFIG_DEBUG_MPTABLE == 1
	printk_info("add intsrc srcbus 0x%x srcbusirq 0x%x, dstapic 0x%x, dstirq 0x%x\n",
				srcbus, srcbusirq, dstapic, dstirq);
	hexdump(__FUNCTION__, mpc, sizeof(*mpc));
#endif
}

void smp_write_intsrc_pci_bridge(struct mp_config_table *mc,
	unsigned char irqtype, unsigned short irqflag,
	struct device *dev,
	unsigned char dstapic, unsigned char *dstirq)
{
	struct device *child;

	int linkn;
	int i;
	int srcbus;
	int slot;

	struct bus *link;
	unsigned char dstirq_x[4];

	for (linkn = 0; linkn < dev->links; linkn++) {

		link = &dev->link[linkn];
		child = link->children;
		srcbus = link->secondary;

		while (child) {
			if (child->path.type != DEVICE_PATH_PCI)
				goto next;

			slot = (child->path.u.pci.devfn >> 3);
			/* round pins */
			for (i = 0; i < 4; i++)
				dstirq_x[i] = dstirq[(i + slot) % 4];

			if ((child->class >> 16) != PCI_BASE_CLASS_BRIDGE) {
				/* pci device */
				printk_debug("route irq: %s %04x\n", dev_path(child));
				for (i = 0; i < 4; i++)
					smp_write_intsrc(mc, irqtype, irqflag, srcbus, (slot<<2)|i, dstapic, dstirq_x[i]);
				goto next;
			}

			switch (child->class>>8) {
			case PCI_CLASS_BRIDGE_PCI:
			case PCI_CLASS_BRIDGE_PCMCIA:
			case PCI_CLASS_BRIDGE_CARDBUS:
				printk_debug("route irq bridge: %s %04x\n", dev_path(child));
				smp_write_intsrc_pci_bridge(mc, irqtype, irqflag, child, dstapic, dstirq_x);
			}

		next:
			child = child->sibling;
		}

	}
}

void smp_write_lintsrc(struct mp_config_table *mc,
	unsigned char irqtype, unsigned short irqflag,
	unsigned char srcbusid, unsigned char srcbusirq,
	unsigned char destapic, unsigned char destapiclint)
{
	struct mpc_config_lintsrc *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_LINTSRC;
	mpc->mpc_irqtype = irqtype;
	mpc->mpc_irqflag = irqflag;
	mpc->mpc_srcbusid = srcbusid;
	mpc->mpc_srcbusirq = srcbusirq;
	mpc->mpc_destapic = destapic;
	mpc->mpc_destapiclint = destapiclint;
	smp_add_mpc_entry(mc, sizeof(*mpc));
}

void smp_write_address_space(struct mp_config_table *mc,
	unsigned char busid, unsigned char address_type,
	unsigned int address_base_low, unsigned int address_base_high,
	unsigned int address_length_low, unsigned int address_length_high)
{
	struct mp_exten_system_address_space *mpe;
	mpe = smp_next_mpe_entry(mc);
	memset(mpe, '\0', sizeof(*mpe));
	mpe->mpe_type = MPE_SYSTEM_ADDRESS_SPACE;
	mpe->mpe_length = sizeof(*mpe);
	mpe->mpe_busid = busid;
	mpe->mpe_address_type = address_type;
	mpe->mpe_address_base_low  = address_base_low;
	mpe->mpe_address_base_high = address_base_high;
	mpe->mpe_address_length_low  = address_length_low;
	mpe->mpe_address_length_high = address_length_high;
	smp_add_mpe_entry(mc, (mpe_t)mpe);
}


void smp_write_bus_hierarchy(struct mp_config_table *mc,
	unsigned char busid, unsigned char bus_info,
	unsigned char parent_busid)
{
	struct mp_exten_bus_hierarchy *mpe;
	mpe = smp_next_mpe_entry(mc);
	memset(mpe, '\0', sizeof(*mpe));
	mpe->mpe_type = MPE_BUS_HIERARCHY;
	mpe->mpe_length = sizeof(*mpe);
	mpe->mpe_busid = busid;
	mpe->mpe_bus_info = bus_info;
	mpe->mpe_parent_busid = parent_busid;
	smp_add_mpe_entry(mc, (mpe_t)mpe);
}

void smp_write_compatibility_address_space(struct mp_config_table *mc,
	unsigned char busid, unsigned char address_modifier,
	unsigned int range_list)
{
	struct mp_exten_compatibility_address_space *mpe;
	mpe = smp_next_mpe_entry(mc);
	memset(mpe, '\0', sizeof(*mpe));
	mpe->mpe_type = MPE_COMPATIBILITY_ADDRESS_SPACE;
	mpe->mpe_length = sizeof(*mpe);
	mpe->mpe_busid = busid;
	mpe->mpe_address_modifier = address_modifier;
	mpe->mpe_range_list = range_list;
	smp_add_mpe_entry(mc, (mpe_t)mpe);
}

