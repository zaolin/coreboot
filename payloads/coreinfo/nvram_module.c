/*
 * This file is part of the coreinfo project.
 *
 * Copyright (C) 2008 Uwe Hermann <uwe@hermann-uwe.de>
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

#include "coreinfo.h"

#ifdef CONFIG_MODULE_NVRAM

/**
 * Dump 256 bytes of NVRAM.
 */
static void dump_nvram(WINDOW *win, int row, int col)
{
	int i, x = 0, y = 0;

	for (i = 1; i < 257; i++) {
		mvwprintw(win, row + y, col + x, "%02x ", nvram_read(i - 1));
		x += 3;
		if (i % 16 == 0) {
			y++;	/* Start a newline after 16 bytes. */
			x = 0;
		}
		if (i % 64 == 0) {
			y++;	/* Add an empty line after 64 bytes. */
			x = 0;
		}
	}
}

static int nvram_module_redraw(WINDOW *win)
{
	print_module_title(win, "NVRAM Dump");
	dump_nvram(win, 2, 1);
	return 0;
}

static int nvram_module_init(void)
{
	return 0;
}

struct coreinfo_module nvram_module = {
	.name = "NVRAM",
	.init = nvram_module_init,
	.redraw = nvram_module_redraw,
};

#else

struct coreinfo_module nvram_module = {
};

#endif
