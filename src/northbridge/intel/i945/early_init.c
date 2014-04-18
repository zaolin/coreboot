/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2008 coresystems GmbH
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

#include "i945.h"
#include "pcie_config.c"

static int i945_silicon_revision(void)
{
	return pci_read_config8(PCI_DEV(0, 0x00, 0), PCI_CLASS_REVISION);
}

static void i945_detect_chipset(void)
{
	u8 reg8;

	printk_info("\n");
	reg8 = (pci_read_config8(PCI_DEV(0, 0x00, 0), 0xe7) & 0x70) >> 4;
	switch (reg8) {
	case 1:
		printk_info("Mobile Intel(R) 945GM/GME Express");
		break;
	case 2:
		printk_info("Mobile Intel(R) 945GMS/GU Express");
		break;
	case 3:
		printk_info("Mobile Intel(R) 945PM Express");
		break;
	case 5:
		printk_info("Intel(R) 945GT Express");
		break;
	case 6:
		printk_info("Mobile Intel(R) 943/940GML Express");
		break;
	default:
		printk_info("Unknown (%02x)", reg8);	/* Others reserved. */
	}
	printk_info(" Chipset\n");

	printk_debug("(G)MCH capable of up to FSB ");
	reg8 = (pci_read_config8(PCI_DEV(0, 0x00, 0), 0xe3) & 0xe0) >> 5;
	switch (reg8) {
	case 2:
		printk_debug("800 MHz"); /* According to 965 spec */
		break;
	case 3:
		printk_debug("667 MHz");
		break;
	case 4:
		printk_debug("533 MHz");
		break;
	default:
		printk_debug("N/A MHz (%02x)", reg8);
	}
	printk_debug("\n");

	printk_debug("(G)MCH capable of ");
	reg8 = (pci_read_config8(PCI_DEV(0, 0x00, 0), 0xe4) & 0x07);
	switch (reg8) {
	case 2:
		printk_debug("up to DDR2-667");
		break;
	case 3:
		printk_debug("up to DDR2-533");
		break;
	case 4:
		printk_debug("DDR2-400");
		break;
	default:
		printk_info("unknown max. RAM clock (%02x).", reg8);	/* Others reserved. */
	}
	printk_debug("\n");
}

static void i945_setup_bars(void)
{
	u8 reg8;

	/* As of now, we don't have all the A0 workarounds implemented */
	if (i945_silicon_revision() == 0)
		printk_info
		    ("Warning: i945 silicon revision A0 might not work correctly.\n");

	/* Setting up Southbridge. In the northbridge code. */
	printk_debug("Setting up static southbridge registers...");
	pci_write_config32(PCI_DEV(0, 0x1f, 0), RCBA, DEFAULT_RCBA | 1);

	pci_write_config32(PCI_DEV(0, 0x1f, 0), PMBASE, DEFAULT_PMBASE | 1);
	pci_write_config8(PCI_DEV(0, 0x1f, 0), 0x44 /* ACPI_CNTL */ , 0x80); /* Enable ACPI BAR */

	pci_write_config32(PCI_DEV(0, 0x1f, 0), GPIOBASE, DEFAULT_GPIOBASE | 1);
	pci_write_config8(PCI_DEV(0, 0x1f, 0), 0x4c /* GC */ , 0x10);	/* Enable GPIOs */
	setup_ich7_gpios();
	printk_debug(" done.\n");

	printk_debug("Disabling Watchdog reboot...");
	RCBA32(GCS) = (RCBA32(0x3410)) | (1 << 5);	/* No reset */
	outw((1 << 11), DEFAULT_PMBASE | 0x60 | 0x08);	/* halt timer */
	printk_debug(" done.\n");

	printk_debug("Setting up static northbridge registers...");
	/* Set up all hardcoded northbridge BARs */
	pci_write_config32(PCI_DEV(0, 0x00, 0), EPBAR, DEFAULT_EPBAR | 1);
	pci_write_config32(PCI_DEV(0, 0x00, 0), MCHBAR, DEFAULT_MCHBAR | 1);
	pci_write_config32(PCI_DEV(0, 0x00, 0), PCIEXBAR, DEFAULT_PCIEXBAR | 5); /* 64MB - busses 0-63 */
	pci_write_config32(PCI_DEV(0, 0x00, 0), DMIBAR, DEFAULT_DMIBAR | 1);
	pci_write_config32(PCI_DEV(0, 0x00, 0), X60BAR, DEFAULT_X60BAR | 1);

	/* Hardware default is 8MB UMA. If someone wants to make this a
	 * CMOS or compile time option, send a patch.
	 * pci_write_config16(PCI_DEV(0, 0x00, 0), GGC, 0x30);
	 */

	/* Set C0000-FFFFF to access RAM on both reads and writes */
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM0, 0x30);
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM1, 0x33);
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM2, 0x33);
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM3, 0x33);
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM4, 0x33);
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM5, 0x33);
	pci_write_config8(PCI_DEV(0, 0x00, 0), PAM6, 0x33);

	pci_write_config32(PCI_DEV(0, 0x00, 0), SKPAD, 0xcafebabe);
	printk_debug(" done.\n");

	/* Wait for MCH BAR to come up */
	printk_debug("Waiting for MCHBAR to come up...");
	if ((pci_read_config8(PCI_DEV(0, 0x0f, 0), 0xe6) & 0x2) == 0x00) { /* Bit 49 of CAPID0 */
		do {
			reg8 = *(volatile u8 *)0xfed40000;
		} while (!(reg8 & 0x80));
	}
	printk_debug("ok\n");
}

static void i945_setup_egress_port(void)
{
	u32 reg32;
	u32 timeout;

	printk_debug("Setting up Egress Port RCRB\n");

	/* Egress Port Virtual Channel 0 Configuration */

	/* map only TC0 to VC0 */
	reg32 = EPBAR32(EPVC0RCTL);
	reg32 &= 0xffffff01;
	EPBAR32(EPVC0RCTL) = reg32;

	
	reg32 = EPBAR32(EPPVCCAP1);
	reg32 &= ~(7 << 0);
	reg32 |= 1;
	EPBAR32(EPPVCCAP1) = reg32;

	/* Egress Port Virtual Channel 1 Configuration */
	reg32 = EPBAR32(0x2c);
	reg32 &= 0xffffff00;
	if ((MCHBAR32(CLKCFG) & 7) == 1)
		reg32 |= 0x0d;	/* 533MHz */
	if ((MCHBAR32(CLKCFG) & 7) == 3)
		reg32 |= 0x10;	/* 667MHz */
	EPBAR32(0x2c) = reg32;

	EPBAR32(EPVC1MTS) = 0x0a0a0a0a;

	reg32 = EPBAR32(EPVC1RCAP);
	reg32 &= ~(0x7f << 16);
	reg32 |= (0x0a << 16);
	EPBAR32(EPVC1RCAP) = reg32;

	if ((MCHBAR32(CLKCFG) & 7) == 1) {	/* 533MHz */
		EPBAR32(EPVC1IST + 0) = 0x009c009c;
		EPBAR32(EPVC1IST + 4) = 0x009c009c;
	}

	if ((MCHBAR32(CLKCFG) & 7) == 3) {	/* 667MHz */
		EPBAR32(EPVC1IST + 0) = 0x00c000c0;
		EPBAR32(EPVC1IST + 4) = 0x00c000c0;
	}

	/* Is internal graphics enabled? */
	if (pci_read_config8(PCI_DEV(0, 0x0, 0), 54) & ((1 << 4) | (1 << 3))) {	/* DEVEN */
		MCHBAR32(MMARB1) |= (1 << 17);
	}

	/* Assign Virtual Channel ID 1 to VC1 */
	reg32 = EPBAR32(EPVC1RCTL);
	reg32 &= ~(7 << 24);
	reg32 |= (1 << 24);
	EPBAR32(EPVC1RCTL) = reg32;
	
	reg32 = EPBAR32(EPVC1RCTL);
	reg32 &= 0xffffff01;
	reg32 |= (1 << 7);
	EPBAR32(EPVC1RCTL) = reg32;
	
	EPBAR32(PORTARB + 0x00) = 0x01000001;
	EPBAR32(PORTARB + 0x04) = 0x00040000;
	EPBAR32(PORTARB + 0x08) = 0x00001000;
	EPBAR32(PORTARB + 0x0c) = 0x00000040;
	EPBAR32(PORTARB + 0x10) = 0x01000001;
	EPBAR32(PORTARB + 0x14) = 0x00040000;
	EPBAR32(PORTARB + 0x18) = 0x00001000;
	EPBAR32(PORTARB + 0x1c) = 0x00000040;
	
	EPBAR32(EPVC1RCTL) |= (1 << 16);
	EPBAR32(EPVC1RCTL) |= (1 << 16);

	printk_debug("Loading port arbitration table ...");
	/* Loop until bit 0 becomes 0 */
	timeout = 0x7fffff;
	while ((EPBAR16(EPVC1RSTS) & 1) && --timeout) ;
	if (!timeout)
		printk_debug("timeout!\n");
	else
		printk_debug("ok\n");

	/* Now enable VC1 */
	EPBAR32(EPVC1RCTL) |= (1 << 31);

	printk_debug("Wait for VC1 negotiation ...");
	/* Wait for VC1 negotiation pending */
	timeout = 0x7fff;
	while ((EPBAR16(EPVC1RSTS) & (1 << 1)) && --timeout) ;
	if (!timeout)
		printk_debug("timeout!\n");
	else
		printk_debug("ok\n");

}

static void ich7_setup_dmi_rcrb(void)
{
	u16 reg16;

	
	reg16 = RCBA16(LCTL);
	reg16 &= ~(3 << 0);
	reg16 |= 1;
	RCBA16(LCTL) = reg16;

	RCBA32(V0CTL) = 0x80000001;
	RCBA32(V1CAP) = 0x03128010;
	RCBA32(ESD) = 0x00000810;
	RCBA32(RP1D) = 0x01000003;
	RCBA32(RP2D) = 0x02000002;
	RCBA32(RP3D) = 0x03000002;
	RCBA32(RP4D) = 0x04000002;
	RCBA32(HDD) = 0x0f000003;
	RCBA32(RP5D) = 0x05000002;

	RCBA32(RPFN) = 0x00543210;

	pci_write_config16(PCI_DEV(0, 0x1c, 0), 0x42, 0x0141);	
	pci_write_config16(PCI_DEV(0, 0x1c, 4), 0x42, 0x0141);	
	pci_write_config16(PCI_DEV(0, 0x1c, 5), 0x42, 0x0141);	

	pci_write_config32(PCI_DEV(0, 0x1c, 4), 0x54, 0x00480ce0);	
	pci_write_config32(PCI_DEV(0, 0x1c, 5), 0x54, 0x00500ce0);	
}

static void i945_setup_dmi_rcrb(void)
{
	u32 reg32;
	u32 timeout;

	printk_debug("Setting up DMI RCRB\n");

	/* Virtual Channel 0 Configuration */
	reg32 = DMIBAR32(DMIVC0RCTL0);
	reg32 &= 0xffffff01;
	DMIBAR32(DMIVC0RCTL0) = reg32;
	
	reg32 = DMIBAR32(DMIPVCCAP1);
	reg32 &= ~(7 << 0);
	reg32 |= 1;
	DMIBAR32(DMIPVCCAP1) = reg32;
	
	reg32 = DMIBAR32(DMIVC1RCTL);
	reg32 &= ~(7 << 24);
	reg32 |= (1 << 24);	/* NOTE: This ID must match ICH7 side */
	DMIBAR32(DMIVC1RCTL) = reg32;
	
	reg32 = DMIBAR32(DMIVC1RCTL);
	reg32 &= 0xffffff01;
	reg32 |= (1 << 7);
	DMIBAR32(DMIVC1RCTL) = reg32;

	/* Now enable VC1 */
	DMIBAR32(DMIVC1RCTL) |= (1 << 31);

	printk_debug("Wait for VC1 negotiation ...");
	/* Wait for VC1 negotiation pending */
	timeout = 0x7ffff;
	while ((DMIBAR16(DMIVC1RSTS) & (1 << 1)) && --timeout) ;
	if (!timeout)
		printk_debug("timeout!\n");
	else
		printk_debug("done..\n");
#if 1
	/* Enable Active State Power Management (ASPM) L0 state */

	reg32 = DMIBAR32(DMILCAP);
	reg32 &= ~(7 << 12);
	reg32 |= (2 << 12);	

	reg32 &= ~(7 << 15);
	
	reg32 |= (2 << 15);	
	DMIBAR32(DMILCAP) = reg32;

	reg32 = DMIBAR32(DMICC);
	reg32 &= 0x00ffffff;	
	reg32 &= ~(3 << 0);
	reg32 |= (1 << 0);	
	DMIBAR32(DMICC) = reg32;

	if (0) {
		DMIBAR32(DMILCTL) |= (3 << 0);
	}
#endif

	/* Last but not least, some additional steps */
	reg32 = MCHBAR32(FSBSNPCTL);
	reg32 &= ~(0xff << 2);
	reg32 |= (0xaa << 2);
	MCHBAR32(FSBSNPCTL) = reg32;

	DMIBAR32(0x2c) = 0x86000040;

	reg32 = DMIBAR32(0x204);
	reg32 &= ~0x3ff;
#if 1
	reg32 |= 0x13f;		/* for x4 DMI only */
#else
	reg32 |= 0x1e4; /* for x2 DMI only */
#endif
	DMIBAR32(0x204) = reg32;

	if (pci_read_config8(PCI_DEV(0, 0x0, 0), 54) & ((1 << 4) | (1 << 3))) {	/* DEVEN */
		DMIBAR32(0x200) |= (1 << 21);
	} else {
		DMIBAR32(0x200) &= ~(1 << 21);
	}

	reg32 = DMIBAR32(0x204);
	reg32 &= ~((1 << 11) | (1 << 10));
	DMIBAR32(0x204) = reg32;

	reg32 = DMIBAR32(0x204);
	reg32 &= ~(0xff << 12);
	reg32 |= (0x0d << 12);
	DMIBAR32(0x204) = reg32;

	DMIBAR32(DMICTL1) |= (3 << 24);

	reg32 = DMIBAR32(0x200);
	reg32 &= ~(0x3 << 26);
	reg32 |= (0x02 << 26);
	DMIBAR32(0x200) = reg32;

	DMIBAR32(DMIDRCCFG) &= ~(1 << 31);
	DMIBAR32(DMICTL2) |= (1 << 31);

	if (i945_silicon_revision() >= 3) {
		reg32 = DMIBAR32(0xec0);
		reg32 &= 0x0fffffff;
		reg32 |= (2 << 28);
		DMIBAR32(0xec0) = reg32;

		reg32 = DMIBAR32(0xed4);
		reg32 &= 0x0fffffff;
		reg32 |= (2 << 28);
		DMIBAR32(0xed4) = reg32;

		reg32 = DMIBAR32(0xee8);
		reg32 &= 0x0fffffff;
		reg32 |= (2 << 28);
		DMIBAR32(0xee8) = reg32;

		reg32 = DMIBAR32(0xefc);
		reg32 &= 0x0fffffff;
		reg32 |= (2 << 28);
		DMIBAR32(0xefc) = reg32;
	}

	/* wait for bit toggle to 0 */
	printk_debug("Waiting for DMI hardware...");
	timeout = 0x7fffff;
	while ((DMIBAR8(0x32) & (1 << 1)) && --timeout) ;
	if (!timeout)
		printk_debug("timeout!\n");
	else
		printk_debug("ok\n");
	
	DMIBAR32(0x1c4) = 0xffffffff;
	DMIBAR32(0x1d0) = 0xffffffff;
	DMIBAR32(0x228) = 0xffffffff;
	
	DMIBAR32(0x308) = DMIBAR32(0x308);
	DMIBAR32(0x314) = DMIBAR32(0x314);
	DMIBAR32(0x324) = DMIBAR32(0x324);
	DMIBAR32(0x328) = DMIBAR32(0x328);
	DMIBAR32(0x338) = DMIBAR32(0x334);
	DMIBAR32(0x338) = DMIBAR32(0x338);

	if (i945_silicon_revision() == 1 && ((MCHBAR8(0xe08) & (1 << 5)) == 1)) {
		if ((MCHBAR32(0x214) & 0xf) != 0x3) {	
			printk_info
			    ("DMI link requires A1 stepping workaround. Rebooting.\n");
			reg32 = MCHBAR32(MMARB1);
			reg32 &= 0xfffffff8;
			reg32 |= 3;
			outb(0x06, 0xcf9);
			for (;;) ;	/* wait for reset */
		}
	}
}

static void i945_setup_pci_express_x16(void)
{
	u32 timeout;
	u32 reg32;
	u16 reg16;

	/* For now we just disable the x16 link */
	printk_debug("Disabling PCI Express x16 Link\n");

	MCHBAR16(UPMC1) |= (1 << 5) | (1 << 0);

	reg16 = pcie_read_config16(PCI_DEV(0, 0x01, 0), BCTRL1);
	reg16 |= (1 << 6);
	pcie_write_config16(PCI_DEV(0, 0x01, 0), BCTRL1, reg16);

	reg32 = pcie_read_config32(PCI_DEV(0, 0x01, 0), 0x224);
	reg32 |= (1 << 8);
	pcie_write_config32(PCI_DEV(0, 0x01, 0), 0x224, reg32);

	reg16 = pcie_read_config16(PCI_DEV(0, 0x01, 0), BCTRL1);
	reg16 &= ~(1 << 6);
	pcie_write_config16(PCI_DEV(0, 0x01, 0), BCTRL1, reg16);

	printk_debug("Wait for link to enter detect state... ");
	timeout = 0x7fffff;
	for (reg32 = pcie_read_config32(PCI_DEV(0, 0x01, 0), 0x214);
	     (reg32 & 0x000f0000) && --timeout;) ;
	if (!timeout)
		printk_debug("timeout!\n");
	else
		printk_debug("ok\n");

	/* Finally: Disable the PCI config header */
	reg16 = pci_read_config16(PCI_DEV(0, 0x00, 0), DEVEN);
	reg16 &= ~DEVEN_D1F0;
	pci_write_config16(PCI_DEV(0, 0x00, 0), DEVEN, reg16);
}

static void i945_setup_root_complex_topology(void)
{
	u32 reg32;

	printk_debug("Setting up Root Complex Topology\n");
	/* Egress Port Root Topology */
	reg32 = EPBAR32(EPESD);
	reg32 &= 0xff00ffff;
	reg32 |= (1 << 16);
	EPBAR32(EPESD) = reg32;
	
	EPBAR32(EPLE1D) |= (1 << 0);
	
	EPBAR32(EPLE1A) = DEFAULT_PCIEXBAR + 0x4000;
	
	EPBAR32(EPLE2D) |= (1 << 0);

	/* DMI Port Root Topology */
	reg32 = DMIBAR32(DMILE1D);
	reg32 &= 0x00ffffff;
	DMIBAR32(DMILE1D) = reg32;
	
	reg32 = DMIBAR32(DMILE1D);
	reg32 &= 0xff00ffff;
	reg32 |= (2 << 16);
	DMIBAR32(DMILE1D) = reg32;
	
	DMIBAR32(DMILE1D) |= (1 << 0);
	
	DMIBAR32(DMILE1A) = DEFAULT_PCIEXBAR + 0x8000;
	
	DMIBAR32(DMILE2D) |= (1 << 0);
	
	DMIBAR32(DMILE2A) = DEFAULT_PCIEXBAR + 0x5000;

	/* PCI Express x16 Port Root Topology */
	if (pci_read_config8(PCI_DEV(0, 0x00, 0), DEVEN) & DEVEN_D1F0) {
		pcie_write_config32(PCI_DEV(0, 0x01, 0), 0x158,
				    DEFAULT_PCIEXBAR + 0x5000);

		reg32 = pcie_read_config32(PCI_DEV(0, 0x01, 0), 0x150);
		reg32 |= (1 << 0);
		pcie_write_config32(PCI_DEV(0, 0x01, 0), 0x150, reg32);
	}
}

static void ich7_setup_root_complex_topology(void)
{
	RCBA32(0x104) = 0x00000802;
	RCBA32(0x110) = 0x00000001;
	RCBA32(0x114) = 0x00000000;
	RCBA32(0x118) = 0x00000000;
}

static void ich7_setup_pci_express(void)
{
	RCBA32(CG) |= (1 << 0);	
	
	pci_write_config32(PCI_DEV(0, 0x1c, 0), 0x54, 0x00000060);

	pci_write_config32(PCI_DEV(0, 0x1c, 0), 0xd8, 0x00110000);
}

static void i945_early_initialization(void)
{
	/* Print some chipset specific information */
	i945_detect_chipset();

	/* Setup all BARs required for early PCIe and raminit */
	i945_setup_bars();

	/* Change port80 to LPC */
	RCBA32(GCS) &= (~0x04);
}

static void i945_late_initialization(void)
{
	i945_setup_egress_port();

	ich7_setup_root_complex_topology();

	ich7_setup_pci_express();

	ich7_setup_dmi_rcrb();

	i945_setup_dmi_rcrb();

	i945_setup_pci_express_x16();

	i945_setup_root_complex_topology();
}
