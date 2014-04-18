/* Copyright 2000  AG Electronics Ltd. */
/* This code is distributed without warranty under the GPL v2 (see COPYING) */

#include <ppc.h>
#include <timer.h>
#include <clock.h>

unsigned long get_hz(void)
{
	return get_timer_freq();
}

unsigned long ticks_since_boot(void)
{
	extern unsigned long _get_ticks(void);
	return _get_ticks();
}

void udelay(int usecs)
{
	extern void _wait_ticks(unsigned long);
	unsigned long ticksperusec = get_hz() / 1000000;

	_wait_ticks(ticksperusec * usecs);
}
