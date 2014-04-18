/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
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

#include <types.h>
#include <spd.h>
#include <spd_ddr2.h>
#include <sdram_mode.h>
#include <delay.h>
#include "cx700_registers.h"

/* Debugging macros. */
#if CONFIG_DEBUG_RAM_SETUP
#define PRINTK_DEBUG(x...)      printk(BIOS_DEBUG, x)
#else
#define PRINTK_DEBUG(x...)
#endif

#define RAM_COMMAND_NORMAL	0x0
#define RAM_COMMAND_NOP		0x1
#define RAM_COMMAND_PRECHARGE	0x2
#define RAM_COMMAND_MRS		0x3
#define RAM_COMMAND_CBR		0x4

#define HOSTCTRL		PCI_DEV(0, 0, 2)
#define MEMCTRL			PCI_DEV(0, 0, 3)

#define DDRII_666	0x5
#define DDRII_533	0x4
#define DDRII_400	0x3
#define DDRII_333	0x2
#define DDRII_266	0x1
#define DDRII_200	0x0

#define OHM_150 1

#ifdef	MEM_WIDTH_32BIT_MODE
#define SDRAM1X_RA_14		30
#define SDRAM1X_RA_13		29
#define SDRAM1X_RA_12		28
#define SDRAM1X_RA_12_8bk	26
#define	SDRAM1X_CA_12		15
#define	SDRAM1X_CA_11		14
#define	SDRAM1X_CA_09		11
#define	SDRAM1X_CA_09_8bk	11
#define	SDRAM1X_BA1		13
#define	SDRAM1X_BA2_8bk		14
#define	SDRAM1X_BA1_8bk		13
#else
#define	SDRAM1X_RA_14		31
#define	SDRAM1X_RA_13		30
#define	SDRAM1X_RA_12		29
#define	SDRAM1X_RA_12_8bk	27
#define SDRAM1X_CA_12		16
#define SDRAM1X_CA_11		15
#define SDRAM1X_CA_09		12
#define SDRAM1X_CA_09_8bk	12
#define SDRAM1X_BA1		14
#define SDRAM1X_BA2_8bk		15
#define SDRAM1X_BA1_8bk		14
#endif

#define	MA_Column	0x06
#define	MA_Bank		0x08
#define	MA_Row		0x30
#define	MA_4_Bank	0x00
#define	MA_8_Bank	0x08
#define	MA_12_Row	0x00
#define	MA_13_Row	0x10
#define	MA_14_Row	0x20
#define	MA_15_Row	0x30
#define	MA_9_Column	0x00
#define	MA_10_Column	0x02
#define	MA_11_Column	0x04
#define	MA_12_Column	0x06

#define	GET_SPD(i, val, tmp, reg)								\
	do{											\
		val = 0;									\
		tmp = 0;									\
		for(i = 0; i < 2; i++)	{							\
			if(pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_REG_BASE + (i << 1)))) {	\
				tmp = get_spd_data(ctrl, i, reg);				\
				if(tmp > val)							\
					val = tmp;						\
			}									\
		}										\
	} while ( 0 )

#define REGISTERPRESET(bus,dev,fun,bdfspec) \
	{ u8 i, reg; \
		for (i=0; i<(sizeof((bdfspec))/sizeof(struct regmask)); i++) { \
			printk(BIOS_DEBUG, "Writing bus " #bus " dev " #dev " fun " #fun " register "); \
			printk(BIOS_DEBUG, "%02x", (bdfspec)[i].reg); \
			printk(BIOS_DEBUG, "\n"); \
			reg = pci_read_config8(PCI_DEV((bus), (dev), (fun)), (bdfspec)[i].reg); \
			reg &= (bdfspec)[i].mask; \
			reg |= (bdfspec)[i].val; \
			pci_write_config8(PCI_DEV((bus), (dev), (fun)), (bdfspec)[i].reg, reg); \
		} \
	}


static void do_ram_command(const struct mem_controller *ctrl, u8 command)
{
	u8 reg;

	reg = pci_read_config8(MEMCTRL, 0x6b);
	reg &= 0xf8;		/* Clear bits 2-0. */
	reg |= command;
	pci_write_config8(MEMCTRL, 0x6b, reg);

	PRINTK_DEBUG("    Sending RAM command 0x%02x\n", reg);
}

// TODO factor out to another file
static void c7_cpu_setup(const struct mem_controller *ctrl)
{
	u8 size, i;
	size = sizeof(Reg_Val) / sizeof(Reg_Val[0]);
	for (i = 0; i < size; i += 2)
		pci_write_config8(HOSTCTRL, Reg_Val[i], Reg_Val[i + 1]);
}

static void ddr_detect(const struct mem_controller *ctrl)
{
	/* FIXME: Only supports 2 ranks per DIMM */
	u8 val, rsize, dimm;
	u8 nrank = 0;
	u8 ndimm = 0;
	u8 rmap = 0;
	for (dimm = 0; dimm < DIMM_SOCKETS; dimm++) {
		val = get_spd_data(ctrl, dimm, 0);
		if ((val == 0x80) || (val == 0xff)) {
			ndimm++;
			rsize = get_spd_data(ctrl, dimm, SPD_RANK_SIZE);
			/* unit is 128M */
			rsize = (rsize << 3) | (rsize >> 5);
			val =
			    get_spd_data(ctrl, dimm,
					 SPD_MOD_ATTRIB_RANK) & SPD_MOD_ATTRIB_RANK_NUM_MASK;
			switch (val) {
			case 1:
				pci_write_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_1 + (dimm << 1)),
						  rsize);
				rmap |= (1 << ((dimm << 1) + 1));
				nrank++;
			case 0:
				pci_write_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + (dimm << 1)),
						  rsize);
				rmap |= (1 << (dimm << 1));
				nrank++;
			}
		}
	}
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_DIMM_NUM, ndimm);
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_NUM, nrank);
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_MAP, rmap);
}

static void sdram_set_safe_values(const struct mem_controller *ctrl)
{
	/* The purpose of this function is to set initial values for the dram
	 * size and timings. It will be replaced with the SPD based function
	 * once the RAM commands are working with these values.
	 */
	u8 regs, val, t, dimm;
	u32 spds, tmp;

	regs = pci_read_config8(MEMCTRL, 0x6c);
	if (regs & (1 << 6))
		printk(BIOS_DEBUG, "DDR2 Detected.\n");
	else
		die("ERROR: DDR1 memory detected but not supported by coreboot.\n");

	/* Enable DDR2 */
	regs |= (1 << 7);
	pci_write_config8(MEMCTRL, 0x6c, regs);

	/* SPD 5 # of ranks */
	pci_write_config8(MEMCTRL, 0x6d, 0xc0);

	/**********************************************/
	/*          Set DRAM Freq (DDR2 533)          */
	/**********************************************/
	/* SPD 9 SDRAM Cycle Time */
	GET_SPD(dimm, spds, regs, 9);

	printk(BIOS_DEBUG, "\nDDRII ");
	if (spds <= 0x3d) {
		printk(BIOS_DEBUG, "533");
		val = DDRII_533;
		t = 38;
	} else if (spds <= 0x50) {
		printk(BIOS_DEBUG, "400");
		val = DDRII_400;
		t = 50;
	} else if (spds <= 0x60) {
		printk(BIOS_DEBUG, "333");
		val = DDRII_333;
		t = 60;
	} else if (spds <= 0x75) {
		printk(BIOS_DEBUG, "266");
		val = DDRII_266;
		t = 75;
	} else {
		printk(BIOS_DEBUG, "200");
		val = DDRII_200;
		t = 100;
	}
	/* To store DDRII frequence */
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_FREQ, val);

	/* Manual reset and adjust DLL when DRAM change frequency 
	 * This is a necessary sequence.
	 */
	udelay(2000);
	regs = pci_read_config8(MEMCTRL, 0x90);
	regs |= 0x7;
	pci_write_config8(MEMCTRL, 0x90, regs);
	udelay(2000);
	regs = pci_read_config8(MEMCTRL, 0x90);
	regs &= ~0x7;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x90, regs);
	udelay(2000);
	regs = pci_read_config8(MEMCTRL, 0x6b);
	regs |= 0xc0;
	regs &= ~0x10;
	pci_write_config8(MEMCTRL, 0x6b, regs);
	udelay(1);
	regs |= 0x10;
	pci_write_config8(MEMCTRL, 0x6b, regs);
	udelay(1);
	regs &= ~0xc0;
	pci_write_config8(MEMCTRL, 0x6b, regs);
	regs = pci_read_config8(MEMCTRL, 0x6f);
	regs |= 0x1;
	pci_write_config8(MEMCTRL, 0x6f, regs);

	/**********************************************/
	/*      Set DRAM Timing Setting (DDR2 533)    */
	/**********************************************/
	/* SPD 9 18 23 25 CAS Latency NB3DRAM_REG62[2:0] */
	/* Read SPD byte 18 CAS Latency */
	GET_SPD(dimm, spds, regs, SPD_CAS_LAT);
	printk(BIOS_DEBUG, "\nCAS Supported ");
	if (spds & SPD_CAS_LAT_2)
		printk(BIOS_DEBUG, "2 ");
	if (spds & SPD_CAS_LAT_3)
		printk(BIOS_DEBUG, "3 ");
	if (spds & SPD_CAS_LAT_4)
		printk(BIOS_DEBUG, "4 ");
	if (spds & SPD_CAS_LAT_5)
		printk(BIOS_DEBUG, "5 ");
	if (spds & SPD_CAS_LAT_6)
		printk(BIOS_DEBUG, "6");

	/* We don't consider CAS = 6, because CX700 doesn't support it */
	printk(BIOS_DEBUG, "\n CAS:");
	if (spds & SPD_CAS_LAT_5) {
		printk(BIOS_DEBUG, "Starting at CL5");
		val = 0x3;
		/* See whether we can improve it */
		GET_SPD(dimm, tmp, regs, SPD_CAS_LAT_MIN_X_1);
		if ((spds & SPD_CAS_LAT_4) && (tmp < 0x50)) {
			printk(BIOS_DEBUG, "\n... going to CL4");
			val = 0x2;
		}
		GET_SPD(dimm, tmp, regs, SPD_CAS_LAT_MIN_X_2);
		if ((spds & SPD_CAS_LAT_3) && (tmp < 0x50)) {
			printk(BIOS_DEBUG, "\n... going to CL3");
			val = 0x1;
		}
	} else {
		printk(BIOS_DEBUG, "Starting at CL4");
		val = 0x2;
		GET_SPD(dimm, tmp, regs, SPD_CAS_LAT_MIN_X_1);
		if ((spds & SPD_CAS_LAT_3) && (tmp < 0x50)) {
			printk(BIOS_DEBUG, "\n... going to CL3");
			val = 0x1;
		}
		GET_SPD(dimm, tmp, regs, SPD_CAS_LAT_MIN_X_2);
		if ((spds & SPD_CAS_LAT_2) && (tmp < 0x50)) {
			printk(BIOS_DEBUG, "\n... going to CL2");
			val = 0x0;
		}
	}
	regs = pci_read_config8(MEMCTRL, 0x62);
	regs &= ~0x7;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x62, regs);

	/* SPD 27 Trp NB3DRAM_REG64[3:2] */
	GET_SPD(dimm, spds, regs, SPD_TRP);
	printk(BIOS_DEBUG, "\nTrp %d", spds);
	spds >>= 2;
	for (val = 2; val <= 5; val++) {
		if (spds <= (val * t / 10)) {
			val = val - 2;
			break;
		}
	}
	val <<= 2;
	regs = pci_read_config8(MEMCTRL, 0x64);
	regs &= ~0xc;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x64, regs);

	/* SPD 29 Trcd NB3DRAM_REG64[7:6] */
	GET_SPD(dimm, spds, regs, SPD_TRCD);
	printk(BIOS_DEBUG, "\nTrcd %d", spds);
	spds >>= 2;
	for (val = 2; val <= 5; val++) {
		if (spds <= (val * t / 10)) {
			val = val - 2;
			break;
		}
	}
	val <<= 6;
	regs = pci_read_config8(MEMCTRL, 0x64);
	regs &= ~0xc0;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x64, regs);

	/* SPD 30 Tras NB3DRAM_REG62[7:4] */
	GET_SPD(dimm, spds, regs, SPD_TRAS);
	printk(BIOS_DEBUG, "\nTras %d", spds);
	for (val = 5; val <= 20; val++) {
		if (spds <= (val * t / 10)) {
			val = val - 5;
			break;
		}
	}
	val <<= 4;
	regs = pci_read_config8(MEMCTRL, 0x62);
	regs &= ~0xf0;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x62, regs);

	/* SPD 42 SPD 40 Trfc NB3DRAM_REG61[5:0] */
	GET_SPD(dimm, spds, regs, SPD_TRFC);
	printk(BIOS_DEBUG, "\nTrfc %d", spds);
	tmp = spds;
	GET_SPD(dimm, spds, regs, SPD_EX_TRC_TRFC);
	if (spds & 0x1)
		tmp += 256;
	if (spds & 0xe)
		tmp++;
	for (val = 8; val <= 71; val++) {
		if (tmp <= (val * t / 10)) {
			val = val - 8;
			break;
		}
	}
	regs = pci_read_config8(MEMCTRL, 0x61);
	regs &= ~0x3f;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x61, regs);

	/* SPD 28 Trrd NB3DRAM_REG63[7:6] */
	GET_SPD(dimm, spds, regs, SPD_TRRD);
	for (val = 2; val <= 5; val++) {
		if (spds <= (val * t / 10)) {
			val = val - 2;
			break;
		}
	}
	val <<= 6;
	printk(BIOS_DEBUG, "\nTrrd val = 0x%x", val);
	regs = pci_read_config8(MEMCTRL, 0x63);
	regs &= ~0xc0;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x63, regs);

	/* SPD 36 Twr NB3DRAM_REG61[7:6] */
	GET_SPD(dimm, spds, regs, SPD_TWR);
	for (val = 2; val <= 5; val++) {
		if (spds <= (val * t / 10)) {
			val = val - 2;
			break;
		}
	}
	val <<= 6;
	printk(BIOS_DEBUG, "\nTwr val = 0x%x", val);

	regs = pci_read_config8(MEMCTRL, 0x61);
	regs &= ~0xc0;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x61, regs);

	/* SPD 37 Twtr NB3DRAM_REG63[1] */
	GET_SPD(dimm, spds, regs, SPD_TWTR);
	spds >>= 2;
	printk(BIOS_DEBUG, "\nTwtr 0x%x", spds);
	if (spds <= (t * 2 / 10))
		val = 0;
	else
		val = 1;
	val <<= 1;
	printk(BIOS_DEBUG, "\nTwtr val = 0x%x", val);

	regs = pci_read_config8(MEMCTRL, 0x63);
	regs &= ~0x2;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x63, regs);

	/* SPD 38 Trtp NB3DRAM_REG63[3] */
	GET_SPD(dimm, spds, regs, SPD_TRTP);
	spds >>= 2;
	printk(BIOS_DEBUG, "\nTrtp 0x%x", spds);
	if (spds <= (t * 2 / 10))
		val = 0;
	else
		val = 1;
	val <<= 3;
	printk(BIOS_DEBUG, "\nTrtp val = 0x%x", val);

	regs = pci_read_config8(MEMCTRL, 0x63);
	regs &= ~0x8;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x63, regs);

	/**********************************************/
	/*           Set DRAM DRDY Setting            */
	/**********************************************/
	/* Write slowest value to register */
	tmp = sizeof(Host_Reg_Val) / sizeof(Host_Reg_Val[0]);
	for (val = 0; val < tmp; val += 2)
		pci_write_config8(HOSTCTRL, Host_Reg_Val[val], Host_Reg_Val[val + 1]);

	/* F2_RX51[7]=0, disable DRDY timing */
	regs = pci_read_config8(HOSTCTRL, 0x51);
	regs &= ~0x80;
	pci_write_config8(HOSTCTRL, 0x51, regs);

	/**********************************************/
	/*           Set DRAM BurstLength             */
	/**********************************************/
	regs = pci_read_config8(MEMCTRL, 0x6c);
	for (dimm = 0; dimm < DIMM_SOCKETS; dimm++) {
		if (pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_REG_BASE + (dimm << 1)))) {
			spds = get_spd_data(ctrl, dimm, 16);
			if (!(spds & 0x8))
				break;
		}
	}
	if (dimm == 2)
		regs |= 0x8;
	pci_write_config8(MEMCTRL, 0x6c, regs);
	val = pci_read_config8(HOSTCTRL, 0x54);
	val &= ~0x10;
	if (dimm == 2)
		val |= 0x10;
	pci_write_config8(HOSTCTRL, 0x54, val);

	/**********************************************/
	/*          Set DRAM Driving Setting          */
	/**********************************************/
	/* DRAM Timing ODT */
	tmp = sizeof(Dram_Driving_ODT_CTRL) / sizeof(Dram_Driving_ODT_CTRL[0]);
	for (val = 0; val < tmp; val += 2)
		pci_write_config8(MEMCTRL, Dram_Driving_ODT_CTRL[val],
				  Dram_Driving_ODT_CTRL[val + 1]);

	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_NUM);
	val = pci_read_config8(MEMCTRL, 0xd5);
	val &= ~0xaa;
	switch (regs) {
	case 3:
	case 2:
		val |= 0xa0;
		break;
	default:
		val |= 0x80;
	}
	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DIMM_NUM);
	if (regs == 1)
		val |= 0xa;
	pci_write_config8(MEMCTRL, 0xd5, val);

	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DIMM_NUM);
	val = pci_read_config8(MEMCTRL, 0xd6);
	val &= ~0x2;
	if (regs == 1)
		val |= 0x2;
	pci_write_config8(MEMCTRL, 0xd6, val);

	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_MAP);
	tmp = sizeof(ODT_TBL) / sizeof(ODT_TBL[0]);
	for (val = 0; val < tmp; val += 3) {
		if (regs == ODT_TBL[val]) {
			pci_write_config8(MEMCTRL, 0xd8, ODT_TBL[val + 1]);
			/* Store DRAM & NB ODT setting in d0f4_Rxd8 */
			pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_NB_ODT, ODT_TBL[val + 2]);
			break;
		}
	}

	pci_write_config8(MEMCTRL, 0xd9, 0x0a);
	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_NUM);
	regs--;
	regs = regs << 1;
	pci_write_config8(MEMCTRL, 0xe0, DQS_DQ_TBL[regs++]);
	pci_write_config8(MEMCTRL, 0xe2, DQS_DQ_TBL[regs]);

	/* DRAM Timing CS */
	pci_write_config8(MEMCTRL, 0xe4, 0x66);

	/* DRAM Timing MAA */
	val = 0;
	for (dimm = 0; dimm < DIMM_SOCKETS; dimm++) {
		if (pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_REG_BASE + (dimm << 1)))) {
			spds = get_spd_data(ctrl, dimm, SPD_PRI_WIDTH);
			spds = 64 / spds;
			if (pci_read_config8
			    (PCI_DEV(0, 0, 4), (SCRATCH_REG_BASE + (dimm << 1) + 1)))
				spds = spds << 1;
			val += spds;
		}
	}
	printk(BIOS_DEBUG, "\nchip #%d", val);
	if (val > 18)
		regs = 0xdb;
	else
		regs = 0x86;
	pci_write_config8(MEMCTRL, 0xe8, regs);

	/* DRAM Timing MAB */
	pci_write_config8(MEMCTRL, 0xe9, 0x0);

	/* DRAM Timing DCLK VT8454C always 0x66 */
	pci_write_config8(MEMCTRL, 0xe6, 0xaa);

	/**********************************************/
	/*            Set DRAM Duty Control           */
	/**********************************************/
	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_NUM);
	switch (regs) {
	case 1:
	case 2:		/* 1~2 rank */
		val = 0;
		break;
	case 3:
	case 4:		/* 3~4 rank */
		regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_FREQ);
		if (regs == DDRII_533)
			val = 4;
		else		/* DDRII-400 */
			val = 0;
		break;
	}
	regs = 0xec;
	for (t = 0; t < 4; t++) {
		pci_write_config8(MEMCTRL, regs, Duty_Control_DDR2[val]);
		regs++;
		val++;
	}

	/**********************************************/
	/*            Set DRAM Clock Control          */
	/**********************************************/
	/* Write Data Phase */
	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_FREQ);
	regs = pci_read_config8(MEMCTRL, 0x75);
	regs &= 0xf0;
	switch (val) {
	case DDRII_533:
		pci_write_config8(MEMCTRL, 0x74, 0x07);
		regs |= 0x7;
		break;
	case DDRII_400:
	default:
		pci_write_config8(MEMCTRL, 0x74, 0x05);
		regs |= 0x5;
		break;
	}
	pci_write_config8(MEMCTRL, 0x75, regs);
	pci_write_config8(MEMCTRL, 0x76, 0x80);

	/* Clock Phase Control for FeedBack Mode */
	regs = pci_read_config8(MEMCTRL, 0x90);
//      regs |= 0x80;
	pci_write_config8(MEMCTRL, 0x90, regs);

	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_FREQ);
	switch (regs) {
	case DDRII_533:
		regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_NUM);
		if (regs == 1)
			val = 0;
		else
			val = 3;
		break;
	case DDRII_400:
	default:
		val = 6;
		break;
	}
	regs = pci_read_config8(MEMCTRL, 0x91);
	regs &= ~0xc0;
	regs |= 0x80;
	pci_write_config8(MEMCTRL, 0x91, regs);
	regs = 0x91;
	for (t = 0; t < 3; t++) {
		dimm = pci_read_config8(MEMCTRL, regs);
		dimm &= ~0x7;
		dimm |= ChA_Clk_Phase_DDR2_Table[val];
		pci_write_config8(MEMCTRL, regs, dimm);
		regs++;
		val++;
	}

	pci_write_config8(MEMCTRL, 0x97, 0x12);
	pci_write_config8(MEMCTRL, 0x98, 0x33);

	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_0);
	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_2);
	if (regs && val)
		pci_write_config8(MEMCTRL, 0x9d, 0x00);
	else
		pci_write_config8(MEMCTRL, 0x9d, 0x0f);

	tmp = sizeof(DQ_DQS_Table) / sizeof(DQ_DQS_Table[0]);
	for (val = 0; val < tmp; val += 2)
		pci_write_config8(MEMCTRL, DQ_DQS_Table[val], DQ_DQS_Table[val + 1]);
	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_FREQ);
	if (regs == DDRII_533)
		pci_write_config8(MEMCTRL, 0x7b, 0xa0);
	else
		pci_write_config8(MEMCTRL, 0x7b, 0x10);

	/***************************************************/
	/*  Set necessary register before DRAM initialize  */
	/***************************************************/
	tmp = sizeof(Mem_Reg_Init) / sizeof(Mem_Reg_Init[0]);
	for (val = 0; val < tmp; val += 3) {
		regs = pci_read_config8(MEMCTRL, Mem_Reg_Init[val]);
		regs &= Mem_Reg_Init[val + 1];
		regs |= Mem_Reg_Init[val + 2];
		pci_write_config8(MEMCTRL, Mem_Reg_Init[val], regs);
	}
	regs = pci_read_config8(HOSTCTRL, 0x51);
	regs &= 0xbf;		// Clear bit 6 Disable Read Around Write
	pci_write_config8(HOSTCTRL, 0x51, regs);

	regs = pci_read_config8(HOSTCTRL, 0x54);
	t = regs >> 5;
	val = pci_read_config8(HOSTCTRL, 0x57);
	dimm = val >> 5;
	if (t == dimm)
		t = 0x0;
	else
		t = 0x1;
	regs &= ~0x1;
	regs |= t;
	val &= ~0x1;
	val |= t;
	pci_write_config8(HOSTCTRL, 0x57, val);

	regs = pci_read_config8(HOSTCTRL, 0x51);
	regs |= t;
	pci_write_config8(HOSTCTRL, 0x51, regs);

	regs = pci_read_config8(MEMCTRL, 0x90);
	regs &= 0x7;
	val = 0;
	if (regs < 0x2)
		val = 0x80;
	regs = pci_read_config8(MEMCTRL, 0x76);
	regs &= 0x80;
	regs |= val;
	pci_write_config8(MEMCTRL, 0x76, regs);

	regs = pci_read_config8(MEMCTRL, 0x6f);
	regs |= 0x10;
	pci_write_config8(MEMCTRL, 0x6f, regs);

	/***************************************************/
	/*    Find suitable DQS value for ChA and ChB      */
	/***************************************************/
	// Set DQS output delay for Channel A
	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_FREQ);
	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_NUM);
	switch (regs) {
	case DDRII_533:
		if (val < 2)
			val = 0;
		else
			val = 2;
		break;
	case DDRII_400:
	default:
		if (val < 2)
			val = 4;
		else
			val = 6;
		break;
	}
	for (t = 0; t < 2; t++)
		pci_write_config8(MEMCTRL, (0x70 + t), DQSOChA_DDR2_Driving_Table[val + t]);
	// Set DQS output delay for Channel B
	pci_write_config8(MEMCTRL, 0x72, 0x0);

	regs = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_0);
	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_RANK_2);
	if (regs && val)
		pci_write_config8(MEMCTRL, 0x73, 0xfd);
	else
		pci_write_config8(MEMCTRL, 0x73, 0x01);
}

static void sdram_set_registers(const struct mem_controller *ctrl)
{
	c7_cpu_setup(ctrl);
	ddr_detect(ctrl);
	sdram_set_safe_values(ctrl);
}

static void step_20_21(const struct mem_controller *ctrl)
{
	u8 val;

	// Step 20
	udelay(200);

	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_NB_ODT);
	if (val & DDR2_ODT_150ohm)
		read32(0x102200);
	else
		read32(0x102020);

	/* Step 21. Normal operation */
	print_spew("RAM Enable 5: Normal operation\n");
	do_ram_command(ctrl, RAM_COMMAND_NORMAL);
	udelay(3);
}

static void step_2_19(const struct mem_controller *ctrl)
{
	u32 i;
	u8 val;

	//  Step 2
	val = pci_read_config8(MEMCTRL, 0x69);
	val &= ~0x03;
	pci_write_config8(MEMCTRL, 0x69, val);

	/* Step 3 Apply NOP. */
	print_spew("RAM Enable 1: Apply NOP\n");
	do_ram_command(ctrl, RAM_COMMAND_NOP);

	udelay(15);

	// Step 4
	print_spew("SEND: ");
	read32(0);
	print_spew("OK\n");

	// Step 5
	udelay(400);

	/* 6. Precharge all. Wait tRP. */
	print_spew("RAM Enable 2: Precharge all\n");
	do_ram_command(ctrl, RAM_COMMAND_PRECHARGE);

	// Step 7
	print_spew("SEND: ");
	read32(0);
	print_spew("OK\n");

	/* Step 8. Mode register set. */
	print_spew("RAM Enable 4: Mode register set\n");
	do_ram_command(ctrl, RAM_COMMAND_MRS);	//enable dll

	// Step 9
	print_spew("SEND: ");

	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_NB_ODT);
	if (val & DDR2_ODT_150ohm)
		read32(0x102200);	//DDR2_ODT_150ohm
	else
		read32(0x102020);
	print_spew("OK\n");

	// Step 10
	print_spew("SEND: ");
	read32(0x800);
	print_spew("OK\n");

	/* Step 11. Precharge all. Wait tRP. */
	print_spew("RAM Enable 2: Precharge all\n");
	do_ram_command(ctrl, RAM_COMMAND_PRECHARGE);

	// Step 12
	print_spew("SEND: ");
	read32(0x0);
	print_spew("OK\n");

	/* Step 13. Perform 8 refresh cycles. Wait tRC each time. */
	print_spew("RAM Enable 3: CBR\n");
	do_ram_command(ctrl, RAM_COMMAND_CBR);

	/* JEDEC says only twice, do 8 times for posterity */
	// Step 16: Repeat Step 14 and 15 another 7 times
	for (i = 0; i < 8; i++) {
		// Step 14
		read32(0);
		print_spew(".");

		// Step 15
		udelay(100);
	}

	/* Step 17. Mode register set. Wait 200us. */
	print_spew("\nRAM Enable 4: Mode register set\n");

	//safe value for now, BL=8, WR=4, CAS=4
	do_ram_command(ctrl, RAM_COMMAND_MRS);
	udelay(200);

	/* Use Single Chanel temporarily */
	val = pci_read_config8(MEMCTRL, 0x6c);
	if (val & 0x8) {	/* Burst Length = 8 */
		val = pci_read_config8(MEMCTRL, 0x62);
		val &= 0x7;
		i = DDR2_MRS_table[4 + val];
	} else {
		val = pci_read_config8(MEMCTRL, 0x62);
		val &= 0x7;
		i = DDR2_MRS_table[val];
	}

	// Step 18
	val = pci_read_config8(MEMCTRL, 0x61);
	val = val >> 6;
	i |= DDR2_Twr_table[val];
	read32(i);

	printk(BIOS_DEBUG, "MRS = %08x\n", i);

	udelay(15);

	// Step 19
	val = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_NB_ODT);
	if (val & DDR2_ODT_150ohm)
		read32(0x103e00);	//EMRS OCD Default
	else
		read32(0x103c20);
}

static void sdram_set_vr(const struct mem_controller *ctrl, u8 num)
{
	u8 reg, val;
	val = 0x54 + (num >> 1);
	reg = pci_read_config8(MEMCTRL, val);
	reg &= (0xf << (4 * (num & 0x1)));
	reg |= (((0x8 | num) << 4) >> (4 * (num & 0x1)));
	pci_write_config8(MEMCTRL, val, reg);
}
static void sdram_ending_addr(const struct mem_controller *ctrl, u8 num)
{
	u8 reg, val;
	/* Set Ending Address */
	val = 0x40 + num;
	reg = pci_read_config8(MEMCTRL, val);
	reg += 0x10;
	pci_write_config8(MEMCTRL, val, reg);
	/* Set Beginning Address */
	val = 0x48 + num;
	pci_write_config8(MEMCTRL, val, 0x0);
}

static void sdram_clear_vr_addr(const struct mem_controller *ctrl, u8 num)
{
	u8 reg, val;
	val = 0x54 + (num >> 1);
	reg = pci_read_config8(MEMCTRL, val);
	reg = ~(0x80 >> (4 * (num & 0x1)));
	pci_write_config8(MEMCTRL, val, reg);
	val = 0x40 + num;
	reg = pci_read_config8(MEMCTRL, val);
	reg -= 0x10;
	pci_write_config8(MEMCTRL, val, reg);
	val = 0x48 + num;
	pci_write_config8(MEMCTRL, val, 0x0);
}

/* Perform sizing DRAM by dynamic method */
static void sdram_calc_size(const struct mem_controller *ctrl, u8 num)
{
	u8 ca, ra, ba, reg;
	ba = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_FLAGS);
	if (ba == 8) {
		write8(0, 0x0d);
		ra = read8(0);
		write8((1 << SDRAM1X_RA_12_8bk), 0x0c);
		ra = read8(0);

		write8(0, 0x0a);
		ca = read8(0);
		write8((1 << SDRAM1X_CA_09_8bk), 0x0c);
		ca = read8(0);

		write8(0, 0x03);
		ba = read8(0);
		write8((1 << SDRAM1X_BA2_8bk), 0x02);
		ba = read8(0);
		write8((1 << SDRAM1X_BA1_8bk), 0x01);
		ba = read8(0);
	} else {
		write8(0, 0x0f);
		ra = read8(0);
		write8((1 << SDRAM1X_RA_14), 0x0e);
		ra = read8(0);
		write8((1 << SDRAM1X_RA_13), 0x0d);
		ra = read8(0);
		write8((1 << SDRAM1X_RA_12), 0x0c);
		ra = read8(0);

		write8(0, 0x0c);
		ca = read8(0);
		write8((1 << SDRAM1X_CA_12), 0x0b);
		ca = read8(0);
		write8((1 << SDRAM1X_CA_11), 0x0a);
		ca = read8(0);
		write8((1 << SDRAM1X_CA_09), 0x09);
		ca = read8(0);

		write8(0, 0x02);
		ba = read8(0);
		write8((1 << SDRAM1X_BA1), 0x01);
		ba = read8(0);
	}

	if (ra < 10 || ra > 15)
		die("bad RA");
	if (ca < 8 || ca > 12)
		die("bad CA");
	if (ba < 1 || ba > 3)
		die("bad BA");

	/* Calculate MA type save to scratch register */
	reg = 0;

	switch (ra) {
	case 12:
		reg |= MA_12_Row;
		break;
	case 13:
		reg |= MA_13_Row;
		break;
	case 14:
		reg |= MA_14_Row;
		break;
	default:
		reg |= MA_15_Row;
	}

	switch (ca) {
	case 9:
		reg |= MA_9_Column;
		break;
	case 10:
		reg |= MA_10_Column;
		break;
	case 11:
		reg |= MA_11_Column;
		break;
	default:
		reg |= MA_12_Column;
	}

	switch (ba) {
	case 3:
		reg |= MA_8_Bank;
		break;
	default:
		reg |= MA_4_Bank;
	}

	pci_write_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK0_MA_REG + num), reg);

	if (ra >= 13)
		pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_256M_BIT, 1);

	/* Calculate rank size save to scratch register */
	ra = ra + ca + ba + 3 - 26;	/* 1 unit = 64M */
	ra = 1 << ra;
	pci_write_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK0_SIZE_REG + num), ra);
}

static void sdram_enable(const struct mem_controller *ctrl)
{
	u8 reg8;
	u8 val, i;
	device_t dev;
	u8 dl, dh;
	u32 quot;

	/* Init Present Bank */
	val = sizeof(Init_Rank_Reg_Table) / sizeof(Init_Rank_Reg_Table[0]);
	for (i = 0; i < val; i++)
		pci_write_config8(MEMCTRL, Init_Rank_Reg_Table[i], 0x0);

	/* Init other banks */
	for (i = 0; i < 4; i++) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8) {
			sdram_set_vr(ctrl, i);
			sdram_ending_addr(ctrl, i);
			step_2_19(ctrl);
			step_20_21(ctrl);
			sdram_clear_vr_addr(ctrl, i);
		}
	}

#ifdef MEM_WIDTH_32BIT_MODE
	/****************************************************************/
	/*                      Set Dram 32bit Mode                     */
	/****************************************************************/
	reg8 = pci_read_config8(MEMCTRL, 0x6c);
	reg8 |= 0x20;
	pci_write_config(MEMCTRL, 0x6c, reg8);
#endif

	/****************************************************************/
	/* Find the DQSI Low/High bound and save it to Scratch register */
	/****************************************************************/
	for (dl = 0; dl < 0x3f; dl += 2) {
		reg8 = dl & 0x3f;
		reg8 |= 0x80;	/* Set Manual Mode */
		pci_write_config8(MEMCTRL, 0x77, reg8);
		for (i = 0; i < 4; i++) {
			reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
			if (reg8) {
				sdram_set_vr(ctrl, i);
				sdram_ending_addr(ctrl, i);
				write32(0, 0x55555555);
				write32(4, 0x55555555);
				udelay(15);
				if (read32(0) != 0x55555555)
					break;
				if (read32(4) != 0x55555555)
					break;
				write32(0, 0xaaaaaaaa);
				write32(4, 0xaaaaaaaa);
				udelay(15);
				if (read32(0) != 0xaaaaaaaa)
					break;
				if (read32(4) != 0xaaaaaaaa)
					break;
				sdram_clear_vr_addr(ctrl, i);
			}
		}
		if (i == 4)
			break;
		else
			sdram_clear_vr_addr(ctrl, i);
	}
	printk(BIOS_DEBUG, "\nDQSI Low %08x", dl);
	for (dh = dl; dh < 0x3f; dh += 2) {
		reg8 = dh & 0x3f;
		reg8 |= 0x80;	/* Set Manual Mode */
		pci_write_config8(MEMCTRL, 0x77, reg8);
		for (i = 0; i < 4; i++) {
			reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
			if (reg8) {
				sdram_set_vr(ctrl, i);
				sdram_ending_addr(ctrl, i);

				write32(0, 0x55555555);
				write32(4, 0x55555555);
				udelay(15);
				if (read32(0) != 0x55555555)
					break;
				if (read32(4) != 0x55555555)
					break;
				write32(0, 0xaaaaaaaa);
				write32(4, 0xaaaaaaaa);
				udelay(15);
				if (read32(0) != 0xaaaaaaaa)
					break;
				if (read32(4) != 0xaaaaaaaa)
					break;
				sdram_clear_vr_addr(ctrl, i);
			}
		}
		if (i != 4) {
			sdram_clear_vr_addr(ctrl, i);
			break;
		}
	}
	printk(BIOS_DEBUG, "\nDQSI High %02x", dh);
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_CHA_DQSI_LOW_REG, dl);
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_CHA_DQSI_HIGH_REG, dh);
	reg8 = pci_read_config8(MEMCTRL, 0X90) & 0X7;
	val = DQSI_Rate_Table[reg8];
	quot = dh - dl;
	quot = quot * val;
	quot >>= 4;
	val = quot + dl;
	pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_ChA_DQSI_REG, val);
	reg8 = val & 0x3f;
	reg8 |= 0x80;
	pci_write_config8(MEMCTRL, 0x77, reg8);

	/****************************************************************/
	/*     Find out the lowest Bank Interleave and Set Register     */
	/****************************************************************/
#if 0
	//TODO
	reg8 = pci_read_config8(MEMCTRL, 0x69);
	reg8 &= ~0xc0;
	reg8 |= 0x80;		//8 banks
	pci_write_config8(MEMCTRL, 0x69, reg8);
#endif
	dl = 2;
	for (i = 0; i < 4; i++) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8) {
			reg8 = get_spd_data(ctrl, (i >> 1), 17);
			sdram_set_vr(ctrl, i);
			sdram_ending_addr(ctrl, i);
			if (reg8 == 4) {
				write8(0, 0x02);
				val = read8(0);
				write8((1 << SDRAM1X_BA1), 0x01);
				val = read8(0);
			} else {
				write8(0, 0x03);
				val = read8(0);
				write8((1 << SDRAM1X_BA2_8bk), 0x02);
				val = read8(0);
				write8((1 << SDRAM1X_BA1_8bk), 0x01);
				val = read8(0);
			}
			if (val < dl)
				dl = val;
			sdram_clear_vr_addr(ctrl, i);
		}
	}
	dl <<= 6;
	reg8 = pci_read_config8(MEMCTRL, 0x69);
	reg8 &= ~0xc0;
	reg8 |= dl;
	pci_write_config8(MEMCTRL, 0x69, reg8);

	/****************************************************************/
	/*               DRAM Sizing and Fill MA type                   */
	/****************************************************************/
	for (i = 0; i < 4; i++) {
		val = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (val) {
			reg8 = get_spd_data(ctrl, (i >> 1), 17);
			pci_write_config8(PCI_DEV(0, 0, 4), SCRATCH_FLAGS, reg8);
			if (reg8 == 4) {
				/* Use MA Type 3 for DRAM sizing */
				reg8 = pci_read_config8(MEMCTRL, 0x50);
				reg8 &= 0x11;
				reg8 |= 0x66;
				pci_write_config8(MEMCTRL, 0x50, reg8);
				pci_write_config8(MEMCTRL, 0x51, reg8);
			} else {
				/* Use MA Type 5 for DRAM sizing */
				reg8 = pci_read_config8(MEMCTRL, 0x50);
				reg8 &= 0x11;
				reg8 |= 0xaa;
				pci_write_config8(MEMCTRL, 0x50, reg8);
				pci_write_config8(MEMCTRL, 0x51, reg8);
				reg8 = pci_read_config8(MEMCTRL, 0x53);
				reg8 &= 0x0f;
				reg8 |= 0x90;
				pci_write_config8(MEMCTRL, 0x53, reg8);
			}
			sdram_set_vr(ctrl, i);
			val = 0x40 + i;
			reg8 = pci_read_config8(MEMCTRL, val);
			/* max size 3G for new MA table */
			reg8 += 0x30;
			pci_write_config8(MEMCTRL, val, reg8);
			/* Set Beginning Address */
			val = 0x48 + i;
			pci_write_config8(MEMCTRL, val, 0x0);

			sdram_calc_size(ctrl, i);

			/* Clear */
			val = 0x54 + (i >> 1);
			reg8 = pci_read_config8(MEMCTRL, val);
			reg8 = ~(0x80 >> (4 * (i & 0x1)));
			pci_write_config8(MEMCTRL, val, reg8);
			val = 0x40 + i;
			reg8 = pci_read_config8(MEMCTRL, val);
			reg8 -= 0x30;
			pci_write_config8(MEMCTRL, val, reg8);
			val = 0x48 + i;
			pci_write_config8(MEMCTRL, val, 0x0);

		}
	}
	/* Clear MA Type */
	reg8 = pci_read_config8(MEMCTRL, 0x50);
	reg8 &= 0x11;
	pci_write_config8(MEMCTRL, 0x50, reg8);
	pci_write_config8(MEMCTRL, 0x51, reg8);
	reg8 = pci_read_config8(MEMCTRL, 0x6b);
	reg8 &= ~0x08;
	pci_write_config8(MEMCTRL, 0x6b, reg8);

	/****************************************************************/
	/*             DRAM re-initialize for burst length              */
	/****************************************************************/
	for (i = 0; i < 4; i++) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8) {
			sdram_set_vr(ctrl, i);
			sdram_ending_addr(ctrl, i);
			step_2_19(ctrl);
			step_20_21(ctrl);
			sdram_clear_vr_addr(ctrl, i);
		}
	}

	/****************************************************************/
	/*                    Set the MA Type                           */
	/****************************************************************/
	reg8 = pci_read_config8(MEMCTRL, 0x50);
	reg8 &= 0x11;
	pci_write_config8(MEMCTRL, 0x50, reg8);

	reg8 = pci_read_config8(MEMCTRL, 0x51);
	reg8 &= 0x11;
	pci_write_config8(MEMCTRL, 0x51, reg8);

	reg8 = pci_read_config8(MEMCTRL, 0x6b);
	reg8 &= ~0x08;
	pci_write_config8(MEMCTRL, 0x6b, reg8);

	for (i = 0; i < 4; i += 2) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8) {
			reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK0_MA_REG + i));
			reg8 &= (MA_Bank + MA_Column);
			val = pci_read_config8(MEMCTRL, 0x50);
			if (i == 0) {
				reg8 <<= 4;
				val &= 0x1f;
			} else
				val &= 0xf1;
			val |= reg8;
			pci_write_config8(MEMCTRL, 0x50, val);
		}
	}

	/****************************************************************/
	/*                 Set Start and Ending Address                 */
	/****************************************************************/
	dl = 0;			/* Begin Address */
	dh = 0;			/* Ending Address */
	for (i = 0; i < 4; i++) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8) {
			reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK0_SIZE_REG + i));
			if (reg8 == 0)
				continue;
			dh += reg8;
			pci_write_config8(MEMCTRL, (0x40 + i), dh);
			pci_write_config8(MEMCTRL, (0x48 + i), dl);
			dl = dh;
		}
	}
	dh <<= 2;
	// F7_Rx57 Ending address mirror register
	pci_write_config8(PCI_DEV(0, 0, 7), 0x57, dh);
	dev = pci_locate_device(PCI_ID(0x1106, 0x324e), 0);
	pci_write_config8(dev, 0x57, dh);
	// LOW TOP Address
	pci_write_config8(MEMCTRL, 0x88, dh);
	pci_write_config8(MEMCTRL, 0x85, dh);
	// also program vlink mirror
	pci_write_config8(PCI_DEV(0, 0, 7), 0xe5, dh);

	/****************************************************************/
	/*            Set Physical to Virtual Rank mapping              */
	/****************************************************************/
	pci_write_config32(MEMCTRL, 0x54, 0x0);
	for (i = 0; i < 4; i++) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8) {
			reg8 = pci_read_config8(MEMCTRL, (0x54 + (i >> 1)));
			if (i & 0x1) {	/* Odd Rank */
				reg8 &= 0xf0;
				reg8 |= (0x8 | i);
			} else {	/* Even Rank */

				reg8 &= 0x0f;
				reg8 |= ((0x8 | i) << 4);
			}
			pci_write_config8(MEMCTRL, (0x54 + (i >> 1)), reg8);
		}
	}

	/****************************************************************/
	/*                   Set DRAM Refresh Counter                   */
	/****************************************************************/
	val = pci_read_config8(MEMCTRL, 0X90) & 0X7;
	val <<= 1;
	reg8 = pci_read_config8(PCI_DEV(0, 0, 4), SCRATCH_DRAM_256M_BIT);
	if (reg8)
		val++;
	pci_write_config8(MEMCTRL, 0x6a, REFC_Table[val]);

	/****************************************************************/
	/*  Chipset Performance UP and other setting after DRAM Sizing  */
	/****************************************************************/
	/* Dram Registers */
	val = sizeof(Dram_Table) / sizeof(Dram_Table[0]);
	for (i = 0; i < val; i += 3) {
		reg8 = pci_read_config8(MEMCTRL, Dram_Table[i]);
		reg8 &= Dram_Table[i + 1];
		reg8 |= Dram_Table[i + 2];
		pci_write_config8(MEMCTRL, Dram_Table[i], reg8);
	}

	/* Host Registers */
	val = sizeof(Host_Table) / sizeof(Host_Table[0]);
	for (i = 0; i < val; i += 3) {
		reg8 = pci_read_config8(HOSTCTRL, Host_Table[i]);
		reg8 &= Host_Table[i + 1];
		reg8 |= Host_Table[i + 2];
		pci_write_config8(HOSTCTRL, Host_Table[i], reg8);
	}

	/* PM Registers */
#ifdef SETUP_PM_REGISTERS
	val = sizeof(PM_Table) / sizeof(PM_Table[0]);
	for (i = 0; i < val; i += 3) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), PM_Table[i]);
		reg8 &= PM_Table[i + 1];
		reg8 |= PM_Table[i + 2];
		pci_write_config8(PCI_DEV(0, 0, 4), PM_Table[i], reg8);
	}
#endif
	pci_write_config8(HOSTCTRL, 0x5d, 0xb2);

	/****************************************************************/
	/*              UMA registers for N-series projects             */
	/****************************************************************/

	/* Manual setting frame buffer bank */
	for (i = 0; i < 4; i++) {
		reg8 = pci_read_config8(PCI_DEV(0, 0, 4), (SCRATCH_RANK_0 + i));
		if (reg8)
			val = i;
	}
	pci_write_config8(MEMCTRL, 0xb0, val);
	reg8 = 0x40;		// Frame buffer size 64M
	reg8 |= 0x80;		// VGA Enable
	reg8 |= 0x0a;		// A[31:28] = 1010b
	pci_write_config8(MEMCTRL, 0xa1, reg8);

#ifdef ECC
	// Clear Ecc
	outl(0x80000180, 0xcf8);
	outb(0xff, 0xcfc);
	// Enable Ecc
	outl(0x80000188, 0xcf8);
	outb(0xcf, 0xcfc);

	reg8 = pci_read_config8(PCI_DEV(0, 0, 0), 0xa5);
	reg8 |= 0x10;
	pci_write_config8(PCI_DEV(0, 0, 0), 0xa5, reg8);

	reg8 = pci_read_config8(PCI_DEV(0, 0, 0), 0x91);
	reg8 |= 0x20;
	pci_write_config8(PCI_DEV(0, 0, 0), 0x91, reg8);
#endif

	static const struct regmask {
		u8 reg;
		u8 mask;
		u8 val;
	} b0d1f0[] = {
		{ 0x40, 0x00, 0x8b}, 
		{ 0x41, 0x80, 0x43},
		{ 0x42, 0x00, 0x62},
		{ 0x43, 0x00, 0x44},
		{ 0x44, 0x00, 0x34},
		{ 0x45, 0x00, 0x72}
	}, b0d0f3[] = {
		{ 0x53, 0xf0, 0x0f},
		{ 0x60, 0x00, 0x03},
		{ 0x65, 0x00, 0xd9},
		{ 0x66, 0x00, 0x80},
		{ 0x67, 0x00, 0x00},
		{ 0x68, 0x00, 0x01},
		{ 0x69, 0xe0, 0x03},
		{ 0x6b, 0x00, 0x10},
		{ 0x6c, 0xc1, 0x08},
		{ 0x6e, 0x00, 0x89},
		{ 0x6f, 0x00, 0x51},
		{ 0x75, ~0x40, 0x40},
		{ 0x76, 0x8f, 0x00},
		{ 0x7b, 0x00, 0xa0},
		{ 0x86, 0x01, 0x24},
		{ 0x86, 0x04, 0x29},
		{ 0x8c, 0x00, 0x00},
		{ 0x8d, 0x00, 0x00},
		{ 0x95, ~0x40, 0x00},
		{ 0xa2, 0x00, 0x44},
		{ 0xb1, 0x00, 0xaa}
	}, b0d0f0[] = {
		{ 0x4d, 0x00, 0x24},
		{ 0x4f, 0x00, 0x01},
		{ 0xbc, 0x00, 0x21},
		{ 0xbe, 0x00, 0x00},
		{ 0xbf, 0x7f, 0x80}
	}, b0d17f0[] = {
		{ 0x40, ~0x01, 0x01},		// enable timer/counter shadow registers
		{ 0x67, ~0x03, 0x01},
		{ 0x5b, ~0x01, 0x00},
		{ 0x8d, ~0x02, 0x02},
		{ 0x97, ~0x80, 0x00},
		{ 0xd2, ~0x18, 0x00},
		{ 0xe2, ~0x36, 0x06},
		{ 0xe4, ~0x80, 0x00},
		{ 0xe5, 0x00, 0x40},
		{ 0xe6, 0x00, 0x20},
		{ 0xe7, ~0xd0, 0xc0},
		{ 0xec, ~0x08, 0x00}
	}, b0d17f7[] = {
		{ 0x4e, ~0x80, 0x80},
		{ 0x4f, ~(1 << 6), 1 << 6 },	/* PG_CX700: 14.1.1 enable P2P Bridge Header for External PCI Bus */
		{ 0x74, ~0x00, 0x04},		/* PG_CX700: 14.1.2 APIC FSB directly up to snmic, not on pci */
		{ 0x7c, ~0x00, 0x02},		/* PG_CX700: 14.1.1 APIC FSB directly up to snmic, not on pci */
		{ 0xe6, 0x0, 0x04}		// MSI post
	}, b0d19f0[] = {	/* P2PE */
		{ 0x42, ~0x08, 0x08},		// Disable HD Audio,
		{ 0x40, ~0xc0, 0x80}		// 14.1.3.1.1 of the PG: extended cfg mode for pcie. enable capability, but don't activate
	}, b0d0f2[] = {
		{ 0x50, ~0x40, 0x88},
		{ 0x51, 0x80, 0x7b},
		{ 0x52, 0x90, 0x6f},
		{ 0x53, 0x00, 0x88},
		{ 0x54, 0xe4, 0x16},
		{ 0x55, 0xf2, 0x04},
		{ 0x56, 0x0f, 0x00},
		{ 0x57, ~0x04, 0x00},
		{ 0x5d, 0x00, 0xb2},
		{ 0x5e, 0x00, 0x88},
		{ 0x5f, 0x00, 0xc7},
		{ 0x5c, 0x00, 0x01}
	};

	REGISTERPRESET(0, 0, 0, b0d0f0);
	REGISTERPRESET(0, 0, 2, b0d0f2);
	REGISTERPRESET(0, 0, 3, b0d0f3);
	REGISTERPRESET(0, 1, 0, b0d1f0);
	REGISTERPRESET(0, 17, 0, b0d17f0);
	REGISTERPRESET(0, 17, 7, b0d17f7);
	REGISTERPRESET(0, 19, 0, b0d19f0);
}
