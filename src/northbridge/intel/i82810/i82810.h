/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Corey Osgood <corey@slightlyhackish.com>
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

/*
 * Datasheet:
 *   - Name: Intel 810 Chipset:
 *           82810/82810-DC100 Graphics and Memory Controller Hub (GMCH)
 *   - URL: http://www.intel.com/design/chipsets/datashts/290656.htm
 *   - PDF: ftp://download.intel.com/design/chipsets/datashts/29065602.pdf
 *   - Order Number: 290656-002
 */

/*
 * PCI Configuration Registers.
 *
 * Any addresses between 0x00 and 0xff not listed below are reserved and
 * should not be touched.
 */

#define VID	0x00 /* Vendor Identification   */
#define DID	0x02 /* Device Identification   */
#define PCICMD	0x04 /* PCI Command Register    */
#define PCISTS	0x06 /* PCI Status Register     */
#define RID	0x08 /* Revision Identification */
#define SUBC	0x0a /* Sub-Class Code          */
#define BCC	0x0b /* Base Class Code         */
#define MLT	0x0d /* Master Latency Timer    */
#define HDR	0x0e /* Header Type             */
#define SVID	0x2c /* Subsystem Vendor Identification  */
#define SID	0x2e /* Subsystem Identification         */
#define CAPPTR	0x34 /* Capabilities Pointer             */

/* TODO: Descriptions  */
#define GMCHCFG	0x50
#define PAM	0x51
#define DRP	0x52
#define DRAMT	0x53
#define FDHC	0x58
#define SMRAM	0x70 /* System Management RAM Control */
#define MISSC	0x72
#define MISSC2	0x80
#define BUFF_SC	0x92
