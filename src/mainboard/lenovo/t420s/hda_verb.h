/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
 * Copyright (C) 2014 Vladimir Serbinenko
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*	Vendor Name    : IDT
 *	Vendor ID      : 0x10ec0269
 *	Subsystem ID   : 0x17aa21fa
 *	Revision ID    : 0x100303
 */

/*
0x19 0x04211040
0x1a 0x61a19050
0x1b 0x04a11060
0x1c 0x6121401f
0x1d 0x40f001f0
0x1e 0x40f001f0
0x1f 0x90170110
0x20 0x40f001f0
0x22 0x40f001f0
0x23 0x90a60170
*/

static const u32 mainboard_cim_verb_data[] = {
	/* coreboot specific header */
  	0x14f1506e,	// Codec Vendor / Device ID: Conexant CX20590 / Datasheet says CX20672
  	0x17aa21d2,	// Subsystem ID
	0x0000000a,	// Number of 4 dword sets

	0x04211040,
	0x61a19050,
	0x04a11060,
	0x6121401f,
	0x40f001f0,
	0x40f001f0,
	0x90170110,
	0x40f001f0,
	0x40f001f0,
	0x90a60170
};

static const u32 mainboard_pc_beep_verbs[] = {
	0x02177a00, /* Digital PCBEEP Gain: 0h=-9db, 1h=-6db ... 4h=+3db, 5h=+6db */
};

static const u32 mainboard_pc_beep_verbs_size =
	ARRAY_SIZE(mainboard_pc_beep_verbs);


