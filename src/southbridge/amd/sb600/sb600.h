/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
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

#ifndef SB600_H
#define SB600_H

#include <device/pci_ids.h>
#include "chip.h"

extern void pm_iowrite(u8 reg, u8 value);
extern u8 pm_ioread(u8 reg);
extern void pm2_iowrite(u8 reg, u8 value);
extern u8 pm2_ioread(u8 reg);
extern void set_sm_enable_bits(device_t sm_dev, u32 reg_pos, u32 mask, u32 val);

void sb600_enable(device_t dev);

#endif /* SB600_H */
