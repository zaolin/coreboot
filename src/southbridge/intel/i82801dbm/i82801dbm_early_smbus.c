
//#define SMBUS_IO_BASE 0x1000
#define SMBUS_IO_BASE 0x0f00

#define SMBHSTSTAT 0x0
#define SMBHSTCTL  0x2
#define SMBHSTCMD  0x3
#define SMBXMITADD 0x4
#define SMBHSTDAT0 0x5
#define SMBHSTDAT1 0x6
#define SMBBLKDAT  0x7
#define SMBTRNSADD 0x9
#define SMBSLVDATA 0xa
#define SMLINK_PIN_CTL 0xe
#define SMBUS_PIN_CTL  0xf 

/* Between 1-10 seconds, We should never timeout normally 
 * Longer than this is just painful when a timeout condition occurs.
 */
#define SMBUS_TIMEOUT (100*1000*10)

static void enable_smbus(void)
{
	device_t dev;
	dev = pci_locate_device(PCI_ID(0x8086, 0x24c3), 0);
	if (dev == PCI_DEV_INVALID) {
		die("SMBUS controller not found\r\n");
	}
	
	print_debug("SMBus controller enabled\r\n");
	/* set smbus iobase */
	pci_write_config32(dev, 0x20, SMBUS_IO_BASE | 1);
	/* Set smbus enable */
	pci_write_config8(dev, 0x40, 0x01);
	/* Set smbus iospace enable */ 
	pci_write_config16(dev, 0x4, 0x01);
	/* Disable interrupt generation */ 
	outb(0, SMBUS_IO_BASE + SMBHSTCTL);
	/* clear any lingering errors, so the transaction will run */
	outb(inb(SMBUS_IO_BASE + SMBHSTSTAT), SMBUS_IO_BASE + SMBHSTSTAT);
}


static inline void smbus_delay(void)
{
	outb(0x80, 0x80);
}

static int smbus_wait_until_active(void)
{
	unsigned long loops;
	loops = SMBUS_TIMEOUT;
	do {
		unsigned char val;
		smbus_delay();
		val = inb(SMBUS_IO_BASE + SMBHSTSTAT);
		if ((val & 1)) {
			break;
		}
	} while(--loops);
	return loops?0:-4;
}

static int smbus_wait_until_ready(void)
{
	unsigned long loops;
	loops = SMBUS_TIMEOUT;
	do {
		unsigned char val;
		smbus_delay();
		val = inb(SMBUS_IO_BASE + SMBHSTSTAT);
		if ((val & 1) == 0) {
			break;
		}
		if(loops == (SMBUS_TIMEOUT / 2)) {
			outb(inb(SMBUS_IO_BASE + SMBHSTSTAT), 
				SMBUS_IO_BASE + SMBHSTSTAT);
		}
	} while(--loops);
	return loops?0:-2;
}

static int smbus_wait_until_done(void)
{
	unsigned long loops;
	loops = SMBUS_TIMEOUT;
	do {
		unsigned char val;
		smbus_delay();
		
		val = inb(SMBUS_IO_BASE + SMBHSTSTAT);
		if ( (val & 1) == 0) {
			break;
		}
		if ((val & ~((1<<6)|(1<<0)) ) != 0 ) {
			break;
		}
	} while(--loops);
	return loops?0:-3;
}

static int smbus_read_byte(unsigned device, unsigned address)
{
	unsigned char global_control_register;
	unsigned char global_status_register;
	unsigned char byte;

print_err("smbus_read_byte\r\n");
	if (smbus_wait_until_ready() < 0) {
		print_err_hex8(-2);
		return -2;
	}
	
	/* setup transaction */
	/* disable interrupts */
	outb(inb(SMBUS_IO_BASE + SMBHSTCTL) & 0xfe, SMBUS_IO_BASE + SMBHSTCTL);
	/* set the device I'm talking too */
	outb(((device & 0x7f) << 1) | 1, SMBUS_IO_BASE + SMBXMITADD);
	/* set the command/address... */
	outb(address & 0xFF, SMBUS_IO_BASE + SMBHSTCMD);
	/* set up for a byte data read */
	outb((inb(SMBUS_IO_BASE + SMBHSTCTL) & 0xe3) | (0x2<<2), SMBUS_IO_BASE + SMBHSTCTL);

	/* clear any lingering errors, so the transaction will run */
	outb(inb(SMBUS_IO_BASE + SMBHSTSTAT), SMBUS_IO_BASE + SMBHSTSTAT);

	/* clear the data byte...*/
	outb(0, SMBUS_IO_BASE + SMBHSTDAT0);

	/* start a byte read, with interrupts disabled */
	outb((inb(SMBUS_IO_BASE + SMBHSTCTL) | 0x40), SMBUS_IO_BASE + SMBHSTCTL);
	/* poll for it to start */
	if (smbus_wait_until_active() < 0) {
		print_err_hex8(-4);
		return -4;
	}

	/* poll for transaction completion */
	if (smbus_wait_until_done() < 0) {
		print_err_hex8(-3);
		return -3;
	}

	global_status_register = inb(SMBUS_IO_BASE + SMBHSTSTAT) & ~(1<<6); /* Ignore the In Use Status... */

	/* read results of transaction */
	byte = inb(SMBUS_IO_BASE + SMBHSTDAT0);

	if (global_status_register != 2) {
		print_err_hex8(-1);
		return -1;
	}
        print_err("smbus_read_byte: ");
	print_err_hex32(device); print_err(" ad "); print_err_hex32(address);
	print_err("value "); print_err_hex8(byte); print_err("\r\n");
	return byte;
}
#if 0
static void smbus_write_byte(unsigned device, unsigned address, unsigned char val)
{
	if (smbus_wait_until_ready() < 0) {
		return;
	}

	/* by LYH */
	outb(0x37,SMBUS_IO_BASE + SMBHSTSTAT);
	/* set the device I'm talking too */
	outw(((device & 0x7f) << 1) | 0, SMBUS_IO_BASE + SMBHSTADDR);

	/* data to send */
	outb(val, SMBUS_IO_BASE + SMBHSTDAT);

	outb(address & 0xFF, SMBUS_IO_BASE + SMBHSTCMD);

	/* start the command */
	outb(0xa, SMBUS_IO_BASE + SMBHSTCTL);

	/* poll for transaction completion */
	smbus_wait_until_done();
	return;
}
#endif
