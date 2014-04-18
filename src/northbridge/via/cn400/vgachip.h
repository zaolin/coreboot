/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Corey Osgood <corey.osgood@gmail.com>
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

#ifndef _PC80_VGABIOS
#define _PC80_VGABIOS

extern struct chip_control pc80_vgabios_control;

struct pc80_vgabios_config {
	int nothing;
};

void vga_enable_console(void);
void do_vgabios(void);
void setup_realmode_idt(void);
void write_protect_vgabios(void);

#endif /* _PC80_VGABIOS */
