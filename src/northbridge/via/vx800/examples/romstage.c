/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 One Laptop per Child, Association, Inc.
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

#define ASSEMBLY 1
#define __PRE_RAM__
#define RAMINIT_SYSINFO 1
#define CACHE_AS_RAM_ADDRESS_DEBUG 0

#include <stdint.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "console/console.c"
#include "lib/ramtest.c"
#include "northbridge/via/vx800/vx800.h"
#include "cpu/x86/mtrr/earlymtrr.c"
#include "cpu/x86/bist.h"
#include "pc80/udelay_io.c"
#include "lib/delay.c"
#include "lib/memcpy.c"
#include "cpu/x86/lapic/boot_cpu.c"

#include "driving_clk_phase_data.c"

#include "northbridge/via/vx800/raminit.h"
#include "northbridge/via/vx800/raminit.c"


int acpi_is_wakeup_early_via_vx800(void)
{
	device_t dev;
	u16 tmp, result;

	print_debug("In acpi_is_wakeup_early_via_vx800\n");
	/* Power management controller */
	dev = pci_locate_device(PCI_ID(PCI_VENDOR_ID_VIA,
				       PCI_DEVICE_ID_VIA_VX855_LPC), 0);

	if (dev == PCI_DEV_INVALID)
		die("Power management controller not found\n");

	/* Set ACPI base address to I/O VX800_ACPI_IO_BASE. */
	pci_write_config16(dev, 0x88, VX800_ACPI_IO_BASE | 0x1);

	/* Enable ACPI accessm RTC signal gated with PSON. */
	pci_write_config8(dev, 0x81, 0x84);

	tmp = inw(VX800_ACPI_IO_BASE + 0x04);
	result = ((tmp & (7 << 10)) >> 10) == 1 ? 3 : 0;
	print_debug("         boot_mode=");
	print_debug_hex16(result);
	print_debug("\n");
	return result;
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}


static void enable_mainboard_devices(void)
{
	device_t dev;
	uint16_t values;

	print_debug("In enable_mainboard_devices \n");

	/*
	   Enable P2P Bridge Header for External PCI BUS.
	 */
	dev = pci_locate_device(PCI_ID(0x1106, 0xa353), 0);
	pci_write_config8(dev, 0x4f, 0x41);
}

static void enable_shadow_ram(void)
{
	uint8_t shadowreg;
	pci_write_config8(PCI_DEV(0, 0, 3), 0x80, 0xff);
	/* 0xf0000-0xfffff - ACPI tables */
	shadowreg = pci_read_config8(PCI_DEV(0, 0, 3), 0x83);
	shadowreg |= 0x30;
	pci_write_config8(PCI_DEV(0, 0, 3), 0x83, shadowreg);
	/* 0xe0000-0xeffff - elfload? */

	pci_write_config8(PCI_DEV(0, 0, 3), 0x82, 0xff);

}


/*
this table contains the value needed to be set before begin to init dram.
Note: REV_Bx should be cared when porting a new board!!!!! */
static const struct VIA_PCI_REG_INIT_TABLE mNbStage1InitTbl[] = {
	//VT3409 no pcie
	0x00, 0xFF, NB_APIC_REG(0x61), 0xFF, 0x0E,	// Set Exxxxxxx as pcie mmio config range
	0x00, 0xFF, NB_APIC_REG(0x60), 0xF4, 0x0B,	// Support extended cfg address of pcie
	//0x00, 0xFF, NB_APIC_REG(0x42), 0xF9, 0x02, // APIC Interrupt((BT_INTR)) Control
	// Set ROMSIP value by software

	/*0x00, 0xFF, NB_HOST_REG(0x70), 0x77, 0x33, // 2x Host Adr Strobe/Pad Pullup Driving = 3
	   0x00, 0xFF, NB_HOST_REG(0x71), 0x77, 0x33, // 2x Host Adr Strobe/Pad Pulldown Driving = 3
	   0x00, 0xFF, NB_HOST_REG(0x72), 0x77, 0x33, // 4x Host Dat Strobe/Pad Pullup Driving = 3
	   0x00, 0xFF, NB_HOST_REG(0x73), 0x77, 0x33, // 4x Host Dat Strobe/Pad Pulldown Driving = 3
	   0x00, 0xFF, NB_HOST_REG(0x74), 0xFF, 0x21, // Memory I/F timing ctrl
	   0x00, 0xFF, NB_HOST_REG(0x74), 0xFF, 0xE1, // Memory I/F timing ctrl
	   0x00, 0xFF, NB_HOST_REG(0x75), 0xFF, 0x18, // AGTL+ I/O Circuit
	   0x00, 0xFF, NB_HOST_REG(0x76), 0xFB, 0x0C, // AGTL+ Compensation Status
	   0x00, 0xFF, NB_HOST_REG(0x78), 0xFF, 0x33, // 2X AGTL+ Auto Compensation Offset
	   0x00, 0xFF, NB_HOST_REG(0x79), 0xFF, 0x33, // 4X AGTL+ Auto Compensation Offset
	   0x00, 0xFF, NB_HOST_REG(0x7A), 0x3F, 0x72, // AGTL Compensation Status
	   0x00, 0xFF, NB_HOST_REG(0x7A), 0x3F, 0x77, // AGTL Compensation Status
	   0x00, 0xFF, NB_HOST_REG(0x7B), 0xFF, 0x44, // Input Host Address / Host Strobe Delay Control for HA Group
	   0x00, 0xFF, NB_HOST_REG(0x7B), 0xFF, 0x22, // Input Host Address / Host Strobe Delay Control for HA Group
	   0x00, 0xFF, NB_HOST_REG(0x7C), 0xFF, 0x00, // Output Delay Control of PAD for HA Group
	   0x00, 0xFF, NB_HOST_REG(0x7D), 0xFF, 0xAA, // Host Address / Address Clock Output Delay Control (Only for P4 Bus)
	   0x00, 0xFF, NB_HOST_REG(0x7E), 0xFF, 0x10, // Host Address CKG Rising / Falling Time Control (Only for P4 Bus)
	   0x00, 0xFF, NB_HOST_REG(0x7E), 0xFF, 0x40, // Host Address CKG Rising / Falling Time Control (Only for P4 Bus)
	   0x00, 0xFF, NB_HOST_REG(0x7F), 0xFF, 0x10, // Host Address CKG Rising / Falling Time Control (Only for P4 Bus)
	   0x00, 0xFF, NB_HOST_REG(0x7F), 0xFF, 0x40, // Host Address CKG Rising / Falling Time Control (Only for P4 Bus)
	   0x00, 0xFF, NB_HOST_REG(0x80), 0x3F, 0x44, // Host Data Receiving Strobe Delay Ctrl 1
	   0x00, 0xFF, NB_HOST_REG(0x81), 0xFF, 0x44, // Host Data Receiving Strobe Delay Ctrl 2
	   0x00, 0xFF, NB_HOST_REG(0x82), 0xFF, 0x00, // Output Delay of PAD for HDSTB
	   0x00, 0xFF, NB_HOST_REG(0x83), 0xFF, 0x00, // Output Delay of PAD for HD
	   0x00, 0xFF, NB_HOST_REG(0x84), 0xFF, 0x44, // Host Data / Strobe CKG Control (Group 0)
	   0x00, 0xFF, NB_HOST_REG(0x85), 0xFF, 0x44, // Host Data / Strobe CKG Control (Group 1)
	   0x00, 0xFF, NB_HOST_REG(0x86), 0xFF, 0x44, // Host Data / Strobe CKG Control (Group 2)
	   0x00, 0xFF, NB_HOST_REG(0x87), 0xFF, 0x44, // Host Data / Strobe CKG Control (Group 3) */


	// CPU Host Bus Control
	0x00, 0xFF, NB_HOST_REG(0x50), 0x1F, 0x08,	// Request phase ctrl: Dynamic Defer Snoop Stall Count = 8
	//0x00, 0xFF, NB_HOST_REG(0x51), 0xFF, 0x7F, // CPU I/F Ctrl-1: Disable Fast DRDY and RAW
	0x00, 0xFF, NB_HOST_REG(0x51), 0xFF, 0x7C,	// CPU I/F Ctrl-1: Disable Fast DRDY and RAW
	0x00, 0xFF, NB_HOST_REG(0x52), 0xCB, 0xCB,	// CPU I/F Ctrl-2: Enable all for performance
	//0x00, 0xFF, NB_HOST_REG(0x53), 0xFF, 0x88, // Arbitration: Host/Master Occupancy timer = 8*4 HCLK
	0x00, 0xFF, NB_HOST_REG(0x53), 0xFF, 0x44,	// Arbitration: Host/Master Occupancy timer = 4*4 HCLK
	0x00, 0xFF, NB_HOST_REG(0x54), 0x1E, 0x1C,	// Misc Ctrl: Enable 8QW burst Mem Access
	//0x00, 0xFF, NB_HOST_REG(0x55), 0x06, 0x06, // Miscellaneous Control 2
	0x00, 0xFF, NB_HOST_REG(0x55), 0x06, 0x04,	// Miscellaneous Control 2
	0x00, 0xFF, NB_HOST_REG(0x56), 0xF7, 0x63,	// Write Policy 1
	//0x00, 0xFF, NB_HOST_REG(0x59), 0x3D, 0x01, // CPU Miscellaneous Control 1, enable Lowest-Priority IPL
	//0x00, 0xFF, NB_HOST_REG(0x5c), 0xFF, 0x00, // CPU Miscellaneous Control 2
	0x00, 0xFF, NB_HOST_REG(0x5D), 0xFF, 0xA2,	// Write Policy
	0x00, 0xFF, NB_HOST_REG(0x5E), 0xFF, 0x88,	// Bandwidth Timer
	0x00, 0xFF, NB_HOST_REG(0x5F), 0x46, 0x46,	// CPU Misc Ctrl
	// 0x00, 0xFF, NB_HOST_REG(0x90), 0xFF, 0x0B, // CPU Miscellaneous Control 3
	//0x00, 0xFF, NB_HOST_REG(0x96), 0x0B, 0x0B, // CPU Miscellaneous Control 2
	0x00, 0xFF, NB_HOST_REG(0x96), 0x0B, 0x0A,	// CPU Miscellaneous Control 2
	0x00, 0xFF, NB_HOST_REG(0x98), 0xC1, 0x41,	// CPU Miscellaneous Control 3
	0x00, 0xFF, NB_HOST_REG(0x99), 0x0E, 0x06,	// CPU Miscellaneous Control 4


	// Set APIC and SMRAM
	0x00, 0xFF, NB_HOST_REG(0x97), 0xFF, 0x00,	// APIC Related Control
	0x00, 0xFF, NB_DRAMC_REG(0x86), 0xD6, 0x29,	// SMM and APIC Decoding: enable APIC, MSI and SMRAM A-Seg
	0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	// End of the table
};

#define USE_VCP     1		//0 means use DVP
#define USE_COM1    1
#define USE_COM2    0

#define gCom1Base   0x3f8
#define gCom2Base   0x2f8
void EmbedComInit()
{
	u8 ByteVal;
	u16 ComBase;

	//enable NB multiple function control
	ByteVal = pci_read_config8(PCI_DEV(0, 0, 0), 0x4f);
	ByteVal = ByteVal | 0x01;
	pci_write_config8(PCI_DEV(0, 0, 0), 0x4f, ByteVal);

	//VGA Enable
	ByteVal = pci_read_config8(PCI_DEV(0, 0, 3), 0xA1);
	ByteVal = ByteVal | 0x80;
	pci_write_config8(PCI_DEV(0, 0, 3), 0xA1, ByteVal);

	ByteVal = pci_read_config8(PCI_DEV(0, 0, 3), 0xA7);
	ByteVal = ByteVal | 0x08;
	pci_write_config8(PCI_DEV(0, 0, 3), 0xA7, ByteVal);

	//Enable p2p  IO/mem
	ByteVal = pci_read_config8(PCI_DEV(0, 1, 0), 0x4);
	ByteVal = ByteVal | 0x07;
	pci_write_config8(PCI_DEV(0, 1, 0), 0x4, ByteVal);

	//Turn on Graphic chip IO port port access
	ByteVal = inb(0x3C3);
	ByteVal = ByteVal | 0x01;
	outb(ByteVal, 0x3C3);

	//Turn off Graphic chip Register protection
	outb(0x10, 0x3C4);
	ByteVal = inb(0x3C5);
	ByteVal = ByteVal | 0x01;
	outb(ByteVal, 0x3C5);

	//south module pad share enable 0x3C5.78[7]
	outb(0x78, 0x3C4);
	ByteVal = inb(0x3C5);
	ByteVal = ByteVal | 0x80;
	outb(ByteVal, 0x3C5);

	//enable  UART Function multiplex with DVP or VCP pad D17F0Rx46[7,6]
	ByteVal = pci_read_config8(PCI_DEV(0, 17, 0), 0x46);
	//multiplex with VCP
	if (USE_VCP == 1)
		ByteVal = (ByteVal & 0x3F) | 0x40;
	//multiplex with DVP
	else
		ByteVal = (ByteVal & 0x3F) | 0xC0;
	pci_write_config8(PCI_DEV(0, 17, 0), 0x46, ByteVal);



	//enable embeded com1 and com2 D17F0RxB0[5,4]
	ByteVal = pci_read_config8(PCI_DEV(0, 17, 0), 0xB0);
	ByteVal = ByteVal & 0xcf;
	//multiplex with VCP
	if (USE_COM1 == 1)
		ByteVal = ByteVal | 0x10;
	if (USE_COM2 == 1)
		ByteVal = ByteVal | 0x20;
	pci_write_config8(PCI_DEV(0, 17, 0), 0xB0, ByteVal);

	if (USE_COM1 == 1)
		ComBase = gCom1Base;
	else
		ComBase = gCom2Base;

//noharddrive

	//set embeded com1 IO base = 0x3E8
	//D17F0RB4
	//ByteVal = 0xFD;
	if (USE_COM1 == 1) {
		ByteVal = (u8) ((gCom1Base >> 3) | 0x80);
		pci_write_config8(PCI_DEV(0, 17, 0), 0xB4, ByteVal);
		ByteVal = pci_read_config8(PCI_DEV(0, 17, 0), 0xb2);
		ByteVal = (ByteVal & 0xf0) | 0x04;
		pci_write_config8(PCI_DEV(0, 17, 0), 0xB2, ByteVal);
	}
	//set embeded com2 IO base = 0x2E8
	//D17F0RB5
	//ByteVal = 0xDD;
	if (USE_COM2 == 1) {
		ByteVal = (u8) ((gCom2Base >> 3) | 0x80);
		pci_write_config8(PCI_DEV(0, 17, 0), 0xB5, ByteVal);
		ByteVal = pci_read_config8(PCI_DEV(0, 17, 0), 0xb2);
		ByteVal = (ByteVal & 0x0f) | 0x30;
		pci_write_config8(PCI_DEV(0, 17, 0), 0xB2, ByteVal);
	}
	//no port 80 biger then 0x10

	//disable interrupt
	ByteVal = inb(ComBase + 3);
	outb(ByteVal & 0x7F, ComBase + 3);
	outb(0x00, ComBase + 1);

	//set baudrate
	ByteVal = inb(ComBase + 3);
	outb(ByteVal | 0x80, ComBase + 3);
	outb(0x01, ComBase);
	outb(0x00, ComBase + 1);

	//set  frame  fromat
	ByteVal = inb(ComBase + 3);
	outb(ByteVal & 0x3F, ComBase + 3);
	outb(0x03, ComBase + 3);
	outb(0x00, ComBase + 2);
	outb(0x00, ComBase + 4);

	//SOutput("Embeded com output\n");
	//while(1);
}

/* cache_as_ram.inc jump to here
*/
void stage1_main(unsigned long bist)
{
	unsigned cpu_reset = 0;
	u16 boot_mode;
	u8 rambits;

	//device_t dev;
	/* Enable multifunction for northbridge. */
	pci_write_config8(PCI_DEV(0, 0, 0), 0x4f, 0x01);
	EmbedComInit();
	//enable_vx800_serial();
	//uart_init();


/*	1.    D15F0

a)      RxBAh = 71h

b)      RxBBh = 05h

c)      RxBEh = 71h

d)      RxBFh = 05h

2.    D17F0

a)      RxA0h = 06h

b)      RxA1h = 11h

c)      RxA2h = 27h

d)      RxA3h = 32h

e)      Rx79h = 40h

f)      Rx72h = 27h

g)      Rx73h = 32h
*/

	u8 Data8;

	pci_write_config16(PCI_DEV(0, 0xf, 0), 0xBA,
			   PCI_DEVICE_ID_VIA_VX855_IDE);
	pci_write_config16(PCI_DEV(0, 0xf, 0), 0xBE,
			   PCI_DEVICE_ID_VIA_VX855_IDE);
	pci_write_config16(PCI_DEV(0, 0x11, 0), 0xA0, PCI_VENDOR_ID_VIA);
	pci_write_config16(PCI_DEV(0, 0x11, 0), 0xA2,
			   PCI_DEVICE_ID_VIA_VX855_LPC);
	Data8 = pci_read_config8(PCI_DEV(0, 0x11, 0), 0x79);
	Data8 &= ~0x40;
	Data8 |= 0x40;
	pci_write_config8(PCI_DEV(0, 0x11, 0), 0x79, Data8);
	pci_write_config16(PCI_DEV(0, 0x11, 0), 0x72,
			   PCI_DEVICE_ID_VIA_VX855_LPC);

	console_init();		//there are to function defination of console_init(), while the src/archi386/lib is the right one

	/* decide if this is a s3 wakeup or a normal boot */
	boot_mode = acpi_is_wakeup_early_via_vx800();
	/*add this, to transfer "cpu restart" to "cold boot"
	   When this boot is not a S3 resume, and PCI registers had been written, 
	   then this must be a cpu restart(result of os reboot cmd). so we need a real "cold boot". */
	if ((boot_mode != 3)
	    && (pci_read_config8(PCI_DEV(0, 0, 3), 0x80) != 0)) {
		outb(6, 0xcf9);
	}

	/*x86 cold boot I/O cmd */
	enable_smbus();
	//smbus_fixup(&ctrl);// this fix does help vx800!, but vx855 no need this 

	if (bist == 0) {
		// CAR need mtrr untill mem is ok, so i disable this early_mtrr_init();
		//print_debug("doing early_mtrr\n");
		//early_mtrr_init();
	}

	/* Halt if there was a built-in self test failure. */
	report_bist_failure(bist);

	print_debug("Enabling mainboard devices\n");
	enable_mainboard_devices();

	u8 Data;
	device_t device;
	/* Get NB Chip revision from D0F4RxF6, revision will be used in via_pci_inittable */
	device = PCI_DEV(0, 0, 4);
	Data = pci_read_config8(device, 0xf6);
	print_debug("NB chip revision =");
	print_debug_hex8(Data);
	print_debug("\n");
	/* make NB ready before draminit */
	via_pci_inittable(Data, mNbStage1InitTbl);

	/*add this.
	   When resume from s3, draminit is skiped, so need to recovery any PCI register related to draminit.
	   and d0f3 didnt lost its Power during whole s3 time, so any register not belongs to d0f3 need to be recoveried . */
#if 1
	if (boot_mode == 3) {
		u8 i;
		u8 ramregs[] = { 0x43, 0x42, 0x41, 0x40 };
		DRAM_SYS_ATTR DramAttr;

		print_debug("This is a S3 wakeup\n");

		memset(&DramAttr, 0, sizeof(DRAM_SYS_ATTR));
		/*Step1 DRAM Detection; DDR1 or DDR2; Get SPD Data; Rank Presence;64 or 128bit; Unbuffered or registered; 1T or 2T */
		DRAMDetect(&DramAttr);

		/*begin to get ram size, 43,42 41 40 contains the end address of last rank in ddr2-slot */
		device = PCI_DEV(0, 0, 3);
		for (rambits = 0, i = 0; i < ARRAY_SIZE(ramregs); i++) {
			rambits = pci_read_config8(device, ramregs[i]);
			if (rambits != 0)
				break;
		}

		DRAMDRDYSetting(&DramAttr);

		Data = 0x80;	// this value is same with dev_init.c
		pci_write_config8(PCI_DEV(0, 0, 4), 0xa3, Data);
		pci_write_config8(PCI_DEV(0, 17, 7), 0x60, rambits << 2);
		Data = pci_read_config8(MEMCTRL, 0x88);
		pci_write_config8(PCI_DEV(0, 17, 7), 0xE5, Data);

		DRAMRegFinalValue(&DramAttr);	// I just copy this function from draminit to here!
		SetUMARam();	// I just copy this function from draminit to here!
		print_debug("Resume from S3, RAM init was ignored\n");
	} else {
		ddr2_ram_setup();
		ram_check(0, 640 * 1024);
	}
#endif
	//ddr2_ram_setup();
	/*this line is the same with cx700 port . */
	enable_shadow_ram();

	/*
	   For coreboot most time of S3 resume is the same as normal boot, so some memory area under 1M become dirty,
	   so before this happen, I need to backup the content of mem to top-mem. 
	   I will reserve the 1M top-men in LBIO table in coreboot_table.c and recovery the content of 1M-mem in wakeup.c
	 */
#if PAYLOAD_IS_SEABIOS==1	//
	if (boot_mode == 3) {
		/*   some idea of Libo.Feng at amd.com in  http://www.coreboot.org/pipermail/coreboot/2008-December/043111.html
		   I want move the 1M data, I have to set some MTRRs myself. */
		/* seting mtrr before back memoy save s3 resume time about 0.14 seconds */
		/*because CAR stack use cache, and here to use cache , must be careful, 
		   1 during these mtrr code, must no function call, (after this mtrr, I think it should be ok to use function)
		   2 before stack switch, no use variable that have value set before this
		   3 due to 2, take care of "cpu_reset", I directlly set it to ZERO.
		 */
		u32 memtop = *(u32 *) WAKE_MEM_INFO;
		u32 memtop1 = *(u32 *) WAKE_MEM_INFO - 0x100000;
		u32 memtop2 = *(u32 *) WAKE_MEM_INFO - 0x200000;
		u32 memtop3 =
		    *(u32 *) WAKE_MEM_INFO - 64 * 1024 - 0x100000;
		u32 memtop4 =
		    *(u32 *) WAKE_MEM_INFO - 64 * 1024 - 0x100000 +
		    0xe0000;
		/*      __asm__ volatile (              
		   "movl    $0x204, %%ecx\n\t"
		   "xorl    %%edx, %%edx\n\t"
		   "movl     %0,%%eax\n\t"
		   "orl     $(0 | 6), %%eax\n\t"
		   "wrmsr\n\t"

		   "movl    $0x205, %%ecx\n\t"
		   "xorl    %%edx, %%edx\n\t"
		   "movl   $0x100000,%%eax\n\t"
		   "decl                %%eax\n\t"
		   "notl                %%eax\n\t"
		   "orl    $(0 | 0x800), %%eax\n\t"
		   "wrmsr\n\t"
		   ::"g"(memtop2)
		   );
		   __asm__ volatile (           
		   "movl    $0x206, %%ecx\n\t"
		   "xorl    %%edx, %%edx\n\t"
		   "movl     %0,%%eax\n\t"
		   "orl     $(0 | 6), %%eax\n\t"
		   "wrmsr\n\t"

		   "movl    $0x207, %%ecx\n\t"
		   "xorl    %%edx, %%edx\n\t"
		   "movl   $0x100000,%%eax\n\t"
		   "decl                %%eax\n\t"
		   "notl                %%eax\n\t"
		   "orl    $(0 | 0x800), %%eax\n\t"
		   "wrmsr\n\t"
		   ::"g"(memtop1)
		   );
		   __asm__ volatile (       
		   "movl    $0x208, %ecx\n\t"
		   "xorl    %edx, %edx\n\t"
		   "movl    $0,%eax\n\t"
		   "orl     $(0 | 6), %eax\n\t"
		   "wrmsr\n\t"

		   "movl    $0x209, %ecx\n\t"
		   "xorl    %edx, %edx\n\t"
		   "movl     $0x100000,%eax\n\t"
		   "decl                %eax\n\t"
		   "notl                %eax\n\t"
		   "orl     $(0 | 0x800), %eax\n\t"
		   "wrmsr\n\t"
		   );
		 */
		// WAKE_MEM_INFO is  inited in get_set_top_available_mem in tables.c
		// these two memcpy not not be enabled if set the MTRR around this two lines.
		/*__asm__ volatile (		
	 			"movl    $0, %%esi\n\t"
        "movl    %0, %%edi\n\t"
       	"movl    $0xa0000, %%ecx\n\t"
       	"shrl    $2, %%ecx\n\t"
        "rep movsd\n\t"    
        ::"g"(memtop3)        
   	);
	__asm__ volatile (		
	 			"movl    $0xe0000, %%esi\n\t"
        "movl    %0, %%edi\n\t"
       	"movl    $0x20000, %%ecx\n\t"
       	"shrl    $2, %%ecx\n\t"
        "rep movsd\n\t"    
        ::"g"(memtop4)        
   	);*/
		print_debug("copy memory to high memory to protect s3 wakeup vector code \n");	//this can have function call, because no variable used before this
		memcpy((unsigned char *) ((*(u32 *) WAKE_MEM_INFO) -
					  64 * 1024 - 0x100000),
		       (unsigned char *) 0, 0xa0000);
		memcpy((unsigned char *) ((*(u32 *) WAKE_MEM_INFO) -
					  64 * 1024 - 0x100000 + 0xe0000),
		       (unsigned char *) 0xe0000, 0x20000);

		/* restore the MTRR previously modified. */
/*		__asm__ volatile (	
        "wbinvd\n\t"     			
        "xorl    %edx, %edx\n\t"
       	"xorl    %eax, %eax\n\t"
       	"movl    $0x204, %ecx\n\t"
        "wrmsr\n\t"
	 			"movl    $0x205, %ecx\n\t"                                      
        "wrmsr\n\t"        
	 			"movl    $0x206, %ecx\n\t"
        "wrmsr\n\t"
	 			"movl    $0x207, %ecx\n\t"                     
        "wrmsr\n\t"        
	 			"movl    $0x208, %ecx\n\t"                     
        "wrmsr\n\t"        
	 			"movl    $0x209, %ecx\n\t"                     
        "wrmsr\n\t"        
		);*/
	}
#endif
/*
the following code is  copied from src/mainboard/tyan/s2735/romstage.c
Only the code around CLEAR_FIRST_1M_RAM is changed.
I remove all the code around CLEAR_FIRST_1M_RAM and #include "cpu/x86/car/cache_as_ram_post.c"
the CLEAR_FIRST_1M_RAM seems to make cpu/x86/car/cache_as_ram_post.c stop at somewhere, 
and cpu/x86/car/cache_as_ram_post.c  do not cache my $CONFIG_XIP_ROM_BASE+SIZE area.

So, I use: #include "cpu/via/car/cache_as_ram_post.c". my via-version post.c have some diff with x86-version
*/
#if 1
	{
		/* Check value of esp to verify if we have enough rom for stack in Cache as RAM */
		unsigned v_esp;
		__asm__ volatile ("movl   %%esp, %0\n\t":"=a" (v_esp)
		    );
#if CONFIG_USE_PRINTK_IN_CAR
		printk(BIOS_DEBUG, "v_esp=%08x\n", v_esp);
#else
		print_debug("v_esp=");
		print_debug_hex32(v_esp);
		print_debug("\n");
#endif
	}

#endif
#if 1

      cpu_reset_x:
// it seems that cpu_reset is not used before this, so I just reset it, (this is because the s3 resume, setting in mtrr and copy data may destroy 
//stack
	cpu_reset = 0;
#if CONFIG_USE_PRINTK_IN_CAR
	printk(BIOS_DEBUG, "cpu_reset = %08x\n", cpu_reset);
#else
	print_debug("cpu_reset = ");
	print_debug_hex32(cpu_reset);
	print_debug("\n");
#endif

	if (cpu_reset == 0) {
		print_debug("Clearing initial memory region: ");
	}
	print_debug("No cache as ram now - ");

	/* store cpu_reset to ebx */
	__asm__ volatile ("movl %0, %%ebx\n\t"::"a" (cpu_reset)
	    );


/* cancel these lines, CLEAR_FIRST_1M_RAM cause the cpu/x86/car/cache_as_ram_post.c stop at somewhere

	if(cpu_reset==0) {
#define CLEAR_FIRST_1M_RAM 1
#include "cpu/via/car/cache_as_ram_post.c"	
	}
	else {
#undef CLEAR_FIRST_1M_RAM 
#include "cpu/via/car/cache_as_ram_post.c"
	}
*/
#include "cpu/via/car/cache_as_ram_post.c"
//#include "cpu/x86/car/cache_as_ram_post.c"    
	__asm__ volatile (
				 /* set new esp *//* before CONFIG_RAMBASE */
				 "subl   %0, %%ebp\n\t"
				 "subl   %0, %%esp\n\t"::
				 "a" ((CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE) -
				      CONFIG_RAMBASE)
	    );

	{
		unsigned new_cpu_reset;

		/* get back cpu_reset from ebx */
		__asm__ volatile ("movl %%ebx, %0\n\t":"=a" (new_cpu_reset)
		    );

		/* We can not go back any more, we lost old stack data in cache as ram */
		if (new_cpu_reset == 0) {
			print_debug("Use Ram as Stack now - done\n");
		} else {
			print_debug("Use Ram as Stack now - \n");
		}
#if CONFIG_USE_PRINTK_IN_CAR
		printk(BIOS_DEBUG, "new_cpu_reset = %08x\n", new_cpu_reset);
#else
		print_debug("new_cpu_reset = ");
		print_debug_hex32(new_cpu_reset);
		print_debug("\n");
#endif
		/*copy and execute coreboot_ram */
		copy_and_run(new_cpu_reset);
		/* We will not return */
	}
#endif


	print_debug("should not be here -\n");

}
