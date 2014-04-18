/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
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
#include <lib.h>
#include <console.h>
#include <device/pci.h>
#include <msr.h>
#include <legacy.h>
#include <device/pci.h>
#include <statictree.h>
#include <config.h>
#include <io.h>
BORKED
#define SMBUS_IO_BASE 0x1000	/* Is it a temporary SMBus I/O base address? */
	 /*SIZE 0x40 */

/* Get SB ASIC Revision.*/
static u8 get_sb600_revision()
{
	u32 dev;
	pci_conf1_find_device(0x1002,  0x4385, &dev);

	if (dev == PCI_DEV_INVALID) {
		die("SMBUS controller not found\r\n");
	}
	return pci_conf1_read_config8(dev, 0x08);
}


/***************************************
* Legacy devices are mapped to LPC space.
*	serial port 0
*	KBC Port
*	ACPI Micro-controller port
*	LPC ROM size,
* NOTE: Call me ASAP, because I will reset LPC ROM size!
***************************************/
static void sb600_lpc_init(void)
{
	u8 reg8;
	u32 reg32;
	u32 dev;

	/* Enable lpc controller */
	pci_conf1_find_device(0x1002,  0x4385, &dev);	/* SMBUS controller */
	reg32 = pci_conf1_read_config32(dev, 0x64);
	reg32 |= 0x00100000;
	pci_conf1_write_config32(dev, 0x64, reg32);

	pci_conf1_find_device(0x1002,  0x438d, &dev);	/* LPC Controller */
	/* Serial 0 */
	reg8 = pci_conf1_read_config8(dev, 0x44);
	reg8 |= (1 << 6);
	pci_conf1_write_config8(dev, 0x44, reg8);

	/* PS/2 keyboard, ACPI */
	reg8 = pci_conf1_read_config8(dev, 0x47);
	reg8 |= (1 << 5) | (1 << 6);
	pci_conf1_write_config8(dev, 0x47, reg8);

	/* SuperIO, LPC ROM */
	reg8 = pci_conf1_read_config8(dev, 0x48);
	reg8 |= (1 << 1) | (1 << 0);	/* enable Super IO config port 2e-2h, 4e-4f */
	reg8 |= (1 << 3) | (1 << 4);	/* enable for LPC ROM address range1&2, Enable 512KB rom access at 0xFFF80000 - 0xFFFFFFFF  */
	reg8 |= 1 << 6;		/* enable for RTC I/O range */
	pci_conf1_write_config8(dev, 0x48, reg8);

	/* hardware should enable LPC ROM by pin strapes */
	/*  rom access at 0xFFF80000/0xFFF00000 - 0xFFFFFFFF */
	/* See detail in BDG-215SB600-03.pdf page 15. */
	pci_conf1_write_config16(dev, 0x68, 0x000e);	/* enable LPC ROM range, 0xfff8: 512KB, 0xfff0: 1MB; */
	pci_conf1_write_config16(dev, 0x6c, 0xfff0);	/* enable LPC ROM range, 0xfff8: 512KB, 0xfff0: 1MB  */
}

/* what is its usage? */
static u32 get_sbdn(u32 bus)
{
	u32 dev;

	/* Find the device. */
	pci_conf1_find_on_bus(bus, 0x1002, 0x4385, &dev);
	return (dev >> 15) & 0x1f;
}


static u8 dual_core()
{
	if(((cpuid_eax(0x80000000) & ~0xff) >= 8)) {
		if(cpuid_ecx(0x80000008) & 1)
			return 1;
	}
	return 0;
}

/*
SB600 VFSMAF (VID/FID System Management Action Field)  is 010b by default.
RPR 2.3.3 C-state and VID/FID change for the K8 platform.
*/
static void enable_fid_change_on_sb(u32 sbbusn, u32 sbdn)
{
	u8 byte;
	byte = pmio_read(0x9a);
	byte &= ~0x34;
	if(dual_core())
		byte |= 0x34;
	else
		byte |= 0x04;
	pmio_write(0x9a, byte);

	byte = pmio_read(0x8f);
	byte &= ~0x30;
	byte |= 0x20;
	pmio_write(0x8f, byte);

	pmio_write(0x8b, 0x01);
	pmio_write(0x8a, 0x90);

	if(get_sb600_revision() > 0x13)
		pmio_write(0x88, 0x10);
	else
		pmio_write(0x88, 0x06);

	byte = pmio_read(0x7c);
	byte &= ~0x01;
	byte |= 0x01;
	pmio_write(0x7c, byte);

	/*Must be 0 for K8 platform.*/
	byte = pmio_read(0x68);
	byte &= ~0x01;
	pmio_write(0x68, byte);
	/*Must be 0 for K8 platform.*/
	byte = pmio_read(0x8d);
	byte &= ~(1<<6);
	pmio_write(0x8d, byte);

	byte = pmio_read(0x61);
	byte &= ~0x04;
	pmio_write(0x61, byte);

	byte = pmio_read(0x42);
	byte &= ~0x04;
	pmio_write(0x42, byte);

	if(get_sb600_revision() == 0x14) {
		pmio_write(0x89, 0x10);

		byte = pmio_read(0x52);
		byte |= 0x80;
		pmio_write(0x52, byte);
	}
}


static void hard_reset(void)
{
	set_bios_reset();

	/* full reset */
	outb(0x0a, 0x0cf9);
	outb(0x0e, 0x0cf9);
}

static void soft_reset(void)
{
	set_bios_reset();
	/* link reset */
	outb(0x06, 0x0cf9);
}


static void sb600_pci_port80()
{
	u8 byte;
	u32 dev;

	/* P2P Bridge */
	pci_conf1_find_device(0x1002,  0x4384, &dev);

	byte = pci_conf1_read_config8(dev, 0x40);
	byte |= 1 << 5;
	pci_conf1_write_config8(dev, 0x40, byte);

	byte = pci_conf1_read_config8(dev, 0x4B);
	byte |= 1 << 7;
	pci_conf1_write_config8(dev, 0x4B, byte);

	byte = pci_conf1_read_config8(dev, 0x1C);
	byte |= 0xF << 4;
	pci_conf1_write_config8(dev, 0x1C, byte);

	byte = pci_conf1_read_config8(dev, 0x1D);
	byte |= 0xF << 4;
	pci_conf1_write_config8(dev, 0x1D, byte);

	byte = pci_conf1_read_config8(dev, 0x04);
	byte |= 1 << 0;
	pci_conf1_write_config8(dev, 0x04, byte);

	pci_conf1_find_device(0x1002,  0x438D, &dev);

	byte = pci_conf1_read_config8(dev, 0x4A);
	byte &= ~(1 << 5);	/* disable lpc port 80 */
	pci_conf1_write_config8(dev, 0x4A, byte);
}

static void sb600_lpc_port80(void)
{
	u8 byte;
	u32 dev;
	u32 reg32;

	/* enable lpc controller */
	pci_conf1_find_device(0x1002,  0x4385, &dev);
	reg32 = pci_conf1_read_config32(dev, 0x64);
	reg32 |= 0x00100000;	/* lpcEnable */
	pci_conf1_write_config32(dev, 0x64, reg32);

	/* enable prot80 LPC decode in pci function 3 configuration space. */
	pci_conf1_find_device(0x1002,  0x438d, &dev);
	byte = pci_conf1_read_config8(dev, 0x4a);
	byte |= 1 << 5;		/* enable port 80 */
	pci_conf1_write_config8(dev, 0x4a, byte);
}


/* sbDevicesPorInitTable */
static void sb600_devices_por_init()
{
	u32 dev;
	u8 byte;

	printk(BIOS_INFO, "sb600_devices_por_init()\n");
	/* SMBus Device, BDF:0-20-0 */
	printk(BIOS_INFO, "sb600_devices_por_init(): SMBus Device, BDF:0-20-0\n");
	pci_conf1_find_device(0x1002,  0x4385, &dev);

	if (dev == PCI_DEV_INVALID) {
		die("SMBUS controller not found\r\n");
	}
	printk(BIOS_INFO, "SMBus controller enabled, sb revision is 0x%x\r\n",
		    get_sb600_revision());

	/* sbPorAtStartOfTblCfg */
	/* Set A-Link bridge access address. This address is set at device 14h, function 0, register 0xf0.
	 * This is an I/O address. The I/O address must be on 16-byte boundry.  */
	pci_conf1_write_config32(dev, 0xf0, AB_INDX);

	/* To enable AB/BIF DMA access, a specific register inside the BIF register space needs to be configured first. */
	/*Enables the SB600 to send transactions upstream over A-Link Express interface. */
	axcfg_reg(0x04, 1 << 2, 1 << 2);
	axindxc_reg(0x21, 0xff, 0);

	/* 2.3.5:Enabling Non-Posted Memory Write for the K8 Platform */
	axindxc_reg(0x10, 1 << 9, 1 << 9);
	/* END of sbPorAtStartOfTblCfg */

	/* sbDevicesPorInitTables */
	/* set smbus iobase */
	pci_conf1_write_config32(dev, 0x10, SMBUS_IO_BASE | 1);

	/* enable smbus controller interface */
	byte = pci_conf1_read_config8(dev, 0xd2);
	byte |= (1 << 0);
	pci_conf1_write_config8(dev, 0xd2, byte);

	/* set smbus 1, ASF 2.0 (Alert Standard Format), iobase */
	pci_conf1_write_config16(dev, 0x58, SMBUS_IO_BASE | 0x11);

	/* TODO: I don't know the useage of followed two lines. I copied them from CIM. */
	pci_conf1_write_config8(dev, 0x0a, 0x1);
	pci_conf1_write_config8(dev, 0x0b, 0x6);

	/* KB2RstEnable */
	pci_conf1_write_config8(dev, 0x40, 0xd4);

	/* Enable ISA Address 0-960K decoding */
	pci_conf1_write_config8(dev, 0x48, 0x0f);

	/* Enable ISA  Address 0xC0000-0xDFFFF decode */
	pci_conf1_write_config8(dev, 0x49, 0xff);

	/* Enable decode cycles to IO C50, C51, C52 GPM controls. */
	byte = pci_conf1_read_config8(dev, 0x41);
	byte &= 0x80;
	byte |= 0x33;
	pci_conf1_write_config8(dev, 0x41, byte);

	/* Legacy DMA Prefetch Enhancement, CIM masked it. */
	/* pci_conf1_write_config8(dev, 0x43, 0x1); */

	/* Disabling Legacy USB Fast SMI# */
	byte = pci_conf1_read_config8(dev, 0x62);
	byte |= 0x24;
	pci_conf1_write_config8(dev, 0x62, byte);

	/* Features Enable */
	pci_conf1_write_config32(dev, 0x64, 0x829E79BF);

	/* SerialIrq Control */
	pci_conf1_write_config8(dev, 0x69, 0x90);

	/* Test Mode, PCIB_SReset_En Mask is set. */
	pci_conf1_write_config8(dev, 0x6c, 0x20);

	/* IO Address Enable, CIM set 0x78 only and masked 0x79. */
	/*pci_conf1_write_config8(dev, 0x79, 0x4F); */
	pci_conf1_write_config8(dev, 0x78, 0xFF);

	/* This register is not used on sb600. It came from older chipset. */
	/*pci_conf1_write_config8(dev, 0x95, 0xFF); */

	/* Set smbus iospace enable, I don't know why write 0x04 into reg5 that is reserved */
	pci_conf1_write_config16(dev, 0x4, 0x0407);

	/* clear any lingering errors, so the transaction will run */
	outb(inb(SMBUS_IO_BASE + SMBHSTSTAT), SMBUS_IO_BASE + SMBHSTSTAT);

	/* IDE Device, BDF:0-20-1 */
	printk(BIOS_INFO, "sb600_devices_por_init(): IDE Device, BDF:0-20-1\n");
	pci_conf1_find_device(0x1002,  0x438C, &dev);
	/* Disable prefetch */
	byte = pci_conf1_read_config8(dev, 0x63);
	byte |= 0x1;
	pci_conf1_write_config8(dev, 0x63, byte);

	/* LPC Device, BDF:0-20-3 */
	printk(BIOS_INFO, "sb600_devices_por_init(): LPC Device, BDF:0-20-3\n");
	pci_conf1_find_device(0x1002,  0x438D, &dev);
	/* DMA enable */
	pci_conf1_write_config8(dev, 0x40, 0x04);

	/* IO Port Decode Enable */
	pci_conf1_write_config8(dev, 0x44, 0xFF);
	pci_conf1_write_config8(dev, 0x45, 0xFF);
	pci_conf1_write_config8(dev, 0x46, 0xC3);
	pci_conf1_write_config8(dev, 0x47, 0xFF);

	/* IO/Mem Port Decode Enable, I don't know why CIM disable some ports.
	 *  Disable LPC TimeOut counter, enable SuperIO Configuration Port (2e/2f),
	 * Alternate SuperIO Configuration Port (4e/4f), Wide Generic IO Port (64/65).
	 * Enable bits for LPC ROM memory address range 1&2 for 1M ROM setting.*/
	byte = pci_conf1_read_config8(dev, 0x48);
	byte |= (1 << 1) | (1 << 0);	/* enable Super IO config port 2e-2h, 4e-4f */
	byte |= (1 << 3) | (1 << 4);	/* enable for LPC ROM address range1&2, Enable 512KB rom access at 0xFFF80000 - 0xFFFFFFFF */
	byte |= 1 << 6;		/* enable for RTC I/O range */
	pci_conf1_write_config8(dev, 0x48, byte);
	pci_conf1_write_config8(dev, 0x49, 0xFF);
	/* Enable 0x480-0x4bf, 0x4700-0x470B */
	byte = pci_conf1_read_config8(dev, 0x4A);
	byte |= ((1 << 1) + (1 << 6));	/*0x42, save the configuraion for port 0x80. */
	pci_conf1_write_config8(dev, 0x4A, byte);

	/* Set LPC ROM size, it has been done in sb600_lpc_init(). 
	 * enable LPC ROM range, 0xfff8: 512KB, 0xfff0: 1MB;
	 * enable LPC ROM range, 0xfff8: 512KB, 0xfff0: 1MB
	 * pci_conf1_write_config16(dev, 0x68, 0x000e)
	 * pci_conf1_write_config16(dev, 0x6c, 0xfff0);*/

	/* Enable Tpm12_en and Tpm_legacy. I don't know what is its usage and copied from CIM. */
	pci_conf1_write_config8(dev, 0x7C, 0x05);

	/* P2P Bridge, BDF:0-20-4, the configuration of the registers in this dev are copied from CIM, 
	 * TODO: I don't know what are their mean? */
	printk(BIOS_INFO, "sb600_devices_por_init(): P2P Bridge, BDF:0-20-4\n");
	pci_conf1_find_device(0x1002,  0x4384, &dev);
	/* I don't know why CIM tried to write into a read-only reg! */
	/*pci_conf1_write_config8(dev, 0x0c, 0x20) */ ;

	/* Arbiter enable. */
	pci_conf1_write_config8(dev, 0x43, 0xff);

	/* Set PCDMA request into hight priority list. */
	/* pci_conf1_write_config8(dev, 0x49, 0x1) */ ;

	pci_conf1_write_config8(dev, 0x40, 0x26);

	/* I don't know why CIM set reg0x1c as 0x11. 
	 * System will block at sdram_initialize() if I set it before call sdram_initialize().
	 * If it is necessary to set reg0x1c as 0x11, please call this function after sdram_initialize().
	 * pci_conf1_write_config8(dev, 0x1c, 0x11);
	 * pci_conf1_write_config8(dev, 0x1d, 0x11);*/

	/*CIM set this register; but I didn't find its description in RPR.
	On DBM690T platform, I didn't find different between set and skip this register.
	But on Filbert platform, the DEBUG message from serial port on Peanut board can't be displayed
	after the bit0 of this register is set.
	pci_conf1_write_config8(dev, 0x04, 0x21);
	*/
	pci_conf1_write_config8(dev, 0x0d, 0x40);
	pci_conf1_write_config8(dev, 0x1b, 0x40);
	/* Enable PCIB_DUAL_EN_UP will fix potential problem with PCI cards. */
	pci_conf1_write_config8(dev, 0x50, 0x01);

	/* SATA Device, BDF:0-18-0, Non-Raid-5 SATA controller */
	printk(BIOS_INFO, "sb600_devices_por_init(): SATA Device, BDF:0-18-0\n");
	pci_conf1_find_device(0x1002,  0x4380, &dev);

	/*PHY Global Control, we are using A14.
	 * default:  0x2c40 for ASIC revision A12 and below
	 *      0x2c00 for ASIC revision A13 and above.*/
	pci_conf1_write_config16(dev, 0x86, 0x2C00);

	/* PHY Port0-3 Control */
	pci_conf1_write_config32(dev, 0x88, 0xB400DA);
	pci_conf1_write_config32(dev, 0x8c, 0xB400DA);
	pci_conf1_write_config32(dev, 0x90, 0xB400DA);
	pci_conf1_write_config32(dev, 0x94, 0xB400DA);

	/* Port0-3 BIST Control/Status */
	pci_conf1_write_config8(dev, 0xa5, 0xB8);
	pci_conf1_write_config8(dev, 0xad, 0xB8);
	pci_conf1_write_config8(dev, 0xb5, 0xB8);
	pci_conf1_write_config8(dev, 0xbd, 0xB8);
}

/* sbPmioPorInitTable, Pre-initializing PMIO register space
* The power management (PM) block is resident in the PCI/LPC/ISA bridge.
* The PM regs are accessed via IO mapped regs 0xcd6 and 0xcd7.
* The index address is first programmed into IO reg 0xcd6.
* Read or write values are accessed through IO reg 0xcd7.
*/
static void sb600_pmio_por_init()
{
	u8 byte;

	printk(BIOS_INFO, "sb600_pmio_por_init()\n");
	/* K8KbRstEn, KB_RST# control for K8 system. */
	byte = pmio_read(0x66);
	byte |= 0x20;
	pmio_write(0x66, byte);

	/* RPR2.3.4 S3/S4/S5 Function for the K8 Platform. */
	byte = pmio_read(0x52);
	byte &= 0xc0;
	byte |= 0x08;
	pmio_write(0x52, byte);

	/* C state enable and SLP enable in C states. */
	byte = pmio_read(0x67);
	byte |= 0x6;
	pmio_write(0x67, byte);

	/* CIM sets 0x0e, but bit2 is for P4 system. */
	byte = pmio_read(0x68);
	byte &= 0xf0;
	byte |= 0x0c;
	pmio_write(0x68, byte);

	/* Watch Dog Timer Control
	 * Set watchdog time base to 0xfec000f0 to avoid SCSI card boot failure.
	 * But I don't find WDT is enabled in SMBUS 0x41 bit3 in CIM.
	 */
	pmio_write(0x6c, 0xf0);
	pmio_write(0x6d, 0x00);
	pmio_write(0x6e, 0xc0);
	pmio_write(0x6f, 0xfe);

	/* rpr2.14: Enables HPET periodical mode */
	byte = pmio_read(0x9a);
	byte |= 1 << 7;
	pmio_write(0x9a, byte);
	byte = pmio_read(0x9f);
	byte |= 1 << 5;
	pmio_write(0x9f, byte);
	byte = pmio_read(0x9e);
	byte |= (1 << 6) | (1 << 7);
	pmio_write(0x9e, byte);

	/* rpr2.14: Hides SM bus controller Bar1 where stores HPET MMIO base address */
	byte = pmio_read(0x55);
	byte |= 1 << 7;
	pmio_write(0x55, byte);

	/* rpr2.14: Make HPET MMIO decoding controlled by the memory enable bit in command register of LPC ISA bridage */
	byte = pmio_read(0x52);
	byte |= 1 << 6;
	pmio_write(0x52, byte);

	/* rpr2.22: PLL Reset */
	byte = pmio_read(0x86);
	byte |= 1 << 7;
	pmio_write(0x86, byte);

	/* rpr2.3.3 */
	/* This provides 16us delay before the assertion of LDTSTP# when C3 is entered.
	* The delay will allow USB DMA to go on in a continuous manner
	*/
	pmio_write(0x89, 0x10);
	/* Set this bit to allow pop-up request being latched during the minimum LDTSTP# assertion time */
	byte = pmio_read(0x52);
	byte |= 1 << 7;
	pmio_write(0x52, byte);

	/* rpr2.15: ASF Remote Control Action */
	byte = pmio_read(0x9f);
	byte |= 1 << 6;
	pmio_write(0x9f, byte);

	/* rpr2.19: Enabling Spread Spectrum */
	byte = pmio_read(0x42);
	byte |= 1 << 7;
	pmio_write(0x42, byte);
}

/*
* Compliant with CIM_48's sbPciCfg.
* Add any south bridge setting.
*/
static void sb600_pci_cfg()
{
	u32 dev;
	u8 byte;

	/* SMBus Device, BDF:0-20-0 */
	pci_conf1_find_device(0x1002,  0x4385, &dev);
	/* Eable the hidden revision ID, available after A13. */
	byte = pci_conf1_read_config8(dev, 0x70);
	byte |= (1 << 8);
	pci_conf1_write_config8(dev, 0x70, byte);
	/* rpr2.20 Disable Timer IRQ Enhancement for proper operation of the 8254 timer, 0xae[5]. */
	byte = pci_conf1_read_config8(dev, 0xae);
	byte |= (1 << 5);
	pci_conf1_write_config8(dev, 0xae, byte);

	/* Enable watchdog decode timer */
	byte = pci_conf1_read_config8(dev, 0x41);
	byte |= (1 << 3);
	pci_conf1_write_config8(dev, 0x41, byte);

	/* Set to 1 to reset USB on the software (such as IO-64 or IO-CF9 cycles) 
	 * generated PCIRST#. */
	byte = pmio_read(0x65);
	byte |= (1 << 4);
	pmio_write(0x65, byte);
	/*For A13 and above. */
	if (get_sb600_revision() > 0x12) {
		/* rpr2.16 C-State Reset, PMIO 0x9f[7]. */
		byte = pmio_read(0x9f);
		byte |= (1 << 7);
		pmio_write(0x9f, byte);
		/* rpr2.17 PCI Clock Period will increase to 30.8ns. 0x53[7]. */
		byte = pmio_read(0x53);
		byte |= (1 << 7);
		pmio_write(0x53, byte);
	}

	/* IDE Device, BDF:0-20-1 */
	pci_conf1_find_device(0x1002,  0x438C, &dev);
	/* Enable IDE Explicit prefetch, 0x63[0] clear */
	byte = pci_conf1_read_config8(dev, 0x63);
	byte &= 0xfe;
	pci_conf1_write_config8(dev, 0x63, byte);

	/* LPC Device, BDF:0-20-3 */
	pci_conf1_find_device(0x1002,  0x438D, &dev);
	/* rpr7.2 Enabling LPC DMA function. */
	byte = pci_conf1_read_config8(dev, 0x40);
	byte |= (1 << 2);
	pci_conf1_write_config8(dev, 0x40, byte);
	/* rpr7.3 Disabling LPC TimeOut. 0x48[7] clear. */
	byte = pci_conf1_read_config8(dev, 0x48);
	byte &= 0x7f;
	pci_conf1_write_config8(dev, 0x48, byte);
	/* rpr7.5 Disabling LPC MSI Capability, 0x78[1] clear. */
	byte = pci_conf1_read_config8(dev, 0x78);
	byte &= 0xfd;
	pci_conf1_write_config8(dev, 0x78, byte);

	/* SATA Device, BDF:0-18-0, Non-Raid-5 SATA controller */
	pci_conf1_find_device(0x1002,  0x4380, &dev);
	/* rpr6.8 Disabling SATA MSI Capability, for A13 and above, 0x42[7]. */
	if (0x12 < get_sb600_revision()) {
		u32 reg32;
		reg32 = pci_conf1_read_config32(dev, 0x40);
		reg32 |= (1 << 23);
		pci_conf1_write_config32(dev, 0x40, reg32);
	}

	/* EHCI Device, BDF:0-19-5, ehci usb controller */
	pci_conf1_find_device(0x1002,  0x4386, &dev);
	/* rpr5.10 Disabling USB EHCI MSI Capability. 0x50[6]. */
	byte = pci_conf1_read_config8(dev, 0x50);
	byte |= (1 << 6);
	pci_conf1_write_config8(dev, 0x50, byte);

	/* OHCI0 Device, BDF:0-19-0, ohci usb controller #0 */
	pci_conf1_find_device(0x1002,  0x4387, &dev);
	/* rpr5.11 Disabling USB OHCI MSI Capability. 0x40[12:8]=0x1f. */
	byte = pci_conf1_read_config8(dev, 0x41);
	byte |= 0x1f;
	pci_conf1_write_config8(dev, 0x41, byte);

}

/*
* Compliant with CIM_48's ATSBPowerOnResetInitJSP
*/
static void sb600_por_init()
{
	/* sbDevicesPorInitTable + sbK8PorInitTable */
	sb600_devices_por_init();

	/* sbPmioPorInitTable + sbK8PmioPorInitTable */
	sb600_pmio_por_init();
}

/*
* Compliant with CIM_48's AtiSbBeforePciInit
* It should be called during early POST after memory detection and BIOS shadowing but before PCI bus enumeration.
*/
static void sb600_before_pci_init()
{
	sb600_pci_cfg();
}

/*
* This function should be called after enable_sb600_smbus().
*/
static void sb600_stage1(void)
{
	printk(BIOS_INFO, "sb600_early_setup()\n");
	sb600_por_init();
}
