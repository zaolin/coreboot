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

/*
 *  for rs780 internal graphics device
 *  device id of internal grphics:
 *	RS780:	0x9610
 *	RS780C:	0x9611
 *	RS780M:	0x9612
 *	RS780MC:0x9613
 *	RS780E: 0x9615
 *	RS785G: 0x9710 - just works, not much tested
 */
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <delay.h>
#include <cpu/x86/msr.h>
#include "rs780.h"

void set_pcie_reset(void);
void set_pcie_dereset(void);

#define CLK_CNTL_INDEX	0x8
#define CLK_CNTL_DATA	0xC

/* The Integrated Info Table. */
ATOM_INTEGRATED_SYSTEM_INFO_V2 vgainfo;

static u32 clkind_read(device_t dev, u32 index)
{
	u32	gfx_bar2 = pci_read_config32(dev, 0x18) & ~0xF;

	*(u32*)(gfx_bar2+CLK_CNTL_INDEX) = index & 0x7F;
	return *(u32*)(gfx_bar2+CLK_CNTL_DATA);
}

static void clkind_write(device_t dev, u32 index, u32 data)
{
	u32	gfx_bar2 = pci_read_config32(dev, 0x18) & ~0xF;
	/* printk(BIOS_DEBUG, "gfx bar 2 %02x\n", gfx_bar2); */

	*(u32*)(gfx_bar2+CLK_CNTL_INDEX) = index | 1<<7;
	*(u32*)(gfx_bar2+CLK_CNTL_DATA)  = data;
}

/*
* pci_dev_read_resources thinks it is a IO type.
* We have to force it to mem type.
*/
static void rs780_gfx_read_resources(device_t dev)
{
	printk(BIOS_DEBUG, "rs780_gfx_read_resources.\n");

	/* The initial value of 0x24 is 0xFFFFFFFF, which is confusing.
	   Even if we write 0xFFFFFFFF into it, it will be 0xFFF00000,
	   which tells us it is a memory address base.
	 */
	pci_write_config32(dev, 0x24, 0x00000000);

	/* Get the normal pci resources of this device */
	pci_dev_read_resources(dev);
	compact_resources(dev);
}

typedef struct _MMIORANGE
{
	u32	Base;
	u32	Limit;
	u8	Attribute;
} MMIORANGE;

MMIORANGE MMIO[8], CreativeMMIO[8];

static MMIORANGE* AllocMMIO(MMIORANGE* pMMIO)
{
	int i;
	for (i=0; i<8; i++)
	{
		if (pMMIO[i].Limit == 0)
				return &pMMIO[i];
	}
	return 0;
}
static void FreeMMIO(MMIORANGE* pMMIO)
{
	pMMIO->Base = 0;
	pMMIO->Limit = 0;
}

#define CIM_STATUS u32
#define CIM_SUCCESS 0x00000000
#define CIM_ERROR	0x80000000
#define CIM_UNSUPPORTED	0x80000001
#define CIM_DISABLEPORT 0x80000002

#define	MMIO_ATTRIB_NP_ONLY	1
#define MMIO_ATTRIB_BOTTOM_TO_TOP 1<<1
#define MMIO_ATTRIB_SKIP_ZERO 1<<2

static u32 SetMMIO(u32 Base, u32 Limit, u8 Attribute, MMIORANGE *pMMIO)
{
	int i;
	MMIORANGE * TempRange;
	for(i=0; i<8; i++)
	{
		if(pMMIO[i].Attribute != Attribute && Base >= pMMIO[i].Base && Limit <= pMMIO[i].Limit)
		{
			TempRange = AllocMMIO(pMMIO);
			if(TempRange == 0) return 0x80000000;
			TempRange->Base = Limit;
			TempRange->Limit = pMMIO[i].Limit;
			TempRange->Attribute = pMMIO[i].Attribute;
			pMMIO[i].Limit = Base;
		}
	}
	TempRange = AllocMMIO(pMMIO);
	if(TempRange == 0) return 0x80000000;
	TempRange->Base = Base;
	TempRange->Limit = Limit;
	TempRange->Attribute = Attribute;
	return 0;
}

static u8 FinalizeMMIO(MMIORANGE *pMMIO)
{
	int i, j, n = 0;
	for(i=0; i<8; i++)
	{
		if (pMMIO[i].Base == pMMIO[i].Limit)
		{
			FreeMMIO(&pMMIO[i]);
			continue;
		}
		for(j=0; j<i; j++)
		{
			if (i!=j && pMMIO[i].Attribute == pMMIO[j].Attribute)
			{
				if (pMMIO[i].Base == pMMIO[j].Limit)
				{
					pMMIO[j].Limit = pMMIO[i].Limit;
					 FreeMMIO(&pMMIO[i]);
				}
				if (pMMIO[i].Limit == pMMIO[j].Base)
				{
					pMMIO[j].Base = pMMIO[i].Base;
				   FreeMMIO(&pMMIO[i]);
				}
			}
		}
	}
	for (i=0; i<8; i++)
	{
		if (pMMIO[i].Limit != 0) n++;
	}
	return n;
}

static CIM_STATUS GetCreativeMMIO(MMIORANGE *pMMIO)
{
	CIM_STATUS Status = CIM_UNSUPPORTED;
	u8 Bus, Dev, Reg, BusStart, BusEnd;
	u32	Value;
	device_t dev0x14 = dev_find_slot(0, PCI_DEVFN(0x14, 4));
	device_t tempdev;
	Value = pci_read_config32(dev0x14, 0x18);
	BusStart = (Value >> 8) & 0xFF;
	BusEnd = (Value >> 16) & 0xFF;
	for(Bus = BusStart; Bus <= BusEnd; Bus++)
	{
		for(Dev = 0; Dev <= 0x1f; Dev++)
		{
			tempdev = dev_find_slot(Bus, Dev << 3);
			Value = pci_read_config32(tempdev, 0);
			printk(BIOS_DEBUG, "Dev ID %x \n", Value);
			if((Value & 0xffff) == 0x1102)
			{//Creative
				//Found Creative SB
			 	u32	MMIOStart = 0xffffffff;
				u32 MMIOLimit = 0;
				for(Reg = 0x10; Reg < 0x20; Reg+=4)
				{
					u32	BaseA, LimitA;
					BaseA = pci_read_config32(tempdev, Reg);
					Value = BaseA;
					if(!(Value & 0x01))
					{
						Value = Value & 0xffffff00;
						if(Value !=  0)
						{
							if(MMIOStart > Value)
								MMIOStart = Value;
							LimitA = 0xffffffff;
							//WritePCI(PciAddress,AccWidthUint32,&LimitA);
							pci_write_config32(tempdev, Reg, LimitA);
							//ReadPCI(PciAddress,AccWidthUint32,&LimitA);
							LimitA = pci_read_config32(tempdev, Reg);
							LimitA = Value + (~LimitA + 1);
							//WritePCI(PciAddress,AccWidthUint32,&BaseA);
							pci_write_config32(tempdev, Reg, BaseA);
							if (LimitA > MMIOLimit)
								MMIOLimit = LimitA;
						}
					}
				}
				printk(BIOS_DEBUG, " MMIOStart %x MMIOLimit %x \n", MMIOStart, MMIOLimit);
				if (MMIOStart < MMIOLimit)
				{
					Status = SetMMIO(MMIOStart>>8, MMIOLimit>>8, 0x80, pMMIO);
					if(Status == CIM_ERROR) return Status;
				}
			}
		}
	}
	if(Status == CIM_SUCCESS)
	{
		//Lets optimize MMIO
		if(FinalizeMMIO(pMMIO) > 4)
		{
			Status = CIM_ERROR;
		}
	}

	return Status;
}

static void ProgramMMIO(MMIORANGE *pMMIO, u8 LinkID, u8 Attribute)
{
	int i, j, n = 7;
	device_t k8_f1;

	k8_f1 = dev_find_slot(0, PCI_DEVFN(0x18, 1));

	for(i = 0; i < 8; i++)
	{
		int k = 0, MmioReg;
		u32 Base = 0;
		u32 Limit = 0;
		for(j = 0; j < 8; j++)
		{
			if (Base < pMMIO[j].Base)
			{
				Base = pMMIO[j].Base;
				k = j;
			}
		}
		if(pMMIO[k].Limit != 0)
		{
			if(Attribute & MMIO_ATTRIB_NP_ONLY && pMMIO[k].Attribute == 0 )
			{
				Base = 0;
			}
			else
			{
				Base = pMMIO[k].Base | 0x3;
				Limit= ((pMMIO[k].Limit - 1) & 0xffffff00) | pMMIO[k].Attribute | (LinkID << 4);
			}
			FreeMMIO(&pMMIO[k]);
		}
		if (Attribute & MMIO_ATTRIB_SKIP_ZERO && Base == 0 && Limit == 0) continue;
		MmioReg = (Attribute & MMIO_ATTRIB_BOTTOM_TO_TOP)?n:(7-n);
		n--;
		//RWPCI(PCI_ADDRESS(0,CPU_DEV,CPU_F1,0x80+MmioReg*8),AccWidthUint32 |S3_SAVE,0x0,0x0);
		pci_write_config32(k8_f1, 0x80+MmioReg*8, 0);

		//WritePCI(PCI_ADDRESS(0,CPU_DEV,CPU_F1,0x84+MmioReg*8),AccWidthUint32 |S3_SAVE,&Limit);
		pci_write_config32(k8_f1, 0x84+MmioReg*8, Limit);

		//WritePCI(PCI_ADDRESS(0,CPU_DEV,CPU_F1,0x80+MmioReg*8),AccWidthUint32 |S3_SAVE,&Base);
		pci_write_config32(k8_f1, 0x80+MmioReg*8, Base);
	}
}

static void internal_gfx_pci_dev_init(struct device *dev)
{
	unsigned char * bpointer;
	volatile u32 * GpuF0MMReg;
	volatile u32 * pointer;
	int i;
	u16 command;
	u32 value;
	u16 deviceid, vendorid;
	device_t nb_dev = dev_find_slot(0, 0);
	device_t k8_f2 = dev_find_slot(0, PCI_DEVFN(0x18, 2));
	device_t k8_f0 = dev_find_slot(0, PCI_DEVFN(0x18, 0));

	/* We definetely will use this in future. Just leave it here. */
	/*struct southbridge_amd_rs780_config *cfg =
	   (struct southbridge_amd_rs780_config *)dev->chip_info;*/

	deviceid = pci_read_config16(dev, PCI_DEVICE_ID);
	vendorid = pci_read_config16(dev, PCI_VENDOR_ID);
	printk(BIOS_DEBUG, "internal_gfx_pci_dev_init device=%x, vendor=%x.\n",
	     deviceid, vendorid);

	command = pci_read_config16(dev, 0x04);
	command |= 0x7;
	pci_write_config16(dev, 0x04, command);

	/* Clear vgainfo. */
	bpointer = (unsigned char *) &vgainfo;
	for(i=0; i<sizeof(ATOM_INTEGRATED_SYSTEM_INFO_V2); i++)
	{
		*bpointer = 0;
		bpointer++;
	}

	GpuF0MMReg = (u32 *)pci_read_config32(dev, 0x18);

	/* GFX_InitFBAccess. */
	value = nbmc_read_index(nb_dev, 0x10);
	*(GpuF0MMReg + 0x2000/4) = 0x11;
	*(GpuF0MMReg + 0x2180/4) = ((value&0xff00)>>8)|((value&0xff000000)>>8);
	*(GpuF0MMReg + 0x2c04/4) = ((value&0xff0)<<8);
	*(GpuF0MMReg + 0x5428/4) = ((value&0xffff0000)+0x10000)-((value&0xffff)<<16);
	*(GpuF0MMReg + 0x2000/4) = 0x00000011;
	*(GpuF0MMReg + 0x200c/4) = 0x00000020;
	*(GpuF0MMReg + 0x2010/4) = 0x10204810;
	*(GpuF0MMReg + 0x2010/4) = 0x00204810;
	*(GpuF0MMReg + 0x2014/4) = 0x10408810;
	*(GpuF0MMReg + 0x2014/4) = 0x00408810;
	*(GpuF0MMReg + 0x2414/4) = 0x00000080;
	*(GpuF0MMReg + 0x2418/4) = 0x84422415;
	*(GpuF0MMReg + 0x2418/4) = 0x04422415;
	*(GpuF0MMReg + 0x5490/4) = 0x00000001;
	*(GpuF0MMReg + 0x7de4/4) |= (1<<3) | (1<<4);
	/* Force allow LDT_STOP Cool'n'Quiet workaround. */
	*(GpuF0MMReg + 0x655c/4) |= 1<<4;
	/* GFX_InitFBAccess finished. */

	/* GFX_StartMC. */
#if (CONFIG_GFXUMA == 1) /* for UMA mode. */
	/* MC_INIT_COMPLETE. */
	set_nbmc_enable_bits(nb_dev, 0x2, 0, 1<<31);
	/* MC_STARTUP, MC_POWERED_UP and MC_VMODE.*/
	set_nbmc_enable_bits(nb_dev, 0x1, 1<<18, 1|1<<2);

	set_nbmc_enable_bits(nb_dev, 0xb1, 0, 1<<6);
	set_nbmc_enable_bits(nb_dev, 0xc3, 0, 1);
	nbmc_write_index(nb_dev, 0x07, 0x18);
	nbmc_write_index(nb_dev, 0x06, 0x00000102);
	nbmc_write_index(nb_dev, 0x09, 0x40000008);
	set_nbmc_enable_bits(nb_dev, 0x6, 0, 1<<31);
	/* GFX_StartMC finished. */
#else
	/* for SP mode. */
	set_nbmc_enable_bits(nb_dev, 0xaa, 0xf0, 0x30);
	set_nbmc_enable_bits(nb_dev, 0xce, 0xf0, 0x30);
	set_nbmc_enable_bits(nb_dev, 0xca, 0xff000000, 0x47000000);
	set_nbmc_enable_bits(nb_dev, 0xcb, 0x3f000000, 0x01000000);
	set_nbmc_enable_bits(nb_dev, 0x01, 0, 1<<0);
	set_nbmc_enable_bits(nb_dev, 0x04, 0, 1<<31);
	set_nbmc_enable_bits(nb_dev, 0xb4, 0x3f, 0x3f);
	set_nbmc_enable_bits(nb_dev, 0xb4, 0, 1<<6);
	set_nbmc_enable_bits(nb_dev, 0xc3, 1<<11, 0);
	set_nbmc_enable_bits(nb_dev, 0xa0, 1<<29, 0);
	nbmc_write_index(nb_dev, 0xa4, 0x3484576f);
	nbmc_write_index(nb_dev, 0xa5, 0x222222df);
	nbmc_write_index(nb_dev, 0xa6, 0x00000000);
	nbmc_write_index(nb_dev, 0xa7, 0x00000000);
	set_nbmc_enable_bits(nb_dev, 0xc3, 1<<8, 0);
	udelay(10);
	set_nbmc_enable_bits(nb_dev, 0xc3, 1<<9, 0);
	udelay(10);
	set_nbmc_enable_bits(nb_dev, 0x01, 0, 1<<2);
	udelay(200);
	set_nbmc_enable_bits(nb_dev, 0x01, 0, 1<<3);
	set_nbmc_enable_bits(nb_dev, 0xa0, 0, 1<<31);
	udelay(500);
	set_nbmc_enable_bits(nb_dev, 0x02, 0, 1<<31);
	set_nbmc_enable_bits(nb_dev, 0xa0, 0, 1<<30);
	set_nbmc_enable_bits(nb_dev, 0xa0, 1<<31, 0);
	set_nbmc_enable_bits(nb_dev, 0xa0, 0, 1<<29);
	nbmc_write_index(nb_dev, 0xa4, 0x23484576);
	nbmc_write_index(nb_dev, 0xa5, 0x00000000);
	nbmc_write_index(nb_dev, 0xa6, 0x00000000);
	nbmc_write_index(nb_dev, 0xa7, 0x00000000);
	/* GFX_StartMC finished. */

	/* GFX_SPPowerManagment, don't care for new. */
	/* Post MC Init table programming. */
	set_nbmc_enable_bits(nb_dev, 0xac, ~(0xfffffff0), 0x0b);

	/* Do we need Write and Read Calibration? */
	/* GFX_Init finished. */
#endif

	/* GFX_InitIntegratedInfo. */
	/* fill the Integrated Info Table. */
	vgainfo.sHeader.usStructureSize = sizeof(ATOM_INTEGRATED_SYSTEM_INFO_V2);
	vgainfo.sHeader.ucTableFormatRevision = 1;
	vgainfo.sHeader.ucTableContentRevision = 2;

#if (CONFIG_GFXUMA == 0) /* SP mode. */
	vgainfo.ulBootUpSidePortClock = 667*100;
	vgainfo.ucMemoryType = 3;
	vgainfo.ulMinSidePortClock = 333*100;
#endif

	vgainfo.ulBootUpEngineClock = 500 * 100; /* set boot up GFX engine clock. */
	vgainfo.ulReserved1[0] = 0;	vgainfo.ulReserved1[1] = 0;
	value = pci_read_config32(k8_f2, 0x94);
	printk(BIOS_DEBUG, "MEMCLK = %x\n", value&0x7);
	vgainfo.ulBootUpUMAClock = 333 * 100; /* set boot up UMA memory clock. */
	vgainfo.ulBootUpSidePortClock = 0; /* disable SP. */
	vgainfo.ulMinSidePortClock = 0; /* disable SP. */
	for(i=0; i<6; i++)
		vgainfo.ulReserved2[i] = 0;
	vgainfo.ulSystemConfig = 0;
	//vgainfo.ulSystemConfig |= 1<<1 | 1<<3 | 1<<4 | 1<<5 | 1<<6 | 1<<7 | 1;
	vgainfo.ulBootUpReqDisplayVector = 0; //?
	vgainfo.ulOtherDisplayMisc = 0; //?
	vgainfo.ulDDISlot1Config = 0x000c0011; //0; //VGA
	//vgainfo.ulDDISlot1Config = 0x000c00FF; //0; //HDMI
	vgainfo.ulDDISlot2Config = 0x00130022; //0; //?
	vgainfo.ucMemoryType = 2;
	/* UMA Channel Number: 1 or 2. */
	vgainfo.ucUMAChannelNumber = 2;
	vgainfo.ucDockingPinBit = 0; //?
	vgainfo.ucDockingPinPolarity = 0; //?
	vgainfo.ulDockingPinCFGInfo = 0; //?
	vgainfo.ulCPUCapInfo = 3; /* K8. */

	/* page 5-19 on BDG. */
	vgainfo.usNumberOfCyclesInPeriod = 0x8019;
	vgainfo.usMaxNBVoltage = 0x1a;
	vgainfo.usMinNBVoltage = 0;
	vgainfo.usBootUpNBVoltage = 0x1a;

	value = pci_read_config32(nb_dev, 0xd0);
	printk(BIOS_DEBUG, "NB HT speed = %x.\n", value);
	value = pci_read_config32(k8_f0, 0x88);
	printk(BIOS_DEBUG, "CPU HT speed = %x.\n", value);
	vgainfo.ulHTLinkFreq = 100 * 100; /* set HT speed. */

	/* HT width. */
	value = pci_read_config32(nb_dev, 0xc8);
	printk(BIOS_DEBUG, "HT width = %x.\n", value);
	vgainfo.usMinHTLinkWidth = 16;
	vgainfo.usMaxHTLinkWidth = 16;
	vgainfo.usUMASyncStartDelay = 322;
	vgainfo.usUMADataReturnTime = 86;
	vgainfo.usLinkStatusZeroTime = 0x00c8; //0; //?
	vgainfo.usReserved = 0;
	vgainfo.ulHighVoltageHTLinkFreq = 100 * 100;
	vgainfo.ulLowVoltageHTLinkFreq = 100 * 100;
	vgainfo.usMaxUpStreamHTLinkWidth = 16;
	vgainfo.usMaxDownStreamHTLinkWidth = 16;
	vgainfo.usMinUpStreamHTLinkWidth = 16;
	vgainfo.usMinDownStreamHTLinkWidth = 16;
	for(i=0; i<97; i++)
		vgainfo.ulReserved3[i] = 0;

	/* Transfer the Table to VBIOS. */
	pointer = (u32 *)&vgainfo;
	for(i=0; i<sizeof(ATOM_INTEGRATED_SYSTEM_INFO_V2); i+=4)
	{
#if (CONFIG_GFXUMA == 1)
		*GpuF0MMReg = 0x80000000 + 0x10000000 - 512 + i;
#else
		*GpuF0MMReg = 0x80000000 + 0x8000000 - 512 + i;
#endif
		*(GpuF0MMReg+1) = *pointer++;
	}

	/* GFX_InitLate. */
	{
		u8 temp8;
		temp8 = pci_read_config8(dev, 0x4);
		//temp8 &= ~1; /* CIM clears this bit. Strangely, I can'd. */
		temp8 |= 1<<1|1<<2;
		pci_write_config8(dev, 0x4, temp8);
	}

#if 0 /* Trust the original resource allocation. Don't do it again. */
	/* NB_SetupMGMMIO. */

	/* clear MMIO and CreativeMMIO. */
	bpointer = (unsigned char *)MMIO;
	for(i=0; i<sizeof(MMIO); i++)
	{
		*bpointer = 0;
		bpointer++;
	}
	bpointer = (unsigned char *)CreativeMMIO;
	for(i=0; i<sizeof(CreativeMMIO); i++)
	{
		*bpointer = 0;
		bpointer++;
	}

	/* Set MMIO ranges in K8. */
	/* Set MMIO TOM - 4G. */
	SetMMIO(0x400<<12, 0x1000000, 0x80, &MMIO[0]);
	/* Set MMIO for VGA Legacy FB. */
	SetMMIO(0xa00, 0xc00, 0x80, &MMIO[0]);

	/* Set MMIO for non prefetchable P2P. */
	temp = pci_read_config32(dev0x14, 0x20);
	Base32 = (temp & 0x0fff0) << 8;
	Limit32 = ((temp & 0x0fff00000) + 0x100000) >> 8;
	if(Base32 < Limit32)
	{
		Status = GetCreativeMMIO(&CreativeMMIO[0]);
		if(Status != CIM_ERROR)
			SetMMIO(Base32, Limit32, 0x0, &MMIO[0]);
	}
	/* Set MMIO for prefetchable P2P. */
	if(Status != CIM_ERROR)
	{
		temp = pci_read_config32(dev0x14, 0x24);

		Base32 = (temp & 0x0fff0) <<8;
		Limit32 = ((temp & 0x0fff00000) + 0x100000) >> 8;
		if(Base32 < Limit32)
			SetMMIO(Base32, Limit32, 0x0, &MMIO[0]);
	}

	FinalizeMMIO(&MMIO[0]);

	ProgramMMIO(&CreativeMMIO[0], 0, MMIO_ATTRIB_NP_ONLY);
	ProgramMMIO(&MMIO[0], 0, MMIO_ATTRIB_NP_ONLY | MMIO_ATTRIB_BOTTOM_TO_TOP | MMIO_ATTRIB_SKIP_ZERO);
#endif

	pci_dev_init(dev);

	/* clk ind */
	clkind_write(dev, 0x08, 0x01);
	clkind_write(dev, 0x0C, 0x22);
	clkind_write(dev, 0x0F, 0x0);
	clkind_write(dev, 0x11, 0x0);
	clkind_write(dev, 0x12, 0x0);
	clkind_write(dev, 0x14, 0x0);
	clkind_write(dev, 0x15, 0x0);
	clkind_write(dev, 0x16, 0x0);
	clkind_write(dev, 0x17, 0x0);
	clkind_write(dev, 0x18, 0x0);
	clkind_write(dev, 0x19, 0x0);
	clkind_write(dev, 0x1A, 0x0);
	clkind_write(dev, 0x1B, 0x0);
	clkind_write(dev, 0x1C, 0x0);
	clkind_write(dev, 0x1D, 0x0);
	clkind_write(dev, 0x1E, 0x0);
	clkind_write(dev, 0x26, 0x0);
	clkind_write(dev, 0x27, 0x0);
	clkind_write(dev, 0x28, 0x0);
	clkind_write(dev, 0x5C, 0x0);
}


/*
* Set registers in RS780 and CPU to enable the internal GFX.
* Please refer to CIM source code and BKDG.
*/
extern uint64_t uma_memory_base, uma_memory_size;

static void rs780_internal_gfx_enable(device_t dev)
{
	u32 l_dword;
	int i;
	device_t k8_f0 = 0, k8_f2 = 0;
	device_t nb_dev = dev_find_slot(0, 0);
	msr_t sysmem;

#if (CONFIG_GFXUMA == 0)
	u32 FB_Start, FB_End;
#endif

	printk(BIOS_DEBUG, "rs780_internal_gfx_enable dev = 0x%p, nb_dev = 0x%p.\n", dev, nb_dev);

	sysmem = rdmsr(0xc001001a);
	printk(BIOS_DEBUG, "sysmem = %x_%x\n", sysmem.hi, sysmem.lo);

	/* The system top memory in 780. */
	pci_write_config32(nb_dev, 0x90, sysmem.lo);
	htiu_write_index(nb_dev, 0x30, 0);
	htiu_write_index(nb_dev, 0x31, 0);

	/* Disable external GFX and enable internal GFX. */
	l_dword = pci_read_config32(nb_dev, 0x8c);
	l_dword &= ~(1<<0);
	l_dword |= 1<<1;
	pci_write_config32(nb_dev, 0x8c, l_dword);

	/* NB_SetDefaultIndexes */
	pci_write_config32(nb_dev, 0x94, 0x7f);
	pci_write_config32(nb_dev, 0x60, 0x7f);
	pci_write_config32(nb_dev, 0xe0, 0);

	/* NB_InitEarlyNB finished. */

	/* LPC DMA Deadlock workaround? */
	/* GFX_InitCommon*/
	k8_f0 = dev_find_slot(0, PCI_DEVFN(0x18, 0));
	l_dword = pci_read_config32(k8_f0, 0x68);
	l_dword &= ~(3 << 21);
	l_dword |= (1 << 21);
	pci_write_config32(k8_f0, 0x68, l_dword);

	/* GFX_InitCommon. */
	nbmc_write_index(nb_dev, 0x23, 0x00c00010);
	set_nbmc_enable_bits(nb_dev, 0x16, 1<<15, 1<<15);
	set_nbmc_enable_bits(nb_dev, 0x25, 0xffffffff, 0x111f111f);
	set_htiu_enable_bits(nb_dev, 0x37, 1<<24, 1<<24);

#if (CONFIG_GFXUMA == 1)
	/* GFX_InitUMA. */
	/* Copy CPU DDR Controller to NB MC. */
	k8_f2 = dev_find_slot(0, PCI_DEVFN(0x18, 2));
	for (i = 0; i < 12; i++)
	{
		l_dword = pci_read_config32(k8_f2, 0x40 + i * 4);
		nbmc_write_index(nb_dev, 0x30 + i, l_dword);
	}

	l_dword = pci_read_config32(k8_f2, 0x80);
	nbmc_write_index(nb_dev, 0x3c, l_dword);

	l_dword = pci_read_config32(k8_f2, 0x94);
	if(l_dword & (1<<22))
		set_nbmc_enable_bits(nb_dev, 0x3c, 0, 1<<16);
	else
		set_nbmc_enable_bits(nb_dev, 0x3c, 1<<16, 0);

	if(l_dword & (1<<8))
		set_nbmc_enable_bits(nb_dev, 0x3c, 0, 1<<17);
	else
		set_nbmc_enable_bits(nb_dev, 0x3c, 1<<17, 0);

	l_dword = pci_read_config32(k8_f2, 0x90);
	if(l_dword & (1<<10))
		set_nbmc_enable_bits(nb_dev, 0x3c, 0, 1<<18);
	else
		set_nbmc_enable_bits(nb_dev, 0x3c, 1<<18, 0);

	/* Set UMA in the 780 side. */
	/* UMA start address, size. */
	/* The same value in spite of system memory size. */
	nbmc_write_index(nb_dev, 0x10, 0xcfffc000);
	nbmc_write_index(nb_dev, 0x11, uma_memory_base);
	nbmc_write_index(nb_dev, 0x12, 0);
	nbmc_write_index(nb_dev, 0xf0, 256);
	/* GFX_InitUMA finished. */
#else
	/* GFX_InitSP. */
	/* SP memory:Hynix HY5TQ1G631ZNFP. 128MB = 64M * 16. 667MHz. DDR3. */

	/* Enable Async mode. */
	set_nbmc_enable_bits(nb_dev, 0x06, 7<<8, 1<<8);
	set_nbmc_enable_bits(nb_dev, 0x08, 1<<10, 0);
	/* The last item in AsynchMclkTaskFileIndex. Why? */
	/* MC_MPLL_CONTROL2. */
	nbmc_write_index(nb_dev, 0x07, 0x40100028);
	/* MC_MPLL_DIV_CONTROL. */
	nbmc_write_index(nb_dev, 0x0b, 0x00000028);
	/* MC_MPLL_FREQ_CONTROL. */
	set_nbmc_enable_bits(nb_dev, 0x09, 3<<12|15<<16|15<<8, 1<<12|4<<16|0<<8);
	/* MC_MPLL_CONTROL3. For PM. */
	set_nbmc_enable_bits(nb_dev, 0x08, 0xff<<13, 1<<13|1<<18);
	/* MPLL_CAL_TRIGGER. */
	set_nbmc_enable_bits(nb_dev, 0x06, 0, 1<<0);
	udelay(200); /* time is long enough? */
	set_nbmc_enable_bits(nb_dev, 0x06, 0, 1<<1);
	set_nbmc_enable_bits(nb_dev, 0x06, 1<<0, 0);
	/* MCLK_SRC_USE_MPLL. */
	set_nbmc_enable_bits(nb_dev, 0x02, 0, 1<<20);

	/* Pre Init MC. */
	nbmc_write_index(nb_dev, 0x01, 0x88108280);
	set_nbmc_enable_bits(nb_dev, 0x02, ~(1<<20), 0x00030200);
	nbmc_write_index(nb_dev, 0x04, 0x08881018);
	nbmc_write_index(nb_dev, 0x05, 0x000000bb);
	nbmc_write_index(nb_dev, 0x0c, 0x0f00001f);
	nbmc_write_index(nb_dev, 0xa1, 0x01f10000);
	/* MCA_INIT_DLL_PM. */
	set_nbmc_enable_bits(nb_dev, 0xc9, 1<<24, 1<<24);
	nbmc_write_index(nb_dev, 0xa2, 0x74f20000);
	nbmc_write_index(nb_dev, 0xa3, 0x8af30000);
	nbmc_write_index(nb_dev, 0xaf, 0x47d0a41c);
	nbmc_write_index(nb_dev, 0xb0, 0x88800130);
	nbmc_write_index(nb_dev, 0xb1, 0x00000040);
	nbmc_write_index(nb_dev, 0xb4, 0x41247000);
	nbmc_write_index(nb_dev, 0xb5, 0x00066664);
	nbmc_write_index(nb_dev, 0xb6, 0x00000022);
	nbmc_write_index(nb_dev, 0xb7, 0x00000044);
	nbmc_write_index(nb_dev, 0xb8, 0xbbbbbbbb);
	nbmc_write_index(nb_dev, 0xb9, 0xbbbbbbbb);
	nbmc_write_index(nb_dev, 0xba, 0x55555555);
	nbmc_write_index(nb_dev, 0xc1, 0x00000000);
	nbmc_write_index(nb_dev, 0xc2, 0x00000000);
	nbmc_write_index(nb_dev, 0xc3, 0x80006b00);
	nbmc_write_index(nb_dev, 0xc4, 0x00066664);
	nbmc_write_index(nb_dev, 0xc5, 0x00000000);
	nbmc_write_index(nb_dev, 0xd2, 0x00000022);
	nbmc_write_index(nb_dev, 0xd3, 0x00000044);
	nbmc_write_index(nb_dev, 0xd6, 0x00050005);
	nbmc_write_index(nb_dev, 0xd7, 0x00000000);
	nbmc_write_index(nb_dev, 0xd8, 0x00700070);
	nbmc_write_index(nb_dev, 0xd9, 0x00700070);
	nbmc_write_index(nb_dev, 0xe0, 0x00200020);
	nbmc_write_index(nb_dev, 0xe1, 0x00200020);
	nbmc_write_index(nb_dev, 0xe8, 0x00200020);
	nbmc_write_index(nb_dev, 0xe9, 0x00200020);
	nbmc_write_index(nb_dev, 0xe0, 0x00180018);
	nbmc_write_index(nb_dev, 0xe1, 0x00180018);
	nbmc_write_index(nb_dev, 0xe8, 0x00180018);
	nbmc_write_index(nb_dev, 0xe9, 0x00180018);

	/* Misc options. */
	/* Memory Termination. */
	set_nbmc_enable_bits(nb_dev, 0xa1, 0x0ff, 0x044);
	set_nbmc_enable_bits(nb_dev, 0xb4, 0xf00, 0xb00);
#if 0
	/* Controller Termation. */
	set_nbmc_enable_bits(nb_dev, 0xb1, 0x77770000, 0x77770000);
#endif

	/* OEM Init MC. 667MHz. */
	nbmc_write_index(nb_dev, 0xa8, 0x7a5aaa78);
	nbmc_write_index(nb_dev, 0xa9, 0x514a2319);
	nbmc_write_index(nb_dev, 0xaa, 0x54400520);
	nbmc_write_index(nb_dev, 0xab, 0x441460ff);
	nbmc_write_index(nb_dev, 0xa0, 0x20f00a48);
	set_nbmc_enable_bits(nb_dev, 0xa2, ~(0xffffffc7), 0x10);
	nbmc_write_index(nb_dev, 0xb2, 0x00000303);
	set_nbmc_enable_bits(nb_dev, 0xb1, ~(0xffffff70), 0x45);
	/* Do it later. */
	/* set_nbmc_enable_bits(nb_dev, 0xac, ~(0xfffffff0), 0x0b); */

	/* Init PM timing. */
	for(i=0; i<4; i++)
	{
		l_dword = nbmc_read_index(nb_dev, 0xa0+i);
		nbmc_write_index(nb_dev, 0xc8+i, l_dword);
	}
	for(i=0; i<4; i++)
	{
		l_dword = nbmc_read_index(nb_dev, 0xa8+i);
		nbmc_write_index(nb_dev, 0xcc+i, l_dword);
	}
	l_dword = nbmc_read_index(nb_dev, 0xb1);
	set_nbmc_enable_bits(nb_dev, 0xc8, 0xff<<24, ((l_dword&0x0f)<<24)|((l_dword&0xf00)<<20));

	/* Init MC FB. */
	/* FB_Start = ; FB_End = ; iSpSize = 0x0080, 128MB. */
	nbmc_write_index(nb_dev, 0x11, 0x40000000);
	FB_Start = 0xc00 + 0x080;
	FB_End = 0xc00 + 0x080;
	nbmc_write_index(nb_dev, 0x10, (((FB_End&0xfff)<<20)-0x10000)|(((FB_Start&0xfff)-0x080)<<4));
	set_nbmc_enable_bits(nb_dev, 0x0d, ~0x000ffff0, (FB_Start&0xfff)<<20);
	nbmc_write_index(nb_dev, 0x0f, 0);
	nbmc_write_index(nb_dev, 0x0e, (FB_Start&0xfff)|(0xaaaa<<12));
#endif

	/* GFX_InitSP finished. */
}

static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

static struct device_operations pcie_ops = {
	.read_resources = rs780_gfx_read_resources,
	.set_resources = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init = internal_gfx_pci_dev_init,	/* The option ROM initializes the device. rs780_gfx_init, */
	.scan_bus = 0,
	.enable = rs780_internal_gfx_enable,
	.ops_pci = &lops_pci,
};

/*
 * We should list all of them here.
 * */
static const struct pci_driver pcie_driver_780 __pci_driver = {
	.ops = &pcie_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_RS780_INT_GFX,
};

static const struct pci_driver pcie_driver_780c __pci_driver = {
	.ops = &pcie_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_RS780C_INT_GFX,
};
static const struct pci_driver pcie_driver_780m __pci_driver = {
	.ops = &pcie_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_RS780M_INT_GFX,
};
static const struct pci_driver pcie_driver_780mc __pci_driver = {
	.ops = &pcie_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_RS780MC_INT_GFX,
};
static const struct pci_driver pcie_driver_780e __pci_driver = {
	.ops = &pcie_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_RS780E_INT_GFX,
};
static const struct pci_driver pcie_driver_785g __pci_driver = {
	.ops = &pcie_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_RS785G_INT_GFX,
};

/* step 12 ~ step 14 from rpr */
static void single_port_configuration(device_t nb_dev, device_t dev)
{
	u8 result, width;
	u32 reg32;
	struct southbridge_amd_rs780_config *cfg =
	    (struct southbridge_amd_rs780_config *)nb_dev->chip_info;

	printk(BIOS_DEBUG, "rs780_gfx_init single_port_configuration.\n");

	/* step 12 training, releases hold training for GFX port 0 (device 2) */
	PcieReleasePortTraining(nb_dev, dev, 2);
	result = PcieTrainPort(nb_dev, dev, 2);
	printk(BIOS_DEBUG, "rs780_gfx_init single_port_configuration step12.\n");

	/* step 13 Power Down Control */
	/* step 13.1 Enables powering down transmitter and receiver pads along with PLL macros. */
	set_pcie_enable_bits(nb_dev, 0x40, 1 << 0, 1 << 0);

	/* step 13.a Link Training was NOT successful */
	if (!result) {
		set_nbmisc_enable_bits(nb_dev, 0x8, 0, 0x3 << 4); /* prevent from training. */
		set_nbmisc_enable_bits(nb_dev, 0xc, 0, 0x3 << 2); /* hide the GFX bridge. */
		if (cfg->gfx_tmds)
			nbpcie_ind_write_index(nb_dev, 0x65, 0xccf0f0);
		else {
			nbpcie_ind_write_index(nb_dev, 0x65, 0xffffffff);
			set_nbmisc_enable_bits(nb_dev, 0x7, 1 << 3, 1 << 3);
		}
	} else {		/* step 13.b Link Training was successful */
		set_pcie_enable_bits(dev, 0xA2, 0xFF, 0x1);
		reg32 = nbpcie_p_read_index(dev, 0x29);
		width = reg32 & 0xFF;
		printk(BIOS_DEBUG, "GFX Inactive Lanes = 0x%x.\n", width);
		switch (width) {
		case 1:
		case 2:
			nbpcie_ind_write_index(nb_dev, 0x65,
					       cfg->gfx_lane_reversal ? 0x7f7f : 0xccfefe);
			break;
		case 4:
			nbpcie_ind_write_index(nb_dev, 0x65,
					       cfg->gfx_lane_reversal ? 0x3f3f : 0xccfcfc);
			break;
		case 8:
			nbpcie_ind_write_index(nb_dev, 0x65,
					       cfg->gfx_lane_reversal ? 0x0f0f : 0xccf0f0);
			break;
		}
	}
	printk(BIOS_DEBUG, "rs780_gfx_init single_port_configuration step13.\n");

	/* step 14 Reset Enumeration Timer, disables the shortening of the enumeration timer */
	set_pcie_enable_bits(dev, 0x70, 1 << 19, 1 << 19);
	printk(BIOS_DEBUG, "rs780_gfx_init single_port_configuration step14.\n");
}

static void dual_port_configuration(device_t nb_dev, device_t dev)
{
	u8 result, width;
	u32 reg32, dev_ind = dev->path.pci.devfn >> 3;
	struct southbridge_amd_rs780_config *cfg =
		    (struct southbridge_amd_rs780_config *)nb_dev->chip_info;

	/* 5.4.1.2 Dual Port Configuration */
	set_nbmisc_enable_bits(nb_dev, 0x36, 1 << 31, 1 << 31);
	set_nbmisc_enable_bits(nb_dev, 0x08, 0xF << 8, 0x5 << 8);
	set_nbmisc_enable_bits(nb_dev, 0x36, 1 << 31, 0 << 31);

	/* 5.7. Training for Device 2 */
	/* 5.7.1. Releases hold training for GFX port 0 (device 2) */
	PcieReleasePortTraining(nb_dev, dev, dev_ind);
	/* 5.7.2- 5.7.9. PCIE Link Training Sequence */
	result = PcieTrainPort(nb_dev, dev, dev_ind);

	/* Power Down Control for Device 2 */
	/* Link Training was NOT successful */
	if (!result) {
		/* Powers down all lanes for port A */
		/* nbpcie_ind_write_index(nb_dev, 0x65, 0x0f0f); */
		/* Note: I have to disable the slot where there isnt a device,
		 * otherwise the system will hang. I dont know why. */
		set_nbmisc_enable_bits(nb_dev, 0x0c, 1 << dev_ind, 1 << dev_ind);

	} else {		/* step 16.b Link Training was successful */
		reg32 = nbpcie_p_read_index(dev, 0xa2);
		width = (reg32 >> 4) & 0x7;
		printk(BIOS_DEBUG, "GFX LC_LINK_WIDTH = 0x%x.\n", width);
		switch (width) {
		case 1:
		case 2:
			nbpcie_ind_write_index(nb_dev, 0x65,
					       cfg->gfx_lane_reversal ? 0x0707 : 0x0e0e);
			break;
		case 4:
			nbpcie_ind_write_index(nb_dev, 0x65,
					       cfg->gfx_lane_reversal ? 0x0303 : 0x0c0c);
			break;
		}
	}
}

/* For single port GFX configuration Only
* width:
* 	000 = x16
* 	001 = x1
*	010 = x2
*	011 = x4
*	100 = x8
*	101 = x12 (not supported)
*	110 = x16
*/
static void dynamic_link_width_control(device_t nb_dev, device_t dev, u8 width)
{
	u32 reg32;
	device_t sb_dev;
	struct southbridge_amd_rs780_config *cfg =
	    (struct southbridge_amd_rs780_config *)nb_dev->chip_info;

	/* step 5.9.1.1 */
	reg32 = nbpcie_p_read_index(dev, 0xa2);

	/* step 5.9.1.2 */
	set_pcie_enable_bits(nb_dev, 0x40, 1 << 0, 1 << 0);
	/* step 5.9.1.3 */
	set_pcie_enable_bits(dev, 0xa2, 3 << 0, width << 0);
	/* step 5.9.1.4 */
	set_pcie_enable_bits(dev, 0xa2, 1 << 8, 1 << 8);
	/* step 5.9.2.4 */
	if (0 == cfg->gfx_reconfiguration)
		set_pcie_enable_bits(dev, 0xa2, 1 << 11, 1 << 11);

	/* step 5.9.1.5 */
	do {
		reg32 = nbpcie_p_read_index(dev, 0xa2);
	}
	while (reg32 & 0x100);

	/* step 5.9.1.6 */
	sb_dev = dev_find_slot(0, PCI_DEVFN(8, 0));
	do {
		reg32 = pci_ext_read_config32(nb_dev, sb_dev,
					  PCIE_VC0_RESOURCE_STATUS);
	} while (reg32 & VC_NEGOTIATION_PENDING);

	/* step 5.9.1.7 */
	reg32 = nbpcie_p_read_index(dev, 0xa2);
	if (((reg32 & 0x70) >> 4) != 0x6) {
		/* the unused lanes should be powered off. */
	}

	/* step 5.9.1.8 */
	set_pcie_enable_bits(nb_dev, 0x40, 1 << 0, 0 << 0);
}

/*
* GFX Core initialization, dev2, dev3
*/
void rs780_gfx_init(device_t nb_dev, device_t dev, u32 port)
{
	u32 reg32;
	struct southbridge_amd_rs780_config *cfg =
	    (struct southbridge_amd_rs780_config *)nb_dev->chip_info;

	printk(BIOS_DEBUG, "rs780_gfx_init, nb_dev=0x%p, dev=0x%p, port=0x%x.\n",
		    nb_dev, dev, port);

	/* GFX Core Initialization */
	//if (port == 2) return;

	/* step 2, TMDS, (only need if CMOS option is enabled) */
	if (cfg->gfx_tmds) {
	}

#if 1				/* external clock mode */
	/* table 5-22, 5.9.1. REFCLK */
	/* 5.9.1.1. Disables the GFX REFCLK transmitter so that the GFX
	 * REFCLK PAD can be driven by an external source. */
	/* 5.9.1.2. Enables GFX REFCLK receiver to receive the REFCLK from an external source. */
	set_nbmisc_enable_bits(nb_dev, 0x38, 1 << 29 | 1 << 28, 0 << 29 | 1 << 28);

	/* 5.9.1.3 Selects the GFX REFCLK to be the source for PLL A. */
	/* 5.9.1.4 Selects the GFX REFCLK to be the source for PLL B. */
	/* 5.9.1.5 Selects the GFX REFCLK to be the source for PLL C. */
	set_nbmisc_enable_bits(nb_dev, 0x28, 3 << 6 | 3 << 8 | 3 << 10,
			       1 << 6 | 1 << 8 | 1 << 10);
	reg32 = nbmisc_read_index(nb_dev, 0x28);
	printk(BIOS_DEBUG, "misc 28 = %x\n", reg32);

	/* 5.9.1.6.Selects the single ended GFX REFCLK to be the source for core logic. */
	set_nbmisc_enable_bits(nb_dev, 0x6C, 1 << 31, 1 << 31);
#else				/* internal clock mode */
	/* table 5-23, 5.9.1. REFCLK */
	/* 5.9.1.1. Enables the GFX REFCLK transmitter so that the GFX
	 * REFCLK PAD can be driven by the SB REFCLK. */
	/* 5.9.1.2. Disables GFX REFCLK receiver from receiving the
	 * REFCLK from an external source.*/
	set_nbmisc_enable_bits(nb_dev, 0x38, 1 << 29 | 1 << 28, 1 << 29 | 0 << 28);

	/* 5.9.1.3 Selects the GFX REFCLK to be the source for PLL A. */
	/* 5.9.1.4 Selects the GFX REFCLK to be the source for PLL B. */
	/* 5.9.1.5 Selects the GFX REFCLK to be the source for PLL C. */
	set_nbmisc_enable_bits(nb_dev, 0x28, 3 << 6 | 3 << 8 | 3 << 10,
			       0);
	reg32 = nbmisc_read_index(nb_dev, 0x28);
	printk(BIOS_DEBUG, "misc 28 = %x\n", reg32);

	/* 5.9.1.6.Selects the single ended GFX REFCLK to be the source for core logic. */
	set_nbmisc_enable_bits(nb_dev, 0x6C, 1 << 31, 0 << 31);
#endif

	/* step 5.9.3, GFX overclocking, (only need if CMOS option is enabled) */
	/* 5.9.3.1. Increases PLL BW for 6G operation.*/
	/* set_nbmisc_enable_bits(nb_dev, 0x36, 0x3FF << 4, 0xB5 << 4); */
	/* skip */

	/* step 5.9.4, reset the GFX link */
	/* step 5.9.4.1 asserts both calibration reset and global reset */
	set_nbmisc_enable_bits(nb_dev, 0x8, 0x3 << 14, 0x3 << 14);

	/* step 5.9.4.2 de-asserts calibration reset */
	set_nbmisc_enable_bits(nb_dev, 0x8, 1 << 14, 0 << 14);

	/* step 5.9.4.3 wait for at least 200us */
	udelay(300);

	/* step 5.9.4.4 de-asserts global reset */
	set_nbmisc_enable_bits(nb_dev, 0x8, 1 << 15, 0 << 15);

	/* 5.9.5 Reset PCIE_GFX Slot */
	/* It is done in mainboard.c */
	set_pcie_reset();
	mdelay(1);
	set_pcie_dereset();

	/* step 5.9.8 program PCIE memory mapped configuration space */
	/* done by enable_pci_bar3() before */

	/* step 7 compliance state, (only need if CMOS option is enabled) */
	/* the compliance stete is just for test. refer to 4.2.5.2 of PCIe specification */
	if (cfg->gfx_compliance) {
		/* force compliance */
		set_nbmisc_enable_bits(nb_dev, 0x32, 1 << 6, 1 << 6);
		/* release hold training for device 2. GFX initialization is done. */
		set_nbmisc_enable_bits(nb_dev, 0x8, 1 << 4, 0 << 4);
		dynamic_link_width_control(nb_dev, dev, cfg->gfx_link_width);
		printk(BIOS_DEBUG, "rs780_gfx_init step7.\n");
		return;
	}

	/* 5.9.12 Core Initialization. */
	/* 5.9.12.1 sets RCB timeout to be 25ms */
	/* 5.9.12.2. RCB Cpl timeout on link down. */
	set_pcie_enable_bits(dev, 0x70, 7 << 16 | 1 << 19, 4 << 16 | 1 << 19);
	printk(BIOS_DEBUG, "rs780_gfx_init step5.9.12.1.\n");

	/* step 5.9.12.3 disables slave ordering logic */
	set_pcie_enable_bits(nb_dev, 0x20, 1 << 8, 1 << 8);
	printk(BIOS_DEBUG, "rs780_gfx_init step5.9.12.3.\n");

	/* step 5.9.12.4 sets DMA payload size to 64 bytes */
	set_pcie_enable_bits(nb_dev, 0x10, 7 << 10, 4 << 10);
	/* 5.9.12.5. Blocks DMA traffic during C3 state. */
	set_pcie_enable_bits(dev, 0x10, 1 << 0, 0 << 0);

	/* 5.9.12.6. Disables RC ordering logic */
	set_pcie_enable_bits(nb_dev, 0x20, 1 << 9, 1 << 9);

	/* Enabels TLP flushing. */
	/* Note: It is got from RS690. The system will hang without this action. */
	set_pcie_enable_bits(dev, 0x20, 1 << 19, 0 << 19);

	/* 5.9.12.7. Ignores DLLPs during L1 so that txclk can be turned off */
	set_pcie_enable_bits(nb_dev, 0x2, 1 << 0, 1 << 0);

	/* 5.9.12.8 Prevents LC to go from L0 to Rcv_L0s if L1 is armed. */
	set_pcie_enable_bits(dev, 0xA1, 1 << 11, 1 << 11);

	/* 5.9.12.9 CMGOOD_OVERRIDE for end point initiated lane degradation. */
	set_nbmisc_enable_bits(nb_dev, 0x6a, 1 << 17, 1 << 17);
	printk(BIOS_DEBUG, "rs780_gfx_init step5.9.12.9.\n");

	/* 5.9.12.10 Sets the timer in Config state from 20us to */
	/* 5.9.12.11 De-asserts RX_EN in L0s. */
	/* 5.9.12.12 Enables de-assertion of PG2RX_CR_EN to lock clock
	 * recovery parameter when lane is in electrical idle in L0s.*/
	set_pcie_enable_bits(dev, 0xB1, 1 << 23 | 1 << 19 | 1 << 28, 1 << 23 | 1 << 19 | 1 << 28);

	/* 5.9.12.13. Turns off offset calibration. */
	/* 5.9.12.14. Enables Rx Clock gating in CDR */
	set_nbmisc_enable_bits(nb_dev, 0x34, 1 << 10/* | 1 << 22 */, 1 << 10/* | 1 << 22 */);

	/* 5.9.12.15. Sets number of TX Clocks to drain TX Pipe to 3. */
	set_pcie_enable_bits(dev, 0xA0, 0xF << 4, 3 << 4);

	/* 5.9.12.16. Lets PI use Electrical Idle from PHY when
	 * turning off PLL in L1 at Gen2 speed instead Inferred Electrical Idle. */
	set_pcie_enable_bits(nb_dev, 0x40, 3 << 14, 2 << 14);

	/* 5.9.12.17. Prevents the Electrical Idle from causing a transition from Rcv_L0 to Rcv_L0s. */
	set_pcie_enable_bits(dev, 0xB1, 1 << 20, 1 << 20);

	/* 5.9.12.18. Prevents the LTSSM from going to Rcv_L0s if it has already
	 * acknowledged a request to go to L1. */
	set_pcie_enable_bits(dev, 0xA1, 1 << 11, 1 << 11);

	/* 5.9.12.19. LDSK only taking deskew on deskewing error detect */
	set_pcie_enable_bits(nb_dev, 0x40, 1 << 28, 0 << 28);

	/* 5.9.12.20. Bypasses lane de-skew logic if in x1 */
	set_pcie_enable_bits(nb_dev, 0xC2, 1 << 14, 1 << 14);

	/* 5.9.12.21. Sets Electrical Idle Threshold. */
	set_nbmisc_enable_bits(nb_dev, 0x35, 3 << 21, 2 << 21);

	/* 5.9.12.22. Advertises -6 dB de-emphasis value in TS1 Data Rate Identifier
	 * Only if CMOS Option in section. skip */

	/* 5.9.12.23. Disables GEN2 capability of the device. */
	set_pcie_enable_bits(dev, 0xA4, 1 << 0, 0 << 0);

	/* 5.9.12.24.Disables advertising Upconfigure Support. */
	set_pcie_enable_bits(dev, 0xA2, 1 << 13, 1 << 13);

	/* 5.9.12.25. No comment in RPR. */
	set_nbmisc_enable_bits(nb_dev, 0x39, 1 << 10, 0 << 10);

	/* 5.9.12.26. This capacity is required since links wider than x1 and/or multiple link
	 * speed are supported */
	set_pcie_enable_bits(nb_dev, 0xC1, 1 << 0, 1 << 0);

	/* 5.9.12.27. Enables NVG86 ECO. A13 above only. */
	if (get_nb_rev(nb_dev) == REV_RS780_A12)			/* A12 */
		set_pcie_enable_bits(dev, 0x02, 1 << 11, 1 << 11);

	/* 5.9.12.28 Hides and disables the completion timeout method. */
	set_pcie_enable_bits(nb_dev, 0xC1, 1 << 2, 0 << 2);

	/* 5.9.12.29. Use the bif_core de-emphasis strength by default. */
	/* set_nbmisc_enable_bits(nb_dev, 0x36, 1 << 28, 1 << 28); */

	/* 5.9.12.30. Set TX arbitration algorithm to round robin */
	set_pcie_enable_bits(nb_dev, 0x1C,
			     1 << 0 | 0x1F << 1 | 0x1F << 6,
			     1 << 0 | 0x04 << 1 | 0x04 << 6);

	/* Single-port/Dual-port configureation. */
	switch (cfg->gfx_dual_slot) {
	case 0:
		/* step 1, lane reversal (only need if CMOS option is enabled) */
		if (cfg->gfx_lane_reversal) {
			set_nbmisc_enable_bits(nb_dev, 0x33, 1 << 2, 1 << 2);
		}
		printk(BIOS_DEBUG, "rs780_gfx_init step1.\n");

		printk(BIOS_DEBUG, "device = %x\n", dev->path.pci.devfn >> 3);
		if((dev->path.pci.devfn >> 3) == 2) {
			single_port_configuration(nb_dev, dev);
		} else {
			set_nbmisc_enable_bits(nb_dev, 0xc, 0, 0x2 << 2); /* hide the GFX bridge. */
			printk(BIOS_INFO, "Single port. Do nothing.\n"); // If dev3
		}

		break;
	case 1:
		/* step 1, lane reversal (only need if CMOS option is enabled) */
		if (cfg->gfx_lane_reversal) {
			set_nbmisc_enable_bits(nb_dev, 0x33, 1 << 2, 1 << 2);
			set_nbmisc_enable_bits(nb_dev, 0x33, 1 << 3, 1 << 3);
		}
		printk(BIOS_DEBUG, "rs780_gfx_init step1.\n");
		/* step 1.1, dual-slot gfx configuration (only need if CMOS option is enabled) */
		/* AMD calls the configuration CrossFire */
		set_nbmisc_enable_bits(nb_dev, 0x0, 0xf << 8, 5 << 8);
		printk(BIOS_DEBUG, "rs780_gfx_init step2.\n");

		printk(BIOS_DEBUG, "device = %x\n", dev->path.pci.devfn >> 3);
		dual_port_configuration(nb_dev, dev);
		break;
	default:
		printk(BIOS_INFO, "Incorrect configuration of external GFX slot.\n");
		break;
	}
}
