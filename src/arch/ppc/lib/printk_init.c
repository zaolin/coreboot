/*
 *  blantantly copied from linux/kernel/printk.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 */
#include <stdarg.h>
#include <console/loglevel.h>

/* printk's without a loglevel use this.. */
#define DEFAULT_MESSAGE_LOGLEVEL 4 /* BIOS_WARNING */

/* Keep together for sysctl support */

int console_loglevel = DEFAULT_CONSOLE_LOGLEVEL;

extern int vtxprintf(void (*)(unsigned char), const char *, va_list);
extern void uart8250_tx_byte(unsigned, unsigned char);

void console_tx_byte(unsigned char byte)
{
	if (byte == '\n')
		uart8250_tx_byte(UART0_IO_BASE, '\r');
	uart8250_tx_byte(UART0_IO_BASE, byte);
}

int do_printk(int msg_level, const char *fmt, ...)
{
	va_list args;
	int i;

	if (msg_level >= console_loglevel) {
		return 0;
	}

	va_start(args, fmt);
	i = vtxprintf(console_tx_byte, fmt, args);
	va_end(args);

	return i;
}
