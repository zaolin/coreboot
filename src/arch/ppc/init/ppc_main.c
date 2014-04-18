/* 
 * Copyright (C) 2003 by Greg Watson, Los Alamos National Laboratory
 * gwatson@lanl.gov
 */

extern unsigned _iseg[];
extern unsigned _liseg[];
extern unsigned _eliseg[];

void (*hardwaremain)(int) = _iseg;

/*
 * At this point we're running out of flash with our
 * stack in cache ram. We need to do the following:
 *
 * - turn on real memory
 * - relocate our payload into real memory
 * - start executing payload
 */

void ppc_main(void)
{
	unsigned *from;
	unsigned *to;

	sdram_init();

	/*
	 * Relocate payload (text & data) if necessary
	 */
	if (_liseg != _iseg) {	
		from = _liseg;
		to = _iseg;
		while (from < _eliseg)
			*to++ = *from++;
	}

	hardwaremain(0);
}
