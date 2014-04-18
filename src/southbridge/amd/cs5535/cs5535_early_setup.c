/*
 *
 * cs5535_early_setup.c:	Early chipset initialization for CS5535 companion device
 *
 *
 * This file implements the initialization sequence documented in section 4.2 of
 * AMD Geode GX Processor CS5535 Companion Device GoedeROM Porting Guide.
 *
 */

#define CS5535_GLINK_PORT_NUM	0x02	/* the geode link port number to the CS5535 */       
#define CS5535_DEV_NUM 		0x0F	/* default PCI device number for CS5535 */

/**
 * @brief Setup PCI IDSEL for CS5535
 *
 * 
 */

static void cs5535_setup_extmsr(void)
{
	msr_t msr;

	/* forward MSR access to CS5535_GLINK_PORT_NUM to CS5535_DEV_NUM */
	msr.hi = msr.lo = 0x00000000;
	if (CS5535_GLINK_PORT_NUM <= 4) {
		msr.lo = CS5535_DEV_NUM << ((CS5535_GLINK_PORT_NUM - 1) * 8);
	} else {
		msr.hi = CS5535_DEV_NUM << ((CS5535_GLINK_PORT_NUM - 5) * 8);
	}
	wrmsr(0x5000201e, msr);
}

static void cs5535_setup_idsel(void)
{
	/* write IDSEL to the write once register at address 0x0000 */
	outl(0x1 << (CS5535_DEV_NUM + 10), 0);
}

static int cs5535_setup_iobase(void)
{
	msr_t msr;

	/* setup LBAR for SMBus controller */
	__builtin_wrmsr(0x5140000b, 0x00006000, 0x0000f001);
	/* setup LBAR for GPIO */
	__builtin_wrmsr(0x5140000c, 0x00006100, 0x0000f001);
	/* setup LBAR for MFGPT */
	__builtin_wrmsr(0x5140000d, 0x00006200, 0x0000f001);
	/* setup LBAR for ACPI */
	__builtin_wrmsr(0x5140000e, 0x00009c00, 0x0000f001);
	/* setup LBAR for MFGPT */
	__builtin_wrmsr(0x5140000f, 0x00009d00, 0x0000f001);
}

static void cs5535_setup_gpio(void)
{
	uint32_t val;

	/* setup GPIO pins 14/15 for SDA/SCL */
	val = (1<<14 | 1<<15);
	/* Output Enable */
	outl(0x3fffc000, 0x6100 + 0x04);
	//outl(val, 0x6100 + 0x04);
	/* Output AUX1 */
	outl(0x3fffc000, 0x6100 + 0x10);
	//outl(val, 0x6100 + 0x10);
	/* Input Enable */
	//outl(0x0f5af0a5, 0x6100 + 0x20);
	outl(0x3fffc000, 0x6100 + 0x20);
	//outl(val, 0x6100 + 0x20);
	/* Input AUX1 */
	//outl(0x3ffbc004, 0x6100 + 0x34);
	outl(0x3fffc000, 0x6100 + 0x34);
	//outl(val, 0x6100 + 0x34);
}

static void cs5535_setup_cis_mode(void)
{
	msr_t msr;

	/* setup CPU interface serial to mode C on both sides */
	msr = __builtin_rdmsr(0x51000010);
	msr.lo &= ~0x18;
	msr.lo |= 0x10;
	__builtin_wrmsr(0x51000010, msr.lo, msr.hi);
	__builtin_wrmsr(0x54002010, 0x00000002, 0x00000000);
}

static void dummy(void)
{
}

static int cs5535_early_setup(void)
{
	msr_t msr;

	cs5535_setup_extmsr();

	msr = rdmsr(0x4c000014);
	if (msr.lo & (0x3f << 26)) {
		/* PLL is already set and we are reboot from PLL reset */
		print_debug("reboot from BIOS reset\n\r");
		return;
	}
	cs5535_setup_idsel();
	cs5535_setup_iobase();
	cs5535_setup_gpio();
	cs5535_setup_cis_mode();
	cs5535_enable_smbus();
	//get_memory_speed();
	dummy();
}
