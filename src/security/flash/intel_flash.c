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

#include <security/flash.h>
#include <security/intel.h>

#if CONFIG_INTEL_PLATFORM_LOCKDOWN
void spi_lock(struct device *dev) {
#if CONFIG_ME_LOCK_PR
	printk(BIOS_DEBUG, "Locking SPI controller with range FREG0 - FREG4\n");

	/* Copy flash regions from FREG0-4 to PR0-4
	   and enable write protection */
	RCBA32(SPIBAR_PR0) = RCBA32(SPIBAR_FREG0) | (1 << 31);
	RCBA32(SPIBAR_PR1) = RCBA32(SPIBAR_FREG1) | (1 << 31);
	RCBA32(SPIBAR_PR2) = RCBA32(SPIBAR_FREG2) | (1 << 31);
	RCBA32(SPIBAR_PR3) = RCBA32(SPIBAR_FREG3) | (1 << 31);
	RCBA32(SPIBAR_PR4) = RCBA32(SPIBAR_FREG4) | (1 << 31);

#endif

#if CONFIG_ME_LOCK_CNTL
	u8 reg;

	printk(BIOS_DEBUG, "Locking BIOS and SMM via BIOS_CNTL register\n");

	reg = pci_read_config8(dev, BIOS_CNTL);

	reg &= ~(1 << 0);	/* clear BIOSWE */
	pci_write_config8(dev, BIOS_CNTL, reg);
	reg |= (1 << 1);	/* set BLE */
	pci_write_config8(dev, BIOS_CNTL, reg);
	reg |= (1 << 5);  /* disable flashing outside of SMM */
	pci_write_config8(dev, BIOS_CNTL, reg);
#endif
	/*
	 * Always set FLOCKDN in order to stop SPI_BAR changes!!!
	 */

	/* Set SPI opcode menu */
	RCBA16(0x3894) = SPI_OPPREFIX;
	RCBA16(0x3896) = SPI_OPTYPE;
	RCBA32(0x3898) = SPI_OPMENU_LOWER;
	RCBA32(0x389c) = SPI_OPMENU_UPPER;

	/* Lock SPIBAR via FLOCKDN and VCL bit */
	RCBA32_OR(SPIBAR_HSFS, (1 << 15));
	RCBA32_OR(SPIBAR_LVSCC, (1 << 23));
}
#endif