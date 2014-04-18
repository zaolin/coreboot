#include <console/console.h>
#include <pc80/keyboard.h>
#include <device/device.h>
#include <arch/io.h>

/* much better keyboard init courtesy ollie@sis.com.tw 
   TODO: Typematic Setting, the keyboard is too slow for me */
static void pc_keyboard_init(struct pc_keyboard *keyboard)
{
	volatile unsigned char regval;

	/* send cmd = 0xAA, self test 8042 */
	outb(0xaa, 0x64);

	/* empty input buffer or any other command/data will be lost */
	while ((inb(0x64) & 0x02))
		post_code(0);
	/* empty output buffer or any other command/data will be lost */
	while ((inb(0x64) & 0x01) == 0)
		post_code(1);

	/* read self-test result, 0x55 should be returned form 0x60 */
	if ((regval = inb(0x60) != 0x55))
		return;

	/* enable keyboard interface */
	outb(0x60, 0x64);
	while ((inb(0x64) & 0x02))
		post_code(2);

	/* send cmd: enable IRQ 1 */
	outb(0x61, 0x60);
	while ((inb(0x64) & 0x02))
		post_code(3);

	/* reset kerboard and self test  (keyboard side) */
	outb(0xff, 0x60);

	/* empty inut bufferm or any other command/data will be lost */
	while ((inb(0x64) & 0x02))
		post_code(4);
	/* empty output buffer or any other command/data will be lost */
	while ((inb(0x64) & 0x01) == 0)
		post_code(5);

	if ((regval = inb(0x60) != 0xfa))
		return;

	while ((inb(0x64) & 0x01) == 0)
		post_code(6);
	if ((regval = inb(0x60) != 0xaa))
		return;
}

void init_pc_keyboard(unsigned port0, unsigned port1, struct pc_keyboard *kbd)
{
	if ((port0 == 0x60) && (port1 == 0x64)) {
		pc_keyboard_init(kbd);
	}
}
