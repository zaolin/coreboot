/*
 * This file is part of the coreboot project.
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

#ifndef SOUTHBRIDGE_VIA_K8T890_K8T890_H
#define SOUTHBRIDGE_VIA_K8T890_K8T890_H

/* Static resources for K8T890. */
#define K8T890_APIC_ID		0x3

/*
 * Please check the datasheet and traf_ctrl_enable before change!
 * It can't be changed to an arbitrary address.
 */
#define K8T890_APIC_BASE	0xfecc0000

/* The 256 bytes of NVRAM for S3 storage, 256B aligned */
#define K8T890_NVRAM_IO_BASE	0xf00

#define K8T890_MMCONFIG_MBAR	0x61
#define K8T890_MULTIPLE_FN_EN	0x4f

/* the FB size in MB (min is 8MB max is 512MB) */
#define K8M890_FBSIZEMB		64

#include <device/device.h>

extern void writeback(struct device *dev, u16 where, u8 what);
extern void dump_south(device_t dev);

int k8m890_host_fb_size_get(void);
//void k8m890_host_fb_direct_set(uint32_t fb_address);

#endif
