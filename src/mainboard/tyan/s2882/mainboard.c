#include <device/device.h>
#include "chip.h"

#if CONFIG_CHIP_NAME == 1
struct chip_operations mainboard_tyan_s2882_ops = {
	CHIP_NAME("Tyan S2882 Mainboard")
};
#endif

