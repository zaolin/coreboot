#if defined(i786)
#define HAVE_MOVNTI 1
#endif
#if defined(k8)
#define HAVE_MOVNTI 1
#endif

static void write_phys(unsigned long addr, unsigned long value)
{
#if HAVE_MOVNTI
	asm volatile(
		"movnti %1, (%0)"
		: /* outputs */
		: "r" (addr), "r" (value) /* inputs */
#ifndef __GNUC__
		: /* clobbers */
#endif
		);
#else
	volatile unsigned long *ptr;
	ptr = (void *)addr;
	*ptr = value;
#endif
}

static unsigned long read_phys(unsigned long addr)
{
	volatile unsigned long *ptr;
	ptr = (void *)addr;
	return *ptr;
}

static void ram_fill(unsigned long start, unsigned long stop)
{
	unsigned long addr;
	/* 
	 * Fill.
	 */
	print_debug("DRAM fill: ");
	print_debug_hex32(start);
	print_debug("-");
	print_debug_hex32(stop);
	print_debug("\r\n");
	for(addr = start; addr < stop ; addr += 4) {
		/* Display address being filled */
		if (!(addr & 0xffff)) {
			print_debug_hex32(addr);
			print_debug("\r");
		}
		write_phys(addr, addr);
	};
	/* Display final address */
	print_debug_hex32(addr);
	print_debug("\r\nDRAM filled\r\n");
}

static void ram_verify(unsigned long start, unsigned long stop)
{
	unsigned long addr;
	/* 
	 * Verify.
	 */
	print_debug("DRAM verify: ");
	print_debug_hex32(start);
	print_debug_char('-');
	print_debug_hex32(stop);
	print_debug("\r\n");
	for(addr = start; addr < stop ; addr += 4) {
		unsigned long value;
		/* Display address being tested */
		if (!(addr & 0xffff)) {
			print_debug_hex32(addr);
			print_debug("\r");
		}
		value = read_phys(addr);
		if (value != addr) {
			/* Display address with error */
			print_err_hex32(addr);
			print_err_char(':');
			print_err_hex32(value);
			print_err("\r\n");
		}
	}
	/* Display final address */
	print_debug_hex32(addr);
	print_debug("\r\nDRAM verified\r\n");
}


void ram_check(unsigned long start, unsigned long stop)
{
	int result;
	/*
	 * This is much more of a "Is my DRAM properly configured?"
	 * test than a "Is my DRAM faulty?" test.  Not all bits
	 * are tested.   -Tyson
	 */
	print_debug("Testing DRAM : ");
	print_debug_hex32(start);
	print_debug("-");	
	print_debug_hex32(stop);
	print_debug("\r\n");
	ram_fill(start, stop);
	ram_verify(start, stop);
	print_debug("Done.\r\n");
}

