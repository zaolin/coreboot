#define ASSEMBLY 1
 
#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include <arch/cpu.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"
#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "southbridge/amd/amd8111/amd8111_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"
#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "northbridge/amd/amdk8/cpu_rev.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"
#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

static void hard_reset(void)
{
        set_bios_reset();

        /* enable cf9 */
        pci_write_config8(PCI_DEV(0, 0x04, 3), 0x41, 0xf1);
        /* reset */
        outb(0x0e, 0x0cf9);
}

static void soft_reset(void)
{
        set_bios_reset();
        pci_write_config8(PCI_DEV(0, 0x04, 0), 0x47, 1);
}

#define AMD8111_RESET PCI_DEV(     \
                HARD_RESET_BUS,    \
                HARD_RESET_DEVICE, \
                HARD_RESET_FUNCTION)

static void soft2_reset(void)
{  
        set_bios_reset();
        pci_write_config8(AMD8111_RESET, 0x47, 1);
}

static void memreset_setup(void)
{
   if (is_cpu_pre_c0()) {
        outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(0<<0), SMBUS_IO_BASE + 0xc0 + 16);  //REVC_MEMRST_EN=0
   }
   else {
        outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(1<<0), SMBUS_IO_BASE + 0xc0 + 16);  //REVC_MEMRST_EN=1
   }
        outb((0 << 7)|(0 << 6)|(0<<5)|(0<<4)|(1<<2)|(0<<0), SMBUS_IO_BASE + 0xc0 + 17);
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
   if (is_cpu_pre_c0()) {
        udelay(800);
        outb((0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0), SMBUS_IO_BASE + 0xc0 + 17); //REVB_MEMRST_L=1
        udelay(90);
   }
}

static unsigned int generate_row(uint8_t node, uint8_t row, uint8_t maxnodes)
{
	/* Routing Table Node i 
	 *
	 * F0: 0x40, 0x44, 0x48, 0x4c, 0x50, 0x54, 0x58, 0x5c 
	 *  i:    0,    1,    2,    3,    4,    5,    6,    7
	 *
	 * [ 0: 3] Request Route
	 *     [0] Route to this node
	 *     [1] Route to Link 0
	 *     [2] Route to Link 1
	 *     [3] Route to Link 2
	 * [11: 8] Response Route
	 *     [0] Route to this node
	 *     [1] Route to Link 0
	 *     [2] Route to Link 1
	 *     [3] Route to Link 2
	 * [19:16] Broadcast route
	 *     [0] Route to this node
	 *     [1] Route to Link 0
	 *     [2] Route to Link 1
	 *     [3] Route to Link 2
	 */

	uint32_t ret=0x00010101; /* default row entry */
/*
            (L1)       (L1)  (L2)   
        CPU1-------------CPU0--------8131------8111
                          |(L0)
                          |
                          |
                          |
                          |
                          |
                        8151 
*/
	/* Link1 of CPU0 to Link1 of CPU1 */
	static const unsigned int rows_2p[2][2] = {
		{ 0x00050101, 0x00010404 },
		{ 0x00010404, 0x00050101 }
	};

	if(maxnodes>2) {
		print_debug("this mainboard is only designed for 2 cpus\r\n");
		maxnodes=2;
	}


	if (!(node>=maxnodes || row>=maxnodes)) {
		ret=rows_2p[node][row];
	}

	return ret;
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

//#include "northbridge/amd/amdk8/setup_resource_map.c"
#include "northbridge/amd/amdk8/raminit.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "sdram/generic_sdram.c"

/* tyan does not want the default */
#include "resourcemap.c" 

#define FIRST_CPU  1
#define SECOND_CPU 1
#define TOTAL_CPUS (FIRST_CPU + SECOND_CPU)
static void main(unsigned long bist)
{
        static const struct mem_controller cpu[] = {
#if FIRST_CPU
                {
                        .node_id = 0,
                        .f0 = PCI_DEV(0, 0x18, 0),
                        .f1 = PCI_DEV(0, 0x18, 1),
                        .f2 = PCI_DEV(0, 0x18, 2),
                        .f3 = PCI_DEV(0, 0x18, 3),
                        .channel0 = { (0xa<<3)|0, (0xa<<3)|2, 0, 0 },
                        .channel1 = { (0xa<<3)|1, (0xa<<3)|3, 0, 0 },
                },
#endif
#if SECOND_CPU
                {
                        .node_id = 1,
                        .f0 = PCI_DEV(0, 0x19, 0),
                        .f1 = PCI_DEV(0, 0x19, 1),
                        .f2 = PCI_DEV(0, 0x19, 2),
                        .f3 = PCI_DEV(0, 0x19, 3),
                        .channel0 = { (0xa<<3)|4, (0xa<<3)|6, 0, 0 },
                        .channel1 = { (0xa<<3)|5, (0xa<<3)|7, 0, 0 },
                },
#endif
        };

        int needs_reset;
	if (bist == 0) {
		/* Skip this if there was a built in self test failure */
		amd_early_mtrr_init();
	        enable_lapic();
        	init_timer();
	
	        if (cpu_init_detected()) {
#if 1
        	        asm volatile ("jmp __cpu_reset");
#else           
                /* cpu reset also reset the memtroller ????
                        need soft_reset to reset all except keep HT link freq and width */
		/* So we don't need to 
			1. jmp to __cpu_reset
			2. jmp to __main to copy ROM to ram (It costs some time) 
			3. call hardwaremain(), it will according to boot_complete to issue hard_reset in southbridge. 
				(Actually it is soft2_reset(); --- without call hard_reset, the memory is corrupted.
		  We will call soft2_reset directly to spare time in 1 and 2 and 3.2
		*/
                	distinguish_cpu_resets();
                	soft2_reset();
#endif  
        	}
        	distinguish_cpu_resets();
        	if (!boot_cpu()) {
                	stop_this_cpu();
        	}
	}
	
        w83627hf_enable_serial(SERIAL_DEV, TTYS0_BASE);
        uart_init();
        console_init();

	/* Halt if there was a built in self test failure */
//	report_bist_failure(bist);
	
        setup_s2885_resource_map();
        needs_reset = setup_coherent_ht_domain();
#if 0
	needs_reset |= ht_setup_chains(2);
#else
        needs_reset |= ht_setup_chains_x();
#endif
        if (needs_reset) {
                print_info("ht reset -\r\n");
                soft_reset();
        }

	enable_smbus();
#if 0
	dump_spd_registers(&cpu[0]);
#endif
	memreset_setup();
	sdram_initialize(sizeof(cpu)/sizeof(cpu[0]), cpu);

#if 0
        dump_pci_devices();
#endif

        /* Check all of memory */
#if 0
        msr_t msr;
        msr = rdmsr(TOP_MEM2);
        print_debug("TOP_MEM2: ");
        print_debug_hex32(msr.hi);
        print_debug_hex32(msr.lo);
        print_debug("\r\n");
#endif

#if  0
        ram_check(0x00000000, msr.lo+(msr.hi<<32));
#endif

#if 0
        // Check 16MB of memory @ 0
        ram_check(0x00000000, 0x00100000);
        // Check 16MB of memory @ 2GB 
        ram_check(0x80000000, 0x80100000);
#endif


}
