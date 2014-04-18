#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include "chip.h"
#include "northbridge.h"
#include <cpu/amd/gx2def.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/cache.h>

/* put this here for now, we are not sure where it belongs */

struct gliutable {
	unsigned long desc_name;
	unsigned short desc_type;
	unsigned long hi, lo;
};

struct gliutable gliu0table[] = {
	{.desc_name=MSR_GLIU0_BASE1, .desc_type= BM,.hi= MSR_MC + 0x0,.lo=  0x0FFF80},		/*  0-7FFFF to MC*/
	{.desc_name=MSR_GLIU0_BASE2, .desc_type= BM,.hi= MSR_MC + 0x0,.lo=(0x80 << 20) + 0x0FFFE0},		/*  80000-9ffff to Mc*/
	{.desc_name=MSR_GLIU0_SHADOW,.desc_type= SC_SHADOW,.hi=  MSR_MC + 0x0,.lo=  0x03},	/*  C0000-Fffff split to MC and PCI (sub decode) A0000-Bffff handled by SoftVideo*/
	{.desc_name=MSR_GLIU0_SYSMEM,.desc_type= R_SYSMEM,.hi=  MSR_MC,.lo=  0x0},		/*  Catch and fix dynamicly.*/
	{.desc_name=MSR_GLIU0_DMM,   .desc_type= BMO_DMM,.hi=  MSR_MC,.lo=  0x0},		/*  Catch and fix dynamicly.*/
	{.desc_name=MSR_GLIU0_SMM,   .desc_type= BMO_SMM,.hi=  MSR_MC,.lo=  0x0},		/*  Catch and fix dynamicly.*/
	{.desc_name=GLIU0_GLD_MSR_COH,.desc_type= OTHER,.hi= 0x0,.lo= GL0_CPU},
	{.desc_name=GL_END,          .desc_type= GL_END,.hi= 0x0,.lo= 0x0},
};


struct gliutable gliu1table[] = {
	{.desc_name=MSR_GLIU1_BASE1,.desc_type=  BM,.hi=  MSR_GL0 + 0x0,.lo=  0x0FFF80},	/*  0-7FFFF to MC*/
	{.desc_name=MSR_GLIU1_BASE2,.desc_type=  BM,.hi=  MSR_GL0 + 0x0,.lo= (0x80 << 20) +0x0FFFE0},	/*  80000-9ffff to Mc*/
	{.desc_name=MSR_GLIU1_SHADOW,.desc_type=  SC_SHADOW,.hi=  MSR_GL0 + 0x0,.lo=  0x03},/*  C0000-Fffff split to MC and PCI (sub decode)*/
	{.desc_name=MSR_GLIU1_SYSMEM,.desc_type=  R_SYSMEM,.hi=  MSR_GL0,.lo=  0x0},		/*  Cat0xc and fix dynamicly.*/
	{.desc_name=MSR_GLIU1_DMM,.desc_type=  BM_DMM,.hi=  MSR_GL0,.lo=  0x0},			/*  Cat0xc and fix dynamicly.*/
	{.desc_name=MSR_GLIU1_SMM,.desc_type=  BM_SMM,.hi=  MSR_GL0,.lo=  0x0},			/*  Cat0xc and fix dynamicly.*/
	{.desc_name=GLIU1_GLD_MSR_COH,.desc_type= OTHER,.hi= 0x0,.lo= GL1_GLIU0},
	{.desc_name=MSR_GLIU1_FPU_TRAP,.desc_type=  SCIO,.hi=  (GL1_GLCP << 29) + 0x0,.lo=  0x033000F0},	/*  FooGlue FPU 0xF0*/
	{.desc_name=GL_END,.desc_type= GL_END,.hi= 0x0,.lo= 0x0},
};

struct gliutable *gliutables[]  = {gliu0table, gliu1table, 0};

struct msrinit {
	unsigned long msrnum;
	msr_t msr;
};

struct msrinit ClockGatingDefault [] = {
	{GLIU0_GLD_MSR_PM,	{.hi=0x00,.lo=0x0005}},
			/*  MC must stay off in SDR mode. It is turned on in CPUBug??? lotus #77.142*/
	{MC_GLD_MSR_PM,		{.hi=0x00,.lo=0x0000}},
	{GLIU1_GLD_MSR_PM,	{.hi=0x00,.lo=0x0005}},
	{VG_GLD_MSR_PM,		{.hi=0x00,.lo=0x0000}},			/*  lotus #77.163*/
	{GP_GLD_MSR_PM,		{.hi=0x00,.lo=0x0001}},
	{DF_GLD_MSR_PM,		{.hi=0x00,.lo=0x0155}},
	{GLCP_GLD_MSR_PM,	{.hi=0x00,.lo=0x0015}},
	{GLPCI_GLD_MSR_PM,	{.hi=0x00,.lo=0x0015}},
	{FG_GLD_MSR_PM,		{.hi=0x00,.lo=0x0000}},			/* Always on*/
	{0xffffffff, 				{0xffffffff, 0xffffffff}},
};
	/*  All On*/
struct msrinit ClockGatingAllOn[] = {
	{GLIU0_GLD_MSR_PM,	{.hi=0x00,.lo=0x0FFFFFFFF}},
	{MC_GLD_MSR_PM,		{.hi=0x00,.lo=0x0FFFFFFFF}},
	{GLIU1_GLD_MSR_PM,	{.hi=0x00,.lo=0x0FFFFFFFF}},
	{VG_GLD_MSR_PM,		{.hi=0x00, .lo=0x00}},
	{GP_GLD_MSR_PM,		{.hi=0x00,.lo=0x000000001}},
	{DF_GLD_MSR_PM,		{.hi=0x00,.lo=0x0FFFFFFFF}},
	{GLCP_GLD_MSR_PM,	{.hi=0x00,.lo=0x0FFFFFFFF}},
	{GLPCI_GLD_MSR_PM,	{.hi=0x00,.lo=0x0FFFFFFFF}},
	{FG_GLD_MSR_PM,		{.hi=0x00,.lo=0x0000}},
 	{0xffffffff, 				{0xffffffff, 0xffffffff}},
};

	/*  Performance*/
struct msrinit ClockGatingPerformance[] = {
	{VG_GLD_MSR_PM,		{.hi=0x00,.lo=0x0000}},				/*  lotus #77.163*/
	{GP_GLD_MSR_PM,		{.hi=0x00,.lo=0x0001}},
	{DF_GLD_MSR_PM,		{.hi=0x00,.lo=0x0155}},
	{GLCP_GLD_MSR_PM,	{.hi=0x00,.lo=0x0015}},
	{0xffffffff, 				{0xffffffff, 0xffffffff}},
};
/* */
/*  SET GeodeLink PRIORITY*/
/* */
struct msrinit GeodeLinkPriorityTable [] = {
	{CPU_GLD_MSR_CONFIG,		{.hi=0x00,.lo=0x0220}},		/*  CPU Priority.*/
	{DF_GLD_MSR_MASTER_CONF,	{.hi=0x00,.lo=0x0000}},		/*  DF Priority.*/
	{VG_GLD_MSR_CONFIG,		{.hi=0x00,.lo=0x0720}},		/*  VG Primary and Secondary Priority.*/
	{GP_GLD_MSR_CONFIG,		{.hi=0x00,.lo=0x0010}},		/*  Graphics Priority.*/
	{GLPCI_GLD_MSR_CONFIG,		{.hi=0x00,.lo=0x0017}},		/*  GLPCI Priority + PID*/
	{GLCP_GLD_MSR_CONF,		{.hi=0x00,.lo=0x0001}},		/*  GLCP Priority + PID*/
	{VIP_GLD_MSR_CONFIG,		{.hi=0x00,.lo=0x0622}},		/*  VIP PID*/
	{AES_GLD_MSR_CONFIG,		{.hi=0x00,.lo=0x0013}},		/*  AES PID*/
	{0x0FFFFFFFF, 			{0x0FFFFFFFF, 0x0FFFFFFFF}},	/*  END*/
};

/* do we have dmi or not? assume yes */
int havedmi = 1;

static void
writeglmsr(struct gliutable *gl){
	msr_t msr;

	msr.lo = gl->lo;
	msr.hi = gl->hi;
	wrmsr(gl->desc_name, msr);
	printk_debug("%s: write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, gl->desc_name, msr.hi, msr.lo);
	/* they do this, so we do this */
	msr = rdmsr(gl->desc_name);
	printk_debug("%s: AFTER write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, gl->desc_name, msr.hi, msr.lo);
}

static void
ShadowInit(struct gliutable *gl)
{
	msr_t msr;

	msr = rdmsr(gl->desc_name);

	if (msr.lo == 0) {
		writeglmsr(gl); 
	}
}

/* NOTE: transcribed from assembly code. There is the usual redundant assembly nonsense in here. 
  * CLEAN ME UP
   */
/* yes, this duplicates later code, but it seems that is how they want it done. 
  */
extern int sizeram(void);
static void
SysmemInit(struct gliutable *gl)
{
	msr_t msr;
	int sizembytes, sizebytes;

	sizembytes = sizeram();
	printk_debug("%s: enable for %dm bytes\n", __FUNCTION__, sizembytes);
	sizebytes = sizembytes << 20;

	sizebytes -= SMM_SIZE*1024 +1;

	if (havedmi)
		sizebytes -= DMM_SIZE * 1024 + 1;

	sizebytes -= 1;
	msr.hi = gl->hi | (sizebytes >> 24);
	/* set up sizebytes to fit into msr.lo */
	sizebytes <<= 8; /* what? well, we want bits 23:12 in bytes 31:20. */
	sizebytes &= 0xfff00000;
	sizebytes |= 0x100;
	msr.lo = sizebytes;
	wrmsr(gl->desc_name, msr);
	msr = rdmsr(gl->desc_name);
	printk_debug("%s: AFTER write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, 
				gl->desc_name, msr.hi, msr.lo);
	
}
static void
DMMGL0Init(struct gliutable *gl) {
	msr_t msr;
	int sizebytes = sizeram()<<20;
	long offset;

	if (! havedmi)
		return;

	printk_debug("%s: %d bytes\n", __FUNCTION__, sizebytes);

	sizebytes -= DMM_SIZE*1024;
	offset = sizebytes - DMM_OFFSET;
	printk_debug("%s: offset is 0x%x\n", __FUNCTION__, offset);
	offset >>= 12;
	msr.hi = (gl->hi) | (offset << 8);
	/* I don't think this is needed */
	msr.hi &= 0xffffff00;
	msr.hi |= (DMM_OFFSET >> 24);
	msr.lo = DMM_OFFSET << 8;
	msr.lo |= ((~(DMM_SIZE*1024)+1)>>12)&0xfffff;
	
	wrmsr(gl->desc_name, msr);
	msr = rdmsr(gl->desc_name);
	printk_debug("%s: AFTER write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, gl->desc_name, msr.hi, msr.lo);
	
}
static void
DMMGL1Init(struct gliutable *gl) {
	msr_t msr;

	if (! havedmi)
		return;

	printk_debug("%s:\n", __FUNCTION__ );

	msr.hi = gl->hi;
	/* I don't think this is needed */
	msr.hi &= 0xffffff00;
	msr.hi |= (DMM_OFFSET >> 24);
	msr.lo = DMM_OFFSET << 8;
	/* hmm. AMD source has SMM here ... SMM, not DMM? We think DMM */
	printk_err("%s: warning, using DMM_SIZE even though AMD used SMM_SIZE\n", __FUNCTION__);
	msr.lo |= ((~(DMM_SIZE*1024)+1)>>12)&0xfffff;
	
	wrmsr(gl->desc_name, msr);
	msr = rdmsr(gl->desc_name);
	printk_debug("%s: AFTER write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, gl->desc_name, msr.hi, msr.lo);
}
static void
SMMGL0Init(struct gliutable *gl) {
	msr_t msr;
	int sizebytes = sizeram()<<20;
	long offset;

	sizebytes -= SMM_SIZE*1024;

	if (havedmi)
		sizebytes -= DMM_SIZE * 1024;

	printk_debug("%s: %d bytes\n", __FUNCTION__, sizebytes);

	offset = sizebytes - SMM_OFFSET;
	printk_debug("%s: offset is 0x%x\n", __FUNCTION__, offset);
	offset >>= 12;

	msr.hi = offset << 8;
	msr.hi |= SMM_OFFSET>>24;

	msr.lo = SMM_OFFSET << 8;
	msr.lo |= ((~(SMM_SIZE*1024)+1)>>12)&0xfffff;
	
	wrmsr(gl->desc_name, msr);
	msr = rdmsr(gl->desc_name);
	printk_debug("%s: AFTER write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, gl->desc_name, msr.hi, msr.lo);
}
static void
SMMGL1Init(struct gliutable *gl) {
	msr_t msr;
	printk_debug("%s:\n", __FUNCTION__ );

	msr.hi = gl->hi;
	/* I don't think this is needed */
	msr.hi &= 0xffffff00;
	msr.hi |= (SMM_OFFSET >> 24);
	msr.lo = SMM_OFFSET << 8;
	msr.lo |= ((~(SMM_SIZE*1024)+1)>>12)&0xfffff;
	
	wrmsr(gl->desc_name, msr);
	msr = rdmsr(gl->desc_name);
	printk_debug("%s: AFTER write msr 0x%x, val 0x%x:0x%x\n", __FUNCTION__, gl->desc_name, msr.hi, msr.lo);
}

static void
GLIUInit(struct gliutable *gl){

	while (gl->desc_type != GL_END){
		switch(gl->desc_type){
		default: 
				printk_err("%s: name %x, type %x, hi %x, lo %x: unsupported  type: ", __FUNCTION__, 
							gl->desc_name, gl->desc_type, gl->hi, gl->hi);
				printk_err("Must be %x, %x, %x, %x, %x, or %x\n", SC_SHADOW,R_SYSMEM,BMO_DMM,
											BM_DMM, BMO_SMM,BM_SMM);
	
		case SC_SHADOW: /*  Check for a Shadow entry*/
			ShadowInit(gl);
			break;
	
		case R_SYSMEM: /*  check for a SYSMEM entry*/
			SysmemInit(gl);
			break;
	
		case 	BMO_DMM: /*  check for a DMM entry*/
			DMMGL0Init(gl);
			break;
	
		case BM_DMM	: /*  check for a DMM entry*/
			DMMGL1Init(gl);
			break;
	
		case BMO_SMM	: /*  check for a SMM entry*/
			SMMGL0Init(gl);
			break;
	
		case BM_SMM	: /*  check for a SMM entry*/
			SMMGL1Init(gl);	
			break;
		}
		gl++;
	}

}
	/* ***************************************************************************/
	/* **/
	/* *	GLPCIInit*/
	/* **/
	/* *	Set up GLPCI settings for reads/write into memory*/
	/* *	R0:  0-640KB,*/
	/* *	R1:  1MB - Top of System Memory*/
	/* *	R2: SMM Memory*/
	/* *	R3: Framebuffer? - not set up yet*/
	/* *	R4: ??*/
	/* **/
	/* *	Entry:*/
	/* *	Exit:*/
	/* *	Modified:*/
	/* **/
	/* ***************************************************************************/
static void GLPCIInit(void){
	struct gliutable *gl = 0;
	int i;
	msr_t msr;
	int msrnum;
	unsigned long  val;
	/* */
	/*  R0 - GLPCI settings for Conventional Memory space.*/
	/* */
	msr.hi =  (0x09F000 >> 12) << GLPCI_RC_UPPER_TOP_SHIFT		/*  640*/;
	msr.lo =  0													/*  0*/;
	msr.lo |= GLPCI_RC_LOWER_EN_SET+ GLPCI_RC_LOWER_PF_SET + GLPCI_RC_LOWER_WC_SET;
	msrnum = GLPCI_RC0;
	wrmsr(msrnum, msr);

	/* */
	/*  R1 - GLPCI settings for SysMem space.*/
	/* */
	/*  Get systop from GLIU0 SYSTOP Descriptor*/
	for(i = 0; gliu0table[i].desc_name != GL_END; i++) {
		if (gliu0table[i].desc_type == R_SYSMEM) {
			gl = &gliu0table[i];
			break;
		}
	}
	if (gl) {
		unsigned long pah, pal;
		msrnum = gl->desc_name;
		msr = rdmsr(msrnum);
		/* example R_SYSMEM value: 20:00:00:0f:fb:f0:01:00
                 * translates to a base of 0x00100000 and top of 0xffbf0000
                 * base of 1M and top of around 256M
		 */
		/* we have to create a page-aligned (4KB page) address for base and top */
		/* So we need a high page aligned addresss (pah) and low page aligned address (pal)
		 * pah is from msr.hi << 12 | msr.low >> 20. pal is msr.lo << 12
		 */
		printk_debug("GLPCI r1: system msr.lo 0x%x msr.hi 0x%x\n", msr.lo, msr.hi);
		pah = ((msr.hi &0xff) << 12) | ((msr.lo >> 20) & 0xfff);
		/* we have the page address. Now make it a page-aligned address */
		pah <<= 12;

		pal = msr.lo << 12;
		msr.hi =  pah;
		msr.lo =  pal;
		msr.lo |= GLPCI_RC_LOWER_EN_SET | GLPCI_RC_LOWER_PF_SET | GLPCI_RC_LOWER_WC_SET;
		printk_debug("GLPCI r1: system msr.lo 0x%x msr.hi 0x%x\n", msr.lo, msr.hi);
		msrnum = GLPCI_RC1;
		wrmsr(msrnum, msr);
	}

	/* */
	/*  R2 - GLPCI settings for SMM space.*/
	/* */
	msr.hi =  ((SMM_OFFSET+(SMM_SIZE*1024-1)) >> 12) << GLPCI_RC_UPPER_TOP_SHIFT;
	msr.lo =  (SMM_OFFSET >> 12) << GLPCI_RC_LOWER_BASE_SHIFT;
	msr.lo |= GLPCI_RC_LOWER_EN_SET | GLPCI_RC_LOWER_PF_SET;
	msrnum = GLPCI_RC2;
	wrmsr(msrnum, msr);

	/* this is done elsewhere already, but it does no harm to do it more than once */
	/*  write serialize memory hole to PCI. Need to to unWS when something is shadowed regardless of cachablility.*/
	msr.lo =  0x021212121								/*  cache disabled and write serialized*/;
	msr.hi =  0x021212121								/*  cache disabled and write serialized*/;

	msrnum = CPU_RCONF_A0_BF;
	wrmsr(msrnum, msr);

	msrnum = CPU_RCONF_C0_DF;
	wrmsr(msrnum, msr);

	msrnum = CPU_RCONF_E0_FF;
	wrmsr(msrnum, msr);

	/*  Set Non-Cacheable Read Only for NorthBound Transactions to Memory. The Enable bit is handled in the Shadow setup.*/
	msrnum = GLPCI_A0_BF;
	msr.hi =  0x35353535;
	msr.lo =  0x35353535;
	wrmsr(msrnum, msr);

	msrnum = GLPCI_C0_DF;
	msr.hi =  0x35353535;
	msr.lo =  0x35353535;
	wrmsr(msrnum, msr);

	msrnum = GLPCI_E0_FF;
	msr.hi =  0x35353535;
	msr.lo =  0x35353535;
	wrmsr(msrnum, msr);

	/*  Set WSREQ*/
	msrnum = CPU_DM_CONFIG0;
	msr = rdmsr(msrnum);
	msr.hi &= ~ (7 << DM_CONFIG0_UPPER_WSREQ_SHIFT);
	msr.hi |= 2 << DM_CONFIG0_UPPER_WSREQ_SHIFT	;	/*  reduce to 1 for safe mode.*/
	wrmsr(msrnum, msr);

	/* we are ignoring the 5530 case for now, and perhaps forever. */

	/* */
	/* 5535 NB Init*/
	/* */	
	msrnum = GLPCI_ARB;
	msr = rdmsr(msrnum);
	msr.hi |=  GLPCI_ARB_UPPER_PRE0_SET | GLPCI_ARB_UPPER_PRE1_SET;
	msr.lo |=  GLPCI_ARB_LOWER_IIE_SET;
	wrmsr(msrnum, msr);


	msrnum = GLPCI_CTRL;
	msr = rdmsr(msrnum);

	msr.lo |=  GLPCI_CTRL_LOWER_ME_SET | GLPCI_CTRL_LOWER_OWC_SET | GLPCI_CTRL_LOWER_PCD_SET; 	/*   (Out will be disabled in CPUBUG649 for < 2.0 parts .)*/
	msr.lo |=  GLPCI_CTRL_LOWER_LDE_SET;

	msr.lo &=  ~ (0x03 << GLPCI_CTRL_LOWER_IRFC_SHIFT);
	msr.lo |=  0x02 << GLPCI_CTRL_LOWER_IRFC_SHIFT;

	msr.lo &=  ~ (0x07 << GLPCI_CTRL_LOWER_IRFT_SHIFT);
	msr.lo |=  0x06 << GLPCI_CTRL_LOWER_IRFT_SHIFT;
	
	msr.hi &=  ~ (0x0f << GLPCI_CTRL_UPPER_FTH_SHIFT);
	msr.hi |=  0x0F << GLPCI_CTRL_UPPER_FTH_SHIFT;
	
	msr.hi &=  ~ (0x0f << GLPCI_CTRL_UPPER_RTH_SHIFT);
	msr.hi |=  0x0F << GLPCI_CTRL_UPPER_RTH_SHIFT;
	
	msr.hi &=  ~ (0x0f << GLPCI_CTRL_UPPER_SBRTH_SHIFT);
	msr.hi |=  0x0F << GLPCI_CTRL_UPPER_SBRTH_SHIFT;
	
	msr.hi &=  ~ (0x03 << GLPCI_CTRL_UPPER_WTO_SHIFT);
	msr.hi |=  0x06 << GLPCI_CTRL_UPPER_WTO_SHIFT;
	
	msr.hi &=  ~ (0x03 << GLPCI_CTRL_UPPER_ILTO_SHIFT);
	msr.hi |=  0x00 << GLPCI_CTRL_UPPER_ILTO_SHIFT;
	wrmsr(msrnum, msr);


	/*  Set GLPCI Latency Timer.*/
	msrnum = GLPCI_CTRL;
	msr = rdmsr(msrnum);
	msr.hi |=  0x1F << GLPCI_CTRL_UPPER_LAT_SHIFT; 	/*  Change once 1.x is gone.*/
	wrmsr(msrnum, msr);

	/*  GLPCI_SPARE*/
	msrnum = GLPCI_SPARE;
	msr = rdmsr(msrnum);
	msr.lo &=  ~ 0x7;
	msr.lo |=  GLPCI_SPARE_LOWER_AILTO_SET | GLPCI_SPARE_LOWER_PPD_SET | GLPCI_SPARE_LOWER_PPC_SET | GLPCI_SPARE_LOWER_MPC_SET | GLPCI_SPARE_LOWER_NSE_SET | GLPCI_SPARE_LOWER_SUPO_SET;
	wrmsr(msrnum, msr);

}



	/* ***************************************************************************/
	/* **/
	/* *	ClockGatingInit*/
	/* **/
	/* *	Enable Clock Gating.*/
	/* **/
	/* *	Entry:*/
	/* *	Exit:*/
	/* *	Modified:*/
	/* **/
	/* ***************************************************************************/
static void 
ClockGatingInit (void){
	msr_t msr;
	struct msrinit *gating = ClockGatingDefault;
	int i;

#if 0
	mov	cx, TOKEN_CLK_GATE
	NOSTACK	bx, GetNVRAMValueBX
	cmp	al, TVALUE_CG_OFF
	je	gatingdone
	
	cmp	al, TVALUE_CG_DEFAULT
	jb	allon
	ja	performance
	lea	si, ClockGatingDefault
	jmp	nextdevice

allon:
	lea	si, ClockGatingAllOn
	jmp	nextdevice

performance:
	lea	si, ClockGatingPerformance
#endif

	for(i = 0; gating->msrnum != 0xffffffff; i++) {
		msr = rdmsr(gating->msrnum);
		printk_debug("%s: MSR 0x%x is 0x%x:0x%x\n", __FUNCTION__, gating->msrnum, msr.hi, msr.lo);
		msr.hi |= gating->msr.hi;
		msr.lo |= gating->msr.lo;
		printk_debug("%s: MSR 0x%x will be set to  0x%x:0x%x\n", __FUNCTION__, 
			gating->msrnum, msr.hi, msr.lo);
		wrmsr(gating->msrnum, msr);
		gating +=1;
	}

}

static void 
GeodeLinkPriority(void){
	msr_t msr;
	struct msrinit *prio = GeodeLinkPriorityTable;
	int i;

	for(i = 0; prio->msrnum != 0xffffffff; i++) {
		msr = rdmsr(prio->msrnum);
		printk_debug("%s: MSR 0x%x is 0x%x:0x%x\n", __FUNCTION__, prio->msrnum, msr.hi, msr.lo);
		msr.hi |= prio->msr.hi;
		msr.lo &= ~0xfff;
		msr.lo |= prio->msr.lo;
		printk_debug("%s: MSR 0x%x will be set to  0x%x:0x%x\n", __FUNCTION__, 
			prio->msrnum, msr.hi, msr.lo);
		wrmsr(prio->msrnum, msr);
		prio +=1;
	}
}
	
	/* ***************************************************************************/
	/* **/
	/* *	northBridgeInit*/
	/* **/
	/* *	Core Logic initialization:  Host bridge*/
	/* **/
	/* *	Entry:*/
	/* *	Exit:*/
	/* *	Modified:*/
	/* **/
	/* ***************************************************************************/

void
northbridgeinit(void)
{
	int i;
	printk_debug("Enter %s\n", __FUNCTION__);

	for(i = 0; gliutables[i]; i++)
		GLIUInit(gliutables[i]);

	GeodeLinkPriority();


	/*  Now that the descriptor to memory is set up.*/
	/*  The memory controller needs one read to synch it's lines before it can be used.*/
	i = *(int *) 0;

	GLPCIInit();
	ClockGatingInit();
	__asm__("FINIT\n");
	/* CPUBugsFix -- called elsewhere */
	printk_debug("Exit %s\n", __FUNCTION__);
}

