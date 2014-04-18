/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2005 Tyan Computer
 * (Written by Yinghai Lu <yinghailu@gmail.com> for Tyan Computer)
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

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include "i82801xx.h"

static struct device_operations ac97_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= 0,
	.scan_bus		= 0,
	.enable			= i82801xx_enable,
};

/* 82801AA */
static const struct pci_driver i82801aa_ac97_audio __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2415,
};

static const struct pci_driver i82801aa_ac97_modem __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2416,
};

/* 82801AB */
static const struct pci_driver i82801ab_ac97_audio __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2425,
};

static const struct pci_driver i82801ab_ac97_modem __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2426,
};

/* 82801BA */
static const struct pci_driver i82801ba_ac97_audio __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2445,
};

static const struct pci_driver i82801ba_ac97_modem __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2446,
};

/* 82801CA */
static const struct pci_driver i82801ca_ac97_audio __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2485,
};

static const struct pci_driver i82801ca_ac97_modem __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x2486,
};

/* 82801DB & 82801DBM */
static const struct pci_driver i82801db_ac97_audio __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x24c5,
};

static const struct pci_driver i82801db_ac97_modem __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x24c6,
};

/* 82801EB & 82801ER */
static const struct pci_driver i82801ex_ac97_audio __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x24d5,
};

static const struct pci_driver i82801ex_ac97_modem __pci_driver = {
	.ops	= &ac97_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= 0x24d6,
};
