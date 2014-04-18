#include <cpu/amd/gx2def.h>

static void sdram_set_registers(const struct mem_controller *ctrl)
{
}

static void sdram_set_spd_registers(const struct mem_controller *ctrl) 
{
	
}

/* Section 6.1.3, LX processor databooks, BIOS Initialization Sequence
 * Section 4.1.4, GX/CS5535 GeodeROM Porting guide */
static void sdram_enable(int controllers, const struct mem_controller *ctrl)
{
	int i;
	msr_t msr;

	/* 1. Initialize GLMC registers base on SPD values,
	 * Hard coded as XpressROM for now */
	print_debug("sdram_enable step 1\r\n");
	msr = rdmsr(0x20000018);
	msr.hi = 0x10076013;
	msr.lo = 0x00004800;
	wrmsr(0x20000018, msr);

	msr = rdmsr(0x20000019);
	msr.hi = 0x18000108;
	msr.lo = 0x286332a3;
	wrmsr(0x20000019, msr);

	/* 2. release from PMode */
	msr = rdmsr(0x20002004);
	msr.lo &= !0x04;
	msr.lo |= 0x01;
	wrmsr(0x20002004, msr);
	/* undocmented bits in GX, in LX there are
	 * 8 bits in PM1_UP_DLY */
	msr = rdmsr(0x2000001a);
	//msr.lo |= 0xF000;
	msr.lo = 0x0101;
	wrmsr(0x2000001a, msr);
	print_debug("sdram_enable step 2\r\n");

	/* 3. release CKE mask to enable CKE */
	msr = rdmsr(0x2000001d);
	msr.lo &= !(0x03 << 8);
	wrmsr(0x2000201d, msr);
	print_debug("sdram_enable step 3\r\n");

	/* 4. set and clear REF_TST 16 times, more shouldn't hurt */
	for (i = 0; i < 19; i++) {
		msr = rdmsr(0x20000018);
		msr.lo |=  (0x01 << 3);
		wrmsr(0x20000018, msr);
		msr.lo &= !(0x01 << 3);
		wrmsr(0x20000018, msr);
	}
	print_debug("sdram_enable step 4\r\n");

	/* 5. set refresh interval */
	msr = rdmsr(0x20000018);
	msr.lo |= (0x48 << 8);
	wrmsr(0x20000018, msr);

	/* set refresh staggering to 4 SDRAM clocks */
	msr = rdmsr(0x20000018);
	msr.lo &= !(0x03 << 6);
	wrmsr(0x20000018, msr);


	/* 6. enable RLL, load Extended Mode Register by set and clear PROG_DRAM */
	msr = rdmsr(0x20000018);
	msr.lo |=  ((0x01 << 28) | 0x01);
	wrmsr(0x20000018, msr);
	msr.lo &= !((0x01 << 28) | 0x01);
	wrmsr(0x20000018, msr);
	print_debug("sdram_enable step 7\r\n");

	/* 7. Reset DLL, Bit 27 is undocumented in GX datasheet,
	 * it is documented in LX datasheet  */	
	/* load Mode Register by set and clear PROG_DRAM */
	msr = rdmsr(0x20000018);
	msr.lo |=  ((0x01 << 27) | 0x01);
	wrmsr(0x20000018, msr);
	msr.lo &= !((0x01 << 27) | 0x01);
	wrmsr(0x20000018, msr);
	print_debug("sdram_enable step 9\r\n");


	/* 8. load Mode Register by set and clear PROG_DRAM */
	msr = rdmsr(0x20000018);
	msr.lo |=  0x01;
	wrmsr(0x20000018, msr);
	msr.lo &= !0x01;
	wrmsr(0x20000018, msr);
	print_debug("sdram_enable step 10\r\n");

	/* wait 200 SDCLKs */
	for (i = 0; i < 200; i++)
		outb(0xaa, 0x80);

	/* load RDSYNC */
	msr = rdmsr(0x2000001a);
	msr.hi = 0x000ff310;
	wrmsr(0x20000018, msr);
	print_debug("sdram_enable step 10\r\n");

	/* DRAM working now?? */

}
