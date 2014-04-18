/* 2004.12 yhlu add dual core support */




#ifndef SET_NB_CFG_54
#define SET_NB_CFG_54 1
#endif

#include "cpu/amd/dualcore/dualcore_id.c"

#if 0
static inline unsigned get_core_num_in_bsp(unsigned nodeid)
{
        return ((pci_read_config32(PCI_DEV(0, 0x18+nodeid, 3), 0xe8)>>12) & 3);
}

static inline 
#if SET_NB_CFG_54 == 1
		uint8_t
#else
		void 
#endif
			set_apicid_cpuid_lo(void) {
#if SET_NB_CFG_54
	//for pre_e0, even we set nb_cfg_54, but it will still be 0
	//for e0 later you should use get_node_id(read_nb_cfg_54()) even for single core cpu
	//get siblings via cpuid(0x80000008) ecx[7:0]
	#if CONFIG_MAX_PHYSICAL_CPUS != 8
	if( get_core_num_in_bsp(0) == 0) {
                /*first node only has one core, pre_e0
                  all e0 single core installed don't need enable lo too,
                  So if mixing e0 single core and dual core, 
                  don't put single core in first socket */
        	return 0;
        }
	#endif
	
	if(read_option(CMOS_VSTART_dual_core, CMOS_VLEN_dual_core, 0) != 0)  { // disable dual_core
		return 0;
	}

		// set the NB_CFG[54]=1; why the OS will be happy with that ???
        msr_t msr; 
        msr = rdmsr(NB_CFG_MSR);
        msr.hi |= (1<<(54-32)); // InitApicIdCpuIdLo
        wrmsr(NB_CFG_MSR, msr);

	return 1;

#endif                  

}

static inline void real_start_other_core(unsigned nodeid)
{
	uint32_t dword;
	// set PCI_DEV(0, 0x18+nodeid, 3), 0x44 bit 27 to redirect all MC4 accesses and error logging to core0
	dword = pci_read_config32(PCI_DEV(0, 0x18+nodeid, 3), 0x44);
	dword |= 1<<27; // NbMcaToMstCpuEn bit
	pci_write_config32(PCI_DEV(0, 0x18+nodeid, 3), 0x44, dword);
	// set PCI_DEV(0, 0x18+nodeid, 0), 0x68 bit 5 to start core1
	dword = pci_read_config32(PCI_DEV(0, 0x18+nodeid, 0), 0x68);
	dword |= 1<<5;
	pci_write_config32(PCI_DEV(0, 0x18+nodeid, 0), 0x68, dword);
}

//it is running on core0 of every node
static inline void start_other_core(unsigned nodeid) {

        if(read_option(CMOS_VSTART_dual_core, CMOS_VLEN_dual_core, 0) != 0)  { // disable dual_core
                return;
        }

	if( get_core_num() >0) { // defined in dualcore_id.c
		real_start_other_core(nodeid);
        }
}

static inline unsigned get_nodes(void) 
{
	return ((pci_read_config32(PCI_DEV(0, 0x18, 0), 0x60)>>4) & 7) + 1;
}

//it is running on core0 of node0
static inline void start_other_cores(void) {
	unsigned nodes;
	unsigned nodeid;

        if(read_option(CMOS_VSTART_dual_core, CMOS_VLEN_dual_core, 0) != 0)  { // disable dual_core
                return;
        }

        nodes = get_nodes();

        for(nodeid=0; nodeid<nodes; nodeid++) {
		if( get_core_num_in_bsp(nodeid) > 0) {
			real_start_other_core(nodeid);
		}
	}

}
#endif

static void k8_init_and_stop_secondaries(void)
{
	struct node_core_id id;
	device_t dev;
	unsigned apicid;
	unsigned max_siblings;
	int init_detected;
	msr_t msr;
	
	/* Skip this if there was a built in self test failure */
	init_detected = early_mtrr_init_detected();
	amd_early_mtrr_init();
	enable_lapic();
	init_timer();
	if (init_detected) {
		asm volatile ("jmp __cpu_reset");
	}

	if (is_cpu_pre_e0()) {
		id.nodeid = lapicid() & 0x7;
		id.coreid = 0;
	} else {
		/* Which cpu are we on? */
		id = get_node_core_id_x();

		/* Set NB_CFG_MSR
		 * Linux expect the core to be in the least signficant bits.
		 */
		msr = rdmsr(NB_CFG_MSR);
		msr.hi |= (1<<(54-32)); // InitApicIdCpuIdLo
		wrmsr(NB_CFG_MSR, msr);
	}

	/* For now assume all cpus have the same number of siblings */
	max_siblings = (cpuid_ecx(0x80000008) & 0xff) + 1;

	/* Set the lapicid */
	lapic_write(LAPIC_ID,((id.nodeid*max_siblings) + id.coreid) << 24);

	/* Remember the cpuid */
	if (id.coreid == 0) {
		dev = PCI_DEV(0, 0x18 + id.nodeid, 2);
		pci_write_config32(dev, 0x9c, cpuid_eax(1));	
	}
	
	/* Maybe call distinguish_cpu_resets only on the last core? */
	distinguish_cpu_resets(id.nodeid);
	if (!boot_cpu()) {
		stop_this_cpu();
	}
}
