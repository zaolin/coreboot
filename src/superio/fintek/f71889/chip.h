/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Alec Ari <neotheuser@ymail.com>
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

#ifndef SUPERIO_FINTEK_F71889_CHIP_H
#define SUPERIO_FINTEK_F71889_CHIP_H

#include <pc80/keyboard.h>
#include <device/device.h>
#include <uart8250.h>

extern struct chip_operations superio_fintek_f71889_ops;

struct superio_fintek_f71889_config {

	struct pc_keyboard keyboard;
};

#endif
