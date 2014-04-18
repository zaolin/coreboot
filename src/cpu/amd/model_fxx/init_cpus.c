//it takes the ENABLE_APIC_EXT_ID and APIC_ID_OFFSET and LIFT_BSP_APIC_ID
#ifndef K8_SET_FIDVID
	#define K8_SET_FIDVID 0
	
#endif

#ifndef K8_SET_FIDVID_CORE0_ONLY
	/* MSR FIDVID_CTL and FIDVID_STATUS are shared by cores, so may don't need to do twice*/
       	#define K8_SET_FIDVID_CORE0_ONLY 1
#endif

typedef void (*process_ap_t)(unsigned apicid, void *gp);

static void for_each_ap(unsigned bsp_apicid, unsigned core0_only, process_ap_t process_ap, void *gp)
{
	// here assume the OS don't change our apicid
	unsigned ap_apicid;

	unsigned nodes;
        unsigned siblings = 0;
        unsigned disable_siblings;
        unsigned e0_later_single_core;
        unsigned nb_cfg_54;
        int i,j;

	/* get_nodes define in in_coherent_ht.c */
	nodes = get_nodes();

        disable_siblings = !CONFIG_LOGICAL_CPUS;
#if CONFIG_LOGICAL_CPUS == 1
        if(read_option(CMOS_VSTART_dual_core, CMOS_VLEN_dual_core, 0) != 0) { // 0 mean dual core
                disable_siblings = 1;
        }
#endif

	/* here I assume that all node are same stepping, otherwise we can use use nb_cfg_54 from bsp for all nodes */
        nb_cfg_54 = read_nb_cfg_54();


        for(i=0; i<nodes;i++) {
		e0_later_single_core = 0;
                j = ((pci_read_config32(PCI_DEV(0, 0x18+i, 3), 0xe8) >> 12) & 3);
                if(nb_cfg_54) {
 	               if(j == 0 ){ // if it is single core, we need to increase siblings for apic calculation 
        	               e0_later_single_core = is_e0_later_in_bsp(i);  // single core
                       } 
                       if(e0_later_single_core) {
                                j=1;
                       }
                }
                siblings = j;

		unsigned jj;

		if(e0_later_single_core || disable_siblings || core0_only) {
			jj = 0;
		} else {
			jj = siblings;
		}	
		
	
                for(j=0; j<=jj; j++) {

                        ap_apicid = i * (nb_cfg_54?(siblings+1):1) + j * (nb_cfg_54?1:8);

                #if (ENABLE_APIC_EXT_ID == 1)
			#if LIFT_BSP_APIC_ID == 0
			if( (i!=0) || (j!=0)) /* except bsp */
			#endif
                        	ap_apicid += APIC_ID_OFFSET;
                #endif

			if(ap_apicid == bsp_apicid) continue;

			process_ap(ap_apicid, gp); 

                }
        }
}


static inline int lapic_remote_read(int apicid, int reg, unsigned *pvalue)
{
        int timeout;
        unsigned status;
        int result;
        lapic_wait_icr_idle();
        lapic_write(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(apicid));
        lapic_write(LAPIC_ICR, LAPIC_DM_REMRD | (reg >> 4));
        timeout = 0;

        do {
                status = lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY;
        } while (status == LAPIC_ICR_BUSY && timeout++ < 1000);

        timeout = 0;
        do {
                status = lapic_read(LAPIC_ICR) & LAPIC_ICR_RR_MASK;
        } while (status == LAPIC_ICR_RR_INPROG && timeout++ < 1000);

        result = -1;
        if (status == LAPIC_ICR_RR_VALID) {
                *pvalue = lapic_read(LAPIC_RRR);
                result = 0;
        }
        return result;
}

#define LAPIC_MSG_REG 0x380


#if K8_SET_FIDVID == 1
static void init_fidvid_ap(unsigned bsp_apicid, unsigned apicid);
#endif

static inline __attribute__((always_inline)) void print_apicid_nodeid_coreid(unsigned apicid, struct node_core_id id, const char *str)
{
	#if CONFIG_USE_INIT == 0
		print_debug(str);
        	print_debug(" ---- {APICID = "); print_debug_hex8(apicid);
		print_debug(" NODEID = "), print_debug_hex8(id.nodeid); print_debug(" COREID = "), print_debug_hex8(id.coreid);
	        print_debug("} --- \r\n");
        #else
                printk_debug("%s --- {  APICID = %02x NODEID = %02x COREID = %02x} ---\r\n", str, apicid, id.nodeid, id.coreid);
        #endif
}


static void wait_cpu_state(unsigned apicid, unsigned state)
{
        unsigned readback;
	int loop =100000;
        while(--loop>0) {
                if(lapic_remote_read(apicid, LAPIC_MSG_REG, &readback)!=0) continue;
                if((readback & 0xff) == state) break; //target cpu is in stage started
        }
}

static void wait_ap_started(unsigned ap_apicid, void *gp )
{
        wait_cpu_state(ap_apicid, 0x33); // started
}

static void wait_all_aps_started(unsigned bsp_apicid)
{
        for_each_ap(bsp_apicid, 0 , wait_ap_started, (void *)0);
}

static void allow_all_aps_stop(unsigned bsp_apicid)
{
        lapic_write(LAPIC_MSG_REG, (bsp_apicid<<24) | 0x44); // allow aps to stop
}


#if RAMINIT_SYSINFO == 1
static unsigned init_cpus(unsigned cpu_init_detectedx ,struct sys_info *sysinfo)
#else
static unsigned init_cpus(unsigned cpu_init_detectedx)
#endif
{
		unsigned bsp_apicid = 0;
		unsigned apicid;
                struct node_core_id id;

		/* 
                 * already set early mtrr in cache_as_ram.inc
		 */

		/* that is from initial apicid, we need nodeid and coreid later */
		id = get_node_core_id_x(); 


                /* NB_CFG MSR is shared between cores, so we need make sure core0 is done at first --- use wait_all_core0_started  */
		if(id.coreid == 0) {
                	set_apicid_cpuid_lo(); /* only set it on core0 */
			#if ENABLE_APIC_EXT_ID == 1
                        enable_apic_ext_id(id.nodeid);
			#endif
                }

		enable_lapic();
//              init_timer(); // We need TMICT to pass msg for FID/VID change

        #if (ENABLE_APIC_EXT_ID == 1)
		unsigned initial_apicid = get_initial_apicid();	
                #if LIFT_BSP_APIC_ID == 0
                if( initial_apicid != 0 ) // other than bsp
                #endif
                {
                                /* use initial apic id to lift it */
                                uint32_t dword = lapic_read(LAPIC_ID);
                                dword &= ~(0xff<<24);
                                dword |= (((initial_apicid + APIC_ID_OFFSET) & 0xff)<<24);

                                lapic_write(LAPIC_ID, dword);
                }

                #if LIFT_BSP_APIC_ID == 1
                bsp_apicid += APIC_ID_OFFSET;
                #endif

        #endif

		/* get the apicid, it may be lifted already */
		apicid = lapicid();

#if 1 
		// show our apicid, nodeid, and coreid
		if( id.coreid==0 ) {
			if (id.nodeid!=0) //all core0 except bsp
				print_apicid_nodeid_coreid(apicid, id, " core0: ");
		}
	#if 0 
                else { //all core1
			print_apicid_nodeid_coreid(apicid, id, " core1: ");
                }
	#endif

#endif

                if (cpu_init_detectedx) {
		#if RAMINIT_SYSINFO == 1
			//We need to init sblnk and sbbusn, because it is called before ht_setup_chains_x
		        sysinfo->sblnk = get_sblnk();
			sysinfo->sbbusn = node_link_to_bus(0, sysinfo->sblnk);
		#endif
			print_apicid_nodeid_coreid(apicid, id, "\r\n\r\n\r\nINIT detect from ");

			print_debug("\r\nIssuing SOFT_RESET...\r\n");

			#if RAMINIT_SYSINFO == 1
                        soft_reset(sysinfo);
			#else
			soft_reset();
			#endif

                }

		if(id.coreid==0) {
	                distinguish_cpu_resets(id.nodeid);
	//              start_other_core(id.nodeid); // start second core in first cpu, only allowed for nb_cfg_54 is not set
		}

		//here don't need to wait 
		lapic_write(LAPIC_MSG_REG, (apicid<<24) | 0x33); // mark the cpu is started

		if(apicid != bsp_apicid) {
	#if K8_SET_FIDVID == 1
		#if (CONFIG_LOGICAL_CPUS == 1) && (K8_SET_FIDVID_CORE0_ONLY == 1)
			if(id.coreid == 0 ) // only need set fid for core0
		#endif 
       		                init_fidvid_ap(bsp_apicid, apicid);
	#endif

                        // We need to stop the CACHE as RAM for this CPU, really?
			wait_cpu_state(bsp_apicid, 0x44);
			lapic_write(LAPIC_MSG_REG, (apicid<<24) | 0x44); // bsp can not check it before stop_this_cpu

			disable_cache_as_ram(); // inline
                        stop_this_cpu(); // inline, it will stop all cores except node0/core0 the bsp .... 
                }

		return bsp_apicid;
}

#if CONFIG_LOGICAL_CPUS == 1

static unsigned is_core0_started(unsigned nodeid)
{
        uint32_t htic;
        device_t device;
        device = PCI_DEV(0, 0x18 + nodeid, 0);
        htic = pci_read_config32(device, HT_INIT_CONTROL);
        htic &= HTIC_INIT_Detect;
        return htic;
}

static void wait_all_core0_started(void)
{
	//When core0 is started, it will distingush_cpu_resets. So wait for that
	unsigned i;
	unsigned nodes = get_nodes();

	for(i=1;i<nodes;i++) { // skip bsp, because it is running on bsp
		while(!is_core0_started(i)) {}
	}
}

#endif
