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

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <arch/io.h>
#include <boot/tables.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <device/pci_def.h>
#include "southbridge/amd/sb700/sb700.h"
#include "southbridge/amd/sb700/smbus.h"
#include "chip.h"

uint64_t uma_memory_base, uma_memory_size;

void set_pcie_dereset(void);
void set_pcie_reset(void);
u8 is_dev3_present(void);
/* TODO - Need to find GPIO for PCIE slot.
 * Kino uses GPIO ? as PCIe slot reset, GPIO? as GFX slot reset. We need to
 * pull it up before training the slot.
 ***/
void set_pcie_dereset()
{
	/* PCIE slot not yet supported.*/
}

void set_pcie_reset()
{
	/* PCIE slot not yet supported.*/
}

u8 is_dev3_present(void)
{
	return 0;
}

/*************************************************
* enable the dedicated function in kino board.
* This function called early than rs780_enable.
*************************************************/
static void kino_enable(device_t dev)
{
	printk(BIOS_INFO, "Mainboard Kino Enable. dev=0x%p\n", dev);

#if CONFIG_GFXUMA
	msr_t msr, msr2;

	/* TOP_MEM: the top of DRAM below 4G */
	msr = rdmsr(TOP_MEM);
	printk
	    (BIOS_INFO, "%s, TOP MEM: msr.lo = 0x%08x, msr.hi = 0x%08x\n",
	     __func__, msr.lo, msr.hi);

	/* TOP_MEM2: the top of DRAM above 4G */
	msr2 = rdmsr(TOP_MEM2);
	printk
	    (BIOS_INFO, "%s, TOP MEM2: msr2.lo = 0x%08x, msr2.hi = 0x%08x\n",
	     __func__, msr2.lo, msr2.hi);

	/* refer to UMA Size Consideration in 780 BDG. */
	switch (msr.lo) {
	case 0x10000000:	/* 256M system memory */
		uma_memory_size = 0x4000000;	/* 64M recommended UMA */
		break;

	case 0x20000000:	/* 512M system memory */
		uma_memory_size = 0x8000000;	/* 128M recommended UMA */
		break;

	default:		/* 1GB and above system memory */
		uma_memory_size = 0x10000000;	/* 256M recommended UMA */
		break;
	}

	uma_memory_base = msr.lo - uma_memory_size;	/* TOP_MEM1 */
	printk(BIOS_INFO, "%s: uma size 0x%08llx, memory start 0x%08llx\n",
		    __func__, uma_memory_size, uma_memory_base);

	/* TODO: TOP_MEM2 */
#else
	uma_memory_size = 0x8000000;	/* 128M recommended UMA */
	uma_memory_base = 0x38000000;	/* 1GB  system memory supposed */
#endif

	set_pcie_dereset();
	/* get_ide_dma66(); */
}

int add_mainboard_resources(struct lb_memory *mem)
{
	/* UMA is removed from system memory in the northbridge code, but
	 * in some circumstances we want the memory mentioned as reserved.
 	 */
#if CONFIG_GFXUMA
	printk(BIOS_INFO, "uma_memory_start=0x%llx, uma_memory_size=0x%llx \n",
		    uma_memory_base, uma_memory_size);
	lb_add_memory_range(mem, LB_MEM_RESERVED, uma_memory_base,
			    uma_memory_size);
#endif
	return 0;
}

struct chip_operations mainboard_ops = {
	CHIP_NAME("IEI Kino-780AM2 Mainboard")
	.enable_dev = kino_enable,
};
