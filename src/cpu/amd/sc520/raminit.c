/* this setupcpu function comes from: */
/*==============================================================================*/
/* FILE   :  start16.asm*/
/**/
/* DESC   : A  16 bit mode assembly language startup program, intended for*/
/*          use with on Aspen SC520 platforms.*/
/**/
/* 11/16/2000 Added support for the NetSC520*/
/* 12/28/2000 Modified to boot linux image*/
/**/
/* =============================================================================*/
/*                                                                             */
/*  Copyright 2000 Advanced Micro Devices, Inc.                                */
/*                                                                             */
/* This software is the property of Advanced Micro Devices, Inc  (AMD)  which */
/* specifically grants the user the right to modify, use and distribute this */
/* software provided this COPYRIGHT NOTICE is not removed or altered.  All */
/* other rights are reserved by AMD.                                                       */
/*                                                                            */
/* THE MATERIALS ARE PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTY */
/* OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT OF */
/* THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY PARTICULAR PURPOSE.*/
/* IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY DAMAGES WHATSOEVER*/
/* (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS, BUSINESS*/
/* INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR INABILITY*/
/* TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY OF*/
/* SUCH DAMAGES.  BECAUSE SOME JURSIDICTIONS PROHIBIT THE EXCLUSION OR*/
/* LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE*/
/* LIMITATION MAY NOT APPLY TO YOU.*/
/**/
/* AMD does not assume any responsibility for any errors that may appear in*/
/* the Materials nor any responsibility to support or update the Materials.*/
/* AMD retains the right to make changes to its test specifications at any*/
/* time, without notice.*/
/**/
/* So that all may benefit from your experience, please report  any  problems */
/* or suggestions about this software back to AMD.  Please include your name, */
/* company,  telephone number,  AMD product requiring support and question or */
/* problem encountered.                                                       */
/*                                                                            */
/* Advanced Micro Devices, Inc.         Worldwide support and contact           */
/* Embedded Processor Division            information available at:               */
/* Systems Engineering                       epd.support@amd.com*/
/* 5204 E. Ben White Blvd.                          -or-*/
/* Austin, TX 78741                http://www.amd.com/html/support/techsup.html*/
/* ============================================================================*/

#define OUTC(addr, val) *(unsigned char *)(addr) = (val)


void
setupsc520(void){
	unsigned char *cp;
	unsigned short *sp;
	unsigned long *edi;
	unsigned long *par;

	/* turn off the write buffer*/
	cp = (unsigned char *)0xfffef040;
	*cp = 0;

	/*set the GP CS offset*/
	sp =  (unsigned short *)0xfffefc08;
	*sp = 0x00001;
	/*set the GP CS width*/
	sp =  (unsigned short *)0xfffefc09;
	*sp = 0x00003;
	/*set the GP CS width*/
	sp =  (unsigned short *)0xfffefc0a;
	*sp = 0x00001;
	/*set the RD pulse width*/
	sp =  (unsigned short *)0xfffefc0b;
	*sp = 0x00003;
	/*set the GP RD offse*/
	sp =  (unsigned short *)0xfffefc0c;
	*sp = 0x00001;
	/*set the GP WR pulse width*/
	sp =  (unsigned short *)0xfffefc0d;
	*sp = 0x00003;
	/*set the GP WR offset*/
	sp =  (unsigned short *)0xfffefc0e;
	*sp = 0x00001;
	/* set up the GP IO pins*/
	/*set the GPIO directionreg*/
	sp =  (unsigned short *)0xfffefc2c;
	*sp = 0x00000;
	/*set the GPIO directionreg*/
	sp =  (unsigned short *)0xfffefc2a;
	*sp = 0x00000;
	/*set the GPIO pin function 31-16 reg*/
	sp =  (unsigned short *)0xfffefc22;
	*sp = 0x0FFFF;
	/*set the GPIO pin function 15-0 reg*/
	sp =  (unsigned short *)0xfffefc20;
	*sp = 0x0FFFF;

#ifdef NETSC520
if NetSC520
; set the PIO regs correctly.
	/*set the GPIO16-31 direction reg*/
	sp =  (unsigned short *)0xfffefc2c;
	*sp = 0x000ff;
	/*set the PIODIR15_0 direction reg*/
	sp =  (unsigned short *)0xfffefc2a;
	*sp = 0x00440;
	/*set the PIOPFS31_16 direction reg*/
	sp =  (unsigned short *)0xfffefc22;
	*sp = 0x00600;
	/*set the PIOPFS15_0 direction reg*/
	sp =  (unsigned short *)0xfffefc20;
	*sp = 0x0FBBF;
	/*set the PIODATA15_0 reg*/
	sp =  (unsigned short *)0x0xfffefc30;
	*sp = 0x0f000;
	/*set the CSPFS reg*/
	sp =  (unsigned short *)0xfffefc24;
	*sp = 0x0000;

; The NetSC520 uses PIOs 16-23 for LEDs instead of port 80
; output a 1 to the leds
	/*set the GPIO16-31 direction reg*/
	sp =  (unsigned short *)0xfffefc32;
	mov	al, not 1
else
#endif

	/* setup for the CDP*/
	/*set the GPIO directionreg*/
	sp =  (unsigned short *)0xfffefc2c;
	*sp = 0x00000;
	/*set the GPIO directionreg*/
	sp =  (unsigned short *)0xfffefc2a;
	*sp = 0x00000;
	/*set the GPIO pin function 31-16 reg*/
	sp =  (unsigned short *)0xfffefc22;
	*sp = 0x0FFFF;
	/*set the GPIO pin function 15-0 reg*/
	sp =  (unsigned short *)0xfffefc20;
	*sp = 0x0FFFF;
	/* the 0x80 led should now be working*/
	outb(0xaa, 0x80);
/*
; set up a PAR to allow access to the 680 leds
;	WriteMMCR( 0xc4,0x28000680);		// PAR15
 */
	/*set PAR 15 for access to led 680*/
/* skip hairy pci hack for now *
	sp =  (unsigned short *)0xfffef0c4;
	mov	eax,028000680h
	mov	dx,0680h
	*sp = 0x02;	; output a 2 to led 680
	out	dx,ax
*/
/*; set the uart baud rate clocks to the normal 1.8432 MHz.*/
	cp = (unsigned char *)0xfffefcc0;
	*cp = 4;					/* uart 1 clock source */
	cp = (unsigned char *)0xfffefcc4;
	*cp = 4;					/* uart 2 clock source */
/*; set the interrupt mapping registers.*/
	cp = (unsigned char *)0x0fffefd20;
	*cp = 0x01;
	
	cp = (unsigned char *)0x0fffefd28;
	*cp = 0x0c;
	
	cp = (unsigned char *)0x0fffefd29;
	*cp = 0x0b;
	
	cp = (unsigned char *)0x0fffefd30;
	*cp = 0x07;
	
	cp = (unsigned char *)0x0fffefd43;
	*cp = 0x03;
	
	cp = (unsigned char *)0x0fffefd51;
	*cp = 0x02;
	
/*; "enumerate" the PCI. Mainly set the interrupt bits on the PCnetFast. */
	outl(0xcf8, 0x08000683c);
	outl(0xcfc, 0xc); /* set the interrupt line */

/*; Set the SC520 PCI host bridge to target mode to allow external*/
/*; bus mastering events*/

	outl(0x0cf8,0x080000004);	/*index the status command register on device 0*/
	outl(0xcfc, 0x2);			/*set the memory access enable bit*/
	OUTC(0x0fffef072, 1);		/* enable req bits in SYSARBMENB */



	/* set up the PAR registers as they are on the MSM586SEG */
	par = (unsigned long *) 0xfffef080;
	*par++ = 0x607c00a0; /*PAR0: PCI:Base 0xa0000; size 0x1f000:*/
	*par++ = 0x480400d8; /*PAR1: GP BUS MEM:CS2:Base 0xd8, size 0x4:*/
	*par++ = 0x340100ea; /*PAR2: GP BUS IO:CS5:Base 0xea, size 0x1:*/
	*par++ = 0x380701f0; /*PAR3: GP BUS IO:CS6:Base 0x1f0, size 0x7:*/
	*par++ = 0x3c0003f6; /*PAR4: GP BUS IO:CS7:Base 0x3f6, size 0x0:*/
	*par++ = 0x35ff0400; /*PAR5: GP BUS IO:CS5:Base 0x400, size 0xff:*/
	*par++ = 0x35ff0600; /*PAR6: GP BUS IO:CS5:Base 0x600, size 0xff:*/
	*par++ = 0x35ff0800; /*PAR7: GP BUS IO:CS5:Base 0x800, size 0xff:*/
	*par++ = 0x35ff0a00; /*PAR8: GP BUS IO:CS5:Base 0xa00, size 0xff:*/
	*par++ = 0x35ff0e00; /*PAR9: GP BUS IO:CS5:Base 0xe00, size 0xff:*/
	*par++ = 0x34fb0104; /*PAR10: GP BUS IO:CS5:Base 0x104, size 0xfb:*/
	*par++ = 0x35af0200; /*PAR11: GP BUS IO:CS5:Base 0x200, size 0xaf:*/
	*par++ = 0x341f03e0; /*PAR12: GP BUS IO:CS5:Base 0x3e0, size 0x1f:*/
	*par++ = 0xe41c00c0; /*PAR13: SDRAM:code:cache:nowrite:Base 0xc0000, size 0x7000:*/
	*par++ = 0x545c00c8; /*PAR14: GP BUS MEM:CS5:Base 0xc8, size 0x5c:*/
	*par++ = 0x8a020200; /*PAR15: BOOTCS:code:nocache:write:Base 0x2000000, size 0x80000:*/


}
	

/*
 *
 *
 */

#define DRCCTL        *(char*)0x0fffef010  /* DRAM control register*/
#define DRCTMCTL      *(char*)0x0fffef012  /* DRAM timing control register*/
#define DRCCFG        *(char*)0x0fffef014  /* DRAM bank configuration register*/
#define DRCBENDADR    *(char*)0x0fffef018  /* DRAM bank ending address register*/
#define ECCCTL        *(char*)0x0fffef020  /* DRAM ECC control register*/
#define DBCTL         *(char*)0x0fffef040  /* DRAM buffer control register*/

#define CACHELINESZ   0x00000010  /*  size of our cache line (read buffer)*/

#define COL11_ADR  *(unsigned int *)0x0e001e00 /* 11 col addrs*/
#define COL10_ADR  *(unsigned int *)0x0e000e00 /* 10 col addrs*/
#define COL09_ADR  *(unsigned int *)0x0e000600 /*  9 col addrs*/
#define COL08_ADR  *(unsigned int *)0x0e000200 /*  8 col addrs*/

#define ROW14_ADR  *(unsigned int *)0x0f000000 /* 14 row addrs*/
#define ROW13_ADR  *(unsigned int *)0x07000000 /* 13 row addrs*/
#define ROW12_ADR  *(unsigned int *)0x03000000 /* 12 row addrs*/
#define ROW11_ADR  *(unsigned int *)0x01000000 /* 11 row addrs/also bank switch*/
#define ROW10_ADR  *(unsigned int *)0x00000000 /* 10 row addrs/also bank switch*/

#define COL11_DATA 0x0b0b0b0b	/*  11 col addrs*/
#define COL10_DATA 0x0a0a0a0a	/*  10 col data*/
#define COL09_DATA 0x09090909	/*   9 col data*/
#define COL08_DATA 0x08080808	/*   8 col data*/
#define ROW14_DATA 0x3f3f3f3f	/*  14 row data (MASK)*/
#define ROW13_DATA 0x1f1f1f1f	/*  13 row data (MASK)*/
#define ROW12_DATA 0x0f0f0f0f	/*  12 row data (MASK)*/
#define ROW11_DATA 0x07070707	/*  11 row data/also bank switch (MASK)*/
#define ROW10_DATA 0xaaaaaaaa	/*  10 row data/also bank switch (MASK)*/

#define dummy_write()   *(short *)CACHELINESZ=0x1010

void udelay(int microseconds) {
	volatile int x;
	for(x = 0; x < 1000; x++)
		;
}

int nextbank(int bank)
{
	int rows,banks, i, ending_adr;
	
start:
	/* write col 11 wrap adr */
	COL11_ADR=COL11_DATA;
	if(COL11_ADR!=COL11_DATA)
		goto bad_ram;

	/* write col 10 wrap adr */
	COL10_ADR=COL10_DATA;
	if(COL10_ADR!=COL10_DATA)
		goto bad_ram;

	/* write col 9 wrap adr */
	COL09_ADR=COL09_DATA;
	if(COL09_ADR!=COL09_DATA)
		goto bad_ram;

	/* write col 8 wrap adr */
	COL08_ADR=COL08_DATA;
	if(COL08_ADR!=COL08_DATA)
		goto bad_ram;

	/* write row 14 wrap adr */
	ROW14_ADR=ROW14_DATA;
	if(ROW14_ADR!=ROW14_DATA)
		goto bad_ram;

	/* write row 13 wrap adr */
	ROW13_ADR=ROW13_DATA;
	if(ROW13_ADR!=ROW13_DATA)
		goto bad_ram;

	/* write row 12 wrap adr */
	ROW12_ADR=ROW12_DATA;
	if(ROW12_ADR!=ROW12_DATA)
		goto bad_ram;

	/* write row 11 wrap adr */
	ROW11_ADR=ROW11_DATA;
	if(ROW11_ADR!=ROW11_DATA)
		goto bad_ram;

	/* write row 10 wrap adr */
	ROW10_ADR=ROW10_DATA;
	if(ROW10_ADR!=ROW10_DATA)
		goto bad_ram;

/*
 * read data @ row 12 wrap adr to determine # banks,
 *  and read data @ row 14 wrap adr to determine # rows.
 *  if data @ row 12 wrap adr is not AA, 11 or 12 we have bad RAM.
 * if data @ row 12 wrap == AA, we only have 2 banks, NOT 4
 * if data @ row 12 wrap == 11 or 12, we have 4 banks
 */

	banks=2;
	if (ROW12_ADR != ROW10_DATA) {
		banks=4;
		if(ROW12_ADR != ROW11_DATA) {
			if(ROW12_ADR != ROW12_DATA)
				goto bad_ram;
		}
	}

	/* validate row mask */
	i=ROW14_ADR;
	if (i<ROW11_DATA)
		goto bad_ram;
	if (i>ROW14_DATA)
		goto bad_ram;
	/* verify all 4 bytes of dword same */
	if(i&0xffff!=(i>>16)&0xffff)
		goto bad_ram;
	if(i&0xff!=(i>>8)&0xff)
		goto bad_ram;
	
	
	/* validate column data */
	i=COL11_ADR;
	if(i<COL08_DATA)
		goto bad_ram;
	if (i>COL11_DATA)
		goto bad_ram;
	/* verify all 4 bytes of dword same */
	if(i&0xffff!=(i>>16)&0xffff)
		goto bad_ram;
	if(i&0xff!=(i>>8)&0xff)
		goto bad_ram;
	
	if(banks==4)
		i+=8; /* <-- i holds merged value */
	
	/* fix ending addr mask*/
	/*FIXME*/
	ending_adr=0xff;

bad_reint:
	/* issue all banks recharge */
	DRCCTL=0x02;
	dummy_write();

	/* update ending address register */
#warning FIX ME NOW I AM BUSTED
//	*(DRCBENDADR+0)=ending_adr;
	
	/* update config register */
//	DRCCFG=DRCCFG&YYY|ZZZZ;

	if(bank!=0) {
		bank--;
		//*(&DRCBENDADR+XXYYXX)=0xff;
		goto start;
	}

	/* set control register to NORMAL mode */
	DRCCTL=0x00;
	dummy_write();
	return bank;
	
bad_ram:
	print_info("bad ram!\r\n");
}

/* cache is assumed to be disabled */
int sizemem(void)
{
	int i;
	/* initialize dram controller registers */

	DBCTL=0; /* disable write buffer/read-ahead buffer */
	ECCCTL=0; /* disable ECC */
	DRCTMCTL=0x1e; /* Set SDRAM timing for slowest speed. */

	/* setup loop to do 4 external banks starting with bank 3 */

	/* enable last bank and setup ending address 
	 * register for max ram in last bank
	 */
	DRCBENDADR=0x0ff000000;
	/* setup dram register for all banks
	 * with max cols and max banks
	 */
	DRCCFG=0xbbbb;

	/* issue a NOP to all DRAMs */

	/* Asetup DRAM control register with Disable refresh,
 	 * disable write buffer Test Mode and NOP command select
 	 */
	DRCCTL=0x01;

	/* dummy write for NOP to take effect */
	dummy_write();

	/* 100? 200? */
	udelay(100);

	/* issue all banks precharge */
	DRCCTL=0x02;
	dummy_write();

	/* issue 2 auto refreshes to all banks */
	DRCCTL=0x04;
	dummy_write();
	dummy_write();

	/* issue LOAD MODE REGISTER command */
	DRCCTL=0x03;
	dummy_write();

	DRCCTL=0x04;
	for (i=0; i<8; i++) /* refresh 8 times */
		dummy_write();

	/* set control register to NORMAL mode */
	DRCCTL=0x00;

	nextbank(3);

}	
