/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#define DMI_TABLE_SIZE 0x55

static u8 dmi_table[DMI_TABLE_SIZE] = {
	0x5f, 0x53, 0x4d, 0x5f, 0x2d, 0x1f, 0x02, 0x03, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5f, 0x44, 0x4d, 0x49, 0x5f, 0xeb, 0xa8, 0x03, 0xa0, 0xff, 0x0f, 0x00, 0x01, 0x00, 0x23, 0x00,
	0x00, 0x14, 0x00, 0x00, 0x01, 0x02, 0x00, 0xe0, 0x03, 0x07, 0x90, 0xde, 0xcb, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x37, 0x01, 0x63, 0x6f, 0x72, 0x65, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x73, 0x20,
	0x47, 0x6d, 0x62, 0x48, 0x00, 0x32, 0x2e, 0x30, 0x00, 0x30, 0x33, 0x2f, 0x31, 0x33, 0x2f, 0x32,
	0x30, 0x30, 0x38, 0x00, 0x00
};
