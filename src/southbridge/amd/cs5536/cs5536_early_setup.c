/*
 *
 * cs5536_early_setup.c:	Early chipset initialization for CS5536 companion device
 *
 *
 * This file implements the initialization sequence documented in section 4.2 of
 * AMD Geode GX Processor CS5536 Companion Device GoedeROM Porting Guide.
 *
 */

#define CS5536_GLINK_PORT_NUM	0x02	/* the geode link port number to the CS5536 */       
#define CS5536_DEV_NUM 		0x0F	/* default PCI device number for CS5536 */

/**
 * @brief Setup PCI IDSEL for CS5536
 *
 * 
 */

static void cs5536_setup_extmsr(void)
{
	msr_t msr;

	/* forward MSR access to CS5536_GLINK_PORT_NUM to CS5536_DEV_NUM */
	msr.hi = msr.lo = 0x00000000;
	if (CS5536_GLINK_PORT_NUM <= 4) {
		msr.lo = CS5536_DEV_NUM << ((CS5536_GLINK_PORT_NUM - 1) * 8);
	} else {
		msr.hi = CS5536_DEV_NUM << ((CS5536_GLINK_PORT_NUM - 5) * 8);
	}
	wrmsr(0x5000201e, msr);
}

static void cs5536_setup_idsel(void)
{
	/* write IDSEL to the write once register at address 0x0000 */
	outl(0x1 << (CS5536_DEV_NUM + 10), 0);
}

static void cs5536_usb_swapsif(void)
{
	msr_t msr;

	msr = rdmsr(0x51600005);
	//USB Serial short detect bit.
	if (msr.hi & 0x10) {
		/* We need to preserve bits 32,33,35 and not clear any BIST error, but clear the
		 * SERSHRT error bit */
		msr.hi &= 0xFFFFFFFB;
		wrmsr(0x51600005, msr);
	}
}

static int cs5536_setup_iobase(void)
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
	/* setup LBAR for PM Support */
	__builtin_wrmsr(0x5140000f, 0x00009d00, 0x0000f001);
}

static void cs5536_setup_power_bottun(void)
{
	/* not implemented yet */
#if 0
	pwrBtn_setup:
	;
	;	Power Button Setup
	;
	;mov	eax, 0C0020000h				; 4 seconds + lock
	mov	eax, 040020000h				; 4 seconds no lock
	mov	dx, PMLogic_BASE + 40h
	out	dx, eax

	; setup GPIO24, it is the external signal for 5536 vsb_work_aux
	; which controls all voltage rails except Vstandby & Vmem.
	; We need to enable, OUT_AUX1 and OUTPUT_ENABLE in this order.
	; If GPIO24 is not enabled then soft-off will not work.
	mov	dx, GPIOH_OUT_AUX1_SELECT
	mov	eax, GPIOH_24_SET
	out	dx, eax
	mov	dx, GPIOH_OUTPUT_ENABLE
	out	dx, eax

#endif
}

static void cs5536_setup_gpio(void)
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

static void cs5536_disable_internal_uart(void)
{
	/* not implemented yet */
#if 0
	; The UARTs default to enabled.
	; Disable and reset them and configure them later. (SIO init)
	mov	ecx, MDD_UART1_CONF
	RDMSR
	mov	eax, 1h					; reset
	WRMSR
	mov	eax, 0h					; disabled
	WRMSR

	mov	ecx, MDD_UART2_CONF
	RDMSR
	mov	eax, 1h					; reset
	WRMSR
	mov	eax, 0h					; disabled
	WRMSR

#endif
}

static void cs5536_setup_cis_mode(void)
{
	msr_t msr;

	/* setup CPU interface serial to mode C on both sides */
	msr = __builtin_rdmsr(0x51000010);
	msr.lo &= ~0x18;
	msr.lo |= 0x10;
	__builtin_wrmsr(0x51000010, msr.lo, msr.hi);
	//Only do this if we are building for 5536
	__builtin_wrmsr(0x54002010, 0x00000002, 0x00000000);
}

static void dummy(void)
{
}

/* see page 412 of the cs5536 companion book */
static int cs5536_setup_onchipuart(void)
{
	/* ToDo:
	 * 1. Eanble GPIO 8 to OUT_AUX1, 9 to IN_AUX1
	 *    GPIO LBAR + 0x04, LBAR + 0x10, LBAR + 0x20, LBAR + 34
	 * 2. Enable UART IO space in MDD
	 *    MSR 0x51400014 bit 18:16
	 * 3. Enable UART controller
	 *    MSR 0x5140003A bit 0, 1
	 * 4. IRQ routing on IRQ Mapper
	 *    MSR 0x51400021 bit [27:24]
	 */
	msr_t msr;
	msr.lo = 2;
	msr.hi = 0;
	wrmsr(0x5140003a, msr);
	wrmsr(0x5140003e, msr);
}

static int cs5536_early_setup(void)
{
	msr_t msr;

	cs5536_setup_extmsr();

	msr = rdmsr(GLCP_SYS_RSTPLL);
	if (msr.lo & (0x3f << 26)) {
		/* PLL is already set and we are reboot from PLL reset */
		print_debug("reboot from BIOS reset\n\r");
		return;
	}
	print_debug("Setup idsel\r\n");
	cs5536_setup_idsel();
	print_debug("Setup iobase\r\n");
	cs5536_usb_swapsif();
	cs5536_setup_iobase();
	print_debug("Setup gpio\r\n");
	cs5536_setup_gpio();
	print_debug("Setup cis_mode\r\n");
	cs5536_setup_cis_mode();
	print_debug("Setup smbus\r\n");
	cs5536_enable_smbus();
	dummy();
}
