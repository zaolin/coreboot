/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Carl-Daniel Hailfinger
 * Copyright (C) 2007 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "superiotool.h"

#define CHIP_ID_BYTE1_REG	0x20
#define CHIP_ID_BYTE2_REG	0x21
#define CHIP_VERSION_REG	0x22

/* Note: IT8726F has ID 0x8726 (datasheet wrongly says 0x8716). */
const static struct superio_registers reg_table[] = {
	{0x8702, "IT8702F", {
		{EOT}}},
	{0x8705, "IT8705F or IT8700F", {
		{EOT}}},
	{0x8708, "IT8708F", {
		{NOLDN, NULL,
			{0x07,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
			 0x29,0x2a,0x2e,0x2f,EOT},
			{NANA,0x87,0x08,0x00,0x00,NANA,0x3f,0x00,0xff,0xff,
			 0xff,0xff,0x00,0x00,EOT}},
		{0x0, "Floppy",
			{0x30,0x60,0x61,0x70,0x74,0xf0,0xf1,EOT},
			{0x00,0x03,0xf0,0x06,0x02,0x00,0x00,EOT}},
		{0x1, "COM1",
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0xf8,0x04,0x00,EOT}},
		{0x2, "COM2",
			{0x30,0x60,0x61,0x70,0xf0,0xf1,0xf2,0xf3,EOT},
			{0x00,0x02,0xf8,0x03,0x00,0x50,0x00,0x7f,EOT}},
		{0x3, "Parallel port",
			{0x30,0x60,0x61,0x62,0x63,0x64,0x65,0x70,0x74,
			 0xf0,EOT},
			{0x00,0x03,0x78,0x07,0x78,0x00,0x80,0x07,0x03,
			 0x03,EOT}},
		{0x4, "SWC",
			{0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
			 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,EOT},
			{NANA,NANA,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,NANA,NANA,EOT}},
		{0x5, "Keyboard",
			/* Note: 0x30 can actually be 0x00 _or_ 0x01. */
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0xf0,EOT},
			{0x01,0x00,0x60,0x00,0x64,0x01,0x02,0x00,EOT}},
		{0x6, "Mouse",
			{0x30,0x70,0x71,0xf0,EOT},
			{0x00,0x0c,0x02,0x00,EOT}},
		{0x7, "GPIO",
			{0x70,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb8,0xb9,0xba,
			 0xbb,0xbc,0xbd,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc8,
			 0xc9,0xca,0xcb,0xcc,0xcd,0xd0,0xd1,0xd2,0xd3,0xd4,
			 0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xf0,0xf1,
			 0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,
			 0xfc,EOT},
			{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,NANA,NANA,NANA,NANA,NANA,NANA,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,NANA,
			 0x00,EOT}},
		{0x8, "Game port",
			{0x30,0x60,0x61,EOT},
			{0x00,0x02,0x01,EOT}},
		{0x9, "Consumer IR",
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0x10,0x0b,0x00,EOT}},
		{0xa, "MIDI port",
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0x00,0x0a,0x00,EOT}},
		{EOT}}},
	{0x8710, "IT8710F", {
		{EOT}}},
	{0x8712, "IT8712F", {
		{NOLDN, NULL,
			{0x07,0x20,0x21,0x22,0x23,0x24,0x2b,EOT},
			{NANA,0x87,0x12,0x08,0x00,0x00,0x00,EOT}},
		{0x0, "Floppy",
			{0x30,0x60,0x61,0x70,0x74,0xf0,0xf1,EOT},
			{0x00,0x03,0xf0,0x06,0x02,0x00,0x00,EOT}},
		{0x1, "COM1",
			{0x30,0x60,0x61,0x70,0xf0,0xf1,0xf2,0xf3,EOT},
			{0x00,0x03,0xf8,0x04,0x00,0x50,0x00,0x7f,EOT}},
		{0x2, "COM2",
			{0x30,0x60,0x61,0x70,0xf0,0xf1,0xf2,0xf3,EOT},
			{0x00,0x02,0xf8,0x03,0x00,0x50,0x00,0x7f,EOT}},
		{0x3, "Parallel port",
			{0x30,0x60,0x61,0x62,0x63,0x70,0x74,0xf0,EOT},
			{0x00,0x03,0x78,0x07,0x78,0x07,0x03,0x03,EOT}},
		{0x4, "Environment controller",
			{0x30,0x60,0x61,0x62,0x63,0x70,0xf0,0xf1,0xf2,0xf3,
			 0xf4,0xf5,0xf6,EOT},
			{0x00,0x02,0x90,0x02,0x30,0x09,0x00,0x00,0x00,0x00,
			 0x00,NANA,NANA,EOT}},
		{0x5, "Keyboard",
			/* TODO: 0xf0: Error in datasheet? */
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0xf0,EOT},
			{0x01,0x00,0x60,0x00,0x64,0x01,0x02,0x08,EOT}},
		{0x6, "Mouse",
			{0x30,0x70,0x71,0xf0,EOT},
			{0x00,0x0c,0x02,0x00,EOT}},
		{0x7, "GPIO", /* TODO: 0x72, 0x73: Errors in datasheet? */
			{0x25,0x26,0x27,0x28,0x29,0x2a,0x2c,0x60,0x61,0x62,
			 0x63,0x64,0x65,0x70,0x71,0x72,0x73,0x74,0xb0,0xb1,
			 0xb2,0xb3,0xb4,0xb5,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,
			 0xc0,0xc1,0xc2,0xc3,0xc4,0xc8,0xc9,0xca,0xcb,0xcc,
			 0xe0,0xe1,0xe2,0xe3,0xe4,0xf0,0xf1,0xf2,0xf3,0xf4,
			 0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,EOT},
			{0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x01,0x00,0x00,0x40,0x00,0x01,0x00,0x00,0x40,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,NANA,0x00,EOT}},
		{0x8, "MIDI port",
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0x00,0x0a,0x00,EOT}},
		{0x9, "Game port",
			{0x30,0x60,0x61,EOT},
			{0x00,0x02,0x01,EOT}},
		{0xa, "Consumer IR",
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0x10,0x0b,0x00,EOT}},
		{EOT}}},
	{0x8716, "IT8716F", {
		{NOLDN, NULL,
			{0x07,0x20,0x21,0x22,0x23,0x24,0x2b,EOT},
			{NANA,0x87,0x16,0x01,0x00,0x00,0x00,EOT}},
		{0x0, NULL,
			{0x30,0x60,0x61,0x70,0x74,0xf0,0xf1,EOT},
			{0x00,0x03,0xf0,0x06,0x02,0x00,0x00,EOT}},
		{0x1, NULL,
			{0x30,0x60,0x61,0x70,0xf0,0xf1,0xf2,0xf3,EOT},
			{0x00,0x03,0xf8,0x04,0x00,0x50,0x00,0x7f,EOT}},
		{0x2, NULL,
			{0x30,0x60,0x61,0x70,0xf0,0xf1,0xf2,0xf3,EOT},
			{0x00,0x02,0xf8,0x03,0x00,0x50,0x00,0x7f,EOT}},
		{0x3, NULL,
			{0x30,0x60,0x61,0x62,0x63,0x70,0x74,0xf0,EOT},
			{0x00,0x03,0x78,0x07,0x78,0x07,0x03,0x03,EOT}},
		{0x4, NULL,
			{0x30,0x60,0x61,0x62,0x63,0x70,0xf0,0xf1,0xf2,0xf3,
			 0xf4,0xf5,0xf6,EOT},
			{0x00,0x02,0x90,0x02,0x30,0x09,0x00,0x00,0x00,0x00,
			 0x00,NANA,NANA,EOT}},
		{0x5, NULL,
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0xf0,EOT},
			{0x01,0x00,0x60,0x00,0x64,0x01,0x02,0x00,EOT}},
		{0x6, NULL,
			{0x30,0x70,0x71,0xf0,EOT},
			{0x00,0x0c,0x02,0x00,EOT}},
		{0x7, NULL,
			{0x25,0x26,0x27,0x28,0x29,0x2a,0x2c,0x60,0x61,0x62,
			 0x63,0x64,0x65,0x70,0x71,0x72,0x73,0x74,0xb0,0xb1,
			 0xb2,0xb3,0xb4,0xb5,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,
			 0xc0,0xc1,0xc2,0xc3,0xc4,0xc8,0xc9,0xca,0xcb,0xcc,
			 0xe0,0xe1,0xe2,0xe3,0xe4,0xf0,0xf1,0xf2,0xf3,0xf4,
			 0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,EOT},
			{0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x20,0x38,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x01,0x00,0x00,0x40,0x00,0x01,0x00,0x00,0x40,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			 0x00,0x00,0x00,0x00,0x00,0x00,0x00,NANA,0x00,EOT}},
		{0x8, NULL,
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0x00,0x0a,0x00,EOT}},
		{0x9, NULL,
			{0x30,0x60,0x61,EOT},
			{0x00,0x02,0x01,EOT}},
		{0xa, NULL,
			{0x30,0x60,0x61,0x70,0xf0,EOT},
			{0x00,0x03,0x10,0x0b,0x00,EOT}},
		{EOT}}},
	{0x8718, "IT8718F", {
		{EOT}}},
	{EOT}
};

/**
 * Enable configuration sequence (ITE uses this for newer IT87[012]xF).
 *
 * IT871[01]F uses 0x87, 0x87 -> Fintek detection should handle it
 * IT8708F uses 0x87, 0x87 -> Fintek detection should handle it
 * IT8761F uses 0x87, 0x61, 0x55, 0x55/0xaa
 * IT86xxF series uses different ports
 * IT8661F uses 0x86, 0x61, 0x55/0xaa, 0x55/0xaa and 32 more writes
 * IT8673F uses 0x86, 0x80, 0x55/0xaa, 0x55/0xaa and 32 more writes
 */
static void enter_conf_mode_ite(uint16_t port)
{
	outb(0x87, port);
	outb(0x01, port);
	outb(0x55, port);
	if (port == 0x2e)
		outb(0x55, port);
	else
		outb(0xaa, port);
}

static void exit_conf_mode_ite(uint16_t port)
{
	regwrite(port, 0x02, 0x02);
}

static void probe_idregs_ite_helper(uint16_t port)
{
	uint16_t id, chipver;

	id = regval(port, CHIP_ID_BYTE1_REG) << 8;
	id |= regval(port, CHIP_ID_BYTE2_REG);
	chipver = regval(port, CHIP_VERSION_REG) & 0x0f; /* Only bits 3..0 */

	if (superio_unknown(reg_table, id)) {
		no_superio_found(port);
		exit_conf_mode_ite(port);
		return;
	}

	printf("Found ITE %s (id=0x%04x, rev=0x%01x) at port=0x%x\n",
	       get_superio_name(reg_table, id), id, chipver, port);

	dump_superio("ITE", reg_table, port, id);
	dump_superio_readable(port); /* TODO */
}

void probe_idregs_ite(uint16_t port)
{
	enter_conf_mode_ite(port);
	probe_idregs_ite_helper(port);
	exit_conf_mode_ite(port);

	enter_conf_mode_winbond_fintek_ite_8787(port);
	probe_idregs_ite_helper(port);
	exit_conf_mode_winbond_fintek_ite_8787(port);
}

