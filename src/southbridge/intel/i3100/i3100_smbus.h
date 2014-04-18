/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Arastra, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/* This code is based on src/southbridge/intel/esb6300/esb6300_smbus.h */

#include <device/smbus_def.h>

#define SMBHSTSTAT 0x0
#define SMBHSTCTL  0x2
#define SMBHSTCMD  0x3
#define SMBXMITADD 0x4
#define SMBHSTDAT0 0x5
#define SMBHSTDAT1 0x6
#define SMBBLKDAT  0x7
#define SMBTRNSADD 0x9
#define SMBSLVDATA 0xa
#define SMLINK_PIN_CTL 0xe
#define SMBUS_PIN_CTL  0xf
#define SMBSLVCMD  0x11

#define SMBUS_TIMEOUT (100*1000*10)

static void smbus_delay(void)
{
	outb(0x80, 0x80);
}

static int smbus_wait_until_ready(u32 smbus_io_base)
{
	u32 loops = SMBUS_TIMEOUT;
	u8 byte;
	do {
		smbus_delay();
		if (--loops == 0)
			break;
		byte = inb(smbus_io_base + SMBHSTSTAT);
	} while (byte & 1);
	return loops ? 0 : -1;
}

static int smbus_wait_until_done(u32 smbus_io_base)
{
	u32 loops = SMBUS_TIMEOUT;
	u8 byte;
	do {
		smbus_delay();
		if (--loops == 0)
			break;
		byte = inb(smbus_io_base + SMBHSTSTAT);
	} while ((byte & 1) || (byte & ~((1 << 6)|(1 << 0))) == 0);
	return loops ? 0 : -1;
}

static int do_smbus_read_byte(u32 smbus_io_base, u16 device, u8 address)
{
	u8 global_status_register;
	u8 byte;

	if (smbus_wait_until_ready(smbus_io_base) < 0) {
		return SMBUS_WAIT_UNTIL_READY_TIMEOUT;
	}
	/* setup transaction */
	/* disable interrupts */
	outb(inb(smbus_io_base + SMBHSTCTL) & (~1), smbus_io_base + SMBHSTCTL);
	/* set the device I'm talking too */
	outb(((device & 0x7f) << 1) | 1, smbus_io_base + SMBXMITADD);
	/* set the command/address... */
	outb(address & 0xFF, smbus_io_base + SMBHSTCMD);
	/* set up for a byte data read */
	outb((inb(smbus_io_base + SMBHSTCTL) & 0xE3) | (0x2 << 2), smbus_io_base + SMBHSTCTL);
	/* clear any lingering errors, so the transaction will run */
	outb(inb(smbus_io_base + SMBHSTSTAT), smbus_io_base + SMBHSTSTAT);

	/* clear the data byte...*/
	outb(0, smbus_io_base + SMBHSTDAT0);

	/* start the command */
	outb((inb(smbus_io_base + SMBHSTCTL) | 0x40), smbus_io_base + SMBHSTCTL);

	/* poll for transaction completion */
	if (smbus_wait_until_done(smbus_io_base) < 0) {
		return SMBUS_WAIT_UNTIL_DONE_TIMEOUT;
	}

	global_status_register = inb(smbus_io_base + SMBHSTSTAT);

	/* Ignore the In Use Status... */
	global_status_register &= ~(3 << 5);

	/* read results of transaction */
	byte = inb(smbus_io_base + SMBHSTDAT0);
	if (global_status_register != (1 << 1)) {
		return SMBUS_ERROR;
	}
	return byte;
}
