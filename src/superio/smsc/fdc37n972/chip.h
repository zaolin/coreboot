/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 coresystems GmbH
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

#ifndef _SUPERIO_SMSC_FDC37N972
#define _SUPERIO_SMSC_FDC37N972

#include <device/device.h>
#include <pc80/keyboard.h>
#include <uart8250.h>

extern struct chip_operations superio_smsc_fdc37n972_ops;

struct superio_smsc_fdc37n972_config {
	struct uart8250 com1, com2;
	struct pc_keyboard keyboard;
};

#endif /* _SUPERIO_SMSC_FDC37N972 */

