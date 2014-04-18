/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 coresystems GmbH
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2003 Ollie Lo <ollielo@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */


#include <console/console.h>
#include <pc80/keyboard.h>
#include <device/device.h>
#include <arch/io.h>
#include <delay.h>

#define KBD_DATA	0x60
#define KBD_COMMAND	0x64
#define KBD_STATUS	0x64
#define   KBD_IBF	(1 << 1) // 1: input buffer full (data ready for ec)
#define   KBD_OBF	(1 << 0) // 1: output buffer full (data ready for host)

// Keyboard Controller Commands
#define KBC_CMD_READ_COMMAND	0x20 // Read command byte
#define KBC_CMD_WRITE_COMMAND	0x60 // Write command byte
#define KBC_CMD_SELF_TEST	0xAA // Controller self-test

/* The Keyboard controller command byte
 *  BIT	| Description
 *  ----+-------------------------------------------------------
 *   7  | reserved, must be zero
 *   6  | XT Translation, (1 = on, 0 = off)
 *   5  | Disable Mouse Port (1 = disable, 0 = enable)
 *   4  | Disable Keyboard Port (1 = disable, 0 = enable)
 *   3  | reserved, must be zero
 *   2  | System Flag (1 = self-test passed. DO NOT SET TO ZERO)
 *   1  | Mouse Port Interrupts (1 = enable, 0 = disable)
 *   0  | Keyboard Port Interrupts (1 = enable, 0 = disable)
 */

// Keyboard Controller Replies
#define KBC_REPLY_SELFTEST_OK	0x55 // controller self-test succeeded

//
// Keyboard Replies
//
#define KBD_REPLY_POR		0xAA    // Power on reset
#define KBD_REPLY_ACK		0xFA    // Command ACK
#define KBD_REPLY_RESEND	0xFE    // Command NACK, send command again

/* Wait 200ms for keyboard controller answers */
#define KBC_TIMEOUT_IN_MS 200

static int kbc_input_buffer_empty(void)
{
	u32 timeout;
	for(timeout = KBC_TIMEOUT_IN_MS; timeout && (inb(KBD_STATUS) & KBD_IBF); timeout--) {
		mdelay(1);
	}

	if (!timeout) {
		printk_warning("Unexpected Keyboard controller input buffer full\n");
	}
	return !!timeout;
}


static int kbc_output_buffer_full(void)
{
	u32 timeout;
	for(timeout = KBC_TIMEOUT_IN_MS; timeout && ((inb(KBD_STATUS) & KBD_OBF) == 0); timeout--) {
		mdelay(1);
	}

	if (!timeout) {
		printk_warning("Keyboard controller output buffer result timeout\n");
	}
	return !!timeout;
}


static int kbc_cleanup_buffers(void)
{
	u32 timeout;
	for(timeout = KBC_TIMEOUT_IN_MS; timeout && (inb(KBD_STATUS) & (KBD_OBF | KBD_IBF)); timeout--) {
		mdelay(1);
		inb(KBD_DATA);
	}

	if (!timeout) {
		printk_err("Couldn't cleanup the keyboard controller buffers\n");
		printk_err("Status (0x%x): 0x%x, Buffer (0x%x): 0x%x\n",
				KBD_STATUS, inb(KBD_STATUS), KBD_DATA, inb(KBD_DATA));
	}

	return !!timeout;
}

static int kbc_self_test(void)
{
	u8 self_test;

	/* Clean up any junk that might have been in the KBC.
	 * Both input and output buffers must be empty.
	 */
	if (!kbc_cleanup_buffers())
		return 0;

	/* reset/self test 8042 - send cmd 0xAA */
	outb(KBC_CMD_SELF_TEST, KBD_COMMAND);

	if (!kbc_output_buffer_full()) {
		/* There probably is no keyboard controller. */
		printk_err("Could not reset keyboard controller.\n");
		return 0;
	}

	/* read self-test result, 0x55 is returned in the output buffer */
	self_test = inb(KBD_DATA);

	if (self_test != 0x55) {
		printk_err("Keyboard Controller self-test failed: 0x%x\n",
				self_test);
		return 0;
	}

	return 1;
}

static u8 send_keyboard(u8 command)
{
	u8 regval = 0;
	u8 resend = 10;

	do {
		if (!kbc_input_buffer_empty()) return 0;
		outb(command, KBD_DATA);
		if (!kbc_output_buffer_full()) {
			printk_err("Could not send keyboard command %02x\n",
					command);
			return 0;
		}
		regval = inb(KBD_DATA);
		--resend;
	} while (regval == 0xFE && resend > 0);

	return regval;
}

static void pc_keyboard_init(struct pc_keyboard *keyboard)
{
	u8 regval;
	printk_debug("Keyboard init...\n");

	/* Run a keyboard controller self-test */
	if (!kbc_self_test())
		return;

	/* Enable keyboard interface - No IRQ */
	if (!kbc_input_buffer_empty()) return;
	outb(0x60, KBD_COMMAND);
	if (!kbc_input_buffer_empty()) return;
	outb(0x20, KBD_DATA);	/* send cmd: enable keyboard */
	if (!kbc_input_buffer_empty()) {
		printk_info("Timeout while enabling keyboard\n");
		return;
	}

	/* clean up any junk that might have been in the keyboard */
	if (!kbc_cleanup_buffers()) return;

	/* reset keyboard and self test (keyboard side) */
	regval = send_keyboard(0xFF);
	if (regval != 0xFA) {
		printk_err("Keyboard selftest failed ACK: 0x%x\n", regval);
		return;
	}

	if (!kbc_output_buffer_full()) {
		printk_err("Timeout waiting for keyboard after reset.\n");
		return;
	}

	regval = inb(KBD_DATA);
	if (regval != 0xAA) {
		printk_err("Keyboard selftest failed: 0x%x\n", regval);
		return;
	}

	/*
	 * The following set scancode stuff is what normal BIOS do. It could be
	 * argued that coreboot shouldn't set the scan code.....
	 */

	/* disable the keyboard */
	regval = send_keyboard(0xF5);
	if (regval != 0xFA) {
		printk_err("Keyboard disable failed ACK: 0x%x\n", regval);
		return;
	}

	/* Set scancode command */
	regval = send_keyboard(0xF0);
	if (regval != 0xFA) {
		printk_err("Keyboard set scancode cmd failed ACK: 0x%x\n", regval);
		return;
	}
	/* Set scancode mode 2 */
	regval = send_keyboard(0x02);
	if (regval != 0xFA) {
		printk_err("Keyboard set scancode mode failed ACK: 0x%x\n", regval);
		return;
	}

	/* enable the keyboard */
	regval = send_keyboard(0xF4);
	if (regval != 0xFA) {
		printk_err("Keyboard enable failed ACK: 0x%x\n", regval);
		return;
	}

	/* All is well - enable keyboard interface */
	if (!kbc_input_buffer_empty()) return;
	outb(0x60, KBD_COMMAND);
	if (!kbc_input_buffer_empty()) return;
	outb(0x61, KBD_DATA);	/* send cmd: enable keyboard and IRQ 1 */
	if (kbc_output_buffer_full()) {
		printk_err("Timeout during final keyboard enable\n");
		return;
	}
}


void init_pc_keyboard(unsigned port0, unsigned port1, struct pc_keyboard *kbd)
{
	if ((port0 == 0x60) && (port1 == 0x64)) {
		pc_keyboard_init(kbd);
	} else {
		printk_warning("Unsupported keyboard controller.\n");
	}
}

/*
 * Support PS/2 mode -  oddball SIOs(KBC) need this setup
 * Not well documented. Google - 0xcb keyboard controller
 * This is called before pc_keyboard_init().
 */
void set_kbc_ps2_mode(void)
{
	/* Run a keyboard controller self-test */
	if (!kbc_self_test())
		return;

	/* Support PS/2 mode */
	if (!kbc_input_buffer_empty()) return;
	outb(0xcb, KBD_COMMAND);

	if (!kbc_input_buffer_empty()) return;
	outb(0x01, KBD_DATA);

	kbc_cleanup_buffers();
}
