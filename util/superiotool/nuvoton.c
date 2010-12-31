/*
 * This file is part of the superiotool project.
 *
 * Copyright (C) 2010 Google Inc.
 * Written by David Hendricks <dhendrix@google.com> for Nuvoton Technology Corp.
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

#define DEVICE_ID_REG		0x20	/* Super I/O ID (SID) / family */
#define DEVICE_REV_REG		0x27	/* Super I/O revision ID (SRID) */

static const struct superio_registers reg_table[] = {
	{0xfc, "WPCE775x / NPCE781x", {
		{NOLDN, NULL,
			{0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
			 0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,EOT},
			{0xFC,0x11,RSVD,RSVD,RSVD,0x00,0x00,MISC,0x00,
			 0x04,RSVD,RSVD,RSVD,0x00,RSVD,RSVD,EOT}},
		{0x03, "CIR Port (CIRP)",	/* where supported */
			{0x30,0x60,0x61,0x70,0x71,0x74,0x75,0xf0,EOT},
			{0x00,0x03,0xf8,0x04,0x03,0x04,0x04,0x02,EOT}},
		{0x04, "Mobile System Wake-Up Control Config (MSWC)",
			{0x30,0x60,0x61,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x00,0x00,0x00,0x03,0x04,0x04,EOT}},
		{0x05, "Mouse config (KBC)",
			{0x30,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x0c,0x03,0x04,0x04,EOT}},
		{0x06, "Keyboard config (KBC)",
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x00,0x60,0x00,0x64,0x01,0x03,0x04,0x04,EOT}},
		{0x0f, "Shared memory (SHM)",
			{0x30,0x60,0x61,0x70,0x71,0x74,0x75,0xf0,0xf1,0xf2,
			0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,EOT},
			{0x00,0x00,0x00,0x00,0x00,0x04,0x04,MISC,0x07,RSVD,
			RSVD,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,EOT}},
		{0x11, "Power management I/F Channel 1 (PM1)",
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x00,0x62,0x00,0x66,0x01,0x03,0x04,0x04,EOT}},
		{0x12, "Power management I/F Channel 2 (PM2)",
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x00,0x68,0x00,0x6c,0x01,0x03,0x04,0x04,EOT}},
		{0x15, "Enhanced Wake On CIR (EWOC)",
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x04,0x04,EOT}},
		{0x17, "Power Management I/F Channel 3 (PM3)",
			{0x30,0x60,0x61,0x62,0x63,0x70,0x71,0x74,0x75,EOT},
			{0x00,0x00,0x6a,0x00,0x6e,0x01,0x03,0x04,0x04,EOT}},
		{0x1a, "Serial Port with Fast Infrared Port (FIR)",
			{0x30,0x60,0x61,0x70,0x71,0x74,0x75,0xf0,EOT},
			{0x00,0x02,0xf8,0x03,0x03,0x04,0x04,0x02,EOT}},
		{EOT}}},
	{0x1a, "WPCM450", {
		{EOT}}},
	{EOT}
};

void probe_idregs_nuvoton(uint16_t port)
{
	uint8_t sid, srid;
	uint8_t chip_id = 0, chip_rev = 0;

	probing_for("Nuvoton", "(sid=0xfc) ", port);

	sid = regval(port, DEVICE_ID_REG);
	srid = regval(port, DEVICE_REV_REG);

	if (sid == 0xfc) {	/* WPCE775xL family */
		/*
		 * bits 7-5: Chip ID
		 * bits 4-0: Chip revision
		 */
		chip_id = srid >> 5;
		chip_rev = srid & 0x1f;
	}

	if (superio_unknown(reg_table, sid)) {
		if (verbose)
			printf(NOTFOUND
			       "sid=0x%02x, id=0x%02x, rev=0x%02x\n", sid,
			       chip_id, chip_rev);
		return;
	}

	printf("Found Nuvoton %s (id=0x%02x, rev=0x%02x) at 0x%x\n",
	       get_superio_name(reg_table, sid), chip_id, chip_rev, port);
	chip_found = 1;

	dump_superio("Nuvoton", reg_table, port, sid, LDN_SEL);
}

void print_nuvoton_chips(void)
{
	print_vendor_chips("Nuvoton", reg_table);
}
