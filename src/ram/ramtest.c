static void write_phys(unsigned long addr, unsigned long value)
{
	unsigned long *ptr;
	ptr = (void *)addr;
	*ptr = value;
}

static unsigned long read_phys(unsigned long addr)
{
	unsigned long *ptr;
	ptr = (void *)addr;
	return *ptr;
}

void ram_fill(unsigned long start, unsigned long stop)
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
		if ((addr & 0xffff) == 0) {
			print_debug_hex32(addr);
			print_debug("\r");
		}
		write_phys(addr, addr);
	};
	/* Display final address */
	print_debug_hex32(addr);
	print_debug("\r\nDRAM filled\r\n");
}

void ram_verify(unsigned long start, unsigned long stop)
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
		if ((addr & 0xffff) == 0) {
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


void ramcheck(unsigned long start, unsigned long stop)
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
	print_debug("Done.\n");
}

