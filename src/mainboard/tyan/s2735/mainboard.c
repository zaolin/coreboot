#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "chip.h"

struct chip_operations mainboard_tyan_s2735_ops = {
	CHIP_NAME("Tyan s2735 mainboard")
};
