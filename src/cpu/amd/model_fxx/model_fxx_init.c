/* Needed so the AMD K8 runs correctly.  */
/* this should be done by Eric
 * 2004.11 yhlu add d0 e0 support
 * 2004.12 yhlu add dual core support
 * 2005.02 yhlu add e0 memory hole support
*/
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <device/device.h>
#include <device/device.h>
#include <device/pci.h>
#include <string.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/pae.h>
#include <pc80/mc146818rtc.h>
#include <cpu/x86/lapic.h>
#include "../../../northbridge/amd/amdk8/amdk8.h"
#include "../../../northbridge/amd/amdk8/cpu_rev.c"
#include <cpu/cpu.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/mem.h>

#if CONFIG_LOGICAL_CPUS==1
#include <cpu/amd/dualcore.h>
#endif

#include "model_fxx_msr.h"

#define MCI_STATUS 0x401

static inline msr_t rdmsr_amd(unsigned index)
{
        msr_t result;
        __asm__ __volatile__ (
                "rdmsr"
                : "=a" (result.lo), "=d" (result.hi)
                : "c" (index), "D" (0x9c5a203a)
                );
        return result;
}

static inline void wrmsr_amd(unsigned index, msr_t msr)
{
        __asm__ __volatile__ (
                "wrmsr"
                : /* No outputs */
                : "c" (index), "a" (msr.lo), "d" (msr.hi), "D" (0x9c5a203a)
                );
}



#define MTRR_COUNT 8
#define ZERO_CHUNK_KB 0x800UL /* 2M */
#define TOLM_KB 0x400000UL

struct mtrr {
	msr_t base;
	msr_t mask;
};
struct mtrr_state {
	struct mtrr mtrrs[MTRR_COUNT];
	msr_t top_mem, top_mem2;
	msr_t def_type;
};

static void save_mtrr_state(struct mtrr_state *state)
{
	int i;
	for(i = 0; i < MTRR_COUNT; i++) {
		state->mtrrs[i].base = rdmsr(MTRRphysBase_MSR(i));
		state->mtrrs[i].mask = rdmsr(MTRRphysMask_MSR(i));
	}
	state->top_mem  = rdmsr(TOP_MEM);
	state->top_mem2 = rdmsr(TOP_MEM2);
	state->def_type = rdmsr(MTRRdefType_MSR);
}

static void restore_mtrr_state(struct mtrr_state *state)
{
	int i;
	disable_cache();

	for(i = 0; i < MTRR_COUNT; i++) {
		wrmsr(MTRRphysBase_MSR(i), state->mtrrs[i].base);
		wrmsr(MTRRphysMask_MSR(i), state->mtrrs[i].mask);
	}
	wrmsr(TOP_MEM,         state->top_mem);
	wrmsr(TOP_MEM2,        state->top_mem2);
	wrmsr(MTRRdefType_MSR, state->def_type);

	enable_cache();
}


#if 0
static void print_mtrr_state(struct mtrr_state *state)
{
	int i;
	for(i = 0; i < MTRR_COUNT; i++) {
		printk_debug("var mtrr %d: %08x%08x mask: %08x%08x\n",
			i,
			state->mtrrs[i].base.hi, state->mtrrs[i].base.lo,
			state->mtrrs[i].mask.hi, state->mtrrs[i].mask.lo);
	}
	printk_debug("top_mem:  %08x%08x\n",
		state->top_mem.hi, state->top_mem.lo);
	printk_debug("top_mem2: %08x%08x\n",
		state->top_mem2.hi, state->top_mem2.lo);
	printk_debug("def_type: %08x%08x\n",
		state->def_type.hi, state->def_type.lo);
}
#endif

static void set_init_ecc_mtrrs(void)
{
	msr_t msr;
	int i;
	disable_cache();

	/* First clear all of the msrs to be safe */
	for(i = 0; i < MTRR_COUNT; i++) {
		msr_t zero;
		zero.lo = zero.hi = 0;
		wrmsr(MTRRphysBase_MSR(i), zero);
		wrmsr(MTRRphysMask_MSR(i), zero);
	}

	/* Write back cache the first 1MB */
	msr.hi = 0x00000000;
	msr.lo = 0x00000000 | MTRR_TYPE_WRBACK;
	wrmsr(MTRRphysBase_MSR(0), msr);
	msr.hi = 0x000000ff;
	msr.lo = ~((CONFIG_LB_MEM_TOPK << 10) - 1) | 0x800;
	wrmsr(MTRRphysMask_MSR(0), msr);

	/* Set the default type to write combining */
	msr.hi = 0x00000000;
	msr.lo = 0xc00 | MTRR_TYPE_WRCOMB;
	wrmsr(MTRRdefType_MSR, msr);

	/* Set TOP_MEM to 4G */
	msr.hi = 0x00000001;
	msr.lo = 0x00000000;
	wrmsr(TOP_MEM, msr);

	enable_cache();
}

static void init_ecc_memory(unsigned node_id)
{
	unsigned long startk, begink, endk;
#if K8_E0_MEM_HOLE_SIZEK != 0
	unsigned long hole_startk = 0, hole_endk = 0;
#endif
	unsigned long basek;
	struct mtrr_state mtrr_state;
	device_t f1_dev, f2_dev, f3_dev;
	int enable_scrubbing;
	uint32_t dcl;

	f1_dev = dev_find_slot(0, PCI_DEVFN(0x18 + node_id, 1));
	if (!f1_dev) {
		die("Cannot find cpu function 1\n");
	}
	f2_dev = dev_find_slot(0, PCI_DEVFN(0x18 + node_id, 2));
	if (!f2_dev) {
		die("Cannot find cpu function 2\n");
	}
	f3_dev = dev_find_slot(0, PCI_DEVFN(0x18 + node_id, 3));
	if (!f3_dev) {
		die("Cannot find cpu function 3\n");
	}

	/* See if we scrubbing should be enabled */
	enable_scrubbing = 1;
	get_option(&enable_scrubbing, "hw_scrubber");

	/* Enable cache scrubbing at the lowest possible rate */
	if (enable_scrubbing) {
		pci_write_config32(f3_dev, SCRUB_CONTROL,
			(SCRUB_84ms << 16) | (SCRUB_84ms << 8) | (SCRUB_NONE << 0));
	} else {
		pci_write_config32(f3_dev, SCRUB_CONTROL,
			(SCRUB_NONE << 16) | (SCRUB_NONE << 8) | (SCRUB_NONE << 0));
		printk_debug("Scrubbing Disabled\n");
	}
	

	/* If ecc support is not enabled don't touch memory */
	dcl = pci_read_config32(f2_dev, DRAM_CONFIG_LOW);
	if (!(dcl & DCL_DimmEccEn)) {
		return;
	}

	startk = (pci_read_config32(f1_dev, 0x40 + (node_id*8)) & 0xffff0000) >> 2;
	endk   = ((pci_read_config32(f1_dev, 0x44 + (node_id*8)) & 0xffff0000) >> 2) + 0x4000;

#if K8_E0_MEM_HOLE_SIZEK != 0
        if (!is_cpu_pre_e0()) {
                uint32_t val;
                val = pci_read_config32(f1_dev, 0xf0);
                if((val & 1)==1) {
        	        hole_startk = ((val & (0xff<<24)) >> 10);
			hole_endk = ((val & (0xff<<8))<<(16-10)) - startk;
			hole_endk += hole_startk;
                }
        }
#endif
	

	/* Don't start too early */
	begink = startk;
	if (begink < CONFIG_LB_MEM_TOPK) {
		begink = CONFIG_LB_MEM_TOPK;
	}
	printk_debug("Clearing memory %uK - %uK: ", startk, endk);

	/* Save the normal state */
	save_mtrr_state(&mtrr_state);

	/* Switch to the init ecc state */
	set_init_ecc_mtrrs();
	disable_lapic();

	/* Walk through 2M chunks and zero them */
	for(basek = begink; basek < endk; basek = ((basek + ZERO_CHUNK_KB) & ~(ZERO_CHUNK_KB - 1))) {
		unsigned long limitk;
		unsigned long size;
		void *addr;

#if K8_E0_MEM_HOLE_SIZEK != 0
		if ((basek >= hole_startk) && (basek < hole_endk)) continue;
#endif
		/* Report every 64M */
		if ((basek % (64*1024)) == 0) {
			/* Restore the normal state */
			map_2M_page(0);
			restore_mtrr_state(&mtrr_state);
			enable_lapic();

			/* Print a status message */
			printk_debug("%c", (basek >= TOLM_KB)?'+':'-');

			/* Return to the initialization state */
			set_init_ecc_mtrrs();
			disable_lapic();
		}

		limitk = (basek + ZERO_CHUNK_KB) & ~(ZERO_CHUNK_KB - 1);
		if (limitk > endk) {
			limitk = endk;
		}
		size = (limitk - basek) << 10;
		addr = map_2M_page(basek >> 11);
		addr = (void *)(((uint32_t)addr) | ((basek & 0x7ff) << 10));
		if (addr == MAPPING_ERROR) {
			continue;
		}

		/* clear memory 2M (limitk - basek) */
		clear_memory(addr, size);
	}
	/* Restore the normal state */
	map_2M_page(0);
	restore_mtrr_state(&mtrr_state);
	enable_lapic();

	/* Set the scrub base address registers */
	pci_write_config32(f3_dev, SCRUB_ADDR_LOW,  startk << 10);
	pci_write_config32(f3_dev, SCRUB_ADDR_HIGH, startk >> 22);

	/* Enable the scrubber? */
	if (enable_scrubbing) {
		/* Enable scrubbing at the lowest possible rate */
		pci_write_config32(f3_dev, SCRUB_CONTROL,
			(SCRUB_84ms << 16) | (SCRUB_84ms << 8) | (SCRUB_84ms << 0));
	}

	printk_debug(" done\n");
}

static inline void k8_errata(void)
{
	msr_t msr;
	if (is_cpu_pre_c0()) {
		/* Erratum 63... */
		msr = rdmsr(HWCR_MSR);
		msr.lo |= (1 << 6);
		wrmsr(HWCR_MSR, msr);

		/* Erratum 69... */
		msr = rdmsr_amd(BU_CFG_MSR);
		msr.hi |= (1 << (45 - 32));
		wrmsr_amd(BU_CFG_MSR, msr);

		/* Erratum 81... */
		msr = rdmsr_amd(DC_CFG_MSR);
		msr.lo |=  (1 << 10);
		wrmsr_amd(DC_CFG_MSR, msr);
			
	}
	/* I can't touch this msr on early buggy cpus */
	if (!is_cpu_pre_b3()) {

		/* Erratum 89 ... */
		msr = rdmsr(NB_CFG_MSR);
		msr.lo |= 1 << 3;
		
		if (!is_cpu_pre_c0() && is_cpu_pre_d0()) {
			/* D0 later don't need it */
			/* Erratum 86 Disable data masking on C0 and 
			 * later processor revs.
			 * FIXME this is only needed if ECC is enabled.
			 */
			msr.hi |= 1 << (36 - 32);
		}	
		wrmsr(NB_CFG_MSR, msr);
	}
// AMD_D0_SUPPORT	
	if (!is_cpu_pre_c0() && is_cpu_pre_d0()) {
		/* D0 later don't need it */
		/* Erratum 97 ... */
		msr = rdmsr_amd(DC_CFG_MSR);
		msr.lo |= 1 << 3;
		wrmsr_amd(DC_CFG_MSR, msr);
	}
		
//AMD_D0_SUPPORT
	if(is_cpu_pre_d0()) {
		/*D0 later don't need it */
		/* Erratum 94 ... */
		msr = rdmsr_amd(IC_CFG_MSR);
		msr.lo |= 1 << 11;
		wrmsr_amd(IC_CFG_MSR, msr);
	}

	/* Erratum 91 prefetch miss is handled in the kernel */

//AMD_D0_SUPPORT
	if(is_cpu_d0()) {
		/* Erratum 110 ...*/
		msr = rdmsr_amd(CPU_ID_HYPER_EXT_FEATURES);
		msr.hi |=1;
		wrmsr_amd(CPU_ID_HYPER_EXT_FEATURES, msr);
	}

//AMD_E0_SUPPORT
	if(!is_cpu_pre_e0()) {
                /* Erratum 110 ...*/
                msr = rdmsr_amd(CPU_ID_EXT_FEATURES_MSR);
                msr.hi |=1;
                wrmsr_amd(CPU_ID_EXT_FEATURES_MSR, msr);
	}
}

void model_fxx_init(device_t dev)
{
	unsigned long i;
	msr_t msr;
#if CONFIG_LOGICAL_CPUS==1
	struct node_core_id id;
        unsigned siblings;
	id.coreid=0;
#else
	unsigned nodeid;
#endif

	/* Turn on caching if we haven't already */
	x86_enable_cache(); 
	amd_setup_mtrrs();
	x86_mtrr_check();

	disable_cache();
	
	/* zero the machine check error status registers */
	msr.lo = 0;
	msr.hi = 0;
	for(i=0; i<5; i++) {
		wrmsr(MCI_STATUS + (i*4),msr);
	}

	k8_errata();
	
	enable_cache();

#if CONFIG_LOGICAL_CPUS==1
//AMD_DUAL_CORE_SUPPORT
        siblings = cpuid_ecx(0x80000008) & 0xff;

//	id = get_node_core_id((!is_cpu_pre_e0())? read_nb_cfg_54():0);
	id = get_node_core_id(read_nb_cfg_54()); // pre e0 nb_cfg_54 can not be set

       	if(siblings>0) {
                msr = rdmsr_amd(CPU_ID_FEATURES_MSR);
                msr.lo |= 1 << 28; 
                wrmsr_amd(CPU_ID_FEATURES_MSR, msr);

       	        msr = rdmsr_amd(LOGICAL_CPUS_NUM_MSR);
                msr.lo = (siblings+1)<<16; 
       	        wrmsr_amd(LOGICAL_CPUS_NUM_MSR, msr);

                msr = rdmsr_amd(CPU_ID_EXT_FEATURES_MSR);
       	        msr.hi |= 1<<(33-32); 
               	wrmsr_amd(CPU_ID_EXT_FEATURES_MSR, msr);
	} 
        
	/* Is this a bad location?  In particular can another node prefecth
	 * data from this node before we have initialized it?
	 */
	if(id.coreid == 0) init_ecc_memory(id.nodeid); // only do it for core0
#else
	/* For now there is a 1-1 mapping between node_id and cpu_id */
	nodeid = lapicid() & 0x7;
	init_ecc_memory(nodeid);
#endif
	
	/* Enable the local cpu apics */
	setup_lapic();

#if CONFIG_LOGICAL_CPUS==1
//AMD_DUAL_CORE_SUPPORT
        /* Start up my cpu siblings */
//	if(id.coreid==0)  amd_sibling_init(dev); // Don't need core1 is already be put in the CPU BUS in bus_cpu_scan
#endif
}

static struct device_operations cpu_dev_ops = {
	.init = model_fxx_init,
};
static struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_AMD, 0xf50 }, /* B3 */
	{ X86_VENDOR_AMD, 0xf51 }, /* SH7-B3 */
	{ X86_VENDOR_AMD, 0xf58 }, /* SH7-C0 */
	{ X86_VENDOR_AMD, 0xf48 },
#if 1
	{ X86_VENDOR_AMD, 0xf5A }, /* SH7-CG */
	{ X86_VENDOR_AMD, 0xf4A },
	{ X86_VENDOR_AMD, 0xf7A },
	{ X86_VENDOR_AMD, 0xfc0 }, /* DH7-CG */
	{ X86_VENDOR_AMD, 0xfe0 },
	{ X86_VENDOR_AMD, 0xff0 },
	{ X86_VENDOR_AMD, 0xf82 }, /* CH7-CG */
	{ X86_VENDOR_AMD, 0xfb2 },
//AMD_D0_SUPPORT
	{ X86_VENDOR_AMD, 0x10f50 }, /* SH7-D0 */
	{ X86_VENDOR_AMD, 0x10f40 },
	{ X86_VENDOR_AMD, 0x10f70 },
        { X86_VENDOR_AMD, 0x10fc0 }, /* DH7-D0 */
        { X86_VENDOR_AMD, 0x10ff0 },
        { X86_VENDOR_AMD, 0x10f80 }, /* CH7-D0 */
        { X86_VENDOR_AMD, 0x10fb0 },
//AMD_E0_SUPPORT
        { X86_VENDOR_AMD, 0x20f50 }, /* SH7-E0*/
        { X86_VENDOR_AMD, 0x20f40 },
        { X86_VENDOR_AMD, 0x20f70 },
        { X86_VENDOR_AMD, 0x20fc0 }, /* DH7-E0 */ /* DH-E3 */
        { X86_VENDOR_AMD, 0x20ff0 },
        { X86_VENDOR_AMD, 0x20f10 }, /* JH7-E0 */
        { X86_VENDOR_AMD, 0x20f30 },
        { X86_VENDOR_AMD, 0x20f51 }, /* SH-E4 */
        { X86_VENDOR_AMD, 0x20f71 },
        { X86_VENDOR_AMD, 0x20f42 }, /* SH-E5 */
        { X86_VENDOR_AMD, 0x20ff2 }, /* DH-E6 */
        { X86_VENDOR_AMD, 0x20fc2 },
        { X86_VENDOR_AMD, 0x20f12 }, /* JH-E6 */
        { X86_VENDOR_AMD, 0x20f32 },
#endif

	{ 0, 0 },
};
static struct cpu_driver model_fxx __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
