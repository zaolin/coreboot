#if K8_SET_FIDVID == 1

#define K8_SET_FIDVID_DEBUG 0

#define K8_SET_FIDVID_ONE_BY_ONE 1

#define K8_SET_FIDVID_STORE_AP_APICID_AT_FIRST 1

#ifndef SB_VFSMAF
#define SB_VFSMAF 1
#endif

#define FX_SUPPORT 1

static inline void print_debug_fv(const char *str, unsigned val)
{
#if K8_SET_FIDVID_DEBUG == 1
	#if CONFIG_USE_PRINTK_IN_CAR
		printk_debug("%s%x\r\n", str, val);
	#else
		print_debug(str); print_debug_hex32(val); print_debug("\r\n");
	#endif
#endif
}

static inline void print_debug_fv_8(const char *str, unsigned val)
{
#if K8_SET_FIDVID_DEBUG == 1
	#if CONFIG_USE_PRINTK_IN_CAR
		printk_debug("%s%02x\r\n", str, val);
	#else
		print_debug(str); print_debug_hex8(val); print_debug("\r\n");
	#endif
#endif
}

static inline void print_debug_fv_64(const char *str, unsigned val, unsigned val2)
{
#if K8_SET_FIDVID_DEBUG == 1
	#if CONFIG_USE_PRINTK_IN_CAR
		printk_debug("%s%x%x\r\n", str, val, val2);
	#else
		print_debug(str); print_debug_hex32(val); print_debug_hex32(val2); print_debug("\r\n");
	#endif
#endif
}


static void enable_fid_change(void)
{
	uint32_t dword;
	unsigned nodes;
	int i;

	nodes = ((pci_read_config32(PCI_DEV(0, 0x18, 0), 0x60) >> 4) & 7) + 1;

	for (i = 0; i < nodes; i++) {
		dword = pci_read_config32(PCI_DEV(0, 0x18+i, 3), 0xd8);
		dword &= 0x8ff00000;
		dword |= (2 << 28) | (0x02710);
		pci_write_config32(PCI_DEV(0, 0x18+i, 3), 0xd8, dword);

		dword = 0x04e2a707;
		pci_write_config32(PCI_DEV(0, 0x18+i, 3), 0xd4, dword);

		/* disable the DRAM interface at first, it will be enabled
		 * by raminit again */
		dword = pci_read_config32(PCI_DEV(0, 0x18+i, 2), 0x94);
		dword |= (1 << 14);
		pci_write_config32(PCI_DEV(0, 0x18+i, 2), 0x94, dword);

		dword = 0x23070700; /* enable FID/VID change */
//		dword = 0x00070000; /* enable FID/VID change */
		pci_write_config32(PCI_DEV(0, 0x18+i, 3), 0x80, dword);

#if HAVE_ACPI_RESUME
		dword = 0x21132113;
#else
		dword = 0x00132113;
#endif
		pci_write_config32(PCI_DEV(0, 0x18+i, 3), 0x84, dword);

	}
}

#if K8_SET_FIDVID_ONE_BY_ONE == 0
static unsigned set_fidvid_without_init(unsigned fidvid)
{
	msr_t msr;
	uint32_t vid;
	uint32_t fid;

	fid = (fidvid >> 8) & 0x3f;
	vid = (fidvid >> 16) & 0x3f;

	/* set new FID/VID */
	msr.hi = 1;
	msr.lo = (vid<<8) | fid;
	wrmsr(0xc0010041, msr);
	return fidvid;
}
#endif

static u32 set_fidvid(unsigned apicid, unsigned fidvid, int showmessage)
{

/*  CurrentFID--> 4x(00h)   5x(02h)   6x(04h)   7x(06h) ...
 *       --------------------------------------
 * TargetFID   | Next_FID, Next_FID, Next_FID, Next_FID ...
 *      |      | Next_FID, Next_FID, Next_FID, Next_FID ...
 *     \|/     | Next_FID, Next_FID, Next_FID, Next_FID ...
 */
	static const u8 next_fid_200[] = {
/*         x4  x5  x6  x7  x8  x9 x10 x11 x12 x13 x14 x15  x16 */
/* x4 */    0, -1, -1, -1,  0,  0,  9, 10, 11, 12, 13, 14, 15, /*  800 */
/* x5 */   -1,  0, -1, -1, -1,  5,  5,  5, 11, 12, 13, 14, 15, /* 1000 */
/* x6 */   -1, -1,  0, -1, -1, -1, -1,  6,  6,  6, 13, 14, 15, /* 1200 */
/* x7 */   -1, -1, -1,  0, -1, -1, -1, -1, -1,  7,  7,  7, 15, /* 1400 */
/* lower table to upper table boarder (table 70 and 71 in BKDG) */
/* x8 */    8, -1, -1, -1,  0,  8,  9, 10, 11, 12, 13, 14, 15, /* 1600 */
/* x9 */    9,  9, -1, -1,  9,  0,  9, 10, 11, 12, 13, 14, 15, /* 1800 */
/*x10 */    9, 10, -1, -1,  9, 10,  0, 10, 11, 12, 13, 14, 15, /* 2000 */
/*x11 */    9, 11, 11, -1,  9, 10, 11,  0, 11, 12, 13, 14, 15, /* 2200 */
/*x12 */    9, 11, 12, -1,  9, 10, 11, 12,  0, 12, 13, 14, 15, /* 2400 */
/*x13 */    9, 11, 13, 13,  9, 10, 11, 12, 13,  0, 13, 14, 15, /* 2600 */
/*x14 */    9, 11, 13, 14,  9, 10, 11, 12, 13, 14,  0, 14, 15, /* 2800 */
/*x15 */    9, 11, 13, 15,  9, 10, 11, 12, 13, 14, 15,  0, 15, /* 3000 */
/*x15 */    9, 11, 13, 15,  9, 10, 11, 12, 13, 14, 15, 16,  0, /* 3200 */
	};

	msr_t msr;
	uint32_t vid_new;
	uint32_t fid_new;
	uint32_t vid_max;
	uint32_t fid_max;
	uint32_t vid_cur;
	uint32_t fid_cur;
	unsigned apicidx;

	int step_limit;
	int loop;

	apicidx = lapicid();

	if (apicid != apicidx) {
#if CONFIG_USE_PRINTK_IN_CAR
		printk_err("wrong apicid, we want change %x, but it is %x\r\n", apicid, apicidx);
#else
		print_err("wrong apicid, we want change "); print_err_hex8(apicid); print_err(" but it is "); print_err_hex8(apicidx); print_err("\r\n");
#endif
		return fidvid;
	}

	fid_new = (fidvid >> 8) & 0x3f;
	vid_new = (fidvid >> 16) & 0x3f;

	msr = rdmsr(0xc0010042);

	vid_cur = msr.hi & 0x3f;
	fid_cur = msr.lo & 0x3f;

	if ((vid_cur == vid_new) && (fid_cur == fid_new))
		return fidvid;

	vid_max = (msr.hi >> (48-32)) & 0x3f;
	fid_max = ((msr.lo >> 16) & 0x3f); /* max fid */

#if FX_SUPPORT
	if (fid_max >= ((25 - 4) * 2)) { /* FX max fid is 5G */
		fid_max = ((msr.lo >> 8) & 0x3f) + 5 * 2; /* max FID is min fid + 1G */
		if (fid_max >= ((25-4) * 2)) {
			fid_max = (10-4) * 2; /* hard set to 2G */
		}
	}
#endif

	/* set vid to max */
	/* TODO - make this more correct. Not a big deal for setting max...
	 * BKDG figure 11
	 * if TargetFID > InitialFID
	 *	TargetVID = FinalVID - RVO
	 * else
	 *	if CurrentVID > FinalVID
	 *		TargetVID = FinalVID - RVO
	 *	else
	 *		TargetVID = CurrentVIDD - RVO
	 */
	msr.hi = 1;
	msr.lo = (vid_max << 8) | (fid_cur);
#if SB_VFSMAF == 1
	msr.lo |= (1 << 16); /* init changes */
#endif
	wrmsr(0xc0010041, msr);
#if SB_VFSMAF == 0
	ldtstop_sb();
#endif


	for (loop=0; loop < 100000; loop++){
		msr = rdmsr(0xc0010042);
		if (!(msr.lo & (1 << 31)))
			break;
	}
	vid_cur = msr.hi & 0x3f;

	/* BKDG figure 12 and 13
	 * if current fid is odd
	 *	current fid -1 (next lower and even fid)(odd fid for rev G)
	 * if current fid in high-freq table
	 *	while current fid < target fid
	 *		transition to next higher fid in table
	 * else if target fid > VCO portal of current fid
	 *	transition to highest portal fid in higher fid table
	 *		while current fid < target fid
	 *			transition to next higher fid in table
	 * else
	 *	transition to target fid
	 */

	printk_debug("Current fid_cur: 0x%x, fid_max: 0x%x\n", fid_cur, fid_max);
	printk_debug("Requested fid_new: 0x%x\n", fid_new);

	step_limit = 8; /* max 8 steps just in case... */
	while ((fid_cur != fid_new) && (step_limit--)) {
		u32 fid_temp;
		int step;

		if (fid_cur < fid_new)
			/* Force Fid steps even. step == 0 means 100MHz step */
			step = ((fid_new/2) - (fid_cur/2)) * 2;
		else
			step = ((fid_cur/2) - (fid_new/2)) * 2;

		/* If 200Mhz step OR past 3200 max table value */
		if ((step == 2) || (fid_new >= 0x18 || fid_cur >= 0x18)) {

			printk_debug("200MHZ step ");

			/* Step +/- 200MHz at a time */
			if (fid_cur < fid_new)
				fid_temp = fid_cur + 2;
			else
				fid_temp = fid_cur - 2;


		} else if ( step > 2) {	/* If more than a 200Mhz step */
			int temp;

			/* look it up in the table */
			printk_debug("FidVid table step ");

			temp = next_fid_200[((fid_new/2) * 13) + (fid_cur/2)];

			if (temp > 0)
				fid_temp = (temp-4) * 2; /* Table 108 */
			else if (temp == 0)
				fid_temp = fid_new;
			else
				break; /* table error */

		} else { /* step < 2 (100MHZ) */
			printk_debug("100MHZ step ");

			/* The table adjust in 200MHz increments. If requested,
			 * do the 100MHz increment if the CPU supports it.*/
			if (cpuid_edx(0x80000007) & (1 << 6)) {
				fid_temp = fid_cur + 1;
			} else {
				/* 100 MHZ not supported. Get out of the loop */
				printk_debug("is not supported.\n");
				break;
			}
		}

		if(fid_temp > fid_max) {
			printk_debug("fid_temp 0x%x > fid_max 0x%x\n", fid_temp, fid_max);
			break;
		}

		printk_debug("fidvid: 0x%x\n", fid_temp);

		/* set target fid */
		msr.hi = 0x190; /* 2 us for AMD NPT Family 0Fh Processors */
		msr.lo = (vid_cur << 8) | fid_temp;
#if SB_VFSMAF == 1
		msr.lo |= (1 << 16); /* init changes */
#endif
		wrmsr(0xc0010041, msr);
#if SB_VFSMAF == 0
		ldtstop_sb();
#endif


#if K8_SET_FIDVID_DEBUG == 1
		if (showmessage) {
			print_debug_fv_8("set_fidvid APICID = ", apicid);
			print_debug_fv_64("fidvid ctrl msr ", msr.hi, msr.lo);
		}
#endif

		for (loop = 0; loop < 100000; loop++){
			msr = rdmsr(0xc0010042);
			if (!(msr.lo & (1 << 31)))
				break;
		}
		fid_cur = msr.lo & 0x3f;

#if K8_SET_FIDVID_DEBUG == 1
		if (showmessage)	{
			print_debug_fv_64("fidvid status msr ", msr.hi, msr.lo);
		}
#endif
	}

	/* set vid to final */
	/* TODO - make this more correct. Not a big deal for setting max...
	 * BKDG figure 11
	 * if TargetFID > InitialFID
	 *	TargetVID = FinalVID - RVO
	 * else
	 *	if CurrentVID > FinalVID
	 *		TargetVID = FinalVID - RVO
	 *	else
	 *		TargetVID = CurrentVIDD - RVO
	 */
	msr.hi = 1;
	msr.lo = (vid_new << 8) | (fid_cur);
#if SB_VFSMAF == 1
	msr.lo |= (1 << 16); // init changes
#endif
	wrmsr(0xc0010041, msr);
#if SB_VFSMAF == 0
	ldtstop_sb();
#endif

	for (loop = 0; loop < 100000; loop++){
		msr = rdmsr(0xc0010042);
		if (!(msr.lo & (1 << 31)))
			break;
	}
	vid_cur = msr.hi & 0x3f;

	fidvid = (vid_cur << 16) | (fid_cur << 8);

	if (showmessage) {
		if (vid_new != vid_cur) {
			print_err("set vid failed for apicid ="); print_err_hex8(apicidx);  print_err("\r\n");
		}
		if (fid_new != fid_cur) {
			print_err("set fid failed for apicid ="); print_err_hex8(apicidx); print_err("\r\n");
		}
	}

	return fidvid;

}

static void init_fidvid_ap(unsigned bsp_apicid, unsigned apicid)
{
	uint32_t send;
	uint32_t readback = 0;
	unsigned timeout = 1;
	msr_t msr;
	uint32_t vid_cur;
	uint32_t fid_cur;
	uint32_t fid_max;
	int loop;

	msr =	rdmsr(0xc0010042);
	fid_max = ((msr.lo >> 16) & 0x3f); /* max fid */
#if FX_SUPPORT
	if (fid_max >= ((25-4) * 2)) { /* FX max fid is 5G */
		fid_max = ((msr.lo>>8) & 0x3f) + 5*2; /* maxFID = minFID + 1G */
		if (fid_max >= ((25-4) * 2)) {
			fid_max = (10-4) * 2; // hard set to 2G
		}
	}
#endif
	send = fid_max<<8;

	send |= ((msr.hi >> (48-32)) & 0x3f) << 16; /* max vid */
	send |= (apicid << 24); /* ap apicid */

#if K8_SET_FIDVID_ONE_BY_ONE == 1
	vid_cur = msr.hi & 0x3f;
	fid_cur = msr.lo & 0x3f;

	/* set to current */ 
	msr.hi = 1;
	msr.lo = (vid_cur << 8) | (fid_cur);
	wrmsr(0xc0010041, msr);
#endif

	timeout = wait_cpu_state(bsp_apicid, 1);
	if (timeout) {
		print_initcpu8("fidvid_ap_stage1: time out while reading from BSP on ", apicid);
	}
	/* send signal to BSP about this AP max fid and vid */
	/* AP at state 1 that sent our fid and vid */
	lapic_write(LAPIC_MSG_REG, send | 1);

//	wait_cpu_state(bsp_apicid, 2); /* don't need we can use apicid directly */
	loop = 1000000;
	while (--loop > 0) {
		/* remote read BSP signal that include vid/fid that need to set */
		if (lapic_remote_read(bsp_apicid, LAPIC_MSG_REG, &readback)!=0)
			continue;
		if (((readback >> 24) & 0xff) == apicid)
			break; /* it is this cpu turn */
	}

	if (loop > 0) {
	#if K8_SET_FIDVID_ONE_BY_ONE == 1
			readback = set_fidvid(apicid, readback & 0xffff00, 1); // this AP
	#else
		readback = set_fidvid_without_init(readback & 0xffff00); // this AP
	#endif
		/* send signal to BSP that this AP fid/vid is set */
		/* allow to change state2 is together with apicid */
		/* AP at state that We set the requested fid/vid */
		send = (apicid<<24) | (readback & 0x00ffff00); 
	} else {
		print_initcpu8("fidvid_ap_stage2: time out while reading from BSP on ", apicid);
	}

		lapic_write(LAPIC_MSG_REG, send | 2);

	timeout = wait_cpu_state(bsp_apicid, 3);
	if (timeout) {
		print_initcpu8("fidvid_ap_stage3: time out while reading from BSP on ", apicid);
	}
}

static unsigned calc_common_fidvid(unsigned fidvid, unsigned fidvidx)
{
	/* FIXME: need to check the change path to verify if it is reachable 
	 * when common fid is small than 1.6G */
	if ((fidvid & 0xff00) <= (fidvidx & 0xff00)) {
		return fidvid;
	}
	else {
		return fidvidx;
	}
}

struct fidvid_st {
	unsigned common_fidvid;
};

static void init_fidvid_bsp_stage1(unsigned ap_apicid, void *gp )
{
	unsigned readback = 0;
	unsigned timeout = 1;

	struct fidvid_st *fvp = gp;
	int loop;

	print_debug_fv("state 1: ap_apicid=", ap_apicid);

	loop = 1000000;
	while (--loop > 0) {
		if (lapic_remote_read(ap_apicid, LAPIC_MSG_REG, &readback)!=0)
			continue;
		if ((readback & 0xff) == 1) {
			timeout = 0;
			break; /* target ap is in stage 1 */
		}
	}
	if (timeout) {
		print_initcpu8("fidvid_bsp_stage1: time out while reading from ap ", ap_apicid);
		return;
	}

	print_debug_fv("\treadback=", readback);

	fvp->common_fidvid = calc_common_fidvid(fvp->common_fidvid, readback & 0xffff00);

	print_debug_fv("\tcommon_fidvid=", fvp->common_fidvid);
}

static void init_fidvid_bsp_stage2(unsigned ap_apicid, void *gp)
{
	unsigned readback = 0;
	unsigned timeout = 1;

	struct fidvid_st *fvp = gp;
	int loop;

	print_debug_fv("state 2: ap_apicid=", ap_apicid);

	/* all set to state2 */
	lapic_write(LAPIC_MSG_REG, fvp->common_fidvid | (ap_apicid<<24) | 2);

	loop = 1000000;
	while (--loop > 0) {
		if (lapic_remote_read(ap_apicid, LAPIC_MSG_REG, &readback)!=0)
			continue;
		if ((readback & 0xff) == 2) {
			timeout = 0;
			break; /* target ap is stage 2, it's FID has beed set */
		}
	}

	if (timeout) {
		print_initcpu8("fidvid_bsp_stage2: time out while reading from ap ", ap_apicid);
		return;
	}

	print_debug_fv("\treadback=", readback);
}

#if K8_SET_FIDVID_STORE_AP_APICID_AT_FIRST == 1
struct ap_apicid_st {
	unsigned num;
	unsigned apicid[16]; /* 8 way dual core need 16 */
	/* FIXME: 32 node quad core, may need 128 */
};

static void store_ap_apicid(unsigned ap_apicid, void *gp)
{
	struct ap_apicid_st *p = gp;

	p->apicid[p->num++] = ap_apicid;

}
#endif

static void init_fidvid_bsp(unsigned bsp_apicid)
{
	uint32_t vid_max;
	uint32_t fid_max;

	struct fidvid_st fv;

#if K8_SET_FIDVID_STORE_AP_APICID_AT_FIRST == 1
	struct ap_apicid_st ap_apicidx;
	unsigned i;
#endif

	msr_t msr;
	msr =	rdmsr(0xc0010042);
	fid_max = ((msr.lo >> 16) & 0x3f); /* max fid */
#if FX_SUPPORT == 1
	if (fid_max >= ((25-4) * 2)) { /* FX max fid is 5G */
		fid_max = ((msr.lo >> 8) & 0x3f) + 5*2; /* maxFID = minFID + 1G */
		if (fid_max >= ((25-4) * 2)) {
			fid_max = (10-4) * 2; /* hard set to 2G */
		}
	}
#endif
	vid_max = ((msr.hi>>(48-32)) & 0x3f); //max vid
	fv.common_fidvid = (fid_max << 8)|(vid_max << 16);


	/* for all APs (We know the APIC ID of all APs even the APIC ID is lifted)
	 * remote read from AP about max fid/vid */

	/* let all ap trains to state 1 */
	lapic_write(LAPIC_MSG_REG,  (bsp_apicid << 24) | 1);

	/* calculate the common max fid/vid that could be used for 
	 * all APs and BSP */
#if K8_SET_FIDVID_STORE_AP_APICID_AT_FIRST == 1
	ap_apicidx.num = 0;

	for_each_ap(bsp_apicid, K8_SET_FIDVID_CORE0_ONLY, store_ap_apicid, &ap_apicidx);

	for (i = 0; i < ap_apicidx.num; i++) {
		init_fidvid_bsp_stage1(ap_apicidx.apicid[i], &fv);
	}
#else
	for_each_ap(bsp_apicid, K8_SET_FIDVID_CORE0_ONLY, init_fidvid_bsp_stage1, &fv);
#endif


#if 0
	unsigned fid, vid;
	/* Can we use max only? So we can only set fid in one around,
	 * otherwise we need to set that to max after raminit */
	/* set fid vid to DQS training required */
	fid = (fv.common_fidvid >> 8)  & 0x3f;
	vid = (fv.common_fidvid >> 16) & 0x3f;

	if (fid > (10-4) * 2) {
		fid = (10-4) * 2; // x10
	}

	if (vid >= 0x1f) {
		vid += 4; /* unit is 12.5mV */
	} else {
		vid += 2; /* unit is 25mV */
	}

	fv.common_fidvid = (fid<<8) | (vid<<16);

	print_debug_fv("common_fidvid=", fv.common_fidvid);

#endif

#if K8_SET_FIDVID_ONE_BY_ONE == 1
	/* set BSP fid and vid */
	print_debug_fv("bsp apicid=", bsp_apicid);
	fv.common_fidvid = set_fidvid(bsp_apicid, fv.common_fidvid, 1);
	print_debug_fv("common_fidvid=", fv.common_fidvid);
#endif

	/* For all APs ( We know the APIC ID of all AP even the APIC ID is lifted)
	 *  send signal to the AP it could change it's fid/vid */
	/* remote read singnal from AP that AP is done */

	fv.common_fidvid &= 0xffff00;

	/* set state 2 allow is in init_fidvid_bsp_stage2 */
#if K8_SET_FIDVID_STORE_AP_APICID_AT_FIRST == 1
	for (i = 0; i < ap_apicidx.num; i++) {
		init_fidvid_bsp_stage2(ap_apicidx.apicid[i], &fv);
	}
#else
	for_each_ap(bsp_apicid, K8_SET_FIDVID_CORE0_ONLY, init_fidvid_bsp_stage2, &fv);
#endif

#if K8_SET_FIDVID_ONE_BY_ONE == 0
	/* set BSP fid and vid */
	print_debug_fv("bsp apicid=", bsp_apicid);
	fv.common_fidvid = set_fidvid(bsp_apicid, fv.common_fidvid, 1);
	print_debug_fv("common_fidvid=", fv.common_fidvid);

#endif

	/* clear the state */
	lapic_write(LAPIC_MSG_REG, fv.common_fidvid | (bsp_apicid << 24) | 3);

	/* here wait a while, so last ap could read pack, and stop it, don't
	 * call init_timer too early or just don't use init_timer */

}

#endif
