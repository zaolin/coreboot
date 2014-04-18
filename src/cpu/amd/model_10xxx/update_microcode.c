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


#ifndef __ROMCC__
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <string.h>

#include <cpu/amd/microcode.h>
#endif

static const u8 microcode_updates[] __attribute__ ((aligned(16))) = {

#ifdef __ROMCC__

	// Barcelona revAx
//		#include "mc_patch_01000020.h"

	// Barcelona revBx
		#include "mc_patch_01000033.h"

	// Barcelona rev Cx??
//		#include "mc_patch_01000035.h"

#endif
	/*  Dummy terminator  */
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
};

static u32 get_equivalent_processor_rev_id(u32 orig_id) {
	static unsigned id_mapping_table[] = {
		0x100f00, 0x1000,
		0x100f01, 0x1000,
		0x100f02, 0x1000,
		0x100f20, 0x1020,
		0x100f21, 0x1020,
		0x100f2A, 0x1020,
		0x100f22, 0x1022,
	};

	u32 new_id;
	int i;

	new_id = 0;

	for (i = 0; i < sizeof(id_mapping_table); i += 2 ) {
		if(id_mapping_table[i]==orig_id) {
			new_id = id_mapping_table[i + 1];
			break;
		}
	}

	return new_id;

}

void update_microcode(u32 cpu_deviceid)
{
	u32 equivalent_processor_rev_id;

	/* Update the microcode */
	equivalent_processor_rev_id = get_equivalent_processor_rev_id(cpu_deviceid );
	if (equivalent_processor_rev_id != 0) {
		amd_update_microcode((void *) microcode_updates, equivalent_processor_rev_id);
	} else {
		printk_debug("microcode: rev id not found. Skipping microcode patch!\n");
	}

}
