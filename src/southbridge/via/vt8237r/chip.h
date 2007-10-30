/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Rudolf Marek <r.marek@assembler.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2 as published by
 * the Free Software Foundation.
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

#ifndef SOUTHBRIDGE_VIA_VT8237R_CHIP_H
#define SOUTHBRIDGE_VIA_VT8237R_CHIP_H

#include <stdint.h>

extern struct chip_operations southbridge_via_vt8237r_ops;

struct southbridge_via_vt8237r_config {
	/**
	 * Function disable. 1 = disabled.
	 * 7 Dev 17 fn 6 MC97
	 * 6 Dev 17 fn 5 AC97
	 * 5 Dev 16 fn 1 USB 1.1 UHCI Ports 2-3
	 * 4 Dev 16 fn 0 USB 1.1 UHCI Ports 0-1
	 * 3 Dev 15 fn 0 Serial ATA
	 * 2 Dev 16 fn 2 USB 1.1 UHCI Ports 4-5
	 * 1 Dev 16 fn 4 USB 2.0 EHCI
	 * 0 Dev 16 fn 3 USB 1.1 UHCI Ports 6-7
	 */
	u16 fn_ctrl_lo;

	/**
	 * 7 USB Device Mode 1=dis
	 * 6 Reserved
	 * 5 Internal LAN Controller Clock Gating 1=gated
	 * 4 Internal LAN Controller 1=di
	 * 3 Internal RTC 1=en
	 * 2 Internal PS2 Mouse 1=en
	 * 1 Internal KBC Configuration 0=dis ports 0x2e/0x2f off 0xe0-0xef
	 * 0 Internal Keyboard Controller 1=en
	 */
	u16 fn_ctrl_hi;

	int ide0_enable:1;
	int ide1_enable:1;
	/* 1 = 80-pin cable */
	int ide0_80pin_cable:1;
	int ide1_80pin_cable:1;
};

#endif /* SOUTHBRIDGE_VIA_VT8237R_CHIP_H */
