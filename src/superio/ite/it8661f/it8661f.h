/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 Uwe Hermann <uwe@hermann-uwe.de>
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

#ifndef SUPERIO_ITE_IT8661F_IT8661F_H
#define SUPERIO_ITE_IT8661F_IT8661F_H

/* Datasheet: http://www.ite.com.tw/product_info/PC/Brief-IT8661_2.asp */

#define IT8661F_FDC  0x00 /* Floppy */
#define IT8661F_SP1  0x01 /* Com1 */
#define IT8661F_SP2  0x02 /* Com2 */
#define IT8661F_PP   0x03 /* Parallel port */
#define IT8661F_IR   0x04 /* IR */
#define IT8661F_GPIO 0x05 /* GPIO & Alternate Function Configuration */

#endif
