/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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

/* Public Revisions - USE THESE VERSIONS TO MAKE COMPARE WITH CPULOGICALID RETURN VALUE*/
#define	AMD_SAFEMODE	0x80000000	/* Unknown future revision - SAFE MODE */
#define	AMD_NPT_F0	0x00000001	/* F0 stepping */
#define	AMD_NPT_F1	0x00000002	/* F1 stepping */
#define	AMD_NPT_F2C	0x00000004
#define	AMD_NPT_F2D	0x00000008
#define	AMD_NPT_F2E	0x00000010	/* F2 stepping E */
#define	AMD_NPT_F2G	0x00000020	/* F2 stepping G */
#define	AMD_NPT_F2J	0x00000040
#define	AMD_NPT_F2K	0x00000080
#define	AMD_NPT_F3L	0x00000100	/* F3 Stepping */
#define	AMD_NPT_G0A	0x00000200	/* G0 stepping */
#define	AMD_NPT_G1B	0x00000400	/* G1 stepping */
#define	AMD_DR_A0A	0x00010000	/* Barcelona A0 */
#define	AMD_DR_A1B	0x00020000	/* Barcelona A1 */
#define	AMD_DR_A2	0x00040000	/* Barcelona A2 */
#define	AMD_DR_B0	0x00080000	/* Barcelona B0 */
#define	AMD_DR_B1	0x00100000	/* Barcelona B1 */
#define	AMD_DR_B2	0x00200000	/* Barcelona B2 */
#define	AMD_DR_BA	0x00400000	/* Barcelona BA */

/*
 Groups - Create as many as you wish, from the above public values
*/
#define	AMD_NPT_F2	(AMD_NPT_F2C + AMD_NPT_F2D + AMD_NPT_F2E + AMD_NPT_F2G + AMD_NPT_F2J + AMD_NPT_F2K)
#define	AMD_NPT_F3	(AMD_NPT_F3L)
#define	AMD_NPT_Fx	(AMD_NPT_F0 + AMD_NPT_F1 + AMD_NPT_F2 + AMD_NPT_F3)
#define	AMD_NPT_Gx	(AMD_NPT_G0A + AMD_NPT_G1B)
#define	AMD_NPT_ALL	(AMD_NPT_Fx + AMD_NPT_Gx)
#define	AMD_DR_Ax	(AMD_DR_A0A + AMD_DR_A1B + AMD_DR_A2)
#define	AMD_FINEDELAY	(AMD_NPT_F0 + AMD_NPT_F1 + AMD_NPT_F2)
#define	AMD_GT_F0	(AMD_NPT_ALL AND NOT AMD_NPT_F0)


#define CPUID_EXT_PM		0x80000007

#define CPUID_MODEL		1


#define HWCR			0xC0010015


#define FidVidStatus		0xC0010042


#define FS_Base		0xC0000100


#define BU_CFG			0xC0011023
#define BU_CFG2		0xC001102A
