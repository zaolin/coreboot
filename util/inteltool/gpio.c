/*
 * inteltool - dump all registers on an Intel CPU + chipset based system.
 *
 * Copyright (C) 2008 by coresystems GmbH 
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sys/io.h>
#include "inteltool.h"

static const io_register_t ich0_gpio_registers[] = {
	{ 0x00, 4, "GPIO_USE_SEL" },
	{ 0x04, 4, "GP_IO_SEL" },
	{ 0x08, 4, "RESERVED" },
	{ 0x0c, 4, "GP_LVL" },
	{ 0x10, 4, "RESERVED" },
	{ 0x14, 4, "GPO_TTL" },
	{ 0x18, 4, "GPO_BLINK" },
	{ 0x1c, 4, "RESERVED" },
	{ 0x20, 4, "RESERVED" },
	{ 0x24, 4, "RESERVED" },
	{ 0x28, 4, "RESERVED" },
	{ 0x2c, 4, "GPI_INV" },
	{ 0x30, 4, "RESERVED" },
	{ 0x34, 4, "RESERVED" },
	{ 0x38, 4, "RESERVED" },
	{ 0x3C, 4, "RESERVED" }
};

static const io_register_t ich4_gpio_registers[] = {
	{ 0x00, 4, "GPIO_USE_SEL" },
	{ 0x04, 4, "GP_IO_SEL" },
	{ 0x08, 4, "RESERVED" },
	{ 0x0c, 4, "GP_LVL" },
	{ 0x10, 4, "RESERVED" },
	{ 0x14, 4, "GPO_TTL" },
	{ 0x18, 4, "GPO_BLINK" },
	{ 0x1c, 4, "RESERVED" },
	{ 0x20, 4, "RESERVED" },
	{ 0x24, 4, "RESERVED" },
	{ 0x28, 4, "RESERVED" },
	{ 0x2c, 4, "GPI_INV" },
	{ 0x30, 4, "GPIO_USE_SEL2" },
	{ 0x34, 4, "GP_IO_SEL2" },
	{ 0x38, 4, "GP_LVL2" },
	{ 0x3C, 4, "RESERVED" }
};

static const io_register_t ich7_gpio_registers[] = {
	{ 0x00, 4, "GPIO_USE_SEL" },
	{ 0x04, 4, "GP_IO_SEL" },
	{ 0x08, 4, "RESERVED" },
	{ 0x0c, 4, "GP_LVL" },
	{ 0x10, 4, "RESERVED" },
	{ 0x14, 4, "RESERVED" },
	{ 0x18, 4, "GPO_BLINK" },
	{ 0x1c, 4, "RESERVED" },
	{ 0x20, 4, "RESERVED" },
	{ 0x24, 4, "RESERVED" },
	{ 0x28, 4, "RESERVED" },
	{ 0x2c, 4, "GPI_INV" },
	{ 0x30, 4, "GPIO_USE_SEL2" },
	{ 0x34, 4, "GP_IO_SEL2" },
	{ 0x38, 4, "GP_LVL2" },
	{ 0x3C, 4, "RESERVED" }
};

int print_gpios(struct pci_dev *sb)
{
	int i, size;
	uint16_t gpiobase;
	const io_register_t *gpio_registers;

	printf("\n============= GPIOS =============\n\n");

	switch (sb->device_id) {
	case PCI_DEVICE_ID_INTEL_ICH7:
	case PCI_DEVICE_ID_INTEL_ICH7M:
	case PCI_DEVICE_ID_INTEL_ICH7DH:
	case PCI_DEVICE_ID_INTEL_ICH7MDH:
		gpiobase = pci_read_word(sb, 0x48) & 0xfffc;
		gpio_registers = ich7_gpio_registers;
		size = ARRAY_SIZE(ich7_gpio_registers);
		break;
	case PCI_DEVICE_ID_INTEL_ICH4:
	case PCI_DEVICE_ID_INTEL_ICH4M:
		gpiobase = pci_read_word(sb, 0x58) & 0xfffc;
		gpio_registers = ich4_gpio_registers;
		size = ARRAY_SIZE(ich4_gpio_registers);
		break;
	case PCI_DEVICE_ID_INTEL_ICH:
	case PCI_DEVICE_ID_INTEL_ICH0:
		gpiobase = pci_read_word(sb, 0x58) & 0xfffc;
		gpio_registers = ich0_gpio_registers;
		size = ARRAY_SIZE(ich0_gpio_registers);
		break;
	case 0x1234: // Dummy for non-existent functionality
		printf("This southbridge does not have GPIOBASE.\n");
		return 1;
	default:
		printf("Error: Dumping GPIOs on this southbridge is not (yet) supported.\n");
		return 1;
	}

	printf("GPIOBASE = 0x%04x (IO)\n\n", gpiobase);

	for (i = 0; i < size; i++) {
		switch (gpio_registers[i].size) {
		case 4:
			printf("gpiobase+0x%04x: 0x%08x (%s)\n",
				gpio_registers[i].addr,
				inl(gpiobase+gpio_registers[i].addr),
				gpio_registers[i].name);
			break;
		case 2:
			printf("gpiobase+0x%04x: 0x%04x     (%s)\n",
				gpio_registers[i].addr,
				inw(gpiobase+gpio_registers[i].addr),
				gpio_registers[i].name);
			break;
		case 1:
			printf("gpiobase+0x%04x: 0x%02x       (%s)\n",
				gpio_registers[i].addr,
				inb(gpiobase+gpio_registers[i].addr),
				gpio_registers[i].name);
			break;
		}
	}

	return 0;
}

