#define ASSEMBLY 1
#include <stdint.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <arch/romcc_io.h>
#include <arch/smp/lapic.h>
#include "pc80/mc146818rtc_early.c"
#include "southbridge/amd/amd8111/amd8111_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"
#include "cpu/p6/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"

#define HAVE_REGPARM_SUPPORT 0
#if HAVE_REGPARM_SUPPORT
static unsigned long main(unsigned long bist)
{
#else
static void main(void)
{
	unsigned long bist = 0;
#endif
	/* Make cerain my local apic is useable */
	enable_lapic();

	/* Is this a cpu only reset? */
	if (cpu_init_detected()) {
		if (last_boot_normal()) {
			goto normal_image;
		} else {
			goto cpu_reset;
		}
	}
	/* Is this a secondary cpu? */
	if (!boot_cpu()) {
		if (last_boot_normal()) {
			goto normal_image;
		} else {
			goto fallback_image;
		}
	}
	

	/* Nothing special needs to be done to find bus 0 */
	/* Allow the HT devices to be found */
	enumerate_ht_chain();
	
	/* Setup the 8111 */
	amd8111_enable_rom();

	/* Is this a deliberate reset by the bios */
	if (bios_reset_detected() && last_boot_normal()) {
		goto normal_image;
	}
	/* This is the primary cpu how should I boot? */
	else if (do_normal_boot()) {
		goto normal_image;
	}
	else {
		goto fallback_image;
	}
 normal_image:
	asm("jmp __normal_image" 
		: /* outputs */ 
		: "a" (bist) /* inputs */
		: /* clobbers */
		);
 cpu_reset:
	asm("jmp __cpu_reset"
		: /* outputs */ 
		: "a"(bist) /* inputs */
		: /* clobbers */
		);
 fallback_image:
#if HAVE_REGPARM_SUPPORT
	return bist;
#else
	return;
#endif
}
