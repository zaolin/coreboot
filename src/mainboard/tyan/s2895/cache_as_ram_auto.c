#define ASSEMBLY 1
#define __ROMCC__
 
#include <stdint.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"


#include "northbridge/amd/amdk8/cpu_rev.c"
#define K8_HT_FREQ_1G_SUPPORT 0
#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "southbridge/nvidia/ck804/ck804_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#if CONFIG_USE_INIT == 0
#include "lib/memcpy.c"
#endif

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "superio/smsc/lpc47b397/lpc47b397_early_serial.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "superio/smsc/lpc47b397/lpc47b397_early_gpio.c"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, LPC47B397_SP1)

static void hard_reset(void)
{
        set_bios_reset();

        /* full reset */
	outb(0x0a, 0x0cf9);
        outb(0x0e, 0x0cf9);
}

static void soft_reset(void)
{
        set_bios_reset();
#if 1
        /* link reset */
	outb(0x02, 0x0cf9);
        outb(0x06, 0x0cf9);
#endif
}

static void memreset_setup(void)
{
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

#define SUPERIO_GPIO_DEV PNP_DEV(0x2e, LPC47B397_RT)

#define SUPERIO_GPIO_IO_BASE 0x400

static void sio_gpio_setup(void){

        unsigned value;


#if 1
        /*Enable onboard scsi*/
        lpc47b397_gpio_offset_out(SUPERIO_GPIO_IO_BASE, 0x2c, (1<<7)|(0<<2)|(0<<1)|(0<<0)); // GP21, offset 0x2c, DISABLE_SCSI_L 
        value = lpc47b397_gpio_offset_in(SUPERIO_GPIO_IO_BASE, 0x4c);
        lpc47b397_gpio_offset_out(SUPERIO_GPIO_IO_BASE, 0x4c, (value|(1<<1)));
#endif

}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#define K8_4RANK_DIMM_SUPPORT 1

#include "northbridge/amd/amdk8/raminit.c"
#if 0
        #define ENABLE_APIC_EXT_ID 1
        #define APIC_ID_OFFSET 0x10
        #define LIFT_BSP_APIC_ID 0
#else
        #define ENABLE_APIC_EXT_ID 0
#endif
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "sdram/generic_sdram.c"

 /* tyan does not want the default */
#include "resourcemap.c" 

#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#include "cpu/amd/dualcore/dualcore.c"
#endif

#define FIRST_CPU  1
#define SECOND_CPU 1
#define TOTAL_CPUS (FIRST_CPU + SECOND_CPU)

#define CK804_NUM 2
//#define CK804B_BUSN 0x80
#define CK804B_BUSN 0xc
#define CK804_USE_NIC 1
#define CK804_USE_ACI 1

#include "southbridge/nvidia/ck804/ck804_early_setup_ss.h"

//set GPIO to input mode
#define CK804_MB_SETUP \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+ 5, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* M9,GPIO6, PCIXB2_PRSNT1_L*/  \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+15, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* M8,GPIO16, PCIXB2_PRSNT2_L*/ \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+44, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* P5,GPIO45, PCIXA_PRSNT1_L*/  \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+ 7, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* M5,GPIO8, PCIXA_PRSNT2_L*/   \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+16, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* K4,GPIO17, PCIXB_PRSNT1_L*/  \
                RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+45, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* P7,GPIO46, PCIXB_PRSNT2_L*/

#include "southbridge/nvidia/ck804/ck804_early_setup.c"

#include "cpu/amd/car/copy_and_run.c"

#if USE_FALLBACK_IMAGE == 1

#include "southbridge/nvidia/ck804/ck804_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"


static void sio_setup(void)
{

        unsigned value;
        uint32_t dword;
        uint8_t byte;

        
        pci_write_config32(PCI_DEV(0, CK804_DEVN_BASE+1, 0), 0xac, 0x047f0400);
        
        byte = pci_read_config32(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0x7b);
        byte |= 0x20; 
        pci_write_config8(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0x7b, byte);
        
        dword = pci_read_config32(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0xa0);
        dword |= (1<<29)|(1<<0);
        pci_write_config32(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0xa0, dword);
        
#if  1  
        lpc47b397_enable_serial(SUPERIO_GPIO_DEV, SUPERIO_GPIO_IO_BASE);
                
        value =  lpc47b397_gpio_offset_in(SUPERIO_GPIO_IO_BASE, 0x77);
        value &= 0xbf; 
        lpc47b397_gpio_offset_out(SUPERIO_GPIO_IO_BASE, 0x77, value);
#endif

}

void real_main(unsigned long bist);

void amd64_main(unsigned long bist)
{
#if CONFIG_LOGICAL_CPUS==1
        struct node_core_id id;
#else
        unsigned nodeid;
#endif
        /* Make cerain my local apic is useable */
//        enable_lapic();
        
#if CONFIG_LOGICAL_CPUS==1
        id = get_node_core_id_x();
        /* Is this a cpu only reset? */
        if (cpu_init_detected(id.nodeid)) {
#else   
//        nodeid = lapicid() & 0xf;
        nodeid = get_node_id();
        /* Is this a cpu only reset? */
        if (cpu_init_detected(nodeid)) {
#endif
                if (last_boot_normal()) {
                        goto normal_image;
                } else {
                        goto cpu_reset;
                }
        }

        /* Is this a secondary cpu? */
        if (!boot_cpu()) {
                if (last_boot_normal()) {
                        goto normal_image;
                } else {
                        goto fallback_image;
                }
        }

        /* Nothing special needs to be done to find bus 0 */
        /* Allow the HT devices to be found */

        enumerate_ht_chain();

        sio_setup();

        /* Setup the ck804 */
        ck804_enable_rom();

        /* Is this a deliberate reset by the bios */
        if (bios_reset_detected() && last_boot_normal()) {
                goto normal_image;
        }
        /* This is the primary cpu how should I boot? */
        else if (do_normal_boot()) {
                goto normal_image;
        }
        else {
                goto fallback_image;
        }
 normal_image:
        __asm__ volatile ("jmp __normal_image"
                : /* outputs */
                : "a" (bist) /* inputs */
                );
 cpu_reset:
#if 0
        //CPU reset will reset memtroller ???
        asm volatile ("jmp __cpu_reset" 
                : /* outputs */ 
                : "a"(bist) /* inputs */
                );
#endif

 fallback_image:
        real_main(bist);
}
void real_main(unsigned long bist)
#else
void amd64_main(unsigned long bist)
#endif
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

	unsigned cpu_reset = 0;

        if (bist == 0) {
#if CONFIG_LOGICAL_CPUS==1
        	struct node_core_id id;
#else
	        unsigned nodeid;
#endif
                /* Skip this if there was a built in self test failure */
//                amd_early_mtrr_init(); # don't need, already done in cache_as_ram

#if CONFIG_LOGICAL_CPUS==1
                set_apicid_cpuid_lo();
                id = get_node_core_id_x(); // that is initid
        #if ENABLE_APIC_EXT_ID == 1
                if(id.coreid == 0) {
                        enable_apic_ext_id(id.nodeid);
                }
        #endif
#else
                nodeid = get_node_id();
        #if ENABLE_APIC_EXT_ID == 1
                enable_apic_ext_id(nodeid);
        #endif
#endif

		enable_lapic();

                init_timer();


#if CONFIG_LOGICAL_CPUS==1
        #if ENABLE_APIC_EXT_ID == 1
            #if LIFT_BSP_APIC_ID == 0
                if( id.nodeid != 0 ) //all except cores in node0
            #endif
                        lapic_write(LAPIC_ID, ( lapic_read(LAPIC_ID) | (APIC_ID_OFFSET<<24) ) );
        #endif
                if(id.coreid == 0) {
                        if (cpu_init_detected(id.nodeid)) {
                                cpu_reset = 1;
                                goto cpu_reset_x;
                        }
                        distinguish_cpu_resets(id.nodeid);
                }
#else
        #if ENABLE_APIC_EXT_ID == 1
            #if LIFT_BSP_APIC_ID == 0
                if(nodeid != 0)
            #endif
                        lapic_write(LAPIC_ID, ( lapic_read(LAPIC_ID) | (APIC_ID_OFFSET<<24) ) ); // CPU apicid is from 0x10

        #endif
                if (cpu_init_detected(nodeid)) {
                                cpu_reset = 1;
                                goto cpu_reset_x;
                }
                distinguish_cpu_resets(nodeid);
#endif


                if (!boot_cpu()
#if CONFIG_LOGICAL_CPUS==1 
                        || (id.coreid != 0)
#endif
                ) {
			// We need stop the CACHE as RAM for this CPU too
			#include "cpu/amd/car/cache_as_ram_post.c"
                        stop_this_cpu(); // it will stop all cores except core0 of cpu0
                }
        }


	lpc47b397_enable_serial(SERIAL_DEV, TTYS0_BASE);
        uart_init();
        console_init();
	
	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

        setup_s2895_resource_map();
#if 0
        dump_pci_device(PCI_DEV(0, 0x18, 0));
	dump_pci_device(PCI_DEV(0, 0x19, 0));
#endif

	needs_reset = setup_coherent_ht_domain();

#if CONFIG_LOGICAL_CPUS==1
        // It is said that we should start core1 after all core0 launched
        start_other_cores();
#endif

#if CK804B_BUSN == 0x80 
        // You need to preset bus num in PCI_DEV(0, 0x18,1) 0xe0, 0xe4, 0xe8, 0xec
        needs_reset |= ht_setup_chains(3);
#else
        // automatically set that for you, but you might meet tight space
        // Bcause it has two Ck804, we need to set CK804B_BUSN to 0xc (ht_setup_chains_x will let second CK804 use that bus num.    
    // otherwise ck804_eary_setup can not work rightly.
        needs_reset |= ht_setup_chains_x();
#endif

        needs_reset |= ck804_early_setup_x();

       	if (needs_reset) {
               	print_info("ht reset -\r\n");
               	soft_reset();
       	}

	enable_smbus();
#if 0
	dump_spd_registers(&cpu[0]);
#endif
#if 0
	dump_smbus_registers();
#endif

	memreset_setup();
	sdram_initialize(sizeof(cpu)/sizeof(cpu[0]), cpu);

#if 0
	dump_pci_devices();
#endif

#if 1
	{	
	/* Check value of esp to verify if we have enough rom for stack in Cache as RAM */
	unsigned v_esp;
        __asm__ volatile (
	        "movl	%%esp, %0\n\t"
                : "=a" (v_esp)
        );
#if CONFIG_USE_INIT
	printk_debug("v_esp=%08x\r\n", v_esp); 
#else
	print_debug("v_esp="); print_debug_hex32(v_esp); print_debug("\r\n");
#endif
	}

#endif

cpu_reset_x:    

#if CONFIG_USE_INIT
	printk_debug("cpu_reset = %08x\r\n",cpu_reset);
#else
	print_debug("cpu_reset = "); print_debug_hex32(cpu_reset); print_debug("\r\n");
#endif

        if(cpu_reset == 0) {
                print_debug("Clearing initial memory region: ");
        }       
        print_debug("No cache as ram now - ");
        
        /* store cpu_reset to ebx */
        __asm__ volatile (
                "movl %0, %%ebx\n\t"
                ::"a" (cpu_reset)
        );      

        if(cpu_reset==0) {
#define CLEAR_FIRST_1M_RAM 1
#include "cpu/amd/car/cache_as_ram_post.c"
        }
        else {
#undef CLEAR_FIRST_1M_RAM 
#include "cpu/amd/car/cache_as_ram_post.c"
        }

        __asm__ volatile (
                /* set new esp */ /* before _RAMBASE */
                "movl   %0, %%ebp\n\t"
                "movl   %0, %%esp\n\t"
		::"a"( _RAMBASE - 4 )
        );

        {
                unsigned new_cpu_reset;

                /* get back cpu_reset from ebx */
                __asm__ volatile (
                        "movl %%ebx, %0\n\t"
                        :"=a" (new_cpu_reset)
                );

                /* We can not go back any more, we lost old stack data in cache as ram*/
                if(new_cpu_reset==0) {
                        print_debug("Use Ram as Stack now - done\r\n");
                } else
                {  
                        print_debug("Use Ram as Stack now - \r\n");
                }
#if CONFIG_USE_INIT
		printk_debug("new_cpu_reset = %08x\r\n", new_cpu_reset);
#else
                print_debug("new_cpu_reset = "); print_debug_hex32(new_cpu_reset); print_debug("\r\n");
#endif

                /*copy and execute linuxbios_ram */
                copy_and_run(new_cpu_reset);
                /* We will not return */
        }


        print_err("should not be here -\r\n");
}
