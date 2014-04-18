/*
 * Copyright 2004 Tyan Computer
 *  by yhlu@tyan.com
 */

#include <device/smbus_def.h>

#define SMBHSTSTAT  0x1
#define SMBHSTPRTCL 0x0
#define SMBHSTCMD   0x3
#define SMBXMITADD  0x2
#define SMBHSTDAT0  0x4
#define SMBHSTDAT1  0x5

/*
 * Between 1-10 seconds, We should never timeout normally.
 * Longer than this is just painful when a timeout condition occurs.
 */
#define SMBUS_TIMEOUT (100 * 1000 * 10)

static inline void smbus_delay(void)
{
	outb(0x80, 0x80);
}

static int smbus_wait_until_ready(unsigned smbus_io_base)
{
	unsigned long loops;
	loops = SMBUS_TIMEOUT;
	do {
		unsigned char val;
		smbus_delay();
		val = inb(smbus_io_base + SMBHSTSTAT);
		val &= 0x1f;
		if (val == 0)
			return 0;
		outb(val, smbus_io_base + SMBHSTSTAT);
	} while (--loops);
	return -2;
}

static int smbus_wait_until_done(unsigned smbus_io_base)
{
	unsigned long loops;
	loops = SMBUS_TIMEOUT;
	do {
		unsigned char val;
		smbus_delay();
		val = inb(smbus_io_base + SMBHSTSTAT);
		if ((val & 0xff) != 0)
			return 0;
	} while (--loops);
	return -3;
}

static int do_smbus_recv_byte(unsigned smbus_io_base, unsigned device)
{
	unsigned char global_status_register, byte;

#if 0
	/* Not needed, upon write to PRTCL, the status will be auto-cleared. */
	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;
#endif

	/* Set the device I'm talking to. */
	outb(((device & 0x7f) << 1) | 1, smbus_io_base + SMBXMITADD);
	smbus_delay();

	/* Set the command/address. */
	outb(0, smbus_io_base + SMBHSTCMD);
	smbus_delay();

	/* Byte data recv */
	outb(0x05, smbus_io_base + SMBHSTPRTCL);
	smbus_delay();

	/* Poll for transaction completion. */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;

	/* Lose check */
	global_status_register = inb(smbus_io_base + SMBHSTSTAT) & 0x80;

	/* Read results of transaction. */
	byte = inb(smbus_io_base + SMBHSTDAT0);

	/* Lose check, otherwise it should be 0. */
	if (global_status_register != 0x80)
		return -1;

	return byte;
}

static int do_smbus_send_byte(unsigned smbus_io_base, unsigned device,
			      unsigned char val)
{
	unsigned global_status_register;

#if 0
	/* Not needed, upon write to PRTCL, the status will be auto-cleared. */
	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;
#endif

	outb(val, smbus_io_base + SMBHSTDAT0);
	smbus_delay();

	/* Set the device I'm talking to. */
	outb(((device & 0x7f) << 1) | 0, smbus_io_base + SMBXMITADD);
	smbus_delay();

	outb(0, smbus_io_base + SMBHSTCMD);
	smbus_delay();

	/* Set up for a byte data write. */
	outb(0x04, smbus_io_base + SMBHSTPRTCL);
	smbus_delay();

	/* Poll for transaction completion. */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;

	/* Lose check */
	global_status_register = inb(smbus_io_base + SMBHSTSTAT) & 0x80;

	if (global_status_register != 0x80)
		return -1;

	return 0;
}

static int do_smbus_read_byte(unsigned smbus_io_base, unsigned device,
			      unsigned address)
{
	unsigned char global_status_register, byte;

#if 0
	/* Not needed, upon write to PRTCL, the status will be auto-cleared. */
	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;
#endif

	/* Set the device I'm talking to. */
	outb(((device & 0x7f) << 1) | 1, smbus_io_base + SMBXMITADD);
	smbus_delay();

	/* Set the command/address. */
	outb(address & 0xff, smbus_io_base + SMBHSTCMD);
	smbus_delay();

	/* Byte data read */
	outb(0x07, smbus_io_base + SMBHSTPRTCL);
	smbus_delay();

	/* Poll for transaction completion. */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;

	/* Lose check */
	global_status_register = inb(smbus_io_base + SMBHSTSTAT) & 0x80;

	/* Read results of transaction. */
	byte = inb(smbus_io_base + SMBHSTDAT0);

	/* Lose check, otherwise it should be 0. */
	if (global_status_register != 0x80)
		return -1;

	return byte;
}

static int do_smbus_write_byte(unsigned smbus_io_base, unsigned device,
			       unsigned address, unsigned char val)
{
	unsigned global_status_register;

#if 0
	/* Not needed, upon write to PRTCL, the status will be auto-cleared. */
	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;
#endif

	outb(val, smbus_io_base + SMBHSTDAT0);
	smbus_delay();

	/* Set the device I'm talking to. */
	outb(((device & 0x7f) << 1) | 0, smbus_io_base + SMBXMITADD);
	smbus_delay();

	outb(address & 0xff, smbus_io_base + SMBHSTCMD);
	smbus_delay();

	/* Set up for a byte data write. */
	outb(0x06, smbus_io_base + SMBHSTPRTCL);
	smbus_delay();

	/* Poll for transaction completion. */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;

	/* Lose check */
	global_status_register = inb(smbus_io_base + SMBHSTSTAT) & 0x80;

	if (global_status_register != 0x80)
		return -1;

	return 0;
}
