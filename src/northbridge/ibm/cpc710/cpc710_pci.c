#include <stdint.h>
#include <arch/io.h>
#include "cpc710.h"
#include "cpc710_pci.h"

extern void setCPC710(uint32_t, uint32_t);

void
setCPC710PCI32_16(uint32_t addr, uint16_t data)
{
	out_le16((unsigned short *)(CPC710_BRIDGE_CPCI_PHYS + addr), data);
}

void
setCPC710PCI32_32(uint32_t addr, uint32_t data)
{
	out_le32((unsigned *)(CPC710_BRIDGE_CPCI_PHYS + addr), data);
}

void
setCPC710PCI64_16(uint16_t addr, uint16_t data)
{
	out_le16((unsigned short *)(CPC710_BRIDGE_LOCAL_PHYS + addr), data);
}

void
setCPC710PCI64_32(uint32_t addr, uint32_t data)
{
	out_le32((unsigned *)(CPC710_BRIDGE_LOCAL_PHYS + addr), data);
}

void
cpc710_pci_init(void)
{
	/* Enable PCI32 */
	setCPC710(CPC710_CPC0_PCICNFR, 0x80000002); /* activate PCI32 config */
	setCPC710(CPC710_CPC0_PCIBAR,  CPC710_BRIDGE_CPCI_PHYS); /* PCI32 base address */
	setCPC710(CPC710_CPC0_PCIENB,  0x80000000); /* enable addr space */
	setCPC710(CPC710_CPC0_PCICNFR, 0x00000000); /* config done */

	/* Reset PCI Status register */
	setCPC710PCI32_32(CPC710_BRIDGE_CFGADDR, 0x80000006);
	setCPC710PCI32_16(CPC710_BRIDGE_CFGDATA, 0xffff);

	/* Configure bus number */
	setCPC710PCI32_32(CPC710_BRIDGE_CFGADDR, 0x80000040);
	setCPC710PCI32_16(CPC710_BRIDGE_CFGDATA, 0x0000);

	/* Set PCI configuration registers */
	setCPC710PCI32_32(CPC710_BRIDGE_PCIDG,  0x40000000);
	setCPC710PCI32_32(CPC710_BRIDGE_PIBAR,  0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_PMBAR,  0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_PR,     0xa000c000);
	setCPC710PCI32_32(CPC710_BRIDGE_ACR,    0xfc000000);
	setCPC710PCI32_32(CPC710_BRIDGE_MSIZE,  0xf8000000); /* 128Mb */
	setCPC710PCI32_32(CPC710_BRIDGE_IOSIZE, 0xf8000000); /* 128Mb */
	setCPC710PCI32_32(CPC710_BRIDGE_SMBAR,  0xc0000000);
	setCPC710PCI32_32(CPC710_BRIDGE_SIBAR,  0x80000000);
	setCPC710PCI32_32(CPC710_BRIDGE_CTLRW,  0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_PSSIZE, 0x00000080);
	setCPC710PCI32_32(CPC710_BRIDGE_BARPS,  0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_PSBAR,  0x00000080);
	setCPC710PCI32_32(CPC710_BRIDGE_BPMDLK, 0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_TPMDLK, 0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_BIODLK, 0x00000000);
	setCPC710PCI32_32(CPC710_BRIDGE_TIODLK, 0x00000000);

	/* Enable address space */
	setCPC710PCI32_32(CPC710_BRIDGE_CFGADDR, 0x80000004);
	setCPC710PCI32_16(CPC710_BRIDGE_CFGDATA, 0xfda7);

	setCPC710PCI32_32(CPC710_BRIDGE_CRR,    0xfc000000);

	/* Enable PCI64 */
	setCPC710(CPC710_CPC0_PCICNFR, 0x80000003); /* activate PCI64 config */
	setCPC710(CPC710_CPC0_PCIBAR,  CPC710_BRIDGE_LOCAL_PHYS); /* PCI64 base address */
	setCPC710(CPC710_CPC0_PCIENB,  0x80000000); /* enable addr space */
	setCPC710(CPC710_CPC0_PCICNFR, 0x00000000); /* config done */

	/* Reset PCI Status register */
	setCPC710PCI64_32(CPC710_BRIDGE_CFGADDR, 0x80000006);
	setCPC710PCI64_16(CPC710_BRIDGE_CFGDATA, 0xffff);

	/* Reset G_INT[A-D] bits in INT_RESET */
	setCPC710PCI64_32(CPC710_BRIDGE_CFGADDR, 0x80000068);
	setCPC710PCI64_32(CPC710_BRIDGE_CFGDATA, 0x0000000f);

	/* Configure bus number BUSNO=1, SUBNO=1 */
	setCPC710PCI64_32(CPC710_BRIDGE_CFGADDR, 0x80000040);
	setCPC710PCI64_16(CPC710_BRIDGE_CFGDATA, 0x0101);

	/* Set PCI configuration registers */
	setCPC710PCI64_32(CPC710_BRIDGE_PSEA,   0x00000000);
	setCPC710PCI64_32(CPC710_BRIDGE_PCIDG,  0xc0000000);
	setCPC710PCI64_32(CPC710_BRIDGE_PIBAR,  0x00000000);
	setCPC710PCI64_32(CPC710_BRIDGE_PMBAR,  0x00000000);
	setCPC710PCI64_32(CPC710_BRIDGE_PR,     0x80008000);
	setCPC710PCI64_32(CPC710_BRIDGE_ACR,    0xff000000);
	setCPC710PCI64_32(CPC710_BRIDGE_MSIZE,  0xf8000000); /* 128Mb */
	setCPC710PCI64_32(CPC710_BRIDGE_IOSIZE, 0xf8000000); /* 128Mb */
	setCPC710PCI64_32(CPC710_BRIDGE_SMBAR,  0xc8000000);
	setCPC710PCI64_32(CPC710_BRIDGE_SIBAR,  0x88000000);
	setCPC710PCI64_32(CPC710_BRIDGE_CTLRW,  0x02000000);
	setCPC710PCI64_32(CPC710_BRIDGE_PSSIZE, 0x00000080);

	/* Config PSBAR for PCI64 */
	setCPC710PCI64_32(CPC710_BRIDGE_CFGADDR, 0x80000010);
	setCPC710PCI64_32(CPC710_BRIDGE_CFGDATA, 0x80000000);

	setCPC710PCI64_32(CPC710_BRIDGE_BARPS,  0x00000000);
	setCPC710PCI64_32(CPC710_BRIDGE_INTSET,  0x00000000);

	/* Enable address space */
	setCPC710PCI64_32(CPC710_BRIDGE_CFGADDR, 0x80010004);
	setCPC710PCI64_16(CPC710_BRIDGE_CFGDATA, 0xfda7);

	setCPC710PCI64_32(CPC710_BRIDGE_CRR,    0xfc000000);
}
