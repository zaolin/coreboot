#include <console/loglevel.h>

#if CONFIG_USE_PRINTK_IN_CAR == 0
#include "console_print.c"
#else  /* CONFIG_USE_PRINTK_IN_CAR == 1 */
#include "console_printk.c"
#endif /* CONFIG_USE_PRINTK_IN_CAR */

#ifndef COREBOOT_EXTRA_VERSION
#define COREBOOT_EXTRA_VERSION ""
#endif

void console_init(void)
{
	static const char console_test[] = 
		"\r\n\r\ncoreboot-"
		COREBOOT_VERSION
		COREBOOT_EXTRA_VERSION
		" "
		COREBOOT_BUILD
		" starting...\r\n";
	print_info(console_test);
}


void die(const char *str)
{
	print_emerg(str);
	do {
		hlt();
	} while(1);
}
