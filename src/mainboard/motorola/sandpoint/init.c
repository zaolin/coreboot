/*
 * Copyright (C) 2003, Greg Watson <gwatson@lanl.gov>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Do very early board initialization:
 *
 * - Configure External Bus (EBC)
 * - Setup Flash
 * - Setup NVRTC
 * - Setup Board Control and Status Registers (BCSR)
 * - Enable UART0 for debugging
 */

#include <ppc_asm.tmpl>
#include <ppc.h>
#include <arch/io.h>

void pnp_output(char address, char data)
{
	outb(address, PNP_CFGADDR);
	outb(data, PNP_CFGDATA);
}

void
board_init(void)
{
	/*
	 * Configure FLASH
	 */

	/*
	 * Configure NVTRC/BCSR
	 */
	
	/*
	 * Enable UART0
	 */
	pnp_output(0x07, 6); /* LD 6 = UART0 */
	pnp_output(0x30, 0); /* Dectivate */
	pnp_output(0x60, TTYS0_BASE >> 8); /* IO Base */
	pnp_output(0x61, TTYS0_BASE & 0xFF); /* IO Base */
	pnp_output(0x30, 1); /* Activate */
	uart8250_init(UART0_IO_BASE, 115200/TTYS0_BAUD, TTYS0_LCS);
}
