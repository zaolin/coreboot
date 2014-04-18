#include <cpu/x86/tsc.h>

#define CLOCK_TICK_RATE	1193180U /* Underlying HZ */
#define CALIBRATE_INTERVAL ((20*CLOCK_TICK_RATE)/1000) /* 20ms */
#define CALIBRATE_DIVISOR  (20*1000) /* 20ms / 20000 == 1usec */

static unsigned int calibrate_tsc(void)
{
	/* Set the Gate high, disable speaker */
	outb((inb(0x61) & ~0x02) | 0x01, 0x61);

	/*
	 * Now let's take care of CTC channel 2
	 *
	 * Set the Gate high, program CTC channel 2 for mode 0,
	 * (interrupt on terminal count mode), binary count,
	 * load 5 * LATCH count, (LSB and MSB) to begin countdown.
	 */
	outb(0xb0, 0x43);			/* binary, mode 0, LSB/MSB, Ch 2 */
	outb(CALIBRATE_INTERVAL	& 0xff, 0x42);	/* LSB of count */
	outb(CALIBRATE_INTERVAL	>> 8, 0x42);	/* MSB of count */

	{
		tsc_t start;
		tsc_t end;
		unsigned long count;

		start = rdtsc();
		count = 0;
		do {
			count++;
		} while ((inb(0x61) & 0x20) == 0);
		end = rdtsc();

		/* Error: ECTCNEVERSET */
		if (count <= 1)
			goto bad_ctc;

		/* 64-bit subtract - gcc just messes up with long longs */
		__asm__("subl %2,%0\n\t"
			"sbbl %3,%1"
			:"=a" (end.lo), "=d" (end.hi)
			:"g" (start.lo), "g" (start.hi),
			 "0" (end.lo), "1" (end.hi));

		/* Error: ECPUTOOFAST */
		if (end.hi)
			goto bad_ctc;


		/* Error: ECPUTOOSLOW */
		if (end.lo <= CALIBRATE_DIVISOR)
			goto bad_ctc;

		return (end.lo + CALIBRATE_DIVISOR -1)/CALIBRATE_DIVISOR;
	}

	/*
	 * The CTC wasn't reliable: we got a hit on the very first read,
	 * or the CPU was so fast/slow that the quotient wouldn't fit in
	 * 32 bits..
	 */
bad_ctc:
	print_err("bad_ctc\n");
	return 0;
}

/* spll_raw_clk = SYSREF * FbDIV,
 * GLIU Clock   = spll_raw_clk / MDIV
 * CPU Clock    = sppl_raw_clk / VDIV
 */

/* table for Feedback divisor to FbDiv register value */
static const unsigned char plldiv2fbdiv[] = {
	 0,  0,  0,  0,  0,  0, 15,  7,  3,  1,  0, 32, 16, 40, 20, 42, /* pll div  0 - 15 */
	21, 10, 37, 50, 25, 12, 38, 19,  9,  4, 34, 17,  8, 36, 18, 41, /* pll div 16 - 31 */
	52, 26, 45, 54, 27, 13,  6, 35, 49, 56, 28, 46, 23, 11, 05, 02, /* pll div 32 - 47 */
	33, 48, 24, 44, 22, 43, 53, 58, 29, 14, 39, 51, 57, 60, 30, 47, /* pll div 48 - 63 */
};

/* table for FbDiv register value to Feedback divisor */
static const unsigned char fbdiv2plldiv[] = {
	10,  9, 47,  8, 25, 46, 38,  7, 28, 24, 17, 45, 21, 37, 57,  6,
	12, 27, 30, 23, 14, 16, 52, 44, 50, 20, 33, 36, 42, 56,  0,  0,
	11, 48, 26, 39, 29, 18, 22, 58, 13, 31, 15, 53, 51, 34, 43,  0,
	49, 40, 19, 59, 32, 54, 35,  0, 41, 60, 55,  0, 61,  0,  0,  0
};

static const unsigned char pci33_sdr_crt [] = {
	/* FbDIV, VDIV, MDIV		   CPU/GeodeLink */
	      12,    2,    4,		// 200/100
	      16,    2,    4,		// 266/133
              18,    2,    5,		// 300/120
              20,    2,    5,		// 333/133
              22,    2,    6,		// 366/122
              24,    2,    6,		// 400/133
              26,    2,    6            // 433/144
};

static const unsigned char pci33_ddr_crt [] = {
	/* FbDIV, VDIV, MDIV		   CPU/GeodeLink */
	     12,    2,    3,		// 200/133
	     16,    2,    3,		// 266/177
             18,    2,    3,		// 300/200
             20,    2,    3,		// 333/222
             22,    2,    3,		// 366/244
             24,    2,    3,		// 400/266
             26,    2,    3             // 433/289
};

static unsigned int get_memory_speed(void)
{
	unsigned char val, hi, lo;

	val = spd_read_byte(0xA0, 9);
	hi = (val >> 4) & 0x0f;
	lo = val & 0x0f;

	return 20000/(hi*10 + lo);
}

static void pll_reset(void)
{
	msr_t msr;
	unsigned int sysref, spll_raw, cpu_core, gliu;
	unsigned mdiv, vdiv, fbdiv;

	/* get CPU core clock in MHZ */
	cpu_core = calibrate_tsc();
	print_debug("Cpu core is ");
	print_debug_hex32(cpu_core);
	print_debug("\n");

	msr = rdmsr(GLCP_SYS_RSTPLL);
	if (msr.lo & (1 << GLCP_SYS_RSTPLL_BYPASS)) {
#if 0
		msr.hi = PLLMSRhi;
		msr.lo = PLLMSRlo;
		wrmsr(GLCP_SYS_RSTPLL, msr);
		msr.lo |= PLLMSRlo1;
		wrmsr(GLCP_SYS_RSTPLL, msr);

		print_debug("Reset PLL\n\r");

		msr.lo |= PLLMSRlo2;
		wrmsr(GLCP_SYS_RSTPLL,msr);
		print_debug("should not be here\n\r");
#endif
		print_err("shit");
		while (1)
			;
	}

	if (msr.lo & GLCP_SYS_RSTPLL_SWFLAGS_MASK) {
		/* PLL is already set and we are reboot from PLL reset */
		print_debug("reboot from BIOS reset\n\r");
		return;
	}

	/* get the sysref clock rate */
	vdiv  = (msr.hi >> GLCP_SYS_RSTPLL_VDIV_SHIFT) & 0x07;
	vdiv += 2;
	fbdiv = (msr.hi >> GLCP_SYS_RSTPLL_FBDIV_SHIFT) & 0x3f;
	fbdiv = fbdiv2plldiv[fbdiv];
	spll_raw = cpu_core * vdiv;
	sysref   = spll_raw / fbdiv;

	/* get target memory rate by SPD */
	//gliu = get_memory_speed();

	msr.hi = 0x00000019;
	msr.lo = 0x06de0378;
	wrmsr(0x4c000014, msr);
	msr.lo |= ((0xde << 16) | (1 << 26) | (1 << 24));
	wrmsr(0x4c000014, msr);

	print_debug("Reset PLL\n\r");

	msr.lo |= ((1<<14) |(1<<13) | (1<<0));
	wrmsr(0x4c000014,msr);

	print_debug("should not be here\n\r");
}
