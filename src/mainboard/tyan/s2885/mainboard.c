#include <device/device.h>
#include "chip.h"

#if CONFIG_CHIP_NAME == 1
struct chip_operations mainboard_ops = {
	CHIP_NAME("Tyan S2885 Mainboard")
};
#endif

