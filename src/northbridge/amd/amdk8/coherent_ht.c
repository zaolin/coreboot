/* coherent hypertransport initialization for AMD64 
 * 
 * written by Stefan Reinauer <stepan@openbios.org>
 * (c) 2003-2004 by SuSE Linux AG
 *
 * (c) 2004 Tyan Computer
 *  2004.12 yhlu added support to create routing table dynamically.
 *          it also support 8 ways too. (8 ways ladder or 8 ways crossbar)
 *
 * This code is licensed under GPL.
 */

/*
 * This algorithm assumes a grid configuration as follows:
 *
 * nodes :  1    2    4    6    8
 * org.  :  1x1  2x1  2x2  2x3  2x4
 Ladder:
			CPU7-------------CPU6 
			|                |    
			|                |
			|                | 
			|                |     
			|                |        
			|                |    
			CPU5-------------CPU4                    
			|                |    
			|                |   
			|                |  
			|                |    
			|                |         
			|                |       
			CPU3-------------CPU2              
			|                |    
			|                | 
			|                |
			|                |
			|                | 
			|                |    
			CPU1-------------CPU0    
 CROSS_BAR_47_56:
			CPU7-------------CPU6 
			|  \____    ___/ |    
			|       \  /     |
			|        \/      | 
			|        /\      |     
			|       /  \     |        
			|  ____/    \___ |    
			CPU5             CPU4                    
			|                |    
			|                |   
			|                |  
			|                |    
			|                |         
			|                |       
			CPU3-------------CPU2              
			|                |    
			|                | 
			|                |
			|                |
			|                | 
			|                |    
			CPU1-------------CPU0     	
 */

#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/hypertransport_def.h>
#include "arch/romcc_io.h"
#include "amdk8.h"

#define enable_bsp_routing()	enable_routing(0)

#define NODE_HT(x) PCI_DEV(0,24+x,0)
#define NODE_MP(x) PCI_DEV(0,24+x,1)
#define NODE_MC(x) PCI_DEV(0,24+x,3)

#define DEFAULT 0x00010101	/* default row entry */

typedef uint8_t u8;
typedef uint32_t u32;

#ifndef CROSS_BAR_47_56
	#define CROSS_BAR_47_56 0
#endif

#ifndef TRY_HIGH_FIRST
	#define TRY_HIGH_FIRST 0
#endif


static inline void print_linkn (const char *strval, uint8_t byteval) 
{
#if 1
	print_debug(strval); print_debug_hex8(byteval); print_debug("\r\n");
#endif
}

static void disable_probes(void)
{
	/* disable read/write/fill probes for uniprocessor setup
	 * they don't make sense if only one cpu is available
	 */

	/* Hypetransport Transaction Control Register 
	 * F0:0x68
	 * [ 0: 0] Disable read byte probe
	 *         0 = Probes issues
	 *         1 = Probes not issued
	 * [ 1: 1] Disable Read Doubleword probe
	 *         0 = Probes issued
	 *         1 = Probes not issued
	 * [ 2: 2] Disable write byte probes
	 *         0 = Probes issued
	 *         1 = Probes not issued
	 * [ 3: 3] Disable Write Doubleword Probes
	 *         0 = Probes issued
	 *         1 = Probes not issued.
	 * [10:10] Disable Fill Probe
	 *         0 = Probes issued for cache fills
	 *         1 = Probes not issued for cache fills.
	 */

	u32 val;

	print_spew("Disabling read/write/fill probes for UP... ");

	val=pci_read_config32(NODE_HT(0), 0x68);
	val |= (1<<10)|(1<<9)|(1<<8)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|(1 << 0);
	pci_write_config32(NODE_HT(0), 0x68, val);

	print_spew("done.\r\n");

}

#ifndef ENABLE_APIC_EXT_ID
#define ENABLE_APIC_EXT_ID 0 
#endif

static void enable_apic_ext_id(u8 node)
{
#if ENABLE_APIC_EXT_ID==1
#warning "FIXME Is the right place to enable apic ext id here?"

        u32 val;

        val = pci_read_config32(NODE_HT(node), 0x68);
        val |= HTTC_APIC_EXT_ID | HTTC_APIC_EXT_BRD_CST ;
        pci_write_config32(NODE_HT(node), 0x68, val);
#endif
}

static void enable_routing(u8 node)
{
	u32 val;

	/* HT Initialization Control Register
	 * F0:0x6C
	 * [ 0: 0] Routing Table Disable
	 *         0 = Packets are routed according to routing tables
	 *         1 = Packets are routed according to the default link field
	 * [ 1: 1] Request Disable (BSP should clear this)
	 *         0 = Request packets may be generated
	 *         1 = Request packets may not be generated.
	 * [ 3: 2] Default Link (Read-only)
	 *         00 = LDT0
	 *         01 = LDT1
	 *         10 = LDT2
	 *         11 = CPU on same node
	 * [ 4: 4] Cold Reset
	 *         - Scratch bit cleared by a cold reset
	 * [ 5: 5] BIOS Reset Detect
	 *         - Scratch bit cleared by a cold reset
	 * [ 6: 6] INIT Detect
	 *         - Scratch bit cleared by a warm or cold reset not by an INIT
	 *
	 */

	/* Enable routing table */
	print_spew("Enabling routing table for node ");
	print_spew_hex8(node);
	
	val=pci_read_config32(NODE_HT(node), 0x6c);
	val &= ~((1<<1)|(1<<0));
	pci_write_config32(NODE_HT(node), 0x6c, val);

	print_spew(" done.\r\n");
}

static void fill_row(u8 node, u8 row, u32 value)
{
	pci_write_config32(NODE_HT(node), 0x40+(row<<2), value);
}

#if CONFIG_MAX_CPUS > 1
static u8 link_to_register(int ldt)
{
	/*
	 * [ 0: 3] Request Route
	 *     [0] Route to this node
	 *     [1] Route to Link 0
	 *     [2] Route to Link 1
	 *     [3] Route to Link 2
	 */

	if (ldt&0x08) return 0x40;
	if (ldt&0x04) return 0x20;
	if (ldt&0x02) return 0x00;
	
	/* we should never get here */
	print_spew("Unknown Link\n");
	return 0;
}

static u32 get_row(u8 node, u8 row)
{
	return pci_read_config32(NODE_HT(node), 0x40+(row<<2));
}

static int link_connection(u8 src, u8 dest)
{
	return get_row(src, dest) & 0x0f;
}

static void rename_temp_node(u8 node)
{
	uint32_t val;

	print_spew("Renaming current temporary node to ");
	print_spew_hex8(node);

	val=pci_read_config32(NODE_HT(7), 0x60);
	val &= (~7);  /* clear low bits. */
        val |= node;   /* new node        */
	pci_write_config32(NODE_HT(7), 0x60, val);

	print_spew(" done.\r\n");
}

static int check_connection(u8 dest)
{
	/* See if we have a valid connection to dest */
	u32 val;

	/* Verify that the coherent hypertransport link is
	 * established and actually working by reading the
	 * remode node's vendor/device id
	 */
        val = pci_read_config32(NODE_HT(dest),0);
	if(val != 0x11001022)
		return 0;

	return 1;
}

static unsigned read_freq_cap(device_t dev, unsigned pos)
{
	/* Handle bugs in valid hypertransport frequency reporting */
	unsigned freq_cap;
	uint32_t id;

	freq_cap = pci_read_config16(dev, pos);
	freq_cap &= ~(1 << HT_FREQ_VENDOR); /* Ignore Vendor HT frequencies */

	id = pci_read_config32(dev, 0);

	/* AMD 8131 Errata 48 */
	if (id == (PCI_VENDOR_ID_AMD | (PCI_DEVICE_ID_AMD_8131_PCIX << 16))) {
		freq_cap &= ~(1 << HT_FREQ_800Mhz);
	}
	/* AMD 8151 Errata 23 */
	if (id == (PCI_VENDOR_ID_AMD | (PCI_DEVICE_ID_AMD_8151_SYSCTRL << 16))) {
		freq_cap &= ~(1 << HT_FREQ_800Mhz);
	}
	/* AMD K8 Unsupported 1Ghz? */
	if (id == (PCI_VENDOR_ID_AMD | (0x1100 << 16))) {
		freq_cap &= ~(1 << HT_FREQ_1000Mhz);
	}
	return freq_cap;
}

static int optimize_connection(device_t node1, uint8_t link1, device_t node2, uint8_t link2)
{
	static const uint8_t link_width_to_pow2[]= { 3, 4, 0, 5, 1, 2, 0, 0 };
	static const uint8_t pow2_to_link_width[] = { 0x7, 4, 5, 0, 1, 3 };
	uint16_t freq_cap1, freq_cap2, freq_cap, freq_mask;
	uint8_t width_cap1, width_cap2, width_cap, width, old_width, ln_width1, ln_width2;
	uint8_t freq, old_freq;
	int needs_reset;
	/* Set link width and frequency */

	/* Initially assume everything is already optimized and I don't need a reset */
	needs_reset = 0;

	/* Get the frequency capabilities */
	freq_cap1 = read_freq_cap(node1, link1 + PCI_HT_CAP_HOST_FREQ_CAP);
	freq_cap2 = read_freq_cap(node2, link2 + PCI_HT_CAP_HOST_FREQ_CAP);

	/* Calculate the highest possible frequency */
	freq = log2(freq_cap1 & freq_cap2);

	/* See if I am changing the link freqency */
	old_freq = pci_read_config8(node1, link1 + PCI_HT_CAP_HOST_FREQ);
	needs_reset |= old_freq != freq;
	old_freq = pci_read_config8(node2, link2 + PCI_HT_CAP_HOST_FREQ);
	needs_reset |= old_freq != freq;

	/* Set the Calulcated link frequency */
	pci_write_config8(node1, link1 + PCI_HT_CAP_HOST_FREQ, freq);
	pci_write_config8(node2, link2 + PCI_HT_CAP_HOST_FREQ, freq);

	/* Get the width capabilities */
	width_cap1 = pci_read_config8(node1, link1 + PCI_HT_CAP_HOST_WIDTH);
	width_cap2 = pci_read_config8(node2, link2 + PCI_HT_CAP_HOST_WIDTH);

	/* Calculate node1's input width */
	ln_width1 = link_width_to_pow2[width_cap1 & 7];
	ln_width2 = link_width_to_pow2[(width_cap2 >> 4) & 7];
	if (ln_width1 > ln_width2) {
		ln_width1 = ln_width2;
	}
	width = pow2_to_link_width[ln_width1];
	/* Calculate node1's output width */
	ln_width1 = link_width_to_pow2[(width_cap1 >> 4) & 7];
	ln_width2 = link_width_to_pow2[width_cap2 & 7];
	if (ln_width1 > ln_width2) {
		ln_width1 = ln_width2;
	}
	width |= pow2_to_link_width[ln_width1] << 4;
	
	/* See if I am changing node1's width */
	old_width = pci_read_config8(node1, link1 + PCI_HT_CAP_HOST_WIDTH + 1);
	needs_reset |= old_width != width;

	/* Set node1's widths */
	pci_write_config8(node1, link1 + PCI_HT_CAP_HOST_WIDTH + 1, width);

	/* Calculate node2's width */
	width = ((width & 0x70) >> 4) | ((width & 0x7) << 4);

	/* See if I am changing node2's width */
	old_width = pci_read_config8(node2, link2 + PCI_HT_CAP_HOST_WIDTH + 1);
	needs_reset |= old_width != width;

	/* Set node2's widths */
	pci_write_config8(node2, link2 + PCI_HT_CAP_HOST_WIDTH + 1, width);

	return needs_reset;
}

static void setup_row_local(u8 source, u8 row) /* source will be 7 when it is for temp use*/
{
	unsigned linkn;
	uint32_t val;
	val = 1;
	for(linkn = 0; linkn<3; linkn++) { 
		unsigned regpos; 
		uint32_t reg;
		regpos = 0x98 + 0x20 * linkn;
		reg = pci_read_config32(NODE_HT(source), regpos);
		if ((reg & 0x17) != 3) continue; /* it is not conherent or not connected*/
		val |= 1<<(linkn+1); 
	}
	val <<= 16;
	val |= 0x0101;
	fill_row(source,row, val);
}

static void setup_row_direct_x(u8 temp, u8 source, u8 dest, u8 linkn)
{
	uint32_t val;
	uint32_t val_s;
	val = 1<<(linkn+1);
	val |= 1<<(linkn+1+8); /*for direct connect response route should equal to request table*/

	if(((source &1)!=(dest &1)) 
#if CROSS_BAR_47_56
		&& (source<4) && (dest<4) 
#endif
	){
		val |= (1<<16);
	} else {
		/*for CROSS_BAR_47_56  47, 74, 56, 65 should be here too*/
		val_s = get_row(temp, source);
		val |= ((val_s>>16) - (1<<(linkn+1)))<<16;
	}

	fill_row(temp,dest, val );
}

static void setup_row_direct(u8 source, u8 dest, u8 linkn){
	setup_row_direct_x(source, source, dest, linkn);
}

static void setup_remote_row_direct(u8 source, u8 dest, u8 linkn){
	setup_row_direct_x(7, source, dest, linkn);
}

static uint8_t get_linkn_first(uint8_t byte)
{
	if(byte & 0x02) { byte = 0; }
	else if(byte & 0x04) { byte = 1; }
	else if(byte & 0x08) { byte = 2; }
	return byte; 
}

static uint8_t get_linkn_last(uint8_t byte)
{
	if(byte & 0x02) { byte &= 0x0f; byte |= 0x00;  }
	if(byte & 0x04) { byte &= 0x0f; byte |= 0x10;  }
	if(byte & 0x08) { byte &= 0x0f; byte |= 0x20;  }
	return byte>>4; 
}

static uint8_t get_linkn_last_count(uint8_t byte)
{
	byte &= 0x3f;
	if(byte & 0x02) { byte &= 0xcf; byte |= 0x00; byte+=0x40; }
	if(byte & 0x04) { byte &= 0xcf; byte |= 0x10; byte+=0x40; }
	if(byte & 0x08) { byte &= 0xcf; byte |= 0x20; byte+=0x40; }
	return byte>>4;
}

static void setup_temp_row(u8 source, u8 dest)
{
	/* copy val from (source, dest) to (source,7) */
	fill_row(source,7,get_row(source,dest));
}

static void clear_temp_row(u8 source)
{
	fill_row(source, 7, DEFAULT);
}

static void setup_remote_node(u8 node)
{
	static const uint8_t pci_reg[] = { 
		0x44, 0x4c, 0x54, 0x5c, 0x64, 0x6c, 0x74, 0x7c, 
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78,
		0x84, 0x8c, 0x94, 0x9c, 0xa4, 0xac, 0xb4, 0xbc,
		0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8,
		0xc4, 0xcc, 0xd4, 0xdc,
		0xc0, 0xc8, 0xd0, 0xd8,
		0xe0, 0xe4, 0xe8, 0xec,
	};
	int i;

	print_spew("setup_remote_node: ");

	/* copy the default resource map from node 0 */
	for(i = 0; i < sizeof(pci_reg)/sizeof(pci_reg[0]); i++) {
		uint32_t value;
		uint8_t reg;
		reg = pci_reg[i];
		value = pci_read_config32(NODE_MP(0), reg);
		pci_write_config32(NODE_MP(7), reg, value);

	}
	print_spew("done\r\n");
}

#endif /* CONFIG_MAX_CPUS > 1*/


#if CONFIG_MAX_CPUS > 2
#if !CROSS_BAR_47_56
static void setup_row_indirect_x(u8 temp, u8 source, u8 dest, u8 gateway)
#else
static void setup_row_indirect_x(u8 temp, u8 source, u8 dest, u8 gateway, u8 diff)
#endif
{
	/*for indirect connection, we need to compute the val from val_s(source, source), and val_g(source, gateway) */
	uint32_t val_s;
	uint32_t val;
#if !CROSS_BAR_47_56	
	u8 diff;
#endif
	val_s = get_row(temp, source);
	val = get_row(temp, gateway);
	
	val &= 0xffff;
	val_s >>= 16;
	val_s &= 0xfe;
	
#if !CROSS_BAR_47_56
	diff = ((source&1)!=(dest &1));
#endif

	if(diff && (val_s!=(val&0xff)) ) { /* use another connect as response*/
		val_s -= val &  0xff;
#if CONFIG_MAX_CPUS > 4
		uint8_t byte;
		/* Some node have two links left
		 * don't worry we only have (2, (3 as source need to handle
		 */
		byte = val_s;
		byte = get_linkn_last_count(byte);
		if((byte>>2)>1) { /* make sure not the corner*/
			if(source<dest) {
				val_s-=link_connection(temp, source-2); /* -down*/
			} else {
				val_s-=link_connection(temp, source+2); /* -up*/
			}
		}
#endif
		val &= 0xff;
		val |= (val_s<<8);
	} 

	if(diff) { /* cross rung?*/
		val |= (1<<16);
	}
	else {
		val_s = get_row(temp, source);
		val |= ((val_s>>16) - link_connection(temp, gateway))<<16; 
	}

	fill_row(temp, dest, val);

}

#if !CROSS_BAR_47_56
static void setup_row_indirect(u8 source, u8 dest, u8 gateway)
{	
	setup_row_indirect_x(source, source, dest, gateway);
}
#else           
static void setup_row_indirect(u8 source, u8 dest, u8 gateway, u8 diff)
{
	setup_row_indirect_x(source, source, dest, gateway, diff);
}
#endif   

static void setup_row_indirect_group(const u8 *conn, int num)
{
	int i;

#if !CROSS_BAR_47_56
	for(i=0; i<num; i+=3) {
		setup_row_indirect(conn[i], conn[i+1],conn[i+2]);
#else
	for(i=0; i<num; i+=4) {
		setup_row_indirect(conn[i], conn[i+1],conn[i+2], conn[i+3]);
#endif

	}
}

#if !CROSS_BAR_47_56
static void setup_remote_row_indirect(u8 source, u8 dest, u8 gateway)
{
	setup_row_indirect_x(7, source, dest, gateway);
}
#else
static void setup_remote_row_indirect(u8 source, u8 dest, u8 gateway, u8 diff)
{
	setup_row_indirect_x(7, source, dest, gateway, diff);
}
#endif

static void setup_remote_row_indirect_group(const u8 *conn, int num)
{
	int i;

#if !CROSS_BAR_47_56
	for(i=0; i<num; i+=3) {
		setup_remote_row_indirect(conn[i], conn[i+1],conn[i+2]);
#else
	for(i=0; i<num; i+=4) {
		setup_remote_row_indirect(conn[i], conn[i+1],conn[i+2], conn[i+3]);
#endif
	}
}

#endif /*CONFIG_MAX_CPUS > 2*/


static void setup_uniprocessor(void)
{
	print_spew("Enabling UP settings\r\n");
	disable_probes();
}

struct setup_smp_result {
	int nodes;
	int needs_reset;
};

#if CONFIG_MAX_CPUS > 2
static int optimize_connection_group(const u8 *opt_conn, int num) {
	int needs_reset = 0;
	int i;
	for(i=0; i<num; i+=2) {
		needs_reset = optimize_connection(
			NODE_HT(opt_conn[i]), 0x80 + link_to_register(link_connection(opt_conn[i],opt_conn[i+1])),
			NODE_HT(opt_conn[i+1]), 0x80 + link_to_register(link_connection(opt_conn[i+1],opt_conn[i])) );
	}               
	return needs_reset;
}  
#endif

#if CONFIG_MAX_CPUS > 1
static struct setup_smp_result setup_smp2(void)
{
	struct setup_smp_result result;
	u8 byte;
	uint32_t val;
	result.nodes = 2;
	result.needs_reset = 0;

	setup_row_local(0, 0); /* it will update the broadcast RT*/
	
	val = get_row(0,0);
	byte = (val>>16) & 0xfe;
	if(byte<0x2) { /* no coherent connection so get out.*/
		result.nodes = 1;
		return result;
	}

	/* Setup and check a temporary connection to node 1 */
#if TRY_HIGH_FIRST == 1
	byte = get_linkn_last(byte); /* Max Link to node1 */
#else
	byte = get_linkn_first(byte); /*Min Link to node1 --- according to AMD*/
#endif
	print_linkn("(0,1) byte=", byte);
	setup_row_direct(0,1, byte);
	setup_temp_row(0, 1);
	
	if (!check_connection(7)) {
		print_spew("No connection to Node 1.\r\n");
		result.nodes = 1;
		return result;
	}

	/* We found 2 nodes so far */
	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /*get default link on node7 to node0*/
	print_linkn("(1,0) byte=", byte);
	setup_row_local(7,1);
	setup_remote_row_direct(1, 0, byte);

#if CONFIG_MAX_CPUS > 4    
	val = get_row(7,1);
	byte = (val>>16) & 0xfe;
	byte = get_linkn_last_count(byte);
	if((byte>>2)==3) { /* Oh! we need to treat it as node2. So use another link*/
		val = get_row(0,0);
		byte = (val>>16) & 0xfe;
#if TRY_HIGH_FIRST == 1
		byte = get_linkn_first(byte); /* Min link to Node1 */
#else
		byte = get_linkn_last(byte);  /* Max link to Node1*/
#endif
		print_linkn("-->(0,1) byte=", byte);
		setup_row_direct(0,1, byte);
		setup_temp_row(0, 1);
	
		if (!check_connection(7)) {
			print_spew("No connection to Node 1.\r\n");
			result.nodes = 1;
			return result;
		}               
			
		/* We found 2 nodes so far */
		val = pci_read_config32(NODE_HT(7), 0x6c);
		byte = (val>>2) & 0x3; /* get default link on node7 to node0*/
		print_linkn("-->(1,0) byte=", byte); 
		setup_row_local(7,1);
		setup_remote_row_direct(1, 0, byte);
	}
#endif
	
	setup_remote_node(1);  /* Setup the regs on the remote node */
	rename_temp_node(1);    /* Rename Node 7 to Node 1  */
	enable_routing(1);      /* Enable routing on Node 1 */
#if 0
	/*don't need and it is done by clear_dead_links */
	clear_temp_row(0);
#endif
	
	result.needs_reset = optimize_connection(
		NODE_HT(0), 0x80 + link_to_register(link_connection(0,1)),
		NODE_HT(1), 0x80 + link_to_register(link_connection(1,0)) );


	return result;
}
#endif /*CONFIG_MAX_CPUS > 1 */

#if CONFIG_MAX_CPUS > 2

static struct setup_smp_result setup_smp4(int needs_reset)
{
	struct setup_smp_result result;
	u8 byte;
	uint32_t val;

	result.nodes=4;
	result.needs_reset = needs_reset;

	/* Setup and check temporary connection from Node 0 to Node 2 */
	val = get_row(0,0);
	byte = ((val>>16) & 0xfe) - link_connection(0,1);
	byte = get_linkn_last_count(byte);

	if((byte>>2)==0) { /* We should have two coherent for 4p and above*/
		result.nodes = 2;
		return result;
	}

	byte &= 3; /* bit [3,2] is count-1*/
	print_linkn("(0,2) byte=", byte); 
	setup_row_direct(0, 2, byte);  /*(0,2) direct link done*/
	setup_temp_row(0, 2);

	if (!check_connection(7) ) {
		print_spew("No connection to Node 2.\r\n");
		result.nodes = 2;
		return result;
	}

	/* We found 3 nodes so far. Now setup a temporary
	* connection from node 0 to node 3 via node 1
	*/
	setup_temp_row(0,1); /* temp. link between nodes 0 and 1 */
	/* here should setup_row_direct(1,3) at first, before that we should find the link in node 1 to 3*/
	val = get_row(1,1);
	byte = ((val>>16) & 0xfe) - link_connection(1,0);
	byte = get_linkn_first(byte);
	print_linkn("(1,3) byte=", byte); 
	setup_row_direct(1,3,byte);  /* (1, 3) direct link done*/
	setup_temp_row(1,3); /* temp. link between nodes 1 and 3 */
	
	if (!check_connection(7)) {
		print_spew("No connection to Node 3.\r\n");
		result.nodes = 2;
		return result;
	}

	/* We found 4 nodes so far. Now setup all nodes for 4p */
#if !CROSS_BAR_47_56
	static const u8 conn4_1[] = {
		0,3,2,
		1,2,3,
	};	
#else
	static const u8 conn4_1[] = {
		0,3,2,1,
		1,2,3,1,
	};
#endif

	setup_row_indirect_group(conn4_1, sizeof(conn4_1)/sizeof(conn4_1[0]));

	setup_temp_row(0,2);

	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 0*/
	print_linkn("(2,0) byte=", byte); 

	setup_row_local(7,2);
	setup_remote_row_direct(2, 0, byte);
	setup_remote_node(2);  /* Setup the regs on the remote node */
#if !CROSS_BAR_47_56
	static const u8 conn4_2[] = {
		2,1,0,
	};
#else   
	static const u8 conn4_2[] = {
		2,1,0,1, 
	};      
#endif          
	setup_remote_row_indirect_group(conn4_2, sizeof(conn4_2)/sizeof(conn4_2[0]));

	rename_temp_node(2);    /* Rename Node 7 to Node 2  */
	enable_routing(2);      /* Enable routing on Node 2 */

	setup_temp_row(0,1);
	setup_temp_row(1,3);

	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 1*/
	print_linkn("(3,1) byte=", byte); 

	setup_row_local(7,3);
	setup_remote_row_direct(3, 1, byte);
	setup_remote_node(3);  /* Setup the regs on the remote node */

#if !CROSS_BAR_47_56
	static const u8 conn4_3[] = {
		3,0,1,
	};
#else   
	static const u8 conn4_3[] = {
		3,0,1,1,
	};      
#endif          
	setup_remote_row_indirect_group(conn4_3, sizeof(conn4_3)/sizeof(conn4_3[0]));

	/* We need to init link between 2, and 3 direct link */
	val = get_row(2,2);
	byte = ((val>>16) & 0xfe) - link_connection(2,0);
	byte = get_linkn_last_count(byte);
	print_linkn("(2,3) byte=", byte & 3);
	
	setup_row_direct(2,3, byte & 0x3);
	setup_temp_row(0,2);
	setup_temp_row(2,3);
	check_connection(7); /* to 3*/

#if CONFIG_MAX_CPUS > 4
	/* We need to find out which link is to node3 */
	
	if((byte>>2)==2) { /* one to node3, one to node0, one to node4*/
		val = get_row(7,3);
		if((val>>16) == 1) { /* that link is to node4, because via node3 it has been set, recompute it*/
			val = get_row(2,2);
			byte = ((val>>16) & 0xfe) - link_connection(2,0);
			byte = get_linkn_first(byte);
			print_linkn("-->(2,3) byte=", byte); 
			setup_row_direct(2,3,byte);
			setup_temp_row(2,3);
			check_connection(7); /* to 3*/
		}
	} 
#endif

	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 2*/
	print_linkn("(3,2) byte=", byte); 
	setup_remote_row_direct(3,2, byte);

/* ready to enable RT for Node 3 */
	rename_temp_node(3);
	enable_routing(3);      /* enable routing on node 3 (temp.) */

#if 0
	/*We need to do sth to reverse work for setup_temp_row (0,1) (1,3) */
	/* it will be done by clear_dead_links */
	clear_temp_row(0);
	clear_temp_row(1);
#endif

	/* optimize physical connections - by LYH */
	static const u8 opt_conn4[] = {
		0,2,
		1,3,
		2,3,
	};

	result.needs_reset = optimize_connection_group(opt_conn4, sizeof(opt_conn4)/sizeof(opt_conn4[0]));
	
	return result;

}

#endif /* CONFIG_MAX_CPUS > 2 */

#if CONFIG_MAX_CPUS > 4

static struct setup_smp_result setup_smp6(int needs_reset)
{
	struct setup_smp_result result;
	u8 byte;
	uint32_t val;

	result.nodes=6;
	result.needs_reset = needs_reset;

	/* Setup and check temporary connection from Node 0 to Node 4 via 2 */
	val = get_row(2,2);
	byte = ((val>>16) & 0xfe) - link_connection(2,3) - link_connection(2,0);
	byte = get_linkn_last_count(byte);

	if((byte>>2)==0) { /* We should have two coherent link on node 2 for 6p and above*/
		result.nodes = 4;
		return result;
	}
	byte &= 3; /* bit [3,2] is count-2*/
	print_linkn("(2,4) byte=", byte);
	setup_row_direct(2, 4, byte);
	
	/* Setup and check temporary connection from Node 0 to Node 4  through 2*/
	for(byte=0; byte<4; byte+=2) {
		setup_temp_row(byte,byte+2); 
	}

	if (!check_connection(7) ) {
		print_spew("No connection to Node 4.\r\n");
		result.nodes = 4;
		return result; 
	}       

	/* Setup and check temporary connection from Node 0 to Node 5  through 1, 3*/
	val = get_row(3,3);
	byte = ((val>>16) & 0xfe) - link_connection(3,2) - link_connection(3,1);
	byte = get_linkn_last_count(byte);
	if((byte>>2)==0) { /* We should have two coherent links on node 3 for 6p and above*/
		result.nodes = 4;
		return result;
	}

	byte &= 3; /*bit [3,2] is count-2*/
	print_linkn("(3,5) byte=", byte);
	setup_row_direct(3, 5, byte);

	setup_temp_row(0,1); /* temp. link between nodes 0 and 1 */
	for(byte=0; byte<4; byte+=2) {
		setup_temp_row(byte+1,byte+3); 
	}
	
	if (!check_connection(7)) {
		print_spew("No connection to Node 5.\r\n");
		result.nodes = 4;
		return result; 
	}       
	
	/* We found 6 nodes so far. Now setup all nodes for 6p */
#warning "FIXME we need to find out the correct gateway for 6p"	
	static const u8 conn6_1[] = {
#if !CROSS_BAR_47_56
		0, 4, 2,
		0, 5, 1,
		1, 4, 3,
		1, 5, 3,
		2, 5, 3,
		3, 4, 5,
#else
		0, 4, 2, 0,
		0, 5, 1, 1,
		1, 4, 3, 1,
		1, 5, 3, 0,
		2, 5, 3, 0,
		3, 4, 2, 0,
#endif
	}; 

	setup_row_indirect_group(conn6_1, sizeof(conn6_1)/sizeof(conn6_1[0]));
	
	for(byte=0; byte<4; byte+=2) {
		setup_temp_row(byte,byte+2);
	}
	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /*get default link on 7 to 2*/
	print_linkn("(4,2) byte=", byte); 
	
	setup_row_local(7,4);
	setup_remote_row_direct(4, 2, byte);
	setup_remote_node(4);  /* Setup the regs on the remote node */
	/* Set indirect connection to 0, to 3 */
	static const u8 conn6_2[] = {
#if !CROSS_BAR_47_56
		4, 0, 2,
		4, 1, 2,
		4, 3, 2,
#else
		4, 0, 2, 0,
		4, 1, 2, 0,
		4, 3, 2, 0,
		4, 5, 2, 0,
#endif
	};      
	
	setup_remote_row_indirect_group(conn6_2, sizeof(conn6_2)/sizeof(conn6_2[0]));
	
	rename_temp_node(4);
	enable_routing(4);

	setup_temp_row(0,1);
	for(byte=0; byte<4; byte+=2) {
		setup_temp_row(byte+1,byte+3);
	}

	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 3*/
	print_linkn("(5,3) byte=", byte); 
	setup_row_local(7,5);
	setup_remote_row_direct(5, 3, byte);
	setup_remote_node(5);  /* Setup the regs on the remote node */
	
#if !CROSS_BAR_47_56
	/* We need to init link between 4, and 5 direct link */
	val = get_row(4,4);
	byte = ((val>>16) & 0xfe) - link_connection(4,2);
	byte = get_linkn_last_count(byte);
	print_linkn("(4,5) byte=", byte & 3);
	
	setup_row_direct(4,5, byte & 0x3);
	setup_temp_row(0,2);
	setup_temp_row(2,4);
	setup_temp_row(4,5);
	check_connection(7); /* to 5*/

#if CONFIG_MAX_CPUS > 6
	/* We need to find out which link is to node5 */
	
	if((byte>>2)==2) { /* one to node5, one to node2, one to node6*/
		val = get_row(7,5);
		if((val>>16) == 1) { /* that link is to node6, because via node 3 node 5 has been set*/
			val = get_row(4,4);
			byte = ((val>>16) & 0xfe) - link_connection(4,2);
			byte = get_linkn_first(byte);
			print_linkn("-->(4,5) byte=", byte);
			setup_row_direct(4,5,byte);
			setup_temp_row(4,5);
			check_connection(7); /* to 5*/
		}
	} 
#endif

	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 4*/
	print_linkn("(5,4) byte=", byte); 
	setup_remote_row_direct(5,4, byte);
#endif	
	
	/* Set indirect connection to 0, to 3  for indirect we will use clockwise routing */
	static const u8 conn6_3[] = {
#if !CROSS_BAR_47_56
		5, 0, 4,
		5, 2, 4,
		5, 1, 3,
#else
		5, 0, 3, 0,
		5, 2, 3, 0,
		5, 1, 3, 0,
		5, 4, 3, 0,
#endif
	};      
	
	setup_remote_row_indirect_group(conn6_3, sizeof(conn6_3)/sizeof(conn6_3[0]));

/* ready to enable RT for 5 */
	rename_temp_node(5);
	enable_routing(5);      /* enable routing on node 5 (temp.) */

#if 0
	/* We need to do sth about reverse about setup_temp_row (0,1), (2,4), (1, 3), (3,5)
	 * It will be done by clear_dead_links 
	 */
	for(byte=0; byte<4; byte++) {
		clear_temp_row(byte);
	}
#endif

	/* optimize physical connections - by LYH */
	static const uint8_t opt_conn6[] ={
		2, 4,
		3, 5,
#if !CROSS_BAR_47_56
		4, 5,
#endif
	}; 
	result.needs_reset = optimize_connection_group(opt_conn6, sizeof(opt_conn6)/sizeof(opt_conn6[0]));

	return result;

}

#endif /* CONFIG_MAX_CPUS > 4 */

#if CONFIG_MAX_CPUS > 6

static struct setup_smp_result setup_smp8(int needs_reset)
{
	struct setup_smp_result result;
	u8 byte;
	uint32_t val;

	result.nodes=8;
	result.needs_reset = needs_reset;

	/* Setup and check temporary connection from Node 0 to Node 6 via 2 and 4 to 7 */
	val = get_row(4,4);
#if !CROSS_BAR_47_56
	byte = ((val>>16) & 0xfe) - link_connection(4,5) - link_connection(4,2);
#else
	byte = ((val>>16) & 0xfe) - link_connection(4,2);
#endif

#if TRY_HIGH_FIRST == 1
	byte = get_linkn_last_count(byte); /* Max link to 6*/
	if((byte>>2)==0) { /* We should have two or three coherent links on node 4 for 8p*/
		result.nodes = 6;
		return result;
	}
	byte &= 3; /* bit [3,2] is count-1 or 2*/
#else
	byte = get_linkn_first(byte);  /*Min link to 6*/
#endif
	print_linkn("(4,6) byte=", byte);
	setup_row_direct(4, 6, byte);

	/* Setup and check temporary connection from Node 0 to Node 6  through 2, and 4*/
	for(byte=0; byte<6; byte+=2) {
		setup_temp_row(byte,byte+2); 
	}

	if (!check_connection(7) ) {
		print_spew("No connection to Node 6.\r\n");
		result.nodes = 6;
		return result;
	}
#if !CROSS_BAR_47_56
	/* Setup and check temporary connection from Node 0 to Node 7  through 1, 3, 5*/
	val = get_row(5,5);
	byte = ((val>>16) & 0xfe) - link_connection(5,4) - link_connection(5,3);
	byte = get_linkn_first(byte);
	print_linkn("(5,7) byte=", byte);
	setup_row_direct(5, 7, byte);

	setup_temp_row(0,1); /* temp. link between nodes 0 and 1 */
	for(byte=0; byte<6; byte+=2) {
		setup_temp_row(byte+1,byte+3); 
	}
#else
	val = get_row(4,4);
	byte = ((val>>16) & 0xfe) - link_connection(4,2) - link_connection(4,6);
	byte = get_linkn_first(byte); 
	print_linkn("(4,7) byte=", byte); 
	setup_row_direct(4, 7, byte);

	/* Setup and check temporary connection from Node 0 to Node 7 through 2, and 4*/
	for(byte=0; byte<4; byte+=2) {
		setup_temp_row(byte,byte+2); 
	}
	setup_temp_row(4, 7);

#endif

	if (!check_connection(7)) {
		print_spew("No connection to Node 7.\r\n");
		result.nodes = 6;
		return result;
	}


	/* We found 8 nodes so far. Now setup all nodes for 8p */
	static const u8 conn8_1[] = {
#if !CROSS_BAR_47_56
		0, 6, 2,
		/*0, 7, 1,*/
		1, 6, 3,
		/*1, 7, 3,*/
		2, 6, 4,
		/*2, 7, 3,*/
		3, 6, 5,
		/*3, 7, 5,*/
		/*4, 7, 5,*/
#else
		0, 6, 2, 0,
		/*0, 7, 2, 0,*/
		1, 6, 3, 0,
		/*1, 7, 3, 0,*/
		2, 6, 4, 0,
		/*2, 7, 4, 0,*/
		3, 6, 5, 0,
		/*3, 7, 5, 0,*/
#endif
	};

	setup_row_indirect_group(conn8_1,sizeof(conn8_1)/sizeof(conn8_1[0]));

	for(byte=0; byte<6; byte+=2) {
		setup_temp_row(byte,byte+2);
	}
	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 4*/
	print_linkn("(6,4) byte=", byte);
	
	setup_row_local(7,6);
	setup_remote_row_direct(6, 4, byte);
	setup_remote_node(6);  /* Setup the regs on the remote node */
	/* Set indirect connection to 0, to 3   */
#warning "FIXME we need to find out the correct gateway for 8p"		
	static const u8 conn8_2[] = {
#if !CROSS_BAR_47_56
		6, 0, 4,
		6, 1, 4,
		6, 2, 4,
		6, 3, 4,
		6, 5, 4,
#else
		6, 0, 4, 0,
		/*6, 1, 5, 0,*/
		6, 2, 4, 0,
		/*6, 3, 5, 0,*/
#endif
	};

	setup_remote_row_indirect_group(conn8_2, sizeof(conn8_2)/sizeof(conn8_2[0]));
	
	rename_temp_node(6);
	enable_routing(6);

#if !CROSS_BAR_47_56
	setup_temp_row(0,1);
	for(byte=0; byte<6; byte+=2) {
		setup_temp_row(byte+1,byte+3);
	}

	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 5*/
	print_linkn("(7,5) byte=", byte); 
	setup_row_local(7,7);
	setup_remote_row_direct(7, 5, byte);

#else
	for(byte=0; byte<4; byte+=2) {
		setup_temp_row(byte,byte+2);
	}
	setup_temp_row(4,7);
	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 4*/
	print_linkn("(7,4) byte=", byte); 
	setup_row_local(7,7);
	setup_remote_row_direct(7, 4, byte);
	/* till now 4-7, 7-4 done. */
#endif
	setup_remote_node(7);  /* Setup the regs on the remote node */

#if CROSS_BAR_47_56
	/* here init 5, 7 */
	/* Setup and check temporary connection from Node 0 to Node 5  through 1, 3, 5*/
	val = get_row(5,5);
	byte = ((val>>16) & 0xfe) - link_connection(5,3);
	byte = get_linkn_last(byte);
	print_linkn("(5,7) byte=", byte); 
	setup_row_direct(5, 7, byte);
	
	setup_temp_row(0,1); /* temp. link between nodes 0 and 1 */
	for(byte=0; byte<6; byte+=2) {
		setup_temp_row(byte+1,byte+3); 
	}

	if (!check_connection(7)) {
		/* We need to recompute link to 7 */
		val = get_row(5,5);
		byte = ((val>>16) & 0xfe) - link_connection(5,3);
		byte = get_linkn_first(byte);

		print_linkn("-->(5,7) byte=", byte);
		setup_row_direct(5, 7, byte);
#if 0
		setup_temp_row(0,1); /* temp. link between nodes 0 and 1 */
		for(byte=0; byte<6; byte+=2) {
			setup_temp_row(byte+1,byte+3); 
		}
#else
		setup_temp_row(5,7);
#endif
		check_connection(7);
	}
	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 5*/
	print_linkn("(7,5) byte=", byte);
	setup_remote_row_direct(7, 5, byte);
	/*Till now 57, 75 done */
	
	/* init init 5, 6 */
	val = get_row(5,5);
	byte = ((val>>16) & 0xfe) - link_connection(5,3) - link_connection(5,7);
	byte = get_linkn_first(byte);
	print_linkn("(5,6) byte=", byte);
	setup_row_direct(5, 6, byte);

	/* init 6,7 */
	val = get_row(6,6);
	byte = ((val>>16) & 0xfe) - link_connection(6,4);
	byte = get_linkn_last(byte);
	print_linkn("(6,7) byte=", byte);
	setup_row_direct(6, 7, byte);

	for(byte=0; byte<6; byte+=2) {
		setup_temp_row(byte,byte+2); 
	}
	setup_temp_row(6,7);

	if (!check_connection(7)) {
		/* We need to recompute link to 7 */
		val = get_row(6,6);
		byte = ((val>>16) & 0xfe) - link_connection(6,4);
		byte = get_linkn_first(byte);
		print_linkn("-->(6,7) byte=", byte);

		setup_row_direct(6, 7, byte);
#if 0
		for(byte=0; byte<6; byte+=2) {
			setup_temp_row(byte,byte+2); /* temp. link between nodes 0 and 2 */
		}
#endif
		setup_temp_row(6,7);
		check_connection(7);
	}
	val = pci_read_config32(NODE_HT(7), 0x6c);
	byte = (val>>2) & 0x3; /* get default link on 7 to 6*/
	print_linkn("(7,6) byte=", byte);

	setup_remote_row_direct(7, 6, byte);
	/* Till now 67, 76 done*/

	/* init 6,5 */
	val = get_row(6,6);
	byte = ((val>>16) & 0xfe) - link_connection(6,4) - link_connection(6,7);
	byte = get_linkn_first(byte);
	print_linkn("(6,5) byte=", byte);
	setup_row_direct(6, 5, byte);

#endif

#if !CROSS_BAR_47_56
	/* We need to init link between 6, and 7 direct link */
	val = get_row(6,6);
	byte = ((val>>16) & 0xfe) - link_connection(6,4);
	byte = get_linkn_first(byte);
	print_linkn("(6,7) byte=", byte);
	setup_row_direct(6,7, byte);

	val = get_row(7,7);
	byte = ((val>>16) & 0xfe) - link_connection(7,5);
	byte = get_linkn_first(byte);
	print_linkn("(7,6) byte=", byte);
	setup_row_direct(7,6, byte);
#endif

	/* Set indirect connection to 0, to 3  for indirect we will use clockwise routing */
	static const u8 conn8_3[] = {
#if !CROSS_BAR_47_56
		0, 7, 1,  /* restore it*/
		1, 7, 3,
		2, 7, 3,
		3, 7, 5,
		4, 7, 5,

		7, 0, 6,
		7, 1, 5,
		7, 2, 6,
		7, 3, 5,
		7, 4, 6,
#else
		0, 7, 2, 0, /* restore it*/
		1, 7, 3, 0,
		2, 7, 4, 0,
		3, 7, 5, 0,
		
		6, 1, 5, 0, /*???*/
		6, 3, 5, 0, /*???*/
		
		7, 0, 4, 0,
		7, 1, 5, 0,
		7, 2, 4, 0,
		7, 3, 5, 0,
		4, 5, 7, 0,
		5, 4, 6, 0,
#endif
	};

	setup_row_indirect_group(conn8_3, sizeof(conn8_3)/sizeof(conn8_3[0]));
	
/* ready to enable RT for Node 7 */
	enable_routing(7);      /* enable routing on node 7 (temp.) */
	

	static const uint8_t opt_conn8[] ={
		4, 6,
#if CROSS_BAR_47_56
		4, 7,
		5, 6,
#endif
		5, 7,
		6, 7,
	};
	/* optimize physical connections - by LYH */
	result.needs_reset = optimize_connection_group(opt_conn8, sizeof(opt_conn8)/sizeof(opt_conn8[0]));

	return result;
}

#endif /* CONFIG_MAX_CPUS > 6 */


#if CONFIG_MAX_CPUS > 1

static struct setup_smp_result setup_smp(void)
{
	struct setup_smp_result result;

	print_spew("Enabling SMP settings\r\n");
		
	result = setup_smp2();
#if CONFIG_MAX_CPUS > 2
	result = setup_smp4(result.needs_reset);
#endif
	
#if CONFIG_MAX_CPUS > 4
	result = setup_smp6(result.needs_reset);
#endif

#if CONFIG_MAX_CPUS > 6
	result = setup_smp6(result.needs_reset);
#endif

	print_debug_hex8(result.nodes);
	print_debug(" nodes initialized.\r\n");
	
	return result;

}

static unsigned verify_mp_capabilities(unsigned nodes)
{
	unsigned node, mask;
	
	mask = 0x06; /* BigMPCap */

	for (node=0; node<nodes; node++) {
		mask &= pci_read_config32(NODE_MC(node), 0xe8);
	}
	
	switch(mask) {
#if CONFIG_MAX_CPUS > 2
	case 0x02: /* MPCap    */
		if(nodes > 2) {
			print_err("Going back to DP\r\n");
			return 2;
		}
		break;
#endif
	case 0x00: /* Non SMP */
		if(nodes >1 ) {
			print_err("Going back to UP\r\n");
			return 1;
		}
		break;
	}
	
	return nodes;

}


static void clear_dead_routes(unsigned nodes)
{
	int last_row;
	int node, row;
#if CONFIG_MAX_CPUS > 6
	if(nodes==8) return;/* don't touch (7,7)*/
#endif
	last_row = nodes;
	if (nodes == 1) {
		last_row = 0;
	}
	for(node = 7; node >= 0; node--) {
		for(row = 7; row >= last_row; row--) {
			fill_row(node, row, DEFAULT);
		}
	}
	
	/* Update the local row */
	for( node=0; node<nodes; node++) {
		uint32_t val = 0;
		for(row =0; row<nodes; row++) {
			val |= get_row(node, row);
		}
		fill_row(node, row, (((val & 0xff) | ((val >> 8) & 0xff)) << 16) | 0x0101); 
	}
}
#endif /* CONFIG_MAX_CPUS > 1 */

static void coherent_ht_finalize(unsigned nodes)
{
	unsigned node;
	int rev_a0;
	
	/* set up cpu count and node count and enable Limit
	* Config Space Range for all available CPUs.
	* Also clear non coherent hypertransport bus range
	* registers on Hammer A0 revision.
	*/

	print_spew("coherent_ht_finalize\r\n");
	rev_a0 = is_cpu_rev_a0();
	for (node = 0; node < nodes; node++) {
		device_t dev;
		uint32_t val;
		dev = NODE_HT(node);

		/* Set the Total CPU and Node count in the system */
		val = pci_read_config32(dev, 0x60);
		val &= (~0x000F0070);
		val |= ((nodes-1)<<16)|((nodes-1)<<4);
		pci_write_config32(dev, 0x60, val);

		/* Only respond to real cpu pci configuration cycles
		* and optimize the HT settings 
		*/
		val=pci_read_config32(dev, 0x68);
		val &= ~((HTTC_BUF_REL_PRI_MASK << HTTC_BUF_REL_PRI_SHIFT) |
			(HTTC_MED_PRI_BYP_CNT_MASK << HTTC_MED_PRI_BYP_CNT_SHIFT) |
			(HTTC_HI_PRI_BYP_CNT_MASK << HTTC_HI_PRI_BYP_CNT_SHIFT));
		val |= HTTC_LIMIT_CLDT_CFG | 
			(HTTC_BUF_REL_PRI_8 << HTTC_BUF_REL_PRI_SHIFT) |
			HTTC_RSP_PASS_PW |
			(3 << HTTC_MED_PRI_BYP_CNT_SHIFT) |
			(3 << HTTC_HI_PRI_BYP_CNT_SHIFT);
		pci_write_config32(dev, 0x68, val);

		if (rev_a0) {
			print_spew("shit it is an old cup\n");
			pci_write_config32(dev, 0x94, 0);
			pci_write_config32(dev, 0xb4, 0);
			pci_write_config32(dev, 0xd4, 0);
		}
	}

	print_spew("done\r\n");
}

static int apply_cpu_errata_fixes(unsigned nodes, int needs_reset)
{
	unsigned node;
	for(node = 0; node < nodes; node++) {
		device_t dev;
		uint32_t cmd;
		dev = NODE_MC(node);
		if (is_cpu_pre_c0()) {

			/* Errata 66
			* Limit the number of downstream posted requests to 1 
			*/
			cmd = pci_read_config32(dev, 0x70);
			if ((cmd & (3 << 0)) != 2) {
				cmd &= ~(3<<0);
				cmd |= (2<<0);
				pci_write_config32(dev, 0x70, cmd );
				needs_reset = 1;
			}
			cmd = pci_read_config32(dev, 0x7c);
			if ((cmd & (3 << 4)) != 0) {
				cmd &= ~(3<<4);
				cmd |= (0<<4);
				pci_write_config32(dev, 0x7c, cmd );
				needs_reset = 1;
			}
			/* Clock Power/Timing Low */
			cmd = pci_read_config32(dev, 0xd4);
			if (cmd != 0x000D0001) {
				cmd = 0x000D0001;
				pci_write_config32(dev, 0xd4, cmd);
				needs_reset = 1; /* Needed? */
			}

		}
		else {
			uint32_t cmd_ref;
			/* Errata 98 
			* Set Clk Ramp Hystersis to 7
			* Clock Power/Timing Low
			*/
			cmd_ref = 0x04e20707; /* Registered */
			cmd = pci_read_config32(dev, 0xd4);
			if(cmd != cmd_ref) {
				pci_write_config32(dev, 0xd4, cmd_ref );
				needs_reset = 1; /* Needed? */
			}
		}
	}
	return needs_reset;
}

static int optimize_link_read_pointers(unsigned nodes, int needs_reset)
{
	unsigned node;
	for(node = 0; node < nodes; node++) {
		device_t f0_dev, f3_dev;
		uint32_t cmd_ref, cmd;
		int link;
		f0_dev = NODE_HT(node);
		f3_dev = NODE_MC(node);
		cmd_ref = cmd = pci_read_config32(f3_dev, 0xdc);
		for(link = 0; link < 3; link++) {
			uint32_t link_type;
			unsigned reg;
			/* This works on an Athlon64 because unimplemented links return 0 */
			reg = 0x98 + (link * 0x20);
			link_type = pci_read_config32(f0_dev, reg);
			if ((link_type & 3) == 3) { 
				cmd &= ~(0xff << (link *8));
				cmd |= 0x25 << (link *8);
			}
		}
		if (cmd != cmd_ref) {
			pci_write_config32(f3_dev, 0xdc, cmd);
			needs_reset = 1;
		}
	}
	return needs_reset;
}

static int setup_coherent_ht_domain(void)
{
	struct setup_smp_result result;

	enable_bsp_routing();

#if CONFIG_MAX_CPUS > 1
	result = setup_smp();
	result.nodes = verify_mp_capabilities(result.nodes);
	clear_dead_routes(result.nodes);
#else
	result.nodes = 1;
	result.needs_reset = 0;
#endif

	if (result.nodes == 1) {
		setup_uniprocessor();
	} 
	coherent_ht_finalize(result.nodes);
	result.needs_reset = apply_cpu_errata_fixes(result.nodes, result.needs_reset);
	result.needs_reset = optimize_link_read_pointers(result.nodes, result.needs_reset);
	return result.needs_reset;
}
