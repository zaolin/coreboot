#include "southbridge/amd/amd8111/amd8111_enable_rom.c"

static void bootblock_southbridge_init(void) {
	/* Setup the rom access for 4M */
	amd8111_enable_rom();
}
