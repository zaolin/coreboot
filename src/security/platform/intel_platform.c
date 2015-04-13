/*
* This file is part of the coreboot project.
*
* Copyright (C) 2015 Philipp Deppenwiese <philipp@deppenwiese.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#define __SIMPLE_DEVICE__

#include <security/platform.h>
#include <security/intel.h>

void platform_lock(void) {
	/* TCLOCKDN: TC Lockdown */
	RCBA32_OR(CIR0, (1 << 31));

	/* BIOS Interface Lockdown */
	RCBA32_OR(GCS, (1 << 0));

	/* Function Disable SUS Well Lockdown */
	RCBA_AND_OR(8, FDSW, ~0U, (1 << 7));

	/* Global SMI Lock */
	pci_or_config16(PCH_LPC_DEV, GEN_PMCON_1, 1 << 4);

	/* GEN_PMCON Lock */
	pci_or_config8(PCH_LPC_DEV, GEN_PMCON_LOCK, (1 << 1) | (1 << 2));

	/* R/WO registers */
	RCBA32(0x21a4) = RCBA32(0x21a4);
	pci_write_config32(PCI_DEV(0, 27, 0), 0x74,
		    pci_read_config32(PCI_DEV(0, 27, 0), 0x74));
}
