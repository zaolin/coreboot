/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2010 Joseph Smith <joe@settoplinux.org>
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
 *
 */

#include <spd.h>
#include <sdram_mode.h>
#include <delay.h>
#include "lib/debug.c"
#include "i82830.h"

/*-----------------------------------------------------------------------------
Macros and definitions.
-----------------------------------------------------------------------------*/

/* Uncomment this to enable debugging output. */
/* #define DEBUG_RAM_SETUP 1 */

/* Debugging macros. */
#if defined(DEBUG_RAM_SETUP)
#define PRINT_DEBUG(x)		print_debug(x)
#define PRINT_DEBUG_HEX8(x)	print_debug_hex8(x)
#define PRINT_DEBUG_HEX16(x)	print_debug_hex16(x)
#define PRINT_DEBUG_HEX32(x)	print_debug_hex32(x)
#define DUMPNORTH()		dump_pci_device(PCI_DEV(0, 0, 0))
#else
#define PRINT_DEBUG(x)
#define PRINT_DEBUG_HEX8(x)
#define PRINT_DEBUG_HEX16(x)
#define PRINT_DEBUG_HEX32(x)
#define DUMPNORTH()
#endif

/* DRC[10:8] - Refresh Mode Select (RMS).
 * 0x0 for Refresh Disabled (Self Refresh)
 * 0x1 for Refresh interval 15.6 us for 133MHz
 * 0x2 for Refresh interval 7.8 us for 133MHz
 * 0x7 /* Refresh interval 128 Clocks. (Fast Refresh Mode)
 */
#define RAM_COMMAND_REFRESH		0x1

/* DRC[6:4] - SDRAM Mode Select (SMS). */
#define RAM_COMMAND_SELF_REFRESH	0x0
#define RAM_COMMAND_NOP			0x1
#define RAM_COMMAND_PRECHARGE		0x2
#define RAM_COMMAND_MRS			0x3
#define RAM_COMMAND_CBR			0x6
#define RAM_COMMAND_NORMAL		0x7

/* DRC[29] - Initialization Complete (IC). */
#define RAM_COMMAND_IC			0x1

/*-----------------------------------------------------------------------------
DIMM-initialization functions.
-----------------------------------------------------------------------------*/

static void do_ram_command(uint32_t command)
{
	uint32_t reg32;

	/* Configure the RAM command. */
	reg32 = pci_read_config32(NORTHBRIDGE, DRC);
	/* Clear bits 29, 10-8, 6-4. */
	reg32 &= 0xdffff88f;
	reg32 |= command << 4;
	pci_write_config32(NORTHBRIDGE, DRC, reg32);
	PRINT_DEBUG("RAM command 0x");
	PRINT_DEBUG_HEX32(reg32);
	PRINT_DEBUG("\r\n");
}

static void ram_read32(uint8_t dimm_start, uint32_t offset)
{
	if (offset == 0x55aa55aa) {
		PRINT_DEBUG("  Reading RAM at 0x");
		PRINT_DEBUG_HEX32(dimm_start * 32 * 1024 * 1024);
		PRINT_DEBUG(" => 0x");
		PRINT_DEBUG_HEX32(read32(dimm_start * 32 * 1024 * 1024));
		PRINT_DEBUG("\r\n");

		PRINT_DEBUG("  Writing RAM at 0x");
		PRINT_DEBUG_HEX32(dimm_start * 32 * 1024 * 1024);
		PRINT_DEBUG(" <= 0x");
		PRINT_DEBUG_HEX32(offset);
		PRINT_DEBUG("\r\n");
		write32(dimm_start * 32 * 1024 * 1024, offset);

		PRINT_DEBUG("  Reading RAM at 0x");
		PRINT_DEBUG_HEX32(dimm_start * 32 * 1024 * 1024);
		PRINT_DEBUG(" => 0x");
		PRINT_DEBUG_HEX32(read32(dimm_start * 32 * 1024 * 1024));
		PRINT_DEBUG("\r\n");
	} else {
		PRINT_DEBUG("  Sending RAM command to 0x");
		PRINT_DEBUG_HEX32((dimm_start * 32 * 1024 * 1024) + offset);
		PRINT_DEBUG("\r\n");
		read32((dimm_start * 32 * 1024 * 1024) + offset);
	}
}

static void initialize_dimm_rows(void)
{
	int i, row;
	uint8_t dimm_start, dimm_end;
	unsigned device;

	dimm_start = 0;

	for (row = 0; row < (DIMM_SOCKETS * 2); row++) {

		switch (row) {
			case 0:
				device = DIMM_SPD_BASE;
				break;
			case 1:
				device = DIMM_SPD_BASE;
				break;
			case 2:
				device = DIMM_SPD_BASE + 1;
				break;
			case 3:
				device = DIMM_SPD_BASE + 1;
				break;
		}

		dimm_end = pci_read_config8(NORTHBRIDGE, DRB + row);

		if (dimm_end > dimm_start) {
			print_debug("Initializing SDRAM Row ");
			print_debug_hex8(row);
			print_debug("\r\n");

			/* NOP command */
			PRINT_DEBUG(" NOP ");
			do_ram_command(RAM_COMMAND_NOP);
			ram_read32(dimm_start, 0);
			udelay(200);

			/* Pre-charge all banks (at least 200 us after NOP) */
			PRINT_DEBUG(" Pre-charging all banks ");
			do_ram_command(RAM_COMMAND_PRECHARGE);
			ram_read32(dimm_start, 0);
			udelay(1);

			/* 8 CBR refreshes (Auto Refresh) */
			PRINT_DEBUG(" 8 CBR refreshes ");
			for (i = 0; i < 8; i++) {
				do_ram_command(RAM_COMMAND_CBR);
				ram_read32(dimm_start, 0);
				udelay(1);
			}

			/* MRS command */
			/* TODO: Set offset 0x1d0 according to DRT values */
			PRINT_DEBUG(" MRS ");
			do_ram_command(RAM_COMMAND_MRS);
			ram_read32(dimm_start, 0x1d0);
			udelay(2);

			/* Set GMCH-M Mode Select bits back to NORMAL operation mode */
			PRINT_DEBUG(" Normal operation mode ");
			do_ram_command(RAM_COMMAND_NORMAL);
			ram_read32(dimm_start, 0);
			udelay(1);

			/* Perform a dummy memory read/write cycle */
			PRINT_DEBUG(" Performing dummy read/write\r\n");
			ram_read32(dimm_start, 0x55aa55aa);
			udelay(1);
		}
		/* Set the start of the next DIMM. */
		dimm_start = dimm_end;
	}
}

/*-----------------------------------------------------------------------------
DIMM-independant configuration functions.
-----------------------------------------------------------------------------*/

struct dimm_size {
	unsigned long side1;
	unsigned long side2;
};

static struct dimm_size spd_get_dimm_size(unsigned device)
{
	struct dimm_size sz;
	int i, module_density, dimm_banks;
	sz.side1 = 0;
	module_density = spd_read_byte(device, SPD_DENSITY_OF_EACH_ROW_ON_MODULE);
	dimm_banks = spd_read_byte(device, SPD_NUM_DIMM_BANKS);

	/* Find the size of side1. */
	/* Find the larger value. The larger value is always side1. */
	for (i = 512; i >= 0; i >>= 1) {
		if ((module_density & i) == i) {
			sz.side1 = i;
			break;
		}
	}

	/* Set to 0 in case it's single sided. */
	sz.side2 = 0;

	/* Test if it's a dual-sided DIMM. */
	if (dimm_banks > 1) {
		/* Test to see if there's a second value, if so it's asymmetrical. */
		if (module_density != i) {
			/* Find the second value, picking up where we left off. */
			/* i >>= 1 done initially to make sure we don't get the same value again. */
			for (i >>= 1; i >= 0; i >>= 1) {
				if (module_density == (sz.side1 | i)) {
					sz.side2 = i;
					break;
				}
			}
			/* If not, it's symmetrical */
		} else {
			sz.side2 = sz.side1;
		}
	}

	/* SPD byte 31 is the memory size divided by 4 so we
	 * need to muliply by 4 to get the total size.
	 */
	sz.side1 *= 4;
	sz.side2 *= 4;
	return sz;
}

static void set_dram_row_boundaries(void)
{
	int i, value, drb1, drb2;

	for (i = 0; i < DIMM_SOCKETS; i++) {
		struct dimm_size sz;
		unsigned device;
		device = DIMM_SPD_BASE + i;
		drb1 = 0;
		drb2 = 0;

		/* First check if a DIMM is actually present. */
		if (spd_read_byte(device, SPD_MEMORY_TYPE) == 0x4) {
			print_debug("Found DIMM in slot ");
			print_debug_hex8(i);
			print_debug("\r\n");

			sz = spd_get_dimm_size(device);

			/* WISHLIST: would be nice to display it as decimal? */
			print_debug("DIMM is 0x");
			print_debug_hex16(sz.side1);
			print_debug(" on side 1\r\n");
			print_debug("DIMM is 0x");
			print_debug_hex16(sz.side2);
			print_debug(" on side 2\r\n");

			/* - Memory compatibility checks - */
			/* Test for PC133 (i82830 only supports PC133) */
			/* PC133 SPD9 - cycle time is always 75 */
			if (spd_read_byte(device, SPD_MIN_CYCLE_TIME_AT_CAS_MAX) != 0x75) {
				print_err("SPD9 DIMM Is Not PC133 Compatable\r\n");
				die("HALT\r\n");
			}
			/* PC133 SPD10 - access time is always 54 */
			if (spd_read_byte(device, SPD_ACCESS_TIME_FROM_CLOCK) != 0x54) {
				print_err("SPD10 DIMM Is Not PC133 Compatable\r\n");
				die("HALT\r\n");
			}

			/* The i82830 only supports a symmetrical dual-sided dimms
			 * and can't handle DIMMs smaller than 32MB per
			 * side or larger than 256MB per side.
			 */
			if ((sz.side2 != 0) && (sz.side1 != sz.side2)) {
				print_err("This northbridge only supports\r\n");
				print_err("symmetrical dual-sided DIMMs\r\n");
				print_err("booting as a single-sided DIMM\r\n");
				sz.side2 = 0;
			}
			if ((sz.side1 < 32)) {
				print_err("DIMMs smaller than 32MB per side\r\n");
				print_err("are not supported on this northbridge\r\n");
				die("HALT\r\n");
			}

			if ((sz.side1 > 256)) {
				print_err
				    ("DIMMs larger than 256MB per side\r\n");
				print_err
				    ("are not supported on this northbridge\r\n");
				die("HALT\r\n");
			}
			/* - End Memory compatibility checks - */

			/* We need to divide size by 32 to set up the
			 * DRB registers.
			 */
			if (sz.side1)
				drb1 = sz.side1 / 32;
			if (sz.side2)
				drb2 = sz.side2 / 32;
		} else {
			PRINT_DEBUG("No DIMM found in slot ");
			PRINT_DEBUG_HEX8(i);
			PRINT_DEBUG("\r\n");

			/* If there's no DIMM in the slot, set value to 0. */
			drb1 = 0;
			drb2 = 0;
		}
		/* Set the value for DRAM Row Boundary Registers */
		if (i == 0) {
			pci_write_config8(NORTHBRIDGE, DRB, drb1);
			pci_write_config8(NORTHBRIDGE, DRB + 1, drb1 + drb2);
			PRINT_DEBUG("DRB 0x");
			PRINT_DEBUG_HEX8(DRB);
			PRINT_DEBUG(" has been set to 0x");
			PRINT_DEBUG_HEX8(drb1);
			PRINT_DEBUG("\r\n");
			PRINT_DEBUG("DRB1 0x");
			PRINT_DEBUG_HEX8(DRB + 1);
			PRINT_DEBUG(" has been set to 0x");
			PRINT_DEBUG_HEX8(drb1 + drb2);
			PRINT_DEBUG("\r\n");
		} else if (i == 1) {
			value = pci_read_config8(NORTHBRIDGE, DRB + 1);
			pci_write_config8(NORTHBRIDGE, DRB + 2, value + drb1);
			pci_write_config8(NORTHBRIDGE, DRB + 3, value + drb1 + drb2);
			PRINT_DEBUG("DRB2 0x");
			PRINT_DEBUG_HEX8(DRB + 2);
			PRINT_DEBUG(" has been set to 0x");
			PRINT_DEBUG_HEX8(value + drb1);
			PRINT_DEBUG("\r\n");
			PRINT_DEBUG("DRB3 0x");
			PRINT_DEBUG_HEX8(DRB + 3);
			PRINT_DEBUG(" has been set to 0x");
			PRINT_DEBUG_HEX8(value + drb1 + drb2);
			PRINT_DEBUG("\r\n");

			/* We need to set the highest DRB value to 0x64 and 0x65.
			 * These are supposed to be "Reserved" but memory will
			 * not initialize properly if we don't.
			 */
			value = pci_read_config8(NORTHBRIDGE, DRB + 3);
			pci_write_config8(NORTHBRIDGE, DRB + 4, value);
			pci_write_config8(NORTHBRIDGE, DRB + 5, value);
		}
	}
}

static void set_dram_row_attributes(void)
{
	int i, dra, col, width, value;

	for (i = 0; i < DIMM_SOCKETS; i++) {
		unsigned device;
		device = DIMM_SPD_BASE + i;

		/* First check if a DIMM is actually present. */
		if (spd_read_byte(device, SPD_MEMORY_TYPE) == 0x4) {
			print_debug("Found DIMM in slot ");
			print_debug_hex8(i);
			print_debug(", setting DRA...\r\n");

			dra = 0x00;

			/* columns */
			col = spd_read_byte(device, SPD_NUM_COLUMNS);

			/* data width */
			width = spd_read_byte(device, SPD_MODULE_DATA_WIDTH_LSB);

			/* calculate page size in bits */
			value = ((1 << col) * width);

			/* convert to Kilobytes */
			dra = ((value / 8) >> 10);

			/* # of banks of DIMM (single or double sided) */
			value = spd_read_byte(device, SPD_NUM_DIMM_BANKS);

			if (value == 1) {
				if (dra == 2) {
					dra = 0xF0; /* 2KB */
				} else if (dra == 4) {
					dra = 0xF1; /* 4KB */
				} else if (dra == 8) {
					dra = 0xF2; /* 8KB */
				} else if (dra == 16) {
					dra = 0xF3; /* 16KB */
				} else {
					print_err("Page size not supported\r\n");
					die("HALT\r\n");
				}
			} else if (value == 2) {
				if (dra == 2) {
					dra = 0x00; /* 2KB */
				} else if (dra == 4) {
					dra = 0x11; /* 4KB */
				} else if (dra == 8) {
					dra = 0x22; /* 8KB */
				} else if (dra == 16) {
					dra = 0x33; /* 16KB */
				} else {
					print_err("Page size not supported\r\n");
					die("HALT\r\n");
				}
			} else {
				print_err("# of banks of DIMM not supported\r\n");
				die("HALT\r\n");
			}

		} else {
			PRINT_DEBUG("No DIMM found in slot ");
			PRINT_DEBUG_HEX8(i);
			PRINT_DEBUG(", setting DRA to 0xFF\r\n");

			/* If there's no DIMM in the slot, set dra value to 0xFF. */
			dra = 0xFF;
		}

		/* Set the value for DRAM Row Attribute Registers */
		pci_write_config8(NORTHBRIDGE, DRA + i, dra);
		PRINT_DEBUG("DRA 0x");
		PRINT_DEBUG_HEX8(DRA + i);
		PRINT_DEBUG(" has been set to 0x");
		PRINT_DEBUG_HEX8(dra);
		PRINT_DEBUG("\r\n");
	}
}

static void set_dram_timing(void)
{
	/* Set the value for DRAM Timing Register */
	/* TODO: Configure the value according to SPD values. */
	pci_write_config32(NORTHBRIDGE, DRT, 0x00000010);
}

static void set_dram_buffer_strength(void)
{
	/* TODO: This needs to be set according to the DRAM tech
	 * (x8, x16, or x32). Argh, Intel provides no docs on this!
	 * Currently, it needs to be pulled from the output of
	 * lspci -xxx Rx92
	 */

	/* Set the value for System Memory Buffer Strength Control Registers */
	pci_write_config32(NORTHBRIDGE, BUFF_SC, 0xFC9B491B);
}

/*-----------------------------------------------------------------------------
Public interface.
-----------------------------------------------------------------------------*/

static void sdram_set_registers(void)
{
	PRINT_DEBUG("Setting initial sdram registers....\r\n");

	/* Calculate the value for DRT DRAM Timing Register */
	set_dram_timing();

	/* Setup System Memory Buffer Strength Control Registers */
	set_dram_buffer_strength();

	/* Setup DRAM Row Boundary Registers */
	set_dram_row_boundaries();

	/* Setup DRAM Row Attribute Registers */
	set_dram_row_attributes();

	PRINT_DEBUG("Initial sdram registers have been set.\r\n");
}

static void northbridge_set_registers(void)
{
	uint16_t value;
	int igd_memory = 0;

	PRINT_DEBUG("Setting initial nothbridge registers....\r\n");

	/* Set the value for Fixed DRAM Hole Control Register */
	pci_write_config8(NORTHBRIDGE, FDHC, 0x00);

	/* Set the value for Programable Attribute Map Registers
	 * Ideally, this should be R/W for as many ranges as possible.
	 */
	pci_write_config8(NORTHBRIDGE, PAM0, 0x30);
	pci_write_config8(NORTHBRIDGE, PAM1, 0x33);
	pci_write_config8(NORTHBRIDGE, PAM2, 0x33);
	pci_write_config8(NORTHBRIDGE, PAM3, 0x33);
	pci_write_config8(NORTHBRIDGE, PAM4, 0x33);
	pci_write_config8(NORTHBRIDGE, PAM5, 0x33);
	pci_write_config8(NORTHBRIDGE, PAM6, 0x33);

	/* Set the value for System Management RAM Control Register */
	pci_write_config8(NORTHBRIDGE, SMRAM, 0x02);

	/* Set the value for GMCH Control Register #0 */
	pci_write_config16(NORTHBRIDGE, GCC0, 0xA072);

	/* Set the value for Aperture Base Configuration Register */
	pci_write_config32(NORTHBRIDGE, APBASE, 0x00000008);

	/* Set the value for GMCH Control Register #1 */
	switch (CONFIG_VIDEO_MB) {
	case 512: /* 512K of memory */
		igd_memory = 0x2;
		break;
	case 1: /* 1M of memory */
		igd_memory = 0x3;
		break;
	case 8: /* 8M of memory */
		igd_memory = 0x4;
		break;
	default: /* No memory */
		pci_write_config16(NORTHBRIDGE, GCC1, 0x0002);
		igd_memory = 0x0;
	}

	value = pci_read_config16(NORTHBRIDGE, GCC1);
	value |= igd_memory << 4;
	pci_write_config16(NORTHBRIDGE, GCC1, value);

	PRINT_DEBUG("Initial northbridge registers have been set.\r\n");
}

static void sdram_initialize(void)
{
	int i;
	uint32_t reg32;

	/* Setup Initial SDRAM Registers */
	sdram_set_registers();

	/* 0. Wait until power/voltages and clocks are stable (200us). */
	udelay(200);

	/* Initialize each row of memory one at a time */
	initialize_dimm_rows();

	/* Enable Refresh */
	PRINT_DEBUG("Enabling Refresh\r\n");
	reg32 = pci_read_config32(NORTHBRIDGE, DRC);
	reg32 |= (RAM_COMMAND_REFRESH << 8);
	pci_write_config32(NORTHBRIDGE, DRC, reg32);

	/* Set initialization complete */
	PRINT_DEBUG("Setting initialization complete\r\n");
	reg32 = pci_read_config32(NORTHBRIDGE, DRC);
	reg32 |= (RAM_COMMAND_IC << 29);
	pci_write_config32(NORTHBRIDGE, DRC, reg32);

	/* Setup Initial Northbridge Registers */
	northbridge_set_registers();

	PRINT_DEBUG("Northbridge following SDRAM init:\r\n");
	DUMPNORTH();
}
