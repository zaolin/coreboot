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

#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <boot/tables.h>
#include "chip.h"

int add_mainboard_resources(struct lb_memory *mem)
{
#if CONFIG_HAVE_ACPI_RESUME == 1
	lb_add_memory_range(mem, LB_MEM_RESERVED,
		CONFIG_RAMBASE, ((CONFIG_LB_MEM_TOPK<<10) - CONFIG_RAMBASE));
	lb_add_memory_range(mem, LB_MEM_RESERVED,
		CONFIG_DCACHE_RAM_BASE, CONFIG_DCACHE_RAM_SIZE);
#endif
	return 0;
}

struct chip_operations mainboard_ops = {
	CHIP_NAME("ASUS M2V-MX SE Mainboard")
};
