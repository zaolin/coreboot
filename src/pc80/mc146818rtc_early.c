#include <pc80/mc146818rtc.h>
#include <part/fallback_boot.h>

#ifndef MAX_REBOOT_CNT
#error "MAX_REBOOT_CNT not defined"
#endif
#if  MAX_REBOOT_CNT > 15
#error "MAX_REBOOT_CNT too high"
#endif

static unsigned char cmos_read(unsigned char addr)
{
	outb(addr, RTC_BASE_PORT + 0);
	return inb(RTC_BASE_PORT + 1);
}

static void cmos_write(unsigned char val, unsigned char addr)
{
	outb(addr, RTC_BASE_PORT + 0);
	outb(val, RTC_BASE_PORT + 1);
}

static int cmos_error(void)
{
	unsigned char reg_d;
	/* See if the cmos error condition has been flagged */
	reg_d = cmos_read(RTC_REG_D);
	return (reg_d & RTC_VRT) == 0;
}

static int cmos_chksum_valid(void)
{
	unsigned char addr;
	unsigned long sum, old_sum;
	sum = 0;
	/* Comput the cmos checksum */
	for(addr = LB_CKS_RANGE_START; addr <= LB_CKS_RANGE_END; addr++) {
		sum += cmos_read(addr);
	}
	sum = (sum & 0xffff) ^ 0xffff;

	/* Read the stored checksum */
	old_sum = cmos_read(LB_CKS_LOC) << 8;
	old_sum |=  cmos_read(LB_CKS_LOC+1);

	return sum == old_sum;
}


static int last_boot_normal(void)
{
	unsigned char byte;
	byte = cmos_read(RTC_BOOT_BYTE);
	return (byte & (1 << 1));
}

static int do_normal_boot(void)
{
	unsigned char byte;

	if (cmos_error() || !cmos_chksum_valid()) {
		unsigned char byte;
		/* There are no impossible values, no cheksums so just
		 * trust whatever value we have in the the cmos,
		 * but clear the fallback bit.
		 */
		byte = cmos_read(RTC_BOOT_BYTE);
		byte &= 0x0c;
		byte |= MAX_REBOOT_CNT << 4;
		cmos_write(byte, RTC_BOOT_BYTE);
	}

	/* The RTC_BOOT_BYTE is now o.k. see where to go. */
	byte = cmos_read(RTC_BOOT_BYTE);
	
	/* Are we in normal mode? */
	if (byte & 1) {
		byte &= 0x0f; /* yes, clear the boot count */
	}

	/* Properly set the last boot flag */
	byte &= 0xfc;
	if ((byte >> 4) < MAX_REBOOT_CNT) {
		byte |= (1<<1);
	}

	/* Are we already at the max count? */
	if ((byte >> 4) < MAX_REBOOT_CNT) {
		byte += 1 << 4; /* No, add 1 to the count */
	}
	else {
		byte &= 0xfc;	/* Yes, put in fallback mode */
	}

	/* Save the boot byte */
	cmos_write(byte, RTC_BOOT_BYTE);

	return (byte & (1<<1));
}
