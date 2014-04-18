#include <console/loglevel.h>

static void __console_tx_byte(unsigned char byte)
{
	uart_tx_byte(byte);
}

static void __console_tx_nibble(unsigned nibble)
{
	unsigned char digit;
	digit = nibble + '0';
	if (digit > '9') {
		digit += 39;
	}
	__console_tx_byte(digit);
}

static void __console_tx_char(int loglevel, unsigned char byte)
{
	if (ASM_CONSOLE_LOGLEVEL > loglevel) {
		uart_tx_byte(byte);
	}
}

static void __console_tx_hex8(int loglevel, unsigned char value)
{
	if (ASM_CONSOLE_LOGLEVEL > loglevel) {
		__console_tx_nibble((value >>  4U) & 0x0fU);
		__console_tx_nibble(value & 0x0fU);
	}
}

static void __console_tx_hex16(int loglevel, unsigned short value)
{
	if (ASM_CONSOLE_LOGLEVEL > loglevel) {
		__console_tx_nibble((value >> 12U) & 0x0fU);
		__console_tx_nibble((value >>  8U) & 0x0fU);
		__console_tx_nibble((value >>  4U) & 0x0fU);
		__console_tx_nibble(value & 0x0fU);
	}
}

static void __console_tx_hex32(int loglevel, unsigned int value)
{
	if (ASM_CONSOLE_LOGLEVEL > loglevel) {
		__console_tx_nibble((value >> 28U) & 0x0fU);
		__console_tx_nibble((value >> 24U) & 0x0fU);
		__console_tx_nibble((value >> 20U) & 0x0fU);
		__console_tx_nibble((value >> 16U) & 0x0fU);
		__console_tx_nibble((value >> 12U) & 0x0fU);
		__console_tx_nibble((value >>  8U) & 0x0fU);
		__console_tx_nibble((value >>  4U) & 0x0fU);
		__console_tx_nibble(value & 0x0fU);
	}
}

static void __console_tx_string(int loglevel, const char *str)
{
	if (ASM_CONSOLE_LOGLEVEL > loglevel) {
		unsigned char ch;
		while((ch = *str++) != '\0') {
			__console_tx_byte(ch);
		}
	}
}

static void print_emerg_char(unsigned char byte) { __console_tx_char(BIOS_EMERG, byte); }
static void print_emerg_hex8(unsigned char value){ __console_tx_hex8(BIOS_EMERG, value); }
static void print_emerg_hex16(unsigned short value){ __console_tx_hex16(BIOS_EMERG, value); }
static void print_emerg_hex32(unsigned int value) { __console_tx_hex32(BIOS_EMERG, value); }
static void print_emerg(const char *str) { __console_tx_string(BIOS_EMERG, str); }

static void print_alert_char(unsigned char byte) { __console_tx_char(BIOS_ALERT, byte); }
static void print_alert_hex8(unsigned char value) { __console_tx_hex8(BIOS_ALERT, value); }
static void print_alert_hex16(unsigned short value){ __console_tx_hex16(BIOS_ALERT, value); }
static void print_alert_hex32(unsigned int value) { __console_tx_hex32(BIOS_ALERT, value); }
static void print_alert(const char *str) { __console_tx_string(BIOS_ALERT, str); }

static void print_crit_char(unsigned char byte) { __console_tx_char(BIOS_CRIT, byte); }
static void print_crit_hex8(unsigned char value) { __console_tx_hex8(BIOS_CRIT, value); }
static void print_crit_hex16(unsigned short value){ __console_tx_hex16(BIOS_CRIT, value); }
static void print_crit_hex32(unsigned int value) { __console_tx_hex32(BIOS_CRIT, value); }
static void print_crit(const char *str) { __console_tx_string(BIOS_CRIT, str); }

static void print_err_char(unsigned char byte) { __console_tx_char(BIOS_ERR, byte); }
static void print_err_hex8(unsigned char value) { __console_tx_hex8(BIOS_ERR, value); }
static void print_err_hex16(unsigned short value){ __console_tx_hex16(BIOS_ERR, value); }
static void print_err_hex32(unsigned int value) { __console_tx_hex32(BIOS_ERR, value); }
static void print_err(const char *str) { __console_tx_string(BIOS_ERR, str); }

static void print_warning_char(unsigned char byte) { __console_tx_char(BIOS_WARNING, byte); }
static void print_warning_hex8(unsigned char value) { __console_tx_hex8(BIOS_WARNING, value); }
static void print_warning_hex16(unsigned short value){ __console_tx_hex16(BIOS_WARNING, value); }
static void print_warning_hex32(unsigned int value) { __console_tx_hex32(BIOS_WARNING, value); }
static void print_warning(const char *str) { __console_tx_string(BIOS_WARNING, str); }

static void print_notice_char(unsigned char byte) { __console_tx_char(BIOS_NOTICE, byte); }
static void print_notice_hex8(unsigned char value) { __console_tx_hex8(BIOS_NOTICE, value); }
static void print_notice_hex16(unsigned short value){ __console_tx_hex16(BIOS_NOTICE, value); }
static void print_notice_hex32(unsigned int value) { __console_tx_hex32(BIOS_NOTICE, value); }
static void print_notice(const char *str) { __console_tx_string(BIOS_NOTICE, str); }

static void print_info_char(unsigned char byte) { __console_tx_char(BIOS_INFO, byte); }
static void print_info_hex8(unsigned char value) { __console_tx_hex8(BIOS_INFO, value); }
static void print_info_hex16(unsigned short value){ __console_tx_hex16(BIOS_INFO, value); }
static void print_info_hex32(unsigned int value) { __console_tx_hex32(BIOS_INFO, value); }
static void print_info(const char *str) { __console_tx_string(BIOS_INFO, str); }

static void print_debug_char(unsigned char byte) { __console_tx_char(BIOS_DEBUG, byte); }
static void print_debug_hex8(unsigned char value) { __console_tx_hex8(BIOS_DEBUG, value); }
static void print_debug_hex16(unsigned short value){ __console_tx_hex16(BIOS_DEBUG, value); }
static void print_debug_hex32(unsigned int value) { __console_tx_hex32(BIOS_DEBUG, value); }
static void print_debug(const char *str) { __console_tx_string(BIOS_DEBUG, str); }

static void print_spew_char(unsigned char byte) { __console_tx_char(BIOS_SPEW, byte); }
static void print_spew_hex8(unsigned char value) { __console_tx_hex8(BIOS_SPEW, value); }
static void print_spew_hex16(unsigned short value){ __console_tx_hex16(BIOS_SPEW, value); }
static void print_spew_hex32(unsigned int value) { __console_tx_hex32(BIOS_SPEW, value); }
static void print_spew(const char *str) { __console_tx_string(BIOS_SPEW, str); }

#ifndef LINUXBIOS_EXTRA_VERSION
#define LINUXBIOS_EXTRA_VERSION ""
#endif

static void console_init(void)
{
	static const char console_test[] = 
		"\r\n\r\nLinuxBIOS-"
		LINUXBIOS_VERSION
		LINUXBIOS_EXTRA_VERSION
		" "
		LINUXBIOS_BUILD
		" starting...\r\n";
	print_info(console_test);
}


static void die(const char *str)
{
	print_emerg(str);
	do {
		hlt();
	} while(1);
}
