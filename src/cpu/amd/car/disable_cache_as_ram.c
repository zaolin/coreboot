/* by yhlu 6.2005 */
/* be warned, this file will be used other cores and core 0 / node 0 */
        __asm__ volatile (
	/* 
	FIXME : backup stack in CACHE_AS_RAM into mmx and sse and after we get STACK up, we restore that.
		It is only needed if we want to go back
	*/
	
        /* We don't need cache as ram for now on */
        /* disable cache */
        "movl    %cr0, %eax\n\t"
        "orl    $(0x1<<30),%eax\n\t"
        "movl    %eax, %cr0\n\t"

        /* clear sth */
        "movl    $0x269, %ecx\n\t"  /* fix4k_c8000*/
        "xorl    %edx, %edx\n\t"
        "xorl    %eax, %eax\n\t"
	"wrmsr\n\t"
#if DCACHE_RAM_SIZE > 0x8000
	"movl    $0x268, %ecx\n\t"  /* fix4k_c0000*/
        "wrmsr\n\t"
#endif

        /* disable fixed mtrr from now on, it will be enabled by linuxbios_ram again*/
        "movl    $0xC0010010, %ecx\n\t"
//        "movl    $SYSCFG_MSR, %ecx\n\t"
        "rdmsr\n\t"
        "andl    $(~(3<<18)), %eax\n\t"
//        "andl    $(~(SYSCFG_MSR_MtrrFixDramModEn | SYSCFG_MSR_MtrrFixDramEn)), %eax\n\t"
        "wrmsr\n\t"

        /* Set the default memory type and disable fixed and enable variable MTRRs */
        "movl    $0x2ff, %ecx\n\t"
//        "movl    $MTRRdefType_MSR, %ecx\n\t"
        "xorl    %edx, %edx\n\t"
        /* Enable Variable and Disable Fixed MTRRs */
        "movl    $0x00000800, %eax\n\t"
        "wrmsr\n\t"

        /* enable cache */
        "movl    %cr0, %eax\n\t"
        "andl    $0x9fffffff,%eax\n\t"
        "movl    %eax, %cr0\n\t"

        );
