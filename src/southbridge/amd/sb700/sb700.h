/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
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

#ifndef SB700_H
#define SB700_H

#include <device/pci_ids.h>
#include "chip.h"

/* Power management index/data registers */
#define PM_INDEX	0xcd6
#define PM_DATA		0xcd7
#define PM2_INDEX	0xcd0
#define PM2_DATA	0xcd1

extern void pm_iowrite(u8 reg, u8 value);
extern u8 pm_ioread(u8 reg);
extern void pm2_iowrite(u8 reg, u8 value);
extern u8 pm2_ioread(u8 reg);
extern void set_sm_enable_bits(device_t sm_dev, u32 reg_pos, u32 mask, u32 val);

#define REV_SB700_A11	0x11
#define REV_SB700_A12	0x12
#define REV_SB700_A14	0x14
#define REV_SB700_A15	0x15

/* This shouldn't be called before set_sb700_revision() is called.
 * Once set_sb700_revision() is called, we use get_sb700_revision(),
 * the simpler one, to get the sb700 revision ID.
 * The id is 0x39 if A11, 0x3A if A12, 0x3C if A14, 0x3D if A15.
 * The differentiate is 0x28, isn't it? */
#define get_sb700_revision(sm_dev)	(pci_read_config8((sm_dev), 0x08) - 0x28)

void sb700_enable(device_t dev);

#ifdef __PRE_RAM__
void sb700_lpc_port80(void);
void sb700_pci_port80(void);
#else
#include <device/pci.h>
/* allow override in mainboard.c */
void sb700_setup_sata_phys(struct device *dev);

#endif

#endif /* SB700_H */
