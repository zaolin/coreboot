/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 AMD
 * Written by Yinghai Lu <yinghai.lu@amd.com> for AMD.
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

#define W83627EHG_FDC              0   /* Floppy */
#define W83627EHG_PP               1   /* Parallel Port */
#define W83627EHG_SP1              2   /* Com1 */
#define W83627EHG_SP2              3   /* Com2 */
#define W83627EHG_KBC              5   /* Keyboard & Mouse */
#define W83627EHG_GPIO_GAME_MIDI   7   /* GPIO1, GPIO6, Game Port and MIDI Port */
#define W83627EHG_WDTO_PLED        8   /* TODO */
#define W83627EHG_GPIO_SUSLED      9   /* GPIO2, GPIO3, GPIO4, GPIO5 and SUSLED */
#define W83627EHG_ACPI            10   /* ACPI */
#define W83627EHG_HWM             11   /* Hardware Monitor */

/* virtual devices sharing the enables are encoded as follows:
   VLDN = baseLDN[7:0] | [10:8] bitpos of enable in 0x30 of baseLDN
*/

#define W83627EHG_SFI		((1 << 8) | 6) /* Flash has bit1 as enable */
#define W83627EHG_GPIO1		W83627EHG_GPIO_GAME_MIDI /* GPIO1 is at LDN 7, bit 0 */
#define W83627EHG_GAME		((1 << 8) | 7)
#define W83627EHG_MIDI		((2 << 8) | 7)
#define W83627EHG_GPIO6		((3 << 8) | 7)

#define W83627EHG_GPIO2		W83627EHG_GPIO_SUSLED /* GPIO2 is at LDN 9, bit 0 */
#define W83627EHG_GPIO3		((1 << 8) | 9)
#define W83627EHG_GPIO4		((2 << 8) | 9)
#define W83627EHG_GPIO5		((3 << 8) | 9)
