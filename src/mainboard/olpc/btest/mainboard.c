#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <arch/io.h>
#include <pc80/mc146818rtc.h>
#include "chip.h"
#include "../southbridge/amd/cs5536/cs5536_smbus2.h"
#include <cpu/amd/vr.h>

/* Borrowed from mc146818rtc.c */

#define CMOS_READ(addr) ({ \
		outb((addr),RTC_PORT(0)); \
		inb(RTC_PORT(1)); \
		})

#define CMOS_WRITE(val, addr) ({ \
		outb((addr),RTC_PORT(0)); \
		outb((val),RTC_PORT(1)); \
		})

static void write_bit(unsigned char val) {

	unsigned char byte = CMOS_READ(440 / 8);

	/* Don't change it if its already set */

	if ((byte & 1) == (val & 1))
		return;

	byte &= ~1;
	byte |= val & 1;
	CMOS_WRITE(val, 440/8);
}

static unsigned short _getsmbusbase(void) {
	unsigned devfn = PCI_DEVFN(0xf, 0);
	device_t dev = dev_find_slot(0x0, devfn);
	unsigned long addr = pci_read_config32(dev, PCI_BASE_ADDRESS_0);

	return (unsigned short) (addr & ~1);
}

static void init_dcon(void) {

  int ret = 1;
  unsigned short rev = 0;
  unsigned short iobase = _getsmbusbase();

  printk(BIOS_DEBUG, "CHECKING FOR DCON (%x)\n", iobase);

  /* Get the IO base for the SMBUS */

  rev = do_smbus_read_word(iobase, 0x0D << 1, 0x00);

  if (rev & 0xDC00) {
	printk(BIOS_DEBUG, "DCON FOUND - REV %x\n", rev);

	/* Enable the DCON */
	ret = do_smbus_write_word(iobase, 0x0D << 1, 0x01, 0x0069);
	if (ret != 0)
		printk(BIOS_DEBUG, "DCON ENABLE FAILED %d\n", ret);
  }
  else
	  printk(BIOS_DEBUG, "DCON NOT FOUND (%x)\n", rev);

  write_bit(rev > 0 ? 1 : 0);
}

static void
init_cafe_irq(void){
	const unsigned char slots_cafe[4] = {11, 0, 0, 0};


	/* CAFE PCI slots */
	pci_assign_irqs(0, 0x0C, slots_cafe);

	/* Make the pin assignments - NOTENOTENOTE:  This should be
	  * configurable!
	  */

	/* Configure the GPIO pins to use - class 0, index 9 to configure
	  * AB.  Write 0xFF to disable
	  */

	vrWrite(0x9, 0XFF00);

	/* Configure the GPIO pins to use - class 0, index A to configure
	  * CD.  Write 0xFF to disable
	  */

       vrWrite(0xA, 0xFFFF);

}


static void init(struct device *dev) {
/*
	unsigned bus = 0;
	unsigned devfn = PCI_DEVFN(0xf, 4);
	device_t usb = NULL;
	unsigned char usbirq = 0xa;
*/

	printk(BIOS_DEBUG, "OLPC BTEST ENTER %s\n", __func__);

#if 0
	/* I can't think of any reason NOT to just set this. If it turns out we want this to be
	  * conditional we can make it a config variable later.
	  */

	printk(BIOS_DEBUG, "%s (%x,%x)SET USB PCI interrupt line to %d\n",
		__func__, bus, devfn, usbirq);
	usb = dev_find_slot(bus, devfn);
	if (! usb){
		printk(BIOS_ERR, "Could not find USB\n");
	} else {
		pci_write_config8(usb, PCI_INTERRUPT_LINE, usbirq);
	}
#endif

	init_dcon();
	init_cafe_irq();
	printk(BIOS_DEBUG, "OLPC BTEST EXIT %s\n", __func__);
}

static void enable_dev(struct device *dev)
{
        dev->ops->init = init;
}

struct chip_operations mainboard_ops = {
	CHIP_NAME("OLPC btest Mainboard")
        .enable_dev = enable_dev,
};
